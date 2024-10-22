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

HWTEST(TestAudioSink, audio_sink_set_get_parameter, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);

    // SetParameter before ChangeTrack
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    auto setParameterStatus = audioSink->SetParameter(meta);
    ASSERT_EQ(setParameterStatus, Status::OK);

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
 
HWTEST(TestAudioSink, audio_sink_CalcMaxAmplitude_001, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    AVBufferConfig config;
    config.size = 1;
    config.capacity = 1;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<OHOS::Media::AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    audioSink->CalcMaxAmplitude(buffer);
    ASSERT_EQ(0, audioSink->DoSyncWrite(buffer));
}
 
HWTEST(TestAudioSink, audio_sink_CheckUpdateState_001, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    int8_t frame[] = {0, 127};
    audioSink->CheckUpdateState(reinterpret_cast<char*>(frame), sizeof(frame), 0);
    float amplitude = 0.0f;
    amplitude = audioSink->GetMaxAmplitude();
    ASSERT_EQ(true, fabs(amplitude - 1.0) <= 1e-6);
}
 
HWTEST(TestAudioSink, audio_sink_CheckUpdateState_002, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    int8_t frame[] = {0, -127};
    audioSink->CheckUpdateState(reinterpret_cast<char*>(frame), sizeof(frame), 0);
    float amplitude = 0.0f;
    amplitude = audioSink->GetMaxAmplitude();
    ASSERT_EQ(true, fabs(amplitude - 1.0) <= 1e-6);
}
 
HWTEST(TestAudioSink, audio_sink_CheckUpdateState_003, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    int16_t frame[] = {0, -32767};
    audioSink->CheckUpdateState(reinterpret_cast<char*>(frame), sizeof(frame), 1);
    float amplitude = 0.0f;
    amplitude = audioSink->GetMaxAmplitude();
    ASSERT_EQ(true, fabs(amplitude - 1.0) <= 1e-6);
}
 
HWTEST(TestAudioSink, audio_sink_CheckUpdateState_004, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    int16_t frame[] = {0, 32767};
    audioSink->CheckUpdateState(reinterpret_cast<char*>(frame), sizeof(frame), 1);
    float amplitude = 0.0f;
    amplitude = audioSink->GetMaxAmplitude();
    ASSERT_EQ(true, fabs(amplitude - 1.0) <= 1e-6);
}
 
HWTEST(TestAudioSink, audio_sink_CheckUpdateState_005, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    int8_t frame[] = {0, 0, 0, 1, 0, 0x80};
    audioSink->CheckUpdateState(reinterpret_cast<char*>(frame), sizeof(frame), 2);
    float amplitude = 0.0f;
    amplitude = audioSink->GetMaxAmplitude();
    ASSERT_EQ(true, fabs(amplitude - 1.0) <= 1e-6);
}
 
HWTEST(TestAudioSink, audio_sink_CheckUpdateState_006, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    int8_t frame[] = {0, 0, 0, 0xff, 0xff, 0x7f};
    audioSink->CheckUpdateState(reinterpret_cast<char*>(frame), sizeof(frame), 2);
    float amplitude = 0.0f;
    amplitude = audioSink->GetMaxAmplitude();
    ASSERT_EQ(true, fabs(amplitude - 1.0) <= 1e-6);
}
 
HWTEST(TestAudioSink, audio_sink_CheckUpdateState_007, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    int32_t frame[] = {0, -2147483647};
    audioSink->CheckUpdateState(reinterpret_cast<char*>(frame), sizeof(frame), 3);
    float amplitude = 0.0f;
    amplitude = audioSink->GetMaxAmplitude();
    ASSERT_EQ(true, fabs(amplitude - 1.0) <= 1e-6);
}
 
HWTEST(TestAudioSink, audio_sink_CheckUpdateState_008, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    int32_t frame[] = {0, 2147483647};
    audioSink->CheckUpdateState(reinterpret_cast<char*>(frame), sizeof(frame), 3);
    float amplitude = 0.0f;
    amplitude = audioSink->GetMaxAmplitude();
    ASSERT_EQ(true, fabs(amplitude - 1.0) <= 1e-6);
}
 
HWTEST(TestAudioSink, audio_sink_CheckUpdateState_009, TestSize.Level1)
{
    auto audioSink = std::make_shared<AudioSink>();
    ASSERT_TRUE(audioSink != nullptr);
    int32_t frame[] = {0, 2147483647};
    audioSink->CheckUpdateState(reinterpret_cast<char*>(frame), sizeof(frame), -1);
    float amplitude = 0.0f;
    amplitude = audioSink->GetMaxAmplitude();
    ASSERT_EQ(true, fabs(amplitude) <= 1e-6);
}
} // namespace Test
} // namespace Media
} // namespace OHOS