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

#include "gtest/gtest.h"
#include "plugin/plugin_manager_v2.h"
#include "media_codec.h"
#include "meta/audio_types.h"
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include <vector>
#include <cmath>
#include <iostream>

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;

namespace {
constexpr uint32_t CHANNEL_COUNT_MONO = 1;
constexpr uint32_t CHANNEL_COUNT_STEREO = 2;
constexpr uint32_t AAC_SAMPLE_RATE = 44100;
constexpr uint32_t FLAC_SAMPLE_RATE = 32000;
constexpr uint32_t MP3_SAMPLE_RATE = 44100;
constexpr uint32_t G711MU_SAMPLE_RATE = 8000;
constexpr uint32_t AAC_BITRATE = 128000;
constexpr uint32_t FLAC_BITRATE = 32000;
constexpr uint32_t MP3_BITRATE = 128000;
constexpr uint32_t AAC_IS_ADTS = 1;
}

namespace OHOS {
namespace MediaAVCodec {

class AudioEncoderFormatConvertUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<Plugins::CodecPlugin> CreatePlugin(const std::string &codecName);
    std::shared_ptr<Meta> BuildAacMeta(Plugins::AudioSampleFormat sampleFormat, bool enableConvert);
    std::shared_ptr<Meta> BuildFlacMeta(Plugins::AudioSampleFormat sampleFormat, bool enableConvert);
    std::shared_ptr<Meta> BuildG711muMeta(Plugins::AudioSampleFormat sampleFormat, bool enableConvert);
    std::shared_ptr<Meta> BuildMp3Meta(Plugins::AudioSampleFormat sampleFormat, bool enableConvert);
};

void AudioEncoderFormatConvertUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioEncoderFormatConvertUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioEncoderFormatConvertUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioEncoderFormatConvertUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
}

std::shared_ptr<Plugins::CodecPlugin> AudioEncoderFormatConvertUnitTest::CreatePlugin(const std::string &codecName)
{
    auto plugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(codecName);
    if (plugin == nullptr) {
        return nullptr;
    }
    auto codecPlugin = std::reinterpret_pointer_cast<Plugins::CodecPlugin>(plugin);
    if (codecPlugin->Init() != Status::OK) {
        return nullptr;
    }
    return codecPlugin;
}

std::shared_ptr<Meta> AudioEncoderFormatConvertUnitTest::BuildAacMeta(
    Plugins::AudioSampleFormat sampleFormat, bool enableConvert)
{
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_AAC_IS_ADTS>(static_cast<int32_t>(AAC_IS_ADTS));
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<int32_t>(CHANNEL_COUNT_STEREO));
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<int32_t>(AAC_SAMPLE_RATE));
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(AAC_BITRATE));
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(Plugins::AudioChannelLayout::STEREO);
    if (enableConvert) {
        meta->Set<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(true);
    }
    return meta;
}

std::shared_ptr<Meta> AudioEncoderFormatConvertUnitTest::BuildFlacMeta(
    Plugins::AudioSampleFormat sampleFormat, bool enableConvert)
{
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<int32_t>(CHANNEL_COUNT_STEREO));
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<int32_t>(FLAC_SAMPLE_RATE));
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(FLAC_BITRATE));
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
    meta->Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(static_cast<int32_t>(sampleFormat));
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(Plugins::AudioChannelLayout::STEREO);
    if (enableConvert) {
        meta->Set<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(true);
    }
    return meta;
}

std::shared_ptr<Meta> AudioEncoderFormatConvertUnitTest::BuildG711muMeta(
    Plugins::AudioSampleFormat sampleFormat, bool enableConvert)
{
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<int32_t>(CHANNEL_COUNT_MONO));
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<int32_t>(G711MU_SAMPLE_RATE));
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
    if (enableConvert) {
        meta->Set<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(true);
    }
    return meta;
}

std::shared_ptr<Meta> AudioEncoderFormatConvertUnitTest::BuildMp3Meta(
    Plugins::AudioSampleFormat sampleFormat, bool enableConvert)
{
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<int32_t>(CHANNEL_COUNT_STEREO));
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<int32_t>(MP3_SAMPLE_RATE));
    meta->Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(MP3_BITRATE));
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
    if (enableConvert) {
        meta->Set<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(true);
    }
    return meta;
}

// ======================== AAC Encoder Tests ========================

/**
 * @tc.name: AacFormatConvert_001
 * @tc.desc: AAC encoder with U8 format, format convert disabled, should fail
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, AacFormatConvert_001, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_U8, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: AacFormatConvert_002
 * @tc.desc: AAC encoder with U8 format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, AacFormatConvert_002, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_U8, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: AacFormatConvert_003
 * @tc.desc: AAC encoder with S24LE format, format convert disabled, should fail
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, AacFormatConvert_003, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_S24LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: AacFormatConvert_004
 * @tc.desc: AAC encoder with S24LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, AacFormatConvert_004, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_S24LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: AacFormatConvert_005
 * @tc.desc: AAC encoder with S16LE format, format convert disabled, should succeed (native support)
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, AacFormatConvert_005, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_S16LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: AacFormatConvert_006
 * @tc.desc: AAC encoder with S32LE format, format convert disabled, should succeed (native support)
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, AacFormatConvert_006, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_S32LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: AacFormatConvert_007
 * @tc.desc: AAC encoder with F32LE format, format convert disabled, should succeed (native support)
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, AacFormatConvert_007, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_F32LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: AacFormatConvert_008
 * @tc.desc: AAC encoder with S16LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, AacFormatConvert_008, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_S16LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: AacFormatConvert_009
 * @tc.desc: AAC encoder with F32LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, AacFormatConvert_009, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_F32LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: AacFormatConvert_010
 * @tc.desc: AAC encoder reset clears format convert state
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, AacFormatConvert_010, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_U8, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Reset();
    // After reset, U8 without convert should fail
    auto metaNoConvert = BuildAacMeta(Plugins::AudioSampleFormat::SAMPLE_U8, false);
    ret = plugin->SetParameter(metaNoConvert);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

// ======================== FLAC Encoder Tests ========================

/**
 * @tc.name: FlacFormatConvert_001
 * @tc.desc: FLAC encoder with U8 format, format convert disabled, should fail
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, FlacFormatConvert_001, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildFlacMeta(Plugins::AudioSampleFormat::SAMPLE_U8, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: FlacFormatConvert_002
 * @tc.desc: FLAC encoder with U8 format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, FlacFormatConvert_002, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildFlacMeta(Plugins::AudioSampleFormat::SAMPLE_U8, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: FlacFormatConvert_003
 * @tc.desc: FLAC encoder with S24LE format, format convert disabled, should fail
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, FlacFormatConvert_003, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildFlacMeta(Plugins::AudioSampleFormat::SAMPLE_S24LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: FlacFormatConvert_004
 * @tc.desc: FLAC encoder with S24LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, FlacFormatConvert_004, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildFlacMeta(Plugins::AudioSampleFormat::SAMPLE_S24LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: FlacFormatConvert_005
 * @tc.desc: FLAC encoder with F32LE format, format convert disabled, should fail
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, FlacFormatConvert_005, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildFlacMeta(Plugins::AudioSampleFormat::SAMPLE_F32LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: FlacFormatConvert_006
 * @tc.desc: FLAC encoder with F32LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, FlacFormatConvert_006, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildFlacMeta(Plugins::AudioSampleFormat::SAMPLE_F32LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: FlacFormatConvert_007
 * @tc.desc: FLAC encoder with S16LE format, format convert disabled, should succeed (native support)
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, FlacFormatConvert_007, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildFlacMeta(Plugins::AudioSampleFormat::SAMPLE_S16LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: FlacFormatConvert_008
 * @tc.desc: FLAC encoder with S32LE format, format convert disabled, should succeed (native support)
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, FlacFormatConvert_008, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildFlacMeta(Plugins::AudioSampleFormat::SAMPLE_S32LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: FlacFormatConvert_009
 * @tc.desc: FLAC encoder reset clears format convert state
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, FlacFormatConvert_009, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildFlacMeta(Plugins::AudioSampleFormat::SAMPLE_U8, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Reset();
    // After reset, U8 without convert should fail
    auto metaNoConvert = BuildFlacMeta(Plugins::AudioSampleFormat::SAMPLE_U8, false);
    ret = plugin->SetParameter(metaNoConvert);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

// ======================== G711mu Encoder Tests ========================

/**
 * @tc.name: G711muFormatConvert_001
 * @tc.desc: G711mu encoder with S16LE format, format convert disabled, should succeed (native support)
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, G711muFormatConvert_001, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildG711muMeta(Plugins::AudioSampleFormat::SAMPLE_S16LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: G711muFormatConvert_002
 * @tc.desc: G711mu encoder with U8 format, format convert disabled, should fail
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, G711muFormatConvert_002, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildG711muMeta(Plugins::AudioSampleFormat::SAMPLE_U8, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: G711muFormatConvert_003
 * @tc.desc: G711mu encoder with U8 format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, G711muFormatConvert_003, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildG711muMeta(Plugins::AudioSampleFormat::SAMPLE_U8, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: G711muFormatConvert_004
 * @tc.desc: G711mu encoder with S24LE format, format convert disabled, should fail
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, G711muFormatConvert_004, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildG711muMeta(Plugins::AudioSampleFormat::SAMPLE_S24LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: G711muFormatConvert_005
 * @tc.desc: G711mu encoder with S24LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, G711muFormatConvert_005, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildG711muMeta(Plugins::AudioSampleFormat::SAMPLE_S24LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: G711muFormatConvert_006
 * @tc.desc: G711mu encoder with S32LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, G711muFormatConvert_006, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildG711muMeta(Plugins::AudioSampleFormat::SAMPLE_S32LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: G711muFormatConvert_007
 * @tc.desc: G711mu encoder with F32LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, G711muFormatConvert_007, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildG711muMeta(Plugins::AudioSampleFormat::SAMPLE_F32LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: G711muFormatConvert_008
 * @tc.desc: G711mu encoder reset clears format convert state
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, G711muFormatConvert_008, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildG711muMeta(Plugins::AudioSampleFormat::SAMPLE_U8, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Reset();
    // After reset, U8 without convert should fail
    auto metaNoConvert = BuildG711muMeta(Plugins::AudioSampleFormat::SAMPLE_U8, false);
    ret = plugin->SetParameter(metaNoConvert);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

// ======================== MP3 Encoder Tests ========================

/**
 * @tc.name: Mp3FormatConvert_001
 * @tc.desc: MP3 encoder with S16LE format, format convert disabled, should succeed (native support)
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, Mp3FormatConvert_001, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_MP3_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildMp3Meta(Plugins::AudioSampleFormat::SAMPLE_S16LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: Mp3FormatConvert_002
 * @tc.desc: MP3 encoder with U8 format, format convert disabled, should fail
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, Mp3FormatConvert_002, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_MP3_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildMp3Meta(Plugins::AudioSampleFormat::SAMPLE_U8, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: Mp3FormatConvert_003
 * @tc.desc: MP3 encoder with U8 format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, Mp3FormatConvert_003, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_MP3_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildMp3Meta(Plugins::AudioSampleFormat::SAMPLE_U8, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: Mp3FormatConvert_004
 * @tc.desc: MP3 encoder with S24LE format, format convert disabled, should fail
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, Mp3FormatConvert_004, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_MP3_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildMp3Meta(Plugins::AudioSampleFormat::SAMPLE_S24LE, false);
    auto ret = plugin->SetParameter(meta);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: Mp3FormatConvert_005
 * @tc.desc: MP3 encoder with S24LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, Mp3FormatConvert_005, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_MP3_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildMp3Meta(Plugins::AudioSampleFormat::SAMPLE_S24LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: Mp3FormatConvert_006
 * @tc.desc: MP3 encoder with S32LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, Mp3FormatConvert_006, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_MP3_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildMp3Meta(Plugins::AudioSampleFormat::SAMPLE_S32LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: Mp3FormatConvert_007
 * @tc.desc: MP3 encoder with F32LE format, format convert enabled, should succeed
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, Mp3FormatConvert_007, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_MP3_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildMp3Meta(Plugins::AudioSampleFormat::SAMPLE_F32LE, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Release();
}

/**
 * @tc.name: Mp3FormatConvert_008
 * @tc.desc: MP3 encoder reset clears format convert state
 * @tc.type: FUNC
 */
HWTEST_F(AudioEncoderFormatConvertUnitTest, Mp3FormatConvert_008, TestSize.Level1)
{
    auto plugin = CreatePlugin(std::string(AVCodecCodecName::AUDIO_ENCODER_MP3_NAME));
    ASSERT_NE(plugin, nullptr);
    auto meta = BuildMp3Meta(Plugins::AudioSampleFormat::SAMPLE_U8, true);
    auto ret = plugin->SetParameter(meta);
    EXPECT_EQ(ret, Status::OK);
    plugin->Reset();
    // After reset, U8 without convert should fail
    auto metaNoConvert = BuildMp3Meta(Plugins::AudioSampleFormat::SAMPLE_U8, false);
    ret = plugin->SetParameter(metaNoConvert);
    EXPECT_NE(ret, Status::OK);
    plugin->Release();
}
} // namespace MediaAVCodec
} // namespace OHOS
