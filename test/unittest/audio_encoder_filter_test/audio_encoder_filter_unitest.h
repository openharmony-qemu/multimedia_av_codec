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
#ifndef HISTREAMER_AUDIO_ENCODER_FILTER_UNITEST_H
#define HISTREAMER_AUDIO_ENCODER_FILTER_UNITEST_H
 
#include "gtest/gtest.h"
#include "media_codec.h"
#include "audio_encoder_filter.h"
#include "plugin/codec_plugin.h"
#include "gmock/gmock.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class AudioEncoderFilterUnitest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
 
protected:
    std::shared_ptr<AudioEncoderFilter> audioEncoderFilter_{ nullptr };
};

class MockCodecPlugin : public Plugins::CodecPlugin {
public:
    explicit MockCodecPlugin(const std::string& name) : Plugins::CodecPlugin(name) {}

    MOCK_METHOD(Status, GetInputBuffers, (std::vector<std::shared_ptr<AVBuffer>>& inputBuffers), (override));
    MOCK_METHOD(Status, GetOutputBuffers, (std::vector<std::shared_ptr<AVBuffer>>& outputBuffers), (override));
    MOCK_METHOD(Status, QueueInputBuffer, (const std::shared_ptr<AVBuffer>& inputBuffer), (override));
    MOCK_METHOD(Status, QueueOutputBuffer, (std::shared_ptr<AVBuffer>& outputBuffer), (override));
    MOCK_METHOD(Status, SetParameter, (const std::shared_ptr<Meta>& parameter), (override));
    MOCK_METHOD(Status, GetParameter, (std::shared_ptr<Meta>& parameter), (override));
    MOCK_METHOD(Status, Start, (), (override));
    MOCK_METHOD(Status, Stop, (), (override));
    MOCK_METHOD(Status, Flush, (), (override));
    MOCK_METHOD(Status, Reset, (), (override));
    MOCK_METHOD(Status, Release, (), (override));
    MOCK_METHOD(Status, SetDataCallback, (Plugins::DataCallback* dataCallback), (override));
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_AUDIO_ENCODER_FILTER_UNITEST_H