/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "water_mark_filter.h"
#include "opengl_filter_internal.h"
#include "filter/filter_factory.h"
#include "common/media_core.h"
#include "avcodec_log.h"
#include "external_window.h"
#include "native_window.h"
#include "native_image.h"
#include "avcodec_trace.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer.h"
#include "utils.h"
#include "surface/native_buffer.h"
#include <unistd.h>

using OHOS::Media::Pipeline::GlInternal::ScopedFboBind;
using OHOS::Media::Pipeline::GlInternal::SetTextureParams2D;
using OHOS::Media::Pipeline::GlInternal::QUAD_INDICES;
using OHOS::Media::Pipeline::GlInternal::SHARED_VERTEX_SHADER;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "WaterMarkFilter" };

constexpr int32_t STRIDE_ALIGNMENT = 8;
constexpr int32_t RGBA_BYTES_PER_PIXEL = 4;
}

namespace OHOS {
namespace Media {
namespace Pipeline {

const uint32_t MAX_WATERMARK_NUMBER = 5;
const uint64_t MAX_WATERMARK_SIZE = 80 * 1024 * 1024;
const uint32_t ROTATE_90 = 90;
const uint32_t ROTATE_180 = 180;
const uint32_t ROTATE_270 = 270;
const uint32_t MIN_VERTICES_SIZE = 16;

/**
 * @brief 水印叠加片段着色器：仅采样水印纹理，通过GL_BLEND硬件Alpha混合叠加到视频帧上
 * 与 视频直通 配合使用，避免在水印区域外执行双纹理采样+Alpha混合计算
 */
static std::string watermarkOverlayFragShader = R"delimiter(
precision mediump float;

varying vec2 vVideoTexCoord;
varying vec2 vWmTexCoord;

uniform sampler2D textTexture;

void main()
{
    gl_FragColor = texture2D(textTexture, vWmTexCoord);
}
)delimiter";


static AutoRegisterFilter<WaterMarkFilter> g_registerWaterMarkFilter("builtin.transcoder.watermark",
    FilterType::WATERMARK,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<WaterMarkFilter>(name, FilterType::WATERMARK);
    });

WaterMarkFilter::WaterMarkFilter(std::string name, FilterType type) : OpenGLVideoFilterBase(std::move(name), type)
{
    MEDIA_LOG_I("water mark filter create");
}

WaterMarkFilter::~WaterMarkFilter()
{
    MEDIA_LOG_I("water mark filter destroy");
    DoRelease();
}

// ---- OpenGLVideoFilterBase virtual hook overrides ----

Status WaterMarkFilter::DoPrepare()
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::DoPrepare");
    MEDIA_LOG_I("Prepare");
    FALSE_RETURN_V_MSG_E(filterCallback_ != nullptr, Status::ERROR_UNKNOWN, "filterCallback_ is nullptr");
    if (filterType_ != FilterType::WATERMARK) {
        return Status::OK;
    }
    if (!isAVRecorderMode_.load()) {
        return filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
            StreamType::STREAMTYPE_RAW_VIDEO);
    }
    if (!isHardWatermarkMode_.load()) {
        filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
            StreamType::STREAMTYPE_WATERMARK);
    }
    return Status::OK;
}

Status WaterMarkFilter::SetVideoResize(int32_t width, int32_t height)
{
    FALSE_RETURN_V_MSG(width > 0 && height > 0, Status::ERROR_INVALID_PARAMETER, "Invalid resize dimensions");
    videoWidth_.store(width);
    videoHeight_.store(height);
    targetHeight_.store(height);
    targetWidth_.store(width);
    MEDIA_LOG_I("WaterMarkFilter::SetVideoResize videoWidth_: %{public}d videoHeight_: %{public}d, "
        "isHardWatermarkMode_: %{public}d", videoWidth_.load(), videoHeight_.load(),
        isHardWatermarkMode_.load());
    if (!isHardWatermarkMode_) {
        InitOpenGl();
    }

    return Status::OK;
}

void WaterMarkFilter::RenderFrame(int32_t viewWidth, int32_t viewHeight)
{
    // SDR 水印路径: 视频直通 + 水印叠加（通过 SwapBuffers 输出到 window surface）
    RenderVideoFrame();
    RenderWatermarkOverlay(viewWidth, viewHeight);
}

bool WaterMarkFilter::OnGLResourcesCreated()
{
    // 有水印时创建水印叠加着色器（通过GL_BLEND硬件Alpha混合叠加）
    if (watermarkList_.empty()) {
        return true;
    }
    FALSE_RETURN_V_MSG(CreateWatermarkTextures(), false, "Failed to create watermark textures");
    FALSE_RETURN_V_MSG(MergeWaterMarksWithOpenGL(), false, "Failed to merge watermarks with OpenGL");

    // 创建水印叠加着色器（视频 shader + VAO/VBO 由基类 CreateGLResources 创建）
    watermarkOverlayProgram_ = std::make_unique<ShaderProgram>(SHARED_VERTEX_SHADER, watermarkOverlayFragShader);
    FALSE_RETURN_V_MSG(watermarkOverlayProgram_->Valid(), false,
        "Failed to create watermark overlay shader program");

    // textTexId_ 指向合成后的水印纹理，供 RenderWatermarkOverlay 采样
    textTexId_ = mergedWatermarkTexId_;
    return true;
}

void WaterMarkFilter::OnSetupOverlayUniforms()
{
    // 水印叠加着色器 - 仅在有水印时设置
    if (watermarkOverlayProgram_ != nullptr) {
        watermarkOverlayProgram_->Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textTexId_);
        watermarkOverlayProgram_->SetInt("textTexture", 0);
    }
}

void WaterMarkFilter::OnReleaseGpuResources()
{
    // 释放水印专属 GL 资源（textTexId_ + 水印纹理 + 合成 FBO/VAO/VBO/shader）
    // textTexId_ 与 mergedWatermarkTexId_ 指向同一 GL texture name，此处仅删一次，
    // ReleaseMergeResources 中的 mergedWatermarkTexId_ 删除为 no-op（GL 已删除 name）
    if (textTexId_ != 0 && textTexId_ != DEFINED_VALUE) {
        glDeleteTextures(1, &textTexId_);
        textTexId_ = 0;
    }
    if (!watermarkTexIds_.empty()) {
        glDeleteTextures(watermarkTexIds_.size(), watermarkTexIds_.data());
        watermarkTexIds_.clear();
    }
    ReleaseMergeResources();
}

void WaterMarkFilter::OnReleaseCpuResources()
{
    // unique_ptr自动释放buffer内存
    watermarkList_.clear();
}

void WaterMarkFilter::OnReleaseShaders()
{
    watermarkOverlayProgram_.reset();
    // watermarkMergeShaderProgram_ 已由 ReleaseMergeResources 释放
}

// ---- 水印专属方法 ----

bool WaterMarkFilter::CreateWatermarkTextures()
{
    FALSE_RETURN_V_MSG(!watermarkList_.empty(), false, "watermarkList is empty, skip CreateWatermarkTextures");

    watermarkTexIds_.resize(watermarkList_.size());
    glGenTextures(watermarkList_.size(), watermarkTexIds_.data());

    for (size_t i = 0; i < watermarkList_.size(); i++) {
        const auto& config = watermarkList_[i];

        FALSE_RETURN_V_MSG(config.buffer != nullptr, false, "Watermark config buffer is empty");
        FALSE_RETURN_V_MSG(config.rawWidth > 0 && config.rawHeight > 0, false, "invalid watermark dimensions");
        constexpr int64_t RGBA_BPP = 4;
        int64_t needed = static_cast<int64_t>(config.rawWidth) * static_cast<int64_t>(config.rawHeight) * RGBA_BPP;
        FALSE_RETURN_V_MSG(needed <= config.size, false,
            "watermark buffer too small: need %{public}lld have %{public}lld",
            static_cast<long long>(needed), static_cast<long long>(config.size));

        glBindTexture(GL_TEXTURE_2D, watermarkTexIds_[i]);

        SetTextureParams2D(GL_TEXTURE_2D);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.rawWidth, config.rawHeight,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, config.buffer.get());

        CHECK_GL_ERROR("Failed to create watermark texture");
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    MEDIA_LOG_I("CreateWatermarkTextures success, created %{public}zu textures", watermarkList_.size());
    return true;
}

bool WaterMarkFilter::CreateWatermarkFBO(int32_t width, int32_t height)
{
    glGenFramebuffers(1, &watermarkFBO_);
    CHECK_GL_ERROR("glGenFramebuffers failed with GL error");

    glGenTextures(1, &mergedWatermarkTexId_);
    CHECK_GL_ERROR("glGenTextures failed with GL error");

    glBindTexture(GL_TEXTURE_2D, mergedWatermarkTexId_);
    CHECK_GL_ERROR("glBindTexture failed with GL error");

    // 水印合成使用GL_RGBA (8bit) FBO纹理，HDR精度由子类的输出FBO(GL_RGB10_A2)保证
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    GLenum texError = glGetError();
    FALSE_RETURN_V_MSG(texError == GL_NO_ERROR, false, "glTexImage2D failed with GL error: %{public}d", texError);

    SetTextureParams2D(GL_TEXTURE_2D);

    glBindFramebuffer(GL_FRAMEBUFFER, watermarkFBO_);
    CHECK_GL_ERROR("glBindFramebuffer failed with GL error");

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mergedWatermarkTexId_, 0);
    CHECK_GL_ERROR("glFramebufferTexture2D failed with GL error");


    FALSE_RETURN_V_MSG(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, false,
                       "Framebuffer is not complete");

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    CHECK_GL_ERROR("CreateWatermarkFBO failed with GL error");

    return true;
}

