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
#include "sinksecond_fuzzer.h"
#include "test_template.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace OHOS {
namespace Media {
static constexpr int32_t MAX_CODE_LEN = 512;
static constexpr int32_t CONSTANT_ONE = 6;
static constexpr int32_t CONSTANT_TWO = 2;
SinkFuzzerSecond::SinkFuzzerSecond()
{
}

SinkFuzzerSecond::~SinkFuzzerSecond()
{
}

void SinkFuzzerSecond::FuzzSinkAll(uint8_t *data, size_t size)
{
    FuzzResume(data, size);
    FuzzPause(data, size);
    FuzzPauseTransitent(data, size);
    FuzzGetLatency(data, size);
    FuzzDrainCacheDataOne(data, size);
    FuzzDrainCacheDataTwo(data, size);
    FuzzCacheData(data, size);
    FuzzWriteAudioBuffer(data, size);
}

void SinkFuzzerSecond::InitDate(Plugins::AudioServerSinkPlugin &sinkPlugin)
{
    if (sinkPlugin.Init() != Status::OK) {
        cout << "Init failed !" << endl;
    }
    (void)sinkPlugin.Prepare();
}

void SinkFuzzerSecond::FlushDate(Plugins::AudioServerSinkPlugin &sinkPlugin)
{
    (void)sinkPlugin.Flush();
    (void)sinkPlugin.Freeze();
    (void)sinkPlugin.Reset();
}

void SinkFuzzerSecond::FuzzResume(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);
    InitDate(sinkPlugin);
    (void)sinkPlugin.Resume();
    FlushDate(sinkPlugin);
}

void SinkFuzzerSecond::FuzzPause(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    InitDate(sinkPlugin);
    (void)sinkPlugin.Start();
    (void)sinkPlugin.Pause();
    FlushDate(sinkPlugin);
}

void SinkFuzzerSecond::FuzzPauseTransitent(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    InitDate(sinkPlugin);
    (void)sinkPlugin.Start();
    (void)sinkPlugin.PauseTransitent();
    FlushDate(sinkPlugin);
}

void SinkFuzzerSecond::FuzzGetLatency(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    InitDate(sinkPlugin);
    (void)sinkPlugin.Start();
    uint64_t hstTime;
    (void)sinkPlugin.GetLatency(hstTime);
    FlushDate(sinkPlugin);
}

void SinkFuzzerSecond::FuzzDrainCacheDataOne(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    InitDate(sinkPlugin);
    (void)sinkPlugin.Start();
    AVBufferConfig config;
    uint8_t randomMemType = GetData<uint8_t>();
    config.memoryType = static_cast<MemoryType>(randomMemType % CONSTANT_ONE);
    uint8_t randomSizeRaw = GetData<uint8_t>();
    config.size = randomSizeRaw % CONSTANT_ONE + CONSTANT_TWO;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    (void)sinkPlugin.Write(buffer);
    (void)sinkPlugin.OnFirstFrameWriting();
    FlushDate(sinkPlugin);
}

void SinkFuzzerSecond::FuzzDrainCacheDataTwo(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    InitDate(sinkPlugin);
    (void)sinkPlugin.Start();
    AVBufferConfig config;
    uint8_t randomMemType = GetData<uint8_t>();
    config.memoryType = static_cast<MemoryType>(randomMemType % CONSTANT_ONE);
    uint8_t randomSizeRaw = GetData<uint8_t>();
    config.size = randomSizeRaw % CONSTANT_ONE + CONSTANT_TWO;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    (void)sinkPlugin.Write(buffer);
    FlushDate(sinkPlugin);
}

void SinkFuzzerSecond::FuzzCacheData(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    InitDate(sinkPlugin);
    (void)sinkPlugin.Start();
    (void)sinkPlugin.SetMuted(true);
    AVBufferConfig config;
    uint8_t randomMemType = GetData<uint8_t>();
    config.memoryType = static_cast<MemoryType>(randomMemType % CONSTANT_ONE);
    uint8_t randomSizeRaw = GetData<uint8_t>();
    config.size = randomSizeRaw % CONSTANT_ONE + CONSTANT_TWO;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    (void)sinkPlugin.Write(buffer);
    FlushDate(sinkPlugin);
}

void SinkFuzzerSecond::FuzzWriteAudioBuffer(uint8_t *data, size_t size)
{
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string videold(testStrings[randomNum % testStrings.size()]);
    Plugins::AudioServerSinkPlugin sinkPlugin(videold);

    InitDate(sinkPlugin);
    (void)sinkPlugin.Start();
    AVBufferConfig config;
    uint8_t randomMemType = GetData<uint8_t>();
    config.memoryType = static_cast<MemoryType>(randomMemType % CONSTANT_ONE);
    uint8_t randomSizeRaw = GetData<uint8_t>();
    config.size = randomSizeRaw % CONSTANT_ONE + CONSTANT_TWO;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    (void)sinkPlugin.WriteAudioVivid(buffer);
    FlushDate(sinkPlugin);
}

} // namespace Media

void FuzzTestSinkSecond(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t) || size > MAX_CODE_LEN) {
        return;
    }
    g_baseFuzzData = data;
    g_baseFuzzSize = size;
    g_baseFuzzPos = 0;

    SinkFuzzerSecond testSink;
    return testSink.FuzzSinkAll(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestSinkSecond(data, size);
    return 0;
}