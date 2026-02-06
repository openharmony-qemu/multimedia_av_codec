/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#ifndef HISTREAMER_VIDEO_CAPTURE_FILTER_UNIT_TEST_H
#define HISTREAMER_VIDEO_CAPTURE_FILTER_UNIT_TEST_H

#include <gmock/gmock.h>
#include "consumer_surface.h"
#include "gtest/gtest.h"
#include "video_capture_filter.h"
#include "buffer_producer_listener.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class VideoCaptureFilterUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

protected:
    std::shared_ptr<VideoCaptureFilter> videoCaptureFilter_{ nullptr };
};

class MockAVBufferQueueProducer : public IRemoteStub<AVBufferQueueProducer> {
public:
    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), (override));
    MOCK_METHOD(Status,
                RequestBuffer,
                (std::shared_ptr<AVBuffer> & outBuffer, const AVBufferConfig &config, int32_t timeoutMs),
                (override));
    MOCK_METHOD(Status,
                RequestBufferWaitUs,
                (std::shared_ptr<AVBuffer> & outBuffer, const AVBufferConfig &config, int64_t timeoutUs),
                (override));
    MOCK_METHOD(Status, PushBuffer, (const std::shared_ptr<AVBuffer> &inBuffer, bool available), (override));
    MOCK_METHOD(Status, ReturnBuffer, (const std::shared_ptr<AVBuffer> &inBuffer, bool available), (override));
    MOCK_METHOD(Status, AttachBuffer, (std::shared_ptr<AVBuffer> & inBuffer, bool isFilled), (override));
    MOCK_METHOD(Status, DetachBuffer, (const std::shared_ptr<AVBuffer> &outBuffer), (override));
    MOCK_METHOD(Status, SetBufferFilledListener, (sptr<IBrokerListener> & listener), (override));
    MOCK_METHOD(Status, RemoveBufferFilledListener, (sptr<IBrokerListener> & listener), (override));
    MOCK_METHOD(Status, SetBufferAvailableListener, (sptr<IProducerListener> & listener), (override));
    MOCK_METHOD(Status, Clear, (), (override));
    MOCK_METHOD(Status, ClearBufferIf, (std::function<bool(const std::shared_ptr<AVBuffer> &)> pred), (override));
    DECLARE_INTERFACE_DESCRIPTOR(u"Media.MyAVBufferQueueProducer");

protected:
    enum : uint32_t {
        PRODUCER_GET_QUEUE_SIZE = 0,
        PRODUCER_SET_QUEUE_SIZE = 1,
        PRODUCER_REQUEST_BUFFER = 2,
        PRODUCER_PUSH_BUFFER = 3,
        PRODUCER_RETURN_BUFFER = 4,
        PRODUCER_ATTACH_BUFFER = 5,
        PRODUCER_DETACH_BUFFER = 6,
        PRODUCER_SET_FILLED_LISTENER = 7,
        PRODUCER_REMOVE_FILLED_LISTENER = 8,
        PRODUCER_SET_AVAILABLE_LISTENER = 9
    };
};

class TestEventReceiver : public Pipeline::EventReceiver {
public:
    ~TestEventReceiver() = default;

    void OnEvent(const Event &event)
    {
        return;
    }

private:
};

class TestFilterCallback : public FilterCallback {
public:
    explicit TestFilterCallback()
    {
        std::cout << "filter back constructor" << std::endl;
    }

    Status OnCallback(const std::shared_ptr<Filter> &filter, FilterCallBackCommand cmd, StreamType outType)

    {
        return Status::OK;
    }
};

class TestFilterLinkCallback : public FilterLinkCallback {
public:
    ~TestFilterLinkCallback() = default;
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta)
    {
        return;
    }
    void OnUnlinkedResult(std::shared_ptr<Meta> &meta)
    {
        return;
    }
    void OnUpdatedResult(std::shared_ptr<Meta> &meta)
    {
        return;
    }
};

class TestFilter : public Filter {
public:
    TestFilter() : Filter("TestFilter", FilterType::FILTERTYPE_SOURCE) {}
    ~TestFilter() = default;
    Status OnLinked(StreamType inType,
                    const std::shared_ptr<Meta> &meta,
                    const std::shared_ptr<FilterLinkCallback> &callback)
    {
        (void)inType;
        (void)meta;
        (void)callback;
        return onLinked_;
    }

protected:
    Status onLinked_;
};

class MockConsumerSurface : public ConsumerSurface {
public:
    explicit MockConsumerSurface(const std::string &name) : ConsumerSurface(name) {}
    static sptr<MockConsumerSurface> CreateSurfaceAsConsumer(std::string name);
    MOCK_METHOD(GSError,
                AcquireBuffer,
                (sptr<SurfaceBuffer> & buffer, sptr<SyncFence> &fence, int64_t &timestamp, Rect &damage),
                (override));
    MOCK_METHOD(GSError, ReleaseBuffer, (sptr<SurfaceBuffer> & buffer, int32_t fence), (override));

private:
    std::map<std::string, std::string> userData_;
    sptr<BufferQueueProducer> producer_ = nullptr;
    sptr<BufferQueueConsumer> consumer_ = nullptr;
    std::string name_ = "not init";
    std::map<std::string, OnUserDataChangeFunc> onUserDataChange_;
    std::mutex lockMutex_;
    uint64_t uniqueId_ = 0;
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_VIDEO_CAPTURE_FILTER_UNIT_TEST_H