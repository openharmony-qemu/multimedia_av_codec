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
#ifndef HISTREAMER_AUDIO_DATA_SOURCE_FILTER_UNIT_TEST_H
#define HISTREAMER_AUDIO_DATA_SOURCE_FILTER_UNIT_TEST_H

#include "gtest/gtest.h"
#include "audio_data_source_filter.h"
#include "filter/filter.h"
#include "common/status.h"
#include "osal/task/task.h"
#include "audio_capturer.h"
#include "media_data_source.h"


namespace OHOS {
namespace Media {
namespace Pipeline {
class AudioDataSourceFilterUnitTest : public testing::Test {
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
    std::shared_ptr<AudioDataSourceFilter> audioDataSourceFilter_{ nullptr };
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

class MyIAudioDataSource : public IAudioDataSource {
public:
    ~MyIAudioDataSource() = default;
    int32_t ReadAt(std::shared_ptr<AVBuffer> buffer, uint32_t length)
    {
        if (buffer == nullptr) {
            return  0;
        } else {
            return  1;
        }
    }
    int32_t GetSize(int64_t& size)
    {
        if (size == 0) {
            return 0;
        } else {
            return 1;
        }
    }
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

class TestFilterLinkCallback : public Pipeline::FilterLinkCallback {
public:
    explicit TestFilterLinkCallback()
    {
        std::cout << "filter back constructor" << std::endl;
    }
    void OnLinkedResult(const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnLinkedResult" << std::endl;
    }
    void OnUnlinkedResult(std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnUnlinkedResult" << std::endl;
    }
    void OnUpdatedResult(std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnUpdatedResult" << std::endl;
    }
};

class TestFilterCallback : public FilterCallback {
public:
    explicit TestFilterCallback()
    {
        std::cout << "filter back constructor" << std::endl;
    }

    Status OnCallback(const std::shared_ptr<Filter>& filter,
        FilterCallBackCommand cmd, StreamType outType)

    {
        return Status::OK;
    }
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
#endif  // HISTREAMER_AUDIO_DATA_SOURCE_FILTER_UNIT_TEST_H