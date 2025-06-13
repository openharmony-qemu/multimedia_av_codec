/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef MEDIA_SYNC_CENTER_MOCK_H
#define MEDIA_SYNC_CENTER_MOCK_H
#include <queue>
#include "common/status.h"
#include "media_sync_manager.h"

namespace OHOS {
namespace Media {
namespace Test {
class MockMediaSyncCenter : public Pipeline::MediaSyncManager {
public:
    Status Reset() override;
    void AddSynchronizer(Pipeline::IMediaSynchronizer* syncer) override;
    void RemoveSynchronizer(Pipeline::IMediaSynchronizer* syncer) override;
    bool UpdateTimeAnchor(int64_t clockTime, int64_t delayTime, IMediaTime iMediaTime,
        Pipeline::IMediaSynchronizer* supplier) override;
    int64_t GetMediaTimeNow() override;
    int64_t GetClockTimeNow() override;
    int64_t GetAnchoredClockTime(int64_t mediaTime) override;
    void ReportPrerolled(Pipeline::IMediaSynchronizer* supplier) override;
    void ReportEos(Pipeline::IMediaSynchronizer* supplier) override;
    void SetMediaTimeRangeStart(int64_t startMediaTime, int32_t trackId,
        Pipeline::IMediaSynchronizer* supplier) override;
    void SetMediaTimeRangeEnd(int64_t endMediaTime, int32_t trackId, Pipeline::IMediaSynchronizer* supplier) override;
    int64_t GetSeekTime() override;
    Status SetPlaybackRate(float rate) override;
    float GetPlaybackRate() override;
    void SetMediaStartPts(int64_t startPts) override;
    int64_t GetMediaStartPts() override;
    void SetLastVideoBufferPts(int64_t bufferPts) override;
public:
    bool returnBool_{false};
    std::queue<int64_t> returnInt64Queue_{};
    float returnFloat_{0.0f};
    std::queue<Status> returnStatusQueue_{};
    bool synchronizerAdded_ {false};
    int32_t startPtsGetTime_ {0};
    int32_t setMediaRangeStartTime_ {0};
    int32_t setMediaRangeEndTime_ {0};
    int64_t mediaRangeEndValue_ {0};
    int32_t setLastVideoBufferPtsTimes_ {0};
    int32_t updateTimeAnchorTimes_ {0};
    int64_t lastTimeAnchorClock_ {0};
private:
    int64_t GetRetInt64Value();
};
}  // namespace Test
}  // namespace Media
}  // namespace OHOS
#endif // MEDIA_SYNC_CENTER_MOCK_H