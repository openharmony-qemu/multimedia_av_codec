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
#ifndef AUDIO_DATA_SOURCE_FILTER_UNITTEST_H
#define AUDIO_DATA_SOURCE_FILTER_UNITTEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "mock/task.h"
#include "audio_data_source_filter.h"
#include "audio_data_source_filter.cpp"

namespace OHOS {
namespace Media {
namespace Pipeline {
class AudioDataSourceFilterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    std::shared_ptr<AudioDataSourceFilter> filter_;
};

class MockAudioDataSource : public IAudioDataSource {
public:
    MOCK_METHOD(AudioDataSourceReadAtActionState, ReadAt,
                (std::shared_ptr<AVBuffer> buffer, uint32_t length), (override));
    MOCK_METHOD(int32_t, GetSize, (int64_t& size), (override));
    MOCK_METHOD(void, SetVideoFirstFramePts, (int64_t firstFramePts), (override));
};

} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // AUDIO_DATA_SOURCE_FILTER_UNITTEST_H