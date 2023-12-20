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

constexpr int32_t CHANNEL_COUNT = 1;
constexpr int32_t SAMPLE_RATE = 48000;
constexpr int64_t BITS_RATE = 320000;
constexpr int32_t BITS_PER_CODED_SAMPLE = 24;
constexpr int32_t DEFAULT_SIZE = 100;

int32_t Create(AudioDecoderDemo *decoderDemo)
{
    return decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
}

int32_t SetCallback(AudioDecoderDemo *decoderDemo, std::shared_ptr<InnerADecDemoCallback> cb_)
{
    return decoderDemo->InnerSetCallback(cb_);
}

int32_t Configure(AudioDecoderDemo *decoderDemo)
{
    Format format;
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, BITS_RATE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, BITS_PER_CODED_SAMPLE);

    return decoderDemo->InnerConfigure(format);
}

int32_t Prepare(AudioDecoderDemo *decoderDemo)
{
    return decoderDemo->InnerPrepare();
}

int32_t Start(AudioDecoderDemo *decoderDemo)
{
    return decoderDemo->InnerStart();
}

int32_t Flush(AudioDecoderDemo *decoderDemo)
{
    return decoderDemo->InnerFlush();
}

int32_t Reset(AudioDecoderDemo *decoderDemo)
{
    return decoderDemo->InnerReset();
}

uint32_t GetInputIndex(AudioDecoderDemo *decoderDemo)
{
    return decoderDemo->NativeGetInputIndex();
}

int32_t QueueInputBuffer(AudioDecoderDemo *decoderDemo, uint32_t index)
{
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;

    info.presentationTimeUs = 0;
    info.size = DEFAULT_SIZE;
    info.offset = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;

    return decoderDemo->InnerQueueInputBuffer(index, info, flag);
}

int32_t Stop(AudioDecoderDemo *decoderDemo)
{
    return decoderDemo->InnerStop();
}

int32_t Destroy(AudioDecoderDemo *decoderDemo)
{
    return decoderDemo->InnerDestroy();
}
} // namespace

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_001
 * @tc.name      : Create -> SetCallback -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_001, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_002
 * @tc.name      : Create -> SetCallback -> Configure -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_002, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_003
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_003, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_004
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_004, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_005
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_005, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_006
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_006, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_007
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_007, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_008
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Reset -> Configure
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_008, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_009
 * @tc.name      : Create -> SetCallback -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_009, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_010
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_010, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_011
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_011, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_012
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start-> QueueInputBuffer -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_012, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_013
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start-> Flush -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_013, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_014
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start-> Stop -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_014, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_015
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start-> Reset -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_015, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_016
 * @tc.name      : Create -> SetCallback -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_016, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_017
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_017, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_018
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_018, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_019
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_019, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_020
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_020, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_021
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Reset -> Flush
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_021, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_022
 * @tc.name      : Create -> SetCallback -> stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_022, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_023
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_023, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_024
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_024, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_025
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_025, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_026
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_026, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_027
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_027, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_028
 * @tc.name      : Create -> SetCallback -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_028, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_029
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_029, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);

    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_030
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_030, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_031
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_031, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_032
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_032, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);

    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_033
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_033, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_034
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset -> Reset
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_034, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_035
 * @tc.name      : Create -> SetCallback -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_035, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_036
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_036, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_037
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_037, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_038
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_038, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    sleep(2);
    uint32_t index = signal_->inQueue_.front();

    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_039
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_039, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_040
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_040, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_041
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_041, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_042
 * @tc.name      : Create -> SetCallback -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_042, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = 0;
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_043
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_043, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = 0;
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_044
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_044, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_045
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_045, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_046
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_046, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = ERROR_INDEX;
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_047
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Start -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_047, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_048
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_048, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = ERROR_INDEX;
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_049
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_049, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = ERROR_INDEX;
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_050
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> QueueInputBuffer -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_050, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_051
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> GetInputBuffer -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_051, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);

    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_052
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_052, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    int32_t index = ERROR_INDEX;
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_053
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Flush -> Start -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_053, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Flush(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = GetInputIndex(decoderDemo);
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_054
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_054, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = ERROR_INDEX;
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_055
 * @tc.name      : Create -> SetCallback -> Configure -> Prepare -> Start -> Stop -> Reset -> QueueInputBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerInterfaceDependCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_INTERFACE_DEPEND_CHECK_055, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    int32_t ret = Create(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);

    ret = SetCallback(decoderDemo, cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Configure(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Prepare(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Reset(decoderDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint32_t index = ERROR_INDEX;
    ret = QueueInputBuffer(decoderDemo, index);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, ret);

    Destroy(decoderDemo);
    delete decoderDemo;
}