bool WaterMarkFilter::CreateWatermarkVaoVbo()
{
    glGenVertexArrays(1, &watermarkVAO_);
    CHECK_GL_ERROR("glGenVertexArrays failed with GL error");

    glGenBuffers(1, &watermarkVBO_);
    CHECK_GL_ERROR("glGenBuffers failed with GL error");

    glBindVertexArray(watermarkVAO_);
    CHECK_GL_ERROR("glBindVertexArray failed with GL error");

    glBindBuffer(GL_ARRAY_BUFFER, watermarkVBO_);
    CHECK_GL_ERROR("glBindBuffer failed with GL error");
    return true;
}

bool WaterMarkFilter::CreateWatermarkMergeShader()
{
    // Vertex shader for watermark merge rendering
    std::string watermarkVertexShader = R"delimiter(
attribute vec2 position;
attribute vec2 texCoord;
varying vec2 vTexCoord;
void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    vTexCoord = texCoord;
}
)delimiter";

    // Fragment shader: output texture color directly (blending via GL_BLEND hardware)
    // 不在此处翻转Y轴 — V坐标已在TEX_COORDS_PER_ROTATION中翻转，FBO中水印图片正立
    std::string watermarkFragmentShader = R"delimiter(
precision mediump float;
varying vec2 vTexCoord;
uniform sampler2D watermarkTexture;
void main()
{
    vec4 color = texture2D(watermarkTexture, vTexCoord);
    gl_FragColor = color;
}
)delimiter";

    watermarkMergeShaderProgram_ = std::make_unique<ShaderProgram>(watermarkVertexShader, watermarkFragmentShader);
    FALSE_RETURN_V_MSG(watermarkMergeShaderProgram_->Valid(), false,
                       "Failed to create watermark merge shader program");

    FALSE_RETURN_V_MSG(CreateWatermarkVaoVbo(), false, "CreateWatermarkVaoVbo failed");
    return true;
}

WaterMarkConfig WaterMarkFilter::ApplyRotationToConfig(const WaterMarkConfig& configTemp, uint32_t rotation) const
{
    WaterMarkConfig config{};
    config.top = configTemp.top;
    config.left = configTemp.left;
    config.watermarkWidth = configTemp.watermarkWidth;
    config.watermarkHeight = configTemp.watermarkHeight;
    config.rawWidth = configTemp.rawWidth;
    config.rawHeight = configTemp.rawHeight;
    config.rotation = configTemp.rotation;
    switch (rotation) {
        case ROTATE_90:
            config.top = targetHeight_.load() - configTemp.left - configTemp.watermarkWidth;
            config.left = configTemp.top;
            config.watermarkWidth = configTemp.watermarkHeight;
            config.watermarkHeight = configTemp.watermarkWidth;
            break;
        case ROTATE_270:
            config.left = targetWidth_.load() - configTemp.top - configTemp.watermarkHeight;
            config.top = configTemp.left;
            config.watermarkWidth = configTemp.watermarkHeight;
            config.watermarkHeight = configTemp.watermarkWidth;
            break;
        case ROTATE_180:
            config.left = targetWidth_.load() - configTemp.left - configTemp.watermarkWidth;
            config.top = targetHeight_.load() - configTemp.top - configTemp.watermarkHeight;
            break;
        default:
            break;
    }
    return config;
}

bool WaterMarkFilter::HasHardWatermarkPositionViolation() const
{
    if (watermarkList_.empty()) {
        return false;
    }

    const uint32_t frameWidth = static_cast<uint32_t>(targetWidth_.load());
    const uint32_t frameHeight = static_cast<uint32_t>(targetHeight_.load());
    if (frameWidth == 0 || frameHeight == 0) {
        MEDIA_LOG_W("HasHardWatermarkPositionViolation: video dimensions not set, treat as violation");
        return true;
    }

    static const uint32_t ROTATIONS[] = {0, ROTATE_90, ROTATE_180, ROTATE_270};
    static const uint32_t ALIGN = 16U;

    for (uint32_t rotation : ROTATIONS) {
        for (const auto& configTemp : watermarkList_) {
            WaterMarkConfig config = ApplyRotationToConfig(configTemp, rotation);
            if (config.watermarkWidth <= 0 || config.watermarkHeight <= 0) {
                MEDIA_LOG_I("HasHardWatermarkPositionViolation: invalid watermark size "
                    "w=%{public}d h=%{public}d, treat as violation",
                    config.watermarkWidth, config.watermarkHeight);
                return true;
            }
            uint32_t dstX = static_cast<uint32_t>(std::max(config.left, 0));
            uint32_t dstY = static_cast<uint32_t>(std::max(config.top, 0));
            uint32_t dstW = static_cast<uint32_t>(config.watermarkWidth);
            uint32_t dstH = static_cast<uint32_t>(config.watermarkHeight);
            if (dstX > UINT32_MAX - dstW || dstY > UINT32_MAX - dstH) {
                MEDIA_LOG_I("HasHardWatermarkPositionViolation: overflow detected "
                    "dstX=%{public}u dstW=%{public}u dstY=%{public}u dstH=%{public}u, treat as violation",
                    dstX, dstW, dstY, dstH);
                return true;
            }

            uint32_t alignedDstX  = MediaAVCodec::AlignDown(dstX, ALIGN);
            uint32_t alignedDstY  = MediaAVCodec::AlignDown(dstY, ALIGN);
            uint32_t alignedRight  = MediaAVCodec::AlignUp(dstX + dstW, ALIGN);
            uint32_t alignedBottom = MediaAVCodec::AlignUp(dstY + dstH, ALIGN);
            uint32_t alignedDstW  = alignedRight - alignedDstX;
            uint32_t alignedDstH  = alignedBottom - alignedDstY;

            if (alignedRight > frameWidth || alignedBottom > frameHeight ||
                alignedDstW > frameWidth || alignedDstH > frameHeight) {
                MEDIA_LOG_I("HasHardWatermarkPositionViolation: rotation=%{public}u, "
                    "dst(%{public}u,%{public}u,%{public}u,%{public}u), "
                    "aligned(%{public}u,%{public}u,%{public}u,%{public}u), "
                    "frame=%{public}ux%{public}u -> violation",
                    rotation, dstX, dstY, dstW, dstH,
                    alignedDstX, alignedDstY, alignedDstW, alignedDstH,
                    frameWidth, frameHeight);
                return true;
            }
        }
    }
    return false;
}

// Texture coordinates per rotation: {left-top, left-bottom, right-bottom, right-top}
// V坐标已翻转以补偿glTexImage2D底部原点约定(texture v=0=图片顶部)，使FBO中水印图片正立。
// ApplyRotationToConfig对90°/270°会swap watermarkWidth/Height，纹理坐标需同步旋转匹配。
static const float TEX_COORDS_PER_ROTATION[][8] = {
    {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},   // 0
    {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f},     // 90
    {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},     // 180
    {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f},     // 270
};

void WaterMarkFilter::CalculateRotatedWatermarkVertices(const WaterMarkConfig& configTemp,
    int32_t width, int32_t height, float wmVertices[16], size_t wmVerticesSize)
{
    FALSE_RETURN_V_MSG(wmVerticesSize >= MIN_VERTICES_SIZE, void(), "wmVerticesSize is too small");
    WaterMarkConfig config = ApplyRotationToConfig(configTemp, rotation_.load());

    float x = static_cast<float>(config.left) * 2.0f / width - 1.0f;
    // config.top is screen coordinate (origin at top-left), convert to GL coordinate (origin at bottom-left)
    float y = 1.0f - static_cast<float>(config.top + config.watermarkHeight) * 2.0f / height;
    float w = static_cast<float>(config.watermarkWidth) * 2.0f / width;
    float h = static_cast<float>(config.watermarkHeight) * 2.0f / height;

    // ApplyRotationToConfig对90°/270°旋转会交换watermarkWidth/Height，
    // 纹理坐标需同步旋转以匹配swap后的quad尺寸，否则图像会被拉伸成90°旋转效果。
    // V坐标已在TEX_COORDS_PER_ROTATION中翻转以补偿glTexImage2D底部原点约定。
    // 注意：ApplyRotationToConfig(ROTATE_270)实际做90°CW映射(逆旋转)，
    // 故纹理坐标也需用逆旋转索引(4-N)%4而非正旋转索引N。
    int32_t rotation = rotation_.load();
    uint32_t rotIdx = (rotation >= 0 && rotation % ROTATE_90 == 0 &&
        static_cast<uint32_t>(rotation) <= ROTATE_270)
        ? (4 - static_cast<uint32_t>(rotation) / ROTATE_90) % 4
        : 0;
    const float* tc = TEX_COORDS_PER_ROTATION[rotIdx];

    // left-top
    wmVertices[0] = x;
    wmVertices[1] = y + h;
    wmVertices[2] = tc[0];
    wmVertices[3] = tc[1];

    // left-bottom
    wmVertices[4] = x;
    wmVertices[5] = y;
    wmVertices[6] = tc[2];
    wmVertices[7] = tc[3];

    // right-bottom
    wmVertices[8] = x + w;
    wmVertices[9] = y;
    wmVertices[10] = tc[4];
    wmVertices[11] = tc[5];

    // right-top
    wmVertices[12] = x + w;
    wmVertices[13] = y + h;
    wmVertices[14] = tc[6];
    wmVertices[15] = tc[7];

    MEDIA_LOG_I("CalcRotatedWmVerts: top=%{public}d left=%{public}d wmW=%{public}d wmH=%{public}d "
        "fboW=%{public}d fboH=%{public}d rot=%{public}u -> x=%{public}.3f y=%{public}.3f w=%{public}.3f h=%{public}.3f "
        "(FBO pixels: x=%{public}d..%{public}d, y=%{public}d..%{public}d from bottom)",
        config.top, config.left, config.watermarkWidth, config.watermarkHeight,
        width, height, rotation_.load(), x, y, w, h,
        static_cast<int>((x + 1.0f) * width / 2), static_cast<int>((x + w + 1.0f) * width / 2),
        static_cast<int>((y + 1.0f) * height / 2), static_cast<int>((y + h + 1.0f) * height / 2));
}

bool WaterMarkFilter::RenderSingleWatermarkToFBO(const WaterMarkConfig& config,
                                                  GLuint texId,
                                                  int32_t width,
                                                  int32_t height)
{
    // 计算旋转后的顶点坐标和纹理坐标
    float wmVertices[16];
    CalculateRotatedWatermarkVertices(config, width, height, wmVertices, sizeof(wmVertices));

    // 设置顶点数据
    glBufferData(GL_ARRAY_BUFFER, sizeof(wmVertices), wmVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 绑定纹理并绘制
    glBindTexture(GL_TEXTURE_2D, texId);
    watermarkMergeShaderProgram_->SetInt("watermarkTexture", 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    CHECK_GL_ERROR("RenderSingleWatermarkToFBO failed with GL error");
    return true;
}

bool WaterMarkFilter::RenderWatermarksToFBO(int32_t width, int32_t height)
{
    // 安全检查：防止除零
    FALSE_RETURN_V_MSG(width > 0 && height > 0, false,
                       "Invalid FBO dimensions: %{public}dx%{public}d", width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    watermarkMergeShaderProgram_->Use();
    glActiveTexture(GL_TEXTURE0);

    for (size_t i = 0; i < watermarkList_.size(); i++) {
        const auto& config = watermarkList_[i];
        FALSE_RETURN_V_MSG(RenderSingleWatermarkToFBO(config, watermarkTexIds_[i], width, height),
                           false, "Failed to render watermark %{public}zu", i);
    }

    // 恢复OpenGL状态
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CHECK_GL_ERROR("RenderWatermarksToFBO failed with GL error");
    return true;
}

bool WaterMarkFilter::MergeWaterMarksWithOpenGL()
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::MergeWaterMarks");
    FALSE_RETURN_V_MSG(!watermarkList_.empty(), false, "watermarkList is empty, skip MergeWaterMarksWithOpenGL");

    // 计算旋转后所有水印的包围盒（用于glScissor限制渲染区域）
    ComputeWatermarkBoundingBox();

    // 计算目标尺寸
    int32_t mergedWidth = std::max(targetWidth_.load(), 1);
    int32_t mergedHeight = std::max(targetHeight_.load(), 1);

    FALSE_RETURN_V_MSG(CreateWatermarkFBO(mergedWidth, mergedHeight), false,
                       "Failed to create watermark FBO");

    FALSE_RETURN_V_MSG(CreateWatermarkMergeShader(), false,
                       "Failed to create watermark merge shader");

    FALSE_RETURN_V_MSG(RenderWatermarksToFBO(mergedWidth, mergedHeight), false,
                       "Failed to render watermarks to FBO");

    MEDIA_LOG_I("MergeWaterMarksWithOpenGL success, %{public}zu watermarks on target canvas: %{public}dx%{public}d",
        watermarkList_.size(), mergedWidth, mergedHeight);
    return true;
}

void WaterMarkFilter::ComputeWatermarkBoundingBox()
{
    if (watermarkList_.empty()) {
        wmBBoxLeft_ = 0;
        wmBBoxTop_ = 0;
        wmBBoxWidth_ = targetWidth_.load();
        wmBBoxHeight_ = targetHeight_.load();
        return;
    }

    int32_t minLeft = INT32_MAX;
    int32_t minTop = INT32_MAX;
    int32_t maxRight = INT32_MIN;
    int32_t maxBottom = INT32_MIN;

    for (const auto& configTemp : watermarkList_) {
        WaterMarkConfig config = ApplyRotationToConfig(configTemp, rotation_.load());
        minLeft = std::min(minLeft, config.left);
        minTop = std::min(minTop, config.top);
        maxRight = std::max(maxRight, config.left + config.watermarkWidth);
        maxBottom = std::max(maxBottom, config.top + config.watermarkHeight);
    }

    minLeft = std::max(0, minLeft);
    minTop = std::max(0, minTop);
    maxRight = std::min(targetWidth_.load(), maxRight);
    maxBottom = std::min(targetHeight_.load(), maxBottom);

    wmBBoxLeft_ = minLeft;
    wmBBoxTop_ = minTop;
    wmBBoxWidth_ = std::max(1, maxRight - minLeft);
    wmBBoxHeight_ = std::max(1, maxBottom - minTop);

    MEDIA_LOG_I("ComputeWatermarkBoundingBox left:%{public}d top:%{public}d w:%{public}d h:%{public}d",
        wmBBoxLeft_, wmBBoxTop_, wmBBoxWidth_, wmBBoxHeight_);
}

Status WaterMarkFilter::SetWatermark(std::shared_ptr<AVBuffer> &waterMarkBuffer, int32_t width, int32_t height)
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::SetWatermark");
    FALSE_RETURN_V_MSG(watermarkList_.size() < MAX_WATERMARK_NUMBER,
        Status::ERROR_WATERMARK_NUM_OUT_OF_RANGE, "watermarkList_ has out");
    FALSE_RETURN_V_MSG(waterMarkBuffer != nullptr && waterMarkBuffer->memory_ != nullptr &&
        waterMarkBuffer->meta_ != nullptr, Status::ERROR_INVALID_OPERATION, "waterMarkBuffer or memory_ is null");
    int32_t hileft = 0;
    int32_t hitop = 0;
    int32_t hiwidth = 0;
    int32_t hiheight = 0;
    int32_t histride = 0;
    waterMarkBuffer->meta_->Get<Tag::VIDEO_COORDINATE_X>(hileft);
    waterMarkBuffer->meta_->Get<Tag::VIDEO_COORDINATE_Y>(hitop);
    waterMarkBuffer->meta_->Get<Tag::VIDEO_COORDINATE_W>(hiwidth);
    waterMarkBuffer->meta_->Get<Tag::VIDEO_COORDINATE_H>(hiheight);
    waterMarkBuffer->meta_->Get<Tag::VIDEO_STRIDE>(histride);
    // 检查乘法溢出
    FALSE_RETURN_V_MSG(histride > 0 && hiheight > 0, Status::ERROR_INVALID_PARAMETER,
        "Invalid watermark dimensions");
    FALSE_RETURN_V_MSG(static_cast<uint64_t>(hiheight) <= MAX_WATERMARK_SIZE / static_cast<uint64_t>(histride),
        Status::ERROR_INVALID_PARAMETER, "Watermark size exceeds maximum limit");
    int64_t bufferSize = static_cast<int64_t>(hiheight) * histride;
    FALSE_RETURN_V_MSG(static_cast<uint64_t>(bufferSize) < MAX_WATERMARK_SIZE && bufferSize > 0,
        Status::ERROR_INVALID_PARAMETER, "Failed to allocate memory for image data");
    auto dataBuffer = std::make_unique<uint8_t[]>(bufferSize);
    int32_t bytesRead = waterMarkBuffer->memory_->Read(dataBuffer.get(), bufferSize, 0);
    FALSE_RETURN_V_MSG(bytesRead == bufferSize, Status::ERROR_INVALID_OPERATION,
        "Failed to read watermark data, expected %{public}ld got %{public}d",
        static_cast<long>(bufferSize), bytesRead);
    MEDIA_LOG_I("WaterMarkFilter::SetWatermark hileft: %{public}d hitop:"
       "%{public}d hiwidth: %{public}d hiheight: %{public}d width: %{public}d height: %{public}d histride: %{public}d",
        hileft, hitop, hiwidth, hiheight, width, height, histride);
    WaterMarkConfig config;
    config.top = hitop;
    config.left = hileft;
    config.rawHeight = hiheight;
    config.rawWidth = hiwidth;
    config.size = bufferSize;
    config.buffer = std::move(dataBuffer);
    // 根据旋转角度设置目标显示尺寸
    config.watermarkWidth = (width > 0) ? width : hiwidth;
    config.watermarkHeight = (height > 0) ? height : hiheight;
    watermarkList_.push_back(std::move(config));
    return Status::OK;
}

void WaterMarkFilter::SetAVRecorderMode()
{
    MEDIA_LOG_I("SetAVRecorderMode");
    isAVRecorderMode_.store(true);
}

void WaterMarkFilter::SetHardWatermarkMode(bool isHardMode)
{
    MEDIA_LOG_I("SetHardWatermarkMode, isHardMode: %{public}d", isHardMode);
    isHardWatermarkMode_.store(isHardMode);
}

Status WaterMarkFilter::MergeAndSaveBBox()
{
    bool ret = MergeWaterMarksWithOpenGL();
    FALSE_RETURN_V_MSG(ret, Status::ERROR_UNKNOWN, "MergeWaterMarksWithOpenGL failed");
    MEDIA_LOG_I("MergeAndSaveBBox bbox: left=%{public}d top=%{public}d w=%{public}d h=%{public}d",
        wmBBoxLeft_, wmBBoxTop_, wmBBoxWidth_, wmBBoxHeight_);
    return Status::OK;
}

Status WaterMarkFilter::PreprocessWatermarks()
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::PreprocessWatermarks");
    MEDIA_LOG_I("PreprocessWatermarks enter, watermark count: %{public}zu", watermarkList_.size());
    FALSE_RETURN_V_MSG(watermarkList_.size() > 0, Status::ERROR_INVALID_OPERATION,
        "no watermark to preprocess");

    if (!eglContextReady_) {
        renderContext_ = std::make_unique<EglRenderContext>();
        FALSE_RETURN_V_MSG(renderContext_ != nullptr, Status::ERROR_UNKNOWN,
            "Failed to create EGL render context");
        FALSE_RETURN_V_MSG(renderContext_->Init(), Status::ERROR_UNKNOWN,
            "Failed to initialize EGL render context");

        if (eglSurface_ != EGL_NO_SURFACE && renderContext_ != nullptr) {
            renderContext_->DestroyEglSurface(eglSurface_);
        }
        eglSurface_ = renderContext_->CreatePbufferSurface();
        FALSE_RETURN_V_MSG(eglSurface_ != EGL_NO_SURFACE, Status::ERROR_UNKNOWN,
            "Failed to create Pbuffer surface");

        renderContext_->MakeCurrent(eglSurface_);
        eglContextReady_ = true;
    }

    if (!watermarkList_.empty()) {
        FALSE_RETURN_V_MSG(CreateWatermarkTextures(), Status::ERROR_UNKNOWN,
            "Failed to create watermark textures");
    }

    return MergeAndSaveBBox();
}

Status WaterMarkFilter::ApplyRotation(int32_t rotation)
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::ApplyRotation");
    MEDIA_LOG_I("ApplyRotation enter, rotation: %{public}d", rotation);

    if (rotation == rotation_.load()) {
        return Status::OK;
    }

    rotation_.store(rotation);

    if (!eglContextReady_) {
        MEDIA_LOG_E("EGL context not ready, cannot apply rotation");
        return Status::ERROR_INVALID_OPERATION;
    }

    renderContext_->MakeCurrent(eglSurface_);

    ReleaseMergeResources();

    return MergeAndSaveBBox();
}

std::shared_ptr<AVBuffer> WaterMarkFilter::GetMergedWatermarkBuffer(int32_t rotation)
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::GetMergedWatermarkBuffer");
    MEDIA_LOG_I("GetMergedWatermarkBuffer enter, rotation: %{public}d", rotation);

    if (!eglContextReady_ || mergedWatermarkTexId_ == 0) {
        MEDIA_LOG_E("EGL not ready or no merged texture");
        return nullptr;
    }

    renderContext_->MakeCurrent(eglSurface_);

    auto waterMarkBuffer = ReadMergedWatermarkPixels();
    FALSE_RETURN_V_MSG(waterMarkBuffer != nullptr, nullptr, "ReadMergedWatermarkPixels failed");

    waterMarkBuffer->meta_->Set<Tag::VIDEO_ENCODER_ENABLE_WATERMARK>(true);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_ROTATION>(static_cast<Plugins::VideoRotation>(rotation));
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(wmBBoxLeft_);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(wmBBoxTop_);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(wmBBoxWidth_);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(wmBBoxHeight_);

    MEDIA_LOG_I("GetMergedWatermarkBuffer done, size=%{public}d, bbox=(%{public}d,%{public}d,%{public}d,%{public}d)",
        static_cast<int32_t>(waterMarkBuffer->memory_->GetSize()),
        wmBBoxLeft_, wmBBoxTop_, wmBBoxWidth_, wmBBoxHeight_);
    return waterMarkBuffer;
}

sptr<SurfaceBuffer> WaterMarkFilter::AllocateWatermarkSurfaceBuffer(int32_t wmWidth, int32_t wmHeight)
{
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    FALSE_RETURN_V_MSG_E(surfaceBuffer != nullptr, nullptr, "Failed to create SurfaceBuffer");

    BufferRequestConfig bufferConfig = {
        .width = wmWidth,
        .height = wmHeight,
        .strideAlignment = STRIDE_ALIGNMENT,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    int32_t ret = surfaceBuffer->Alloc(bufferConfig);
    FALSE_RETURN_V_MSG_E(ret == 0, nullptr, "SurfaceBuffer Alloc failed, ret: %{public}d", ret);
    return surfaceBuffer;
}

bool WaterMarkFilter::CopyPixelsToSurfaceBuffer(const std::unique_ptr<uint8_t[]>& srcBuf,
    int32_t rowStride, int32_t wmHeight, const sptr<SurfaceBuffer>& surfaceBuffer)
{
    int32_t surfaceStride = surfaceBuffer->GetStride();
    // FBO中水印图片正立(FBO顶部=图片顶部)，glReadPixels从FBO底部(row 0)开始读取，
    // srcBuf row 0=图片底部，需Y翻转使surfaceBuffer row 0=图片顶部(硬件水印期望row 0=顶部)
    for (int32_t i = 0; i < wmHeight; i++) {
        int32_t srcRow = wmHeight - 1 - i;
        errno_t err = memcpy_s(static_cast<uint8_t *>(surfaceBuffer->GetVirAddr()) + i * surfaceStride,
            rowStride, srcBuf.get() + srcRow * rowStride, rowStride);
        FALSE_RETURN_V_MSG_E(err == 0, false, "memcpy_s failed at row %{public}d", i);
    }
    return true;
}

std::shared_ptr<AVBuffer> WaterMarkFilter::ReadMergedWatermarkPixels()
{
    MEDIA_LOG_I("ReadMergedWatermarkPixels enter");
    int32_t wmLeft = wmBBoxLeft_;
    int32_t wmWidth = wmBBoxWidth_;
    int32_t wmHeight = wmBBoxHeight_;
    // wmBBoxTop_ is screen coordinate (origin at top-left), convert to GL coordinate (origin at bottom-left)
    int32_t wmTop = targetHeight_.load() - wmBBoxTop_ - wmBBoxHeight_;
    FALSE_RETURN_V_MSG_E(wmWidth > 0 && wmHeight > 0, nullptr,
        "Invalid watermark bbox: left=%{public}d top=%{public}d w=%{public}d h=%{public}d",
        wmLeft, wmTop, wmWidth, wmHeight);

    ScopedFboBind fboBinder(watermarkFBO_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, mergedWatermarkTexId_, 0);
    GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    FALSE_RETURN_V_MSG_E(fbStatus == GL_FRAMEBUFFER_COMPLETE, nullptr,
        "Framebuffer not complete, status: %{public}d", fbStatus);

    sptr<SurfaceBuffer> surfaceBuffer = AllocateWatermarkSurfaceBuffer(wmWidth, wmHeight);
    FALSE_RETURN_V_MSG_E(surfaceBuffer != nullptr, nullptr, "AllocateWatermarkSurfaceBuffer failed");

    int64_t rowStride64 = static_cast<int64_t>(wmWidth) * RGBA_BYTES_PER_PIXEL;
    FALSE_RETURN_V_MSG_E(rowStride64 <= INT32_MAX, nullptr, "rowStride overflow: wmWidth=%{public}d", wmWidth);
    int32_t rowStride = static_cast<int32_t>(rowStride64);
    int64_t cpuBufSize = rowStride64 * wmHeight;
    auto tmpBuf = std::make_unique<uint8_t[]>(cpuBufSize);
    FALSE_RETURN_V_MSG_E(tmpBuf != nullptr, nullptr, "Failed to allocate temp buffer");

    glReadPixels(wmLeft, wmTop, wmWidth, wmHeight, GL_RGBA, GL_UNSIGNED_BYTE, tmpBuf.get());
    GLenum error = glGetError();
    FALSE_RETURN_V_MSG_E(error == GL_NO_ERROR, nullptr, "glReadPixels failed, GL error: %{public}d", error);

    FALSE_RETURN_V_MSG_E(CopyPixelsToSurfaceBuffer(tmpBuf, rowStride, wmHeight, surfaceBuffer),
        nullptr, "CopyPixelsToSurfaceBuffer failed");

    auto waterMarkBuffer = AVBuffer::CreateAVBuffer(surfaceBuffer);
    FALSE_RETURN_V_MSG_E(waterMarkBuffer != nullptr && waterMarkBuffer->memory_ != nullptr, nullptr,
        "Failed to create AVBuffer from SurfaceBuffer");

    MEDIA_LOG_I("ReadMergedWatermarkPixels success, bbox=(%{public}d,%{public}d,%{public}d,%{public}d), "
        "stride=%{public}d, size=%{public}d",
        wmLeft, wmTop, wmWidth, wmHeight, surfaceBuffer->GetStride(), surfaceBuffer->GetSize());
    return waterMarkBuffer;
}

void WaterMarkFilter::ReleaseMergeResources()
{
    if (watermarkFBO_ != 0) {
        glDeleteFramebuffers(1, &watermarkFBO_);
        watermarkFBO_ = 0;
    }
    if (mergedWatermarkTexId_ != 0) {
        glDeleteTextures(1, &mergedWatermarkTexId_);
        mergedWatermarkTexId_ = 0;
    }
    if (watermarkVAO_ != 0) {
        glDeleteVertexArrays(1, &watermarkVAO_);
        watermarkVAO_ = 0;
    }
    if (watermarkVBO_ != 0) {
        glDeleteBuffers(1, &watermarkVBO_);
        watermarkVBO_ = 0;
    }
    watermarkMergeShaderProgram_.reset();
}

void WaterMarkFilter::RenderWatermarkOverlay(int32_t viewWidth, int32_t viewHeight)
{
    // 在水印包围盒区域叠加水印（glScissor限制渲染区域 + GL_BLEND硬件Alpha混合）
    if (wmBBoxWidth_ <= 0 || wmBBoxHeight_ <= 0 || !watermarkOverlayProgram_) {
        return;
    }
    int32_t scissorX = std::max(0, wmBBoxLeft_);
    int32_t scissorY = std::max(0, viewHeight - wmBBoxTop_ - wmBBoxHeight_);
    int32_t scissorW = std::min(wmBBoxWidth_, viewWidth - scissorX);
    int32_t scissorH = std::min(wmBBoxHeight_, viewHeight - scissorY);
    MEDIA_LOG_I("RenderWatermarkOverlay: bbox(top=%{public}d left=%{public}d w=%{public}d h=%{public}d) "
        "view=%{public}dx%{public}d -> scissor(x=%{public}d y=%{public}d w=%{public}d h=%{public}d)",
        wmBBoxTop_, wmBBoxLeft_, wmBBoxWidth_, wmBBoxHeight_,
        viewWidth, viewHeight, scissorX, scissorY, scissorW, scissorH);
    if (scissorW <= 0 || scissorH <= 0) {
        return;
    }
    glEnable(GL_SCISSOR_TEST);
    glScissor(scissorX, scissorY, scissorW, scissorH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    watermarkOverlayProgram_->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textTexId_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, QUAD_INDICES);

    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

#undef CHECK_GL_ERROR

} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
