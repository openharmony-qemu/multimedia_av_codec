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

#include "audio_decoder_filter_unittest.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Pipeline;
namespace OHOS::Media {
constexpr const int32_t ID_TEST = 0;
constexpr const int64_t NUM_TEST = 1;
constexpr const int64_t SAMEPLE_S24P = 1;

void AudioDecoderFilterUnitTest::SetUpTestCase(void) {}

void AudioDecoderFilterUnitTest::TearDownTestCase(void) {}

void AudioDecoderFilterUnitTest::SetUp(void)
{
    audioDecoderFilter_ = std::make_shared<AudioDecoderFilter>("test", FilterType::FILTERTYPE_AENC);
}

void AudioDecoderFilterUnitTest::TearDown(void)
{
    audioDecoderFilter_ = nullptr;
}

/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_001
 * @tc.desc  : Test isReleased_.load() == true
 */
HWTEST_F(AudioDecoderFilterUnitTest, DoRelease_001, TestSize.Level0)
{
    ASSERT_NE(audioDecoderFilter_, nullptr);
    audioDecoderFilter_->isReleased_.store(true);
    auto ret = audioDecoderFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test UpdateTrackInfoSampleFormat
 * @tc.number: UpdateTrackInfoSampleFormat_001
 * @tc.desc  : Test sampleRateGetRes == fasle && sampleRate < SAMPLE_RATE_48K
 *             Test sampleFormatGetRes == true
 */
HWTEST_F(AudioDecoderFilterUnitTest, UpdateTrackInfoSampleFormat_001, TestSize.Level0)
{
    ASSERT_NE(audioDecoderFilter_, nullptr);
    audioDecoderFilter_->isReleased_.store(true);
    std::string mime = "audio/x-ape";
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    audioDecoderFilter_->UpdateTrackInfoSampleFormat(mime, meta);
    int32_t sampleRate = NUM_TEST;
    meta->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleRate);
    EXPECT_EQ(sampleRate, Plugins::SAMPLE_S16LE);

    meta->SetData(Tag::AUDIO_SAMPLE_RATE, ID_TEST);
    meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, SAMEPLE_S24P);
    audioDecoderFilter_->UpdateTrackInfoSampleFormat(mime, meta);
    meta->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleRate);
    EXPECT_EQ(sampleRate, SAMEPLE_S32LE);
}

/**
 * @tc.name  : Test SetDecryptionConfig
 * @tc.number: SetDecryptionConfig_001
 * @tc.desc  : Test keySessionProxy == nullptr
 */
HWTEST_F(AudioDecoderFilterUnitTest, SetDecryptionConfig_001, TestSize.Level0)
{
    ASSERT_NE(audioDecoderFilter_, nullptr);
    audioDecoderFilter_->isReleased_.store(true);
    sptr<DrmStandard::IMediaKeySessionService> keySessionProxy = nullptr;
    bool svp = true;
    auto ret = audioDecoderFilter_->SetDecryptionConfig(keySessionProxy, svp);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test IsNeedProcessInput
 * @tc.number: IsNeedProcessInput_001
 * @tc.desc  : Test bufferStatus_ != BUFFER_STATUS_AVAIL_NONE
 */
HWTEST_F(AudioDecoderFilterUnitTest, IsNeedProcessInput_001, TestSize.Level0)
{
    ASSERT_NE(audioDecoderFilter_, nullptr);
    audioDecoderFilter_->isReleased_.store(true);
    audioDecoderFilter_->bufferStatus_ = static_cast<uint32_t>(InOutPortBufferStatus::INIT_IGNORE_RET);
    bool isOutPort = true;
    auto ret = audioDecoderFilter_->IsNeedProcessInput(isOutPort);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: OnOutputFormatChanged_001
 * @tc.desc: Test AudioDecoderFilter OnOutputFormatChanged interface normal case
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderFilterUnitTest, OnOutputFormatChanged_001, TestSize.Level1)
{
    ASSERT_NE(audioDecoderFilter_, nullptr);
    Format testFormat;
    testFormat.PutStringValue("mime", "audio/aac");
    testFormat.PutIntValue("sample_rate", 44100);
    testFormat.PutIntValue("channels", 2);
    audioDecoderFilter_->OnOutputFormatChanged(testFormat);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: OnOutputFormatChanged_002
 * @tc.desc: Test AudioDecoderFilter OnOutputFormatChanged interface empty format case
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderFilterUnitTest, OnOutputFormatChanged_002, TestSize.Level1)
{
    ASSERT_NE(audioDecoderFilter_, nullptr);
    Format emptyFormat;
    audioDecoderFilter_->OnOutputFormatChanged(emptyFormat);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: OnOutputFormatChanged_003
 * @tc.desc: Test AudioDecoderFilter OnOutputFormatChanged interface vivid format case
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderFilterUnitTest, OnOutputFormatChanged_003, TestSize.Level1)
{
    ASSERT_NE(audioDecoderFilter_, nullptr);
    Format vividFormat;
    vividFormat.PutStringValue("mime", "audio/vivid");
    vividFormat.PutIntValue("sample_rate", 48000);
    vividFormat.PutIntValue("channels", 6);
    audioDecoderFilter_->OnOutputFormatChanged(vividFormat);
    EXPECT_TRUE(true);
}
} // namespace OHOS::Media