/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
const string CODEC_ILBC_DEC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_ILBC_NAME);
constexpr int32_t ILBC_SAMPLE_RATE = 8000;
constexpr int32_t ILBC_CHANNEL_COUNT = 1;
constexpr int64_t ILBC_BIT_RATE = 13300;
constexpr int32_t ILBC_MAX_INPUT_SIZE = 256;
constexpr size_t ILBC_BLOCK_SIZE_20MS = 38;
constexpr size_t ILBC_BLOCK_SIZE_30MS = 50;
constexpr size_t ILBC_AVBUFFER_SIZE = 480; // 240 samples * 2 bytes (30ms)
constexpr int PROBE = 2;

static const uint8_t K_ILBC_BLOCK_20MS[ILBC_BLOCK_SIZE_20MS] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
    0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25};
static const uint8_t K_ILBC_BLOCK_30MS[ILBC_BLOCK_SIZE_30MS] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B,
    0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E,
    0x5F, 0x60, 0x61};

static const std::array<const uint8_t *, 2> K_ILBC_BLOCKS = {K_ILBC_BLOCK_20MS, K_ILBC_BLOCK_30MS};
static const std::array<size_t, 2> K_ILBC_BLOCK_SIZES = {ILBC_BLOCK_SIZE_20MS, ILBC_BLOCK_SIZE_30MS};

shared_ptr<AVBuffer> MakeILBCBlock(size_t idx)
{
    idx %= K_ILBC_BLOCKS.size();
    auto buf = AVBuffer::CreateAVBuffer(
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE), K_ILBC_BLOCK_SIZES[idx]);
    buf->memory_->SetSize(K_ILBC_BLOCK_SIZES[idx]);
    if (memcpy_s(buf->memory_->GetAddr(), buf->memory_->GetCapacity(), K_ILBC_BLOCKS[idx], K_ILBC_BLOCK_SIZES[idx]) !=
        0) {
        return nullptr;
    }
    return buf;
}

} // namespace

class ILBCUnitTest : public testing::Test, public DataCallback {
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

void ILBCUnitTest::SetUpTestCase(void) {}
void ILBCUnitTest::TearDownTestCase(void) {}

void ILBCUnitTest::SetUp(void)
{
    plugin_ = CreatePlugin();
    ASSERT_NE(plugin_, nullptr);
    meta_ = CreateMeta();
    ASSERT_NE(meta_, nullptr);
}

void ILBCUnitTest::TearDown(void)
{
    if (plugin_) {
        plugin_->Release();
    }
}

shared_ptr<CodecPlugin> ILBCUnitTest::CreatePlugin()
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName(CODEC_ILBC_DEC_NAME);
    if (!plugin) {
        return nullptr;
    }
    return reinterpret_pointer_cast<CodecPlugin>(plugin);
}

shared_ptr<Media::Meta> ILBCUnitTest::CreateMeta()
{
    auto meta = make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(ILBC_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(ILBC_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::MONO);
    meta->Set<Tag::MEDIA_BITRATE>(ILBC_BIT_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    return meta;
}

shared_ptr<AVBuffer> ILBCUnitTest::CreateAVBuffer(size_t capacity, size_t size)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto avBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    avBuffer->memory_->SetSize(size);
    return avBuffer;
}

void ILBCUnitTest::OutHelper(size_t &produced)
{
    for (int probe = 0; probe < PROBE; ++probe) {
        auto out = CreateAVBuffer(ILBC_AVBUFFER_SIZE, 0);
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

HWTEST_F(ILBCUnitTest, SetParameter_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

HWTEST_F(ILBCUnitTest, SetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(-1);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ILBC_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(-1);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ILBC_SAMPLE_RATE);
}

HWTEST_F(ILBCUnitTest, SetParameter_003, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_CHANNEL_COUNT);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ILBC_CHANNEL_COUNT);
    meta_->Remove(Tag::AUDIO_SAMPLE_RATE);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ILBC_SAMPLE_RATE);
}

HWTEST_F(ILBCUnitTest, Lifecycle_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    ASSERT_EQ(plugin_->Flush(), Status::OK);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    ASSERT_EQ(plugin_->Release(), Status::OK);
}

HWTEST_F(ILBCUnitTest, Eos_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    auto inputBuffer = CreateAVBuffer(1024, 0);
    inputBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::OK);
    auto outputBuffer = CreateAVBuffer(ILBC_MAX_INPUT_SIZE * 4, 0);
    ASSERT_EQ(plugin_->QueueOutputBuffer(outputBuffer), Status::END_OF_STREAM);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(ILBCUnitTest, GetParameter_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(ILBC_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    int32_t channelCount = 0;
    int32_t sampleRate = 0;
    int32_t maxInputSize = 0;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount));
    ASSERT_EQ(channelCount, ILBC_CHANNEL_COUNT);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_EQ(sampleRate, ILBC_SAMPLE_RATE);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_GT(maxInputSize, 0);
}

HWTEST_F(ILBCUnitTest, GetParameter_002, TestSize.Level1)
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

HWTEST_F(ILBCUnitTest, Decode_With_Embedded_RealBlocks_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(ILBC_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    size_t pushed = 0;
    size_t produced = 0;
    for (size_t i = 0; i < K_ILBC_BLOCKS.size(); ++i) {
        auto in = MakeILBCBlock(i);
        auto st = plugin_->QueueInputBuffer(in);
        ASSERT_EQ(st, Status::OK) << "iLBC block rejected at index=" << i << " with status " << static_cast<int>(st);
        pushed++;
        OutHelper(produced);
    }
    auto eos = CreateAVBuffer(0, 0);
    eos->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(eos), Status::OK);
    for (int tries = 0; tries < 16; ++tries) {
        auto out = CreateAVBuffer(ILBC_AVBUFFER_SIZE, 0);
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
