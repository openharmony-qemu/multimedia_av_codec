/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "mpeg4_demuxer_plugin.h"
#include "mpeg4_sample_helper.h"
#include "mpeg4_utils.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "meta/video_types.h"
#include "avcodec_sysevent.h"
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4ReferenceParser" };
}

int64_t GetDuration(int64_t startTime)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - startTime;
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {

constexpr uint32_t REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT = 200000u;

bool MPEG4DemuxerPlugin::IsRefParserSupported()
{
    FileType fileType = FileType::UNKNOW;
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_FILE_TYPE>(fileType), false,
        "fileType does not exist in mediaInfo_");
    FALSE_RETURN_V_MSG_E(fileType == FileType::MP4 || fileType == FileType::MOV, false,
        "RefParser unsupported file type " PUBLIC_LOG_D32, fileType);
    FALSE_RETURN_V_MSG_E(!IsMultiVideoTracks(), false, "multi-video-tracks video is unsupported");
    FALSE_RETURN_V_MSG_W(IsSupportedCodec(), false, "RefParser unsupported stream type");
    return true;
}

Status MPEG4DemuxerPlugin::ParserRefUpdatePos(int64_t timeStampMs, bool isForward)
{
    FALSE_RETURN_V_MSG_E(IsRefParserSupported(), Status::ERROR_UNSUPPORTED_FORMAT, "Unsupport ref parser");
    int64_t clipTimeStampMs = std::max(timeStampMs, static_cast<int64_t>(0));
    if (syncSamplesPos_.size() == 0 || referenceParser_ == nullptr) {
        MEDIA_LOG_W("Parse failed, size: " PUBLIC_LOG_ZU, syncSamplesPos_.size());
        refParserPendingSeekMsTime_ = clipTimeStampMs;
        refParserIsForward_ = isForward;
        return Status::OK;
    }

    int32_t gopId = 0;
    FALSE_RETURN_V_MSG_E(GetGopIdBySeekPos(clipTimeStampMs, gopId) == Status::OK, Status::ERROR_UNKNOWN,
        "GetGopIdBySeekPos failed.");

    GopLayerInfo gopLayerInfo;
    Status ret = GetGopLayerInfo(gopId, gopLayerInfo);
    if (ret == Status::ERROR_AGAIN && gopId != refParserCurGopId_) {
        refParserPendingSeekMsTime_ = clipTimeStampMs;
        refParserState_ = false;
        MEDIA_LOG_I("Pending time: " PUBLIC_LOG_D64, refParserPendingSeekMsTime_);
    }
    refParserIsForward_ = isForward;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ParserRefInfo()
{
    if (!isInit_) {
        isInit_ = true;
        Status ret = RefParserInit();
        FALSE_RETURN_V_NOLOG(ret != Status::END_OF_STREAM, Status::OK);
        FALSE_RETURN_V(ret == Status::OK, Status::ERROR_UNKNOWN);
    }
    int64_t duration = GetDuration(refParserStartTime_);
    RefTrace trace("ParserRefCost_2_" + std::to_string(duration) + "_" + std::to_string(refParserCurGopId));
    FALSE_RETURN_V_MSG_W(duration < REFERENCE_PARSER_TIMEOUT_MS, Status::ERROR_UNKNOWN, "Reference parser timeout");
    FALSE_RETURN_V_MSG(refParserCurGopId != -1, Status::OK, "Reference parser end");  // 参考关系解析完毕
    FALSE_RETURN_V_MSG_E(SelectProGopId() == Status::OK, Status::ERROR_UNKNOWN, "Call selectProGopId failed");
    uint32_t curSyncSampleIndex = syncSamplesPos_[refParserCurGopId];
    MEDIA_LOG_I("curSyncSampleIndex: " PUBLIC_LOG_U32 ", parserCurGopId: " PUBLIC_LOG_D32 ", IFramePos size: "
        PUBLIC_LOG_ZU ", processingIFrame_ size: " PUBLIC_LOG_ZU ", duration: " PUBLIC_LOG_D64,
        curSyncSampleIndex, refParserCurGopId, syncSamplesPos_.size(), processingIFrame_.size(), duration);

    int loopCount = 0;
    while (parserState_ && refParserCurGopId != -1) {
        Status rlt = ParserRefInfoLoop(curSyncSampleIndex);
        if (rlt == Status::ERROR_EARLY_STOP) {  // START 首GOP提前终止
            MEDIA_LOG_I("Exiting reference parsing early due to simple first GOP structure.");
            av_packet_unref(pkt);
            av_packet_free(&pkt);
            parserState_ = true;
            return Status::OK;
        }
        if ((loopCount++ % 100) == 0) {  // 100 trace降采样
            duration = GetDuration(parserRefStartTime_);
            std::string suffix = "_" + std::to_string(refParserCurGopId) + "_" + std::to_string(curSyncSampleIndex);
            RefTrace traceInLoop("ParserRefCost_3_" + std::to_string(duration) + suffix);
            FALSE_RETURN_V_MSG_W(duration < REFERENCE_PARSER_TIMEOUT_MS, Status::ERROR_UNKNOWN, "Infoloop timeout");
        }
        if (rlt != Status::OK) {
            parserState_ = true;
            return rlt;
        }
        if (pkt->stream_index == parserRefIdx_) {
            curSyncSampleIndex++;
        }
    }
    parserState_ = true;
    return Status::ERROR_AGAIN;
}

Status MPEG4DemuxerPlugin::GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo)
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

