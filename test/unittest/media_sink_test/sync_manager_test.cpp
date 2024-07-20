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

#include "gtest/gtest.h"
#include "filter/filter.h"
#include "media_sync_manager.h"

#include "video_sink.h"
#include "media_synchronous_sink.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Test {
using namespace Pipeline;
std::shared_ptr<MediaSyncManager> SyncManagerCreate()
{
    auto syncManager = std::make_shared<MediaSyncManager>();
    return syncManager;
}

HWTEST(TestSyncManager, sync_manager_set, TestSize.Level1)
{
    auto syncManager = SyncManagerCreate();
    ASSERT_TRUE(syncManager != nullptr);
    float rate = 0;
    auto setPlaybackRateStatus = syncManager->SetPlaybackRate(rate);
    ASSERT_EQ(setPlaybackRateStatus, Status::OK);
}

HWTEST(TestSyncManager, sync_manager_get, TestSize.Level1)
{
    auto syncManager = SyncManagerCreate();
    ASSERT_TRUE(syncManager != nullptr);
    auto rateBack = syncManager->GetPlaybackRate();
    ASSERT_EQ(rateBack, 1);
}

HWTEST(TestSyncManager, sync_manager_life_cycle, TestSize.Level1)
{
    auto syncManager = SyncManagerCreate();
    ASSERT_TRUE(syncManager != nullptr);

    // Resume
    auto resumeStatus = syncManager->Resume();
    ASSERT_EQ(resumeStatus, Status::OK);

    // Pause
    auto pauseStatus = syncManager->Pause();
    ASSERT_EQ(pauseStatus, Status::OK);

    // Seek
    int64_t seekTime = 0;
    auto seekStatus = syncManager->Seek(seekTime);
    ASSERT_NE(seekStatus, Status::OK);

    // Reset
    auto resetStatus = syncManager->Reset();
    ASSERT_EQ(resetStatus, Status::OK);

    // IsSeeking
    auto isSeeking = syncManager->InSeeking();
    ASSERT_EQ(isSeeking, false);
}

HWTEST(TestSyncManager, sync_manager_update_time_without_synchronizer, TestSize.Level1)
{
    auto syncManager = SyncManagerCreate();
    ASSERT_TRUE(syncManager != nullptr);
    // AddSynchronizer
    VideoSink sync;
    syncManager->AddSynchronizer(&sync);
    // RemoveSynchronizer
    syncManager->RemoveSynchronizer(&sync);
    // UpdateTimeAnchor
    auto updateTimeStatus = syncManager->UpdateTimeAnchor(-1, -1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, -1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, 1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, 1, 1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, 1, 1, 1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, 1, 1, 1, 1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, 1, 1, 1, 1, &sync);
    ASSERT_EQ(updateTimeStatus, true);
}

HWTEST(TestSyncManager, sync_manager_life_func, TestSize.Level1)
{
    auto syncManager = SyncManagerCreate();
    ASSERT_TRUE(syncManager != nullptr);
    // AddSynchronizer
    VideoSink sync;
    syncManager->AddSynchronizer(&sync);
    // UpdateTimeAnchor
    auto updateTimeStatus = syncManager->UpdateTimeAnchor(-1, -1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, -1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, 1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, 1, 1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, 1, 1, 1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, 1, 1, 1, 1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager->UpdateTimeAnchor(1, 1, 1, 1, 1, &sync);
    ASSERT_EQ(updateTimeStatus, true);

    // GetMediaTimeNow
    auto mediaTimeNow = syncManager->GetMediaTimeNow();
    ASSERT_EQ(mediaTimeNow, 0);

    // GetClockTimeNow
    auto clockTimeNow = syncManager->GetMediaTimeNow();
    ASSERT_EQ(clockTimeNow, 0);

    // GetClockTime
    auto clockTime = syncManager->GetClockTime(0);
    ASSERT_NE(clockTime, 0);

    // Report
    syncManager->ReportPrerolled(&sync);
    syncManager->ReportEos(&sync);
    syncManager->SetMediaTimeRangeEnd(0, 0, &sync);
    syncManager->SetMediaTimeRangeStart(0, 0, &sync);

    // GetSeekTime
    int64_t seekTime = syncManager->GetSeekTime();
    ASSERT_NE(seekTime, 0);
}
}  // namespace Test
}  // namespace Media
}  // namespace OHOS