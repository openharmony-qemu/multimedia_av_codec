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
constexpr int32_t OPUS_CHANNELS = 1;
constexpr int32_t OPUS_SAMPLE_RATE = 48000;
constexpr int32_t OPUS_FRAME_SAMPLES = 960; // 48000 * 0.02s = 960 samples per 20ms frame
constexpr int32_t FRAME_BYTES = OPUS_FRAME_SAMPLES * 2; // 1920: one S16LE mono 20ms frame
constexpr int32_t OUTPUT_CAPACITY = 8192; // 8192: larger than one decoded 60ms mono frame (5760)
constexpr int32_t OPUS_HEAD_SIZE = 19;
constexpr int32_t OFFSET_2 = 2;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 1500;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 255;
constexpr int32_t SKIP_INFO_SIZE = 10; // 10: bytes parsed by the plugin (skipSamples + discardEnd + 2)
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {

class OpusDecoderPluginUnitTest : public testing::Test, public DataCallback {
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

    // Produce a real, valid opus packet by driving the opus ENCODER plugin (loaded via
    // dlopen, same as the decoder under test) with one 20ms silence frame. This keeps
    // the test binary free of any direct opus dependency. The packet decodes to
    // OPUS_FRAME_SAMPLES (960) samples at the decoder side.
    vector<uint8_t> MakeOpusPacket()
    {
        auto enc = PluginManagerV2::Instance().CreatePluginByName(
            std::string(MediaAVCodec::AVCodecCodecName::AUDIO_ENCODER_OPUS_NAME));
        auto encoder = reinterpret_pointer_cast<CodecPlugin>(enc);
        vector<uint8_t> packet;
        do {
            if (encoder == nullptr) {
                break;
            }
            if (encoder->Init() != Status::OK) {
                break;
            }
            auto meta = make_shared<Meta>();
            meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
            meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
            meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
            meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(64000)); // 64000: valid opus bitrate
            if (encoder->SetParameter(meta) != Status::OK) {
                break;
            }
            if (encoder->Prepare() != Status::OK) {
                break;
            }
            auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            auto inBuf = AVBuffer::CreateAVBuffer(allocator, FRAME_BYTES);
            if (inBuf == nullptr || inBuf->memory_ == nullptr) {
                break;
            }
            vector<uint8_t> pcm(FRAME_BYTES, 0); // 20ms silence
            inBuf->memory_->Write(pcm.data(), FRAME_BYTES, 0);
            inBuf->memory_->SetSize(FRAME_BYTES);
            inBuf->meta_ = make_shared<Meta>();
            if (encoder->QueueInputBuffer(inBuf) != Status::OK) {
                break;
            }
            auto outBuf = AVBuffer::CreateAVBuffer(allocator, OUTPUT_CAPACITY);
            if (encoder->QueueOutputBuffer(outBuf) != Status::OK) {
                break;
            }
            if (outBuf == nullptr || outBuf->memory_ == nullptr) {
                break;
            }
            uint8_t *addr = outBuf->memory_->GetAddr();
            int32_t sz = outBuf->memory_->GetSize();
            if (addr != nullptr && sz > 0) {
                packet.assign(addr, addr + sz);
            }
        } while (false);
        if (encoder) {
            encoder->Release();
        }
        return packet;
    }

    // Build a 10-byte BUFFER_SKIP_SAMPLES_INFO blob: skipSamples[0..3] LE,
    // discardEndSamples[4..7] LE, plus 2 padding bytes.
    vector<uint8_t> MakeSkipInfo(uint32_t skipSamples, uint32_t discardEnd)
    {
        vector<uint8_t> info(SKIP_INFO_SIZE, 0);
        for (int i = 0; i < 4; i++) { // 4: sizeof(uint32_t)
            info[i] = static_cast<uint8_t>((skipSamples >> (i * 8)) & 0xff);
            info[i + 4] = static_cast<uint8_t>((discardEnd >> (i * 8)) & 0xff); // 4: discardEnd offset
        }
        return info;
    }

    shared_ptr<AVBuffer> CreateInputBuffer(const vector<uint8_t> &data, bool isEos)
    {
        int32_t size = static_cast<int32_t>(data.size());
        auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        auto buffer = AVBuffer::CreateAVBuffer(allocator, size > 0 ? size : 1);
        if (buffer && buffer->memory_ && size > 0) {
            buffer->memory_->Write(data.data(), size, 0);
            buffer->memory_->SetSize(size);
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

    // Init + SetParameter (mono, S16LE) + Prepare, so the decoder is ready to decode.
    Status InitAndConfigure(bool enableSkip = false, bool setSampleFormat = true)
    {
        auto ret = plugin_->Init();
        if (ret != Status::OK) {
            return ret;
        }
        plugin_->SetDataCallback(this);
        auto meta = make_shared<Meta>();
        meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
        meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
        if (setSampleFormat) {
            meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
        }
        if (enableSkip) {
            meta->Set<Tag::ENABLE_BUFFER_SKIP_SAMPLES>(1);
        }
        ret = plugin_->SetParameter(meta);
        if (ret != Status::OK) {
            return ret;
        }
        return plugin_->Prepare();
    }

    // Run a skip-samples scenario: attach skipInfo to the input meta, queue input,
    // then queue output (optionally EOS). Resets outputCallbackCount_ first so the
    // caller can assert how many output callbacks fired. Returns the QO status.
    Status RunSkipScenario(const vector<uint8_t> &skipInfo, bool outputEos)
    {
        outputCallbackCount_ = 0;
        auto packet = MakeOpusPacket();
        auto input = CreateInputBuffer(packet, false);
        input->meta_->Set<Tag::BUFFER_SKIP_SAMPLES_INFO>(skipInfo);
        auto qret = plugin_->QueueInputBuffer(input);
        if (qret != Status::OK) {
            return qret;
        }
        auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
        if (outputEos) {
            output->flag_ = BUFFER_FLAG_EOS;
        }
        return plugin_->QueueOutputBuffer(output);
    }

protected:
    shared_ptr<CodecPlugin> plugin_ = nullptr;
    int32_t inputCallbackCount_ = 0;
    int32_t outputCallbackCount_ = 0;
    int32_t outputCallbackSize_ = 0;
};

void OpusDecoderPluginUnitTest::SetUpTestCase(void)
{
    cout << "[OpusDecoder SetUpTestCase]" << endl;
}

void OpusDecoderPluginUnitTest::TearDownTestCase(void)
{
    cout << "[OpusDecoder TearDownTestCase]" << endl;
}

void OpusDecoderPluginUnitTest::SetUp()
{
    auto opusPlugin = PluginManagerV2::Instance().CreatePluginByName(
        std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_OPUS_NAME));
    if (opusPlugin == nullptr) {
        cout << "WARNING: Opus decoder plugin not available, tests will skip" << endl;
        return;
    }
    plugin_ = reinterpret_pointer_cast<CodecPlugin>(opusPlugin);
    inputCallbackCount_ = 0;
    outputCallbackCount_ = 0;
    outputCallbackSize_ = 0;
}

void OpusDecoderPluginUnitTest::TearDown()
{
    if (plugin_) {
        plugin_->Stop();
        plugin_->Release(); // safe to call even if already released (returns ERROR_UNKNOWN)
        plugin_ = nullptr;
    }
}

TEST_F(OpusDecoderPluginUnitTest, PluginLoadable)
{
    ASSERT_NE(plugin_, nullptr) << "Opus decoder plugin should load via PluginManagerV2";
}

TEST_F(OpusDecoderPluginUnitTest, Init_SetParameter_Prepare_Start_Succeed)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin_->Prepare());
    EXPECT_EQ(Status::OK, plugin_->Start()); // Start == Prepare
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_MissingChannelCount)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_ChannelTooSmall)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MIN_CHANNELS - 1); // 0: < MIN_CHANNELS(1)
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_ChannelTooLarge)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MAX_CHANNELS + 1); // 256: > MAX_CHANNELS(255)
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_MissingSampleRate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_InvalidSampleRate)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(11025); // 11025: not in opus sample rate table
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_LowSampleRate_Succeed)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(8000); // 8000: lowest valid opus sample rate
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_MissingSampleFormat)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    // AUDIO_SAMPLE_FORMAT not set -> covers the warning branch (still OK)
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_MaxInputSize_Present)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(500); // 500: present, covers the Get branch
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_MaxOutputSize_Present)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(4096); // 4096: present, covers the Get branch
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_EnableSkipSamples)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::ENABLE_BUFFER_SKIP_SAMPLES>(1); // 1: enable, covers GetData true branch
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

// Build a multistream extradata blob (opus head + streams + coupled + mapping[channels]).
static vector<uint8_t> MakeMultistreamExtradata(int32_t channels, int32_t sizeOverride = -1)
{
    int32_t size = (sizeOverride >= 0) ? sizeOverride : (OPUS_HEAD_SIZE + OFFSET_2 + channels);
    vector<uint8_t> extra(size, 0);
    if (size >= OPUS_HEAD_SIZE + OFFSET_2 + channels) {
        extra[OPUS_HEAD_SIZE] = static_cast<uint8_t>(channels); // streams
        extra[OPUS_HEAD_SIZE + 1] = 0;                          // coupled
        for (int i = 0; i < channels; i++) {
            extra[OPUS_HEAD_SIZE + OFFSET_2 + i] = static_cast<uint8_t>(i); // mapping
        }
    }
    return extra;
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_Multistream_MissingCodecConfig)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(3); // 3: > 2 -> multistream
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    // MEDIA_CODEC_CONFIG not set
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_Multistream_ExtradataTooSmall)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(3); // 3: needs extradata >= 19 + 2 + 3 = 24 bytes
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_CODEC_CONFIG>(MakeMultistreamExtradata(3, 23)); // 23: too small
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_Multistream_ValidExtradata)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(3); // 3: multistream, channels in (2, 8] -> remap path
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::MEDIA_CODEC_CONFIG>(MakeMultistreamExtradata(3)); // 24 bytes: valid
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, SetParameter_Multistream_StereoNotMultistream)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2: not > 2 -> not multistream, no extradata needed
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
}

