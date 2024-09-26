/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "audio_sink.h"
#include "audio_effect.h"
#include "filter/filter.h"
#include "common/log.h"
#include "sink/media_synchronous_sink.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_ONLY_PRERELEASE, LOG_DOMAIN_SYSTEM_PLAYER, "AudioSinkTest" };
constexpr int64_t MAX_BUFFER_DURATION_US = 200000; // Max buffer duration is 200 ms
}

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Test {

class TestEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TestEventReceiver()
    {
        MEDIA_LOG_I("TestEventReceiver ctor ");
    }

    void OnEvent(const Event &event)
    {
        MEDIA_LOG_I("TestEventReceiver OnEvent " PUBLIC_LOG_S, event.srcFilter.c_str());
    }

private:
};

class TestAudioSinkMock : public AudioSinkPlugin {
public:

    explicit TestAudioSinkMock(std::string name): AudioSinkPlugin(std::move(name)) {}

    Status Start() override
    {
        return Status::ERROR_UNKNOWN;
    };

    Status Stop() override
    {
        return Status::ERROR_UNKNOWN;
    };

    Status PauseTransitent() override
    {
        return Status::ERROR_UNKNOWN;
    };

    Status Pause() override
    {
        return Status::ERROR_UNKNOWN;
    };

    Status Resume() override
    {
        return Status::ERROR_UNKNOWN;
    };

    Status GetLatency(uint64_t &hstTime) override
    {
        (void)hstTime;
        return Status::ERROR_UNKNOWN;
    };

    Status SetAudioEffectMode(int32_t effectMode) override
    {
        (void)effectMode;
        return Status::ERROR_UNKNOWN;
    };

    Status GetAudioEffectMode(int32_t &effectMode) override
    {
        (void)effectMode;
        return Status::ERROR_UNKNOWN;
    };

    int64_t GetPlayedOutDurationUs(int64_t nowUs) override
    {
        (void)nowUs;
        return 0;
    }
    Status GetMute(bool& mute) override
    {
        (void)mute;
        return Status::ERROR_UNKNOWN;
    }
    Status SetMute(bool mute) override
    {
        (void)mute;
        return Status::ERROR_UNKNOWN;
    }

    Status GetVolume(float& volume) override
    {
        (void)volume;
        return Status::ERROR_UNKNOWN;
    }
    Status SetVolume(float volume) override
    {
        (void)volume;
        return Status::ERROR_UNKNOWN;
    }
    Status GetSpeed(float& speed) override
    {
        (void)speed;
        return Status::ERROR_UNKNOWN;
    }
    Status SetSpeed(float speed) override
    {
        (void)speed;
        return Status::ERROR_UNKNOWN;
    }
    Status GetFrameSize(size_t& size) override
    {
        (void)size;
        return Status::ERROR_UNKNOWN;
    }
    Status GetFrameCount(uint32_t& count) override
    {
        (void)count;
        return Status::ERROR_UNKNOWN;
    }
    Status Write(const std::shared_ptr<AVBuffer>& input) override
    {
        (void)input;
        return Status::ERROR_UNKNOWN;
    }
    Status Flush() override
    {
        return Status::ERROR_UNKNOWN;
    }
    Status Drain() override
    {
        return Status::ERROR_UNKNOWN;
    }
    Status GetFramePosition(int32_t &framePosition) override
    {
        (void)framePosition;
        return Status::ERROR_UNKNOWN;
    }
    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver) override
    {
        (void)receiver;
    }
    int32_t SetVolumeWithRamp(float targetVolume, int32_t duration) override
    {
        (void)targetVolume;
        (void)duration;
        return 0;
    }
    Status SetMuted(bool isMuted) override
    {
        (void)isMuted;
        return Status::OK;
    }
};

std::shared_ptr<AudioSink> AudioSinkCreate()
{
    auto audioSink = std::make_shared<AudioSink>();
    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    auto meta = std::make_shared<Meta>();
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    if (initStatus == Status::OK) {
        return audioSink;
    } else {
        return nullptr;
    }
}

HWTEST(TestAudioSink, find_audio_sink_process, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    auto preStatus = audioSink->Prepare();
    ASSERT_TRUE(preStatus == Status::OK);
    auto startStatus = audioSink->Start();
    ASSERT_TRUE(startStatus == Status::OK);
    auto pauseStatus = audioSink->Pause();
    ASSERT_TRUE(pauseStatus == Status::OK);
    auto stopStatus = audioSink->Stop();
    ASSERT_TRUE(stopStatus == Status::OK);
    auto flushStatus = audioSink->Flush();
    ASSERT_TRUE(flushStatus == Status::OK);
    auto resumeStatus = audioSink->Resume();
    ASSERT_TRUE(resumeStatus == Status::OK);
    auto freeStatus = audioSink->Release();
    ASSERT_TRUE(freeStatus == Status::OK);
}

HWTEST(TestAudioSink, find_audio_sink_set_volume, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    float volume = 0.5f;
    auto setVolumeStatus =  audioSink->SetVolume(volume);
    ASSERT_TRUE(setVolumeStatus == Status::OK);

    // SetVolumeWithRamp
    float targetVolume = 0;
    int32_t duration = 0;
    auto setVolumeWithRampStatus = audioSink->SetVolumeWithRamp(targetVolume, duration);
    ASSERT_TRUE(setVolumeWithRampStatus == 0);
}

HWTEST(TestAudioSink, find_audio_sink_set_volume002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    float volume = -0.5f;
    auto setVolumeStatus =  audioSink->SetVolume(volume);
    ASSERT_TRUE(setVolumeStatus != Status::OK);

    // SetVolumeWithRamp
    float targetVolume = 0;
    int32_t duration = 0;
    auto setVolumeWithRampStatus = audioSink->SetVolumeWithRamp(targetVolume, duration);
    ASSERT_TRUE(setVolumeWithRampStatus == 0);
}

HWTEST(TestAudioSink, find_audio_sink_set_volume003, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = audioSink->CreatePlugin();
    float volume = 0.5f;
    auto setVolumeStatus =  audioSink->SetVolume(volume);
    ASSERT_TRUE(setVolumeStatus != Status::OK);

    // SetVolumeWithRamp
    float targetVolume = 0;
    int32_t duration = 0;
    auto setVolumeWithRampStatus = audioSink->SetVolumeWithRamp(targetVolume, duration);
    ASSERT_TRUE(setVolumeWithRampStatus == 0);
}

HWTEST(TestAudioSink, find_audio_sink_set_volume004, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = audioSink->CreatePlugin();
    float volume = -0.5f;
    auto setVolumeStatus =  audioSink->SetVolume(volume);
    ASSERT_TRUE(setVolumeStatus != Status::OK);

    // SetVolumeWithRamp
    float targetVolume = 0;
    int32_t duration = 0;
    auto setVolumeWithRampStatus = audioSink->SetVolumeWithRamp(targetVolume, duration);
    ASSERT_TRUE(setVolumeWithRampStatus == 0);
}

HWTEST(TestAudioSink, find_audio_sink_set_volume005, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = nullptr;
    float volume = -0.5f;
    auto setVolumeStatus =  audioSink->SetVolume(volume);
    ASSERT_TRUE(setVolumeStatus == Status::ERROR_NULL_POINTER);
}

HWTEST(TestAudioSink, find_audio_sink_set_sync_center, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    float volume = 0.5f;
    auto setVolumeStatus =  audioSink->SetVolume(volume);
    ASSERT_TRUE(setVolumeStatus == Status::OK);

    // SetVolumeWithRamp
    float targetVolume = 0;
    int32_t duration = 0;
    auto setVolumeWithRampStatus = audioSink->SetVolumeWithRamp(targetVolume, duration);
    ASSERT_TRUE(setVolumeWithRampStatus == 0);

    // SetSyncCenter
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    audioSink->SetSyncCenter(syncCenter);
}

HWTEST(TestAudioSink, find_audio_sink_set_speed, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    float speed = 1.0f;
    auto setVolumeStatus =  audioSink->SetSpeed(speed);
    ASSERT_TRUE(setVolumeStatus == Status::OK);
}

HWTEST(TestAudioSink, find_audio_sink_set_speed002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    float speed = -1.0f;
    auto setVolumeStatus =  audioSink->SetSpeed(speed);
    ASSERT_TRUE(setVolumeStatus != Status::OK);
}

HWTEST(TestAudioSink, find_audio_sink_set_speed003, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = nullptr;
    float speed = 1.0f;
    auto setVolumeStatus =  audioSink->SetSpeed(speed);
    ASSERT_TRUE(setVolumeStatus != Status::OK);
}

