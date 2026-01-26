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

#ifndef ENCHANCER_VIDEO_MOCK_H
#define ENCHANCER_VIDEO_MOCK_H

#include <gmock/gmock.h>
#include "video_resize_filter.h"

#ifdef USE_VIDEO_PROCESSING_ENGINE
#include "detail_enhancer_video.h"
#include "detail_enhancer_video_common.h"

namespace OHOS {
namespace Media {
namespace VideoProcessingEngine {

class MockDetailEnhancerVideo : public DetailEnhancerVideo {
public:
    MockDetailEnhancerVideo() = default;
    ~MockDetailEnhancerVideo() = default;

    MOCK_METHOD(VPEAlgoErrCode, SetParameter, (const DetailEnhancerParameters& parameter, SourceType type), ());
    MOCK_METHOD(VPEAlgoErrCode, RenderOutputBuffer, (uint32_t index), ());

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
}  // namespace VideoProcessingEngine
}  // namespace Media
}  // namespace OHOS
#endif
#endif // ENCHANCER_VIDEO_MOCK_Hs