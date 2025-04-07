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
#include "audio_resample.h"
#include "avcodec_log.h"
#include "securec.h"
#include "ffmpeg_converter.h"
#include "audio_buffer_info.h"

using namespace std;
using namespace testing::test;

namespace OHOS {
namespace MediaAVCodec {
class AudioResampleUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    std::shared_ptr<AudioResample> audioResample_;
};

void AudioResampleUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioResampleUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioResampleUnitTest::SetUp(void)
{
    audioResample_ = std::make_shared<AudioResample>();
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioResampleUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
}

/**
 * @tc.name: InitSwrContext_001
 * @tc.desc: err < 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioResampleUnitTest, InitSwrContext_001, TestSize.Level1)
{
    ResamplePara para;
    para.sampleRate = -1; // invalid samplerate

    int32_t ret = audioResample_->InitSwrContext(para);
    EXPECT_NE(ret, AVCodecServiceErrCode::AVCS_ERR_OK);
}

/**
 * @tc.name: InitSwrContext_002
 * @tc.desc: swr_init(swrContext) == 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioResampleUnitTest, InitSwrContext_002, TestSize.Level1)
{
    ResamplePara para;
    para.sampleRate = 44100;
    para.srcFmt = AV_SAMPLE_FMT_S16;
    para.destFmt = AV_SAMPLE_FMT_S16;
    para.channelLayout = 2; // STEREO
    para.destSamplesPerFrame = 1024;

    int32_t ret = audioResample_->InitSwrContext(para);
    EXPECT_NE(ret, AVCodecServiceErrCode::AVCS_ERR_OK);
}

/**
 * @tc.name: Init_001
 * @tc.desc: Init failed
 * @tc.type: FUNC
 */
HWTEST_F(AudioResampleUnitTest, Init_001, TestSize.Level1)
{
    ResamplePara para;
    para.sampleRate = 44100;
    para.srcFmt = AV_SAMPLE_FMT_S16;
    para.destFmt = AV_SAMPLE_FMT_S16;
    para.channelLayout = 2; // STEREO
    para.destSamplesPerFrame = 1024;

    int32_t ret = audioResample_->Init(para);
    EXPECT_NE(ret, AVCodecServiceErrCode::AVCS_ERR_OK);
}

/**
 * @tc.name: ComvertFrame_001
 * @tc.desc: ComvertFrame failed
 * @tc.type: FUNC
 */
HWTEST_F(AudioResampleUnitTest, ComvertFrame_001, TestSize.Level1)
{
    auto outputFrame = nullptr;
    auto inputFrame = nullptr;

    auto ret = audioResample_->ConvertFrame(outputFrame, inputFrame);
    EXPECT_NE(ret, AVCodecServiceErrCode::AVCS_ERR_OK);
}
} // namespace MediaAVCodec
} // namespace OHOS
