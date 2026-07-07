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
constexpr int32_t AMR_CHANNELS = 1;
constexpr int32_t AMR_SAMPLE_RATE = 8000;
constexpr int32_t AMR_BITRATE = 12200;       // 12200: MR122 mode
constexpr int32_t AMR_BITRATE_LOW = 4750;    // 4750: MR475 mode (lowest)
constexpr int32_t AMR_FRAME_SAMPLES = 160;   // 160: amr-nb fixed frame length in samples
constexpr int32_t FRAME_BYTES = AMR_FRAME_SAMPLES * 2; // 320: one S16LE mono frame
constexpr int32_t TWO_FRAME_PCM = FRAME_BYTES * 2; // 640: two frames, leaves residual for ERROR_AGAIN
constexpr int32_t S32_FRAME_BYTES = AMR_FRAME_SAMPLES * 4; // 640: one S32LE mono frame
constexpr int32_t MULTI_FRAME_PCM = FRAME_BYTES * 4; // 1280: enough for 4 frames in pcmCache
constexpr int32_t SMALL_RESIDUAL_BYTES = 100; // 100: less than one frame (320)
constexpr int32_t OUTPUT_CAPACITY = 256;      // 256: larger than any amr-nb packet
constexpr int32_t MAX_INPUT_BUFFER_SIZE_LIMIT = 10 * 1024 * 1024; // 10MB: clamp limit in plugin
constexpr int32_t OVER_LIMIT_BUFFER_SIZE = 11 * 1024 * 1024; // 11MB: exceeds clamp limit
// 2: AUDIO_ENCODER_PTS_MODE_FIRST_INPUT_START
constexpr int32_t PTS_MODE_FIRST_INPUT_START = 2;
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {

class AmrNbEncoderPluginUnitTest : public testing::Test, public DataCallback {
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

    Status InitAndConfigure(bool enableCache, int32_t bitrate = AMR_BITRATE,
        AudioSampleFormat fmt = AudioSampleFormat::SAMPLE_S16LE, bool enableConvert = false)
    {
        auto ret = plugin_->Init();
        if (ret != Status::OK) {
            return ret;
        }
        plugin_->SetDataCallback(this);
        auto meta = make_shared<Meta>();
        meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
        meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
        meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(fmt);
        meta->Set<Tag::MEDIA_BITRATE>(bitrate);
        if (enableConvert) {
            meta->Set<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(true);
        }
        if (enableCache) {
            meta->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(MULTI_FRAME_PCM);
        }
        ret = plugin_->SetParameter(meta);
        if (ret != Status::OK) {
            return ret;
        }
        return plugin_->Prepare();
    }

protected:
    shared_ptr<CodecPlugin> plugin_ = nullptr;
    int32_t inputCallbackCount_ = 0;
    int32_t outputCallbackCount_ = 0;
    int32_t outputCallbackSize_ = 0;
};

void AmrNbEncoderPluginUnitTest::SetUpTestCase(void)
{
    cout << "[AmrNbEncoder SetUpTestCase]" << endl;
}

void AmrNbEncoderPluginUnitTest::TearDownTestCase(void)
{
    cout << "[AmrNbEncoder TearDownTestCase]" << endl;
}

void AmrNbEncoderPluginUnitTest::SetUp()
{
    auto amrPlugin = PluginManagerV2::Instance().CreatePluginByName(
        std::string(MediaAVCodec::AVCodecCodecName::AUDIO_ENCODER_AMRNB_NAME));
    if (amrPlugin == nullptr) {
        cout << "WARNING: AmrNb encoder plugin not available, tests will skip" << endl;
        return;
    }
    plugin_ = reinterpret_pointer_cast<CodecPlugin>(amrPlugin);
    inputCallbackCount_ = 0;
    outputCallbackCount_ = 0;
    outputCallbackSize_ = 0;
}

void AmrNbEncoderPluginUnitTest::TearDown()
{
    if (plugin_) {
        plugin_->Stop();
        plugin_->Release();
        plugin_ = nullptr;
    }
}

TEST_F(AmrNbEncoderPluginUnitTest, PluginLoadable)
{
    ASSERT_NE(plugin_, nullptr) << "AmrNb plugin should load via PluginManagerV2";
}

TEST_F(AmrNbEncoderPluginUnitTest, Init_SetParameter_Start_Succeed)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    meta->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(MULTI_FRAME_PCM);
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin_->Prepare());
    EXPECT_EQ(Status::OK, plugin_->Start());
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_NullParameter)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    shared_ptr<Meta> nullMeta = nullptr;
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(nullMeta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_InvalidChannelCount)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2: invalid, amr-nb only supports mono
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_MissingChannelCount)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_InvalidSampleRate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(16000); // 16000: invalid, amr-nb only supports 8000
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_MissingBitrate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_InvalidBitrate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(99999); // 99999: not in amr-nb bitrate map
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_LowBitrate_Succeed)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE_LOW); // 4750: lowest valid bitrate MR475
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_MissingSampleFormat)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_WrongSampleFormat_NoConvert)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S32LE); // not S16LE, convert off
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_UnsupportedSampleFormat_WithConvert)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    // 99: a value not in the extended sample format set
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(static_cast<AudioSampleFormat>(99));
    meta->Set<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(true);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_PtsMode_Present_Branch)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    meta->Set<Tag::AUDIO_ENCODE_PTS_MODE>(PTS_MODE_FIRST_INPUT_START); // covers Get<PTS_MODE> true branch
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(AmrNbEncoderPluginUnitTest, SetParameter_MaxInputBufferSize_ExceedsLimit)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    // exceeds 10MB limit, plugin should clamp and still enable cache
    meta->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(OVER_LIMIT_BUFFER_SIZE);
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    auto outMeta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(outMeta));
    int32_t maxInputSize = 0;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    EXPECT_EQ(MAX_INPUT_BUFFER_SIZE_LIMIT, maxInputSize);
}

TEST_F(AmrNbEncoderPluginUnitTest, GetParameter_NullParameter)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    shared_ptr<Meta> nullMeta = nullptr;
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->GetParameter(nullMeta));
}

TEST_F(AmrNbEncoderPluginUnitTest, GetParameter_ClampInvalidSizes)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(-1);   // -1: <=0, should be clamped to default
    meta->Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(-1);  // -1: <=0, should be clamped to default
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

TEST_F(AmrNbEncoderPluginUnitTest, GetParameter_ClampOversizeSizes)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(AMR_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(AMR_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(AMR_BITRATE);
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(1024 * 1024);  // larger than default input size
    meta->Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(1024 * 1024); // larger than default output size
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    auto outMeta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(outMeta));
}

TEST_F(AmrNbEncoderPluginUnitTest, GetParameter_CacheEnabled)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    auto outMeta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(outMeta));
}

TEST_F(AmrNbEncoderPluginUnitTest, QueueInputBuffer_NullBuffer)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    shared_ptr<AVBuffer> nullBuf = nullptr;
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(nullBuf));
}

TEST_F(AmrNbEncoderPluginUnitTest, QueueInputBuffer_NullMemory)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    ASSERT_NE(input, nullptr);
    input->memory_ = nullptr; // force null memory branch
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(input));
}

TEST_F(AmrNbEncoderPluginUnitTest, QueueInputBuffer_EosFlag)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto input = CreatePcmInputBuffer(0, true);
    inputCallbackCount_ = 0;
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(1, inputCallbackCount_);
}

TEST_F(AmrNbEncoderPluginUnitTest, QueueInputBuffer_ZeroSizeWithoutEos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto input = CreatePcmInputBuffer(0, false);
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(input));
}

TEST_F(AmrNbEncoderPluginUnitTest, BasicMode_EncodeDirect)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    EXPECT_EQ(Status::OK, plugin_->Start());
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

TEST_F(AmrNbEncoderPluginUnitTest, BasicMode_QueueOutputNull)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    shared_ptr<AVBuffer> nullBuf = nullptr;
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->QueueOutputBuffer(nullBuf));
}

TEST_F(AmrNbEncoderPluginUnitTest, BasicMode_OutputCapacityNotEnough)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    auto output = CreateOutputBuffer(1); // 1: capacity smaller than encoded packet
    EXPECT_EQ(Status::ERROR_NO_MEMORY, plugin_->QueueOutputBuffer(output));
}

TEST_F(AmrNbEncoderPluginUnitTest, BasicMode_OutputNullMemory)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    ASSERT_NE(output, nullptr);
    output->memory_ = nullptr; // force null memory branch (non-eos)
    EXPECT_EQ(Status::ERROR_NO_MEMORY, plugin_->QueueOutputBuffer(output));
}

TEST_F(AmrNbEncoderPluginUnitTest, QueueOutputBuffer_EosFlag)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    output->flag_ = BUFFER_FLAG_EOS;
    outputCallbackCount_ = 0;
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::END_OF_STREAM, ret);
    EXPECT_EQ(1, outputCallbackCount_);
}

TEST_F(AmrNbEncoderPluginUnitTest, CacheMode_EmptyCache_NotEnoughData)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    EXPECT_EQ(Status::ERROR_NOT_ENOUGH_DATA, plugin_->QueueOutputBuffer(output));
}

TEST_F(AmrNbEncoderPluginUnitTest, CacheMode_InputLessThanFrame_EarlyReturn)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(SMALL_RESIDUAL_BYTES, false); // < one frame, no encode yet
    inputCallbackCount_ = 0;
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(1, inputCallbackCount_);
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    EXPECT_EQ(Status::ERROR_NOT_ENOUGH_DATA, plugin_->QueueOutputBuffer(output));
}

TEST_F(AmrNbEncoderPluginUnitTest, CacheMode_EncodeProducing)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    EXPECT_EQ(Status::OK, plugin_->Start());
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

TEST_F(AmrNbEncoderPluginUnitTest, CacheMode_MultipleFramesDrain)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    EXPECT_EQ(Status::OK, plugin_->Start());
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

TEST_F(AmrNbEncoderPluginUnitTest, CacheMode_FullFlow_MultipleInputsThenEos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    EXPECT_EQ(Status::OK, plugin_->Start());
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
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(eosInput));
    bool gotEos = false;
    int maxIter = 50; // 50: max iterations to wait for EOS output
    while (maxIter-- > 0) {
        auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
        output->flag_ = BUFFER_FLAG_EOS;
        Status ret = plugin_->QueueOutputBuffer(output);
        if ((output->flag_ & BUFFER_FLAG_EOS) != 0) {
            gotEos = true;
            break;
        }
        if (ret != Status::ERROR_AGAIN && ret != Status::OK) {
            break;
        }
    }
    EXPECT_TRUE(gotEos);
}

TEST_F(AmrNbEncoderPluginUnitTest, FormatConvert_EncodeDirect)
{
    ASSERT_NE(plugin_, nullptr);
    // enable format convert with S32LE input -> S16LE internally
    EXPECT_EQ(Status::OK, InitAndConfigure(false, AMR_BITRATE,
        AudioSampleFormat::SAMPLE_S32LE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(S32_FRAME_BYTES, false); // 160 samples * 4 bytes
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

TEST_F(AmrNbEncoderPluginUnitTest, Flush_ClearsCache)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    // QueueInputBuffer encodes one frame and leaves a 3-frame residual in pcmCache_.
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Flush());
    // Flush() clears pcmCache_ but does NOT reset the pending packetSize_, so the
    // first QueueOutputBuffer after flush still delivers that leftover packet (OK).
    auto output1 = CreateOutputBuffer(OUTPUT_CAPACITY);
    EXPECT_EQ(Status::OK, plugin_->QueueOutputBuffer(output1));
    // The 3-frame residual that was in pcmCache_ is gone, so no more data is available.
    auto output2 = CreateOutputBuffer(OUTPUT_CAPACITY);
    EXPECT_EQ(Status::ERROR_NOT_ENOUGH_DATA, plugin_->QueueOutputBuffer(output2));
}

TEST_F(AmrNbEncoderPluginUnitTest, Reset_ResetsState)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Reset());
    // reconfigure after reset
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    EXPECT_EQ(Status::OK, plugin_->Start());
}

TEST_F(AmrNbEncoderPluginUnitTest, Stop_AndRestart)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Stop());
    EXPECT_EQ(Status::OK, plugin_->Start());
}

TEST_F(AmrNbEncoderPluginUnitTest, GetInputBuffers_And_GetOutputBuffers)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(false));
    vector<shared_ptr<AVBuffer>> inputBuffers;
    EXPECT_EQ(Status::OK, plugin_->GetInputBuffers(inputBuffers));
    vector<shared_ptr<AVBuffer>> outputBuffers;
    EXPECT_EQ(Status::OK, plugin_->GetOutputBuffers(outputBuffers));
}

TEST_F(AmrNbEncoderPluginUnitTest, Prepare_And_Release)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    EXPECT_EQ(Status::OK, plugin_->Prepare());
    EXPECT_EQ(Status::OK, plugin_->Release());
}

}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS
