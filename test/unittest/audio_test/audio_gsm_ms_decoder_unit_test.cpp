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

#include <cstdint>
#include <gtest/gtest.h>
#include <fstream>
#include <array>
#include "plugin/plugin_manager_v2.h"
#include "plugin/codec_plugin.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace {
const string CODEC_GSM_MS_DEC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_GSM_MS_NAME);
constexpr int32_t GSM_MS_SAMPLE_RATE = 8000;
constexpr int32_t GSM_MS_CHANNEL_COUNT = 1;
constexpr int64_t GSM_MS_BIT_RATE = 13000;
constexpr int32_t GSM_MS_MAX_INPUT_SIZE = 256;
constexpr size_t GSM_MS_BLOCK_SIZE = 65;
constexpr int32_t GSM_MS_DEFAULT_INPUT_SIZE = 4096;
constexpr uint32_t GSM_MS_BUFFER_SIZE = 1024;
constexpr uint32_t GSM_MS_AVBUFFER_SIZE = GSM_MS_MAX_INPUT_SIZE * 4;
constexpr uint32_t GSM_MS_PROBE_BUFFER_SIZE = 640;
constexpr uint32_t GSM_MS_TRIES_SIZE = 16;
constexpr uint32_t GSM_MS_INVALID_VAL = 0xAB;
constexpr int32_t GSM_MS_LOOP_COUNT = 2;
constexpr uint32_t GSM_MS_DEFAULT_FACTOR = 2;

static const uint8_t K_REAL_GSM_MS_BLOCK0[GSM_MS_BLOCK_SIZE] = {
    0x5a, 0x4e, 0x63, 0x29, 0x89, 0xed, 0x04, 0xca, 0x69, 0x2c, 0x29, 0xb1, 0xc9, 0x00, 0x38, 0x89,
    0x2d, 0x25, 0xb2, 0x48, 0xdd, 0xe3, 0x1e, 0xc5, 0x2e, 0x60, 0x59, 0xcd, 0x64, 0x3e, 0x61, 0x1b,
    0x20, 0xbc, 0x88, 0xdd, 0xe1, 0x1a, 0x85, 0x42, 0xef, 0x16, 0x40, 0x9b, 0xd2, 0x8a, 0x82, 0xe6,
    0x1c, 0xdb, 0x69, 0x24, 0x47, 0x05, 0x62, 0x43, 0xfa, 0x2f, 0x59, 0x8b, 0x47, 0x02, 0xf1, 0xbf,
    0x64
};
static const uint8_t K_REAL_GSM_MS_BLOCK1[GSM_MS_BLOCK_SIZE] = {
    0x2d, 0x85, 0xd8, 0x3d, 0xee, 0x51, 0x78, 0x08, 0xc4, 0x49, 0x6b, 0x27, 0xd1, 0xdb, 0x04, 0x86,
    0xd0, 0x9e, 0x42, 0x9b, 0xb6, 0x06, 0xa2, 0xba, 0xca, 0x6a, 0x4e, 0x48, 0x05, 0xc6, 0x72, 0x12,
    0x5b, 0x49, 0x88, 0xdd, 0xe1, 0x1a, 0x75, 0x13, 0x60, 0xe5, 0x22, 0x96, 0x94, 0x96, 0x14, 0xa0,
    0x9d, 0xb6, 0x76, 0x1b, 0xbb, 0x75, 0x60, 0x1a, 0xeb, 0x6d, 0x6c, 0xb7, 0x35, 0x60, 0x23, 0x4b,
    0x89
};
static const uint8_t K_REAL_GSM_MS_BLOCK2[GSM_MS_BLOCK_SIZE] = {
    0x2d, 0x85, 0xdc, 0x1d, 0xaa, 0x51, 0xee, 0x02, 0xc4, 0x76, 0x12, 0xeb, 0x49, 0xcb, 0x04, 0x5a,
    0x4e, 0x63, 0x29, 0x89, 0xed, 0x04, 0xca, 0x69, 0x2c, 0x29, 0xb1, 0xc9, 0x00, 0x38, 0x89, 0x2d,
    0x25, 0xb2, 0x48, 0xdd, 0xe3, 0x1e, 0xc5, 0x2e, 0x60, 0x59, 0xcd, 0x64, 0x3e, 0x61, 0x1b, 0x20,
    0xbc, 0x88, 0xdd, 0xe1, 0x1a, 0x85, 0x42, 0xef, 0x16, 0x40, 0x9b, 0xd2, 0x8a, 0x82, 0xe6, 0x1c,
    0x00
};

