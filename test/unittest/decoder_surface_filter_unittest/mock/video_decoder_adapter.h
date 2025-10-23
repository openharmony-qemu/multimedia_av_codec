/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    virtual  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VIDEO_DECODER_ADAPTER_H
#define VIDEO_DECODER_ADAPTER_H

#include "gmock/gmock.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "drm_i_keysession_service.h"

namespace OHOS {
namespace Media {

class VideoDecoderAdapter {
public:
    MOCK_METHOD(Status, Init, (MediaAVCodec::AVCodecType type, bool isMimeType, const std::string &name), ());
    MOCK_METHOD(Status, Configure, (const Format &format), ());
    MOCK_METHOD(int32_t, SetParameter, (const Format &format), ());
    MOCK_METHOD(Status, Start, (), ());
    MOCK_METHOD(Status, Flush, (), ());
    MOCK_METHOD(Status, Stop, (), ());
    MOCK_METHOD(Status, Reset, (), ());
    MOCK_METHOD(Status, Release, (), ());
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback), ());
    MOCK_METHOD(void, PrepareInputBufferQueue, (), ());
    MOCK_METHOD(sptr<AVBufferQueueProducer>, GetBufferQueueProducer, (), ());
    MOCK_METHOD(sptr<AVBufferQueueConsumer>, GetBufferQueueConsumer, (), ());
    MOCK_METHOD(void, OnInputBufferAvailable, (uint32_t index, std::shared_ptr<AVBuffer> buffer), ());
    MOCK_METHOD(void, OnError, (MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode), ());
    MOCK_METHOD(void, OnOutputFormatChanged, (const MediaAVCodec::Format &format), ());
    MOCK_METHOD(void, OnOutputBufferAvailable, (uint32_t index, std::shared_ptr<AVBuffer> buffer), ());
    MOCK_METHOD(int32_t, ReleaseOutputBuffer, (uint32_t index, bool render), ());
    MOCK_METHOD(int32_t, ReleaseOutputBuffer, (uint32_t index, bool render, int64_t pts), ());
    MOCK_METHOD(int32_t, RenderOutputBufferAtTime, (uint32_t index, int64_t renderTimestampNs), ());
    MOCK_METHOD(int32_t, RenderOutputBufferAtTime, (uint32_t index, int64_t renderTimestampNs, int64_t pts), ());
    MOCK_METHOD(void, AquireAvailableInputBuffer, (), ());
    MOCK_METHOD(int32_t, SetOutputSurface, (sptr<Surface> videoSurface), ());
    MOCK_METHOD(int32_t, GetOutputFormat, (Format & format), ());
    MOCK_METHOD(void, SetEventReceiver, (const std::shared_ptr<Pipeline::EventReceiver> &receiver), ());
    MOCK_METHOD(int32_t, SetDecryptConfig,
        (const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag), ());
    MOCK_METHOD(void, OnDumpInfo, (int32_t fd), ());
    MOCK_METHOD(
        void, SetCallingInfo, (int32_t appUid, int32_t appPid, const std::string &bundleName, uint64_t instanceId), ());
    MOCK_METHOD(void, ResetRenderTime, (), ());
    MOCK_METHOD(Status, SetPerfRecEnabled, (bool perfRecEnabled), ());
    MOCK_METHOD(bool, IsHwDecoder, (), ());
    MOCK_METHOD(void, NotifyMemoryExchange, (bool exchangeFlag), ());
};
}  // namespace Media
}  // namespace OHOS
#endif  // VIDEO_DECODER_ADAPTER_H
