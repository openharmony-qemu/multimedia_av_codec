/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "audio_sink_filter.h"
#include "common/log.h"
#include "osal/utils/util.h"
#include "osal/utils/dump_buffer.h"
#include "filter/filter_factory.h"
#include "media_core.h"
#include "parameters.h"
#include "audio_sink_filter_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void AudioSinkFilterUnitTest::SetUpTestCase(void) {}

void AudioSinkFilterUnitTest::TearDownTestCase(void) {}

void AudioSinkFilterUnitTest::SetUp(void)
{
    audioSinkFilter_ = std::make_shared<AudioSinkFilter>("testAudioSinkFilter", FilterType::FILTERTYPE_VIDEODEC);
    audioSinkFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
}

void AudioSinkFilterUnitTest::TearDown(void)
{
    audioSinkFilter_->eventReceiver_ = nullptr;
    audioSinkFilter_ = nullptr;
}

/**
 * @tc.name: AudioSinkFilter_DoPrepare_0100
 * @tc.desc: DoPrepare
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_DoPrepare_0100, TestSize.Level1)
{
    audioSinkFilter_->inputBufferQueueConsumer_ = sptr<Media::AVBufferQueueConsumer>();
    audioSinkFilter_->onLinkedResultCallback_ = std::make_shared<MyFilterLinkCallback>();
    EXPECT_EQ(audioSinkFilter_->DoPrepare(), Status::OK);
}

/**
 * @tc.name: AudioSinkFilter_DoStart_0100
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_DoStart_0100, TestSize.Level1)
{
    audioSinkFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    audioSinkFilter_->state_ = FilterState::RUNNING;
    EXPECT_EQ(audioSinkFilter_->DoStart(), Status::OK);
    audioSinkFilter_->state_ = FilterState::CREATED;
    EXPECT_EQ(audioSinkFilter_->DoStart(), Status::ERROR_INVALID_OPERATION);
    audioSinkFilter_->state_ = FilterState::READY;
    audioSinkFilter_->state_ = FilterState::PAUSED;
    audioSinkFilter_->DoStart();
    audioSinkFilter_->state_ = FilterState::CREATED;
    EXPECT_EQ(audioSinkFilter_->DoStart(), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: AudioSinkFilter_DoPause_0100
 * @tc.desc: DoPause
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_DoPause_0100, TestSize.Level1)
{
    audioSinkFilter_->state_ = FilterState::PAUSED;
    audioSinkFilter_->state_ = FilterState::STOPPED;
    EXPECT_EQ(audioSinkFilter_->DoPause(), Status::OK);
    audioSinkFilter_->state_ = FilterState::ERROR;
    audioSinkFilter_->state_ = FilterState::RELEASED;
    EXPECT_EQ(audioSinkFilter_->DoPause(), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: AudioSinkFilter_DoResume_0100
 * @tc.desc: DoResume
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_DoResume_0100, TestSize.Level1)
{
    audioSinkFilter_->state_ = FilterState::PAUSED;
    audioSinkFilter_->frameCnt_ = 1;
    audioSinkFilter_->DoResume();
    audioSinkFilter_->state_ = FilterState::STOPPED;
    EXPECT_EQ(audioSinkFilter_->DoResume(), Status::OK);
}

/**
 * @tc.name: AudioSinkFilter_DoFlush_0100
 * @tc.desc: DoFlush
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_DoFlush_0100, TestSize.Level1)
{
    audioSinkFilter_->DoFlush();
    audioSinkFilter_->audioSink_ = nullptr;
    EXPECT_EQ(audioSinkFilter_->DoFlush(), Status::OK);
}

/**
 * @tc.name: AudioSinkFilter_DoStop_0100
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_DoStop_0100, TestSize.Level1)
{
    audioSinkFilter_->DoStop();
    audioSinkFilter_->audioSink_ = nullptr;
    EXPECT_EQ(audioSinkFilter_->DoStop(), Status::OK);
}

/**
 * @tc.name: AudioSinkFilter_OnLinked_0100
 * @tc.desc: OnLinked
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_OnLinked_0100, TestSize.Level1)
{
    StreamType inType = StreamType::STREAMTYPE_MAX;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    std::shared_ptr<FilterLinkCallback> callback = std::make_shared<MyFilterLinkCallback>();

    audioSinkFilter_->globalMeta_ = std::make_shared<Meta>();
    audioSinkFilter_->OnLinked(inType, meta, callback);
    meta = nullptr;
    audioSinkFilter_->OnLinked(inType, meta, callback);
    EXPECT_EQ(audioSinkFilter_->trackMeta_, meta);
}

/**
 * @tc.name: AudioSinkFilter_SetVolume_0100
 * @tc.desc: SetVolume
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_SetVolume_0100, TestSize.Level1)
{
    float volume = -1;
    EXPECT_EQ(audioSinkFilter_->SetVolume(volume), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: AudioSinkFilter_SetSpeed_0100
 * @tc.desc: SetSpeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_SetSpeed_0100, TestSize.Level1)
{
    EXPECT_EQ(audioSinkFilter_->SetSpeed(-1), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: AudioSinkFilter_HandleFormatChange_0100
 * @tc.desc: HandleFormatChange
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_HandleFormatChange_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = nullptr;
    EXPECT_EQ(audioSinkFilter_->HandleFormatChange(meta), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: AudioSinkFilter_HandleFormatChange_0200
 * @tc.desc: HandleFormatChange
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_HandleFormatChange_0200, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    audioSinkFilter_->audioSink_->plugin_ = nullptr;
    EXPECT_EQ(audioSinkFilter_->HandleFormatChange(meta), Status::ERROR_INVALID_STATE);
}

/**
 * @tc.name: AudioSinkFilter_HandleFormatChange_0300
 * @tc.desc: HandleFormatChange
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_HandleFormatChange_0300, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    EXPECT_EQ(audioSinkFilter_->HandleFormatChange(meta), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: AudioSinkFilter_HandleFormatChange_0400
 * @tc.desc: HandleFormatChange
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_HandleFormatChange_0400, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::AUDIO_SAMPLE_RATE, 0);
    EXPECT_EQ(audioSinkFilter_->HandleFormatChange(meta), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: AudioSinkFilter_HandleFormatChange_0500
 * @tc.desc: HandleFormatChange
 * @tc.type: FUNC
 */
HWTEST_F(AudioSinkFilterUnitTest, AudioSinkFilter_HandleFormatChange_0500, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::AUDIO_SAMPLE_RATE, 44100);
    meta->SetData(Tag::AUDIO_CHANNEL_COUNT, 1);
    meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_NE(audioSinkFilter_->HandleFormatChange(meta), Status::OK);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS