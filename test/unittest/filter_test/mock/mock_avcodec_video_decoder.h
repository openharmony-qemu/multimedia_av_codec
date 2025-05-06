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

#ifndef MOCK_AVCODEC_VIDEO_DECODER_H
#define MOCK_AVCODEC_VIDEO_DECODER_H

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "avcodec_video_decoder.h"
#include "surface_decoder_adapter.h"
#include "avbuffer_queue_define.h"
#include "surface.h"
#include "meta/meta.h"
#include "common/log.h"
#include "avcodec_common.h"

namespace OHOS {
namespace Media {

class MockAVCodecVideoDecoder : public MediaAVCodec::AVCodecVideoDecoder {
public:
    ~MockAVCodecVideoDecoder() override = default;
    MOCK_METHOD(int32_t, Configure, (const Format &format), (override));
    MOCK_METHOD(int32_t, Prepare, (), (override));
    MOCK_METHOD(int32_t, Start, (), (override));
    MOCK_METHOD(int32_t, Stop, (), (override));
    MOCK_METHOD(int32_t, Flush, (), (override));
    MOCK_METHOD(int32_t, Reset, (), (override));
    MOCK_METHOD(int32_t, Release, (), (override));
    MOCK_METHOD(int32_t, SetOutputSurface, (sptr<Surface> surface), (override));
    MOCK_METHOD(int32_t, QueueInputBuffer, (uint32_t index, MediaAVCodec::AVCodecBufferInfo info,
        MediaAVCodec::AVCodecBufferFlag flag), (override));
    MOCK_METHOD(int32_t, QueueInputBuffer, (uint32_t index), (override));
    MOCK_METHOD(int32_t, GetOutputFormat, (Format &format), (override));
    MOCK_METHOD(int32_t, ReleaseOutputBuffer, (uint32_t index, bool render), (override));
    MOCK_METHOD(int32_t, RenderOutputBufferAtTime, (uint32_t index, int64_t renderTimestampNs), (override));
    MOCK_METHOD(int32_t, SetParameter, (const Format &format), (override));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaAVCodec::AVCodecCallback> &callback), (override));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback), (override));
    MOCK_METHOD(int32_t, SetDecryptConfig, (const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag), (override));
};

class MockDecoderAdapterCallback : public DecoderAdapterCallback {
public:
    MOCK_METHOD(void, OnError, (MediaAVCodec::AVCodecErrorType type, int32_t errorCode), (override));
    MOCK_METHOD(void, OnOutputFormatChanged, (const std::shared_ptr<Meta> &format), (override));
    MOCK_METHOD(void, OnBufferEos, (int64_t pts, int64_t frameNum), (override));
};
} // namespace Media
} // namespace OHOS
#endif // MOCK_AVCODEC_VIDEO_DECODER_H