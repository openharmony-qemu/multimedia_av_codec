/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#define MEDIA_PLUGIN
#define HST_LOG_TAG "FfmpegDemuxerPlugin"
#include <unistd.h>
#include <algorithm>
#include <malloc.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <fstream>
#include <chrono>
#include <limits>
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "securec.h"
#include "ffmpeg_format_helper.h"
#include "ffmpeg_utils.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "common/log.h"
#include "meta/video_types.h"
#include "demuxer_log_compressor.h"
#include "ffmpeg_demuxer_plugin.h"
#include "meta/format.h"
#include "syspara/parameters.h"
#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "FfmpegDemuxerPlugin" };

int32_t GetMinCttsDuration(const AVStream* avStream)
{
    if (avStream == nullptr) {
        return 0;
    }
    int32_t minCttsDuration = INT_MAX;
    for (uint32_t i = 0u; i < avStream->ctts_count; i++) {
        if (avStream->ctts_data[i].duration < minCttsDuration) {
            minCttsDuration = avStream->ctts_data[i].duration;
        }
    }
    return std::min(minCttsDuration, 0);
}
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
const uint32_t DEFAULT_READ_SIZE = 4096;
const uint32_t DEFAULT_SNIFF_SIZE = 4096 * 4;
const int32_t MP3_PROBE_SCORE_LIMIT = 5;
const int32_t DEF_PROBE_SCORE_LIMIT = 50;
const uint32_t RANK_MAX = 100;
const int32_t NAL_START_CODE_SIZE = 4;
const uint32_t INIT_DOWNLOADS_DATA_SIZE_THRESHOLD = 2 * 1024 * 1024;
const int64_t LIVE_FLV_PROBE_SIZE = 100 * 1024 * 2;
const uint32_t DEFAULT_CACHE_LIMIT = 50 * 1024 * 1024; // 50M
const int64_t INIT_TIME_THRESHOLD = 1000;
const uint32_t ID3V2_HEADER_SIZE = 10;
const int32_t MS_TO_US = 1000;
const int32_t MS_TO_NS = 1000 * 1000;
const uint32_t REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT = 200000;
const int DEFAULT_CHANNEL_CNT = 3;
const int READ_SIZE_LIMIT_FACTOR = 2;
const int READ_SIZE_LIMIT_DEFAULT = 4096 * 2160 * 3 * 2;
const char* PLUGIN_NAME_PREFIX = "avdemux_";
const char* PLUGIN_NAME_MP3 = "mp3";
const char* PLUGIN_NAME_MPEGPS = "mpeg";
const uint8_t START_CODE[] = {0x00, 0x00, 0x01};
const int32_t MPEGPS_START_CODE_SIZE = 4;
const uint8_t MPEGPS_START_CODE[] = {0x00, 0x00, 0x01, 0xBA};
const uint32_t SETTIMER_TIMEOUT = 5; // second
constexpr int64_t LOG_INTERVAL_MS = 2000; // 2s
constexpr uint32_t LOG_MAX_COUNT = 10; // 10 times
constexpr int32_t SEEK_TRACK_DEFAULT = -1;

// id3v2 tag position
const int32_t POS_0 = 0;
const int32_t POS_1 = 1;
const int32_t POS_2 = 2;
const int32_t POS_3 = 3;
const int32_t POS_4 = 4;
const int32_t POS_5 = 5;
const int32_t POS_6 = 6;
const int32_t POS_7 = 7;
const int32_t POS_8 = 8;
const int32_t POS_9 = 9;
const int32_t POS_14 = 14;
const int32_t POS_16 = 16;
const int32_t POS_21 = 21;
const int32_t POS_24 = 24;
const int32_t POS_FF = 0xff;
const int32_t LEN_MASK = 0x7f;
const int32_t TAG_MASK = 0x80;
const int32_t TAG_VERSION_MASK = 0x10;

const std::vector<AVCodecID> g_streamContainedXPS = {
    AV_CODEC_ID_H264,
    AV_CODEC_ID_HEVC,
    AV_CODEC_ID_VVC
};
namespace {
std::mutex g_mtx;

int Sniff(const std::string& pluginName, std::shared_ptr<DataSource> dataSource);
int SniffWithSize(const std::string& pluginName, std::shared_ptr<DataSource> dataSource, int probSize);
int SniffMPEGPS(const std::string& pluginName, std::shared_ptr<DataSource> dataSource);

Status RegisterPlugins(const std::shared_ptr<Register>& reg);

void ReplaceDelimiter(const std::string &delmiters, char newDelimiter, std::string &str);

inline std::string ProcessPluginName(const std::string& pluginName)
{
    size_t pos = pluginName.find("avdemux_");
    if (pos != std::string::npos) {
        return pluginName.substr(pos + strlen("avdemux_")); // Get the name after avdemux_
    }
    return pluginName;
}

static const std::map<SeekMode, int32_t>  g_seekModeToFFmpegSeekFlags = {
    { SeekMode::SEEK_PREVIOUS_SYNC, AVSEEK_FLAG_BACKWARD },
    { SeekMode::SEEK_NEXT_SYNC, AVSEEK_FLAG_FRAME },
    { SeekMode::SEEK_CLOSEST_SYNC, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD }
};

static const std::map<AVCodecID, std::string> g_bitstreamFilterMap = {
    { AV_CODEC_ID_H264, "h264_mp4toannexb" },
};

static const std::map<AVCodecID, VideoStreamType> g_streamParserMap = {
    { AV_CODEC_ID_HEVC, VideoStreamType::HEVC },
    { AV_CODEC_ID_VVC,  VideoStreamType::VVC },
};

static const std::vector<AVMediaType> g_streamMediaTypeVec = {
    AVMEDIA_TYPE_AUDIO,
    AVMEDIA_TYPE_VIDEO,
    AVMEDIA_TYPE_SUBTITLE,
    AVMEDIA_TYPE_TIMEDMETA,
    AVMEDIA_TYPE_AUXILIARY
};

static const std::vector<AVMediaType> g_streamGetFirstFrameTypeVec = {
    AVMEDIA_TYPE_AUDIO,
    AVMEDIA_TYPE_VIDEO,
    AVMEDIA_TYPE_AUXILIARY
};

static const std::vector<FileType> g_streamCheckFileTypeVec = {
    FileType::MPEGTS,
    FileType::MPEGPS,
    FileType::VOB
};

static const std::vector<FileType> g_fileContainSkipInfo = {
    FileType::OGG,
    FileType::MP3
};

static const std::vector<FileType> g_fileSkipGetMinTsPktInfo = {
    FileType::FLV,
    FileType::MKV,
    FileType::WMV,
    FileType::WMA
};

bool HaveValidParser(const AVCodecID codecId)
{
    return g_streamParserMap.count(codecId) != 0;
}

int64_t GetFileDuration(const AVFormatContext& avFormatContext)
{
    int64_t duration = 0;
    const AVDictionaryEntry *metaDuration = av_dict_get(avFormatContext.metadata, "DURATION", NULL, 0);
    int64_t us;
    if (metaDuration != nullptr && (av_parse_time(&us, metaDuration->value, 1) == 0)) {
        if (us > duration) {
            MEDIA_LOG_D("Get duration from file");
            duration = us;
        }
    }

    if (duration <= 0) {
        for (uint32_t i = 0; i < avFormatContext.nb_streams; ++i) {
            auto streamDuration = (ConvertTimeFromFFmpeg(avFormatContext.streams[i]->duration,
                avFormatContext.streams[i]->time_base)) / 1000; // us
            if (streamDuration > duration) {
                MEDIA_LOG_D("Get duration from stream " PUBLIC_LOG_U32, i);
                duration = streamDuration;
            }
        }
    }
    return duration;
}

int64_t GetStreamDuration(const AVStream& avStream)
{
    int64_t duration = 0;
    const AVDictionaryEntry *metaDuration = av_dict_get(avStream.metadata, "DURATION", NULL, 0);
    int64_t us;
    if (metaDuration != nullptr && (av_parse_time(&us, metaDuration->value, 1) == 0)) {
        if (us > duration) {
            MEDIA_LOG_D("Get duration from stream");
            duration = us;
        }
    }
    return duration;
}

bool CheckStartTime(const AVFormatContext *formatContext, const AVStream *stream, int64_t &timeStamp, int64_t seekTime)
{
    int64_t startTime = 0;
    int64_t num = 1000; // ms convert us
    FALSE_RETURN_V_MSG_E(stream != nullptr, false, "String is nullptr");
    if (stream->start_time != AV_NOPTS_VALUE) {
        startTime = stream->start_time;
        if (timeStamp > 0 && startTime > INT64_MAX - timeStamp) {
            MEDIA_LOG_E("Seek value overflow with start time: " PUBLIC_LOG_D64 " timeStamp: " PUBLIC_LOG_D64,
                startTime, timeStamp);
            return false;
        }
    }
    MEDIA_LOG_D("StartTime: " PUBLIC_LOG_D64, startTime);
    int64_t fileDuration = formatContext->duration;
    int64_t streamDuration = stream->duration;
    if (fileDuration == AV_NOPTS_VALUE || fileDuration <= 0) {
        fileDuration = GetFileDuration(*formatContext);
    }
    if (streamDuration == AV_NOPTS_VALUE || streamDuration <= 0) {
        streamDuration = GetStreamDuration(*stream);
    }
    MEDIA_LOG_D("File duration=" PUBLIC_LOG_D64 ", stream duration=" PUBLIC_LOG_D64, fileDuration, streamDuration);
    // when timestemp out of file duration, return error
    if (fileDuration > 0 && seekTime > fileDuration / num) { // fileDuration us
        MEDIA_LOG_E("Seek to timestamp=" PUBLIC_LOG_D64 " failed, max=" PUBLIC_LOG_D64, timeStamp, fileDuration);
        return false;
    }
    // when timestemp out of stream duration, seek to end of stream
    if (streamDuration > 0 && timeStamp > streamDuration) {
        MEDIA_LOG_W("Out of stream, seek to " PUBLIC_LOG_D64, timeStamp);
        timeStamp = streamDuration;
    }
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO || stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ||
        stream->codecpar->codec_type == AVMEDIA_TYPE_AUXILIARY) {
        MEDIA_LOG_D("Reset timeStamp by start time [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "]",
            timeStamp, timeStamp + startTime);
        timeStamp += startTime;
    }
    return true;
}

int ConvertFlagsToFFmpeg(AVStream *avStream, int64_t ffTime, SeekMode mode, int64_t seekTime)
{
    FALSE_RETURN_V_MSG_E(avStream != nullptr && avStream->codecpar != nullptr, -1, "AVStream is nullptr");
    if (avStream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE && ffTime == 0) {
        return AVSEEK_FLAG_FRAME;
    }
    if (FFmpegFormatHelper::IsAudioType(*avStream) && seekTime != 0) {
        int64_t streamDuration = avStream->duration;
        if (streamDuration == AV_NOPTS_VALUE || streamDuration <= 0) {
            streamDuration = GetStreamDuration(*avStream);
        }
        // When the seekTime is within the last 0.5s, still use BACKWARD mode to ensure consistent func behavior.
        int64_t buffering = ConvertTimeToFFmpeg(500 * MS_TO_NS, avStream->time_base); // 0.5s
        if (streamDuration > 0  && (streamDuration < buffering || ffTime >= streamDuration - buffering)) {
            return AVSEEK_FLAG_BACKWARD;
        }
        return g_seekModeToFFmpegSeekFlags.at(mode);
    }
    if (!FFmpegFormatHelper::IsVideoType(*avStream) || seekTime == 0) {
        return AVSEEK_FLAG_BACKWARD;
    }
    if (mode == SeekMode::SEEK_NEXT_SYNC || mode == SeekMode::SEEK_PREVIOUS_SYNC) {
        return g_seekModeToFFmpegSeekFlags.at(mode);
    }
    // find closest time in next and prev
    int keyFrameNext = av_index_search_timestamp(avStream, ffTime, AVSEEK_FLAG_FRAME);
    FALSE_RETURN_V_MSG_E(keyFrameNext >= 0, AVSEEK_FLAG_BACKWARD, "Not next key frame");

    int keyFramePrev = av_index_search_timestamp(avStream, ffTime, AVSEEK_FLAG_BACKWARD);
    FALSE_RETURN_V_MSG_E(keyFramePrev >= 0, AVSEEK_FLAG_FRAME, "Not pre key frame");

    int64_t ffTimePrev = CalculateTimeByFrameIndex(avStream, keyFramePrev);
    int64_t ffTimeNext = CalculateTimeByFrameIndex(avStream, keyFrameNext);
    MEDIA_LOG_D("FfTime=" PUBLIC_LOG_D64 ", ffTimePrev=" PUBLIC_LOG_D64 ", ffTimeNext=" PUBLIC_LOG_D64,
        ffTime, ffTimePrev, ffTimeNext);
    if (ffTimePrev == ffTimeNext || (ffTimeNext - ffTime < ffTime - ffTimePrev)) {
        return AVSEEK_FLAG_FRAME;
    } else {
        return AVSEEK_FLAG_BACKWARD;
    }
}

bool IsSupportedTrackType(const AVStream& avStream)
{
    return (std::find(g_streamMediaTypeVec.cbegin(), g_streamMediaTypeVec.cend(),
        avStream.codecpar->codec_type) != g_streamMediaTypeVec.cend());
}

bool IsSupportedGetFirstFrame(const AVStream& avStream)
{
    return (std::find(g_streamGetFirstFrameTypeVec.cbegin(), g_streamGetFirstFrameTypeVec.cend(),
        avStream.codecpar->codec_type) != g_streamGetFirstFrameTypeVec.cend());
}

bool IsSupportedTrack(const AVStream& avStream)
{
    FALSE_RETURN_V_MSG_E(avStream.codecpar != nullptr, false, "Codecpar is nullptr");
    if (!IsSupportedTrackType(avStream)) {
        MEDIA_LOG_E("Unsupport track type: " PUBLIC_LOG_D32, avStream.codecpar->codec_type);
        return false;
    }
    if (avStream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (avStream.codecpar->codec_id == AV_CODEC_ID_RAWVIDEO) {
            MEDIA_LOG_E("Unsupport raw video track");
            return false;
        }
        if (FFmpegFormatHelper::IsImageTrack(avStream)) {
            MEDIA_LOG_E("Unsupport image track");
            return false;
        }
    }
    return true;
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

void FfmpegLogPrint(void* avcl, int level, const char* fmt, va_list vl)
{
    (void)avcl;
    char buf[500] = {0}; // 500
    int ret = vsnprintf_s(buf, sizeof(buf), sizeof(buf) - 1, fmt, vl);
    if (ret < 0) {
        return;
    }
    switch (level) {
        case AV_LOG_WARNING:
            MEDIA_LOG_D("[FFLogW] " PUBLIC_LOG_S, buf);
            break;
        case AV_LOG_ERROR:
            AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGE, LOG_INTERVAL_MS, LOG_MAX_COUNT, "[FFLogE] " PUBLIC_LOG_S, buf);
            break;
        case AV_LOG_FATAL:
            AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGE, LOG_INTERVAL_MS, LOG_MAX_COUNT, "[FFLogF] " PUBLIC_LOG_S, buf);
            break;
        case AV_LOG_PANIC:
            AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGE, LOG_INTERVAL_MS, LOG_MAX_COUNT, "[FFLogP] " PUBLIC_LOG_S, buf);
            break;
        case AV_LOG_INFO:
            MEDIA_LOG_D("[FFLogI] " PUBLIC_LOG_S, buf);
            break;
        case AV_LOG_DEBUG:
            MEDIA_LOG_D("[FFLogD] " PUBLIC_LOG_S, buf);
            break;
        default:
            break;
    }
}
} // namespace

std::atomic<int> FFmpegDemuxerPlugin::readatIndex_ = 0;
FFmpegDemuxerPlugin::FFmpegDemuxerPlugin(std::string name)
    : DemuxerPlugin(std::move(name)),
      seekable_(Seekable::SEEKABLE),
      ioContext_(),
      selectedTrackIds_(),
      cacheQueue_("cacheQueue"),
      parserRefIoContext_()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("In");
#ifndef _WIN32
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
#endif
    av_log_set_callback(FfmpegLogPrint);
#ifdef BUILD_ENG_VERSION
    std::string dumpModeStr = OHOS::system::GetParameter("FFmpegDemuxerPlugin.dump", "0");
    dumpMode_ = static_cast<DumpMode>(strtoul(dumpModeStr.c_str(), nullptr, 2)); // 2 is binary
    MEDIA_LOG_D("Dump mode = %s(%lu)", dumpModeStr.c_str(), dumpMode_);
#endif
    MEDIA_LOG_D("Out");
}

FFmpegDemuxerPlugin::~FFmpegDemuxerPlugin()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("In");
    ioContext_.avReadPacketStopState.store(AVReadPacketStopState::TRUE);
    ReleaseFFmpegReadLoop();
#ifndef _WIN32
    (void)mallopt(M_FLUSH_THREAD_CACHE, 0);
#endif
    formatContext_ = nullptr;
    pluginImpl_ = nullptr;
    for (auto &item : avbsfContexts_) {
        if (item.second != nullptr) {
            item.second = nullptr;
        }
    }
    streamParsers_ = nullptr;
    referenceParser_ = nullptr;
    parserRefCtx_ = nullptr;
    selectedTrackIds_.clear();
    videoFirstFrameMap_.clear();
    MEDIA_LOG_D("Out");
}

void FFmpegDemuxerPlugin::Dump(const DumpParam &dumpParam)
{
    std::string path;
    switch (dumpParam.mode) {
        case DUMP_READAT_INPUT:
            path = "Readat_index." + std::to_string(dumpParam.index) + "_offset." + std::to_string(dumpParam.offset) +
                "_size." + std::to_string(dumpParam.size);
            break;
        case DUMP_AVPACKET_OUTPUT:
            path = "AVPacket_index." + std::to_string(dumpParam.index) + "_track." +
                std::to_string(dumpParam.trackId) + "_pts." + std::to_string(dumpParam.pts) + "_pos." +
                std::to_string(dumpParam.pos);
            break;
        case DUMP_AVBUFFER_OUTPUT:
            path = "AVBuffer_track." + std::to_string(dumpParam.trackId) + "_index." +
                std::to_string(dumpParam.index) + "_pts." + std::to_string(dumpParam.pts);
            break;
        default:
            return;
    }
    std::ofstream ofs;
    path = "/data/ff_dump/" + path;
    ofs.open(path, std::ios::out); //  | std::ios::app
    if (ofs.is_open()) {
        ofs.write(reinterpret_cast<char*>(dumpParam.buf), dumpParam.size);
        ofs.close();
    }
    MEDIA_LOG_D("Dump path:" PUBLIC_LOG_S, path.c_str());
}

bool FFmpegDemuxerPlugin::GetProbeSize(int32_t &offset, int32_t &size)
{
    offset = 0;
    size = 5000000; // 5000000 ff init probe size
    return true;
}

void FFmpegDemuxerPlugin::ResetParam()
{
    readatIndex_ = 0;
    avpacketIndex_ = 0;
    ioContext_.offset = 0;
    ioContext_.retry = false;
    ioContext_.eos = false;
    ioContext_.initDownloadDataSize = 0;
    mediaInfo_ = MediaInfo();
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        cacheQueue_.RemoveTrackQueue(selectedTrackIds_[i]);
    }
    selectedTrackIds_.clear();
    checkedTrackIds_.clear();
    pluginImpl_.reset();
    formatContext_.reset();
    streamSnapshots_.clear();

    streamParsers_.reset();
    for (auto item : avbsfContexts_) {
        item.second.reset();
    }
    trackMtx_.clear();
    trackDfxInfoMap_.clear();
}

Status FFmpegDemuxerPlugin::Reset()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("In");
    ReleaseFFmpegReadLoop();
    ResetParam();
    return Status::OK;
}

void FFmpegDemuxerPlugin::InitBitStreamContext(const AVStream& avStream)
{
    FALSE_RETURN_MSG(avStream.codecpar != nullptr, "Codecpar is nullptr");
    AVCodecID codecID = avStream.codecpar->codec_id;
    MEDIA_LOG_D("For track " PUBLIC_LOG_D32, avStream.index);
    FALSE_RETURN_MSG(g_bitstreamFilterMap.count(codecID) != 0, "Can not match any BitStreamContext");
    const AVBitStreamFilter* avBitStreamFilter = av_bsf_get_by_name(g_bitstreamFilterMap.at(codecID).c_str());

    FALSE_RETURN_MSG((avBitStreamFilter != nullptr), "Call av_bsf_get_by_name failed for" PUBLIC_LOG_S,
            g_bitstreamFilterMap.at(codecID).c_str());

    if (avbsfContexts_.count(avStream.index) <= 0 || !avbsfContexts_[avStream.index]) {
        AVBSFContext* avbsfContext {nullptr};
        int ret = av_bsf_alloc(avBitStreamFilter, &avbsfContext);
        FALSE_RETURN_MSG((ret >= 0 && avbsfContext != nullptr),
            "Call av_bsf_alloc failed, err:" PUBLIC_LOG_S, AVStrError(ret).c_str());

        ret = avcodec_parameters_copy(avbsfContext->par_in, avStream.codecpar);
        FALSE_RETURN_MSG((ret >= 0), "Call avcodec_parameters_copy failed, err:" PUBLIC_LOG_S, AVStrError(ret).c_str());

        ret = av_bsf_init(avbsfContext);
        FALSE_RETURN_MSG((ret >= 0), "Call av_bsf_init failed, err:" PUBLIC_LOG_S, AVStrError(ret).c_str());

        avbsfContexts_[avStream.index] = std::shared_ptr<AVBSFContext>(avbsfContext, [](AVBSFContext* ptr) {
            if (ptr) {
                av_bsf_free(&ptr);
            }
        });
    }
    FALSE_RETURN_MSG(avbsfContexts_[avStream.index] != nullptr,
        "Stream " PUBLIC_LOG_S " will not be converted to annexb", g_bitstreamFilterMap.at(codecID).c_str());
    MEDIA_LOG_D("Track " PUBLIC_LOG_D32 " will convert to annexb", avStream.index);
}

Status FFmpegDemuxerPlugin::ConvertAvcToAnnexb(AVPacket& pkt)
{
    int32_t trackId = pkt.stream_index;
    int ret = av_bsf_send_packet(avbsfContexts_[trackId].get(), &pkt);
    FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_UNKNOWN,
        "Call av_bsf_send_packet failed, err:" PUBLIC_LOG_S, AVStrError(ret).c_str());
    av_packet_unref(&pkt);

    ret = av_bsf_receive_packet(avbsfContexts_[trackId].get(), &pkt);
    FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_PACKET_CONVERT_FAILED,
        "Call av_bsf_receive_packet failed, err:" PUBLIC_LOG_S, AVStrError(ret).c_str());
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ConvertHevcToAnnexb(AVPacket& pkt, std::shared_ptr<SamplePacket> samplePacket)
{
    size_t cencInfoSize = 0;
    AVPacket *firstPkt = (samplePacket->pkts[0] != nullptr) ? samplePacket->pkts[0]->GetAVPacket() : nullptr;
    FALSE_RETURN_V_MSG_E(firstPkt != nullptr, Status::ERROR_INVALID_OPERATION, "First pkt is nullptr");
    uint8_t *cencInfo = av_packet_get_side_data(firstPkt, AV_PKT_DATA_ENCRYPTION_INFO, &cencInfoSize);
    PacketConvertInfo convertInfo {cencInfo, cencInfoSize, false};
    streamParsers_->ConvertPacketToAnnexb(pkt.stream_index, &(pkt.data), pkt.size, convertInfo);
    if (NeedCombineFrame(firstPkt->stream_index) &&
        streamParsers_->IsSyncFrame(pkt.stream_index, pkt.data, pkt.size)) {
        pkt.flags = static_cast<int32_t>(static_cast<uint32_t>(pkt.flags) | static_cast<uint32_t>(AV_PKT_FLAG_KEY));
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ConvertVvcToAnnexb(AVPacket& pkt, std::shared_ptr<SamplePacket> samplePacket)
{
    PacketConvertInfo convertInfo {nullptr, 0, false};
    streamParsers_->ConvertPacketToAnnexb(pkt.stream_index, &(pkt.data), pkt.size, convertInfo);
    return Status::OK;
}

Status FFmpegDemuxerPlugin::WriteBuffer(
    std::shared_ptr<AVBuffer> outBuffer, const uint8_t *writeData, uint32_t writeSize)
{
    FALSE_RETURN_V_MSG_E(outBuffer != nullptr, Status::ERROR_NULL_POINTER, "Buffer is nullptr");
    if (writeData != nullptr && writeSize > 0) {
        FALSE_RETURN_V_MSG_E(outBuffer->memory_ != nullptr, Status::ERROR_NULL_POINTER, "Memory is nullptr");
        int32_t ret = outBuffer->memory_->Write(writeData, writeSize, 0);
        FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_INVALID_OPERATION, "Memory write failed");
    }

    MEDIA_LOG_D("CurrentBuffer: [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "/" PUBLIC_LOG_U32 "]",
        outBuffer->pts_, outBuffer->duration_, outBuffer->flag_);
    return Status::OK;
}

Status FFmpegDemuxerPlugin::SetDrmCencInfo(
    std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_INVALID_OPERATION, "Sample is nullptr");
    // 0 mean sync read, 1 mean async read
    // sync read (0) need check memory_, async read (1) don't need check memory_
    bool isAsyncRead = (readModeMap_.find(1) != readModeMap_.end() && readModeMap_[1] == 1);
    FALSE_RETURN_V_MSG_E(isAsyncRead || sample->memory_ != nullptr, Status::ERROR_INVALID_OPERATION,
        "Memory is nullptr");
    FALSE_RETURN_V_MSG_E((samplePacket != nullptr && samplePacket->pkts.size() > 0), Status::ERROR_INVALID_OPERATION,
        "Packet is nullptr");
    AVPacket *firstPkt = (samplePacket->pkts[0] != nullptr) ? samplePacket->pkts[0]->GetAVPacket() : nullptr;
    FALSE_RETURN_V_MSG_E((firstPkt != nullptr && firstPkt->size >= 0),
        Status::ERROR_INVALID_OPERATION, "Packet empty");

    size_t cencInfoSize = 0;
    MetaDrmCencInfo *cencInfo = (MetaDrmCencInfo *)av_packet_get_side_data(firstPkt,
        AV_PKT_DATA_ENCRYPTION_INFO, &cencInfoSize);
    if ((cencInfo != nullptr) && (cencInfoSize != 0)) {
        std::vector<uint8_t> drmCencVec(reinterpret_cast<uint8_t *>(cencInfo),
            (reinterpret_cast<uint8_t *>(cencInfo)) + sizeof(MetaDrmCencInfo));
        sample->meta_->SetData(Media::Tag::DRM_CENC_INFO, std::move(drmCencVec));
    }
    return Status::OK;
}

bool FFmpegDemuxerPlugin::NeedCombineFrame(uint32_t trackId)
{
    const auto* snapshot = GetStreamSnapshot(trackId);
    FALSE_RETURN_V_MSG_E(snapshot != nullptr && snapshot->valid, false, "Stream snapshot is invalid");
    return snapshot->needCombineFrame;
}

Plugins::AVPacketWrapperPtr FFmpegDemuxerPlugin::CombinePackets(std::shared_ptr<SamplePacket> samplePacket)
{
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr && !samplePacket->pkts.empty(), nullptr, "SamplePacket is invalid");
    Plugins::AVPacketWrapperPtr firstWrapper = samplePacket->pkts[0];
    AVPacket *firstPkt = (firstWrapper != nullptr) ? firstWrapper->GetAVPacket() : nullptr;
    FALSE_RETURN_V_MSG_E(firstPkt != nullptr, nullptr, "First pkt is nullptr");
    if (!NeedCombineFrame(firstPkt->stream_index) || samplePacket->pkts.size() <= 1) {
        return firstWrapper;
    }

    int totalSize = 0;
    for (const auto &pktWrapper : samplePacket->pkts) {
        FALSE_RETURN_V_MSG_E(pktWrapper != nullptr && pktWrapper->GetAVPacket() != nullptr, nullptr,
            "AVPacket is nullptr");
        totalSize += pktWrapper->GetSize();
    }

    Plugins::AVPacketWrapperPtr tempWrapper = std::make_shared<Plugins::AVPacketWrapper>();
    FALSE_RETURN_V_MSG_E(tempWrapper != nullptr && tempWrapper->GetAVPacket() != nullptr, nullptr, "Create failed");
    AVPacket *tempPkt = tempWrapper->GetAVPacket();
    int ret = av_new_packet(tempPkt, totalSize);
    FALSE_RETURN_V_MSG_E(ret >= 0, nullptr, "Call av_new_packet failed");
    av_packet_copy_props(tempPkt, firstPkt);

    int offset = 0;
    for (const auto &pktWrapper : samplePacket->pkts) {
        AVPacket *pkt = pktWrapper->GetAVPacket();
        if (pkt == nullptr || tempPkt->data == nullptr || pkt->data == nullptr ||
            offset < 0 || pkt->size < 0 || offset > INT_MAX - pkt->size || offset + pkt->size > totalSize) {
            MEDIA_LOG_E("Memcpy param invalid: totalSize=" PUBLIC_LOG_D32 ", offset=" PUBLIC_LOG_D32 ", pkt->size="
                PUBLIC_LOG_D32, totalSize, offset, (pkt != nullptr) ? pkt->size : -1);
            tempWrapper.reset();
            return nullptr;
        }
        ret = memcpy_s(tempPkt->data + offset, tempPkt->size - offset, pkt->data, pkt->size);
        if (ret != EOK) {
            MEDIA_LOG_E("Memcpy failed, ret:" PUBLIC_LOG_D32, ret);
            return nullptr;
        }
        offset += pkt->size;
    }
    tempPkt->size = totalSize;
    MEDIA_LOG_D("Combine " PUBLIC_LOG_ZU " packets, total size=" PUBLIC_LOG_D32,
        samplePacket->pkts.size(), totalSize);
    return tempWrapper;
}

Status FFmpegDemuxerPlugin::ConvertPacketToAnnexb(std::shared_ptr<AVBuffer> sample,
    Plugins::AVPacketWrapperPtr srcWrapper, std::shared_ptr<SamplePacket> dstSamplePacket)
{
    AVPacket *srcAVPacket = (srcWrapper != nullptr) ? srcWrapper->GetAVPacket() : nullptr;
    FALSE_RETURN_V_MSG_E(srcAVPacket != nullptr, Status::ERROR_NULL_POINTER, "srcAVPacket is nullptr");
    Status ret = Status::OK;
    if (dstSamplePacket->isAnnexb) {
        MEDIA_LOG_D("Has converted");
        return ret;
    }
    const AVStreamSnapshot* snapshot = GetStreamSnapshot(static_cast<uint32_t>(srcAVPacket->stream_index));
    FALSE_RETURN_V_MSG_E(snapshot != nullptr && snapshot->valid,
        Status::ERROR_INVALID_OPERATION, "Stream snapshot is invalid");
    auto codecId = snapshot->codecId;
    if (codecId == AV_CODEC_ID_HEVC && streamParsers_ != nullptr &&
        streamParsers_->ParserIsInited(srcAVPacket->stream_index)) {
        ret = ConvertHevcToAnnexb(*srcAVPacket, dstSamplePacket);
        SetDropTag(*srcAVPacket, sample, AV_CODEC_ID_HEVC);
    } else if (codecId == AV_CODEC_ID_VVC && streamParsers_ != nullptr &&
        streamParsers_->ParserIsInited(srcAVPacket->stream_index)) {
        ret = ConvertVvcToAnnexb(*srcAVPacket, dstSamplePacket);
    } else if (codecId == AV_CODEC_ID_H264 &&
        avbsfContexts_.count(srcAVPacket->stream_index) > 0 && avbsfContexts_[srcAVPacket->stream_index] != nullptr) {
        ret = ConvertAvcToAnnexb(*srcAVPacket);
        SetDropTag(*srcAVPacket, sample, AV_CODEC_ID_H264);
    }
    if (ret != Status::OK) {
        Plugins::AVPacketWrapperPtr firstWrapper =
            (dstSamplePacket->pkts.size() > 0) ? dstSamplePacket->pkts[0] : nullptr;
        AVPacket *firstPkt = (firstWrapper != nullptr) ? firstWrapper->GetAVPacket() : nullptr;
        if (firstPkt != nullptr) {
            cacheQueue_.Pop(static_cast<uint32_t>(firstPkt->stream_index));
        }
        if (ioContext_.retry) {
            ioContext_.retry = false;
            formatContext_->pb->eof_reached = 0;
            formatContext_->pb->error = 0;
            return Status::ERROR_AGAIN;
        }
    }
    dstSamplePacket->isAnnexb = true;
    return ret;
}

bool FFmpegDemuxerPlugin::VideoFirstFrameValid(uint32_t trackIndex)
{
    return videoFirstFrameMap_.count(trackIndex) > 0 && videoFirstFrameMap_[trackIndex] != nullptr;
}

void FFmpegDemuxerPlugin::WriteBufferAttr(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket)
{
    Plugins::AVPacketWrapperPtr firstWrapper = (samplePacket->pkts.size() > 0) ? samplePacket->pkts[0] : nullptr;
    AVPacket *firstPkt = (firstWrapper != nullptr) ? firstWrapper->GetAVPacket() : nullptr;
    FALSE_RETURN_MSG(firstPkt != nullptr, "First pkt is nullptr");
    uint32_t trackIndex = static_cast<uint32_t>(firstPkt->stream_index);
    const AVStreamSnapshot* snapshot = GetStreamSnapshot(trackIndex);
    FALSE_RETURN_MSG(snapshot != nullptr && snapshot->valid, "Stream snapshot is invalid");
    AVRational timeBase = snapshot->timeBase;
    if (firstPkt->pts != AV_NOPTS_VALUE) {
        sample->pts_ = AvTime2Us(ConvertTimeFromFFmpeg(firstPkt->pts, timeBase));
    }
    if (firstPkt->duration != AV_NOPTS_VALUE) {
        int64_t duration = AvTime2Us(ConvertTimeFromFFmpeg(firstPkt->duration, timeBase));
        sample->duration_ = duration;
        sample->meta_->SetData(Media::Tag::BUFFER_DURATION, duration);
    }
    if (firstPkt->dts != AV_NOPTS_VALUE) {
        int64_t dts = AvTime2Us(ConvertTimeFromFFmpeg(firstPkt->dts, timeBase));
        sample->dts_ = dts;
        sample->meta_->SetData(Media::Tag::BUFFER_DECODING_TIMESTAMP, dts);
    }

    if (snapshot->isVideo && snapshot->codecId != AV_CODEC_ID_H264 &&
        VideoFirstFrameValid(trackIndex)) {
        Plugins::AVPacketWrapperPtr firstFrameWrapper = videoFirstFrameMap_[trackIndex];
        AVPacket *firstFrame = (firstFrameWrapper != nullptr) ? firstFrameWrapper->GetAVPacket() : nullptr;
        if (firstFrame != nullptr && firstPkt->dts == firstFrame->dts && streamParsers_ != nullptr) {
            streamParsers_->ResetXPSSendStatus(trackIndex);
        }
    }

    if (std::find(
        g_fileContainSkipInfo.cbegin(), g_fileContainSkipInfo.cend(), fileType_) != g_fileContainSkipInfo.cend()) {
        uint8_t* skipInfoData = nullptr;
        size_t skipInfoDataSize = 0;
        skipInfoData = av_packet_get_side_data(firstPkt, AV_PKT_DATA_SKIP_SAMPLES, &skipInfoDataSize);
        if (skipInfoData != nullptr && skipInfoDataSize > 0) {
            std::vector<uint8_t> skipInfo(skipInfoDataSize);
            skipInfo.assign(skipInfoData, skipInfoData + skipInfoDataSize);
            sample->meta_->SetData(Tag::BUFFER_SKIP_SAMPLES_INFO, skipInfo);
        }
    }
}

Status FFmpegDemuxerPlugin::BufferIsValid(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket)
{
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr && samplePacket->pkts.size() > 0 &&
        samplePacket->pkts[0] != nullptr && samplePacket->pkts[0]->GetAVPacket() != nullptr &&
        samplePacket->pkts[0]->GetSize() >= 0,
        Status::ERROR_INVALID_OPERATION, "Input packet is nullptr or empty");
    uint32_t trackId = static_cast<uint32_t>(samplePacket->pkts[0]->GetStreamIndex());
    const AVStreamSnapshot* snapshot = GetStreamSnapshot(trackId);
    FALSE_RETURN_V_MSG_E(snapshot != nullptr && snapshot->valid,
        Status::ERROR_INVALID_OPERATION, "Stream snapshot is invalid");
    MEDIA_LOG_D("Convert packet info for track " PUBLIC_LOG_D32, samplePacket->pkts[0]->GetStreamIndex());
    // 0 mean sync read, 1 mean async read
    // sync read (0) need check memory_, async read (1) don't need check memory_
    bool isAsyncRead = (readModeMap_.find(1) != readModeMap_.end() && readModeMap_[1] == 1);
    if (isAsyncRead) {
        FALSE_RETURN_V_MSG_E(sample != nullptr && sample->meta_ != nullptr, Status::ERROR_INVALID_OPERATION,
            "Input sample is error");
    } else {
        FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr && sample->meta_ != nullptr,
            Status::ERROR_INVALID_OPERATION, "Input sample is nullptr");
        FALSE_RETURN_V_MSG_E(sample->memory_->GetCapacity() >= 0, Status::ERROR_INVALID_DATA,
            "Invalid capability[%{public}d]", sample->memory_->GetCapacity());
    }
    return Status::OK;
}

void FFmpegDemuxerPlugin::UpdateLastPacketInfo(int32_t trackId, int64_t pts, int64_t pos, int64_t duration)
{
    trackDfxInfoMap_[trackId].lastPts = pts;
    trackDfxInfoMap_[trackId].lastDuration = duration;
    trackDfxInfoMap_[trackId].lastPos = pos;
}

