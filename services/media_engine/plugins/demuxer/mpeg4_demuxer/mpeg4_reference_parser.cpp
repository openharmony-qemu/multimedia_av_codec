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

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {

constexpr uint32_t REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT = 200000u;
constexpr int64_t THOUSAND = 1000;

bool MPEG4DemuxerPlugin::IsRefParserSupported()
{
    return true;
}

Status MPEG4DemuxerPlugin::ParserRefUpdatePos(int64_t timeStampMs, bool isForward)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ParserRefInfo()
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::Dts2FrameId(int64_t dts, uint32_t &frameId)
{
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
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, 0, "firstTrack_ is nullptr");
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
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, Status::ERROR_INVALID_DATA, "firstTrack_ is nullptr");

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
        ProcessConversionByMode(mode, pts, absolutePTS, sampleIndex);

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
        ProcessConversionByMode(mode, pts, absolutePTS, sampleIndex);
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
    uint32_t sampleIndex)
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
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS