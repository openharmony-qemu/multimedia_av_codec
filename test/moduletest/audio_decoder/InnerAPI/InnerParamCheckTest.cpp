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
#include "AudioDecoderDemoCommon.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "av_common.h"
#include "meta/format.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;

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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_001
 * @tc.name      : InnerCreateByMime
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_001, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();

    int32_t ret = decoderDemo->InnerCreateByMime("audio/mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByMime("audio/mp4a-latm");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByMime("audio/vorbis");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByMime("audio/flac");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByMime("audio/amr-wb");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByMime("audio/3gpp");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByMime("audio/g711mu");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByMime("aaa");
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    decoderDemo->InnerDestroy();
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_002
 * @tc.name      : InnerCreateByName
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_002, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();

    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Flac");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Vorbis");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Amrwb");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Amrnb");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.G711mu");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    decoderDemo->InnerDestroy();

    ret = decoderDemo->InnerCreateByName("aaa");
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    decoderDemo->InnerDestroy();
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_003
 * @tc.name      : InnerConfigure - MD_KEY_BITRATE
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_003, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();

    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerReset();
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, -1);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerReset();
    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_BITRATE, "aaaaaa");
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerReset();
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, 0);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerReset();
    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_BITRATE, 0.1);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerReset();
    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_BITRATE, 0.1);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t b[100];
    decoderDemo->InnerReset();
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_BITRATE, b, 100);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerDestroy();
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_004
 * @tc.name      : InnerConfigure - MD_KEY_CHANNEL_COUNT check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_004, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerReset();
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, -1);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerReset();
    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, "aaaaaa");
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerReset();
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerReset();
    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0.1);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerReset();
    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0.1);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    uint8_t b[100];
    decoderDemo->InnerReset();
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, b, 100);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerDestroy();
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_005
 * @tc.name      : InnerConfigure - MD_KEY_SAMPLE_RATE check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_005, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerReset();
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, -1);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerReset();
    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, "aaaaaa");
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerReset();
    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerReset();
    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0.1);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerReset();
    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0.1);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    uint8_t b[100];
    decoderDemo->InnerReset();
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, b, 100);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerDestroy();
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_006
 * @tc.name      : InnerQueueInputBuffer - index check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_006, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerPrepare();
    decoderDemo->InnerStart();

    uint32_t index = decoderDemo->NativeGetInputIndex();
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;

    info.presentationTimeUs = 0;
    info.size = 100;
    info.offset = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;

    ret = decoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    index = -1;
    ret = decoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_NO_MEMORY, ret);

    decoderDemo->InnerDestroy();
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_007
 * @tc.name      : InnerQueueInputBuffer - info.presentationTimeUs check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_007, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerPrepare();
    decoderDemo->InnerStart();

    uint32_t index = decoderDemo->NativeGetInputIndex();
    std::shared_ptr<AVSharedMemory> buffer = signal_->inInnerBufQueue_.front();
    ASSERT_NE(nullptr, buffer);

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;

    info.presentationTimeUs = 0;
    info.size = 100;
    info.offset = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;

    ret = decoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    index = decoderDemo->NativeGetInputIndex();
    info.presentationTimeUs = -1;
    ret = decoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerDestroy();
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_008
 * @tc.name      : InnerQueueInputBuffer - info.size check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_008, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerPrepare();
    decoderDemo->InnerStart();

    uint32_t index = decoderDemo->NativeGetInputIndex();

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;

    info.presentationTimeUs = 0;
    info.size = 100;
    info.offset = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;

    ret = decoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    index = decoderDemo->NativeGetInputIndex();
    info.size = -1;
    ret = decoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerDestroy();

    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_009
 * @tc.name      : InnerQueueInputBuffer - offset
 * @tc.desc      : param check test
 */
HWTEST_F(InnerParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_009, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format audioParams;

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    audioParams.PutIntValue("bits_per_coded_sample", 4);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);
    ret = decoderDemo->InnerConfigure(audioParams);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    decoderDemo->InnerPrepare();
    decoderDemo->InnerStart();

    uint32_t index = decoderDemo->NativeGetInputIndex();

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;

    info.presentationTimeUs = 0;
    info.size = 100;
    info.offset = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;
    index = 0;

    ret = decoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    index = decoderDemo->NativeGetInputIndex();
    info.offset = -1;
    ret = decoderDemo->InnerQueueInputBuffer(index, info, flag);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    decoderDemo->InnerDestroy();
    delete decoderDemo;
}
