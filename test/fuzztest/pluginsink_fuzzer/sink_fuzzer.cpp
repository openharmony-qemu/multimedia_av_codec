/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <cmath>
#include <iostream>
#include <cstdint>
#include "string_ex.h"
#include "audio_server_sink_plugin.h"
#include "sink_fuzzer.h"
#include "test_template.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace OHOS {
namespace Media {
static constexpr int32_t MAX_CODE_LEN = 512;
SinkFuzzer::SinkFuzzer()
{
}

SinkFuzzer::~SinkFuzzer()
{
}

void SinkFuzzer::FuzzSinkAll(uint8_t *data, size_t size)
{
    FuzzSinkReset(data, size);
    FuzzSinkSetVolumeWithRamp(data, size);
    FuzzInit(data, size);
    FuzzSinkGetAudioEffectMode(data, size);
    FuzzSinkSetAudioEffectMode(data, size);
    FuzzSinkGetSpeed(data, size);
}

void SinkFuzzer::FuzzSinkReset(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    (void)sinkPlugin.Reset();
}

void SinkFuzzer::FuzzSinkSetVolumeWithRamp(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    float targetVolume = GetData<float>();
    int32_t duration = GetData<int32_t>();
    (void)sinkPlugin.SetVolumeWithRamp(targetVolume, duration);
}

void SinkFuzzer::FuzzSinkGetAudioEffectMode(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    int32_t effectMode = GetData<int32_t>();
    (void)sinkPlugin.GetAudioEffectMode(effectMode);
}

void SinkFuzzer::FuzzSinkSetAudioEffectMode(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    int32_t effectMode = GetData<int32_t>();
    (void)sinkPlugin.SetAudioEffectMode(effectMode);
}

void SinkFuzzer::FuzzSinkGetSpeed(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    float speed = GetData<float>();
    (void)sinkPlugin.GetSpeed(speed);
}

void SinkFuzzer::FuzzInit(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    (void)sinkPlugin.Init();
    (void)sinkPlugin.Prepare();
    (void)sinkPlugin.Start();
    (void)sinkPlugin.OnFirstFrameWriting();
    float loudnessGainTest = GetData<float>();
    (void)sinkPlugin.SetLoudnessGain(loudnessGainTest);
    (void)sinkPlugin.IsOffloading();
    timespec time;
    uint32_t framePositionTest;
    (void)sinkPlugin.GetAudioPosition(time, framePositionTest);
    size_t lengthTest = GetData<size_t>();
    (void)sinkPlugin.OnWriteData(lengthTest);
    AudioStandard::BufferDesc bufferDescTest;
    (void)sinkPlugin.GetBufferDesc(bufferDescTest);
    const AudioStandard::BufferDesc bufferDescTestOne;
    (void)sinkPlugin.EnqueueBufferDesc(bufferDescTestOne);
    (void)sinkPlugin.Drain();
    int32_t frameTest;
    (void)sinkPlugin.GetFramePosition(frameTest);
    int64_t nowUsTest = GetData<int64_t>();
    (void)sinkPlugin.GetPlayedOutDurationUs(nowUsTest);
    (void)sinkPlugin.Flush();
    (void)sinkPlugin.Freeze();
    (void)sinkPlugin.Reset();
}

} // namespace Media

void FuzzTestSink(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t) || size > MAX_CODE_LEN) {
        return;
    }
    g_baseFuzzData = data;
    g_baseFuzzSize = size;
    g_baseFuzzPos = 0;
    SinkFuzzer testSink;
    return testSink.FuzzSinkAll(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestSink(data, size);
    return 0;
}