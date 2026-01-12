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

#ifndef SINK_FUZZER
#define SINK_FUZZER

#include <fcntl.h>
#include <securec.h>
#include <unistd.h>
#include <cstdint>
#include <climits>
#include <cstdio>
#include <cstdlib>

#define FUZZ_PROJECT_NAME "sink_fuzzer"

namespace OHOS {
namespace Media {
class SinkFuzzer {
public:
    SinkFuzzer();
    ~SinkFuzzer();
    void FuzzSinkReset(uint8_t *data, size_t size);
    void FuzzSinkAll(uint8_t *data, size_t size);
    void FuzzSinkSetVolumeWithRamp(uint8_t *data, size_t size);
    void FuzzInit(uint8_t *data, size_t size);
    void FuzzSinkGetAudioEffectMode(uint8_t *data, size_t size);
    void FuzzSinkSetAudioEffectMode(uint8_t *data, size_t size);
    void FuzzSinkGetSpeed(uint8_t *data, size_t size);
};
} // namespace Media
void FuzzTestSink(uint8_t *data, size_t size);
} // namespace OHOS
#endif