static const std::array<const uint8_t*, 3> K_REAL_BLOCKS = {
    K_REAL_GSM_MS_BLOCK0, K_REAL_GSM_MS_BLOCK1, K_REAL_GSM_MS_BLOCK2
};

shared_ptr<AVBuffer> MakeRealBlock(size_t idx)
{
    idx %= K_REAL_BLOCKS.size();
    auto buf = AVBuffer::CreateAVBuffer(
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE),
        GSM_MS_BLOCK_SIZE);
    buf->memory_->SetSize(GSM_MS_BLOCK_SIZE);
    int ret = memcpy_s(buf->memory_->GetAddr(), buf->memory_->GetCapacity(), K_REAL_BLOCKS[idx], GSM_MS_BLOCK_SIZE);
    if (ret != 0) {
        return nullptr;
    }
    return buf;
}

} // namespace

class GsmMsUnitTest : public testing::Test, public DataCallback {
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
    void QueueOutputBuffer(size_t& produced);
    shared_ptr<CodecPlugin> plugin_ = nullptr;
    shared_ptr<Media::Meta> meta_ = nullptr;
};

void GsmMsUnitTest::SetUpTestCase(void)
{
}

void GsmMsUnitTest::TearDownTestCase(void)
{
}

void GsmMsUnitTest::SetUp(void)
{
    plugin_ = CreatePlugin();
    ASSERT_NE(plugin_, nullptr);
    meta_ = CreateMeta();
    ASSERT_NE(meta_, nullptr);
}

void GsmMsUnitTest::TearDown(void)
{
    if (plugin_) {
        plugin_->Release();
    }
}

shared_ptr<CodecPlugin> GsmMsUnitTest::CreatePlugin()
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName(CODEC_GSM_MS_DEC_NAME);
    if (!plugin) {
        return nullptr;
    }
    return reinterpret_pointer_cast<CodecPlugin>(plugin);
}

shared_ptr<Media::Meta> GsmMsUnitTest::CreateMeta()
{
    auto meta = make_shared<Media::Meta>();
    if (meta == nullptr) {
        return nullptr;
    }
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(GSM_MS_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(GSM_MS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::MONO);
    meta->Set<Tag::MEDIA_BITRATE>(GSM_MS_BIT_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    return meta;
}

shared_ptr<AVBuffer> GsmMsUnitTest::CreateAVBuffer(size_t capacity, size_t size)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto avBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    if (avBuffer != nullptr && avBuffer->memory_ != nullptr) {
        avBuffer->memory_->SetSize(size);
    }
    return avBuffer;
}

void GsmMsUnitTest::QueueOutputBuffer(size_t& produced)
{
    ASSERT_NE(plugin_, nullptr);
    for (int probe = 0; probe < GSM_MS_LOOP_COUNT; ++probe) {
        auto out = CreateAVBuffer(GSM_MS_PROBE_BUFFER_SIZE, 0);
        ASSERT_NE(out, nullptr);
        auto ost = plugin_->QueueOutputBuffer(out);
        if ((ost == Status::OK || ost == Status::ERROR_NOT_ENOUGH_DATA || ost == Status::ERROR_AGAIN)) {
            if (out->memory_->GetSize() > 0) {
                produced++;
            }
        } else if (ost == Status::END_OF_STREAM) {
            break;
        }
    }
}

HWTEST_F(GsmMsUnitTest, SetParameter_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

HWTEST_F(GsmMsUnitTest, SetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(-1);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(GSM_MS_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(-1);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(GSM_MS_SAMPLE_RATE);
}

HWTEST_F(GsmMsUnitTest, SetParameter_003, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_CHANNEL_COUNT);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(GSM_MS_CHANNEL_COUNT);
    meta_->Remove(Tag::AUDIO_SAMPLE_RATE);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(GSM_MS_SAMPLE_RATE);
}

HWTEST_F(GsmMsUnitTest, Lifecycle_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    ASSERT_EQ(plugin_->Flush(), Status::OK);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    ASSERT_EQ(plugin_->Release(), Status::OK);
}

HWTEST_F(GsmMsUnitTest, Eos_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    auto inputBuffer = CreateAVBuffer(GSM_MS_BUFFER_SIZE, 0);
    ASSERT_NE(inputBuffer, nullptr);
    inputBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::OK);
    auto outputBuffer = CreateAVBuffer(GSM_MS_AVBUFFER_SIZE, 0);
    ASSERT_NE(outputBuffer, nullptr);
    ASSERT_EQ(plugin_->QueueOutputBuffer(outputBuffer), Status::END_OF_STREAM);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(GsmMsUnitTest, GetParameter_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(GSM_MS_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    int32_t channelCount = 0;
    int32_t sampleRate = 0;
    int32_t maxInputSize = 0;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount));
    ASSERT_EQ(channelCount, GSM_MS_CHANNEL_COUNT);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_EQ(sampleRate, GSM_MS_SAMPLE_RATE);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_GT(maxInputSize, 0);
}

HWTEST_F(GsmMsUnitTest, GetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    int32_t defaultInputSize = GSM_MS_DEFAULT_INPUT_SIZE;
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    ASSERT_NE(outMeta, nullptr);
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
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize / GSM_MS_DEFAULT_FACTOR);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
    usrInputSize = 0;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize * GSM_MS_DEFAULT_FACTOR);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
}

HWTEST_F(GsmMsUnitTest, Decode_With_Invalid_File_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(GSM_MS_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    const int invalidSize = 16;
    auto inputBuffer = CreateAVBuffer(invalidSize, invalidSize);
    ASSERT_NE(inputBuffer, nullptr);
    ASSERT_NE(inputBuffer->memory_, nullptr);
    std::fill_n(inputBuffer->memory_->GetAddr(), invalidSize, GSM_MS_INVALID_VAL);
    auto st = plugin_->QueueInputBuffer(inputBuffer);
    ASSERT_NE(st, Status::OK);
    auto outputBuffer = CreateAVBuffer(GSM_MS_MAX_INPUT_SIZE, 0);
    ASSERT_NE(outputBuffer, nullptr);
    Status status = plugin_->QueueOutputBuffer(outputBuffer);
    ASSERT_TRUE(status != Status::OK);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(GsmMsUnitTest, Decode_With_Embedded_RealBlocks_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(GSM_MS_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    size_t pushed = 0;
    size_t produced = 0;
    for (size_t i = 0; i < K_REAL_BLOCKS.size(); ++i) {
        auto in = MakeRealBlock(i);
        ASSERT_NE(in, nullptr);
        auto st = plugin_->QueueInputBuffer(in);
        ASSERT_EQ(st, Status::OK);
        pushed++;
        QueueOutputBuffer(produced);
    }
    auto eos = CreateAVBuffer(0, 0);
    ASSERT_NE(eos, nullptr);
    eos->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(eos), Status::OK);
    for (int tries = 0; tries < GSM_MS_TRIES_SIZE; ++tries) {
        auto out = CreateAVBuffer(GSM_MS_PROBE_BUFFER_SIZE, 0);
        ASSERT_NE(out, nullptr);
        ASSERT_NE(out->memory_, nullptr);
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
