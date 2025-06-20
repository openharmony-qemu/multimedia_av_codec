/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef HISTREAMER_MUXER_FILTER_UNIT_TEST_H
#define HISTREAMER_MUXER_FILTER_UNIT_TEST_H

#include <cstring>
#include <map>
#include "gtest/gtest.h"
#include "muxer_filter.h"
#include "filter/filter.h"
#include "common/status.h"
#include "meta/media_types.h"
#include "muxer/media_muxer.h"
#include "gmock/gmock.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class MuxerFilterUnitTest : public testing::Test {
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
    std::shared_ptr<MuxerFilter> muxerFilter_{ nullptr };
};

class MockMediaMuxer : public MediaMuxer {
public:
    MockMediaMuxer(int32_t appUid, int32_t appPid) : MediaMuxer(appUid, appPid) {}
    MOCK_METHOD(Status, Start, (), (override));
    MOCK_METHOD(Status, Stop, (), (override));
};

class MyFilterLinkCallback : public FilterLinkCallback {
public:
    ~MyFilterLinkCallback() = default;
    void OnLinkedResult(const sptr<AVBufferQueueProducer>& queue,
                                        std::shared_ptr<Meta>& meta)
    {
        return;
    }
    void OnUnlinkedResult(std::shared_ptr<Meta>& meta)
    {
        return;
    }
    void OnUpdatedResult(std::shared_ptr<Meta>& meta)
    {
        return;
    }
};
class MyEventReceiver : public EventReceiver {
public:
    ~MyEventReceiver() = default;
    void OnEvent(const Event& event)
    {
        return;
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
        return  Status::OK;
    }
    Status RequestBufferWaitUs(std::shared_ptr<AVBuffer>& outBuffer,
                                 const AVBufferConfig& config, int64_t timeoutUs)
    {
        return  Status::OK;
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

class TestFilter : public Filter {
public:
    TestFilter():Filter("TestFilter", FilterType::FILTERTYPE_SOURCE) {}
    ~TestFilter() = default;
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta>& meta,
                            const std::shared_ptr<FilterLinkCallback>& callback)
    {
        (void)inType;
        (void)meta;
        (void)callback;
        return onLinked_;
    }
protected:
    Status onLinked_;
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_MUXER_FILTER_UNIT_TEST_H