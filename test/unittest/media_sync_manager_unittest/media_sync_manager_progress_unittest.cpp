/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <limits>
#include "media_sync_manager_unittest.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {

/**
 * @tc.name  : Test GetMaxMediaProgress
 * @tc.number: GetMaxMediaProgress_001
 * @tc.desc  : Test GetMaxMediaProgress for each syncer priority, including the
 *             NONE priority case where audio yielded via ReportEos
 */
HWTEST_F(MediaSyncManagerUnitTest, GetMaxMediaProgress_001, TestSize.Level0)
{
    ASSERT_NE(manager_, nullptr);
    // audio sink priority: bounded by anchor media time plus last audio buffer duration
    manager_->currentSyncerPriority_ = IMediaSynchronizer::AUDIO_SINK;
    manager_->currentAnchorMediaTime_ = 1000;
    manager_->lastAudioBufferDuration_ = 100;
    EXPECT_EQ(manager_->GetMaxMediaProgress(), 1100);

    // video sink priority: bounded by last video buffer pts
    manager_->currentSyncerPriority_ = IMediaSynchronizer::VIDEO_SINK;
    manager_->lastVideoBufferPts_ = 900;
    EXPECT_EQ(manager_->GetMaxMediaProgress(), 900);

    // NONE priority (audio yielded via ReportEos): unbounded so video is not capped
    manager_->currentSyncerPriority_ = IMediaSynchronizer::NONE;
    EXPECT_EQ(manager_->GetMaxMediaProgress(), INT64_MAX);

    // other priorities: bounded by the larger of anchor media time and last report time
    manager_->currentSyncerPriority_ = IMediaSynchronizer::VIDEO_SRC;
    manager_->lastReportMediaTime_ = 500;
    EXPECT_EQ(manager_->GetMaxMediaProgress(), 1000);
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
