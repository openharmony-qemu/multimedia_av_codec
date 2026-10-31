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
 
#ifndef HISTREAMER_DASH_COMMON_H
#define HISTREAMER_DASH_COMMON_H

#include <string>
#include <vector>
#include <cstdint>
#include "av_common.h"
#include "safe_arithmetic.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
constexpr const char *const DRM_URN_UUID_PREFIX = "urn:uuid:";

using DashSegmentInitValue = enum DashSegmentInitValue {
    DASH_SEGMENT_INIT_FAILED = -1,
    DASH_SEGMENT_INIT_SUCCESS,
    DASH_SEGMENT_INIT_UNDO
};

using DashSegsState = enum DashSegsState {
    DASH_SEGS_STATE_NONE,
    DASH_SEGS_STATE_PARSING,
    DASH_SEGS_STATE_FINISH
};

using DashVideoType = enum DashVideoType {
    DASH_VIDEO_TYPE_SDR,
    DASH_VIDEO_TYPE_HDR_VIVID,
    DASH_VIDEO_TYPE_HDR_10
};

struct DashSegment {
    DashSegment()
    {
        streamId_ = -1;
        duration_ = 0;
        bandwidth_ = 0;
        startNumberSeq_ = 1;
        numberSeq_ = 1;
        startRangeValue_ = 0;
        endRangeValue_ = 0;
        isLast_ = false;
    }

    DashSegment(const DashSegment& srcSegment)
    {
        streamId_ = srcSegment.streamId_;
        duration_ = srcSegment.duration_;
        bandwidth_ = srcSegment.bandwidth_;
        startNumberSeq_ = srcSegment.startNumberSeq_;
        numberSeq_ = srcSegment.numberSeq_;
        startRangeValue_ = srcSegment.startRangeValue_;
        endRangeValue_ = srcSegment.endRangeValue_;
        url_ = srcSegment.url_;
        byteRange_ = srcSegment.byteRange_;
        isLast_ = srcSegment.isLast_;
    }

    DashSegment& operator=(const DashSegment& srcSegment)
    {
        if (this != &srcSegment) {
            streamId_ = srcSegment.streamId_;
            duration_ = srcSegment.duration_;
            bandwidth_ = srcSegment.bandwidth_;
            startNumberSeq_ = srcSegment.startNumberSeq_;
            numberSeq_ = srcSegment.numberSeq_;
            startRangeValue_ = srcSegment.startRangeValue_;
            endRangeValue_ = srcSegment.endRangeValue_;
            url_ = srcSegment.url_;
            byteRange_ = srcSegment.byteRange_;
            isLast_ = srcSegment.isLast_;
        }
        return *this;
    }

    int32_t streamId_;
    uint64_t duration_;  // nanoseconds
    unsigned int bandwidth_;
    int64_t startNumberSeq_;
    int64_t numberSeq_;
    int64_t startRangeValue_;
    int64_t endRangeValue_;
    std::string url_;
    std::string byteRange_;
    bool isLast_;
};

struct DashIndexSegment {
    DashIndexSegment()
    {
        indexRangeBegin_ = 0;
        indexRangeEnd_ = 0;
    }

    int64_t indexRangeBegin_;
    int64_t indexRangeEnd_;
    std::string url_;
};

using InitSegmentState = enum InitSegmentState {
    INIT_SEGMENT_STATE_UNUSE,
    INIT_SEGMENT_STATE_USING,
    INIT_SEGMENT_STATE_USED
};

struct DashInitSegment {
    DashInitSegment()
    {
        isDownloadFinish_ = false;
        streamId_ = 0;
        readIndex_ = 0;
        rangeBegin_ = 0;
        rangeEnd_ = 0;
        readState_ = INIT_SEGMENT_STATE_USED;
        writeState_ = INIT_SEGMENT_STATE_USED;
    }

    bool isDownloadFinish_;
    int streamId_;
    unsigned int readIndex_;
    int64_t rangeBegin_;
    int64_t rangeEnd_;
    std::string url_;
    std::string content_;
    InitSegmentState readState_; // no need to get init segment as init segment is null
    InitSegmentState writeState_;
};

// Lazy expansion mode for DashStreamDescription.
// DASH_LAZY_MODE_NONE = use mediaSegments_ (pre-expanded); others = use compact entries.
enum DashLazyMode {
    DASH_LAZY_MODE_NONE = 0,
    DASH_LAZY_MODE_TIMELINE = 1,     // SegmentTemplate + SegmentTimeline
    DASH_LAZY_MODE_DURATION = 2,     // SegmentTemplate + @duration
    DASH_LAZY_MODE_SIDX = 3,         // SegmentBase (sidx box)
    DASH_LAZY_MODE_SEGLIST = 4,      // SegmentList
};

// Compact timeline entry for SegmentTimeline lazy expansion.
// Instead of expanding @r into individual DashSegment objects (which can OOM
// with malicious @r values), we store one entry per <S> element and compute
// segment URL/duration on demand.
struct DashTimelineEntry {
    uint64_t startTime;     // @t value (timescale units), start time of first segment in this entry
    uint64_t duration;      // @d value (timescale units), duration of each segment
    uint64_t segDurationNs; // @d converted to nanoseconds
    int64_t repeatCount;    // @r + 1 (number of segments described by this entry, ≥1)
    int64_t startSeq;       // $Number of the first segment in this entry
};

// Compact info for fixed-duration SegmentTemplate (no SegmentTimeline).
// Segment count is computed from totalDuration / segDuration.
struct DashDurationSegInfo {
    std::string mediaUrlTemplate;  // URL template with $Time/$Number (after $RepresentationID/$Bandwidth substitution)
    uint64_t segDurationNs = 0;    // segment duration in nanoseconds
    int64_t baseDuration = 0;      // @duration in timescale units (for $Time computation)
    int64_t startNumberSeq = 0;    // start number sequence
    uint64_t totalDuration = 0;    // total duration in nanoseconds (for segment count)
};

// Compact entry for SegmentBase (sidx box) lazy expansion.
// Instead of expanding each sidx subsegment into a DashSegment object, we store
// one entry per subsegment and build the DashSegment on demand.
struct DashSidxEntry {
    uint64_t durationNs;    // segment duration in nanoseconds
    int64_t numberSeq;      // $Number of this segment
    int64_t startRange;     // byte range start
    int64_t endRange;       // byte range end
    std::string url;        // segment URL (from indexSegment_->url_)
};

// Compact entry for SegmentList lazy expansion.
// Instead of expanding each <SegmentURL> into a DashSegment object, we store
// one entry per <SegmentURL> and build the DashSegment on demand.
struct DashSegListEntry {
    std::string mediaUrl;   // <SegmentURL media="...">
    std::string mediaRange; // <SegmentURL mediaRange="...">
    uint64_t durationNs;    // segment duration in nanoseconds
    int64_t numberSeq;      // $Number of this segment
};

struct DashStreamDescription {
    DashStreamDescription() = default;
    DashStreamDescription(const DashStreamDescription&) = default;
    DashStreamDescription& operator=(const DashStreamDescription&) = default;

    DashSegsState segsState_ = DASH_SEGS_STATE_NONE;
    bool inUse_ = false;
    DashVideoType videoType_ = DASH_VIDEO_TYPE_SDR;
    MediaAVCodec::MediaType type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    int streamId_ = 0;
    unsigned int periodIndex_ = 0;
    unsigned int adptSetIndex_ = 0;
    unsigned int representationIndex_ = 0;
    uint64_t duration_ = 0;  // nanoseconds
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    unsigned int bandwidth_ = 0;
    unsigned int channels_ = 0;
    int64_t startNumberSeq_ = 1;
    int64_t currentNumberSeq_ = -1;
    std::string lang_ {};
    unsigned int minBandwidth_ = 0;
    unsigned int maxBandwidth_ = 0;
    unsigned int minWidth_ = 0;
    unsigned int maxWidth_ = 0;
    unsigned int minHeight_ = 0;
    unsigned int maxHeight_ = 0;
    std::string frameRate_ {};
    std::string minFrameRate_ {};
    std::string maxFrameRate_ {};
    std::string mimeType_ {};
    std::string codecs_ {};
    uint64_t periodStartTime_ = 0;
    std::shared_ptr<DashInitSegment> initSegment_ = nullptr;
    std::shared_ptr<DashIndexSegment> indexSegment_ = nullptr;
    std::vector<std::shared_ptr<DashSegment>> mediaSegments_;
    DashLazyMode lazyMode_ = DASH_LAZY_MODE_NONE;  // NONE = mediaSegments_ used; otherwise compact entries below
    std::vector<DashTimelineEntry> timelineEntries_;  // for SegmentTimeline (DASH_LAZY_MODE_TIMELINE)
    DashDurationSegInfo durationSegInfo_;              // for fixed-duration SegmentTemplate (DASH_LAZY_MODE_DURATION)
    std::vector<DashSidxEntry> sidxEntries_;           // for SegmentBase/sidx (DASH_LAZY_MODE_SIDX)
    std::vector<DashSegListEntry> segListEntries_;     // for SegmentList (DASH_LAZY_MODE_SEGLIST)
    // Get total number of media segments (lazy or expanded).
    uint64_t GetMediaSegmentCount() const
    {
        switch (lazyMode_) {
            case DASH_LAZY_MODE_NONE:
                return mediaSegments_.size();
            case DASH_LAZY_MODE_TIMELINE:
                return GetTimelineSegmentCount();
            case DASH_LAZY_MODE_DURATION:
                if (durationSegInfo_.segDurationNs == 0 || durationSegInfo_.totalDuration == 0) {
                    return 0;
                }
                return (durationSegInfo_.totalDuration - 1) / durationSegInfo_.segDurationNs + 1;
            case DASH_LAZY_MODE_SIDX:
                return sidxEntries_.size();
            case DASH_LAZY_MODE_SEGLIST:
                return segListEntries_.size();
            default:
                return 0;
        }
    }

    // Sum repeat counts across timeline entries, saturating on overflow.
    uint64_t GetTimelineSegmentCount() const
    {
        uint64_t count = 0;
        for (const auto& entry : timelineEntries_) {
            if (entry.repeatCount <= 0) {
                continue;
            }
            uint64_t newCount;
            if (!SafeAdd(count, static_cast<uint64_t>(entry.repeatCount), newCount)) {
                return count;
            }
            count = newCount;
        }
        return count;
    }

    uint64_t GetMediaSegmentDuration(uint64_t index) const
    {
        switch (lazyMode_) {
            case DASH_LAZY_MODE_NONE:
                return (index < mediaSegments_.size() && mediaSegments_[index]) ? mediaSegments_[index]->duration_ : 0;
            case DASH_LAZY_MODE_TIMELINE:
                for (const auto& entry : timelineEntries_) {
                    if (entry.repeatCount <= 0) {
                        continue;
                    }
                    uint64_t cnt = static_cast<uint64_t>(entry.repeatCount);
                    if (index < cnt) {
                        return entry.segDurationNs;
                    }
                    index -= cnt;
                }
                return 0;
            case DASH_LAZY_MODE_DURATION: {
                uint64_t count = GetMediaSegmentCount();
                if (durationSegInfo_.segDurationNs == 0 || index >= count) {
                    return 0;
                }
                if (index == count - 1) {
                    uint64_t remainder = durationSegInfo_.totalDuration % durationSegInfo_.segDurationNs;
                    return remainder > 0 ? remainder : durationSegInfo_.segDurationNs;
                }
                return durationSegInfo_.segDurationNs;
            }
            case DASH_LAZY_MODE_SIDX:
                return (index < sidxEntries_.size()) ? sidxEntries_[index].durationNs : 0;
            case DASH_LAZY_MODE_SEGLIST:
                return (index < segListEntries_.size()) ? segListEntries_[index].durationNs : 0;
            default:
                return 0;
        }
    }

    int64_t GetMediaSegmentNumberSeq(uint64_t index) const
    {
        switch (lazyMode_) {
            case DASH_LAZY_MODE_NONE:
                if (index < mediaSegments_.size() && mediaSegments_[index]) {
                    return mediaSegments_[index]->numberSeq_;
                }
                return -1;
            case DASH_LAZY_MODE_TIMELINE:
                return GetTimelineSegmentNumberSeq(index);
            case DASH_LAZY_MODE_DURATION:
                if (index >= GetMediaSegmentCount()) {
                    return -1;
                }
                int64_t indexI;
                int64_t result;
                if (!SafeIntCast<uint64_t, int64_t>(index, indexI) ||
                    !SafeAdd(durationSegInfo_.startNumberSeq, indexI, result)) {
                    return std::numeric_limits<int64_t>::max();
                }
                return result;
            case DASH_LAZY_MODE_SIDX:
                return (index < sidxEntries_.size()) ? sidxEntries_[index].numberSeq : -1;
            case DASH_LAZY_MODE_SEGLIST:
                return (index < segListEntries_.size()) ? segListEntries_[index].numberSeq : -1;
            default:
                return -1;
        }
    }

    // Locate segment number seq by walking timeline entries, subtracting each
    // entry's repeat count from index; returns -1 if index is out of range.
    int64_t GetTimelineSegmentNumberSeq(uint64_t index) const
    {
        for (const auto& entry : timelineEntries_) {
            if (entry.repeatCount <= 0) {
                continue;
            }
            uint64_t cnt = static_cast<uint64_t>(entry.repeatCount);
            if (index < cnt) {
                int64_t result;
                if (!SafeAdd(entry.startSeq, static_cast<int64_t>(index), result)) {
                    return std::numeric_limits<int64_t>::max();
                }
                return result;
            }
            index -= cnt;
        }
        return -1;
    }
};

struct DashDrmInfo {
    std::string drmId_;
    std::string uuid_;
    std::string pssh_;
};
}
}
}
}
#endif