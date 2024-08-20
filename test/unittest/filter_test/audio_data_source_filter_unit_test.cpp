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

#include "audio_data_source_filter_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void AudioDataSourceFilterUnitTest::SetUpTestCase(void) {}

void AudioDataSourceFilterUnitTest::TearDownTestCase(void) {}

void AudioDataSourceFilterUnitTest::SetUp(void)
{
    audioDataSourceFilter_ =
        std::make_shared<AudioDataSourceFilter>("testAudioDataSourceFilter", FilterType::AUDIO_DATA_SOURCE);
}

void AudioDataSourceFilterUnitTest::TearDown(void)
{
    audioDataSourceFilter_ = nullptr;
}

/**
 * @tc.name: First
 * @tc.desc: First
 * @tc.type: FUNC
 */
HWTEST_F(AudioDataSourceFilterUnitTest, First, TestSize.Level1)
{
    ASSERT_NE(audioDataSourceFilter_, nullptr);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS