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
#include "audio_decoder_g711a_demo.h"
#include "audio_decoder_g711a_new_demo.h"
#define FUZZ_PROJECT_NAME "audiodecoderg711a_fuzzer"

using namespace std;
using namespace OHOS::MediaAVCodec;
using namespace OHOS;
using namespace OHOS::MediaAVCodec::AudioBufferDemo;
using namespace OHOS::MediaAVCodec::AudioBufferNewDemo;

namespace OHOS {

bool AudioDecoderG711AFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ g711 new
    G711aFuzzDemo* g711aFuzzDemo = new G711aFuzzDemo();
    g711aFuzzDemo->DoG711aParserWithParserAPI(data, size);
    // FUZZ g711
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
    aDecBufferDemo->InitFile("g711a");
    auto res = aDecBufferDemo->RunCase(data, size);
    delete aDecBufferDemo;
    return res;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioDecoderG711AFuzzTest(data, size);
    return 0;
}