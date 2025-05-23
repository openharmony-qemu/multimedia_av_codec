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

const string CODEC_LBVC_DEC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_LBVC_NAME);
const string CODEC_LBVC_ENC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_ENCODER_LBVC_NAME);
constexpr int32_t LBVC_SAMPLE_RATE = 16000;
constexpr int64_t LBVC_BIT_RATE = 6000;
constexpr int32_t LBVC_SIZE = 640;

class LbvcUnitTest : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
    int32_t CheckFunc(bool isEncoder);
    int32_t ProceFunc(bool isEncoder);
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
    std::unique_ptr<std::ifstream> soFile_;
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
    meta_ = make_shared<Media::Meta>();
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(LBVC_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::MEDIA_BITRATE>(LBVC_BIT_RATE);
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer_ = AVBuffer::CreateAVBuffer(avAllocator, (LBVC_SIZE << 1));
    avBuffer_->memory_->SetSize(LBVC_SIZE);
}

void LbvcUnitTest::TearDown(void)
{
    if (plugin_) {
        plugin_->Release();
        plugin_ = nullptr;
        meta_ = nullptr;
    }
}

int32_t LbvcUnitTest::CheckFunc(bool isEncoder)
{
    shared_ptr<CodecPlugin> tmpPlugin = nullptr;
    if (isEncoder) {
        auto tmp = PluginManagerV2::Instance().CreatePluginByName(CODEC_LBVC_ENC_NAME);
        tmpPlugin = reinterpret_pointer_cast<CodecPlugin>(tmp);
    } else {
        auto tmp = PluginManagerV2::Instance().CreatePluginByName(CODEC_LBVC_DEC_NAME);
        tmpPlugin = reinterpret_pointer_cast<CodecPlugin>(tmp);
    }
    bool ret = (tmpPlugin->Init() == Status::OK);
    tmpPlugin->Release();
    tmpPlugin = nullptr;
    return ret;
}

int32_t LbvcUnitTest::ProceFunc(bool isEncoder)
{
    if (isEncoder) {
        auto tmp = PluginManagerV2::Instance().CreatePluginByName(CODEC_LBVC_ENC_NAME);
        plugin_ = reinterpret_pointer_cast<CodecPlugin>(tmp);
    } else {
        auto tmp = PluginManagerV2::Instance().CreatePluginByName(CODEC_LBVC_DEC_NAME);
        plugin_ = reinterpret_pointer_cast<CodecPlugin>(tmp);
    }
    EXPECT_EQ(true, plugin_ != nullptr);
    return 0;
}

HWTEST_F(LbvcUnitTest, decode_invalid, TestSize.Level1)
{
    if (!CheckFunc(false)) {
        return;
    }
    ProceFunc(false);
    plugin_->Init();
    plugin_->SetDataCallback(this);
    meta_->Remove(Tag::AUDIO_CHANNEL_COUNT);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    meta_->Remove(Tag::AUDIO_SAMPLE_RATE);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(LBVC_SAMPLE_RATE);
    meta_->Remove(Tag::AUDIO_SAMPLE_FORMAT);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S32LE);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(1);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(LBVC_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    EXPECT_EQ(plugin_->SetParameter(meta_), Status::OK);
    plugin_->Start();
    EXPECT_NE(plugin_->QueueInputBuffer(avBuffer_), Status::OK);
    EXPECT_NE(plugin_->QueueOutputBuffer(avBuffer_), Status::OK);
    plugin_->Flush();
    plugin_->Stop();
    EXPECT_EQ(plugin_->Reset(), Status::OK);
}

HWTEST_F(LbvcUnitTest, encode_invalid, TestSize.Level1)
{
    if (!CheckFunc(true)) {
        return;
    }
    ProceFunc(true);
    plugin_->Init();
    plugin_->SetDataCallback(this);
    meta_->Remove(Tag::AUDIO_CHANNEL_COUNT);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    meta_->Remove(Tag::AUDIO_SAMPLE_RATE);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(LBVC_SAMPLE_RATE);
    meta_->Remove(Tag::AUDIO_SAMPLE_FORMAT);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Remove(Tag::MEDIA_BITRATE);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::MEDIA_BITRATE>(LBVC_BIT_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S32LE);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::MEDIA_BITRATE>(1);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::MEDIA_BITRATE>(LBVC_BIT_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(1);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(LBVC_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    EXPECT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    EXPECT_EQ(plugin_->SetParameter(meta_), Status::OK);
    plugin_->Start();
    avBuffer_->memory_->SetSize(1);
    EXPECT_NE(plugin_->QueueInputBuffer(avBuffer_), Status::OK);
    EXPECT_NE(plugin_->QueueOutputBuffer(avBuffer_), Status::OK);
    plugin_->Flush();
    plugin_->Stop();
    EXPECT_EQ(plugin_->Reset(), Status::OK);
}

} // namespace Plugins
} // namespace Media
} // namespace OHOS