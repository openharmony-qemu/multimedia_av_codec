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

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "WaterMarkFilter" };

class ScopedFboBind {
public:
    explicit ScopedFboBind(GLuint fbo) : prevFbo_(0)
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&prevFbo_));
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }
    ~ScopedFboBind() { glBindFramebuffer(GL_FRAMEBUFFER, prevFbo_); }
private:
    GLuint prevFbo_;
};

constexpr int32_t STRIDE_ALIGNMENT = 8;
constexpr int32_t RGBA_BYTES_PER_PIXEL = 4;

void SetTextureParams2D(GLenum target)
{
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

#define CHECK_GL_ERROR(msg) \
    do { GLenum e_ = glGetError(); \
         FALSE_RETURN_V_MSG(e_ == GL_NO_ERROR, false, msg ": %{public}d", e_); \
    } while (0)
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
 * @brief 顶点着色器：处理顶点位置和纹理坐标
 */
static std::string vertexShader = R"delimiter(
attribute vec3 position;
attribute vec2 texCoord;

varying vec2 vTexCoord;

uniform mat4 matTransform;

void main()
{
    gl_Position = vec4(position, 1.0);
    vTexCoord = texCoord;
}
)delimiter";

/**
 * @brief 片段着色器：仅采样视频纹理，无水印混合
 * 用于视频直通渲染（两通道方案的视频通道）
 */
static std::string videoFragShader = R"delimiter(
#extension GL_OES_EGL_image_external : require
precision highp float;

varying vec2 vTexCoord;

uniform samplerExternalOES texture;

void main()
{
    vec2 flippedTexCoord = vec2(1.0 - vTexCoord.x, 1.0 - vTexCoord.y);
    vec4 videoColor = texture2D(texture, flippedTexCoord);
    gl_FragColor = vec4(videoColor.rgb, 1.0);
}
)delimiter";

/**
 * @brief 水印叠加片段着色器：仅采样水印纹理，通过GL_BLEND硬件Alpha混合叠加到视频帧上
 * 与 视频直通 配合使用，避免在水印区域外执行双纹理采样+Alpha混合计算
 */
static std::string watermarkOverlayFragShader = R"delimiter(
precision mediump float;

varying vec2 vTexCoord;

uniform sampler2D textTexture;

void main()
{
    vec2 flippedTexCoord = vec2(1.0 - vTexCoord.x, 1.0 - vTexCoord.y);
    gl_FragColor = texture2D(textTexture, flippedTexCoord);
}
)delimiter";

// 顶点数据：位置坐标(x, y, z) + 纹理坐标(u, v)
static GLfloat vertices[] = {
    -1.0f, 1.0f,  0.0f,    1.0f, 1.0f,  // 左上角
    -1.0f, -1.0f, 0.0f,    1.0f, 0.0f,  // 左下角
    1.0f,  -1.0f, 0.0f,    0.0f, 0.0f,  // 右下角
    1.0f,  1.0f,  0.0f,    0.0f, 1.0f,  // 右上角
};

// 索引数据：定义两个三角形组成四边形
static GLuint indices[] = {
    0, 1, 2,  // 第一个三角形
    0, 2, 3   // 第二个三角形
};


static AutoRegisterFilter<WaterMarkFilter> g_registerWaterMarkFilter("builtin.transcoder.watermark",
    FilterType::WATERMARK,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<WaterMarkFilter>(name, FilterType::WATERMARK);
    });

class WaterMarkFilterLinkCallback : public FilterLinkCallback {
public:
    explicit WaterMarkFilterLinkCallback(std::shared_ptr<WaterMarkFilter> waterMarkFilter)
        : waterMarkFilter_(std::move(waterMarkFilter))
    {
    }

    ~WaterMarkFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        auto filter = waterMarkFilter_.lock();
        FALSE_RETURN_MSG(filter != nullptr, "filter is nullptr");
        filter->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        auto filter = waterMarkFilter_.lock();
        FALSE_RETURN_MSG(filter != nullptr, "filter is nullptr");
        filter->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        auto filter = waterMarkFilter_.lock();
        FALSE_RETURN_MSG(filter != nullptr, "filter is nullptr");
        filter->OnUpdatedResult(meta);
    }
private:
    std::weak_ptr<WaterMarkFilter> waterMarkFilter_;
};

bool WaterMarkFilter::CreateNativeImage()
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::CreateNativeImage");
    nativeImage_ = OH_NativeImage_Create(-1, GL_TEXTURE_EXTERNAL_OES);
    FALSE_RETURN_V_MSG(nativeImage_ != nullptr, false, "Failed to create native image");
    nativeImageWindow_ = OH_NativeImage_AcquireNativeWindow(nativeImage_);
    if (nativeImageWindow_ == nullptr) {
        MEDIA_LOG_E("Failed to acquire native window, destroying nativeImage");
        OH_NativeImage_Destroy(&nativeImage_);
        return false;
    }

    int ret = 0;
    nativeImageFrameAvailableListener_ = std::make_unique<OH_OnFrameAvailableListener>();
    nativeImageFrameAvailableListener_->context = this;
    nativeImageFrameAvailableListener_->onFrameAvailable = &WaterMarkFilter::OnNativeImageFrameAvailable;
    ret = OH_NativeImage_SetOnFrameAvailableListener(nativeImage_, *nativeImageFrameAvailableListener_);
    if (ret != 0) {
        MEDIA_LOG_E("Failed to set frame available listener: %{public}d, destroying nativeImage", ret);
        OH_NativeImage_Destroy(&nativeImage_);
        nativeImageWindow_ = nullptr;
        return false;
    }
    return true;
}

void WaterMarkFilter::UpdateNativeWindow(void *window, uint64_t width, uint64_t height)
{
    auto nativeWindow = reinterpret_cast<OHNativeWindow *>(window);
    nativeWindow_ = nativeWindow;
    FALSE_RETURN_MSG(nativeWindow_ != nullptr, "nativeWindow_ is nullptr");
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_BUFFER_GEOMETRY, static_cast<int>(width),
        static_cast<int>(height));
}

void WaterMarkFilter::RenderThreadEntry()
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::RenderThreadEntry");
    MEDIA_LOG_I("WaterMarkFilter::RenderThreadEntry");
    // 初始化EGL上下文
    FALSE_RETURN_MSG(InitializeEGLContext(), "Failed to initialize EGL context");
    FALSE_RETURN_MSG(CreateGLResources(), "Failed to create GL resources");
    if (!watermarkList_.empty()) {
        FALSE_RETURN_MSG(CreateWatermarkTextures(), "Failed to create watermark textures");
        FALSE_RETURN_MSG(MergeWaterMarksWithOpenGL(), "Failed to merge watermarks with OpenGL");
    }
    CreateVideoTexture();

    textTexId_ = mergedWatermarkTexId_;
    FALSE_RETURN_MSG(SetupShaderUniforms(), "Failed to setup shader uniforms");

    // 将Native Image与OpenGL纹理关联
    OH_NativeImage_AttachContext(nativeImage_, nativeImageTexId_);
    RenderLoop();
}

bool WaterMarkFilter::InitializeEGLContext()
{
    // 创建EGL渲染上下文
    renderContext_ = std::make_unique<EglRenderContext>();
    FALSE_RETURN_V_MSG(renderContext_ != nullptr, false, "Failed to create EGL render context");

    // 初始化EGL：初始化显示、配置EGL、创建上下文
    FALSE_RETURN_V_MSG(renderContext_->Init(), false, "Failed to initialize EGL render context");
    FALSE_RETURN_V_MSG(nativeWindow_ != nullptr, false, "nativeWindow_ is nullptr");

    // 创建EGL表面，将OpenGL渲染输出关联到Native Window
    if (eglSurface_ != EGL_NO_SURFACE && renderContext_ != nullptr) {
        renderContext_->DestroyEglSurface(eglSurface_);
    }
    eglSurface_ = renderContext_->CreateEglSurface(static_cast<EGLNativeWindowType>(nativeWindow_));
    FALSE_RETURN_V_MSG(eglSurface_ != EGL_NO_SURFACE, false, "Failed to create EGL surface");

    // 将EGL上下文和表面绑定到当前线程
    renderContext_->MakeCurrent(eglSurface_);
    return true;
}

void WaterMarkFilter::DrawImage(const int64_t pts)
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::DrawImage");
    FALSE_RETURN_MSG(shaderProgram_ != nullptr, "shaderProgram_ is nullptr");
    glViewport(0, 0, targetWidth_.load(), targetHeight_.load());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glBindVertexArray(vertexArrayObject_);

    // 全屏渲染纯视频帧（单纹理采样，无水印混合计算）
    shaderProgram_->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

    // 在水印包围盒区域叠加水印（glScissor限制渲染区域 + GL_BLEND硬件Alpha混合）
    if (wmBBoxWidth_ > 0 && wmBBoxHeight_ > 0 && watermarkOverlayProgram_) {
        int32_t scissorX = std::max(0, wmBBoxLeft_);
        int32_t scissorY = std::max(0, targetHeight_.load() - wmBBoxTop_ - wmBBoxHeight_);
        int32_t scissorW = std::min(wmBBoxWidth_, targetWidth_.load() - scissorX);
        int32_t scissorH = std::min(wmBBoxHeight_, targetHeight_.load() - scissorY);
        if (scissorW > 0 && scissorH > 0) {
            glEnable(GL_SCISSOR_TEST);
            glScissor(scissorX, scissorY, scissorW, scissorH);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            watermarkOverlayProgram_->Use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textTexId_);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

            glDisable(GL_BLEND);
            glDisable(GL_SCISSOR_TEST);
        }
    }

    FALSE_RETURN_MSG(nativeWindow_ != nullptr, "nativeWindow_ is nullptr");
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_UI_TIMESTAMP, pts);
    SwapFrameBuffers();
}

void WaterMarkFilter::SwapFrameBuffers()
{
    FALSE_RETURN_MSG(renderContext_ != nullptr, "renderContext_ is nullptr");
    renderContext_->SwapBuffers(eglSurface_);
}

/**
 * @brief 为每个水印创建OpenGL纹理
 * @return true：创建成功；false：创建失败
 */
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

bool WaterMarkFilter::CreateVideoTexture()
{
    glGenTextures(1, &nativeImageTexId_);
    CHECK_GL_ERROR("glGenTextures failed with GL error");

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    CHECK_GL_ERROR("glBindTexture failed with GL error");

    SetTextureParams2D(GL_TEXTURE_EXTERNAL_OES);

    CHECK_GL_ERROR("Failed to create native image texture");
    return true;
}

bool WaterMarkFilter::SetupShaderUniforms()
{
    FALSE_RETURN_V_MSG(shaderProgram_ != nullptr, false, "shaderProgram_ is nullptr");

    // 视频直通着色器 - 仅采样视频纹理
    shaderProgram_->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    shaderProgram_->SetInt("texture", 0);
    shaderProgram_->SetMatrix4v("matTransform", matrix_, 16, false);

    // 水印叠加着色器 - 仅在有水印时设置
    if (watermarkOverlayProgram_ != nullptr) {
        watermarkOverlayProgram_->Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textTexId_);
        watermarkOverlayProgram_->SetInt("textTexture", 0);
    }

    MEDIA_LOG_I("SetupShaderUniforms video: %{public}dx%{public}d -> target: %{public}dx%{public}d",
        videoWidth_.load(), videoHeight_.load(), targetWidth_.load(), targetHeight_.load());

    return true;
}

void WaterMarkFilter::RenderLoop()
{
    while (running_) {
        MEDIA_LOG_I("RenderLoop availableFrameCnt_");
        bool flag = false;
        std::unique_lock<std::mutex> lock(wakeUpMutex_);
        wakeUpCond_.wait(lock, [this]() { return availableFrameCnt_.load() > 0 || eosPts_.load() != -1 || !running_;});
        if (availableFrameCnt_.load() > 0) {
            availableFrameCnt_.fetch_sub(1, std::memory_order_relaxed);
            currentFrameNum_.fetch_add(1, std::memory_order_relaxed);
            flag = true;
        }

        if (flag) {
            UpdateVideoFrame();
            DrawImage(lastPts_.load());
        }

        if (eosPts_.load() == lastPts_.load()) {
            MEDIA_LOG_I("RenderLoop pts %{public}ld %{public}ld", lastPts_.load(), eosPts_.load());
            NotifyNextFilterEos();
            break;
        }
    }
}

void WaterMarkFilter::UpdateVideoFrame()
{
    FALSE_RETURN_MSG(renderContext_ != nullptr, "renderContext_ is null");
    // 确保EGL上下文和表面在当前线程生效
    renderContext_->MakeCurrent(eglSurface_);
    FALSE_RETURN_MSG(nativeImage_ != nullptr, "nativeImage_ is nullptr");
    OH_NativeImage_UpdateSurfaceImage(nativeImage_);
    lastPts_.store(OH_NativeImage_GetTimestamp(nativeImage_));
    MEDIA_LOG_D("UpdateVideoFrame pts %{public}ld", lastPts_.load());
}

bool WaterMarkFilter::CreateWatermarkFBO(int32_t width, int32_t height)
{
    glGenFramebuffers(1, &watermarkFBO_);
    CHECK_GL_ERROR("glGenFramebuffers failed with GL error");

    glGenTextures(1, &mergedWatermarkTexId_);
    CHECK_GL_ERROR("glGenTextures failed with GL error");

    glBindTexture(GL_TEXTURE_2D, mergedWatermarkTexId_);
    CHECK_GL_ERROR("glBindTexture failed with GL error");

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    CHECK_GL_ERROR("glTexImage2D failed with GL error");

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
                    "frame=%{public}ux%{public}u -> violation", rotation, dstX, dstY, dstW, dstH,
                    alignedDstX, alignedDstY, alignedDstW, alignedDstH, frameWidth, frameHeight);
                return true;
            }
        }
    }
    return false;
}

// Texture coordinates per rotation: {left-bottom, left-top, right-top, right-bottom}
static const float TEX_COORDS_PER_ROTATION[][8] = {
    {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f},   // default (0)
    {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f},     // 90
    {1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f},     // 180
    {1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f},     // 270
};

void WaterMarkFilter::CalculateRotatedWatermarkVertices(const WaterMarkConfig& configTemp,
    int32_t width, int32_t height, float wmVertices[16], size_t wmVerticesSize)
{
    FALSE_RETURN_V_MSG(wmVerticesSize >= MIN_VERTICES_SIZE, void(), "wmVerticesSize is too small");
    WaterMarkConfig config = ApplyRotationToConfig(configTemp, rotation_.load());

    float x = static_cast<float>(config.left) * 2.0f / width - 1.0f;
    float y = -1.0f + static_cast<float>(config.top) * 2.0f / height;
    float w = static_cast<float>(config.watermarkWidth) * 2.0f / width;
    float h = static_cast<float>(config.watermarkHeight) * 2.0f / height;

    int32_t rotation = rotation_.load();
    uint32_t rotIdx = (rotation >= 0 && rotation % ROTATE_90 == 0 &&
        static_cast<uint32_t>(rotation) <= ROTATE_270) ? static_cast<uint32_t>(rotation) / ROTATE_90 : 0;
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

bool WaterMarkFilter::CreateGLResources()
{
    // 创建视频直通着色器程序（仅采样视频纹理）
    shaderProgram_ = std::make_unique<ShaderProgram>(vertexShader, videoFragShader);
    FALSE_RETURN_V_MSG(shaderProgram_->Valid(), false, "Failed to create video shader program");

    // 有水印时创建水印叠加着色器（通过GL_BLEND硬件Alpha混合叠加）
    if (!watermarkList_.empty()) {
        watermarkOverlayProgram_ = std::make_unique<ShaderProgram>(vertexShader, watermarkOverlayFragShader);
        FALSE_RETURN_V_MSG(watermarkOverlayProgram_->Valid(), false,
            "Failed to create watermark overlay shader program");
    }

    glGenVertexArrays(1, &vertexArrayObject_);
    CHECK_GL_ERROR("glGenVertexArrays failed with GL error");

    glGenBuffers(1, &vertexBufferObject_);
    CHECK_GL_ERROR("glGenBuffers failed with GL error");

    glBindVertexArray(vertexArrayObject_);
    CHECK_GL_ERROR("glBindVertexArray failed with GL error");

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject_);
    CHECK_GL_ERROR("glBindBuffer failed with GL error");

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    CHECK_GL_ERROR("glBufferData failed with GL error");

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

    // 启用纹理坐标属性
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    CHECK_GL_ERROR("CreateGLResources failed with GL error");
    return true;
}

void WaterMarkFilter::OnNativeImageFrameAvailable(void *data)
{
    MEDIA_LOG_D("OnNativeImageFrameAvailable");
    auto renderThread = reinterpret_cast<WaterMarkFilter *>(data);
    FALSE_RETURN_MSG(renderThread != nullptr, "renderThread is nullptr");
    std::unique_lock<std::mutex> lock(renderThread->wakeUpMutex_);
    renderThread->availableFrameCnt_.fetch_add(1, std::memory_order_relaxed);
    renderThread->wakeUpCond_.notify_one();
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

void WaterMarkFilter::InitOpenGl()
{
    FALSE_RETURN_MSG(nativeImage_ == nullptr, "CreateNativeImage success");
    FALSE_RETURN_MSG(CreateNativeImage(), "Failed to create native image");
}

WaterMarkFilter::WaterMarkFilter(std::string name, FilterType type): Filter(name, type)
{
    filterType_ = type;
    // 初始化为单位矩阵
    int matrixStep = 4;
    for (int i = 0; i < matrixStep; ++i) {
        matrix_[i * matrixStep + i] = 1.0f;
    }
    MEDIA_LOG_I("water mark filter create");
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

WaterMarkFilter::~WaterMarkFilter()
{
    MEDIA_LOG_I("water mark filter destroy");
    DoRelease();
}

Status WaterMarkFilter::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    MEDIA_LOG_I("SetCodecFormat");
    return Status::OK;
}

void WaterMarkFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status WaterMarkFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    configureParameter_ = parameter;
    return Status::OK;
}

sptr<Surface> WaterMarkFilter::GetInputSurface()
{
    MEDIA_LOG_I("GetInputSurface");
    std::lock_guard<std::mutex> lock(releaseMutex_);
    FALSE_RETURN_V_MSG(nativeImageWindow_ != nullptr, nullptr, "nativeImageWindow_ is nullptr");
    return nativeImageWindow_->surface;
}

Status WaterMarkFilter::SetOutputSurface(sptr<Surface> surface, int32_t width, int32_t height, int32_t rotation)
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::SetOutputSurface");
    MEDIA_LOG_I("WaterMarkFilter SetOutputSurface %{public}d, %{public}d", width, height);
    targetWidth_.store(width);
    targetHeight_.store(height);
    rotation_.store(rotation);
    FALSE_RETURN_V_MSG(surface != nullptr, Status::ERROR_UNKNOWN, "surface is null");
    auto window = CreateNativeWindowFromSurface(&surface);
    UpdateNativeWindow(window, width, height);

    // 首次调用时启动渲染线程，确保帧到达前线程已就绪
    if (!thread_.joinable()) {
        StartRenderThread();
    }
    return Status::OK;
}

bool WaterMarkFilter::StartRenderThread()
{
    MEDIA_LOG_I("WaterMarkFilter::StartRenderThread");
    running_ = true;
    thread_ = std::thread([this]() {
        RenderThreadEntry();
    });
    return true;
}

void WaterMarkFilter::StopRenderThread()
{
    MEDIA_LOG_I("WaterMarkFilter::StopRenderThread");
    {
        std::lock_guard<std::mutex> wakeLock(wakeUpMutex_);
        running_ = false;
    }
    wakeUpCond_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

Status WaterMarkFilter::DoPrepare()
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::DoPrepare");
    MEDIA_LOG_I("Prepare");
    FALSE_RETURN_V_MSG(filterCallback_ != nullptr, Status::ERROR_UNKNOWN, "filterCallback_ is nullptr");
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

Status WaterMarkFilter::DoStart()
{
    MEDIA_LOG_I("Start");
    return Status::OK;
}

Status WaterMarkFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    return Status::OK;
}

Status WaterMarkFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    return Status::OK;
}

Status WaterMarkFilter::DoStop()
{
    MEDIA_LOG_I("Stop");
    return Status::OK;
}

Status WaterMarkFilter::DoFlush()
{
    return Status::OK;
}

Status WaterMarkFilter::DoRelease()
{
    MediaAVCodec::AVCodecTrace trace("WaterMark::DoRelease");
    MEDIA_LOG_I("DoRelease enter");
    {
        std::unique_lock<std::mutex> lock(releaseMutex_);
        // 使用标志位防止重复释放
        if (hasReleased_) {
            MEDIA_LOG_W("DoRelease already called, skip");
            return Status::OK;
        }
        hasReleased_ = true;
    }
    isAVRecorderMode_.store(false);

    // 先注销回调，防止新回调触发，再停止渲染线程
    if (nativeImage_ != nullptr) {
        OH_NativeImage_UnsetOnFrameAvailableListener(nativeImage_);
    }

    // 停止渲染线程
    StopRenderThread();
    ReleaseCpuResources();

    // 释放Native Image
    if (nativeImage_ != nullptr) {
        OH_NativeImage_Destroy(&nativeImage_);
        nativeImage_ = nullptr;
    }
    nativeImageWindow_ = nullptr;

    // 释放GPU侧资源
    ReleaseGpuResources();

    shaderProgram_.reset();
    watermarkMergeShaderProgram_.reset();
    watermarkOverlayProgram_.reset();
    ReleaseEGLResources();
    nativeWindow_ = nullptr;

    MEDIA_LOG_I("DoRelease success");
    return Status::OK;
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

void WaterMarkFilter::ReleaseCpuResources()
{
    // unique_ptr自动释放buffer内存
    watermarkList_.clear();
}

void WaterMarkFilter::ReleaseGpuResources()
{
    if (vertexArrayObject_ != 0) {
        glDeleteVertexArrays(1, &vertexArrayObject_);
        vertexArrayObject_ = 0;
    }

    if (vertexBufferObject_ != 0) {
        glDeleteBuffers(1, &vertexBufferObject_);
        vertexBufferObject_ = 0;
    }

    if (nativeImageTexId_ != 0 && nativeImageTexId_ != DEFINED_VALUE) {
        glDeleteTextures(1, &nativeImageTexId_);
        nativeImageTexId_ = 0;
    }

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

void WaterMarkFilter::ReleaseEGLResources()
{
    if (renderContext_ != nullptr && eglSurface_ != EGL_NO_SURFACE) {
        renderContext_->DestroyEglSurface(eglSurface_);
        eglSurface_ = EGL_NO_SURFACE;
    }
    renderContext_.reset();
}

Status WaterMarkFilter::NotifyNextFilterEos()
{
    MEDIA_LOG_I("NotifyNextFilterEos");
    for (const auto &iter : nextFiltersMap_) {
        for (const auto &filter : iter.second) {
            std::shared_ptr<Meta> eosMeta = std::make_shared<Meta>();
            eosMeta->Set<Tag::MEDIA_END_OF_STREAM>(true);
            eosMeta->Set<Tag::USER_FRAME_PTS>(eosPts_.load());
            FALSE_RETURN_V_MSG(filter != nullptr, Status::ERROR_UNKNOWN, "filter is nullptr");
            filter->SetParameter(eosMeta);
        }
    }
    return Status::OK;
}

void WaterMarkFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    std::lock_guard<std::mutex> lock(wakeUpMutex_);
    FALSE_RETURN_MSG(parameter != nullptr, "parameter is nullptr");
    int64_t pts = 0;
    parameter->Get<Tag::USER_FRAME_PTS>(pts);
    eosPts_.store(pts);    
    parameter->Get<Tag::USER_PUSH_DATA_TIME>(frameNum_);
    MEDIA_LOG_I("lastBuffer PTS: " PUBLIC_LOG_D64 " frameNum: " PUBLIC_LOG_D64, eosPts_.load(), frameNum_);
    wakeUpCond_.notify_one();
}

void WaterMarkFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
}

Status WaterMarkFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    FALSE_RETURN_V_MSG(nextFilter != nullptr, Status::ERROR_UNKNOWN, "nextFilter");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<WaterMarkFilterLinkCallback>(shared_from_this());
    auto ret = nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    if (ret != Status::OK && eventReceiver_) {
        eventReceiver_->OnEvent({"WaterMarkFilter::LinkNext error", EventType::EVENT_ERROR,
            static_cast<int32_t>(MSERR_UNKNOWN)});
    }
    return ret;
}

Status WaterMarkFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status WaterMarkFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

Status WaterMarkFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    onLinkedResultCallback_ = callback;
    return Status::OK;
}

Status WaterMarkFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}

Status WaterMarkFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void WaterMarkFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnLinkedResult enter");
    FALSE_RETURN_MSG(meta != nullptr, "meta is nullptr");
    if (onLinkedResultCallback_) {
        onLinkedResultCallback_->OnLinkedResult(nullptr, meta);
    }
}

void WaterMarkFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUpdatedResult");
    (void) meta;
}

void WaterMarkFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUnlinkedResult");
    (void) meta;
}

void WaterMarkFilter::OnOutputBufferAvailable(uint32_t index, uint32_t flag)
{
    MEDIA_LOG_D("OnOutputBufferAvailable enter. index: " PUBLIC_LOG_U32, index);
}

void WaterMarkFilter::SetFaultEvent(const std::string &errMsg, int32_t ret)
{
    MEDIA_LOG_I("SetFaultEvent");
}

void WaterMarkFilter::SetFaultEvent(const std::string &errMsg)
{
    MEDIA_LOG_I("SetFaultEvent");
}

void WaterMarkFilter::SetCallingInfo(int32_t appUid, int32_t appPid,
    const std::string &bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;   
}

FilterType WaterMarkFilter::GetFilterType()
{
    MEDIA_LOG_I("GetFilterType");
    return filterType_;
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
    for (int32_t i = 0; i < wmHeight; i++) {
        errno_t err = memcpy_s(static_cast<uint8_t *>(surfaceBuffer->GetVirAddr()) + i * surfaceStride,
            rowStride, srcBuf.get() + i * rowStride, rowStride);
        FALSE_RETURN_V_MSG_E(err == 0, false, "memcpy_s failed at row %{public}d", i);
    }
    return true;
}

std::shared_ptr<AVBuffer> WaterMarkFilter::ReadMergedWatermarkPixels()
{
    MEDIA_LOG_I("ReadMergedWatermarkPixels enter");
    int32_t wmLeft = wmBBoxLeft_;
    int32_t wmTop = wmBBoxTop_;
    int32_t wmWidth = wmBBoxWidth_;
    int32_t wmHeight = wmBBoxHeight_;
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

#undef CHECK_GL_ERROR
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS