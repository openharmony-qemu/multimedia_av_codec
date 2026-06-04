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
constexpr int32_t MP3_CHANNELS = 2;
constexpr int32_t MP3_SAMPLE_RATE = 44100;
constexpr int32_t MP3_BIT_RATE = 128000;
constexpr int32_t MP3_FRAME_SIZE = 1152;
constexpr int32_t FRAME_BYTES = MP3_FRAME_SIZE * MP3_CHANNELS * 2; // 4608
// Allocate enough for 10+ frames in pcmCache to test retry and multiple drains
constexpr int32_t MULTI_FRAME_PCM = FRAME_BYTES * 12;
// Small residual (less than frame, more than minResidual=channels*2=4)
constexpr int32_t SMALL_RESIDUAL_BYTES = 256;
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {

class Mp3EncoderPluginUnitTest : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

    void OnInputBufferDone(const shared_ptr<AVBuffer> &inputBuffer) override
    {
        (void)inputBuffer;
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
            vector<uint8_t> data(sizeInBytes, 0x10);
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

    Status InitAndConfigure(int32_t channels, int32_t sampleRate, int32_t bitRate, bool enableCache)
    {
        auto ret = plugin_->Init();
        if (ret != Status::OK) return ret;
        plugin_->SetDataCallback(this);
        auto meta = make_shared<Meta>();
        meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
        meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
        meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
        meta->Set<Tag::MEDIA_BITRATE>(bitRate);
        if (enableCache) {
            meta->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(MULTI_FRAME_PCM);
        }
        ret = plugin_->SetParameter(meta);
        if (ret != Status::OK) return ret;
        return plugin_->Prepare();
    }

protected:
    shared_ptr<CodecPlugin> plugin_ = nullptr;
    int32_t outputCallbackCount_ = 0;
    int32_t outputCallbackSize_ = 0;
};

void Mp3EncoderPluginUnitTest::SetUpTestCase(void)
{
    cout << "[Mp3Encoder SetUpTestCase]" << endl;
}

void Mp3EncoderPluginUnitTest::TearDownTestCase(void)
{
    cout << "[Mp3Encoder TearDownTestCase]" << endl;
}

void Mp3EncoderPluginUnitTest::SetUp()
{
    auto mp3Plugin = PluginManagerV2::Instance().CreatePluginByName(
        std::string(MediaAVCodec::AVCodecCodecName::AUDIO_ENCODER_MP3_NAME));
    if (mp3Plugin == nullptr) {
        cout << "WARNING: MP3 encoder plugin not available, tests will skip" << endl;
        return;
    }
    plugin_ = reinterpret_pointer_cast<CodecPlugin>(mp3Plugin);
    outputCallbackCount_ = 0;
    outputCallbackSize_ = 0;
}

void Mp3EncoderPluginUnitTest::TearDown()
{
    if (plugin_) {
        plugin_->Stop();
        plugin_->Release();
        plugin_ = nullptr;
    }
}

TEST_F(Mp3EncoderPluginUnitTest, PluginLoadable)
{
    ASSERT_NE(plugin_, nullptr) << "MP3 plugin should load via PluginManagerV2";
}

TEST_F(Mp3EncoderPluginUnitTest, Init_SetParameter_Start_Succeed)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MP3_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(MP3_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(MP3_BIT_RATE);
    meta->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(MULTI_FRAME_PCM);
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin_->Prepare());
    EXPECT_EQ(Status::OK, plugin_->Start());
}

TEST_F(Mp3EncoderPluginUnitTest, SetParameter_InvalidChannelCount)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(7); // invalid
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(MP3_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(MP3_BIT_RATE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(Mp3EncoderPluginUnitTest, SetParameter_InvalidSampleRate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MP3_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(99999); // invalid
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(MP3_BIT_RATE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(Mp3EncoderPluginUnitTest, SetParameter_InvalidBitRate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MP3_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(MP3_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_BITRATE>(99999); // invalid
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(Mp3EncoderPluginUnitTest, SetParameter_MissingChannelCount)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(MP3_SAMPLE_RATE);
    meta->Set<Tag::MEDIA_BITRATE>(MP3_BIT_RATE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(Mp3EncoderPluginUnitTest, SetParameter_MissingSampleRate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MP3_CHANNELS);
    meta->Set<Tag::MEDIA_BITRATE>(MP3_BIT_RATE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(Mp3EncoderPluginUnitTest, SetParameter_MissingBitRate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MP3_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(MP3_SAMPLE_RATE);
    meta->Set<Tag::MEDIA_BITRATE>(MP3_BIT_RATE);
    // bitrate missing in next set
    auto meta2 = make_shared<Meta>();
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta2));
}

TEST_F(Mp3EncoderPluginUnitTest, Start_BeforeInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->Start());
}

TEST_F(Mp3EncoderPluginUnitTest, QueueOutputBuffer_NullBuffer)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    shared_ptr<AVBuffer> nullBuf = nullptr;
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->QueueOutputBuffer(nullBuf));
}

TEST_F(Mp3EncoderPluginUnitTest, CacheMode_EmptyCache_NotEnoughData)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto output = CreateOutputBuffer(16384);
    EXPECT_EQ(Status::ERROR_NOT_ENOUGH_DATA, plugin_->QueueOutputBuffer(output));
}

TEST_F(Mp3EncoderPluginUnitTest, CacheMode_Branch1_EncodeProducing)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    outputCallbackCount_ = 0;
    auto output = CreateOutputBuffer(16384);
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::ERROR_AGAIN, ret);
    EXPECT_EQ(1, outputCallbackCount_);
    EXPECT_GT(outputCallbackSize_, 0);
}