Status FFmpegDemuxerPlugin::PreparePacketForConversion(std::shared_ptr<AVBuffer> sample,
    std::shared_ptr<SamplePacket> samplePacket, Plugins::AVPacketWrapperPtr& tempPktWrapper, bool& combined)
{
    tempPktWrapper = CombinePackets(samplePacket);
    FALSE_RETURN_V_MSG_E(tempPktWrapper != nullptr && tempPktWrapper->GetAVPacket() != nullptr,
        Status::ERROR_INVALID_OPERATION, "Combine packets failed");
    combined = tempPktWrapper != nullptr && tempPktWrapper != samplePacket->pkts[0];
    if (cacheQueue_.ResetInfo(samplePacket) == false) {
        MEDIA_LOG_D("Reset info failed");
    }
    Status ret = ConvertPacketToAnnexb(sample, tempPktWrapper, samplePacket);
    if (ret != Status::OK && combined) {
        tempPktWrapper.reset();
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Convert annexb failed");
    if (cacheQueue_.SetInfo(samplePacket) == false) {
        MEDIA_LOG_D("Set info failed");
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::PrepareBufferMetadata(std::shared_ptr<AVBuffer> sample,
    std::shared_ptr<SamplePacket> samplePacket, Plugins::AVPacketWrapperPtr tempPktWrapper, uint32_t& copySize)
{
    FALSE_RETURN_V_MSG_E(tempPktWrapper->GetSize() >= 0 &&
        static_cast<uint32_t>(tempPktWrapper->GetSize()) >= samplePacket->offset, Status::ERROR_INVALID_DATA,
        "Invalid size[%{public}d] offset[%{public}u]", tempPktWrapper->GetSize(), samplePacket->offset);
    uint32_t remainSize = static_cast<uint32_t>(tempPktWrapper->GetSize()) - samplePacket->offset;
    uint32_t capability = static_cast<uint32_t>(sample->memory_->GetCapacity()); // memory_ is checked in BufferIsValid
    copySize = remainSize < capability ? remainSize : capability;
    MEDIA_LOG_D("Convert size [" PUBLIC_LOG_D32 "/" PUBLIC_LOG_U32 "/" PUBLIC_LOG_U32 "/" PUBLIC_LOG_U32 "]",
        tempPktWrapper->GetSize(), remainSize, copySize, samplePacket->offset);
    SetDrmCencInfo(sample, samplePacket);
    sample->flag_ = ConvertFlagsFromFFmpeg(*tempPktWrapper->GetAVPacket(),
        (copySize != static_cast<uint32_t>(tempPktWrapper->GetSize())));
    return Status::OK;
}

Status FFmpegDemuxerPlugin::WritePacketDataAndUpdateInfo(std::shared_ptr<AVBuffer> sample,
    std::shared_ptr<SamplePacket> samplePacket, Plugins::AVPacketWrapperPtr tempPktWrapper,
    uint32_t copySize, bool combined)
{
    Status ret = WriteBuffer(sample, tempPktWrapper->GetData() + samplePacket->offset, copySize);
    if (ret != Status::OK && combined) {
        tempPktWrapper.reset();
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Write buffer failed");
    if (!samplePacket->isEOS) {
        UpdateLastPacketInfo(tempPktWrapper->GetStreamIndex(), sample->pts_, tempPktWrapper->GetPos(),
            sample->duration_);
    }
    
#ifdef BUILD_ENG_VERSION
    DumpParam dumpParam {DumpMode(DUMP_AVBUFFER_OUTPUT & dumpMode_), tempPktWrapper->GetData() + samplePacket->offset,
        tempPktWrapper->GetStreamIndex(), -1, copySize, trackDfxInfoMap_[tempPktWrapper->GetStreamIndex()].frameIndex++,
        tempPktWrapper->GetPts(), -1};
    Dump(dumpParam);
#endif
    if (combined) {
        tempPktWrapper.reset();
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ConvertAVPacketToSample(
    std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket)
{
    Status bufferIsValid = BufferIsValid(sample, samplePacket);
    FALSE_RETURN_V_MSG_E(bufferIsValid == Status::OK, bufferIsValid, "AVBuffer or packet is invalid");
    WriteBufferAttr(sample, samplePacket);

    Plugins::AVPacketWrapperPtr tempPktWrapper;
    bool combined = false;
    Status ret = PreparePacketForConversion(sample, samplePacket, tempPktWrapper, combined);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Prepare packet for conversion failed");

    uint32_t copySize = 0;
    ret = PrepareBufferMetadata(sample, samplePacket, tempPktWrapper, copySize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Prepare buffer metadata failed");

    ret = WritePacketDataAndUpdateInfo(sample, samplePacket, tempPktWrapper, copySize, combined);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Write packet data failed");

    uint32_t remainSize = static_cast<uint32_t>(tempPktWrapper->GetSize()) - samplePacket->offset;
    if (copySize < remainSize) {
        FALSE_RETURN_V_MSG_E(samplePacket->offset <= UINT32_MAX - copySize, Status::ERROR_INVALID_DATA,
            "Invalid offset[%{public}u] copySize[%{public}u]", samplePacket->offset, copySize);
        samplePacket->offset += copySize;
        MEDIA_LOG_D("Buffer is not enough, next buffer to copy remain data");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::PushEOSToAllCache()
{
    Status ret = Status::OK;
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        auto streamIndex = selectedTrackIds_[i];
        MEDIA_LOG_I("Track " PUBLIC_LOG_D32, streamIndex);
        std::shared_ptr<SamplePacket> eosSample = std::make_shared<SamplePacket>();
        eosSample->isEOS = true;
        cacheQueue_.Push(streamIndex, eosSample);
        ret = CheckCacheDataLimit(streamIndex);
    }
    return ret;
}

bool FFmpegDemuxerPlugin::WebvttPktProcess(AVPacket *pkt)
{
    auto trackId = pkt->stream_index;
    if (pkt->size > 0) {    // vttc
        return false;
    }
    // vtte
    if (!cacheQueue_.HasCache(trackId)) {
        av_packet_unref(pkt);
        return true;
    }
    std::shared_ptr<SamplePacket> cacheSamplePacket = cacheQueue_.Back(static_cast<uint32_t>(trackId));
    if (cacheSamplePacket == nullptr || cacheSamplePacket->pkts.empty() || cacheSamplePacket->pkts[0] == nullptr) {
        av_packet_unref(pkt);
        return true;
    }
    Plugins::AVPacketWrapperPtr firstWrapper = cacheSamplePacket->pkts[0];
    AVPacket *firstPkt = firstWrapper != nullptr ? firstWrapper->GetAVPacket() : nullptr;
    if (firstPkt != nullptr && firstPkt->duration == 0 &&
        pkt->pts != AV_NOPTS_VALUE && firstPkt->pts != AV_NOPTS_VALUE) {
        firstPkt->duration = pkt->pts - firstPkt->pts;
    }
    av_packet_unref(pkt);
    return true;
}

bool FFmpegDemuxerPlugin::IsWebvttMP4(const AVStream *avStream)
{
    return (avStream->codecpar->codec_id == AV_CODEC_ID_WEBVTT && FFmpegFormatHelper::IsMpeg4File(fileType_));
}

void FFmpegDemuxerPlugin::WebvttMP4EOSProcess(const AVPacket *pkt)
{
    if (pkt == nullptr || pkt->size != 0) {
        return;
    }
    auto trackId = pkt->stream_index;
    AVStream *avStream = formatContext_->streams[trackId];
    if (!IsWebvttMP4(avStream) || !cacheQueue_.HasCache(trackId)) {
        return;
    }
    std::shared_ptr<SamplePacket> cacheSamplePacket = cacheQueue_.Back(static_cast<uint32_t>(trackId));
    if (cacheSamplePacket == nullptr || cacheSamplePacket->pkts.empty() || cacheSamplePacket->pkts[0] == nullptr) {
        return;
    }
    Plugins::AVPacketWrapperPtr firstWrapper = cacheSamplePacket->pkts[0];
    AVPacket *firstPkt = (firstWrapper != nullptr) ? firstWrapper->GetAVPacket() : nullptr;
    if (firstPkt != nullptr && firstPkt->duration == 0) {
        firstPkt->duration = formatContext_->streams[trackId]->duration - firstPkt->pts;
    }
}

void FFmpegDemuxerPlugin::ResetContext()
{
    formatContext_->pb->eof_reached = 0;
    formatContext_->pb->error = 0;
    ioContext_.retry = false;
}

bool FFmpegDemuxerPlugin::SelectedVideo()
{
    for (uint32_t index : selectedTrackIds_) {
        const AVStreamSnapshot* snapshot = GetStreamSnapshot(index);
        FALSE_RETURN_V_NOLOG(snapshot != nullptr && snapshot->valid, false);
        if (snapshot->codecType == AVMEDIA_TYPE_VIDEO) {
            return true;
        }
    }
    return false;
}

bool FFmpegDemuxerPlugin::NeedDropAfterSeek(uint32_t trackId, int64_t pts)
{
    FALSE_RETURN_V_NOLOG(seekTime_ != AV_NOPTS_VALUE && seekMode_ == SeekMode::SEEK_NEXT_SYNC, false);
    FALSE_RETURN_V_NOLOG(fileType_ != FileType::OGG && fileType_ != FileType::UNKNOW, false);
    const AVStreamSnapshot* snapshot = GetStreamSnapshot(trackId);
    FALSE_RETURN_V_NOLOG(snapshot != nullptr && snapshot->valid, false);
    FALSE_RETURN_V_NOLOG(snapshot->startTime != AV_NOPTS_VALUE, false);
    if (snapshot->startTime < 0) {
        FALSE_RETURN_V_NOLOG(pts <= INT64_MAX + snapshot->startTime, false);
    } else if (snapshot->startTime > 0) {
        FALSE_RETURN_V_NOLOG(pts >= INT64_MIN + snapshot->startTime, false);
    }
    if (!SelectedVideo() && snapshot->isAudio && // audio seek
        AvTime2Us(ConvertTimeFromFFmpeg(pts - snapshot->startTime, snapshot->timeBase)) < seekTime_ * MS_TO_US) {
        MEDIA_LOG_W("Seek frame behind time, drop");
        return true;
    }
    seekTime_ = AV_NOPTS_VALUE;
    return false;
}

int FFmpegDemuxerPlugin::AVReadFrameLimit(AVPacket *pkt)
{
    if (!ioContext_.isLimitType) {
        return av_read_frame(formatContext_.get(), pkt);
    }

    ioContext_.isLimit = true;
    int ffmpegRet = av_read_frame(formatContext_.get(), pkt);
    ioContext_.isLimit = false;
    ioContext_.readSizeCnt = 0;
    return ffmpegRet;
}

Status FFmpegDemuxerPlugin::ReadPacketToCacheQueue(const uint32_t readId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    Plugins::AVPacketWrapperPtr pktWrapper = nullptr;
    bool continueRead = true;
    Status ret = Status::OK;
    while (continueRead) {
        FALSE_RETURN_V(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE);
        if (!pktWrapper) {
            pktWrapper = std::make_shared<Plugins::AVPacketWrapper>();
            FALSE_RETURN_V_MSG_E(pktWrapper != nullptr && pktWrapper->GetAVPacket() != nullptr,
                Status::ERROR_NULL_POINTER, "Create AVPacketWrapper failed");
        }
        std::unique_lock<std::mutex> sLock(syncMutex_);
        int ffmpegRet = AVReadFrameLimit(pktWrapper->GetAVPacket());
        sLock.unlock();
        UpdMinTsPacketInfo(pktWrapper->GetAVPacket());
        if (ffmpegRet == AVERROR_EOF) { // eos
            WebvttMP4EOSProcess(pktWrapper->GetAVPacket());
            ret = PushEOSToAllCache();
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Push eos failed");
            return Status::END_OF_STREAM;
        }
        if (ffmpegRet < 0) { // fail
            MEDIA_LOG_E("Call av_read_frame failed:" PUBLIC_LOG_S ", retry: " PUBLIC_LOG_D32,
                AVStrError(ffmpegRet).c_str(), int(ioContext_.retry));
            return ioContext_.retry ? (ResetContext(), Status::ERROR_AGAIN) : Status::ERROR_UNKNOWN;
        }
        auto trackId = pktWrapper->GetStreamIndex();
        if (!TrackIsSelected(trackId) || NeedDropAfterSeek(trackId, pktWrapper->GetPts())) {
            av_packet_unref(pktWrapper->GetAVPacket());
            continue;
        }
        AVStream *avStream = formatContext_->streams[trackId];
        if (IsWebvttMP4(avStream) && WebvttPktProcess(pktWrapper->GetAVPacket())) {
            pktWrapper.reset();
            break;
        } else if (!IsWebvttMP4(avStream) && (!NeedCombineFrame(readId) ||
            (cacheQueue_.HasCache(static_cast<uint32_t>(trackId)) &&
            IsBeginAsAnnexb(pktWrapper->GetData(), pktWrapper->GetSize())))) {
            continueRead = false;
        }
        ret = AddPacketToCacheQueue(pktWrapper);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add cache failed");
        // 生命周期交由 SamplePacket 持有，这里清掉本地引用，方便下一轮重新分配
        pktWrapper.reset();
    }
    return ret;
}

Status FFmpegDemuxerPlugin::SetEosSample(std::shared_ptr<AVBuffer> sample)
{
    MEDIA_LOG_D("In");
    // 0 mean sync read, 1 mean async read
    // sync read (0) need check memory_, async read (1) don't need check memory_
    bool isAsyncRead = (readModeMap_.find(1) != readModeMap_.end() && readModeMap_[1] == 1);
    if (!isAsyncRead) {
        FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr, Status::ERROR_INVALID_PARAMETER,
            "AVBuffer or memory is nullptr");
    }
    sample->pts_ = 0;
    sample->flag_ = (uint32_t)(AVBufferFlag::EOS);
    if (isAsyncRead) {
        auto avPacketWrapper = std::make_shared<AVPacketWrapper>();
        FALSE_RETURN_V_MSG_E(avPacketWrapper != nullptr, Status::ERROR_INVALID_OPERATION, "Create pktWrapper failed");
        auto pkt = avPacketWrapper->GetAVPacket();
        int ret = av_new_packet(pkt, 1);
        FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_INVALID_OPERATION, "Call av_new_packet failed");
        auto avPacketMemory = std::make_shared<AVPacketMemory>(avPacketWrapper);
        FALSE_RETURN_V_MSG_E(avPacketMemory != nullptr, Status::ERROR_INVALID_OPERATION, "Create pktMemory failed");
        sample->memory_ = std::static_pointer_cast<Media::AVMemory>(avPacketMemory);
    } else {
        Status ret = WriteBuffer(sample, nullptr, 0);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Write buffer failed");
    }
    MEDIA_LOG_I("Out");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::Start()
{
    return Status::OK;
}

Status FFmpegDemuxerPlugin::Stop()
{
    return Status::OK;
}

// Write packet unimplemented, return 0
int FFmpegDemuxerPlugin::AVWritePacket(void* opaque, uint8_t* buf, int bufSize)
{
    (void)opaque;
    (void)buf;
    (void)bufSize;
    return 0;
}

int FFmpegDemuxerPlugin::CheckContextIsValid(void* opaque, int &bufSize)
{
    int ret = -1;
    auto ioContext = static_cast<IOContext*>(opaque);
    FALSE_RETURN_V_MSG_E(ioContext != nullptr, ret, "IOContext is nullptr");
    FALSE_RETURN_V_MSG_E(ioContext->dataSource != nullptr, ret, "DataSource is nullptr");
    FALSE_RETURN_V_MSG_E(ioContext->offset <= INT64_MAX - static_cast<int64_t>(bufSize), ret, "Offset invalid");

    if (ioContext->dataSource->IsDash() && ioContext->eos == true) {
        MEDIA_LOG_I("Read eos");
        return AVERROR_EOF;
    }

    MEDIA_LOG_D("Offset: " PUBLIC_LOG_D64 ", totalSize: " PUBLIC_LOG_U64, ioContext->offset, ioContext->fileSize);
    if (ioContext->fileSize > 0) {
        FALSE_RETURN_V_MSG_E(static_cast<uint64_t>(ioContext->offset) <= ioContext->fileSize, ret, "Out of file size");
        if (static_cast<size_t>(ioContext->offset + bufSize) > ioContext->fileSize) {
            bufSize = static_cast<int64_t>(ioContext->fileSize) - ioContext->offset;
        }
    }

    if (ioContext->isLimit) {
        if (bufSize > ioContext->sizeLimit - ioContext->readSizeCnt) {
            MEDIA_LOG_E("Read limit cur: " PUBLIC_LOG_D32 ", limit: " PUBLIC_LOG_D32 ", read: " PUBLIC_LOG_D32,
                ioContext->readSizeCnt, ioContext->sizeLimit, bufSize);
            return ret;
        }
        ioContext->readSizeCnt += bufSize;
    }
    return 0;
}

int64_t FFmpegDemuxerPlugin::AVSeek(void* opaque, int64_t offset, int whence)
{
    auto ioContext = static_cast<IOContext*>(opaque);
    uint64_t newPos = 0;
    FALSE_RETURN_V_MSG_E(ioContext != nullptr, newPos, "IOContext is nullptr");
    switch (whence) {
        case SEEK_SET:
            newPos = static_cast<uint64_t>(offset);
            ioContext->offset = newPos;
            MEDIA_LOG_D("Whence: " PUBLIC_LOG_D32 ", pos = " PUBLIC_LOG_D64 ", newPos = " PUBLIC_LOG_U64,
                whence, offset, newPos);
            break;
        case SEEK_CUR:
            newPos = ioContext->offset + offset;
            MEDIA_LOG_D("Whence: " PUBLIC_LOG_D32 ", pos = " PUBLIC_LOG_D64 ", newPos = " PUBLIC_LOG_U64,
                whence, offset, newPos);
            break;
        case SEEK_END:
        case AVSEEK_SIZE: {
            FALSE_RETURN_V_MSG_E(ioContext->dataSource != nullptr, newPos, "DataSource is nullptr");
            if (ioContext->dataSource->IsDash()) {
                return -1;
            }
            uint64_t mediaDataSize = 0;
            if (ioContext->dataSource->GetSize(mediaDataSize) == Status::OK && (mediaDataSize > 0)) {
                newPos = mediaDataSize + offset;
                MEDIA_LOG_D("Whence: " PUBLIC_LOG_D32 ", pos = " PUBLIC_LOG_D64 ", newPos = " PUBLIC_LOG_U64,
                    whence, offset, newPos);
            }
            break;
        }
        default:
            MEDIA_LOG_E("Unexpected whence " PUBLIC_LOG_D32, whence);
            break;
    }
    if (whence != AVSEEK_SIZE) {
        ioContext->offset = newPos;
    }
    MEDIA_LOG_D("Current offset: " PUBLIC_LOG_D64 ", new pos: " PUBLIC_LOG_U64, ioContext->offset, newPos);
    return newPos;
}

AVIOContext* FFmpegDemuxerPlugin::AllocAVIOContext(int flags, IOContext *ioContext)
{
    auto buffer = static_cast<unsigned char*>(av_malloc(DEFAULT_READ_SIZE));
    FALSE_RETURN_V_MSG_E(buffer != nullptr, nullptr, "Call av_malloc failed");

    AVIOContext* avioContext = avio_alloc_context(
        buffer, DEFAULT_READ_SIZE,
        static_cast<int>(static_cast<uint32_t>(flags) & static_cast<uint32_t>(AVIO_FLAG_WRITE)),
        static_cast<void*>(ioContext),
        AVReadPacket, AVWritePacket, AVSeek);
    if (avioContext == nullptr) {
        MEDIA_LOG_E("Call avio_alloc_context failed");
        av_free(buffer);
        return nullptr;
    }
    avioContext->seekable = (seekable_ == Seekable::SEEKABLE) ? AVIO_SEEKABLE_NORMAL : 0;
    if (!(static_cast<uint32_t>(flags) & static_cast<uint32_t>(AVIO_FLAG_WRITE))) {
        avioContext->buf_ptr = avioContext->buf_end;
        avioContext->write_flag = 0;
    }
    return avioContext;
}

void FreeContext(AVFormatContext* formatContext, AVIOContext* avioContext)
{
    if (formatContext) {
        avformat_close_input(&formatContext);
    }
    if (avioContext) {
        if (avioContext->buffer) {
            av_freep(&(avioContext->buffer));
        }
        avio_context_free(&avioContext);
    }
}

int32_t ParseHeader(AVFormatContext* formatContext, std::shared_ptr<AVInputFormat> pluginImpl, AVDictionary **options)
{
    FALSE_RETURN_V_MSG_E(formatContext && pluginImpl, -1, "AVFormatContext is nullptr");
    MediaAVCodec::AVCodecTrace trace("ffmpeg_init");

    AVIOContext* avioContext = formatContext->pb;
    auto begin = std::chrono::steady_clock::now();
    int ret = avformat_open_input(&formatContext, nullptr, pluginImpl.get(), options);
    if (ret < 0) {
        FreeContext(formatContext, avioContext);
        MEDIA_LOG_E("Call avformat_open_input failed by " PUBLIC_LOG_S ", err:" PUBLIC_LOG_S,
            pluginImpl->name, AVStrError(ret).c_str());
        return ret;
    }

    auto open = std::chrono::steady_clock::now();
    if (FFmpegFormatHelper::GetFileTypeByName(*formatContext) == FileType::FLV) { // Fix init live-flv-source too slow
        formatContext->probesize = LIVE_FLV_PROBE_SIZE;
    }

    ret = avformat_find_stream_info(formatContext, NULL);
    auto parse = std::chrono::steady_clock::now();
    int64_t openSpend = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(open - begin).count());
    int64_t parseSpend = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(parse - open).count());
    if ((parseSpend < 0) || (openSpend > INT64_MAX - parseSpend) || (openSpend + parseSpend > INIT_TIME_THRESHOLD)) {
        MEDIA_LOG_W("Spend [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "]", openSpend, parseSpend);
    }
    if (ret < 0) {
        FreeContext(formatContext, avioContext);
        MEDIA_LOG_E("Parse stream info failed by " PUBLIC_LOG_S ", err:" PUBLIC_LOG_S,
            pluginImpl->name, AVStrError(ret).c_str());
        return ret;
    }
    return 0;
}

std::shared_ptr<AVFormatContext> FFmpegDemuxerPlugin::InitAVFormatContext(IOContext *ioContext)
{
    AVFormatContext* formatContext = avformat_alloc_context();
    FALSE_RETURN_V_MSG_E(formatContext != nullptr, nullptr, "AVFormatContext is nullptr");

    formatContext->pb = AllocAVIOContext(AVIO_FLAG_READ, ioContext);
    if (formatContext->pb == nullptr) {
        FreeContext(formatContext, nullptr);
        return nullptr;
    }

    formatContext->flags = static_cast<uint32_t>(formatContext->flags) | static_cast<uint32_t>(AVFMT_FLAG_CUSTOM_IO);
    if (std::string(pluginImpl_->name) == PLUGIN_NAME_MP3) {
        formatContext->flags =
            static_cast<uint32_t>(formatContext->flags) | static_cast<uint32_t>(AVFMT_FLAG_FAST_SEEK);
    }
    AVDictionary *options = nullptr;
    if (ioContext_.dataSource->IsDash()) {
        av_dict_set(&options, "use_tfdt", "true", 0);
    }
    
    int ret = ParseHeader(formatContext, pluginImpl_, &options);
    av_dict_free(&options);
    FALSE_RETURN_V_MSG_E(ret >= 0, nullptr, "ParseHeader failed");

    std::shared_ptr<AVFormatContext> retFormatContext =
        std::shared_ptr<AVFormatContext>(formatContext, [](AVFormatContext *ptr) {
            if (ptr) {
                auto ctx = ptr->pb;
                avformat_close_input(&ptr);
                if (ctx) {
                    ctx->opaque = nullptr;
                    av_freep(&(ctx->buffer));
                    av_opt_free(ctx);
                    avio_context_free(&ctx);
                    ctx = nullptr;
                }
            }
        });
    return retFormatContext;
}

void FFmpegDemuxerPlugin::NotifyInitializationCompleted()
{
    ioContext_.initCompleted = true;
    if (ioContext_.initDownloadDataSize >= INIT_DOWNLOADS_DATA_SIZE_THRESHOLD) {
        MEDIA_LOG_I("Large init size %{public}u", ioContext_.initDownloadDataSize);
    }
}

void FFmpegDemuxerPlugin::InitIoContextInDemuxer(const std::shared_ptr<DataSource>& source)
{
    ioContext_.dataSource = source;
    ioContext_.offset = 0;
    ioContext_.eos = false;
    ioContext_.dumpMode = dumpMode_;
    seekable_ = ioContext_.dataSource->IsDash() ? Plugins::Seekable::UNSEEKABLE : source->GetSeekable();
    if (seekable_ == Plugins::Seekable::SEEKABLE) {
        ioContext_.dataSource->GetSize(ioContext_.fileSize);
    } else {
        ioContext_.fileSize = -1;
    }
    MEDIA_LOG_I("FileSize: " PUBLIC_LOG_U64 ", seekable: " PUBLIC_LOG_D32, ioContext_.fileSize, seekable_);
}

Status FFmpegDemuxerPlugin::SetDataSource(const std::shared_ptr<DataSource>& source)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ == nullptr, Status::ERROR_WRONG_STATE,
        "AVFormatContext has been initialized");
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "DataSource is nullptr");
    if (ioContext_.invokerType != InvokerType::INIT) {
        std::lock_guard<std::mutex> initLock(ioContext_.invokerTypeMutex);
        ioContext_.invokerType = InvokerType::INIT;
    }
    auto id = HiviewDFX::XCollie::GetInstance().SetTimer("av_codec::demuxer_setdatasource", SETTIMER_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG);
    InitIoContextInDemuxer(source);
    {
        std::lock_guard<std::mutex> glock(g_mtx);
        auto inputFormat = av_find_input_format(ProcessPluginName(pluginName_).c_str());
        pluginImpl_ = std::shared_ptr<AVInputFormat>(const_cast<AVInputFormat*>(inputFormat), [](void*) {});
    }
    FALSE_RETURN_V_MSG_E(pluginImpl_ != nullptr, Status::ERROR_UNSUPPORTED_FORMAT, "No match inputformat");
    formatContext_ = InitAVFormatContext(&ioContext_);
    if (formatContext_ == nullptr) {
        if (ioContext_.retry) {
            MEDIA_LOG_E("SetDataSource failed cause not enough data");
            return Status::ERROR_NOT_ENOUGH_DATA;
        }
        MEDIA_LOG_E("AVFormatContext is nullptr");
        return Status::ERROR_UNKNOWN;
    }
    fileType_ = FFmpegFormatHelper::GetFileTypeByName(*formatContext_);
    InitParser();

    SetAVReadFrameLimitDefault();

    // parse media info
    GetMediaInfo();

    // Create stream snapshots to avoid data race when accessing formatContext_->streams
    UpdateStreamSnapshots();

    SetAVReadFrameLimit();

    // check param
    if (ioContext_.retry && !HasCodecParameters()) {
        ResetParam();
        MEDIA_LOG_E("SetDataSource failed cause not enough data");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    NotifyInitializationCompleted();
    MEDIA_LOG_I("Out");
    cachelimitSize_ = DEFAULT_CACHE_LIMIT;
    if (ioContext_.initErrorAgain == true && formatContext_->pb->error == -1) { // -1 means error_again during init
        MEDIA_LOG_E("Initialization error_again occurred");
        ResetContext();
        ioContext_.initErrorAgain = false;
    }
    HiviewDFX::XCollie::GetInstance().CancelTimer(id);
    return Status::OK;
}

void FFmpegDemuxerPlugin::UpdateStreamSnapshots()
{
    streamSnapshots_.clear();
    if (formatContext_ == nullptr) {
        return;
    }
    streamSnapshots_.reserve(formatContext_->nb_streams);
    for (uint32_t i = 0; i < formatContext_->nb_streams; ++i) {
        AVStreamSnapshot snapshot;
        AVStream* stream = formatContext_->streams[i];
        if (stream != nullptr && stream->codecpar != nullptr) {
            snapshot.valid = true;
            snapshot.codecId = stream->codecpar->codec_id;
            snapshot.codecType = stream->codecpar->codec_type;
            snapshot.timeBase = stream->time_base;
            snapshot.extradataSize = stream->codecpar->extradata_size;
            snapshot.startTime = stream->start_time;
            snapshot.isVideo = FFmpegFormatHelper::IsVideoType(*stream);
            snapshot.isAudio = FFmpegFormatHelper::IsAudioType(*stream);
            snapshot.needCombineFrame =
                (fileType_ == FileType::MPEGTS && snapshot.codecId == AV_CODEC_ID_HEVC);
        }
        streamSnapshots_.emplace_back(snapshot);
    }
}

const FFmpegDemuxerPlugin::AVStreamSnapshot* FFmpegDemuxerPlugin::GetStreamSnapshot(uint32_t trackId) const
{
    if (trackId >= streamSnapshots_.size()) {
        return nullptr;
    }
    return &streamSnapshots_[trackId];
}

static bool CheckProbScore(const std::string& pluginName, const int32_t probScore)
{
    if (probScore >= DEF_PROBE_SCORE_LIMIT) {
        return true;
    }

    std::string pluginType = pluginName.substr(std::string(PLUGIN_NAME_PREFIX).size());
    if (StartWith(pluginType.c_str(), PLUGIN_NAME_MP3) && probScore > MP3_PROBE_SCORE_LIMIT) {
        return true;
    }

    return false;
}

Status FFmpegDemuxerPlugin::SetDataSourceWithProbSize(const std::shared_ptr<DataSource>& source,
    const int32_t probSize)
{
    std::unique_lock<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ == nullptr, Status::ERROR_WRONG_STATE,
        "AVFormatContext has been initialized");
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "DataSource is nullptr");
    FALSE_RETURN_V_MSG_E(probSize >= 0, Status::ERROR_INVALID_PARAMETER, "probSize is invalid");
    
    int32_t probScore = SniffWithSize(pluginName_, source, probSize);
    FALSE_RETURN_V_MSG_E(CheckProbScore(pluginName_, probScore), Status::ERROR_INVALID_PARAMETER,
        "source and inputformat mismatch");
    lock.unlock();

    return SetDataSource(source);
}

bool FFmpegDemuxerPlugin::HasCodecParameters()
{
    int32_t param;
    FALSE_RETURN_V_MSG_E(static_cast<size_t>(formatContext_->nb_streams) == mediaInfo_.tracks.size(), false,
        "mediaInfo is error");
    for (uint32_t i = 0; i < formatContext_->nb_streams; i++) {
        auto avStream = formatContext_->streams[i];
        FALSE_RETURN_V_MSG_E(avStream != nullptr && avStream->codecpar != nullptr, false, "AVStream is nullptr");
        Meta &format = mediaInfo_.tracks[i];
        bool flag = !HaveValidParser(avStream->codecpar->codec_id) ||
            (HaveValidParser(avStream->codecpar->codec_id) && streamParsers_ != nullptr);
        if (FFmpegFormatHelper::IsAudioType(*avStream)) {
            FALSE_RETURN_V_MSG_E(format.GetData(Tag::AUDIO_CHANNEL_COUNT, param), false,
                "unspecified channel_count");
            FALSE_RETURN_V_MSG_E(format.GetData(Tag::AUDIO_SAMPLE_RATE, param), false, "unspecified sample_rate");
        } else if (FFmpegFormatHelper::IsVideoType(*avStream)) {
            FALSE_RETURN_V_MSG_E(flag && format.GetData(Tag::VIDEO_WIDTH, param), false, "unspecified width");
            FALSE_RETURN_V_MSG_E(flag && format.GetData(Tag::VIDEO_HEIGHT, param), false, "unspecified height");
        }
    }
    return true;
}

void FFmpegDemuxerPlugin::InitParser()
{
    FALSE_RETURN_MSG(formatContext_ != nullptr, "AVFormatContext is nullptr");
    ParserBoxInfo();
    streamParsers_ = std::make_shared<MultiStreamParserManager>();
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        if (formatContext_->streams[trackIndex] == nullptr ||
            formatContext_->streams[trackIndex]->codecpar == nullptr) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " info is nullptr", trackIndex);
            continue;
        }
        if (g_bitstreamFilterMap.count(formatContext_->streams[trackIndex]->codecpar->codec_id) != 0) {
            InitBitStreamContext(*(formatContext_->streams[trackIndex]));
            continue;
        }
        if (HaveValidParser(formatContext_->streams[trackIndex]->codecpar->codec_id) && streamParsers_ != nullptr) {
            Status ret = streamParsers_->Create(
                trackIndex, g_streamParserMap.at(formatContext_->streams[trackIndex]->codecpar->codec_id));
            if (ret != Status::OK) {
                MEDIA_LOG_W("Init failed");
            } else {
                MEDIA_LOG_D("Track " PUBLIC_LOG_D32 " will be converted", trackIndex);
            }
        }
    }
}

Status FFmpegDemuxerPlugin::GetMediaInfo(MediaInfo& mediaInfo)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    mediaInfo = mediaInfo_;
    return Status::OK;
}

void FFmpegDemuxerPlugin::SetAVReadFrameLimitDefault()
{
    if (!FFmpegFormatHelper::IsMpeg4File(fileType_)) {
        return;
    }
    ioContext_.isLimitType = true;
    ioContext_.sizeLimit = READ_SIZE_LIMIT_DEFAULT;
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        if (formatContext_->streams[trackIndex] == nullptr ||
            formatContext_->streams[trackIndex]->codecpar == nullptr) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " info is nullptr", trackIndex);
            continue;
        }
        if (FFmpegFormatHelper::IsVideoType(*(formatContext_->streams[trackIndex]))) {
            auto codecpar = formatContext_->streams[trackIndex]->codecpar;
            int32_t limitSize = codecpar->width * codecpar->height * DEFAULT_CHANNEL_CNT * READ_SIZE_LIMIT_FACTOR;
            ioContext_.sizeLimit = std::max(ioContext_.sizeLimit, limitSize);
            MEDIA_LOG_D("Track " PUBLIC_LOG_U32 " hei:" PUBLIC_LOG_D32 ", wid:" PUBLIC_LOG_D32
                " limit " PUBLIC_LOG_D32, trackIndex, codecpar->height, codecpar->width, limitSize);
        }
    }
    return;
}

Status FFmpegDemuxerPlugin::SetAVReadFrameLimit()
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    if (fileType_ != FileType::FLV && !FFmpegFormatHelper::IsMpeg4File(fileType_)) {
        return Status::OK;
    }

    ioContext_.isLimitType = true;
    ioContext_.sizeLimit = READ_SIZE_LIMIT_DEFAULT;
    FALSE_RETURN_V_MSG_E(static_cast<size_t>(formatContext_->nb_streams) == mediaInfo_.tracks.size(), Status::OK,
        "mediaInfo size is error");
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        if (formatContext_->streams[trackIndex] == nullptr) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " info is nullptr", trackIndex);
            continue;
        }
        if (FFmpegFormatHelper::IsVideoType(*(formatContext_->streams[trackIndex]))) {
            int width = 0;
            int height = 0;
            Meta &format = mediaInfo_.tracks[trackIndex];
            format.GetData(Tag::VIDEO_WIDTH, width);
            format.GetData(Tag::VIDEO_HEIGHT, height);
            if (width * height > 0) {
                int32_t limitSize = width * height * DEFAULT_CHANNEL_CNT * READ_SIZE_LIMIT_FACTOR;
                ioContext_.sizeLimit = std::max(ioContext_.sizeLimit, limitSize);
                MEDIA_LOG_D("Track " PUBLIC_LOG_U32 " hei:" PUBLIC_LOG_D32 ", wid:" PUBLIC_LOG_D32
                    " limit " PUBLIC_LOG_D32, trackIndex, height, width, limitSize);
            }
        }
    }
    return Status::OK;
}

void FFmpegDemuxerPlugin::UpdateReferenceIds()
{
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        if (trackIndex >= mediaInfo_.tracks.size()) {
            break;
        }
        Meta meta = mediaInfo_.tracks[trackIndex];
        std::vector<int32_t> referenceIds;
        meta.Get<Tag::REFERENCE_TRACK_IDS>(referenceIds);
        for (auto id : referenceIds) {
            if (referenceIdsMap_.count(id) == 0) {
                referenceIdsMap_[id] = std::vector<int32_t>();
            }
            if (!std::any_of(referenceIdsMap_[id].begin(), referenceIdsMap_[id].end(),
                [trackIndex](int32_t i) { return static_cast<uint32_t>(i) == trackIndex; })) {
                referenceIdsMap_[id].push_back(trackIndex);
            }

            if (referenceIdsMap_.count(trackIndex) == 0) {
                referenceIdsMap_[trackIndex] = std::vector<int32_t>();
            }
            if (!std::any_of(referenceIdsMap_[trackIndex].begin(), referenceIdsMap_[trackIndex].end(),
                [id](int32_t i) { return i == id; })) {
                referenceIdsMap_[trackIndex].push_back(id);
            }
        }
    }
    for (auto item : referenceIdsMap_) {
        if (item.second.size() == 0 || static_cast<size_t>(item.first) >= mediaInfo_.tracks.size()) {
            continue;
        }
        for (auto id : item.second) {
            MEDIA_LOG_D("Track " PUBLIC_LOG_D32 " ref " PUBLIC_LOG_D32, item.first, id);
        }
        mediaInfo_.tracks[item.first].Set<Tag::REFERENCE_TRACK_IDS>(item.second);
    }
}

void FFmpegDemuxerPlugin::GetStreamInitialParams()
{
    FALSE_RETURN_MSG_W(formatContext_ != nullptr, "AVFormatContext is nullptr");
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        auto stream = formatContext_->streams[trackIndex];
        if (stream == nullptr || stream->codecpar == nullptr) {
            continue;
        }
        Meta format;
        int64_t bitRate = static_cast<int64_t>(stream->codecpar->bit_rate);
        if (bitRate > 0) {
            format.Set<Tag::MEDIA_BITRATE>(bitRate);
        } else {
            MEDIA_LOG_D("Track " PUBLIC_LOG_D32 " bitrate parse failed", trackIndex);
        }
        streamInitialParam_[trackIndex] = format;
    }
}

void FFmpegDemuxerPlugin::SetStreamInitialParams(uint32_t trackId, Meta &format)
{
    FALSE_RETURN_MSG_W(streamInitialParam_.count(trackId) > 0, "TrackId is invalid");
    int64_t bitRate = 0;
    bool ret = streamInitialParam_[trackId].GetData(Tag::MEDIA_BITRATE, bitRate);
    if (!ret || bitRate <= 0) {
        MEDIA_LOG_W("Track " PUBLIC_LOG_D32 " has no bitrate", trackId);
    } else {
        format.Set<Tag::MEDIA_BITRATE>(bitRate);
    }
}

void FFmpegDemuxerPlugin::ProcessHevcFirstFrame(uint32_t trackId, AVStream* avStream, bool parserReady,
    bool firstFrameReady, Meta &meta)
{
    if (!parserReady) {
        MEDIA_LOG_W("Parse hevc info failed: parser not ready");
        return;
    }
    if (!firstFrameReady) {
        MEDIA_LOG_W("Parse hevc info failed");
        return;
    }
    Plugins::AVPacketWrapperPtr firstWrapper = videoFirstFrameMap_[trackId];
    AVPacket *firstFrame = (firstWrapper != nullptr) ? firstWrapper->GetAVPacket() : nullptr;
    if (firstFrame == nullptr) {
        MEDIA_LOG_W("First frame is nullptr");
        return;
    }

    // Parser only sends xps info when first call ConvertPacketToAnnexb
    // readSample will call ConvertPacketToAnnexb again, so rest here
    PacketConvertInfo convertInfo {nullptr, 0, false};
    streamParsers_->ConvertPacketToAnnexb(trackId, &(firstFrame->data), firstFrame->size, convertInfo);
    streamParsers_->ParseAnnexbExtraData(trackId, firstFrame->data, firstFrame->size);
    streamParsers_->ResetXPSSendStatus(trackId); // only send xps once
    ParseHEVCMetadataInfo(*avStream, meta);
}

void FFmpegDemuxerPlugin::BuildTrackMeta(uint32_t trackId, AVStream* avStream)
{
    Meta meta;
    if (avStream == nullptr) {
        MEDIA_LOG_W("Track " PUBLIC_LOG_D32 " info is nullptr", trackId);
        mediaInfo_.tracks.push_back(meta);
        return;
    }

    FFmpegFormatHelper::ParseTrackInfo(*avStream, meta, *formatContext_);
    bool isHevc = (avStream->codecpar->codec_id == AV_CODEC_ID_HEVC);
    bool isH264 = (avStream->codecpar->codec_id == AV_CODEC_ID_H264);
    bool isVvc = (avStream->codecpar->codec_id == AV_CODEC_ID_VVC);
    bool parserReady = streamParsers_ != nullptr && streamParsers_->ParserIsInited(trackId);
    bool firstFrameReady = parserReady && VideoFirstFrameValid(trackId);

    if (isHevc) {
        ProcessHevcFirstFrame(trackId, avStream, parserReady, firstFrameReady, meta);
    }
    if (isHevc || isH264 || isVvc) {
        ConvertCsdToAnnexb(*avStream, meta);
    }

    SetStreamInitialParams(trackId, meta);
    mediaInfo_.tracks.push_back(meta);
    DemuxerLogCompressor::StringifyMeta(meta, trackId);
}

Status FFmpegDemuxerPlugin::GetMediaInfo()
{
    MediaAVCodec::AVCodecTrace trace("FFmpegDemuxerPlugin::GetMediaInfo");
    GetStreamInitialParams();
    Status ret = ParseVideoFirstFrames();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse video info failed");

    ret = GetFileFirstPacket();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Get file first packet failed");

    FFmpegFormatHelper::ParseMediaInfo(*formatContext_, mediaInfo_.general);
    DemuxerLogCompressor::StringifyMeta(mediaInfo_.general, -1); // source meta
    for (uint32_t trackId = 0; trackId < formatContext_->nb_streams; ++trackId) {
        BuildTrackMeta(trackId, formatContext_->streams[trackId]);
    }
    UpdateReferenceIds();
    return Status::OK;
}

Status FFmpegDemuxerPlugin::GetUserMeta(std::shared_ptr<Meta> meta)
{
    MediaAVCodec::AVCodecTrace trace("FFmpegDemuxerPlugin::GetUserMeta");
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(meta != nullptr, Status::ERROR_NULL_POINTER, "Meta is nullptr");
    
    FFmpegFormatHelper::ParseUserMeta(*formatContext_, meta);
    return Status::OK;
}

void FFmpegDemuxerPlugin::ParseDrmInfo(const MetaDrmInfo *const metaDrmInfo, size_t drmInfoSize,
    std::multimap<std::string, std::vector<uint8_t>>& drmInfo)
{
    MEDIA_LOG_D("In");
    size_t infoCount = drmInfoSize / sizeof(MetaDrmInfo);
    for (size_t index = 0; index < infoCount; index++) {
        std::stringstream ssConverter;
        std::string uuid;
        for (uint32_t i = 0; i < metaDrmInfo[index].uuidLen; i++) {
            int32_t singleUuid = static_cast<int32_t>(metaDrmInfo[index].uuid[i]);
            ssConverter << std::hex << std::setfill('0') << std::setw(2) << singleUuid; // 2:w
            uuid = ssConverter.str();
        }
        drmInfo.insert({ uuid, std::vector<uint8_t>(metaDrmInfo[index].pssh,
            metaDrmInfo[index].pssh + metaDrmInfo[index].psshLen) });
    }
}

Status FFmpegDemuxerPlugin::GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo)
{
    MEDIA_LOG_D("In");
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");

    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        Meta meta;
        AVStream *avStream = formatContext_->streams[trackIndex];
        if (avStream == nullptr) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_D32 " is nullptr", trackIndex);
            continue;
        }
        MEDIA_LOG_D("GetDrmInfo by stream side data");
        size_t drmInfoSize = 0;
        MetaDrmInfo *tmpDrmInfo = (MetaDrmInfo *)av_stream_get_side_data(avStream,
            AV_PKT_DATA_ENCRYPTION_INIT_INFO, &drmInfoSize);
        if (tmpDrmInfo != nullptr && drmInfoSize != 0) {
            ParseDrmInfo(tmpDrmInfo, drmInfoSize, drmInfo);
        }
    }
    return Status::OK;
}

void FFmpegDemuxerPlugin::ConvertCsdToAnnexb(const AVStream& avStream, Meta &format)
{
    uint8_t *extradata = avStream.codecpar->extradata;
    int32_t extradataSize = avStream.codecpar->extradata_size;
    if (HaveValidParser(avStream.codecpar->codec_id) && streamParsers_ != nullptr &&
        streamParsers_->ParserIsInited(avStream.index)) {
        PacketConvertInfo convertInfo {nullptr, 0, true};
        streamParsers_->ConvertPacketToAnnexb(avStream.index, &(extradata), extradataSize, convertInfo);
    } else if (avStream.codecpar->codec_id == AV_CODEC_ID_H264 &&
        avbsfContexts_.count(avStream.index) > 0 && avbsfContexts_[avStream.index] != nullptr &&
        avbsfContexts_[avStream.index]->par_out->extradata != nullptr &&
        avbsfContexts_[avStream.index]->par_out->extradata_size > 0) {
            extradata = avbsfContexts_[avStream.index]->par_out->extradata;
            extradataSize = avbsfContexts_[avStream.index]->par_out->extradata_size;
    }
    if (extradata != nullptr && extradataSize > 0) {
        std::vector<uint8_t> extra(extradataSize);
        extra.assign(extradata, extradata + extradataSize);
        format.Set<Tag::MEDIA_CODEC_CONFIG>(extra);
    }
}

Status FFmpegDemuxerPlugin::AddPacketToCacheQueue(Plugins::AVPacketWrapperPtr pktWrapper)
{
    FALSE_RETURN_V_MSG_E(pktWrapper != nullptr && pktWrapper->GetAVPacket() != nullptr,
        Status::ERROR_NULL_POINTER, "Pkt is nullptr");
#ifdef BUILD_ENG_VERSION
    DumpParam dumpParam {DumpMode(DUMP_AVPACKET_OUTPUT & dumpMode_), pktWrapper->GetData(),
        pktWrapper->GetStreamIndex(), -1, pktWrapper->GetSize(), avpacketIndex_++, pktWrapper->GetPts(),
        pktWrapper->GetPos()};
    Dump(dumpParam);
#endif
    auto trackId = pktWrapper->GetStreamIndex();
    Status ret = Status::OK;
    if (NeedCombineFrame(trackId) && !IsBeginAsAnnexb(pktWrapper->GetData(), pktWrapper->GetSize())
        && cacheQueue_.HasCache(trackId)) {
        std::shared_ptr<SamplePacket> cacheSamplePacket = cacheQueue_.Back(static_cast<uint32_t>(trackId));
        if (cacheSamplePacket != nullptr) {
            cacheSamplePacket->pkts.push_back(pktWrapper);
        }
    } else {
        std::shared_ptr<SamplePacket> cacheSamplePacket = std::make_shared<SamplePacket>();
        if (cacheSamplePacket != nullptr) {
            cacheSamplePacket->pkts.push_back(pktWrapper);
            cacheSamplePacket->offset = 0;
            cacheQueue_.Push(static_cast<uint32_t>(trackId), cacheSamplePacket);
            ret = CheckCacheDataLimit(static_cast<uint32_t>(trackId));
        }
    }
    return ret;
}

Status FFmpegDemuxerPlugin::SetVideoFirstFrame(Plugins::AVPacketWrapperPtr pktWrapper, bool isConvert)
{
    AVPacket *pkt = (pktWrapper != nullptr) ? pktWrapper->GetAVPacket() : nullptr;
    FALSE_RETURN_V_MSG_E(pkt != nullptr, Status::ERROR_NULL_POINTER, "Pkt is nullptr");

    Plugins::AVPacketWrapperPtr firstWrapper = std::make_shared<Plugins::AVPacketWrapper>();
    FALSE_RETURN_V_MSG_E(firstWrapper != nullptr && firstWrapper->GetAVPacket() != nullptr,
        Status::ERROR_NULL_POINTER, "Create AVPacketWrapper failed");
    AVPacket *firstFrameRaw = firstWrapper->GetAVPacket();
    int32_t avRet = av_new_packet(firstFrameRaw, pkt->size);
    FALSE_RETURN_V_MSG_E(avRet >= 0, Status::ERROR_INVALID_DATA, "Call av_new_packet failed");
    avRet = av_packet_copy_props(firstFrameRaw, pkt);
    FALSE_RETURN_V_MSG_E(avRet >= 0, Status::ERROR_INVALID_DATA, "Call av_packet_copy_props failed");
    auto ret = memcpy_s(firstFrameRaw->data, firstFrameRaw->size, pkt->data, pkt->size);
    if (ret != EOK) {
        MEDIA_LOG_E("Memcpy failed, ret:" PUBLIC_LOG_D32, ret);
        return Status::ERROR_INVALID_DATA;
    }
    FALSE_RETURN_V_MSG_E(firstFrameRaw->data != nullptr, Status::ERROR_INVALID_DATA, "Get first frame failed");
    if (isConvert) {
        bool convertRet = streamParsers_->ConvertExtraDataToAnnexb(pkt->stream_index,
            formatContext_->streams[pkt->stream_index]->codecpar->extradata,
            formatContext_->streams[pkt->stream_index]->codecpar->extradata_size);
        FALSE_RETURN_V_MSG_E(convertRet, Status::ERROR_INVALID_DATA, "ConvertExtraDataToAnnexb failed: " PUBLIC_LOG_D32,
            pkt->stream_index);
    }
    videoFirstFrameMap_[pkt->stream_index] = firstWrapper;
    if (pkt->pts != AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE && pkt->pts >= 0 && pkt->dts >= 0) {
        seekCalibMap_[pkt->stream_index] = pkt->pts - pkt->dts;
    }
    return Status::OK;
}

static bool IsSyncFrameCheckNeeded(std::shared_ptr<AVFormatContext> formatContext)
{
    FALSE_RETURN_V_MSG_E(formatContext != nullptr, false, "AVFormatContext is nullptr");
    FileType fileType = FFmpegFormatHelper::GetFileTypeByName(*formatContext);

    return (std::find(g_streamCheckFileTypeVec.cbegin(), g_streamCheckFileTypeVec.cend(),
        fileType) == g_streamCheckFileTypeVec.cend());
}

bool FFmpegDemuxerPlugin::Mp4CheckKeyFrame(AVStream* stream)
{
    FALSE_RETURN_V_MSG_E(stream != nullptr, false, "AVStream is nullptr");
    const AVIndexEntry *entry = avformat_index_get_entry(stream, POS_0);
    FALSE_RETURN_V_MSG_E(entry != nullptr, false, "First AVIndexEntry is nullptr");
    int keyIndex = av_index_search_timestamp(stream, entry->timestamp, AVINDEX_KEYFRAME);
    FALSE_RETURN_V_MSG_E(keyIndex >= 0, false, "KeyIndex is invalid");
    return true;
}

bool FFmpegDemuxerPlugin::AllSupportTrackFramesReady()
{
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        AVStream* stream = formatContext_->streams[trackIndex];
        if (stream == nullptr || stream->codecpar == nullptr || !IsSupportedGetFirstFrame(*stream)) {
            continue;
        }
        if (FFmpegFormatHelper::IsMpeg4File(fileType_) && !Mp4CheckKeyFrame(stream)) {
            continue;
        }
        if (!TrackIsChecked(trackIndex)) {
            return false;
        }
    }
    return true;
}

bool FFmpegDemuxerPlugin::AllVideoFirstFramesReady()
{
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        AVStream* stream = formatContext_->streams[trackIndex];
        if (stream == nullptr || stream->codecpar == nullptr || !FFmpegFormatHelper::IsVideoType(*stream)) {
            continue;
        }
        if (!TrackIsChecked(trackIndex)) {
            return false;
        }
    }
    return true;
}

/*
return true:
  unsupport track
  support track:
    packet with key FLAG
    packet without key FLAG:
      HEVC + TS/PS file
      HEVC + IsHevcSyncFrame pass
return false:
  support track + without key FLAG + not hevc track
  support track + without key FLAG + hevc track + not ts/ps + IsHevcSyncFrame fail
*/
static bool IsSyncFrame(AVStream *stream, AVPacket *pkt, std::shared_ptr<AVFormatContext> formatContext)
{
    FALSE_RETURN_V_MSG_E(stream != nullptr && stream->codecpar != nullptr, false, "stream is nullptr");
    FALSE_RETURN_V_MSG_E(pkt != nullptr, false, "pkt is nullptr");
    FALSE_RETURN_V_MSG_E(formatContext != nullptr, false, "AVFormatContext is nullptr");
    if (!FFmpegFormatHelper::IsValidCodecId(stream->codecpar->codec_id)) {
        return true;
    }
    return (static_cast<uint32_t>(pkt->flags) & static_cast<uint32_t>(AV_PKT_FLAG_KEY) ||
            (stream->codecpar->codec_id == AV_CODEC_ID_HEVC &&
                (!IsSyncFrameCheckNeeded(formatContext) || IsHevcSyncFrame(pkt->data, pkt->size))));
}

Status FFmpegDemuxerPlugin::ParseVideoFirstFrames()
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(streamParsers_ != nullptr, Status::ERROR_NULL_POINTER, "StreamParser is nullptr");
    Plugins::AVPacketWrapperPtr pktWrapper = nullptr;
    Status ret = Status::OK;
    bool extraType = (fileType_ == FileType::MPEGTS || FFmpegFormatHelper::IsMpeg4File(fileType_) ||
        fileType_ == FileType::FLV);
    // Finish for extraType: get all support stream
    // Finish: read all video or init all parser
    while ((extraType && !AllSupportTrackFramesReady()) ||
           (!extraType && !AllVideoFirstFramesReady() && !streamParsers_->AllParserInited())) {
        FALSE_RETURN_V_MSG_E(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE, "ParseVideoFirstFrames interrupt");
        if (pktWrapper == nullptr) {
            pktWrapper = std::make_shared<Plugins::AVPacketWrapper>();
            FALSE_RETURN_V_MSG_E(pktWrapper != nullptr && pktWrapper->GetAVPacket() != nullptr,
                Status::ERROR_NULL_POINTER, "Create AVPacketWrapper failed");
        }
        std::unique_lock<std::mutex> sLock(syncMutex_);
        int ffmpegRet = AVReadFrameLimit(pktWrapper->GetAVPacket());
        sLock.unlock();
        if (ffmpegRet < 0) {
            MEDIA_LOG_E("Call av_read_frame failed, ret:" PUBLIC_LOG_D32, ffmpegRet);
            pktWrapper.reset();
            break;
        }
        int32_t trackId = pktWrapper->GetStreamIndex();
        auto stream = formatContext_->streams[trackId];
        FALSE_RETURN_V_MSG_E(stream != nullptr && stream->codecpar != nullptr, Status::ERROR_NULL_POINTER,
            "Stream " PUBLIC_LOG_D32 " is invalid", trackId);
        InitMinTsPacketInfo(pktWrapper->GetAVPacket());
        ret = AddPacketToCacheQueue(pktWrapper);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add to cache failed");
        bool needCheck = (stream->codecpar->codec_id != AV_CODEC_ID_VVC) && (!TrackIsChecked(trackId) &&
            IsSyncFrame(stream, pktWrapper->GetAVPacket(), formatContext_));
        if (!needCheck) {
            pktWrapper = nullptr;
            continue;
        }
        checkedTrackIds_.push_back(trackId);
        if (streamParsers_->ParserIsCreated(trackId) && !streamParsers_->ParserIsInited(trackId)) {
            ret = SetVideoFirstFrame(pktWrapper);
        } else if (extraType && FFmpegFormatHelper::IsVideoType(*stream)) {
            ret = SetVideoFirstFrame(pktWrapper, false);
        }
        if (ret != Status::OK) {
            MEDIA_LOG_E("Set first frame failed, track " PUBLIC_LOG_D32, trackId);
            return ret;
        }
        pktWrapper = nullptr;
    }
    return ret;
}

void FFmpegDemuxerPlugin::ParseHEVCMetadataInfo(const AVStream& avStream, Meta& format)
{
    HevcParseFormat parse;
    parse.isHdrVivid = streamParsers_->IsHdrVivid(avStream.index);
    parse.colorRange = streamParsers_->GetColorRange(avStream.index);
    parse.colorPrimaries = streamParsers_->GetColorPrimaries(avStream.index);
    parse.colorTransfer = streamParsers_->GetColorTransfer(avStream.index);
    parse.colorMatrixCoeff = streamParsers_->GetColorMatrixCoeff(avStream.index);
    parse.profile = streamParsers_->GetProfileIdc(avStream.index);
    parse.level = streamParsers_->GetLevelIdc(avStream.index);
    parse.chromaLocation = streamParsers_->GetChromaLocation(avStream.index);
    parse.picWidInLumaSamples = streamParsers_->GetPicWidInLumaSamples(avStream.index);
    parse.picHetInLumaSamples = streamParsers_->GetPicHetInLumaSamples(avStream.index);

    FFmpegFormatHelper::ParseHevcInfo(*formatContext_, parse, format);
}

bool FFmpegDemuxerPlugin::TrackIsSelected(const uint32_t trackId)
{
    return std::any_of(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                       [trackId](uint32_t id) { return id == trackId; });
}

bool FFmpegDemuxerPlugin::TrackIsChecked(const uint32_t trackId)
{
    return std::any_of(checkedTrackIds_.begin(), checkedTrackIds_.end(),
                       [trackId](uint32_t id) { return id == trackId; });
}

Status FFmpegDemuxerPlugin::SelectTrack(uint32_t trackId)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("Select " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    if (trackId >= static_cast<uint32_t>(formatContext_.get()->nb_streams)) {
        MEDIA_LOG_E("Track is invalid, just have " PUBLIC_LOG_D32 " tracks in file", formatContext_.get()->nb_streams);
        return Status::ERROR_INVALID_PARAMETER;
    }

    AVStream* avStream = formatContext_->streams[trackId];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER, "AVStream is nullptr");
    if (!IsSupportedTrack(*avStream)) {
        MEDIA_LOG_E("Track type is unsupport");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (!TrackIsSelected(trackId)) {
        selectedTrackIds_.push_back(trackId);
        trackMtx_[trackId] = std::make_shared<std::mutex>();
        trackDfxInfoMap_[trackId] = {0, -1, -1, -1, false};
        return cacheQueue_.AddTrackQueue(trackId);
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has been selected", trackId);
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::UnselectTrack(uint32_t trackId)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("Unselect " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    auto index = std::find_if(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                              [trackId](uint32_t selectedId) {return trackId == selectedId; });
    if (index != selectedTrackIds_.end()) {
        selectedTrackIds_.erase(index);
        trackMtx_.erase(trackId);
        trackDfxInfoMap_.erase(trackId);
        return cacheQueue_.RemoveTrackQueue(trackId);
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " is not in selected list", trackId);
    }
    return Status::OK;
}

int FFmpegDemuxerPlugin::SelectSeekTrack() const
{
    int trackIndex = static_cast<int>(selectedTrackIds_[0]);
    for (size_t i = 1; i < selectedTrackIds_.size(); i++) {
        int idx = static_cast<int>(selectedTrackIds_[i]);
        if (formatContext_->streams[idx]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            return idx;
        }
    }
    return trackIndex;
}

Status FFmpegDemuxerPlugin::CheckSeekParams(int64_t seekTime, SeekMode mode) const
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(seekTime >= 0 && seekTime <= INT64_MAX / MS_TO_NS, Status::ERROR_INVALID_PARAMETER,
        "Seek time " PUBLIC_LOG_D64 " is not supported", seekTime);
    FALSE_RETURN_V_MSG_E(g_seekModeToFFmpegSeekFlags.count(mode) != 0, Status::ERROR_INVALID_PARAMETER,
        "Seek mode " PUBLIC_LOG_U32 " is not supported", static_cast<uint32_t>(mode));
    return Status::OK;
}

void FFmpegDemuxerPlugin::SyncSeekThread()
{
    if (ioContext_.invokerType != InvokerType::SEEK) {
        std::lock_guard<std::mutex> seekLock(ioContext_.invokerTypeMutex);
        ioContext_.invokerType = InvokerType::SEEK;
    }
    if (readThread_ != nullptr && threadState_ == READING) {
        MEDIA_LOG_I("Seek notify read thread to stop");
        std::unique_lock<std::mutex> waitLock(seekWaitMutex_);
        ioContext_.readCbReady = true;
        readCbCv_.notify_all();
        seekWaitCv_.wait(waitLock, [this] { return threadState_ == WAITING || threadState_ == NOT_STARTED; });
    }
}

bool FFmpegDemuxerPlugin::IsUseFirstFrameDts(int trackIndex, int64_t seekTime)
{
    if (seekTime == 0 &&
        (fileType_ == FileType::MPEGTS || FFmpegFormatHelper::IsMpeg4File(fileType_)) &&
        FFmpegFormatHelper::IsVideoType(*(formatContext_->streams[trackIndex])) &&
        VideoFirstFrameValid(trackIndex)) {
            return true;
    }
    return false;
}

Status FFmpegDemuxerPlugin::DoSeekInternal(int trackIndex, int64_t seekTime, int64_t ffTime,
    SeekMode mode, int64_t& realSeekTime)
{
    auto avStream = formatContext_->streams[trackIndex];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER, "AVStream is nullptr");
    realSeekTime = ConvertTimeFromFFmpeg(ffTime, avStream->time_base);
    int flag = ConvertFlagsToFFmpeg(avStream, ffTime, mode, seekTime);
    MEDIA_LOG_I("Time [" PUBLIC_LOG_U64 "/" PUBLIC_LOG_U64 "/" PUBLIC_LOG_D64 "] flag ["
                PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "]",
                seekTime, ffTime, realSeekTime, static_cast<int32_t>(mode), flag);
    if (flag == AVSEEK_FLAG_FRAME && FFmpegFormatHelper::IsMpeg4File(fileType_)) {
        int keyFrameNext = av_index_search_timestamp(avStream, ffTime, AVSEEK_FLAG_FRAME);
        FALSE_RETURN_V_MSG_E(keyFrameNext >= 0, Status::ERROR_OUT_OF_RANGE,
            "Seek failed, err: Not next key frame");
    }
    std::unique_lock<std::mutex> sLock(syncMutex_);
    auto ret = av_seek_frame(formatContext_.get(), trackIndex, ffTime, flag);
    sLock.unlock();
    formatContext_->pb->error = 0;
    FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_UNKNOWN,
        "Call av_seek_frame failed, err: " PUBLIC_LOG_S, AVStrError(ret).c_str());
    if (readLoopStatus_ != Status::OK) {
        MEDIA_LOG_E("Read loop status is not OK, release thread");
        ReleaseFFmpegReadLoop();
    }
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        cacheQueue_.RemoveTrackQueue(selectedTrackIds_[i]);
        cacheQueue_.AddTrackQueue(selectedTrackIds_[i]);
    }
    seekTime_ = seekTime;
    seekMode_ = flag == AVSEEK_FLAG_BACKWARD ? SeekMode::SEEK_PREVIOUS_SYNC : mode;
    return Status::OK;
}

static bool IsEnableSeekTimeCalib(const std::shared_ptr<AVFormatContext> &formatContext)
{
    FALSE_RETURN_V_MSG_E(formatContext != nullptr, false, "AVFormatContext is nullptr");
    if (FileType::FLV == FFmpegFormatHelper::GetFileTypeByName(*formatContext)) {
        return true;
    }
    return false;
}

Status FFmpegDemuxerPlugin::SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode, int64_t& realSeekTime)
{
    (void) trackId;
    Status ret = Status::OK;
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("SeekTo");

    Status check = CheckSeekParams(seekTime, mode);
    FALSE_RETURN_V_MSG_E(check == Status::OK, check, "CheckSeekParams failed");
    if (readThread_ != nullptr) {
        SyncSeekThread();
    }
    int trackIndex = SelectSeekTrack();
    MEDIA_LOG_D("Seek based on track " PUBLIC_LOG_D32, trackIndex);

    auto avStream = formatContext_->streams[trackIndex];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER, "AVStream is nullptr");
    int64_t ffTime = ConvertTimeToFFmpeg(seekTime * MS_TO_NS, avStream->time_base);
    if (!CheckStartTime(formatContext_.get(), avStream, ffTime, seekTime)) {
        MEDIA_LOG_E("Get start time from track " PUBLIC_LOG_D32 " failed", trackIndex);
        return Status::ERROR_INVALID_OPERATION;
    }
    auto id = HiviewDFX::XCollie::GetInstance().SetTimer("av_codec::demuxer_seek", SETTIMER_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG);
    if (IsEnableSeekTimeCalib(formatContext_) && seekCalibMap_.count(trackIndex) > 0) {
        int64_t calibTime = ffTime - seekCalibMap_[trackIndex];
        ret = DoSeekInternal(trackIndex, seekTime, calibTime, mode, realSeekTime);
        if (ret == Status::OK) {
            return ret;
        }
        MEDIA_LOG_E("Seek using calibTime " PUBLIC_LOG_D64 " failed", calibTime);
    }
    
    if (IsUseFirstFrameDts(trackIndex, seekTime)) {
        ffTime = videoFirstFrameMap_[trackIndex]->GetDts();
    }
    ret = DoSeekInternal(trackIndex, seekTime, ffTime, mode, realSeekTime);
    HiviewDFX::XCollie::GetInstance().CancelTimer(id);
    return ret;
}

Status FFmpegDemuxerPlugin::Flush()
{
    Status ret = Status::OK;
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("In");
    if (ioContext_.invokerType != InvokerType::FLUSH) {
        std::lock_guard<std::mutex> seekLock(ioContext_.invokerTypeMutex);
        ioContext_.invokerType = InvokerType::FLUSH;
    }
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        ret = cacheQueue_.RemoveTrackQueue(selectedTrackIds_[i]);
        ret = cacheQueue_.AddTrackQueue(selectedTrackIds_[i]);
    }
    if (formatContext_) {
        std::unique_lock<std::mutex> sLock(syncMutex_);
        avio_flush(formatContext_.get()->pb);
        avformat_flush(formatContext_.get());
        sLock.unlock();
    }
    return ret;
}

void FFmpegDemuxerPlugin::ResetEosStatus()
{
    MEDIA_LOG_I("In");
    if (formatContext_ != nullptr && formatContext_->pb != nullptr) {
        formatContext_->pb->eof_reached = 0;
        formatContext_->pb->error = 0;
    }
}

void FFmpegDemuxerPlugin::DumpPacketInfo(int32_t trackId, Stage stage)
{
    if (trackDfxInfoMap_.count(trackId) <= 0) {
        return;
    }
    if (stage == Stage::FIRST_READ && !trackDfxInfoMap_[trackId].dumpFirstInfo) {
        MEDIA_LOG_I("Info Track[" PUBLIC_LOG_D32 "] [first] [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "]",
            trackId, trackDfxInfoMap_[trackId].lastPts,
            trackDfxInfoMap_[trackId].lastDuration, trackDfxInfoMap_[trackId].lastPos);
        trackDfxInfoMap_[trackId].dumpFirstInfo = true;
    } else if (stage == Stage::FILE_END) {
        MEDIA_LOG_I("Info Track[" PUBLIC_LOG_D32 "] [eos] [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "]",
            trackId, trackDfxInfoMap_[trackId].lastPts,
            trackDfxInfoMap_[trackId].lastDuration, trackDfxInfoMap_[trackId].lastPos);
    }
}

