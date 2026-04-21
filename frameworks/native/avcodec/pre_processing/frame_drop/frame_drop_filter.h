/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef MEDIA_AVCODEC_FRAME_DROP_FILTER_H
#define MEDIA_AVCODEC_FRAME_DROP_FILTER_H

#include <memory>
#include <mutex>

#include "avcodec_errors.h"
#include "avcodec_log_ex.h"

#include "frame_drop_strategy.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

class FrameDropFilter {
public:
    FrameDropFilter() = default;
    ~FrameDropFilter() = default;

    int32_t Configure(double dropToFps, double frameRate);
    bool ShouldDropFrame(uint64_t pts);
    void FlushTimeStamp();

private:
    double dropToFps_ = 0.0;
    double frameRate_ = 0.0;

    bool strategyDecided_ = false;
    bool hasFirstPts_ = false;
    uint64_t firstPts_ = 0;

    std::unique_ptr<IDropStrategy> activeStrategy_;

    std::mutex mutex_;
};

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_FRAME_DROP_FILTER_H
