/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef MOCK_AVCODEC_VIDEO_ENCODER_H
#define MOCK_AVCODEC_VIDEO_ENCODER_H

#include "gmock/gmock.h"
#include "avcodec_video_encoder.h"

namespace OHOS {
namespace Media {
class MockAVCodecVideoEncoder : public MediaAVCodec::AVCodecVideoEncoder {
public:
    ~MockAVCodecVideoEncoder() override = default;
    MOCK_METHOD(int32_t, Configure, (const Format &format), (override));
    MOCK_METHOD(int32_t, Prepare, (), (override));
    MOCK_METHOD(int32_t, Start, (), (override));
    MOCK_METHOD(int32_t, Stop, (), (override));
    MOCK_METHOD(int32_t, Flush, (), (override));
    MOCK_METHOD(int32_t, NotifyEos, (), (override));
    MOCK_METHOD(int32_t, Reset, (), (override));
    MOCK_METHOD(int32_t, Release, (), (override));
    MOCK_METHOD(sptr<Surface>, CreateInputSurface, (), (override));
    MOCK_METHOD(int32_t, QueueInputBuffer, (uint32_t index, MediaAVCodec::AVCodecBufferInfo info,
        MediaAVCodec::AVCodecBufferFlag flag), (override));
    MOCK_METHOD(int32_t, QueueInputBuffer, (uint32_t index), (override));
    MOCK_METHOD(int32_t, QueueInputParameter, (uint32_t index), (override));
    MOCK_METHOD(int32_t, GetOutputFormat, (Format &format), (override));
    MOCK_METHOD(int32_t, ReleaseOutputBuffer, (uint32_t index), (override));
    MOCK_METHOD(int32_t, SetParameter, (const Format &format), (override));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaAVCodec::AVCodecCallback>
        &callback), (override));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaAVCodec::MediaCodecCallback>
        &callback), (override));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaAVCodec::MediaCodecParameterCallback>
        &callback), (override));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaAVCodec::MediaCodecParameterWithAttrCallback>
        &callback), (override));
    MOCK_METHOD(int32_t, GetInputFormat, (Format &format), (override));
    MOCK_METHOD(int32_t, SetCustomBuffer, (std::shared_ptr<AVBuffer> buffer), (override));
};
} // namespace Media
} // namespace OHOS
#endif // MOCK_AVCODEC_VIDEO_ENCODER_H