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
#include <algorithm>
#include <functional>
#include <map>
#include "common/log.h"
#include "ffmpeg_utils.h"
#include "meta/media_types.h"

#define AV_CODEC_TIME_BASE (static_cast<int64_t>(1))
#define AV_CODEC_NSECOND AV_CODEC_TIME_BASE
#define AV_CODEC_USECOND (static_cast<int64_t>(1000) * AV_CODEC_NSECOND)
#define AV_CODEC_MSECOND (static_cast<int64_t>(1000) * AV_CODEC_USECOND)
#define AV_CODEC_SECOND (static_cast<int64_t>(1000) * AV_CODEC_MSECOND)

namespace OHOS {
namespace Media {
namespace Plugin {
namespace Ffmpeg {
namespace {

static const std::map<AVMediaType, std::string_view> g_FFmpegMediaTypeToString {
    {AVMediaType::AVMEDIA_TYPE_VIDEO, "video"},
    {AVMediaType::AVMEDIA_TYPE_AUDIO, "audio"},
    {AVMediaType::AVMEDIA_TYPE_DATA, "data"},
    {AVMediaType::AVMEDIA_TYPE_SUBTITLE, "subtitle"},
    {AVMediaType::AVMEDIA_TYPE_ATTACHMENT, "attachment"},
};
} // namespace

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
    MEDIA_LOG_D("Base: [" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "], time convert [" PUBLIC_LOG_D64 "]->[" PUBLIC_LOG_D64 "].",
        base.num, base.den, pts, out);
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
    MEDIA_LOG_D("Base: [" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "], time convert [" PUBLIC_LOG_D64 "]->[" PUBLIC_LOG_D64 "].",
        base.num, base.den, timestampUs, result);
    return result;
}

std::string_view ConvertFFmpegMediaTypeToString(AVMediaType mediaType)
{
    auto ite = std::find_if(g_FFmpegMediaTypeToString.begin(), g_FFmpegMediaTypeToString.end(),
                            [&mediaType](const auto &item) -> bool { return item.first == mediaType; });
    if (ite == g_FFmpegMediaTypeToString.end()) {
        return "unknow";
    }
    return ite->second;
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
    if (memoryNotEnough) {
        flags |= (uint32_t)(AVBufferFlag::PARTIAL_FRAME);
    }
    return flags;
}

int64_t CalculateTimeByFrameIndex(const AVStream* avStream, int keyFrameIdx)
{
    FALSE_RETURN_V_MSG_E(avStream != nullptr, 0, "Track is nullptr.");
#if defined(LIBAVFORMAT_VERSION_INT) && defined(AV_VERSION_INT)
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(58, 78, 0)
    return avformat_index_get_entry(avStream, keyFrameIdx)->timestamp;
#elif LIBAVFORMAT_VERSION_INT == AV_VERSION_INT(58, 76, 100)
    return avStream->index_entries[keyFrameIdx].timestamp;
#elif LIBAVFORMAT_VERSION_INT > AV_VERSION_INT(58, 64, 100)
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
} // namespace Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS
