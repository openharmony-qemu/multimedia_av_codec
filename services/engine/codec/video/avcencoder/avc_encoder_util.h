/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef AVC_ENCODER_UTIL_H
#define AVC_ENCODER_UTIL_H

#include "AvcEnc_Typedef.h"
#include "surface.h"
#include "avcodec_common.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "avc_encoder_convert.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {

enum H264Level : int32_t {
    H264_LEVEL_10 = 10,
    H264_LEVEL_1B = 9,
    H264_LEVEL_11 = 11,
    H264_LEVEL_12 = 12,
    H264_LEVEL_13 = 13,
    H264_LEVEL_20 = 20,
    H264_LEVEL_21 = 21,
    H264_LEVEL_22 = 22,
    H264_LEVEL_30 = 30,
    H264_LEVEL_31 = 31,
    H264_LEVEL_32 = 32,
    H264_LEVEL_40 = 40,
    H264_LEVEL_41 = 41,
    H264_LEVEL_42 = 42,
    H264_LEVEL_50 = 50,
    H264_LEVEL_51 = 51,
    H264_LEVEL_52 = 52,
    H264_LEVEL_60 = 60,
    H264_LEVEL_61 = 61,
    H264_LEVEL_62 = 62,
};

AVCodecBufferFlag AvcFrameTypeToBufferFlag(uint32_t frameType);
VideoPixelFormat TranslateVideoPixelFormat(GraphicPixelFormat surfaceFormat);
GraphicPixelFormat TranslateSurfacePixFormat(const VideoPixelFormat &pixelFormat);
COLOR_FORMAT TranslateVideoFormatToAvc(const VideoPixelFormat &pixelFormat);
COLOR_MATRIX TranslateMatrix(MatrixCoefficient matrixFormat);
COLOR_RANGE TranslateRange(uint8_t range);
ENC_MODE TranslateEncMode(VideoEncodeBitrateMode mode);
ENC_PROFILE TranslateEncProfile(AVCProfile profile);
H264Level TranslateEncLevel(AVCLevel level);

} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVC_ENCODER_UTIL_H