/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "common/log.h"
#include "parameters.h"
#include "audio_encoder_filter_unittest.h"
 
namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing::ext;
using namespace testing;

void AudioEncoderFilterUnitest::SetUpTestCase(void) {}
 
void AudioEncoderFilterUnitest::TearDownTestCase(void) {}
 
void AudioEncoderFilterUnitest::SetUp(void)
{
    audioEncoderFilter_ =
        std::make_shared<AudioEncoderFilter>("testAudioEncoderFilter", FilterType::FILTERTYPE_AENC);
    ASSERT_NE(audioEncoderFilter_, nullptr);
}
 
void AudioEncoderFilterUnitest::TearDown(void)
{
    audioEncoderFilter_ = nullptr;
}
/**
 * @tc.name  : Test OnLinkedResult
 * @tc.number: OnLinkedResult_001
 * @tc.desc  : Test OnUnlinkedResult encoderFilter == nullptr
 *             Test OnUpdatedResult encoderFilter == nullptr
 *             Test OnLinkedResult encoderFilter != nullptr
 *             Test OnUnlinkedResult encoderFilter != nullptr
 *             Test OnUpdatedResult encoderFilter != nullptr
 */
HWTEST_F(AudioEncoderFilterUnitest, OnLinkedResult_001, TestSize.Level0)
{
    auto linkCallback = std::make_shared<AudioEncoderFilterLinkCallback>(nullptr);
    sptr<AVBufferQueueProducer> outputBufferQueue = nullptr;
    std::shared_ptr<Meta> meta = nullptr;
    linkCallback->OnUnlinkedResult(meta);
    linkCallback->OnUpdatedResult(meta);
    audioEncoderFilter_->mediaCodec_ = std::make_shared<MediaCodec>();
    ASSERT_NE(audioEncoderFilter_->mediaCodec_, nullptr);
    audioEncoderFilter_->onLinkedResultCallback_ = std::make_shared<MockFilterLinkCallback>();
    audioEncoderFilter_->mediaCodec_ = std::make_shared<MediaCodec>();
    linkCallback = std::make_shared<AudioEncoderFilterLinkCallback>(audioEncoderFilter_);
    EXPECT_CALL(*(audioEncoderFilter_->mediaCodec_),
        SetOutputBufferQueue(_)).WillRepeatedly(Return((int32_t)Status::OK));
    EXPECT_CALL(*(audioEncoderFilter_->mediaCodec_), Prepare()).WillRepeatedly(Return((int32_t)Status::OK));
    auto ptr = linkCallback->audioEncoderFilter_.lock();
    ptr->onLinkedResultCallback_ = nullptr;
    linkCallback->OnLinkedResult(outputBufferQueue, meta);
    linkCallback->OnUnlinkedResult(meta);
    linkCallback->OnUpdatedResult(meta);
    EXPECT_CALL(*(audioEncoderFilter_->mediaCodec_), SetOutputBufferQueue(_)).Times(0);
    EXPECT_CALL(*(audioEncoderFilter_->mediaCodec_), Prepare()).Times(0);
    ptr->mediaCodec_ = nullptr;
    linkCallback = nullptr;
}

/**
 * @tc.name  : Test GetInputSurface
 * @tc.number: GetInputSurface_001
 * @tc.desc  : Test GetInputSurface with valid mediaCodec_
 */
HWTEST_F(AudioEncoderFilterUnitest, GetInputSurface_001, TestSize.Level0)
{
    ASSERT_NE(audioEncoderFilter_, nullptr);
    auto mediaCodec = std::make_shared<MediaCodec>();
    audioEncoderFilter_->mediaCodec_ = mediaCodec;
    auto ret = audioEncoderFilter_->GetInputSurface();
    EXPECT_EQ(ret, mediaCodec->GetInputSurface());
}

/**
 * @tc.name  : Test GetInputSurface
 * @tc.number: GetInputSurface_002
 * @tc.desc  : Test GetInputSurface with nullptr mediaCodec_
 */
