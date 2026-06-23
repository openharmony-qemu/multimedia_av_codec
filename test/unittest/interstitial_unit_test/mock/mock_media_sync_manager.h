/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MOCK_MEDIA_SYNC_MANAGER_H
#define MOCK_MEDIA_SYNC_MANAGER_H

#include <gmock/gmock.h>
#include "media_sync_manager.h"

namespace OHOS {
namespace Media {
namespace Pipeline {

class MockMediaSyncManager : public MediaSyncManager {
public:
    MockMediaSyncManager() = default;
    ~MockMediaSyncManager() override = default;

    MOCK_METHOD(void, AddSynchronizer, (IMediaSynchronizer* syncer), (override));
    MOCK_METHOD(void, RemoveSynchronizer, (IMediaSynchronizer* syncer), (override));
    MOCK_METHOD(void, ReportPrerolled, (IMediaSynchronizer* supplier), (override));
    MOCK_METHOD(void, ReportEos, (IMediaSynchronizer* supplier), (override));
    MOCK_METHOD(bool, UpdateTimeAnchor, (int64_t clockTime, int64_t delayTime,
        IMediaTime iMediaTime, IMediaSynchronizer* supplier), (override));
    MOCK_METHOD(int64_t, GetMediaTimeNow, (), (override));
    MOCK_METHOD(int64_t, GetClockTimeNow, (), (override));
    MOCK_METHOD(int64_t, GetAnchoredClockTime, (int64_t mediaTime), (override));
    MOCK_METHOD(int64_t, GetSeekTime, (), (override));
    MOCK_METHOD(int64_t, GetMediaStartPts, (), (override));
    MOCK_METHOD(float, GetPlaybackRate, (), (override));
    MOCK_METHOD(void, SetMediaTimeRangeStart, (int64_t startMediaTime,
        int32_t trackId, IMediaSynchronizer* supplier), (override));
    MOCK_METHOD(void, SetMediaTimeRangeEnd, (int64_t endMediaTime,
        int32_t trackId, IMediaSynchronizer* supplier), (override));
    MOCK_METHOD(void, SetMediaStartPts, (int64_t startPts), (override));
    MOCK_METHOD(void, SetLastAudioBufferDuration, (int64_t durationUs), (override));
    MOCK_METHOD(void, SetLastVideoBufferPts, (int64_t bufferPts), (override));
    MOCK_METHOD(Status, SetPlaybackRate, (float rate), (override));
    MOCK_METHOD(double, GetInitialVideoFrameRate, (), (override));
    MOCK_METHOD(void, ResetMediaStartPts, (), (override));
    MOCK_METHOD(Status, Reset, (), (override));
    MOCK_METHOD(void, SetLastVideoBufferAbsPts, (int64_t lastVideoBufferAbsPts), (override));
    MOCK_METHOD(int64_t, GetLastVideoBufferAbsPts, (), (const, override));

};

} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // MOCK_MEDIA_SYNC_MANAGER_H
