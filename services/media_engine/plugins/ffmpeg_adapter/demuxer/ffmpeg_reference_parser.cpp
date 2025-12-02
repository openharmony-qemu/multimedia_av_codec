/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "FfmpegReferenceParser" };
constexpr int64_t REFERENCE_PARSER_TIMEOUT_MS = 10000;
const uint32_t REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT = 200000u;
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {

bool FFmpegDemuxerPlugin::IsLessMaxReferenceParserFrames(uint32_t trackIndex)
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, false, "AVFormatContext is nullptr");
    AVStream *avStream = formatContext_->streams[trackIndex];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, false, "Avstream is nullptr");
    FALSE_RETURN_V_MSG_E(avStream->stts_data != nullptr && avStream->stts_count != 0, false,
                         "AVStream->stts_data is empty");
    uint32_t frames = 0u;
    for (auto i = 0u; i < avStream->stts_count; i++) {
        FALSE_RETURN_V_MSG_E(frames <= UINT32_MAX - avStream->stts_data[i].count, false, "Frame count exceeds limit");
        frames += avStream->stts_data[i].count;
        FALSE_RETURN_V_MSG_E(frames <= REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT, false, "Frame count exceeds limit");
    }
    return true;
}

void FFmpegDemuxerPlugin::ParserBoxInfo()
{
    if (!IsRefParserSupported()) {
        return;
    }
    AVStream *videoStream = GetVideoStream();
    FALSE_RETURN_MSG(videoStream != nullptr, "Video stream is nullptr");
    struct KeyFrameNode *keyFramePosInfo = nullptr;
    if (av_get_key_frame_pos_from_stream(videoStream, &keyFramePosInfo) == 0) {
        struct KeyFrameNode *cur = keyFramePosInfo;
        while (cur != nullptr) {
            IFramePos_.emplace_back(cur->pos);
            cur = cur->next;
        }
        av_destory_key_frame_pos_list(keyFramePosInfo);
    }
    startPts_ = AvTime2Us(ConvertTimeFromFFmpeg(videoStream->start_time, videoStream->time_base));
    FALSE_RETURN_MSG(GetPresentationTimeUsFromFfmpegMOV(
        GET_ALL_FRAME_PTS, parserRefIdx_, 0, 0) == Status::OK, "get all frame pts failed.");
    MEDIA_LOG_I("Success parse, start pts:" PUBLIC_LOG_D64 ", IFramePos size: " PUBLIC_LOG_ZU,
        startPts_, IFramePos_.size());
}

AVStream *FFmpegDemuxerPlugin::GetVideoStream()
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, nullptr, "AVFormatContext is nullptr");
    if (parserRefIdx_ < 0 || parserRefIdx_ >= static_cast<int32_t>(formatContext_->nb_streams)) {
        int32_t streamIdx = av_find_best_stream(formatContext_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        FALSE_RETURN_V_MSG_E(streamIdx >= 0 && streamIdx < static_cast<int32_t>(formatContext_->nb_streams), nullptr,
            "Can not find video stream, streamIdx " PUBLIC_LOG_D32 ", nb_streams " PUBLIC_LOG_U32,
            streamIdx, formatContext_->nb_streams);
        parserRefIdx_ = streamIdx;
    }
    return formatContext_->streams[static_cast<uint32_t>(parserRefIdx_)];
}

bool FFmpegDemuxerPlugin::IsMultiVideoTrack()
{
    bool hasVideo = false;
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        auto avStream = formatContext_->streams[trackIndex];
        if (avStream == nullptr || avStream->codecpar == nullptr) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_D32 " info is nullptr", trackIndex);
            continue;
        }
        if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
            FFmpegFormatHelper::IsVideoCodecId(avStream->codecpar->codec_id)) {
            if (hasVideo) {
                return true;
            }
            hasVideo = true;
        }
    }
    return false;
}

bool FFmpegDemuxerPlugin::IsRefParserSupported()
{
    std::shared_ptr<AVFormatContext> formatContext = parserRefCtx_ != nullptr ?
        parserRefCtx_ : formatContext_;
    FALSE_RETURN_V_MSG_E(formatContext != nullptr, false, "AVFormatContext is nullptr");
    FileType type = FFmpegFormatHelper::GetFileTypeByName(*formatContext);
    FALSE_RETURN_V_MSG_W(type == FileType::MP4 || type == FileType::MOV, false,
        "RefParser unsupported file type " PUBLIC_LOG_U32, type);
    FALSE_RETURN_V_MSG_W(!IsMultiVideoTrack(), false, "multi-video-tracks video is unsupported!");
    FALSE_RETURN_V_MSG_W(ParserRefCheckVideoValid(GetVideoStream()) == Status::OK, false,
        "RefParser unsupported stream type");
    return true;
}

Status FFmpegDemuxerPlugin::ParserRefUpdatePos(int64_t timeStampMs, bool isForward)
{
    FALSE_RETURN_V_MSG_E(IsRefParserSupported(), Status::ERROR_UNSUPPORTED_FORMAT, "Unsupported ref parser");
    int64_t clipTimeStampMs = std::max(timeStampMs, static_cast<int64_t>(0));
    if (IFramePos_.size() == 0 || referenceParser_ == nullptr) {
        MEDIA_LOG_W("Parse failed, size: " PUBLIC_LOG_ZU, IFramePos_.size());
        pendingSeekMsTime_ = clipTimeStampMs;
        updatePosIsForward_ = isForward;
        return Status::OK;
    }
    int32_t gopId = 0;
    FALSE_RETURN_V_MSG_E(GetGopIdFromSeekPos(clipTimeStampMs, gopId) == Status::OK,
        Status::ERROR_UNKNOWN, "GetGopIdFromSeekPos failed.");
    GopLayerInfo gopLayerInfo;
    Status ret = GetGopLayerInfo(gopId, gopLayerInfo);
    if (ret == Status::ERROR_AGAIN && gopId != parserCurGopId_) {
        pendingSeekMsTime_ = clipTimeStampMs;
        parserState_ = false;
        MEDIA_LOG_I("Pending time: " PUBLIC_LOG_D64, pendingSeekMsTime_);
    }
    updatePosIsForward_ = isForward;
    return Status::OK;
}

Status FFmpegDemuxerPlugin::InitIoContext()
{
    parserRefIoContext_.dataSource = ioContext_.dataSource;
    parserRefIoContext_.offset = 0;
    parserRefIoContext_.eos = false;
    parserRefIoContext_.initCompleted = true;
    FALSE_RETURN_V_MSG_E(parserRefIoContext_.dataSource != nullptr, Status::ERROR_UNKNOWN, "Data source is nullptr");
    if (parserRefIoContext_.dataSource->GetSeekable() == Plugins::Seekable::SEEKABLE) {
        parserRefIoContext_.dataSource->GetSize(parserRefIoContext_.fileSize);
    } else {
        parserRefIoContext_.fileSize = -1;
        MEDIA_LOG_E("Not support online video");
        return Status::ERROR_INVALID_OPERATION;
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ParserRefCheckVideoValid(const AVStream *videoStream)
{
    if (videoStream == nullptr || videoStream->codecpar == nullptr) {
        MEDIA_LOG_D("videoStream or codecpar is nullptr: video track id " PUBLIC_LOG_D32, parserRefIdx_);
        return Status::ERROR_UNKNOWN;
    }
    FALSE_RETURN_V_MSG_E(
        videoStream->codecpar->codec_id == AV_CODEC_ID_HEVC || videoStream->codecpar->codec_id == AV_CODEC_ID_H264,
        Status::ERROR_UNSUPPORTED_FORMAT, "Codec type not support " PUBLIC_LOG_D32, videoStream->codecpar->codec_id);
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ParserRefInit()
{
    parserRefStartTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string suffix = std::to_string(parserRefStartTime_) + "_" + std::to_string(IFramePos_.size());
    MediaAVCodec::AVCodecTrace trace("ParserRefCost_1_" + suffix);
    MEDIA_LOG_I("Parser ref start time: " PUBLIC_LOG_D64, parserRefStartTime_);
    FALSE_RETURN_V_MSG_E(IFramePos_.size() > 0, Status::ERROR_UNKNOWN,
        "Init failed, IFramePos size:" PUBLIC_LOG_ZU, IFramePos_.size());
    FALSE_RETURN_V_MSG_E(InitIoContext() == Status::OK, Status::ERROR_UNKNOWN, "Init IOContext failed");
    parserRefCtx_ = InitAVFormatContext(&parserRefIoContext_);
    FALSE_RETURN_V_MSG_E(parserRefCtx_ != nullptr, Status::ERROR_UNKNOWN, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(IsRefParserSupported(), Status::ERROR_UNSUPPORTED_FORMAT, "Unsupported ref parser");
    for (uint32_t trackIndex = 0; trackIndex < parserRefCtx_->nb_streams; trackIndex++) {
        AVStream *stream = parserRefCtx_->streams[trackIndex];
        FALSE_RETURN_V_MSG_E(stream != nullptr && stream->codecpar != nullptr, Status::ERROR_UNKNOWN,
            "AVStream or codecpar is nullptr, track " PUBLIC_LOG_U32, trackIndex);
        if (stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            stream->discard = AVDISCARD_ALL;
        }
    }
    AVStream *videoStream = GetVideoStream();
    FALSE_RETURN_V_MSG_E(videoStream != nullptr && videoStream->codecpar != nullptr, Status::ERROR_UNKNOWN,
        "AVStream or codecpar is nullptr");
    processingIFrame_.assign(IFramePos_.begin(), IFramePos_.end());
    CodecType codecType = videoStream->codecpar->codec_id == AV_CODEC_ID_HEVC ? CodecType::H265 : CodecType::H264;
    referenceParser_ = ReferenceParserManager::Create(codecType, IFramePos_);
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "Reference is nullptr");
    ParserSdtpInfo *sc = (ParserSdtpInfo *)videoStream->priv_data;
    if (sc->sdtpCount > 0 && sc->sdtpData != nullptr) {
        MEDIA_LOG_E("Sdtp exist: " PUBLIC_LOG_D32, sc->sdtpCount);
        if (referenceParser_->ParserSdtpData(sc->sdtpData, sc->sdtpCount) == Status::OK) {
            isSdtpExist_ = true;
            return Status::END_OF_STREAM;
        }
    }
    return referenceParser_->ParserExtraData(videoStream->codecpar->extradata, videoStream->codecpar->extradata_size);
}

static void InsertIframePtsMap(AVPacket *pkt, int32_t gopId, int32_t trackIdx,
    std::unordered_map<int32_t, int64_t> &iFramePtsMap)
{
    bool validCheck = (pkt != nullptr) && (pkt->stream_index == trackIdx) &&
        (pkt->flags == AV_PKT_FLAG_KEY) && (gopId != -1); // -1 disable
    if (validCheck && (iFramePtsMap.find(gopId) == iFramePtsMap.end())) {
        iFramePtsMap.insert(std::pair<int32_t, int64_t>(gopId, pkt->pts));
    }
}

Status FFmpegDemuxerPlugin::ParserRefInfoLoop(AVPacket *pkt, uint32_t curStreamId)
{
    std::unique_lock<std::mutex> sLock(syncMutex_);
    FALSE_RETURN_V_MSG_E(pkt != nullptr, Status::ERROR_UNKNOWN, "Packet is nullptr!");
    int ffmpegRet = av_read_frame(parserRefCtx_.get(), pkt);
    sLock.unlock();
    if (ffmpegRet < 0 && ffmpegRet != AVERROR_EOF) {
        MEDIA_LOG_E("Call av_read_frame failed:" PUBLIC_LOG_S ", retry:" PUBLIC_LOG_D32,
                    AVStrError(ffmpegRet).c_str(), int(parserRefIoContext_.retry));
        if (parserRefIoContext_.retry) {
            parserRefCtx_->pb->eof_reached = 0;
            parserRefCtx_->pb->error = 0;
            parserRefIoContext_.retry = false;
            return Status::ERROR_AGAIN;
        }
        return Status::ERROR_UNKNOWN;
    }
    InsertIframePtsMap(pkt, parserCurGopId_, parserRefIdx_, iFramePtsMap_);
    FALSE_RETURN_V_MSG_D(pkt->stream_index == parserRefIdx_ || ffmpegRet == AVERROR_EOF, Status::OK,
                         "eos or not video");
    int64_t dts = AvTime2Us(ConvertTimeFromFFmpeg(pkt->dts, parserRefCtx_->streams[parserRefIdx_]->time_base));
    Status result = referenceParser_->ParserNalUnits(pkt->data, pkt->size, curStreamId, dts);
    FALSE_RETURN_V_MSG_E(result == Status::OK, Status::ERROR_UNKNOWN, "parse nal units error!");
    int32_t iFramePosSize = static_cast<int32_t>(IFramePos_.size());
    if (ffmpegRet == AVERROR_EOF || result != Status::OK ||
        (parserCurGopId_ + 1 < iFramePosSize && curStreamId == IFramePos_[parserCurGopId_ + 1] - 1)) { // 处理完一个GOP
        MEDIA_LOG_D("IFramePos: " PUBLIC_LOG_ZU ", processingIFrame: " PUBLIC_LOG_ZU ", curStreamId: " PUBLIC_LOG_U32
            ", curGopId: " PUBLIC_LOG_U32, IFramePos_.size(), processingIFrame_.size(), curStreamId, parserCurGopId_);
        processingIFrame_.remove(IFramePos_[parserCurGopId_]);
        if (processingIFrame_.size() == 0) {
            parserCurGopId_ = -1;
            return Status::OK;
        }
        int32_t tmpGopId = parserCurGopId_;
        int32_t searchCnt = 0;
        while (formatContext_ != nullptr && std::find(processingIFrame_.begin(), processingIFrame_.end(),
                                                      IFramePos_[parserCurGopId_]) == processingIFrame_.end()) {
            if (updatePosIsForward_) {
                parserCurGopId_ = (parserCurGopId_ + 1) % iFramePosSize;
            } else {
                parserCurGopId_ = parserCurGopId_ == 0 ? iFramePosSize - 1 : parserCurGopId_ - 1;
            }
            searchCnt++;
            FALSE_RETURN_V_MSG_E(searchCnt < iFramePosSize, Status::ERROR_UNKNOWN, "Cannot find gop");
        }
        if (formatContext_ == nullptr || tmpGopId + 1 != parserCurGopId_ || !updatePosIsForward_) {
            return Status::END_OF_STREAM;
        }
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::GetGopIdFromSeekPos(int64_t seekMs, int32_t &gopId)
{
    FALSE_RETURN_V_MSG_E(seekMs >= 0L, Status::ERROR_INVALID_PARAMETER, "SeekMs is less than 0");
    AVStream *st = parserRefCtx_->streams[parserRefIdx_];
    FALSE_RETURN_V_MSG_E(st != nullptr, Status::ERROR_UNKNOWN, "AVStream is nullptr");
    uint32_t frameId = 0U;
    Status ret = SeekMs2FrameId(seekMs, frameId);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "SeekMs2FrameId failed, seekMs=" PUBLIC_LOG_D64, seekMs);
    gopId = std::upper_bound(IFramePos_.begin(), IFramePos_.end(), frameId) - IFramePos_.begin();
    if (gopId > 0) {
        --gopId;
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::SelectProGopId()
{
    AVStream *st = parserRefCtx_->streams[parserRefIdx_];
    FALSE_RETURN_V_MSG_E(st != nullptr, Status::ERROR_UNKNOWN, "AVStream is nullptr");
    if (pendingSeekMsTime_ >= 0) {
        FALSE_RETURN_V_MSG_E(GetGopIdFromSeekPos(pendingSeekMsTime_, parserCurGopId_) == Status::OK,
            Status::ERROR_UNKNOWN, "GetGopIdFromSeekPos failed");
        pendingSeekMsTime_ = -1;
    }
    int64_t ptsSeek;
    if (iFramePtsMap_.find(parserCurGopId_) != iFramePtsMap_.end()) { // if I frame pts had got before decoding
        ptsSeek = iFramePtsMap_[parserCurGopId_];
        MEDIA_LOG_D("get I frame pts from which had been decoded");
    } else {
        int32_t iFramePosSize = static_cast<int32_t>(IFramePos_.size());
        int64_t dtsCur = CalculateTimeByFrameIndex(st, IFramePos_[parserCurGopId_]);
        if (parserCurGopId_ + 1 < iFramePosSize) {
            int64_t dtsNext = CalculateTimeByFrameIndex(st, IFramePos_[parserCurGopId_ + 1]);
            ptsSeek = dtsCur + (dtsNext - dtsCur) / 2; // 2 middle between cur gop and next gop
        } else {
            ptsSeek = INT64_MAX; // seek last gop
        }
        MEDIA_LOG_D("get I frame pts from simulated dts");
    }
    auto ret = av_seek_frame(parserRefCtx_.get(), parserRefIdx_, ptsSeek, AVSEEK_FLAG_BACKWARD);
    FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_UNKNOWN,
                         "Call av_seek_frame failed, err: " PUBLIC_LOG_S, AVStrError(ret).c_str());
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ParserRefInfo()
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::OK, "AVFormatContext is nullptr");
    if (!isInit_) {
        isInit_ = true;
        Status ret = ParserRefInit();
        if (ret == Status::END_OF_STREAM) {
            return Status::OK;
        }
        FALSE_RETURN_V(ret == Status::OK, Status::ERROR_UNKNOWN);
    }
    int64_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - parserRefStartTime_;
    std::string suffix = std::to_string(duration) + "_" + std::to_string(parserCurGopId_);
    MediaAVCodec::AVCodecTrace trace("ParserRefCost_2_" + suffix);
    FALSE_RETURN_V_MSG_W(duration < REFERENCE_PARSER_TIMEOUT_MS, Status::ERROR_UNKNOWN, "Reference parser timeout");
    FALSE_RETURN_V_MSG(parserCurGopId_ != -1, Status::OK, "Reference parser end"); // 参考关系解析完毕
    FALSE_RETURN_V_MSG_E(SelectProGopId() == Status::OK, Status::ERROR_UNKNOWN, "Call selectProGopId failed");
    uint32_t curStreamId = IFramePos_[parserCurGopId_];
    MEDIA_LOG_I("curStreamId: " PUBLIC_LOG_U32 ", parserCurGopId: " PUBLIC_LOG_D32 ", IFramePos size: " PUBLIC_LOG_ZU
        ", processingIFrame_ size: " PUBLIC_LOG_ZU ", duration: " PUBLIC_LOG_D64, curStreamId, parserCurGopId_,
        IFramePos_.size(), processingIFrame_.size(), duration);
    AVPacket *pkt = av_packet_alloc();
    FALSE_RETURN_V_MSG_E(pkt != nullptr, Status::ERROR_UNKNOWN, "Packet is nullptr!");
    while (formatContext_ != nullptr && parserState_ && parserCurGopId_ != -1) {
        Status rlt = ParserRefInfoLoop(pkt, curStreamId);
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() - parserRefStartTime_;
        suffix = std::to_string(duration) + "_" + std::to_string(parserCurGopId_) + "_" + std::to_string(curStreamId);
        MediaAVCodec::AVCodecTrace traceInLoop("ParserRefCost_3_" + suffix);
        FALSE_RETURN_V_MSG_W(duration < REFERENCE_PARSER_TIMEOUT_MS, Status::ERROR_UNKNOWN, "Reference parser timeout");
        if (rlt != Status::OK) {
            av_packet_unref(pkt);
            av_packet_free(&pkt);
            parserState_ = true;
            return rlt;
        }
        if (pkt->stream_index == parserRefIdx_) {
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
    FALSE_RETURN_V_MSG_W(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "Reference is nullptr");
    MEDIA_LOG_D("In, dts: " PUBLIC_LOG_D64, videoSample->dts_);
    if (isSdtpExist_) {
        uint32_t frameId = 0;
        Status ret = Dts2FrameId(videoSample->dts_, frameId);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Dts2FrameId failed");
        return referenceParser_->GetFrameLayerInfo(frameId, frameLayerInfo);
    }
    return referenceParser_->GetFrameLayerInfo(videoSample->dts_, frameLayerInfo);
}

Status FFmpegDemuxerPlugin::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_W(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "Reference is nullptr");
    MEDIA_LOG_D("In, dts: " PUBLIC_LOG_U32, frameId);
    return referenceParser_->GetFrameLayerInfo(frameId, frameLayerInfo);
}

Status FFmpegDemuxerPlugin::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    FALSE_RETURN_V_MSG_W(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "Reference is nullptr");
    MEDIA_LOG_D("In, gopId: " PUBLIC_LOG_U32, gopId);
    return referenceParser_->GetGopLayerInfo(gopId, gopLayerInfo);
}

Status FFmpegDemuxerPlugin::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    FALSE_RETURN_V_MSG_E(IFramePos_.size() > 0, Status::ERROR_UNKNOWN, "IFramePos size is 0");
    IFramePos = IFramePos_;
    return Status::OK;
}

Status FFmpegDemuxerPlugin::Dts2FrameId(int64_t dts, uint32_t &frameId)
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "formatContext_ is nullptr");
    auto avStream = formatContext_->streams[parserRefIdx_];
    int64_t ffTimeDts = ConvertTimeToFFmpegByUs(dts, avStream->time_base);
    int32_t tmpFrameId = av_index_search_timestamp(avStream, ffTimeDts, AVSEEK_FLAG_ANY);
    FALSE_RETURN_V_MSG_E(tmpFrameId >= 0, Status::ERROR_OUT_OF_RANGE, "find nearest frame failed");
    frameId = static_cast<uint32_t>(tmpFrameId);
    MEDIA_LOG_D("dts " PUBLIC_LOG_D64 ", frameId " PUBLIC_LOG_U32, dts, frameId);
    return Status::OK;
}

Status FFmpegDemuxerPlugin::SeekMs2FrameId(int64_t seekMs, uint32_t &frameId)
{
    FALSE_RETURN_V_MSG_E(seekMs >= 0L, Status::ERROR_INVALID_PARAMETER, "SeekMs is less than 0");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "formatContext_ is nullptr");
    auto avStream = formatContext_->streams[parserRefIdx_];
    FALSE_RETURN_V_MSG_E(avStream != nullptr, Status::ERROR_NULL_POINTER, "AVStream is nullptr");
    FALSE_RETURN_V_MSG_E(avStream->stts_data != nullptr && avStream->stts_count != 0,
        Status::ERROR_NULL_POINTER, "AVStream->stts_data is empty");
    int64_t pts = seekMs * 1000; // 1000 for ms to us
    auto iter = pts2DtsMap_.upper_bound(pts);
    if (iter != pts2DtsMap_.begin()) {
        --iter;
    }
    int64_t dts = iter->second + startPts_;
    MEDIA_LOG_D("seekMs " PUBLIC_LOG_D64 ", pts " PUBLIC_LOG_D64 ", mapSize " PUBLIC_LOG_ZU ", dts " PUBLIC_LOG_D64
        ", startPts " PUBLIC_LOG_D64, seekMs, pts, pts2DtsMap_.size(), dts, startPts_);
    return Dts2FrameId(dts, frameId);
}

Status FFmpegDemuxerPlugin::FrameId2SeekMs(uint32_t frameId, int64_t &seekMs)
{
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "formatContext_ is nullptr");
    AVStream *st = formatContext_->streams[parserRefIdx_];
    FALSE_RETURN_V_MSG_E(st != nullptr, Status::ERROR_NULL_POINTER, "AVStream is nullptr");
    int64_t ffTimeDts = CalculateTimeByFrameIndex(st, static_cast<int32_t>(frameId));
    int64_t dts = AvTime2Us(ConvertTimeFromFFmpeg(ffTimeDts, st->time_base)) - startPts_;
    auto it = std::find_if(pts2DtsMap_.begin(), pts2DtsMap_.end(), DtsFinder(dts));
    FALSE_RETURN_V_MSG_E(it != pts2DtsMap_.end(), Status::ERROR_UNKNOWN, "find pts failed, frameId "
        PUBLIC_LOG_U32 ", dts " PUBLIC_LOG_D64, frameId, dts);
    seekMs = it->first / 1000; // 1000 for ms to us
    MEDIA_LOG_D("frameId " PUBLIC_LOG_U32 ", dts " PUBLIC_LOG_D64 ", seekMs " PUBLIC_LOG_D64 ", startPts "
        PUBLIC_LOG_D64, frameId, dts, seekMs, startPts_);
    return Status::OK;
}

} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
