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

#include "audio_sink_filter_unittest.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing;
using namespace testing::ext;

void AudioSinkFilterUnitTest::SetUpTestCase(void) {}

void AudioSinkFilterUnitTest::TearDownTestCase(void) {}

void AudioSinkFilterUnitTest::SetUp(void) {}

void AudioSinkFilterUnitTest::TearDown(void) {}

/**
 * @tc.name  : Test OnBufferAvailable
 * @tc.number: OnBufferAvailable_001
 * @tc.desc  : Test isProcessInputMerged_ == false
 *             Test NeedImmediateRender() == false
 *             Test NeedImmediateRender() == true
 */
HWTEST_F(AudioSinkFilterUnitTest, OnBufferAvailable_001, TestSize.Level0)
{
    auto filter = std::make_shared<AudioSinkFilter>("testAudioSinkFilter");
    filter->isProcessInputMerged_ = false;
    filter->isAsyncMode_ = false;
    filter->OnBufferAvailable();
    filter->isRenderCallbackMode_ = false;
    filter->OnBufferAvailable();
    filter->needImmediateRender_ = true;
    filter->audioSink_ = std::make_shared<AudioSink>();
    EXPECT_CALL(*(filter->audioSink_), DrainOutputBuffer(_)).WillOnce(Return());
    filter->OnBufferAvailable();
}

/**
 * @tc.name  : Test GetParameter
 * @tc.number: GetParameter_001
 * @tc.desc  : Test GetParameter with valid audioSink_
 */
HWTEST_F(AudioSinkFilterUnitTest, GetParameter_001, TestSize.Level0)
{
    auto filter = std::make_shared<AudioSinkFilter>("testAudioSinkFilter");
    auto mockAudioSink = std::make_shared<AudioSink>();
    filter->audioSink_ = mockAudioSink;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    EXPECT_CALL(*mockAudioSink, GetParameter(_)).WillOnce(Return());
    filter->GetParameter(meta);
}

/**
 * @tc.name  : Test DoSetPerfRecEnabled
 * @tc.number: DoSetPerfRecEnabled_001
 * @tc.desc  : Test DoSetPerfRecEnabled with valid audioSink_
 */
HWTEST_F(AudioSinkFilterUnitTest, DoSetPerfRecEnabled_001, TestSize.Level0)
{
    auto filter = std::make_shared<AudioSinkFilter>("testAudioSinkFilter");
    auto mockAudioSink = std::make_shared<AudioSink>();
    filter->audioSink_ = mockAudioSink;
    EXPECT_CALL(*mockAudioSink, SetPerfRecEnabled(true)).WillOnce(Return(Status::OK));
    auto ret = filter->DoSetPerfRecEnabled(true);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(filter->isPerfRecEnabled_);
}

/**
 * @tc.name  : Test DoSetPerfRecEnabled
 * @tc.number: DoSetPerfRecEnabled_002
 * @tc.desc  : Test DoSetPerfRecEnabled with nullptr audioSink_
 */
HWTEST_F(AudioSinkFilterUnitTest, DoSetPerfRecEnabled_002, TestSize.Level0)
{
    auto filter = std::make_shared<AudioSinkFilter>("testAudioSinkFilter");
    filter->audioSink_ = nullptr;
    auto ret = filter->DoSetPerfRecEnabled(false);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_FALSE(filter->isPerfRecEnabled_);
}

/**
 * @tc.name  : Test SetVolumeWithRamp
 * @tc.number: SetVolumeWithRamp_001
 * @tc.desc  : Test SetVolumeWithRamp covers the function implementation
 */
HWTEST_F(AudioSinkFilterUnitTest, SetVolumeWithRamp_001, TestSize.Level0)
{
    auto filter = std::make_shared<AudioSinkFilter>("testAudioSinkFilter");
    auto mockAudioSink = std::make_shared<AudioSink>();
    filter->audioSink_ = mockAudioSink;
    float targetVolume = 0.5f;
    int32_t duration = 1000;
    int32_t expectedRet = 0;
    EXPECT_CALL(*mockAudioSink, SetVolumeWithRamp(targetVolume, duration)).WillOnce(Return(expectedRet));
    auto ret = filter->SetVolumeWithRamp(targetVolume, duration);
    EXPECT_EQ(ret, expectedRet);
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS