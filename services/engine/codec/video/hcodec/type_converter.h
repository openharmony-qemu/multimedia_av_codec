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
#include "meta/mime_type.h"
#include "codec_hdi.h"

namespace OHOS::MediaAVCodec {
struct PixelFmt {
    GraphicPixelFormat graphicFmt;
    VideoPixelFormat innerFmt;
    std::string strFmt;
};

class TypeConverter {
public:
    static std::optional<AVCodecType> HdiCodecTypeToInnerCodecType(CodecHDI::CodecType type);
    // coding type
    static std::optional<OMX_VIDEO_CODINGTYPE> HdiRoleToOmxCodingType(CodecHDI::AvCodecRole role);
    static std::string HdiRoleToMime(CodecHDI::AvCodecRole role);
    // pixel format
    static std::optional<PixelFmt> GraphicFmtToFmt(GraphicPixelFormat format);
    static std::optional<PixelFmt> InnerFmtToFmt(VideoPixelFormat format);
    static std::optional<VideoPixelFormat> DisplayFmtToInnerFmt(GraphicPixelFormat format);
    // rotate
    static std::optional<GraphicTransformType> InnerRotateToDisplayRotate(VideoRotation rotate);
    // profile
    static std::optional<AVCProfile> OmxAvcProfileToInnerProfile(OMX_VIDEO_AVCPROFILETYPE profile);
    static std::optional<AVCLevel> OmxAvcLevelToInnerLevel(OMX_VIDEO_AVCLEVELTYPE level);
    static std::optional<HEVCProfile> OmxHevcProfileToInnerProfile(CodecHevcProfile profile);
    static std::optional<HEVCLevel> OmxHevcLevelToInnerLevel(CodecHevcLevel level);
    static std::optional<VVCProfile> OmxVvcProfileToInnerProfile(CodecVvcProfile profile);
    static std::optional<VVCLevel> OmxVvcLevelToInnerLevel(CodecVvcLevel level);
    static std::optional<OMX_VIDEO_AVCPROFILETYPE> InnerAvcProfileToOmxProfile(AVCProfile profile);
    static std::optional<CodecHevcProfile> InnerHevcProfileToOmxProfile(HEVCProfile profile);
    static std::optional<CodecVvcProfile> InnerVvcProfileToOmxProfile(VVCProfile profile);
    static std::optional<std::vector<int32_t>> InnerVvcMaxLevelToAllLevels(VVCLevel maxLevel);
    // bitrate mode
    static std::optional<OMX_VIDEO_CONTROLRATETYPE> InnerModeToOmxBitrateMode(VideoEncodeBitrateMode mode);
    static std::optional<VideoEncodeBitrateMode> OmxBitrateModeToInnerMode(OMX_VIDEO_CONTROLRATETYPE mode);
};
}
#endif // TYPE_CONVERTER_H
