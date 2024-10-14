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

#define MEDIA_PLUGIN
#define HST_LOG_TAG "FfmpegDemuxerPlugin"
#include <unistd.h>
#include <algorithm>
#include <malloc.h>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <chrono>
#include <limits>
#include "avcodec_trace.h"
#include "securec.h"
#include "ffmpeg_format_helper.h"
#include "ffmpeg_utils.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "common/log.h"
#include "meta/video_types.h"
#include "avcodec_sysevent.h"
#include "ffmpeg_demuxer_plugin.h"
#include "meta/format.h"
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "FfmpegDemuxerPlugin" };
}

#if defined(LIBAVFORMAT_VERSION_INT) && defined(LIBAVFORMAT_VERSION_INT)
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 78, 0) and LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(58, 64, 100)
#if LIBAVFORMAT_VERSION_INT != AV_VERSION_INT(58, 76, 100)
#include "libavformat/internal.h"
#endif
#endif
#endif

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
const uint32_t DEFAULT_READ_SIZE = 4096;
const uint32_t DEFAULT_SNIFF_SIZE = 4096 * 4;
const int32_t MP3_PROBE_SCORE_LIMIT = 5;
const uint32_t STR_MAX_LEN = 4;
const uint32_t RANK_MAX = 100;
const uint32_t NAL_START_CODE_SIZE = 4;
const uint32_t INIT_DOWNLOADS_DATA_SIZE_THRESHOLD = 2 * 1024 * 1024;
const int64_t LIVE_FLV_PROBE_SIZE = 100 * 1024 * 2;
const uint32_t DEFAULT_CACHE_LIMIT = 50 * 1024 * 1024; // 50M
namespace {
std::map<std::string, std::shared_ptr<AVInputFormat>> g_pluginInputFormat;
std::mutex g_mtx;

int Sniff(const std::string& pluginName, std::shared_ptr<DataSource> dataSource);

Status RegisterPlugins(const std::shared_ptr<Register>& reg);

void ReplaceDelimiter(const std::string &delmiters, char newDelimiter, std::string &str);

static const std::map<SeekMode, int32_t>  g_seekModeToFFmpegSeekFlags = {
    { SeekMode::SEEK_PREVIOUS_SYNC, AVSEEK_FLAG_BACKWARD },
    { SeekMode::SEEK_NEXT_SYNC, AVSEEK_FLAG_FRAME },
    { SeekMode::SEEK_CLOSEST_SYNC, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD }
};

static const std::map<AVCodecID, std::string> g_bitstreamFilterMap = {
    { AV_CODEC_ID_H264, "h264_mp4toannexb" },
};

static const std::map<AVCodecID, StreamType> g_streamParserMap = {
    { AV_CODEC_ID_HEVC, StreamType::HEVC },
    { AV_CODEC_ID_VVC,  StreamType::VVC },
};

static const std::vector<AVMediaType> g_streamMediaTypeVec = {
    AVMEDIA_TYPE_AUDIO,
    AVMEDIA_TYPE_VIDEO,
    AVMEDIA_TYPE_SUBTITLE,
    AVMEDIA_TYPE_TIMEDMETA
};

static std::vector<AVCodecID> g_imageCodecID = {
    AV_CODEC_ID_MJPEG,
    AV_CODEC_ID_PNG,
    AV_CODEC_ID_PAM,
    AV_CODEC_ID_BMP,
    AV_CODEC_ID_JPEG2000,
    AV_CODEC_ID_TARGA,
    AV_CODEC_ID_TIFF,
    AV_CODEC_ID_GIF,
    AV_CODEC_ID_PCX,
    AV_CODEC_ID_XWD,
    AV_CODEC_ID_XBM,
    AV_CODEC_ID_WEBP,
    AV_CODEC_ID_APNG,
    AV_CODEC_ID_XPM,
    AV_CODEC_ID_SVG,
};

void StringifyMeta(Meta meta, int32_t trackIndex)
{
    OHOS::Media::Format format;
    for (TagType key: g_supportSourceFormat) {
        if (meta.Find(std::string(key)) != meta.end()) {
            meta.SetData(std::string(key), "***");
        }
    }
    if (meta.Find(std::string(Tag::MEDIA_CONTAINER_START_TIME)) != meta.end()) {
        meta.SetData(std::string(Tag::MEDIA_CONTAINER_START_TIME), "***");
    }
    if (meta.Find(std::string(Tag::MEDIA_START_TIME)) != meta.end()) {
        meta.SetData(std::string(Tag::MEDIA_START_TIME), "***");
    }
    if (meta.Find(std::string(Tag::MEDIA_LATITUDE)) != meta.end()) {
        meta.SetData(std::string(Tag::MEDIA_LATITUDE), "***");
    }
    if (meta.Find(std::string(Tag::MEDIA_LONGITUDE)) != meta.end()) {
        meta.SetData(std::string(Tag::MEDIA_LONGITUDE), "***");
    }
    if (meta.Find(std::string(Tag::TIMED_METADATA_KEY)) != meta.end()) {
        meta.SetData(std::string(Tag::TIMED_METADATA_KEY), "***");
    }
    if (meta.Find(std::string(Tag::TIMED_METADATA_SRC_TRACK)) != meta.end()) {
        meta.SetData(std::string(Tag::TIMED_METADATA_SRC_TRACK), "***");
    }
    format.SetMeta(std::make_shared<Meta>(meta));
    if (trackIndex < 0) {
        MEDIA_LOG_I("meta info[source]: " PUBLIC_LOG_S, format.Stringify().c_str());
    } else {
        MEDIA_LOG_I("meta info[track " PUBLIC_LOG_D32 "]: " PUBLIC_LOG_S, trackIndex, format.Stringify().c_str());
    }
}

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
            MEDIA_LOG_D("Get duration from file metadata.");
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
            MEDIA_LOG_D("Get duration from stream metadata.");
            duration = us;
        }
    }
    return duration;
}

bool CheckStartTime(const AVFormatContext *formatContext, const AVStream *stream, int64_t &timeStamp, int64_t seekTime)
{
    int64_t startTime = 0;
    int64_t num = 1000; // ms convert us
    FALSE_RETURN_V_MSG_E(stream != nullptr, false, "String is nullptr.");
    if (stream->start_time != AV_NOPTS_VALUE) {
        startTime = stream->start_time;
        if (timeStamp > 0 && startTime > INT64_MAX - timeStamp) {
            MEDIA_LOG_E("Seek value overflow with start time: " PUBLIC_LOG_D64 " timeStamp: " PUBLIC_LOG_D64 "",
                startTime, timeStamp);
            return false;
        }
    }
    MEDIA_LOG_D("startTime: " PUBLIC_LOG_D64, startTime);
    int64_t fileDuration = formatContext->duration;
    int64_t streamDuration = stream->duration;
    if (fileDuration == AV_NOPTS_VALUE || fileDuration <= 0) {
        fileDuration = GetFileDuration(*formatContext);
    }
    if (streamDuration == AV_NOPTS_VALUE || streamDuration <= 0) {
        streamDuration = GetStreamDuration(*stream);
    }
    MEDIA_LOG_D("file duration = " PUBLIC_LOG_D64 "us, stream duration = " PUBLIC_LOG_D64 "(fftime)",
        fileDuration, streamDuration);
    // when timestemp out of file duration, return error
    if (fileDuration > 0 && seekTime * num > fileDuration) { // fileDuration us
        MEDIA_LOG_E("Seek to timestamp = " PUBLIC_LOG_D64 " failed, max = " PUBLIC_LOG_D64 "",
                        timeStamp, fileDuration);
        return false;
    }
    // when timestemp out of stream duration, seek to end of stream
    if (streamDuration > 0 && timeStamp > streamDuration) {
        MEDIA_LOG_W("Out of stream, seek to " PUBLIC_LOG_D64, timeStamp);
        timeStamp = streamDuration;
    }
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        MEDIA_LOG_D("Reset timeStamp by start time: " PUBLIC_LOG_D64 " to " PUBLIC_LOG_D64,
            timeStamp, timeStamp + startTime);
        timeStamp += startTime;
    }
    return true;
}

int ConvertFlagsToFFmpeg(AVStream *avStream, int64_t ffTime, SeekMode mode, int64_t seekTime)
{
    FALSE_RETURN_V_MSG_E(avStream != nullptr && avStream->codecpar != nullptr, -1, "stream is nullptr.");
    if (avStream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE && ffTime == 0) {
        return AVSEEK_FLAG_FRAME;
    }
    if (avStream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO || seekTime == 0) {
        return AVSEEK_FLAG_BACKWARD;
    }
    if (mode == SeekMode::SEEK_NEXT_SYNC || mode == SeekMode::SEEK_PREVIOUS_SYNC) {
        return g_seekModeToFFmpegSeekFlags.at(mode);
    }
    // find closest time in next and prev
    int keyFrameNext = av_index_search_timestamp(avStream, ffTime, AVSEEK_FLAG_FRAME);
    FALSE_RETURN_V_MSG_E(keyFrameNext >= 0, AVSEEK_FLAG_BACKWARD, "No next key frame.");

    int keyFramePrev = av_index_search_timestamp(avStream, ffTime, AVSEEK_FLAG_BACKWARD);
    FALSE_RETURN_V_MSG_E(keyFramePrev >= 0, AVSEEK_FLAG_FRAME, "No next key frame.");

    int64_t ffTimePrev = CalculateTimeByFrameIndex(avStream, keyFramePrev);
    int64_t ffTimeNext = CalculateTimeByFrameIndex(avStream, keyFrameNext);
    MEDIA_LOG_D("ffTime=" PUBLIC_LOG_D64 ", ffTimePrev=" PUBLIC_LOG_D64 ", ffTimeNext=" PUBLIC_LOG_D64 ".",
        ffTime, ffTimePrev, ffTimeNext);
    if (ffTimePrev == ffTimeNext || (ffTimeNext - ffTime < ffTime - ffTimePrev)) {
        return AVSEEK_FLAG_FRAME;
    } else {
        return AVSEEK_FLAG_BACKWARD;
    }
}

bool IsSupportedTrack(const AVStream& avStream)
{
    FALSE_RETURN_V_MSG_E(avStream.codecpar != nullptr, false, "Codec par is nullptr.");
    if (std::find(g_streamMediaTypeVec.cbegin(), g_streamMediaTypeVec.cend(),
        avStream.codecpar->codec_type) == g_streamMediaTypeVec.cend()) {
        MEDIA_LOG_E("Unsupport track type: " PUBLIC_LOG_S ".",
            ConvertFFmpegMediaTypeToString(avStream.codecpar->codec_type).data());
        return false;
    }
    if (avStream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (avStream.codecpar->codec_id == AV_CODEC_ID_RAWVIDEO) {
            MEDIA_LOG_E("Unsupport raw video track.");
            return false;
        }
        if (std::count(g_imageCodecID.begin(), g_imageCodecID.end(), avStream.codecpar->codec_id) > 0) {
            MEDIA_LOG_E("Unsupport image track.");
            return false;
        }
    }
    return true;
}
} // namespace

std::atomic<int> FFmpegDemuxerPlugin::readatIndex_ = 0;
FFmpegDemuxerPlugin::FFmpegDemuxerPlugin(std::string name)
    : DemuxerPlugin(std::move(name)),
      seekable_(Seekable::SEEKABLE),
      ioContext_(),
      selectedTrackIds_(),
      cacheQueue_("cacheQueue"),
      streamParserInited_(false),
      parserRefIoContext_()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Create FFmpeg Demuxer Plugin.");
