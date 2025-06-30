/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef SURFACE_DECODER_ADAPTER_UNITTEST_H
#define SURFACE_DECODER_ADAPTER_UNITTEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "mock/avbuffer.h"
#include "surface_decoder_adapter.h"
#include "surface_decoder_adapter.cpp"

namespace OHOS {
namespace Media {
using namespace MediaAVCodec;
class SurfaceDecoderAdapterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    std::shared_ptr<SurfaceDecoderAdapter> adapter_ { nullptr };
};

class MockDecoderAdapterCallback : public DecoderAdapterCallback {
public:
    MOCK_METHOD(void, OnError, (MediaAVCodec::AVCodecErrorType type, int32_t errorCode), (override));
    MOCK_METHOD(void, OnOutputFormatChanged, (const std::shared_ptr<Meta> &format), (override));
    MOCK_METHOD(void, OnBufferEos, (int64_t pts, int64_t frameNum), (override));
};

class MockAVBufferQueue : public AVBufferQueue {
public:
    MOCK_METHOD(std::shared_ptr<AVBufferQueueProducer>, GetLocalProducer, (), (override));
    MOCK_METHOD(std::shared_ptr<AVBufferQueueConsumer>, GetLocalConsumer, (), (override));
    MOCK_METHOD(sptr<AVBufferQueueProducer>, GetProducer, (), (override));
    MOCK_METHOD(sptr<AVBufferQueueConsumer>, GetConsumer, (), (override));
    MOCK_METHOD(sptr<Surface>, GetSurfaceAsProducer, (), (override));
    MOCK_METHOD(sptr<Surface>, GetSurfaceAsConsumer, (), (override));
    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), (override));
    MOCK_METHOD(Status, SetLargerQueueSize, (uint32_t size), (override));
    MOCK_METHOD(bool, IsBufferInQueue, (const std::shared_ptr<AVBuffer>& buffer), (override));
    MOCK_METHOD(Status, Clear, (), (override));
    MOCK_METHOD(Status, ClearBufferIf, (std::function<bool(const std::shared_ptr<AVBuffer>&)> pred), (override));
    MOCK_METHOD(Status, SetQueueSizeAndAttachBuffer,
        (uint32_t size, std::shared_ptr<AVBuffer>& buffer, bool isFilled), (override));
    MOCK_METHOD(uint32_t, GetFilledBufferSize, (), (override));
    MOCK_METHOD(uint32_t, GetMemoryUsage, (), (override));
};

class MockAVBufferQueueProducer : public AVBufferQueueProducer {
public:
    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), (override));
    MOCK_METHOD(Status, RequestBuffer, (std::shared_ptr<AVBuffer>& outBuffer,
                                        const AVBufferConfig& config, int32_t timeoutMs), (override));
    MOCK_METHOD(Status, PushBuffer, (const std::shared_ptr<AVBuffer>& inBuffer, bool available), (override));
    MOCK_METHOD(Status, ReturnBuffer, (const std::shared_ptr<AVBuffer>& inBuffer, bool available), (override));
    MOCK_METHOD(Status, AttachBuffer, (std::shared_ptr<AVBuffer>& inBuffer, bool isFilled), (override));
    MOCK_METHOD(Status, DetachBuffer, (const std::shared_ptr<AVBuffer>& outBuffer), (override));
    MOCK_METHOD(Status, SetBufferFilledListener, (sptr<IBrokerListener>& listener), (override));
    MOCK_METHOD(Status, RemoveBufferFilledListener, (sptr<IBrokerListener>& listener), (override));
    MOCK_METHOD(Status, SetBufferAvailableListener, (sptr<IProducerListener>& listener), (override));
    MOCK_METHOD(Status, Clear, (), (override));
    MOCK_METHOD(Status, ClearBufferIf, (std::function<bool(const std::shared_ptr<AVBuffer>&)> pred), (override));
    MOCK_METHOD(sptr<IRemoteObject>, AsObject, (), (override));
};

class MockAVCodecVideoDecoder : public MediaAVCodec::AVCodecVideoDecoder {
public:
    MOCK_METHOD(int32_t, Configure, (const Format &format), (override));
    MOCK_METHOD(int32_t, Prepare, (), (override));
    MOCK_METHOD(int32_t, Start, (), (override));
    MOCK_METHOD(int32_t, Stop, (), (override));
    MOCK_METHOD(int32_t, Flush, (), (override));
    MOCK_METHOD(int32_t, Reset, (), (override));
    MOCK_METHOD(int32_t, Release, (), (override));
    MOCK_METHOD(int32_t, SetOutputSurface, (sptr<Surface> surface), (override));
    MOCK_METHOD(int32_t, QueueInputBuffer,
        (uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag), (override));
    MOCK_METHOD(int32_t, QueueInputBuffer, (uint32_t index), (override));
    MOCK_METHOD(int32_t, GetOutputFormat, (Format &format), (override));
    MOCK_METHOD(int32_t, ReleaseOutputBuffer, (uint32_t index, bool render), (override));
    MOCK_METHOD(int32_t, RenderOutputBufferAtTime, (uint32_t index, int64_t renderTimestampNs), (override));
    MOCK_METHOD(int32_t, SetParameter, (const Format &format), (override));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<AVCodecCallback> &callback), (override));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaCodecCallback> &callback), (override));
    MOCK_METHOD(int32_t, GetChannelId, (int32_t &channelId), (override));
    MOCK_METHOD(int32_t, SetLowPowerPlayerMode, (bool isLpp), (override));
};
} // namespace Media
} // namespace OHOS
#endif // SURFACE_DECODER_ADAPTER_UNITTEST_H