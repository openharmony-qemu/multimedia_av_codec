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

namespace OHOS {
namespace Media {
constexpr int64_t MAX_PTS_DIFFER_THRESHOLD_US = 10000000; // The maximum difference between Segment 10s.
constexpr int64_t INVALID_PTS_DATA = -1; // The invalid pts data -1.

Status MediaDemuxer::HandleAutoMaintainPts(uint32_t trackId, std::shared_ptr<AVBuffer> sample)
{
    FALSE_RETURN_V(isAutoMaintainPts_, Status::OK);
    int64_t curPacketPts = sample->pts_;
    std::shared_ptr<MaintainBaseInfo> baseInfo = maintainBaseInfos_[trackId];
    int64_t diff = 0;
    diff = curPacketPts - baseInfo->lastPts;
    baseInfo->lastPts = curPacketPts;
    if (diff < 0) {
        diff = 0 - diff;
    }
    if (baseInfo->segmentOffset == INVALID_PTS_DATA || diff > MAX_PTS_DIFFER_THRESHOLD_US) {
        int64_t offset = static_cast<int64_t>(source_->GetSegmentOffset());
        if (baseInfo->segmentOffset != offset) {
            baseInfo->segmentOffset = offset;
            baseInfo->basePts = curPacketPts;
        }
    }
    int64_t offsetUs = 0;
    Plugins::Us2HstTime(baseInfo->segmentOffset, offsetUs);
    sample->pts_ = offsetUs + curPacketPts - baseInfo->basePts;
    MEDIA_LOG_I("HandleAutoMaintainPts success, trackId: " PUBLIC_LOG_U32 ", orginal pts: "
    PUBLIC_LOG_D64 ", pts: " PUBLIC_LOG_D64 ", Offset: " PUBLIC_LOG_D64 ", basePts: "
    PUBLIC_LOG_D64, trackId, curPacketPts, sample->pts_, offsetUs, baseInfo->basePts);
    return Status::OK;
}

Status MediaDemuxer::InitPtsInfo()
{
    FALSE_RETURN_V(source_ != nullptr && source_->GetHLSDiscontinuity(), Status::OK);
    MEDIA_LOG_I("Enable hls disContinuity auto maintain pts");
    isAutoMaintainPts_ = true;
    for (auto it = bufferQueueMap_.begin(); it != bufferQueueMap_.end(); it++) {
        uint32_t trackId = it->first;
        if (maintainBaseInfos_[trackId] == nullptr) {
            maintainBaseInfos_[trackId] = std::make_shared<MaintainBaseInfo>();
        }
        maintainBaseInfos_[trackId]->segmentOffset = INVALID_PTS_DATA;
        maintainBaseInfos_[trackId]->basePts = INVALID_PTS_DATA;
    }
    return Status::OK;
}

} // namespace Media
} // namespace OHOS
