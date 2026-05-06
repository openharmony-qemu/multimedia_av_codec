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
 
#ifndef FILTERS_WATER_MARK_FILTER_H
#define FILTERS_WATER_MARK_FILTER_H
 
#include <cstring>
#include "filter/filter.h"
#include "osal/task/task.h"
#include "common/status.h"
#include "common/log.h"
#include "external_window.h"
#include <egl_render_context.h>
#include "shader_program.h"
#include "surface.h"
#include "native_image.h"
 
namespace OHOS {//water_mark_filter
namespace Media {
namespace Pipeline {
using RenderTask = std::function<void(EglRenderContext &renderContext)>;
const GLuint DEFINED_VALUE = 9999U;
struct WaterMarkConfig {
   int32_t top;
   int32_t left;
   int32_t watermarkWidth ;
   int32_t watermarkHeight ;
   int32_t rawHeight ;
   int32_t rawWidth ;
   int64_t size;
   unsigned char *buffer;
};
 
class WaterMarkFilter : public Filter, public std::enable_shared_from_this<WaterMarkFilter> {
public:
    explicit WaterMarkFilter(std::string name, FilterType type);
    ~WaterMarkFilter() override;
    Status SetCodecFormat(const std::shared_ptr<Meta> &format);
    void Init(const std::shared_ptr<EventReceiver> &receiver,
        const std::shared_ptr<FilterCallback> &callback) override;
    void InitOpenGl();
    Status Configure(const std::shared_ptr<Meta> &parameter);
    sptr<Surface> GetInputSurface();
    Status SetOutputSurface(sptr<Surface> surface, int32_t width, int32_t height);
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
    Status SetWatermark(std::shared_ptr<AVBuffer> &waterMarkBuffer, int32_t width, int32_t height);
    Status SetVideoResize(int32_t width, int32_t height);  // 设置视频缩放的目标宽高
 
protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;
 
private:
    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;
    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;
    std::shared_ptr<Meta> configureParameter_;
    std::shared_ptr<Filter> nextFilter_;
    std::atomic<int64_t> eosPts_ {-1};
    std::atomic<int64_t> lastPts_ {UINT64_MAX};
    std::atomic<int64_t> currentFrameNum_ = 0;
    std::string bundleName_;
    int64_t frameNum_ {UINT32_MAX};
    uint64_t instanceId_{0};
    int32_t appUid_ {0};
    int32_t appPid_ {0};
    int32_t targetWidth_ = -1;      // 视频缩放后的目标宽度
    int32_t targetHeight_ = -1;     // 视频缩放后的目标高度
    std::atomic<int32_t> videoWidth_ = 0;
    std::atomic<int32_t> videoHeight_ = 0;
    OH_NativeImage *nativeImage_ = nullptr;
    std::unique_ptr<ShaderProgram> shaderProgram_;
    std::vector<WaterMarkConfig> watermarkList_;
    std::thread thread_;
    std::mutex wakeUpMutex_;
    std::mutex releaseMutex_;
    std::atomic<int> availableFrameCnt_{ 0 };
    float matrix_[16];
    OH_OnFrameAvailableListener nativeImageFrameAvailableListener_{};
    std::atomic<bool> running_{ true };
    bool hasReleased_ = false;  // 标记是否已释放资源，防止重复释放
    std::unique_ptr<EglRenderContext> renderContext_;
    EGLSurface eglSurface_ = EGL_NO_SURFACE;
    OHNativeWindow *nativeWindow_ = nullptr;
    OHNativeWindow *nativeImageWindow_ = nullptr;
    GLuint nativeImageTexId_ = DEFINED_VALUE;
    GLuint textTexId_ = DEFINED_VALUE;
    GLuint mergedWatermarkTexId_ = 0;         // 合并后的水印纹理ID
    GLuint watermarkFBO_ = 0;                 // 用于合成的帧缓冲对象
    GLuint watermarkVAO_ = 0;                 // 水印合成用的VAO
    GLuint watermarkVBO_ = 0;
    GLuint vertexArrayObject_ = 0;
    GLuint vertexBufferObject_ = 0;
    std::condition_variable wakeUpCond_;
    std::vector<GLuint> watermarkTexIds_;     // 每个水印的纹理ID
    std::unique_ptr<ShaderProgram> watermarkMergeShaderProgram_;  // 水印合成shader
 
    bool CreateNativeImage();
    void UpdateNativeWindow(void *window, uint64_t width, uint64_t height);
    void ThreadMainLoop();
    void DrawImage(const int64_t pts);
    static void OnNativeImageFrameAvailable(void *data);
    bool CreateGLResources();
    bool InitializeEGLContext();
    bool CreateVideoTexture();
    bool CreateWatermarkTextureAndCopy();
    bool SetupShaderUniforms();
    void RenderLoop();
    bool CreateWatermarkTextures();           // 为每个水印创建纹理
    bool MergeWaterMarksWithOpenGL();         // 使用OpenGL合成多个水印
    bool CreateWatermarkFBO(int32_t width, int32_t height);  // 创建水印合并用的FBO
    bool CreateWatermarkMergeShader();        // 创建水印合并着色器
    bool RenderWatermarksToFBO(int32_t width, int32_t height);  // 渲染水印到FBO
    void ReleaseCpuResources();
    void ReleaseGpuResources();
    void ReleaseEGLResources();
    FilterType GetFilterType();
};
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
#endif // FILTERS_WATER_MARK_FILTER_H