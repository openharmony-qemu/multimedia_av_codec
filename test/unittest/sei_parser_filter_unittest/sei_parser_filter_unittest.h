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
#ifndef SEI_PARSER_FILTER_UNITTEST_H
#define SEI_PARSER_FILTER_UNITTEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "mock/sei_parser_helper.h"
#include "sei_parser_filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class SeiParserFilterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    std::shared_ptr<SeiParserFilter> seiParserFilter_;
};

class MockFilterLinkCallback : public FilterLinkCallback {
public:
    MOCK_METHOD(void, OnLinkedResult,
        (const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta), (override));
    MOCK_METHOD(void, OnUnlinkedResult, (std::shared_ptr<Meta>& meta), (override));
    MOCK_METHOD(void, OnUpdatedResult, (std::shared_ptr<Meta>& meta), (override));
};

class MockAVBufferQueueConsumer : public AVBufferQueueConsumer {
public:
    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), (override));
    MOCK_METHOD(bool, IsBufferInQueue, (const std::shared_ptr<AVBuffer>& buffer), (override));
    MOCK_METHOD(Status, AcquireBuffer, (std::shared_ptr<AVBuffer>& outBuffer), (override));
    MOCK_METHOD(Status, ReleaseBuffer, (const std::shared_ptr<AVBuffer>& inBuffer), (override));
    MOCK_METHOD(Status, AttachBuffer, (std::shared_ptr<AVBuffer>& inBuffer, bool isFilled), (override));
    MOCK_METHOD(Status, DetachBuffer, (const std::shared_ptr<AVBuffer>& outBuffer), (override));
    MOCK_METHOD(Status, SetBufferAvailableListener, (sptr<IConsumerListener>& listener), (override));
    MOCK_METHOD(Status, SetQueueSizeAndAttachBuffer,
        (uint32_t size, std::shared_ptr<AVBuffer>& buffer, bool isFilled), (override));
    MOCK_METHOD(uint32_t, GetFilledBufferSize, (), (override));
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

class MockMediaSyncCenter : public IMediaSyncCenter {
public:
    virtual ~MockMediaSyncCenter() = default;
    MOCK_METHOD(Status, Reset, (), (override));
    MOCK_METHOD(void, AddSynchronizer, (IMediaSynchronizer* syncer), (override));
    MOCK_METHOD(void, RemoveSynchronizer, (IMediaSynchronizer* syncer), (override));
    bool UpdateTimeAnchor(int64_t clockTime, int64_t delayTime, IMediaTime iMediaTime, 
        IMediaSynchronizer* supplier) override { return true; }
    MOCK_METHOD(bool, UpdateTimeAnchor_Forward, 
                (int64_t clockTime, int64_t delayTime, IMediaTime iMediaTime, IMediaSynchronizer* supplier));
    MOCK_METHOD(int64_t, GetMediaTimeNow, (), (override));
    MOCK_METHOD(int64_t, GetClockTimeNow, (), (override));
    MOCK_METHOD(int64_t, GetAnchoredClockTime, (int64_t mediaTime), (override));
    MOCK_METHOD(void, ReportPrerolled, (IMediaSynchronizer* supplier), (override));
    MOCK_METHOD(void, ReportEos, (IMediaSynchronizer* supplier), (override));
    MOCK_METHOD(void, SetMediaTimeRangeStart, (int64_t startMediaTime, int32_t trackId, IMediaSynchronizer* supplier), (override));
    MOCK_METHOD(void, SetMediaTimeRangeEnd, (int64_t endMediaTime, int32_t trackId, IMediaSynchronizer* supplier), (override));
    MOCK_METHOD(int64_t, GetSeekTime, (), (override));
    MOCK_METHOD(Status, SetPlaybackRate, (float rate), (override));
    MOCK_METHOD(float, GetPlaybackRate, (), (override));
    MOCK_METHOD(void, SetMediaStartPts, (int64_t startPts), (override));
    MOCK_METHOD(void, ResetMediaStartPts, (), (override));
    MOCK_METHOD(int64_t, GetMediaStartPts, (), (override));
    MOCK_METHOD(void, SetLastAudioBufferDuration, (int64_t durationUs), (override));
    MOCK_METHOD(void, SetLastVideoBufferPts, (int64_t bufferPts), (override));
    MOCK_METHOD(void, SetLastVideoBufferAbsPts, (int64_t lastVideoBufferAbsPts), (override));
    MOCK_METHOD(double, GetInitialVideoFrameRate, (), (override));
    MOCK_METHOD(int64_t, GetLastVideoBufferAbsPts, (), (const, override));
};

class MockEventReceiver : public EventReceiver {
public:
    MOCK_METHOD(void, OnEvent, (const Event& event), (override));
    MOCK_METHOD(void, OnDfxEvent, (const DfxEvent& event), (override));
    MOCK_METHOD(void, NotifyRelease, (), (override));
    MOCK_METHOD(void, OnMemoryUsageEvent, (const DfxEvent& event), (override));
};

} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // SEI_PARSER_FILTER_UNITTEST_H