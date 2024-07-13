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
#include "audio_server_sink_plugin.h"

using namespace testing::ext;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
namespace Test {
class TestEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TestEventReceiver()
    {
        std::cout << "event receiver constructor" << std::endl;
    }

    void OnEvent(const Event &event)
    {
        std::cout << event.srcFilter << std::endl;
    }

private:
};

std::shared_ptr<AudioServerSinkPlugin> CreateAudioServerSinkPlugin(const std::string &name)
{
    return std::make_shared<AudioServerSinkPlugin>(name);
}

HWTEST(TestAudioServerSinkPlugin, find_audio_server_sink_plugin, TestSize.Level1)
{
    auto audioServerSinkPlugin = CreateAudioServerSinkPlugin("process");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    auto initStatus = audioServerSinkPlugin->Init();
    ASSERT_TRUE(initStatus == Status::OK);
    auto freeStatus = audioServerSinkPlugin->Deinit();
    ASSERT_TRUE(freeStatus == Status::OK);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_get_parameter, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin = CreateAudioServerSinkPlugin("get parameter");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    auto meta = std::make_shared<Meta>();
    auto resGetParam = audioServerSinkPlugin->GetParameter(meta);
    ASSERT_TRUE(resGetParam == Status::OK);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_set_parameter, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin = CreateAudioServerSinkPlugin("set parameter");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    auto meta = std::make_shared<Meta>();
    auto resGetParam = audioServerSinkPlugin->SetParameter(meta);
    ASSERT_TRUE(resGetParam == Status::OK);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_life_cycle, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin = CreateAudioServerSinkPlugin("get allocator");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    auto initStatus = audioServerSinkPlugin->Init();
    ASSERT_TRUE(initStatus == Status::OK);
    auto prepareStatus = audioServerSinkPlugin->Prepare();
    ASSERT_TRUE(prepareStatus == Status::OK);
    auto startStatus = audioServerSinkPlugin->Start();
    ASSERT_TRUE(startStatus == Status::OK);
    auto flushStatus = audioServerSinkPlugin->Flush();
    ASSERT_TRUE(flushStatus == Status::OK);
    auto pauseStatus = audioServerSinkPlugin->Pause();
    ASSERT_TRUE(pauseStatus == Status::OK);
    auto resumeStatus = audioServerSinkPlugin->Resume();
    ASSERT_TRUE(resumeStatus == Status::OK);
    auto pauseTransitentStatus = audioServerSinkPlugin->PauseTransitent();
    ASSERT_TRUE(pauseTransitentStatus == Status::OK);
    auto drainStatus = audioServerSinkPlugin->Drain();
    ASSERT_TRUE(drainStatus == Status::OK);
    auto stopStatus = audioServerSinkPlugin->Stop();
    ASSERT_TRUE(stopStatus == Status::OK);
    auto resetStatus = audioServerSinkPlugin->Reset();
    ASSERT_TRUE(resetStatus == Status::OK);
    auto deinitStatus = audioServerSinkPlugin->Deinit();
    ASSERT_TRUE(deinitStatus == Status::OK);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_get, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin = CreateAudioServerSinkPlugin("get");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    auto initStatus = audioServerSinkPlugin->Init();
    ASSERT_TRUE(initStatus == Status::OK);

    // get latency
    uint64_t latency = 0;
    auto getLatencyStatus = audioServerSinkPlugin->GetLatency(latency);
    ASSERT_TRUE(getLatencyStatus == Status::OK);

    // get played out duration us
    int64_t nowUs = 0;
    auto durationUs = audioServerSinkPlugin->GetPlayedOutDurationUs(nowUs);
    ASSERT_TRUE(durationUs == 0);

    // get frame position
    int32_t framePos = 0;
    auto getFramePositionStatus = audioServerSinkPlugin->GetFramePosition(framePos);
    ASSERT_TRUE(getFramePositionStatus == Status::OK);

    // get audio effect mode
    int32_t audioEffectMode = 0;
    auto getAudioEffectModeStatus = audioServerSinkPlugin->GetAudioEffectMode(audioEffectMode);
    ASSERT_TRUE(getAudioEffectModeStatus == Status::OK);

    // get volume
    float volume = 0;
    auto getVolumeStatus = audioServerSinkPlugin->GetVolume(volume);
    ASSERT_TRUE(getVolumeStatus == Status::OK);

    // get speed
    float speed = 0;
    auto getSpeedStatus = audioServerSinkPlugin->GetSpeed(speed);
    ASSERT_TRUE(getSpeedStatus == Status::OK);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_get_without_init, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin = CreateAudioServerSinkPlugin("get");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);

    // get latency
    uint64_t latency = 0;
    auto getLatencyStatus = audioServerSinkPlugin->GetLatency(latency);
    ASSERT_FALSE(getLatencyStatus == Status::OK);

    // get played out duration us
    int64_t nowUs = 0;
    auto durationUs = audioServerSinkPlugin->GetPlayedOutDurationUs(nowUs);
    ASSERT_FALSE(durationUs == 0);

    // get frame position
    int32_t framePos = 0;
    auto getFramePositionStatus = audioServerSinkPlugin->GetFramePosition(framePos);
    ASSERT_FALSE(getFramePositionStatus == Status::OK);

    // get audio effect mode
    int32_t audioEffectMode = 0;
    auto getAudioEffectModeStatus = audioServerSinkPlugin->GetAudioEffectMode(audioEffectMode);
    ASSERT_FALSE(getAudioEffectModeStatus == Status::OK);

    // get volume
    float volume = 0;
    auto getVolumeStatus = audioServerSinkPlugin->GetVolume(volume);
    ASSERT_FALSE(getVolumeStatus == Status::OK);

    // get speed
    float speed = 0;
    auto getSpeedStatus = audioServerSinkPlugin->GetSpeed(speed);
    ASSERT_FALSE(getSpeedStatus == Status::OK);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_set, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin = CreateAudioServerSinkPlugin("get");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    auto initStatus = audioServerSinkPlugin->Init();
    ASSERT_TRUE(initStatus == Status::OK);

    // set event receiver
    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    audioServerSinkPlugin->SetEventReceiver(testEventReceiver);

    // set volume
    float targetVolume = 0;
    auto setVolumeStatus = audioServerSinkPlugin->SetVolume(targetVolume);
    ASSERT_TRUE(setVolumeStatus == Status::OK);

    // set volume with ramp
    int32_t duration = 0;
    auto setVolumeWithRampStatus = audioServerSinkPlugin->SetVolumeWithRamp(targetVolume, duration);
    ASSERT_TRUE(setVolumeWithRampStatus == 0);

    // set audio effect mode
    int32_t audioEffectMode = 0;
    auto setAudioEffectModeStatus = audioServerSinkPlugin->SetAudioEffectMode(audioEffectMode);
    ASSERT_TRUE(setAudioEffectModeStatus == Status::OK);

    // set speed
    float speed = 2.0F;
    auto setSpeedStatus = audioServerSinkPlugin->SetSpeed(speed);
    ASSERT_TRUE(setSpeedStatus == Status::OK);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_set_without_init, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin = CreateAudioServerSinkPlugin("get");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);

    // set event receiver
    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    audioServerSinkPlugin->SetEventReceiver(testEventReceiver);

    // set volume
    float targetVolume = 0;
    auto setVolumeStatus = audioServerSinkPlugin->SetVolume(targetVolume);
    ASSERT_FALSE(setVolumeStatus == Status::OK);

    // set volume with ramp
    int32_t duration = 0;
    auto setVolumeWithRampStatus = audioServerSinkPlugin->SetVolumeWithRamp(targetVolume, duration);
    ASSERT_TRUE(setVolumeWithRampStatus == 0);

    // set audio effect mode
    int32_t audioEffectMode = 0;
    auto setAudioEffectModeStatus = audioServerSinkPlugin->SetAudioEffectMode(audioEffectMode);
    ASSERT_FALSE(setAudioEffectModeStatus == Status::OK);

    // set speed
    float speed = 2.0F;
    auto setSpeedStatus = audioServerSinkPlugin->SetSpeed(speed);
    ASSERT_FALSE(setSpeedStatus == Status::OK);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_write, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin = CreateAudioServerSinkPlugin("set mute");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    auto writeStatus = audioServerSinkPlugin->Write(buffer);
    ASSERT_TRUE(writeStatus == Status::OK);

    // WriteAudioVivid
    auto writeVividStatus = audioServerSinkPlugin->WriteAudioVivid(buffer);
    ASSERT_TRUE(writeVividStatus != 0);
}
}  // namespace Test
}  // namespace Media
}  // namespace OHOS