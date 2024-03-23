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

#ifndef TEMPORAL_LEVEL_SCALE_H
#define TEMPORAL_LEVEL_SCALE_H

#include <shared_mutex>
#include <unordered_map>
#include "av_common.h"
#include "block_queue.h"
#include "buffer/avbuffer.h"

namespace OHOS {
namespace MediaAVCodec {

class TemporalLevelScale {
public:
    TemporalLevelScale();
    virtual ~TemporalLevelScale();
    int32_t ValidateTemporalGopParam(Media::Format &format);
    void StoreAVBuffer(uint32_t index, std::shared_ptr<Media::AVBuffer> buffer);
    uint32_t GetFirstBufferIndex();
    void SetBlockQueueActive();
    void ConfigureLTR(uint32_t index);

private:
    bool isMarkLTR_;
    bool isUseLTR_;
    uint32_t ltrPoc_;
    uint32_t poc_ = 0;
    uint32_t temporalPoc_ = 0;
    int32_t frameNum_ = 0;
    uint32_t gopSize_;
    int32_t temporalGopSize_;
    int32_t tRefMode_;
    int32_t frameInterval_;
    double frameRate_;
    std::shared_mutex inputBufMutex_;
    std::unordered_map<uint32_t, std::shared_ptr<Media::AVBuffer>> inputBufferMap_;
    std::shared_ptr<BlockQueue<uint32_t>> inputIndexQueue_;
    void LTRDecision();
    void ConfigFrameGop(Format &format);
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif // TEMPORAL_LEVEL_SCALE_H