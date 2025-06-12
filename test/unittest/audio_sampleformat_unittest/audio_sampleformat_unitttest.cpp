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

#include "audio_sampleformat_unittest.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing;
using namespace testing::ext;

static const int32_t RIGHT_RET = 8;
static const int32_t WRONG_RET = -1;

void AudioSampleFormatUnitTest::SetUpTestCase(void)
{
}

void AudioSampleFormatUnitTest::TearDownTestCase(void)
{
}

void AudioSampleFormatUnitTest::SetUp(void)
{
}

void AudioSampleFormatUnitTest::TearDown(void)
{
}

/**
 * @tc.name  : Test AudioSampleFormatToBitDepth
 * @tc.number: AudioSampleFormatToBitDepth_001
 * @tc.desc  : Test SAMPLEFORMAT_INFOS.count(sampleFormat) != 0
 *             Test SAMPLEFORMAT_INFOS.count(sampleFormat) == 0
 */
HWTEST_F(AudioSampleFormatUnitTest, AudioSampleFormatToBitDepth_001, TestSize.Level0)
{
    // Test SAMPLEFORMAT_INFOS.count(sampleFormat) != 0
    EXPECT_EQ(AudioSampleFormatToBitDepth(Plugins::SAMPLE_U8), RIGHT_RET);
    // Test SAMPLEFORMAT_INFOS.count(sampleFormat) == 0
    EXPECT_EQ(AudioSampleFormatToBitDepth(Plugins::SAMPLE_S16BE), WRONG_RET);
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS