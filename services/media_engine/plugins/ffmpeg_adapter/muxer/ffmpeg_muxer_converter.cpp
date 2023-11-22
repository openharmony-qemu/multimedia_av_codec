/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "ffmpeg_muxer_converter.h"
#include <algorithm>
#include <functional>
#include <unordered_map>
#include "meta/mime_type.h"

#define AV_CODEC_TIME_BASE (static_cast<int64_t>(1))
#define AV_CODEC_NSECOND AV_CODEC_TIME_BASE
#define AV_CODEC_USECOND (static_cast<int64_t>(1000) * AV_CODEC_NSECOND)
#define AV_CODEC_MSECOND (static_cast<int64_t>(1000) * AV_CODEC_USECOND)
#define AV_CODEC_SECOND (static_cast<int64_t>(1000) * AV_CODEC_MSECOND)

namespace OHOS {
namespace Media {
namespace Plugin {
namespace Ffmpeg {
bool Mime2CodecId(const std::string &mime, AVCodecID &codecId)
{
    /* MIME to AVCodecID */
    static const std::unordered_map<std::string, AVCodecID> table = {
        {MimeType::AUDIO_MPEG, AV_CODEC_ID_MP3},
        {MimeType::AUDIO_AAC, AV_CODEC_ID_AAC},
        {MimeType::VIDEO_MPEG4, AV_CODEC_ID_MPEG4},
        {MimeType::VIDEO_AVC, AV_CODEC_ID_H264},
        {MimeType::VIDEO_HEVC, AV_CODEC_ID_HEVC},
        {MimeType::IMAGE_JPG, AV_CODEC_ID_MJPEG},
        {MimeType::IMAGE_PNG, AV_CODEC_ID_PNG},
        {MimeType::IMAGE_BMP, AV_CODEC_ID_BMP},
    };
    auto it = table.find(mime);
    if (it != table.end()) {
        codecId = it->second;
        return true;
    }
    return false;
}

std::pair<bool, AVColorPrimaries> ColorPrimary2AVColorPrimaries(ColorPrimary primary)
{
    static const std::unordered_map<ColorPrimary, AVColorPrimaries> table = {
        {ColorPrimary::COLOR_PRIMARY_BT709, AVCOL_PRI_BT709},
        {ColorPrimary::COLOR_PRIMARY_UNSPECIFIED, AVCOL_PRI_UNSPECIFIED},
        {ColorPrimary::COLOR_PRIMARY_BT470_M, AVCOL_PRI_BT470M},
        {ColorPrimary::COLOR_PRIMARY_BT601_625, AVCOL_PRI_BT470BG},
        {ColorPrimary::COLOR_PRIMARY_BT601_525, AVCOL_PRI_SMPTE170M},
        {ColorPrimary::COLOR_PRIMARY_SMPTE_ST240, AVCOL_PRI_SMPTE240M},
        {ColorPrimary::COLOR_PRIMARY_GENERIC_FILM, AVCOL_PRI_FILM},
        {ColorPrimary::COLOR_PRIMARY_BT2020, AVCOL_PRI_BT2020},
        {ColorPrimary::COLOR_PRIMARY_SMPTE_ST428, AVCOL_PRI_SMPTE428},
        {ColorPrimary::COLOR_PRIMARY_P3DCI, AVCOL_PRI_SMPTE431},
        {ColorPrimary::COLOR_PRIMARY_P3D65, AVCOL_PRI_SMPTE432},
    };
    auto it = table.find(primary);
    if (it != table.end()) {
        return { true, it->second };
    }
    return { false, AVCOL_PRI_UNSPECIFIED };
}

std::pair<bool, AVColorTransferCharacteristic> ColorTransfer2AVColorTransfer(TransferCharacteristic transfer)
{
    static const std::unordered_map<TransferCharacteristic, AVColorTransferCharacteristic> table = {
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT709, AVCOL_TRC_BT709},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED, AVCOL_TRC_UNSPECIFIED},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_GAMMA_2_2, AVCOL_TRC_GAMMA22},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_GAMMA_2_8, AVCOL_TRC_GAMMA28},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT601, AVCOL_TRC_SMPTE170M},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_SMPTE_ST240, AVCOL_TRC_SMPTE240M},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_LINEAR, AVCOL_TRC_LINEAR},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_LOG, AVCOL_TRC_LOG},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_LOG_SQRT, AVCOL_TRC_LOG_SQRT},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_IEC_61966_2_4, AVCOL_TRC_IEC61966_2_4},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT1361, AVCOL_TRC_BT1361_ECG},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_IEC_61966_2_1, AVCOL_TRC_IEC61966_2_1},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT2020_10BIT, AVCOL_TRC_BT2020_10},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT2020_12BIT, AVCOL_TRC_BT2020_12},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_PQ, AVCOL_TRC_SMPTE2084},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_SMPTE_ST428, AVCOL_TRC_SMPTE428},
        {TransferCharacteristic::TRANSFER_CHARACTERISTIC_HLG, AVCOL_TRC_ARIB_STD_B67},
    };
    auto it = table.find(transfer);
    if (it != table.end()) {
        return { true, it->second };
    }
    return { false, AVCOL_TRC_UNSPECIFIED };
}

std::pair<bool, AVColorSpace> ColorMatrix2AVColorSpace(MatrixCoefficient matrix)
{
    static const std::unordered_map<MatrixCoefficient, AVColorSpace> table = {
        {MatrixCoefficient::MATRIX_COEFFICIENT_IDENTITY, AVCOL_SPC_RGB},
        {MatrixCoefficient::MATRIX_COEFFICIENT_BT709, AVCOL_SPC_BT709},
        {MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED, AVCOL_SPC_UNSPECIFIED},
        {MatrixCoefficient::MATRIX_COEFFICIENT_FCC, AVCOL_SPC_FCC},
        {MatrixCoefficient::MATRIX_COEFFICIENT_BT601_625, AVCOL_SPC_BT470BG},
        {MatrixCoefficient::MATRIX_COEFFICIENT_BT601_525, AVCOL_SPC_SMPTE170M},
        {MatrixCoefficient::MATRIX_COEFFICIENT_SMPTE_ST240, AVCOL_SPC_SMPTE240M},
        {MatrixCoefficient::MATRIX_COEFFICIENT_YCGCO, AVCOL_SPC_YCGCO},
        {MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_NCL, AVCOL_SPC_BT2020_NCL},
        {MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_CL, AVCOL_SPC_BT2020_CL},
        {MatrixCoefficient::MATRIX_COEFFICIENT_SMPTE_ST2085, AVCOL_SPC_SMPTE2085},
        {MatrixCoefficient::MATRIX_COEFFICIENT_CHROMATICITY_NCL, AVCOL_SPC_CHROMA_DERIVED_NCL},
        {MatrixCoefficient::MATRIX_COEFFICIENT_CHROMATICITY_CL, AVCOL_SPC_CHROMA_DERIVED_CL},
        {MatrixCoefficient::MATRIX_COEFFICIENT_ICTCP, AVCOL_SPC_ICTCP},
    };
    auto it = table.find(matrix);
    if (it != table.end()) {
        return { true, it->second };
    }
    return { false, AVCOL_SPC_UNSPECIFIED };
}

void ReplaceDelimiter(const std::string &delimiters, char newDelimiter, std::string &str)
{
    for (char & it : str) {
        if (delimiters.find(newDelimiter) != std::string::npos) {
            it = newDelimiter;
        }
    }
}

std::string AVStrError(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}

int64_t ConvertTimeFromFFmpeg(int64_t pts, AVRational base)
{
    int64_t out;
    if (pts == AV_NOPTS_VALUE) {
        out = -1;
    } else {
        AVRational bq = {1, AV_CODEC_SECOND};
        out = av_rescale_q(pts, base, bq);
    }
    return out;
}

int64_t ConvertTimeToFFmpeg(int64_t timestampUs, AVRational base)
{
    int64_t result;
    if (base.num == 0) {
        result = AV_NOPTS_VALUE;
    } else {
        AVRational bq = {1, AV_CODEC_SECOND};
        result = av_rescale_q(timestampUs, bq, base);
    }
    return result;
}
} // namespace Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS
