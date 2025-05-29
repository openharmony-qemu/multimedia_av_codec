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
#include <gtest/gtest.h>
#include <fstream>
#include "plugin/plugin_manager_v2.h"
#include "plugin/codec_plugin.h"
#include "avcodec_codec_name.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
namespace Plugins {

const string CODEC_G711A_DEC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_G711A_NAME);
constexpr int32_t G711A_SAMPLE_RATE = 16000;
constexpr int64_t G711A_BIT_RATE = 6000;
constexpr int32_t G711A_SIZE = 640;  // 40ms
constexpr int32_t G711A_MAX_INPUT_SIZE = 8192;
static constexpr int32_t TEST_INPUT_SIZE = 8;
static const uint8_t TEST_INPUT_ARR[TEST_INPUT_SIZE] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
static const uint8_t TEST_OUTPUT_ARR[TEST_INPUT_SIZE * 2] = {0x80, 0xeb, 0x00, 0xa6, 0xf8, 0xfe, 0x60, 0xfb, 0x80, 0x1c,
                                          0x00, 0x7a, 0x88, 0x01, 0xa0, 0x06};

class LbvcUnitTest : public testing::Test, public DataCallback {
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
    // std::unique_ptr<std::ifstream> soFile_;
    shared_ptr<Media::Meta> meta_ = nullptr;
    shared_ptr<CodecPlugin> plugin_ = nullptr;
    shared_ptr<AVBuffer> avBuffer_ = nullptr;
};

void LbvcUnitTest::SetUpTestCase(void)
{
}

void LbvcUnitTest::TearDownTestCase(void)
{
}

void LbvcUnitTest::SetUp(void)
{
    auto tmp = PluginManagerV2::Instance().CreatePluginByName(CODEC_G711A_DEC_NAME);
    plugin_ = reinterpret_pointer_cast<CodecPlugin>(tmp);
    meta_ = make_shared<Media::Meta>();
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(G711A_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::MEDIA_BITRATE>(G711A_BIT_RATE);
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer_ = AVBuffer::CreateAVBuffer(avAllocator, (G711A_SIZE << 1));
    avBuffer_->memory_->SetSize(G711A_SIZE);
}

void LbvcUnitTest::TearDown(void)
{
    if (plugin_) {
        plugin_->Release();
        plugin_ = nullptr;
        meta_ = nullptr;
    }
}

HWTEST_F(LbvcUnitTest, SetParamter_001, TestSize.Level1)
{
    if (plugin_ == nullptr) {
        cout << "g711a plugin is nullptr!" << endl;
        return;
    }
    plugin_->Init();
    plugin_->SetDataCallback(this);
    EXPECT_EQ(plugin_->SetParameter(meta_), Status::OK);
    meta_->Remove(Tag::AUDIO_CHANNEL_COUNT);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    meta_->Remove(Tag::AUDIO_SAMPLE_RATE);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(G711A_SAMPLE_RATE);
    meta_->Remove(Tag::AUDIO_SAMPLE_FORMAT);
    EXPECT_EQ(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(G711A_MAX_INPUT_SIZE);
    EXPECT_EQ(plugin_->SetParameter(meta_), Status::OK);
    plugin_->Start();
    EXPECT_EQ(plugin_->QueueInputBuffer(avBuffer_), Status::OK);
    EXPECT_EQ(plugin_->QueueOutputBuffer(avBuffer_), Status::OK);
    plugin_->Flush();
    plugin_->Stop();
    EXPECT_EQ(plugin_->Reset(), Status::OK);
}

HWTEST_F(LbvcUnitTest, GetParamter_001, TestSize.Level1)
{
    if (plugin_ == nullptr) {
        cout << "g711a plugin is nullptr!" << endl;
        return;
    }
    shared_ptr<Media::Meta> tmpMeta = make_shared<Media::Meta>();
    int32_t maxInputSize = 0;
    int32_t maxOutputSize = 0;
    plugin_->Init();
    plugin_->Reset();
    plugin_->SetDataCallback(this);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(G711A_MAX_INPUT_SIZE + 1);
    EXPECT_EQ(plugin_->SetParameter(meta_), Status::OK);
    EXPECT_EQ(plugin_->GetParameter(tmpMeta), Status::OK);
    tmpMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize);
    EXPECT_EQ(maxInputSize, G711A_MAX_INPUT_SIZE);
    tmpMeta->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize);
    EXPECT_EQ(maxOutputSize, G711A_MAX_INPUT_SIZE * 2);
    plugin_->Flush();
    plugin_->Stop();
    EXPECT_EQ(plugin_->Reset(), Status::OK);
}

HWTEST_F(LbvcUnitTest, Decode_001, TestSize.Level1)
{
    if (plugin_ == nullptr) {
        cout << "g711a plugin is nullptr!" << endl;
        return;
    }
    plugin_->Init();
    plugin_->SetDataCallback(this);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(G711A_MAX_INPUT_SIZE + 1);
    EXPECT_EQ(plugin_->SetParameter(meta_), Status::OK);
    plugin_->Start();
    uint8_t *buffer = avBuffer_->memory_->GetAddr();
    for (int i = 0; i < TEST_INPUT_SIZE; i++) {
        buffer[i] = TEST_INPUT_ARR[i];
    }
    EXPECT_EQ(plugin_->QueueInputBuffer(avBuffer_), Status::OK);
    EXPECT_EQ(plugin_->QueueOutputBuffer(avBuffer_), Status::OK);
    buffer = avBuffer_->memory_->GetAddr();
    EXPECT_EQ(avBuffer_->memory_->GetSize(), G711A_SIZE * 2);
    EXPECT_EQ(avBuffer_->duration_, 40000);  // 40000us -> 40ms
    for (int i = 0; i < TEST_INPUT_SIZE * 2; i++) {
        EXPECT_EQ(buffer[i], TEST_OUTPUT_ARR[i]);
    }
    plugin_->Flush();
    plugin_->Stop();
    EXPECT_EQ(plugin_->Reset(), Status::OK);
}

HWTEST_F(LbvcUnitTest, Decode_002, TestSize.Level1)
{
    if (plugin_ == nullptr) {
        cout << "g711a plugin is nullptr!" << endl;
        return;
    }
    plugin_->Init();
    plugin_->SetDataCallback(this);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S32LE);
    EXPECT_EQ(plugin_->SetParameter(meta_), Status::OK);
    plugin_->Start();
    EXPECT_EQ(plugin_->QueueInputBuffer(avBuffer_), Status::OK);
    EXPECT_EQ(plugin_->QueueOutputBuffer(avBuffer_), Status::OK);
    EXPECT_NE(avBuffer_->duration_, 40000);  // 40000us -> 40ms
    plugin_->Flush();
    plugin_->Stop();
    EXPECT_EQ(plugin_->Reset(), Status::OK);
}

} // namespace Plugins
} // namespace Media
} // namespace OHOS