TEST_F(OpusDecoderPluginUnitTest, GetParameter_ClampInvalidSizes)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(-1);  // -1: <=0, clamped to default (1500)
    meta->Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(-1); // -1: <=0, clamped to default (8192)
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    auto outMeta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(outMeta));
    int32_t maxInputSize = 0;
    int32_t maxOutputSize = 0;
    EXPECT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    EXPECT_TRUE(outMeta->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize));
    EXPECT_EQ(INPUT_BUFFER_SIZE_DEFAULT, maxInputSize);
    EXPECT_EQ(OUTPUT_BUFFER_SIZE_DEFAULT, maxOutputSize);
}

TEST_F(OpusDecoderPluginUnitTest, GetParameter_ClampOversizeSizes)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(5000);  // 5000: > 1500, clamped to default
    meta->Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(20000); // 20000: > 8192, clamped to default
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    auto outMeta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(outMeta));
}

TEST_F(OpusDecoderPluginUnitTest, GetParameter_Multistream_NoClamp)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(3); // 3: multistream -> no clamp in GetParameter
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(5000); // 5000: would be clamped for mono, kept for multistream
    meta->Set<Tag::MEDIA_CODEC_CONFIG>(MakeMultistreamExtradata(3));
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    auto outMeta = make_shared<Meta>();
    EXPECT_EQ(Status::OK, plugin_->GetParameter(outMeta));
    int32_t maxInputSize = 0;
    EXPECT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    EXPECT_EQ(5000, maxInputSize); // not clamped because isMultistream_
}

TEST_F(OpusDecoderPluginUnitTest, Prepare_NoInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->Prepare()); // pluginCodecPtr_ null
}

TEST_F(OpusDecoderPluginUnitTest, Start_NoInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->Start());
}

TEST_F(OpusDecoderPluginUnitTest, Reset_NoInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->Reset());
}

TEST_F(OpusDecoderPluginUnitTest, Flush_NoInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->Flush());
}

TEST_F(OpusDecoderPluginUnitTest, Release_NoInit_Fails)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->Release());
}

TEST_F(OpusDecoderPluginUnitTest, QueueInputBuffer_NullBuffer)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    shared_ptr<AVBuffer> nullBuf = nullptr;
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(nullBuf));
}

TEST_F(OpusDecoderPluginUnitTest, QueueInputBuffer_NullMemory)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    auto packet = MakeOpusPacket();
    auto input = CreateInputBuffer(packet, false);
    ASSERT_NE(input, nullptr);
    input->memory_ = nullptr; // force null memory branch
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(input));
}

TEST_F(OpusDecoderPluginUnitTest, QueueInputBuffer_ZeroSizeWithoutEos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    auto input = CreateInputBuffer({}, false); // empty, non-EOS
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(input));
}

TEST_F(OpusDecoderPluginUnitTest, QueueInputBuffer_NoInit_ErrorUnknown)
{
    ASSERT_NE(plugin_, nullptr);
    // no Init: pluginCodecPtr_ is null
    auto packet = MakeOpusPacket();
    auto input = CreateInputBuffer(packet, false);
    EXPECT_EQ(Status::ERROR_UNKNOWN, plugin_->QueueInputBuffer(input));
}

TEST_F(OpusDecoderPluginUnitTest, QueueInputBuffer_NoPrepare_DecodeFail)
{
    ASSERT_NE(plugin_, nullptr);
    // Init + SetParameter but no Prepare: pluginCodecPtr_ set, but dec_ is null, so
    // ProcessSendData returns -1 -> ERROR_INVALID_DATA (decode-fail branch).
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    auto packet = MakeOpusPacket();
    auto input = CreateInputBuffer(packet, false);
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(input));
}

TEST_F(OpusDecoderPluginUnitTest, QueueInputBuffer_NullMeta)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    auto packet = MakeOpusPacket();
    auto input = CreateInputBuffer(packet, false);
    ASSERT_NE(input, nullptr);
    input->meta_ = nullptr; // decode succeeds, then the meta_ null-check returns error
    EXPECT_EQ(Status::ERROR_INVALID_DATA, plugin_->QueueInputBuffer(input));
}

TEST_F(OpusDecoderPluginUnitTest, QueueInputBuffer_Success)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    auto packet = MakeOpusPacket();
    auto input = CreateInputBuffer(packet, false);
    inputCallbackCount_ = 0;
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(1, inputCallbackCount_);
}

TEST_F(OpusDecoderPluginUnitTest, QueueOutputBuffer_Null)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    shared_ptr<AVBuffer> nullBuf = nullptr;
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->QueueOutputBuffer(nullBuf));
}

TEST_F(OpusDecoderPluginUnitTest, QueueOutputBuffer_SampleRateNotSet)
{
    ASSERT_NE(plugin_, nullptr);
    // fresh plugin: sampleRate_ == 0
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->QueueOutputBuffer(output));
}

TEST_F(OpusDecoderPluginUnitTest, QueueOutputBuffer_NoBufferMeta)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    // no QueueInputBuffer yet -> bufferMeta_ is null
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, plugin_->QueueOutputBuffer(output));
}

TEST_F(OpusDecoderPluginUnitTest, QueueOutputBuffer_Success)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    auto packet = MakeOpusPacket();
    auto input = CreateInputBuffer(packet, false);
    ASSERT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    outputCallbackCount_ = 0;
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_EQ(1, outputCallbackCount_);
    EXPECT_GT(outputCallbackSize_, 0); // decoded PCM written
    EXPECT_GT(output->duration_, 0);  // sampleFormat_ == S16LE -> duration set
}

TEST_F(OpusDecoderPluginUnitTest, QueueOutputBuffer_Success_NoSampleFormat)
{
    ASSERT_NE(plugin_, nullptr);
    // AUDIO_SAMPLE_FORMAT not set -> sampleFormat_ == INVALID_WIDTH -> covers the
    // false branch of (sampleFormat_ == SAMPLE_S16LE); duration_ is not set.
    EXPECT_EQ(Status::OK, InitAndConfigure(false, false));
    auto packet = MakeOpusPacket();
    auto input = CreateInputBuffer(packet, false);
    ASSERT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    auto output = CreateOutputBuffer(OUTPUT_CAPACITY);
    outputCallbackCount_ = 0;
    Status ret = plugin_->QueueOutputBuffer(output);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_EQ(1, outputCallbackCount_);
}

TEST_F(OpusDecoderPluginUnitTest, Skip_SkipEntireFrame_NonEos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true)); // enable skip
    // skipSamples (960) >= decSamples (960): skip whole frame, non-EOS -> NOT_ENOUGH_DATA
    Status ret = RunSkipScenario(MakeSkipInfo(OPUS_FRAME_SAMPLES, 0), false);
    EXPECT_EQ(Status::ERROR_NOT_ENOUGH_DATA, ret);
    EXPECT_EQ(0, outputCallbackCount_); // no output delivered
}

TEST_F(OpusDecoderPluginUnitTest, Skip_SkipEntireFrame_Eos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    // skipSamples (960) >= decSamples (960): skip whole frame, EOS -> OK (callback fired)
    Status ret = RunSkipScenario(MakeSkipInfo(OPUS_FRAME_SAMPLES, 0), true);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_EQ(1, outputCallbackCount_);
}

TEST_F(OpusDecoderPluginUnitTest, Skip_PartialStart_MoreThanDiscardEnd)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    // skipSamples=100 < 960: outSamples=860 > discardEnd=200 -> write partial, OK
    Status ret = RunSkipScenario(MakeSkipInfo(100, 200), false);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_EQ(1, outputCallbackCount_);
}

TEST_F(OpusDecoderPluginUnitTest, Skip_PartialStart_LessThanDiscardEnd)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    // skipSamples=0: outSamples=960 < discardEnd=1000 -> write all, OK
    Status ret = RunSkipScenario(MakeSkipInfo(0, 1000), false);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_EQ(1, outputCallbackCount_);
}

TEST_F(OpusDecoderPluginUnitTest, Skip_EqualDiscardEnd_NonEos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    // outSamples (960) == discardEnd (960), non-EOS -> NOT_ENOUGH_DATA
    Status ret = RunSkipScenario(MakeSkipInfo(0, OPUS_FRAME_SAMPLES), false);
    EXPECT_EQ(Status::ERROR_NOT_ENOUGH_DATA, ret);
    EXPECT_EQ(0, outputCallbackCount_);
}

TEST_F(OpusDecoderPluginUnitTest, Skip_EqualDiscardEnd_Eos)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure(true));
    // outSamples (960) == discardEnd (960), EOS -> OK (callback fired)
    Status ret = RunSkipScenario(MakeSkipInfo(0, OPUS_FRAME_SAMPLES), true);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_EQ(1, outputCallbackCount_);
}

TEST_F(OpusDecoderPluginUnitTest, Flush_Succeeds)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    auto packet = MakeOpusPacket();
    auto input = CreateInputBuffer(packet, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Flush());
}

TEST_F(OpusDecoderPluginUnitTest, Reset_ResetsState)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    auto packet = MakeOpusPacket();
    auto input = CreateInputBuffer(packet, false);
    EXPECT_EQ(Status::OK, plugin_->QueueInputBuffer(input));
    EXPECT_EQ(Status::OK, plugin_->Reset());
}

TEST_F(OpusDecoderPluginUnitTest, Stop_Succeeds)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    EXPECT_EQ(Status::OK, plugin_->Stop());
}

TEST_F(OpusDecoderPluginUnitTest, GetInputBuffers_And_GetOutputBuffers)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, InitAndConfigure());
    vector<shared_ptr<AVBuffer>> inputBuffers;
    EXPECT_EQ(Status::OK, plugin_->GetInputBuffers(inputBuffers));
    vector<shared_ptr<AVBuffer>> outputBuffers;
    EXPECT_EQ(Status::OK, plugin_->GetOutputBuffers(outputBuffers));
}

TEST_F(OpusDecoderPluginUnitTest, Prepare_And_Release)
{
    ASSERT_NE(plugin_, nullptr);
    EXPECT_EQ(Status::OK, plugin_->Init());
    plugin_->SetDataCallback(this);
    auto meta = make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(OPUS_CHANNELS);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(OPUS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::OK, plugin_->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin_->Prepare());
    EXPECT_EQ(Status::OK, plugin_->Release());
}

}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS
