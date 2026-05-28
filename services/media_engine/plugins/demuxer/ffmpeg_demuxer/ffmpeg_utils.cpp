/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "HiStreamer" };

constexpr uint8_t HEVC_NAL_TYPE_MASK = 0x7E; // NAL unit type mask (bits 1-6 of first byte)
constexpr uint8_t HEVC_NAL_TYPE_SHIFT = 1;   // Right shift to get NAL type
constexpr uint8_t HEVC_NAL_TYPE_VPS = 32;    // Video Parameter Set
constexpr uint8_t HEVC_NAL_TYPE_SPS = 33;    // Sequence Parameter Set
constexpr uint8_t HEVC_NAL_TYPE_PPS = 34;    // Picture Parameter Set
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
bool IsHvccSyncFrame(const uint8_t *sample, int32_t size);
int32_t GetNaluSize(const uint8_t *nalStart);
const uint8_t* FindNalStartCode(const uint8_t *start, const uint8_t *end, int32_t &startCodeLen);
bool IsAnnexbSyncFrame(const uint8_t *sample, int32_t size);

std::string AVStrError(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}

int64_t ConvertTimeFromFFmpeg(int64_t pts, AVRational base)
{
    int64_t out;
    if (base.den == 0) {
        return pts;
    }
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
    if (base.num == 0 || base.den == 0) {
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
    if (base.num == 0 || base.den == 0) {
        result = AV_NOPTS_VALUE;
    } else {
        AVRational bq = {1, AV_CODEC_MSECOND};
        result = av_rescale_q(timestampUs, bq, base);
    }
    MEDIA_LOG_D("Base: [" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "], time convert ["
        PUBLIC_LOG_D64 "]->[" PUBLIC_LOG_D64 "].", base.num, base.den, timestampUs, result);
    return result;
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


std::vector<std::string> SplitString(const char* str, char delimiter)
{
    std::vector<std::string> rtv;
    if (str == nullptr || *str == '\0') {
        return rtv;
    }
    const char* start = str;
    const char* current = str;
    while (*current) {
        if (*current == delimiter) {
            if (current > start) {
                rtv.emplace_back(start, current - start);
            }
            start = current + 1;
        }
        current++;
    }
    if (current > start) {
        rtv.emplace_back(start, current - start);
    }
    return rtv;
}

int FindNaluSpliter(int size, const uint8_t* data)
{
    int naluPos = -1;
    FALSE_RETURN_V_MSG_D(data != nullptr, naluPos, "data is nullptr");
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
        sample->meta_->Set<Media::Tag::VIDEO_BUFFER_CAN_DROP>(true);
    }
}

int64_t AvTime2Us(int64_t hTime)
{
    return hTime / AV_CODEC_USECOND;
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
    if (iter != end && iter > start && *(iter - 1) == 0x00) {
        ++startCodeLen;
        return iter - 1;
    }
    return iter;
}

bool IsAnnexbSyncFrame(const uint8_t *sample, int32_t size)
{
    if (sample == nullptr || size < 0 || size > INT32_MAX) {
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

static bool IsHvccOnlyParameterSets(const uint8_t *sample, int32_t size)
{
    if (sample == nullptr || size < 0 || size > INT32_MAX) {
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
        uint8_t naluType = static_cast<uint8_t>((nalStart[0] & HEVC_NAL_TYPE_MASK) >> HEVC_NAL_TYPE_SHIFT);
        if (naluType != HEVC_NAL_TYPE_VPS && naluType != HEVC_NAL_TYPE_SPS && naluType != HEVC_NAL_TYPE_PPS) {
            return false;
        }
        if (nalStart > end - naluSize) {
            return false;
        }
        nalStart = nalStart + naluSize;
        if (nalStart > end - sizeLen) {
            return true;
        }
        naluSize = GetNaluSize(nalStart);
        if (naluSize < 0) {
            return false;
        }
        nalStart = nalStart + sizeLen;
    }
    return true;
}

static bool IsAnnexbOnlyParameterSets(const uint8_t *sample, int32_t size)
{
    if (sample == nullptr || size < 0 || size > INT32_MAX) {
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
        uint8_t naluType = static_cast<uint8_t>((nalStart[0] & HEVC_NAL_TYPE_MASK) >> HEVC_NAL_TYPE_SHIFT);
        if (naluType != HEVC_NAL_TYPE_VPS && naluType != HEVC_NAL_TYPE_SPS && naluType != HEVC_NAL_TYPE_PPS) {
            return false;
        }
        if (nalEnd > end - startCodeLen) {
            return true;
        }
        nalStart = nalEnd + startCodeLen;
    }
    return true;
}

bool IsPktOnlyParameterSets(const uint8_t *sample, int32_t size)
{
    if (sample == nullptr || size < 0 || size > INT32_MAX) {
        return false;
    }
    if (IsBeginAsAnnexb(sample, size)) {
        return IsAnnexbOnlyParameterSets(sample, size);
    } else {
        return IsHvccOnlyParameterSets(sample, size);
    }
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
