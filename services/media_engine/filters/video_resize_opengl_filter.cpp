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

#include "video_resize_opengl_filter.h"
#include "opengl_filter_internal.h"
#include "filter/filter_factory.h"
#include "common/media_core.h"
#include "avcodec_log.h"
#include "external_window.h"
#include "native_window.h"
#include "native_image.h"
#include "avcodec_trace.h"
#include "meta/video_types.h"
#include "surface/native_buffer.h"
#include <unistd.h>
#include <poll.h>
#include <cinttypes>

using OHOS::Media::Pipeline::GlInternal::SetTextureParams2D;
using OHOS::Media::Pipeline::GlInternal::QUAD_INDICES;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "VideoResizeOpenGLFilter" };

/**
 * @brief YUV转换顶点着色器：GLSL ES 3.10，用于 RGB10_A2 → YUV 零拷贝转换通道
 */
static std::string yuvConvertVertexShader = R"delimiter(#version 310 es
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
out vec2 vTexCoord;
void main()
{
    gl_Position = vec4(position, 1.0);
    // outputFBO_中视频正立(FBO顶部=视频顶部)，OES FBO渲染时NDC底部对应DMA buffer row 0，
    // 需翻转Y轴使NDC底部采样FBO顶部(=视频顶部)，保证编码器row 0=视频顶部
    vTexCoord = vec2(texCoord.x, 1.0 - texCoord.y);
}
)delimiter";

/**
 * @brief YUV转换片段着色器（直接BT.2020）：将RGB10_A2纹理转换为YUV并直接写入编码器P010 DMA buffer
 * 使用GL_EXT_YUV_target扩展，通过layout(yuv) out输出YUV格式
 * 直接使用BT.2020系数手写RGB→YUV转换 + limited range压缩，绕过rgb_2_yuv()内建函数
 *
 * BT.2020 luma coefficients: KR=0.2627, KG=0.6780, KB=0.0593
 * Limited range (10-bit): Y∈[64,940], C∈[64,960]
 * 与CPU端转换逻辑使用完全相同的系数和range压缩（零拷贝 GPU 实现取代了原 CPU 路径）
 */
static std::string yuvConvertFragShaderDirect = R"delimiter(#version 310 es
#extension GL_EXT_YUV_target : require
precision highp float;
uniform sampler2D inputTexture;
in vec2 vTexCoord;
layout(yuv) out vec4 vOutColor;

const float KR = 0.2627;
const float KG = 0.6780;
const float KB = 0.0593;

const float Y_RANGE = 876.0 / 1023.0;
const float Y_OFFSET = 64.0 / 1023.0;
const float Y_MAX = 940.0 / 1023.0;
const float C_RANGE = 896.0 / 1023.0;
const float C_OFFSET = 64.0 / 1023.0;
const float C_MAX = 960.0 / 1023.0;

void main()
{
    vec3 rgb = texture(inputTexture, vTexCoord).rgb;

    float yLin = KR * rgb.r + KG * rgb.g + KB * rgb.b;
    float cbLin = (rgb.b - yLin) / (2.0 * (1.0 - KB)) + 0.5;
    float crLin = (rgb.r - yLin) / (2.0 * (1.0 - KR)) + 0.5;

    float y = clamp(yLin * Y_RANGE + Y_OFFSET, Y_OFFSET, Y_MAX);
    float cb = clamp(cbLin * C_RANGE + C_OFFSET, C_OFFSET, C_MAX);
    float cr = clamp(crLin * C_RANGE + C_OFFSET, C_OFFSET, C_MAX);

    vOutColor = vec4(y, cb, cr, 1.0);
}
)delimiter";

/**
 * @brief YUV转换片段着色器（itu_709回退）：当直接BT.2020 shader编译失败时的fallback
 * 使用rgb_2_yuv()内建函数 + BT.709⁻¹×BT.2020预补偿矩阵实现BT.2020转换
 */
static std::string yuvConvertFragShaderFallback = R"delimiter(#version 310 es
#extension GL_EXT_YUV_target : require
precision highp float;
uniform sampler2D inputTexture;
in vec2 vTexCoord;
layout(yuv) out vec4 vOutColor;

const mat3 BT2020_COMPENSATE = mat3(
    1.0501,  0.0547, 0.0036,
   -0.0460,  0.9608, 0.0092,
   -0.0040, -0.0156, 0.9871
);

void main()
{
    vec3 rgb = texture(inputTexture, vTexCoord).rgb;
    vec3 rgbCompensated = BT2020_COMPENSATE * rgb;
    vOutColor = vec4(rgb_2_yuv(rgbCompensated, itu_709), 1.0);
}
)delimiter";
} // namespace

namespace OHOS {
namespace Media {
namespace Pipeline {

VideoResizeOpenGLFilter::VideoResizeOpenGLFilter(std::string name, FilterType type)
    : OpenGLVideoFilterBase(std::move(name), type)
{
    MEDIA_LOG_I("VideoResizeOpenGLFilter create");
}

VideoResizeOpenGLFilter::~VideoResizeOpenGLFilter()
{
    MEDIA_LOG_I("VideoResizeOpenGLFilter destroy");
}

// ---- 工厂注册 ----
static AutoRegisterFilter<VideoResizeOpenGLFilter> g_registerVideoResizeOpenGLFilter(
    "builtin.transcoder.videoresize_opengl",
    FilterType::FILTERTYPE_VIDRESIZE_OPENGL,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<VideoResizeOpenGLFilter>(name, FilterType::FILTERTYPE_VIDRESIZE_OPENGL);
    });

// ---- OpenGLVideoFilterBase virtual hook overrides ----

bool VideoResizeOpenGLFilter::SetupEglContextForMode()
{
    // HDR 模式: 使用 10bit EGL config (RGB10_A2) + pbuffer surface + FBO渲染
    // SDR 模式: 委托给基类 InitializeEGLContext() 使用 8bit + window surface
    if (isHdrMode_) {
        return InitializeEGLContextHdr();
    }
    return InitializeEGLContext();
}

bool VideoResizeOpenGLFilter::InitializeEGLContextHdr()
{
    renderContext_ = std::make_unique<EglRenderContext>();
    FALSE_RETURN_V_MSG(renderContext_ != nullptr, false, "Failed to create EGL render context");

    renderContext_->SetHdrMode(true);
    FALSE_RETURN_V_MSG(renderContext_->Init(), false, "Failed to initialize EGL render context");
    eglSurface_ = renderContext_->CreatePbufferSurface();
    FALSE_RETURN_V_MSG(eglSurface_ != EGL_NO_SURFACE, false, "Failed to create pbuffer surface for HDR mode");
    MEDIA_LOG_I("HDR mode: 10bit pbuffer surface + FBO (zero-copy GPU path)");

    renderContext_->MakeCurrent(eglSurface_);
    return true;
}

bool VideoResizeOpenGLFilter::OnGLResourcesCreated()
{
    // HDR 模式: 创建输出 FBO，用于渲染到 RGB10_A2 纹理后零拷贝转换 P010
    // SDR 模式: 无需额外资源，直接返回 true
    FALSE_RETURN_V_NOLOG(isHdrMode_, true);
    FALSE_RETURN_V_MSG(CreateOutputTexturesAndFBO(), false,
        "Failed to create output textures and FBO for HDR mode");
    // 检测GPU是否支持零拷贝P010输出（GL_EXT_YUV_target + P010 OES纹理FBO）。
    // 不再提供 CPU 回退路径：检测失败则拒绝进入 HDR 模式（OnGLResourcesCreated 返回 false
    // 会中止渲染线程初始化，调用方应回退到 SDR 转码或报错）。
    FALSE_RETURN_V_MSG(DetectZeroCopyCapability(), false,
        "HDR zero-copy not supported on this GPU (GL_EXT_YUV_target / P010 OES FBO unavailable)");
    return true;
}

void VideoResizeOpenGLFilter::ProcessFrame()
{
    // SDR 模式: 委托给基类（UpdateSurfaceImage + DrawImage + SwapBuffers）
    // HDR 模式: 走完整 HDR 管线（AcquireBuffer + FBO渲染 + YUV转换 + 元数据透传）
    if (isHdrMode_) {
        RenderHdrFrame();
    } else {
        // 基类 SDR 路径
        UpdateVideoFrame();
        DrawImage(lastPts_.load());
    }
}

void VideoResizeOpenGLFilter::ConfigureInputSurface()
{
    // HDR 模式: 输入 Surface 配置 P010 像素格式 + HDR 色彩空间
    FALSE_RETURN_NOLOG(isHdrMode_ && nativeImageWindow_ != nullptr);
    MEDIA_LOG_I("HDR mode: configure input surface P010 + colorspace %{public}d", dstColorSpace_);
    OH_NativeWindow_NativeWindowHandleOpt(nativeImageWindow_,
        SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_P010);
    OH_NativeWindow_SetColorSpace(nativeImageWindow_,
        static_cast<OH_NativeBuffer_ColorSpace>(dstColorSpace_));
}

void VideoResizeOpenGLFilter::ConfigureOutputSurface()
{
    // HDR 模式: 设置 native window 格式为 P010 + HDR 色彩空间
    // HDR 渲染路径使用 pbuffer surface + FBO + GPU 直写(零拷贝)或 CPU 回退 + RequestBuffer/FlushBuffer
    // 不使用 window surface（10bit config + P010 window = EGL_BAD_MATCH）
    FALSE_RETURN_NOLOG(isHdrMode_ && nativeWindow_ != nullptr);
    MEDIA_LOG_I("HDR mode: set output surface P010 format + colorspace %{public}d", dstColorSpace_);
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_,
        SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_P010);
    OH_NativeWindow_SetColorSpace(nativeWindow_,
        static_cast<OH_NativeBuffer_ColorSpace>(dstColorSpace_));
}

void VideoResizeOpenGLFilter::OnSetCodecFormat(const std::shared_ptr<Meta> &format)
{
    FALSE_RETURN_NOLOG(format != nullptr);
    (void)format->Get<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(dstColorSpace_);
    isHdrMode_ = IsHdrColorSpace(dstColorSpace_);
    MEDIA_LOG_I("VideoResizeOpenGL OnSetCodecFormat: dstColorSpace=%{public}d, isHdrMode=%{public}d",
        dstColorSpace_, static_cast<int>(isHdrMode_));
}

void VideoResizeOpenGLFilter::OnReleaseGpuResources()
{
    // 释放 HDR 专属 GL 资源（output FBO / texture / YUV shader）
    if (outputTexId_ != 0 && outputTexId_ != DEFINED_VALUE) {
        glDeleteTextures(1, &outputTexId_);
        outputTexId_ = 0;
    }
    if (outputFBO_ != 0) {
        glDeleteFramebuffers(1, &outputFBO_);
        outputFBO_ = 0;
    }
    yuvConvertProgram_.reset();
}

Status VideoResizeOpenGLFilter::DoPrepare()
{
    MediaAVCodec::AVCodecTrace trace("VideoResizeOpenGL::DoPrepare");
    MEDIA_LOG_I("VideoResizeOpenGLFilter::DoPrepare");
    FALSE_RETURN_V_MSG_E(filterCallback_ != nullptr, Status::ERROR_UNKNOWN, "filterCallback_ is nullptr");
    return filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
        StreamType::STREAMTYPE_RAW_VIDEO);
}

// ---- HDR 渲染管线实现 ----

bool VideoResizeOpenGLFilter::IsHdrColorSpace(int32_t colorSpace) const
{
    return colorSpace == static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_FULL) ||
           colorSpace == static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT) ||
           colorSpace == static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_PQ_FULL) ||
           colorSpace == static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_PQ_LIMIT) ||
           colorSpace == static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_HLG_FULL) ||
           colorSpace == static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_HLG_LIMIT) ||
           colorSpace == static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_PQ_FULL) ||
           colorSpace == static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_PQ_LIMIT);
}

bool VideoResizeOpenGLFilter::CreateOutputTexturesAndFBO()
{
    // 创建输出GL_TEXTURE_2D纹理（用于FBO渲染目标，零拷贝路径采样或CPU回退glReadPixels读取）
    glGenTextures(1, &outputTexId_);
    GLenum error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false,
        "glGenTextures for outputTexId_ failed with GL error: %{public}d", error);

    glBindTexture(GL_TEXTURE_2D, outputTexId_);
    SetTextureParams2D(GL_TEXTURE_2D);

    int32_t viewWidth = targetWidth_.load();
    int32_t viewHeight = targetHeight_.load();
    // HDR模式使用10bit纹理(GL_RGB10_A2)保留HDR精度
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, viewWidth, viewHeight,
                 0, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, nullptr);

    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false,
        "CreateOutputTexturesAndFBO: glTexImage2D failed with GL error: %{public}d", error);

    glBindTexture(GL_TEXTURE_2D, 0);

    // 创建输出FBO
    glGenFramebuffers(1, &outputFBO_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false,
        "glGenFramebuffers for outputFBO_ failed with GL error: %{public}d", error);

    MEDIA_LOG_I("CreateOutputTexturesAndFBO success (GL_TEXTURE_2D %dx%d)", viewWidth, viewHeight);
    return true;
}

bool VideoResizeOpenGLFilter::DetectZeroCopyCapability()
{
    // ① 检查GL_EXT_YUV_target扩展是否可用
    const GLubyte *extsRaw = glGetString(GL_EXTENSIONS);
    FALSE_RETURN_V_MSG(extsRaw != nullptr, false,
        "DetectZeroCopy: glGetString returned nullptr, HDR zero-copy unavailable");
    std::string exts = reinterpret_cast<const char *>(extsRaw);
    FALSE_RETURN_V_MSG(exts.find("GL_EXT_YUV_target") != std::string::npos, false,
        "DetectZeroCopy: GL_EXT_YUV_target not supported, HDR zero-copy unavailable");

    // ② 尝试编译YUV转换shader：优先使用直接BT.2020 shader，失败则回退到itu_709+预补偿矩阵
    //    （两个变体均为 GPU 零拷贝路径，仅在 shader 实现上不同）
    yuvConvertProgram_ = std::make_unique<ShaderProgram>(yuvConvertVertexShader, yuvConvertFragShaderDirect);
    if (yuvConvertProgram_ && yuvConvertProgram_->Valid()) {
        useDirectBt2020Shader_ = true;
        MEDIA_LOG_I("DetectZeroCopy: direct BT.2020 YUV shader compiled successfully");
    } else {
        MEDIA_LOG_W("DetectZeroCopy: direct BT.2020 shader failed, trying itu_709 fallback shader");
        yuvConvertProgram_ = std::make_unique<ShaderProgram>(yuvConvertVertexShader, yuvConvertFragShaderFallback);
        if (!yuvConvertProgram_ || !yuvConvertProgram_->Valid()) {
            MEDIA_LOG_E("DetectZeroCopy: YUV shader compilation failed, HDR zero-copy unavailable");
            yuvConvertProgram_.reset();
            return false;
        }
        useDirectBt2020Shader_ = false;
        MEDIA_LOG_I("DetectZeroCopy: itu_709 + precompensation shader compiled successfully");
    }

    // ③ 尝试创建P010 OES纹理FBO（用一个测试buffer验证FBO完整性）
    OHNativeWindowBuffer *testBuffer = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow_, &testBuffer, &fenceFd);
    FALSE_RETURN_V_MSG(ret == 0 && testBuffer != nullptr, false,
        "DetectZeroCopy: RequestBuffer failed ret=%{public}d, HDR zero-copy unavailable", ret);
    if (fenceFd > 0) {
        close(fenceFd);
    }

    EGLImageKHR testImg = renderContext_->CreateEGLImage(testBuffer);
    if (testImg == EGL_NO_IMAGE_KHR) {
        MEDIA_LOG_E("DetectZeroCopy: CreateEGLImage failed, HDR zero-copy unavailable");
        NativeWindowCancelBuffer(nativeWindow_, testBuffer);
        return false;
    }

    GLuint testTex = 0;
    glGenTextures(1, &testTex);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, testTex);
    renderContext_->EGLImageTargetTexture2DOES(testImg);
    GLenum glErr = glGetError();

    GLuint testFbo = 0;
    glGenFramebuffers(1, &testFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, testFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_EXTERNAL_OES, testTex, 0);
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    bool capable = (glErr == GL_NO_ERROR && fboStatus == GL_FRAMEBUFFER_COMPLETE);

    // 清理测试资源
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &testFbo);
    glDeleteTextures(1, &testTex);
    renderContext_->DeleteEGLImage(testImg);
    NativeWindowCancelBuffer(nativeWindow_, testBuffer);

    MEDIA_LOG_I("DetectZeroCopy: P010 YUV render target %{public}s",
        capable ? "CAPABLE (zero-copy enabled)" : "NOT capable (HDR zero-copy unavailable)");
    return capable;
}

void VideoResizeOpenGLFilter::ConfigureHdrSurface()
{
    FALSE_RETURN_NOLOG(isHdrMode_);
    MEDIA_LOG_I("ConfigureHdrSurface: input P010 + output P010 + colorspace %{public}d", dstColorSpace_);
    if (nativeImageWindow_ != nullptr) {
        OH_NativeWindow_NativeWindowHandleOpt(nativeImageWindow_,
            SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_P010);
        OH_NativeWindow_SetColorSpace(nativeImageWindow_,
            static_cast<OH_NativeBuffer_ColorSpace>(dstColorSpace_));
    }
    if (nativeWindow_ != nullptr) {
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_,
            SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_P010);
        OH_NativeWindow_SetColorSpace(nativeWindow_,
            static_cast<OH_NativeBuffer_ColorSpace>(dstColorSpace_));
    }
}

void VideoResizeOpenGLFilter::RenderHdrFrame()
{
    MediaAVCodec::AVCodecTrace trace("VideoResizeOpenGL::RenderHdrFrame");
    FALSE_RETURN_MSG(renderContext_ != nullptr, "renderContext_ is null");
    FALSE_RETURN_MSG(nativeImage_ != nullptr, "nativeImage_ is nullptr");
    FALSE_RETURN_MSG(nativeWindow_ != nullptr, "nativeWindow_ is nullptr");

    // 1. 通过 AcquireNativeWindowBuffer 获取输入帧（保留 NativeBuffer 用于读取 HDR 元数据）
    OHNativeWindowBuffer *inBuffer = nullptr;
    int64_t pts = 0;
    if (!AcquireInputBufferWithMetadata(inBuffer, pts)) {
        return;
    }
    lastPts_.store(pts);
    MEDIA_LOG_D("RenderHdrFrame pts %{public}" PRId64, lastPts_.load());

    // 2. 从输入 buffer 读取 HDR 帧级元数据（仅 HDR2HDR 模式）
    ReadHdrMetadataFromBuffer(inBuffer);

    // 3. 将输入 buffer 绑定为 OES 纹理（替代 OH_NativeImage_UpdateSurfaceImage）
    renderContext_->MakeCurrent(eglSurface_);
    EGLImageKHR inImg = renderContext_->CreateEGLImage(inBuffer);
    if (inImg == EGL_NO_IMAGE_KHR) {
        MEDIA_LOG_E("RenderHdrFrame: CreateEGLImage for input buffer failed");
        ReleaseInputBuffer(inBuffer);
        return;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    renderContext_->EGLImageTargetTexture2DOES(inImg);
    SetTextureParams2D(GL_TEXTURE_EXTERNAL_OES);
    renderContext_->DeleteEGLImage(inImg);  // 纹理已引用底层 buffer，可释放 EGLImage

    int32_t viewWidth = targetWidth_.load();
    int32_t viewHeight = targetHeight_.load();

    // 4. 绑定输出FBO，渲染合成帧（视频 + 水印）到RGB10_A2纹理
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, outputTexId_, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        MEDIA_LOG_E("RenderHdrFrame: output FBO not complete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        ReleaseInputBuffer(inBuffer);
        return;
    }

    glViewport(0, 0, viewWidth, viewHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(vertexArrayObject_);

    // 渲染视频到 outputFBO_（VideoResizeOpenGLFilter 为纯降分辨率 filter，无水印叠加）
    RenderVideoFrame();

    MEDIA_LOG_I("RenderHdrFrame: rendered to outputFBO_, directBt2020=%{public}d", useDirectBt2020Shader_);

    // 5. 零拷贝路径：GPU 直写编码器 DMA buffer（输出路径内部会调用 WriteHdrMetadataToBuffer）。
    //    能力检测在 OnGLResourcesCreated 阶段已完成，运行时不再降级。
    MEDIA_LOG_D("RenderHdrFrame: zero-copy path (%{public}s)",
        useDirectBt2020Shader_ ? "direct BT.2020" : "itu_709+precomp");
    RenderHdrFrameZeroCopy(viewWidth, viewHeight, pts);

    // 6. 释放输入 buffer（渲染已完成，纹理不再需要）
    ReleaseInputBuffer(inBuffer);
}

bool VideoResizeOpenGLFilter::SetupYuvFbo(OHNativeWindowBuffer *outBuffer, GLuint &outTex,
    GLuint &yuvFbo, EGLImageKHR &outImg)
{
    outTex = 0;
    yuvFbo = 0;
    outImg = EGL_NO_IMAGE_KHR;

    // 编码器DMA buffer → EGLImage → OES纹理 → YUV FBO（零拷贝渲染目标）
    outImg = renderContext_->CreateEGLImage(outBuffer);
    if (outImg == EGL_NO_IMAGE_KHR) {
        MEDIA_LOG_E("RenderHdrFrameZeroCopy: CreateEGLImage failed");
        return false;
    }

    glGenTextures(1, &outTex);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, outTex);
    renderContext_->EGLImageTargetTexture2DOES(outImg);
    SetTextureParams2D(GL_TEXTURE_EXTERNAL_OES);
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        MEDIA_LOG_E("RenderHdrFrameZeroCopy: OES texture bind failed, GL error: %{public}d, drop frame", glErr);
        glDeleteTextures(1, &outTex);
        renderContext_->DeleteEGLImage(outImg);
        outTex = 0;
        outImg = EGL_NO_IMAGE_KHR;
        return false;
    }

    glGenFramebuffers(1, &yuvFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, yuvFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_EXTERNAL_OES, outTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        MEDIA_LOG_E("RenderHdrFrameZeroCopy: YUV FBO not complete, drop frame");
        ReleaseYuvFboResources(outTex, yuvFbo, outImg);
        outTex = 0;
        yuvFbo = 0;
        outImg = EGL_NO_IMAGE_KHR;
        return false;
    }
    return true;
}

void VideoResizeOpenGLFilter::ReleaseYuvFboResources(GLuint outTex, GLuint yuvFbo, EGLImageKHR outImg)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &yuvFbo);
    glDeleteTextures(1, &outTex);
    renderContext_->DeleteEGLImage(outImg);
}

void VideoResizeOpenGLFilter::RenderHdrFrameZeroCopy(int32_t viewWidth, int32_t viewHeight, int64_t pts)
{
    MediaAVCodec::AVCodecTrace trace("VideoResizeOpenGL::RenderHdrFrameZeroCopy");

    // 解绑outputFBO_（后续需要采样outputTexId_作为纹理输入）
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 1. 设置 timestamp（必须在 RequestBuffer 之前，dequeue 时附加到 buffer->uiTimestamp）
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_UI_TIMESTAMP, pts);

    // 2. 从编码器surface请求P010 buffer（dequeue 时将 pts 附加到 buffer->uiTimestamp）
    OHNativeWindowBuffer *outBuffer = nullptr;
    if (!RequestEncoderBuffer(outBuffer)) {
        return;
    }

    // 3. 编码器DMA buffer → EGLImage → OES纹理 → YUV FBO（零拷贝渲染目标）
    GLuint outTex = 0;
    GLuint yuvFbo = 0;
    EGLImageKHR outImg = EGL_NO_IMAGE_KHR;
    if (!SetupYuvFbo(outBuffer, outTex, yuvFbo, outImg)) {
        NativeWindowCancelBuffer(nativeWindow_, outBuffer);
        return;
    }

    // 4. Fragment Shader: RGB10_A2 → YUV，GPU直接写入编码器DMA buffer
    glViewport(0, 0, viewWidth, viewHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glBindVertexArray(vertexArrayObject_);

    yuvConvertProgram_->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outputTexId_);  // RGB10_A2 结果
    yuvConvertProgram_->SetInt("inputTexture", 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, QUAD_INDICES);

    glFinish();  // 确保GPU写入DMA buffer完成

    // 5. 清理临时GL资源（DMA buffer已包含P010数据，无需CPU读取）
    ReleaseYuvFboResources(outTex, yuvFbo, outImg);

    // 6. 将 HDR 帧级元数据写入输出 buffer
    WriteHdrMetadataToBuffer(outBuffer);

    // 7. 提交buffer给编码器（无CPU转换，无glReadPixels）
    FlushEncoderBuffer(outBuffer, viewWidth, viewHeight);
}

bool VideoResizeOpenGLFilter::RequestEncoderBuffer(OHNativeWindowBuffer *&outBuffer)
{
    outBuffer = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow_, &outBuffer, &fenceFd);
    FALSE_RETURN_V_MSG(ret == 0 && outBuffer != nullptr, false,
        "RequestEncoderBuffer failed, ret: %{public}d", ret);
    if (fenceFd > 0) {
        close(fenceFd);
    }
    return true;
}

void VideoResizeOpenGLFilter::FlushEncoderBuffer(OHNativeWindowBuffer *outBuffer,
    int32_t viewWidth, int32_t viewHeight)
{
    Region region;
    Region::Rect rect = {0, 0, static_cast<uint32_t>(viewWidth), static_cast<uint32_t>(viewHeight)};
    region.rects = &rect;
    region.rectNumber = 1;
    int32_t ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, outBuffer, -1, region);
    if (ret != 0) {
        MEDIA_LOG_E("FlushEncoderBuffer failed, ret: %{public}d", ret);
    }
}

bool VideoResizeOpenGLFilter::AcquireInputBufferWithMetadata(OHNativeWindowBuffer *&inBuffer, int64_t &pts)
{
    inBuffer = nullptr;
    int32_t inFenceFd = -1;
    int32_t ret = OH_NativeImage_AcquireNativeWindowBuffer(nativeImage_, &inBuffer, &inFenceFd);
    FALSE_RETURN_V_MSG(ret == 0 && inBuffer != nullptr, false,
        "AcquireInputBufferWithMetadata: AcquireNativeWindowBuffer failed, ret: %{public}d", ret);
    pts = OH_NativeImage_GetTimestamp(nativeImage_);

    // 等待 input fence 完成（解码器写完输入 buffer）
    if (inFenceFd != -1) {
        struct pollfd pollFd = {};
        pollFd.fd = inFenceFd;
        pollFd.events = POLLIN;
        int32_t retCode = -1;
        do {
            retCode = poll(&pollFd, 1, 3000);  // 3s 超时
        } while (retCode == -1 && (errno == EINTR || errno == EAGAIN));
        close(inFenceFd);
    }

    // 增加引用计数，防止 buffer 在渲染期间被回收
    ret = OH_NativeWindow_NativeObjectReference(inBuffer);
    if (ret != 0) {
        MEDIA_LOG_E("AcquireInputBufferWithMetadata: NativeObjectReference failed, ret: %{public}d", ret);
        OH_NativeImage_ReleaseNativeWindowBuffer(nativeImage_, inBuffer, -1);
        inBuffer = nullptr;
        return false;
    }
    return true;
}

void VideoResizeOpenGLFilter::ReadSingleMetadata(OH_NativeBuffer *nativeBuffer,
    OH_NativeBuffer_MetadataKey key, std::vector<uint8_t> &dest)
{
    int32_t size = 0;
    uint8_t *data = nullptr;
    if (OH_NativeBuffer_GetMetadataValue(nativeBuffer, key, &size, &data) == 0 &&
        size > 0 && data != nullptr) {
        dest.assign(data, data + static_cast<size_t>(size));
        delete[] data;
        data = nullptr;
    }
}

void VideoResizeOpenGLFilter::ReadHdrMetadataFromBuffer(OHNativeWindowBuffer *inBuffer)
{
    cachedHdrMetaData_.valid = false;
    cachedHdrMetaData_.dynamicMetadata.clear();
    cachedHdrMetaData_.staticMetadata.clear();
    cachedHdrMetaData_.metadataType.clear();

    // 仅 HDR2HDR 模式需要透传帧级 HDR 元数据，HDR2SDR / SDR2SDR 无需处理
    FALSE_RETURN_NOLOG(isHdrMode_ && inBuffer != nullptr);

    // 使用 OH_NativeBuffer C API 读取元数据（避免依赖 HDI 生成头文件）
    OH_NativeBuffer *nativeBuffer = nullptr;
    if (OH_NativeBuffer_FromNativeWindowBuffer(inBuffer, &nativeBuffer) != 0 || nativeBuffer == nullptr) {
        MEDIA_LOG_E("ReadHdrMetadataFromBuffer: OH_NativeBuffer_FromNativeWindowBuffer failed");
        return;
    }

    ReadSingleMetadata(nativeBuffer, OH_HDR_DYNAMIC_METADATA, cachedHdrMetaData_.dynamicMetadata);
    ReadSingleMetadata(nativeBuffer, OH_HDR_STATIC_METADATA, cachedHdrMetaData_.staticMetadata);
    ReadSingleMetadata(nativeBuffer, OH_HDR_METADATA_TYPE, cachedHdrMetaData_.metadataType);

    cachedHdrMetaData_.valid = !cachedHdrMetaData_.dynamicMetadata.empty() ||
                               !cachedHdrMetaData_.staticMetadata.empty() ||
                               !cachedHdrMetaData_.metadataType.empty();
    MEDIA_LOG_I("ReadHdrMetadataFromBuffer: dynamic=%{public}zu static=%{public}zu type=%{public}zu valid=%{public}d",
        cachedHdrMetaData_.dynamicMetadata.size(),
        cachedHdrMetaData_.staticMetadata.size(),
        cachedHdrMetaData_.metadataType.size(),
        cachedHdrMetaData_.valid);
}

void VideoResizeOpenGLFilter::WriteSingleMetadata(OH_NativeBuffer *nativeBuffer,
    OH_NativeBuffer_MetadataKey key, std::vector<uint8_t> &src, const char *metadataName)
{
    if (src.empty()) {
        return;
    }
    int32_t ret = OH_NativeBuffer_SetMetadataValue(nativeBuffer, key,
        static_cast<int32_t>(src.size()), src.data());
    if (ret != 0) {
        MEDIA_LOG_W("WriteHdrMetadataToBuffer: SetMetadataValue %{public}s failed, ret=%{public}d",
            metadataName, ret);
    }
}

void VideoResizeOpenGLFilter::WriteHdrMetadataToBuffer(OHNativeWindowBuffer *outBuffer)
{
    // 仅 HDR2HDR 模式且有有效元数据时写入，HDR2SDR / SDR2SDR 无需处理
    FALSE_RETURN_NOLOG(isHdrMode_ && cachedHdrMetaData_.valid && outBuffer != nullptr);

    // 使用 OH_NativeBuffer C API 写入元数据（避免依赖 HDI 生成头文件）
    OH_NativeBuffer *nativeBuffer = nullptr;
    if (OH_NativeBuffer_FromNativeWindowBuffer(outBuffer, &nativeBuffer) != 0 || nativeBuffer == nullptr) {
        MEDIA_LOG_E("WriteHdrMetadataToBuffer: OH_NativeBuffer_FromNativeWindowBuffer failed");
        return;
    }

    WriteSingleMetadata(nativeBuffer, OH_HDR_DYNAMIC_METADATA,
        cachedHdrMetaData_.dynamicMetadata, "OH_HDR_DYNAMIC_METADATA");
    WriteSingleMetadata(nativeBuffer, OH_HDR_STATIC_METADATA,
        cachedHdrMetaData_.staticMetadata, "OH_HDR_STATIC_METADATA");
    WriteSingleMetadata(nativeBuffer, OH_HDR_METADATA_TYPE,
        cachedHdrMetaData_.metadataType, "OH_HDR_METADATA_TYPE");

    MEDIA_LOG_I("WriteHdrMetadataToBuffer: HDR metadata written to output buffer "
        "(dynamic=%{public}zu static=%{public}zu type=%{public}zu)",
        cachedHdrMetaData_.dynamicMetadata.size(),
        cachedHdrMetaData_.staticMetadata.size(),
        cachedHdrMetaData_.metadataType.size());
}

void VideoResizeOpenGLFilter::ReleaseInputBuffer(OHNativeWindowBuffer *inBuffer)
{
    FALSE_RETURN_NOLOG(inBuffer != nullptr);
    OH_NativeWindow_NativeObjectUnreference(inBuffer);
    OH_NativeImage_ReleaseNativeWindowBuffer(nativeImage_, inBuffer, -1);
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS
