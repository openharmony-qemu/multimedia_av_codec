/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "FfmpegUtils"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include "common/log.h"
#include "meta/mime_type.h"
#include "ffmpeg_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "HiStreamer" };
}

#define AV_CODEC_TIME_BASE (static_cast<int64_t>(1))
#define AV_CODEC_NSECOND AV_CODEC_TIME_BASE
#define AV_CODEC_USECOND (static_cast<int64_t>(1000) * AV_CODEC_NSECOND)
#define AV_CODEC_MSECOND (static_cast<int64_t>(1000) * AV_CODEC_USECOND)
#define AV_CODEC_SECOND (static_cast<int64_t>(1000) * AV_CODEC_MSECOND)

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
bool Mime2CodecId(const std::string &mime, AVCodecID &codecId)
{
    /* MIME to AVCodecID */
    static const std::unordered_map<std::string, AVCodecID> table = {
        {MimeType::AUDIO_MPEG, AV_CODEC_ID_MP3},
        {MimeType::AUDIO_AAC, AV_CODEC_ID_AAC},
        {MimeType::AUDIO_AMR_NB, AV_CODEC_ID_AMR_NB},
        {MimeType::AUDIO_AMR_WB, AV_CODEC_ID_AMR_WB},
        {MimeType::AUDIO_RAW, AV_CODEC_ID_PCM_U8},
        {MimeType::AUDIO_G711MU, AV_CODEC_ID_PCM_MULAW},
        {MimeType::VIDEO_MPEG4, AV_CODEC_ID_MPEG4},
        {MimeType::VIDEO_AVC, AV_CODEC_ID_H264},
        {MimeType::VIDEO_HEVC, AV_CODEC_ID_HEVC},
        {MimeType::IMAGE_JPG, AV_CODEC_ID_MJPEG},
        {MimeType::IMAGE_PNG, AV_CODEC_ID_PNG},
        {MimeType::IMAGE_BMP, AV_CODEC_ID_BMP},
        {MimeType::TIMED_METADATA, AV_CODEC_ID_FFMETADATA},
    };
    auto it = table.find(mime);
    if (it != table.end()) {
        codecId = it->second;
        return true;
    }
    return false;
}

bool Raw2CodecId(AudioSampleFormat sampleFormat, AVCodecID &codecId)
{
    static const std::unordered_map<AudioSampleFormat, AVCodecID> table = {
        {AudioSampleFormat::SAMPLE_U8, AV_CODEC_ID_PCM_U8},
        {AudioSampleFormat::SAMPLE_S16LE, AV_CODEC_ID_PCM_S16LE},
        {AudioSampleFormat::SAMPLE_S24LE, AV_CODEC_ID_PCM_S24LE},
        {AudioSampleFormat::SAMPLE_S32LE, AV_CODEC_ID_PCM_S32LE},
        {AudioSampleFormat::SAMPLE_F32LE, AV_CODEC_ID_PCM_F32LE},
    };
    auto it = table.find(sampleFormat);
    if (it != table.end()) {
        codecId = it->second;
        return true;
    }
    return false;
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
    MEDIA_LOG_D("Base: [" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "], time convert ["
        PUBLIC_LOG_D64 "]->[" PUBLIC_LOG_D64 "].", base.num, base.den, pts, out);
    return out;
}

int64_t ConvertPts(int64_t pts, int64_t startTime)
{
    int64_t inputPts;
    if (startTime >= 0 && (pts < INT64_MIN + startTime)) {
        inputPts = AV_NOPTS_VALUE;
        MEDIA_LOG_D("pts is anomalous, pts: " PUBLIC_LOG_D64, pts);
        return inputPts;
    } else if (startTime < 0 && (pts > INT64_MAX + startTime)) {
        inputPts = AV_NOPTS_VALUE;
        MEDIA_LOG_D("pts is anomalous, pts: " PUBLIC_LOG_D64, pts);
        return inputPts;
    }
    inputPts = pts - startTime;

    return inputPts;
}

int64_t ConvertTimeToFFmpeg(int64_t timestampNs, AVRational base)
{
    int64_t result;
    if (base.num == 0) {
        result = AV_NOPTS_VALUE;
    } else {
        AVRational bq = {1, AV_CODEC_SECOND};
        result = av_rescale_q(timestampNs, bq, base);
    }
    MEDIA_LOG_D("Base: [" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "], time convert ["
        PUBLIC_LOG_D64 "]->[" PUBLIC_LOG_D64 "].", base.num, base.den, timestampNs, result);
    return result;
}

int64_t ConvertTimeToFFmpegByUs(int64_t timestampUs, AVRational base)
{
    int64_t result;
    if (base.num == 0) {
        result = AV_NOPTS_VALUE;
    } else {
        AVRational bq = {1, AV_CODEC_MSECOND};
        result = av_rescale_q(timestampUs, bq, base);
    }
    MEDIA_LOG_D("Base: [" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "], time convert ["
        PUBLIC_LOG_D64 "]->[" PUBLIC_LOG_D64 "].", base.num, base.den, timestampUs, result);
    return result;
}

std::string_view ConvertFFmpegMediaTypeToString(AVMediaType mediaType)
{
    static const std::unordered_map<AVMediaType, std::string_view> table = {
        {AVMediaType::AVMEDIA_TYPE_VIDEO, "video"},
        {AVMediaType::AVMEDIA_TYPE_AUDIO, "audio"},
        {AVMediaType::AVMEDIA_TYPE_DATA, "data"},
        {AVMediaType::AVMEDIA_TYPE_SUBTITLE, "subtitle"},
        {AVMediaType::AVMEDIA_TYPE_ATTACHMENT, "attachment"},
        {AVMediaType::AVMEDIA_TYPE_TIMEDMETA, "timedmetadata"}
    };
    auto it = table.find(mediaType);
    if (it == table.end()) {
        return "unknow";
    }
    return it->second;
}

bool StartWith(const char* name, const char* chars)
{
    MEDIA_LOG_D("[" PUBLIC_LOG_S "] start with [" PUBLIC_LOG_S "].", name, chars);
    return !strncmp(name, chars, strlen(chars));
}

uint32_t ConvertFlagsFromFFmpeg(const AVPacket& pkt, bool memoryNotEnough)
{
    uint32_t flags = (uint32_t)(AVBufferFlag::NONE);
    if (static_cast<uint32_t>(pkt.flags) & static_cast<uint32_t>(AV_PKT_FLAG_KEY)) {
        flags |= (uint32_t)(AVBufferFlag::SYNC_FRAME);
        flags |= (uint32_t)(AVBufferFlag::CODEC_DATA);
    }
    if (static_cast<uint32_t>(pkt.flags) & static_cast<uint32_t>(AV_PKT_FLAG_DISCARD)) {
        flags |= (uint32_t)(AVBufferFlag::DISCARD);
    }
    if (memoryNotEnough) {
        flags |= (uint32_t)(AVBufferFlag::PARTIAL_FRAME);
    }
    return flags;
}

int64_t CalculateTimeByFrameIndex(AVStream* avStream, int keyFrameIdx)
{
    FALSE_RETURN_V_MSG_E(avStream != nullptr, 0, "Track is nullptr.");
#if defined(LIBAVFORMAT_VERSION_INT) && defined(AV_VERSION_INT)
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(58, 78, 0) // 58 and 78 are avformat version range
    FALSE_RETURN_V_MSG_E(avformat_index_get_entry(avStream, keyFrameIdx) != nullptr, 0, "Track is nullptr.");
    return avformat_index_get_entry(avStream, keyFrameIdx)->timestamp;
#elif LIBAVFORMAT_VERSION_INT == AV_VERSION_INT(58, 76, 100) // 58, 76 and 100 are avformat version range
    return avStream->index_entries[keyFrameIdx].timestamp;
#elif LIBAVFORMAT_VERSION_INT > AV_VERSION_INT(58, 64, 100) // 58, 64 and 100 are avformat version range
    FALSE_RETURN_V_MSG_E(avStream->internal != nullptr, 0, "Track is nullptr.");
    return avStream->internal->index_entries[keyFrameIdx].timestamp;
#else
    return avStream->index_entries[keyFrameIdx].timestamp;
#endif
#else
    return avStream->index_entries[keyFrameIdx].timestamp;
#endif
}

void ReplaceDelimiter(const std::string &delimiters, char newDelimiter, std::string &str)
{
    for (char &it : str) {
        if (delimiters.find(newDelimiter) != std::string::npos) {
            it = newDelimiter;
        }
    }
}

std::vector<std::string> SplitString(const char* str, char delimiter)
{
    std::vector<std::string> rtv;
    if (str) {
        SplitString(std::string(str), delimiter).swap(rtv);
    }
    return rtv;
}

std::vector<std::string> SplitString(const std::string& str, char delimiter)
{
    if (str.empty()) {
        return {};
    }
    std::vector<std::string> rtv;
    std::string::size_type startPos = 0;
    std::string::size_type endPos = str.find_first_of(delimiter, startPos);
    while (startPos != endPos) {
        rtv.emplace_back(str.substr(startPos, endPos - startPos));
        if (endPos == std::string::npos) {
            break;
        }
        startPos = endPos + 1;
        endPos = str.find_first_of(delimiter, startPos);
    }
    return rtv;
}

std::pair<bool, AVColorPrimaries> ColorPrimary2AVColorPrimaries(ColorPrimary primary)
{
    static const std::unordered_map<ColorPrimary, AVColorPrimaries> table = {
        {ColorPrimary::BT709, AVCOL_PRI_BT709},
        {ColorPrimary::UNSPECIFIED, AVCOL_PRI_UNSPECIFIED},
        {ColorPrimary::BT470_M, AVCOL_PRI_BT470M},
        {ColorPrimary::BT601_625, AVCOL_PRI_BT470BG},
        {ColorPrimary::BT601_525, AVCOL_PRI_SMPTE170M},
        {ColorPrimary::SMPTE_ST240, AVCOL_PRI_SMPTE240M},
        {ColorPrimary::GENERIC_FILM, AVCOL_PRI_FILM},
        {ColorPrimary::BT2020, AVCOL_PRI_BT2020},
        {ColorPrimary::SMPTE_ST428, AVCOL_PRI_SMPTE428},
        {ColorPrimary::P3DCI, AVCOL_PRI_SMPTE431},
        {ColorPrimary::P3D65, AVCOL_PRI_SMPTE432},
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
        {TransferCharacteristic::BT709, AVCOL_TRC_BT709},
        {TransferCharacteristic::UNSPECIFIED, AVCOL_TRC_UNSPECIFIED},
        {TransferCharacteristic::GAMMA_2_2, AVCOL_TRC_GAMMA22},
        {TransferCharacteristic::GAMMA_2_8, AVCOL_TRC_GAMMA28},
        {TransferCharacteristic::BT601, AVCOL_TRC_SMPTE170M},
        {TransferCharacteristic::SMPTE_ST240, AVCOL_TRC_SMPTE240M},
        {TransferCharacteristic::LINEAR, AVCOL_TRC_LINEAR},
        {TransferCharacteristic::LOG, AVCOL_TRC_LOG},
        {TransferCharacteristic::LOG_SQRT, AVCOL_TRC_LOG_SQRT},
        {TransferCharacteristic::IEC_61966_2_4, AVCOL_TRC_IEC61966_2_4},
        {TransferCharacteristic::BT1361, AVCOL_TRC_BT1361_ECG},
        {TransferCharacteristic::IEC_61966_2_1, AVCOL_TRC_IEC61966_2_1},
        {TransferCharacteristic::BT2020_10BIT, AVCOL_TRC_BT2020_10},
        {TransferCharacteristic::BT2020_12BIT, AVCOL_TRC_BT2020_12},
        {TransferCharacteristic::PQ, AVCOL_TRC_SMPTE2084},
        {TransferCharacteristic::SMPTE_ST428, AVCOL_TRC_SMPTE428},
        {TransferCharacteristic::HLG, AVCOL_TRC_ARIB_STD_B67},
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
        {MatrixCoefficient::IDENTITY, AVCOL_SPC_RGB},
        {MatrixCoefficient::BT709, AVCOL_SPC_BT709},
        {MatrixCoefficient::UNSPECIFIED, AVCOL_SPC_UNSPECIFIED},
        {MatrixCoefficient::FCC, AVCOL_SPC_FCC},
        {MatrixCoefficient::BT601_625, AVCOL_SPC_BT470BG},
        {MatrixCoefficient::BT601_525, AVCOL_SPC_SMPTE170M},
        {MatrixCoefficient::SMPTE_ST240, AVCOL_SPC_SMPTE240M},
        {MatrixCoefficient::YCGCO, AVCOL_SPC_YCGCO},
        {MatrixCoefficient::BT2020_NCL, AVCOL_SPC_BT2020_NCL},
        {MatrixCoefficient::BT2020_CL, AVCOL_SPC_BT2020_CL},
        {MatrixCoefficient::SMPTE_ST2085, AVCOL_SPC_SMPTE2085},
        {MatrixCoefficient::CHROMATICITY_NCL, AVCOL_SPC_CHROMA_DERIVED_NCL},
        {MatrixCoefficient::CHROMATICITY_CL, AVCOL_SPC_CHROMA_DERIVED_CL},
        {MatrixCoefficient::ICTCP, AVCOL_SPC_ICTCP},
    };
    auto it = table.find(matrix);
    if (it != table.end()) {
        return { true, it->second };
    }
    return { false, AVCOL_SPC_UNSPECIFIED };
}

std::vector<uint8_t> GenerateAACCodecConfig(int32_t profile, int32_t sampleRate, int32_t channels)
{
    const std::unordered_map<AACProfile, int32_t> profiles = {
        {AAC_PROFILE_LC, 1},
        {AAC_PROFILE_ELD, 38},
        {AAC_PROFILE_ERLC, 1},
        {AAC_PROFILE_HE, 4},
        {AAC_PROFILE_HE_V2, 28},
        {AAC_PROFILE_LD, 22},
        {AAC_PROFILE_MAIN, 0},
    };
    const std::unordered_map<int32_t, int32_t> sampleRates = {
        {96000, 0}, {88200, 1}, {64000, 2}, {48000, 3},
        {44100, 4}, {32000, 5}, {24000, 6}, {22050, 7},
        {16000, 8}, {12000, 9}, {11025, 10}, {8000,  11},
        {7350,  12},
    };
    int32_t profileVal = FF_PROFILE_AAC_LOW;
    auto it1 = profiles.find(static_cast<AACProfile>(profile));
    if (it1 != profiles.end()) {
        profileVal = it1->second;
    }
    int32_t sampleRateIndex = 0x10;
    auto it2 = sampleRates.find(sampleRate);
    if (it2 != sampleRates.end()) {
        sampleRateIndex = it2->second;
    }
    std::vector<uint8_t> codecConfig = {0, 0, 0x56, 0xE5, 0};
    codecConfig[0] = ((profileVal + 1) << 0x03) | ((sampleRateIndex & 0x0F) >> 0x01);
    codecConfig[1] = ((sampleRateIndex & 0x01) << 0x07) | ((channels & 0x0F) << 0x03);
    return codecConfig;
}

uint32_t TimeStampUs2FrameId(int64_t timeUs, double fps)
{
    uint32_t us2Sec = MS_TO_SEC * MS_TO_SEC;
    return (timeUs * fps + us2Sec / 2) / us2Sec;  // 2
}

void FfmpegLogPrint(void* avcl, int level, const char* fmt, va_list vl)
{
    (void)avcl;
    char buf[500] = {0}; // 500
    int ret = vsnprintf_s(buf, sizeof(buf), sizeof(buf), fmt, vl);
    if (ret < 0) {
        return;
    }
    switch (level) {
        case AV_LOG_WARNING:
            MEDIA_LOG_D("[FFmpeg Log " PUBLIC_LOG_D32 " WARN] " PUBLIC_LOG_S, level, buf);
            break;
        case AV_LOG_ERROR:
            MEDIA_LOG_D("[FFmpeg Log " PUBLIC_LOG_D32 " ERROR] " PUBLIC_LOG_S, level, buf);
            break;
        case AV_LOG_FATAL:
            MEDIA_LOG_D("[FFmpeg Log " PUBLIC_LOG_D32 " FATAL] " PUBLIC_LOG_S, level, buf);
            break;
        case AV_LOG_INFO:
            MEDIA_LOG_D("[FFmpeg Log " PUBLIC_LOG_D32 " INFO] " PUBLIC_LOG_S, level, buf);
            break;
        case AV_LOG_DEBUG:
            MEDIA_LOG_D("[FFmpeg Log " PUBLIC_LOG_D32 " DEBUG] " PUBLIC_LOG_S, level, buf);
            break;
        default:
            break;
    }
}

int FindNaluSpliter(int size, const uint8_t* data)
{
    int naluPos = -1;
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00) { // 4: least size
        if (data[2] == 0x01) { // 2: next index
            naluPos = 3; // 3: the actual start pos of nal unit
        } else if (size >= 5 && data[2] == 0x00 && data[3] == 0x01) { // 5: least size, 2, 3: next indecies
            naluPos = 4; // 4: the actual start pos of nal unit
        }
    }
    return naluPos;
}

bool CanDropAvcPkt(const AVPacket& pkt)
{
    const uint8_t *data = pkt.data;
    int size = pkt.size;
    int naluPos = FindNaluSpliter(size, data);
    if (naluPos < 0) {
        MEDIA_LOG_D("pkt->data starts with error start code!");
        return false;
    }
    int nalRefIdc = (data[naluPos] >> 5) & 0x03; // 5: get H.264 nal_ref_idc
    int nalUnitType = data[naluPos] & 0x1f; // get H.264 nal_unit_type
    bool isCodedSliceData = nalUnitType == 1 || nalUnitType == 2 || // 1: non-IDR, 2: partiton A
        nalUnitType == 3 || nalUnitType == 4 || nalUnitType == 5; // 3: partiton B, 4: partiton C, 5: IDR
    return nalRefIdc == 0 && isCodedSliceData;
}

bool CanDropHevcPkt(const AVPacket& pkt)
{
    const uint8_t *data = pkt.data;
    int size = pkt.size;
    int naluPos = FindNaluSpliter(size, data);
    if (naluPos < 0) {
        MEDIA_LOG_D("pkt->data starts with error start code!");
        return false;
    }
    int nalUnitType = (data[naluPos] >> 1) & 0x3f; // get H.265 nal_unit_type
    return nalUnitType == 0 || nalUnitType == 2 || nalUnitType == 4 || // 0: TRAIL_N, 2: TSA_N, 4: STSA_N
        nalUnitType == 6 || nalUnitType == 8; // 6: RADL_N, 8: RASL_N
}

void SetDropTag(const AVPacket& pkt, std::shared_ptr<AVBuffer> sample, AVCodecID codecId)
{
    sample->meta_->Remove(Media::Tag::VIDEO_BUFFER_CAN_DROP);
    bool canDrop = false;
    if (codecId == AV_CODEC_ID_HEVC) {
        canDrop = CanDropHevcPkt(pkt);
    } else if (codecId == AV_CODEC_ID_H264) {
        canDrop = CanDropAvcPkt(pkt);
    }
    if (canDrop) {
        sample->meta_->SetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, true);
    }
}

bool IsInputFormatSupported(const char* name)
{
    MEDIA_LOG_D("Check support " PUBLIC_LOG_S " or not.", name);
    if (!strcmp(name, "audio_device") || StartWith(name, "image") ||
        !strcmp(name, "mjpeg") || !strcmp(name, "redir") || StartWith(name, "u8") ||
        StartWith(name, "u16") || StartWith(name, "u24") ||
        StartWith(name, "u32") ||
        StartWith(name, "s8") || StartWith(name, "s16") ||
        StartWith(name, "s24") ||
        StartWith(name, "s32") || StartWith(name, "f32") ||
        StartWith(name, "f64") ||
        !strcmp(name, "mulaw") || !strcmp(name, "alaw")) {
        return false;
    }
    if (!strcmp(name, "sdp") || !strcmp(name, "rtsp") || !strcmp(name, "applehttp")) {
        return false;
    }
    return true;
}

int64_t AvTime2Us(int64_t hTime)
{
    return hTime / AV_CODEC_USECOND;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
