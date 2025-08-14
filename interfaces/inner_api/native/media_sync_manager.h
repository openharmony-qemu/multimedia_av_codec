/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_PIPELINE_CORE_PIPELINE_CLOCK_H
#define HISTREAMER_PIPELINE_CORE_PIPELINE_CLOCK_H
#include <condition_variable>
#include <memory>
#include <vector>
#include "sink/i_media_sync_center.h"
#include "osal/task/mutex.h"
#include "osal/task/autolock.h"
#include "osal/utils/steady_clock.h"
#include "filter/filter.h"
#include "common/status.h"
#include "plugin/plugin_time.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace OHOS::Media::Plugins;
class MediaSyncManager : public IMediaSyncCenter, public std::enable_shared_from_this<MediaSyncManager> {
public:
    MediaSyncManager() = default;
    virtual ~MediaSyncManager();

    void AddSynchronizer(IMediaSynchronizer* syncer) override;
    void RemoveSynchronizer(IMediaSynchronizer* syncer) override;

    // after supplier has received the first frame, it should call this function to report the receiving of
    // the first frame.
    void ReportPrerolled(IMediaSynchronizer* supplier) override;

    // after supplier has received the eos frame, it should call this function to report the receiving of
    // the eos frame.
    void ReportEos(IMediaSynchronizer* supplier) override;

    // anchor a media time(pts) with real clock time.
    bool UpdateTimeAnchor(int64_t clockTime, int64_t delayTime, IMediaTime iMediaTime,
        IMediaSynchronizer* supplier) override;

    int64_t GetMediaTimeNow() override;
    int64_t GetClockTimeNow() override;
    int64_t GetAnchoredClockTime(int64_t mediaTime) override;
    int64_t GetSeekTime() override;
    int64_t GetMediaStartPts() override;
    float GetPlaybackRate() override;

    void SetMediaTimeRangeStart(int64_t startMediaTime, int32_t trackId, IMediaSynchronizer* supplier) override;
    void SetMediaTimeRangeEnd(int64_t endMediaTime, int32_t trackId, IMediaSynchronizer* supplier) override;
    void SetMediaStartPts(int64_t startPts) override;
    void SetLastAudioBufferDuration(int64_t durationUs) override;
    void SetLastVideoBufferPts(int64_t bufferPts) override;
    Status SetPlaybackRate(float rate) override;

    void SetInitialVideoFrameRate(double frameRate);
    double GetInitialVideoFrameRate() override;

    void ResetMediaStartPts() override;
    Status Reset() override;
    Status Pause();
    Status Resume();
    Status Seek(int64_t mediaTime, bool isClosest = false);
    Status Stop();

    // return isSeeking_
    bool InSeeking();

    void UpdataPausedMediaTime(int32_t pausedMediaTime);

    // notified when leaving seeking，that is, isSeeking_ is set to false
    std::condition_variable seekCond_;

    void SetLastVideoBufferAbsPts(int64_t lastVideoBufferAbsPts) override;
    int64_t GetLastVideoBufferAbsPts() const override;
private:
    enum class State {
        RESUMED,
        PAUSED,
    };

    std::vector<std::function<bool(MediaSyncManager*, int64_t&)>> setMediaTimeFuncs {
        &MediaSyncManager::CheckSeekingMediaTime,
        &MediaSyncManager::CheckPausedMediaTime,
        &MediaSyncManager::CheckIfMediaTimeIsNone,
        &MediaSyncManager::CheckFirstMediaTimeAfterSeek
    };

    static int64_t GetSystemClock();
    int64_t GetMediaTime(int64_t clockTime);

    void SimpleUpdateTimeAnchor(int64_t clockTime, int64_t mediaTime);
    void ResetTimeAnchorNoLock();

    void UpdateFirstPtsAfterSeek(int64_t mediaTime);
    void SetAllSyncShouldWaitNoLock();

    bool IsSupplierValid(IMediaSynchronizer* supplier);
    bool IsPlayRateValid(float playRate);

    // GetMediaTimeNow executes the following functions in sequence,
    // Check and set media time,
    // return true to indicate that subsequent functions should continue to execute,
    // false to indicate that the current mediaTime should be returned directly
    bool CheckSeekingMediaTime(int64_t& mediaTime);
    bool CheckPausedMediaTime(int64_t& mediaTime);
    bool CheckIfMediaTimeIsNone(int64_t& mediaTime);
    bool CheckFirstMediaTimeAfterSeek(int64_t& mediaTime);

    // Check the media time range,
    // ensure that media time does not regress in non-seek state,
    // and does not exceed the maximum media progress
    int64_t BoundMediaProgress(int64_t newMediaProgressTime);
    // get the maximum media progress
    int64_t GetMaxMediaProgress();

    OHOS::Media::Mutex syncersMutex_ {};
    std::vector<IMediaSynchronizer*> syncers_;
    std::vector<IMediaSynchronizer*> prerolledSyncers_;

    std::atomic<int64_t> lastAudioBufferDuration_ {0};
    std::atomic<int64_t> lastVideoBufferPts_ {0};
    std::atomic<int64_t> lastReportMediaTime_ {HST_TIME_NONE};
    std::atomic<bool> isFrameAfterSeeked_ {false};

    double videoInitialFrameRate_ {-1.0};

    OHOS::Media::Mutex clockMutex_ {};
    float playRate_ {1.0f};
    bool isSeeking_ {false};
    State clockState_ {State::PAUSED};
    int64_t delayTime_ {HST_TIME_NONE};
    int64_t startPts_ {HST_TIME_NONE};
    int64_t firstMediaTimeAfterSeek_ {HST_TIME_NONE};
    bool alreadySetSyncersShouldWait_ {false};
    int64_t seekingMediaTime_ {HST_TIME_NONE};
    int64_t pausedMediaTime_ {HST_TIME_NONE};
    int64_t pausedClockTime_ {HST_TIME_NONE};
    int64_t minRangeStartOfMediaTime_ {HST_TIME_NONE};
    int64_t maxRangeEndOfMediaTime_ {HST_TIME_NONE};
    int64_t currentAnchorClockTime_ {HST_TIME_NONE};
    int64_t currentAnchorMediaTime_ {HST_TIME_NONE};
    int8_t currentSyncerPriority_ {IMediaSynchronizer::NONE};
    int8_t currentRangeStartPriority_ {IMediaSynchronizer::NONE};
    int8_t currentRangeEndPriority_ {IMediaSynchronizer::NONE};

    int64_t lastVideoBufferAbsPts_ {HST_TIME_NONE};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_PIPELINE_CORE_PIPELINE_CLOCK_H
