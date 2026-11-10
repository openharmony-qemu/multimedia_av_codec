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

#include "opengl_video_filter_base.h"
#include "opengl_filter_internal.h"
#include "filter/filter_factory.h"
#include "common/media_core.h"
#include "avcodec_log.h"
#include "external_window.h"
#include "native_window.h"
#include "native_image.h"
#include "avcodec_trace.h"
#include "avcodec_sysevent.h"
#include "surface/native_buffer.h"
#include <unistd.h>
#include <cinttypes>

using OHOS::Media::Pipeline::GlInternal::SetTextureParams2D;
using OHOS::Media::Pipeline::GlInternal::QUAD_INDICES;
using OHOS::Media::Pipeline::GlInternal::SHARED_VERTEX_SHADER;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "OpenGLVideoFilterBase" };

/**
 * @brief 视频直通片段着色器：仅采样视频 OES 纹理
 */
static std::string videoFragShader = R"delimiter(
#extension GL_OES_EGL_image_external : require
precision highp float;

varying vec2 vVideoTexCoord;
varying vec2 vWmTexCoord;

uniform samplerExternalOES texture;

void main()
{
    // OES纹理V=0在顶部(图片坐标系)，需翻转Y轴使视频正立显示；
    // X轴已由顶点texCoord补偿，此处不翻转X轴
    vec2 videoTexCoord = vec2(vVideoTexCoord.x, 1.0 - vVideoTexCoord.y);
    vec4 videoColor = texture2D(texture, videoTexCoord);
    gl_FragColor = vec4(videoColor.rgb, 1.0);
}
)delimiter";

// 顶点数据：位置坐标(x, y, z) + 纹理坐标(u, v)
static GLfloat vertices[] = {
    -1.0f, 1.0f,  0.0f,    0.0f, 1.0f,  // 左上角
    -1.0f, -1.0f, 0.0f,    0.0f, 0.0f,  // 左下角
    1.0f,  -1.0f, 0.0f,    1.0f, 0.0f,  // 右下角
    1.0f,  1.0f,  0.0f,    1.0f, 1.0f,  // 右上角
};
} // namespace

namespace OHOS {
namespace Media {
namespace Pipeline {

/**
 * @brief 通用 Filter 链接回调适配器，持有 weak_ptr<OpenGLVideoFilterBase>，
 * 将 FilterLinkCallback 转发到基类的 OnLinkedResult/OnUpdatedResult/OnUnlinkedResult 虚函数。
 */
class OpenGLVideoFilterLinkCallback : public FilterLinkCallback {
public:
    explicit OpenGLVideoFilterLinkCallback(std::shared_ptr<OpenGLVideoFilterBase> filter)
        : filter_(std::move(filter))
    {
    }

    ~OpenGLVideoFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        auto filter = filter_.lock();
        FALSE_RETURN_MSG(filter != nullptr, "filter is nullptr");
        filter->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        auto filter = filter_.lock();
        FALSE_RETURN_MSG(filter != nullptr, "filter is nullptr");
        filter->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        auto filter = filter_.lock();
        FALSE_RETURN_MSG(filter != nullptr, "filter is nullptr");
        filter->OnUpdatedResult(meta);
    }
private:
    std::weak_ptr<OpenGLVideoFilterBase> filter_;
};

OpenGLVideoFilterBase::OpenGLVideoFilterBase(std::string name, FilterType type) : Filter(std::move(name), type)
{
    filterType_ = type;
    // 初始化为单位矩阵
    int matrixStep = 4;
    for (int i = 0; i < matrixStep; ++i) {
        matrix_[i * matrixStep + i] = 1.0f;
    }
    MEDIA_LOG_I("OpenGLVideoFilterBase create");
}

OpenGLVideoFilterBase::~OpenGLVideoFilterBase()
{
    MEDIA_LOG_I("OpenGLVideoFilterBase destroy");
    DoRelease();
}

bool OpenGLVideoFilterBase::CreateNativeImage()
{
    MediaAVCodec::AVCodecTrace trace("OpenGLVideoFilterBase::CreateNativeImage");
    nativeImage_ = OH_NativeImage_Create(-1, GL_TEXTURE_EXTERNAL_OES);
    FALSE_RETURN_V_MSG(nativeImage_ != nullptr, false, "Failed to create native image");
    nativeImageWindow_ = OH_NativeImage_AcquireNativeWindow(nativeImage_);
    if (nativeImageWindow_ == nullptr) {
        MEDIA_LOG_E("Failed to acquire native window, destroying nativeImage");
        OH_NativeImage_Destroy(&nativeImage_);
        return false;
    }

    // 子类（VideoResizeOpenGLFilter）可在 ConfigureInputSurface 中配置 P010 + HDR 色彩空间
    ConfigureInputSurface();

    nativeImageFrameAvailableListener_ = std::make_unique<OH_OnFrameAvailableListener>();
    nativeImageFrameAvailableListener_->context = this;
    nativeImageFrameAvailableListener_->onFrameAvailable = &OpenGLVideoFilterBase::OnNativeImageFrameAvailable;
    int32_t ret = OH_NativeImage_SetOnFrameAvailableListener(nativeImage_, *nativeImageFrameAvailableListener_);
    if (ret != 0) {
        MEDIA_LOG_E("Failed to set frame available listener: %{public}d, destroying nativeImage", ret);
        OH_NativeImage_Destroy(&nativeImage_);
        nativeImageWindow_ = nullptr;
        return false;
    }
    return true;
}

void OpenGLVideoFilterBase::UpdateNativeWindow(void *window, uint64_t width, uint64_t height)
{
    auto nativeWindow = reinterpret_cast<OHNativeWindow *>(window);
    nativeWindow_ = nativeWindow;
    FALSE_RETURN_MSG(nativeWindow_ != nullptr, "nativeWindow_ is nullptr");
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_BUFFER_GEOMETRY, static_cast<int>(width),
        static_cast<int>(height));
}

void OpenGLVideoFilterBase::RenderThreadEntry()
{
    MediaAVCodec::AVCodecTrace trace("OpenGLVideoFilterBase::RenderThreadEntry");
    MEDIA_LOG_I("OpenGLVideoFilterBase::RenderThreadEntry");
    if (InitRenderResources()) {
        RenderLoop();
    } else {
        running_.store(false);
    }
    CleanupRenderResources();
}

bool OpenGLVideoFilterBase::CheckInitStep(bool result, const std::string &errMsg, int32_t errorCode)
{
    if (!result) {
        ReportInitError(errMsg, errorCode);
    }
    return result;
}

void OpenGLVideoFilterBase::AttachNativeImage()
{
    FALSE_RETURN_NOLOG(nativeImage_ != nullptr);
    int32_t attachRet = OH_NativeImage_AttachContext(nativeImage_, nativeImageTexId_);
    if (attachRet != 0) {
        MEDIA_LOG_E("OH_NativeImage_AttachContext failed, ret: %{public}d", attachRet);
    } else {
        MEDIA_LOG_I("OH_NativeImage_AttachContext success, texId: %{public}u", nativeImageTexId_);
    }
}

bool OpenGLVideoFilterBase::InitRenderResources()
{
    // 任一初始化步骤失败均通过 ReportInitError 上报 EVENT_ERROR，使上层收到错误并通知应用。
    // 子类 GL 资源创建扩展点：WaterMark 创建水印 shader/纹理/merge；HDR 创建 output FBO/YUV shader/零拷贝检测。
    return CheckInitStep(SetupEglContextForMode(), "Failed to initialize EGL context", MSERR_UNKNOWN)
        && CheckInitStep(CreateGLResources(), "Failed to create GL resources", MSERR_UNKNOWN)
        && CheckInitStep(OnGLResourcesCreated(), "OnGLResourcesCreated failed", MSERR_VID_RESIZE_FAILED)
        && CheckInitStep(CreateVideoTexture(), "Failed to create video texture", MSERR_UNKNOWN)
        && (AttachNativeImage(), true)  // 非致命：将 NativeImage 绑定到 OES 纹理，失败仅日志
        && CheckInitStep(SetupShaderUniforms(), "Failed to setup shader uniforms", MSERR_UNKNOWN);
}

void OpenGLVideoFilterBase::CleanupRenderResources()
{
    // GL 资源必须在 EGL 上下文所在的线程释放，且在 ReleaseEGLResources (eglTerminate) 之前完成，
    // 否则 EGL display 失效后 glDeleteProgram/glDeleteTextures 等调用将静默失败导致 GPU 对象泄漏。
    OnRenderThreadExit();
    ReleaseGpuResources();
    shaderProgram_.reset();
    OnReleaseShaders();
    // 销毁 NativeImage（内部需要 detach GL context / 销毁 EGLImage），
    // 必须在 EGL display 仍有效时执行，否则关联的 buffer queue 缓冲区会泄漏。
    if (nativeImage_ != nullptr) {
        OH_NativeImage_UnsetOnFrameAvailableListener(nativeImage_);
        OH_NativeImage_Destroy(&nativeImage_);
        nativeImage_ = nullptr;
    }
    nativeImageWindow_ = nullptr;
    ReleaseEGLResources();
}

bool OpenGLVideoFilterBase::InitializeEGLContext()
{
    // SDR 基类: 使用8bit EGL config + window surface + SwapBuffers路径
    // HDR 子类通过 SetupEglContextForMode override 使用 10bit pbuffer surface + FBO
    renderContext_ = std::make_unique<EglRenderContext>();
    FALSE_RETURN_V_MSG(renderContext_ != nullptr, false, "Failed to create EGL render context");

    renderContext_->SetHdrMode(false);
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

void OpenGLVideoFilterBase::DrawImage(const int64_t pts)
{
    MediaAVCodec::AVCodecTrace trace("OpenGLVideoFilterBase::DrawImage");
    FALSE_RETURN_MSG(shaderProgram_ != nullptr, "shaderProgram_ is nullptr");
    int32_t viewWidth = targetWidth_.load();
    int32_t viewHeight = targetHeight_.load();
    glViewport(0, 0, viewWidth, viewHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glBindVertexArray(vertexArrayObject_);
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_UI_TIMESTAMP, pts);

    // 通过虚函数分发：基类仅渲染视频直通，WaterMark 子类增加水印 overlay
    RenderFrame(viewWidth, viewHeight);

    FALSE_RETURN_MSG(nativeWindow_ != nullptr, "nativeWindow_ is nullptr");
    SwapFrameBuffers();
}

void OpenGLVideoFilterBase::RenderFrame(int32_t viewWidth, int32_t viewHeight)
{
    // 基类默认渲染: 仅视频直通（无水印 overlay）
    // WaterMark 子类 override 此方法: RenderVideoFrame() + RenderWatermarkOverlay()
    (void)viewWidth;
    (void)viewHeight;
    RenderVideoFrame();
}

void OpenGLVideoFilterBase::ProcessFrame()
{
    // 基类默认: UpdateSurfaceImage + DrawImage (window surface + SwapBuffers)
    // HDR 子类 override 此方法走 RenderHdrFrame (AcquireBuffer + FBO + GPU直写/CPU回退)
    UpdateVideoFrame();
    DrawImage(lastPts_.load());
}

void OpenGLVideoFilterBase::SwapFrameBuffers()
{
    FALSE_RETURN_MSG(renderContext_ != nullptr, "renderContext_ is nullptr");
    renderContext_->SwapBuffers(eglSurface_);
}

bool OpenGLVideoFilterBase::CreateVideoTexture()
{
    glGenTextures(1, &nativeImageTexId_);
    CHECK_GL_ERROR("glGenTextures failed with GL error");

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    CHECK_GL_ERROR("glBindTexture failed with GL error");

    SetTextureParams2D(GL_TEXTURE_EXTERNAL_OES);

    CHECK_GL_ERROR("Failed to create native image texture");
    return true;
}

bool OpenGLVideoFilterBase::SetupShaderUniforms()
{
    FALSE_RETURN_V_MSG(shaderProgram_ != nullptr, false, "shaderProgram_ is nullptr");

    // 视频直通着色器 - 仅采样视频纹理
    shaderProgram_->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    shaderProgram_->SetInt("texture", 0);

    // 子类扩展点: WaterMark 子类设置水印 overlay shader uniform
    OnSetupOverlayUniforms();

    MEDIA_LOG_I("SetupShaderUniforms video: %{public}dx%{public}d -> target: %{public}dx%{public}d",
        videoWidth_.load(), videoHeight_.load(), targetWidth_.load(), targetHeight_.load());

    return true;
}

void OpenGLVideoFilterBase::RenderLoop()
{
    while (running_) {
        MEDIA_LOG_D("RenderLoop availableFrameCnt_");
        std::unique_lock<std::mutex> lock(wakeUpMutex_);
        wakeUpCond_.wait(lock, [this]() {
            return availableFrameCnt_.load() > 0 || eosReceived_.load() || !running_;
        });
        if (availableFrameCnt_.load() > 0) {
            availableFrameCnt_.fetch_sub(1, std::memory_order_relaxed);
            currentFrameNum_.fetch_add(1, std::memory_order_relaxed);
            lock.unlock();

            // SDR 和 HDR 均通过 ProcessFrame 虚函数分发：
            // SDR: UpdateVideoFrame() + DrawImage() (window surface + SwapBuffers)
            // HDR: RenderHdrFrame() (pbuffer + FBO + GPU直写/CPU回退)
            ProcessFrame();
        }

        // EOS检测：使用独立标志位，避免PTS比较的竞态条件
        if (eosReceived_.load() && availableFrameCnt_.load() <= 0) {
            MEDIA_LOG_I("RenderLoop EOS received, all frames processed");
            NotifyNextFilterEos();
            break;
        }
    }
}

void OpenGLVideoFilterBase::UpdateVideoFrame()
{
    FALSE_RETURN_MSG(renderContext_ != nullptr, "renderContext_ is null");
    // 确保EGL上下文和表面在当前线程生效
    renderContext_->MakeCurrent(eglSurface_);
    FALSE_RETURN_MSG(nativeImage_ != nullptr, "nativeImage_ is nullptr");
    OH_NativeImage_UpdateSurfaceImage(nativeImage_);
    lastPts_.store(OH_NativeImage_GetTimestamp(nativeImage_));
    MEDIA_LOG_D("UpdateVideoFrame pts %{public}" PRId64, lastPts_.load());
}

bool OpenGLVideoFilterBase::CreateGLResources()
{
    // 创建视频直通着色器程序（仅采样视频纹理）
    shaderProgram_ = std::make_unique<ShaderProgram>(SHARED_VERTEX_SHADER, videoFragShader);
    FALSE_RETURN_V_MSG(shaderProgram_->Valid(), false, "Failed to create video shader program");

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

void OpenGLVideoFilterBase::OnNativeImageFrameAvailable(void *data)
{
    MEDIA_LOG_D("OnNativeImageFrameAvailable");
    auto renderThread = reinterpret_cast<OpenGLVideoFilterBase *>(data);
    FALSE_RETURN_MSG(renderThread != nullptr, "renderThread is nullptr");
    // 渲染线程已退出（初始化失败或 DoStop 后），拒绝新帧避免 buffer queue 无限增长。
    // 上层通过 EVENT_ERROR 主动 Pause pipeline 后，解码器将停止送帧。
    FALSE_RETURN_MSG(renderThread->running_.load(), "render thread not running, drop frame");
    std::unique_lock<std::mutex> lock(renderThread->wakeUpMutex_);
    renderThread->availableFrameCnt_.fetch_add(1, std::memory_order_relaxed);
    renderThread->wakeUpCond_.notify_one();
}

Status OpenGLVideoFilterBase::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    MEDIA_LOG_I("SetCodecFormat");
    // HDR 子类通过 OnSetCodecFormat override 读取色彩空间并设置 isHdrMode_
    OnSetCodecFormat(format);
    return Status::OK;
}

void OpenGLVideoFilterBase::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status OpenGLVideoFilterBase::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    configureParameter_ = parameter;
    return Status::OK;
}

sptr<Surface> OpenGLVideoFilterBase::GetInputSurface()
{
    MEDIA_LOG_I("GetInputSurface");
    std::lock_guard<std::mutex> lock(releaseMutex_);
    FALSE_RETURN_V_MSG(nativeImageWindow_ != nullptr, nullptr, "nativeImageWindow_ is nullptr");

    // HDR 子类可在 ConfigureInputSurface 中补配置 P010 + HDR 色彩空间
    // （因 SetCodecFormat 可能在 CreateNativeImage 之后调用）
    ConfigureInputSurface();

    return nativeImageWindow_->surface;
}

Status OpenGLVideoFilterBase::SetOutputSurface(sptr<Surface> surface, int32_t width, int32_t height, int32_t rotation)
{
    MediaAVCodec::AVCodecTrace trace("OpenGLVideoFilterBase::SetOutputSurface");
    std::unique_lock<std::mutex> lock(releaseMutex_);
    MEDIA_LOG_I("SetOutputSurface %{public}d, %{public}d", width, height);
    targetWidth_.store(width);
    targetHeight_.store(height);
    rotation_.store(rotation);
    FALSE_RETURN_V_MSG(surface != nullptr, Status::ERROR_UNKNOWN, "surface is null");
    auto window = CreateNativeWindowFromSurface(&surface);
    UpdateNativeWindow(window, width, height);

    // HDR 子类可在 ConfigureOutputSurface 中设置 native window 格式为 P010 + HDR 色彩空间
    // HDR 渲染路径使用 pbuffer surface + FBO + GPU 直写(零拷贝)或 CPU 回退。
    ConfigureOutputSurface();

    // 首次调用时启动渲染线程，确保帧到达前线程已就绪
    if (!thread_.joinable()) {
        StartRenderThread();
    }
    return Status::OK;
}

Status OpenGLVideoFilterBase::SetVideoResize(int32_t width, int32_t height)
{
    FALSE_RETURN_V_MSG(width > 0 && height > 0, Status::ERROR_INVALID_PARAMETER, "Invalid resize dimensions");
    videoWidth_.store(width);
    videoHeight_.store(height);
    targetHeight_.store(height);
    targetWidth_.store(width);
    MEDIA_LOG_I("SetVideoResize videoWidth_: %{public}d videoHeight_: %{public}d",
        videoWidth_.load(), videoHeight_.load());
    InitOpenGl();
    return Status::OK;
}

void OpenGLVideoFilterBase::InitOpenGl()
{
    FALSE_RETURN_MSG(nativeImage_ == nullptr, "CreateNativeImage success");
    FALSE_RETURN_MSG(CreateNativeImage(), "Failed to create native image");
}

bool OpenGLVideoFilterBase::StartRenderThread()
{
    MEDIA_LOG_I("StartRenderThread");
    running_ = true;
    thread_ = std::thread([this]() {
        RenderThreadEntry();
    });
    return true;
}

void OpenGLVideoFilterBase::StopRenderThread()
{
    MEDIA_LOG_I("StopRenderThread");
    {
        std::lock_guard<std::mutex> wakeLock(wakeUpMutex_);
        running_ = false;
    }
    wakeUpCond_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

Status OpenGLVideoFilterBase::DoPrepare()
{
    MediaAVCodec::AVCodecTrace trace("OpenGLVideoFilterBase::DoPrepare");
    MEDIA_LOG_I("DoPrepare");
    FALSE_RETURN_V_MSG_E(filterCallback_ != nullptr, Status::ERROR_UNKNOWN, "filterCallback_ is nullptr");
    // 基类默认: 直接回调 NEXT_FILTER_NEEDED 驱动 pipeline 构建
    // WaterMark 子类 override 此方法添加 filterType / AVRecorderMode / HardWatermarkMode 检查
    return filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
        StreamType::STREAMTYPE_RAW_VIDEO);
}

Status OpenGLVideoFilterBase::DoStart()
{
    MEDIA_LOG_I("Start");
    return Status::OK;
}

Status OpenGLVideoFilterBase::DoPause()
{
    MEDIA_LOG_I("Pause");
    return Status::OK;
}

Status OpenGLVideoFilterBase::DoResume()
{
    MEDIA_LOG_I("Resume");
    return Status::OK;
}

Status OpenGLVideoFilterBase::DoStop()
{
    MEDIA_LOG_I("Stop");
    return Status::OK;
}

Status OpenGLVideoFilterBase::DoFlush()
{
    return Status::OK;
}

Status OpenGLVideoFilterBase::DoRelease()
{
    MediaAVCodec::AVCodecTrace trace("OpenGLVideoFilterBase::DoRelease");
    MEDIA_LOG_I("DoRelease enter");
    {
        std::unique_lock<std::mutex> lock(releaseMutex_);
        // 使用标志位防止重复释放
        FALSE_RETURN_V_MSG_W(!hasReleased_, Status::OK, "DoRelease already called, skip");
        hasReleased_ = true;
    }
    isAVRecorderMode_.store(false);

    // 先注销回调，防止新回调触发，再停止渲染线程
    // nativeImage_ 的销毁在 RenderThreadEntry 退出路径中完成（需要在 EGL display 有效时执行），
    // 这里仅注销回调，避免渲染线程退出期间新回调触发。
    if (nativeImage_ != nullptr) {
        OH_NativeImage_UnsetOnFrameAvailableListener(nativeImage_);
    }

    // 停止渲染线程（join 后 nativeImage_ 已在渲染线程销毁）
    StopRenderThread();
    OnReleaseCpuResources();  // 子类 CPU 资源释放（WaterMark: watermarkList_.clear()）

    // 释放GPU侧资源（渲染线程退出时已清理，此处作为安全兜底，防止渲染线程未启动时的泄漏）
    ReleaseGpuResources();

    shaderProgram_.reset();
    OnReleaseShaders();  // 子类 shader 资源释放（WaterMark: reset 水印 shader）
    ReleaseEGLResources();
    // 销毁由 CreateNativeWindowFromSurface 创建的 OHNativeWindow，释放其关联的 buffer queue 和 gralloc 缓冲区。
    // 必须在 ReleaseEGLResources (DestroyEglSurface) 之后调用，确保 EGL surface 不再持有 window 引用。
    if (nativeWindow_ != nullptr) {
        OH_NativeWindow_DestroyNativeWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }

    MEDIA_LOG_I("DoRelease success");
    return Status::OK;
}

void OpenGLVideoFilterBase::ReleaseGpuResources()
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

    // 子类 GPU 资源释放
    // WaterMark: 删除 textTexId_ + watermarkTexIds_ + ReleaseMergeResources()
    // HDR: 删除 outputTexId_ / outputFBO_ / reset yuvConvertProgram_
    OnReleaseGpuResources();
}

void OpenGLVideoFilterBase::ReleaseEGLResources()
{
    if (renderContext_ != nullptr && eglSurface_ != EGL_NO_SURFACE) {
        renderContext_->DestroyEglSurface(eglSurface_);
        eglSurface_ = EGL_NO_SURFACE;
    }
    renderContext_.reset();
}

Status OpenGLVideoFilterBase::NotifyNextFilterEos()
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

void OpenGLVideoFilterBase::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    std::lock_guard<std::mutex> lock(wakeUpMutex_);
    FALSE_RETURN_MSG(parameter != nullptr, "parameter is nullptr");
    int64_t pts = 0;
    parameter->Get<Tag::USER_FRAME_PTS>(pts);
    eosPts_.store(pts);
    eosReceived_.store(true);
    parameter->Get<Tag::USER_PUSH_DATA_TIME>(frameNum_);
    MEDIA_LOG_I("lastBuffer PTS: " PUBLIC_LOG_D64 " frameNum: " PUBLIC_LOG_D64, eosPts_.load(), frameNum_);
    wakeUpCond_.notify_one();
}

void OpenGLVideoFilterBase::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
}

Status OpenGLVideoFilterBase::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    FALSE_RETURN_V_MSG(nextFilter != nullptr, Status::ERROR_UNKNOWN, "nextFilter");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<OpenGLVideoFilterLinkCallback>(shared_from_this());
    auto ret = nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    if (ret != Status::OK && eventReceiver_) {
        eventReceiver_->OnEvent({"OpenGLVideoFilterBase::LinkNext error", EventType::EVENT_ERROR,
            static_cast<int32_t>(MSERR_UNKNOWN)});
    }
    return ret;
}

Status OpenGLVideoFilterBase::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status OpenGLVideoFilterBase::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

Status OpenGLVideoFilterBase::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    onLinkedResultCallback_ = callback;
    return Status::OK;
}

Status OpenGLVideoFilterBase::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}

Status OpenGLVideoFilterBase::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void OpenGLVideoFilterBase::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnLinkedResult enter");
    FALSE_RETURN_MSG(meta != nullptr, "meta is nullptr");
    if (onLinkedResultCallback_) {
        onLinkedResultCallback_->OnLinkedResult(nullptr, meta);
    }
}

void OpenGLVideoFilterBase::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUpdatedResult");
    (void) meta;
}

void OpenGLVideoFilterBase::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUnlinkedResult");
    (void) meta;
}

void OpenGLVideoFilterBase::OnOutputBufferAvailable(uint32_t index, uint32_t flag)
{
    MEDIA_LOG_D("OnOutputBufferAvailable enter. index: " PUBLIC_LOG_U32, index);
}

void OpenGLVideoFilterBase::SetFaultEvent(const std::string &errMsg, int32_t ret)
{
    SetFaultEvent(errMsg + ", ret = " + std::to_string(ret));
}

void OpenGLVideoFilterBase::SetFaultEvent(const std::string &errMsg)
{
    MediaAVCodec::VideoCodecFaultInfo videoCodecFaultInfo;
    videoCodecFaultInfo.appName = bundleName_;
    videoCodecFaultInfo.instanceId = std::to_string(instanceId_);
    videoCodecFaultInfo.callerType = "player_framework";
    videoCodecFaultInfo.errMsg = errMsg;
    MediaAVCodec::FaultVideoCodecEventWrite(videoCodecFaultInfo);
}

void OpenGLVideoFilterBase::ReportInitError(const std::string &errMsg, int32_t errorCode)
{
    MEDIA_LOG_E("RenderThreadEntry init failed: %{public}s, errorCode: %{public}d", errMsg.c_str(), errorCode);
    SetFaultEvent(errMsg);
    if (eventReceiver_ != nullptr) {
        eventReceiver_->OnEvent({name_, EventType::EVENT_ERROR, errorCode});
    }
}

void OpenGLVideoFilterBase::SetCallingInfo(int32_t appUid, int32_t appPid,
    const std::string &bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;
}

FilterType OpenGLVideoFilterBase::GetFilterType()
{
    MEDIA_LOG_I("GetFilterType");
    return filterType_;
}

#undef CHECK_GL_ERROR

void OpenGLVideoFilterBase::RenderVideoFrame()
{
    // 全屏渲染纯视频帧（单纹理采样，无水印混合计算）
    shaderProgram_->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, QUAD_INDICES);
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS
