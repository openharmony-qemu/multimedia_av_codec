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

#include <cstdint>
#include <string>
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <iostream>
#include <fstream>
#include "plugin/plugin_manager_v2.h"

#include "meta/format.h"
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "meta/mime_type.h"
#include "plugin/codec_plugin.h"
#include "audio_decoder_mock_base.h"

using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins;

namespace {
constexpr int32_t VALID_SR = 44100;
constexpr int32_t INVALID_SR_0 = 0;
constexpr int32_t INVALID_SR_N = -48000;
constexpr std::string_view INPUT_FILE_PATH = "/data/test/media/wma_2c_44100hz_128k.dat";

constexpr Plugins::AudioSampleFormat S16 = Plugins::AudioSampleFormat::SAMPLE_S16LE;
constexpr Plugins::AudioSampleFormat F32 = Plugins::AudioSampleFormat::SAMPLE_F32LE;

std::shared_ptr<CodecPlugin> CreatePluginByName(const std::string &name)
{
    auto p = PluginManagerV2::Instance().CreatePluginByName(name);
    if (!p) {
        return nullptr;
    }
    return std::reinterpret_pointer_cast<CodecPlugin>(p);
}

static inline std::vector<uint8_t> MakeWmaExtraData()
{
    return {
        0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00
    };
}
constexpr int32_t WMA_BLOCK_ALIGN = 5462;
} // namespace

class AudioWMAPluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
protected:
    int32_t CreateWmaDecoder();
    int32_t FillInputData();
    std::unique_ptr<AudioDecoderMockBase> decoderMock_ = nullptr;
    std::unique_ptr<std::ifstream> inputFile_ = nullptr;
    std::vector<uint8_t> inData_;
    uint32_t inDataSize_ = 0;
    int64_t pts_ = 0;
};

int32_t AudioWMAPluginUnitTest::CreateWmaDecoder()
{
    // BLOCK_ALIGN 5945
    int64_t size = 0;
    inData_.resize(10240);  // 10240
    decoderMock_ = AudioDecoderMockBase::CreateDecoder();
    inputFile_ = std::make_unique<std::ifstream>(INPUT_FILE_PATH, std::ios::binary);
    inputFile_->read(reinterpret_cast<char *>(&size), sizeof(size));
    std::vector<uint8_t> codecConfig(size, 0);
    inputFile_->read(reinterpret_cast<char *>(codecConfig.data()), size);
    if (decoderMock_->CreateByName(AVCodecCodecName::AUDIO_DECODER_WMAPRO_NAME.data()) != 0) {
        return -1;
    }
    decoderMock_->SetBlockAlign(5945);  // 5945
    if (decoderMock_->Start(44100, 2, &codecConfig) != 0) {  // 44100 2
        return -2;  // -2
    }
    return 0;
}

int32_t AudioWMAPluginUnitTest::FillInputData()
{
    if (inputFile_ == nullptr) {
        return -1;
    }
    int64_t size = 0;
    int64_t pts = 0;
    inputFile_->read(reinterpret_cast<char *>(&size), sizeof(size));
    if (inputFile_->eof() || inputFile_->gcount() == 0 || size == 0) {
        // 文件读完了，调用stop
        return 1;
    }
    if (inputFile_->gcount() != sizeof(size) || size > 10240) {  // 10240
        std::cout << "FillInputData read size failed! size:" << size << std::endl;
        return -1;
    }
    inputFile_->read(reinterpret_cast<char *>(&pts), sizeof(pts));
    if (inputFile_->gcount() != sizeof(pts)) {
        std::cout << "FillInputData read pts failed!" << std::endl;
        return -1;
    }
    inputFile_->read(reinterpret_cast<char *>(inData_.data()), size);
    if (inputFile_->gcount() != size) {
        std::cout << "FillInputData read data failed!" << std::endl;
        return -1;
    }
    inDataSize_ = static_cast<uint32_t>(size);
    pts_ = pts;
    return 0;
}

/* =========================
 *  插件层：参数/状态机用例
 * ========================= */

// 1) WMAv1：缺失必填键（channel_count）
HWTEST_F(AudioWMAPluginUnitTest, WMAv1_SetParameter_MissingChannel, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV1_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    meta->Set<Tag::AUDIO_BLOCK_ALIGN>(WMA_BLOCK_ALIGN);
    meta->Set<Tag::MEDIA_CODEC_CONFIG>(MakeWmaExtraData());
    // 未设置 AUDIO_CHANNEL_COUNT
    EXPECT_NE(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Release());
}

// 2) WMAv2：非法采样率（<=0）
HWTEST_F(AudioWMAPluginUnitTest, WMAv2_SetParameter_InvalidSampleRate, TestSize.Level1)
{
    {
        auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME));
        ASSERT_NE(nullptr, plugin);
        ASSERT_EQ(Status::OK, plugin->Init());

        auto meta = std::make_shared<Meta>();
        meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
        meta->Set<Tag::AUDIO_SAMPLE_RATE>(INVALID_SR_0);
        meta->Set<Tag::AUDIO_BLOCK_ALIGN>(WMA_BLOCK_ALIGN);
        meta->Set<Tag::MEDIA_CODEC_CONFIG>(MakeWmaExtraData());
        EXPECT_NE(Status::OK, plugin->SetParameter(meta));

        EXPECT_EQ(Status::OK, plugin->Release());
    }

    {
        auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME));
        ASSERT_NE(nullptr, plugin);
        ASSERT_EQ(Status::OK, plugin->Init());

        auto meta = std::make_shared<Meta>();
        meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
        meta->Set<Tag::AUDIO_SAMPLE_RATE>(INVALID_SR_N);
        meta->Set<Tag::AUDIO_BLOCK_ALIGN>(WMA_BLOCK_ALIGN);
        meta->Set<Tag::MEDIA_CODEC_CONFIG>(MakeWmaExtraData());
        EXPECT_NE(Status::OK, plugin->SetParameter(meta));

        EXPECT_EQ(Status::OK, plugin->Release());
    }
}


// 3) WMAPro：正常参数（S16 / F32 都应可通过 base_->CheckSampleFormat）
HWTEST_F(AudioWMAPluginUnitTest, WMAPro_SetParameter_ValidParams, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAPRO_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(S16);
    meta->Set<Tag::AUDIO_BLOCK_ALIGN>(WMA_BLOCK_ALIGN);
    meta->Set<Tag::MEDIA_CODEC_CONFIG>(MakeWmaExtraData());
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Release());

    auto metaF32 = std::make_shared<Meta>();
    metaF32->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    metaF32->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    metaF32->Set<Tag::AUDIO_SAMPLE_FORMAT>(F32);
    metaF32->Set<Tag::AUDIO_BLOCK_ALIGN>(WMA_BLOCK_ALIGN);
    metaF32->Set<Tag::MEDIA_CODEC_CONFIG>(MakeWmaExtraData());
    
    auto plugin2 = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAPRO_NAME));
    ASSERT_NE(nullptr, plugin2);
    ASSERT_EQ(Status::OK, plugin2->Init());
    EXPECT_EQ(Status::OK, plugin2->SetParameter(metaF32));
    EXPECT_EQ(Status::OK, plugin2->Release());
}

// 4) 状态机：未配置直接 Prepare / Start
HWTEST_F(AudioWMAPluginUnitTest, WMAv2_StateMachine_Rough, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());
    EXPECT_EQ(Status::OK, plugin->Prepare());
    EXPECT_EQ(Status::OK, plugin->Start());
    EXPECT_EQ(Status::OK, plugin->Flush());
    EXPECT_EQ(Status::OK, plugin->Reset());
    EXPECT_EQ(Status::OK, plugin->Stop());
    EXPECT_EQ(Status::OK, plugin->Release());
}

// 5) SetParameter 后校验 GetParameter 返回 max buffer / mime
HWTEST_F(AudioWMAPluginUnitTest, WMAv1_GetParameter_CheckOutputFormat, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV1_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(S16);
    meta->Set<Tag::AUDIO_BLOCK_ALIGN>(WMA_BLOCK_ALIGN);
    meta->Set<Tag::MEDIA_CODEC_CONFIG>(MakeWmaExtraData());
    ASSERT_EQ(Status::OK, plugin->SetParameter(meta));

    std::shared_ptr<Meta> got;
    EXPECT_EQ(Status::OK, plugin->GetParameter(got));
    ASSERT_NE(nullptr, got);

    int32_t inMax = 0;
    int32_t outMax = 0;
    EXPECT_TRUE(got->Get<Tag::AUDIO_MAX_INPUT_SIZE>(inMax));
    EXPECT_TRUE(got->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(outMax));

    // 与插件实现保持一致：输入不超过 8192（默认兜底），输出固定为 4 * 2048 * 8
    EXPECT_GT(inMax, 0);
    EXPECT_GT(inMax, 8192);
    EXPECT_EQ(outMax, 4 * 2048 * 8);

    // 校验 MIME_TYPE：WMAv1
    std::string mime;
    EXPECT_TRUE(got->Get<Tag::MIME_TYPE>(mime));
    EXPECT_EQ(mime, MimeType::AUDIO_WMAV1);
    EXPECT_EQ(Status::OK, plugin->Release());
}

HWTEST_F(AudioWMAPluginUnitTest, WMAv2_GetParameter_CheckOutputFormat, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(S16);
    meta->Set<Tag::AUDIO_BLOCK_ALIGN>(WMA_BLOCK_ALIGN);
    meta->Set<Tag::MEDIA_CODEC_CONFIG>(MakeWmaExtraData());
    ASSERT_EQ(Status::OK, plugin->SetParameter(meta));

    std::shared_ptr<Meta> got;
    ASSERT_EQ(Status::OK, plugin->GetParameter(got));
    std::string mime;
    ASSERT_TRUE(got->Get<Tag::MIME_TYPE>(mime));
    EXPECT_EQ(mime, MimeType::AUDIO_WMAV2);
    EXPECT_EQ(Status::OK, plugin->Release());
}

HWTEST_F(AudioWMAPluginUnitTest, WMAPro_GetParameter_CheckOutputFormat, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAPRO_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(S16);
    meta->Set<Tag::AUDIO_BLOCK_ALIGN>(WMA_BLOCK_ALIGN);
    meta->Set<Tag::MEDIA_CODEC_CONFIG>(MakeWmaExtraData());
    ASSERT_EQ(Status::OK, plugin->SetParameter(meta));

    std::shared_ptr<Meta> got;
    ASSERT_EQ(Status::OK, plugin->GetParameter(got));
    std::string mime;
    ASSERT_TRUE(got->Get<Tag::MIME_TYPE>(mime));
    EXPECT_EQ(mime, MimeType::AUDIO_WMAPRO);
    EXPECT_EQ(Status::OK, plugin->Release());
}

HWTEST_F(AudioWMAPluginUnitTest, WMAPro_Write_Frame, TestSize.Level1)
{
    CreateWmaDecoder();
    int32_t outSize = 0;
    int32_t i = 0;
    while (FillInputData() == 0) {
        i++;
        decoderMock_->SetPts(pts_ + 1);
        decoderMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        int32_t j = 0;
        int64_t lastPts = 0;
        while (decoderMock_->DecodeOutput(nullptr, outSize) > 0) {
            j++;
            if (i == 1 && j == 1) {
                EXPECT_EQ(decoderMock_->GetOutputPts(), 1);
            } else if (i == 1) {
                EXPECT_GT(decoderMock_->GetOutputPts(), lastPts);
            }
            lastPts = decoderMock_->GetOutputPts();
        }
    }
    decoderMock_->Stop();
}