HWTEST(TestAudioSink, find_audio_sink_set_speed004, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = nullptr;
    float speed = -1.0f;
    auto setVolumeStatus =  audioSink->SetSpeed(speed);
    ASSERT_TRUE(setVolumeStatus != Status::OK);
}

HWTEST(TestAudioSink, find_audio_sink_audio_effect, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    auto setEffectStatus =  audioSink->SetAudioEffectMode(AudioStandard::EFFECT_NONE);
    ASSERT_TRUE(setEffectStatus == Status::OK);
    int32_t audioEffectMode;
    auto getEffectStatus =  audioSink->GetAudioEffectMode(audioEffectMode);
    ASSERT_TRUE(getEffectStatus == Status::OK);
}

HWTEST(TestAudioSink, find_audio_sink_audio_effect002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = nullptr;
    auto setEffectStatus =  audioSink->SetAudioEffectMode(AudioStandard::EFFECT_NONE);
    ASSERT_TRUE(setEffectStatus != Status::OK);
    int32_t audioEffectMode;
    auto getEffectStatus =  audioSink->GetAudioEffectMode(audioEffectMode);
    ASSERT_TRUE(getEffectStatus != Status::OK);
}

HWTEST(TestAudioSink, find_audio_sink_audio_reset_sync_info, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->ResetSyncInfo();
    SUCCEED();
}

HWTEST(TestAudioSink, find_audio_sink_audio_change_track, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::APP_UID, 0);
    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    Status res = audioSink->ChangeTrack(meta, testEventReceiver);
    ASSERT_EQ(res, Status::OK);
}

HWTEST(TestAudioSink, audio_sink_set_get_parameter, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    // SetParameter before ChangeTrack
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    auto setParameterStatus = audioSink->SetParameter(meta);
    ASSERT_EQ(setParameterStatus, Status::OK);

    // SetParameter after ChangeTrack
    meta->SetData(Tag::APP_UID, 9999);
    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    auto changeTrackStatus = audioSink->ChangeTrack(meta, testEventReceiver);
    ASSERT_EQ(changeTrackStatus, Status::OK);
    auto setParamterStatus = audioSink->SetParameter(meta);
    ASSERT_EQ(setParamterStatus, Status::OK);

    // GetParameter
    std::shared_ptr<Meta> newMeta = std::make_shared<Meta>();
    audioSink->GetParameter(newMeta);
    int32_t appUid = 0;
    (void)newMeta->Get<Tag::APP_UID>(appUid);
    ASSERT_FALSE(appUid == 9999);
}

HWTEST(TestAudioSink, audio_sink_write, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    // SetSyncCenter
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    audioSink->SetSyncCenter(syncCenter);

    // DoSyncWrite
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    buffer->flag_ = 0; // not eos
    buffer->pts_ = -1;
    auto doSyncWriteRes = audioSink->DoSyncWrite(buffer);
    ASSERT_TRUE(doSyncWriteRes == 0);
    buffer->pts_ = 1;
    doSyncWriteRes = audioSink->DoSyncWrite(buffer);
    ASSERT_TRUE(doSyncWriteRes == 0);
}

HWTEST(TestAudioSink, audio_sink_init, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto meta = std::make_shared<Meta>();
    auto testEventReceiver = std::make_shared<TestEventReceiver>();

    // Set some data in meta for testing
    meta->SetData(Tag::APP_PID, 12345);
    meta->SetData(Tag::APP_UID, 67890);
    meta->SetData(Tag::AUDIO_SAMPLE_RATE, 44100);
    meta->SetData(Tag::AUDIO_SAMPLE_PER_FRAME, 1024);

    // Call Init method
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    ASSERT_EQ(initStatus, Status::OK) << "Init should succeed with valid parameters";

    // Verify the internal state of AudioSink
    ASSERT_TRUE(audioSink->IsInitialized()) << "AudioSink should be initialized";
    ASSERT_TRUE(audioSink->HasPlugin()) << "Plugin should be initialized";
}

HWTEST(TestAudioSink, audio_sink_init002, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto meta = std::make_shared<Meta>();
    auto testEventReceiver = std::make_shared<TestEventReceiver>();

    // Set some data in meta for testing
    meta->SetData(Tag::APP_PID, 12345);
    meta->SetData(Tag::APP_UID, 67890);
    meta->SetData(Tag::AUDIO_SAMPLE_RATE, 0);
    meta->SetData(Tag::AUDIO_SAMPLE_PER_FRAME, 1024);

    // Call Init method
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    ASSERT_EQ(initStatus, Status::OK) << "Init should succeed with valid parameters";

    // Verify the internal state of AudioSink
    ASSERT_TRUE(audioSink->IsInitialized()) << "AudioSink should be initialized";
    ASSERT_TRUE(audioSink->HasPlugin()) << "Plugin should be initialized";
}

HWTEST(TestAudioSink, audio_sink_init003, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto meta = std::make_shared<Meta>();
    auto testEventReceiver = std::make_shared<TestEventReceiver>();

    // Set some data in meta for testing
    meta->SetData(Tag::APP_PID, 12345);
    meta->SetData(Tag::APP_UID, 67890);
    meta->SetData(Tag::AUDIO_SAMPLE_RATE, 44100);
    meta->SetData(Tag::AUDIO_SAMPLE_PER_FRAME, 0);

    // Call Init method
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    ASSERT_EQ(initStatus, Status::OK) << "Init should succeed with valid parameters";

    // Verify the internal state of AudioSink
    ASSERT_TRUE(audioSink->IsInitialized()) << "AudioSink should be initialized";
    ASSERT_TRUE(audioSink->HasPlugin()) << "Plugin should be initialized";
}

HWTEST(TestAudioSink, audio_sink_init004, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto meta = std::make_shared<Meta>();
    auto testEventReceiver = std::make_shared<TestEventReceiver>();

    // Set some data in meta for testing
    meta->SetData(Tag::APP_PID, 12345);
    meta->SetData(Tag::APP_UID, 67890);
    meta->SetData(Tag::AUDIO_SAMPLE_RATE, 0);
    meta->SetData(Tag::AUDIO_SAMPLE_PER_FRAME, 0);

    // Call Init method
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    ASSERT_EQ(initStatus, Status::OK) << "Init should succeed with valid parameters";

    // Verify the internal state of AudioSink
    ASSERT_TRUE(audioSink->IsInitialized()) << "AudioSink should be initialized";
    ASSERT_TRUE(audioSink->HasPlugin()) << "Plugin should be initialized";
}

HWTEST(TestAudioSink, audio_sink_init005, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto meta = std::make_shared<Meta>();
    auto testEventReceiver = std::make_shared<TestEventReceiver>();

    // Set some data in meta for testing
    meta->SetData(Tag::APP_PID, 12345);
    meta->SetData(Tag::APP_UID, 67890);
    meta->SetData(Tag::AUDIO_SAMPLE_RATE, 44100);
    meta->SetData(Tag::AUDIO_SAMPLE_PER_FRAME, 1024);
    meta->SetData(Tag::MEDIA_START_TIME, 1);
    meta->SetData(Tag::MIME_TYPE, "audio/x-ape");

    // Call Init method
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    ASSERT_EQ(initStatus, Status::OK) << "Init should succeed with valid parameters";

    // Verify the internal state of AudioSink
    ASSERT_TRUE(audioSink->IsInitialized()) << "AudioSink should be initialized";
    ASSERT_TRUE(audioSink->HasPlugin()) << "Plugin should be initialized";
}

HWTEST(TestAudioSink, audio_sink_init006, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto meta = std::make_shared<Meta>();
    auto testEventReceiver = std::make_shared<TestEventReceiver>();

    // Set some data in meta for testing
    meta->SetData(Tag::APP_PID, 12345);
    meta->SetData(Tag::APP_UID, 67890);
    meta->SetData(Tag::AUDIO_SAMPLE_RATE, 44100);
    meta->SetData(Tag::AUDIO_SAMPLE_PER_FRAME, 1024);
    meta->SetData(Tag::MIME_TYPE, "audio/mpeg");

    // Call Init method
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    ASSERT_EQ(initStatus, Status::OK) << "Init should succeed with valid parameters";

    // Verify the internal state of AudioSink
    ASSERT_TRUE(audioSink->IsInitialized()) << "AudioSink should be initialized";
    ASSERT_TRUE(audioSink->HasPlugin()) << "Plugin should be initialized";
}

