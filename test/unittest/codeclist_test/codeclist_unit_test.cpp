/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "codeclist_unit_test.h"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "gtest/gtest.h"

using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::CodecListTestParam;

void CodecListUnitTest::SetUpTestCase() {}

void CodecListUnitTest::TearDownTestCase() {}

void CodecListUnitTest::SetUp() {}

void CodecListUnitTest::TearDown() {}

string VectorToString(const std::vector<int32_t> list, const std::string listName)
{
    std::string ret = listName;
    ret += "={";
    for (auto it = list.begin(); it != list.end(); ++it) {
        ret += std::to_string(*it) + ", ";
    }
    ret += "}";
    return ret;
}

/**
 * @tc.name: CodecList_GetCapabilityByCategory_001
 * @tc.desc: CodecList GetCapabilityByCategory
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetCapabilityByCategory_001, TestSize.Level1)
{
    capability_ = CodecListMockFactory::GetCapability(DEFAULT_VIDEO_MIME, false);
    if (capability_ != nullptr && capability_->IsHardware()) {
        isHardIncluded_ = true;
    }
    AVCodecCategory category;
    if (isHardIncluded_) {
        category = AVCodecCategory::AVCODEC_HARDWARE;
        capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
        EXPECT_NE(nullptr, capability_);

        capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, true, category);
        EXPECT_NE(nullptr, capability_);
    }
    category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_AUDIO_MIME, false, category);
    EXPECT_NE(nullptr, capability_);

    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_AUDIO_MIME, true, category);
    EXPECT_NE(nullptr, capability_);

    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    EXPECT_NE(nullptr, capability_);
}

/**
 * @tc.name: CodecList_GetCapability_001
 * @tc.desc: CodecList GetCapability
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetCapability_001, TestSize.Level1)
{
    for (auto it = CAPABILITY_DECODER_NAME.begin(); it != CAPABILITY_DECODER_NAME.end(); ++it) {
        std::string mime = it->first;
        capability_ = CodecListMockFactory::GetCapability(mime, false);
        EXPECT_NE(nullptr, capability_) << mime << " can not found!" << std::endl;
    }
    for (auto it = CAPABILITY_ENCODER_NAME.begin(); it != CAPABILITY_ENCODER_NAME.end(); ++it) {
        std::string mime = it->first;
        capability_ = CodecListMockFactory::GetCapability(mime, true);
        EXPECT_NE(nullptr, capability_) << mime << " can not found!" << std::endl;
    }
    if (isHardIncluded_) {
        for (auto it = CAPABILITY_DECODER_HARD_NAME.begin(); it != CAPABILITY_DECODER_HARD_NAME.end(); ++it) {
            std::string mime = it->first;
            capability_ = CodecListMockFactory::GetCapability(mime, false);
            EXPECT_NE(nullptr, capability_) << mime << " can not found!" << std::endl;
        }

        for (auto it = CAPABILITY_ENCODER_HARD_NAME.begin(); it != CAPABILITY_ENCODER_HARD_NAME.end(); ++it) {
            std::string mime = it->first;
            capability_ = CodecListMockFactory::GetCapability(mime, true);
            EXPECT_NE(nullptr, capability_) << mime << " can not found!" << std::endl;
        }
    }
}

/**
 * @tc.name: CodecList_IsHardware_001
 * @tc.desc: CodecList IsHardware
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_IsHardware_001, TestSize.Level1)
{
    // audio decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_AUDIO_MIME, false, category);
    ASSERT_NE(nullptr, capability_);
    EXPECT_FALSE(capability_->IsHardware());

    // audio encoder
    category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_AUDIO_MIME, false, category);
    ASSERT_NE(nullptr, capability_);
    EXPECT_FALSE(capability_->IsHardware());

    // video decoder
    category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_);
    EXPECT_FALSE(capability_->IsHardware());

    if (isHardIncluded_) {
        category = AVCodecCategory::AVCODEC_HARDWARE;
        capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
        EXPECT_NE(nullptr, capability_);
        EXPECT_FALSE(capability_->IsHardware());
    }
}

/**
 * @tc.name: CodecList_GetName_001
 * @tc.desc: CodecList GetName
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetName_001, TestSize.Level1)
{
    for (auto it = CAPABILITY_DECODER_NAME.begin(); it != CAPABILITY_DECODER_NAME.end(); ++it) {
        std::string mime = it->first;
        std::string nameOfMime = it->second;
        AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
        capability_ = CodecListMockFactory::GetCapabilityByCategory(mime, false, category);
        ASSERT_NE(nullptr, capability_) << mime << " can not found!" << std::endl;
        std::string codecName = capability_->GetName();
        EXPECT_EQ(nameOfMime, codecName) << mime << " get error name: " << codecName << std::endl;
        std::cout << "codecName is out : " << codecName << std::endl;
        // std::cout << "codecName addr is out : " << (void*)codecName << std::endl;
    }
    for (auto it = CAPABILITY_ENCODER_NAME.begin(); it != CAPABILITY_ENCODER_NAME.end(); ++it) {
        std::string mime = it->first;
        std::string nameOfMime = it->second;
        AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
        capability_ = CodecListMockFactory::GetCapabilityByCategory(mime, true, category);
        ASSERT_NE(nullptr, capability_) << mime << " can not found!" << std::endl;
        std::string codecName = capability_->GetName();
        EXPECT_EQ(nameOfMime, codecName) << mime << " get error name: " << codecName << std::endl;
    }
    if (isHardIncluded_) {
        for (auto it = CAPABILITY_DECODER_HARD_NAME.begin(); it != CAPABILITY_DECODER_HARD_NAME.end(); ++it) {
            std::string mime = it->first;
            std::string nameOfMime = it->second;
            capability_ = CodecListMockFactory::GetCapability(mime, false);
            ASSERT_NE(nullptr, capability_) << mime << " can not found!" << std::endl;
            std::string codecName = capability_->GetName();
            EXPECT_EQ(nameOfMime, codecName) << mime << " get error name: " << codecName << std::endl;
        }
        for (auto it = CAPABILITY_ENCODER_HARD_NAME.begin(); it != CAPABILITY_ENCODER_HARD_NAME.end(); ++it) {
            std::string mime = it->first;
            std::string nameOfMime = it->second;
            capability_ = CodecListMockFactory::GetCapability(mime, true);
            ASSERT_NE(nullptr, capability_) << mime << " can not found!" << std::endl;
            std::string codecName = capability_->GetName();
            if (codecName != "") {
                EXPECT_EQ(nameOfMime, codecName) << mime << " get error name: " << codecName << std::endl;
            }
        }
    }
}

/**
 * @tc.name: CodecList_GetMaxSupportedInstances_001
 * @tc.desc: CodecList GetMaxSupportedInstances
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetMaxSupportedInstances_001, TestSize.Level1)
{
    // audio decoder
    capability_ = CodecListMockFactory::GetCapability(DEFAULT_AUDIO_MIME, false);
    ASSERT_NE(nullptr, capability_) << "audio decoder codeclist create fail!" << std::endl;
    EXPECT_EQ(MAX_SURPPORT_ACODEC, capability_->GetMaxSupportedInstances());

    // audio encoder
    capability_ = CodecListMockFactory::GetCapability(DEFAULT_AUDIO_MIME, true);
    ASSERT_NE(nullptr, capability_) << "audio encoder codeclist create fail!" << std::endl;
    EXPECT_EQ(MAX_SURPPORT_ACODEC, capability_->GetMaxSupportedInstances());

    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;
    EXPECT_EQ(MAX_SURPPORT_VCODEC, capability_->GetMaxSupportedInstances());
}

/**
 * @tc.name: CodecList_GetEncoderBitrateRange_001
 * @tc.desc: CodecList GetEncoderBitrateRange
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetEncoderBitrateRange_001, TestSize.Level1)
{
    // audio encoder
    capability_ = CodecListMockFactory::GetCapability(DEFAULT_AUDIO_MIME, true);
    ASSERT_NE(nullptr, capability_) << "audio encoder codeclist create fail!" << std::endl;
    Range bitrateRange = capability_->GetEncoderBitrateRange();
    EXPECT_EQ(DEFAULT_BITRATE_RANGE.minVal, bitrateRange.minVal);
    EXPECT_EQ(DEFAULT_BITRATE_RANGE.maxVal, bitrateRange.maxVal);
}

/**
 * @tc.name: CodecList_IsEncoderBitrateModeSupported_001
 * @tc.desc: CodecList IsEncoderBitrateModeSupported
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_IsEncoderBitrateModeSupported_001, TestSize.Level1)
{
    // audio encoder
    capability_ = CodecListMockFactory::GetCapability(DEFAULT_AUDIO_MIME, true);
    ASSERT_NE(nullptr, capability_) << "audio encoder codeclist create fail!" << std::endl;

    // positive case
    OH_BitrateMode bitrateMode = OH_BitrateMode::BITRATE_MODE_CBR;
    EXPECT_FALSE(capability_->IsEncoderBitrateModeSupported(bitrateMode));

    // negative case
    bitrateMode = OH_BitrateMode::BITRATE_MODE_VBR;
    EXPECT_FALSE(capability_->IsEncoderBitrateModeSupported(bitrateMode));
}

/**
 * @tc.name: CodecList_GetEncoderQualityRange_001
 * @tc.desc: CodecList GetEncoderQualityRange
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetEncoderQualityRange_001, TestSize.Level1)
{
    // audio encoder
    capability_ = CodecListMockFactory::GetCapability(DEFAULT_AUDIO_MIME, true);
    ASSERT_NE(nullptr, capability_) << "audio encoder codeclist create fail!" << std::endl;

    Range qualityRange = capability_->GetEncoderQualityRange();
    EXPECT_EQ(DEFAULT_QUALITY_RANGE.minVal, qualityRange.minVal);
    EXPECT_EQ(DEFAULT_QUALITY_RANGE.maxVal, qualityRange.maxVal);
}

/**
 * @tc.name: CodecList_GetEncoderComplexityRange_001
 * @tc.desc: CodecList GetEncoderComplexityRange
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetEncoderComplexityRange_001, TestSize.Level1)
{
    // audio encoder
    capability_ = CodecListMockFactory::GetCapability(DEFAULT_AUDIO_MIME, true);
    ASSERT_NE(nullptr, capability_) << "audio encoder codeclist create fail!" << std::endl;

    Range complexityRange = capability_->GetEncoderComplexityRange();
    EXPECT_EQ(DEFAULT_COMPLEXITY_RANGE.minVal, complexityRange.minVal);
    EXPECT_EQ(DEFAULT_COMPLEXITY_RANGE.maxVal, complexityRange.maxVal);
}

/**
 * @tc.name: CodecList_GetAudioSupportedSampleRates_001
 * @tc.desc: CodecList GetAudioSupportedSampleRates
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetAudioSupportedSampleRates_001, TestSize.Level1)
{
    // audio encoder
    capability_ = CodecListMockFactory::GetCapability(DEFAULT_AUDIO_MIME, true);
    ASSERT_NE(nullptr, capability_) << "audio encoder codeclist create fail!" << std::endl;
    std::vector<int32_t> sampleRates = capability_->GetAudioSupportedSampleRates();
    EXPECT_EQ(DEFAULT_AUDIO_ACC_SAMPLES, sampleRates);
}

/**
 * @tc.name: CodecList_GetAudioChannelsRange_001
 * @tc.desc: CodecList GetAudioChannelsRange
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetAudioChannelsRange_001, TestSize.Level1)
{
    // audio encoder
    capability_ = CodecListMockFactory::GetCapability(DEFAULT_AUDIO_MIME, true);
    ASSERT_NE(nullptr, capability_) << "audio encoder codeclist create fail!" << std::endl;

    Range channelCountRange = capability_->GetAudioChannelsRange();
    EXPECT_EQ(DEFAULT_CHANNELCOUNT_RANGE.minVal, channelCountRange.minVal);
    EXPECT_EQ(DEFAULT_CHANNELCOUNT_RANGE.maxVal, channelCountRange.maxVal);
}

/**
 * @tc.name: CodecList_GetVideoWidthAlignment_001
 * @tc.desc: CodecList GetVideoWidthAlignment
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetVideoWidthAlignment_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    int32_t widthAlignment = capability_->GetVideoWidthAlignment();
    EXPECT_EQ(DEFAULT_WIDTH_ALIGNMENT, widthAlignment);
}

/**
 * @tc.name: CodecList_GetVideoHeightAlignment_001
 * @tc.desc: CodecList GetVideoHeightAlignment
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetVideoHeightAlignment_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    int32_t heightAlignment = capability_->GetVideoHeightAlignment();
    EXPECT_EQ(DEFAULT_HEIGHT_ALIGNMENT, heightAlignment);
}

/**
 * @tc.name: CodecList_GetVideoHeightRangeForWidth_001
 * @tc.desc: CodecList GetVideoHeightRangeForWidth
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetVideoHeightRangeForWidth_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    Range widthRange = capability_->GetVideoHeightRangeForWidth(DEFAULT_WIDTH);
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.minVal, widthRange.minVal);
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.maxVal, widthRange.maxVal);
}

/**
 * @tc.name: CodecList_GetVideoWidthRangeForHeight_001
 * @tc.desc: CodecList GetVideoWidthRangeForHeight
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetVideoWidthRangeForHeight_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    Range heightRange = capability_->GetVideoWidthRangeForHeight(DEFAULT_HEIGHT);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE_OF_WIDTH.minVal, heightRange.minVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE_OF_WIDTH.maxVal, heightRange.maxVal);
}

/**
 * @tc.name: CodecList_GetVideoWidthRange_001
 * @tc.desc: CodecList GetVideoWidthRange
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetVideoWidthRange_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    Range widthRange = capability_->GetVideoWidthRange();
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.minVal, widthRange.minVal);
    EXPECT_EQ(DEFAULT_WIDTH_RANGE.maxVal, widthRange.maxVal);
}

/**
 * @tc.name: CodecList_GetVideoHeightRange_001
 * @tc.desc: CodecList GetVideoHeightRange
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetVideoHeightRange_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    Range heightRange = capability_->GetVideoHeightRange();
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.minVal, heightRange.minVal);
    EXPECT_EQ(DEFAULT_HEIGHT_RANGE.maxVal, heightRange.maxVal);
}

/**
 * @tc.name: CodecList_IsVideoSizeSupported_001
 * @tc.desc: CodecList IsVideoSizeSupported
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_IsVideoSizeSupported_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    int32_t wmin = DEFAULT_WIDTH_RANGE.minVal;
    int32_t wmax = DEFAULT_WIDTH_RANGE.maxVal;
    int32_t hmin = DEFAULT_HEIGHT_RANGE.minVal;
    int32_t hmax = DEFAULT_HEIGHT_RANGE.maxVal;
    // case 1 - 13, postive param
    EXPECT_TRUE(capability_->IsVideoSizeSupported(DEFAULT_WIDTH, DEFAULT_HEIGHT))
        << "width:" << DEFAULT_WIDTH << "height:" << DEFAULT_HEIGHT << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(wmin, DEFAULT_HEIGHT))
        << "width:" << wmin << "height:" << DEFAULT_HEIGHT << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(wmin + 2, DEFAULT_HEIGHT))
        << "width:" << wmin + 2 << "height:" << DEFAULT_HEIGHT << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(wmin + 2, hmax))
        << "width:" << wmin + 2 << "height:" << hmax << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(wmin + 2, hmax - 2))
        << "width:" << wmin + 2 << "height:" << hmax - 2 << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(DEFAULT_WIDTH, hmax))
        << "width:" << DEFAULT_WIDTH << "height:" << hmax << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(DEFAULT_WIDTH, hmax - 2))
        << "width:" << DEFAULT_WIDTH << "height:" << hmax - 2 << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(wmax, DEFAULT_HEIGHT))
        << "width:" << wmax << "height:" << DEFAULT_HEIGHT << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(wmax - 2, DEFAULT_HEIGHT))
        << "width:" << wmax - 2 << "height:" << DEFAULT_HEIGHT << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(wmax - 2, hmin))
        << "width:" << wmax - 2 << "height:" << DEFAULT_HEIGHT << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(wmax - 2, hmin + 2))
        << "width:" << wmax - 2 << "height:" << hmin + 2 << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(DEFAULT_WIDTH, hmin))
        << "width:" << DEFAULT_WIDTH << "height:" << hmin << std::endl;
    EXPECT_TRUE(capability_->IsVideoSizeSupported(DEFAULT_WIDTH, hmin + 2))
        << "width:" << DEFAULT_WIDTH << "height:" << hmin + 2 << std::endl;
}

/**
 * @tc.name: CodecList_IsVideoSizeSupported_002
 * @tc.desc: CodecList IsVideoSizeSupported
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_IsVideoSizeSupported_002, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    int32_t wmin = DEFAULT_WIDTH_RANGE.minVal;
    int32_t wmax = DEFAULT_WIDTH_RANGE.maxVal;
    int32_t hmin = DEFAULT_HEIGHT_RANGE.minVal;
    int32_t hmax = DEFAULT_HEIGHT_RANGE.maxVal;
    // case 1 - 10, negative param
    EXPECT_FALSE(capability_->IsVideoSizeSupported(wmax + 2, hmin))
        << "width:" << wmin + 2 << "height:" << hmin << std::endl;
    EXPECT_FALSE(capability_->IsVideoSizeSupported(wmin - 2, hmin))
        << "width:" << wmin - 2 << "height:" << hmin << std::endl;
    EXPECT_FALSE(capability_->IsVideoSizeSupported(wmax + 2, hmax))
        << "width:" << wmin + 2 << "height:" << hmax << std::endl;
    EXPECT_FALSE(capability_->IsVideoSizeSupported(wmin - 2, hmax))
        << "width:" << wmin - 2 << "height:" << hmax << std::endl;

    EXPECT_FALSE(capability_->IsVideoSizeSupported(wmin, hmin - 2))
        << "width:" << wmin << "height:" << hmin - 2 << std::endl;
    EXPECT_FALSE(capability_->IsVideoSizeSupported(wmin, hmax + 2))
        << "width:" << wmin << "height:" << hmax + 2 << std::endl;
    EXPECT_FALSE(capability_->IsVideoSizeSupported(wmax, hmin - 2))
        << "width:" << wmax << "height:" << hmin - 2 << std::endl;
    EXPECT_FALSE(capability_->IsVideoSizeSupported(wmax, hmax + 2))
        << "width:" << wmax << "height:" << hmax + 2 << std::endl;

    EXPECT_FALSE(capability_->IsVideoSizeSupported(DEFAULT_WIDTH + 1, DEFAULT_HEIGHT))
        << "width:" << DEFAULT_WIDTH + 1 << "height:" << DEFAULT_HEIGHT << std::endl;
    EXPECT_FALSE(capability_->IsVideoSizeSupported(DEFAULT_WIDTH, DEFAULT_HEIGHT + 1))
        << "width:" << DEFAULT_WIDTH << "height:" << DEFAULT_HEIGHT + 1 << std::endl;
}

/**
 * @tc.name: CodecList_GetVideoFrameRateRange_001
 * @tc.desc: CodecList GetVideoFrameRateRange
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetVideoFrameRateRange_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    Range framerateRange = capability_->GetVideoFrameRateRange();
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.minVal, framerateRange.minVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.maxVal, framerateRange.maxVal);
}

/**
 * @tc.name: CodecList_AreVideoSizeAndFrameRateSupported_001
 * @tc.desc: CodecList AreVideoSizeAndFrameRateSupported
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_AreVideoSizeAndFrameRateSupported_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;
    int32_t minVal = DEFAULT_FRAMERATE_RANGE.minVal;
    int32_t maxVal = DEFAULT_FRAMERATE_RANGE.maxVal;
    // case 1 - 4, positive param
    EXPECT_TRUE(capability_->AreVideoSizeAndFrameRateSupported(DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FRAMERATE))
        << "width:" << DEFAULT_WIDTH << "height:" << DEFAULT_HEIGHT << "framerate: " << DEFAULT_FRAMERATE << std::endl;
    EXPECT_TRUE(capability_->AreVideoSizeAndFrameRateSupported(DEFAULT_WIDTH, DEFAULT_HEIGHT, minVal + 1))
        << "width:" << DEFAULT_WIDTH << "height:" << DEFAULT_HEIGHT << "framerate: " << minVal + 1 << std::endl;
    EXPECT_TRUE(capability_->AreVideoSizeAndFrameRateSupported(DEFAULT_WIDTH, DEFAULT_HEIGHT, maxVal))
        << "width:" << DEFAULT_WIDTH << "height:" << DEFAULT_HEIGHT << "framerate: " << maxVal << std::endl;
    EXPECT_TRUE(capability_->AreVideoSizeAndFrameRateSupported(DEFAULT_WIDTH, DEFAULT_HEIGHT, maxVal - 1))
        << "width:" << DEFAULT_WIDTH << "height:" << DEFAULT_HEIGHT << "framerate: " << maxVal - 1 << std::endl;
}

/**
 * @tc.name: CodecList_AreVideoSizeAndFrameRateSupported_002
 * @tc.desc: CodecList AreVideoSizeAndFrameRateSupported
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_AreVideoSizeAndFrameRateSupported_002, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;
    int32_t minVal = DEFAULT_FRAMERATE_RANGE.minVal;
    int32_t maxVal = DEFAULT_FRAMERATE_RANGE.maxVal;
    // case 1 - 9, negative param
    EXPECT_FALSE(capability_->AreVideoSizeAndFrameRateSupported(DEFAULT_WIDTH, DEFAULT_HEIGHT, minVal - 1))
        << "width:" << DEFAULT_WIDTH << "height:" << DEFAULT_HEIGHT << "framerate: " << minVal - 1 << std::endl;
    EXPECT_FALSE(capability_->AreVideoSizeAndFrameRateSupported(DEFAULT_WIDTH, DEFAULT_HEIGHT, maxVal + 1))
        << "width:" << DEFAULT_WIDTH << "height:" << DEFAULT_HEIGHT << "framerate: " << minVal + 1 << std::endl;
    EXPECT_FALSE(capability_->AreVideoSizeAndFrameRateSupported(DEFAULT_WIDTH, DEFAULT_HEIGHT, ERROR_FRAMERATE))
        << "width:" << DEFAULT_WIDTH << "height:" << DEFAULT_HEIGHT << "framerate: " << ERROR_FRAMERATE << std::endl;
    EXPECT_FALSE(capability_->AreVideoSizeAndFrameRateSupported(DEFAULT_WIDTH, ERROR_HEIGHT, DEFAULT_FRAMERATE))
        << "width:" << DEFAULT_WIDTH << "height:" << ERROR_HEIGHT << "framerate: " << DEFAULT_FRAMERATE << std::endl;
    EXPECT_FALSE(capability_->AreVideoSizeAndFrameRateSupported(DEFAULT_WIDTH, ERROR_HEIGHT, ERROR_FRAMERATE))
        << "width:" << DEFAULT_WIDTH << "height:" << ERROR_HEIGHT << "framerate: " << ERROR_FRAMERATE << std::endl;
    EXPECT_FALSE(capability_->AreVideoSizeAndFrameRateSupported(ERROR_WIDTH, DEFAULT_HEIGHT, DEFAULT_FRAMERATE))
        << "width:" << ERROR_WIDTH << "height:" << DEFAULT_HEIGHT << "framerate: " << DEFAULT_FRAMERATE << std::endl;
    EXPECT_FALSE(capability_->AreVideoSizeAndFrameRateSupported(ERROR_WIDTH, DEFAULT_HEIGHT, ERROR_FRAMERATE))
        << "width:" << ERROR_WIDTH << "height:" << DEFAULT_HEIGHT << "framerate: " << ERROR_FRAMERATE << std::endl;
    EXPECT_FALSE(capability_->AreVideoSizeAndFrameRateSupported(ERROR_WIDTH, ERROR_HEIGHT, DEFAULT_FRAMERATE))
        << "width:" << ERROR_WIDTH << "height:" << ERROR_HEIGHT << "framerate: " << DEFAULT_FRAMERATE << std::endl;
    EXPECT_FALSE(capability_->AreVideoSizeAndFrameRateSupported(ERROR_WIDTH, ERROR_HEIGHT, ERROR_FRAMERATE))
        << "width:" << ERROR_WIDTH << "height:" << ERROR_HEIGHT << "framerate: " << ERROR_FRAMERATE << std::endl;
}

/**
 * @tc.name: CodecList_GetVideoFrameRateRangeForSize_001
 * @tc.desc: CodecList GetVideoFrameRateRangeForSize
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetVideoFrameRateRangeForSize_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    int32_t wmax = DEFAULT_HEIGHT_RANGE_OF_WIDTH.maxVal;
    int32_t hmax = DEFAULT_WIDTH_RANGE_OF_HEIGHT.maxVal;
    Range framerateRange = capability_->GetVideoFrameRateRangeForSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.minVal, framerateRange.minVal);
    EXPECT_EQ(DEFAULT_FRAMERATE_RANGE.maxVal, framerateRange.maxVal);

    framerateRange = capability_->GetVideoFrameRateRangeForSize(wmax, hmax);
    EXPECT_EQ(0, framerateRange.minVal);
    EXPECT_EQ(26, framerateRange.maxVal); // 26: The return values after (2304, 4096) are passed in
}

/**
 * @tc.name: CodecList_GetVideoFrameRateRangeForSize_002
 * @tc.desc: CodecList GetVideoFrameRateRangeForSize
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetVideoFrameRateRangeForSize_002, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    int32_t wmax = DEFAULT_WIDTH_RANGE.maxVal;
    int32_t hmax = DEFAULT_HEIGHT_RANGE.maxVal;
    Range framerateRange = capability_->GetVideoFrameRateRangeForSize(0, 0);
    EXPECT_EQ(0, framerateRange.minVal);
    EXPECT_EQ(0, framerateRange.maxVal);

    framerateRange = capability_->GetVideoFrameRateRangeForSize(wmax + 2, hmax + 2);
    EXPECT_EQ(0, framerateRange.minVal);
    EXPECT_EQ(0, framerateRange.maxVal);

    framerateRange = capability_->GetVideoFrameRateRangeForSize(wmax - 1, hmax - 1);
    EXPECT_EQ(0, framerateRange.minVal);
    EXPECT_EQ(0, framerateRange.maxVal);

    framerateRange = capability_->GetVideoFrameRateRangeForSize(-2, -2);
    EXPECT_EQ(0, framerateRange.minVal);
    EXPECT_EQ(0, framerateRange.maxVal);
}

/**
 * @tc.name: CodecList_GetVideoSupportedPixelFormats_001
 * @tc.desc: CodecList GetVideoSupportedPixelFormats
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetVideoSupportedPixelFormats_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    std::vector<int32_t> pixFormats = capability_->GetVideoSupportedPixelFormats();
    EXPECT_EQ(DEFAULT_VIDEO_AVC_PIXFORMATS, pixFormats);
}

/**
 * @tc.name: CodecList_GetSupportedProfiles_001
 * @tc.desc: CodecList GetSupportedProfiles
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetSupportedProfiles_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    std::vector<int32_t> profiles = capability_->GetSupportedProfiles();
    EXPECT_EQ(DEFAULT_VIDEO_AVC_PROFILES, profiles);
}

/**
 * @tc.name: CodecList_GetSupportedLevelsForProfile_001
 * @tc.desc: CodecList GetSupportedLevelsForProfile
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_GetSupportedLevelsForProfile_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;

    std::vector<int32_t> levels = capability_->GetSupportedLevelsForProfile(DEFAULT_VIDEO_AVC_PROFILE);
    EXPECT_EQ(DEFAULT_VIDEO_AVC_LEVELS, levels);
}

/**
 * @tc.name: CodecList_AreProfileAndLevelSupported_001
 * @tc.desc: CodecList AreProfileAndLevelSupported
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_AreProfileAndLevelSupported_001, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;
    // case 1 - 16, postive param
    for (auto it : DEFAULT_VIDEO_AVC_LEVELS) {
        EXPECT_TRUE(capability_->AreProfileAndLevelSupported(DEFAULT_VIDEO_AVC_PROFILE, it));
    }
}

/**
 * @tc.name: CodecList_AreProfileAndLevelSupported_002
 * @tc.desc: CodecList AreProfileAndLevelSupported
 * @tc.type: FUNC
 */
HWTEST_F(CodecListUnitTest, CodecList_AreProfileAndLevelSupported_002, TestSize.Level1)
{
    // video decoder
    AVCodecCategory category = AVCodecCategory::AVCODEC_SOFTWARE;
    capability_ = CodecListMockFactory::GetCapabilityByCategory(DEFAULT_VIDEO_MIME, false, category);
    ASSERT_NE(nullptr, capability_) << "video decoder codeclist create fail!" << std::endl;
    // case 1, negative param
    EXPECT_FALSE(capability_->AreProfileAndLevelSupported(DEFAULT_VIDEO_AVC_PROFILE, ERROR_LEVEL));
    // case 2, negative param
    EXPECT_FALSE(capability_->AreProfileAndLevelSupported(ERROR_VIDEO_AVC_PROFILE, DEFAULT_LEVEL));
    // case 3, negative param
    EXPECT_FALSE(capability_->AreProfileAndLevelSupported(ERROR_VIDEO_AVC_PROFILE, ERROR_LEVEL));
}