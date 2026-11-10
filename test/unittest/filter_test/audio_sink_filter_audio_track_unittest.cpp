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
#include "audio_sink_filter_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {

// Fixture for the unsupported-audio-track rebuild behavior of AudioSinkFilter.
class AudioSinkFilterAudioTrackTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void) override;
    // TearDown: Called after each test cases
    void TearDown(void) override;

protected:
    std::shared_ptr<AudioSinkFilter> audioSinkFilter_ { nullptr };
};

void AudioSinkFilterAudioTrackTest::SetUpTestCase(void) {}

void AudioSinkFilterAudioTrackTest::TearDownTestCase(void) {}

void AudioSinkFilterAudioTrackTest::SetUp(void)
{
    audioSinkFilter_ = std::make_shared<AudioSinkFilter>("testAudioSinkFilterAudioTrack", FilterType::FILTERTYPE_ASINK);
}

void AudioSinkFilterAudioTrackTest::TearDown(void)
{
    audioSinkFilter_ = nullptr;
}

/**
 * @tc.name: AudioSinkFilter_ReportEosToSyncCenter_0100
 * @tc.desc: Test ReportEosToSyncCenter with null audioSink_ and with a valid audioSink_
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterAudioTrackTest, AudioSinkFilter_ReportEosToSyncCenter_0100, TestSize.Level1)
{
    // audioSink_ is nullptr: FALSE_RETURN branch, no crash
    audioSinkFilter_->audioSink_ = nullptr;
    audioSinkFilter_->ReportEosToSyncCenter();

    // valid audioSink_: delegates to AudioSink::ReportEosToSyncCenter and yields the syncer priority
    auto audioSink = std::make_shared<AudioSink>();
    audioSinkFilter_->audioSink_ = audioSink;
    auto syncCenter = std::make_shared<MediaSyncManager>();
    audioSink->SetSyncCenter(syncCenter);
    syncCenter->AddSynchronizer(audioSink.get());
    syncCenter->currentSyncerPriority_ = IMediaSynchronizer::AUDIO_SINK;
    audioSinkFilter_->ReportEosToSyncCenter();
    EXPECT_TRUE(audioSink->hasYieldedSyncerPriority_.load());
    // audio yielded the priority: max media progress is unbounded so video can take over
    EXPECT_EQ(syncCenter->GetMaxMediaProgress(), INT64_MAX);
}

/**
 * @tc.name: AudioSinkFilter_ChangeTrack_0100
 * @tc.desc: Test ChangeTrack with null audioSink_ returns ERROR_INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterAudioTrackTest, AudioSinkFilter_ChangeTrack_0100, TestSize.Level1)
{
    audioSinkFilter_->audioSink_ = nullptr;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    EXPECT_EQ(audioSinkFilter_->ChangeTrack(meta), Status::ERROR_INVALID_STATE);
}

/**
 * @tc.name: AudioSinkFilter_ChangeTrack_0200
 * @tc.desc: Test ChangeTrack with null meta returns ERROR_NULL_POINTER without render info copy
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterAudioTrackTest, AudioSinkFilter_ChangeTrack_0200, TestSize.Level1)
{
    audioSinkFilter_->audioSink_ = std::make_shared<AudioSink>();
    audioSinkFilter_->globalMeta_ = std::make_shared<Meta>();
    std::shared_ptr<Meta> meta = nullptr;
    EXPECT_EQ(audioSinkFilter_->ChangeTrack(meta), Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: AudioSinkFilter_ChangeTrack_0300
 * @tc.desc: Test ChangeTrack does not copy AUDIO_RENDER_INFO when globalMeta_ lacks it
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterAudioTrackTest, AudioSinkFilter_ChangeTrack_0300, TestSize.Level1)
{
    audioSinkFilter_->audioSink_ = std::make_shared<AudioSink>();
    // globalMeta_ is nullptr: no render info available to copy
    audioSinkFilter_->globalMeta_ = nullptr;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    audioSinkFilter_->ChangeTrack(meta);
    Plugins::AudioRenderInfo renderInfo;
    EXPECT_FALSE(meta->GetData(Tag::AUDIO_RENDER_INFO, renderInfo));

    // globalMeta_ exists but has no AUDIO_RENDER_INFO: nothing is copied either
    audioSinkFilter_->globalMeta_ = std::make_shared<Meta>();
    std::shared_ptr<Meta> metaWithoutRenderInfo = std::make_shared<Meta>();
    audioSinkFilter_->ChangeTrack(metaWithoutRenderInfo);
    EXPECT_FALSE(metaWithoutRenderInfo->GetData(Tag::AUDIO_RENDER_INFO, renderInfo));
}

/**
 * @tc.name: AudioSinkFilter_ChangeTrack_0400
 * @tc.desc: Test ChangeTrack copies AUDIO_RENDER_INFO from globalMeta_ to the track meta
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterAudioTrackTest, AudioSinkFilter_ChangeTrack_0400, TestSize.Level1)
{
    audioSinkFilter_->audioSink_ = std::make_shared<AudioSink>();
    std::shared_ptr<Meta> globalMeta = std::make_shared<Meta>();
    Plugins::AudioRenderInfo renderInfo;
    renderInfo.contentType = 1;
    renderInfo.streamUsage = 2;
    renderInfo.rendererFlags = 3;
    globalMeta->SetData(Tag::AUDIO_RENDER_INFO, renderInfo);
    audioSinkFilter_->globalMeta_ = globalMeta;

    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    audioSinkFilter_->ChangeTrack(meta);
    Plugins::AudioRenderInfo copiedRenderInfo;
    ASSERT_TRUE(meta->GetData(Tag::AUDIO_RENDER_INFO, copiedRenderInfo));
    EXPECT_EQ(copiedRenderInfo.contentType, renderInfo.contentType);
    EXPECT_EQ(copiedRenderInfo.streamUsage, renderInfo.streamUsage);
    EXPECT_EQ(copiedRenderInfo.rendererFlags, renderInfo.rendererFlags);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