TEST_F(Mp3EncoderPluginUnitTest, CacheMode_Branch1_MultipleFramesDrain)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    outputCallbackCount_ = 0;
    int encodeCount = 0;
    while (true) {
        auto output = CreateOutputBuffer(16384);
        Status ret = plugin_->QueueOutputBuffer(output);
        if (ret != Status::ERROR_AGAIN) break;
        encodeCount++;
        if (encodeCount > 30) break; // safety
    }
    EXPECT_GT(encodeCount, 0);
    EXPECT_EQ(encodeCount, outputCallbackCount_);
}

TEST_F(Mp3EncoderPluginUnitTest, CacheMode_Branch2_ResidualEncode_SmallResidual)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    while (true) {
        auto output = CreateOutputBuffer(16384);
        if (plugin_->QueueOutputBuffer(output) != Status::ERROR_AGAIN) break;
    }
    auto residualInput = CreatePcmInputBuffer(SMALL_RESIDUAL_BYTES, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(residualInput));
    auto eosInput = CreatePcmInputBuffer(0, true);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(eosInput));
    outputCallbackCount_ = 0;
    int residualCount = 0;
    while (true) {
        auto output = CreateOutputBuffer(16384);
        output->flag_ = BUFFER_FLAG_EOS;
        Status ret = plugin_->QueueOutputBuffer(output);
        if (ret != Status::ERROR_AGAIN && ret != Status::OK) break;
        if (outputCallbackCount_ > 0) residualCount++;
        if (residualCount > 10) break;
    }
    EXPECT_GT(outputCallbackCount_, 0);
}

TEST_F(Mp3EncoderPluginUnitTest, CacheMode_Branch3_Flush_NoResidual)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    while (true) {
        auto output = CreateOutputBuffer(16384);
        if (plugin_->QueueOutputBuffer(output) != Status::ERROR_AGAIN) break;
    }
    auto eosInput = CreatePcmInputBuffer(0, true);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(eosInput));
    outputCallbackCount_ = 0;
    outputCallbackSize_ = -1;
    auto output = CreateOutputBuffer(16384);
    output->flag_ = BUFFER_FLAG_EOS;
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_EQ(1, outputCallbackCount_);
    EXPECT_TRUE((output->flag_ & BUFFER_FLAG_EOS) != 0);
    EXPECT_GE(outputCallbackSize_, 0);
}

TEST_F(Mp3EncoderPluginUnitTest, CacheMode_Branch3_FlushWithEos_NoInput)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto eosInput = CreatePcmInputBuffer(0, true);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(eosInput));
    auto output = CreateOutputBuffer(16384);
    output->flag_ = BUFFER_FLAG_EOS;
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_TRUE((output->flag_ & BUFFER_FLAG_EOS) != 0);
}

TEST_F(Mp3EncoderPluginUnitTest, CacheMode_FullFlow_MultipleInputsThenEos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    for (int i = 0; i < 5; i++) {
        auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
        EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    }
    int outputFrames = 0;
    while (true) {
        auto output = CreateOutputBuffer(16384);
        Status ret = plugin_->QueueOutputBuffer(output);
        if (ret != Status::ERROR_AGAIN) break;
        outputFrames++;
        if (outputFrames > 100) break;
    }
    EXPECT_GT(outputFrames, 0);
    auto eosInput = CreatePcmInputBuffer(0, true);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(eosInput));
    bool gotEos = false;
    int maxIter = 50;
    while (maxIter-- > 0) {
        auto output = CreateOutputBuffer(16384);
        output->flag_ = BUFFER_FLAG_EOS;
        Status ret = plugin_->QueueOutputBuffer(output);
        if ((output->flag_ & BUFFER_FLAG_EOS) != 0) {
            gotEos = true;
            break;
        }
        if (ret != Status::ERROR_AGAIN && ret != Status::OK) break;
    }
    EXPECT_TRUE(gotEos);
}

TEST_F(Mp3EncoderPluginUnitTest, BasicMode_EncodeDirect)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, false));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    auto output = CreateOutputBuffer(16384);
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_TRUE(ret == Status::OK || ret == Status::ERROR_NOT_ENOUGH_DATA);
}

TEST_F(Mp3EncoderPluginUnitTest, BasicMode_QueueOutputNull_ReturnsError)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, false));
    shared_ptr<AVBuffer> nullBuf = nullptr;
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->QueueOutputBuffer(nullBuf));
}

TEST_F(Mp3EncoderPluginUnitTest, BasicMode_InvalidInputSize)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, false));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(32 * 1024, false);
    EXPECT_NE(Status::OK, plugin_->QueueInputBuffer(input));
}

TEST_F(Mp3EncoderPluginUnitTest, BasicMode_ZeroSizeWithoutEos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, false));
    auto input = CreatePcmInputBuffer(0, false);
    EXPECT_NE(Status::OK, plugin_->QueueInputBuffer(input));
}

TEST_F(Mp3EncoderPluginUnitTest, Flush_ClearsCache)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Flush());
    auto output = CreateOutputBuffer(16384);
    EXPECT_EQ(Status::ERROR_NOT_ENOUGH_DATA, plugin_->QueueOutputBuffer(output));
}

TEST_F(Mp3EncoderPluginUnitTest, Reset_ResetsState)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Reset());
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
}

TEST_F(Mp3EncoderPluginUnitTest, MonoChannel_Encode)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(1, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    int monoFrameBytes = MP3_FRAME_SIZE * 1 * 2; // 2304
    auto input = CreatePcmInputBuffer(monoFrameBytes * 3, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    outputCallbackCount_ = 0;
    while (true) {
        auto output = CreateOutputBuffer(16384);
        Status ret = plugin_->QueueOutputBuffer(output);
        if (ret != Status::ERROR_AGAIN) break;
    }
    EXPECT_GT(outputCallbackCount_, 0);
}

TEST_F(Mp3EncoderPluginUnitTest, CacheMode_FirstCallNoEos_SecondCallEos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(MULTI_FRAME_PCM, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    outputCallbackCount_ = 0;
    int drainCount = 0;
    while (true) {
        auto output = CreateOutputBuffer(16384);
        Status ret = plugin_->QueueOutputBuffer(output);
        if (ret != Status::ERROR_AGAIN) break;
        drainCount++;
        if (drainCount > 50) break;
    }
    auto eosInput = CreatePcmInputBuffer(0, true);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(eosInput));
    bool eosReceived = false;
    while (true) {
        auto output = CreateOutputBuffer(16384);
        output->flag_ = BUFFER_FLAG_EOS;
        Status ret = plugin_->QueueOutputBuffer(output);
        if ((output->flag_ & BUFFER_FLAG_EOS) != 0) {
            eosReceived = true;
            break;
        }
        if (ret != Status::ERROR_AGAIN && ret != Status::OK) break;
    }
    EXPECT_TRUE(eosReceived);
}

TEST_F(Mp3EncoderPluginUnitTest, CacheMode_Branch2_ResidualEncode_SmallEosResidual)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    int smallSize = 20; // > minResidual (4) but < frameBytes (4608)
    auto input = CreatePcmInputBuffer(smallSize, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    auto eosInput = CreatePcmInputBuffer(0, true);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(eosInput));
    outputCallbackCount_ = 0;
    auto output1 = CreateOutputBuffer(16384);
    output1->flag_ = BUFFER_FLAG_EOS;
    Status ret = plugin_->QueueOutputBuffer(output1);
    EXPECT_EQ(Status::ERROR_AGAIN, ret);
    EXPECT_EQ(1, outputCallbackCount_);
    if (ret == Status::ERROR_AGAIN) {
        auto output2 = CreateOutputBuffer(16384);
        output2->flag_ = BUFFER_FLAG_EOS;
        Status ret2 = plugin_->QueueOutputBuffer(output2);
        EXPECT_EQ(Status::OK, ret2);
        EXPECT_TRUE((output2->flag_ & BUFFER_FLAG_EOS) != 0);
    }
}

TEST_F(Mp3EncoderPluginUnitTest, CacheMode_GetParameter)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    auto meta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(meta));
}

TEST_F(Mp3EncoderPluginUnitTest, Stop_AndRestart)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(MP3_CHANNELS, MP3_SAMPLE_RATE, MP3_BIT_RATE, true));
    EXPECT_EQ(Status::OK, plugin_->Start());
    auto input = CreatePcmInputBuffer(FRAME_BYTES, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Stop());
    EXPECT_EQ(Status::OK, plugin_->Start());
}

}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS
