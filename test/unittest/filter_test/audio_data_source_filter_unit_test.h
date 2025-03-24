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
#include "gmock/gmock.h"


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

class MockAVBufferQueueProducer : public IRemoteStub<AVBufferQueueProducer> {
public:
    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), (override));
    MOCK_METHOD(Status, RequestBuffer,
                (std::shared_ptr<AVBuffer> & outBuffer, const AVBufferConfig & config, int32_t timeoutMs), (override));
    MOCK_METHOD(Status, PushBuffer, (const std::shared_ptr<AVBuffer> & inBuffer, bool available), (override));
    MOCK_METHOD(Status, ReturnBuffer, (const std::shared_ptr<AVBuffer> & inBuffer, bool available), (override));
    MOCK_METHOD(Status, AttachBuffer, (std::shared_ptr<AVBuffer> & inBuffer, bool isFilled), (override));
    MOCK_METHOD(Status, DetachBuffer, (const std::shared_ptr<AVBuffer> & outBuffer), (override));
    MOCK_METHOD(Status, SetBufferFilledListener, (sptr<IBrokerListener> & listener), (override));
    MOCK_METHOD(Status, RemoveBufferFilledListener, (sptr<IBrokerListener> & listener), (override));
    MOCK_METHOD(Status, SetBufferAvailableListener, (sptr<IProducerListener> & listener), (override));
    MOCK_METHOD(Status, Clear, (), (override));
    MOCK_METHOD(Status, ClearBufferIf, (std::function<bool(const std::shared_ptr<AVBuffer> &)> pred), (override));
    DECLARE_INTERFACE_DESCRIPTOR(u"Media.MockAVBufferQueueProducer");

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

class MockAudioDataSource : public IAudioDataSource {
public:
    ~MockAudioDataSource() = default;
    MOCK_METHOD(OHOS::Media::AudioDataSourceReadAtActionState, ReadAt,
        (std::shared_ptr<AVBuffer> buffer, uint32_t length), (override));
    MOCK_METHOD(int32_t, GetSize, (int64_t &size), (override));
    MOCK_METHOD(void, SetVideoFirstFramePts, (int64_t firstFramePts), (override));
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