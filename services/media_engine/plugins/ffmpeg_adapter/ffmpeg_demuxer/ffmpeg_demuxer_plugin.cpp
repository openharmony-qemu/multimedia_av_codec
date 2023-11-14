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

#include <unistd.h>
#include <algorithm>
#include <malloc.h>
#include <string>
#include <sstream>
#include "ffmpeg_demuxer_plugin.h"
#include "ffmpeg_format_helper.h"
#include "ffmpeg_utils.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "common/log.h"

#define FFMPEG_LOG_DEBUG_ENABLE 1
#define DEFAULT_READ_SIZE 4096
#define STR_MAX_LEN 4
#define RANK_MAX 100
#define CACHE_BLOCK_LIMIT 10 // 10s

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
namespace Plugin {
namespace Ffmpeg {
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
    { SeekMode::SEEK_NEXT_SYNC, AVSEEK_FLAG_BACKWARD },
    { SeekMode::SEEK_PREVIOUS_SYNC, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD },
    { SeekMode::SEEK_CLOSEST_SYNC, AVSEEK_FLAG_FRAME },
};

static const std::map<AVCodecID, std::string> g_bitstreamFilterMap = {
    { AV_CODEC_ID_H264, "h264_mp4toannexb" },
    { AV_CODEC_ID_HEVC, "hevc_mp4toannexb" },
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
    (void)vl;
    switch (level) {
        case AV_LOG_WARNING:
            MEDIA_LOG_W("Ffmpeg Message " PUBLIC_LOG_D32 " " PUBLIC_LOG_S "", level, fmt);
            break;
        case AV_LOG_ERROR:
            MEDIA_LOG_E("Ffmpeg Message " PUBLIC_LOG_D32 " " PUBLIC_LOG_S "", level, fmt);
            break;
        case AV_LOG_FATAL:
            MEDIA_LOG_E("Ffmpeg Message " PUBLIC_LOG_D32 " " PUBLIC_LOG_S "", level, fmt);
            break;
        case AV_LOG_INFO:
        case AV_LOG_DEBUG:
            MEDIA_LOG_D("Ffmpeg Message " PUBLIC_LOG_D32 " " PUBLIC_LOG_S "", level, fmt);
            break;
        default:
            break;
    }
}

void FfmpegLogInit()
{
    av_log_set_callback(FfmpegLogPrint);
}

bool CheckStartTime(const AVFormatContext *formatContext, const AVStream *stream, int64_t &timeStamp)
{
    int64_t startTime = 0;
    if (stream->start_time != AV_NOPTS_VALUE) {
        startTime = stream->start_time;
        if (timeStamp > 0 && startTime > INT64_MAX - timeStamp) {
            MEDIA_LOG_E("Seek value overflow with start time: " PUBLIC_LOG_D64 " timeStamp: " PUBLIC_LOG_D64 ".",
                startTime, timeStamp);
            return false;
        }
    }
    MEDIA_LOG_D("Get duration from track.");
    int64_t duration = stream->duration;
    if (duration == AV_NOPTS_VALUE) {
        duration = 0;
        const AVDictionaryEntry *metaDuration = av_dict_get(stream->metadata, "DURATION", NULL, 0);
        int64_t us;
        if (metaDuration && (av_parse_time(&us, metaDuration->value, 1) == 0)) {
            if (us > duration) {
                MEDIA_LOG_D("Get duration from metadddata.");
                duration = us;
            }
        }
    }
    if (duration <= 0) {
        if (formatContext->duration != AV_NOPTS_VALUE) {
            MEDIA_LOG_D("Get duration from formatContext.");
            duration = formatContext->duration;
        }
    }
    if (duration >= 0 && timeStamp > duration) {
        MEDIA_LOG_E("Seek to timestamp = " PUBLIC_LOG_D64 " failed, max = ." PUBLIC_LOG_D64 ".",
                        timeStamp, duration);
        return false;
    }
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        MEDIA_LOG_D("Reset timeStamp.");
        timeStamp += startTime;
    }
    return true;
}

int ConvertFlagsToFFmpeg(AVStream *avStream, int64_t ffTime, SeekMode mode)
{
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
    FALSE_RETURN_V_MSG_E(avStream.codecpar != nullptr, false,
        "Check track supported failed due to codec par is nullptr.");
    if (avStream.codecpar->codec_type != AVMEDIA_TYPE_AUDIO && avStream.codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
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
      ioContext_(),
      selectedTrackIds_(),
      bufferQueueMap_(),
      cacheQueue_("cacheQueue")
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Create FFmpeg Demuxer Plugin.");
#ifndef _WIN32
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
#endif
    FfmpegLogInit();
    MEDIA_LOG_D("Create FFmpeg Demuxer Plugin successfully.");
}

FFmpegDemuxerPlugin::~FFmpegDemuxerPlugin()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Destroy FFmpeg Demuxer Plugin.");
    if (!isThreadExit_) {
        InnerStop();
    }
#ifndef _WIN32
    (void)mallopt(M_FLUSH_THREAD_CACHE, 0);
#endif
    pluginImpl_ = nullptr;
    formatContext_ = nullptr;
    avbsfContext_ = nullptr;
    selectedTrackIds_.clear();
    bufferQueueMap_.clear();
    MEDIA_LOG_D("Destroy FFmpeg Demuxer Plugin successfully.");
}

Status FFmpegDemuxerPlugin::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Reset FFmpeg Demuxer Plugin.");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    isInited_ = false;

    ioContext_.offset = 0;
    ioContext_.eos = false;
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        cacheQueue_.RemoveTrackQueue(selectedTrackIds_[i]);
    }
    selectedTrackIds_.clear();
    bufferQueueMap_.clear();
    buffer_.reset();
    pluginImpl_.reset();
    formatContext_.reset();
    avbsfContext_.reset();
    readThread_.reset();
    copyThread_.reset();
    isThreadExit_ = true;
    isFileEOS_ = false;
    isInited_ = false;
    MEDIA_LOG_D("Reset FFmpeg Demuxer Plugin successfully.");
    return Status::OK;
}

void FFmpegDemuxerPlugin::InitBitStreamContext(const AVStream& avStream)
{
    FALSE_RETURN_MSG(avStream.codecpar != nullptr, "Init BitStreamContext failed due to codec par is nullptr.");
    AVCodecID codecID = avStream.codecpar->codec_id;
    MEDIA_LOG_I("Init BitStreamContext for track " PUBLIC_LOG_D32 "[" PUBLIC_LOG_S "].", avStream.index, avcodec_get_name(codecID));
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
    MEDIA_LOG_I("Track " PUBLIC_LOG_D32 " will convert to annexb.", avStream.index);
}

Status FFmpegDemuxerPlugin::ConvertAvcOrHevcToAnnexb(AVPacket& pkt)
{
    int ret;
    ret = av_bsf_send_packet(avbsfContext_.get(), &pkt);
    MEDIA_LOG_D("Convert avcc/hevc to annexb for track " PUBLIC_LOG_D32 "", pkt.stream_index);
    FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_UNKNOWN, 
        "Convert avc/hevc to annexb failed due to av_bsf_send_packet failed, err:" PUBLIC_LOG_S ".", AVStrError(ret).c_str());

    av_packet_unref(&pkt);

    ret = 1;
    while (ret >= 0) {
        ret = av_bsf_receive_packet(avbsfContext_.get(), &pkt);
        if ((ret < 0) && (ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
            MEDIA_LOG_E(
                "Convert avc/hevc to annexb failed due to av_bsf_receive_packet failed, err:" PUBLIC_LOG_S ".",
                AVStrError(ret).c_str());
            return Status::ERROR_UNKNOWN;
        }
    }

    MEDIA_LOG_D("Convert avc/hevc to annexb successfully, ret:" PUBLIC_LOG_S ".", AVStrError(ret).c_str());
    return Status::OK;
}

bool FFmpegDemuxerPlugin::GetBufferFromUserQueue(uint32_t queueIndex, int32_t size)
{
    MEDIA_LOG_D("Get buffer from user queue " PUBLIC_LOG_D32 ".", queueIndex);
    FALSE_RETURN_V_MSG_E(IsInSelectedTrack(queueIndex) && bufferQueueMap_.count(queueIndex) > 0, false,
        "Get buffer failed due to track index has not been selected.");

    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = size;
    Status ret = bufferQueueMap_[queueIndex]->RequestBuffer(buffer_, avBufferConfig, 100);
    if (ret == Status::ERROR_WAIT_TIMEOUT) {
        MEDIA_LOG_D("Get buffer failed due to not more buffer, consume firstly");
    }
    if (ret != Status::OK) {
        MEDIA_LOG_D("Get buffer failed due to get buffer from bufferQueue failed.");
        return false;
    }
    FALSE_RETURN_V_MSG_E(buffer_->memory_ != nullptr, false, "Get buffer failed due to inner memory is nullptr.");
    FALSE_RETURN_V_MSG_E(buffer_->memory_->GetCapacity() > 0, false,
        "Get buffer failed due to capacity of memory in buffer is nonpositive.");
    return true;
}

Status FFmpegDemuxerPlugin::WriteBuffer(
    int32_t queueIndex, int64_t pts, uint32_t flag, const uint8_t *writeData, int32_t writeSize)
{
    buffer_->pts_ = pts;
    buffer_->flag_ = flag;
    if (writeData != nullptr && writeSize > 0) {
        int32_t ret = buffer_->memory_->Write(writeData, writeSize, 0);
        FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_INVALID_OPERATION,
            "Convert info failed due to write memory in buffer failed.");
    }
    Status queRet = bufferQueueMap_[queueIndex]->PushBuffer(buffer_, true);
    FALSE_RETURN_V_MSG_E(queRet == Status::OK, Status::ERROR_INVALID_OPERATION,
        "Convert info failed due to push buffer to bufferQueue failed.");
    MEDIA_LOG_D("CurrentBuffer:");
    MEDIA_LOG_D("pts=" PUBLIC_LOG_D64 ", flag=." PUBLIC_LOG_U32, buffer_->pts_, buffer_->flag_);
    MEDIA_LOG_D("mem size=" PUBLIC_LOG_D32 ".", buffer_->memory_->GetSize());
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ConvertAVPacketToFrameInfo(std::shared_ptr<SamplePacket> samplePacket)
{
    FALSE_RETURN_V_MSG_E((samplePacket != nullptr && samplePacket->pkt != nullptr), Status::ERROR_INVALID_OPERATION,
        "Convert packet info failed due to input packet is nullptr.");

    MEDIA_LOG_D("Convert packet info for track " PUBLIC_LOG_D32 ", copy start offset: " PUBLIC_LOG_D64 ".",
        samplePacket->pkt->stream_index, samplePacket->offset);
    
    FALSE_RETURN_V_MSG_E((samplePacket->pkt->size >= 0), Status::ERROR_INVALID_OPERATION,
        "Convert packet info failed due to input packet is empty.");

    int64_t pts = 0;
    AVStream *avStream = formatContext_->streams[samplePacket->pkt->stream_index];
    if (avStream->start_time == AV_NOPTS_VALUE) {
        avStream->start_time = 0;
    }
    if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        pts = AvTime2Us(ConvertTimeFromFFmpeg(samplePacket->pkt->pts - avStream->start_time, avStream->time_base));
    } else if (avStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        pts = AvTime2Us(ConvertTimeFromFFmpeg(samplePacket->pkt->pts, avStream->time_base));
    }

    int32_t remainSize = samplePacket->pkt->size - samplePacket->offset;
    int32_t bufferCap = buffer_->memory_->GetCapacity();
    int32_t copySize = remainSize < bufferCap ? remainSize : bufferCap;
    MEDIA_LOG_D("BufferCap=" PUBLIC_LOG_D32 ", pktSize=" PUBLIC_LOG_D32 ", remainSize=" PUBLIC_LOG_D32 ", copySize=" PUBLIC_LOG_D32 ", copyOffset" PUBLIC_LOG_D64 ".",
        bufferCap, samplePacket->pkt->size, remainSize, copySize, samplePacket->offset);

    uint32_t flag = ConvertFlagsFromFFmpeg(*(samplePacket->pkt), (copySize != samplePacket->pkt->size));
    Status ret = WriteBuffer(
        samplePacket->pkt->stream_index, pts, flag, 
        samplePacket->pkt->data+samplePacket->offset, copySize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Convert packet info failed due to write buffer failed.");

    if (copySize < remainSize) {
        samplePacket->offset += copySize;
        MEDIA_LOG_D("Buffer is not enough, next buffer to save remain data.");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }

    av_packet_free(&(samplePacket->pkt));
    return Status::OK;
}

void FFmpegDemuxerPlugin::PushEosBufferToCacheQueue()
{
    MEDIA_LOG_I("Push EOS buffer for all cache buffer queue.");
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        auto streamIndex = selectedTrackIds_[i];
        MEDIA_LOG_I("Cache queue " PUBLIC_LOG_D32 ".", streamIndex);
        std::shared_ptr<SamplePacket> eosSample = std::make_shared<SamplePacket>();
        eosSample->isEOS = true;
        cacheQueue_.Push(streamIndex, eosSample);
    }
    MEDIA_LOG_I("Push EOS buffer for all cache buffer queue finish.");
}

void FFmpegDemuxerPlugin::PushEosBufferToUserQueue()
{
    MEDIA_LOG_I("Push EOS buffer for all user buffer queue.");
    for (auto item : bufferQueueMap_) {
        MEDIA_LOG_D("User queue " PUBLIC_LOG_D32 ".", item.first);
        bool requestSuccessfully = GetBufferFromUserQueue(item.first, 1);
        FALSE_RETURN_MSG(requestSuccessfully,
            "Push EOS buffer for track " PUBLIC_LOG_D32 " failed due to request buffer from buffer queue failed.", item.first);

        Status ret = WriteBuffer(item.first, 0, (uint32_t)(AVBufferFlag::EOS), nullptr, 0);
        FALSE_RETURN_MSG(ret == Status::OK, "Push EOS buffer failed due to write buffer failed.");
    }
    MEDIA_LOG_D("Push EOS buffer for all user buffer queue finish.");
}

Status FFmpegDemuxerPlugin::ReadFrameToCacheQueue()
{
    MEDIA_LOG_D("Read next frame.");
    int ffmpegRet = 0;
    AVPacket *pkt = av_packet_alloc();
    FALSE_RETURN_V_MSG_E(pkt != nullptr, Status::ERROR_NULL_POINTER,
        "Read next frame failed due to av_packet_alloc failed, err:" PUBLIC_LOG_S ".", AVStrError(ffmpegRet).c_str());
    do {
        ffmpegRet = av_read_frame(formatContext_.get(), pkt);
    } while (ffmpegRet >= 0 && !selectedTrackIds_.empty() && (pkt != nullptr && !IsInSelectedTrack(pkt->stream_index)));
    MEDIA_LOG_D("Read next frame, ret=" PUBLIC_LOG_D32 ", selectedTrackIds is empty=" PUBLIC_LOG_D32 ".",
        ffmpegRet, selectedTrackIds_.empty());
    if (ffmpegRet == AVERROR_EOF) {
        av_packet_free(&pkt);
        isFileEOS_ = true;
        MEDIA_LOG_W("Read frame failed due to reach end of track.");
        return Status::END_OF_STREAM;
    } else if (ffmpegRet < 0) {
        av_packet_free(&pkt);
        MEDIA_LOG_E("Read frame failed due to av_read_frame failed, err:" PUBLIC_LOG_S ".", AVStrError(ffmpegRet).c_str());
        return Status::ERROR_UNKNOWN;
    } else if (selectedTrackIds_.empty()) {
        av_packet_free(&pkt);
        MEDIA_LOG_W("Read frame failed due to no track has been selected.");
    } else if (pkt == nullptr || pkt->size < 0) {
        av_packet_free(&pkt);
        MEDIA_LOG_E("Read frame failed due to av_read_frame failed.");
        return Status::ERROR_NULL_POINTER;
    } else {
        uint32_t streamIndex = static_cast<uint32_t>(pkt->stream_index);
        AVStream *avStream = formatContext_->streams[streamIndex];
        if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (!avbsfContext_) {
                InitBitStreamContext(*avStream);
            }
            if (avbsfContext_) {
                MEDIA_LOG_D("Frame is video, need to convert to annexb.");
                Status ret = ConvertAvcOrHevcToAnnexb(*pkt);
                if (ret != Status::OK) {
                    av_packet_free(&pkt);
                    MEDIA_LOG_E("Read next frame failed due to convert avc/hevc to annexb failed.");
                    return ret;
                }
            }
        }
        std::shared_ptr<SamplePacket> cacheSamplePacket = std::make_shared<SamplePacket>();
        cacheSamplePacket->offset = 0;
        cacheSamplePacket->pkt = pkt;

        time_t startTime = time(nullptr);
        bool pushed = false;
        bool blockLog = false;
        while (time(nullptr) - startTime < CACHE_BLOCK_LIMIT) {
            if (cacheQueue_.GetValidCacheSize(streamIndex) > 0) {
                if (blockLog) {
                    MEDIA_LOG_D("[ReadLoop Stop Block] Cache queeu is valid");
                    blockLog = false;
                }
                cacheQueue_.Push(streamIndex, cacheSamplePacket);
                pkt = nullptr;
                pushed = true;
                break;
            } else {
                if (!blockLog) {
                    MEDIA_LOG_D("[ReadLoop Start Block] Cache queeu is full, wait for copying to user buffer queue");
                    blockLog = true;
                }
                MEDIA_LOG_D("Cache queeu is full, waiting 100us...");
                usleep(100);
            }
        }
        if (!pushed) {
            av_packet_free(&pkt);
            MEDIA_LOG_E("Read frame failed due to push to cache failed.");
            return Status::ERROR_TIMED_OUT;
        }
    }
    MEDIA_LOG_D("Read next frame finish.");
    return Status::OK;
}

void FFmpegDemuxerPlugin::ReadLoop()
{
    MEDIA_LOG_I("Enter [" PUBLIC_LOG_S "] read thread.", threadReadName_.c_str());
    constexpr uint32_t nameSizeMax = 15;
    pthread_setname_np(pthread_self(), threadReadName_.substr(0, nameSizeMax).c_str());
    for (;;) {
        if (isThreadExit_) {
            MEDIA_LOG_I("Exit [" PUBLIC_LOG_S "] read thread.", threadReadName_.c_str());
            break;
        }
        if (isFileEOS_) {
            PushEosBufferToCacheQueue();
            MEDIA_LOG_I("Exit [" PUBLIC_LOG_S "] read thread, read to end.", threadReadName_.c_str());
            break;
        }
        (void)ReadFrameToCacheQueue();
    }
}

Status FFmpegDemuxerPlugin::CopyFrameToUserQueue()
{
    MEDIA_LOG_D("Copy one loop.");
    for (auto item : bufferQueueMap_) {
        if (!cacheQueue_.HasCache(item.first)) {
            continue;
        }

        std::shared_ptr<SamplePacket> samplePacket = cacheQueue_.Front(item.first);

        if (samplePacket->isEOS) {
            MEDIA_LOG_I("File is end, push EOS buffer to user queue.");
            PushEosBufferToUserQueue();
            cacheQueue_.Pop(item.first);
            continue;
        }
        
        if (samplePacket->pkt == nullptr || !GetBufferFromUserQueue(item.first, samplePacket->pkt->size)) {
            continue;
        }
        Status ret = ConvertAVPacketToFrameInfo(samplePacket);
        if (ret == Status::OK) {
            cacheQueue_.Pop(item.first);
        }
        MEDIA_LOG_D("Copy ret=" PUBLIC_LOG_D32 "", (uint32_t)(ret));
        return ret;
    }
    return Status::OK;
}

void FFmpegDemuxerPlugin::CopyLoop()
{
    MEDIA_LOG_I("Enter [" PUBLIC_LOG_S "] copy thread.", threadCopyName_.c_str());
    constexpr uint32_t nameSizeMax = 15;
    pthread_setname_np(pthread_self(), threadCopyName_.substr(0, nameSizeMax).c_str());
    for (;;) {
        if (isThreadExit_) {
            MEDIA_LOG_I("Exit [" PUBLIC_LOG_S "] copy thread.", threadCopyName_.c_str());
            break;
        }
        (void)CopyFrameToUserQueue();
    }
}

Status FFmpegDemuxerPlugin::Start()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Prepare to start FFmpeg Demuxer Plugin.");
    FALSE_RETURN_V_MSG_E(isInited_, Status::ERROR_WRONG_STATE, "Can not call this func before set data source.");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::OK,
        "Process has been started already, neet to stop it first.");
    FALSE_RETURN_V_MSG_E(readThread_ == nullptr, Status::OK, "Read task is running already.");
    FALSE_RETURN_V_MSG_E(copyThread_ == nullptr, Status::OK, "Copy task is running already.");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Start demuxer failed due to AVFormatContext is nullptr.");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION,
        "Start demuxer failed due to no track has been selected.");
    threadReadName_ = "DemuxerReadLoop";
    threadCopyName_ = "DemuxerCopyLoop";
    for (auto trackId : selectedTrackIds_) {
        if (bufferQueueMap_.count(trackId) == 0) {
            InnerUnselectTrack(trackId);
            MEDIA_LOG_W("Track " PUBLIC_LOG_D32 " has not been set buffer queue, will not be demuxed.", trackId);
        }
    }

    isThreadExit_ = false;
    copyThread_ = std::make_unique<std::thread>(&FFmpegDemuxerPlugin::CopyLoop, this);
    MEDIA_LOG_I("Demuxer copy thread started.");
    readThread_ = std::make_unique<std::thread>(&FFmpegDemuxerPlugin::ReadLoop, this);
    MEDIA_LOG_I("Demuxer read thread started.");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::InnerStop()
{
    FALSE_RETURN_V_MSG_E(!isThreadExit_, Status::OK, "Process has been stopped already, need to start if first.");
    FALSE_RETURN_V_MSG_E(std::this_thread::get_id() != readThread_->get_id(),
        Status::ERROR_INVALID_PARAMETER, "Stop at the copy task thread, reject.");
    isThreadExit_ = true;
    std::unique_ptr<std::thread> t;
    std::swap(readThread_, t);
    if (t != nullptr && t->joinable()) {
        t->join();
    }
    readThread_ = nullptr;
    MEDIA_LOG_I("Demuxer read thread stopped.");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Prepare to stop FFmpeg Demuxer Plugin");
    return InnerStop(); 
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
        auto result = ioContext->dataSource->ReadAt(ioContext->offset, buffer, static_cast<size_t>(bufSize));
        MEDIA_LOG_D("AVReadPacket read data size = " PUBLIC_LOG_D32 ".", static_cast<int>(buffer->GetMemory()->GetSize()));
        if (result == Status::OK) {
            ioContext->offset += buffer->GetMemory()->GetSize();
            ret = buffer->GetMemory()->GetSize();
        } else if (result == Status::END_OF_STREAM) {
            MEDIA_LOG_D("File is end.");
            ioContext->eos = true;
            ret = AVERROR_EOF;
        } else {
            MEDIA_LOG_E("AVReadPacket failed, ret=" PUBLIC_LOG_D32 ".", static_cast<int>(result));
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
                MEDIA_LOG_D("AVSeek whence: " PUBLIC_LOG_D32 ", pos = " PUBLIC_LOG_D64 ", newPos = " PUBLIC_LOG_U64 ".",
                         whence, offset, newPos);
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

    int ret = avformat_open_input(&formatContext, nullptr, pluginImpl_.get(), nullptr);
    FALSE_RETURN_MSG((ret == 0),
        "Init AVFormatContext failed due to avformat_open_input failed by " PUBLIC_LOG_S ", err:" PUBLIC_LOG_S ".",
        pluginImpl_->name, AVStrError(ret).c_str());

    ret = avformat_find_stream_info(formatContext, NULL);
    FALSE_RETURN_MSG((ret >= 0),
        "Init AVFormatContext failed due to avformat_find_stream_info failed by " PUBLIC_LOG_S ", err:" PUBLIC_LOG_S ".",
            pluginImpl_->name, AVStrError(ret).c_str());

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
            avformat_close_input(&ptr);
        }
    });
}

Status FFmpegDemuxerPlugin::SetDataSource(const std::shared_ptr<DataSource>& source)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Set data source for demuxer.");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    FALSE_RETURN_V_MSG_E(!isInited_, Status::ERROR_WRONG_STATE, "DataSource has been inited, need reset first.");
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set datasource failed due to source is nullptr.");

    ioContext_.dataSource = source;
    seekable_ = source->GetSeekable();

    pluginImpl_ = g_pluginInputFormat[pluginName_];
    FALSE_RETURN_V_MSG_E(pluginImpl_ != nullptr, Status::ERROR_UNSUPPORTED_FORMAT,
        "Set datasource failed due to can not find inputformat for format.");

    InitAVFormatContext();
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_UNKNOWN,
        "Set datasource failed due to can not init formatContext for source.");
    isInited_ = true;
    MEDIA_LOG_D("Set data source for demuxer successfully.");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::GetMediaInfo(MediaInfo& mediaInfo)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Get media info by FFmpeg Demuxer Plugin.");
    FALSE_RETURN_V_MSG_E(isInited_, Status::ERROR_WRONG_STATE, "Can not call this func before set data source.");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Get media info failed due to formatContext_ is nullptr.");
    
    FFmpegFormatHelper::ParseMediaInfo(*formatContext_, mediaInfo.general);

    AVStream* avStream;
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        Meta meta;
        avStream = formatContext_->streams[trackIndex];
        if (avStream == nullptr) {
            MEDIA_LOG_W("Get track " PUBLIC_LOG_D32 " info failed due to track is nullptr.", trackIndex);
        } else {
            FFmpegFormatHelper::ParseTrackInfo(*avStream, meta);
        }
        mediaInfo.tracks.push_back(meta);
    }
    return Status::OK;
}

void FFmpegDemuxerPlugin::ShowSelectedTracks()
{
    std::string selectedTracksString;
    for (auto index : selectedTrackIds_) {
        selectedTracksString += std::to_string(index) + " | ";
    }
    MEDIA_LOG_I("Total track : " PUBLIC_LOG_D32 " | Selected tracks: " PUBLIC_LOG_S ".",
        formatContext_.get()->nb_streams, selectedTracksString.c_str());
}

bool FFmpegDemuxerPlugin::IsInSelectedTrack(const uint32_t trackId)
{
    return std::any_of(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                       [trackId](uint32_t id) { return id == trackId; });
}

Status FFmpegDemuxerPlugin::InnerSelectTrack(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(isInited_, Status::ERROR_WRONG_STATE, "Can not call this func before set data source.");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Select track failed due to AVFormatContext is nullptr.");
    if (trackId >= formatContext_.get()->nb_streams) {
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
        return cacheQueue_.AddTrackQueue(trackId);
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " is already in selected list.", trackId);
    }
    MEDIA_LOG_D("Select track successfully.");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::SelectTrack(uint32_t trackId)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Select track " PUBLIC_LOG_D32 ".", trackId);
    ShowSelectedTracks();
    return InnerSelectTrack(trackId);
}

Status FFmpegDemuxerPlugin::InnerUnselectTrack(uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(isInited_, Status::ERROR_WRONG_STATE, "Can not call this func before set data source.");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");

    auto index = std::find_if(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                              [trackId](uint32_t selectedId) {return trackId == selectedId; });
    if (IsInSelectedTrack(trackId)) {
        selectedTrackIds_.erase(index);
        return cacheQueue_.RemoveTrackQueue(trackId);
    } else {
        MEDIA_LOG_W("Unselect track failed due to track " PUBLIC_LOG_U32 " is not in selected list.", trackId);
    }
    MEDIA_LOG_D("Unselect track successfully.");
    return Status::OK;
}

Status FFmpegDemuxerPlugin::UnselectTrack(uint32_t trackId)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Unselect track " PUBLIC_LOG_D32 ".", trackId);
    ShowSelectedTracks();
    return InnerUnselectTrack(trackId);
}

Status FFmpegDemuxerPlugin::SetOutputBufferQueue(uint32_t trackId, const sptr<AVBufferQueueProducer>& bufferQueue)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Set bufferQueue for track " PUBLIC_LOG_D32 ".", trackId);
    FALSE_RETURN_V_MSG_E(isInited_, Status::ERROR_WRONG_STATE, "Can not call this func before set data source.");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    FALSE_RETURN_V_MSG_E(bufferQueue != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set bufferQueue failed due to input bufferQueue is nullptr.");
    if (bufferQueueMap_.count(trackId) != 0) {
        MEDIA_LOG_W("BufferQueue has been set already, will be overwritten.");
    }
    Status ret = InnerSelectTrack(trackId);
    if (ret == Status::OK) {
        bufferQueueMap_.insert(std::pair<uint32_t, sptr<AVBufferQueueProducer>>(trackId, bufferQueue));
        MEDIA_LOG_I("Set bufferQueue successfully.");
    }
    return ret;
}

Status FFmpegDemuxerPlugin::SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode, int64_t& realSeekTime)
{
    (void) trackId;
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_I("Seek to " PUBLIC_LOG_D64 ", mode=" PUBLIC_LOG_D32 ".", seekTime, static_cast<int32_t>(mode));
    FALSE_RETURN_V_MSG_E(isInited_, Status::ERROR_WRONG_STATE, "Can not call this func before set data source.");
    FALSE_RETURN_V_MSG_E(isThreadExit_, Status::ERROR_WRONG_STATE, "Process is running, need to stop if first.");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER,
        "Seek failed due to AVFormatContext is nullptr.");
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
    if (!CheckStartTime(formatContext_.get(), avStream, ffTime)) {
        MEDIA_LOG_E("Seek failed due to check get start time from track " PUBLIC_LOG_D32 " failed.", trackIndex);
        return Status::ERROR_INVALID_PARAMETER;
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
        MEDIA_LOG_I("Reset the cache buffer queue.");
        cacheQueue_.RemoveTrackQueue(selectedTrackIds_[i]);
        cacheQueue_.AddTrackQueue(selectedTrackIds_[i]);
    }
    isFileEOS_ = false;
    return Status::OK;
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
    uint64_t fileSize = 0;
    if (dataSource->GetSize(fileSize) == Status::OK) {
        bufferSize = (bufferSize < fileSize) ? bufferSize : fileSize;
    }
    // fix ffmpeg probe crash,refer to ffmpeg/tools/probetest.c
    std::vector<uint8_t> buff(bufferSize + AVPROBE_PADDING_SIZE);
    auto bufferInfo = std::make_shared<Buffer>();
    auto bufData = bufferInfo->WrapMemory(buff.data(), bufferSize, bufferSize);
    FALSE_RETURN_V_MSG_E(bufData != nullptr, 0, "Sniff failed due to alloc buffer failed.");
    MEDIA_LOG_D("Prepare buffer for probe, input param bufferSize=" PUBLIC_LOG_D32 ", real buffer size=" PUBLIC_LOG_D32 ".",
        bufferSize + AVPROBE_PADDING_SIZE, bufferSize);

    Status ret = dataSource->ReadAt(0, bufferInfo, bufferSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, 0, "Sniff failed due to read probe data failed.");
    FALSE_RETURN_V_MSG_E(bufData!= nullptr && buff.data() != nullptr, 0, "Sniff failed due to probe data invalid.");

    AVProbeData probeData{"", buff.data(), static_cast<int>(bufferInfo->GetMemory()->GetSize()), ""};
    int confidence = plugin->read_probe(&probeData);

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
            MEDIA_LOG_E("RegisterPlugins failed due to add plugin failed, err=" PUBLIC_LOG_D32 ".", static_cast<int>(ret));
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
} // namespace Plugin
} // namespace Media
} // namespace OHOS
