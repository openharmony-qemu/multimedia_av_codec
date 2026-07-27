/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <cstring>
#include <climits>
#include "plugin/plugin_manager_v2.h"
#include "plugin/codec_plugin.h"
#include "avcodec_codec_name.h"
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "meta/meta.h"

#define BUFFER_FLAG_EOS 1

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace {
constexpr int32_t OPUS_CHANNELS = 1;
constexpr int32_t OPUS_SAMPLE_RATE = 48000;
constexpr int32_t OPUS_BITRATE = 64000;
constexpr int32_t OPUS_FRAME_SAMPLES = 960; // 48000 * 0.02s = 960 samples per 20ms frame
constexpr int32_t FRAME_BYTES = OPUS_FRAME_SAMPLES * 2; // 1920: one S16LE mono 20ms frame
constexpr int32_t TWO_FRAME_PCM = FRAME_BYTES * 2; // 3840: two frames, leaves residual for ERROR_AGAIN
constexpr int32_t MULTI_FRAME_PCM = FRAME_BYTES * 4; // 7680: enough for 4 frames in pcmCache
constexpr int32_t S32_FRAME_BYTES = OPUS_FRAME_SAMPLES * 4; // 3840: one S32LE mono 20ms frame
constexpr int32_t SMALL_RESIDUAL_BYTES = 256; // 256: less than one frame (1920)
constexpr int32_t OUTPUT_CAPACITY = 4096; // 4096: larger than any opus packet
constexpr int32_t MAX_INPUT_BUFFER_SIZE_LIMIT = 10 * 1024 * 1024; // 10MB: clamp limit in plugin
constexpr int32_t OVER_LIMIT_BUFFER_SIZE = 11 * 1024 * 1024; // 11MB: exceeds clamp limit
constexpr int32_t MIN_BITRATE = 6000;
constexpr int32_t MAX_BITRATE = 510000;
// 2: AUDIO_ENCODER_PTS_MODE_FIRST_INPUT_START
constexpr int32_t PTS_MODE_FIRST_INPUT_START = 2;
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {

class OpusEncoderPluginUnitTest : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

    void OnInputBufferDone(const shared_ptr<AVBuffer> &inputBuffer) override
    {
        (void)inputBuffer;
        inputCallbackCount_++;
    }

    void OnOutputBufferDone(const shared_ptr<AVBuffer> &outputBuffer) override
    {
        outputCallbackCount_++;
        outputCallbackSize_ = outputBuffer ? outputBuffer->memory_->GetSize() : 0;
    }

    void OnEvent(const shared_ptr<PluginEvent> event) override
    {
        (void)event;
    }

    shared_ptr<AVBuffer> CreatePcmInputBuffer(int32_t sizeInBytes, bool isEos)
    {
        auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        auto buffer = AVBuffer::CreateAVBuffer(allocator, sizeInBytes);
        if (buffer && buffer->memory_ && sizeInBytes > 0) {
            vector<uint8_t> data(sizeInBytes, 0x10); // 0x10: fill pattern for test PCM data
            buffer->memory_->Write(data.data(), sizeInBytes, 0);
            buffer->memory_->SetSize(sizeInBytes);
        } else if (buffer && buffer->memory_) {
            buffer->memory_->SetSize(0);
        }
        if (buffer) {
            buffer->meta_ = make_shared<Meta>(); // ensure meta_ is non-null for the plugin's null check
        }
        if (buffer && isEos) {
            buffer->flag_ = BUFFER_FLAG_EOS;
        }
        return buffer;
    }

    shared_ptr<AVBuffer> CreateOutputBuffer(int32_t capacity)
    {
        auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        return AVBuffer::CreateAVBuffer(allocator, capacity);
    }

    // ptsMode < 0 means do not set AUDIO_ENCODE_PTS_MODE.
    // Calls Init + SetParameter + Prepare so the encoder is ready to use.
    Status InitAndConfigure(bool enableCache, int32_t bitrate = OPUS_BITRATE,
        AudioSampleFormat fmt = AudioSampleFormat::SAMPLE_S16LE, bool enableConvert = false,
        int32_t sampleRate = OPUS_SAMPLE_RATE, int32_t channels = OPUS_CHANNELS, int32_t ptsMode = -1)
    {
        auto ret = plugin_->Init();
        if (ret != Status::OK) {
            return ret;
        }
        plugin_->SetDataCallback(this);
        auto meta = make_shared<Meta>();
        meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
        meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
        meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(fmt);
        meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(bitrate));
        if (enableConvert) {
            meta->Set<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(true);
        }
        if (ptsMode >= 0) {
            meta->Set<Tag::AUDIO_ENCODE_PTS_MODE>(ptsMode);
        }
        if (enableCache) {
            meta->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(MULTI_FRAME_PCM);
        }
        ret = plugin_->SetParameter(meta);
        if (ret != Status::OK) {
            return ret;
        }
        return plugin_->Prepare(); // Prepare creates the opus encoder
    }

protected:
    shared_ptr<CodecPlugin> plugin_ = nullptr;
    int32_t inputCallbackCount_ = 0;
    int32_t outputCallbackCount_ = 0;
    int32_t outputCallbackSize_ = 0;
};