Status MPEG4DemuxerPlugin::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_W(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "Reference is nullptr");
    MEDIA_LOG_D("In, dts: " PUBLIC_LOG_U32, frameId);
    return referenceParser_->GetFrameLayerInfo(frameId, frameLayerInfo);
}

Status MPEG4DemuxerPlugin::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    FALSE_RETURN_V_MSG_W(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "Reference is nullptr");
    MEDIA_LOG_D("In, gopId: " PUBLIC_LOG_U32, gopId);
    return referenceParser_->GetGopLayerInfo(gopId, gopLayerInfo);
}

Status MPEG4DemuxerPlugin::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    FALSE_RETURN_V_MSG_E(syncSamplesPos_.size() > 0, Status::ERROR_UNKNOWN, "IFramePos size is 0");
    IFramePos = syncSamplesPos_;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::Dts2FrameId(int64_t dts, uint32_t &sampleIndex)
{
    int64_t ffTimeDts = ConvertTimeToFFmpegByUs(dts, avStream->time_base);
    int32_t tmpFrameId = av_index_search_timestamp(avStream, ffTimeDts, AVSEEK_FLAG_ANY);
    FALSE_RETURN_V_MSG_E(tmpFrameId >= 0, Status::ERROR_OUT_OF_RANGE, "find nearest frame failed");
    frameId = static_cast<uint32_t>(tmpFrameId);
    MEDIA_LOG_D("dts " PUBLIC_LOG_D64 ", frameId " PUBLIC_LOG_U32, dts, frameId);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::SeekMs2FrameId(int64_t seekMs, uint32_t &frameId)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::FrameId2SeekMs(uint32_t frameId, int64_t &seekMs)
{
    return Status::OK;
}

bool MPEG4DemuxerPlugin::IsMultiVideoTracks()
{
    uint32_t videoTrackCount = 0;
    for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
        TrackType trackType = INVALID_TYPE;
        mediaInfo_.tracks[i].Get<Tag::MEDIA_TYPE>(trackType);
        std::string mimeType = MimeType::INVALID_TYPE;
        mediaInfo_.tracks[i].Get<Tag::MIME_TYPE>(mimeType);
        MEDIA_LOG_I("track index is " PUBLIC_LOG_U32 ", trackType is " PUBLIC_LOG_D32 ", mimeType is " PUBLIC_LOG_S, i,
            trackType, mimeType);
        if (trackType == TrackType::VIDEO_TYPE && StartWith(mimeType, "video/")) {
            refParserTrackIdx_ = (refParserTrackIdx_ == -1) ? i : refParserTrackIdx_;
            videoTrackCount++;
            if (videoTrackCount > 1) {
                return true;
            }
        }
    }
    return false;
}

bool MPEG4DemuxerPlugin::IsSupportedCodec()
{
    for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
        TrackType trackType = INVALID_TYPE;
        mediaInfo_.tracks[i].Get<Tag::MEDIA_TYPE>(trackType);
        if (trackType == TrackType::VIDEO_TYPE) {
            std::string mimeType = MimeType::INVALID_TYPE;
            mediaInfo_.tracks[i].Get<Tag::MIME_TYPE>(mimeType);
            if (mimeType == MimeType::VIDEO_AVC || mimeType == MimeType::VIDEO_HEVC) {
                return true;
            }
        }
    }
    return false;
}

Status MPEG4DemuxerPlugin::GetGopIdBySeekPos(int64_t seekMs, int32_t &gopId)
{
    FALSE_RETURN_V_MSG_E(seekMs >= 0, Status::ERROR_INVALID_PARAMETER, "SeekMs is less than 0");
    uint32_t sampleIdx = 0;
    FALSE_RETURN_V_MSG_E(GetSampleIndexByDts(seekMs, sampleIdx) == Status::OK, ret, "GetSampleIdByDts failed, seekMs is"
        PUBLIC_LOG_D64, seekMs);

    std::shared_ptr<MPEG4AtomParser::Track> track = FindTrackById(refParserTrackIdx_);
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, 0, "track is nullptr");
    syncSamplesPos_ = track->sampleHelper->GetSyncSamplesPosFromStssEntry();
    FALSE_RETURN_V_MSG_E(!syncSamplesPos_.empty(), ret, "syncSamplesPos_ is empty");

    gopId = std::upper_bound(syncSamplesPos_.begin(), syncSamplesPos_.end(), sampleIdx) - syncSamplesPos_.begin();
    MEDIA_LOG_I("gopId is " PUBLIC_LOG_D32, gopId);
    if (gopId > 0) {
        --gopId;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetSampleIndexBySeekMs(int64_t seekMs, uint32_t &sampleIndex)
{
    FALSE_RETURN_V_MSG_E(seekMs >= 0, Status::ERROR_INVALID_PARAMETER, "SeekMs is less than 0");
    int64_t pts = seekMs * 1000; // 1000 for ms to us
    auto iter = pts2Dts_.upper_bound(pts);
    if (iter != pts2Dts_.begin()) {
        --iter;
    }
    int64_t dts = iter->second + startPts_;
    MEDIA_LOG_D("seekMs " PUBLIC_LOG_D64 ", pts " PUBLIC_LOG_D64 ", mapSize " PUBLIC_LOG_ZU ", dts " PUBLIC_LOG_D64
        ", startPts " PUBLIC_LOG_D64, seekMs, pts, pts2DtsMap_.size(), dts, startPts_);
    return Dts2FrameId(dts, frameId);
}

Status MPEG4DemuxerPlugin::RefParserInit()
{
    refParserStartTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string suffix = std::to_string(refParserStartTime_) + "_" + std::to_string(syncSamplesPos_.size());
    RefTrace trace("ParserRefCost_1_" + suffix);
    MEDIA_LOG_I("Parser ref start time is " PUBLIC_LOG_D64, refParserStartTime_);
    FALSE_RETURN_V_MSG_E(syncSamplesPos_.size() > 0, Status::ERROR_UNKNOWN,
        "Init failed, syncSamplesPos_ size is" PUBLIC_LOG_ZU, syncSamplesPos_.size());
    FALSE_RETURN_V_MSG_E(IsRefParserSupported(), Status::ERROR_UNSUPPORTED_FORMAT, "Unsupported ref parser");
    /* todo 处理discard*/
    
    processingSyncSamplesPos_.assign(syncSamplesPos_.begin(), syncSamplesPos_.end());
    referenceParser_ = ReferenceParserManager::Create(codecType, IFramePos_);
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "referenceParser_ is nullptr");
    
    std::shared_ptr<MPEG4AtomParser::Track> track = FindTrackById(refParserTrackIdx_);
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, Status::ERROR_INVALID_DATA, "track is nullptr");
    SdtpEntry sdtpEntry = track->sampleHelper->GetSdtpEntry();
    if (sdtpEntry->sdtpEntryCount > 0 && !sdtpEntry->sdtpData.empty()) {
        if (referenceParser_->ParserSdtpData(sdtpEntry->sdtpEntryCount, sdtpEntry->sdtpData.data()) == Status::OK) {
            isSdtpExist_ = true;
            return Status::END_OF_STREAM;
        }
    }
    /* todo ParserExtraData */
    return referenceParser_->ParserExtraData(videoStream->codecpar->extradata, videoStream->codecpar->extradata_size);
}

Status MPEG4DemuxerPlugin::SelectProGopId()
{
    if (pendingSeekMsTime_ >= 0) {
        FALSE_RETURN_V_MSG_E(GetGopIdBySeekPos(pendingSeekMsTime_, refParserCurGopId_) == Status::OK,
            Status::ERROR_UNKNOWN, "GetGopIdBySeekPos failed");
        pendingSeekMsTime_ = -1;
    }
    int64_t seekPts;
    if (syncSampleIndex2SeekPts_.find(refParserCurGopId_) != syncSampleIndex2SeekPts_.end()) { // if I frame pts had got before decoding
        seekPts = syncSampleIndex2SeekPts_[refParserCurGopId_];
        MEDIA_LOG_D("get I frame pts from which had been decoded");
    } else {
        int32_t syncSampleNum = static_cast<int32_t>(syncSampleIndex2SeekPts_.size());
        /* todo */
        int64_t dtsCur = CalculateTimeByFrameIndex(st, syncSampleIndex2SeekPts_[refParserCurGopId_]);
        if (refParserCurGopId_ + 1 < syncSampleNum) {
            int64_t dtsNext = CalculateTimeByFrameIndex(st, syncSampleIndex2SeekPts_[refParserCurGopId_ + 1]);
            seekPts = dtsCur + (dtsNext - dtsCur) / 2; // 2 middle between cur gop and next gop
        } else {
            seekPts = INT64_MAX; // seek last gop
        }
        MEDIA_LOG_D("get I frame pts from simulated dts");
    }
    /* todo */
    auto ret = av_seek_frame(parserRefCtx_.get(), parserRefIdx_, seekPts, AVSEEK_FLAG_BACKWARD);
    FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_UNKNOWN,
                         "Call av_seek_frame failed, err: " PUBLIC_LOG_S, AVStrError(ret).c_str());
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ParserRefInfoLoop(uint32_t curSyncSampleIndex)
{
    std::unique_lock<std::mutex> sLock(syncMutex_);
    /* todo */
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
    FALSE_RETURN_V_MSG_E(result == Status::OK, result, "parse nal units end! ret=" PUBLIC_LOG_D32,
                         static_cast<int32_t>(result));
    int32_t iFramePosSize = static_cast<int32_t>(IFramePos_.size());
    if (ffmpegRet == AVERROR_EOF || result != Status::OK ||
        (parserCurGopId_ + 1 < iFramePosSize && curSampleId == IFramePos_[parserCurGopId_ + 1] - 1)) { // 处理完一个GOP
        MEDIA_LOG_D("IFramePos: " PUBLIC_LOG_ZU ", processingIFrame: " PUBLIC_LOG_ZU ", curSampleId: " PUBLIC_LOG_U32
            ", curGopId: " PUBLIC_LOG_U32, IFramePos_.size(), processingIFrame_.size(), curSampleId, parserCurGopId_);
        processingIFrame_.remove(IFramePos_[parserCurGopId_]);
        if (processingIFrame_.size() == 0) {
            parserCurGopId_ = -1;
            return Status::OK;
        }
        return UpdateParserGopId(iFramePosSize);
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
    const uint64_t relativePresentationTimeUs, uint32_t &index)
{
    MEDIA_LOG_I("GetIndexByRelativePresentationTimeUs, trackIndex is " PUBLIC_LOG_U32 ", time is " PUBLIC_LOG_U64
        ", index is " PUBLIC_LOG_U32, trackIndex, relativePresentationTimeUs, index);
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    uint32_t trackCount = static_cast<uint32_t>(CountTracks());
    FALSE_RETURN_V_MSG_E(trackIndex < trackCount, Status::ERROR_INVALID_DATA, "Track is out of range");

    uint32_t sampleCount = GetSampleCount(trackIndex);
    FALSE_RETURN_V_MSG_E(sampleCount <= REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT, Status::ERROR_INVALID_DATA,
        "Sample count exceeds limit");
    
    ResetParametersOfPTSAndIndexConversion();
    // Obtain the absolute pts of the first sample
    Status ret = GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode::GET_FIRST_SAMPLE_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN,
        "Failed to get the absolute pts of the first sample");
    int64_t absolutePTS = static_cast<int64_t>(relativePresentationTimeUs) + absolutePTSOfIndexZero_;

    ret = GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode::RELATIVEPTS_TO_INDEX, trackIndex,
        absolutePTS, index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Failed to get pts");

    FALSE_RETURN_V_MSG_E(absolutePTS >= minRelativePTS_ && absolutePTS <= maxRelativePTS_, Status::ERROR_INVALID_DATA,
        "Pts is out of range");
    if (minRelativePTSDiffOfLeft_ == 0 || minRelativePTSDiffOfRight_ == 0 ||
        minRelativePTSDiffOfLeft_ >= minRelativePTSDiffOfRight_) {
        index = index2RelativePTS_;
    } else if (minRelativePTSDiffOfLeft_ < minRelativePTSDiffOfRight_) {
        index = index2RelativePTS_ - 1;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
    const uint32_t index, uint64_t &relativePresentationTimeUs)
{
    MEDIA_LOG_I("GetRelativePresentationTimeUsByIndex, trackIndex is " PUBLIC_LOG_U32 ", time is " PUBLIC_LOG_U64
        ", index is " PUBLIC_LOG_U32, trackIndex, relativePresentationTimeUs, index);
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    uint32_t trackCount = static_cast<uint32_t>(CountTracks());
    FALSE_RETURN_V_MSG_E(trackIndex < trackCount, Status::ERROR_INVALID_DATA, "Track is out of range");
    
    FALSE_RETURN_V_MSG_E(index < UINT32_MAX, Status::ERROR_INVALID_DATA, "Index is out of range");
    uint32_t sampleCount = GetSampleCount(trackIndex);
    FALSE_RETURN_V_MSG_E(sampleCount <= REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT, Status::ERROR_INVALID_DATA,
        "Sample count exceeds limit");
    FALSE_RETURN_V_MSG_E(index + 1 <= sampleCount, Status::ERROR_INVALID_DATA, "Index is out of range");
    
    ResetParametersOfPTSAndIndexConversion();
    // Obtain the absolute pts of the first sample
    Status ret = GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode::GET_FIRST_SAMPLE_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN,
        "Failed to get the absolute pts of the first sample");

    ret = GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode::INDEX_TO_RELATIVEPTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Failed to get pts");
    
    int64_t relativePts = index2RelativePTSHeap_.top() - absolutePTSOfIndexZero_;
    FALSE_RETURN_V_MSG_E(relativePts >= 0, Status::ERROR_INVALID_DATA, "Calculation results less than 0");
    relativePresentationTimeUs = static_cast<uint64_t>(relativePts);

    return Status::OK;
}

uint32_t MPEG4DemuxerPlugin::GetSampleCount(uint32_t trackIndex)
{
    std::shared_ptr<MPEG4AtomParser::Track> track = FindTrackById(trackIndex);
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, 0, "track is nullptr");
    return track->sampleHelper->CalculateSampleCount();
}

void MPEG4DemuxerPlugin::ResetParametersOfPTSAndIndexConversion()
{
    absolutePTSOfIndexZero_ = INT64_MAX;
    index2RelativePTSHeap_ = std::priority_queue<int64_t>();
    minRelativePTS_ = INT64_MAX;
    maxRelativePTS_ = INT64_MIN;
    minRelativePTSDiffOfLeft_ = INT64_MAX;
    minRelativePTSDiffOfRight_ = INT64_MAX;
    index2RelativePTS_ = 0;
}

Status MPEG4DemuxerPlugin::GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode mode, uint32_t trackIndex,
    int64_t absolutePTS, uint32_t sampleIndex)
{
    std::shared_ptr<MPEG4AtomParser::Track> track = FindTrackById(trackIndex);
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, Status::ERROR_INVALID_DATA, "track is nullptr");

    uint32_t sttsEnrtyCount = track->sampleHelper->GetSttsEntryCount();
    FALSE_RETURN_V_MSG_E(sttsEnrtyCount != 0, Status::ERROR_INVALID_DATA, "sttsEnrtyCount is empty");
    uint32_t cttsEnrtyCount = track->sampleHelper->GetCttsEntryCount();

    return (cttsEnrtyCount == 0) ? ConvertPTSAndIndexOnlyByStts(track, mode, absolutePTS, sampleIndex) :
        ConvertPTSAndIndexBySttsAndCtts(track, mode, absolutePTS, sampleIndex);
}

Status MPEG4DemuxerPlugin::ConvertPTSAndIndexOnlyByStts(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    IndexAndPTSConversionMode mode, int64_t absolutePTS, uint32_t sampleIndex)
{
    MEDIA_LOG_I("absolutePTS is " PUBLIC_LOG_D64 ", sampleIndex is " PUBLIC_LOG_U32, absolutePTS, sampleIndex);
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, Status::ERROR_INVALID_DATA, "track or sampleHelper is nullptr");

    int32_t timeScale = 0;
    mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(timeScale);
    FALSE_RETURN_V_MSG_E(timeScale > 0, Status::ERROR_INVALID_DATA, "timescale is zero");
    double timeScaleRate = static_cast<double>(S_TO_US) / static_cast<double>(timeScale);

    uint32_t sttsEntryIndex = 0;
    uint32_t sttsEntryCount = track->sampleHelper->GetSttsEntryCount();
    uint32_t sampleCountOfCurrentSttsEntry = track->sampleHelper->GetSampleCountOfSttsEntry(sttsEntryIndex);
    int64_t pts = 0;
    int64_t dts = 0;

    while (sttsEntryIndex < sttsEntryCount) {
        FALSE_RETURN_V_MSG_E(timeScaleRate != 0 && static_cast<double>(INT64_MAX) / timeScaleRate >=
            static_cast<double>(dts), Status::ERROR_INVALID_DATA, "dts overflow");
        pts = static_cast<int64_t>(dts * timeScaleRate);
        ProcessConversionByMode(mode, pts, absolutePTS, sampleIndex, dts);

        if (sampleCountOfCurrentSttsEntry == 0) {
            break;
        }
        sampleCountOfCurrentSttsEntry--;
        int64_t sampleDeltaOfCurrentSttsEntry =
            static_cast<int64_t>(track->sampleHelper->GetSampleDeltaOfSttsEntry(sttsEntryIndex));
        FALSE_RETURN_V_MSG_E(INT64_MAX - dts >= sampleDeltaOfCurrentSttsEntry, Status::ERROR_INVALID_DATA,
            "dts overflow");

        dts += sampleDeltaOfCurrentSttsEntry;
        if (sampleCountOfCurrentSttsEntry == 0) {
            sttsEntryIndex++;
            sampleCountOfCurrentSttsEntry = sttsEntryIndex < sttsEntryCount ?
                track->sampleHelper->GetSampleCountOfSttsEntry(sttsEntryIndex) : 0;
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ConvertPTSAndIndexBySttsAndCtts(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    IndexAndPTSConversionMode mode, int64_t absolutePTS, uint32_t sampleIndex)
{
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, Status::ERROR_INVALID_DATA, "track or sampleHelper is nullptr");

    int32_t timeScale = 0;
    mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(timeScale);
    FALSE_RETURN_V_MSG_E(timeScale > 0, Status::ERROR_INVALID_DATA, "timescale is zero");
    double timeScaleRate = static_cast<double>(S_TO_US) / static_cast<double>(timeScale);

    uint32_t sttsEntryIndex = 0;
    uint32_t sttsEntryCount = track->sampleHelper->GetSttsEntryCount();
    uint32_t sampleCountOfCurrentSttsEntry = track->sampleHelper->GetSampleCountOfSttsEntry(sttsEntryIndex);
    uint32_t cttsEntryIndex = 0;
    uint32_t cttsEntryCount = track->sampleHelper->GetCttsEntryCount();
    uint32_t sampleCountOfCurrentCttsEntry = track->sampleHelper->GetSampleCountOfCttsEntry(cttsEntryIndex);
    // Avoid offset less than 0 in Ctts Box
    int32_t minSampleOffsetOfCttsEntry = track->sampleHelper->GetMinOffsetOfCttsEntry();

    int64_t pts = 0;
    int64_t dts = 0;

    while (sttsEntryIndex < sttsEntryCount && cttsEntryIndex < cttsEntryCount) {
        if (sampleCountOfCurrentCttsEntry == 0) {
            cttsEntryIndex++;
            if (cttsEntryIndex >= cttsEntryCount) {
                break;
            }
            sampleCountOfCurrentCttsEntry = track->sampleHelper->GetSampleCountOfCttsEntry(cttsEntryIndex);
            if (sampleCountOfCurrentCttsEntry == 0) {
                break;
            }
        }
        sampleCountOfCurrentCttsEntry--;
        
        int64_t sampleOffsetOfCurrentCttsEntry = static_cast<int64_t>(
            track->sampleHelper->GetSampleOffsetOfCttsEntry(cttsEntryIndex) - minSampleOffsetOfCttsEntry);
        FALSE_RETURN_V_MSG_E(UpdatePTSBySttsAndCtts(pts, dts, timeScaleRate, sampleOffsetOfCurrentCttsEntry) ==
            Status::OK, Status::ERROR_INVALID_DATA,  "dts overflow");
        ProcessConversionByMode(mode, pts, absolutePTS, sampleIndex, dts);
        if (sampleCountOfCurrentSttsEntry == 0) {
            break;
        }
        sampleCountOfCurrentSttsEntry--;
        int64_t sampleDeltaOfCurrentSttsEntry =
            static_cast<int64_t>(track->sampleHelper->GetSampleDeltaOfSttsEntry(sttsEntryIndex));
        FALSE_RETURN_V_MSG_E(INT64_MAX - dts >= sampleDeltaOfCurrentSttsEntry, Status::ERROR_INVALID_DATA,
            "dts overflow");
        dts += sampleDeltaOfCurrentSttsEntry;
        if (sampleCountOfCurrentSttsEntry == 0) {
            sttsEntryIndex++;
            sampleCountOfCurrentSttsEntry = sttsEntryIndex < sttsEntryCount ?
                track->sampleHelper->GetSampleCountOfSttsEntry(sttsEntryIndex) : 0;
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::UpdatePTSBySttsAndCtts(int64_t& pts, int64_t dts, double timeScaleRate,
    int64_t sampleOffsetOfCurrentCttsEntry)
{
    MEDIA_LOG_D("pts is " PUBLIC_LOG_D64 ", dts is " PUBLIC_LOG_D64 ", timeScaleRate is " PUBLIC_LOG_F
        ", sampleOffsetOfCurrentCttsEntry is " PUBLIC_LOG_D64, pts, dts, timeScaleRate, sampleOffsetOfCurrentCttsEntry);
    FALSE_RETURN_V_MSG_E(INT64_MAX - sampleOffsetOfCurrentCttsEntry >= dts, Status::ERROR_INVALID_DATA, "dts overflow");
    FALSE_RETURN_V_MSG_E(timeScaleRate != 0 && static_cast<double>(INT64_MAX) / timeScaleRate >=
        static_cast<double>(dts + sampleOffsetOfCurrentCttsEntry), Status::ERROR_INVALID_DATA, "dts overflow");
    pts = static_cast<int64_t>((dts + sampleOffsetOfCurrentCttsEntry) * timeScaleRate);
    return Status::OK;
}

void MPEG4DemuxerPlugin::ProcessConversionByMode(IndexAndPTSConversionMode mode, int64_t pts, int64_t absolutePTS,
    uint32_t sampleIndex, int64_t dts)
{
    switch (mode) {
        case IndexAndPTSConversionMode::GET_FIRST_SAMPLE_PTS:
            absolutePTSOfIndexZero_ = pts < absolutePTSOfIndexZero_ ? pts : absolutePTSOfIndexZero_;
            break;
        case IndexAndPTSConversionMode::INDEX_TO_RELATIVEPTS:
            ProcessIndex2RelativePTS(pts, sampleIndex);
            break;
        case IndexAndPTSConversionMode::RELATIVEPTS_TO_INDEX:
            ProcessRelativePTS2Index(pts, absolutePTS);
            break;
        case IndexAndPTSConversionMode::PTS_TO_DTS_MAPPING:
            ProcessPts2DtsMap(pts, dts);
            break;
        default:
            MEDIA_LOG_E("mode(" PUBLIC_LOG_U32 ") is invalid", mode);
            break;
    }
}

void MPEG4DemuxerPlugin::ProcessIndex2RelativePTS(int64_t pts, uint32_t index)
{
    if (index2RelativePTSHeap_.size() < index + 1) {
        index2RelativePTSHeap_.push(pts);
    } else if (pts < index2RelativePTSHeap_.top()) {
        index2RelativePTSHeap_.pop();
        index2RelativePTSHeap_.push(pts);
    }
}

void MPEG4DemuxerPlugin::ProcessRelativePTS2Index(int64_t pts, int64_t absolutePTS)
{
    minRelativePTS_ = minRelativePTS_ > pts ? pts : minRelativePTS_;
    maxRelativePTS_ = maxRelativePTS_ < pts ? pts : maxRelativePTS_;
    
    int64_t relativePTSDiff = abs(pts - absolutePTS);
    if (pts < absolutePTS && relativePTSDiff < minRelativePTSDiffOfLeft_) {
        minRelativePTSDiffOfLeft_ = relativePTSDiff;
    }
    if (pts >= absolutePTS && relativePTSDiff < minRelativePTSDiffOfRight_) {
        minRelativePTSDiffOfRight_ = relativePTSDiff;
    }
    if (pts < absolutePTS) {  // Record the first index greater than or equal to pts
        index2RelativePTS_++;
    }
}

void MPEG4DemuxerPlugin::ProcessPts2DtsMap(int64_t pts, int64_t dts)
{
    if (dts == 0) {
        ptsOfIndexZero_ = pts;
        MEDIA_LOG_I("ptsOfIndexZero_ is " PUBLIC_LOG_D64, ptsOfIndexZero_);
    }
    MEDIA_LOG_D("[pts" PUBLIC_LOG_D64 ", dts]" PUBLIC_LOG_D64, pts - ptsOfIndexZero_, dts - ptsOfIndexZero_);
    pts2Dts_.emplace(pts - ptsOfIndexZero_, dts - ptsOfIndexZero_);
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS