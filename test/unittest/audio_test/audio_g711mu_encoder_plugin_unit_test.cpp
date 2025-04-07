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

#include "gtest/gtest.h"
#include "audio_g711mu_encoder_plugin.h"
#include "media_description.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "avcodec_audio_common.h"

namespace {
constexpr int32_t SUPPORT_CHANNELS = 1;
constexpr int SUPPORT_SAMPLE_RATE = 8000;
} // namespace

using namespace std;
using namespace testing::test;

namespace OHOS {
namespace MediaAVCodec {
class G711EncPluginUnitTest : public testing::test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    std::shared_ptr<AudioG711muEncoderPlugin> g711EncPlugin_;
};

void G711EncPluginUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void G711EncPluginUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void G711EncPluginUnitTest::SetUp(void)
{
    g711EncPlugin_ = std::make_shared<AudioG711muEncoderPlugin>();
    cout << "[SetUp]: SetUp!!!" << endl;
}

void G711EncPluginUnitTest::TearDown(void)
{
    g711EncPlugin_->Release();
    g711EncPlugin_->Reset();
    cout << "[TearDown]: over!!!" << endl;
}

/**
 * @tc.name: G711MuLawEncode_001
 * @tc.desc: nagative value
 * @tc.type: FUNC
 */
HWTEST_F(G711EncPluginUnitTest, G711MuLawEncode_001, TestSize.Level1)
{
    Format format;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SUPPORT_SAMPLE_RATE);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, SUPPORT_CHANNELS);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    auto ret = g711EncPlugin_->Init(format);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);

    ret = g711EncPlugin_->CheckSampleFormat();
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
    int16_t pcmValue = -3000; // negative value
    uint8_t result = g711EncPlugin_->G711MuLawEncode(pcmValue);
    EXPECT_EQ(result, 0x37);
}

/**
 * @tc.name: G711MuLawEncode_002
 * @tc.desc: zero value
 * @tc.type: FUNC
 */
HWTEST_F(G711EncPluginUnitTest, G711MuLawEncode_002, TestSize.Level1)
{
    Format format;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SUPPORT_SAMPLE_RATE);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, SUPPORT_CHANNELS);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    auto ret = g711EncPlugin_->Init(format);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);

    ret = g711EncPlugin_->CheckSampleFormat();
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
    int16_t pcmValue = 0; // zero value
    uint8_t result = g711EncPlugin_->G711MuLawEncode(pcmValue);
    EXPECT_EQ(result, 0xFF);
}

 /**
 * @tc.name: G711MuLawEncode_003
 * @tc.desc: positive value and LT AVCODEC_G711MU_CLIP
 * @tc.type: FUNC
 */
HWTEST_F(G711EncPluginUnitTest, G711MuLawEncode_003, TestSize.Level1)
{
    Format format;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SUPPORT_SAMPLE_RATE);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, SUPPORT_CHANNELS);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    auto ret = g711EncPlugin_->Init(format);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);

    ret = g711EncPlugin_->CheckSampleFormat();
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
    int16_t pcmValue = 3000; // positive value and LT AVCODEC_G711MU_CLIP
    uint8_t result = g711EncPlugin_->G711MuLawEncode(pcmValue);
    EXPECT_EQ(result, 0xB7);
}

 /**
 * @tc.name: G711MuLawEncode_004
 * @tc.desc: positive value and GT AVCODEC_G711MU_CLIP
 * @tc.type: FUNC
 */
HWTEST_F(G711EncPluginUnitTest, G711MuLawEncode_004, TestSize.Level1)
{
    Format format;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SUPPORT_SAMPLE_RATE);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, SUPPORT_CHANNELS);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    auto ret = g711EncPlugin_->Init(format);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);

    ret = g711EncPlugin_->CheckSampleFormat();
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
    int16_t pcmValue = 10000; // positive value and GT AVCODEC_G711MU_CLIP
    uint8_t result = g711EncPlugin_->G711MuLawEncode(pcmValue);
    EXPECT_EQ(result, 0x9C);
}

 /**
 * @tc.name: G711MuLawEncode_005
 * @tc.desc: GT AVCODEC_G711MU_CLIP and LT AVCODEC_G711MU_SEG_END[7]
 * @tc.type: FUNC
 */
HWTEST_F(G711EncPluginUnitTest, G711MuLawEncode_005, TestSize.Level1)
{
    Format format;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SUPPORT_SAMPLE_RATE);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, SUPPORT_CHANNELS);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    auto ret = g711EncPlugin_->Init(format);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);

    ret = g711EncPlugin_->CheckSampleFormat();
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
    int16_t pcmValue = 8500; // GT AVCODEC_G711MU_CLIP and LT AVCODEC_G711MU_SEG_END[7]
    uint8_t result = g711EncPlugin_->G711MuLawEncode(pcmValue);
    EXPECT_EQ(result, 0x9F);
}

 /**
 * @tc.name: G711MuLawEncode_006
 * @tc.desc: LT AVCODEC_G711MU_SEG_END[7], seg EQ 8
 * @tc.type: FUNC
 */
HWTEST_F(G711EncPluginUnitTest, G711MuLawEncode_006, TestSize.Level1)
{
    Format format;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SUPPORT_SAMPLE_RATE);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, SUPPORT_CHANNELS);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    auto ret = g711EncPlugin_->Init(format);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);

    ret = g711EncPlugin_->CheckSampleFormat();
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
    int16_t pcmValue = 32670; // LT AVCODEC_G711MU_SEG_END[7], seg EQ 8
    uint8_t result = g711EncPlugin_->G711MuLawEncode(pcmValue);
    EXPECT_EQ(result, 0x80);
}

 /**
 * @tc.name: G711MuLawEncode_007
 * @tc.desc: EQ AVCODEC_G711MU_CLIP
 * @tc.type: FUNC
 */
HWTEST_F(G711EncPluginUnitTest, G711MuLawEncode_007, TestSize.Level1)
{
    Format format;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SUPPORT_SAMPLE_RATE);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, SUPPORT_CHANNELS);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    auto ret = g711EncPlugin_->Init(format);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);

    ret = g711EncPlugin_->CheckSampleFormat();
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
    int16_t pcmValue = 8159; // EQ AVCODEC_G711MU_CLIP
    uint8_t result = g711EncPlugin_->G711MuLawEncode(pcmValue);
    EXPECT_EQ(result, 0x9F);
}

 /**
 * @tc.name: G711MuLawEncode_008
 * @tc.desc: GT AVCODEC_G711MU_CLIP and LT LT AVCODEC_G711MU_SEG_END[7]
 * @tc.type: FUNC
 */
HWTEST_F(G711EncPluginUnitTest, G711MuLawEncode_008, TestSize.Level1)
{
    Format format;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SUPPORT_SAMPLE_RATE);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, SUPPORT_CHANNELS);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    auto ret = g711EncPlugin_->Init(format);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);

    ret = g711EncPlugin_->CheckSampleFormat();
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
    int16_t pcmValue = 8200; // GT AVCODEC_G711MU_CLIP and LT LT AVCODEC_G711MU_SEG_END[7]
    uint8_t result = g711EncPlugin_->G711MuLawEncode(pcmValue);
    EXPECT_EQ(result, 0x9F);
}
} // namespace MediaAVCodec
} // namespace OHOS