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
#include "av_common.h"

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
    unsigned int duration_;
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

struct DashStreamDescription {
    DashStreamDescription()
    {
    }

    DashStreamDescription(const DashStreamDescription& desc)
    {
        streamId_ = desc.streamId_;
        periodIndex_ = desc.periodIndex_;
        adptSetIndex_ = desc.adptSetIndex_;
        representationIndex_ = desc.representationIndex_;
        duration_ = desc.duration_;
        width_ = desc.width_;
        height_ = desc.height_;
        bandwidth_ = desc.bandwidth_;
        startNumberSeq_ = desc.startNumberSeq_;
        type_ = desc.type_;
        segsState_ = desc.segsState_;
        inUse_ = desc.inUse_;
        videoType_ = desc.videoType_;
        currentNumberSeq_ = desc.currentNumberSeq_;
        lang_ = desc.lang_;
    }

    DashStreamDescription& operator=(const DashStreamDescription& desc)
    {
        if (this != &desc) {
            streamId_ = desc.streamId_;
            periodIndex_ = desc.periodIndex_;
            adptSetIndex_ = desc.adptSetIndex_;
            representationIndex_ = desc.representationIndex_;
            duration_ = desc.duration_;
            width_ = desc.width_;
            height_ = desc.height_;
            bandwidth_ = desc.bandwidth_;
            startNumberSeq_ = desc.startNumberSeq_;
            type_ = desc.type_;
            segsState_ = desc.segsState_;
            inUse_ = desc.inUse_;
            videoType_ = desc.videoType_;
            currentNumberSeq_ = desc.currentNumberSeq_;
            lang_ = desc.lang_;
        }
        return *this;
    }

    DashSegsState segsState_ = DASH_SEGS_STATE_NONE;
    bool inUse_ = false;
    DashVideoType videoType_ = DASH_VIDEO_TYPE_SDR;
    MediaAVCodec::MediaType type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    int streamId_ = 0;
    unsigned int periodIndex_ = 0;
    unsigned int adptSetIndex_ = 0;
    unsigned int representationIndex_ = 0;
    unsigned int duration_ = 0;
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    unsigned int bandwidth_ = 0;
    int64_t startNumberSeq_ = 1;
    int64_t currentNumberSeq_ = -1;
    std::string lang_ {};
    std::shared_ptr<DashInitSegment> initSegment_ = nullptr;
    std::shared_ptr<DashIndexSegment> indexSegment_ = nullptr;
    std::vector<std::shared_ptr<DashSegment>> mediaSegments_;
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