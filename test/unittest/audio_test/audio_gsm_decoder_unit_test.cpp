/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_manager_v2.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <gtest/gtest.h>

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace {
const string CODEC_GSM_DEC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_GSM_NAME);
constexpr int32_t GSM_SAMPLE_RATE = 8000;
constexpr int32_t GSM_CHANNEL_COUNT = 1;
constexpr int64_t GSM_BIT_RATE = 13000;
constexpr int32_t GSM_MAX_INPUT_SIZE = 256;
constexpr size_t GSM_BLOCK_SIZE = 33;
constexpr size_t GSM_AVBUFFER_SIZE = 640; // 320 samples * 2 bytes
constexpr int PROBE = 2;

static const uint8_t K_REAL_GSM_BLOCK_0[GSM_BLOCK_SIZE] = {0xD4, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
static const uint8_t K_REAL_GSM_BLOCK_1[GSM_BLOCK_SIZE] = {0xD4, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
static const uint8_t K_REAL_GSM_BLOCK_2[GSM_BLOCK_SIZE] = {0xD4, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

static const std::array<const uint8_t *, 3> K_REAL_BLOCKS = {
    K_REAL_GSM_BLOCK_0, K_REAL_GSM_BLOCK_1, K_REAL_GSM_BLOCK_2};

shared_ptr<AVBuffer> MakeRealBlock(size_t idx)
{
    idx %= K_REAL_BLOCKS.size();
    auto buf = AVBuffer::CreateAVBuffer(
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE), GSM_BLOCK_SIZE);
    buf->memory_->SetSize(GSM_BLOCK_SIZE);
    if (memcpy_s(buf->memory_->GetAddr(), buf->memory_->GetCapacity(), K_REAL_BLOCKS[idx], GSM_BLOCK_SIZE) != 0) {
        return nullptr;
    }
    return buf;
}

} // namespace

class GSMUnitTest : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

    void OnInputBufferDone(const shared_ptr<Media::AVBuffer> &inputBuffer) override
    {
        (void)inputBuffer;
    }
    void OnOutputBufferDone(const shared_ptr<Media::AVBuffer> &outputBuffer) override
    {
        (void)outputBuffer;
    }
    void OnEvent(const shared_ptr<Plugins::PluginEvent> event) override
    {
        (void)event;
    }

protected:
    shared_ptr<CodecPlugin> CreatePlugin();
    void DestroyPlugin();
    shared_ptr<Media::Meta> CreateMeta();
    shared_ptr<AVBuffer> CreateAVBuffer(size_t capacity, size_t size);
    void OutHelper(size_t &produced);
    shared_ptr<CodecPlugin> plugin_ = nullptr;
    shared_ptr<Media::Meta> meta_ = nullptr;
};

void GSMUnitTest::SetUpTestCase(void) {}

void GSMUnitTest::TearDownTestCase(void) {}

void GSMUnitTest::SetUp(void)
{
    plugin_ = CreatePlugin();
    ASSERT_NE(plugin_, nullptr);
    meta_ = CreateMeta();
    ASSERT_NE(meta_, nullptr);
}

void GSMUnitTest::TearDown(void)
{
    if (plugin_) {
        plugin_->Release();
    }
}

shared_ptr<CodecPlugin> GSMUnitTest::CreatePlugin()
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName(CODEC_GSM_DEC_NAME);
    if (!plugin) {
        return nullptr;
    }
    return reinterpret_pointer_cast<CodecPlugin>(plugin);
}

shared_ptr<Media::Meta> GSMUnitTest::CreateMeta()
{
    auto meta = make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(GSM_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(GSM_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::MONO);
    meta->Set<Tag::MEDIA_BITRATE>(GSM_BIT_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    return meta;
}

shared_ptr<AVBuffer> GSMUnitTest::CreateAVBuffer(size_t capacity, size_t size)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto avBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    avBuffer->memory_->SetSize(size);
    return avBuffer;
}
void GSMUnitTest::OutHelper(size_t &produced)
{
    for (int probe = 0; probe < PROBE; ++probe) {
        auto out = CreateAVBuffer(GSM_AVBUFFER_SIZE, 0);
        auto ost = plugin_->QueueOutputBuffer(out);
        if ((ost == Status::OK || ost == Status::ERROR_NOT_ENOUGH_DATA || ost == Status::ERROR_AGAIN)) {
            if (out->memory_->GetSize() > 0) {
                produced++;
            }
        } else if (ost == Status::END_OF_STREAM) {
            break;
        } else {
            std::cout << "QueueOutputBuffer returned status " << static_cast<int>(ost) << std::endl;
        }
    }
}
HWTEST_F(GSMUnitTest, SetParameter_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

HWTEST_F(GSMUnitTest, SetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(-1);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(GSM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(-1);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(GSM_SAMPLE_RATE);
}

HWTEST_F(GSMUnitTest, SetParameter_003, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_CHANNEL_COUNT);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(GSM_CHANNEL_COUNT);
    meta_->Remove(Tag::AUDIO_SAMPLE_RATE);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(GSM_SAMPLE_RATE);
}

HWTEST_F(GSMUnitTest, Lifecycle_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    ASSERT_EQ(plugin_->Flush(), Status::OK);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    ASSERT_EQ(plugin_->Release(), Status::OK);
}

HWTEST_F(GSMUnitTest, Eos_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    auto inputBuffer = CreateAVBuffer(1024, 0);
    inputBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::OK);
    auto outputBuffer = CreateAVBuffer(GSM_MAX_INPUT_SIZE * 4, 0);
    ASSERT_EQ(plugin_->QueueOutputBuffer(outputBuffer), Status::END_OF_STREAM);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(GSMUnitTest, GetParameter_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(GSM_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    int32_t channelCount = 0;
    int32_t sampleRate = 0;
    int32_t maxInputSize = 0;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount));
    ASSERT_EQ(channelCount, GSM_CHANNEL_COUNT);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_EQ(sampleRate, GSM_SAMPLE_RATE);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_GT(maxInputSize, 0);
}

HWTEST_F(GSMUnitTest, GetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    int32_t defaultInputSize = 4096;
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    int32_t usrInputSize = 0;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
    usrInputSize = 0;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(-1);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
    usrInputSize = 0;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize / 2);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
    usrInputSize = 0;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize * 2);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
}

HWTEST_F(GSMUnitTest, Decode_With_Invalid_File_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(GSM_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    // This causes ffmpeg complaining 'Packet is too small'
    const int invalidSize = 16;
    auto inputBuffer = CreateAVBuffer(invalidSize, invalidSize);
    std::fill_n(inputBuffer->memory_->GetAddr(), 0xAB, invalidSize);
    auto st = plugin_->QueueInputBuffer(inputBuffer);
    ASSERT_NE(st, Status::OK);
    auto outputBuffer = CreateAVBuffer(GSM_MAX_INPUT_SIZE, 0);
    Status status = plugin_->QueueOutputBuffer(outputBuffer);
    ASSERT_TRUE(status != Status::OK) << "Unexpected status=" << static_cast<int>(status);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(GSMUnitTest, Decode_With_Embedded_RealBlocks_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(GSM_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    size_t pushed = 0;
    size_t produced = 0;
    for (size_t i = 0; i < K_REAL_BLOCKS.size(); ++i) {
        auto in = MakeRealBlock(i);
        auto st = plugin_->QueueInputBuffer(in);
        ASSERT_EQ(st, Status::OK) << "Real block rejected at index=" << i << " with status " << static_cast<int>(st);
        pushed++;
        OutHelper(produced);
    }
    auto eos = CreateAVBuffer(0, 0);
    eos->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(eos), Status::OK);
    for (int tries = 0; tries < 16; ++tries) {
        auto out = CreateAVBuffer(GSM_AVBUFFER_SIZE, 0);
        auto s = plugin_->QueueOutputBuffer(out);
        if (s == Status::END_OF_STREAM) {
            break;
        }
        if ((s == Status::OK || s == Status::ERROR_NOT_ENOUGH_DATA || s == Status::ERROR_AGAIN) &&
            out->memory_->GetSize() > 0) {
            produced++;
        }
    }
    ASSERT_GT(pushed, 0u);
    ASSERT_GT(produced, 0u);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

} // namespace Plugins
} // namespace Media
} // namespace OHOS
