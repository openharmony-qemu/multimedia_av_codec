/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include <string>
#include "gtest/gtest.h"
#include "AudioEncoderDemoCommon.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "av_common.h"
#include "format.h"
#include "avcodec_audio_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
constexpr uint32_t SAMPLE_RATE_44100 = 44100;
constexpr uint32_t SAMPLE_RATE_112000 = 112000;

namespace {
class InnerParamCheckTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void InnerParamCheckTest::SetUpTestCase() {}
void InnerParamCheckTest::TearDownTestCase() {}
void InnerParamCheckTest::SetUp() {}
void InnerParamCheckTest::TearDown() {}
} // namespace

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_001
 * @tc.name      : InnerCreateByMime
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_001, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();

    int32_t ret = encoderDemo->InnerCreateByMime("audio/mp4a-latm");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    encoderDemo->InnerDestroy();

    ret = encoderDemo->InnerCreateByMime("audio/flac");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    encoderDemo->InnerDestroy();

    ret = encoderDemo->InnerCreateByMime("aaa");
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);
    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_002
 * @tc.name      : InnerCreateByName
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_002, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();

    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    encoderDemo->InnerDestroy();

    ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Flac");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    encoderDemo->InnerDestroy();

    ret = encoderDemo->InnerCreateByName("aaa");
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);
    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_003
 * @tc.name      : InnerConfigure - MD_KEY_BITRATE
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_003, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();

    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    encoderDemo->InnerReset();
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, -1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_AUD_PARAMS, ret);

    encoderDemo->InnerReset();
    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_BITRATE, "aaaaaa");
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_AUD_PARAMS, ret);

    encoderDemo->InnerReset();
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, -1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_AUD_PARAMS, ret);

    encoderDemo->InnerReset();
    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_BITRATE, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_AUD_PARAMS, ret);

    encoderDemo->InnerReset();
    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_BITRATE, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_AUD_PARAMS, ret);

    encoderDemo->InnerReset();
    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_BITRATE, b, 100);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_AUD_PARAMS, ret);

    encoderDemo->InnerDestroy();
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_004
 * @tc.name      : InnerConfigure - MD_KEY_BITS_PER_CODED_SAMPLE
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_004, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();

    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, -1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, "aaaaaa");
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, 0);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, 0);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, b, 100);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_005
 * @tc.name      : InnerConfigure - MD_KEY_AUDIO_SAMPLE_FORMAT
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_005, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();

    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, -1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, "aaaaaa");
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, 0);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, 0);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, b, 100);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_006
 * @tc.name      : InnerConfigure - MD_KEY_BITRATE check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_006, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, -1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, 0);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_BITRATE, "aaaaaa");
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 0);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_BITRATE, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_BITRATE, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_BITRATE, b, 100);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_007
 * @tc.name      : InnerConfigure - MD_KEY_CHANNEL_LAYOUT check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_007, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, -1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, "aaaaaa");
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, 0);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, b, 100);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}
/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_009
 * @tc.name      : InnerConfigure - MD_KEY_CHANNEL_COUNT check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_009, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, -1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, "aaaaaa");
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, b, 100);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_010
 * @tc.name      : InnerConfigure - MD_KEY_SAMPLE_RATE check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_010, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, -1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, "aaaaaa");
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0.1);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, b, 100);
    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_012
 * @tc.name      : InnerQueueInputBuffer - info check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_012, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    ret = encoderDemo->InnerSetCallback(cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    encoderDemo->InnerPrepare();
    encoderDemo->InnerStart();

    sleep(2);
    int32_t index;

    std::shared_ptr<AVSharedMemory> results;

    index = signal_->inQueue_.front();

    results = encoderDemo->InnerGetInputBuffer(index);
    ASSERT_NE(nullptr, results);

    index = -1;
    results = encoderDemo->InnerGetInputBuffer(index);
    ASSERT_EQ(nullptr, results);

    index = 100;
    results = encoderDemo->InnerGetInputBuffer(index);
    ASSERT_EQ(nullptr, results);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_013
 * @tc.name      : InnerQueueInputBuffer size
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_013, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    ret = encoderDemo->InnerSetCallback(cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    encoderDemo->InnerPrepare();
    encoderDemo->InnerStart();
    sleep(2);
    int32_t index;
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;

    info.presentationTimeUs = 0;
    info.size = 100;
    info.offset = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;
    index = 0;

    ret = encoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    info.size = -1;
    ret = encoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_014
 * @tc.name      : InnerQueueInputBuffer - offset
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_014, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    ret = encoderDemo->InnerSetCallback(cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    encoderDemo->InnerPrepare();
    encoderDemo->InnerStart();
    sleep(2);
    int32_t index;
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;

    index = signal_->inQueue_.front();
    std::shared_ptr<AVSharedMemory> buffer = encoderDemo->InnerGetInputBuffer(index);
    ASSERT_NE(nullptr, buffer);

    info.presentationTimeUs = 0;
    info.size = 100;
    info.offset = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;
    index = 0;

    ret = encoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    sleep(2);
    info.offset = -1;
    ret = encoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_016
 * @tc.name      : InnerGetInputBuffer - index check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_016, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    ret = encoderDemo->InnerSetCallback(cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    encoderDemo->InnerPrepare();
    encoderDemo->InnerStart();
    sleep(2);
    int32_t index;
    index = signal_->inQueue_.front();
    std::shared_ptr<AVSharedMemory> results = encoderDemo->InnerGetInputBuffer(index);

    index = 0;
    results = encoderDemo->InnerGetInputBuffer(index);
    ASSERT_NE(nullptr, results);

    index = -1;
    results = encoderDemo->InnerGetInputBuffer(index);
    ASSERT_EQ(nullptr, results);

    index = 100;
    results = encoderDemo->InnerGetInputBuffer(index);
    ASSERT_EQ(nullptr, results);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_017
 * @tc.name      : InnerGetOutputBuffer - index check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_017, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    ret = encoderDemo->InnerSetCallback(cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    encoderDemo->InnerPrepare();
    encoderDemo->InnerStart();
    sleep(2);
    int32_t index;
    index = signal_->inQueue_.front();
    std::shared_ptr<AVSharedMemory> results;

    index = 0;
    results = encoderDemo->InnerGetOutputBuffer(index);
    ASSERT_EQ(nullptr, results);

    index = -1;
    results = encoderDemo->InnerGetOutputBuffer(index);
    ASSERT_EQ(nullptr, results);

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}
