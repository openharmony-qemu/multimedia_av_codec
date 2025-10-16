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

#include <gtest/gtest.h>
#include <memory>

#include "plugin/plugin_manager_v2.h"
#include "meta/format.h"
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "meta/mime_type.h"
#include "plugin/codec_plugin.h"

using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins;

namespace {
constexpr int32_t VALID_SR = 44100;
constexpr int32_t INVALID_SR_0 = 0;
constexpr int32_t INVALID_SR_N = -48000;

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
} // namespace

class AudioWMAPluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

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
    meta->Set<Tag::AUDIO_BLOCK_ALIGN>(1);
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
        meta->Set<Tag::AUDIO_BLOCK_ALIGN>(1);
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
        meta->Set<Tag::AUDIO_BLOCK_ALIGN>(1);
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
    meta->Set<Tag::AUDIO_BLOCK_ALIGN>(1);
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Release());

    auto metaF32 = std::make_shared<Meta>();
    metaF32->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    metaF32->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    metaF32->Set<Tag::AUDIO_SAMPLE_FORMAT>(F32);
    metaF32->Set<Tag::AUDIO_BLOCK_ALIGN>(1);
    
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
    meta->Set<Tag::AUDIO_BLOCK_ALIGN>(1);
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
    EXPECT_LE(inMax, 8192);
    EXPECT_EQ(outMax, 4 * 2048 * 8);

    // 校验 MIME_TYPE：WMAv1
    std::string mime;
    EXPECT_TRUE(got->Get<Tag::MIME_TYPE>(mime));
    EXPECT_EQ(mime, MimeType::AUDIO_WMAV1);
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
    meta->Set<Tag::AUDIO_BLOCK_ALIGN>(1);
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
    meta->Set<Tag::AUDIO_BLOCK_ALIGN>(1);
    ASSERT_EQ(Status::OK, plugin->SetParameter(meta));

    std::shared_ptr<Meta> got;
    ASSERT_EQ(Status::OK, plugin->GetParameter(got));
    std::string mime;
    ASSERT_TRUE(got->Get<Tag::MIME_TYPE>(mime));
    EXPECT_EQ(mime, MimeType::AUDIO_WMAPRO);
    EXPECT_EQ(Status::OK, plugin->Release());
}
