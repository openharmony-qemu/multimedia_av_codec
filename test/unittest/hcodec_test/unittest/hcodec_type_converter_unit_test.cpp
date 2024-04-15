/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <string>
#include "gtest/gtest.h"
#include "type_converter.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace testing::ext;

HWTEST(HCodecTypeConverterUnitTest, hdi_codec_type_to_inner_codec_type_find, TestSize.Level1)
{
    optional<AVCodecType> ret = TypeConverter::HdiCodecTypeToInnerCodecType(OHOS::HDI::Codec::V3_0::VIDEO_DECODER);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), AVCODEC_TYPE_VIDEO_DECODER);
}

HWTEST(HCodecTypeConverterUnitTest, hdi_codec_type_to_inner_codec_type_not_find, TestSize.Level1)
{
    optional<AVCodecType> ret = TypeConverter::HdiCodecTypeToInnerCodecType(OHOS::HDI::Codec::V3_0::INVALID_TYPE);
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, hdi_role_to_omx_coding_type_find, TestSize.Level1)
{
    optional<OMX_VIDEO_CODINGTYPE> ret = TypeConverter::HdiRoleToOmxCodingType(
        OHOS::HDI::Codec::V3_0::MEDIA_ROLETYPE_VIDEO_AVC);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), OMX_VIDEO_CodingAVC);
}

HWTEST(HCodecTypeConverterUnitTest, hdi_role_to_omx_coding_type_not_find, TestSize.Level1)
{
    optional<OMX_VIDEO_CODINGTYPE> ret = TypeConverter::HdiRoleToOmxCodingType(
        OHOS::HDI::Codec::V3_0::MEDIA_ROLETYPE_INVALID);
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, hdi_role_to_mime_find, TestSize.Level1)
{
    string ret = TypeConverter::HdiRoleToMime(OHOS::HDI::Codec::V3_0::MEDIA_ROLETYPE_VIDEO_HEVC);
    EXPECT_EQ(ret, string(CodecMimeType::VIDEO_HEVC));
}

HWTEST(HCodecTypeConverterUnitTest, hdi_role_to_mime_not_find, TestSize.Level1)
{
    string ret = TypeConverter::HdiRoleToMime(OHOS::HDI::Codec::V3_0::MEDIA_ROLETYPE_INVALID);
    ASSERT_TRUE(ret.empty());
}

HWTEST(HCodecTypeConverterUnitTest, graphic_fmt_to_fmt_find, TestSize.Level1)
{
    optional<PixelFmt> ret = TypeConverter::GraphicFmtToFmt(GRAPHIC_PIXEL_FMT_YCBCR_420_P);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value().innerFmt, VideoPixelFormat::YUVI420);
}

HWTEST(HCodecTypeConverterUnitTest, graphic_fmt_to_fmt_not_find, TestSize.Level1)
{
    optional<PixelFmt> ret = TypeConverter::GraphicFmtToFmt(GRAPHIC_PIXEL_FMT_BUTT);
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, inner_fmt_to_fmt_find, TestSize.Level1)
{
    optional<PixelFmt> ret = TypeConverter::InnerFmtToFmt(VideoPixelFormat::NV12);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value().graphicFmt, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
}

HWTEST(HCodecTypeConverterUnitTest, inner_fmt_to_fmt_not_find, TestSize.Level1)
{
    optional<PixelFmt> ret = TypeConverter::InnerFmtToFmt(VideoPixelFormat::UNKNOWN);
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, inner_fmt_to_display_fmt_find, TestSize.Level1)
{
    optional<GraphicPixelFormat> ret = TypeConverter::InnerFmtToDisplayFmt(VideoPixelFormat::RGBA);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), GRAPHIC_PIXEL_FMT_RGBA_8888);
}

HWTEST(HCodecTypeConverterUnitTest, inner_fmt_to_display_fmt_not_find, TestSize.Level1)
{
    optional<GraphicPixelFormat> ret = TypeConverter::InnerFmtToDisplayFmt(VideoPixelFormat::UNKNOWN);
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, display_fmt_to_inner_fmt_find, TestSize.Level1)
{
    optional<VideoPixelFormat> ret = TypeConverter::DisplayFmtToInnerFmt(GRAPHIC_PIXEL_FMT_YCRCB_420_SP);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), VideoPixelFormat::NV21);
}

HWTEST(HCodecTypeConverterUnitTest, display_fmt_to_inner_fmt_not_find, TestSize.Level1)
{
    optional<VideoPixelFormat> ret = TypeConverter::DisplayFmtToInnerFmt(GRAPHIC_PIXEL_FMT_BUTT);
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, inner_rotate_to_display_rotate_find, TestSize.Level1)
{
    optional<GraphicTransformType> ret = TypeConverter::InnerRotateToDisplayRotate(VIDEO_ROTATION_270);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), GRAPHIC_ROTATE_90);
}

HWTEST(HCodecTypeConverterUnitTest, inner_rotate_to_display_rotate_not_find, TestSize.Level1)
{
    optional<GraphicTransformType> ret = TypeConverter::InnerRotateToDisplayRotate(static_cast<VideoRotation>(123));
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, range_flag_to_omx_range_type_full, TestSize.Level1)
{
    RangeType ret = TypeConverter::RangeFlagToOmxRangeType(true);
    EXPECT_EQ(ret, RANGE_FULL);
}

HWTEST(HCodecTypeConverterUnitTest, range_flag_to_omx_range_type_limited, TestSize.Level1)
{
    RangeType ret = TypeConverter::RangeFlagToOmxRangeType(false);
    EXPECT_EQ(ret, RANGE_LIMITED);
}

HWTEST(HCodecTypeConverterUnitTest, inner_primary_to_omx_primary_find, TestSize.Level1)
{
    Primaries ret = TypeConverter::InnerPrimaryToOmxPrimary(COLOR_PRIMARY_BT709);
    EXPECT_EQ(ret, PRIMARIES_BT709);
}

HWTEST(HCodecTypeConverterUnitTest, inner_primary_to_omx_primary_not_finddefault, TestSize.Level1)
{
    Primaries ret = TypeConverter::InnerPrimaryToOmxPrimary(static_cast<ColorPrimary>(100));
    EXPECT_EQ(ret, PRIMARIES_UNSPECIFIED);
}

HWTEST(HCodecTypeConverterUnitTest, inner_transfer_to_omx_transfer_find, TestSize.Level1)
{
    Transfer ret = TypeConverter::InnerTransferToOmxTransfer(TRANSFER_CHARACTERISTIC_GAMMA_2_2);
    EXPECT_EQ(ret, TRANSFER_GAMMA22);
}

HWTEST(HCodecTypeConverterUnitTest, inner_transfer_to_omx_transfer_default, TestSize.Level1)
{
    Transfer ret = TypeConverter::InnerTransferToOmxTransfer(static_cast<TransferCharacteristic>(100));
    EXPECT_EQ(ret, TRANSFER_UNSPECIFIED);
}

HWTEST(HCodecTypeConverterUnitTest, inner_matrix_to_omx_matrix_find, TestSize.Level1)
{
    MatrixCoeffs ret = TypeConverter::InnerMatrixToOmxMatrix(MATRIX_COEFFICIENT_BT2020_NCL);
    EXPECT_EQ(ret, MATRIX_BT2020);
}

HWTEST(HCodecTypeConverterUnitTest, inner_matrix_to_omx_matrix_default, TestSize.Level1)
{
    MatrixCoeffs ret = TypeConverter::InnerMatrixToOmxMatrix(static_cast<MatrixCoefficient>(100));
    EXPECT_EQ(ret, MATRIX_UNSPECIFED);
}

HWTEST(HCodecTypeConverterUnitTest, omx_avc_profile_to_inner_profile_find, TestSize.Level1)
{
    optional<AVCProfile> ret = TypeConverter::OmxAvcProfileToInnerProfile(OMX_VIDEO_AVCProfileBaseline);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), AVC_PROFILE_BASELINE);
}

HWTEST(HCodecTypeConverterUnitTest, omx_avc_profile_to_inner_profile_not_find, TestSize.Level1)
{
    optional<AVCProfile> ret = TypeConverter::OmxAvcProfileToInnerProfile(OMX_VIDEO_AVCProfileMax);
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, omx_avc_level_to_inner_level_find, TestSize.Level1)
{
    optional<AVCLevel> ret = TypeConverter::OmxAvcLevelToInnerLevel(OMX_VIDEO_AVCLevel1b);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), AVC_LEVEL_1b);
}

HWTEST(HCodecTypeConverterUnitTest, omx_avc_level_to_inner_level_not_find, TestSize.Level1)
{
    optional<AVCLevel> ret = TypeConverter::OmxAvcLevelToInnerLevel(OMX_VIDEO_AVCLevelMax);
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, omx_hevc_profile_to_inner_profile_find, TestSize.Level1)
{
    optional<HEVCProfile> ret = TypeConverter::OmxHevcProfileToInnerProfile(CODEC_HEVC_PROFILE_MAIN);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), HEVC_PROFILE_MAIN);
}

HWTEST(HCodecTypeConverterUnitTest, omx_hevc_profile_to_inner_profile_not_find, TestSize.Level1)
{
    optional<HEVCProfile> ret = TypeConverter::OmxHevcProfileToInnerProfile(CODEC_HEVC_PROFILE_MAX);
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, omx_hevc_level_to_inner_level_find, TestSize.Level1)
{
    optional<HEVCLevel> ret = TypeConverter::OmxHevcLevelToInnerLevel(CODEC_HEVC_HIGH_TIER_LEVEL3);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), HEVC_LEVEL_3);
}

HWTEST(HCodecTypeConverterUnitTest, omx_hevc_level_to_inner_level_not_find, TestSize.Level1)
{
    optional<HEVCLevel> ret = TypeConverter::OmxHevcLevelToInnerLevel(CODEC_HEVC_HIGH_TIER_MAX);
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, inner_avc_profile_to_omx_profile_find, TestSize.Level1)
{
    optional<OMX_VIDEO_AVCPROFILETYPE> ret = TypeConverter::InnerAvcProfileToOmxProfile(AVC_PROFILE_EXTENDED);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), OMX_VIDEO_AVCProfileExtended);
}

HWTEST(HCodecTypeConverterUnitTest, inner_avc_profile_to_omx_profile_not_find, TestSize.Level1)
{
    optional<OMX_VIDEO_AVCPROFILETYPE> ret = TypeConverter::InnerAvcProfileToOmxProfile(static_cast<AVCProfile>(100));
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, inner_hevc_profile_to_omx_profile_find, TestSize.Level1)
{
    optional<CodecHevcProfile> ret = TypeConverter::InnerHevcProfileToOmxProfile(HEVC_PROFILE_MAIN_10_HDR10);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), CODEC_HEVC_PROFILE_MAIN10_HDR10);
}

HWTEST(HCodecTypeConverterUnitTest, inner_hevc_profile_to_omx_profile_not_find, TestSize.Level1)
{
    optional<CodecHevcProfile> ret = TypeConverter::InnerHevcProfileToOmxProfile(static_cast<HEVCProfile>(100));
    ASSERT_FALSE(ret.has_value());
}

HWTEST(HCodecTypeConverterUnitTest, hdi_bitrate_mode_to_inner_mode_find, TestSize.Level1)
{
    optional<VideoEncodeBitrateMode> ret =
        TypeConverter::HdiBitrateModeToInnerMode(OHOS::HDI::Codec::V3_0::BIT_RATE_MODE_VBR);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), VBR);
}

HWTEST(HCodecTypeConverterUnitTest, hdi_bitrate_mode_to_inner_mode_not_find, TestSize.Level1)
{
    optional<VideoEncodeBitrateMode> ret =
        TypeConverter::HdiBitrateModeToInnerMode(OHOS::HDI::Codec::V3_0::BIT_RATE_MODE_INVALID);
    ASSERT_FALSE(ret.has_value());
}
} // OHOS::MediaAVCodec