void OpusEncoderPluginUnitTest::SetUpTestCase(void)
{
    cout << "[OpusEncoder SetUpTestCase]" << endl;
}

void OpusEncoderPluginUnitTest::TearDownTestCase(void)
{
    cout << "[OpusEncoder TearDownTestCase]" << endl;
}

void OpusEncoderPluginUnitTest::SetUp()
{
    auto opusPlugin = PluginManagerV2::Instance().CreatePluginByName(
        std::string(MediaAVCodec::AVCodecCodecName::AUDIO_ENCODER_OPUS_NAME));
    if (opusPlugin == nullptr) {
        cout << "WARNING: Opus encoder plugin not available, tests will skip" << endl;
        return;
    }
    plugin_ = reinterpret_pointer_cast<CodecPlugin>(opusPlugin);
    inputCallbackCount_ = 0;
    outputCallbackCount_ = 0;
    outputCallbackSize_ = 0;
}

void OpusEncoderPluginUnitTest::TearDown()
{
    if (plugin_) {
        plugin_->Stop();
        plugin_->Release(); // safe to call even if already released (returns ERROR_UNKNOWN)
        plugin_ = nullptr;
    }
}

TEST_F(OpusEncoderPluginUnitTest, PluginLoadable)
{
    ASSERT_NE(plugin_, nullptr) << "Opus plugin should load via PluginManagerV2";
}

TEST_F(OpusEncoderPluginUnitTest, Init_SetParameter_Start_Succeed)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    meta->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(MULTI_FRAME_PCM);
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin_->Start()); // Start == Prepare
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_InvalidChannelCount_TooSmall)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(0); // 0: < MIN_CHANNELS(1)
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_InvalidChannelCount_TooLarge)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(3); // 3: > MAX_CHANNELS(2)
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_MissingChannelCount)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_MissingSampleFormat)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_WrongSampleFormat_NoConvert)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S32LE); // not S16LE, convert off
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_UnsupportedSampleFormat_WithConvert)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    // 99: a value not in the extended sample format set
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(static_cast<AudioSampleFormat>(99));
    meta->Set<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(true);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_MissingBitrate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_BitrateTooSmall)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(MIN_BITRATE - 1)); // 5999: < MIN_BITRATE
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_BitrateTooLarge)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(MAX_BITRATE + 1)); // 510001: > MAX_BITRATE
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_MissingSampleRate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_InvalidSampleRate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(11025); // 11025: not in opus sample rate table
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_LowSampleRate_Succeed)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(8000); // 8000: lowest valid opus sample rate
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_Stereo_Succeed)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2: stereo, valid
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_PtsMode_Present)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    meta->Set<Tag::AUDIO_ENCODE_PTS_MODE>(PTS_MODE_FIRST_INPUT_START); // covers Get<PTS_MODE> true branch
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(OpusEncoderPluginUnitTest, SetParameter_MaxInputBufferSize_ExceedsLimit)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    // exceeds 10MB limit, plugin should clamp and still enable cache
    meta->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(OVER_LIMIT_BUFFER_SIZE);
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    auto outMeta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(outMeta));
    int32_t maxInputSize = 0;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    EXPECT_EQ(MAX_INPUT_BUFFER_SIZE_LIMIT, maxInputSize);
}

TEST_F(OpusEncoderPluginUnitTest, GetParameter_ClampInvalidSizes)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(-1);  // -1: <=0, should be clamped to default
    meta->Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(-1); // -1: <=0, should be clamped to default
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    auto outMeta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(outMeta));
    int32_t maxInputSize = 0;
    int32_t maxOutputSize = 0;
    EXPECT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    EXPECT_TRUE(outMeta->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize));
    EXPECT_GT(maxInputSize, 0);
    EXPECT_GT(maxOutputSize, 0);
}

TEST_F(OpusEncoderPluginUnitTest, GetParameter_ClampOversizeSizes)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(1024 * 1024);  // larger than default input size (8192)
    meta->Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(1024 * 1024); // larger than default output size (1500)
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    auto outMeta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(outMeta));
}

TEST_F(OpusEncoderPluginUnitTest, GetParameter_CacheEnabled)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    auto outMeta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(outMeta));
    int32_t maxInputSize = 0;
    EXPECT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    EXPECT_EQ(MULTI_FRAME_PCM, maxInputSize); // cache path sets AUDIO_MAX_INPUT_SIZE to buffer size
}

TEST_F(OpusEncoderPluginUnitTest, Prepare_NoInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    // pluginCodecPtr_ is null until Init() is called
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->Prepare());
}

TEST_F(OpusEncoderPluginUnitTest, Start_NoInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->Start());
}

TEST_F(OpusEncoderPluginUnitTest, Reset_NoInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->Reset());
}

TEST_F(OpusEncoderPluginUnitTest, Flush_NoInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->Flush());
}

TEST_F(OpusEncoderPluginUnitTest, Release_NoInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->Release());
}

TEST_F(OpusEncoderPluginUnitTest, QueueInputBuffer_NullBuffer)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    shared_ptr<AVBuffer> nullBuf = nullptr;
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(nullBuf));
}

TEST_F(OpusEncoderPluginUnitTest, QueueInputBuffer_NullMemory)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    ASSERT_NE(input, nullptr);
    input->memory_ = nullptr; // force null memory branch
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(input));
}

TEST_F(OpusEncoderPluginUnitTest, QueueInputBuffer_EosFlag)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto input = CreatePcmInputBuffer(0, true);
    inputCallbackCount_ = 0;
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(1, inputCallbackCount_);
}

TEST_F(OpusEncoderPluginUnitTest, QueueInputBuffer_ZeroSizeWithoutEos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto input = CreatePcmInputBuffer(0, false);
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(input));
}

TEST_F(OpusEncoderPluginUnitTest, QueueInputBuffer_WrongSizeNoCache)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    // non-cache mode requires input size == one frame (1920); 256 != 1920
    auto input = CreatePcmInputBuffer(SMALL_RESIDUAL_BYTES, false);
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(input));
}

TEST_F(OpusEncoderPluginUnitTest, QueueInputBuffer_NoInit_ErrorUnknown)
{
    ASSERT_NE(plugin_, nullptr);
    // SetParameter (sets channels_/sampleRate_ for the size check) but skip Init so pluginCodecPtr_ is null
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->QueueInputBuffer(input));
}

TEST_F(OpusEncoderPluginUnitTest, QueueInputBuffer_NullMeta)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    ASSERT_NE(input, nullptr);
    input->meta_ = nullptr; // encode succeeds, then the meta_ null-check returns error
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(input));
}

TEST_F(OpusEncoderPluginUnitTest, BasicMode_EncodeDirect)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false)); // Prepare already done
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    inputCallbackCount_ = 0;
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(1, inputCallbackCount_);
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    outputCallbackCount_ = 0;
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_EQ(1, outputCallbackCount_);
    EXPECT_GT(outputCallbackSize_, 0);
}

TEST_F(OpusEncoderPluginUnitTest, BasicMode_QueueOutputNull)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    shared_ptr<AVBuffer> nullBuf = nullptr;
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->QueueOutputBuffer(nullBuf));
}

TEST_F(OpusEncoderPluginUnitTest, QueueOutputBuffer_NoInit_ErrorWrongState)
{
    ASSERT_NE(plugin_, nullptr);
    // no Init: pluginCodecPtr_ is null
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    EXPECT_EQ(Status::ERROR_WRONG_STATE, plugin_->QueueOutputBuffer(output));
}

TEST_F(OpusEncoderPluginUnitTest, QueueOutputBuffer_NotEnoughData)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    // no input queued yet, encodeLen_ == 0
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    EXPECT_EQ(Status::ERROR_NOT_ENOUGH_DATA, plugin_->QueueOutputBuffer(output));
}

TEST_F(OpusEncoderPluginUnitTest, QueueOutputBuffer_EosFlag)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    output->flag_ = BUFFER_FLAG_EOS; // encodeLen_ == 0 -> EOS branch -> END_OF_STREAM
    outputCallbackCount_ = 0;
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::END_OF_STREAM, ret);
    EXPECT_EQ(1, outputCallbackCount_);
}

TEST_F(OpusEncoderPluginUnitTest, BasicMode_OutputWriteFails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input)); // encodeLen_ > 0 now
    auto output = CreateOutputBuffer(1); // 1: capacity smaller than encoded packet
    EXPECT_EQ(Status::ERROR_WRONG_STATE, plugin_->QueueOutputBuffer(output));
}

TEST_F(OpusEncoderPluginUnitTest, CacheMode_InputLessThanFrame_EarlyReturn)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    auto input = CreatePcmInputBuffer(SMALL_RESIDUAL_BYTES, false); // < one frame, no encode yet
    inputCallbackCount_ = 0;
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(1, inputCallbackCount_);
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    EXPECT_EQ(Status::ERROR_NOT_ENOUGH_DATA, plugin_->QueueOutputBuffer(output)); // encodeLen_ == 0
}

TEST_F(OpusEncoderPluginUnitTest, CacheMode_EncodeProducing)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    // feed two frames: QueueInputBuffer encodes the first and leaves one frame in
    // pcmCache_, so QueueOutputBuffer delivers the packet and encodes the residual
    // frame, returning ERROR_AGAIN.
    auto input = CreatePcmInputBuffer(TWO_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    outputCallbackCount_ = 0;
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::ERROR_AGAIN, ret);
    EXPECT_EQ(1, outputCallbackCount_);
    EXPECT_GT(outputCallbackSize_, 0);
}

TEST_F(OpusEncoderPluginUnitTest, CacheMode_MultipleFramesDrain)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    outputCallbackCount_ = 0;
    int encodeCount = 0;
    while (true) {
        auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
        Status ret = plugin_->QueueOutputBuffer(output);
        if (ret != Status::ERROR_AGAIN) {
            break;
        }
        encodeCount++;
        if (encodeCount > 30) { // 30: max safety iteration limit
            break;
        }
    }
    EXPECT_GT(encodeCount, 0);
    // Every QueueOutputBuffer during the drain triggers OnOutputBufferDone, including
    // the final call that returns OK (it delivers the last packet). So the total
    // output callback count is one more than the ERROR_AGAIN count.
    EXPECT_EQ(encodeCount + 1, outputCallbackCount_);
}

TEST_F(OpusEncoderPluginUnitTest, CacheMode_FullFlow_MultipleInputsThenEos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    for (int i = 0; i < 3; i++) { // 3: feed several multi-frame inputs
        auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
        EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    }
    int outputFrames = 0;
    while (true) {
        auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
        Status ret = plugin_->QueueOutputBuffer(output);
        if (ret != Status::ERROR_AGAIN) {
            break;
        }
        outputFrames++;
        if (outputFrames > 100) { // 100: max safety iteration limit for drain
            break;
        }
    }
    EXPECT_GT(outputFrames, 0);
    auto eosInput = CreatePcmInputBuffer(0, true);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(eosInput)); // resets encodeLen_ to 0
    bool gotEos = false;
    int maxIter = 50; // 50: max iterations to wait for EOS output
    while (maxIter-- > 0) {
        auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
        output->flag_ = BUFFER_FLAG_EOS;
        Status ret = plugin_->QueueOutputBuffer(output);
        if (ret == Status::END_OF_STREAM) {
            gotEos = true;
            break;
        }
        if (ret != Status::ERROR_AGAIN && ret != Status::OK) {
            break;
        }
    }
    EXPECT_TRUE(gotEos);
}

