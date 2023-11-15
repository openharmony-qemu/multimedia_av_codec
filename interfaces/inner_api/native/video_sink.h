/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd.
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

#ifndef MEDIA_PIPELINE_VIDEO_SINK_H
#define MEDIA_PIPELINE_VIDEO_SINK_H

#ifdef VIDEO_SUPPORT

#include "inner_api/meta/meta.h"
#include "inner_api/buffer/avbuffer_queue.h"
#include "inner_api/buffer/avbuffer_queue_define.h"
#include "inner_api/osal/task/task.h"
#include "plugin/video_sink_plugin.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class VideoSink {
public:
    explicit VideoSink(const std::string& name);
    ~VideoSink();
    Status Init(std::shared_ptr<Meta>& meta);
    std::shared_ptr<AVBufferQueue> GetInputBufferQueue();
    Status SetParameter(std::shared_ptr<Meta>& meta);
    Status GetParameter(std::shared_ptr<Meta>& meta);
    Status Prepare();
    Status Start();
    Status Stop();
    Status Pause();
    Status Release();
    bool CheckBufferLatenessMayWait(AVBuffer &buffer);
    Status Resume();
    void FlushStart();
    void FlushEnd();
#if !defined(OHOS_LITE) && defined(VIDEO_SUPPORT)
    Status SetVideoSurface(sptr<Surface> surface);
#endif
protected:
    std::atomic<FilterState> state_;
private:
    Status ConfigurePluginParams(const std::shared_ptr<Meta>& meta);
    Status ConfigurePluginToStartNoLocked(const std::shared_ptr<Meta>& meta);
    bool CreateVideoSinkPlugin(const std::shared_ptr<Plugin::PluginInfo>& selectedPluginInfo);
    void RenderFrame();
    bool CheckBufferLatenessMayWait(AVBuffer buffer);
    std::shared_ptr<Plugin::VideoSinkPlugin> plugin_ {};
    std::shared_ptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    std::shared_ptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
#ifndef OHOS_LITE
    sptr<Surface> surface_;
#endif
    int64_t refreshTime_ {0};
    bool isFirstFrame_ {true};
    uint32_t frameRate_ {0};
    bool forceRenderNextFrame_ {false};
    Plugin::VideoScaleType videoScaleType_ {Plugin::VideoScaleType::VIDEO_SCALE_TYPE_FIT};

    void CalcFrameRate();
    std::shared_ptr<OHOS::Media::Task> frameRateTask_ {nullptr};
    std::atomic<uint64_t> renderFrameCnt_ {0};
    std::atomic<uint64_t> discardFrameCnt_ {0};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // VIDEO_SUPPORT
#endif // MEDIA_PIPELINE_VIDEO_SINK_FILTER_H