HWTEST_F(AudioEncoderFilterUnitest, GetInputSurface_002, TestSize.Level0)
{
    ASSERT_NE(audioEncoderFilter_, nullptr);
    audioEncoderFilter_->mediaCodec_ = nullptr;
    auto ret = audioEncoderFilter_->GetInputSurface();
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name  : Test SetClipStartTimeUs
 * @tc.number: SetClipStartTimeUs_001
 * @tc.desc  : SetClipStartTimeUs stores the clip start time to clipStartTimeUs_
 */
HWTEST_F(AudioEncoderFilterUnitest, SetClipStartTimeUs_001, TestSize.Level0)
{
    ASSERT_NE(audioEncoderFilter_, nullptr);
    int64_t expected = 5000000;
    audioEncoderFilter_->SetClipStartTimeUs(expected);
    EXPECT_EQ(audioEncoderFilter_->clipStartTimeUs_, expected);
}

/**
 * @tc.name  : Test SetClipStartTimeUs with negative value
 * @tc.number: SetClipStartTimeUs_002
 * @tc.desc  : SetClipStartTimeUs accepts negative sentinel value (-1)
 */
HWTEST_F(AudioEncoderFilterUnitest, SetClipStartTimeUs_002, TestSize.Level0)
{
    ASSERT_NE(audioEncoderFilter_, nullptr);
    audioEncoderFilter_->SetClipStartTimeUs(-1);
    EXPECT_EQ(audioEncoderFilter_->clipStartTimeUs_, -1);
}

/**
 * @tc.name  : Test SetClipTimeFilterNeeded
 * @tc.number: SetClipTimeFilterNeeded_001
 * @tc.desc  : SetClipTimeFilterNeeded sets needClipTimeFilter_ to true
 */
HWTEST_F(AudioEncoderFilterUnitest, SetClipTimeFilterNeeded_001, TestSize.Level0)
{
    ASSERT_NE(audioEncoderFilter_, nullptr);
    audioEncoderFilter_->needClipTimeFilter_.store(false);
    audioEncoderFilter_->SetClipTimeFilterNeeded();
    EXPECT_TRUE(audioEncoderFilter_->needClipTimeFilter_.load());
}

/**
 * @tc.name  : Test SetTranscoderMode
 * @tc.number: SetTranscoderMode_001
 * @tc.desc  : SetTranscoderMode sets isTranscoderMode_ to true
 */
HWTEST_F(AudioEncoderFilterUnitest, SetTranscoderMode_001, TestSize.Level0)
{
    ASSERT_NE(audioEncoderFilter_, nullptr);
    audioEncoderFilter_->isTranscoderMode_ = false;
    auto ret = audioEncoderFilter_->SetTranscoderMode();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(audioEncoderFilter_->isTranscoderMode_);
}

/**
 * @tc.name  : Test SetPureAudioMode
 * @tc.number: SetPureAudioMode_001
 * @tc.desc  : SetPureAudioMode sets isPureAudioMode_ to true
 */
HWTEST_F(AudioEncoderFilterUnitest, SetPureAudioMode_001, TestSize.Level0)
{
    ASSERT_NE(audioEncoderFilter_, nullptr);
    audioEncoderFilter_->isPureAudioMode_.store(false);
    auto ret = audioEncoderFilter_->SetPureAudioMode();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(audioEncoderFilter_->isPureAudioMode_.load());
}

/**
 * @tc.name  : Test ProcessIntermediateBuffer clip filter disabled
 * @tc.number: ProcessIntermediateBuffer_001
 * @tc.desc  : ProcessIntermediateBuffer returns early when intermediateQueueConsumer_ is nullptr
 */
HWTEST_F(AudioEncoderFilterUnitest, ProcessIntermediateBuffer_001, TestSize.Level0)
{
    ASSERT_NE(audioEncoderFilter_, nullptr);
    audioEncoderFilter_->intermediateQueueConsumer_ = nullptr;
    // Should return early without crash due to FALSE_RETURN_MSG guard
    audioEncoderFilter_->ProcessIntermediateBuffer();
    // No crash, no side effects
}

/**
 * @tc.name  : Test ProcessIntermediateBuffer with clip filter enabled but no buffer
 * @tc.number: ProcessIntermediateBuffer_002
 * @tc.desc  : ProcessIntermediateBuffer with clip filter enabled and consumer set, no buffer available
 */
HWTEST_F(AudioEncoderFilterUnitest, ProcessIntermediateBuffer_002, TestSize.Level0)
{
    ASSERT_NE(audioEncoderFilter_, nullptr);
    // Set up clip filter mode
    audioEncoderFilter_->needClipTimeFilter_.store(true);
    audioEncoderFilter_->clipStartTimeUs_ = 1000000;
    // Set isProcessingRunning_ so do-while condition is met
    audioEncoderFilter_->isProcessingRunning_.store(true);
    // consumer is nullptr — should return early
    audioEncoderFilter_->intermediateQueueConsumer_ = nullptr;
    audioEncoderFilter_->ProcessIntermediateBuffer();
    // No crash — FALSE_RETURN_MSG guard on intermediateQueueConsumer_
    audioEncoderFilter_->needClipTimeFilter_.store(false);
    audioEncoderFilter_->clipStartTimeUs_ = -1;
    audioEncoderFilter_->isProcessingRunning_.store(false);
}

/**
 * @tc.name  : Test AcquireIntermediateBuffer with nullptr consumer
 * @tc.number: AcquireIntermediateBuffer_001
 * @tc.desc  : AcquireIntermediateBuffer returns ERROR_NULL_POINTER when consumer is nullptr
 */
HWTEST_F(AudioEncoderFilterUnitest, AcquireIntermediateBuffer_001, TestSize.Level0)
{
    ASSERT_NE(audioEncoderFilter_, nullptr);
    audioEncoderFilter_->intermediateQueueConsumer_ = nullptr;
    std::shared_ptr<AVBuffer> buffer;
    auto ret = audioEncoderFilter_->AcquireIntermediateBuffer(buffer);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS