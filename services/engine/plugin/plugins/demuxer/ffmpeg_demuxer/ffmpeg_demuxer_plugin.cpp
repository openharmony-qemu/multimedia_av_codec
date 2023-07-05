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

#include <algorithm>
#include <string>
#include <sstream>
#include <type_traits>
#include <cinttypes>
#include <malloc.h>
#include "securec.h"
#include "avcodec_errors.h"
#include "native_avcodec_base.h"
#include "plugin_definition.h"
#include "avcodec_log.h"
#include "avcodec_dfx.h"
#include "ffmpeg_demuxer_plugin.h"

#if defined(LIBAVFORMAT_VERSION_INT) && defined(LIBAVFORMAT_VERSION_INT)
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 78, 0) and LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(58, 64, 100)
#if LIBAVFORMAT_VERSION_INT != AV_VERSION_INT(58, 76, 100)
#include "libavformat/internal.h"
#endif
#endif
#endif

#define AV_CODEC_TIME_BASE (static_cast<int64_t>(1))
#define AV_CODEC_NSECOND AV_CODEC_TIME_BASE
#define AV_CODEC_USECOND (static_cast<int64_t>(1000) * AV_CODEC_NSECOND)
#define AV_CODEC_MSECOND (static_cast<int64_t>(1000) * AV_CODEC_USECOND)
#define AV_CODEC_SECOND (static_cast<int64_t>(1000) * AV_CODEC_MSECOND)


namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "FFmpegDemuxerPlugin"};
}


namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
namespace FFmpeg {
namespace {
static const std::map<AVSeekMode, int32_t>  g_seekModeToFFmpegSeekFlags = {
    { AVSeekMode::SEEK_MODE_PREVIOUS_SYNC, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD },
    { AVSeekMode::SEEK_MODE_NEXT_SYNC, AVSEEK_FLAG_FRAME },
    { AVSeekMode::SEEK_MODE_CLOSEST_SYNC, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_ANY }
};

constexpr int32_t MAX_CONFIDENCE = 100;

int32_t Sniff(const std::string& pluginName)
{
    return MAX_CONFIDENCE;
}

Status RegisterDemuxerPlugins(const std::shared_ptr<Register>& reg)
{
    DemuxerPluginDef def;
    constexpr int32_t rankScore = 100;
    def.name = "ffmpegDemuxer";
    def.description = "ffmpeg demuxer";
    def.rank = rankScore;
    def.creator = []() -> std::shared_ptr<DemuxerPlugin> {
        return std::make_shared<FFmpegDemuxerPlugin>();
    };
    def.sniffer = Sniff;
    return reg->AddPlugin(def);
}

PLUGIN_DEFINITION(FFmpegDemuxer, LicenseType::GPL, RegisterDemuxerPlugins, [] {})
}

inline int64_t AvTime2Ms(int64_t hTime)
{
    return hTime / AV_CODEC_MSECOND;
}

inline int64_t AvTime2Us(int64_t hTime)
{
    return hTime / AV_CODEC_USECOND;
}

bool CheckStartTime(AVStream *stream, int64_t &timeStamp)
{
    int64_t startTime = 0;
    if (stream->start_time != AV_NOPTS_VALUE) {
        startTime = stream->start_time;
        if (timeStamp > 0 && startTime > INT64_MAX - timeStamp) {
            AVCODEC_LOGE("seek value overflow with start time: %{public}" PRId64 " timeStamp: %{public}" PRId64 "",
                startTime, timeStamp);
            return false;
        }
        timeStamp += startTime;
    }
    if (timeStamp > (stream->duration + startTime)) {
        AVCODEC_LOGE("seek to timestamp = %{public}" PRId64 " failed, max = %{public}" PRId64,
                        timeStamp, stream->duration);
        return false;
    }
    return true;
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

int64_t FFmpegDemuxerPlugin::GetTotalStreamFrames(int streamIndex)
{
    AVCODEC_LOGD("FFmpegDemuxerPlugin::GetTotalStreamFrames is called");
    return formatContext_->streams[streamIndex]->nb_frames;
}


AVCodecBufferFlag FFmpegDemuxerPlugin::ConvertFlagsFromFFmpeg(AVPacket* pkt,  AVStream* avStream)
{
    AVCodecBufferFlag flags = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    if (static_cast<uint32_t>(pkt->flags) & static_cast<uint32_t>(AV_PKT_FLAG_KEY)) {
        flags = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME;
    }
    return flags;
}

int32_t FFmpegDemuxerPlugin::Create(uintptr_t sourceAddr)
{
    AVCODEC_LOGI("FFmpegDemuxerPlugin::Create");
    if (std::is_object<decltype(sourceAddr)>::value) {
        // Do not automatically release the source object whose address is sourceAddr when the shared_ptr
        // formatContext_ is released. Because this responsibility belongs to manually calling source deconstruction
        // function. Otherwise, the main service will crash.
        formatContext_ = std::shared_ptr<AVFormatContext>((AVFormatContext*)sourceAddr,
                                                          [](AVFormatContext* p) { (void)p; });
        SetBitStreamFormat();
        AVCODEC_LOGD("create FFmpegDemuxerPlugin successful.");
    } else {
        formatContext_ = nullptr;
        AVCODEC_LOGW("create FFmpegDemuxerPlugin failed, becasue sourceAddr is not a class address.");
        return AVCS_ERR_INVALID_VAL;
    }
    return AVCS_ERR_OK;
}

FFmpegDemuxerPlugin::FFmpegDemuxerPlugin()
    :blockQueue_("cache_que")
{
    AVCODEC_LOGI("FFmpegDemuxerPlugin::FFmpegDemuxerPlugin");
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
}

FFmpegDemuxerPlugin::~FFmpegDemuxerPlugin()
{
    AVCODEC_LOGI("FFmpegDemuxerPlugin::~FFmpegDemuxerPlugin");
    (void)mallopt(M_FLUSH_THREAD_CACHE, 0);
    selectedTrackIds_.clear();
}

int32_t FFmpegDemuxerPlugin::SetBitStreamFormat()
{
    AVCODEC_LOGI("FFmpegDemuxerPlugin::SetBitStreamFormat");
    uint32_t trackCount = formatContext_->nb_streams;
    for (uint32_t i = 0; i < trackCount; i++) {
        if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            InitBitStreamContext(*(formatContext_->streams[i]));
        }
    }
    return AVCS_ERR_OK;
}

int32_t FFmpegDemuxerPlugin::SelectTrackByID(uint32_t trackIndex)
{
    AVCODEC_LOGI("FFmpegDemuxerPlugin::SelectTrackByID: trackIndex=%{public}u", trackIndex);
    std::unique_lock<std::mutex> lock(mutex_);
    std::stringstream selectedTracksString;
    for (const auto &index : selectedTrackIds_) {
        selectedTracksString << index << " | ";
    }
    AVCODEC_LOGI("Total track in file: %{public}d | add track index: %{public}u",
        formatContext_.get()->nb_streams, trackIndex);
    AVCODEC_LOGI("Selected tracks in file: %{public}s.", selectedTracksString.str().c_str());

    if (trackIndex >= static_cast<uint32_t>(formatContext_.get()->nb_streams)) {
        AVCODEC_LOGE("trackIndex is invalid! Just have %{public}d tracks in file", formatContext_.get()->nb_streams);
        return AVCS_ERR_INVALID_VAL;
    }
    AVStream* avStream = formatContext_->streams[trackIndex];
    if (avStream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO && avStream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
        AVCODEC_LOGE("unsupport stream type");
        return AVCS_ERR_INVALID_VAL;
    }
    if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && avStream->codecpar->codec_id == AV_CODEC_ID_RAWVIDEO) {
        AVCODEC_LOGE("unsupport raw video");
        return AVCS_ERR_INVALID_VAL;
    }
    auto index = std::find_if(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                              [trackIndex](uint32_t selectedId) {return trackIndex == selectedId;});
    if (index == selectedTrackIds_.end()) {
        selectedTrackIds_.push_back(trackIndex);
        if (trackIsEnd_.count(trackIndex) == 0) {
            trackIsEnd_.insert(std::make_pair(trackIndex, false));
        }
        return blockQueue_.AddTrackQueue(trackIndex);
    } else {
        AVCODEC_LOGW("track %{public}u is already in selected list!", trackIndex);
    }
    return AVCS_ERR_OK;
}

int32_t FFmpegDemuxerPlugin::UnselectTrackByID(uint32_t trackIndex)
{
    AVCODEC_LOGI("FFmpegDemuxerPlugin::UnselectTrackByID: trackIndex=%{public}u", trackIndex);
    std::unique_lock<std::mutex> lock(mutex_);
    std::stringstream selectedTracksString;
    for (const auto &index : selectedTrackIds_) {
        selectedTracksString << index << " | ";
    }
    AVCODEC_LOGI("Selected track in file: %{public}s | remove track: %{public}u",
        selectedTracksString.str().c_str(), trackIndex);

    auto index = std::find_if(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                              [trackIndex](uint32_t selectedId) {return trackIndex == selectedId; });
    if (index != selectedTrackIds_.end()) {
        selectedTrackIds_.erase(index);
        return blockQueue_.RemoveTrackQueue(trackIndex);
    } else {
        AVCODEC_LOGW("Unselect track failed, track %{public}u is not in selected list!", trackIndex);
    }
    return AVCS_ERR_OK;
}

std::vector<uint32_t> FFmpegDemuxerPlugin::GetSelectedTrackIds()
{
    AVCODEC_LOGD("FFmpegDemuxerPlugin::GetSelectedTrackIds");
    std::vector<uint32_t> trackIds;
    trackIds = selectedTrackIds_;
    return trackIds;
}

bool FFmpegDemuxerPlugin::IsInSelectedTrack(uint32_t trackIndex)
{
    return std::any_of(selectedTrackIds_.begin(), selectedTrackIds_.end(),
                       [trackIndex](uint32_t id) { return id == trackIndex; });
}

void FFmpegDemuxerPlugin::InitBitStreamContext(const AVStream& avStream)
{
    AVCODEC_LOGI("FFmpegDemuxerPlugin::InitBitStreamContext");
    const AVBitStreamFilter* avBitStreamFilter {nullptr};
    AVCodecID codecID = avStream.codecpar->codec_id;
    if (codecID == AV_CODEC_ID_H264) {
        AVCODEC_LOGI("codec_id is H264, will convert to annexb");
        avBitStreamFilter = av_bsf_get_by_name("h264_mp4toannexb");
    } else {
        AVCODEC_LOGW("Can not find valid bit stream filter for %{public}s, stream will not be converted",
                     avcodec_get_name(codecID));
        return;
    }
    if (avBitStreamFilter && !avbsfContext_) {
        AVBSFContext* avbsfContext {nullptr};
        int ret = av_bsf_alloc(avBitStreamFilter, &avbsfContext);
        if (ret < 0) {
            AVCODEC_LOGE("init bitStreamContext failed when av_bsf_alloc, err:%{public}s", av_err2str(ret));
        }
        ret = avcodec_parameters_copy(avbsfContext->par_in, avStream.codecpar);
        if (ret < 0) {
            AVCODEC_LOGE("init bitStreamContext failed when avcodec_parameters_copy, err:%{public}s", av_err2str(ret));
        }
        ret = av_bsf_init(avbsfContext);
        if (ret < 0) {
            AVCODEC_LOGE("init bitStreamContext failed when av_bsf_init, err:%{public}s", av_err2str(ret));
        }
        avbsfContext_ = std::shared_ptr<AVBSFContext>(avbsfContext, [](AVBSFContext* ptr) {
            if (ptr) {
                av_bsf_free(&ptr);
            }
        });
    }
    if (avbsfContext_ == nullptr) {
        AVCODEC_LOGW("init bitStreamContext failed for format %{public}s, stream will not be converted",
                     avcodec_get_name(codecID));
    }
}

void FFmpegDemuxerPlugin::ConvertAvcOrHevcToAnnexb(AVPacket& pkt)
{
    (void)av_bsf_send_packet(avbsfContext_.get(), &pkt);
    (void)av_packet_unref(&pkt);
    int ret = 1;
    while (ret >= 0) {
        ret = av_bsf_receive_packet(avbsfContext_.get(), &pkt);
    }
}

int32_t FFmpegDemuxerPlugin::ConvertAVPacketToSample(AVStream* avStream, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo &bufferInfo, AVCodecBufferFlag &flag, std::shared_ptr<SamplePacket> samplePacket)
{
    if (samplePacket == nullptr || samplePacket->pkt == nullptr) {
        return AVCS_ERR_INVALID_OPERATION;
    }
    uint64_t frameSize = 0;
    if (avStream->start_time == AV_NOPTS_VALUE) {
        avStream->start_time = 0;
    }
    if (avStream->duration + avStream->start_time <= samplePacket->pkt->pts) {
        SetEndStatus(samplePacket->pkt->stream_index);
    }
    bufferInfo.presentationTimeUs = AvTime2Us(ConvertTimeFromFFmpeg(samplePacket->pkt->pts - avStream->start_time,
                                                                    avStream->time_base));
    flag = ConvertFlagsFromFFmpeg(samplePacket->pkt, avStream);
    CHECK_AND_RETURN_RET_LOG(samplePacket->pkt->size >= 0, AVCS_ERR_DEMUXER_FAILED,
        "the sample size is must be positive");
    if (avStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        frameSize = static_cast<uint64_t>(samplePacket->pkt->size);
    } else if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (avbsfContext_) {
            ConvertAvcOrHevcToAnnexb(*(samplePacket->pkt));
        }
        frameSize = static_cast<uint64_t>(samplePacket->pkt->size);
    }
    bufferInfo.offset = 0;
    auto copyFrameSize = static_cast<uint64_t>(frameSize) - samplePacket->offset;
    auto copySize = copyFrameSize;
    if (copySize > static_cast<uint64_t>(sample->GetSize())) {
        copySize = static_cast<uint64_t>(sample->GetSize());
    }
    bufferInfo.size = static_cast<int32_t>(copySize);
    errno_t rc = memcpy_s(sample->GetBase(), copySize, samplePacket->pkt->data+samplePacket->offset, copySize);
    CHECK_AND_RETURN_RET_LOG(rc == EOK, AVCS_ERR_UNKNOWN, "memcpy_s failed");
    if (copySize != copyFrameSize) {
        samplePacket->offset += copySize;
        return AVCS_ERR_NO_MEMORY;
    }
    av_packet_free(&(samplePacket->pkt));
    return AVCS_ERR_OK;
}

int32_t FFmpegDemuxerPlugin::GetNextPacket(uint32_t trackIndex, std::shared_ptr<SamplePacket> *samplePacket)
{
    int32_t ffmpegRet;
    AVPacket *pkt = nullptr;
    do {
        if (pkt == nullptr) {
            pkt = av_packet_alloc();
        }
        CHECK_AND_RETURN_RET_LOG(pkt != nullptr, AVCS_ERR_DEMUXER_FAILED, "failed to alloc packet");
        ffmpegRet = av_read_frame(formatContext_.get(), pkt);
        if (ffmpegRet < 0) {
            av_packet_free(&pkt);
            break;
        }
        if (pkt->stream_index < 0) {
            av_packet_free(&pkt);
            AVCODEC_LOGE("the stream_index must be positive");
            return AVCS_ERR_DEMUXER_FAILED;
        }
        uint32_t streamIndex = static_cast<uint32_t>(pkt->stream_index);
        if (IsInSelectedTrack(streamIndex)) {
            std::shared_ptr<SamplePacket> cacheSamplePacket = std::make_shared<SamplePacket>();
            cacheSamplePacket->offset = 0;
            cacheSamplePacket->pkt = pkt;
            if (streamIndex == trackIndex) {
                *samplePacket = cacheSamplePacket;
                break;
            }
            blockQueue_.Push(streamIndex, cacheSamplePacket);
            pkt = nullptr;
        } else {
            av_packet_unref(pkt);
        }
    } while (ffmpegRet >= 0);
    return ffmpegRet;
}

int32_t FFmpegDemuxerPlugin::ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
                                        AVCodecBufferInfo &info, AVCodecBufferFlag &flag)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (selectedTrackIds_.empty() || std::count(selectedTrackIds_.begin(), selectedTrackIds_.end(), trackIndex) == 0) {
        AVCODEC_LOGE("read frame failed, track %{public}u has not been selected", trackIndex);
        return AVCS_ERR_INVALID_OPERATION;
    }
    AVStream* avStream = formatContext_->streams[trackIndex];
    if (blockQueue_.HasCache(trackIndex)) {
        int32_t ret = ConvertAVPacketToSample(avStream, sample, info, flag, blockQueue_.Front(trackIndex));
        if (ret == AVCS_ERR_OK) {
            blockQueue_.Pop(trackIndex);
        }
        return ret;
    }
    if (trackIsEnd_.count(trackIndex) != 0 && trackIsEnd_[trackIndex]) {
        SetEosBufferInfo(info, flag);
        return AVCS_ERR_OK;
    }

    std::shared_ptr<SamplePacket> samplePacket = std::make_shared<SamplePacket>();
    int32_t ffmpegRet = GetNextPacket(trackIndex, &samplePacket);
    if (ffmpegRet == AVERROR_EOF) {
        SetEosBufferInfo(info, flag);
        return AVCS_ERR_OK;
    }
    if (ffmpegRet < 0) {
        AVCODEC_LOGE("read frame failed, ffmpeg error:%{public}d", ffmpegRet);
        return AVCS_ERR_DEMUXER_FAILED;
    }
    int32_t ret = ConvertAVPacketToSample(avStream, sample, info, flag, samplePacket);
    if (ret == AVCS_ERR_NO_MEMORY) {
        blockQueue_.Push(trackIndex, samplePacket);
    }
    return ret;
}

static int64_t CalculateTimeByFrameIndex(AVStream* avStream, int keyFrameIdx)
{
#if defined(LIBAVFORMAT_VERSION_INT) && defined(LIBAVFORMAT_VERSION_INT)
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

static int ConvertFlagsToFFmpeg(AVStream *avStream, int64_t ffTime, AVSeekMode mode)
{
    if (avStream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
        return AVSEEK_FLAG_BACKWARD;
    }
    if (mode == AVSeekMode::SEEK_MODE_NEXT_SYNC || mode == AVSeekMode::SEEK_MODE_PREVIOUS_SYNC) {
        int flags = g_seekModeToFFmpegSeekFlags.at(mode);
        return flags;
    }
    int keyFrameNext = av_index_search_timestamp(avStream, ffTime, AVSEEK_FLAG_FRAME);
    int keyFramePrev = av_index_search_timestamp(avStream, ffTime, AVSEEK_FLAG_BACKWARD);
    if (keyFrameNext < 0) {
        return AVSEEK_FLAG_BACKWARD;
    } else if (keyFramePrev < 0) {
        return AVSEEK_FLAG_FRAME;
    } else {
        int64_t ffTimePrev = CalculateTimeByFrameIndex(avStream, keyFramePrev);
        int64_t ffTimeNext = CalculateTimeByFrameIndex(avStream, keyFrameNext);
        if (ffTimePrev == ffTimeNext || (ffTimeNext - ffTime < ffTime - ffTimePrev)) {
            return AVSEEK_FLAG_FRAME;
        } else {
            return AVSEEK_FLAG_BACKWARD;
        }
    }
}

int32_t FFmpegDemuxerPlugin::SeekToTime(int64_t millisecond, AVSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!g_seekModeToFFmpegSeekFlags.count(mode)) {
        AVCODEC_LOGE("unsupported seek mode: %{public}d", static_cast<uint32_t>(mode));
        return AVCS_ERR_INVALID_OPERATION;
    }
    if (selectedTrackIds_.empty()) {
        AVCODEC_LOGW("no track has been selected");
        return AVCS_ERR_INVALID_OPERATION;
    }
    int trackIndex = static_cast<int>(selectedTrackIds_[0]);
    for (size_t i = 1; i < selectedTrackIds_.size(); i++) {
        int index = static_cast<int>(selectedTrackIds_[i]);
        if (formatContext_->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            trackIndex = index;
            break;
        }
    }
    auto avStream = formatContext_->streams[trackIndex];
    int64_t ffTime = ConvertTimeToFFmpeg(millisecond * 1000 * 1000, avStream->time_base);
    if (!CheckStartTime(avStream, ffTime)) {
        return AVCS_ERR_INVALID_OPERATION;
    }
    int flags = ConvertFlagsToFFmpeg(avStream, ffTime, mode);
    auto rtv = av_seek_frame(formatContext_.get(), trackIndex, ffTime, flags);
    if (rtv < 0) {
        AVCODEC_LOGE("seek failed, return value: ffmpeg error:%{public}d", rtv);
        return AVCS_ERR_SEEK_FAILED;
    }
    ResetStatus();
    return AVCS_ERR_OK;
}

void FFmpegDemuxerPlugin::ResetStatus()
{
    for (size_t i = 0; i < selectedTrackIds_.size(); ++i) {
        blockQueue_.RemoveTrackQueue(selectedTrackIds_[i]);
        blockQueue_.AddTrackQueue(selectedTrackIds_[i]);
        if (trackIsEnd_.count(selectedTrackIds_[i]) != 0) {
            trackIsEnd_[selectedTrackIds_[i]] = false;
        }
    }
}

void FFmpegDemuxerPlugin::SetEndStatus(uint32_t trackIndex)
{
    if (trackIsEnd_.count(trackIndex) != 0) {
        trackIsEnd_[trackIndex] = true;
    } else {
        trackIsEnd_.insert(std::make_pair(trackIndex, true));
    }
}

void FFmpegDemuxerPlugin::SetEosBufferInfo(AVCodecBufferInfo &bufferInfo, AVCodecBufferFlag &flag)
{
    bufferInfo.presentationTimeUs = 0;
    bufferInfo.size = 0;
    bufferInfo.offset = 0;
    flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS;
}
} // FFmpeg
} // Plugin
} // MediaAVCodec
} // OHOS
