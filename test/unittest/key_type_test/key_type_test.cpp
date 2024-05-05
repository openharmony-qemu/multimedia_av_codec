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
#define TEST_SUIT AVCodecKeyTypeTest

using namespace testing::ext;
using namespace OHOS::Media;

namespace {
const std::map <const char *, int32_t> Key_Type_Map = {
    {"DEFAULT_KEY", FORMAT_TYPE_NONE},
    {OH_MD_KEY_TRACK_TYPE, FORMAT_TYPE_INT32},
    {OH_MD_KEY_CODEC_MIME, FORMAT_TYPE_STRING},
    {OH_MD_KEY_DURATION, FORMAT_TYPE_INT64},
    {OH_MD_KEY_BITRATE, FORMAT_TYPE_INT64},
    {OH_MD_KEY_MAX_INPUT_SIZE, FORMAT_TYPE_INT32},
    {OH_MD_KEY_WIDTH, FORMAT_TYPE_INT32},
    {OH_MD_KEY_HEIGHT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_PIXEL_FORMAT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_AUDIO_SAMPLE_FORMAT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_FRAME_RATE, FORMAT_TYPE_DOUBLE},
    {OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, FORMAT_TYPE_INT32},
    {OH_MD_KEY_PROFILE, FORMAT_TYPE_INT32},
    {OH_MD_KEY_AUD_CHANNEL_COUNT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_AUD_SAMPLE_RATE, FORMAT_TYPE_INT32},
    {OH_MD_KEY_I_FRAME_INTERVAL, FORMAT_TYPE_INT32},
    {OH_MD_KEY_ROTATION, FORMAT_TYPE_INT32},
    {OH_MD_KEY_RANGE_FLAG, FORMAT_TYPE_INT32},
    {OH_MD_KEY_COLOR_PRIMARIES, FORMAT_TYPE_INT32},
    {OH_MD_KEY_TRANSFER_CHARACTERISTICS, FORMAT_TYPE_INT32},
    {OH_MD_KEY_MATRIX_COEFFICIENTS, FORMAT_TYPE_INT32},
    {OH_MD_KEY_REQUEST_I_FRAME, FORMAT_TYPE_INT32},
    {OH_MD_KEY_QUALITY, FORMAT_TYPE_INT32},
    {OH_MD_KEY_CODEC_CONFIG, FORMAT_TYPE_ADDR},
    {OH_MD_KEY_TITLE, FORMAT_TYPE_STRING},
    {OH_MD_KEY_ARTIST, FORMAT_TYPE_STRING},
    {OH_MD_KEY_ALBUM, FORMAT_TYPE_STRING},
    {OH_MD_KEY_ALBUM_ARTIST, FORMAT_TYPE_STRING},
    {OH_MD_KEY_DATE, FORMAT_TYPE_STRING},
    {OH_MD_KEY_COMMENT, FORMAT_TYPE_STRING},
    {OH_MD_KEY_GENRE, FORMAT_TYPE_STRING},
    {OH_MD_KEY_COPYRIGHT, FORMAT_TYPE_STRING},
    {OH_MD_KEY_LANGUAGE, FORMAT_TYPE_STRING},
    {OH_MD_KEY_DESCRIPTION, FORMAT_TYPE_STRING},
    {OH_MD_KEY_LYRICS, FORMAT_TYPE_STRING},
    {OH_MD_KEY_TRACK_COUNT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_CHANNEL_LAYOUT, FORMAT_TYPE_INT64},
    {OH_MD_KEY_BITS_PER_CODED_SAMPLE, FORMAT_TYPE_INT32},
    {OH_MD_KEY_AAC_IS_ADTS, FORMAT_TYPE_INT32},
    {OH_MD_KEY_SBR, FORMAT_TYPE_INT32},
    {OH_MD_KEY_COMPLIANCE_LEVEL, FORMAT_TYPE_INT32},
    {OH_MD_KEY_IDENTIFICATION_HEADER, FORMAT_TYPE_ADDR},
    {OH_MD_KEY_SETUP_HEADER, FORMAT_TYPE_ADDR},
    {OH_MD_KEY_SCALING_MODE, FORMAT_TYPE_INT32},
    {OH_MD_MAX_INPUT_BUFFER_COUNT, FORMAT_TYPE_INT32},
    {OH_MD_MAX_OUTPUT_BUFFER_COUNT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_AUDIO_COMPRESSION_LEVEL, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_IS_HDR_VIVID, FORMAT_TYPE_INT32},
    {OH_MD_KEY_AUDIO_OBJECT_NUMBER, FORMAT_TYPE_INT32},
    {OH_MD_KEY_AUDIO_VIVID_METADATA, FORMAT_TYPE_ADDR},
    {OH_FEATURE_PROPERTY_KEY_VIDEO_ENCODER_MAX_LTR_FRAME_COUNT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_ENCODER_QP_MIN, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_SIZE, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_ENCODER_LTR_FRAME_COUNT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_PER_FRAME_IS_LTR, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_MARK_LTR, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_USE_LTR, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_PER_FRAME_POC, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_CROP_TOP, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_CROP_BOTTOM, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_CROP_LEFT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_CROP_RIGHT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_STRIDE, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_SLICE_HEIGHT, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_ENABLE_LOW_LATENCY, FORMAT_TYPE_INT32},
    {OH_MD_KEY_VIDEO_ENCODER_QP_MAX, FORMAT_TYPE_INT32}
};
constexpr int32_t invalidKey = -1;

int32_t GetType(const char * key)
{
    auto type = Key_Type_Map.find(key);
    return type != Key_Type_Map.end() ? type->second : invalidKey;
}

class TEST_SUIT : public testing::TestWithParam<const char *> {
    public:
        static void SetUpTestCase(void){};
        static void TearDownTestCase(void){};
        void SetUp(void){};
        void TearDown(void){};
};

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(
    "DEFAULT_KEY",
    OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_USE_LTR,
    OH_MD_KEY_CODEC_MIME,
    OH_MD_KEY_TRACK_TYPE,
    OH_MD_KEY_AUDIO_SAMPLE_FORMAT,
    OH_MD_KEY_MAX_INPUT_SIZE,
    OH_MD_KEY_WIDTH,
    OH_MD_KEY_VIDEO_IS_HDR_VIVID,
    OH_MD_KEY_PIXEL_FORMAT,
    OH_MD_KEY_BITRATE,
    OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
    OH_MD_KEY_FRAME_RATE,
    OH_MD_KEY_PROFILE,
    OH_MD_KEY_AUD_CHANNEL_COUNT,
    OH_MD_KEY_AUD_SAMPLE_RATE,
    OH_MD_KEY_I_FRAME_INTERVAL,
    OH_MD_KEY_AUDIO_VIVID_METADATA,
    OH_MD_KEY_RANGE_FLAG,
    OH_MD_KEY_COLOR_PRIMARIES,
    OH_MD_KEY_TRANSFER_CHARACTERISTICS,
    OH_MD_KEY_ARTIST,
    OH_MD_KEY_REQUEST_I_FRAME,
    OH_MD_KEY_VIDEO_ENCODER_QP_MIN,
    OH_MD_KEY_CODEC_CONFIG,
    OH_MD_KEY_TITLE,
    OH_MD_KEY_MATRIX_COEFFICIENTS,
    OH_MD_KEY_ALBUM,
    OH_MD_KEY_ALBUM_ARTIST,
    OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_SIZE,
    OH_MD_KEY_COMMENT,
    OH_MD_KEY_DATE,
    OH_MD_MAX_INPUT_BUFFER_COUNT,
    OH_MD_KEY_LANGUAGE,
    OH_MD_KEY_DESCRIPTION,
    OH_MD_KEY_IDENTIFICATION_HEADER,
    OH_MD_KEY_TRACK_COUNT,
    OH_MD_KEY_CHANNEL_LAYOUT,
    OH_MD_KEY_BITS_PER_CODED_SAMPLE,
    OH_MD_KEY_AAC_IS_ADTS,
    OH_MD_KEY_SBR,
    OH_MD_KEY_COMPLIANCE_LEVEL,
    OH_MD_KEY_LYRICS,
    OH_MD_KEY_SETUP_HEADER,
    OH_MD_KEY_VIDEO_ENCODER_QP_MAX,
    OH_MD_KEY_COPYRIGHT,
    OH_MD_MAX_OUTPUT_BUFFER_COUNT,
    OH_MD_KEY_AUDIO_COMPRESSION_LEVEL,
    OH_MD_KEY_HEIGHT,
    OH_MD_KEY_AUDIO_OBJECT_NUMBER,
    OH_FEATURE_PROPERTY_KEY_VIDEO_ENCODER_MAX_LTR_FRAME_COUNT,
    OH_MD_KEY_ROTATION,
    OH_MD_KEY_VIDEO_SLICE_HEIGHT,
    OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY,
    OH_MD_KEY_QUALITY,
    OH_MD_KEY_SCALING_MODE,
    OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
    OH_MD_KEY_VIDEO_CROP_LEFT,
    OH_MD_KEY_GENRE,
    OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_MARK_LTR,
    OH_MD_KEY_DURATION,
    OH_MD_KEY_VIDEO_PER_FRAME_IS_LTR,
    OH_MD_KEY_VIDEO_PER_FRAME_POC,
    OH_MD_KEY_VIDEO_CROP_TOP,
    OH_MD_KEY_VIDEO_CROP_BOTTOM,
    OH_MD_KEY_VIDEO_ENCODER_LTR_FRAME_COUNT,
    OH_MD_KEY_VIDEO_CROP_RIGHT,
    OH_MD_KEY_VIDEO_STRIDE,
    OH_MD_KEY_VIDEO_ENABLE_LOW_LATENCY
));

/**
 * @tc.name: TYPE_INT32_TEST
 * @tc.desc: key calls the OH_AVFormat_SetIntValue func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, TYPE_INT32_TEST, TestSize.Level1)
{
    auto key = GetParam();
    auto type = GetType(key);
    if (type != invalidKey) {
        OH_AVFormat *format = OH_AVFormat_Create();
        int32_t defaultVal = 1;
        bool res = (type == FORMAT_TYPE_INT32 || type == FORMAT_TYPE_NONE);
        ASSERT_EQ(res, OH_AVFormat_SetIntValue(format, key, defaultVal));
        OH_AVFormat_Destroy(format);
    }
    else {
        std::cout << "Key_Type_Map not found key = " << key << std::endl;
    }
}

/**
 * @tc.name: TYPE_INT64_TEST
 * @tc.desc: key calls the OH_AVFormat_SetLongValue func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, TYPE_INT64_TEST, TestSize.Level1)
{
    auto key = GetParam();
    auto type = GetType(key);
    if (type != invalidKey) {
        OH_AVFormat *format = OH_AVFormat_Create();
        int64_t defaultVal = 1;
        bool res = (type == FORMAT_TYPE_INT64 || type == FORMAT_TYPE_NONE);
        ASSERT_EQ(res, OH_AVFormat_SetLongValue(format, key, defaultVal));
        OH_AVFormat_Destroy(format);
    }
    else {
        std::cout << "Key_Type_Map not found key = " << key << std::endl;
    }
}

/**
 * @tc.name: TYPE_FLOAT_TEST
 * @tc.desc: key calls the OH_AVFormat_SetFloatValue func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, TYPE_FLOAT_TEST, TestSize.Level1)
{
    auto key = GetParam();
    auto type = GetType(key);
    if (type != invalidKey) {
        OH_AVFormat *format = OH_AVFormat_Create();
        float defaultVal = 1.1;
        bool res = (type == FORMAT_TYPE_FLOAT || type == FORMAT_TYPE_NONE);
        ASSERT_EQ(res, OH_AVFormat_SetFloatValue(format, key, defaultVal));
        OH_AVFormat_Destroy(format);
    }
    else {
        std::cout << "Key_Type_Map not found key = " << key << std::endl;
    }
}

/**
 * @tc.name: TYPE_DOUBLE_TEST
 * @tc.desc: key calls the OH_AVFormat_SetDoubleValue func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, TYPE_DOUBLE_TEST, TestSize.Level1)
{
    auto key = GetParam();
    auto type = GetType(key);
    if (type != invalidKey) {
        OH_AVFormat *format = OH_AVFormat_Create();
        double defaultVal = 1.1;
        bool res = (type == FORMAT_TYPE_DOUBLE || type == FORMAT_TYPE_NONE);
        ASSERT_EQ(res, OH_AVFormat_SetDoubleValue(format, key, defaultVal));
        OH_AVFormat_Destroy(format);
    }
    else {
        std::cout << "Key_Type_Map not found key = " << key << std::endl;
    }
}

/**
 * @tc.name: TYPE_STRING_TEST
 * @tc.desc: key calls the OH_AVFormat_SetStringValue func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, TYPE_STRING_TEST, TestSize.Level1)
{
    auto key = GetParam();
    auto type = GetType(key);
    if (type != invalidKey) {
        OH_AVFormat *format = OH_AVFormat_Create();
        const char *defaultVal = "aaa";
        bool res = (type == FORMAT_TYPE_STRING || type == FORMAT_TYPE_NONE);
        ASSERT_EQ(res, OH_AVFormat_SetStringValue(format, key, defaultVal));
        OH_AVFormat_Destroy(format);
    }
    else {
        std::cout << "Key_Type_Map not found key = " << key << std::endl;
    }
}

/**
 * @tc.name: TYPE_ADDR_TEST
 * @tc.desc: key calls the OH_AVFormat_SetBuffer func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, TYPE_ADDR_TEST, TestSize.Level1)
{
    auto key = GetParam();
    auto type = GetType(key);
    if (type != invalidKey) {
        OH_AVFormat *format = OH_AVFormat_Create();
        uint8_t buffer[] = {1, 2, 3, 4};
        bool res = (type == FORMAT_TYPE_ADDR || type == FORMAT_TYPE_NONE);
        ASSERT_EQ(res, OH_AVFormat_SetBuffer(format, key, buffer, sizeof(buffer)));
        OH_AVFormat_Destroy(format);
    }
    else {
        std::cout << "Key_Type_Map not found key = " << key << std::endl;
    }
}
}