TEST_F(OpusEncoderPluginUnitTest, FormatConvert_CacheEncode)
{
    ASSERT_NE(plugin_, nullptr);
    // enable format convert with S32LE input -> S16LE internally, cache mode
    EXPECT_EQ(Status::OK, InitAndConfigure(true, OPUS_BITRATE,
        AudioSampleFormat::SAMPLE_S32LE, true));
    // two S32LE frames (7680 bytes) convert to two S16LE frames (3840 bytes) in pcmCache_
    auto input = CreatePcmInputBuffer(S32_FRAME_BYTES * 2, false);
    inputCallbackCount_ = 0;
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(1, inputCallbackCount_);
    outputCallbackCount_ = 0;
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::ERROR_AGAIN, ret); // residual frame still cached
    EXPECT_EQ(1, outputCallbackCount_);
    EXPECT_GT(outputCallbackSize_, 0);
}

TEST_F(OpusEncoderPluginUnitTest, PtsMode_FirstInputStart_Normal)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false, OPUS_BITRATE,
        AudioSampleFormat::SAMPLE_S16LE, false, OPUS_SAMPLE_RATE, OPUS_CHANNELS,
        PTS_MODE_FIRST_INPUT_START));
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    constexpr int64_t inputPts = 1000;
    input->pts_ = inputPts;
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input)); // sets prevPts_ = inputPts
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::OK, ret);
    // ptsMode_ != DEFAULT: output pts_ is set to prevPts_ (the input pts)
    EXPECT_EQ(inputPts, output->pts_);
}

TEST_F(OpusEncoderPluginUnitTest, PtsMode_FirstInputStart_Negative)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false, OPUS_BITRATE,
        AudioSampleFormat::SAMPLE_S16LE, false, OPUS_SAMPLE_RATE, OPUS_CHANNELS,
        PTS_MODE_FIRST_INPUT_START));
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    constexpr int64_t inputPts = -1000;
    input->pts_ = inputPts;
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input)); // prevPts_ = -1000
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::OK, ret);
    // prevPts_ < 0 branch: output pts_ = prevPts_ (-1000), then prevPts_ += frameDuration
    EXPECT_EQ(inputPts, output->pts_);
}

TEST_F(OpusEncoderPluginUnitTest, PtsMode_FirstInputStart_Overflow)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false, OPUS_BITRATE,
        AudioSampleFormat::SAMPLE_S16LE, false, OPUS_SAMPLE_RATE, OPUS_CHANNELS,
        PTS_MODE_FIRST_INPUT_START));
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    input->pts_ = INT64_MAX;
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input)); // prevPts_ = INT64_MAX
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::OK, ret);
    // overflow branch: output pts_ = prevPts_ (INT64_MAX)
    EXPECT_EQ(INT64_MAX, output->pts_);
}

TEST_F(OpusEncoderPluginUnitTest, Flush_ClearsCache)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    // QueueInputBuffer encodes one frame and leaves residual in pcmCache_; encodeLen_ > 0.
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Flush());
    // Flush clears pcmCache_ but does not reset encodeLen_; send EOS input to reset
    // encodeLen_ to 0, then QueueOutputBuffer(EOS) returns END_OF_STREAM (cache cleared).
    auto eosInput = CreatePcmInputBuffer(0, true);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(eosInput)); // encodeLen_ = 0
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    output->flag_ = BUFFER_FLAG_EOS;
    EXPECT_EQ(Status::END_OF_STREAM, plugin_->QueueOutputBuffer(output));
}

TEST_F(OpusEncoderPluginUnitTest, Reset_ResetsState)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Reset());
    // After Reset, cache is disabled but the encoder (enc_) is still usable, so a
    // non-cache encode of one full frame still succeeds.
    auto input2 = CreatePcmInputBuffer(FRAME_BYTES, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input2));
}

TEST_F(OpusEncoderPluginUnitTest, Stop_Succeeds)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Stop());
}

TEST_F(OpusEncoderPluginUnitTest, GetInputBuffers_And_GetOutputBuffers)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    vector<shared_ptr<AVBuffer>> inputBuffers;
    EXPECT_EQ(Status::OK, plugin_->GetInputBuffers(inputBuffers));
    vector<shared_ptr<AVBuffer>> outputBuffers;
    EXPECT_EQ(Status::OK, plugin_->GetOutputBuffers(outputBuffers));
}

TEST_F(OpusEncoderPluginUnitTest, Prepare_And_Release)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(OPUS_BITRATE));
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin_->Prepare());
    EXPECT_EQ(Status::OK, plugin_->Release());
}

}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS
