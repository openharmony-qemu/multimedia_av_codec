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

#ifndef SINKSECOND_FUZZER
#define SINKSECOND_FUZZER

#include <fcntl.h>
#include <securec.h>
#include <unistd.h>
#include <cstdint>
#include <climits>
#include <cstdio>
#include <cstdlib>

#define FUZZ_PROJECT_NAME "sinksecond_fuzzer"

namespace OHOS {
namespace Media {
class SinkFuzzerSecond {
public:
    SinkFuzzerSecond();
    ~SinkFuzzerSecond();
    void FuzzSinkAll(uint8_t *data, size_t size);
    void InitDate(Plugins::AudioServerSinkPlugin &sinkPlugin);
    void FlushDate(Plugins::AudioServerSinkPlugin &sinkPlugin);
    void FuzzResume(uint8_t *data, size_t size);
    void FuzzPause(uint8_t *data, size_t size);
    void FuzzPauseTransitent(uint8_t *data, size_t size);
    void FuzzGetLatency(uint8_t *data, size_t size);
    void FuzzDrainCacheDataOne(uint8_t *data, size_t size);
    void FuzzDrainCacheDataTwo(uint8_t *data, size_t size);
    void FuzzCacheData(uint8_t *data, size_t size);
    void FuzzWriteAudioBuffer(uint8_t *data, size_t size);
};
} // namespace Media
void FuzzTestSinkSecond(uint8_t *data, size_t size);
} // namespace OHOS
#endif