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

#ifndef FILTERS_OPENGL_VIDEO_FILTER_BASE_H
#define FILTERS_OPENGL_VIDEO_FILTER_BASE_H

#include <atomic>
#include <cstring>
#include <vector>
#include "filter/filter.h"
#include "osal/task/task.h"
#include "common/status.h"
#include "common/log.h"
#include "external_window.h"
#include <egl_render_context.h>
#include "shader_program.h"
#include "surface.h"

struct OH_NativeImage;
struct OH_OnFrameAvailableListener;

namespace OHOS {
namespace Media {
namespace Pipeline {

const GLuint DEFINED_VALUE = 9999U;

// OpenGLVideoFilterBase 是 WaterMarkFilter 和 VideoResizeOpenGLFilter 的共享基类，
// 封装通用 OpenGL 视频渲染基础设施（EGL context、NativeImage、渲染线程、视频 OES 纹理渲染、
// VAO/VBO、Surface 管理、生命周期）。水印专属逻辑由 WaterMarkFilter 通过 virtual hook 注入，
// HDR/10bit 转码逻辑由 VideoResizeOpenGLFilter 通过 virtual hook 注入。
class OpenGLVideoFilterBase : public Filter, public std::enable_shared_from_this<OpenGLVideoFilterBase> {
public:
    explicit OpenGLVideoFilterBase(std::string name, FilterType type);
    ~OpenGLVideoFilterBase() override;

    Status SetCodecFormat(const std::shared_ptr<Meta> &format);
    void Init(const std::shared_ptr<EventReceiver> &receiver,
        const std::shared_ptr<FilterCallback> &callback) override;
    void InitOpenGl();
    Status Configure(const std::shared_ptr<Meta> &parameter);
    sptr<Surface> GetInputSurface();
    Status SetOutputSurface(sptr<Surface> surface, int32_t width, int32_t height, int32_t rotation);
    virtual Status SetVideoResize(int32_t width, int32_t height);
    Status DoPrepare() override;
    Status DoStart() override;
    Status DoPause() override;
    Status DoResume() override;
    Status DoStop() override;
    Status DoFlush() override;
    Status DoRelease() override;
    Status NotifyNextFilterEos();
    void SetParameter(const std::shared_ptr<Meta> &parameter) override;
    void GetParameter(std::shared_ptr<Meta> &parameter) override;
    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);
    void OnUpdatedResult(std::shared_ptr<Meta> &meta);
    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);
    void OnOutputBufferAvailable(uint32_t index, uint32_t flag);
    void SetFaultEvent(const std::string &errMsg);
    void SetFaultEvent(const std::string &errMsg, int32_t ret);
    void SetCallingInfo(int32_t appUid, int32_t appPid, const std::string &bundleName, uint64_t instanceId);

protected:
    // ---- Virtual hooks: 子类用于注入水印 / HDR / 10bit 管线 ----
    // EGL 上下文初始化：SDR 基类使用 window surface；HDR 子类使用 pbuffer surface + FBO。
    virtual bool SetupEglContextForMode() { return InitializeEGLContext(); }
    // GL 资源创建后、渲染循环前的子类初始化（WaterMark: 水印 shader+纹理+merge；HDR: outputFBO）。
    virtual bool OnGLResourcesCreated() { return true; }
    // 每帧渲染：基类默认仅渲染视频直通；WaterMark 子类增加水印 overlay。
    virtual void RenderFrame(int32_t viewWidth, int32_t viewHeight);
    // 每帧完整处理：基类默认 UpdateVideoFrame + DrawImage；HDR 子类走 RenderHdrFrame。
    virtual void ProcessFrame();
    // NativeImage 创建后的输入 surface 配置（SDR 无需配置；HDR 子类配置 P010 + 色彩空间）。
    virtual void ConfigureInputSurface() {}
    // 输出 surface 配置（SDR 无需配置；HDR 子类配置 P010 + 色彩空间）。
    virtual void ConfigureOutputSurface() {}
    // SetCodecFormat 扩展点：HDR 子类读取色彩空间并设置 isHdrMode_。
    virtual void OnSetCodecFormat(const std::shared_ptr<Meta> &format) { (void)format; }
    // 子类 GPU 资源释放（WaterMark: 水印纹理+FBO；HDR: output FBO/texture/YUV shader）。
    virtual void OnReleaseGpuResources() {}
    // 渲染线程退出时、EGL display 仍有效时的子类清理扩展点（未使用，在 ReleaseGpuResources 之前调用）。
    virtual void OnRenderThreadExit() {}
    // 水印 overlay shader uniform 设置（WaterMark 子类实现）。
    virtual void OnSetupOverlayUniforms() {}
    // CPU 资源释放（WaterMark 子类清理 watermarkList_）。
    virtual void OnReleaseCpuResources() {}
    // shader 资源释放（WaterMark 子类 reset 水印 shader）。
    virtual void OnReleaseShaders() {}
    // ---------------------------------------------------------------------------------

    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

