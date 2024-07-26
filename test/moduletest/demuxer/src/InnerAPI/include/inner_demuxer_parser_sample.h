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

#ifndef INNER_DEMUXER_PARSER_SAMPLE_H
#define INNER_DEMUXER_PARSER_SAMPLE_H

#include <iostream>
#include <map>
#include "avdemuxer.h"
#include "avsource.h"
#include "nlohmann/json.hpp"

struct JsonFrameLayerInfo {
    int32_t frameId;
    int64_t dts;
    int32_t layer;
    bool discardable;
};

struct JsonGopInfo {
    int32_t gopId;
    int32_t gopSize;
    int32_t startFrameId;
    uint32_t layerCount = 0;
    std::map<uint8_t, uint32_t> layerFrameNum;
};

namespace OHOS {
namespace MediaAVCodec {
using AVBuffer = OHOS::Media::AVBuffer;
using AVAllocator = OHOS::Media::AVAllocator;
using AVAllocatorFactory = OHOS::Media::AVAllocatorFactory;
using MemoryFlag = OHOS::Media::MemoryFlag;
using FormatDataType = OHOS::Media::FormatDataType;

enum struct MP4Scene : uint32_t {
    ONE_I_FRAME_AVC = 0,
    ALL_I_FRAME_AVC = 1,
    IPB_FRAME_AVC,
    SDTP_FRAME_AVC,
    OPENGOP_FRAME_AVC,
    LTR_FRAME_AVC,
    TWO_LAYER_FRAME_AVC,
    THREE_LAYER_FRAME_AVC,
    FOUR_LAYER_FRAME_AVC,
    ONE_I_FRAME_HEVC,
    ALL_I_FRAME_HEVC,
    IPB_FRAME_HEVC,
    SDTP_FRAME_HEVC,
    OPENGOP_FRAME_HEVC,
    LTR_FRAME_HEVC,
    TWO_LAYER_FRAME_HEVC,
    THREE_LAYER_FRAME_HEVC,
    FOUR_LAYER_FRAME_HEVC
};

enum struct WorkPts : uint32_t {
    START_PTS = 0,
    END_PTS = 1,
    RANDOM_PTS,
    SPECIFIED_PTS
};

class InnerDemuxerParserSample {
public:
    InnerDemuxerParserSample(const std::string &filePath);
    ~InnerDemuxerParserSample();
    size_t GetFileSize(const std::string& filePath);
    void InitParameter(MP4Scene scene);
    bool RunSeekScene(WorkPts workPts);
    bool RunSpeedScene(WorkPts workPts);

    std::shared_ptr<AVDemuxer> demuxer_ = nullptr;
    std::shared_ptr<OHOS::Media::AVBuffer> avBuffer;
    int64_t specified_pts = 0;
private:
    void InitMP4Scene(MP4Scene scene);
    void InitAVCScene(MP4Scene scene);
    void InitHEVCScene(MP4Scene scene);
    bool CheckGopLayerResult(GopLayerInfo &info, int32_t gopId);
    bool CheckFrameLayerResult(FrameLayerInfo &info, int64_t dts, bool speedScene);
    uint32_t GetGopIdFromFrameId(int32_t frameId);
    int64_t GetPtsFromWorkPts(WorkPts workPts);
    std::shared_ptr<AVSource> avsource_ = nullptr;
    Format source_format_;
    Format track_format_;
    int32_t fd;
    int32_t trackCount;
    int64_t duration;
    int32_t videoTrackIdx;
    int64_t usleepTime = 300000;

    nlohmann::json gopJson_;
    nlohmann::json frameLayerJson_;
    std::vector<JsonFrameLayerInfo> frameVec_;
    std::vector<JsonGopInfo> gopVec_;
    std::map<int64_t, JsonFrameLayerInfo> frameMap_;
    std::map<int32_t, JsonGopInfo> frameGopMap_;
};
}
}
#endif
