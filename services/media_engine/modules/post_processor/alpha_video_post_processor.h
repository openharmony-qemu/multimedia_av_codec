/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ALPHA_VIDEO_POST_PROCESSOR_H
#define ALPHA_VIDEO_POST_PROCESSOR_H

#include "base_video_post_processor.h"
#include "video_post_processor_factory.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include "external_window.h"
#include "native_window.h"
#include "native_image.h"
#include "egl_render_context.h"
#include "shader_program.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace OHOS {
namespace Media {

// 透明(alpha)视频后处理器：解码帧 -> OH_NativeImage(零拷贝 DMA) -> OES 纹理(EGL image) ->
// 片段着色器合成(按 AlphaVideoCompositeMode 取颜色半/alpha 半, 默认左彩右透明) -> eglSwapBuffers 上屏(预乘 alpha)。
// 整条通路无 CPU 像素拷贝。GPU 渲染内核对标 WaterMarkFilter，契约/注册对标 SuperResolution。
class AlphaVideoPostProcessor : public BaseVideoPostProcessor {
public:
    AlphaVideoPostProcessor();
    ~AlphaVideoPostProcessor() override;

    Status Init() override;
    Status Flush() override;
    Status Stop() override;
    Status Start() override;
    Status Pause() override;
    Status Release() override;
    Status NotifyEos(int64_t eosPts = 0) override;

    sptr<Surface> GetInputSurface() override;
    Status SetOutputSurface(sptr<Surface> surface) override;

    Status ReleaseOutputBuffer(uint32_t index, bool render) override;
    Status RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs) override;

    Status SetCallback(const std::shared_ptr<PostProcessorCallback> callback) override;
    Status SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver) override;
    Status SetParameter(const Format &format) override;
    Status SetPostProcessorOn(bool isPostProcessorOn) override;
    Status SetVideoWindowSize(int32_t width, int32_t height) override;
    Status SetTransparentVideoLayoutMode(int32_t mode) override;

private:
    // 输入侧：解码器直写的 OH_NativeImage 消费面(零拷贝)
    bool CreateNativeImage();
    void DestroyNativeImage();
    // 输出侧：应用(透明 XComponent)显示面
    void UpdateOutputWindow(sptr<Surface> surface);

    // 渲染线程: EGL/GL 资源在其上创建与销毁
    void RenderLoop();
    bool InitializeEGLContext();
    bool CreateGLResources();
    bool CreateVideoTexture();
    void ConsumeAndReport();  // 取一帧到 OES 纹理 + 上报 filter 节奏
    void DrawAndSwap();        // 节奏触发的着色器合成 + 上屏
    void ReportEos();          // 有效帧全消费+上屏后回传带 EOS flag 的 buffer(对标超分 OnOutputBufferAvailable:229-230)
    void DrawImage();
    static void OnNativeImageFrameAvailable(void *data);
    void ReleaseGpuResources();
    void ReleaseEglResources();
    void FlushConsume();       // seek 排空: 只消费 NativeImage 残留帧, 不上报不绘制(render 线程)
    void AdjustOutputGeometryToFrame(); // 首帧自适应输出尺寸(掉帧优化), render 线程调用
    void DealFrame(bool doConsume, bool doDraw, bool doEos, bool doFlushConsume);
    void DecideFrameAction(bool &doConsume, bool &doDraw, bool &doEos, bool &doFlushConsume);

    // 输入(NativeImage 消费面)
    OH_NativeImage *nativeImage_ {nullptr};
    OHNativeWindow *nativeImageWindow_ {nullptr};
    OH_OnFrameAvailableListener frameAvailableListener_ {};

    // 输出(显示面)
    OHNativeWindow *outputWindow_ {nullptr};
    sptr<Surface> outputSurface_ {nullptr};
    int32_t viewWidth_ {0};
    int32_t viewHeight_ {0};
    std::atomic<bool> geometryAdjusted_ {false}; // 首帧自适应输出尺寸只做一次(player线程 SetParameter / render线程首帧 均可能触发)
    // 解码帧(打包帧)宽高: SetParameter 从 configFormat_ 读取(player 线程写, render 线程读)
    std::atomic<int32_t> frameWidth_ {0};
    std::atomic<int32_t> frameHeight_ {0};

    // EGL / GL
    std::unique_ptr<EglRenderContext> renderContext_;
    EGLSurface eglSurface_ {EGL_NO_SURFACE};
    std::unique_ptr<ShaderProgram> shaderProgram_;           // alpha 合成(透明), isPostProcessorOn_=true 时启用
    std::unique_ptr<ShaderProgram> opaqueShaderProgram_;     // 左半直出(不透明, alpha=1), SetAlphaVideo(false) 时启用
    GLuint nativeImageTexId_ {0};
    GLuint vertexArrayObject_ {0};
    GLuint vertexBufferObject_ {0};
    float matrix_[16] {1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; // 上下镜像(FlipV, 列主序): 仅翻转 Y,
    // 修正 NativeImage/OES 纹理上下颠倒, 对齐 demo kMatrixFlipV

    // 渲染线程与节奏状态机
    std::thread thread_;
    std::mutex wakeMutex_;
    std::condition_variable wakeCond_;
    std::atomic<int> availableFrameCnt_ {0};       // NativeImage 已到帧计数(FIFO 消费)
    std::atomic<bool> running_ {false};            // 线程存活标志, 仅 Release 清零
    std::atomic<bool> started_ {false};            // 绘制开关, Start/Pause/Stop 切换
    std::atomic<bool> eglReady_ {false};
    std::atomic<bool> isPostProcessorOn_ {true};
    // 合成模式(颜色半/alpha 半排布): SetTransparentVideoLayoutMode 写入 + 置 dirty, 由 render 线程在 DrawImage 上 uniform
    std::atomic<int32_t> compositeMode_ {static_cast<int32_t>
        (AlphaVideoCompositeMode::ALPHA_VIDEO_LEFT_COLOR_RIGHT_ALPHA)};
    std::atomic<bool> modeDirty_ {false};
    std::atomic<bool> hasReleased_ {false};
    // IDLE: 纹理未持有帧; HELD: 已绑定一帧, 等待 filter 节奏触发上屏
    enum class FrameState { IDLE, HELD };
    std::atomic<int> frameState_ {static_cast<int>(FrameState::IDLE)};
    std::atomic<bool> renderRequested_ {false};
    std::atomic<int64_t> renderTimeNs_ {-1};
    std::atomic<int64_t> lastPts_ {-1};
    std::atomic<int64_t> eosPts_ {-1};
    std::atomic<bool> eosPending_ {false};       // NotifyEos 置位; RenderLoop drain 完最后一帧后据此回传 EOS buffer
    std::atomic<bool> flushDraining_ {false};    // Flush 置位: RenderLoop 处于 seek 排空模式(消费丢弃残留帧)
    std::atomic<uint32_t> flushEpoch_ {0};       // Flush 世代号: 封堵 Flush 与消费中上报的竞态(旧 pts 不得上报)
    std::atomic<uint32_t> outIndex_ {0};

    std::shared_ptr<PostProcessorCallback> filterCallback_ {nullptr};
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver_ {nullptr};
};

} // namespace Media
} // namespace OHOS
#endif // ALPHA_VIDEO_POST_PROCESSOR_H