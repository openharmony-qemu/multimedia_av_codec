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

#include "codec_capability_adapter_unittest.h"
#include "gmock/gmock.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing;
using namespace testing::ext;

const static int32_t TEST_TIMES_ONE = 1;
const static int32_t TEST_TIMES_THREE = 3;
const static int32_t TEST_TIMES_FOUR = 4;
const static int32_t TEST_VIDEO_WATERMARK = 3;
const static int32_t TEST_VIDEO_RPR = 4;

namespace OHOS {
namespace Media {
namespace Pipeline {
void CodecCapabilityAdapterUnitTest::SetUpTestCase(void) {}

void CodecCapabilityAdapterUnitTest::TearDownTestCase(void) {}

void CodecCapabilityAdapterUnitTest::SetUp(void)
{
    mockAvcodecList_ = std::make_shared<MockAVCodecList>();
    codecCapabilityAdapter_ = std::make_shared<CodecCapabilityAdapter>();
}

void CodecCapabilityAdapterUnitTest::TearDown(void)
{
    mockAvcodecList_ = nullptr;
    codecCapabilityAdapter_ = nullptr;
}

/**
 * @tc.name  : Test IsWatermarkSupported
 * @tc.number: IsWatermarkSupported_001
 * @tc.desc  : Test IsWatermarkSupported (capabilityData->featuresMap.count(
               static_cast<int32_t>(MediaAVCodec::AVCapabilityFeature::VIDEO_WATERMARK))) == false
 */
HWTEST_F(CodecCapabilityAdapterUnitTest, IsWatermarkSupported_001, TestSize.Level1)
{
    capabilityData_.featuresMap.insert(std::pair<int32_t, Format>(TEST_VIDEO_RPR, Format()));
    EXPECT_CALL(*(mockAvcodecList_), GetCapability(_, _, _)).Times(TEST_TIMES_ONE).WillOnce(Return(&capabilityData_));
    codecCapabilityAdapter_->codeclist_ = mockAvcodecList_;
    std::string codecMimeType = "";
    bool isWatermarkSupported = true;
    codecCapabilityAdapter_->IsWatermarkSupported(codecMimeType, isWatermarkSupported);
    EXPECT_TRUE(!isWatermarkSupported);
}

/**
 * @tc.name  : Test IsWatermarkSupported
 * @tc.number: IsWatermarkSupported_002
 * @tc.desc  : Test IsWatermarkSupported if (capabilityData->featuresMap.count(
               static_cast<int32_t>(MediaAVCodec::AVCapabilityFeature::VIDEO_WATERMARK)))
 */
HWTEST_F(CodecCapabilityAdapterUnitTest, IsWatermarkSupported_002, TestSize.Level1)
{
    EXPECT_CALL(*(mockAvcodecList_), GetCapability(_, _, _))
        .Times(TEST_TIMES_FOUR)
        .WillOnce(Return(nullptr))
        .WillOnce(Return(&capabilityData_))
        .WillOnce(Return(nullptr))
        .WillOnce(Return(&capabilityData_));
    codecCapabilityAdapter_->codeclist_ = mockAvcodecList_;
    std::string codecMimeType = "";
    bool isWatermarkSupported = true;
    codecCapabilityAdapter_->IsWatermarkSupported(codecMimeType, isWatermarkSupported);
    EXPECT_TRUE(!isWatermarkSupported);

    capabilityData_.featuresMap.insert(std::pair<int32_t, Format>(TEST_VIDEO_WATERMARK, Format()));
    codecCapabilityAdapter_->IsWatermarkSupported(codecMimeType, isWatermarkSupported);
    EXPECT_TRUE(isWatermarkSupported);
}

/**
 * @tc.name  : Test IsWatermarkSupported
 * @tc.number: IsWatermarkSupported_003
 * @tc.desc  : Test IsWatermarkSupported capabilityData == nullptr
 */
HWTEST_F(CodecCapabilityAdapterUnitTest, IsWatermarkSupported_003, TestSize.Level1)
{
    EXPECT_CALL(*(mockAvcodecList_), GetCapability(_, _, _)).WillRepeatedly(Return(nullptr));
    codecCapabilityAdapter_->codeclist_ = mockAvcodecList_;
    std::string codecMimeType = "";
    bool isWatermarkSupported = true;
    Status status = codecCapabilityAdapter_->IsWatermarkSupported(codecMimeType, isWatermarkSupported);
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test GetAudioEncoder
 * @tc.number: GetAudioEncoder_001
 * @tc.desc  : Test GetVideoEncoder capabilityData == nullptr
 */
HWTEST_F(CodecCapabilityAdapterUnitTest, GetAudioEncoder_001, TestSize.Level1)
{
    EXPECT_CALL(*(mockAvcodecList_), GetCapability(_, _, _))
        .Times(TEST_TIMES_ONE)
        .WillOnce(Return(nullptr));
    codecCapabilityAdapter_->codeclist_ = mockAvcodecList_;
    std::vector<MediaAVCodec::CapabilityData*> dataVector;
    codecCapabilityAdapter_->GetAudioEncoder(dataVector);
    EXPECT_TRUE(dataVector.empty());
}

/**
 * @tc.name  : Test GetVideoEncoder
 * @tc.number: GetVideoEncoder_001
 * @tc.desc  : Test GetAudioEncoder capabilityDataAVC == nullptr
 *             Test GetAudioEncoder capabilityDataAVCSoft == nullptr
 *             Test GetAudioEncoder capabilityDataHEVC == nullptr
 */
HWTEST_F(CodecCapabilityAdapterUnitTest, GetVideoEncoder_001, TestSize.Level1)
{
    EXPECT_CALL(*(mockAvcodecList_), GetCapability(_, _, _)).WillRepeatedly(Return(nullptr));
    codecCapabilityAdapter_->codeclist_ = mockAvcodecList_;
    std::vector<MediaAVCodec::CapabilityData*> dataVector;
    codecCapabilityAdapter_->GetVideoEncoder(dataVector);
    EXPECT_TRUE(dataVector.empty());
}

/**
 * @tc.name  : Test GetVideoEncoder
 * @tc.number: GetVideoEncoder_002
 * @tc.desc  : Test GetAudioEncoder capabilityDataAVCSoft != nullptr
 *             Test ~CodecCapabilityAdapter codeclist_ == nullptr
 */
HWTEST_F(CodecCapabilityAdapterUnitTest, GetVideoEncoder_002, TestSize.Level1)
{
    EXPECT_CALL(*(mockAvcodecList_), GetCapability(_, _, _))
        .Times(TEST_TIMES_THREE)
        .WillOnce(Return(nullptr))
        .WillOnce(Return(&capabilityData_))
        .WillOnce(Return(nullptr));
    codecCapabilityAdapter_->codeclist_ = mockAvcodecList_;
    std::vector<MediaAVCodec::CapabilityData*> dataVector;
    codecCapabilityAdapter_->GetVideoEncoder(dataVector);
    EXPECT_EQ(dataVector.size(), 1);

    codecCapabilityAdapter_->codeclist_ = nullptr;
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS