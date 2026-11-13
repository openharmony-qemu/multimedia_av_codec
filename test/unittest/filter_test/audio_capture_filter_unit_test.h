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
namespace AudioStandard {
// Fake AudioCapturer: AudioCaptureModule methods are non-virtual, so the only way
// to exercise the filter's success-path branches (Start/Prepare/Read/GetAudioTime OK)
// is to inject a capturer whose virtual methods return success states.
class FakeAudioCapturer : public AudioCapturer {
public:
    CapturerState status_ = CapturerState::CAPTURER_RUNNING;
    int64_t timestamp_ = 20000000; // ns; 20ms yields audioDataTime==0 after first-frame subtract
    int32_t readResult_ = 16;      // bytes read; >=0 means success
    size_t fakeBufferSize_ = 16;

    CapturerState GetStatus() const override { return status_; }
    bool Start() override { return true; }
    bool Stop() const override { return true; }
    bool Release() override { return true; }
    bool Pause() const override { return true; }
    bool Flush() const override { return true; }
    int32_t Read(uint8_t &buffer, size_t userSize, bool isBlockingRead) override
    {
        (void)buffer; (void)userSize; (void)isBlockingRead;
        return readResult_;
    }
    int32_t GetBufferSize(size_t &bufferSize) const override { bufferSize = fakeBufferSize_; return 0; }
    bool GetTimeStampInfo(Timestamp &timestamp, Timestamp::Timestampbase base) const override
    {
        (void)base;
        timestamp.time.tv_sec = timestamp_ / 1000000000;
        timestamp.time.tv_nsec = timestamp_ % 1000000000;
        return true;
    }
    bool GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base) const override
    {
        return GetTimeStampInfo(timestamp, base);
    }
    int32_t GetAudioTimestampInfo(Timestamp &timestamp, Timestamp::Timestampbase base) const override
    {
        return GetTimeStampInfo(timestamp, base) ? 0 : -1;
    }
    int32_t SetCapturerCallback(const std::shared_ptr<AudioCapturerCallback> &callback) override
    {
        (void)callback; return 0;
    }
    int32_t SetAudioCapturerInfoChangeCallback(
        const std::shared_ptr<AudioCapturerInfoChangeCallback> &callback) override
    {
        (void)callback; return 0;
    }
    int32_t RemoveAudioCapturerInfoChangeCallback(
        const std::shared_ptr<AudioCapturerInfoChangeCallback> &callback) override
    {
        (void)callback; return 0;
    }
    int32_t GetCurrentCapturerChangeInfo(AudioCapturerChangeInfo &changeInfo) const override
    {
        (void)changeInfo; return 0;
    }
    int32_t SetInterruptStrategy(InterruptStrategy strategy) override { (void)strategy; return 0; }

    int32_t SetParams(const AudioCapturerParams params) override { (void)params; return 0; }
    int32_t UpdatePlaybackCaptureConfig(const AudioPlaybackCaptureConfig &config) override
    {
        (void)config; return 0;
    }
    void SetAudioCapturerErrorCallback(std::shared_ptr<AudioCapturerErrorCallback> errorCallback) override
    {
        (void)errorCallback;
    }
    void SetFastStatusChangeCallback(
        const std::shared_ptr<AudioCapturerFastStatusChangeCallback> &callback) override
    {
        (void)callback;
    }
    int32_t GetParams(AudioCapturerParams &params) const override { (void)params; return 0; }
    int32_t GetCapturerInfo(AudioCapturerInfo &capturerInfo) const override { (void)capturerInfo; return 0; }
    int32_t GetStreamInfo(AudioStreamInfo &streamInfo) const override { (void)streamInfo; return 0; }
    int32_t GetAudioStreamId(uint32_t &sessionID) const override { (void)sessionID; return 0; }
    int32_t GetFrameCount(uint32_t &frameCount) const override { (void)frameCount; return 0; }
    int32_t SetCapturerPositionCallback(int64_t markPosition,
        const std::shared_ptr<CapturerPositionCallback> &callback) override
    {
        (void)markPosition; (void)callback; return 0;
    }
    void UnsetCapturerPositionCallback() override {}
    int32_t SetCapturerPeriodPositionCallback(int64_t frameNumber,
        const std::shared_ptr<CapturerPeriodPositionCallback> &callback) override
    {
        (void)frameNumber; (void)callback; return 0;
    }
    void UnsetCapturerPeriodPositionCallback() override {}
    int32_t RegisterAudioPolicyServerDiedCb(const int32_t clientPid,
        const std::shared_ptr<AudioCapturerPolicyServiceDiedCallback> &callback) override
    {
        (void)clientPid; (void)callback; return 0;
    }
    int32_t SetBufferDuration(uint64_t bufferDuration) const override { (void)bufferDuration; return 0; }
    int32_t SetCaptureMode(AudioCaptureMode captureMode) override { (void)captureMode; return 0; }
    AudioCaptureMode GetCaptureMode() const override { return CAPTURE_MODE_NORMAL; }
    int32_t SetCapturerReadCallback(const std::shared_ptr<AudioCapturerReadCallback> &callback) override
    {
        (void)callback; return 0;
    }
    int32_t GetBufferDesc(BufferDesc &bufDesc) override { (void)bufDesc; return 0; }
    int32_t Enqueue(const BufferDesc &bufDesc) override { (void)bufDesc; return 0; }
    int32_t Clear() const override { return 0; }
    int32_t GetBufQueueState(BufferQueueState &bufState) const override { (void)bufState; return 0; }
    void SetValid(bool valid) override { (void)valid; }
    int64_t GetFramesRead() const override { return 0; }
    int32_t SetAudioCapturerDeviceChangeCallback(
        const std::shared_ptr<AudioCapturerDeviceChangeCallback> &callback) override
    {
        (void)callback; return 0;
    }
    int32_t RemoveAudioCapturerDeviceChangeCallback(
        const std::shared_ptr<AudioCapturerDeviceChangeCallback> &callback) override
    {
        (void)callback; return 0;
    }
    int32_t RegisterAudioCapturerEventListener() override { return 0; }
    int32_t UnregisterAudioCapturerEventListener() override { return 0; }
    int32_t GetCurrentInputDevices(AudioDeviceDescriptor &deviceInfo) const override
    {
        (void)deviceInfo; return 0;
    }
    std::vector<sptr<MicrophoneDescriptor>> GetCurrentMicrophones() const override { return {}; }
    uint32_t GetOverflowCount() const override { return 0; }
    int32_t SetInputDevice(DeviceType deviceType) const override { (void)deviceType; return 0; }
    int32_t SetAudioSourceConcurrency(const std::vector<SourceType> &targetSources) override
    {
        (void)targetSources; return 0;
    }
};
}  // namespace AudioStandard
namespace Media {
namespace Pipeline {
// FakeTask: Task::Start() spawns an ffrt::thread running ReadLoop, which is unsafe on
// the device. Override Start/Stop/Pause as no-ops so DoStart/DoResume's taskPtr_->Start()
// call is exercised without spawning a thread.
class FakeTask : public Task {
public:
    FakeTask() : Task("FakeTask") {}
    void Start() override { startCalled_ = true; }
    void Stop() override {}
    void StopAsync() override {}
    void Pause() override {}
    void PauseAsync() override {}
    bool startCalled_ = false;
};

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

class FailingMockOutputBufferQueue : public MockOutputBufferQueue {
public:
    Status RequestBuffer(std::shared_ptr<AVBuffer>& outBuffer,
                         const AVBufferConfig& config, int32_t timeoutMs) override
    {
        return Status::ERROR_INVALID_OPERATION;
    }
};

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_AUDIO_CAPTURE_FILTER_UNIT_TEST_H