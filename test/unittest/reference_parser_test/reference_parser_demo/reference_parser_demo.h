/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef REFERENCE_PARSER_DEMO_H
#define REFERENCE_PARSER_DEMO_H

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
};

namespace OHOS {
namespace MediaAVCodec {
enum struct MP4Scene : uint32_t {
    LOWDELAY_B_SCALA = 0,
    SCENE_MAX
};

class ReferenceParserDemo : public NoCopyable {
public:
    ~ReferenceParserDemo();
    void SetDecIntervalMs(int64_t decIntervalMs);
    int32_t InitScene(MP4Scene scene);
    bool DoAccurateSeek(int64_t seekTimeMs);
    bool DoVariableSpeedPlay(int64_t playTimeMs);

private:
    bool CheckFrameLayerResult(FrameLayerInfo &info, int64_t dts);
    bool CheckGopLayerResult();
    void LoadJson();
    int32_t InitDemuxer(int64_t size);
    MP4Scene scene_ = MP4Scene::LOWDELAY_B_SCALA;
    int64_t decIntervalUs_ = 0;
    nlohmann::json gopJson_;
    nlohmann::json frameLayerJson_;
    std::vector<JsonGopInfo> gopVec_;
    std::vector<JsonFrameLayerInfo> frameVec_;
    std::map<int64_t, JsonFrameLayerInfo> frameMap_;
    int32_t fd_ = 0;
    std::shared_ptr<AVBuffer> buffer_ = nullptr;
    std::shared_ptr<AVSource> source_ = nullptr;
    std::shared_ptr<AVDemuxer> demuxer_ = nullptr;
    int32_t videoTrackId_ = 0;
    int64_t startDts_ = 0L;
    int64_t startPts_ = 0L;
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // REFERENCE_PARSER_DEMO_H