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

#include <cstdint>
#include <limits>
#include "audio_sink_unit_test.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Test {

std::shared_ptr<AudioSink> CreateYieldTestAudioSink()
{
    auto audioSink = std::make_shared<AudioSink>();
    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    auto meta = std::make_shared<Meta>();
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    if (initStatus == Status::OK) {
        return audioSink;
    }
    return nullptr;
}

/**
 * @tc.name Test ReportEosToSyncCenter API
 * @tc.number AudioSink_ReportEosToSyncCenter_001
 * @tc.desc Test syncCenter_ is empty after ReportEosToSyncCenter
 */
HWTEST(TestAudioSinkYield, AudioSink_ReportEosToSyncCenter_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = CreateYieldTestAudioSink();
    ASSERT_TRUE(audioSink != nullptr);
    // no sync center set: expired weak_ptr branch, yield flag is still set
    audioSink->ReportEosToSyncCenter();
    EXPECT_TRUE(audioSink->hasYieldedSyncerPriority_.load());
}

/**
 * @tc.name Test ReportEosToSyncCenter API
 * @tc.number AudioSink_ReportEosToSyncCenter_002
 * @tc.desc Test ReportEosToSyncCenter yields audio syncer priority to the sync center
 */
HWTEST(TestAudioSinkYield, AudioSink_ReportEosToSyncCenter_002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = CreateYieldTestAudioSink();
    ASSERT_TRUE(audioSink != nullptr);
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    audioSink->SetSyncCenter(syncCenter);
    syncCenter->AddSynchronizer(audioSink.get());
    syncCenter->currentSyncerPriority_ = Pipeline::IMediaSynchronizer::AUDIO_SINK;
    audioSink->ReportEosToSyncCenter();
    EXPECT_TRUE(audioSink->hasYieldedSyncerPriority_.load());
    // audio yielded the priority: max media progress is unbounded so video can take over
    EXPECT_EQ(syncCenter->GetMaxMediaProgress(), INT64_MAX);
}

/**
 * @tc.name Test IsTimeAnchorNeedUpdate API
 * @tc.number AudioSink_IsTimeAnchorNeedUpdateAfterYield_001
 * @tc.desc Test IsTimeAnchorNeedUpdate returns false after syncer priority yielded
 */
HWTEST(TestAudioSinkYield, AudioSink_IsTimeAnchorNeedUpdateAfterYield_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = CreateYieldTestAudioSink();
    ASSERT_TRUE(audioSink != nullptr);
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    audioSink->SetSyncCenter(syncCenter);
    audioSink->ReportEosToSyncCenter();
    EXPECT_FALSE(audioSink->IsTimeAnchorNeedUpdate());
}

/**
 * @tc.name Test UpdateTimeAnchorIfNeeded API
 * @tc.number AudioSink_UpdateTimeAnchorIfNeededAfterYield_001
 * @tc.desc Test UpdateTimeAnchorIfNeeded returns false after syncer priority yielded
 */
HWTEST(TestAudioSinkYield, AudioSink_UpdateTimeAnchorIfNeededAfterYield_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = CreateYieldTestAudioSink();
    ASSERT_TRUE(audioSink != nullptr);
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    audioSink->SetSyncCenter(syncCenter);
    audioSink->ReportEosToSyncCenter();
    EXPECT_FALSE(audioSink->UpdateTimeAnchorIfNeeded(nullptr));
}

/**
 * @tc.name Test Init API
 * @tc.number AudioSink_InitResetsYieldedSyncerPriority_001
 * @tc.desc Test Init resets hasYieldedSyncerPriority_ after a previous yield
 */
HWTEST(TestAudioSinkYield, AudioSink_InitResetsYieldedSyncerPriority_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = std::make_shared<AudioSink>();
    audioSink->ReportEosToSyncCenter();
    EXPECT_TRUE(audioSink->hasYieldedSyncerPriority_.load());
    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    auto meta = std::make_shared<Meta>();
    EXPECT_EQ(audioSink->Init(meta, testEventReceiver), Status::OK);
    // re-init clears the yield flag so the new audio pipeline can take over the clock again
    EXPECT_FALSE(audioSink->hasYieldedSyncerPriority_.load());
}
} // namespace Test
} // namespace Media
} // namespace OHOS
