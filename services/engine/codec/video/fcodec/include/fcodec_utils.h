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

#ifndef FCODEC_UTILS_H
#define FCODEC_UTILS_H

#include "v2_0/cm_color_space.h"
#include "v2_0/buffer_handle_meta_key_type.h"
#include "meta/video_types.h"
#include "codec_utils.h"

namespace {
using namespace OHOS::HDI::Display::Graphic::Common::V2_0;
using ColorPrimary = OHOS::Media::Plugins::ColorPrimary;
using TransferCharacteristic = OHOS::Media::Plugins::TransferCharacteristic;
using MatrixCoefficient = OHOS::Media::Plugins::MatrixCoefficient;

enum CM_ColorPrimaries AVColorPrimariesToCM(enum AVColorPrimaries primaries)
{
    switch (primaries) {
        case AVCOL_PRI_BT709:
            return COLORPRIMARIES_BT709;

        case AVCOL_PRI_BT470M:
            return COLORPRIMARIES_BT601_N;

        case AVCOL_PRI_BT470BG:
            return COLORPRIMARIES_BT601_P;

        case AVCOL_PRI_SMPTE170M:
            return COLORPRIMARIES_BT601_N;

        case AVCOL_PRI_SMPTE240M:
            return COLORPRIMARIES_BT709;

        case AVCOL_PRI_BT2020:
            return COLORPRIMARIES_BT2020;

        case AVCOL_PRI_SMPTE431:
            return COLORPRIMARIES_P3_DCI;

        case AVCOL_PRI_SMPTE432:
            return COLORPRIMARIES_P3_D65;

        case AVCOL_PRI_EBU3213:
            return COLORPRIMARIES_MONO;

        case AVCOL_PRI_UNSPECIFIED:
        case AVCOL_PRI_RESERVED0:
        case AVCOL_PRI_SMPTE428:
        case AVCOL_PRI_FILM:
        case AVCOL_PRI_RESERVED:
        default:
            return COLORPRIMARIES_BT709;
    }
}

CM_TransFunc AVColorTransferCharacteristicToCM(AVColorTransferCharacteristic transfer)
{
    switch (transfer) {
        case AVCOL_TRC_BT709:
            return TRANSFUNC_BT709;

        case AVCOL_TRC_LINEAR:
            return TRANSFUNC_LINEAR;

        case AVCOL_TRC_SMPTE240M: // BT709-based gamma transfer
            return TRANSFUNC_BT709;

        case AVCOL_TRC_GAMMA22:
            return TRANSFUNC_GAMMA2_2;

        case AVCOL_TRC_GAMMA28:
            return TRANSFUNC_GAMMA2_4;

        case AVCOL_TRC_IEC61966_2_1:
            return TRANSFUNC_SRGB;

        case AVCOL_TRC_BT2020_10:
        case AVCOL_TRC_BT2020_12:
            return TRANSFUNC_PQ;

        case AVCOL_TRC_ARIB_STD_B67:
            return TRANSFUNC_HLG;

        case AVCOL_TRC_UNSPECIFIED:
        case AVCOL_TRC_RESERVED0:
        case AVCOL_TRC_RESERVED:
        case AVCOL_TRC_SMPTE170M:
        case AVCOL_TRC_LOG:
        case AVCOL_TRC_LOG_SQRT:
        case AVCOL_TRC_BT1361_ECG:
        case AVCOL_TRC_IEC61966_2_4:
        case AVCOL_TRC_SMPTE2084:
        case AVCOL_TRC_SMPTE428:
        default:
            return TRANSFUNC_BT709;
    }
}

CM_Matrix AVColorSpaceToMatrix(AVColorSpace space)
{
    switch (space) {
        case AVCOL_SPC_RGB:
            return MATRIX_ADOBERGB;

        case AVCOL_SPC_BT709:
            return MATRIX_BT709;

        case AVCOL_SPC_BT470BG:
        case AVCOL_SPC_FCC:
        case AVCOL_SPC_SMPTE170M:
            return MATRIX_BT601_N;

        case AVCOL_SPC_BT2020_NCL:
        case AVCOL_SPC_BT2020_CL:
            return MATRIX_BT2020;

        case AVCOL_SPC_ICTCP:
            return MATRIX_BT2100_ICTCP;

        case AVCOL_SPC_UNSPECIFIED:
        case AVCOL_SPC_RESERVED:
        case AVCOL_SPC_SMPTE240M:
        case AVCOL_SPC_YCGCO:
        case AVCOL_SPC_SMPTE2085:
        case AVCOL_SPC_CHROMA_DERIVED_NCL:
        case AVCOL_SPC_CHROMA_DERIVED_CL:
        case AVCOL_SPC_IPT_C2:
        case AVCOL_SPC_YCGCO_RE:
        case AVCOL_SPC_YCGCO_RO:
        default:
            return MATRIX_BT709;
    }
}

CM_Range AVColorRangeToRange(AVColorRange range)
{
    switch (range) {
        case AVCOL_RANGE_MPEG:
            return RANGE_LIMITED;

        case AVCOL_RANGE_JPEG:
            return RANGE_FULL;

        default:
            return RANGE_LIMITED;
    }
}

ColorPrimary CMColorPrimariesToColorPrimary(CM_ColorPrimaries primaries)
{
    switch (primaries) {
        case CM_ColorPrimaries::COLORPRIMARIES_BT709:
            return ColorPrimary::BT709;
        
        case CM_ColorPrimaries::COLORPRIMARIES_BT601_P:
            return ColorPrimary::BT601_625;
        
        case CM_ColorPrimaries::COLORPRIMARIES_BT601_N:
            return ColorPrimary::BT601_525;
        
        case CM_ColorPrimaries::COLORPRIMARIES_BT2020:
            return ColorPrimary::BT2020;
        
        case CM_ColorPrimaries::COLORPRIMARIES_P3_DCI:
            return ColorPrimary::P3DCI;
        
        case CM_ColorPrimaries::COLORPRIMARIES_P3_D65:
            return ColorPrimary::P3D65;
        
        default:
            return ColorPrimary::UNSPECIFIED;
    }
}

TransferCharacteristic CMTransFuncToTransferCharacteristic(CM_TransFunc trans_func)
{
    switch (trans_func) {
        case CM_TransFunc::TRANSFUNC_BT709:
            return TransferCharacteristic::BT709;
        
        case CM_TransFunc::TRANSFUNC_SRGB:
            return TransferCharacteristic::IEC_61966_2_1;
        
        case CM_TransFunc::TRANSFUNC_LINEAR:
            return TransferCharacteristic::LINEAR;
        
        case CM_TransFunc::TRANSFUNC_PQ:
            return TransferCharacteristic::PQ;
        
        case CM_TransFunc::TRANSFUNC_HLG:
            return TransferCharacteristic::HLG;
        
        case CM_TransFunc::TRANSFUNC_GAMMA2_2:
            return TransferCharacteristic::GAMMA_2_2;
        
        case CM_TransFunc::TRANSFUNC_ADOBERGB:
        case CM_TransFunc::TRANSFUNC_GAMMA2_4:
        default:
            return TransferCharacteristic::UNSPECIFIED;
    }
}

MatrixCoefficient CMMatrixToMatrixCoefficient(CM_Matrix matrix)
{
    switch (matrix) {
        case CM_Matrix::MATRIX_ADOBERGB:
            return MatrixCoefficient::IDENTITY;
        
        case CM_Matrix::MATRIX_BT709:
            return MatrixCoefficient::BT709;
        
        case CM_Matrix::MATRIX_BT601_P:
            return MatrixCoefficient::FCC;
        
        case CM_Matrix::MATRIX_BT601_N:
            return MatrixCoefficient::BT601_525;
        
        case CM_Matrix::MATRIX_BT2020:
            return MatrixCoefficient::BT2020_NCL;
        
        case CM_Matrix::MATRIX_BT2100_ICTCP:
            return MatrixCoefficient::ICTCP;
        
        default:
            return MatrixCoefficient::UNSPECIFIED;
    }
}

int32_t CMRangeToBoolean(CM_Range range)
{
    switch (range) {
        case CM_Range::RANGE_LIMITED:
            return 0;

        case CM_Range::RANGE_FULL:
            return 1;

        default:
            return 0;
    }
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
static bool IsValidRawVideoPixelFormat(int32_t val)
{
    return val >= static_cast<int32_t>(RawVideoPixelFormat::YUV420P) &&
           val <= static_cast<int32_t>(RawVideoPixelFormat::RGBA);
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FCODEC_UTILS_H