void FFmpegDemuxerPlugin::ClearUnselectTrackCache()
{
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        if (TrackIsSelected(trackIndex)) {
            continue;
        }
        trackMtx_.erase(trackIndex);
        trackDfxInfoMap_.erase(trackIndex);
        if (cacheQueue_.RemoveTrackQueue(trackIndex) != Status::OK) {
            MEDIA_LOG_W("Reset track cache failed: " PUBLIC_LOG_U32, trackIndex);
        }
    }
}

bool FFmpegDemuxerPlugin::FrameReady(Status ret)
{
    if (ret == Status::END_OF_STREAM) {
        MEDIA_LOG_D("Read to end");
        return true;
    }
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_UNKNOWN, false, "Read from ffmpeg failed");
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_AGAIN, false, "Read from ffmpeg failed, retry");
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_NO_MEMORY, false, "Cache size out of limit");
    FALSE_RETURN_V_MSG_E(ret != Status::ERROR_WRONG_STATE, false, "Read from ffmpeg failed interrupt");
    return true;
}

Status FFmpegDemuxerPlugin::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("ReadSample");
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "AVBuffer or memory is nullptr");
    FALSE_RETURN_V_MSG_E(readModeMap_.find(1) == readModeMap_.end(), Status::ERROR_INVALID_OPERATION,
        "Cannot use sync and async Read together");
    if (needClear_) {
        ClearUnselectTrackCache();
        needClear_ = false;
    }
    readModeMap_[0] = 1;
    Status ret;
    auto id = HiviewDFX::XCollie::GetInstance().SetTimer("av_codec::demuxer_read", SETTIMER_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG);
    if (NeedCombineFrame(trackId) && cacheQueue_.GetCacheSize(trackId) == 1) {
        ret = ReadPacketToCacheQueue(trackId);
    }
    while (!cacheQueue_.HasCache(trackId)) {
        FALSE_RETURN_V_MSG_E(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE, " ReadSample interrupt");
        ret = ReadPacketToCacheQueue(trackId);
        bool frameReady = FrameReady(ret);
        FALSE_RETURN_V_MSG_E(frameReady, ret, "Read from ffmpeg failed");
    }
    HiviewDFX::XCollie::GetInstance().CancelTimer(id);
    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    auto samplePacket = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_NULL_POINTER, "Cache packet is nullptr");
    if (samplePacket->isEOS) {
        ret = SetEosSample(sample);
        if (ret == Status::OK) {
            DumpPacketInfo(trackId, Stage::FILE_END);
            cacheQueue_.Pop(trackId);
        }
        return ret;
    }
    ret = ConvertAVPacketToSample(sample, samplePacket);
    DumpPacketInfo(trackId, Stage::FIRST_READ);
    if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
        return Status::OK;
    } else if (ret == Status::OK) {
        MEDIA_LOG_D("All partial sample has been copied");
        cacheQueue_.Pop(trackId);
    }
    return ret;
}

Status FFmpegDemuxerPlugin::GetNextSampleSize(uint32_t trackId, int32_t& size)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("GetNextSampleSize");
    MEDIA_LOG_D("In, track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_UNKNOWN, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_UNKNOWN, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(readModeMap_.find(1) == readModeMap_.end(), Status::ERROR_INVALID_OPERATION,
        "Cannot use sync and async Read together");
    readModeMap_[0] = 1;
    Status ret;
    if (NeedCombineFrame(trackId) && cacheQueue_.GetCacheSize(trackId) == 1) {
        ret = ReadPacketToCacheQueue(trackId);
    }
    while (!cacheQueue_.HasCache(trackId)) {
        ret = ReadPacketToCacheQueue(trackId);
        bool frameReady = FrameReady(ret);
        FALSE_RETURN_V_MSG_E(frameReady, ret, "Read from ffmpeg failed");
    }
    std::shared_ptr<SamplePacket> samplePacket = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_UNKNOWN, "Cache sample is nullptr");
    if (samplePacket->isEOS) {
        MEDIA_LOG_I("Track " PUBLIC_LOG_D32 " eos", trackId);
        return Status::END_OF_STREAM;
    }
    FALSE_RETURN_V_MSG_E(samplePacket->pkts.size() > 0, Status::ERROR_UNKNOWN, "Cache sample is empty");
    int totalSize = 0;
    for (const auto &pktWrapper : samplePacket->pkts) {
        FALSE_RETURN_V_MSG_E(pktWrapper != nullptr && pktWrapper->GetAVPacket() != nullptr, Status::ERROR_UNKNOWN,
            "Packet in sample is nullptr");
        totalSize += pktWrapper->GetSize();
    }

    const AVStreamSnapshot* snapshot = GetStreamSnapshot(trackId);
    FALSE_RETURN_V_MSG_E(snapshot != nullptr && snapshot->valid, Status::ERROR_UNKNOWN, "Track info invalid");
    if ((std::count(g_streamContainedXPS.begin(), g_streamContainedXPS.end(), snapshot->codecId) > 0) &&
        static_cast<uint32_t>(samplePacket->pkts[0]->GetFlags()) & static_cast<uint32_t>(AV_PKT_FLAG_KEY)) {
        totalSize += snapshot->extradataSize;
    }
    size = totalSize;
    return Status::OK;
}

void FFmpegDemuxerPlugin::InitPTSandIndexConvert()
{
    indexToRelativePTSFrameCount_ = 0; // init IndexToRelativePTSFrameCount_
    relativePTSToIndexPosition_ = 0; // init RelativePTSToIndexPosition_
    indexToRelativePTSMaxHeap_ = std::priority_queue<int64_t>(); // init IndexToRelativePTSMaxHeap_
    relativePTSToIndexPTSMin_ = INT64_MAX;
    relativePTSToIndexPTSMax_ = INT64_MIN;
    relativePTSToIndexRightDiff_ = INT64_MAX;
    relativePTSToIndexLeftDiff_ = INT64_MAX;
    relativePTSToIndexTempDiff_ = INT64_MAX;
}

Status FFmpegDemuxerPlugin::GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
    const uint64_t relativePresentationTimeUs, uint32_t &index)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");

    FALSE_RETURN_V_MSG_E(FFmpegFormatHelper::IsMpeg4File(fileType_),
        Status::ERROR_MISMATCHED_TYPE, "FileType is not MP4");

    FALSE_RETURN_V_MSG_E(trackIndex < formatContext_->nb_streams, Status::ERROR_INVALID_DATA, "Track is out of range");
    bool frameCheck = IsLessMaxReferenceParserFrames(trackIndex);
    FALSE_RETURN_V_MSG_E(frameCheck, Status::ERROR_INVALID_DATA, "Frame count exceeds limit");

    InitPTSandIndexConvert();

    auto avStream = formatContext_->streams[trackIndex];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER, "AVStream is nullptr");

    Status ret = GetPresentationTimeUsFromFfmpegMOV(GET_FIRST_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Get pts failed");

    int64_t absolutePTS = static_cast<int64_t>(relativePresentationTimeUs) + absolutePTSIndexZero_;

    ret = GetPresentationTimeUsFromFfmpegMOV(RELATIVEPTS_TO_INDEX, trackIndex,
        absolutePTS, index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Get pts failed");

    if (absolutePTS < relativePTSToIndexPTSMin_ || absolutePTS > relativePTSToIndexPTSMax_) {
        MEDIA_LOG_E("Pts is out of range");
        return Status::ERROR_INVALID_DATA;
    }

    if (relativePTSToIndexLeftDiff_ == 0 || relativePTSToIndexRightDiff_ == 0) {
        index = relativePTSToIndexPosition_;
    } else {
        index = relativePTSToIndexLeftDiff_ < relativePTSToIndexRightDiff_ ?
        relativePTSToIndexPosition_ - 1 : relativePTSToIndexPosition_;
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
    const uint32_t index, uint64_t &relativePresentationTimeUs)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(index < UINT32_MAX, Status::ERROR_INVALID_DATA, "Index is out of range");
    FALSE_RETURN_V_MSG_E(FFmpegFormatHelper::IsMpeg4File(fileType_),
        Status::ERROR_MISMATCHED_TYPE, "FileType is not MP4");

    FALSE_RETURN_V_MSG_E(trackIndex < formatContext_->nb_streams, Status::ERROR_INVALID_DATA, "Track is out of range");
    bool frameCheck = IsLessMaxReferenceParserFrames(trackIndex);
    FALSE_RETURN_V_MSG_E(frameCheck, Status::ERROR_INVALID_DATA, "Frame count exceeds limit");

    InitPTSandIndexConvert();

    auto avStream = formatContext_->streams[trackIndex];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER, "AVStream is nullptr");

    Status ret = GetPresentationTimeUsFromFfmpegMOV(GET_FIRST_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Get pts failed");

    ret = GetPresentationTimeUsFromFfmpegMOV(INDEX_TO_RELATIVEPTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Get pts failed");

    if (index + 1 > indexToRelativePTSFrameCount_) {
        MEDIA_LOG_E("Index is out of range");
        return Status::ERROR_INVALID_DATA;
    }

    int64_t relativepts = indexToRelativePTSMaxHeap_.top() - absolutePTSIndexZero_;
    FALSE_RETURN_V_MSG_E(relativepts >= 0, Status::ERROR_INVALID_DATA, "Existence of calculation results less than 0");
    relativePresentationTimeUs = static_cast<uint64_t>(relativepts);

    return Status::OK;
}

Status FFmpegDemuxerPlugin::PTSAndIndexConvertSttsAndCttsProcess(IndexAndPTSConvertMode mode,
    const AVStream* avStream, int64_t absolutePTS, uint32_t index)
{
    uint32_t sttsIndex = 0;
    uint32_t cttsIndex = 0;
    int64_t pts = 0; // init pts
    int64_t dts = 0; // init dts
    ptsCnt_ = 0;
    int32_t sttsCurNum = static_cast<int32_t>(avStream->stts_data[sttsIndex].count);
    int32_t cttsCurNum = static_cast<int32_t>(avStream->ctts_data[cttsIndex].count);
    int32_t minCttsDuration = GetMinCttsDuration(avStream);
    while (sttsIndex < avStream->stts_count && cttsIndex < avStream->ctts_count &&
            cttsCurNum >= 0 && sttsCurNum >= 0) {
        if (cttsCurNum == 0) {
            cttsIndex++;
            if (cttsIndex >= avStream->ctts_count) {
                break;
            }
            cttsCurNum = static_cast<int32_t>(avStream->ctts_data[cttsIndex].count);
        }
        cttsCurNum--;
        int64_t currentCttsDuration = static_cast<int64_t>(avStream->ctts_data[cttsIndex].duration - minCttsDuration);
        if ((INT64_MAX / 1000 / 1000) < // 1000 is used for converting pts to us
            ((dts + currentCttsDuration) / static_cast<int64_t>(avStream->time_scale))) {
                MEDIA_LOG_E("pts overflow");
                return Status::ERROR_INVALID_DATA;
        }
        double timeScaleRate = static_cast<double>(MS_TO_NS) / static_cast<double>(avStream->time_scale);
        double ptsTemp = static_cast<double>(dts) + static_cast<double>(currentCttsDuration);
        pts = static_cast<int64_t>(ptsTemp * timeScaleRate);
        if (mode == GET_ALL_FRAME_PTS) {
            if (ptsCnt_ >= REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT) {
                MEDIA_LOG_I("PTS cnt has reached the maximum limit");
                break;
            }
            ptsCnt_++;
        }
        PTSAndIndexConvertSwitchProcess(mode, pts, absolutePTS, index, static_cast<int64_t>(dts * timeScaleRate));
        sttsCurNum--;
        if ((INT64_MAX - dts) < (static_cast<int64_t>(avStream->stts_data[sttsIndex].duration))) {
            MEDIA_LOG_E("dts overflow");
            return Status::ERROR_INVALID_DATA;
        }
        dts += static_cast<int64_t>(avStream->stts_data[sttsIndex].duration);
        if (sttsCurNum == 0) {
            sttsIndex++;
            sttsCurNum = sttsIndex < avStream->stts_count ?
                         static_cast<int32_t>(avStream->stts_data[sttsIndex].count) : 0;
        }
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::PTSAndIndexConvertOnlySttsProcess(IndexAndPTSConvertMode mode,
    const AVStream* avStream, int64_t absolutePTS, uint32_t index)
{
    uint32_t sttsIndex = 0;
    int64_t pts = 0; // init pts
    int64_t dts = 0; // init dts
    ptsCnt_ = 0;
    int32_t sttsCurNum = static_cast<int32_t>(avStream->stts_data[sttsIndex].count);
    while (sttsIndex < avStream->stts_count && sttsCurNum >= 0) {
        if ((INT64_MAX / 1000 / 1000) < // 1000 is used for converting pts to us
            (dts / static_cast<int64_t>(avStream->time_scale))) {
                MEDIA_LOG_E("pts overflow");
                return Status::ERROR_INVALID_DATA;
        }
        double timeScaleRate = static_cast<double>(MS_TO_NS) / static_cast<double>(avStream->time_scale);
        double ptsTemp = static_cast<double>(dts);
        pts = static_cast<int64_t>(ptsTemp * timeScaleRate);
        if (mode == GET_ALL_FRAME_PTS) {
            if (ptsCnt_ >= REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT) {
                MEDIA_LOG_I("PTS cnt has reached the maximum limit");
                break;
            }
            ptsCnt_++;
        }
        PTSAndIndexConvertSwitchProcess(mode, pts, absolutePTS, index, pts);
        sttsCurNum--;
        if ((INT64_MAX - dts) < (static_cast<int64_t>(avStream->stts_data[sttsIndex].duration))) {
            MEDIA_LOG_E("dts overflow");
            return Status::ERROR_INVALID_DATA;
        }
        dts += static_cast<int64_t>(avStream->stts_data[sttsIndex].duration);
        if (sttsCurNum == 0) {
            sttsIndex++;
            sttsCurNum = sttsIndex < avStream->stts_count ?
                         static_cast<int32_t>(avStream->stts_data[sttsIndex].count) : 0;
        }
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::GetPresentationTimeUsFromFfmpegMOV(IndexAndPTSConvertMode mode,
    uint32_t trackIndex, int64_t absolutePTS, uint32_t index)
{
    auto avStream = formatContext_->streams[trackIndex];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER, "AVStream is nullptr");
    FALSE_RETURN_V_MSG_E(avStream->stts_data != nullptr && avStream->stts_count != 0,
        Status::ERROR_NULL_POINTER, "AVStream->stts_data is empty");
    FALSE_RETURN_V_MSG_E(avStream->time_scale != 0, Status::ERROR_INVALID_DATA, "AVStream->time_scale is zero");
    return avStream->ctts_data == nullptr ?
        PTSAndIndexConvertOnlySttsProcess(mode, avStream, absolutePTS, index) :
        PTSAndIndexConvertSttsAndCttsProcess(mode, avStream, absolutePTS, index);
}

void FFmpegDemuxerPlugin::PTSAndIndexConvertSwitchProcess(IndexAndPTSConvertMode mode,
    int64_t pts, int64_t absolutePTS, uint32_t index, int64_t dts)
{
    switch (mode) {
        case GET_FIRST_PTS:
            absolutePTSIndexZero_ = pts < absolutePTSIndexZero_ ? pts : absolutePTSIndexZero_;
            break;
        case INDEX_TO_RELATIVEPTS:
            IndexToRelativePTSProcess(pts, index);
            break;
        case RELATIVEPTS_TO_INDEX:
            RelativePTSToIndexProcess(pts, absolutePTS);
            break;
        case GET_ALL_FRAME_PTS:
            if (dts == 0L) {
                minPts_ = pts;
                MEDIA_LOG_I("minPts=" PUBLIC_LOG_D64, minPts_);
            }
            MEDIA_LOG_D("dts " PUBLIC_LOG_D64 ", pts " PUBLIC_LOG_D64 ", frameId " PUBLIC_LOG_ZU,
                dts - minPts_, pts - minPts_, pts2DtsMap_.size());
            pts2DtsMap_.emplace(pts - minPts_, dts - minPts_);
            break;
        default:
            MEDIA_LOG_E("Wrong mode");
            break;
    }
}

void FFmpegDemuxerPlugin::IndexToRelativePTSProcess(int64_t pts, uint32_t index)
{
    if (indexToRelativePTSMaxHeap_.size() < index + 1) {
        indexToRelativePTSMaxHeap_.push(pts);
    } else {
        if (pts < indexToRelativePTSMaxHeap_.top()) {
            indexToRelativePTSMaxHeap_.pop();
            indexToRelativePTSMaxHeap_.push(pts);
        }
    }
    indexToRelativePTSFrameCount_++;
}

void FFmpegDemuxerPlugin::RelativePTSToIndexProcess(int64_t pts, int64_t absolutePTS)
{
    if (relativePTSToIndexPTSMin_ > pts) {
        relativePTSToIndexPTSMin_ = pts;
    }
    if (relativePTSToIndexPTSMax_ < pts) {
        relativePTSToIndexPTSMax_ = pts;
    }
    relativePTSToIndexTempDiff_ = abs(pts - absolutePTS);
    if (pts < absolutePTS && relativePTSToIndexTempDiff_ < relativePTSToIndexLeftDiff_) {
        relativePTSToIndexLeftDiff_ = relativePTSToIndexTempDiff_;
    }
    if (pts >= absolutePTS && relativePTSToIndexTempDiff_ < relativePTSToIndexRightDiff_) {
        relativePTSToIndexRightDiff_ = relativePTSToIndexTempDiff_;
    }
    if (pts < absolutePTS) {
        relativePTSToIndexPosition_++;
    }
}

Status FFmpegDemuxerPlugin::CheckCacheDataLimit(uint32_t trackId)
{
    if (!outOfLimit_) {
        auto cacheDataSize = cacheQueue_.GetCacheDataSize(trackId);
        if (cacheDataSize > cachelimitSize_) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " cache out of limit: " PUBLIC_LOG_U32 "/" PUBLIC_LOG_U32 ", by user "
                PUBLIC_LOG_D32, trackId, cacheDataSize, cachelimitSize_, static_cast<int32_t>(setLimitByUser));
            outOfLimit_ = true;
        }
    }
    return Status::OK;
}

void FFmpegDemuxerPlugin::SetCacheLimit(uint32_t limitSize)
{
    setLimitByUser = true;
    cachelimitSize_ = limitSize;
}

Status FFmpegDemuxerPlugin::GetCurrentCacheSize(uint32_t trackId, uint32_t& size)
{
    MEDIA_LOG_D("TrackId " PUBLIC_LOG_U32, trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    uint32_t dataSize = cacheQueue_.GetCacheDataSize(trackId);
    FALSE_RETURN_V_MSG_E(dataSize < UINT32_MAX, Status::ERROR_WRONG_STATE, "CacheSize is invalid");
    size = dataSize;
    return Status::OK;
}

void FFmpegDemuxerPlugin::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("SetInterruptState %{public}d", isInterruptNeeded);
    isInterruptNeeded_ = isInterruptNeeded;
}

bool FFmpegDemuxerPlugin::IsSkipGetMinTsPktInfo()
{
    return std::find(g_fileSkipGetMinTsPktInfo.begin(), g_fileSkipGetMinTsPktInfo.end(), fileType_) !=
        g_fileSkipGetMinTsPktInfo.end();
}

Status FFmpegDemuxerPlugin::GetFileFirstPacket()
{
    bool isSkip = IsSkipGetMinTsPktInfo();
    FALSE_RETURN_V_MSG_I(!isSkip, Status::OK, "File skip get first packet info");
    Status ret = Status::OK;
    while (!minTsPktInfo_.isInit) {
        Plugins::AVPacketWrapperPtr pktWrapper = std::make_shared<Plugins::AVPacketWrapper>();
        FALSE_RETURN_V_MSG_E(pktWrapper != nullptr && pktWrapper->GetAVPacket() != nullptr,
            Status::ERROR_NULL_POINTER, "Create AVPacketWrapper failed");
        std::unique_lock<std::mutex> sLock(syncMutex_);
        int ffRet = AVReadFrameLimit(pktWrapper->GetAVPacket());
        sLock.unlock();
        FALSE_RETURN_V_MSG_E(ffRet == 0, Status::ERROR_WRONG_STATE, "Call av_read_frame failed");

        InitMinTsPacketInfo(pktWrapper->GetAVPacket());
        ret = AddPacketToCacheQueue(pktWrapper);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add packet to cache failed");
    }
    return Status::OK;
}

void FFmpegDemuxerPlugin::InitMinTsPacketInfo(AVPacket *pkt)
{
    bool isSkip = IsSkipGetMinTsPktInfo();
    FALSE_RETURN_MSG_W(!isSkip, "File skip init");
    FALSE_RETURN_MSG_W(pkt != nullptr, "AVPacket is nullptr");
    FALSE_RETURN_MSG_W(pkt->dts != AV_NOPTS_VALUE || pkt->pts != AV_NOPTS_VALUE,
        "pkt dts and pts is AV_NOPTS_VALUE");
    FALSE_RETURN_MSG_W(!minTsPktInfo_.isInit, "minTsPktInfo_ has been initialized");
    minTsPktInfo_.streamIndex = pkt->stream_index;
    minTsPktInfo_.minPts = pkt->pts;
    minTsPktInfo_.minDts = pkt->dts;
    minTsPktInfo_.isInit = true;
}

void FFmpegDemuxerPlugin::UpdMinTsPacketInfo(AVPacket *pkt)
{
    minTsPktInfo_.isUpd = true;
    FALSE_RETURN_MSG_W(pkt != nullptr, "AVPacket is nullptr");
    FALSE_RETURN_MSG_D(minTsPktInfo_.isInit, "minTsPktInfo_ is not init");
    if ((pluginImpl_->flags & AVFMT_SEEK_TO_PTS) && !FFmpegFormatHelper::IsMpeg4File(fileType_) &&
        pkt->pts != AV_NOPTS_VALUE && pkt->pts < minTsPktInfo_.minPts) {
        minTsPktInfo_.streamIndex = pkt->stream_index;
        minTsPktInfo_.minPts = pkt->pts;
    } else if (pkt->dts != AV_NOPTS_VALUE && pkt->dts < minTsPktInfo_.minDts) {
        minTsPktInfo_.streamIndex = pkt->stream_index;
        minTsPktInfo_.minDts = pkt->dts;
    }
}

Status FFmpegDemuxerPlugin::SeekToStartInternal()
{
    int64_t seekTs = AV_NOPTS_VALUE;
    int ffRet = -1;
    if (!minTsPktInfo_.isUpd) {
        MEDIA_LOG_I("minTsPktInfo_ is not upd, do not seek.");
        return Status::OK;
    }
    if (IsSkipGetMinTsPktInfo()) {
        av_dict_set_int(&formatContext_->metadata, "seekToStart", 1, 0);
        std::unique_lock<std::mutex> sLock(syncMutex_);
        ffRet = av_seek_frame(formatContext_.get(), SEEK_TRACK_DEFAULT, seekTs, AVSEEK_FLAG_ANY);
        sLock.unlock();
        av_dict_set_int(&formatContext_->metadata, "seekToStart", 0, 0);
    } else if (minTsPktInfo_.isInit) {
        seekTs = (pluginImpl_->flags & AVFMT_SEEK_TO_PTS) && !FFmpegFormatHelper::IsMpeg4File(fileType_) ?
            minTsPktInfo_.minPts : minTsPktInfo_.minDts;
        std::unique_lock<std::mutex> sLock(syncMutex_);
        ffRet = av_seek_frame(formatContext_.get(),
            minTsPktInfo_.streamIndex, seekTs, AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD);
        sLock.unlock();
        MEDIA_LOG_I("av_seek_frame stream_index " PUBLIC_LOG_U32 " seekTs " PUBLIC_LOG_D64 " ffRet " PUBLIC_LOG_D32,
            minTsPktInfo_.streamIndex, seekTs, ffRet);
    }
    if (ffRet < 0) {
        MEDIA_LOG_I("Use default track and startTime " PUBLIC_LOG_D64, formatContext_->start_time);
        std::unique_lock<std::mutex> sLock(syncMutex_);
        ffRet = av_seek_frame(formatContext_.get(), SEEK_TRACK_DEFAULT,
            formatContext_->start_time, AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD);
        sLock.unlock();
        FALSE_RETURN_V_MSG_E(ffRet >= 0, Status::ERROR_UNKNOWN, "av_seek_frame default track failed.");
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::SeekToStart()
{
    MEDIA_LOG_D("in");
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("SeekToStart");
    auto id = HiviewDFX::XCollie::GetInstance().SetTimer("av_codec::demuxer_seekToStart", SETTIMER_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG);
    auto ret = SeekToStartInternal();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "SeekToStartInternal failed.");
    if (readLoopStatus_ != Status::OK) {
        MEDIA_LOG_E("Read loop status is not OK, release thread");
        ReleaseFFmpegReadLoop();
    }
    for (auto track : selectedTrackIds_) {
        cacheQueue_.RemoveTrackQueue(track);
        cacheQueue_.AddTrackQueue(track);
    }
    seekTime_ = AV_NOPTS_VALUE;
    seekMode_ = SeekMode::SEEK_NEXT_SYNC;
    HiviewDFX::XCollie::GetInstance().CancelTimer(id);
    return Status::OK;
}

namespace { // plugin set

int IsStartWithID3(const uint8_t *buf, const char *tagName)
{
    return buf[POS_0] == tagName[POS_0] &&
           buf[POS_1] == tagName[POS_1] &&
           buf[POS_2] == tagName[POS_2] &&
           buf[POS_3] != POS_FF &&
           buf[POS_4] != POS_FF &&
           (buf[POS_6] & TAG_MASK) == 0 &&
           (buf[POS_7] & TAG_MASK) == 0 &&
           (buf[POS_8] & TAG_MASK) == 0 &&
           (buf[POS_9] & TAG_MASK) == 0;
}

int GetID3TagLen(const uint8_t *buf)
{
    int32_t len = ((buf[POS_6] & LEN_MASK) << POS_21) + ((buf[POS_7] & LEN_MASK) << POS_14) +
                  ((buf[POS_8] & LEN_MASK) << POS_7) + (buf[POS_9] & LEN_MASK) +
                  static_cast<int32_t>(ID3V2_HEADER_SIZE);
    if (buf[POS_5] & TAG_VERSION_MASK) {
        len += static_cast<int32_t>(ID3V2_HEADER_SIZE);
    }
    return len;
}

int32_t GetConfidence(std::shared_ptr<AVInputFormat> plugin, const std::string& pluginName,
    std::shared_ptr<DataSource> dataSource, size_t &getData, size_t bufferSize)
{
    uint64_t fileSize = 0;
    Status getFileSize = dataSource->GetSize(fileSize);
    if (getFileSize == Status::OK) {
        bufferSize = (bufferSize < fileSize) ? bufferSize : fileSize;
    }
    std::vector<uint8_t> buff(bufferSize + AVPROBE_PADDING_SIZE); // fix ffmpeg probe crash, refer to tools/probetest.c
    auto bufferInfo = std::make_shared<Buffer>();
    auto bufData = bufferInfo->WrapMemory(buff.data(), bufferSize, bufferSize);
    FALSE_RETURN_V_MSG_E(bufferInfo->GetMemory() != nullptr, -1,
        "Alloc buffer failed for " PUBLIC_LOG_S, pluginName.c_str());
    Status ret = Status::OK;
    {
        std::string traceName = "Sniff_" + pluginName + "_Readat";
        MediaAVCodec::AVCodecTrace trace(traceName.c_str());
        ret = dataSource->ReadAt(0, bufferInfo, bufferSize);
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, -1, "Read probe data failed for " PUBLIC_LOG_S, pluginName.c_str());
    getData = bufferInfo->GetMemory()->GetSize();
    FALSE_RETURN_V_MSG_E(getData > 0, -1, "No data for sniff " PUBLIC_LOG_S, pluginName.c_str());
    if (getFileSize == Status::OK && getData > ID3V2_HEADER_SIZE && IsStartWithID3(buff.data(), "ID3")) {
        int32_t id3Len = GetID3TagLen(buff.data());
        // id3 tag length is out of file, or file just contains id3 tag, no valid data.
        FALSE_RETURN_V_MSG_E(id3Len >= 0 && static_cast<uint64_t>(id3Len) < fileSize, -1,
            "File data error for " PUBLIC_LOG_S, pluginName.c_str());
        if (id3Len > 0) {
            uint64_t remainSize = fileSize - static_cast<uint64_t>(id3Len);
            bufferSize = (bufferSize < remainSize) ? bufferSize : remainSize;
            int resetRet = memset_s(buff.data(), bufferSize, 0, bufferSize);
            FALSE_RETURN_V_MSG_E(resetRet == EOK, -1, "Reset buff failed for " PUBLIC_LOG_S, pluginName.c_str());
            ret = dataSource->ReadAt(id3Len, bufferInfo, bufferSize);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, -1, "Read probe data failed for " PUBLIC_LOG_S, pluginName.c_str());
            getData = bufferInfo->GetMemory()->GetSize();
            FALSE_RETURN_V_MSG_E(getData > 0, -1, "No data for sniff " PUBLIC_LOG_S, pluginName.c_str());
        }
    }
    AVProbeData probeData{"", buff.data(), static_cast<int32_t>(getData), ""};
    return plugin->read_probe(&probeData);
}

int Sniff(const std::string& pluginName, std::shared_ptr<DataSource> dataSource)
{
    FALSE_RETURN_V_MSG_E(!pluginName.empty(), 0, "Plugin name is empty");
    FALSE_RETURN_V_MSG_E(dataSource != nullptr, 0, "DataSource is nullptr");
    return SniffWithSize(pluginName, dataSource, DEFAULT_SNIFF_SIZE);
}

int SniffWithSize(const std::string& pluginName, std::shared_ptr<DataSource> dataSource, int probSize)
{
    FALSE_RETURN_V_MSG_E(!pluginName.empty(), 0, "Plugin name is empty");
    FALSE_RETURN_V_MSG_E(dataSource != nullptr, 0, "DataSource is nullptr");
    std::shared_ptr<AVInputFormat> plugin;
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        auto inputFormat = av_find_input_format(ProcessPluginName(pluginName).c_str());
        plugin = std::shared_ptr<AVInputFormat>(const_cast<AVInputFormat*>(inputFormat), [](void*) {});
    }
    FALSE_RETURN_V_MSG_E((plugin != nullptr && plugin->read_probe), 0,
        "Get plugin for " PUBLIC_LOG_S " failed", pluginName.c_str());
    size_t getData = 0;
    int confidence = GetConfidence(plugin, pluginName, dataSource, getData, probSize);
    if (confidence < 0) {
        return 0;
    }
    if (StartWith(plugin->name, PLUGIN_NAME_MP3) && confidence > 0 && confidence <= MP3_PROBE_SCORE_LIMIT) {
        MEDIA_LOG_W("Score " PUBLIC_LOG_D32 " is too low", confidence);
        confidence = 0;
    }
    if (getData < DEFAULT_SNIFF_SIZE || confidence > 0) {
        MEDIA_LOG_I("Sniff:" PUBLIC_LOG_S "[" PUBLIC_LOG_ZU "/" PUBLIC_LOG_D32 "]", plugin->name, getData, confidence);
    }
    return confidence;
}

bool CheckMPEGPSStartCode(std::shared_ptr<DataSource> &dataSource)
{
    FALSE_RETURN_V_MSG_E(dataSource != nullptr, false, "DataSource is nullptr");
    std::vector<uint8_t> buff(MPEGPS_START_CODE_SIZE);
    auto bufferInfo = std::make_shared<Buffer>();
    auto bufData = bufferInfo->WrapMemory(buff.data(), MPEGPS_START_CODE_SIZE, MPEGPS_START_CODE_SIZE);
    FALSE_RETURN_V_MSG_E(bufferInfo->GetMemory() != nullptr, false, "Alloc buffer failed");

    auto ret = dataSource->ReadAt(0, bufferInfo, MPEGPS_START_CODE_SIZE);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Read data failed");
    for (int32_t i = 0; i < MPEGPS_START_CODE_SIZE; i++) {
        if (buff[i] != MPEGPS_START_CODE[i]) {
            return false;
        }
    }
    return true;
}

int SniffMPEGPS(const std::string& pluginName, std::shared_ptr<DataSource> dataSource)
{
    FALSE_RETURN_V_MSG_E(!pluginName.empty(), 0, "Plugin name is empty");
    FALSE_RETURN_V_MSG_E(dataSource != nullptr, 0, "DataSource is nullptr");
    int32_t psScore = SniffWithSize(pluginName, dataSource, DEFAULT_SNIFF_SIZE);
    if (psScore >= DEF_PROBE_SCORE_LIMIT) {
        std::string mp3PluginName = std::string(PLUGIN_NAME_PREFIX) + std::string(PLUGIN_NAME_MP3);
        int32_t mp3Score = SniffWithSize(mp3PluginName, dataSource, DEFAULT_SNIFF_SIZE);
        if (mp3Score >= DEF_PROBE_SCORE_LIMIT && !CheckMPEGPSStartCode(dataSource)) {
            return 0;
        }
    }
    return psScore;
}

inline static PluginSnifferFunc FindSniffer(const std::string& pluginName)
{
    return (pluginName == std::string(PLUGIN_NAME_MPEGPS)) ? SniffMPEGPS : Sniff;
}

void ReplaceDelimiter(const std::string& delmiters, char newDelimiter, std::string& str)
{
    MEDIA_LOG_D("Reset from [" PUBLIC_LOG_S "]", str.c_str());
    for (auto it = str.begin(); it != str.end(); ++it) {
        if (delmiters.find(newDelimiter) != std::string::npos) {
            *it = newDelimiter;
        }
    }
    MEDIA_LOG_D("Reset to [" PUBLIC_LOG_S "]", str.c_str());
};

Status RegisterPlugins(const std::shared_ptr<Register>& reg)
{
    MEDIA_LOG_I("In");
    FALSE_RETURN_V_MSG_E(reg != nullptr, Status::ERROR_INVALID_PARAMETER, "Register is nullptr");
    std::lock_guard<std::mutex> lock(g_mtx);
    const AVInputFormat* plugin = nullptr;
    void* i = nullptr;
    while ((plugin = av_demuxer_iterate(&i))) {
        if (plugin == nullptr) {
            continue;
        }
        MEDIA_LOG_D("Check ffmpeg demuxer " PUBLIC_LOG_S "[" PUBLIC_LOG_S "]", plugin->name, plugin->long_name);
        std::string pluginName = "avdemux_" + std::string(plugin->name);
        ReplaceDelimiter(".,|-<> ", '_', pluginName);

        DemuxerPluginDef regInfo;
        regInfo.name = pluginName;
        regInfo.description = "ffmpeg demuxer plugin";
        regInfo.rank = RANK_MAX;
        regInfo.AddExtensions(SplitString(plugin->extensions, ','));
        auto func = [](const std::string& name) -> std::shared_ptr<DemuxerPlugin> {
            return std::make_shared<FFmpegDemuxerPlugin>(name);
        };
        regInfo.SetCreator(func);
        regInfo.SetSniffer(FindSniffer(plugin->name));
        auto ret = reg->AddPlugin(regInfo);
        if (ret != Status::OK) {
            MEDIA_LOG_E("Add plugin failed, err=" PUBLIC_LOG_D32, static_cast<int>(ret));
        } else {
            MEDIA_LOG_D("Add plugin " PUBLIC_LOG_S, pluginName.c_str());
        }
    }
    return Status::OK;
}
} // namespace
PLUGIN_DEFINITION(FFmpegDemuxer, LicenseType::LGPL, RegisterPlugins, [] {});
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
