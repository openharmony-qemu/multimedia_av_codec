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
#ifndef HISTREAMER_AUDIO_CAPTURE_FILTER_UNIT_TEST_H
#define HISTREAMER_AUDIO_CAPTURE_FILTER_UNIT_TEST_H

#include "gtest/gtest.h"
#include "audio_capture_filter.h"
#include "filter/filter.h"
#include "audio_capture_module.h"
#include "avbuffer_queue_producer.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class AudioCaptureFilterUnitTest : public testing::Test {
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
    std::shared_ptr<AudioCaptureFilter> audioCaptureFilter_{ nullptr };
};

class TestEventReceiver : public Pipeline::EventReceiver {
public:
    ~TestEventReceiver() = default;

    void OnEvent(const Event &event)
    {
        return;
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

class MockAudioCapturerInfoChangeCallback : public OHOS::AudioStandard::AudioCapturerInfoChangeCallback {
public:
    void OnStateChange(const OHOS::AudioStandard::AudioCapturerChangeInfo &captureChangeInfo) override
    {
        std::cout << "Mock OnstateChange called" << std::endl;
    }
};

class MockAudioCaptureModule : public AudioCaptureModule::AudioCaptureModule {
public:
    Status Start()
    {
        return Status::OK;
    }

    Status Prepare()
    {
        return Status::ERROR_UNKNOWN;
    }
};

class MockOutputBufferQueue : public OHOS::Media::AVBufferQueueProducer {
public:

    uint32_t GetQueueSize() override
    {
        return 0;
    }

    Status SetQueueSize(uint32_t size) override
    {
        return Status::OK;
    }

    Status RequestBuffer(std::shared_ptr<AVBuffer>& outBuffer,
                         const AVBufferConfig& config, int32_t timeoutMs) override
    {
        outBuffer = std::make_shared<AVBuffer>();
        return Status::OK;
    }

    Status RequestBufferWaitUs(std::shared_ptr<AVBuffer>& outBuffer,
                         const AVBufferConfig& config, int64_t timeoutUs) override
    {
        outBuffer = std::make_shared<AVBuffer>();
        return Status::OK;
    }

    Status PushBuffer(const std::shared_ptr<AVBuffer>& inBuffer, bool available) override
    {
        return Status::OK;
    }

    Status ReturnBuffer(const std::shared_ptr<AVBuffer>& inBuffer, bool available) override
    {
        return Status::OK;
    }

    Status AttachBuffer(std::shared_ptr<AVBuffer>& inBuffer, bool isFilled) override
    {
        return Status::OK;
    }

    Status DetachBuffer(const std::shared_ptr<AVBuffer>& outBuffer) override
    {
        return Status::OK;
    }

    Status SetBufferFilledListener(sptr<IBrokerListener>& listener) override
    {
        return Status::OK;
    }

    Status RemoveBufferFilledListener(sptr<IBrokerListener>& listener) override
    {
        return Status::OK;
    }

    Status SetBufferAvailableListener(sptr<IProducerListener>& listener) override
    {
        return Status::OK;
    }

    Status ClearBufferIf(std::function<bool(const std::shared_ptr<AVBuffer> &)> pred)
    {
        return  Status::OK;
    }

    Status Clear() override
    {
        return Status::OK;
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_AUDIO_CAPTURE_FILTER_UNIT_TEST_H