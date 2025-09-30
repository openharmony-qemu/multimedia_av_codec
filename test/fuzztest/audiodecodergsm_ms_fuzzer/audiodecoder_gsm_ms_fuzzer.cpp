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
#include <atomic>
#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include "audio_decoder_gsm_ms_demo.h"
#define FUZZ_PROJECT_NAME "audiodecodergsm_ms_fuzzer"

using namespace std;
using namespace OHOS::MediaAVCodec;
using namespace OHOS;
using namespace OHOS::MediaAVCodec::AudioBufferDemo;

namespace OHOS {

void AudioDecoderGsmMsFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int64_t)) {
        return;
    }

    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
    if (aDecBufferDemo == nullptr) {
        return;
    }
    aDecBufferDemo->InitFile("gsm_ms");
    aDecBufferDemo->RunCase(data, size);
    delete aDecBufferDemo;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioDecoderGsmMsFuzzTest(data, size);
    return 0;
}