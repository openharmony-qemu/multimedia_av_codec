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
    auto mockMediaCodec = std::make_shared<MediaCodec>();
    audioEncoderFilter_->mediaCodec_ = mockMediaCodec;
    sptr<Surface> mockSurface = nullptr;
    EXPECT_CALL(*mockMediaCodec, GetInputSurface()).WillOnce(Return(mockSurface));
    auto ret = audioEncoderFilter_->GetInputSurface();
    EXPECT_EQ(ret, mockSurface);
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

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS