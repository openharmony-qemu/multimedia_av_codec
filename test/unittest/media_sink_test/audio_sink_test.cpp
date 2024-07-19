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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_ONLY_PRERELEASE, LOG_DOMAIN_SYSTEM_PLAYER, "HiStreamer" };
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
}
} // namespace Test
} // namespace Media
} // namespace OHOS