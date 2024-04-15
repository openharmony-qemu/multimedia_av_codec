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
 *
 * Description: header of Type converter from framework to OMX
 */

#ifndef TYPE_CONVERTER_H
#define TYPE_CONVERTER_H

#include <cstdint>
#include "av_common.h"  // foundation/multimedia/av_codec/interfaces/inner_api/native/
#include "avcodec_info.h"
#include "media_description.h"
#include "OMX_IVCommon.h"  // third_party/openmax/api/1.1.2
#include "OMX_Video.h"
#include "OMX_VideoExt.h"
#include "codec_omx_ext.h"
#include "surface_type.h" // foundation/graphic/graphic_2d/interfaces/inner_api/surface/
#include "meta/video_types.h" // foundation/multimedia/histreamer/interface/inner_api/
#include "v3_0/codec_types.h"

namespace OHOS::MediaAVCodec {
struct PixelFmt {
    GraphicPixelFormat graphicFmt;
    VideoPixelFormat innerFmt;
    std::string strFmt;
};

class TypeConverter {
public:
    static std::optional<AVCodecType> HdiCodecTypeToInnerCodecType(OHOS::HDI::Codec::V3_0::CodecType type);
    // coding type
    static std::optional<OMX_VIDEO_CODINGTYPE> HdiRoleToOmxCodingType(OHOS::HDI::Codec::V3_0::AvCodecRole role);
    static std::string HdiRoleToMime(OHOS::HDI::Codec::V3_0::AvCodecRole role);
    // pixel format
    static std::optional<PixelFmt> GraphicFmtToFmt(GraphicPixelFormat format);
    static std::optional<PixelFmt> InnerFmtToFmt(VideoPixelFormat format);
    static std::optional<GraphicPixelFormat> InnerFmtToDisplayFmt(VideoPixelFormat format);
    static std::optional<VideoPixelFormat> DisplayFmtToInnerFmt(GraphicPixelFormat format);
    // rotate
    static std::optional<GraphicTransformType> InnerRotateToDisplayRotate(VideoRotation rotate);
    // color aspects
    static RangeType RangeFlagToOmxRangeType(bool isFullRange) { return isFullRange ? RANGE_FULL : RANGE_LIMITED; }
    static Primaries InnerPrimaryToOmxPrimary(ColorPrimary primary);
    static Transfer InnerTransferToOmxTransfer(TransferCharacteristic transfer);
    static MatrixCoeffs InnerMatrixToOmxMatrix(MatrixCoefficient matrix);
    // profile
    static std::optional<AVCProfile> OmxAvcProfileToInnerProfile(OMX_VIDEO_AVCPROFILETYPE profile);
    static std::optional<AVCLevel> OmxAvcLevelToInnerLevel(OMX_VIDEO_AVCLEVELTYPE level);
    static std::optional<HEVCProfile> OmxHevcProfileToInnerProfile(CodecHevcProfile profile);
    static std::optional<HEVCLevel> OmxHevcLevelToInnerLevel(CodecHevcLevel level);
    static std::optional<OMX_VIDEO_AVCPROFILETYPE> InnerAvcProfileToOmxProfile(AVCProfile profile);
    static std::optional<CodecHevcProfile> InnerHevcProfileToOmxProfile(HEVCProfile profile);
    // bitrate mode
    static std::optional<VideoEncodeBitrateMode> HdiBitrateModeToInnerMode(OHOS::HDI::Codec::V3_0::BitRateMode mode);
};
}
#endif // TYPE_CONVERTER_H
