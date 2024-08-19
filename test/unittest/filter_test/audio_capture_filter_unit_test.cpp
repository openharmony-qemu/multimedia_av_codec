/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "audio_capture_filter_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void AudioCaptureFilterUnitTest::SetUpTestCase(void) {}

void AudioCaptureFilterUnitTest::TearDownTestCase(void) {}

void AudioCaptureFilterUnitTest::SetUp(void)
{
    audioCaptureFilter_ =
        std::make_shared<AudioCaptureFilter>("testAudioCaptureFilter", FilterType::AUDIO_CAPTURE);
}

void AudioCaptureFilterUnitTest::TearDown(void)
{
    audioCaptureFilter_ = nullptr;
}

/**
 * @tc.name: DecoderSurfaceFilter_Destructor_0100
 * @tc.desc: ~DecoderSurfaceFilter
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, DecoderSurfaceFilter_Destructor_0100, TestSize.Level1)
{
    ASSERT_NE(audioCaptureFilter_, nullptr);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS