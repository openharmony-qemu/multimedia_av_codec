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
 
#ifndef VIDEO_SINK_MOCK_TEST_H
#define VIDEO_SINK_MOCK_TEST_H
 
#include "gtest/gtest.h"
#include "filter/filter.h"
#include "video_sink.h"
#include "sink/media_synchronous_sink.h"
#include "media_sync_center_mock.h"
 
 
namespace OHOS {
namespace Media {
namespace Test {
using namespace testing::ext;
using namespace Pipeline;
 
class MockEventReceiver : public EventReceiver {
public:
    explicit MockEventReceiver() {}
    void OnEvent(const Event& event) override
    {
        EventType eventType = event.type;
        if (eventType >= EventType::EVENT_READY && eventType <= EventType::EVENT_FLV_AUTO_SELECT_BITRATE) {
            lastEventType_ = static_cast<int32_t>(eventType);
        }
    }
    void OnDfxEvent(const DfxEvent& event) override
    {
        DfxEventType eventType = event.type;
        if (eventType >= DfxEventType::DFX_INFO_START && eventType <= DfxEventType::DFX_EVENT_BUTT) {
            lastDfxEventType_ = static_cast<int32_t>(eventType);
        }
    }
    bool CheckLastEventType(int32_t eventType)
    {
        return eventType == lastEventType_;
    }
    bool CheckLastDfxEventType(int32_t eventType)
    {
        return eventType == lastDfxEventType_;
    }
    void ResetEvent()
    {
        lastEventType_ = INVALID_EVENT_TYPE;
        lastDfxEventType_ = INVALID_EVENT_TYPE;
    }
private:
    static constexpr int32_t INVALID_EVENT_TYPE = -1; // default event type is -1, which is invalid
    int32_t lastEventType_ = INVALID_EVENT_TYPE;
    int32_t lastDfxEventType_ = INVALID_EVENT_TYPE;
};
 
class TestVideoSinkMock : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void) { }
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void) { }
    // SetUp: Called before each test cases
    void SetUp(void)
    {
        videoSink_ = std::make_shared<VideoSink>();
        ASSERT_TRUE(videoSink_ != nullptr);
        mockEventReceiver_ = std::make_shared<MockEventReceiver>();
        ASSERT_TRUE(mockEventReceiver_ != nullptr);
        mockSyncCenter_ = std::make_shared<MockMediaSyncCenter>();
        ASSERT_TRUE(mockSyncCenter_ != nullptr);
    }
    // TearDown: Called after each test cases
    void TearDown(void)
    {
        videoSink_ = nullptr;
        mockEventReceiver_ = nullptr;
        mockSyncCenter_ = nullptr;
    }
public:
    std::shared_ptr<VideoSink> videoSink_ = nullptr;
    std::shared_ptr<MockEventReceiver> mockEventReceiver_ = nullptr;
    std::shared_ptr<MockMediaSyncCenter> mockSyncCenter_ = nullptr;
};
 
} // namespace Test
} // namespace Media
} // namespace OHOS
 
#endif // VIDEO_SINK_MOCK_TEST_H