#ifndef _WIN32
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
#endif
    av_log_set_callback(FfmpegLogPrint);
#ifdef BUILD_ENG_VERSION
    std::string dumpModeStr = OHOS::system::GetParameter("FFmpegDemuxerPlugin.dump", "0");
    dumpMode_ = static_cast<DumpMode>(strtoul(dumpModeStr.c_str(), nullptr, 2)); // 2 is binary
    MEDIA_LOG_D("dump mode = %s(%lu)", dumpModeStr.c_str(), dumpMode_);
#endif
    MEDIA_LOG_D("Create FFmpeg Demuxer Plugin finish.");
}

FFmpegDemuxerPlugin::~FFmpegDemuxerPlugin()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Destroy FFmpeg Demuxer Plugin.");
#ifndef _WIN32
    (void)mallopt(M_FLUSH_THREAD_CACHE, 0);
#endif
    formatContext_ = nullptr;
    pluginImpl_ = nullptr;
    avbsfContext_ = nullptr;
    streamParser_ = nullptr;
    referenceParser_ = nullptr;
    parserRefFormatContext_ = nullptr;
    selectedTrackIds_.clear();
    if (firstFrame_ != nullptr) {
        av_packet_free(&firstFrame_);
        av_free(firstFrame_);
        firstFrame_ = nullptr;
    }
    MEDIA_LOG_D("Destroy FFmpeg Demuxer Plugin finish.");
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

Status FFmpegDemuxerPlugin::ParserRefUpdatePos(int64_t timeStampMs, bool isForward)
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_UNKNOWN, "formatContext is nullptr.");
    std::string formatName(formatContext_.get()->iformat->name);
    FALSE_RETURN_V_MSG_E(formatName.find("mp4") != std::string::npos, Status::ERROR_UNSUPPORTED_FORMAT,
                         "only support mp4.");
    int64_t clipTimeStampMs = std::clamp(timeStampMs, static_cast<int64_t>(0), formatContext_->duration / MS_TO_SEC);
    if (IFramePos_.size() == 0 || referenceParser_ == nullptr) {
        MEDIA_LOG_W("ParserRefUpdatePos fail, IFramePos size: " PUBLIC_LOG_ZU, IFramePos_.size());
        pendingSeekMsTime_ = clipTimeStampMs;
        updatePosIsForward_ = isForward;
        return Status::OK;
    }
    uint32_t frameId = TimeStampUs2FrameId(timeStampMs * MS_TO_SEC, fps_);
    int32_t gopId = std::upper_bound(IFramePos_.begin(), IFramePos_.end(), frameId) - IFramePos_.begin() - 1;
    GopLayerInfo gopLayerInfo;
    Status ret = GetGopLayerInfo(gopId, gopLayerInfo);
    if (ret == Status::ERROR_AGAIN && gopId != parserCurGopId_) {
        pendingSeekMsTime_ = clipTimeStampMs;
        parserState_ = false;
        MEDIA_LOG_I("pendingSeekMsTime: " PUBLIC_LOG_D64, pendingSeekMsTime_);
    }
    updatePosIsForward_ = isForward;
    return Status::OK;
}

void FFmpegDemuxerPlugin::ParserFirstDts()
{
    AVPacket *pkt = av_packet_alloc();
    bool isEnd = false;
    while (!isEnd) {
        int ffmpegRet = av_read_frame(parserRefFormatContext_.get(), pkt);
        if (ffmpegRet < 0) {
            av_packet_unref(pkt);
            av_packet_free(&pkt);
            return;
        }
        if (pkt->stream_index == parserRefVideoStreamIdx_) {
            firstDts_ = AvTime2Us(
                ConvertTimeFromFFmpeg(pkt->dts, parserRefFormatContext_->streams[parserRefVideoStreamIdx_]->time_base));
            MEDIA_LOG_I("Success to parser first dts: " PUBLIC_LOG_D64, firstDts_);
            isEnd = true;
        }
    }
    av_packet_unref(pkt);
    av_packet_free(&pkt);
}

Status FFmpegDemuxerPlugin::ParserRefInit()
{
    FALSE_RETURN_V_MSG_E(IFramePos_.size() > 0 && fps_ > 0, Status::ERROR_UNKNOWN,
                         "ParserRefInit failed, IFramePos size: " PUBLIC_LOG_ZU ", fps: " PUBLIC_LOG_F,
                         IFramePos_.size(), fps_);
    parserRefIoContext_.dataSource = ioContext_.dataSource;
    parserRefIoContext_.offset = 0;
    parserRefIoContext_.eos = false;
    if (parserRefIoContext_.dataSource->GetSeekable() == Plugins::Seekable::SEEKABLE) {
        parserRefIoContext_.dataSource->GetSize(parserRefIoContext_.fileSize);
    } else {
        parserRefIoContext_.fileSize = -1;
        MEDIA_LOG_E("not support oneline video");
        return Status::ERROR_INVALID_OPERATION;
    }
    parserRefFormatContext_ = InitAVFormatContext(&parserRefIoContext_);
    FALSE_RETURN_V_MSG_E(parserRefFormatContext_ != nullptr, Status::ERROR_UNKNOWN,
                         "ParserRefHeader failed due to can not init formatContext for source.");
    std::string formatName(parserRefFormatContext_.get()->iformat->name);
    FALSE_RETURN_V_MSG_E(formatName.find("mp4") != std::string::npos, Status::ERROR_UNSUPPORTED_FORMAT,
                         "only support mp4.");
    for (uint32_t trackIndex = 0; trackIndex < parserRefFormatContext_->nb_streams; trackIndex++) {
        AVStream *stream = parserRefFormatContext_->streams[trackIndex];
        if (stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            stream->discard = AVDISCARD_ALL;
        } else {
            parserRefVideoStreamIdx_ = trackIndex;
        }
    }
    FALSE_RETURN_V_MSG_E(parserRefVideoStreamIdx_ >= 0, Status::ERROR_UNKNOWN, "Can not find video stream.");
    AVStream *videoStream = parserRefFormatContext_->streams[parserRefVideoStreamIdx_];
    FALSE_RETURN_V_MSG_E(videoStream != nullptr, Status::ERROR_UNKNOWN, "Video stream is null.");
    processingIFrame_.assign(IFramePos_.begin(), IFramePos_.end());
    FALSE_RETURN_V_MSG_E(
        videoStream->codecpar->codec_id == AV_CODEC_ID_HEVC || videoStream->codecpar->codec_id == AV_CODEC_ID_H264,
        Status::ERROR_UNSUPPORTED_FORMAT, "ParserRefHeader failed due to codec type not support." PUBLIC_LOG_D32,
        videoStream->codecpar->codec_id);
    ParserFirstDts();
    CodecType codecType = videoStream->codecpar->codec_id == AV_CODEC_ID_HEVC ? CodecType::H265 : CodecType::H264;
    referenceParser_ = ReferenceParserManager::Create(codecType, IFramePos_);
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "reference is null.");
    ParserSdtpInfo *sc = (ParserSdtpInfo *)videoStream->priv_data;
    if (sc->sdtpCount > 0 && sc->sdtpData != nullptr) {
        MEDIA_LOG_E("sdtp exist: " PUBLIC_LOG_D32, sc->sdtpCount);
        if (referenceParser_->ParserSdtpData(sc->sdtpData, sc->sdtpCount) == Status::OK) {
            isSdtpExist_ = true;
            return Status::END_OF_STREAM;
        }
    }
    return referenceParser_->ParserExtraData(videoStream->codecpar->extradata, videoStream->codecpar->extradata_size);
}

Status FFmpegDemuxerPlugin::ParserRefInfoLoop(AVPacket *pkt, uint32_t curStreamId)
{
    std::unique_lock<std::mutex> sLock(syncMutex_);
    int ffmpegRet = av_read_frame(parserRefFormatContext_.get(), pkt);
    sLock.unlock();
    if (ffmpegRet < 0 && ffmpegRet != AVERROR_EOF) {
        MEDIA_LOG_E("Read frame failed due to av_read_frame failed:" PUBLIC_LOG_S ", retry: " PUBLIC_LOG_D32,
                    AVStrError(ffmpegRet).c_str(), int(parserRefIoContext_.retry));
        if (parserRefIoContext_.retry) {
            parserRefFormatContext_->pb->eof_reached = 0;
            parserRefFormatContext_->pb->error = 0;
            parserRefIoContext_.retry = false;
            return Status::ERROR_AGAIN;
        }
        return Status::ERROR_UNKNOWN;
    }
    if (pkt->stream_index != parserRefVideoStreamIdx_ && ffmpegRet != AVERROR_EOF) {
        return Status::OK;
    }
    int64_t dts = AvTime2Us(
        ConvertTimeFromFFmpeg(pkt->dts, parserRefFormatContext_->streams[parserRefVideoStreamIdx_]->time_base));
    referenceParser_->ParserNalUnits(pkt->data, pkt->size, curStreamId, dts);

    int32_t iFramePosSize = static_cast<int32_t>(IFramePos_.size());
    if (ffmpegRet == AVERROR_EOF ||
        (parserCurGopId_ + 1 < iFramePosSize && curStreamId == IFramePos_[parserCurGopId_ + 1] - 1)) { // 处理完一个GOP
        MEDIA_LOG_I("IFramePos size: " PUBLIC_LOG_ZU ", processingIFrame size: " PUBLIC_LOG_ZU
                    ", curStreamId: " PUBLIC_LOG_U32 ",parserCurGopId: " PUBLIC_LOG_U32,
                    IFramePos_.size(), processingIFrame_.size(), curStreamId, parserCurGopId_);
        processingIFrame_.remove(IFramePos_[parserCurGopId_]);
        if (processingIFrame_.size() == 0) {
            parserCurGopId_ = -1;
            return Status::OK;
        }
        int32_t tmpGopId = parserCurGopId_;
        while (formatContext_ != nullptr && std::find(processingIFrame_.begin(), processingIFrame_.end(),
                                                      IFramePos_[parserCurGopId_]) == processingIFrame_.end()) {
            if (updatePosIsForward_) {
                parserCurGopId_ = (parserCurGopId_ + 1) % iFramePosSize;
            } else {
                parserCurGopId_ = parserCurGopId_ == 0 ? iFramePosSize - 1 : parserCurGopId_ - 1;
            }
        }

        if (formatContext_ == nullptr || tmpGopId + 1 != parserCurGopId_ || !updatePosIsForward_) {
            return Status::END_OF_STREAM;
        }
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::SelectProGopId()
{
    if (pendingSeekMsTime_ >= 0) {
        uint32_t frameId = TimeStampUs2FrameId(pendingSeekMsTime_ * MS_TO_SEC, fps_);
        std::vector<uint32_t>::iterator frameLarger = std::upper_bound(IFramePos_.begin(), IFramePos_.end(), frameId);
        if (frameLarger - IFramePos_.begin() - 1 > std::numeric_limits<int32_t>::max() ||
            frameLarger - IFramePos_.begin() - 1 < std::numeric_limits<int32_t>::min()) {
            MEDIA_LOG_E("parserCurGopId_ overflow");
            return Status::ERROR_UNKNOWN;
        }
        parserCurGopId_ = frameLarger - IFramePos_.begin() - 1;
        pendingSeekMsTime_ = -1;
    }

    int32_t iFramePosSize = static_cast<int32_t>(IFramePos_.size());
    uint32_t gopSize = 0;
    if (parserRefFormatContext_->streams[parserRefVideoStreamIdx_]->nb_frames > std::numeric_limits<uint32_t>::max() ||
        parserRefFormatContext_->streams[parserRefVideoStreamIdx_]->nb_frames < 0) {
        MEDIA_LOG_E("nbFrames overflow");
        return Status::ERROR_UNKNOWN;
    }
    uint32_t nbFrames = static_cast<uint32_t>(parserRefFormatContext_->streams[parserRefVideoStreamIdx_]->nb_frames);
    if (parserCurGopId_ + 1 < iFramePosSize) {
        if (IFramePos_[parserCurGopId_ + 1] < IFramePos_[parserCurGopId_]) {
            MEDIA_LOG_E("gopSize underflow");
            return Status::ERROR_UNKNOWN;
        }
        gopSize = IFramePos_[parserCurGopId_ + 1] - IFramePos_[parserCurGopId_];
    } else {
        if (nbFrames < IFramePos_[parserCurGopId_]) {
            MEDIA_LOG_E("gopSize underflow");
            return Status::ERROR_UNKNOWN;
        }
        gopSize = nbFrames - IFramePos_[parserCurGopId_];
    }

    int64_t pts = ((IFramePos_[parserCurGopId_] + gopSize / 2) / fps_) / // 2
                  av_q2d(parserRefFormatContext_->streams[parserRefVideoStreamIdx_]->time_base);
    MEDIA_LOG_I("parserCurGopId_: " PUBLIC_LOG_U32 ", fps: " PUBLIC_LOG_F ", pts: " PUBLIC_LOG_D64
                ", gopSize: " PUBLIC_LOG_U32,
                parserCurGopId_, fps_, pts, gopSize);
    auto ret = av_seek_frame(parserRefFormatContext_.get(), parserRefVideoStreamIdx_, pts, AVSEEK_FLAG_BACKWARD);
    FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_UNKNOWN,
                         "Seek failed due to av_seek_frame failed, err: " PUBLIC_LOG_S ".", AVStrError(ret).c_str());
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ParserRefInfo()
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::OK, "formatContext is nullptr.");
    if (!isInit_) {
        isInit_ = true;
        Status ret = ParserRefInit();
        if (ret == Status::END_OF_STREAM) {
            return Status::OK;
        }
        if (ret != Status::OK) {
            return Status::ERROR_UNKNOWN;
        }
    }
    if (parserCurGopId_ == -1) {
        MEDIA_LOG_I("reference parser end");
        return Status::OK; // 参考关系解析完毕
    }

    FALSE_RETURN_V_MSG_E(SelectProGopId() == Status::OK, Status::ERROR_UNKNOWN, "SelectProGopId called fail.");
    uint32_t curStreamId = IFramePos_[parserCurGopId_];
    AVPacket *pkt = av_packet_alloc();
    while (formatContext_ != nullptr && parserState_ && parserCurGopId_ != -1) {
        Status rlt = ParserRefInfoLoop(pkt, curStreamId);
        if (rlt != Status::OK) {
            av_packet_unref(pkt);
            av_packet_free(&pkt);
            parserState_ = true;
            return rlt;
        }
        if (pkt->stream_index == parserRefVideoStreamIdx_) {
            curStreamId++;
        }
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    parserState_ = true;
    return Status::ERROR_AGAIN;
}

Status FFmpegDemuxerPlugin::GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "reference is null.");
    MEDIA_LOG_D("GetFrameLayerInfo called, dts: " PUBLIC_LOG_D64, videoSample->dts_);
    if (isSdtpExist_) {
        uint32_t frameId = TimeStampUs2FrameId(videoSample->dts_ - firstDts_, fps_);
        MEDIA_LOG_D("Success get frameId: " PUBLIC_LOG_U32, frameId);
        return referenceParser_->GetFrameLayerInfo(frameId, frameLayerInfo);
    }
    return referenceParser_->GetFrameLayerInfo(videoSample->dts_, frameLayerInfo);
}

Status FFmpegDemuxerPlugin::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "reference is null.");
    MEDIA_LOG_D("GetFrameLayerInfo called, dts: " PUBLIC_LOG_U32, frameId);
    return referenceParser_->GetFrameLayerInfo(frameId, frameLayerInfo);
}

Status FFmpegDemuxerPlugin::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "reference is null.");
    MEDIA_LOG_D("GetGopLayerInfo called, gopId: " PUBLIC_LOG_U32, gopId);
    return referenceParser_->GetGopLayerInfo(gopId, gopLayerInfo);
}

Status FFmpegDemuxerPlugin::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    FALSE_RETURN_V_MSG_E(IFramePos_.size() > 0, Status::ERROR_UNKNOWN, "IFramePos size is 0.");
    IFramePos = IFramePos_;
    return Status::OK;
}

Status FFmpegDemuxerPlugin::Dts2FrameId(int64_t dts, uint32_t &frameId, bool offset)
{
    int64_t tmpDts = offset ? (dts - firstDts_) : dts;
    frameId = TimeStampUs2FrameId(tmpDts, fps_);
    MEDIA_LOG_D("Success get frameId: " PUBLIC_LOG_D32 ", dts: " PUBLIC_LOG_D64, frameId, dts);
    return Status::OK;
}

Status FFmpegDemuxerPlugin::Reset()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Reset FFmpeg Demuxer Plugin.");
    readatIndex_ = 0;
    avpacketIndex_ = 0;
    ioContext_.offset = 0;
    ioContext_.eos = false;
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        cacheQueue_.RemoveTrackQueue(selectedTrackIds_[i]);
    }
    selectedTrackIds_.clear();
    pluginImpl_.reset();
    formatContext_.reset();
    avbsfContext_.reset();
    trackMtx_.clear();
    trackDfxInfoMap_.clear();
    return Status::OK;
}

void FFmpegDemuxerPlugin::InitBitStreamContext(const AVStream& avStream)
{
    FALSE_RETURN_MSG(avStream.codecpar != nullptr, "Init BitStreamContext failed due to codec par is nullptr.");
    AVCodecID codecID = avStream.codecpar->codec_id;
    MEDIA_LOG_D("Init BitStreamContext for track " PUBLIC_LOG_D32, avStream.index);
    FALSE_RETURN_MSG(g_bitstreamFilterMap.count(codecID) != 0,
        "Init BitStreamContext failed due to can not match any BitStreamContext.");
    const AVBitStreamFilter* avBitStreamFilter = av_bsf_get_by_name(g_bitstreamFilterMap.at(codecID).c_str());

    FALSE_RETURN_MSG((avBitStreamFilter != nullptr),
        "Init BitStreamContext failed due to av_bsf_get_by_name failed, name:" PUBLIC_LOG_S ".",
            g_bitstreamFilterMap.at(codecID).c_str());

    if (!avbsfContext_) {
        AVBSFContext* avbsfContext {nullptr};
        int ret = av_bsf_alloc(avBitStreamFilter, &avbsfContext);
        FALSE_RETURN_MSG((ret >= 0 && avbsfContext != nullptr),
            "Init BitStreamContext failed due to av_bsf_alloc failed, err:" PUBLIC_LOG_S ".", AVStrError(ret).c_str());

        ret = avcodec_parameters_copy(avbsfContext->par_in, avStream.codecpar);
        FALSE_RETURN_MSG((ret >= 0),
            "Init BitStreamContext failed due to avcodec_parameters_copy failed, err:" PUBLIC_LOG_S ".",
            AVStrError(ret).c_str());

        ret = av_bsf_init(avbsfContext);
        FALSE_RETURN_MSG((ret >= 0),
            "Init BitStreamContext failed due to av_bsf_init failed, err:" PUBLIC_LOG_S ".", AVStrError(ret).c_str());

        avbsfContext_ = std::shared_ptr<AVBSFContext>(avbsfContext, [](AVBSFContext* ptr) {
            if (ptr) {
                av_bsf_free(&ptr);
            }
        });
    }
    FALSE_RETURN_MSG(avbsfContext_ != nullptr,
        "Init BitStreamContext failed, name:" PUBLIC_LOG_S ", stream will not be converted to annexb",
            g_bitstreamFilterMap.at(codecID).c_str());
    MEDIA_LOG_D("Track " PUBLIC_LOG_D32 " will convert to annexb.", avStream.index);
}

void FFmpegDemuxerPlugin::ConvertAvcToAnnexb(AVPacket& pkt)
{
    int ret = av_bsf_send_packet(avbsfContext_.get(), &pkt);
    if (ret < 0) {
        MEDIA_LOG_E("Convert to annexb failed due to av_bsf_send_packet failed, err:" PUBLIC_LOG_S,
        AVStrError(ret).c_str());
    }
    av_packet_unref(&pkt);
    ret = 1;
    while (ret >= 0) {
        ret = av_bsf_receive_packet(avbsfContext_.get(), &pkt);
    }
    if (ret == AVERROR_EOF) {
        MEDIA_LOG_D("Convert avc to annexb successfully, ret:" PUBLIC_LOG_S ".", AVStrError(ret).c_str());
    }
}

void FFmpegDemuxerPlugin::ConvertHevcToAnnexb(AVPacket& pkt, std::shared_ptr<SamplePacket> samplePacket)
{
    size_t cencInfoSize = 0;
    uint8_t *cencInfo = av_packet_get_side_data(samplePacket->pkts[0], AV_PKT_DATA_ENCRYPTION_INFO,
        &cencInfoSize);
    streamParser_->ConvertPacketToAnnexb(&(pkt.data), pkt.size, cencInfo,
        static_cast<size_t>(cencInfoSize), false);
    if (NeedCombineFrame(samplePacket->pkts[0]->stream_index) &&
        streamParser_->IsSyncFrame(pkt.data, pkt.size)) {
        pkt.flags = static_cast<int32_t>(static_cast<uint32_t>(pkt.flags) | static_cast<uint32_t>(AV_PKT_FLAG_KEY));
    }
}

void FFmpegDemuxerPlugin::ConvertVvcToAnnexb(AVPacket& pkt, std::shared_ptr<SamplePacket> samplePacket)
{
    streamParser_->ConvertPacketToAnnexb(&(pkt.data), pkt.size, nullptr, 0, false);
}

Status FFmpegDemuxerPlugin::WriteBuffer(
    std::shared_ptr<AVBuffer> outBuffer, const uint8_t *writeData, int32_t writeSize)
{
    FALSE_RETURN_V_MSG_E(outBuffer != nullptr, Status::ERROR_NULL_POINTER,
        "Write data failed due to Buffer is nullptr.");
    if (writeData != nullptr && writeSize > 0) {
        FALSE_RETURN_V_MSG_E(outBuffer->memory_ != nullptr, Status::ERROR_NULL_POINTER,
            "Write data failed due to AVBuffer memory is nullptr.");
        int32_t ret = outBuffer->memory_->Write(writeData, writeSize, 0);
        FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_INVALID_OPERATION,
            "Write data failed due to AVBuffer memory write failed.");
    }

    MEDIA_LOG_D("CurrentBuffer: pts=" PUBLIC_LOG_D64 ", duration=" PUBLIC_LOG_D64 ", flag=" PUBLIC_LOG_U32,
        outBuffer->pts_, outBuffer->duration_, outBuffer->flag_);
    return Status::OK;
}

Status FFmpegDemuxerPlugin::SetDrmCencInfo(
    std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr, Status::ERROR_INVALID_OPERATION,
        "Convert packet info failed due to input sample is nullptr.");
    FALSE_RETURN_V_MSG_E((samplePacket != nullptr && samplePacket->pkts.size() > 0), Status::ERROR_INVALID_OPERATION,
        "Convert packet info failed due to input packet is nullptr.");
    FALSE_RETURN_V_MSG_E((samplePacket->pkts[0] != nullptr && samplePacket->pkts[0]->size >= 0),
        Status::ERROR_INVALID_OPERATION, "Convert packet info failed due to input packet is empty.");

    size_t cencInfoSize = 0;
    MetaDrmCencInfo *cencInfo = (MetaDrmCencInfo *)av_packet_get_side_data(samplePacket->pkts[0],
        AV_PKT_DATA_ENCRYPTION_INFO, &cencInfoSize);
    if ((cencInfo != nullptr) && (cencInfoSize != 0)) {
        std::vector<uint8_t> drmCencVec(reinterpret_cast<uint8_t *>(cencInfo),
            (reinterpret_cast<uint8_t *>(cencInfo)) + sizeof(MetaDrmCencInfo));
        sample->meta_->SetData(Media::Tag::DRM_CENC_INFO, std::move(drmCencVec));
    }
    return Status::OK;
}

bool FFmpegDemuxerPlugin::GetNextFrame(const uint8_t *data, const uint32_t size)
{
    if (size < NAL_START_CODE_SIZE) {
        return false;
    }
    bool hasShortStartCode = (data[0] == 0 && data[1] == 0 && data[2] == 1); // 001
    bool hasLongStartCode = (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1); // 0001
    return hasShortStartCode || hasLongStartCode;
}

bool FFmpegDemuxerPlugin::NeedCombineFrame(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, false, "formatContext_ is nullptr.");
    if (FFmpegFormatHelper::GetFileTypeByName(*formatContext_) == FileType::MPEGTS &&
        formatContext_->streams[trackId]->codecpar->codec_id == AV_CODEC_ID_HEVC) {
        return true;
    }
    return false;
}

AVPacket* FFmpegDemuxerPlugin::CombinePackets(std::shared_ptr<SamplePacket> samplePacket)
{
    AVPacket *tempPkt = nullptr;
    if (NeedCombineFrame(samplePacket->pkts[0]->stream_index) && samplePacket->pkts.size() > 1) {
        int totalSize = 0;
        for (auto pkt : samplePacket->pkts) {
            FALSE_RETURN_V_MSG_E(pkt != nullptr, nullptr, "ConvertAVPacketToSample failed due to pkt is nullptr");
            totalSize += pkt->size;
        }
        tempPkt = av_packet_alloc();
        FALSE_RETURN_V_MSG_E(tempPkt != nullptr, nullptr, "ConvertAVPacketToSample failed due to tempPkt is nullptr");
        int ret = av_new_packet(tempPkt, totalSize);
        FALSE_RETURN_V_MSG_E(ret >= 0, nullptr, "av_new_packet failed");
        av_packet_copy_props(tempPkt, samplePacket->pkts[0]);
        int offset = 0;
        bool copySuccess = true;
        for (auto pkt : samplePacket->pkts) {
            if (pkt == nullptr) {
                copySuccess = false;
                MEDIA_LOG_E("cache pkt is nullptr");
                break;
            }
            ret = memcpy_s(tempPkt->data + offset, pkt->size, pkt->data, pkt->size);
            if (ret != EOK) {
                copySuccess = false;
                MEDIA_LOG_E("memcpy_s failed, ret=" PUBLIC_LOG_D32, ret);
                break;
            }
            offset += pkt->size;
        }
        if (!copySuccess) {
            av_packet_free(&tempPkt);
            av_free(tempPkt);
            tempPkt = nullptr;
            return nullptr;
        }
        tempPkt->size = totalSize;
        MEDIA_LOG_D("Combine " PUBLIC_LOG_ZU " packets, total size=" PUBLIC_LOG_D32,
            samplePacket->pkts.size(), totalSize);
    } else {
        tempPkt = samplePacket->pkts[0];
    }
    return tempPkt;
}

void FFmpegDemuxerPlugin::ConvertPacketToAnnexb(std::shared_ptr<AVBuffer> sample, AVPacket* srcAVPacket,
    std::shared_ptr<SamplePacket> dstSamplePacket)
{
    auto codecId = formatContext_->streams[srcAVPacket->stream_index]->codecpar->codec_id;
    if (codecId == AV_CODEC_ID_HEVC && streamParser_ != nullptr && streamParserInited_) {
        ConvertHevcToAnnexb(*srcAVPacket, dstSamplePacket);
        SetDropTag(*srcAVPacket, sample, AV_CODEC_ID_HEVC);
    } else if (codecId == AV_CODEC_ID_VVC && streamParser_ != nullptr && streamParserInited_) {
        ConvertVvcToAnnexb(*srcAVPacket, dstSamplePacket);
    } else if (codecId == AV_CODEC_ID_H264 && avbsfContext_ != nullptr) {
        ConvertAvcToAnnexb(*srcAVPacket);
        SetDropTag(*srcAVPacket, sample, AV_CODEC_ID_H264);
    }
}

void FFmpegDemuxerPlugin::WriteBufferAttr(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket)
{
    AVStream *avStream = formatContext_->streams[samplePacket->pkts[0]->stream_index];
    if (samplePacket->pkts[0]->pts != AV_NOPTS_VALUE) {
        sample->pts_ = AvTime2Us(ConvertTimeFromFFmpeg(samplePacket->pkts[0]->pts, avStream->time_base));
    }
    // durantion dts
    if (samplePacket->pkts[0]->duration != AV_NOPTS_VALUE) {
        int64_t duration = AvTime2Us(ConvertTimeFromFFmpeg(samplePacket->pkts[0]->duration, avStream->time_base));
        sample->duration_ = duration;
        sample->meta_->SetData(Media::Tag::BUFFER_DURATION, duration);
    }
    if (samplePacket->pkts[0]->dts != AV_NOPTS_VALUE) {
        int64_t dts = AvTime2Us(ConvertTimeFromFFmpeg(samplePacket->pkts[0]->dts, avStream->time_base));
        sample->dts_ = dts;
        sample->meta_->SetData(Media::Tag::BUFFER_DECODING_TIMESTAMP, dts);
    }
}

Status FFmpegDemuxerPlugin::ConvertAVPacketToSample(
    std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket)
{
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr && samplePacket->pkts.size() > 0 &&
        samplePacket->pkts[0] != nullptr && samplePacket->pkts[0]->size >= 0,
        Status::ERROR_INVALID_OPERATION, "Convert packet info failed due to input packet is nullptr or empty.");
    MEDIA_LOG_D("Convert packet info for track " PUBLIC_LOG_D32, samplePacket->pkts[0]->stream_index);
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr, Status::ERROR_INVALID_OPERATION,
        "Convert packet info failed due to input sample is nullptr.");

    WriteBufferAttr(sample, samplePacket);

    // convert
    AVPacket *tempPkt = CombinePackets(samplePacket);
    FALSE_RETURN_V_MSG_E(tempPkt != nullptr, Status::ERROR_INVALID_OPERATION, "tempPkt is empty.");
    ConvertPacketToAnnexb(sample, tempPkt, samplePacket);
    // flag\copy
    int32_t remainSize = tempPkt->size - static_cast<int32_t>(samplePacket->offset);
    int32_t copySize = remainSize < sample->memory_->GetCapacity() ? remainSize : sample->memory_->GetCapacity();
    MEDIA_LOG_D("packet size=" PUBLIC_LOG_D32 ", remain size=" PUBLIC_LOG_D32, tempPkt->size, remainSize);
    MEDIA_LOG_D("copySize=" PUBLIC_LOG_D32 ", copyOffset" PUBLIC_LOG_D32, copySize, samplePacket->offset);
    uint32_t flag = ConvertFlagsFromFFmpeg(*tempPkt, (copySize != tempPkt->size));
    SetDrmCencInfo(sample, samplePacket);

    sample->flag_ = flag;
    Status ret = WriteBuffer(sample, tempPkt->data + samplePacket->offset, copySize);
    if (!samplePacket->isEOS) {
        trackDfxInfoMap_[tempPkt->stream_index].lastPts = sample->pts_;
        trackDfxInfoMap_[tempPkt->stream_index].lastDurantion = sample->duration_;
        trackDfxInfoMap_[tempPkt->stream_index].lastPos = tempPkt->pos;
    }
#ifdef BUILD_ENG_VERSION
    DumpParam dumpParam {DumpMode(DUMP_AVBUFFER_OUTPUT & dumpMode_), tempPkt->data + samplePacket->offset,
        tempPkt->stream_index, -1, copySize, trackDfxInfoMap_[tempPkt->stream_index].frameIndex++, tempPkt->pts, -1};
    Dump(dumpParam);
#endif
    if (tempPkt != nullptr && tempPkt->size != samplePacket->pkts[0]->size) {
        av_packet_free(&tempPkt);
        av_free(tempPkt);
        tempPkt = nullptr;
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Convert packet info failed due to write buffer failed.");
    if (copySize < remainSize) {
        samplePacket->offset += static_cast<uint32_t>(copySize);
        MEDIA_LOG_D("Buffer is not enough, next buffer to save remain data.");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::PushEOSToAllCache()
{
    Status ret = Status::OK;
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        auto streamIndex = selectedTrackIds_[i];
        MEDIA_LOG_I("Push eos into the cache " PUBLIC_LOG_D32 ".", streamIndex);
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
    } else {    // vtte
        if (cacheQueue_.HasCache(trackId)) {
            std::shared_ptr<SamplePacket> cacheSamplePacket = cacheQueue_.Back(static_cast<uint32_t>(trackId));
            if (cacheSamplePacket != nullptr && cacheSamplePacket->pkts[0]->duration == 0) {
                cacheSamplePacket->pkts[0]->duration = pkt->pts - cacheSamplePacket->pkts[0]->pts;
            }
        }
    }
    av_packet_free(&pkt);
    return true;
}

bool FFmpegDemuxerPlugin::IsWebvttMP4(const AVStream *avStream)
{
    if (avStream->codecpar->codec_id == AV_CODEC_ID_WEBVTT &&
        FFmpegFormatHelper::GetFileTypeByName(*formatContext_) == FileType::MP4) {
        return true;
    }
    return false;
}

void FFmpegDemuxerPlugin::WebvttMP4EOSProcess(const AVPacket *pkt)
{
    if (pkt != nullptr) {
        auto trackId = pkt->stream_index;
        AVStream *avStream = formatContext_->streams[trackId];
        if (IsWebvttMP4(avStream) && pkt->size == 0 && cacheQueue_.HasCache(trackId)) {
            std::shared_ptr<SamplePacket> cacheSamplePacket = cacheQueue_.Back(static_cast<uint32_t>(trackId));
            if (cacheSamplePacket != nullptr && cacheSamplePacket->pkts[0]->duration == 0) {
                cacheSamplePacket->pkts[0]->duration =
                    formatContext_->streams[pkt->stream_index]->duration - cacheSamplePacket->pkts[0]->pts;
            }
        }
    }
}

Status FFmpegDemuxerPlugin::ReadPacketToCacheQueue(const uint32_t readId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    AVPacket *pkt = nullptr;
    bool continueRead = true;
    Status ret = Status::OK;
    while (continueRead) {
        if (pkt == nullptr) {
            pkt = av_packet_alloc();
            FALSE_RETURN_V_MSG_E(pkt != nullptr, Status::ERROR_NULL_POINTER, "av_packet_alloc failed.");
        }
        std::unique_lock<std::mutex> sLock(syncMutex_);
        int ffmpegRet = av_read_frame(formatContext_.get(), pkt);
        sLock.unlock();
        if (ffmpegRet == AVERROR_EOF) { // eos
            WebvttMP4EOSProcess(pkt);
            av_packet_free(&pkt);
            ret = PushEOSToAllCache();
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "PushEOSToAllCache failed.");
            return Status::END_OF_STREAM;
        }
        if (ffmpegRet < 0) { // fail
            av_packet_free(&pkt);
            MEDIA_LOG_E("Read frame failed due to av_read_frame failed:" PUBLIC_LOG_S ", retry: " PUBLIC_LOG_D32,
                AVStrError(ffmpegRet).c_str(), int(ioContext_.retry));
            if (ioContext_.retry) {
                formatContext_->pb->eof_reached = 0;
                formatContext_->pb->error = 0;
                ioContext_.retry = false;
                return Status::ERROR_AGAIN;
            }
            return Status::ERROR_UNKNOWN;
        }
        auto trackId = pkt->stream_index;
        if (!TrackIsSelected(trackId)) {
            av_packet_unref(pkt);
            continue;
        }
        AVStream *avStream = formatContext_->streams[trackId];
        if (IsWebvttMP4(avStream) && WebvttPktProcess(pkt)) {
            break;
        } else if (!IsWebvttMP4(avStream) && (!NeedCombineFrame(readId) ||
            (cacheQueue_.HasCache(static_cast<uint32_t>(trackId)) && GetNextFrame(pkt->data, pkt->size)))) {
            continueRead = false;
        }
        ret = AddPacketToCacheQueue(pkt);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "AddPacketToCacheQueue failed.");
        pkt = nullptr;
    }
    return ret;
}

Status FFmpegDemuxerPlugin::SetEosSample(std::shared_ptr<AVBuffer> sample)
{
    MEDIA_LOG_D("Set EOS buffer.");
    sample->pts_ = 0;
    sample->flag_ =  (uint32_t)(AVBufferFlag::EOS);
    Status ret = WriteBuffer(sample, nullptr, 0);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Push EOS buffer failed due to write buffer failed.");
    MEDIA_LOG_I("Set EOS buffer finish.");
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
    FALSE_RETURN_V_MSG_E(ioContext != nullptr, ret, "AVReadPacket failed due to IOContext error.");
    FALSE_RETURN_V_MSG_E(ioContext->dataSource != nullptr, ret, "AVReadPacket failed due to dataSource error.");
    FALSE_RETURN_V_MSG_E(ioContext->offset <= INT64_MAX - static_cast<int64_t>(bufSize),
        ret, "AVReadPacket failed due to offset invalid.");

    if (ioContext->dataSource->IsDash() && ioContext->eos == true) {
        MEDIA_LOG_I("AVReadPacket return EOS");
        return AVERROR_EOF;
    }

    MEDIA_LOG_D("Offset: " PUBLIC_LOG_D64 ", totalSize: " PUBLIC_LOG_U64, ioContext->offset, ioContext->fileSize);
    if (ioContext->fileSize > 0) {
        FALSE_RETURN_V_MSG_E(static_cast<uint64_t>(ioContext->offset) <= ioContext->fileSize, ret,
            "Offset out of file size.");
        if (static_cast<size_t>(ioContext->offset + bufSize) > ioContext->fileSize) {
            bufSize = static_cast<int64_t>(ioContext->fileSize) - ioContext->offset;
        }
    }
    return 0;
}

// Write packet data into the buffer provided by ffmpeg
int FFmpegDemuxerPlugin::AVReadPacket(void* opaque, uint8_t* buf, int bufSize)
{
    int ret = CheckContextIsValid(opaque, bufSize);
    FALSE_RETURN_V(ret == 0, ret);

    ret = -1;
    auto ioContext = static_cast<IOContext*>(opaque);
    FALSE_RETURN_V_MSG_E(ioContext != nullptr, ret, "ioContext is nullptr");
    auto buffer = std::make_shared<Buffer>();
    FALSE_RETURN_V_MSG_E(buffer != nullptr, ret, "buffer is nullptr");
    auto bufData = buffer->WrapMemory(buf, bufSize, 0);
    FALSE_RETURN_V_MSG_E(buffer->GetMemory() != nullptr, ret, "AVReadPacket buf is nullptr");

    MediaAVCodec::AVCodecTrace trace("AVReadPacket_ReadAt");
    auto result = ioContext->dataSource->ReadAt(ioContext->offset, buffer, static_cast<size_t>(bufSize));
    int dataSize = static_cast<int>(buffer->GetMemory()->GetSize());
    MEDIA_LOG_D("Want data size:" PUBLIC_LOG_D32 ", Get data size:" PUBLIC_LOG_D32 ", offset:" PUBLIC_LOG_D64
        ", readatIndex:" PUBLIC_LOG_D32, bufSize, dataSize, ioContext->offset, readatIndex_.load());
#ifdef BUILD_ENG_VERSION
    DumpParam dumpParam {DumpMode(DUMP_READAT_INPUT & ioContext->dumpMode), buf, -1, ioContext->offset,
        dataSize, readatIndex_++, -1, -1};
    Dump(dumpParam);
#endif
    switch (result) {
        case Status::OK:
        case Status::ERROR_AGAIN:
            if (dataSize == 0) {
                MEDIA_LOG_I("Read data not enough, read again.");
                ioContext->retry = true;
            } else {
                ioContext->offset += dataSize;
                ret = dataSize;
            }
            break;
        case Status::END_OF_STREAM:
            MEDIA_LOG_I("Read at end of file.");
            ioContext->eos = true;
            ret = AVERROR_EOF;
            break;
        default:
            MEDIA_LOG_I("AVReadPacket failed, result=" PUBLIC_LOG_D32 ".", static_cast<int>(result));
            break;
    }

    if (!ioContext->initCompleted) {
        if (ioContext->initDownloadDataSize <= UINT32_MAX - static_cast<uint32_t>(dataSize)) {
            ioContext->initDownloadDataSize += static_cast<uint32_t>(dataSize);
        } else {
            MEDIA_LOG_W("dataSize " PUBLIC_LOG_U32 " is invalid", static_cast<uint32_t>(dataSize));
        }
    }

    return ret;
}

int64_t FFmpegDemuxerPlugin::AVSeek(void* opaque, int64_t offset, int whence)
{
    auto ioContext = static_cast<IOContext*>(opaque);
    uint64_t newPos = 0;
    FALSE_RETURN_V_MSG_E(ioContext != nullptr, newPos, "AVSeek failed due to IOContext error.");
    switch (whence) {
        case SEEK_SET:
            newPos = static_cast<uint64_t>(offset);
            ioContext->offset = newPos;
            MEDIA_LOG_D("AVSeek whence: " PUBLIC_LOG_D32 ", pos = " PUBLIC_LOG_D64 ", newPos = " PUBLIC_LOG_U64 ".",
                whence, offset, newPos);
            break;
        case SEEK_CUR:
            newPos = ioContext->offset + offset;
            MEDIA_LOG_D("AVSeek whence: " PUBLIC_LOG_D32 ", pos = " PUBLIC_LOG_D64 ", newPos = " PUBLIC_LOG_U64 ".",
                whence, offset, newPos);
            break;
        case SEEK_END:
        case AVSEEK_SIZE: {
            if (ioContext->dataSource->IsDash()) {
                return -1;
            }
            uint64_t mediaDataSize = 0;
            FALSE_RETURN_V_MSG_E(ioContext->dataSource != nullptr, newPos,
                "AVSeek failed due to dataSource is nullptr.");
            if (ioContext->dataSource->GetSize(mediaDataSize) == Status::OK && (mediaDataSize > 0)) {
                newPos = mediaDataSize + offset;
                MEDIA_LOG_D("AVSeek whence: " PUBLIC_LOG_D32 ", pos = " PUBLIC_LOG_D64
                    ", newPos = " PUBLIC_LOG_U64 ".", whence, offset, newPos);
            }
            break;
        }
        default:
            MEDIA_LOG_E("AVSeek unexpected whence: " PUBLIC_LOG_D32 ".", whence);
            break;
    }
    if (whence != AVSEEK_SIZE) {
        ioContext->offset = newPos;
    }
    MEDIA_LOG_D("Current offset: " PUBLIC_LOG_D64 ", new pos: " PUBLIC_LOG_U64 ".",
        ioContext->offset, newPos);
    return newPos;
}

AVIOContext* FFmpegDemuxerPlugin::AllocAVIOContext(int flags, IOContext *ioContext)
{
    auto buffer = static_cast<unsigned char*>(av_malloc(DEFAULT_READ_SIZE));
    FALSE_RETURN_V_MSG_E(buffer != nullptr, nullptr, "Alloc AVIOContext failed due to av_malloc failed.");

    AVIOContext* avioContext = avio_alloc_context(
        buffer, DEFAULT_READ_SIZE, flags & AVIO_FLAG_WRITE, static_cast<void*>(ioContext),
        AVReadPacket, AVWritePacket, AVSeek);
    if (avioContext == nullptr) {
        MEDIA_LOG_E("Alloc AVIOContext failed due to avio_alloc_context failed.");
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

std::shared_ptr<AVFormatContext> FFmpegDemuxerPlugin::InitAVFormatContext(IOContext *ioContext)
{
    AVFormatContext* formatContext = avformat_alloc_context();
    FALSE_RETURN_V_MSG_E(formatContext != nullptr, nullptr, "Alloc formatContext failed.");
    formatContext->pb = AllocAVIOContext(AVIO_FLAG_READ, ioContext);
    FALSE_RETURN_V_MSG_E(formatContext->pb != nullptr, nullptr, "Alloc iOContext failed.");
    formatContext->flags = static_cast<uint32_t>(formatContext->flags) | static_cast<uint32_t>(AVFMT_FLAG_CUSTOM_IO);
    if (std::string(pluginImpl_->name) == "mp3") {
        formatContext->flags =
            static_cast<uint32_t>(formatContext->flags) | static_cast<uint32_t>(AVFMT_FLAG_FAST_SEEK);
    }
    AVDictionary *options = nullptr;
    if (ioContext_.dataSource->IsDash()) {
        av_dict_set(&options, "use_tfdt", "true", 0);
    }
    MediaAVCodec::AVCodecTrace trace("ffmpeg_init");
    auto begin = std::chrono::system_clock::now();
    int ret = avformat_open_input(&formatContext, nullptr, pluginImpl_.get(), &options);
    FALSE_RETURN_V_MSG_E((ret == 0), nullptr, "FormatContext init failed by " PUBLIC_LOG_S ", err:" PUBLIC_LOG_S,
        pluginImpl_->name, AVStrError(ret).c_str());
    auto finishiOpen = std::chrono::system_clock::now();
    if (FFmpegFormatHelper::GetFileTypeByName(*formatContext) == FileType::FLV) { // Fix init live-flv-source too slow
        formatContext->probesize = LIVE_FLV_PROBE_SIZE;
    }
    FALSE_RETURN_V_MSG_E(formatContext->pb->buffer != nullptr, nullptr, "Custom buffer invalid.");
    ret = avformat_find_stream_info(formatContext, NULL);
    auto finishParse = std::chrono::system_clock::now();
    MEDIA_LOG_I("spend: open " PUBLIC_LOG_F " parse " PUBLIC_LOG_F,
        static_cast<std::chrono::duration<double, std::milli>>(finishiOpen - begin).count(),
        static_cast<std::chrono::duration<double, std::milli>>(finishParse - finishiOpen).count());
    FALSE_RETURN_V_MSG_E((ret >= 0), nullptr, "Parse stream info failed by " PUBLIC_LOG_S ", err:" PUBLIC_LOG_S,
        pluginImpl_->name, AVStrError(ret).c_str());
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
        MEDIA_LOG_I("init download data size = %{public}u.", ioContext_.initDownloadDataSize);
        MediaAVCodec::DemuxerInitEventWrite(ioContext_.initDownloadDataSize, pluginName_);
    }
}

void FFmpegDemuxerPlugin::ParserBoxInfo()
{
    std::string formatName(formatContext_.get()->iformat->name);
    if (formatName.find("mp4") == std::string::npos) {
        MEDIA_LOG_D("ParserBoxInfo only support mp4, do not support: " PUBLIC_LOG_S, formatName.c_str());
        return;
    }
    int videoStreamIdx = av_find_best_stream(formatContext_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    FALSE_RETURN_MSG(videoStreamIdx >= 0, "ParserBoxInfo failed due to can not find video stream.");
    AVStream *videoStream = formatContext_->streams[videoStreamIdx];
    FALSE_RETURN_MSG(videoStream != nullptr, "ParserBoxInfo failed due to video stream is null.");
    if (videoStream->avg_frame_rate.den == 0 || videoStream->avg_frame_rate.num == 0) {
        fps_ = videoStream->r_frame_rate.num / (double)videoStream->r_frame_rate.den;
    } else {
        fps_ = videoStream->avg_frame_rate.num / (double)videoStream->avg_frame_rate.den;
    }
    FFStream *sti = ffstream(videoStream);
    for (int32_t i = 0; i < sti->nb_index_entries; i++) {
        uint32_t flags = static_cast<uint32_t>(sti->index_entries[i].flags);
        if (flags & AVINDEX_KEYFRAME) {
            IFramePos_.emplace_back(i);
        }
    }
    MEDIA_LOG_I("Success to parser fps: " PUBLIC_LOG_F ", IFramePos size: " PUBLIC_LOG_ZU, fps_, IFramePos_.size());
}

Status FFmpegDemuxerPlugin::SetDataSource(const std::shared_ptr<DataSource>& source)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ == nullptr, Status::ERROR_WRONG_STATE,
        "DataSource has been inited, need reset first.");
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set datasource failed due to source is nullptr.");
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
    MEDIA_LOG_I("fileSize: " PUBLIC_LOG_U64 ", seekable: " PUBLIC_LOG_D32, ioContext_.fileSize, seekable_);
    {
        std::lock_guard<std::mutex> glock(g_mtx);
        pluginImpl_ = g_pluginInputFormat[pluginName_];
    }
    FALSE_RETURN_V_MSG_E(pluginImpl_ != nullptr, Status::ERROR_UNSUPPORTED_FORMAT,
        "Set datasource failed due to can not find inputformat for format.");
    formatContext_ = InitAVFormatContext(&ioContext_);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_UNKNOWN,
        "Set datasource failed due to can not init formatContext for source.");
    InitParser();

    NotifyInitializationCompleted();
    MEDIA_LOG_I("SetDataSource finish.");
    cachelimitSize_ = DEFAULT_CACHE_LIMIT;
    return Status::OK;
}

void FFmpegDemuxerPlugin::InitParser()
{
    FALSE_RETURN_MSG(formatContext_ != nullptr, "InitParser failed.");
    ParserBoxInfo();
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        if (g_bitstreamFilterMap.count(formatContext_->streams[trackIndex]->codecpar->codec_id) != 0) {
            InitBitStreamContext(*(formatContext_->streams[trackIndex]));
            break;
        }
        if (HaveValidParser(formatContext_->streams[trackIndex]->codecpar->codec_id)) {
            streamParser_ = StreamParserManager::Create(g_streamParserMap.at(
                formatContext_->streams[trackIndex]->codecpar->codec_id));
            if (streamParser_ == nullptr) {
                MEDIA_LOG_W("Init hevc/vvc parser failed");
            } else {
                MEDIA_LOG_I("Track " PUBLIC_LOG_D32 " will convert to annexb.", trackIndex);
            }
            break;
        }
    }
}

Status FFmpegDemuxerPlugin::GetSeiInfo()
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "GetSeiInfo failed due to formatContext_ is nullptr.");
    Status ret = Status::OK;
    if (streamParser_ != nullptr && !streamParserInited_) {
        for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
            auto avStream = formatContext_->streams[trackIndex];
            if (HaveValidParser(avStream->codecpar->codec_id)) {
                ret = GetVideoFirstKeyFrame(trackIndex);
                FALSE_RETURN_V_MSG_E(ret != Status::ERROR_NO_MEMORY, Status::ERROR_NO_MEMORY,
                    "Get first frame failed is due to error no memory");
                FALSE_RETURN_V_MSG_E(firstFrame_ != nullptr && firstFrame_->data != nullptr,
                    Status::ERROR_WRONG_STATE, "Get first frame failed. Get sei info may failed.");
                streamParser_->ConvertExtraDataToAnnexb(
                    avStream->codecpar->extradata, avStream->codecpar->extradata_size);
                streamParserInited_ = true;
                break;
            }
        }
    }
    return ret;
}

Status FFmpegDemuxerPlugin::GetMediaInfo(MediaInfo& mediaInfo)
{
    MediaAVCodec::AVCodecTrace trace("FFmpegDemuxerPlugin::GetMediaInfo");
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Get media info failed due to formatContext_ is nullptr.");

    Status ret = GetSeiInfo();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "GetSeiInfo failed");

    FFmpegFormatHelper::ParseMediaInfo(*formatContext_, mediaInfo.general);
    StringifyMeta(mediaInfo.general, -1); // source meta
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        Meta meta;
        auto avStream = formatContext_->streams[trackIndex];
        if (avStream == nullptr) {
            MEDIA_LOG_W("Get track " PUBLIC_LOG_D32 " info failed due to track is nullptr.", trackIndex);
            mediaInfo.tracks.push_back(meta);
            continue;
        }
        FFmpegFormatHelper::ParseTrackInfo(*avStream, meta, *formatContext_);
        if (avStream->codecpar->codec_id == AV_CODEC_ID_HEVC) {
            if (streamParser_ != nullptr && streamParserInited_ && firstFrame_ != nullptr) {
                streamParser_->ConvertPacketToAnnexb(&(firstFrame_->data), firstFrame_->size, nullptr, 0, false);
                streamParser_->ParseAnnexbExtraData(firstFrame_->data, firstFrame_->size);
                // Parser only sends xps info when first call ConvertPacketToAnnexb
                // readSample will call ConvertPacketToAnnexb again, so rest here
                streamParser_->ResetXPSSendStatus();
                ParseHEVCMetadataInfo(*avStream, meta);
            } else {
                MEDIA_LOG_W("Parser hevc info fail");
            }
        }
        if (avStream->codecpar->codec_id == AV_CODEC_ID_HEVC ||
            avStream->codecpar->codec_id == AV_CODEC_ID_H264 ||
            avStream->codecpar->codec_id == AV_CODEC_ID_VVC) {
            ConvertCsdToAnnexb(*avStream, meta);
        }
        mediaInfo.tracks.push_back(meta);
        StringifyMeta(meta, trackIndex);
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::GetUserMeta(std::shared_ptr<Meta> meta)
{
    MediaAVCodec::AVCodecTrace trace("FFmpegDemuxerPlugin::GetUserMeta");
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Get user data failed due to formatContext_ is nullptr.");
    FALSE_RETURN_V_MSG_E(meta != nullptr, Status::ERROR_NULL_POINTER,
        "Get user data failed due to meta is nullptr.");
    
    FFmpegFormatHelper::ParseUserMeta(*formatContext_, meta);
    return Status::OK;
}

void FFmpegDemuxerPlugin::ParseDrmInfo(const MetaDrmInfo *const metaDrmInfo, int32_t drmInfoSize,
    std::multimap<std::string, std::vector<uint8_t>>& drmInfo)
{
    MEDIA_LOG_D("ParseDrmInfo.");
    uint32_t infoCount = drmInfoSize / sizeof(MetaDrmInfo);
    for (uint32_t index = 0; index < infoCount; index++) {
        std::stringstream ssConverter;
        std::string uuid;
        for (uint32_t i = 0; i < metaDrmInfo[index].uuidLen; i++) {
            ssConverter << std::hex << static_cast<int32_t>(metaDrmInfo[index].uuid[i]);
            uuid = ssConverter.str();
        }
        drmInfo.insert({ uuid, std::vector<uint8_t>(metaDrmInfo[index].pssh,
            metaDrmInfo[index].pssh + metaDrmInfo[index].psshLen) });
    }
}

Status FFmpegDemuxerPlugin::GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo)
{
    MEDIA_LOG_D("GetDrmInfo");
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "GetDrmInfo failed due to formatContext_ is nullptr.");

    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        Meta meta;
        AVStream *avStream = formatContext_->streams[trackIndex];
        if (avStream == nullptr) {
            MEDIA_LOG_W("GetDrmInfo for track " PUBLIC_LOG_D32 " info failed due to track is nullptr.", trackIndex);
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
    if (HaveValidParser(avStream.codecpar->codec_id) && streamParser_ != nullptr && streamParserInited_) {
        streamParser_->ConvertPacketToAnnexb(&(extradata), extradataSize, nullptr, 0, true);
    } else if (avStream.codecpar->codec_id == AV_CODEC_ID_H264 && avbsfContext_ != nullptr) {
        if (avbsfContext_->par_out->extradata != nullptr && avbsfContext_->par_out->extradata_size > 0) {
            extradata = avbsfContext_->par_out->extradata;
            extradataSize = avbsfContext_->par_out->extradata_size;
        }
    }
    if (extradata != nullptr && extradataSize > 0) {
        std::vector<uint8_t> extra(extradataSize);
        extra.assign(extradata, extradata + extradataSize);
        format.Set<Tag::MEDIA_CODEC_CONFIG>(extra);
    }
}

Status FFmpegDemuxerPlugin::AddPacketToCacheQueue(AVPacket *pkt)
{
#ifdef BUILD_ENG_VERSION
    if (pkt == nullptr) {
        MEDIA_LOG_D("Dump failed due to pkt is nullptr.");
    } else {
        DumpParam dumpParam {DumpMode(DUMP_AVPACKET_OUTPUT & dumpMode_), pkt->data, pkt->stream_index, -1, pkt->size,
            avpacketIndex_++, pkt->pts, pkt->pos};
        Dump(dumpParam);
    }
#endif
    auto trackId = pkt->stream_index;
    Status ret = Status::OK;
    if (NeedCombineFrame(trackId) && !GetNextFrame(pkt->data, pkt->size) && cacheQueue_.HasCache(trackId)) {
        std::shared_ptr<SamplePacket> cacheSamplePacket = cacheQueue_.Back(static_cast<uint32_t>(trackId));
        if (cacheSamplePacket != nullptr) {
            cacheSamplePacket->pkts.push_back(pkt);
        }
    } else {
        std::shared_ptr<SamplePacket> cacheSamplePacket = std::make_shared<SamplePacket>();
        if (cacheSamplePacket != nullptr) {
            cacheSamplePacket->pkts.push_back(pkt);
            cacheSamplePacket->offset = 0;
            cacheQueue_.Push(static_cast<uint32_t>(trackId), cacheSamplePacket);
            ret = CheckCacheDataLimit(static_cast<uint32_t>(trackId));
        }
    }
    return ret;
}

Status FFmpegDemuxerPlugin::GetVideoFirstKeyFrame(uint32_t trackIndex)
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "formatContext_ is null");
    AVPacket *pkt = nullptr;
    Status ret = Status::OK;
    while (1) {
        if (pkt == nullptr) {
            pkt = av_packet_alloc();
            FALSE_RETURN_V_MSG_E(pkt != nullptr, Status::ERROR_NULL_POINTER, "av_packet_alloc fail");
        }

        std::unique_lock<std::mutex> sLock(syncMutex_);
        int ffmpegRet = av_read_frame(formatContext_.get(), pkt);
        sLock.unlock();
        if (ffmpegRet < 0) {
            MEDIA_LOG_E("av_read_frame fail, ret=" PUBLIC_LOG_D32, ffmpegRet);
            av_packet_unref(pkt);
            break;
        }
        cacheQueue_.AddTrackQueue(pkt->stream_index);
        ret = AddPacketToCacheQueue(pkt);
        if (ret != Status::OK) {
            return ret;
        }

        if (static_cast<uint32_t>(pkt->stream_index) == trackIndex) {
            firstFrame_ = av_packet_alloc();
            FALSE_RETURN_V_MSG_E(firstFrame_ != nullptr, Status::ERROR_NULL_POINTER, "av_packet_alloc fail");
            int avRet = av_new_packet(firstFrame_, pkt->size);
            FALSE_RETURN_V_MSG_E(avRet >= 0, Status::ERROR_INVALID_DATA, "av_new_packet fail");
            av_packet_copy_props(firstFrame_, pkt);
            memcpy_s(firstFrame_->data, pkt->size, pkt->data, pkt->size);
            break;
        }
        pkt = nullptr;
    }
    return ret;
}

void FFmpegDemuxerPlugin::ParseHEVCMetadataInfo(const AVStream& avStream, Meta& format)
{
    HevcParseFormat parse;
    parse.isHdrVivid = streamParser_->IsHdrVivid();
    parse.colorRange = streamParser_->GetColorRange();
    parse.colorPrimaries = streamParser_->GetColorPrimaries();
    parse.colorTransfer = streamParser_->GetColorTransfer();
    parse.colorMatrixCoeff = streamParser_->GetColorMatrixCoeff();
    parse.profile = streamParser_->GetProfileIdc();
    parse.level = streamParser_->GetLevelIdc();
    parse.chromaLocation = streamParser_->GetChromaLocation();
    parse.picWidInLumaSamples = streamParser_->GetPicWidInLumaSamples();
    parse.picHetInLumaSamples = streamParser_->GetPicHetInLumaSamples();

    FFmpegFormatHelper::ParseHevcInfo(*formatContext_, parse, format);
}

bool FFmpegDemuxerPlugin::TrackIsSelected(const uint32_t trackId)
{
    return std::any_of(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                       [trackId](uint32_t id) { return id == trackId; });
}

Status FFmpegDemuxerPlugin::SelectTrack(uint32_t trackId)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("Select track " PUBLIC_LOG_D32 ".", trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Select track failed due to AVFormatContext is nullptr.");
    if (trackId >= static_cast<uint32_t>(formatContext_.get()->nb_streams)) {
        MEDIA_LOG_E("Select track failed due to trackId is invalid, just have " PUBLIC_LOG_D32 " tracks in file.",
            formatContext_.get()->nb_streams);
        return Status::ERROR_INVALID_PARAMETER;
    }

    AVStream* avStream = formatContext_->streams[trackId];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER,
        "Select track failed due to avStream is nullptr.");
    if (!IsSupportedTrack(*avStream)) {
        MEDIA_LOG_E("Select track failed due to demuxer is unsupport this track type.");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (!TrackIsSelected(trackId)) {
        selectedTrackIds_.push_back(trackId);
        trackMtx_[trackId] = std::make_shared<std::mutex>();
        trackDfxInfoMap_[trackId] = {0, -1, -1};
        return cacheQueue_.AddTrackQueue(trackId);
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " is already in selected list.", trackId);
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::UnselectTrack(uint32_t trackId)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("Unselect track " PUBLIC_LOG_D32 ".", trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Can not call this func before set data source.");
    auto index = std::find_if(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                              [trackId](uint32_t selectedId) {return trackId == selectedId; });
    if (TrackIsSelected(trackId)) {
        selectedTrackIds_.erase(index);
        trackMtx_.erase(trackId);
        trackDfxInfoMap_.erase(trackId);
        return cacheQueue_.RemoveTrackQueue(trackId);
    } else {
        MEDIA_LOG_W("Unselect track failed due to track " PUBLIC_LOG_U32 " is not in selected list.", trackId);
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode, int64_t& realSeekTime)
{
    (void) trackId;
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("SeekTo");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Can not call this func before set data source.");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION,
        "Seek failed due to no track has been selected.");

    FALSE_RETURN_V_MSG_E(seekTime >= 0, Status::ERROR_INVALID_PARAMETER,
        "Seek failed due to seek time " PUBLIC_LOG_D64 " is not unsupported.", seekTime);
    FALSE_RETURN_V_MSG_E(g_seekModeToFFmpegSeekFlags.count(mode) != 0, Status::ERROR_INVALID_PARAMETER,
        "Seek failed due to seek mode " PUBLIC_LOG_D32 " is not unsupported.", static_cast<uint32_t>(mode));

    int trackIndex = static_cast<int>(selectedTrackIds_[0]);
    for (size_t i = 1; i < selectedTrackIds_.size(); i++) {
        int index = static_cast<int>(selectedTrackIds_[i]);
        if (formatContext_->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            trackIndex = index;
            break;
        }
    }
    MEDIA_LOG_D("Seek based on track " PUBLIC_LOG_D32 ".", trackIndex);
    auto avStream = formatContext_->streams[trackIndex];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER,
        "Seek failed due to avStream is nullptr.");
    int64_t ffTime = ConvertTimeToFFmpeg(seekTime * 1000 * 1000, avStream->time_base);
    if (!CheckStartTime(formatContext_.get(), avStream, ffTime, seekTime)) {
        MEDIA_LOG_E("Seek failed due to check get start time from track " PUBLIC_LOG_D32 " failed.", trackIndex);
        return Status::ERROR_INVALID_OPERATION;
    }
    realSeekTime = ConvertTimeFromFFmpeg(ffTime, avStream->time_base);
    int flag = ConvertFlagsToFFmpeg(avStream, ffTime, mode, seekTime);
    MEDIA_LOG_I("Seek:time [" PUBLIC_LOG_U64 "/" PUBLIC_LOG_U64 "/" PUBLIC_LOG_D64 "] flag ["
                PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "]",
                seekTime, ffTime, realSeekTime, static_cast<int32_t>(mode), flag);
    auto ret = av_seek_frame(formatContext_.get(), trackIndex, ffTime, flag);
    if (formatContext_->pb->error) {
        formatContext_->pb->error = 0;
    }
    FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_UNKNOWN,
        "Seek failed due to av_seek_frame failed, err: " PUBLIC_LOG_S ".", AVStrError(ret).c_str());
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        cacheQueue_.RemoveTrackQueue(selectedTrackIds_[i]);
        cacheQueue_.AddTrackQueue(selectedTrackIds_[i]);
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::Flush()
{
    Status ret = Status::OK;
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("Flush enter.");
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        ret = cacheQueue_.RemoveTrackQueue(selectedTrackIds_[i]);
        ret = cacheQueue_.AddTrackQueue(selectedTrackIds_[i]);
    }
    if (formatContext_) {
        avio_flush(formatContext_.get()->pb);
        avformat_flush(formatContext_.get());
    }
    return ret;
}

void FFmpegDemuxerPlugin::ResetEosStatus()
{
    MEDIA_LOG_I("ResetEosStatus enter.");
    formatContext_->pb->eof_reached = 0;
    formatContext_->pb->error = 0;
}

Status FFmpegDemuxerPlugin::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("ReadSample");
    MEDIA_LOG_D("Read Sample.");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Can not call this func before set data source.");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected.");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "track has not been selected");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_!=nullptr, Status::ERROR_INVALID_PARAMETER,
        "Read Sample failed due to input sample is nullptr");
    Status ret;
    if (NeedCombineFrame(trackId) && cacheQueue_.GetCacheSize(trackId) == 1) {
        ret = ReadPacketToCacheQueue(trackId);
    }
    while (!cacheQueue_.HasCache(trackId)) {
        ret = ReadPacketToCacheQueue(trackId);
        if (ret == Status::END_OF_STREAM) {
            MEDIA_LOG_D("read to end.");
        }
        FALSE_RETURN_V_MSG_E(ret != Status::ERROR_UNKNOWN, Status::ERROR_UNKNOWN,
            "read from ffmpeg faild.");
        FALSE_RETURN_V_MSG_E(ret != Status::ERROR_AGAIN, Status::ERROR_AGAIN,
            "read from ffmpeg faild, try again.");
        FALSE_RETURN_V_MSG_E(ret != Status::ERROR_NO_MEMORY, Status::ERROR_NO_MEMORY,
            "cache data size is greater than cache limit size.");
    }
    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    auto samplePacket = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_NULL_POINTER, "Read failed, samplePacket is nullptr");
    if (samplePacket->isEOS) {
        ret = SetEosSample(sample);
        if (ret == Status::OK) {
            MEDIA_LOG_I("Last Buffer track:" PUBLIC_LOG_D32 ", pts=" PUBLIC_LOG_D64 ", duration=" PUBLIC_LOG_D64
                ", pos=" PUBLIC_LOG_D64 "", trackId, trackDfxInfoMap_[trackId].lastPts,
                trackDfxInfoMap_[trackId].lastDurantion, trackDfxInfoMap_[trackId].lastPos);
            cacheQueue_.Pop(trackId);
        }
        return ret;
    }
    ret = ConvertAVPacketToSample(sample, samplePacket);
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
    MEDIA_LOG_D("Get size for track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_UNKNOWN, "Have not set data source.");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_UNKNOWN, "The track has not been selected");
    
    Status ret;
    if (NeedCombineFrame(trackId) && cacheQueue_.GetCacheSize(trackId) == 1) {
        ret = ReadPacketToCacheQueue(trackId);
    }
    while (!cacheQueue_.HasCache(trackId)) {
        ret = ReadPacketToCacheQueue(trackId);
        if (ret == Status::END_OF_STREAM) {
            MEDIA_LOG_D("read thread, read to end.");
        } else if (ret == Status::ERROR_UNKNOWN) {
            MEDIA_LOG_E("read from ffmpeg faild.");
            return Status::ERROR_UNKNOWN;
        } else if (ret == Status::ERROR_AGAIN) {
            MEDIA_LOG_E("read from ffmpeg faild, try again.");
            return Status::ERROR_AGAIN;
        } else if (ret == Status::ERROR_NO_MEMORY) {
            MEDIA_LOG_E("cache data size is greater than cache limit size. ret = " PUBLIC_LOG_D32, ret);
            return Status::ERROR_NO_MEMORY;
        }
    }
    std::shared_ptr<SamplePacket> samplePacket = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_UNKNOWN, "Cache sample is nullptr");
    if (samplePacket->isEOS) {
        MEDIA_LOG_I("Get size for track " PUBLIC_LOG_D32 " EOS.", trackId);
        return Status::END_OF_STREAM;
    }
    FALSE_RETURN_V_MSG_E(samplePacket->pkts.size() > 0, Status::ERROR_UNKNOWN, "Cache sample is empty");
    int totalSize = 0;
    for (auto pkt : samplePacket->pkts) {
        FALSE_RETURN_V_MSG_E(pkt != nullptr, Status::ERROR_UNKNOWN, "Packet in sample is nullptr");
        totalSize += pkt->size;
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
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "GetIndexByRelativePresentationTimeUs failed due to formatContext_ is nullptr.");

    FALSE_RETURN_V_MSG_E(trackIndex < formatContext_->nb_streams, Status::ERROR_INVALID_DATA,
        "GetIndexByRelativePresentationTimeUs failed due to trackIndex is out of range.");

    InitPTSandIndexConvert();

    auto avStream = formatContext_->streams[trackIndex];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER,
        "GetIndexByRelativePresentationTimeUs failed due to avStream is nullptr.");

    FALSE_RETURN_V_MSG_E(FFmpegFormatHelper::GetFileTypeByName(*formatContext_) == FileType::MP4,
        Status::ERROR_MISMATCHED_TYPE, "GetIndexByRelativePresentationTimeUs failed due to fileType is not MP4.");

    Status ret = GetPresentationTimeUsFromFfmpegMOV(GET_FIRST_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "GetPresentationTimeUsFromFfmpegMOV failed.");

    int64_t absolutePTS = static_cast<int64_t>(relativePresentationTimeUs) + absolutePTSIndexZero_;

    ret = GetPresentationTimeUsFromFfmpegMOV(RELATIVEPTS_TO_INDEX, trackIndex,
        absolutePTS, index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "GetPresentationTimeUsFromFfmpegMOV failed.");

    if (absolutePTS < relativePTSToIndexPTSMin_ || absolutePTS > relativePTSToIndexPTSMax_) {
        MEDIA_LOG_E("AbsolutePTS is out of range.");
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
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "GetRelativePresentationTimeUsByIndex failed due to formatContext_ is nullptr.");

    FALSE_RETURN_V_MSG_E(trackIndex < formatContext_->nb_streams, Status::ERROR_INVALID_DATA,
        "GetRelativePresentationTimeUsByIndex failed due to trackIndex is out of range.");

    InitPTSandIndexConvert();

    auto avStream = formatContext_->streams[trackIndex];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER,
        "GetRelativePresentationTimeUsByIndex failed due to avStream is nullptr.");

    FALSE_RETURN_V_MSG_E(FFmpegFormatHelper::GetFileTypeByName(*formatContext_) == FileType::MP4,
        Status::ERROR_MISMATCHED_TYPE, "GetRelativePresentationTimeUsByIndex failed due to fileType is not MP4.");

    Status ret = GetPresentationTimeUsFromFfmpegMOV(GET_FIRST_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "GetPresentationTimeUsFromFfmpegMOV failed.");

    GetPresentationTimeUsFromFfmpegMOV(INDEX_TO_RELATIVEPTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "GetPresentationTimeUsFromFfmpegMOV failed.");

    if (index + 1 > indexToRelativePTSFrameCount_) {
        MEDIA_LOG_E("Index is out of range.");
        return Status::ERROR_INVALID_DATA;
    }

    int64_t relativepts = indexToRelativePTSMaxHeap_.top() - absolutePTSIndexZero_;
    FALSE_RETURN_V_MSG_E(relativepts >= 0, Status::ERROR_INVALID_DATA,
        "GetRelativePresentationTimeUsByIndex failed due to the existence of calculation results less than 0.");
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

    int32_t sttsCurNum = static_cast<int32_t>(avStream->stts_data[sttsIndex].count);
    int32_t cttsCurNum = 0;

    cttsCurNum = static_cast<int32_t>(avStream->ctts_data[cttsIndex].count);
    while (sttsIndex < avStream->stts_count && cttsIndex < avStream->ctts_count &&
            cttsCurNum >= 0 && sttsCurNum >= 0) {
        if (cttsCurNum == 0) {
            cttsIndex++;
            cttsCurNum = cttsIndex < avStream->ctts_count ?
                         static_cast<int32_t>(avStream->ctts_data[cttsIndex].count) : 0;
        }
        cttsCurNum--;
        pts = (dts + static_cast<int64_t>(avStream->ctts_data[cttsIndex].duration)) *
                1000 * 1000 / static_cast<int64_t>(avStream->time_scale); // 1000 is used for converting pts to us
        PTSAndIndexConvertSwitchProcess(mode, pts, absolutePTS, index);
        sttsCurNum--;
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

    int32_t sttsCurNum = static_cast<int32_t>(avStream->stts_data[sttsIndex].count);

    while (sttsIndex < avStream->stts_count && sttsCurNum >= 0) {
        pts = dts * 1000 * 1000 / static_cast<int64_t>(avStream->time_scale); // 1000 is for converting pts to us
        PTSAndIndexConvertSwitchProcess(mode, pts, absolutePTS, index);
        sttsCurNum--;
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
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER,
        "GetPresentationTimeUsFromFfmpegMOV failed due to avStream is nullptr.");
    FALSE_RETURN_V_MSG_E(avStream->stts_data != nullptr && avStream->stts_count != 0,
        Status::ERROR_NULL_POINTER, "GetPresentationTimeUsFromFfmpegMOV failed due to avStream->stts_data is empty.");
    FALSE_RETURN_V_MSG_E(avStream->time_scale != 0, Status::ERROR_INVALID_DATA,
        "GetPresentationTimeUsFromFfmpegMOV failed due to avStream->time_scale is zero.");
    
    return avStream->ctts_data == nullptr ?
        PTSAndIndexConvertOnlySttsProcess(mode, avStream, absolutePTS, index) :
        PTSAndIndexConvertSttsAndCttsProcess(mode, avStream, absolutePTS, index);
}

void FFmpegDemuxerPlugin::PTSAndIndexConvertSwitchProcess(IndexAndPTSConvertMode mode,
    int64_t pts, int64_t absolutePTS, uint32_t index)
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
        default:
            MEDIA_LOG_E("wrong GetPresentationTimeUsFromFfmpegMOV mode");
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

namespace { // plugin set
int Sniff(const std::string& pluginName, std::shared_ptr<DataSource> dataSource)
{
    FALSE_RETURN_V_MSG_E(!pluginName.empty(), 0, "Sniff failed due to plugin name is empty.");
    FALSE_RETURN_V_MSG_E(dataSource != nullptr, 0, "Sniff failed due to dataSource invalid.");
    std::shared_ptr<AVInputFormat> plugin;
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        plugin = g_pluginInputFormat[pluginName];
    }
    FALSE_RETURN_V_MSG_E((plugin != nullptr && plugin->read_probe), 0,
        "Sniff failed due to get plugin for " PUBLIC_LOG_S " failed.", pluginName.c_str());
    size_t bufferSize = DEFAULT_SNIFF_SIZE;
    uint64_t fileSize = 0;
    if (dataSource->GetSize(fileSize) == Status::OK) {
        bufferSize = (bufferSize < fileSize) ? bufferSize : fileSize;
    }
    std::vector<uint8_t> buff(bufferSize + AVPROBE_PADDING_SIZE); // fix ffmpeg probe crash, refer to tools/probetest.c
    auto bufferInfo = std::make_shared<Buffer>();
    auto bufData = bufferInfo->WrapMemory(buff.data(), bufferSize, bufferSize);
    FALSE_RETURN_V_MSG_E(bufferInfo->GetMemory() != nullptr, 0, "Sniff failed due to alloc buffer failed.");
    Status ret;
    {
        std::string traceName = "Sniff_" + pluginName + "_Readat";
        MediaAVCodec::AVCodecTrace trace(traceName.c_str());
        ret = dataSource->ReadAt(0, bufferInfo, bufferSize);
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, 0, "Sniff failed due to read probe data failed.");
    int getData = static_cast<int>(bufferInfo->GetMemory()->GetSize());
    FALSE_RETURN_V_MSG_E(getData > 0, 0, "Not enough data for sniff " PUBLIC_LOG_S, pluginName.c_str());
    AVProbeData probeData{"", buff.data(), getData, ""};
    int confidence = plugin->read_probe(&probeData);
    if (StartWith(plugin->name, "mp3") && confidence > 0 && confidence <= MP3_PROBE_SCORE_LIMIT) {
        MEDIA_LOG_W("Sniff: probe score " PUBLIC_LOG_D32 " is too low, may misdetection, reset to 0", confidence);
        confidence = 0;
    }
    if (confidence > 0) {
        MEDIA_LOG_I("effective sniff: dataSize:" PUBLIC_LOG_D32 " " PUBLIC_LOG_S "[" PUBLIC_LOG_D32 "/100]",
            getData, plugin->name, confidence);
    }
    if (static_cast<uint32_t>(getData) < DEFAULT_SNIFF_SIZE) { // not enough data
        MEDIA_LOG_I("leak sniff: dataSize:" PUBLIC_LOG_D32 " " PUBLIC_LOG_S "[" PUBLIC_LOG_D32 "/100]",
            getData, plugin->name, confidence);
    }
    return confidence;
}

void ReplaceDelimiter(const std::string& delmiters, char newDelimiter, std::string& str)
{
    MEDIA_LOG_D("Reset string [" PUBLIC_LOG_S "].", str.c_str());
    for (auto it = str.begin(); it != str.end(); ++it) {
        if (delmiters.find(newDelimiter) != std::string::npos) {
            *it = newDelimiter;
        }
    }
    MEDIA_LOG_D("Reset to [" PUBLIC_LOG_S "].", str.c_str());
};

Status RegisterPlugins(const std::shared_ptr<Register>& reg)
{
    MEDIA_LOG_I("Register ffmpeg demuxer plugin.");
    FALSE_RETURN_V_MSG_E(reg != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Register plugin failed due to null pointer for reg.");
    std::lock_guard<std::mutex> lock(g_mtx);
    const AVInputFormat* plugin = nullptr;
    void* i = nullptr;
    while ((plugin = av_demuxer_iterate(&i))) {
        if (plugin == nullptr) {
            continue;
        }
        MEDIA_LOG_D("Check ffmpeg demuxer " PUBLIC_LOG_S "[" PUBLIC_LOG_S "].", plugin->name, plugin->long_name);
        if (plugin->long_name != nullptr &&
            !strncmp(plugin->long_name, "pcm ", STR_MAX_LEN)) {
            continue;
        }
        if (!IsInputFormatSupported(plugin->name)) {
            continue;
        }

        std::string pluginName = "avdemux_" + std::string(plugin->name);
        ReplaceDelimiter(".,|-<> ", '_', pluginName);

        DemuxerPluginDef regInfo;
        regInfo.name = pluginName;
        regInfo.description = "ffmpeg demuxer plugin";
        regInfo.rank = RANK_MAX;
        regInfo.AddExtensions(SplitString(plugin->extensions, ','));
        g_pluginInputFormat[pluginName] =
            std::shared_ptr<AVInputFormat>(const_cast<AVInputFormat*>(plugin), [](void*) {});
        auto func = [](const std::string& name) -> std::shared_ptr<DemuxerPlugin> {
            return std::make_shared<FFmpegDemuxerPlugin>(name);
        };
        regInfo.SetCreator(func);
        regInfo.SetSniffer(Sniff);
        auto ret = reg->AddPlugin(regInfo);
        if (ret != Status::OK) {
            MEDIA_LOG_E("RegisterPlugins failed due to add plugin failed, err=" PUBLIC_LOG_D32, static_cast<int>(ret));
        } else {
            MEDIA_LOG_D("Add plugin " PUBLIC_LOG_S ".", pluginName.c_str());
        }
    }
    FALSE_RETURN_V_MSG_E(!g_pluginInputFormat.empty(), Status::ERROR_UNKNOWN, "Can not load any format demuxer.");
    return Status::OK;
}
} // namespace
PLUGIN_DEFINITION(FFmpegDemuxer, LicenseType::LGPL, RegisterPlugins, [] {});
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
