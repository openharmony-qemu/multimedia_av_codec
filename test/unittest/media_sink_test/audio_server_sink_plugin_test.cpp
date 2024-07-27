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

#include "gtest/gtest.h"
#include "audio_server_sink_plugin.h"
#include "audio_info.h"
#include "audio_interrupt_info.h"

using namespace OHOS::AudioStandard;
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

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_start, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin = CreateAudioServerSinkPlugin("start");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, audioServerSinkPlugin->Start());
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_SetVolumeWithRamp, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin = CreateAudioServerSinkPlugin("SetVolumeWithRamp");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    ASSERT_EQ(0, audioServerSinkPlugin->SetVolumeWithRamp(0, 1));
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_AssignSampleRateIfSupported, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("AssignSampleRateIfSupported");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    int32_t sampleRate1 = 16000;
    int32_t sampleRate2 = 0;
    ASSERT_TRUE(audioServerSinkPlugin->AssignSampleRateIfSupported(sampleRate1));
    ASSERT_FALSE(audioServerSinkPlugin->AssignSampleRateIfSupported(sampleRate2));
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_AssignChannelNumIfSupported, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("AssignChannelNumIfSupported");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    int32_t channelNum1 = 0;
    int32_t channelNum2 = 2;
    ASSERT_FALSE(audioServerSinkPlugin->AssignChannelNumIfSupported(channelNum1));
    ASSERT_TRUE(audioServerSinkPlugin->AssignChannelNumIfSupported(channelNum2));
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_AssignSampleFmtIfSupported, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("AssignSampleFmtIfSupported");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    ASSERT_FALSE(audioServerSinkPlugin->AssignSampleFmtIfSupported(Plugins::AudioSampleFormat::SAMPLE_S24P));
    ASSERT_TRUE(audioServerSinkPlugin->AssignSampleFmtIfSupported(Plugins::AudioSampleFormat::SAMPLE_U8P));
    ASSERT_TRUE(audioServerSinkPlugin->AssignSampleFmtIfSupported(Plugins::AudioSampleFormat::SAMPLE_U8));
    ASSERT_FALSE(audioServerSinkPlugin->AssignSampleFmtIfSupported(Plugins::AudioSampleFormat::SAMPLE_F32LE));
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_SetInterruptMode, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("SetInterruptMode");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    ASSERT_TRUE(audioServerSinkPlugin->audioRenderer_ == nullptr);
    audioServerSinkPlugin->SetInterruptMode(AudioStandard::InterruptMode::SHARE_MODE);
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Init());
    audioServerSinkPlugin->SetInterruptMode(AudioStandard::InterruptMode::SHARE_MODE);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_SetParameter, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("SetParameter");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
        // set event receiver
    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    audioServerSinkPlugin->SetEventReceiver(testEventReceiver);
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Init());
    std::shared_ptr<Meta> meta1 = std::make_shared<Meta>();
    meta1->SetData(Tag::AUDIO_SAMPLE_RATE, 16000);
    meta1->SetData(Tag::AUDIO_OUTPUT_CHANNELS, 2);
    meta1->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::AudioSampleFormat::SAMPLE_U8P);
    meta1->SetData(Tag::AUDIO_RENDER_SET_FLAG, true);
    std::shared_ptr<Meta> meta2 = std::make_shared<Meta>();
    meta1->SetData(Tag::AUDIO_SAMPLE_RATE, 0);
    meta1->SetData(Tag::AUDIO_OUTPUT_CHANNELS, 0);
    meta1->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::AudioSampleFormat::SAMPLE_F32LE);
    meta1->SetData(Tag::AUDIO_RENDER_SET_FLAG, false);
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->SetParameter(meta1));
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->SetParameter(meta2));
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_DrainCacheData, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("DrainCacheData");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    ASSERT_TRUE(audioServerSinkPlugin->audioRenderer_ == nullptr);

    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    uint8_t* addr = buffer->memory_->GetAddr();
    audioServerSinkPlugin->CacheData(addr, config.size);
    audioServerSinkPlugin->CacheData(addr, config.size);

    ASSERT_EQ(Status::ERROR_UNKNOWN, audioServerSinkPlugin->DrainCacheData(true));
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Init());

    audioServerSinkPlugin->CacheData(addr, config.size);
    audioServerSinkPlugin->CacheData(addr, config.size);
    audioServerSinkPlugin->CacheData(addr, config.size);
    audioServerSinkPlugin->CacheData(addr, config.size);
    audioServerSinkPlugin->CacheData(addr, config.size);
    audioServerSinkPlugin->CacheData(addr, config.size);
    audioServerSinkPlugin->CacheData(addr, config.size);
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Start());
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Pause());
    ASSERT_EQ(Status::ERROR_AGAIN, audioServerSinkPlugin->DrainCacheData(true));
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Resume());
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->DrainCacheData(true));
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_Write, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("Write");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    ASSERT_TRUE(audioServerSinkPlugin->audioRenderer_ == nullptr);

    auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    int32_t size = 4;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(alloc, size);
    ASSERT_TRUE(buffer != nullptr);
    buffer->memory_->SetSize(4);
    ASSERT_EQ(Status::ERROR_NULL_POINTER, audioServerSinkPlugin->Write(buffer));
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Init());
    ASSERT_EQ(Status::ERROR_UNKNOWN, audioServerSinkPlugin->Write(buffer));
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Start());
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Pause());
    ASSERT_EQ(Status::ERROR_UNKNOWN, audioServerSinkPlugin->Write(buffer));
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Resume());
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Write(buffer));
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_SetAudioDumpBySysParam, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("Write");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    ASSERT_TRUE(audioServerSinkPlugin->audioRenderer_ == nullptr);
    system("param set sys.media.sink.entiredump.enable true");
    system("param set sys.media.sink.slicedump.enable true");
    audioServerSinkPlugin->SetAudioDumpBySysParam();

    auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    int32_t size = 4;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(alloc, size);
    ASSERT_TRUE(buffer != nullptr);
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Init());
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Start());
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Write(buffer));


    audioServerSinkPlugin->DumpEntireAudioBuffer(buffer->memory_->GetAddr(), size);
    audioServerSinkPlugin->DumpSliceAudioBuffer(buffer->memory_->GetAddr(), size);
    system("param set sys.media.sink.entiredump.enable false");
    system("param set sys.media.sink.slicedump.enable false");
    audioServerSinkPlugin->SetAudioDumpBySysParam();
    audioServerSinkPlugin->DumpEntireAudioBuffer(buffer->memory_->GetAddr(), size);
    audioServerSinkPlugin->DumpSliceAudioBuffer(buffer->memory_->GetAddr(), size);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_callback, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("Write");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    ASSERT_TRUE(audioServerSinkPlugin->audioRenderer_ == nullptr);
    std::shared_ptr<Pipeline::EventReceiver> receiver = std::make_shared<TestEventReceiver>();
    audioServerSinkPlugin->SetEventReceiver(receiver);
    bool isPaused = false;
    std::shared_ptr<AudioServerSinkPlugin::AudioRendererCallbackImpl> callback =
        std::make_shared<AudioServerSinkPlugin::AudioRendererCallbackImpl>(receiver, isPaused);
    OHOS::AudioStandard::InterruptEvent event1{InterruptType::INTERRUPT_TYPE_BEGIN,
        InterruptForceType::INTERRUPT_FORCE, InterruptHint::INTERRUPT_HINT_PAUSE};
    callback->OnInterrupt(event1);
    EXPECT_EQ(true, callback->isPaused_);
    OHOS::AudioStandard::InterruptEvent event2{InterruptType::INTERRUPT_TYPE_BEGIN,
        InterruptForceType::INTERRUPT_FORCE, InterruptHint::INTERRUPT_HINT_STOP};
    callback->OnInterrupt(event2);
    EXPECT_EQ(false, callback->isPaused_);
    OHOS::AudioStandard::InterruptEvent event3{InterruptType::INTERRUPT_TYPE_BEGIN,
        InterruptForceType::INTERRUPT_SHARE, InterruptHint::INTERRUPT_HINT_STOP};
    callback->OnInterrupt(event3);
    EXPECT_EQ(false, callback->isPaused_);
    callback->OnStateChange(RendererState::RENDERER_INVALID, StateChangeCmdType::CMD_FROM_CLIENT);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_init, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("Write");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    audioServerSinkPlugin->audioRenderSetFlag_ = true;
    audioServerSinkPlugin->rendererOptions_.rendererInfo.streamUsage = STREAM_USAGE_AUDIOBOOK;
    audioServerSinkPlugin->Init();
    ASSERT_TRUE(audioServerSinkPlugin->audioRenderer_ != nullptr);

    audioServerSinkPlugin->rendererOptions_.rendererInfo.streamUsage = STREAM_USAGE_GAME;
    audioServerSinkPlugin->Init();
    ASSERT_TRUE(audioServerSinkPlugin->audioRenderer_ != nullptr);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_release_file, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("Write");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    std::string path = "data/media/audio-sink-entire.pcm";
    audioServerSinkPlugin->entireDumpFile_ = fopen(path.c_str(), "wb+");
    path = "data/media/audio-sink-slice-.pcm";
    audioServerSinkPlugin->sliceDumpFile_ = fopen(path.c_str(), "wb+");
    ASSERT_EQ(nullptr, audioServerSinkPlugin->entireDumpFile_);
    ASSERT_EQ(nullptr, audioServerSinkPlugin->sliceDumpFile_);
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Reset());
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_reset, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("Write");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    audioServerSinkPlugin->resample_ = std::make_shared<Ffmpeg::Resample>();
    float volume = 0;
    int32_t duration = 0;
    ASSERT_EQ(0, audioServerSinkPlugin->SetVolumeWithRamp(volume, duration));
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->Reset());
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_get_parameter_001, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("Write");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    audioServerSinkPlugin->audioRenderSetFlag_ = true;
    audioServerSinkPlugin->audioRenderInfo_.streamUsage = STREAM_USAGE_AUDIOBOOK;
    audioServerSinkPlugin->Init();
    uint32_t sampleRate = 16000;
    ASSERT_TRUE(audioServerSinkPlugin->AssignSampleRateIfSupported(sampleRate));
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->GetParameter(meta));
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_get_parameter_002, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("Write");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    audioServerSinkPlugin->audioRenderSetFlag_ = true;
    audioServerSinkPlugin->audioRenderInfo_.streamUsage = STREAM_USAGE_AUDIOBOOK;
    audioServerSinkPlugin->Init();
    ASSERT_TRUE(audioServerSinkPlugin->AssignSampleRateIfSupported(AudioSamplingRate::SAMPLE_RATE_11025));
    
    uint32_t sampleRate = 16000;
    ASSERT_TRUE(audioServerSinkPlugin->AssignSampleRateIfSupported(sampleRate));
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_RENDER_SET_FLAG>(true);
    ASSERT_EQ(Status::OK, audioServerSinkPlugin->SetParameter(meta));
    audioServerSinkPlugin->GetParameter(meta);
}

HWTEST(TestAudioServerSinkPlugin, audio_sink_plugin_cache_data, TestSize.Level1)
{
    std::shared_ptr<AudioServerSinkPlugin> audioServerSinkPlugin =
        CreateAudioServerSinkPlugin("Write");
    ASSERT_TRUE(audioServerSinkPlugin != nullptr);
    audioServerSinkPlugin->CacheData(nullptr, 0);
    audioServerSinkPlugin->CacheData(nullptr, 0);
    audioServerSinkPlugin->CacheData(nullptr, 0);
    audioServerSinkPlugin->CacheData(nullptr, 0);
    audioServerSinkPlugin->CacheData(nullptr, 0);
    audioServerSinkPlugin->CacheData(nullptr, 0);
    audioServerSinkPlugin->CacheData(nullptr, 0);
    audioServerSinkPlugin->CacheData(nullptr, 0);
    audioServerSinkPlugin->CacheData(nullptr, 0);
    int32_t DEFAULT_BUFFER_NUM = 8;
    ASSERT_EQ(DEFAULT_BUFFER_NUM, audioServerSinkPlugin->cachedBuffers_.size());
}

}  // namespace Test
}  // namespace Media
}  // namespace OHOS