HWTEST(TestAudioSink, audio_sink_init007, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    auto meta = std::make_shared<Meta>();
    auto testEventReceiver = std::make_shared<TestEventReceiver>();
    audioSink->plugin_ = audioSink->CreatePlugin();
    meta->SetData(Tag::MEDIA_START_TIME, 0);
    ASSERT_NE(nullptr, audioSink->plugin_);
    audioSink->samplePerFrame_ = 1;
    audioSink->sampleRate_  = -1;
    // Call Init method
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    ASSERT_EQ(initStatus, Status::OK) << "Init should succeed with valid parameters";
    // Verify the internal state of AudioSink
    ASSERT_TRUE(audioSink->IsInitialized()) << "AudioSink should be initialized";
    EXPECT_EQ(audioSink->playingBufferDurationUs_, 0);
    ASSERT_TRUE(audioSink->HasPlugin()) << "Plugin should be initialized";
}

HWTEST(TestAudioSink, audio_sink_GetBufferQueueProducer, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto meta = std::make_shared<Meta>();
    auto testEventReceiver = std::make_shared<TestEventReceiver>();

    // Set some data in meta for testing
    meta->SetData(Tag::APP_PID, 12345);
    meta->SetData(Tag::APP_UID, 67890);
    meta->SetData(Tag::AUDIO_SAMPLE_RATE, 44100);
    meta->SetData(Tag::AUDIO_SAMPLE_PER_FRAME, 1024);

    // Call Init method
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    ASSERT_EQ(initStatus, Status::OK) << "Init should succeed with valid parameters";

    // Get Buffer Queue Producer
    ASSERT_EQ(audioSink->GetBufferQueueProducer(), nullptr);

    // Prepare AudioSink
    auto prepareStatus = audioSink->Prepare();
    ASSERT_EQ(prepareStatus, Status::OK) << "Prepare should succeed";

    // Get Buffer Queue Producer
    auto producer = audioSink->GetBufferQueueProducer();
    ASSERT_TRUE(producer != nullptr) << "GetBufferQueueProducer should return a valid producer";
}

HWTEST(TestAudioSink, audio_sink_GetBufferQueueConsumer, TestSize.Level1) {
    
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    ASSERT_EQ(audioSink->GetBufferQueueConsumer(), nullptr);

    auto prepareStatus = audioSink->Prepare();
    ASSERT_EQ(prepareStatus, Status::OK) << "Prepare should succeed";

    auto consumer = audioSink->GetBufferQueueConsumer();
    ASSERT_TRUE(consumer != nullptr) << "GetBufferQueueConsumer should return a valid consumer";
}

HWTEST(TestAudioSink, audio_sink_TestSetParameter, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto meta = std::make_shared<Meta>();

    meta->SetData(Tag::APP_PID, 12345);
    meta->SetData(Tag::APP_UID, 67890);
    meta->SetData(Tag::AUDIO_SAMPLE_RATE, 44100);

    auto setParameterStatus = audioSink->SetParameter(meta);
    ASSERT_EQ(setParameterStatus, Status::OK) << "SetParameter should succeed";
    audioSink->plugin_ = nullptr;
    ASSERT_EQ(audioSink->SetParameter(nullptr), Status::ERROR_NULL_POINTER);
}

HWTEST(TestAudioSink, audio_sink_TestSetParameter02, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    // std::shared_ptr<Meta> meta = nullptr;
    auto meta = std::make_shared<Meta>();
    audioSink->plugin_ = nullptr;
    auto setParameterStatus = audioSink->SetParameter(meta);
    // ASSERT_EQ(setParameterStatus, Status::OK) << "SetParameter should succeed";
    ASSERT_TRUE(setParameterStatus != Status::OK);
}

HWTEST(TestAudioSink, audio_sink_TestPrepare, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto status = audioSink->Prepare();
    ASSERT_EQ(status, Status::OK) << "Prepare should return OK";
    ASSERT_EQ(Status::ERROR_INVALID_OPERATION, audioSink->Prepare());
}

HWTEST(TestAudioSink, audio_sink_TestStart, TestSize.Level1) {
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto prepareStatus = audioSink->Prepare();
    ASSERT_EQ(prepareStatus, Status::OK) << "Prepare should return OK";

    auto startStatus = audioSink->Start();
    ASSERT_EQ(startStatus, Status::OK) << "Start should return OK";

    std::shared_ptr<TestAudioSinkMock> plugin = std::make_shared<TestAudioSinkMock>("test");
    audioSink->plugin_ = plugin;
    ASSERT_EQ(Status::ERROR_UNKNOWN, audioSink->Start());
}

HWTEST(TestAudioSink, audio_sink_TestStop, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto prepareStatus = audioSink->Prepare();
    ASSERT_EQ(prepareStatus, Status::OK) << "Prepare should return OK";
    auto startStatus = audioSink->Start();
    ASSERT_EQ(startStatus, Status::OK) << "Start should return OK";

    auto stopStatus = audioSink->Stop();
    ASSERT_EQ(stopStatus, Status::OK) << "Stop should return OK";

    audioSink->eosInterruptType_ = AudioSink::EosInterruptState::INITIAL;
    ASSERT_EQ(Status::OK, audioSink->Stop());

    std::shared_ptr<TestAudioSinkMock> plugin = std::make_shared<TestAudioSinkMock>("test");
    audioSink->plugin_ = plugin;
    ASSERT_EQ(Status::ERROR_UNKNOWN, audioSink->Stop());
}

HWTEST(TestAudioSink, audio_sink_TestPause, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto prepareStatus = audioSink->Prepare();
    ASSERT_EQ(prepareStatus, Status::OK) << "Prepare should return OK";
    auto startStatus = audioSink->Start();
    ASSERT_EQ(startStatus, Status::OK) << "Start should return OK";

    auto pauseStatus = audioSink->Pause();
    ASSERT_EQ(pauseStatus, Status::OK) << "Pause should return OK";

    audioSink->eosInterruptType_ = AudioSink::EosInterruptState::INITIAL;
    ASSERT_EQ(audioSink->Pause(), Status::OK);

    audioSink->eosInterruptType_ = AudioSink::EosInterruptState::STOP;
    ASSERT_EQ(audioSink->Pause(), Status::OK);

    audioSink->isTransitent_  = true;
    audioSink->isEos_  = false;

    std::shared_ptr<TestAudioSinkMock> plugin = std::make_shared<TestAudioSinkMock>("test");
    audioSink->plugin_ = plugin;
    ASSERT_EQ(Status::ERROR_UNKNOWN, audioSink->Pause());
}

HWTEST(TestAudioSink, audio_sink_TestResume, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto prepareStatus = audioSink->Prepare();
    ASSERT_EQ(prepareStatus, Status::OK) << "Prepare should return OK";
    auto startStatus = audioSink->Start();
    ASSERT_EQ(startStatus, Status::OK) << "Start should return OK";
    auto pauseStatus = audioSink->Pause();
    ASSERT_EQ(pauseStatus, Status::OK) << "Pause should return OK";

    auto resumeStatus = audioSink->Resume();
    ASSERT_EQ(resumeStatus, Status::OK) << "Resume should return OK";

    audioSink->eosInterruptType_  = AudioSink::EosInterruptState::PAUSE;
    audioSink->eosDraining_ = false;
    audioSink->eosTask_  = nullptr;
    ASSERT_EQ(Status::ERROR_UNKNOWN, audioSink->Resume());

    std::shared_ptr<TestAudioSinkMock> plugin = std::make_shared<TestAudioSinkMock>("test");
    audioSink->plugin_ = plugin;
    ASSERT_EQ(Status::ERROR_UNKNOWN, audioSink->Resume());
}

HWTEST(TestAudioSink, audio_sink_SetVolume, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    ASSERT_EQ(Status::ERROR_INVALID_PARAMETER, audioSink->SetVolume(-1));
    audioSink->plugin_ = nullptr;
    ASSERT_EQ(Status::ERROR_NULL_POINTER, audioSink->SetVolume(0));
}

HWTEST(TestAudioSink, audio_sink_TestFlush, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto prepareStatus = audioSink->Prepare();
    ASSERT_EQ(prepareStatus, Status::OK) << "Prepare should return OK";
    auto startStatus = audioSink->Start();
    ASSERT_EQ(startStatus, Status::OK) << "Start should return OK";

    auto flushStatus = audioSink->Flush();
    ASSERT_EQ(flushStatus, Status::OK) << "Flush should return OK";
}

HWTEST(TestAudioSink, audio_sink_UpdateAudioWriteTimeMayWait, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    audioSink->UpdateAudioWriteTimeMayWait();
    ASSERT_EQ(audioSink->lastBufferWriteTime_, 0);

    audioSink->latestBufferDuration_ = MAX_BUFFER_DURATION_US + 1;
    audioSink->lastBufferWriteSuccess_ = true;
    audioSink->UpdateAudioWriteTimeMayWait();
    ASSERT_EQ(audioSink->latestBufferDuration_, MAX_BUFFER_DURATION_US);

    audioSink->lastBufferWriteSuccess_ = false;
    audioSink->UpdateAudioWriteTimeMayWait();
    ASSERT_EQ(audioSink->latestBufferDuration_, MAX_BUFFER_DURATION_US);

    audioSink->latestBufferDuration_ = 1;
    audioSink->UpdateAudioWriteTimeMayWait();
    ASSERT_EQ(audioSink->latestBufferDuration_, 1);
}

HWTEST(TestAudioSink, audio_sink_HandleEosInner, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->eosInterruptType_ = AudioSink::EosInterruptState::INITIAL;
    audioSink->HandleEosInner(true);
    EXPECT_EQ(false, audioSink->eosDraining_);
    EXPECT_EQ(audioSink->eosInterruptType_, AudioSink::EosInterruptState::NONE);

    audioSink->eosInterruptType_ = AudioSink::EosInterruptState::RESUME;
    audioSink->HandleEosInner(true);
    EXPECT_EQ(false, audioSink->eosDraining_);
    EXPECT_EQ(audioSink->eosInterruptType_, AudioSink::EosInterruptState::NONE);

    audioSink->eosInterruptType_ = AudioSink::EosInterruptState::STOP;
    audioSink->HandleEosInner(true);
    EXPECT_EQ(false, audioSink->eosDraining_);

    audioSink->eosInterruptType_ = AudioSink::EosInterruptState::INITIAL;
    audioSink->eosTask_ = std::make_unique<Task>("OS_EOSa", "test", TaskType::AUDIO, TaskPriority::HIGH, false);
    audioSink->HandleEosInner(false);
    EXPECT_EQ(true, audioSink->eosDraining_);

    audioSink->eosInterruptType_ = AudioSink::EosInterruptState::INITIAL;
    audioSink->eosTask_ = nullptr;
    audioSink->HandleEosInner(false);
    EXPECT_EQ(false, audioSink->eosDraining_);
    EXPECT_EQ(audioSink->eosInterruptType_, AudioSink::EosInterruptState::NONE);

    std::shared_ptr<TestAudioSinkMock> plugin = std::make_shared<TestAudioSinkMock>("test");
    audioSink->plugin_ = plugin;
    audioSink->eosInterruptType_ = AudioSink::EosInterruptState::INITIAL;
    audioSink->HandleEosInner(false);
    EXPECT_EQ(false, audioSink->eosDraining_);
    EXPECT_EQ(audioSink->eosInterruptType_, AudioSink::EosInterruptState::NONE);
}

HWTEST(TestAudioSink, audio_sink_DrainAndReportEosEvent, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    audioSink->SetSyncCenter(syncCenter);
    audioSink->DrainAndReportEosEvent();
    ASSERT_EQ(false, audioSink->eosDraining_);
}

// plugin_ == nullptr || inputBufferQueueConsumer_ == nullptr
HWTEST(TestAudioSink, audio_sink_DrainOutputBuffer_001, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = nullptr;
    audioSink->inputBufferQueueConsumer_ = nullptr;
    audioSink->DrainOutputBuffer();
    ASSERT_TRUE(audioSink->lastBufferWriteSuccess_);
}

// plugin_ != nullptr || inputBufferQueueConsumer_ == nullptr
HWTEST(TestAudioSink, audio_sink_DrainOutputBuffer_002, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->inputBufferQueueConsumer_ = nullptr;
    audioSink->DrainOutputBuffer();
    ASSERT_TRUE(audioSink->lastBufferWriteSuccess_);
}

// plugin_ != nullptr || inputBufferQueueConsumer_ != nullptr, ret != Status::OK || filledOutputBuffer == nullptr
HWTEST(TestAudioSink, audio_sink_DrainOutputBuffer_003, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->inputBufferQueue_ = AVBufferQueue::Create(5, MemoryType::SHARED_MEMORY, "test");
    audioSink->inputBufferQueueConsumer_ = audioSink->inputBufferQueue_->GetConsumer();
    audioSink->DrainOutputBuffer();
    ASSERT_TRUE(audioSink->lastBufferWriteSuccess_);
}

HWTEST(TestAudioSink, audio_sink_ResetSyncInfo, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    audioSink->SetSyncCenter(syncCenter);
    audioSink->ResetSyncInfo();
    ASSERT_TRUE(!audioSink->syncCenter_.expired());
}

// lastAnchorClockTime_ != HST_TIME_NONE || forceUpdateTimeAnchorNextTime_ != true
HWTEST(TestAudioSink, audio_sink_DoSyncWrite_001, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    audioSink->firstPts_  = 0;
    audioSink->lastAnchorClockTime_ = 0;
    audioSink->forceUpdateTimeAnchorNextTime_ = false;
    audioSink->playingBufferDurationUs_  = 1;
    AVBufferConfig config;
    config.size = 9;
    config.capacity = 9;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<OHOS::Media::AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_EQ(0, audioSink->DoSyncWrite(buffer));
}

// lastAnchorClockTime_ != HST_TIME_NONE || forceUpdateTimeAnchorNextTime_ == true
HWTEST(TestAudioSink, audio_sink_DoSyncWrite_002, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    audioSink->firstPts_  = 0;
    audioSink->lastAnchorClockTime_ = 0;
    audioSink->forceUpdateTimeAnchorNextTime_ = true;
    audioSink->playingBufferDurationUs_  = 1;
    AVBufferConfig config;
    config.size = 9;
    config.capacity = 9;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<OHOS::Media::AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_EQ(0, audioSink->DoSyncWrite(buffer));
}

HWTEST(TestAudioSink, audio_sink_SetSpeed_001, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    audioSink->plugin_ = nullptr;
    ASSERT_EQ(Status::ERROR_NULL_POINTER, audioSink->SetSpeed(0));
}

HWTEST(TestAudioSink, audio_sink_SetSpeed_002, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    std::shared_ptr<TestAudioSinkMock> plugin = std::make_shared<TestAudioSinkMock>("test");
    audioSink->plugin_ = plugin;

    ASSERT_EQ(Status::ERROR_INVALID_PARAMETER, audioSink->SetSpeed(0));
}

HWTEST(TestAudioSink, audio_sink_SetAudioEffectMode, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = nullptr;

    ASSERT_EQ(Status::ERROR_NULL_POINTER, audioSink->SetAudioEffectMode(0));
}

HWTEST(TestAudioSink, audio_sink_GetAudioEffectMode, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = nullptr;
    int32_t effectMode = 0;
    ASSERT_EQ(Status::ERROR_NULL_POINTER, audioSink->GetAudioEffectMode(effectMode));
}

HWTEST(TestAudioSink, audio_sink_getPendingAudioPlayoutDurationUs_001, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->numFramesWritten_ = 10;
    audioSink->samplePerFrame_ = 10;
    audioSink->sampleRate_ = 1;
    int64_t nowUs = 0;
    ASSERT_EQ(100000000, audioSink->getPendingAudioPlayoutDurationUs(nowUs));
}

HWTEST(TestAudioSink, audio_sink_getPendingAudioPlayoutDurationUs_002, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->numFramesWritten_ = -10;
    audioSink->samplePerFrame_ = 10;
    audioSink->sampleRate_ = 1;
    int64_t nowUs = 0;
    ASSERT_EQ(0, audioSink->getPendingAudioPlayoutDurationUs(nowUs));
}

HWTEST(TestAudioSink, audio_sink_getDurationUsPlayedAtSampleRate_001, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    std::shared_ptr<TestAudioSinkMock> plugin = std::make_shared<TestAudioSinkMock>("test");
    audioSink->plugin_ = plugin;
    uint32_t numFrames = 0;
    ASSERT_EQ(0, audioSink->getDurationUsPlayedAtSampleRate(numFrames));
}

HWTEST(TestAudioSink, audio_sink_getDurationUsPlayedAtSampleRate_002, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    std::shared_ptr<TestAudioSinkMock> plugin = std::make_shared<TestAudioSinkMock>("test");
    audioSink->plugin_ = plugin;
    uint32_t numFrames = 0;
    ASSERT_EQ(0, audioSink->getDurationUsPlayedAtSampleRate(numFrames));
}

HWTEST(TestAudioSink, audio_sink_ChangeTrack, TestSize.Level1)
{
    auto audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = nullptr;
    audioSink->volume_ = 0;
    audioSink->speed_  = -1;
    audioSink->effectMode_ = -1;
    audioSink->state_  = Pipeline::FilterState::RUNNING;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_EQ(Status::OK, audioSink->ChangeTrack(meta, nullptr));
}
} // namespace Test
} // namespace Media
} // namespace OHOS