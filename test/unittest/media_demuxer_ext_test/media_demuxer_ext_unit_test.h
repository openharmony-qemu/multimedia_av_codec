/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef MEDIA_DEMUXER_EXT_UNIT_TEST_H
#define MEDIA_DEMUXER_EXT_UNIT_TEST_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "demuxer_plugin_manager.h"
#include "mock/media_sync_manager.h"
#include "mock/avbuffer.h"
#include "mock/sample_queue.h"
#include "demuxer/media_demuxer.cpp"

namespace OHOS {
namespace Media {
class MediaDemuxerExtUnitTest : public testing::Test {
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
    std::shared_ptr<MediaDemuxer> mediaDemuxer_{ nullptr };
    int32_t mockRetInt32_ = 0;
};

class MockAVBufferQueueProducer : public IRemoteStub<AVBufferQueueProducer> {
public:
    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), (override));
    MOCK_METHOD(Status,
                RequestBuffer,
                (std::shared_ptr<AVBuffer> & outBuffer, const AVBufferConfig &config, int32_t timeoutMs),
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

class MockTask : public Task {
public:
    explicit MockTask(
        const std::string& name,
        const std::string& groupId = "",
        TaskType type = TaskType::SINGLETON,
        TaskPriority priority = TaskPriority::NORMAL,
        bool singleLoop = true
    ) : Task(name, groupId, type, priority, singleLoop) {}

    ~MockTask()  = default;

    MOCK_METHOD(void, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(void, StopAsync, (), ());
    MOCK_METHOD(void, Pause, (), ());
    MOCK_METHOD(void, PauseAsync, (), ());
    MOCK_METHOD(void, RegisterJob, (const std::function<int64_t()>& job), ());
    MOCK_METHOD(void, SubmitJobOnce, (const std::function<void()>& job, int64_t delayUs, bool wait), ());
    MOCK_METHOD(void, SubmitJob, (const std::function<void()>& job, int64_t delayUs, bool wait), ());
    MOCK_METHOD(bool, IsTaskRunning, (), (override));
    MOCK_METHOD(void, UpdateDelayTime, (int64_t delayUs), ());
    MOCK_METHOD(void, UpdateThreadPriority, (const uint32_t newPriority, const std::string& strBundleName), ());
};

class MockEventReceiver : public Pipeline::EventReceiver {
public:
    explicit MockEventReceiver() {}
    ~MockEventReceiver() = default;
    MOCK_METHOD(void, OnEvent, (const Event&), (override));
};
}
}

#endif
