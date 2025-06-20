/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "performance_utils.h"

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
    void SetLastPts(int64_t lastPts, int64_t renderDelay = 0);
    Status SetParameter(const std::shared_ptr<Meta>& meta);
    void UpdateTimeAnchorActually(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer, int64_t renderDelay = 0);
    Status GetLagInfo(int32_t& lagTimes, int32_t& maxLagDuration, int32_t& avgLagDuration);
    Status SetPerfRecEnabled(bool isPerfRecEnabled);
    void SetMediaMuted(bool isMuted);

private:
    int64_t CalcBufferDiff(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer,
        int64_t bufferAnchoredClockTime, int64_t currentClockTime, float playbackRate);
    float AdjustPlaybackRate(float speed);
    int64_t SmoothDeltaTime(int64_t accumulatedDeltaTime, int64_t currentDeltaTime);
    void UpdateTimeAnchorIfNeeded(int64_t nowCt, int64_t waitTime,
        const std::shared_ptr<OHOS::Media::AVBuffer>& buffer);
    void RenderAtTimeLog(int64_t waitTime);
    void PerfRecord(int64_t waitTime);
    void ReportPts(int64_t nowPts);
    void InitWaitPeriod();

    class VideoLagDetector : public LagDetector {
    public:
        void Reset() override;
        bool CalcLag(std::shared_ptr<AVBuffer> buffer) override;
        void GetLagInfo(int32_t& lagTimes, int32_t& maxLagDuration, int32_t& avgLagDuration);
        void ResolveLagEvent(const int64_t &lagTimeMs);
        void SetEventReceiver(const std::shared_ptr<EventReceiver> eventReceiver);
    private:
        int64_t lagTimes_ = 0;
        int64_t maxLagDuration_ = 0;
        int64_t lastSystemTimeMs_ = 0;
        int64_t lastBufferTimeMs_ = 0;
        int64_t totalLagDuration_ = 0;
        std::shared_ptr<EventReceiver> eventReceiver_ { nullptr };
    };

    std::atomic<bool> needUpdateTimeAnchor_ {true};
    int64_t refreshTime_ {0};
    bool isFirstFrame_ {true};
    int64_t firstFramePts_ {0};
    int64_t firstFrameClockTime_ {0};
    int64_t lastBufferRelativePts_ {HST_TIME_NONE};
    int64_t lastBufferAnchoredClockTime_ {HST_TIME_NONE};
    int64_t deltaTimeAccu_ {0};

    int64_t initialVideoWaitPeriod_ {0};

    std::shared_ptr<OHOS::Media::Task> frameRateTask_ {nullptr};
    std::atomic<uint64_t> renderFrameCnt_ {0};
    std::atomic<uint64_t> discardFrameCnt_ {0};
    std::shared_ptr<EventReceiver> eventReceiver_ {nullptr};
    int64_t firstPts_ {HST_TIME_NONE};
    int64_t fixDelay_ {0};
    bool seekFlag_{false};
    bool isPerfRecEnabled_ { false };
    std::atomic<int32_t> dropFrameContinuouslyCnt_ {0};
    int64_t lastPts_ = -1;
    int64_t lastClockTime_ = -1;
    std::atomic<bool> isRenderStarted_{false};
    VideoLagDetector lagDetector_ {};
    int64_t renderAdvanceThreshold_ {80000};
    bool enableRenderAtTime_ {true};
    PerfRecorder perfRecorder_ {};
    bool isMuted_ = false;
    std::atomic<bool> needSyncAfterMute_ {false};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // MEDIA_PIPELINE_VIDEO_SINK_FILTER_H