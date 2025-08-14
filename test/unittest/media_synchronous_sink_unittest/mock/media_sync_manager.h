/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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
#include <gmock/gmock.h>
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
    virtual ~MediaSyncManager() = default;
    MOCK_METHOD(void, AddSynchronizer, (IMediaSynchronizer* syncer), ());
    MOCK_METHOD(void, RemoveSynchronizer, (IMediaSynchronizer* syncer), ());
    MOCK_METHOD(void, ReportPrerolled, (IMediaSynchronizer* supplier), ());
    MOCK_METHOD(void, ReportEos, (IMediaSynchronizer* supplier), ());
    MOCK_METHOD(bool, UpdateTimeAnchor, (int64_t clockTime,
        int64_t delayTime, IMediaTime iMediaTime, IMediaSynchronizer* supplier), ());
    MOCK_METHOD(int64_t, GetMediaTimeNow, (), ());
    MOCK_METHOD(int64_t, GetClockTimeNow, (), ());
    MOCK_METHOD(int64_t, GetAnchoredClockTime, (int64_t mediaTime), ());
    MOCK_METHOD(int64_t, GetSeekTime, (), ());
    MOCK_METHOD(int64_t, GetMediaStartPts, (), ());
    MOCK_METHOD(float, GetPlaybackRate, (), ());
    MOCK_METHOD(void, SetMediaTimeRangeStart, (int64_t startMediaTime,
        int32_t trackId, IMediaSynchronizer* supplier), ());
    MOCK_METHOD(void, SetMediaTimeRangeEnd, (int64_t endMediaTime, int32_t trackId, IMediaSynchronizer* supplier), ());
    MOCK_METHOD(void, SetMediaStartPts, (int64_t startPts), ());
    MOCK_METHOD(void, SetLastAudioBufferDuration, (int64_t durationUs), ());
    MOCK_METHOD(void, SetLastVideoBufferPts, (int64_t bufferPts), ());
    MOCK_METHOD(Status, SetPlaybackRate, (float rate), ());
    MOCK_METHOD(double, GetInitialVideoFrameRate, (), ());
    MOCK_METHOD(void, ResetMediaStartPts, (), ());
    MOCK_METHOD(Status, Reset, (), ());
    MOCK_METHOD(void, SetLastVideoBufferAbsPts, (int64_t lastVideoBufferAbsPts), ());
    MOCK_METHOD(int64_t, GetLastVideoBufferAbsPts, (), (const));
    MOCK_METHOD(Status, Pause, (), ());
    MOCK_METHOD(Status, Resume, (), ());
    MOCK_METHOD(Status, Seek, (int64_t mediaTime, bool isClosest), ());
    MOCK_METHOD(Status, Stop, (), ());
    MOCK_METHOD(bool, InSeeking, (), ());
    std::atomic<bool> isFrameAfterSeeked_ {false};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_PIPELINE_CORE_PIPELINE_CLOCK_H