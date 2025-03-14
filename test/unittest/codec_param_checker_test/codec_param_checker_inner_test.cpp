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
#include "avcodec_video_encoder.h"
#include "format.h"
#include "media_description.h"
#include "avcodec_errors.h"
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

extern uint32_t DEFAULT_QUALITY;
extern uint32_t DEFAULT_WIDTH;
extern uint32_t DEFAULT_HEIGHT;
extern uint32_t ENCODER_PIXEL_FORMAT;
extern uint32_t DEFAULT_MAX_BITRATE;
extern uint32_t DEFAULT_SQR_FACTOR;
extern uint32_t DEFAULT_BITRATE;

void SetFormatBasicParam(Format &format)
{
    format = Format();
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT);
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

namespace {
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
    ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
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
    ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
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
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    }
    ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1522
 * @tc.desc: codec video configure max_bitrate in range and bitrate mode is CQ
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1522, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CQ)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1523
 * @tc.desc: codec video configure quality in range and bitrate mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1523, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, DEFAULT_QUALITY);

    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1524
 * @tc.desc: codec video configure sqr_factor in range and bitrate mode is VBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1524, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);

    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, VBR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1525
 * @tc.desc: codec video configure sqr_factor in range and bitrate mode is CBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1525, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);

    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CBR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1526
 * @tc.desc: codec video configure sqr_factor in range and bitrate mode is CQ
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1526, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);

    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, CQ)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1527
 * @tc.desc: bitrate mode SQR is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1527, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1528
 * @tc.desc: codec video configure sqr_factor in range but SQR is not supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1528, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
    if (!IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1529
 * @tc.desc: codec video configure max_bitrate invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1529, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.maxVal + 1);
    ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1530
 * @tc.desc: codec video configure max_bitrate invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1530, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &maxBitrateRange = codecInfo->GetSupportedMaxBitrate();
    formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, maxBitrateRange.minVal - 1);
    ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1531
 * @tc.desc: codec video configure sqr_factor out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1531, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.minVal - 1);
    ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1532
 * @tc.desc: codec video configure sqr_factor out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1532, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityDataHevc_);
    const auto &sqrFactorRange = codecInfo->GetSupportedSqrFactor();
    formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactorRange.maxVal + 1);
    ASSERT_EQ(AVCS_ERR_CODEC_PARAM_INCORRECT, videoEncHevcInner_->Configure(formatInner_));
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1533
 * @tc.desc: SQR is supported
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1533, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1534
 * @tc.desc: codec video configure bitrate exist value and mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1534, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1535
 * @tc.desc: codec video configure max_bitrate exist value and mode is SQR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1535, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
* @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1536
* @tc.desc: codec video configure bitrate and max_bitrate exist value and mode is SQR
* @tc.type: FUNC
*/
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1536, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
* @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1537
* @tc.desc: codec video configure bitrate、max_bitrate and sqr_factor exist value and mode is SQR
* @tc.type: FUNC
*/
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1537, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
* @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1539
* @tc.desc: codec video configure bitrate、max_bitrate and sqr_factor exist value and SQR is support
* @tc.type: FUNC
*/
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1539, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
* @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1540
* @tc.desc: codec video configure bitrate and sqr_factor exist value and SQR is support
* @tc.type: FUNC
*/
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1540, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_BITRATE);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
* @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1541
* @tc.desc: codec video configure max_bitrate and sqr_factor exist value and SQR is support
* @tc.type: FUNC
*/
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1541, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, DEFAULT_MAX_BITRATE);
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}

/**
* @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1542
* @tc.desc: codec video configure sqr_factor exist value and SQR is support
* @tc.type: FUNC
*/
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_1542, TestSize.Level3)
{
    SetFormatBasicParam(formatInner_);
    if (IsEncoderBitrateModeSupported(capabilityDataHevc_, SQR)) {
        formatInner_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, DEFAULT_SQR_FACTOR);
        ASSERT_EQ(AVCS_ERR_OK, videoEncHevcInner_->Configure(formatInner_));
    }
}
}