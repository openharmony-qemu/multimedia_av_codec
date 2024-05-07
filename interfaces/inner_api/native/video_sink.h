/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "osal/task/task.h"
#include "sink/media_synchronous_sink.h"
#include "buffer/avbuffer.h"
#include "common/status.h"
#include "meta/video_types.h"
#include "filter/filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class VideoSink : public MediaSynchronousSink {
public:
    VideoSink();
    ~VideoSink();
    int64_t DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer) override; // true and render
    void ResetSyncInfo() override;
    Status GetLatency(uint64_t& nanoSec);
    int64_t CheckBufferLatenessMayWait(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer);
    void SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter);
    void SetEventReceiver(const std::shared_ptr<EventReceiver> &receiver);
    void SetFirstPts(int64_t pts);
    void SetSeekFlag();
private:
    int64_t refreshTime_ {0};
    bool isFirstFrame_ {true};
    uint32_t frameRate_ {0};
    int64_t firstFramePts_ {0};
    int64_t firstFrameNowct_ {0};
    int64_t lastTimeStamp_ {HST_TIME_NONE};
    int64_t lastBufferTime_ {HST_TIME_NONE};
    int64_t deltaTimeAccu_ {0};
    VideoScaleType videoScaleType_ {VideoScaleType::VIDEO_SCALE_TYPE_FIT};

    void CalcFrameRate();
    std::shared_ptr<OHOS::Media::Task> frameRateTask_ {nullptr};
    std::atomic<uint64_t> renderFrameCnt_ {0};
    std::atomic<uint64_t> discardFrameCnt_ {0};
    std::shared_ptr<EventReceiver> eventReceiver_ {nullptr};
    int64_t firstPts_ {HST_TIME_NONE};
    int64_t fixDelay_ {0};
    bool seekFlag_{false};
    std::atomic<bool> lastFrameDropped_ {false};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // MEDIA_PIPELINE_VIDEO_SINK_FILTER_H