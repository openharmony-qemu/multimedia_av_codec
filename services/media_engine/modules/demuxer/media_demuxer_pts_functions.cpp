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
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MediaDemuxer" };
} // namespace

namespace OHOS {
namespace Media {
constexpr int64_t MAX_PTS_DIFFER_THRESHOLD_US = 2000000; // The maximum difference between Segment 2s.
constexpr int64_t INVALID_PTS_DATA = -1; // The invalid pts data -1.
constexpr int64_t PTS_MICRO_ADJUSTMENT_US = 1000;
constexpr int64_t LOG_INTERVAL_MS_COUNT = 2000; // 2s
constexpr uint32_t LOG_MAX_PRINTS_COUNT = 10; // 10 times

void MediaDemuxer::HandleAutoMaintainPts(int32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    if (!isAutoMaintainPts_.load()) {
        return;
    }
    int64_t curPacketPts = sample->pts_;
    std::shared_ptr<MaintainBaseInfo> baseInfo = maintainBaseInfos_[trackId];
    FALSE_RETURN_MSG(baseInfo != nullptr, "BaseInfo is nullptr, track " PUBLIC_LOG_D32, trackId);
    int64_t diff = 0;
    diff = curPacketPts - baseInfo->lastPts;
    auto oldPts =  baseInfo->lastPts;
    baseInfo->lastPts = curPacketPts;
    if (diff < 0) {
        diff = 0 - diff;
    }

    if (baseInfo->segmentOffset == INVALID_PTS_DATA) {
        int64_t offset = GetCurrentSegmentOffset(trackId, baseInfo->segmentOffset);
        if (baseInfo->segmentOffset != offset) {
            baseInfo->segmentOffset = offset;
            baseInfo->basePts = curPacketPts;
            InitVideoAudioBasePtsDiff();
        }
        sample->pts_ = baseInfo->segmentOffset + curPacketPts - baseInfo->basePts + mediaStartPts_;
    } else if (diff > MAX_PTS_DIFFER_THRESHOLD_US) {
        videoAudioBasePtsDiff = -1;
        if (baseInfo->isLastPtsChange) {
            int64_t offset = GetCurrentSegmentOffset(trackId, baseInfo->segmentOffset);
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
        FALSE_GOON_NOEXEC(videoAudioBasePtsDiff != -1 && trackId != videoTrackId_,
            sample->pts_ += videoAudioBasePtsDiff);
        baseInfo->isLastPtsChange = false;
    }
    baseInfo->lastPtsModifyedMax = std::max(sample->pts_, baseInfo->lastPtsModifyedMax);

    AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGI, LOG_INTERVAL_MS_COUNT, LOG_MAX_PRINTS_COUNT,
        "Success, track:" PUBLIC_LOG_D32 ", orgPts:"
        PUBLIC_LOG_D64 ", pts:" PUBLIC_LOG_D64 ", basePts:" PUBLIC_LOG_D64, trackId,
        curPacketPts, sample->pts_, baseInfo->basePts);
}

void MediaDemuxer::InitPtsInfo()
{
    if (source_ == nullptr || !isHls_) {
        return;
    }
    MEDIA_LOG_I("Enable hls disContinuity auto maintain pts");
    isAutoMaintainPts_.store(true);
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
        SetCurrentSegmentOffset(trackId, source_->GetSegmentOffset());
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

void MediaDemuxer::UpdateSegmentOffset(int32_t trackId)
{
    FALSE_RETURN_NOLOG(isAutoMaintainPts_.load());
    FALSE_RETURN_NOLOG(source_ != nullptr);

    auto offset = source_->GetSegmentOffset();
    SetCurrentSegmentOffset(trackId, offset);
    FALSE_GOON_NOEXEC(IsAVInOneStream() && trackId == videoTrackId_, SetCurrentSegmentOffset(audioTrackId_, offset));
}

void MediaDemuxer::UpdateSegmentOffset(int32_t oldTrackId, int32_t newTrackId)
{
    FALSE_RETURN_NOLOG(isAutoMaintainPts_.load());
    FALSE_RETURN_NOLOG(source_ != nullptr);

    auto offset = source_->GetSegmentOffset();
    SetCurrentSegmentOffset(newTrackId, GetCurrentSegmentOffset(oldTrackId, offset));
}

void MediaDemuxer::SetCurrentSegmentOffset(int32_t trackId, size_t segmentOffset)
{
    FALSE_RETURN_NOLOG(IsValidTrackId(trackId));

    // segmentOffset is derived from PTS. Clamp to int64_t range to avoid overflow.
    std::lock_guard<std::mutex> lock(segmentOffsetMutex_);
    int64_t max = std::numeric_limits<int64_t>::max();
    currentSegmentOffsetMap_[trackId] =
        (segmentOffset > static_cast<size_t>(max)) ? max : static_cast<int64_t>(segmentOffset);
}

int64_t MediaDemuxer::GetCurrentSegmentOffset(int32_t trackId, int64_t oldSegmentOffset)
{
    FALSE_RETURN_V_NOLOG(isAutoMaintainPts_.load(), oldSegmentOffset);
    FALSE_RETURN_V_NOLOG(IsValidTrackId(trackId), oldSegmentOffset);
    std::lock_guard<std::mutex> lock(segmentOffsetMutex_);
    FALSE_RETURN_V_NOLOG(currentSegmentOffsetMap_.find(trackId) != currentSegmentOffsetMap_.end(), oldSegmentOffset);
    return currentSegmentOffsetMap_[trackId];
}

void MediaDemuxer::InitVideoAudioBasePtsDiff()
{
    FALSE_RETURN(IsAVInOneStream() && maintainBaseInfos_[audioTrackId_] == nullptr
        && maintainBaseInfos_[videoTrackId_] == nullptr);
    FALSE_RETURN(maintainBaseInfos_[audioTrackId_]->basePts != INVALID_PTS_DATA);
    FALSE_RETURN(maintainBaseInfos_[videoTrackId_]->basePts != INVALID_PTS_DATA);
    videoAudioBasePtsDiff = maintainBaseInfos_[videoTrackId_]->basePts - maintainBaseInfos_[audioTrackId_]->basePts;
}
} // namespace Media
} // namespace OHOS
