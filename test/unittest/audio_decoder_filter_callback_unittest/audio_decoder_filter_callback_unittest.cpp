/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "audio_decoder_filter_callback_unittest.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Pipeline;
namespace OHOS::Media {

void AudioDecoderFilterCallBackUnitTest::SetUpTestCase(void) {}

void AudioDecoderFilterCallBackUnitTest::TearDownTestCase(void) {}

void AudioDecoderFilterCallBackUnitTest::SetUp(void)
{
    audioDecoderFilter_ = std::make_shared<Pipeline::AudioDecoderFilter>("test", FilterType::FILTERTYPE_AENC);
}

void AudioDecoderFilterCallBackUnitTest::TearDown(void)
{
    audioDecoderFilter_ = nullptr;
}

/**
 * @tc.name  : Test OnUnlinkedResult
 * @tc.number: CallBackOnUnlinkedResult_001
 * @tc.desc  : Test (auto codecFilter = codecFilter_.lock()) == false
 *             Test (auto codecFilter = codecFilter_.lock()) == true
 */
HWTEST_F(AudioDecoderFilterCallBackUnitTest, CallBackOnUnlinkedResult_001, TestSize.Level0)
{
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoderFilter = nullptr;
    auto testPtr = std::make_shared<AudioDecoderFilterLinkCallback>(audioDecoderFilter);

    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    testPtr->OnUnlinkedResult(meta);

    audioDecoderFilter_->isReleased_.store(true);
    testPtr = std::make_shared<AudioDecoderFilterLinkCallback>(audioDecoderFilter_);
    testPtr->OnUnlinkedResult(meta);
    auto ptr = testPtr->codecFilter_.lock();
    EXPECT_NE(ptr->meta_, nullptr);
}

/**
 * @tc.name  : Test OnUpdatedResult
 * @tc.number: CallBackOnUpdatedResult_001
 * @tc.desc  : Test (auto codecFilter = codecFilter_.lock()) == false
 *             Test (auto codecFilter = codecFilter_.lock()) == true
 */
HWTEST_F(AudioDecoderFilterCallBackUnitTest, CallBackOnUpdatedResult_001, TestSize.Level0)
{
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoderFilter = nullptr;
    auto testPtr = std::make_shared<AudioDecoderFilterLinkCallback>(audioDecoderFilter);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    testPtr->OnUpdatedResult(meta);

    audioDecoderFilter_->isReleased_.store(true);
    testPtr = std::make_shared<AudioDecoderFilterLinkCallback>(audioDecoderFilter_);
    testPtr->OnUpdatedResult(meta);
    auto ptr = testPtr->codecFilter_.lock();
    EXPECT_NE(ptr->meta_, nullptr);
}

/**
 * @tc.name  : Test OnBufferAvailable
 * @tc.number: CallBackOnBufferAvailable_001
 * @tc.desc  : Test AudioDecInputPortConsumerListener:(auto codecFilter = audioDecoderFilter_.lock()) == false
 *             Test AudioDecOutPortProducerListener:(auto codecFilter = audioDecoderFilter_.lock()) == false
 */
HWTEST_F(AudioDecoderFilterCallBackUnitTest, CallBackOnBufferAvailable_001, TestSize.Level0)
{
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoderFilter = nullptr;
    auto testPtr = std::make_shared<AudioDecInputPortConsumerListener>(audioDecoderFilter);
    testPtr->OnBufferAvailable();
    
    audioDecoderFilter_->isReleased_.store(true);
    auto testPtr2 = std::make_shared<AudioDecOutPortProducerListener>(audioDecoderFilter);
    testPtr2->OnBufferAvailable();
    auto ptr = testPtr->audioDecoderFilter_.lock();
    EXPECT_EQ(ptr, audioDecoderFilter);
}

/**
 * @tc.name: OnOutputFormatChanged_001
 * @tc.desc: Test AudioDecoderCallback OnOutputFormatChanged interface normal case
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderFilterCallBackUnitTest, OnOutputFormatChanged_001, TestSize.Level1)
{
    ASSERT_NE(audioDecoderFilter_, nullptr);
    auto callback = std::make_shared<AudioDecoderCallback>(audioDecoderFilter_);
    ASSERT_NE(callback, nullptr);
    Format testFormat;
    testFormat.PutStringValue("mime", "audio/mp3");
    testFormat.PutIntValue("sample_rate", 44100);
    testFormat.PutIntValue("channels", 2);
    callback->OnOutputFormatChanged(testFormat);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: OnOutputFormatChanged_002
 * @tc.desc: Test AudioDecoderCallback OnOutputFormatChanged interface empty format case
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderFilterCallBackUnitTest, OnOutputFormatChanged_002, TestSize.Level1)
{
    ASSERT_NE(audioDecoderFilter_, nullptr);
    auto callback = std::make_shared<AudioDecoderCallback>(audioDecoderFilter_);
    ASSERT_NE(callback, nullptr);
    Format emptyFormat;
    callback->OnOutputFormatChanged(emptyFormat);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: OnOutputFormatChanged_003
 * @tc.desc: Test AudioDecoderCallback OnOutputFormatChanged interface null filter case
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderFilterCallBackUnitTest, OnOutputFormatChanged_003, TestSize.Level1)
{
    auto callback = std::make_shared<AudioDecoderCallback>(nullptr);
    ASSERT_NE(callback, nullptr);
    Format testFormat;
    testFormat.PutStringValue("mime", "audio/aac");
    testFormat.PutIntValue("sample_rate", 48000);
    testFormat.PutIntValue("channels", 6);
    callback->OnOutputFormatChanged(testFormat);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: OnOutputFormatChanged_MultipleCallTest_004
 * @tc.desc: Test AudioDecoderCallback OnOutputFormatChanged interface multiple call case
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderFilterCallBackUnitTest, OnOutputFormatChanged_004, TestSize.Level1)
{
    ASSERT_NE(audioDecoderFilter_, nullptr);
    auto callback = std::make_shared<AudioDecoderCallback>(audioDecoderFilter_);
    ASSERT_NE(callback, nullptr);
    Format format1;
    format1.PutStringValue("mime", "audio/aac");
    format1.PutIntValue("sample_rate", 44100);
    format1.PutIntValue("channels", 2);
    
    Format format2;
    format2.PutStringValue("mime", "audio/mp3");
    format2.PutIntValue("sample_rate", 48000);
    format2.PutIntValue("channels", 6);
    callback->OnOutputFormatChanged(format1);
    callback->OnOutputFormatChanged(format2);
    callback->OnOutputFormatChanged(format1);
    EXPECT_TRUE(true);
}
} // namespace OHOS::Media