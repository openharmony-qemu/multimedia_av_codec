/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "MediaDemuxer"
#define MEDIA_ATOMIC_ABILITY

#include "media_demuxer.h"

#include <algorithm>
#include <memory>
#include <map>

#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "cpp_ext/type_traits_ext.h"
#include "buffer/avallocator.h"
#include "common/event.h"
#include "common/log.h"
#include "hisysevent.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_buffer.h"
#include "source/source.h"
#include "stream_demuxer.h"
#include "media_core.h"
#include "osal/utils/dump_buffer.h"
#include "demuxer_plugin_manager.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MediaDemuxer" };
} // namespace

namespace OHOS {
namespace Media {
constexpr int64_t MAX_PTS_DIFFER_THRESHOLD_US = 10000000; // The maximum difference between Segment 10s.
constexpr int64_t INVALID_PTS_DATA = -1; // The invalid pts data -1.
constexpr int64_t PTS_MICRO_ADJUSTMENT_US = 1000;

void MediaDemuxer::HandleAutoMaintainPts(int32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    if (!isAutoMaintainPts_) {
        return;
    }
    int64_t curPacketPts = sample->pts_;
    std::shared_ptr<MaintainBaseInfo> baseInfo = maintainBaseInfos_[trackId];
    if (baseInfo == nullptr) {
        MEDIA_LOG_E("BaseInfo is nullptr, track " PUBLIC_LOG_D32, trackId);
        return;
    }
    int64_t diff = 0;
    diff = curPacketPts - baseInfo->lastPts;
    auto oldPts =  baseInfo->lastPts;
    baseInfo->lastPts = curPacketPts;
    if (diff < 0) {
        diff = 0 - diff;
    }

    if (baseInfo->segmentOffset == INVALID_PTS_DATA) {
        int64_t offset = static_cast<int64_t>(source_->GetSegmentOffset());
        if (baseInfo->segmentOffset != offset) {
            baseInfo->segmentOffset = offset;
            baseInfo->basePts = curPacketPts;
        }
        sample->pts_ = baseInfo->segmentOffset + curPacketPts - baseInfo->basePts + mediaStartPts_;
    } else if (diff > MAX_PTS_DIFFER_THRESHOLD_US) {
        if (baseInfo->isLastPtsChange) {
            int64_t offset = static_cast<int64_t>(source_->GetSegmentOffset());
            if (baseInfo->segmentOffset != offset) {
                baseInfo->segmentOffset = offset;
                baseInfo->basePts = baseInfo->candidateBasePts;
            }
            sample->pts_ = baseInfo->segmentOffset + curPacketPts - baseInfo->basePts + mediaStartPts_;
            baseInfo->isLastPtsChange = false;
        } else {
            sample->pts_ = baseInfo->lastPtsModifyedMax + PTS_MICRO_ADJUSTMENT_US;
            baseInfo->candidateBasePts = curPacketPts;
            baseInfo->isLastPtsChange = true;
            baseInfo->lastPts = oldPts;
        }
    } else {
        sample->pts_ = baseInfo->segmentOffset + curPacketPts - baseInfo->basePts + mediaStartPts_;
        baseInfo->isLastPtsChange = false;
    }
    baseInfo->lastPtsModifyedMax = std::max(sample->pts_, baseInfo->lastPtsModifyedMax);
    
    MEDIA_LOG_I("Success, track:" PUBLIC_LOG_D32 ", orgPts:"
        PUBLIC_LOG_D64 ", pts:" PUBLIC_LOG_D64 ", basePts: " PUBLIC_LOG_D64, trackId,
        curPacketPts, sample->pts_, baseInfo->basePts);
}

void MediaDemuxer::InitPtsInfo()
{
    if (source_ == nullptr || !source_->GetHLSDiscontinuity()) {
        return;
    }
    MEDIA_LOG_I("Enable hls disContinuity auto maintain pts");
    isAutoMaintainPts_ = true;
    AutoLock lock(mapMutex_);
    for (auto it = bufferQueueMap_.begin(); it != bufferQueueMap_.end(); it++) {
        int32_t trackId = it->first;
        if (maintainBaseInfos_[trackId] == nullptr) {
            maintainBaseInfos_[trackId] = std::make_shared<MaintainBaseInfo>();
        }
        maintainBaseInfos_[trackId]->segmentOffset = INVALID_PTS_DATA;
        maintainBaseInfos_[trackId]->basePts = INVALID_PTS_DATA;
        maintainBaseInfos_[trackId]->isLastPtsChange = false;
        maintainBaseInfos_[trackId]->lastPtsModifyedMax = INVALID_PTS_DATA;
    }
}

void MediaDemuxer::InitMediaStartPts()
{
    std::string mime;
    int64_t startTime = 0;
    for (const auto& trackInfo : mediaMetaData_.trackMetas) {
        if (trackInfo == nullptr || !(trackInfo->GetData(Tag::MIME_TYPE, mime))) {
            MEDIA_LOG_W("TrackInfo is null or get mime fail");
            continue;
        }
        if (!(mime.find("audio/") == 0 || mime.find("video/") == 0)) {
            continue;
        }
        if (trackInfo->GetData(Tag::MEDIA_START_TIME, startTime) &&
            (mediaStartPts_ == HST_TIME_NONE || startTime < mediaStartPts_)) {
                mediaStartPts_ = startTime;
        }
    }
}

void MediaDemuxer::TranscoderInitMediaStartPts()
{
    // Init media start time based on the first video track and the first audio track
    std::string mime;
    int64_t startTime = 0;
    bool isInitVideoStartTime = false;
    bool isInitAudioStartTime = false;
    for (const auto& trackInfo : mediaMetaData_.trackMetas) {
        if (trackInfo == nullptr || !(trackInfo->GetData(Tag::MIME_TYPE, mime))) {
            MEDIA_LOG_W("TrackInfo is null or get mime fail");
            continue;
        }
        if (!isInitVideoStartTime && (mime.find("video/") == 0)) {
            isInitVideoStartTime = true;
            FALSE_RETURN(trackInfo->GetData(Tag::MEDIA_START_TIME, startTime));
            if (transcoderStartPts_ == HST_TIME_NONE || startTime < transcoderStartPts_) {
                transcoderStartPts_ = startTime;
            }
        } else if (!isInitAudioStartTime && (mime.find("audio/") == 0)) {
            isInitAudioStartTime = true;
            FALSE_RETURN(trackInfo->GetData(Tag::MEDIA_START_TIME, startTime));
            if (transcoderStartPts_ == HST_TIME_NONE || startTime < transcoderStartPts_) {
                transcoderStartPts_ = startTime;
            }
        }
        if (isInitAudioStartTime && isInitVideoStartTime) {
            break;
        }
    }
}
} // namespace Media
} // namespace OHOS
