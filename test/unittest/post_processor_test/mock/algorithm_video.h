/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef BASE_VIDEO_POST_PROCESSOR_H
#define INTERFACES_INNER_API_ALGORITHM_VIDEO_H

#include "gmock/gmock.h"
#include <cinttypes>
#include <memory>

#include "meta/format.h"
#include "refbase.h"
#include "surface.h"

#include "algorithm_errors.h"
#include "algorithm_video_common.h"

namespace OHOS {
namespace Media {
namespace VideoProcessingEngine {

class VpeVideo {
public:
    static std::shared_ptr<VpeVideo> Create(uint32_t type) {
        return std::make_shared<VpeVideo>();
    }
    MOCK_METHOD(VPEAlgoErrCode, RegisterCallback, (const std::shared_ptr<VpeVideoCallback>& callback), ());
    MOCK_METHOD(VPEAlgoErrCode, SetOutputSurface, (const sptr<Surface>& surface), ());
    MOCK_METHOD(sptr<Surface>, GetInputSurface, (), ());
    MOCK_METHOD(VPEAlgoErrCode, SetParameter, (const Format& parameter), ());
    MOCK_METHOD(VPEAlgoErrCode, GetParameter, (Format& parameter), ());
    MOCK_METHOD(VPEAlgoErrCode, Start, (), ());
    MOCK_METHOD(VPEAlgoErrCode, Stop, (), ());
    MOCK_METHOD(VPEAlgoErrCode, Release, (), ());
    MOCK_METHOD(VPEAlgoErrCode, Flush, (), ());
    MOCK_METHOD(VPEAlgoErrCode, Enable, (), ());
    MOCK_METHOD(VPEAlgoErrCode, Disable, (), ());
    MOCK_METHOD(VPEAlgoErrCode, NotifyEos, (), ());
    MOCK_METHOD(VPEAlgoErrCode, ReleaseOutputBuffer, (uint32_t index, bool render), ());
    MOCK_METHOD(VPEAlgoErrCode, RenderOutputBufferAtTime, (uint32_t index, int64_t renderTimestamp), ());
};

} // namespace 
} // namespace Media
} // namespace OHOS
#endif // INTERFACES_INNER_API_ALGORITHM_VIDEO_H
