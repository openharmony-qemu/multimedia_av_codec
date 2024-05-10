/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "format.h"
#include "native_avcodec_videoencoder.h"
#include "native_avcodec_videodecoder.h"
#include "native_avcapability.h"

using namespace testing::ext;
using namespace OHOS::Media;

namespace {
constexpr double DEFAULT_FRAME_RATE = 30.0;
constexpr uint32_t DEFAULT_QUALITY = 30;
constexpr uint32_t VALID_ROTATION_ANGLE[] = {0, 90, 180, 270};
constexpr uint32_t DIVISOR = 2;
std::vector<uint32_t> PIXEL_FORMATS = {
    AV_PIXEL_FORMAT_YUVI420,
    AV_PIXEL_FORMAT_NV12,
    AV_PIXEL_FORMAT_NV21,
    AV_PIXEL_FORMAT_RGBA
};
uint32_t DEFAULT_WIDTH = 1280;
uint32_t DEFAULT_HEIGHT = 720;
uint32_t DEFAULT_BITRATE = 10000000;
uint32_t ENCODER_PIXEL_FORMAT = AV_PIXEL_FORMAT_SURFACE_FORMAT;
uint32_t DECODER_PIXEL_FORMAT = AV_PIXEL_FORMAT_SURFACE_FORMAT;
OH_AVFormat *g_format;
OH_AVCodec *g_videoEnc;
OH_AVCodec *g_videoDec;

class AVCodecParamCheckerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void){};
    void SetUp(void);
    void TearDown(void);
};

void AVCodecParamCheckerTest::SetUpTestCase(void)
{
    OH_AVCapability *encoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    OH_AVRange range;
    if (OH_AVCapability_GetVideoWidthRange(encoderCapability, &range) == AV_ERR_OK) {
        std::cout << "width min = " << range.minVal << " width max = " << range.maxVal << std::endl;
        DEFAULT_WIDTH = (range.minVal + range.maxVal) / DIVISOR;
    }

    if (OH_AVCapability_GetVideoHeightRange(encoderCapability, &range) == AV_ERR_OK) {
        std::cout << "height min = " << range.minVal << " height max = " << range.maxVal << std::endl;
        DEFAULT_HEIGHT = (range.minVal + range.maxVal) / DIVISOR;
    }

    if (OH_AVCapability_GetEncoderBitrateRange(encoderCapability, &range) == AV_ERR_OK) {
        std::cout << "bitrate min = " << range.minVal << " bitrate max = " << range.maxVal << std::endl;
        DEFAULT_BITRATE = (range.minVal + range.maxVal) / DIVISOR;
    }

    const int32_t *pixFormats = nullptr;
    uint32_t pixFormatNum = 0;
    auto ret = OH_AVCapability_GetVideoSupportedPixelFormats(encoderCapability, &pixFormats, &pixFormatNum);
    if (ret == AV_ERR_OK) {
        ENCODER_PIXEL_FORMAT = pixFormatNum > 0 ? pixFormats[0] : ENCODER_PIXEL_FORMAT;
        std::cout << "encoder pixel format = " << ENCODER_PIXEL_FORMAT << std::endl;
    }

    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    pixFormats = nullptr;
    pixFormatNum = 0;
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(decoderCapability, &pixFormats, &pixFormatNum);
    if (ret == AV_ERR_OK) {
        DECODER_PIXEL_FORMAT = pixFormatNum > 0 ? pixFormats[0] : DECODER_PIXEL_FORMAT;
        std::cout << "decoder pixel format = " << DECODER_PIXEL_FORMAT << std::endl;
    }
}

void AVCodecParamCheckerTest::SetUp(void)
{
    g_videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, g_videoEnc);
    g_videoDec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, g_videoDec);
    g_format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, g_format);
}

void AVCodecParamCheckerTest::TearDown(void)
{
    OH_AVFormat_Destroy(g_format);
    OH_VideoEncoder_Destroy(g_videoEnc);
    OH_VideoDecoder_Destroy(g_videoDec);
}

void SetFormatBasicParam(bool isDecoder)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    if (!isDecoder) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT));
    }
}

/**
 * @tc.name: ENCODE_KEY_WIDTH_INVAILD_TEST_0101
 * @tc.desc: codec video configure not exsit width
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_WIDTH_INVAILD_TEST_0101, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_WIDTH_INVALID_TEST_0102
 * @tc.desc: codec video configure width out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_WIDTH_INVALID_TEST_0102, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, INT32_MIN));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_WIDTH_INVAILD_TEST_0103
 * @tc.desc: codec video configure width out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_WIDTH_INVAILD_TEST_0103, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, INT32_MAX));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_WIDTH_VAILD_TEST_0104
 * @tc.desc: codec video configure default width
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_WIDTH_VAILD_TEST_0104, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: ENCODE_KEY_HEIGHT_INVALID_TEST_0201
 * @tc.desc: codec video configure not exsit height
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_HEIGHT_INVALID_TEST_0201, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_HEIGHT_INVALID_TEST_0202
 * @tc.desc: codec video configure height out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_HEIGHT_INVALID_TEST_0202, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, INT32_MIN));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_HEIGHT_INVALID_TEST_0203
 * @tc.desc: codec video configure height out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_HEIGHT_INVALID_TEST_0203, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, INT32_MAX));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_HEIGHT_VALID_TEST_0204
 * @tc.desc: codec video configure default height
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_HEIGHT_VALID_TEST_0204, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: ENCODE_KEY_PIXEL_FORMAT_INVALID_TEST_0301
 * @tc.desc: codec video configure not exsit pixel format
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_PIXEL_FORMAT_INVALID_TEST_0301, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_PIXEL_FORMAT_INVALID_TEST_0302
 * @tc.desc: codec video configure pixel format unsupport
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_PIXEL_FORMAT_INVALID_TEST_0302, TestSize.Level1)
{
    OH_AVCapability *encoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    const int32_t *pixFormats = nullptr;
    uint32_t pixFormatNum = 0;
    auto ret = OH_AVCapability_GetVideoSupportedPixelFormats(encoderCapability, &pixFormats, &pixFormatNum);
    ASSERT_EQ(AV_ERR_OK, ret);

    std::vector<uint32_t> unsupportedPixelFormats = PIXEL_FORMATS;
    for (int32_t i = 0; i < pixFormatNum; i++) {
        int32_t pixelFormat = pixFormats[i];
        std::cout << "Capability Supported pixelFormat = " << pixelFormat << std::endl;
        auto iter = std::remove(unsupportedPixelFormats.begin(), unsupportedPixelFormats.end(), pixelFormat);
        unsupportedPixelFormats.erase(iter, unsupportedPixelFormats.end());
    }

    for (int32_t i = 0; i < unsupportedPixelFormats.size(); i++) {
        int32_t unsupportedPixelFormat = unsupportedPixelFormats[i];
        std::cout << "Unsupported Pixel Format = " << unsupportedPixelFormat << std::endl;

        OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
        ASSERT_NE(nullptr, videoEnc);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, unsupportedPixelFormat));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(videoEnc, format);
        ASSERT_EQ(ret, AV_ERR_UNSUPPORT);

        OH_AVFormat_Destroy(format);
        OH_VideoEncoder_Destroy(videoEnc);
    }
}

/**
 * @tc.name: ENCODE_KEY_PIXEL_FORMAT_VALID_TEST_0303
 * @tc.desc: codec video configure default pixel format
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_PIXEL_FORMAT_VALID_TEST_0303, TestSize.Level1)
{
    SetFormatBasicParam(false);
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: ENCODE_KEY_FRAME_RATE_VALID_TEST_0401
 * @tc.desc: codec video configure not exsit frame rate
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_FRAME_RATE_VALID_TEST_0401, TestSize.Level1)
{
    SetFormatBasicParam(false);
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: ENCODE_KEY_FRAME_RATE_INVALID_TEST_0402
 * @tc.desc: codec video configure frame rate out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_FRAME_RATE_INVALID_TEST_0402, TestSize.Level1)
{
    constexpr double frameRate = 0.0;
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetDoubleValue(g_format, OH_MD_KEY_FRAME_RATE, frameRate));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_FRAME_RATE_INVALID_TEST_0403
 * @tc.desc: codec video configure frame rate out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_FRAME_RATE_INVALID_TEST_0403, TestSize.Level1)
{
    constexpr double frameRate = -1.0;
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetDoubleValue(g_format, OH_MD_KEY_FRAME_RATE, frameRate));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_FRAME_RATE_INVALID_TEST_0404
 * @tc.desc: codec video configure default frame rate
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_FRAME_RATE_INVALID_TEST_0404, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetDoubleValue(g_format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0501
 * @tc.desc: codec video configure not exsit bitrate and bitrate mode is CBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0501, TestSize.Level1)
{
    SetFormatBasicParam(false);
    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    if (OH_AVCapability_IsEncoderBitrateModeSupported(capability, BITRATE_MODE_CBR)) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
        ASSERT_EQ(ret, AV_ERR_OK);
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0502
 * @tc.desc: codec video configure not exsit bitrate and bitrate mode is VBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0502, TestSize.Level1)
{
    SetFormatBasicParam(false);
    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    if (OH_AVCapability_IsEncoderBitrateModeSupported(capability, BITRATE_MODE_VBR)) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
        ASSERT_EQ(ret, AV_ERR_OK);
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0503
 * @tc.desc: codec video configure no exist bitrate mode and bitrate value in range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0503, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetLongValue(g_format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0504
 * @tc.desc: codec video configure bitrate in range and bitrate mode is VBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0504, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetLongValue(g_format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE));

    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    if (OH_AVCapability_IsEncoderBitrateModeSupported(capability, BITRATE_MODE_VBR)) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
        ASSERT_EQ(ret, AV_ERR_OK);
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0505
 * @tc.desc: codec video configure bitrate in range and bitrate mode is CBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0505, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetLongValue(g_format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE));

    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    if (OH_AVCapability_IsEncoderBitrateModeSupported(capability, BITRATE_MODE_CBR)) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
        ASSERT_EQ(ret, AV_ERR_OK);
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0506
 * @tc.desc: codec video configure bitrate in range and bitrate mode is CQ
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0506, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetLongValue(g_format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE));

    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    if (OH_AVCapability_IsEncoderBitrateModeSupported(capability, BITRATE_MODE_CQ)) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
        ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0507
 * @tc.desc: codec video configure bitrate invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0507, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetLongValue(g_format, OH_MD_KEY_BITRATE, INT64_MAX));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0508
 * @tc.desc: codec video configure bitrate invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0508, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetLongValue(g_format, OH_MD_KEY_BITRATE, INT64_MIN));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0509
 * @tc.desc: codec video configure bitrate and quality exist value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0509, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetLongValue(g_format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_QUALITY, DEFAULT_QUALITY));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0510
 * @tc.desc: codec video configure quality in range and bitrate mode not exist
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0510, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_QUALITY, DEFAULT_QUALITY));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0511
 * @tc.desc: codec video configure quality in range and bitrate mode is VBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0511, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_QUALITY, DEFAULT_QUALITY));

    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    if (OH_AVCapability_IsEncoderBitrateModeSupported(capability, BITRATE_MODE_VBR)) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
        ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0512
 * @tc.desc: codec video configure quality in range and bitrate mode is CBR
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0512, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_QUALITY, DEFAULT_QUALITY));

    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    if (OH_AVCapability_IsEncoderBitrateModeSupported(capability, BITRATE_MODE_CBR)) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
        ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0513
 * @tc.desc: codec video configure quality in range and bitrate mode is CQ
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_VALID_TEST_0513, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_QUALITY, DEFAULT_QUALITY));

    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    if (OH_AVCapability_IsEncoderBitrateModeSupported(capability, BITRATE_MODE_CQ)) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
        ASSERT_EQ(ret, AV_ERR_OK);
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0514
 * @tc.desc: codec video configure quality out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0514, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_QUALITY, INT32_MIN));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0515
 * @tc.desc: codec video configure quality out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0515, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_QUALITY, INT32_MAX));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0516
 * @tc.desc: codec video configure quality not exist and bitrate mode is CQ
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0516, TestSize.Level1)
{
    SetFormatBasicParam(false);
    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    if (OH_AVCapability_IsEncoderBitrateModeSupported(capability, BITRATE_MODE_CQ)) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
        ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0517
 * @tc.desc: codec video configure bitrate mode invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0517, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, INT32_MIN));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_UNSUPPORT);
}

/**
 * @tc.name: ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0518
 * @tc.desc: codec video configure bitrate mode invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_BITRATE_QUALLITY_INVALID_TEST_0518, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, INT32_MAX));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_UNSUPPORT);
}

/**
 * @tc.name: ENCODE_KEY_PROFILE_VALID_TEST_0601
 * @tc.desc: codec video configure profile support
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_PROFILE_VALID_TEST_0601, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    const int32_t *profiles = nullptr;
    uint32_t profilesNum = 0;
    int profilesRet = OH_AVCapability_GetSupportedProfiles(capability, &profiles, &profilesNum);
    ASSERT_EQ(AV_ERR_OK, profilesRet);

    for (int32_t i = 0; i < profilesNum; i++) {
        std::cout << "Capability Supported Profile = " << profiles[i] << std::endl;

        OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
        ASSERT_NE(nullptr, videoEnc);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, ENCODER_PIXEL_FORMAT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, profiles[i]));
        OH_AVErrCode ret = OH_VideoEncoder_Configure(videoEnc, format);
        ASSERT_EQ(ret, AV_ERR_OK);

        OH_AVFormat_Destroy(format);
        OH_VideoEncoder_Destroy(videoEnc);
    }
}

/**
 * @tc.name: ENCODE_KEY_PROFILE_INVALID_TEST_0602
 * @tc.desc: codec video configure profile unsupport
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_PROFILE_INVALID_TEST_0602, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PROFILE, INT32_MIN));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_UNSUPPORT);
}

/**
 * @tc.name: ENCODE_KEY_PROFILE_INVALID_TEST_0602
 * @tc.desc: codec video configure profile unsupport
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, ENCODE_KEY_PROFILE_INVALID_TEST_0603, TestSize.Level1)
{
    SetFormatBasicParam(false);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PROFILE, INT32_MAX));
    OH_AVErrCode ret = OH_VideoEncoder_Configure(g_videoEnc, g_format);
    ASSERT_EQ(ret, AV_ERR_UNSUPPORT);
}

/**
 * @tc.name: DECODE_KEY_WIDTH_INVALID_TEST_1101
 * @tc.desc: codec video configure not exsit width
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_WIDTH_INVALID_TEST_1101, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: DECODE_KEY_WIDTH_INVALID_TEST_1102
 * @tc.desc: codec video configure width out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_WIDTH_INVALID_TEST_1102, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, INT32_MIN));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: DECODE_KEY_WIDTH_INVALID_TEST_1103
 * @tc.desc: codec video configure width out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_WIDTH_INVALID_TEST_1103, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, INT32_MAX));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: DECODE_KEY_WIDTH_VALID_TEST_1104
 * @tc.desc: codec video configure default width
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_WIDTH_VALID_TEST_1104, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: DECODE_KEY_HEIGHT_INVALID_TEST_1201
 * @tc.desc: codec video configure not exsit height
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_HEIGHT_INVALID_TEST_1201, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: DECODE_KEY_HEIGHT_INVALID_TEST_1202
 * @tc.desc: codec video configure height out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_HEIGHT_INVALID_TEST_1202, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, INT32_MIN));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: DECODE_KEY_HEIGHT_INVALID_TEST_1203
 * @tc.desc: codec video configure height out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_HEIGHT_INVALID_TEST_1203, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, INT32_MAX));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: DECODE_KEY_HEIGHT_VALID_TEST_1204
 * @tc.desc: codec video configure default height
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_HEIGHT_VALID_TEST_1204, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: DECODE_KEY_PIXEL_FORMAT_VALID_TEST_1301
 * @tc.desc: codec video configure not exsit pixel format
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_PIXEL_FORMAT_VALID_TEST_1301, TestSize.Level1)
{
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: DECODE_KEY_PIXEL_FORMAT_INVALID_TEST_1302
 * @tc.desc: codec video configure pixel format unsupport
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_PIXEL_FORMAT_VALID_TEST_1302, TestSize.Level1)
{
    OH_AVCapability *encoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    const int32_t *pixFormats = nullptr;
    uint32_t pixFormatNum = 0;
    auto ret = OH_AVCapability_GetVideoSupportedPixelFormats(encoderCapability, &pixFormats, &pixFormatNum);
    ASSERT_EQ(AV_ERR_OK, ret);

    std::vector<uint32_t> unsupportedPixelFormats = PIXEL_FORMATS;
    for (int32_t i = 0; i < pixFormatNum; i++) {
        int32_t pixelFormat = pixFormats[i];
        std::cout << "Capability Supported pixelFormat = " << pixelFormat << std::endl;
        auto iter = std::remove(unsupportedPixelFormats.begin(), unsupportedPixelFormats.end(), pixelFormat);
        unsupportedPixelFormats.erase(iter, unsupportedPixelFormats.end());
    }

    for (int32_t i = 0; i < unsupportedPixelFormats.size(); i++) {
        int32_t unsupportedPixelFormat = unsupportedPixelFormats[i];
        std::cout << "Unsupported Pixel Format = " << unsupportedPixelFormat << std::endl;

        OH_AVCodec *videoDec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
        ASSERT_NE(nullptr, videoDec);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, unsupportedPixelFormat));
        OH_AVErrCode ret = OH_VideoDecoder_Configure(videoDec, format);
        ASSERT_EQ(ret, AV_ERR_UNSUPPORT);

        OH_AVFormat_Destroy(format);
        OH_VideoDecoder_Destroy(videoDec);
    }
}

/**
 * @tc.name: DECODE_KEY_PIXEL_FORMAT_VALID_TEST_1303
 * @tc.desc: codec video configure default pixel format
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_PIXEL_FORMAT_VALID_TEST_1303, TestSize.Level1)
{
    SetFormatBasicParam(true);
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: DECODE_KEY_FRAME_RATE_VALID_TEST_1401
 * @tc.desc: codec video configure not exsit frame rate
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_FRAME_RATE_VALID_TEST_1401, TestSize.Level1)
{
    SetFormatBasicParam(true);
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: DECODE_KEY_FRAME_RATE_INVALID_TEST_1402
 * @tc.desc: codec video configure frame rate out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_FRAME_RATE_INVALID_TEST_1402, TestSize.Level1)
{
    constexpr double frameRate = 0.0;
    SetFormatBasicParam(true);
    ASSERT_EQ(true, OH_AVFormat_SetDoubleValue(g_format, OH_MD_KEY_FRAME_RATE, frameRate));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: DECODE_KEY_FRAME_RATE_INVALID_TEST_1403
 * @tc.desc: codec video configure frame rate out of range
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_FRAME_RATE_INVALID_TEST_1403, TestSize.Level1)
{
    constexpr double frameRate = -1.0;
    SetFormatBasicParam(true);
    ASSERT_EQ(true, OH_AVFormat_SetDoubleValue(g_format, OH_MD_KEY_FRAME_RATE, frameRate));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: DECODE_KEY_FRAME_RATE_VALID_TEST_1404
 * @tc.desc: codec video configure default frame rate
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_FRAME_RATE_VALID_TEST_1404, TestSize.Level1)
{
    SetFormatBasicParam(true);
    ASSERT_EQ(true, OH_AVFormat_SetDoubleValue(g_format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE));
    OH_AVErrCode ret = OH_VideoDecoder_Configure(g_videoDec, g_format);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: DECODE_KEY_ROTATION_VALID_TEST_1501
 * @tc.desc: codec video configure rotation angle support
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_ROTATION_VALID_TEST_1501, TestSize.Level1)
{
    int32_t len = sizeof(VALID_ROTATION_ANGLE) / sizeof(VALID_ROTATION_ANGLE[0]);
    for (int32_t i = 0; i < len; i++) {
        uint32_t rotationAngle = VALID_ROTATION_ANGLE[i];
        std::cout << "Valid Rotation Angle = " << rotationAngle << std::endl;

        OH_AVCodec *videoDec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
        ASSERT_NE(nullptr, videoDec);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, rotationAngle));
        OH_AVErrCode ret = OH_VideoDecoder_Configure(videoDec, format);
        ASSERT_EQ(ret, AV_ERR_OK);

        OH_AVFormat_Destroy(format);
        OH_VideoEncoder_Destroy(videoDec);
    }
}

/**
 * @tc.name: DECODE_KEY_ROTATION_INVALID_TEST_1502
 * @tc.desc: codec video configure rotation angle support
 * @tc.type: FUNC
 */
HWTEST_F(AVCodecParamCheckerTest, DECODE_KEY_ROTATION_INVALID_TEST_1502, TestSize.Level1)
{
    int32_t minRotationAngle = 0;
    int32_t maxRotationAngle = 360;
    int32_t len = sizeof(VALID_ROTATION_ANGLE) / sizeof(VALID_ROTATION_ANGLE[0]);
    for (int32_t i = minRotationAngle; i < maxRotationAngle; i++) {
        if (std::find(VALID_ROTATION_ANGLE, VALID_ROTATION_ANGLE + len, i) == VALID_ROTATION_ANGLE + len) {
            uint32_t rotationAngle = i;
            std::cout << "Invalid Rotation Angle =" << rotationAngle << std::endl;

            OH_AVCodec *videoDec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
            ASSERT_NE(nullptr, videoDec);
            OH_AVFormat *format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
            ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
            ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT));
            ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, rotationAngle));
            OH_AVErrCode ret = OH_VideoDecoder_Configure(videoDec, format);
            ASSERT_EQ(ret, AV_ERR_UNSUPPORT);

            OH_AVFormat_Destroy(format);
            OH_VideoEncoder_Destroy(videoDec);
        }
    }
}
}