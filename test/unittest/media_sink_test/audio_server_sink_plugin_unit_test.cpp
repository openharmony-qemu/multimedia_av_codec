/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "audio_server_sink_plugin_unit_test.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AudioStandard;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
class MockAudioRenderer : public AudioRenderer {
public:
    MOCK_METHOD(void, SetAudioPrivacyType, (AudioPrivacyType), (override));
    MOCK_METHOD(AudioPrivacyType, GetAudioPrivacyType, (), (override));
    MOCK_METHOD(int32_t, SetParams, (const AudioRendererParams), (override));
    MOCK_METHOD(int32_t, SetRendererCallback, (const std::shared_ptr<AudioRendererCallback> &), (override));
    MOCK_METHOD(int32_t, GetParams, (AudioRendererParams &), (const, override));
    MOCK_METHOD(int32_t, GetRendererInfo, (AudioRendererInfo &), (const, override));
    MOCK_METHOD(int32_t, GetStreamInfo, (AudioStreamInfo &), (const, override));
    MOCK_METHOD(bool, Start, (StateChangeCmdType), (override));
    MOCK_METHOD(int32_t, Write, (uint8_t *, size_t), (override));
    MOCK_METHOD(int32_t, Write, (uint8_t *, size_t, uint8_t *, size_t), (override));
    MOCK_METHOD(RendererState, GetStatus, (), (const, override));
    MOCK_METHOD(bool, GetAudioTime, (Timestamp &, Timestamp::Timestampbase), (const, override));
    MOCK_METHOD(bool, GetAudioPosition, (Timestamp &, Timestamp::Timestampbase), (const, override));
    MOCK_METHOD(int32_t, GetLatency, (uint64_t &), (const, override));
    MOCK_METHOD(bool, Drain, (), (const, override));
    MOCK_METHOD(bool, Flush, (), (const, override));
    MOCK_METHOD(bool, PauseTransitent, (StateChangeCmdType), (override));
    MOCK_METHOD(bool, Pause, (StateChangeCmdType), (override));
    MOCK_METHOD(bool, Stop, (), (override));
    MOCK_METHOD(bool, Release, (), (override));
    MOCK_METHOD(int32_t, GetBufferSize, (size_t &), (const, override));
    MOCK_METHOD(int32_t, GetAudioStreamId, (uint32_t &), (const, override));
    MOCK_METHOD(int32_t, GetFrameCount, (uint32_t &), (const, override));
    MOCK_METHOD(int32_t, SetAudioRendererDesc, (AudioRendererDesc), (override));
    MOCK_METHOD(int32_t, SetStreamType, (AudioStreamType), (override));
    MOCK_METHOD(int32_t, SetVolume, (float), (const, override));
    MOCK_METHOD(float, GetVolume, (), (const, override));
    MOCK_METHOD(int32_t, SetRenderRate, (AudioRendererRate), (const, override));
    MOCK_METHOD(AudioRendererRate, GetRenderRate, (), (const, override));
    MOCK_METHOD(int32_t, SetRendererSamplingRate, (uint32_t), (const, override));
    MOCK_METHOD(uint32_t, GetRendererSamplingRate, (), (const, override));
    MOCK_METHOD(
        int32_t, SetRendererPositionCallback, (int64_t, const std::shared_ptr<RendererPositionCallback> &), (override));
    MOCK_METHOD(void, UnsetRendererPositionCallback, (), (override));
    MOCK_METHOD(int32_t, SetRendererPeriodPositionCallback,
        (int64_t, const std::shared_ptr<RendererPeriodPositionCallback> &), (override));
    MOCK_METHOD(void, UnsetRendererPeriodPositionCallback, (), (override));
    MOCK_METHOD(int32_t, SetBufferDuration, (uint64_t), (const, override));
    MOCK_METHOD(int32_t, SetRenderMode, (AudioRenderMode), (override));
    MOCK_METHOD(AudioRenderMode, GetRenderMode, (), (const, override));
    MOCK_METHOD(int32_t, SetRendererWriteCallback, (const std::shared_ptr<AudioRendererWriteCallback> &), (override));
    MOCK_METHOD(int32_t, SetRendererFirstFrameWritingCallback,
        (const std::shared_ptr<AudioRendererFirstFrameWritingCallback> &), (override));
    MOCK_METHOD(int32_t, GetBufferDesc, (BufferDesc &), (const, override));
    MOCK_METHOD(int32_t, Enqueue, (const BufferDesc &), (const, override));
    MOCK_METHOD(int32_t, Clear, (), (const, override));
    MOCK_METHOD(int32_t, GetBufQueueState, (BufferQueueState &), (const, override));
    MOCK_METHOD(void, SetInterruptMode, (InterruptMode), (override));
    MOCK_METHOD(int32_t, SetParallelPlayFlag, (bool), (override));
    MOCK_METHOD(int32_t, SetLowPowerVolume, (float), (const, override));
    MOCK_METHOD(float, GetLowPowerVolume, (), (const, override));
    MOCK_METHOD(int32_t, SetOffloadAllowed, (bool), (override));
    MOCK_METHOD(int32_t, SetOffloadMode, (int32_t, bool), (const, override));
    MOCK_METHOD(int32_t, UnsetOffloadMode, (), (const, override));
    MOCK_METHOD(float, GetSingleStreamVolume, (), (const, override));
    MOCK_METHOD(float, GetMinStreamVolume, (), (const, override));
    MOCK_METHOD(float, GetMaxStreamVolume, (), (const, override));
    MOCK_METHOD(uint32_t, GetUnderflowCount, (), (const, override));
    MOCK_METHOD(int32_t, GetCurrentOutputDevices, (AudioDeviceDescriptor &), (const, override));
    MOCK_METHOD(AudioEffectMode, GetAudioEffectMode, (), (const, override));
    MOCK_METHOD(int64_t, GetFramesWritten, (), (const, override));
    MOCK_METHOD(int32_t, SetAudioEffectMode, (AudioEffectMode), (const, override));
    MOCK_METHOD(void, SetAudioRendererErrorCallback, (std::shared_ptr<AudioRendererErrorCallback>), (override));
    MOCK_METHOD(int32_t, RegisterOutputDeviceChangeWithInfoCallback,
        (const std::shared_ptr<AudioRendererOutputDeviceChangeCallback> &), (override));
    MOCK_METHOD(int32_t, UnregisterOutputDeviceChangeWithInfoCallback, (), (override));
    MOCK_METHOD(int32_t, UnregisterOutputDeviceChangeWithInfoCallback,
        (const std::shared_ptr<AudioRendererOutputDeviceChangeCallback> &), (override));
    MOCK_METHOD(int32_t, RegisterAudioPolicyServerDiedCb,
        (const int32_t, const std::shared_ptr<AudioRendererPolicyServiceDiedCallback> &), (override));
    MOCK_METHOD(int32_t, UnregisterAudioPolicyServerDiedCb, (const int32_t), (override));
    MOCK_METHOD(int32_t, SetChannelBlendMode, (ChannelBlendMode), (override));
    MOCK_METHOD(int32_t, SetVolumeWithRamp, (float, int32_t), (override));
    MOCK_METHOD(void, SetPreferredFrameSize, (int32_t), (override));
    MOCK_METHOD(int32_t, SetSpeed, (float), (override));
    MOCK_METHOD(float, GetSpeed, (), (override));
    MOCK_METHOD(bool, IsFastRenderer, (), (override));
    MOCK_METHOD(void, SetSilentModeAndMixWithOthers, (bool), (override));
    MOCK_METHOD(bool, GetSilentModeAndMixWithOthers, (), (override));
    MOCK_METHOD(void, EnableVoiceModemCommunicationStartStream, (bool), (override));
    MOCK_METHOD(bool, IsNoStreamRenderer, (), (const, override));
    MOCK_METHOD(int32_t, SetDefaultOutputDevice, (DeviceType), (override));
    MOCK_METHOD(bool, Mute, (StateChangeCmdType), (const, override));
    MOCK_METHOD(bool, Unmute, (StateChangeCmdType), (const, override));
    MOCK_METHOD(int32_t, GetAudioTimestampInfo, (Timestamp &, Timestamp::Timestampbase), (const, override));
    ~MockAudioRenderer() override {}
};

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

void AudioServerSinkPluginUnitTest::SetUpTestCase(void) {}

void AudioServerSinkPluginUnitTest::TearDownTestCase(void) {}

void AudioServerSinkPluginUnitTest::SetUp(void)
{
    plugin_ = std::make_shared<AudioServerSinkPlugin>("TestAudioServerSinkPlugin");
}

void AudioServerSinkPluginUnitTest::TearDown(void)
{
    plugin_ = nullptr;
}

HWTEST_F(AudioServerSinkPluginUnitTest, audio_server_sink_plugin_init_deinit, TestSize.Level1)
{
    ASSERT_TRUE(plugin_ != nullptr);
    auto initStatus = plugin_->Init();
    ASSERT_TRUE(initStatus == Status::OK);

    initStatus = plugin_->Init();
    ASSERT_TRUE(initStatus == Status::OK);

    plugin_->audioRenderer_ = nullptr;
    initStatus = plugin_->Init();
    ASSERT_TRUE(initStatus == Status::OK);

    auto audioRenderer = std::make_unique<NiceMock<MockAudioRenderer>>();

    EXPECT_CALL(*audioRenderer, GetStatus())
        .WillOnce(Return(RendererState::RENDERER_RELEASED));

    plugin_->audioRenderer_ = std::move(audioRenderer);

    auto freeStatus = plugin_->Deinit();
    ASSERT_TRUE(freeStatus == Status::OK);

    freeStatus = plugin_->Deinit();
    ASSERT_TRUE(freeStatus == Status::OK);

    plugin_->audioRenderer_ = nullptr;
    freeStatus = plugin_->Deinit();
    ASSERT_TRUE(freeStatus == Status::OK);
}

HWTEST_F(AudioServerSinkPluginUnitTest, stop_renderer, TestSize.Level1)
{
    auto res = plugin_->StopRender();
    ASSERT_EQ(res, true);

    auto audioRenderer = std::make_unique<NiceMock<MockAudioRenderer>>();

    EXPECT_CALL(*audioRenderer, GetStatus())
        .WillOnce(Return(RendererState::RENDERER_RUNNING))
        .WillOnce(Return(RendererState::RENDERER_STOPPED))
        .WillOnce(Return(RendererState::RENDERER_STOPPED));
    EXPECT_CALL(*audioRenderer, Stop()).WillOnce(Return(false));

    plugin_->audioRenderer_ = std::move(audioRenderer);

    res = plugin_->StopRender();
    ASSERT_EQ(res, false);

    res = plugin_->StopRender();
    ASSERT_EQ(res, true);
}

HWTEST_F(AudioServerSinkPluginUnitTest, audio_server_sink_plugin_start, TestSize.Level1)
{
    auto res = plugin_->Start();
    ASSERT_EQ(res, Status::ERROR_WRONG_STATE);

    auto audioRenderer = std::make_unique<NiceMock<MockAudioRenderer>>();

    EXPECT_CALL(*audioRenderer, Start(_)).WillOnce(Return(true));
    EXPECT_CALL(*audioRenderer, Stop()).WillOnce(Return(false)).WillOnce(Return(true));

    plugin_->audioRenderer_ = std::move(audioRenderer);

    res = plugin_->Start();
    ASSERT_EQ(res, Status::OK);

    auto stopRenderRes = plugin_->StopRender();
    ASSERT_EQ(res, Status::OK);

    stopRenderRes = plugin_->StopRender();
    ASSERT_EQ(res, Status::OK);
}
}  // namespace Media
}  // namespace OHOS