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

#define HST_LOG_TAG "FfmpegDemuxerPlugin"
#include <unistd.h>
#include <algorithm>
#include <malloc.h>
#include <string>
#include <sstream>
#include <map>
#include "avcodec_trace.h"
#include "securec.h"
#include "ffmpeg_format_helper.h"
#include "ffmpeg_utils.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "common/log.h"
#include "meta/video_types.h"
#include "ffmpeg_demuxer_plugin.h"

#define AV_CODEC_TIME_BASE (static_cast<int64_t>(1))
#define AV_CODEC_NSECOND AV_CODEC_TIME_BASE
#define AV_CODEC_USECOND (static_cast<int64_t>(1000) * AV_CODEC_NSECOND)
#define AV_CODEC_MSECOND (static_cast<int64_t>(1000) * AV_CODEC_USECOND)
#define AV_CODEC_SECOND (static_cast<int64_t>(1000) * AV_CODEC_MSECOND)

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
const uint32_t MP3_PROBE_SIZE = DEFAULT_READ_SIZE * 2;
const int32_t MP3_PROBE_SCORE_LIMIT = 5;
const uint32_t STR_MAX_LEN = 4;
const uint32_t RANK_MAX = 100;
const uint32_t NAL_START_CODE_SIZE = 4;
namespace {
std::map<std::string, std::shared_ptr<AVInputFormat>> g_pluginInputFormat;

int Sniff(const std::string& pluginName, std::shared_ptr<DataSource> dataSource);

Status RegisterPlugins(const std::shared_ptr<Register>& reg);

bool IsInputFormatSupported(const char* name);

void ReplaceDelimiter(const std::string& delmiters, char newDelimiter, std::string& str);

inline int64_t AvTime2Us(int64_t hTime)
{
    return hTime / AV_CODEC_USECOND;
}

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
            MEDIA_LOG_D("Get duration from metadata.");
            duration = us;
        }
    }

    if (duration <= 0) {
        for (uint32_t i = 0; i < avFormatContext.nb_streams; ++i) {
            auto streamDuration = (ConvertTimeFromFFmpeg(avFormatContext.streams[i]->duration,
                avFormatContext.streams[i]->time_base)) / 1000; // us
            if (streamDuration > duration) {
                MEDIA_LOG_D("Get duration from stream " PUBLIC_LOG_D32, i);
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
            MEDIA_LOG_D("Get duration from metadata.");
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
    int64_t fileDuration = formatContext->duration;
    int64_t streamDuration = stream->duration;
    if (fileDuration == AV_NOPTS_VALUE || fileDuration <= 0) {
        fileDuration = GetFileDuration(*formatContext);
    }
    if (streamDuration == AV_NOPTS_VALUE || streamDuration <= 0) {
        streamDuration = GetStreamDuration(*stream);
    }
    MEDIA_LOG_D("file duration = " PUBLIC_LOG_D64 ", stream duration = " PUBLIC_LOG_D64 "",
        fileDuration, streamDuration);
    // when timestemp out of file duration, return error
    if (fileDuration > 0 && seekTime * num > fileDuration) {
        MEDIA_LOG_E("Seek to timestamp = " PUBLIC_LOG_D64 " failed, max = " PUBLIC_LOG_D64 "",
                        timeStamp, fileDuration);
        return false;
    }
    // when timestemp out of stream duration, seek to end of stream
    if (streamDuration > 0 && timeStamp > streamDuration) {
        MEDIA_LOG_I("Out of stream duration, will seek to end of stream ,timestamp = " PUBLIC_LOG_D64, timeStamp);
        timeStamp = streamDuration;
    }
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        MEDIA_LOG_I("Reset timeStamp by start time.");
        timeStamp += startTime;
    }
    return true;
}

int ConvertFlagsToFFmpeg(AVStream *avStream, int64_t ffTime, SeekMode mode)
{
    FALSE_RETURN_V_MSG_E(avStream != nullptr && avStream->codecpar != nullptr, -1, "stream is nulltr.");
    if (avStream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
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
    if (avStream.codecpar->codec_type != AVMEDIA_TYPE_AUDIO && avStream.codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
        avStream.codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
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

FFmpegDemuxerPlugin::FFmpegDemuxerPlugin(std::string name)
    : DemuxerPlugin(std::move(name)),
      seekable_(Seekable::SEEKABLE),
      ioContext_(),
      selectedTrackIds_(),
      cacheQueue_("cacheQueue"),
      streamParserInited_(false)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Create FFmpeg Demuxer Plugin.");
#ifndef _WIN32
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
#endif
    av_log_set_callback(FfmpegLogPrint);
    MEDIA_LOG_D("Create FFmpeg Demuxer Plugin successfully.");
}

FFmpegDemuxerPlugin::~FFmpegDemuxerPlugin()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Destroy FFmpeg Demuxer Plugin.");
#ifndef _WIN32
    (void)mallopt(M_FLUSH_THREAD_CACHE, 0);
#endif
    pluginImpl_ = nullptr;
    formatContext_ = nullptr;
    avbsfContext_ = nullptr;
    streamParser_ = nullptr;
    selectedTrackIds_.clear();
    if (firstFrame_ != nullptr) {
        av_packet_free(&firstFrame_);
        av_free(firstFrame_);
        firstFrame_ = nullptr;
    }
    MEDIA_LOG_D("Destroy FFmpeg Demuxer Plugin successfully.");
}

Status FFmpegDemuxerPlugin::Reset()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Reset FFmpeg Demuxer Plugin.");

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
    return Status::OK;
}

void FFmpegDemuxerPlugin::InitBitStreamContext(const AVStream& avStream)
{
    FALSE_RETURN_MSG(avStream.codecpar != nullptr, "Init BitStreamContext failed due to codec par is nullptr.");
    AVCodecID codecID = avStream.codecpar->codec_id;
    MEDIA_LOG_D("Init BitStreamContext for track " PUBLIC_LOG_D32 "[" PUBLIC_LOG_S "].",
        avStream.index, avcodec_get_name(codecID));
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
        "Init BitStreamContext failed due to init avbsfContext_ failed, name:" PUBLIC_LOG_S ".",
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
    int cencInfoSize = 0;
    uint8_t *cencInfo = av_packet_get_side_data(samplePacket->pkts[0], AV_PKT_DATA_ENCRYPTION_INFO,
        reinterpret_cast<size_t *>(&cencInfoSize));
    streamParser_->ConvertPacketToAnnexb(&(pkt.data), pkt.size, cencInfo,
        static_cast<size_t>(cencInfoSize), false);
    if (NeedCombineFrame(samplePacket->pkts[0]->stream_index) &&
        streamParser_->IsSyncFrame(pkt.data, pkt.size)) {
        pkt.flags = static_cast<int32_t>(static_cast<uint32_t>(pkt.flags) | static_cast<uint32_t>(AV_PKT_FLAG_KEY));
    }
}

Status FFmpegDemuxerPlugin::WriteBuffer(
    std::shared_ptr<AVBuffer> outBuffer, int64_t pts, uint32_t flag, const uint8_t *writeData, int32_t writeSize)
{
    outBuffer->pts_ = pts;
    outBuffer->flag_ = flag;
    FALSE_RETURN_V_MSG_E(outBuffer!=nullptr, Status::ERROR_NULL_POINTER,
        "Write data failed due to Buffer is nullptr.");
    if (writeData != nullptr && writeSize > 0) {
        FALSE_RETURN_V_MSG_E(outBuffer->memory_!=nullptr, Status::ERROR_NULL_POINTER,
            "Write data failed due to AVBuffer memory is nullptr.");
        int32_t ret = outBuffer->memory_->Write(writeData, writeSize, 0);
        FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_INVALID_OPERATION,
            "Write data failed due to AVBuffer memory write failed.");
    }
    MEDIA_LOG_D("CurrentBuffer: pts=" PUBLIC_LOG_D64 ", flag=." PUBLIC_LOG_U32 ".", outBuffer->pts_, outBuffer->flag_);
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

    int cencInfoSize = 0;
    MetaDrmCencInfo *cencInfo = (MetaDrmCencInfo *)av_packet_get_side_data(samplePacket->pkts[0],
        AV_PKT_DATA_ENCRYPTION_INFO, reinterpret_cast<size_t *>(&cencInfoSize));
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

void FFmpegDemuxerPlugin::ConvertAvccToAnnexb(std::shared_ptr<AVBuffer> sample, AVPacket* srcAVPacket,
    std::shared_ptr<SamplePacket> dstSamplePacket)
{
    auto codecId = formatContext_->streams[srcAVPacket->stream_index]->codecpar->codec_id;
    if (codecId == AV_CODEC_ID_HEVC && streamParser_ != nullptr && streamParserInited_) {
        ConvertHevcToAnnexb(*srcAVPacket, dstSamplePacket);
        SetDropTag(*srcAVPacket, sample, AV_CODEC_ID_HEVC);
    } else if (codecId == AV_CODEC_ID_H264 && avbsfContext_ != nullptr) {
        ConvertAvcToAnnexb(*srcAVPacket);
        SetDropTag(*srcAVPacket, sample, AV_CODEC_ID_H264);
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
    int64_t pts = 0;
    AVStream *avStream = formatContext_->streams[samplePacket->pkts[0]->stream_index];
    if (avStream->start_time == AV_NOPTS_VALUE) {
        avStream->start_time = 0;
    }
    if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        int64_t inputPts = ConvertPts(samplePacket->pkts[0]->pts, avStream->start_time);
        pts = AvTime2Us(ConvertTimeFromFFmpeg(inputPts, avStream->time_base));
    } else {
        pts = AvTime2Us(ConvertTimeFromFFmpeg(samplePacket->pkts[0]->pts, avStream->time_base));
        if (avStream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            sample->duration_ = AvTime2Us(ConvertTimeFromFFmpeg(samplePacket->pkts[0]->duration, avStream->time_base));
        }
    }

    AVPacket *tempPkt = CombinePackets(samplePacket);
    FALSE_RETURN_V_MSG_E(tempPkt != nullptr, Status::ERROR_INVALID_OPERATION, "tempPkt is empty.");
    ConvertAvccToAnnexb(sample, tempPkt, samplePacket);

    int32_t remainSize = tempPkt->size - static_cast<int32_t>(samplePacket->offset);
    int32_t copySize = remainSize < sample->memory_->GetCapacity() ? remainSize : sample->memory_->GetCapacity();
    MEDIA_LOG_D("packet size=" PUBLIC_LOG_D32 ", remain size=" PUBLIC_LOG_D32, tempPkt->size, remainSize);
    MEDIA_LOG_D("copySize=" PUBLIC_LOG_D32 ", copyOffset" PUBLIC_LOG_D32, copySize, samplePacket->offset);
    uint32_t flag = ConvertFlagsFromFFmpeg(*tempPkt, (copySize != tempPkt->size));
    SetDrmCencInfo(sample, samplePacket);
    Status ret = WriteBuffer(sample, pts, flag, tempPkt->data + samplePacket->offset, copySize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Convert packet info failed due to write buffer failed.");
    if (tempPkt != nullptr && tempPkt->size != samplePacket->pkts[0]->size) {
        av_packet_free(&tempPkt);
        av_free(tempPkt);
        tempPkt = nullptr;
    }
    if (copySize < remainSize) {
        samplePacket->offset += static_cast<uint32_t>(copySize);
        MEDIA_LOG_D("Buffer is not enough, next buffer to save remain data.");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    return Status::OK;
}

void FFmpegDemuxerPlugin::PushEOSToAllCache()
{
    MEDIA_LOG_W("Push EOS frame.");
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        auto streamIndex = selectedTrackIds_[i];
        MEDIA_LOG_I("Cache queue " PUBLIC_LOG_D32 ".", streamIndex);
        std::shared_ptr<SamplePacket> eosSample = std::make_shared<SamplePacket>();
        eosSample->isEOS = true;
        cacheQueue_.Push(streamIndex, eosSample);
    }
}

Status FFmpegDemuxerPlugin::ReadPacketToCacheQueue(const uint32_t readId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    AVPacket *pkt = nullptr;
    bool continueRead = true;
    while (continueRead) {
        if (pkt == nullptr) {
            pkt = av_packet_alloc();
            FALSE_RETURN_V_MSG_E(pkt != nullptr, Status::ERROR_NULL_POINTER, "av_packet_alloc failed.");
        }
        int ffmpegRet = av_read_frame(formatContext_.get(), pkt);
        if (ffmpegRet == AVERROR_EOF) { // eos
            av_packet_free(&pkt);
            PushEOSToAllCache();
            return Status::END_OF_STREAM;
        }
        if (ffmpegRet < 0) { // fail
            av_packet_free(&pkt);
            MEDIA_LOG_E("Read frame failed due to av_read_frame failed:" PUBLIC_LOG_S ", timeout: " PUBLIC_LOG_D32,
                AVStrError(ffmpegRet).c_str(), int(ioContext_.timeout));
            if (ffmpegRet == AVERROR(EAGAIN) || ioContext_.timeout) {
                formatContext_->pb->eof_reached = 0;
                formatContext_->pb->error = 0;
                ioContext_.timeout = false;
                return Status::ERROR_AGAIN;
            }
            return Status::ERROR_UNKNOWN;
        }
        auto trackId = pkt->stream_index;
        if (!IsInSelectedTrack(trackId)) { // not in
            av_packet_unref(pkt);
            continue;
        }
        if (!NeedCombineFrame(readId) || (cacheQueue_.HasCache(trackId) && GetNextFrame(pkt->data, pkt->size))) {
            continueRead = false;
        }
        AddPacketToCacheQueue(pkt);
        pkt = nullptr;
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ReadEosSample(std::shared_ptr<AVBuffer> sample)
{
    MEDIA_LOG_D("Set EOS buffer.");
    Status ret = WriteBuffer(sample, 0, (uint32_t)(AVBufferFlag::EOS), nullptr, 0);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Push EOS buffer failed due to write buffer failed.");
    MEDIA_LOG_D("Set EOS buffer finish.");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::Start()
{
    MEDIA_LOG_I("Start FFmpeg Demuxer Plugin.");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::Stop()
{
    MEDIA_LOG_I("Stop FFmpeg Demuxer Plugin.");
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

// Write packet data into the buffer provided by ffmpeg
int FFmpegDemuxerPlugin::AVReadPacket(void* opaque, uint8_t* buf, int bufSize)
{
    int ret = -1;
    auto ioContext = static_cast<IOContext*>(opaque);
    FALSE_RETURN_V_MSG_E(ioContext != nullptr, ret, "AVReadPacket failed due to IOContext error.");
    if (ioContext && ioContext->dataSource) {
        auto buffer = std::make_shared<Buffer>();
        auto bufData = buffer->WrapMemory(buf, bufSize, 0);
        FALSE_RETURN_V_MSG_E(ioContext->dataSource != nullptr, ret, "AVReadPacket failed due to dataSource error.");
        MEDIA_LOG_D("Offset: " PUBLIC_LOG_D64 ", totalSize: " PUBLIC_LOG_U64, ioContext->offset, ioContext->fileSize);
        if (ioContext->fileSize > 0) {
            FALSE_RETURN_V_MSG_E(static_cast<uint64_t>(ioContext->offset) <= ioContext->fileSize, ret,
                "Offset out of file size.");
            if (static_cast<size_t>(ioContext->offset + bufSize) > ioContext->fileSize) {
                bufSize = static_cast<int64_t>(ioContext->fileSize) - ioContext->offset;
            }
        }
        auto result = ioContext->dataSource->ReadAt(ioContext->offset, buffer, static_cast<size_t>(bufSize));
        MEDIA_LOG_D("Want data size " PUBLIC_LOG_D32 ", Get data size" PUBLIC_LOG_D32 ", offset: " PUBLIC_LOG_D64,
            bufSize, static_cast<int>(buffer->GetMemory()->GetSize()), ioContext->offset);
        if (result == Status::OK) {
            ioContext->offset += buffer->GetMemory()->GetSize();
            ret = buffer->GetMemory()->GetSize();
        } else if (result == Status::ERROR_AGAIN) {
            MEDIA_LOG_I("Read data not enough, read again.");
            ioContext->timeout = true;
            ioContext->offset += buffer->GetMemory()->GetSize();
            ret = buffer->GetMemory()->GetSize();
            if (ret == 0) {
                return AVERROR(EAGAIN);
            }
        } else if (result == Status::END_OF_STREAM) {
            MEDIA_LOG_I("File is end.");
            ioContext->eos = true;
            ret = AVERROR_EOF;
        } else {
            if (result == Status::ERROR_WRONG_STATE) {
                ioContext->timeout = true;
            }
            MEDIA_LOG_I("AVReadPacket failed, result=" PUBLIC_LOG_D32 ".", static_cast<int>(result));
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

AVIOContext* FFmpegDemuxerPlugin::AllocAVIOContext(int flags)
{
    auto buffer = static_cast<unsigned char*>(av_malloc(DEFAULT_READ_SIZE));
    FALSE_RETURN_V_MSG_E(buffer != nullptr, nullptr, "Alloc AVIOContext failed due to av_malloc failed.");

    AVIOContext* avioContext = avio_alloc_context(
        buffer, DEFAULT_READ_SIZE, flags & AVIO_FLAG_WRITE, static_cast<void*>(&ioContext_),
        AVReadPacket, AVWritePacket, AVSeek);
    if (avioContext == nullptr) {
        MEDIA_LOG_E("Alloc AVIOContext failed due to avio_alloc_context failed.");
        av_free(buffer);
        return nullptr;
    }

    MEDIA_LOG_D("Seekable_ is " PUBLIC_LOG_D32 ".", static_cast<int32_t>(seekable_));
    avioContext->seekable = (seekable_ == Seekable::SEEKABLE) ? AVIO_SEEKABLE_NORMAL : 0;
    if (!(static_cast<uint32_t>(flags) & static_cast<uint32_t>(AVIO_FLAG_WRITE))) {
        avioContext->buf_ptr = avioContext->buf_end;
        avioContext->write_flag = 0;
    }
    return avioContext;
}

void FFmpegDemuxerPlugin::InitAVFormatContext()
{
    AVFormatContext* formatContext = avformat_alloc_context();
    FALSE_RETURN_MSG(formatContext != nullptr,
        "Init AVFormatContext failed due to avformat_alloc_context failed.");

    formatContext->pb = AllocAVIOContext(AVIO_FLAG_READ);
    FALSE_RETURN_MSG(formatContext->pb != nullptr,
        "Init AVFormatContext failed due to init AVIOContext failed.");
    formatContext->flags = static_cast<uint32_t>(formatContext->flags) | static_cast<uint32_t>(AVFMT_FLAG_CUSTOM_IO);
    formatContext->flags = static_cast<uint32_t>(formatContext->flags) | static_cast<uint32_t>(AVFMT_FLAG_FAST_SEEK);
    int ret = avformat_open_input(&formatContext, nullptr, pluginImpl_.get(), nullptr);
    FALSE_RETURN_MSG((ret == 0),
        "Init AVFormatContext failed due to avformat_open_input failed by " PUBLIC_LOG_S ", err:" PUBLIC_LOG_S ".",
        pluginImpl_->name, AVStrError(ret).c_str());

    ret = avformat_find_stream_info(formatContext, NULL);
    FALSE_RETURN_MSG((ret >= 0),
        "Init AVFormatContext failed due to avformat_find_stream_info failed by " PUBLIC_LOG_S
        ", err:" PUBLIC_LOG_S ".", pluginImpl_->name, AVStrError(ret).c_str());

    formatContext_ = std::shared_ptr<AVFormatContext>(formatContext, [](AVFormatContext* ptr) {
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
}

Status FFmpegDemuxerPlugin::SetDataSource(const std::shared_ptr<DataSource>& source)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Set data source for demuxer.");
    FALSE_RETURN_V_MSG_E(formatContext_ == nullptr, Status::ERROR_WRONG_STATE,
        "DataSource has been inited, need reset first.");
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set datasource failed due to source is nullptr.");

    ioContext_.dataSource = source;
    ioContext_.offset = 0;
    ioContext_.eos = false;
    if (source->GetSeekable() == Plugins::Seekable::SEEKABLE) {
        ioContext_.dataSource->GetSize(ioContext_.fileSize);
    } else {
        ioContext_.fileSize = -1;
    }
    seekable_ = source->GetSeekable();
    MEDIA_LOG_D("FFmpegDemuxerPlugin SetDataSource, fileSize: " PUBLIC_LOG_U64 ", seekable_: " PUBLIC_LOG_D32,
        ioContext_.fileSize, seekable_);
    pluginImpl_ = g_pluginInputFormat[pluginName_];
    FALSE_RETURN_V_MSG_E(pluginImpl_ != nullptr, Status::ERROR_UNSUPPORTED_FORMAT,
        "Set datasource failed due to can not find inputformat for format.");

    InitAVFormatContext();
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_UNKNOWN,
        "Set datasource failed due to can not init formatContext for source.");

    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        if (g_bitstreamFilterMap.count(formatContext_->streams[trackIndex]->codecpar->codec_id) != 0) {
            InitBitStreamContext(*(formatContext_->streams[trackIndex]));
            if (avbsfContext_ == nullptr) {
                MEDIA_LOG_W("init bitStreamContext failed for format " PUBLIC_LOG_S ", stream will not be converted",
                    avcodec_get_name(formatContext_->streams[trackIndex]->codecpar->codec_id));
            }
            break;
        }
        if (HaveValidParser(formatContext_->streams[trackIndex]->codecpar->codec_id)) {
            streamParser_ = StreamParserManager::Create(g_streamParserMap.at(
                formatContext_->streams[trackIndex]->codecpar->codec_id));
            if (streamParser_ == nullptr) {
                MEDIA_LOG_W("Init hevc parser failed, frame will not be converted to annexb");
            }
            break;
        }
    }

    MEDIA_LOG_I("Set data source for demuxer successfully.");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::GetMediaInfo(MediaInfo& mediaInfo)
{
    MediaAVCodec::AVCodecTrace trace("FFmpegDemuxerPlugin::GetMediaInfo");
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Get media info by FFmpeg Demuxer Plugin.");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Get media info failed due to formatContext_ is nullptr.");
    if (streamParser_ != nullptr && !streamParserInited_) {
        for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
            auto avStream = formatContext_->streams[trackIndex];
            if (HaveValidParser(avStream->codecpar->codec_id)) {
                GetVideoFirstKeyFrame(trackIndex);
                FALSE_RETURN_V_MSG_E(firstFrame_ != nullptr && firstFrame_->data != nullptr,
                    Status::ERROR_WRONG_STATE, "Get first frame failed. Get sei info may failed.");
                streamParser_->ConvertExtraDataToAnnexb(
                    avStream->codecpar->extradata, avStream->codecpar->extradata_size);
                streamParserInited_ = true;
                break;
            }
        }
    }
    FFmpegFormatHelper::ParseMediaInfo(*formatContext_, mediaInfo.general);
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        Meta meta;
        auto avStream = formatContext_->streams[trackIndex];
        if (avStream == nullptr) {
            MEDIA_LOG_W("Get track " PUBLIC_LOG_D32 " info failed due to track is nullptr.", trackIndex);
            mediaInfo.tracks.push_back(meta);
            continue;
        }
        FFmpegFormatHelper::ParseTrackInfo(*avStream, meta);
        if (avStream->codecpar->codec_id == AV_CODEC_ID_HEVC) {
            if (streamParser_ != nullptr && streamParserInited_ && firstFrame_ != nullptr) {
                streamParser_->ConvertPacketToAnnexb(&(firstFrame_->data), firstFrame_->size, nullptr, 0, false);
                streamParser_->ParseAnnexbExtraData(firstFrame_->data, firstFrame_->size);
                // Parser only sends xps info when first call ConvertPacketToAnnexb
                // readSample will call ConvertPacketToAnnexb again, so rest here
                streamParser_->ResetXPSSendStatus();
                ParseHEVCMetadataInfo(*avStream, meta);
            } else {
                MEDIA_LOG_W("streamParser_ or firstFrame_ is nullptr, parser hevc fail");
            }
        }
        if (avStream->codecpar->codec_id == AV_CODEC_ID_HEVC ||
            avStream->codecpar->codec_id == AV_CODEC_ID_H264) {
            ConvertCsdToAnnexb(*avStream, meta);
        }
        mediaInfo.tracks.push_back(meta);
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::GetUserMeta(std::shared_ptr<Meta> meta)
{
    MediaAVCodec::AVCodecTrace trace("FFmpegDemuxerPlugin::GetUserMeta");
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Get user data by FFmpeg Demuxer Plugin.");
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
    int32_t infoCount = drmInfoSize / sizeof(MetaDrmInfo);
    for (int32_t index = 0; index < infoCount; index++) {
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
            MEDIA_LOG_W("GetDrmInfo Get track " PUBLIC_LOG_D32 " info failed due to track is nullptr.", trackIndex);
            continue;
        }
        MEDIA_LOG_D("GetDrmInfo by stream side data");
        int drmInfoSize = 0;
        MetaDrmInfo *tmpDrmInfo = (MetaDrmInfo *)av_stream_get_side_data(avStream,
            AV_PKT_DATA_ENCRYPTION_INIT_INFO, reinterpret_cast<size_t *>(&drmInfoSize));
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

void FFmpegDemuxerPlugin::AddPacketToCacheQueue(AVPacket *pkt)
{
    auto trackId = pkt->stream_index;
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
        }
    }
}

void FFmpegDemuxerPlugin::GetVideoFirstKeyFrame(uint32_t trackIndex)
{
    FALSE_RETURN_MSG(formatContext_ != nullptr, "formatContext_ is null");
    MEDIA_LOG_I("GetVideoFirstKeyFrame enter\n");
    AVPacket *pkt = nullptr;
    while (1) {
        if (pkt == nullptr) {
            pkt = av_packet_alloc();
            FALSE_RETURN_MSG(pkt != nullptr, "av_packet_alloc fail");
        }

        if (av_read_frame(formatContext_.get(), pkt) < 0) {
            av_packet_unref(pkt);
            break;
        }

        cacheQueue_.AddTrackQueue(pkt->stream_index);
        AddPacketToCacheQueue(pkt);

        if (static_cast<uint32_t>(pkt->stream_index) == trackIndex) {
            firstFrame_ = av_packet_alloc();
            FALSE_RETURN_MSG(firstFrame_ != nullptr, "av_packet_alloc fail");
            int ret = av_new_packet(firstFrame_, pkt->size);
            FALSE_RETURN_MSG(ret >= 0, "av_new_packet fail");
            av_packet_copy_props(firstFrame_, pkt);
            memcpy_s(firstFrame_->data, pkt->size, pkt->data, pkt->size);
            break;
        }
        pkt = nullptr;
    }
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

void FFmpegDemuxerPlugin::ShowSelectedTracks()
{
    std::string selectedTracksString;
    for (auto index : selectedTrackIds_) {
        selectedTracksString += std::to_string(index) + " | ";
    }
    MEDIA_LOG_D("Has " PUBLIC_LOG_D32 " tracks in file, selected " PUBLIC_LOG_ZU " tracks",
        formatContext_.get()->nb_streams, selectedTrackIds_.size());
    if (selectedTrackIds_.size() > 0) {
        MEDIA_LOG_I("Selected track ids:" PUBLIC_LOG_S, selectedTracksString.c_str());
    }
}

bool FFmpegDemuxerPlugin::IsInSelectedTrack(const uint32_t trackId)
{
    return std::any_of(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                       [trackId](uint32_t id) { return id == trackId; });
}

Status FFmpegDemuxerPlugin::SelectTrack(uint32_t trackId)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    ShowSelectedTracks();
    MEDIA_LOG_D("Select track " PUBLIC_LOG_D32 ".", trackId);
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

    if (!IsInSelectedTrack(trackId)) {
        selectedTrackIds_.push_back(trackId);
        trackMtx_[trackId] = std::make_shared<std::mutex>();
        return cacheQueue_.AddTrackQueue(trackId);
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " is already in selected list.", trackId);
    }
    MEDIA_LOG_D("Select track successfully.");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::UnselectTrack(uint32_t trackId)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    ShowSelectedTracks();
    MEDIA_LOG_D("Unselect track " PUBLIC_LOG_D32 ".", trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Can not call this func before set data source.");
    auto index = std::find_if(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                              [trackId](uint32_t selectedId) {return trackId == selectedId; });
    if (IsInSelectedTrack(trackId)) {
        selectedTrackIds_.erase(index);
        trackMtx_.erase(trackId);
        return cacheQueue_.RemoveTrackQueue(trackId);
    } else {
        MEDIA_LOG_W("Unselect track failed due to track " PUBLIC_LOG_U32 " is not in selected list.", trackId);
    }
    MEDIA_LOG_D("Unselect track successfully.");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode, int64_t& realSeekTime)
{
    (void) trackId;
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Seek to " PUBLIC_LOG_D64 ", mode=" PUBLIC_LOG_D32 ".", seekTime, static_cast<int32_t>(mode));
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
    MEDIA_LOG_D("SeekTo " PUBLIC_LOG_U64 " / " PUBLIC_LOG_D64 ".", ffTime, realSeekTime);
    int flag = ConvertFlagsToFFmpeg(avStream, ffTime, mode);
    MEDIA_LOG_D("Convert flag [" PUBLIC_LOG_D32 "]->[" PUBLIC_LOG_D32 "], by track " PUBLIC_LOG_D32 "",
        static_cast<int32_t>(mode), flag, avStream->index);
    auto ret = av_seek_frame(formatContext_.get(), trackIndex, ffTime, flag);
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
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("Flush enter.");
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        cacheQueue_.RemoveTrackQueue(selectedTrackIds_[i]);
        cacheQueue_.AddTrackQueue(selectedTrackIds_[i]);
    }
    if (formatContext_) {
        avio_flush(formatContext_.get()->pb);
        avformat_flush(formatContext_.get());
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Read Sample.");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Can not call this func before set data source.");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION,
        "Read Sample failed due to no track has been selected.");
    FALSE_RETURN_V_MSG_E(IsInSelectedTrack(trackId), Status::ERROR_INVALID_PARAMETER,
        "Read Sample failed due to track has not been selected");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_!=nullptr, Status::ERROR_INVALID_PARAMETER,
        "Read Sample failed due to input sample is nullptr");
    Status ret;
    if (NeedCombineFrame(trackId)) {
        ret = ReadPacketToCacheQueue(trackId);
    }
    while (!cacheQueue_.HasCache(trackId)) {
        ret = ReadPacketToCacheQueue(trackId);
        if (ret == Status::END_OF_STREAM) {
            MEDIA_LOG_I("read to end.");
        } else if (ret == Status::ERROR_UNKNOWN) {
            MEDIA_LOG_E("read from ffmpeg faild.");
            return Status::ERROR_UNKNOWN;
        } else if (ret == Status::ERROR_AGAIN) {
            MEDIA_LOG_E("read from ffmpeg faild, retry again.");
            return Status::ERROR_AGAIN;
        }
    }
    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    std::shared_ptr<SamplePacket> samplePacket = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_NULL_POINTER, "Read failed, samplePacket is nullptr");
    if (samplePacket->isEOS) {
        MEDIA_LOG_W("File is end, push EOS buffer to user queue.");
        ret = ReadEosSample(sample);
        if (ret == Status::OK) {
            cacheQueue_.Pop(trackId);
        }
        MEDIA_LOG_I("Copy ret=" PUBLIC_LOG_D32 "", (uint32_t)(ret));
        return ret;
    }
    ret = ConvertAVPacketToSample(sample, samplePacket);
    if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
        return Status::OK;
    }
    if (ret == Status::OK) {
        MEDIA_LOG_D("All partial sample has seend copied");
        cacheQueue_.Pop(trackId);
    }
    return ret;
}

Status FFmpegDemuxerPlugin::GetNextSampleSize(uint32_t trackId, int32_t& size)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("Get size for track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_UNKNOWN, "Have not set data source.");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_UNKNOWN, "No track has been selected.");

    FALSE_RETURN_V_MSG_E(IsInSelectedTrack(trackId), Status::ERROR_UNKNOWN, "The track has not been selected");
    
    Status ret;
    if (NeedCombineFrame(trackId)) {
        ret = ReadPacketToCacheQueue(trackId);
    }
    while (!cacheQueue_.HasCache(trackId)) {
        ret = ReadPacketToCacheQueue(trackId);
        if (ret == Status::END_OF_STREAM) {
            MEDIA_LOG_I("read thread, read to end.");
        } else if (ret == Status::ERROR_UNKNOWN) {
            MEDIA_LOG_E("read from ffmpeg faild.");
            return Status::ERROR_UNKNOWN;
        } else if (ret == Status::ERROR_AGAIN) {
            MEDIA_LOG_E("read from ffmpeg faild, retry again.");
            return Status::ERROR_AGAIN;
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

void FFmpegDemuxerPlugin::SetDropTag(const AVPacket& pkt, std::shared_ptr<AVBuffer> sample, AVCodecID codecId)
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

int FFmpegDemuxerPlugin::FindNaluSpliter(int size, const uint8_t* data)
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

bool FFmpegDemuxerPlugin::CanDropAvcPkt(const AVPacket& pkt)
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

bool FFmpegDemuxerPlugin::CanDropHevcPkt(const AVPacket& pkt)
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

namespace { // plugin set
int Sniff(const std::string& pluginName, std::shared_ptr<DataSource> dataSource)
{
    MEDIA_LOG_I("Sniff: plugin name " PUBLIC_LOG_S ".", pluginName.c_str());

    FALSE_RETURN_V_MSG_E(!pluginName.empty(), 0, "Sniff failed due to plugin name is empty.");
    FALSE_RETURN_V_MSG_E(dataSource != nullptr, 0, "Sniff failed due to dataSource invalid.");

    auto plugin = g_pluginInputFormat[pluginName];
    FALSE_RETURN_V_MSG_E((plugin != nullptr && plugin->read_probe), 0,
        "Sniff failed due to get plugin for " PUBLIC_LOG_S " failed.", pluginName.c_str());

    size_t bufferSize = DEFAULT_READ_SIZE;
    if (StartWith(plugin->name, "mp3")) {
        bufferSize = MP3_PROBE_SIZE; // mp3 needs more data to probe, refer to ffmpeg
        MEDIA_LOG_I("Sniff: expend probe data size to " PUBLIC_LOG_ZU " for mp3", bufferSize);
    }
    uint64_t fileSize = 0;
    if (dataSource->GetSize(fileSize) == Status::OK) {
        bufferSize = (bufferSize < fileSize) ? bufferSize : fileSize;
        if (bufferSize == fileSize) {
            MEDIA_LOG_I("Sniff: file data is not enough, reset probe size to file size");
        }
    }
    // fix ffmpeg probe crash,refer to ffmpeg/tools/probetest.c
    std::vector<uint8_t> buff(bufferSize + AVPROBE_PADDING_SIZE);
    auto bufferInfo = std::make_shared<Buffer>();
    auto bufData = bufferInfo->WrapMemory(buff.data(), bufferSize, bufferSize);
    FALSE_RETURN_V_MSG_E(bufData != nullptr, 0, "Sniff failed due to alloc buffer failed.");
    MEDIA_LOG_D("Prepare buffer for probe, input param bufferSize=" PUBLIC_LOG_ZU
        ", real buffer size=" PUBLIC_LOG_ZU ".", bufferSize + AVPROBE_PADDING_SIZE, bufferSize);

    Status ret = dataSource->ReadAt(0, bufferInfo, bufferSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, 0, "Sniff failed due to read probe data failed.");
    FALSE_RETURN_V_MSG_E(bufData!= nullptr && buff.data() != nullptr, 0, "Sniff failed due to probe data invalid.");

    AVProbeData probeData{"", buff.data(), static_cast<int>(bufferInfo->GetMemory()->GetSize()), ""};
    int confidence = plugin->read_probe(&probeData);
    if (StartWith(plugin->name, "mp3") && confidence > 0 && confidence <= MP3_PROBE_SCORE_LIMIT) {
        MEDIA_LOG_W("Sniff: probe score " PUBLIC_LOG_D32 " is too low, may misdetection, reset to 0", confidence);
        confidence = 0;
    }
    MEDIA_LOG_I("Sniff: plugin name " PUBLIC_LOG_S ", probability " PUBLIC_LOG_D32 "/100.",
        plugin->name, confidence);

    return confidence;
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

    const AVInputFormat* plugin = nullptr;
    void* i = nullptr;
    while ((plugin = av_demuxer_iterate(&i))) {
        if (plugin == nullptr) {
            continue;
        }
        MEDIA_LOG_D("Check ffmpeg demuxer " PUBLIC_LOG_S "[" PUBLIC_LOG_S "].", plugin->name, plugin->long_name);
        if (plugin->long_name != nullptr) {
            if (!strncmp(plugin->long_name, "pcm ", STR_MAX_LEN)) {
                continue;
            }
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
PLUGIN_DEFINITION(FFmpegDemuxer, LicenseType::LGPL, RegisterPlugins, [] { g_pluginInputFormat.clear(); });
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
