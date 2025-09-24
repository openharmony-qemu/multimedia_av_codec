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
    // Test isProcessInputMerged_ == false
    filter->isProcessInputMerged_ = false;
    filter->isAsyncMode_ = false;
    filter->OnBufferAvailable();
    // Test NeedImmediateRender() == false
    filter->isRenderCallbackMode_ = false;
    filter->OnBufferAvailable();
    // Test NeedImmediateRender() == true
    filter->needImmediateRender_ = true;
    filter->audioSink_ = std::make_shared<AudioSink>();
    EXPECT_CALL(*(filter->audioSink_), DrainOutputBuffer(_)).WillOnce(Return());
    filter->OnBufferAvailable();
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS