/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#include "meta/format.h"
#include "avcodec_audio_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
constexpr uint32_t SIZE_NUM = 100;
constexpr uint32_t SAMPLE_RATE_44100 = 44100;
constexpr uint32_t SAMPLE_RATE_112000 = 112000;

namespace {
class InnerInterfaceDependCheckTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void InnerInterfaceDependCheckTest::SetUpTestCase() {}
void InnerInterfaceDependCheckTest::TearDownTestCase() {}
void InnerInterfaceDependCheckTest::SetUp() {}
void InnerInterfaceDependCheckTest::TearDown() {}

int32_t Create(AudioEncoderDemo *encoderDemo)
{
    return encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
}

int32_t SetCallback(AudioEncoderDemo *encoderDemo, const std::shared_ptr<AVCodecCallback> &cb_)
{
    return encoderDemo->InnerSetCallback(cb_);
}
int32_t Configure(AudioEncoderDemo *encoderDemo)
{
    Format format;

    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    return encoderDemo->InnerConfigure(format);
}

int32_t Prepare(AudioEncoderDemo *encoderDemo)
{
    return encoderDemo->InnerPrepare();
}

int32_t Start(AudioEncoderDemo *encoderDemo)
{
    int32_t ret = encoderDemo->InnerStartWithThread();
    sleep(1);
    return ret;
}

int32_t Flush(AudioEncoderDemo *encoderDemo)
{
    return encoderDemo->InnerFlush();
}

int32_t Reset(AudioEncoderDemo *encoderDemo)
{
    return encoderDemo->InnerReset();
}

int32_t QueueInputBuffer(AudioEncoderDemo *encoderDemo, uint32_t index)
{
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    info.presentationTimeUs = 0;
    info.size = SIZE_NUM;
    info.offset = 0;
    flag = AVCODEC_BUFFER_FLAG_PARTIAL_FRAME;
    return encoderDemo->InnerQueueInputBuffer(index, info, flag);
}

int32_t Stop(AudioEncoderDemo *encoderDemo)
{
    return encoderDemo->InnerStop();
}

int32_t Destroy(AudioEncoderDemo *encoderDemo)
{
    return encoderDemo->InnerDestroy();
}
} // namespace

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_001
 * @tc.name      : Create -> SetCallback -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_001, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_002
 * @tc.name      : Create -> SetCallback -> Configure -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_002, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_003
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_003, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_004
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_004, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_005
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_005, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = encoderDemo->InnerGetInputIndex();

    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_006
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_006, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_007
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_007, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_008
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Reset -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_008, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_009
 * @tc.name      : Create -> SetCallback -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_009, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);

    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_010
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_010, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_011
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_011, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_012
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start-> QueueInputBuffer -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_012, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = 0;
    index = encoderDemo->InnerGetInputIndex();
    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_013
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_013, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_014
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start-> Stop -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_014, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_015
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Reset -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_015, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_016
 * @tc.name      : Create -> SetCallback -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_016, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_017
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_017, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_018
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_018, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    int32_t index = encoderDemo->InnerGetInputIndex();

    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_019
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_019, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_020
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_020, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_021
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Reset -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_021, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_022
 * @tc.name      : Create -> SetCallback -> stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_022, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_023
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_023, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_024
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_024, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = encoderDemo->InnerGetInputIndex();

    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_025
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_025, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_026
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_026, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_027
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_027, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_028
 * @tc.name      : Create -> SetCallback -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_028, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_029
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_029, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_030
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_030, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_031
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_031, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_032
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_032, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = encoderDemo->InnerGetInputIndex();

    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_033
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_033, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_034
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_034, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_035
 * @tc.name      : Create -> SetCallback -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_035, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_036
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_036, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_037
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_037, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_038
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_038, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = encoderDemo->InnerGetInputIndex();

    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_039
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_039, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_040
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_040, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_041
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_041, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_042
 * @tc.name      : Create -> SetCallback -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_042, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = 0;
    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_043
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_043, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = 0;
    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_044
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_044, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = encoderDemo->InnerGetInputIndex();
    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_045
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_045, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = encoderDemo->InnerGetInputIndex();
    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    index = encoderDemo->InnerGetInputIndex();
    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_046
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_046, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = 0;
    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_047
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Start -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_047, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = encoderDemo->InnerGetInputIndex();
    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(0, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_048
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_048, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = 0;
    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_049
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_INTERFACE_DEPEND_CHECK_049, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    int32_t ret = Create(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);

    ret = SetCallback(encoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(encoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = 0;
    ret = QueueInputBuffer(encoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(encoderDemo);
    delete encoderDemo;
}
