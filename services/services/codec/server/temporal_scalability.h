/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef TEMPORAL_SCALABILITY_H
#define TEMPORAL_SCALABILITY_H

#include <cstdint>
#include <shared_mutex>
#include <unordered_map>
#include "av_common.h"
#include "block_queue.h"
#include "buffer/avbuffer.h"

namespace OHOS {
namespace MediaAVCodec {
constexpr double DEFAULT_FRAMERATE = 30.0;
constexpr int32_t DEFAULT_I_FRAME_INTERVAL = 2000;
constexpr int32_t MIN_TEMPORAL_GOPSIZE = 2;
constexpr int32_t DEFAULT_TEMPORAL_GOPSIZE = 4;
constexpr int32_t DEFAULT_GOPSIZE = 60;

class TemporalScalability {
public:
    explicit TemporalScalability(std::string name);
    virtual ~TemporalScalability();
    bool svcLTR_ = false; // true: LTR
    void ValidateTemporalGopParam(Format &format);
    uint32_t GetFirstBufferIndex();
    void StoreAVBuffer(uint32_t index, std::shared_ptr<Media::AVBuffer> buffer);
    void ConfigureLTR(uint32_t index);
    void SetDisposableFlag(std::shared_ptr<Media::AVBuffer> buffer);
    void SetBlockQueueActive();

private:
    std::string name_;
    bool isMarkLTR_ = false;
    bool isUseLTR_ = false;
    int32_t ltrPoc_ = 0;
    int32_t poc_ = 0;
    int32_t temporalPoc_ = 0;
    uint32_t inputFrameCounter_ = 0;
    uint32_t outputFrameCounter_ = 0;
    int32_t frameNum_ = 0;
    int32_t gopSize_ = DEFAULT_GOPSIZE;
    int32_t temporalGopSize_ = 0;
    int32_t tRefMode_ = 0;
    std::shared_mutex inputBufMutex_;
    std::shared_mutex frameFlagMapMutex_;
    std::unordered_map<uint32_t, uint32_t> frameFlagMap_;
    std::unordered_map<uint32_t, std::shared_ptr<Media::AVBuffer>> inputBufferMap_;
    std::shared_ptr<BlockQueue<uint32_t>> inputIndexQueue_;
    bool IsLTRSolution();
    int32_t LTRFrameNumCalculate(int32_t tGopSize);
    void MarkLTRDecision();
    int32_t LTRPocDecision(int32_t tPoc);
    void AdjacentJumpLTRDecision();
    void UniformlyScaledLTRDecision();
    void LTRDecision();
    uint32_t DisposableDecision();
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif // TEMPORAL_SCALABILITY_H