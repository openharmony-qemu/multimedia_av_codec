/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef METADATA_FILTER_UNIT_TEST_H
#define METADATA_FILTER_UNIT_TEST_H

#include <cstring>
#include "gtest/gtest.h"
#include <gmock/gmock.h>

#include "filter/filter.h"
#include "surface.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "common/status.h"
#include "consumer_surface.h"


namespace OHOS {
namespace Media {

class MetaDataFilterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);

protected:
    std::shared_ptr<Pipeline::MetaDataFilter> metaData_{ nullptr };
};

class TestEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TestEventReceiver()
    {
        std::cout << "event receiver constructor" << std::endl;
    }

    void OnEvent(const Event &event)
    {
        std::cout << event.srcFilter << std::endl;
    }
};

class TestFilterCallback : public Pipeline::FilterCallback {
public:
    explicit TestFilterCallback()
    {
        std::cout << "filter back constructor" << std::endl;
    }

    Status OnCallback(const std::shared_ptr<Pipeline::Filter>& filter,
        Pipeline::FilterCallBackCommand cmd, Pipeline::StreamType outType)
    {
        return Status::OK;
    }
};

class MyAVBufferQueueProducer : public IRemoteStub<AVBufferQueueProducer> {
public:
    uint32_t GetQueueSize()
    {
        return 0;
    }
    Status SetQueueSize(uint32_t size)
    {
        return  Status::OK;
    }

    Status RequestBuffer(std::shared_ptr<AVBuffer>& outBuffer,
                                 const AVBufferConfig& config, int32_t timeoutMs)
    {
        if (outBuffer == nullptr) {
            return  Status::ERROR_NULL_POINTER;
        } else {
            return  Status::OK;
        }
    }
    Status RequestBufferWaitUs(std::shared_ptr<AVBuffer>& outBuffer,
                                 const AVBufferConfig& config, int64_t timeoutUs)
    {
        if (outBuffer == nullptr) {
            return  Status::ERROR_NULL_POINTER;
        } else {
            return  Status::OK;
        }
    }
    Status PushBuffer(const std::shared_ptr<AVBuffer>& inBuffer, bool available)
    {
        return  Status::OK;
    }
    Status ReturnBuffer(const std::shared_ptr<AVBuffer>& inBuffer, bool available)
    {
        return  Status::OK;
    }

    Status AttachBuffer(std::shared_ptr<AVBuffer>& inBuffer, bool isFilled)
    {
        return  Status::OK;
    }
    Status DetachBuffer(const std::shared_ptr<AVBuffer>& outBuffer)
    {
        return  Status::OK;
    }

    Status SetBufferFilledListener(sptr<IBrokerListener>& listener)
    {
        return  Status::OK;
    }
    Status RemoveBufferFilledListener(sptr<IBrokerListener>& listener)
    {
        return  Status::OK;
    }
    Status SetBufferAvailableListener(sptr<IProducerListener>& listener)
    {
        return  Status::OK;
    }
    Status Clear()
    {
        return  Status::OK;
    }
    Status ClearBufferIf(std::function<bool(const std::shared_ptr<AVBuffer> &)> pred)
    {
        return  Status::OK;
    }
    DECLARE_INTERFACE_DESCRIPTOR(u"Media.MyAVBufferQueueProducer");

protected:
    enum: uint32_t {
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

class TestFilter : public Pipeline::Filter {
public:
    TestFilter() : Filter("TestFilter", Pipeline::FilterType::FILTERTYPE_SOURCE) {}
    ~TestFilter() = default;
    Status OnLinked(Pipeline::StreamType inType,
                    const std::shared_ptr<Meta> &meta,
                    const std::shared_ptr<Pipeline::FilterLinkCallback> &callback)
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
}
}

#endif