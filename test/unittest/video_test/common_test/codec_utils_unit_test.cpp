/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <memory>
#include "codec_utils.h"
#include "media_description.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer.h"
#include "unittest_log.h"
#include "libavcodec/avcodec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Codec;
using AVAllocatorFactory = OHOS::Media::AVAllocatorFactory;
using MemoryFlag = OHOS::Media::MemoryFlag;
using MediaAVBuffer = OHOS::Media::AVBuffer;

namespace {
constexpr int32_t TEST_WIDTH = 320;
constexpr int32_t TEST_HEIGHT = 240;
}

namespace OHOS {
namespace MediaAVCodec {
class CodecUtilsUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    Format CreateFormatWithYuv420();
    Format CreateFormatWithNv12();
    Format CreateFormatWithNv21();
    Format CreateFormatWithRgba();
};

void CodecUtilsUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void CodecUtilsUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void CodecUtilsUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void CodecUtilsUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
}

Format CodecUtilsUnitTest::CreateFormatWithYuv420()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    return fmt;
}

Format CodecUtilsUnitTest::CreateFormatWithNv12()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    return fmt;
}

Format CodecUtilsUnitTest::CreateFormatWithNv21()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV21));
    return fmt;
}

Format CodecUtilsUnitTest::CreateFormatWithRgba()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    return fmt;
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_pixel_format_01, TestSize.Level1)
{
    bool result = IsValidPixelFormat(static_cast<int32_t>(VideoPixelFormat::YUVI420));
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_pixel_format_02, TestSize.Level1)
{
    bool result = IsValidPixelFormat(static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_pixel_format_03, TestSize.Level1)
{
    bool result = IsValidPixelFormat(static_cast<int32_t>(VideoPixelFormat::NV21));
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_pixel_format_04, TestSize.Level1)
{
    bool result = IsValidPixelFormat(static_cast<int32_t>(VideoPixelFormat::RGBA));
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_pixel_format_05, TestSize.Level1)
{
    bool result = IsValidPixelFormat(-1);
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_pixel_format_06, TestSize.Level1)
{
    bool result = IsValidPixelFormat(100); // 100
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_pixel_format_07, TestSize.Level1)
{
    bool result = IsValidPixelFormat(static_cast<int32_t>(VideoPixelFormat::YUV420P));
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_pixel_format_08, TestSize.Level1)
{
    bool result = IsValidPixelFormat(static_cast<int32_t>(VideoPixelFormat::SURFACE_FORMAT));
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_pixel_format_09, TestSize.Level1)
{
    bool result = IsValidPixelFormat(static_cast<int32_t>(VideoPixelFormat::RGBA1010102));
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_scale_type_01, TestSize.Level1)
{
    bool result = IsValidScaleType(static_cast<int32_t>(ScalingMode::SCALING_MODE_SCALE_TO_WINDOW));
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_scale_type_02, TestSize.Level1)
{
    bool result = IsValidScaleType(static_cast<int32_t>(ScalingMode::SCALING_MODE_SCALE_CROP));
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_scale_type_03, TestSize.Level1)
{
    bool result = IsValidScaleType(-1);
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_scale_type_04, TestSize.Level1)
{
    bool result = IsValidScaleType(0);
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_scale_type_05, TestSize.Level1)
{
    bool result = IsValidScaleType(100); // 100
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_rotation_01, TestSize.Level1)
{
    bool result = IsValidRotation(static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_0));
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_rotation_02, TestSize.Level1)
{
    bool result = IsValidRotation(static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_90));
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_rotation_03, TestSize.Level1)
{
    bool result = IsValidRotation(static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_180));
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_rotation_04, TestSize.Level1)
{
    bool result = IsValidRotation(static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_270));
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_rotation_05, TestSize.Level1)
{
    bool result = IsValidRotation(-1);
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_rotation_06, TestSize.Level1)
{
    bool result = IsValidRotation(100); // 100
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_valid_rotation_07, TestSize.Level1)
{
    bool result = IsValidRotation(0);
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_format_01, TestSize.Level1)
{
    GraphicPixelFormat result = TranslateSurfaceFormat(VideoPixelFormat::YUVI420);
    EXPECT_EQ(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_format_02, TestSize.Level1)
{
    GraphicPixelFormat result = TranslateSurfaceFormat(VideoPixelFormat::RGBA);
    EXPECT_EQ(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_format_03, TestSize.Level1)
{
    GraphicPixelFormat result = TranslateSurfaceFormat(VideoPixelFormat::NV12);
    EXPECT_EQ(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_format_04, TestSize.Level1)
{
    GraphicPixelFormat result = TranslateSurfaceFormat(VideoPixelFormat::NV21);
    EXPECT_EQ(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_format_05, TestSize.Level1)
{
    GraphicPixelFormat result = TranslateSurfaceFormat(VideoPixelFormat::UNKNOWN);
    EXPECT_EQ(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_format_06, TestSize.Level1)
{
    GraphicPixelFormat result = TranslateSurfaceFormat(VideoPixelFormat::YUV420P);
    EXPECT_EQ(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_format_07, TestSize.Level1)
{
    GraphicPixelFormat result = TranslateSurfaceFormat(VideoPixelFormat::SURFACE_FORMAT);
    EXPECT_EQ(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_format_08, TestSize.Level1)
{
    GraphicPixelFormat result = TranslateSurfaceFormat(VideoPixelFormat::RGBA1010102);
    EXPECT_EQ(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_rotation_01, TestSize.Level1)
{
    GraphicTransformType result = TranslateSurfaceRotation(VideoRotation::VIDEO_ROTATION_0);
    EXPECT_EQ(GRAPHIC_ROTATE_NONE, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_rotation_02, TestSize.Level1)
{
    GraphicTransformType result = TranslateSurfaceRotation(VideoRotation::VIDEO_ROTATION_90);
    EXPECT_EQ(GRAPHIC_ROTATE_270, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_rotation_03, TestSize.Level1)
{
    GraphicTransformType result = TranslateSurfaceRotation(VideoRotation::VIDEO_ROTATION_180);
    EXPECT_EQ(GRAPHIC_ROTATE_180, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_rotation_04, TestSize.Level1)
{
    GraphicTransformType result = TranslateSurfaceRotation(VideoRotation::VIDEO_ROTATION_270);
    EXPECT_EQ(GRAPHIC_ROTATE_90, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_rotation_05, TestSize.Level1)
{
    GraphicTransformType result = TranslateSurfaceRotation(static_cast<VideoRotation>(999)); // 999
    EXPECT_EQ(GRAPHIC_ROTATE_NONE, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_translate_surface_rotation_06, TestSize.Level1)
{
    GraphicTransformType result = TranslateSurfaceRotation(static_cast<VideoRotation>(-1));
    EXPECT_EQ(GRAPHIC_ROTATE_NONE, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_from_ffmpeg_01, TestSize.Level1)
{
    VideoPixelFormat result = ConvertPixelFormatFromFFmpeg(AV_PIX_FMT_YUV420P);
    EXPECT_EQ(VideoPixelFormat::YUVI420, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_from_ffmpeg_02, TestSize.Level1)
{
    VideoPixelFormat result = ConvertPixelFormatFromFFmpeg(AV_PIX_FMT_NV12);
    EXPECT_EQ(VideoPixelFormat::NV12, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_from_ffmpeg_03, TestSize.Level1)
{
    VideoPixelFormat result = ConvertPixelFormatFromFFmpeg(AV_PIX_FMT_NV21);
    EXPECT_EQ(VideoPixelFormat::NV21, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_from_ffmpeg_04, TestSize.Level1)
{
    VideoPixelFormat result = ConvertPixelFormatFromFFmpeg(AV_PIX_FMT_RGBA);
    EXPECT_EQ(VideoPixelFormat::RGBA, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_from_ffmpeg_05, TestSize.Level1)
{
    VideoPixelFormat result = ConvertPixelFormatFromFFmpeg(AV_PIX_FMT_NONE);
    EXPECT_EQ(VideoPixelFormat::UNKNOWN, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_from_ffmpeg_06, TestSize.Level1)
{
    VideoPixelFormat result = ConvertPixelFormatFromFFmpeg(9999); // 9999
    EXPECT_EQ(VideoPixelFormat::UNKNOWN, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_from_ffmpeg_07, TestSize.Level1)
{
    VideoPixelFormat result = ConvertPixelFormatFromFFmpeg(-1);
    EXPECT_EQ(VideoPixelFormat::UNKNOWN, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_to_ffmpeg_01, TestSize.Level1)
{
    AVPixelFormat result = ConvertPixelFormatToFFmpeg(VideoPixelFormat::YUVI420);
    EXPECT_EQ(AV_PIX_FMT_YUV420P, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_to_ffmpeg_02, TestSize.Level1)
{
    AVPixelFormat result = ConvertPixelFormatToFFmpeg(VideoPixelFormat::NV12);
    EXPECT_EQ(AV_PIX_FMT_NV12, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_to_ffmpeg_03, TestSize.Level1)
{
    AVPixelFormat result = ConvertPixelFormatToFFmpeg(VideoPixelFormat::NV21);
    EXPECT_EQ(AV_PIX_FMT_NV21, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_to_ffmpeg_04, TestSize.Level1)
{
    AVPixelFormat result = ConvertPixelFormatToFFmpeg(VideoPixelFormat::RGBA);
    EXPECT_EQ(AV_PIX_FMT_RGBA, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_to_ffmpeg_05, TestSize.Level1)
{
    AVPixelFormat result = ConvertPixelFormatToFFmpeg(VideoPixelFormat::UNKNOWN);
    EXPECT_EQ(AV_PIX_FMT_NONE, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_to_ffmpeg_06, TestSize.Level1)
{
    AVPixelFormat result = ConvertPixelFormatToFFmpeg(VideoPixelFormat::RGBA1010102);
    EXPECT_EQ(AV_PIX_FMT_NONE, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_to_ffmpeg_07, TestSize.Level1)
{
    AVPixelFormat result = ConvertPixelFormatToFFmpeg(static_cast<VideoPixelFormat>(999)); // 999
    EXPECT_EQ(AV_PIX_FMT_NONE, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_pixel_format_to_ffmpeg_08, TestSize.Level1)
{
    AVPixelFormat result = ConvertPixelFormatToFFmpeg(VideoPixelFormat::YUV420P);
    EXPECT_EQ(AV_PIX_FMT_NONE, result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_yuv_format_01, TestSize.Level1)
{
    VideoPixelFormat fmt = VideoPixelFormat::YUVI420;
    bool result = IsYuvFormat(fmt);
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_yuv_format_02, TestSize.Level1)
{
    VideoPixelFormat fmt = VideoPixelFormat::NV12;
    bool result = IsYuvFormat(fmt);
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_yuv_format_03, TestSize.Level1)
{
    VideoPixelFormat fmt = VideoPixelFormat::NV21;
    bool result = IsYuvFormat(fmt);
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_yuv_format_04, TestSize.Level1)
{
    VideoPixelFormat fmt = VideoPixelFormat::RGBA;
    bool result = IsYuvFormat(fmt);
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_yuv_format_05, TestSize.Level1)
{
    VideoPixelFormat fmt = VideoPixelFormat::RGBA1010102;
    bool result = IsYuvFormat(fmt);
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_yuv_format_06, TestSize.Level1)
{
    VideoPixelFormat fmt = static_cast<VideoPixelFormat>(999); // 999
    bool result = IsYuvFormat(fmt);
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_rgb_format_01, TestSize.Level1)
{
    VideoPixelFormat fmt = VideoPixelFormat::RGBA;
    bool result = IsRgbFormat(fmt);
    EXPECT_TRUE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_rgb_format_02, TestSize.Level1)
{
    VideoPixelFormat fmt = VideoPixelFormat::YUVI420;
    bool result = IsRgbFormat(fmt);
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_rgb_format_03, TestSize.Level1)
{
    VideoPixelFormat fmt = VideoPixelFormat::RGBA1010102;
    bool result = IsRgbFormat(fmt);
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_is_rgb_format_04, TestSize.Level1)
{
    VideoPixelFormat fmt = static_cast<VideoPixelFormat>(999); // 999
    bool result = IsRgbFormat(fmt);
    EXPECT_FALSE(result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_avstrerror_01, TestSize.Level1)
{
    std::string result = AVStrError(0);
    EXPECT_FALSE(result.empty());
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_avstrerror_02, TestSize.Level1)
{
    std::string result = AVStrError(-1);
    EXPECT_FALSE(result.empty());
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_avstrerror_03, TestSize.Level1)
{
    std::string result = AVStrError(-1000); // -1000
    EXPECT_FALSE(result.empty());
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_avstrerror_04, TestSize.Level1)
{
    std::string result = AVStrError(1000); // 1000
    EXPECT_FALSE(result.empty());
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_avstrerror_05, TestSize.Level1)
{
    std::string result = AVStrError(AVERROR_EOF);
    EXPECT_FALSE(result.empty());
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_avstrerror_06, TestSize.Level1)
{
    std::string result = AVStrError(AVERROR(EIO));
    EXPECT_FALSE(result.empty());
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_get_meta_data_type_by_trans_func_01, TestSize.Level1)
{
    uint32_t result = GetMetaDataTypeByTransFunc(TRANSFER_CHARACTERISTIC_PQ);
    EXPECT_EQ(static_cast<uint32_t>(2), result); // 2
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_get_meta_data_type_by_trans_func_02, TestSize.Level1)
{
    uint32_t result = GetMetaDataTypeByTransFunc(TRANSFER_CHARACTERISTIC_HLG);
    EXPECT_EQ(static_cast<uint32_t>(1), result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_get_meta_data_type_by_trans_func_03, TestSize.Level1)
{
    uint32_t result = GetMetaDataTypeByTransFunc(TRANSFER_CHARACTERISTIC_BT709);
    EXPECT_EQ(static_cast<uint32_t>(0), result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_get_meta_data_type_by_trans_func_04, TestSize.Level1)
{
    uint32_t result = GetMetaDataTypeByTransFunc(TRANSFER_CHARACTERISTIC_LINEAR);
    EXPECT_EQ(static_cast<uint32_t>(0), result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_get_meta_data_type_by_trans_func_05, TestSize.Level1)
{
    uint32_t result = GetMetaDataTypeByTransFunc(TRANSFER_CHARACTERISTIC_IEC_61966_2_1);
    EXPECT_EQ(static_cast<uint32_t>(0), result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_get_meta_data_type_by_trans_func_06, TestSize.Level1)
{
    uint32_t result = GetMetaDataTypeByTransFunc(999); // 999
    EXPECT_EQ(static_cast<uint32_t>(0), result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_get_meta_data_type_by_trans_func_07, TestSize.Level1)
{
    uint32_t result = GetMetaDataTypeByTransFunc(TRANSFER_CHARACTERISTIC_BT2020_10BIT);
    EXPECT_EQ(static_cast<uint32_t>(0), result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_get_meta_data_type_by_trans_func_08, TestSize.Level1)
{
    uint32_t result = GetMetaDataTypeByTransFunc(TRANSFER_CHARACTERISTIC_BT2020_12BIT);
    EXPECT_EQ(static_cast<uint32_t>(0), result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_get_meta_data_type_by_trans_func_09, TestSize.Level1)
{
    uint32_t result = GetMetaDataTypeByTransFunc(TRANSFER_CHARACTERISTIC_BT601);
    EXPECT_EQ(static_cast<uint32_t>(0), result);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_01, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(0, COLOR_PRIMARY_BT709,
        TRANSFER_CHARACTERISTIC_BT709, MATRIX_COEFFICIENT_BT709, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_OK, ret);
    EXPECT_EQ(sizeof(int) * 4, colorSpaceInfoData.size()); // 4
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_02, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(1, COLOR_PRIMARY_BT709,
        TRANSFER_CHARACTERISTIC_BT709, MATRIX_COEFFICIENT_BT709, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_03, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(0, COLOR_PRIMARY_BT601_625,
        TRANSFER_CHARACTERISTIC_BT601, MATRIX_COEFFICIENT_BT601_625, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_04, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(0, COLOR_PRIMARY_BT2020,
        TRANSFER_CHARACTERISTIC_PQ, MATRIX_COEFFICIENT_BT2020_NCL, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_05, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(0, COLOR_PRIMARY_P3DCI,
        TRANSFER_CHARACTERISTIC_PQ, MATRIX_COEFFICIENT_ICTCP, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_06, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(0, COLOR_PRIMARY_BT601_525,
        TRANSFER_CHARACTERISTIC_LINEAR, MATRIX_COEFFICIENT_BT601_525, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_07, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(0, COLOR_PRIMARY_BT2020,
        TRANSFER_CHARACTERISTIC_BT2020_10BIT, MATRIX_COEFFICIENT_BT2020_NCL, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_08, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(0, COLOR_PRIMARY_BT2020,
        TRANSFER_CHARACTERISTIC_BT2020_12BIT, MATRIX_COEFFICIENT_BT2020_NCL, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_09, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(1, COLOR_PRIMARY_BT2020,
        TRANSFER_CHARACTERISTIC_PQ, MATRIX_COEFFICIENT_BT2020_NCL, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_10, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(1, COLOR_PRIMARY_P3D65,
        TRANSFER_CHARACTERISTIC_PQ, MATRIX_COEFFICIENT_ICTCP, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_err_01, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(0, 999, // 999
        TRANSFER_CHARACTERISTIC_BT709, MATRIX_COEFFICIENT_BT709, colorSpaceInfoData);
    EXPECT_EQ(AVCS_ERR_UNSUPPORT, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_err_02, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(0, COLOR_PRIMARY_BT709,
        999, MATRIX_COEFFICIENT_BT709, colorSpaceInfoData); // 999
    EXPECT_EQ(AVCS_ERR_UNSUPPORT, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_params_to_colorspace_info_err_03, TestSize.Level1)
{
    std::vector<uint8_t> colorSpaceInfoData;
    int32_t ret = ConvertParamsToColorSpaceInfo(0, COLOR_PRIMARY_BT709,
        TRANSFER_CHARACTERISTIC_BT709, 999, colorSpaceInfoData); // 999
    EXPECT_EQ(AVCS_ERR_UNSUPPORT, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_video_frame_01, TestSize.Level1)
{
    std::shared_ptr<Scale> scale = nullptr;
    std::shared_ptr<AVFrame> frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* f){ av_frame_free(&f); });
    ASSERT_NE(nullptr, frame);
    frame->width = TEST_WIDTH;
    frame->height = TEST_HEIGHT;
    frame->format = AV_PIX_FMT_YUV420P;
    frame->data[0] = new uint8_t[TEST_WIDTH * TEST_HEIGHT];
    frame->data[1] = new uint8_t[TEST_WIDTH * TEST_HEIGHT / 2]; // 2
    frame->data[2] = new uint8_t[TEST_WIDTH * TEST_HEIGHT / 2]; // 2
    frame->linesize[0] = TEST_WIDTH;
    frame->linesize[1] = TEST_WIDTH / 2; // 2
    frame->linesize[2] = TEST_WIDTH / 2; // 2

    uint8_t* dstData[4] = {nullptr}; // 4
    int32_t dstLineSize[4] = {0}; // 4

    int32_t ret = ConvertVideoFrame(&scale, frame, dstData, dstLineSize, AV_PIX_FMT_RGBA);

    delete[] frame->data[0];
    delete[] frame->data[1];
    delete[] frame->data[2]; // 2
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_video_frame_02, TestSize.Level1)
{
    std::shared_ptr<Scale> scale = nullptr;
    uint8_t* srcData[4] = {new uint8_t[TEST_WIDTH * TEST_HEIGHT], nullptr, nullptr, nullptr}; // 4
    int32_t srcLineSize[4] = {TEST_WIDTH, 0, 0, 0}; // 4

    uint8_t* dstData[4] = {nullptr}; // 4
    int32_t dstLineSize[4] = {0}; // 4

    int32_t ret = ConvertVideoFrame(&scale, srcData, srcLineSize, AV_PIX_FMT_YUV420P,
                                    TEST_WIDTH, TEST_HEIGHT, dstData, dstLineSize, AV_PIX_FMT_RGBA);

    delete[] srcData[0];
    EXPECT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_video_frame_03, TestSize.Level1)
{
    std::shared_ptr<Scale> scale = nullptr;
    std::shared_ptr<AVFrame> frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* f){ av_frame_free(&f); });
    ASSERT_NE(nullptr, frame);
    frame->width = TEST_WIDTH;
    frame->height = TEST_HEIGHT;
    frame->format = AV_PIX_FMT_NV12;
    frame->data[0] = new uint8_t[TEST_WIDTH * TEST_HEIGHT];
    frame->data[1] = new uint8_t[TEST_WIDTH * TEST_HEIGHT / 2]; // 2
    frame->linesize[0] = TEST_WIDTH;
    frame->linesize[1] = TEST_WIDTH / 2; // 2

    uint8_t* dstData[4] = {nullptr}; // 4
    int32_t dstLineSize[4] = {0}; // 4

    int32_t ret = ConvertVideoFrame(&scale, frame, dstData, dstLineSize, AV_PIX_FMT_YUV420P);

    delete[] frame->data[0];
    delete[] frame->data[1];
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_video_frame_04, TestSize.Level1)
{
    std::shared_ptr<Scale> scale = nullptr;
    std::shared_ptr<AVFrame> frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* f){ av_frame_free(&f); });
    ASSERT_NE(nullptr, frame);
    frame->width = TEST_WIDTH;
    frame->height = TEST_HEIGHT;
    frame->format = AV_PIX_FMT_RGBA;
    frame->data[0] = new uint8_t[TEST_WIDTH * TEST_HEIGHT * 4]; // 4
    frame->linesize[0] = TEST_WIDTH * 4; // 4

    uint8_t* dstData[4] = {nullptr}; // 4
    int32_t dstLineSize[4] = {0}; // 4

    int32_t ret = ConvertVideoFrame(&scale, frame, dstData, dstLineSize, AV_PIX_FMT_NV21);

    delete[] frame->data[0];
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_video_frame_05, TestSize.Level1)
{
    std::shared_ptr<Scale> scale = nullptr;
    uint8_t* srcData[4] = {new uint8_t[TEST_WIDTH * TEST_HEIGHT], new uint8_t[TEST_WIDTH * TEST_HEIGHT / 2], // 4
        nullptr, nullptr};
    int32_t srcLineSize[4] = {TEST_WIDTH, TEST_WIDTH, 0, 0}; // 4

    uint8_t* dstData[4] = {nullptr}; // 4
    int32_t dstLineSize[4] = {0}; // 4

    int32_t ret = ConvertVideoFrame(&scale, srcData, srcLineSize, AV_PIX_FMT_NV12,
                                    TEST_WIDTH, TEST_HEIGHT, dstData, dstLineSize, AV_PIX_FMT_RGBA);

    delete[] srcData[0];
    delete[] srcData[1];
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_video_frame_06, TestSize.Level1)
{
    std::shared_ptr<Scale> scale = std::make_shared<Scale>();
    uint8_t* srcData[4] = {new uint8_t[TEST_WIDTH * TEST_HEIGHT * 4], nullptr, nullptr, nullptr}; // 4
    int32_t srcLineSize[4] = {TEST_WIDTH * 4, 0, 0, 0}; // 4

    uint8_t* dstData[4] = {nullptr}; // 4
    int32_t dstLineSize[4] = {0}; // 4

    ScalePara scalePara{TEST_WIDTH, TEST_HEIGHT, AV_PIX_FMT_RGBA, TEST_WIDTH, TEST_HEIGHT, AV_PIX_FMT_YUV420P};
    (void)scale->Init(scalePara, dstData, dstLineSize);
    int32_t ret = ConvertVideoFrame(&scale, srcData, srcLineSize, AV_PIX_FMT_RGBA,
                                    TEST_WIDTH, TEST_HEIGHT, dstData, dstLineSize, AV_PIX_FMT_YUV420P);

    delete[] srcData[0];
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_convert_video_frame_07, TestSize.Level1)
{
    std::shared_ptr<Scale> scale = std::make_shared<Scale>();
    std::shared_ptr<AVFrame> frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* f){ av_frame_free(&f); });
    ASSERT_NE(nullptr, frame);
    frame->width = TEST_WIDTH;
    frame->height = TEST_HEIGHT;
    frame->format = AV_PIX_FMT_YUV420P;
    frame->data[0] = new uint8_t[TEST_WIDTH * TEST_HEIGHT];
    frame->data[1] = new uint8_t[TEST_WIDTH * TEST_HEIGHT / 2]; // 2
    frame->data[2] = new uint8_t[TEST_WIDTH * TEST_HEIGHT / 2]; // 2
    frame->linesize[0] = TEST_WIDTH;
    frame->linesize[1] = TEST_WIDTH / 2; // 2
    frame->linesize[2] = TEST_WIDTH / 2; // 2

    uint8_t* dstData[4] = {nullptr}; // 4
    int32_t dstLineSize[4] = {0}; // 4

    ScalePara scalePara{TEST_WIDTH, TEST_HEIGHT, AV_PIX_FMT_YUV420P, TEST_WIDTH, TEST_HEIGHT, AV_PIX_FMT_NV12};
    (void)scale->Init(scalePara, dstData, dstLineSize);
    int32_t ret = ConvertVideoFrame(&scale, frame, dstData, dstLineSize, AV_PIX_FMT_NV12);

    delete[] frame->data[0];
    delete[] frame->data[1];
    delete[] frame->data[2]; // 2
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_surface_data_01, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* scaleData[4] = {nullptr, nullptr, nullptr, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH, TEST_WIDTH / 2, TEST_WIDTH / 2, 0}; // 4, 2
    SurfaceInfo surfaceInfo;
    surfaceInfo.surfaceStride = TEST_WIDTH;
    surfaceInfo.scaleData = scaleData;
    surfaceInfo.scaleLineSize = scaleLineSize;
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    int32_t ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_surface_data_02, TestSize.Level1)
{
    std::shared_ptr<AVMemory> memory = nullptr;
    uint8_t* scaleData[4] = {nullptr, nullptr, nullptr, nullptr}; // 4
    int32_t scaleLineSize[4] = {0, 0, 0, 0}; // 4
    SurfaceInfo surfaceInfo;
    surfaceInfo.surfaceStride = TEST_WIDTH;
    surfaceInfo.scaleData = scaleData;
    surfaceInfo.scaleLineSize = scaleLineSize;
    Format format;
    int32_t ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_EQ(AVCS_ERR_INVALID_VAL, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_surface_data_03, TestSize.Level1)
{
    constexpr int32_t bufferSize = (TEST_WIDTH + 2) * TEST_HEIGHT * 4; // 4, 2
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* yData = new (std::nothrow) uint8_t[(TEST_WIDTH + 2) * TEST_HEIGHT]; // 2
    uint8_t* uData = new (std::nothrow) uint8_t[(TEST_WIDTH + 2) * TEST_HEIGHT / 4]; // 4, 2
    uint8_t* vData = new (std::nothrow) uint8_t[(TEST_WIDTH + 2) * TEST_HEIGHT / 4]; // 4, 2
    ASSERT_NE(yData, nullptr);
    ASSERT_NE(uData, nullptr);
    ASSERT_NE(vData, nullptr);
    uint8_t* scaleData[4] = {yData, uData, vData, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH, TEST_WIDTH / 2, TEST_WIDTH / 2, 0}; // 4, 2
    SurfaceInfo surfaceInfo;
    surfaceInfo.surfaceStride = TEST_WIDTH + 2; // 2
    surfaceInfo.scaleData = scaleData;
    surfaceInfo.scaleLineSize = scaleLineSize;
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    int32_t ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    int32_t scaleLineSize2[4] = {0, 0, 0, 0}; // 4
    surfaceInfo.scaleLineSize = scaleLineSize2;
    ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    delete[] yData;
    delete[] uData;
    delete[] vData;
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_surface_data_04, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* rgbData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT * 4]; // 4
    ASSERT_NE(rgbData, nullptr);
    uint8_t* scaleData[4] = {rgbData, nullptr, nullptr, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH * 4, 0, 0, 0}; // 4
    SurfaceInfo surfaceInfo;
    surfaceInfo.surfaceStride = TEST_WIDTH;
    surfaceInfo.scaleData = scaleData;
    surfaceInfo.scaleLineSize = scaleLineSize;
    surfaceInfo.surfaceFence = sptr<SyncFence>(new SyncFence(-1));
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    int32_t ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    int32_t scaleLineSize2[4] = {0, 0, 0, 0}; // 4
    surfaceInfo.scaleLineSize = scaleLineSize2;
    ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    delete[] rgbData;
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_surface_data_05, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* yData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT];
    uint8_t* uData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    uint8_t* vData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    ASSERT_NE(yData, nullptr);
    ASSERT_NE(uData, nullptr);
    ASSERT_NE(vData, nullptr);
    uint8_t* scaleData[4] = {yData, uData, vData, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH, TEST_WIDTH / 2, TEST_WIDTH / 2, 0}; // 4, 2
    SurfaceInfo surfaceInfo;
    surfaceInfo.surfaceStride = TEST_WIDTH;
    surfaceInfo.scaleData = scaleData;
    surfaceInfo.scaleLineSize = scaleLineSize;
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    int32_t ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV21));
    ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::SURFACE_FORMAT));
    ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_EQ(ret, AVCS_ERR_UNSUPPORT);

    delete[] yData;
    delete[] uData;
    delete[] vData;
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_surface_data_06, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* yData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT];
    uint8_t* uData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    uint8_t* vData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    ASSERT_NE(yData, nullptr);
    ASSERT_NE(uData, nullptr);
    ASSERT_NE(vData, nullptr);
    uint8_t* scaleData[4] = {yData, uData, vData, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH, 0, 0, 0}; // 4
    SurfaceInfo surfaceInfo;
    surfaceInfo.surfaceStride = TEST_WIDTH;
    surfaceInfo.scaleData = scaleData;
    surfaceInfo.scaleLineSize = scaleLineSize;
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    int32_t ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    delete[] yData;
    delete[] uData;
    delete[] vData;
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_surface_data_07, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* yData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT];
    uint8_t* uData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    uint8_t* vData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    ASSERT_NE(yData, nullptr);
    ASSERT_NE(uData, nullptr);
    ASSERT_NE(vData, nullptr);
    uint8_t* scaleData[4] = {yData, uData, vData, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH, TEST_WIDTH / 2, TEST_WIDTH / 2, 0}; // 4, 2
    SurfaceInfo surfaceInfo;
    surfaceInfo.surfaceStride = TEST_WIDTH;
    surfaceInfo.scaleData = scaleData;
    surfaceInfo.scaleLineSize = scaleLineSize;
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    int32_t ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 0);
    ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::UNKNOWN));
    ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA1010102));
    ret = WriteSurfaceData(memory, surfaceInfo, format);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);

    delete[] yData;
    delete[] uData;
    delete[] vData;
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_buffer_data_01, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* scaleData[4] = {nullptr, nullptr, nullptr, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH, TEST_WIDTH / 2, TEST_WIDTH / 2, 0}; // 4, 2
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    int32_t ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_buffer_data_02, TestSize.Level1)
{
    std::shared_ptr<AVMemory> memory = nullptr;
    uint8_t* scaleData[4] = {nullptr, nullptr, nullptr, nullptr}; // 4
    int32_t scaleLineSize[4] = {0, 0, 0, 0}; // 4
    Format format;
    int32_t ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_EQ(AVCS_ERR_INVALID_VAL, ret);
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_buffer_data_03, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* yData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT];
    uint8_t* uData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    uint8_t* vData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    ASSERT_NE(yData, nullptr);
    ASSERT_NE(uData, nullptr);
    ASSERT_NE(vData, nullptr);
    uint8_t* scaleData[4] = {yData, uData, vData, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH, TEST_WIDTH / 2, TEST_WIDTH / 2, 0}; // 4, 2
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    int32_t ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    int32_t scaleLineSize2[4] = {0, 0, 0, 0}; // 4
    ret = WriteBufferData(memory, scaleData, scaleLineSize2, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    delete[] yData;
    delete[] uData;
    delete[] vData;
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_buffer_data_04, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* rgbData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT * 4]; // 4
    ASSERT_NE(rgbData, nullptr);
    uint8_t* scaleData[4] = {rgbData, nullptr, nullptr, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH * 4, 0, 0, 0}; // 4, 2
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    int32_t ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    int32_t scaleLineSize2[4] = {0, 0, 0, 0}; // 4
    ret = WriteBufferData(memory, scaleData, scaleLineSize2, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    delete[] rgbData;
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_buffer_data_05, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* yData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT];
    uint8_t* uData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    uint8_t* vData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    ASSERT_NE(yData, nullptr);
    ASSERT_NE(uData, nullptr);
    ASSERT_NE(vData, nullptr);
    uint8_t* scaleData[4] = {yData, uData, vData, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH, TEST_WIDTH / 2, TEST_WIDTH / 2, 0}; // 4, 2
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    int32_t ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV21));
    ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::SURFACE_FORMAT));
    ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_EQ(ret, AVCS_ERR_UNSUPPORT);

    delete[] yData;
    delete[] uData;
    delete[] vData;
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_buffer_data_06, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* yData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT];
    uint8_t* uData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    uint8_t* vData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    ASSERT_NE(yData, nullptr);
    ASSERT_NE(uData, nullptr);
    ASSERT_NE(vData, nullptr);
    uint8_t* scaleData[4] = {yData, uData, vData, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH, 0, 0, 0}; // 4
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    int32_t ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV21));
    ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_NO_MEMORY);

    delete[] yData;
    delete[] uData;
    delete[] vData;
}

HWTEST_F(CodecUtilsUnitTest, codec_utils_write_buffer_data_07, TestSize.Level1)
{
    constexpr int32_t bufferSize = TEST_WIDTH * TEST_HEIGHT * 4; // 4
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    ASSERT_NE(allocator, nullptr);
    auto buffer = MediaAVBuffer::CreateAVBuffer(allocator, bufferSize, 0);
    ASSERT_NE(buffer, nullptr);
    std::shared_ptr<AVMemory> memory = buffer->memory_;
    ASSERT_NE(memory, nullptr);
    uint8_t* yData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT];
    uint8_t* uData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    uint8_t* vData = new (std::nothrow) uint8_t[TEST_WIDTH * TEST_HEIGHT / 4]; // 4
    ASSERT_NE(yData, nullptr);
    ASSERT_NE(uData, nullptr);
    ASSERT_NE(vData, nullptr);
    uint8_t* scaleData[4] = {yData, uData, vData, nullptr}; // 4
    int32_t scaleLineSize[4] = {TEST_WIDTH, TEST_WIDTH / 2, TEST_WIDTH / 2, 0}; // 4, 2
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    int32_t ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 0);
    ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::UNKNOWN));
    ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);

    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA1010102));
    ret = WriteBufferData(memory, scaleData, scaleLineSize, format);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);

    delete[] yData;
    delete[] uData;
    delete[] vData;
}
} // namespace MediaAVCodec
} // namespace OHOS