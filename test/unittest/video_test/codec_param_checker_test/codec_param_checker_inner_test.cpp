/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "codec_param_checker_test.h"
#include <gtest/gtest.h>
#include "avcodec_info.h"
#include "avcodec_list.h"
#include "avcodec_video_decoder.h"
#include "avcodec_video_encoder.h"
#include "format.h"
#include "media_description.h"
#include "meta/meta_key.h"
#include "avcodec_errors.h"
#include "av_common.h"
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
namespace {
uint32_t DEFAULT_QUALITY = 30; // 30 默认值
uint32_t DEFAULT_BITRATE = 10000000; // 10000000 默认值
uint32_t DEFAULT_MAX_BITRATE = 20000000; // 20000000 默认值
uint32_t DEFAULT_SQR_FACTOR = 30; // 30 默认值

void SetFormatBasicParam(Format &format)
{
    format = Format();
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1280); // 1280 w默认值
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 720); // 720 h默认值
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::SURFACE_FORMAT));
}

bool IsEncoderBitrateModeSupported(CapabilityData *capData, VideoEncodeBitrateMode bitrateMode)
{
    if (!AVCodecInfo::isEncoder(capData->codecType)) {
        return false;
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    const auto &bitrateModeVec = codecInfo->GetSupportedBitrateMode();
    return find(bitrateModeVec.begin(), bitrateModeVec.end(), bitrateMode) != bitrateModeVec.end();
}

/**
 * @tc.name: CONFIGURE_CODEC_DMA_TAG_VALID_TEST_0101
 * @tc.desc: codec dma tag ascii visible accepted after non-ascii/whitespace filtering through inner encoder configure
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, CONFIGURE_CODEC_DMA_TAG_VALID_TEST_0101, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutStringValue(OHOS::Media::Tag::VIDEO_SET_DMABUF_NAME_SUFFIX, "a b\tc\n!");
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: CONFIGURE_CODEC_DMA_TAG_INVALID_TEST_0102
 * @tc.desc: codec dma tag rejected if empty after non-ascii/whitespace filtering through inner encoder configure
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, CONFIGURE_CODEC_DMA_TAG_INVALID_TEST_0102, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutStringValue(OHOS::Media::Tag::VIDEO_SET_DMABUF_NAME_SUFFIX, " \t\n\r");
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: CONFIGURE_CODEC_DMA_TAG_INVALID_TEST_0103
 * @tc.desc: codec dma tag reject pure digits after non-ascii/whitespace filtering through inner encoder configure
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, CONFIGURE_CODEC_DMA_TAG_INVALID_TEST_0103, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutStringValue(OHOS::Media::Tag::VIDEO_SET_DMABUF_NAME_SUFFIX, "1 2\t3");
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: CONFIGURE_CODEC_DMA_TAG_INVALID_TEST_0104
 * @tc.desc: codec dma tag should reject pure digits (no non-digit char) through inner encoder configure
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, CONFIGURE_CODEC_DMA_TAG_INVALID_TEST_0104, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutStringValue(OHOS::Media::Tag::VIDEO_SET_DMABUF_NAME_SUFFIX, "123");
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: CONFIGURE_CODEC_DMA_TAG_VALID_TEST_0105
 * @tc.desc: codec dma tag with ascii visible characters should be accepted through inner decoder configure
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, CONFIGURE_CODEC_DMA_TAG_VALID_TEST_0105, TestSize.Level3)
{
    auto videoDecInner = VideoDecoderFactory::CreateByMime("video/avc");
    ASSERT_NE(nullptr, videoDecInner);

    formatInner_ = Format();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1280); // 1280 w默认值
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 720); // 720 h默认值
    formatInner_.PutStringValue(OHOS::Media::Tag::VIDEO_SET_DMABUF_NAME_SUFFIX, "d e\tc");
    ASSERT_EQ(AVCS_ERR_OK, videoDecInner->Configure(formatInner_));
}

/**
 * @tc.name: CONFIGURE_CODEC_DMA_TAG_VALID_TEST_0106
 * @tc.desc: codec dma tag non-ascii filtered, remaining non-digit accepted through inner encoder configure
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, CONFIGURE_CODEC_DMA_TAG_VALID_TEST_0106, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutStringValue(OHOS::Media::Tag::VIDEO_SET_DMABUF_NAME_SUFFIX, "中文a");
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: CONFIGURE_CODEC_DMA_TAG_INVALID_TEST_0107
 * @tc.desc: codec dma tag non-ascii filtered to pure digits, rejected through inner encoder configure
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, CONFIGURE_CODEC_DMA_TAG_INVALID_TEST_0107, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutStringValue(OHOS::Media::Tag::VIDEO_SET_DMABUF_NAME_SUFFIX, "中文123");
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: CONFIGURE_CODEC_DMA_TAG_INVALID_TEST_0108
 * @tc.desc: codec dma tag only non-ascii filters to empty, rejected through inner encoder configure
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, CONFIGURE_CODEC_DMA_TAG_INVALID_TEST_0108, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutStringValue(OHOS::Media::Tag::VIDEO_SET_DMABUF_NAME_SUFFIX, "中文");
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1519
 * @tc.desc: codec video configure sqr_factor and quality exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1519, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
  * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1520
  * @tc.desc: codec video configure max_bitrate and quality exist value
  * @tc.type: FUNC
  */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1520, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1521
 * @tc.desc: codec video configure bitrate and max_bitrate exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1521, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1522
 * @tc.desc: codec video configure bitrate and sqr_factor exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1522, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1523
 * @tc.desc: codec video configure bitrate and sqr_factor and max_bitrate exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1523, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1524
 * @tc.desc: codec video configure bitrate, sqr_factor, max_bitrate and quality exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1524, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1525
 * @tc.desc: codec video configure sqr_factor and quality exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1525, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
  * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1526
  * @tc.desc: codec video configure max_bitrate and quality exist value
  * @tc.type: FUNC
  */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1526, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1527
 * @tc.desc: codec video configure bitrate and max_bitrate exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1527, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1528
 * @tc.desc: codec video configure bitrate and sqr_factor exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1528, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1529
 * @tc.desc: codec video configure bitrate and sqr_factor and max_bitrate exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1529, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1530
 * @tc.desc: codec video configure bitrate, sqr_factor, max_bitrate and quality exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1530, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1531
 * @tc.desc: codec video configure max_bitrate invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1531, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.maxVal + 1);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}
 
/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1532
 * @tc.desc: codec video configure max_bitrate invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1532, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.minVal - 1);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1533
 * @tc.desc: codec video configure sqr_factor out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1533, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.minVal - 1);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1534
 * @tc.desc: codec video configure sqr_factor out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1534, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1535
 * @tc.desc: codec video configure sqr_factor and quality exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1535, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.minVal - 1);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1536
 * @tc.desc: codec video configure sqr_factor and quality exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1536, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1537
 * @tc.desc: codec video configure max_bitrate invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1537, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.minVal - 1);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1538
 * @tc.desc: codec video configure max_bitrate invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1538, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.maxVal + 1);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1539
 * @tc.desc: codec video configure sqr_factor and quality exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1539, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.minVal - 1);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1540
 * @tc.desc: codec video configure sqr_factor and quality exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1540, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1541
 * @tc.desc: codec video configure max_bitrate invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1541, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.minVal - 1);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1542
 * @tc.desc: codec video configure max_bitrate invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1542, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.maxVal + 1);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1543
 * @tc.desc: codec video configure max_bitrate and sqr_factor invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1543, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.maxVal + 1);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1544
 * @tc.desc: codec video configure max_bitrate and sqr_factor invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1544, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.minVal - 1);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1545
 * @tc.desc: codec video configure quality invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1545, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, INT64_MAX);
    ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1546
 * @tc.desc: codec video configure quality invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1546, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, INT32_MAX);
    ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1547
 * @tc.desc: codec video configure max_bitrate in range and bitrate mode is CQ
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1547, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.minVal - 1);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CQ)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1548
 * @tc.desc: codec video configure sqr_factor in range and bitrate mode is VBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1548, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);

    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, VBR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1549
 * @tc.desc: codec video configure sqr_factor in range and bitrate mode is CBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1549, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);

    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1550
 * @tc.desc: codec video configure sqr_factor in range and bitrate mode is CQ
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1550, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);

    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CQ)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1551
 * @tc.desc: codec video configure sqr_factor in range and bitrate mode is CBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1551, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.minVal - 1);

    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1552
 * @tc.desc: codec video configure sqr_factor in range and bitrate mode is CBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1552, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.minVal - 1);

    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, VBR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1553
 * @tc.desc: SQR is supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1553, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
* @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1554
* @tc.desc: codec video configure quality in range and bitrate mode is SQR
* @tc.type: FUNC
*/
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1554, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
* @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1555
* @tc.desc: codec video configure quality which is not in range and bitrate mode is SQR
* @tc.type: FUNC
*/
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1555, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, INT32_MAX);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1556
 * @tc.desc: codec video configure sqr_factor in range and bitrate mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1556, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1557
 * @tc.desc: codec video configure bitrate exist value and mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1557, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}
 
/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1558
 * @tc.desc: codec video configure max_bitrate exist value and mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1558, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}
 
/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1559
 * @tc.desc: codec video configure bitrate and max_bitrate exist value and mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1559, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1560
 * @tc.desc: codec video configure bitrate、max_bitrate and sqr_factor exist value and mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1560, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1561
 * @tc.desc: codec video configure sqr_factor in range, max_bitrate and bitrate not in range, bitrate mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1561, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, INT64_MAX);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, INT32_MAX);
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
        const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.minVal - 1);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1562
 * @tc.desc: codec video configure sqr_factor not in range and bitrate mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1562, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1563
 * @tc.desc: codec video configure sqr_factor not in range and bitrate mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1563, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
        const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1564
 * @tc.desc: codec video configure sqr_factor not in range and bitrate mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1564, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
        const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}
 
/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1565
 * @tc.desc: bitrate mode SQR is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1565, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}
 
/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1566
 * @tc.desc: itrate mode SQR is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1566, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1567
 * @tc.desc: itrate mode SQR is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1567, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1568
 * @tc.desc: itrate mode SQR is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1568, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, INT32_MAX);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}
 
/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1569
 * @tc.desc: itrate mode SQR is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1569, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, INT64_MAX);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1570
 * @tc.desc: itrate mode SQR is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1570, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1571
 * @tc.desc: itrate mode SQR is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1571, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
* @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1572
* @tc.desc: itrate mode SQR is not supported
* @tc.type: FUNC
*/
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1572, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, INT64_MAX);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1573
 * @tc.desc: itrate mode SQR is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1573, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, INT32_MAX);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, INT64_MAX);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1574
 * @tc.desc: codec video configure sqr_factor not in range and bitrate mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1574, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
        const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1575
 * @tc.desc: codec video configure sqr_factor and bitrate mode is CBR_HIGH_QUALITY
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1575, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1576
 * @tc.desc: codec video configure quality and bitrate mode is CBR_HIGH_QUALITY
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1576, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1577
 * @tc.desc: codec video configure maxBitrate and bitrate mode is CBR_HIGH_QUALITY
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1577, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_BITRATE);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1578
 * @tc.desc: codec video configure bitrate and bitrate mode is CBR_HIGH_QUALITY
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1578, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1579
 * @tc.desc: codec video configure bitrate not in range and bitrate mode is CBR_HIGH_QUALITY
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1579, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, INT64_MAX);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1580
 * @tc.desc: codec video configure bitrate mode is CBR_HIGH_QUALITY
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1580, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1581
 * @tc.desc: bitrate mode CBR_HIGH_QUALITY is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1581, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1582
 * @tc.desc: bitrate mode CBR_HIGH_QUALITY is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1582, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1583
 * @tc.desc: bitrate mode CBR_HIGH_QUALITY is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1583, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1584
 * @tc.desc: bitrate mode CBR_HIGH_QUALITY is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1584, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1585
 * @tc.desc: bitrate mode CBR_HIGH_QUALITY is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1585, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        ASSERT_EQ(AV_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1586
 * @tc.desc: bitrate mode CBR_HIGH_QUALITY is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1586, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR_HIGH_QUALITY)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR_HIGH_QUALITY);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, INT64_MAX);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: VIDEO_CODEC_SCENARIO_TEST_001
 * @tc.desc: codec video scenario checkout
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, VIDEO_CODEC_SCENARIO_TEST_001, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(OHOS::Media::Tag::VIDEO_CODEC_SCENARIO, SCENARIO_RECORDING - 1);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: VIDEO_CODEC_SCENARIO_TEST_002
 * @tc.desc: codec video scenario checkout
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, VIDEO_CODEC_SCENARIO_TEST_002, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(OHOS::Media::Tag::VIDEO_CODEC_SCENARIO, SCENARIO_RECORDING);
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: VIDEO_CODEC_SCENARIO_TEST_003
 * @tc.desc: codec video scenario checkout
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, VIDEO_CODEC_SCENARIO_TEST_003, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(OHOS::Media::Tag::VIDEO_CODEC_SCENARIO, SCENARIO_INVALID);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEncHevcInner_->Configure(formatInner_));
}
}