    // ---- 通用 GL 基础设施成员 ----
    // Pipeline
    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;
    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;
    std::shared_ptr<Meta> configureParameter_;
    std::shared_ptr<Filter> nextFilter_;
    // EOS
    std::atomic<int64_t> eosPts_ {-1};
    std::atomic<bool> eosNotified_ {false};
    std::atomic<int64_t> lastPts_ {INT64_MAX};
    std::atomic<int64_t> currentFrameNum_ = 0;
    // App info
    std::string bundleName_;
    int64_t frameNum_ {INT64_MAX};
    uint64_t instanceId_{0};
    int32_t appUid_ {0};
    int32_t appPid_ {0};
    // Dimensions
    std::atomic<int32_t> targetWidth_ = -1;
    std::atomic<int32_t> targetHeight_ = -1;
    std::atomic<int32_t> videoWidth_ = 0;
    std::atomic<int32_t> videoHeight_ = 0;
    // NativeImage
    OH_NativeImage *nativeImage_ = nullptr;
    std::unique_ptr<OH_OnFrameAvailableListener> nativeImageFrameAvailableListener_;
    OHNativeWindow *nativeImageWindow_ = nullptr;
    GLuint nativeImageTexId_ = DEFINED_VALUE;
    // Render thread
    std::thread thread_;
    std::mutex wakeUpMutex_;
    std::mutex releaseMutex_;
    std::condition_variable wakeUpCond_;
    std::atomic<int> availableFrameCnt_{ 0 };
    float matrix_[16] = {};
    std::atomic<bool> running_{ true };
    std::atomic<bool> eosReceived_{ false };
    std::atomic<bool> hasReleased_ = false;
    std::atomic<bool> eglContextReady_ = false;
    std::atomic<bool> isAVRecorderMode_ = false;
    std::atomic<int32_t> rotation_ = 0;
    // EGL
    std::unique_ptr<EglRenderContext> renderContext_;
    EGLSurface eglSurface_ = EGL_NO_SURFACE;
    // Output
    OHNativeWindow *nativeWindow_ = nullptr;
    // GL
    std::unique_ptr<ShaderProgram> shaderProgram_;
    GLuint vertexArrayObject_ = 0;
    GLuint vertexBufferObject_ = 0;

    // ---- 通用方法 ----
    bool CreateNativeImage();
    void UpdateNativeWindow(void *window, uint64_t width, uint64_t height);
    void RenderThreadEntry();
    bool InitRenderResources();
    void CleanupRenderResources();
    bool CheckInitStep(bool result, const std::string &errMsg, int32_t errorCode);
    void AttachNativeImage();
    void DrawImage(const int64_t pts);
    void UpdateVideoFrame();
    void SwapFrameBuffers();
    static void OnNativeImageFrameAvailable(void *data);
    bool CreateGLResources();
    bool InitializeEGLContext();
    bool CreateVideoTexture();
    bool SetupShaderUniforms();
    void RenderLoop();
    bool StartRenderThread();
    void StopRenderThread();
    void RenderVideoFrame();
    void ReleaseGpuResources();
    void ReleaseEGLResources();
    FilterType GetFilterType();
    void ReportInitError(const std::string &errMsg, int32_t errorCode);
};

} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // FILTERS_OPENGL_VIDEO_FILTER_BASE_H
