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

class TemporalScalability {
public:
    TemporalScalability();
    virtual ~TemporalScalability();
    void ValidateTemporalGopParam(Format &format);
    void StoreAVBuffer(uint32_t index, std::shared_ptr<Media::AVBuffer> buffer);
    uint32_t GetFirstBufferIndex();
    void SetBlockQueueActive();
    void ConfigureLTR(uint32_t index);
    void SetDisposableFlag(std::shared_ptr<Media::AVBuffer> buffer);

private:
    int32_t isMarkLTR_;
    bool isUseLTR_;
    int32_t ltrPoc_;
    int32_t poc_ = 0;
    int32_t temporalPoc_ = 0;
    uint32_t inputFrameCounter_ = 0;
    uint32_t outputFrameCounter_ = 0;
    int32_t frameNum_ = 0;
    int32_t gopSize_;
    int32_t temporalGopSize_;
    int32_t tRefMode_;
    std::shared_mutex inputBufMutex_;
    std::unordered_map<uint32_t, uint32_t> frameFlagMap_;
    std::unordered_map<uint32_t, std::shared_ptr<Media::AVBuffer>> inputBufferMap_;
    std::shared_ptr<BlockQueue<uint32_t>> inputIndexQueue_;
    void LTRDecision();
    void DisposableDecision();
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif // TEMPORAL_SCALABILITY_H