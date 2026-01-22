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
constexpr uint32_t FLAC_CODEC_CONFIG_SIZE = 34;
}

#define AV_CODEC_TIME_BASE (static_cast<int64_t>(1))
#define AV_CODEC_NSECOND AV_CODEC_TIME_BASE
#define AV_CODEC_USECOND (static_cast<int64_t>(1000) * AV_CODEC_NSECOND)
#define AV_CODEC_MSECOND (static_cast<int64_t>(1000) * AV_CODEC_USECOND)
#define AV_CODEC_SECOND (static_cast<int64_t>(1000) * AV_CODEC_MSECOND)
const int32_t NAL_START_CODE_SIZE = 4;
const uint8_t START_CODE[] = {0x00, 0x00, 0x01};
// position of nal unit type
const int32_t POS_0 = 0;
const int32_t POS_1 = 1;
const int32_t POS_2 = 2;
const int32_t POS_3 = 3;
const int32_t POS_8 = 8;
const int32_t POS_16 = 16;
const int32_t POS_24 = 24;

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
        {MimeType::AUDIO_FLAC, AV_CODEC_ID_FLAC},
        {MimeType::AUDIO_OPUS, AV_CODEC_ID_OPUS},
        {MimeType::AUDIO_VORBIS, AV_CODEC_ID_VORBIS},
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

bool Raw2BitPerSample(AudioSampleFormat sampleFormat, uint8_t &bitPerSample)
{
    static const std::unordered_map<AudioSampleFormat, uint8_t> table = {
        {AudioSampleFormat::SAMPLE_U8, 8},
        {AudioSampleFormat::SAMPLE_S16LE, 16},
        {AudioSampleFormat::SAMPLE_S24LE, 24},
        {AudioSampleFormat::SAMPLE_S32LE, 32},
        {AudioSampleFormat::SAMPLE_F32LE, 32},
        {AudioSampleFormat::SAMPLE_U8P, 8},
        {AudioSampleFormat::SAMPLE_S16P, 16},
        {AudioSampleFormat::SAMPLE_S24P, 24},
        {AudioSampleFormat::SAMPLE_S32P, 32},
        {AudioSampleFormat::SAMPLE_F32P, 32},
    };
    auto it = table.find(sampleFormat);
    if (it != table.end()) {
        bitPerSample = it->second;
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

bool StartWith(const char* name, const char* chars)
{
    if (name == nullptr || chars == nullptr) {
        return false;
    }
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
    const std::unordered_map<uint32_t, uint32_t> sampleRates = {
        {96000, 0}, {88200, 1}, {64000, 2}, {48000, 3},
        {44100, 4}, {32000, 5}, {24000, 6}, {22050, 7},
        {16000, 8}, {12000, 9}, {11025, 10}, {8000,  11},
        {7350,  12},
    };
    uint32_t profileVal = FF_PROFILE_AAC_LOW;
    auto it1 = profiles.find(static_cast<AACProfile>(profile));
    if (it1 != profiles.end()) {
        profileVal = it1->second;
    }
    uint32_t sampleRateIndex = 0x10;
    uint32_t baseIndex = 0xF;
    auto it2 = sampleRates.find(sampleRate);
    if (it2 != sampleRates.end()) {
        sampleRateIndex = it2->second;
    }
    it2 = sampleRates.find(sampleRate / 2); // 2: HE-AAC require divide base sample rate
    if (it2 != sampleRates.end()) {
        baseIndex = it2->second;
    }
    std::vector<uint8_t> codecConfig;
    if (profile == AAC_PROFILE_HE || profile == AAC_PROFILE_HE_V2) {
        // HE-AAC v2 only support stereo and only one channel exist
        uint32_t realCh = (profile == AAC_PROFILE_HE_V2) ? 1 : static_cast<uint32_t>(channels);
        codecConfig = {0, 0, 0, 0, 0};
        // 5 bit AOT(0x03:left 3 bits for sample rate) + 4 bit sample rate idx(0x01: 4 - 0x03)
        codecConfig[0] = ((profileVal + 1) << 0x03) | ((baseIndex & 0x0F) >> 0x01);
        // 0x07: left 7bits for other, 4 bit channel cfg,0x03:left for other
        codecConfig[1] = ((baseIndex & 0x01) << 0x07) | ((realCh & 0x0F) << 0x03) | ((sampleRateIndex & 0x0F) >> 1) ;
        // 4 bit ext sample rate idx(0x07: left 7 bits for other) + 4 bit aot(2: LC-AAC, 0x02: left for other)
        codecConfig[2] = ((sampleRateIndex & 0x01) << 0x07) | (2 << 0x02);
    } else {
        codecConfig = {0, 0, 0x56, 0xE5, 0};
        codecConfig[0] = ((profileVal + 1) << 0x03) | ((sampleRateIndex & 0x0F) >> 0x01);
        codecConfig[1] = ((sampleRateIndex & 0x01) << 0x07) | ((static_cast<uint32_t>(channels) & 0x0F) << 0x03);
    }

    return codecConfig;
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

int64_t AvTime2Us(int64_t hTime)
{
    return hTime / AV_CODEC_USECOND;
}

bool FlacCodecConfig::GenerateCodecConfig(const std::shared_ptr<Meta> &trackDesc)
{
    constexpr uint32_t index10 = 10;
    constexpr uint32_t offset4 = 4;
    constexpr uint32_t offset12 = 12;
    constexpr int32_t maxChannel = 8;
    mCodecConfig.resize(FLAC_CODEC_CONFIG_SIZE, 0);
    AudioSampleFormat sampleFormat = INVALID_WIDTH;
    mSampleRate = 0;
    mChannels = 0;
    bool ret = trackDesc->Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
    trackDesc->Get<Tag::AUDIO_SAMPLE_RATE>(mSampleRate);
    trackDesc->Get<Tag::AUDIO_CHANNEL_COUNT>(mChannels);
    if (!ret || !Raw2BitPerSample(sampleFormat, mBitPerSample)) {
        MEDIA_LOG_E("not support codec_id: " PUBLIC_LOG_D32 ", ret: " PUBLIC_LOG_D32,
            static_cast<int32_t>(sampleFormat), static_cast<int32_t>(ret));
        return false;
    }
    if (mChannels < 1 || mChannels > maxChannel) {
        MEDIA_LOG_E("not support mChannels:" PUBLIC_LOG_D32, mChannels);
        return false;
    }
    uint32_t i = index10;
    // 10 - 12
    mCodecConfig.data()[i++] = static_cast<uint8_t>((static_cast<uint32_t>(mSampleRate) >> offset12) & 0XFF);
    mCodecConfig.data()[i++] = static_cast<uint8_t>((static_cast<uint32_t>(mSampleRate) >> offset4) & 0XFF);
    mCodecConfig.data()[i] = static_cast<uint8_t>((static_cast<uint32_t>(mSampleRate) << offset4) & 0XF0);
    mCodecConfig.data()[i] |= static_cast<uint8_t>((static_cast<uint32_t>(mChannels - 1) << 1) & 0x0E);
    mIsFirstDataFrame = true;
    return true;
}

bool FlacCodecConfig::Update()
{
    if (mIsUpdateExtraData) {
        return true;
    }
    constexpr uint32_t index12 = 12;
    constexpr uint32_t offset4 = 4;
    constexpr uint32_t offset8 = 8;
    constexpr uint32_t offset16 = 16;
    constexpr uint32_t offset24 = 24;
    constexpr uint32_t offset32 = 32;
    FALSE_RETURN_V_MSG_E(mCodecConfig.size() >= FLAC_CODEC_CONFIG_SIZE, false,
        "FlacCodecConfig::Update failed! mCodecConfig size not enough!");
    uint32_t i = 0;
    // block size
    mCodecConfig.data()[i++] = static_cast<uint8_t>((mBlockSize >> offset8) & 0xff);
    mCodecConfig.data()[i++] = static_cast<uint8_t>(mBlockSize & 0xff) ;
    mCodecConfig.data()[i++] = mCodecConfig.data()[0];
    mCodecConfig.data()[i++] = mCodecConfig.data()[1];
    // min max auido frame size
    mCodecConfig.data()[i++] |= static_cast<uint8_t>((mMinFrameSize >> offset16) & 0xff);  // 4
    mCodecConfig.data()[i++] |= static_cast<uint8_t>((mMinFrameSize >> offset8) & 0xff);  // 5
    mCodecConfig.data()[i++] |= static_cast<uint8_t>(mMinFrameSize & 0xff);  // 6
    mCodecConfig.data()[i++] |= static_cast<uint8_t>((mMaxFrameSize >> offset16) & 0xff);  // 7
    mCodecConfig.data()[i++] |= static_cast<uint8_t>((mMaxFrameSize >> offset8) & 0xff);  // 8
    mCodecConfig.data()[i++] |= static_cast<uint8_t>(mMaxFrameSize & 0xff);  // 9

    // total sample num
    i = index12;
    // 12
    mCodecConfig.data()[i++] |= static_cast<uint8_t>((static_cast<uint32_t>(mBitPerSample - 1) >> offset4) & 0x01);
    // 13
    mCodecConfig.data()[i] |= static_cast<uint8_t>((static_cast<uint32_t>(mBitPerSample - 1) << offset4) & 0xf0);
    mCodecConfig.data()[i++] |= static_cast<uint8_t>((mTotalSample >> offset32) & 0x0f);  // 13
    mCodecConfig.data()[i++] |= static_cast<uint8_t>((mTotalSample >> offset24) & 0xff);  // 14
    mCodecConfig.data()[i++] |= static_cast<uint8_t>((mTotalSample >> offset16) & 0xff);  // 15
    mCodecConfig.data()[i++] |= static_cast<uint8_t>((mTotalSample >> offset8) & 0xff);  // 16
    mCodecConfig.data()[i++] |= static_cast<uint8_t>(mTotalSample & 0xff);  // 17
    return true;
}

void FlacCodecConfig::UpdateNewConfig(uint8_t *data, size_t size)
{
    if (size != mCodecConfig.size() || size < FLAC_CODEC_CONFIG_SIZE || data == nullptr) {
        MEDIA_LOG_E("UpdateNewConfig failed! curSize:%{public}zu, size:%{public}zu", mCodecConfig.size(), size);
        return;
    }
    constexpr uint32_t startIndex = 10;
    constexpr uint32_t offset4 = 4;
    constexpr uint32_t offset12 = 12;
    int32_t tmpSampleRate = 0;
    int32_t tmpChannels = 0;
    uint8_t tmpBitPerSample = 0;
    uint32_t i = startIndex;
    // 10 - 13
    tmpSampleRate = static_cast<int32_t>((static_cast<uint32_t>(data[i++]) << offset12));
    tmpSampleRate += static_cast<int32_t>(static_cast<uint32_t>(data[i++]) << offset4);
    tmpSampleRate += static_cast<int32_t>(static_cast<uint32_t>(data[i] & 0xf0) >> offset4);
    tmpChannels = static_cast<int32_t>((static_cast<uint32_t>(data[i]) & 0x0e) >> 1);
    tmpChannels += 1;
    tmpBitPerSample = static_cast<uint8_t>(((static_cast<uint32_t>(data[i++]) & 0x01) << offset4));
    tmpBitPerSample |= static_cast<uint8_t>(static_cast<uint32_t>(data[i++] & 0xf0) >> offset4);
    tmpBitPerSample += 1;
    if (tmpSampleRate != mSampleRate || tmpChannels != mChannels || tmpBitPerSample != mBitPerSample) {
        MEDIA_LOG_E("flac muxer params change! SampleRate:" PUBLIC_LOG_D32 " to " PUBLIC_LOG_D32
            " mChannels:" PUBLIC_LOG_D32 " to " PUBLIC_LOG_D32 " mBitPerSample:" PUBLIC_LOG_U8 " to " PUBLIC_LOG_U8,
            mSampleRate, tmpSampleRate, mChannels, tmpChannels, mBitPerSample, tmpBitPerSample);
        mSampleRate = tmpSampleRate;
        mChannels = tmpChannels;
        mBitPerSample = tmpBitPerSample;
    }
    if (memcpy_s(mCodecConfig.data(), mCodecConfig.size(), data, size) != EOK) {
        MEDIA_LOG_E("flac UpdateNewConfig memcpy_s failed!");
        return;
    }
    mIsUpdateExtraData = true;
}

static uint32_t GetUtf8Bytes(uint8_t data)
{
    constexpr uint32_t num2 = 2;
    constexpr int32_t maxBit = 7;
    uint32_t bytes = 0;
    for (int32_t i = maxBit; i > 0; i--) {  // 7 - 1, 7bits
        if ((data & static_cast<uint8_t>(1 << i)) == 0) {
            break;
        }
        bytes++;
    }
    return bytes < num2 ? (bytes + 1) : bytes;
}

void FlacCodecConfig::UpdateBitPerSample(uint8_t byte)
{
    static const uint8_t flacBitDepthTable[8] = {0, 8, 12, 0, 16, 20, 24, 32};
    uint8_t index = static_cast<uint8_t>((byte >> 1) & 0x07);
    uint8_t bitPerSample = flacBitDepthTable[index];
    if (bitPerSample == 0 || bitPerSample == mBitPerSample || mCodecConfig.size() < FLAC_CODEC_CONFIG_SIZE) {
        return;
    }
    MEDIA_LOG_I("update bit per sample from " PUBLIC_LOG_U8 " to " PUBLIC_LOG_U8, mBitPerSample, bitPerSample);
    mBitPerSample = bitPerSample;
}

void FlacCodecConfig::UpdatePerFrame(uint8_t* data, size_t size)
{
    constexpr size_t minSize = 16;
    constexpr uint32_t offset4 = 4;
    constexpr uint32_t offset8 = 8;
    constexpr uint32_t index2 = 2;
    constexpr uint32_t index3 = 3;
    constexpr uint32_t index6 = 6;
    constexpr uint32_t index7 = 7;
    if (size < minSize || data == nullptr) {
        return;
    }
    if (data[0] != 0xff || (data[1] & 0xf8) != 0xf8) {  // flac frame head
        return;
    }
    uint32_t tempBlockSize = 0;
    static const uint16_t flacBlockList[minSize] = {
        0, 192, 576, 1152, 2304, 4608, 0, 0, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768
    };
    uint32_t index = static_cast<uint8_t>((data[index2] >> offset4) & 0x0f);
    if (index == index6) {
        uint32_t offset = offset4 + GetUtf8Bytes(data[offset4]);
        tempBlockSize = static_cast<uint32_t>(data[offset]) + 1;
    } else if (index == index7) {
        uint32_t offset = offset4 + GetUtf8Bytes(data[offset4]);
        tempBlockSize = static_cast<uint32_t>(data[offset] << offset8) +
            static_cast<uint32_t>(data[offset + 1]) + 1;
    } else {
        tempBlockSize = flacBlockList[index];
    }
    mTotalSample += tempBlockSize;
    if (tempBlockSize > mBlockSize) {
        mBlockSize = tempBlockSize;
    }
    if (mIsFirstDataFrame) {
        mMinFrameSize = size;
        mMaxFrameSize = size;
        mIsFirstDataFrame = false;
        UpdateBitPerSample(data[index3]);
    }
    if (size > mMaxFrameSize) {
        mMaxFrameSize = size;
    }
    if (size < mMinFrameSize) {
        mMinFrameSize = size;
    }
    mIsUpdateExtraData = false;
}

std::string hexEncode(const std::vector<uint8_t>& data)
{
    static const char hexDigits[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(data.size() * 2); // extend data size 2
    for (unsigned char c : data) {
        result.push_back(hexDigits[(c >> 4) & 0xF]); // shift right by 4
        result.push_back(hexDigits[c & 0xF]);
    }
    return result;
}

bool IsBeginAsAnnexb(const uint8_t *sample, int32_t size)
{
    if (size < NAL_START_CODE_SIZE) {
        return false;
    }
    bool hasShortStartCode = (sample[0] == 0 && sample[1] == 0 && sample[2] == 1); // 001
    bool hasLongStartCode = (sample[0] == 0 && sample[1] == 0 && sample[2] == 0 && sample[3] == 1); // 0001
    return hasShortStartCode || hasLongStartCode;
}

int32_t GetNaluSize(const uint8_t *nalStart)
{
    return static_cast<int32_t>(
        (nalStart[POS_3]) | (nalStart[POS_2] << POS_8) | (nalStart[POS_1] << POS_16) | (nalStart[POS_0] << POS_24));
}

bool IsHvccSyncFrame(const uint8_t *sample, int32_t size)
{
    if (sample == nullptr || size < NAL_START_CODE_SIZE || size > INT32_MAX) {
        return false;
    }
    const uint8_t* nalStart = sample;
    const uint8_t* end = nalStart + size;
    int32_t sizeLen = NAL_START_CODE_SIZE;
    int32_t naluSize = 0;
    naluSize = GetNaluSize(nalStart);
    if (naluSize <= 0 || nalStart > end - sizeLen) {
        return false;
    }
    nalStart = nalStart + sizeLen;
    while (nalStart < end) {
        uint8_t naluType = static_cast<uint8_t>((nalStart[0] & 0x7E) >> 1);
        if (naluType > 0x10 && naluType <= 0x17) {
            return true;
        }
        if (nalStart > end - naluSize) {
            return false;
        }
        nalStart = nalStart + naluSize;
        if (nalStart > end - sizeLen) {
            return false;
        }
        naluSize = GetNaluSize(nalStart);
        if (naluSize < 0) {
            return false;
        }
        nalStart = nalStart + sizeLen;
    }
    return false;
}

const uint8_t* FindNalStartCode(const uint8_t *start, const uint8_t *end, int32_t &startCodeLen)
{
    startCodeLen = sizeof(START_CODE);
    auto *iter = std::search(start, end, START_CODE, START_CODE + startCodeLen);
    if (iter != end && (iter > start && *(iter - 1) == 0x00)) {
        ++startCodeLen;
        return iter - 1;
    }
    return iter;
}

bool IsAnnexbSyncFrame(const uint8_t *sample, int32_t size)
{
    if (sample == nullptr || size < 0) {
        return false;
    }
    const uint8_t* nalStart = sample;
    const uint8_t* end = nalStart + size;
    const uint8_t* nalEnd = nullptr;
    int32_t startCodeLen = 0;
    nalStart = FindNalStartCode(nalStart, end, startCodeLen);
    if (nalStart > end - startCodeLen) {
        return false;
    }
    nalStart = nalStart + startCodeLen;
    while (nalStart < end) {
        nalEnd = FindNalStartCode(nalStart, end, startCodeLen);
        uint8_t naluType = static_cast<uint8_t>((nalStart[0] & 0x7E) >> 1);
        if (naluType > 0x10 && naluType <= 0x17) {
            return true;
        }
        if (nalEnd > end - startCodeLen) {
            return false;
        }
        nalStart = nalEnd + startCodeLen;
    }
    return false;
}

bool IsHevcSyncFrame(const uint8_t *sample, int32_t size)
{
    if (size < NAL_START_CODE_SIZE) {
        return false;
    }
    if (IsBeginAsAnnexb(sample, size)) {
        return IsAnnexbSyncFrame(sample, size);
    } else {
        return IsHvccSyncFrame(sample, size);
    }
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
