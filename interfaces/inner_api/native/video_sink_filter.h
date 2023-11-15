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

#ifndef MEDIA_PIPELINE_VIDEO_SINK_FILTER_H
#define MEDIA_PIPELINE_VIDEO_SINK_FILTER_H

#ifdef VIDEO_SUPPORT

#include <atomic>
#if !defined(OHOS_LITE) && defined(VIDEO_SUPPORT)
#include "refbase.h"
#include "surface/surface.h"
#endif
#include "interface/inner_api/osal/task/condition_variable.h"
#include "interface/inner_api/osal/task/mutex.h"
#include "interface/inner_api/osal/task/task.h"
#include "inner_api/module/video_sink.h"
#include "inner_api/module/media_synchronous_sink.h"
#include "interface/inner_api/common/status.h"
#include "interface/inner_api/meta/meta.h"
#include "interface/inner_api/filter/filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class VideoSinkFilter : public MediaSynchronousSink, public Filter {
public:
    explicit VideoSinkFilter(const std::string& name);
    ~VideoSinkFilter() override;

    void Init(std::shared_ptr<EventReceiver> receiver, std::shared_ptr<FilterCallback> callback) override;

    Status Prepare() override;

    Status Start() override;

    Status Pause() override;

    Status Resume() override;

    Status Stop() override;

    Status Flush() override;

    Status Release() override;

    void SetParameter(std::shared_ptr<Meta>& meta) override;

    void GetParameter(std::shared_ptr<Meta>& meta) override;

    std::shared_ptr<AVBufferQueueProducer> GetInputBufferQueue();

#if !defined(OHOS_LITE) && defined(VIDEO_SUPPORT)
    Status SetVideoSurface(sptr<Surface> surface);
#endif

private:
    Status ConfigurePluginParams(const std::shared_ptr<const Plugin::Meta>& meta);
    Status ConfigurePluginToStartNoLocked(const std::shared_ptr<const Plugin::Meta>& meta);
    bool CreateVideoSinkPlugin(const std::shared_ptr<Plugin::PluginInfo>& selectedPluginInfo);
    void RenderFrame();

    std::shared_ptr<AVBufferQueueProducer> inBufQueue_;
    std::shared_ptr<OHOS::Media::Task> renderTask_ {nullptr};
    std::atomic<bool> pushThreadIsBlocking_ {false};
    std::atomic<bool> isFlushing_ {false};
    OHOS::Media::ConditionVariable startWorkingCondition_ {};
    OHOS::Media::Mutex mutex_;
#ifndef OHOS_LITE
    sptr<Surface> surface_;
#endif

    int64_t refreshTime_ {0};
    bool isFirstFrame_ {true};
    uint32_t frameRate_ {0};
    bool forceRenderNextFrame_ {false};
    Plugin::VideoScaleType videoScaleType_ {Plugin::VideoScaleType::VIDEO_SCALE_TYPE_FIT};

    void CalcFrameRate();
    std::shared_ptr<OHOS::Media::OSAL::Task> frameRateTask_ {nullptr};
    std::atomic<uint64_t> renderFrameCnt_ {0};
    std::atomic<uint64_t> discardFrameCnt_ {0};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif
#endif // MEDIA_PIPELINE_VIDEO_SINK_FILTER_H