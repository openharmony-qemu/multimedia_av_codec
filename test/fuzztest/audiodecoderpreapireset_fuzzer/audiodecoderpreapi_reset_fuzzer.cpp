/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "audiodecoderdemo.h"
#define FUZZ_PROJECT_NAME "audiodecoderpreapireset_fuzzer"

using namespace std;
using namespace OHOS::MediaAVCodec;
using namespace OHOS;
using namespace OHOS::MediaAVCodec::AudioDemoAuto;

namespace OHOS {
bool AudioDecoderAACResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ aac
    ADecDemoAuto* aDecBufferDemo = new ADecDemoAuto();
    aDecBufferDemo->InitFile("aac");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderFlacResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ flac
    ADecDemoAuto* aDecBufferDemo = new ADecDemoAuto();
    aDecBufferDemo->InitFile("flac");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderVORBISResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ vorbis
    ADecDemoAuto* aDecBufferDemo = new ADecDemoAuto();
    aDecBufferDemo->InitFile("vorbis");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderAMRNBResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ amrnb
    ADecDemoAuto* aDecBufferDemo = new ADecDemoAuto();
    aDecBufferDemo->InitFile("amrnb");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderAMRWBResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ amrwb
    ADecDemoAuto* aDecBufferDemo = new ADecDemoAuto();
    aDecBufferDemo->InitFile("amrwb");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderOPUSResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ opus
    ADecDemoAuto* aDecBufferDemo = new ADecDemoAuto();
    aDecBufferDemo->InitFile("opus");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderG711ResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ g711
    ADecDemoAuto* aDecBufferDemo = new ADecDemoAuto();
    aDecBufferDemo->InitFile("g711mu");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderMP3ResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ mp3
    ADecDemoAuto* aDecBufferDemo = new ADecDemoAuto();
    aDecBufferDemo->InitFile("mp3");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioDecoderAACResetFuzzTest(data, size);
    OHOS::AudioDecoderFlacResetFuzzTest(data, size);
    OHOS::AudioDecoderG711ResetFuzzTest(data, size);
    OHOS::AudioDecoderVORBISResetFuzzTest(data, size);
    OHOS::AudioDecoderAMRNBResetFuzzTest(data, size);
    OHOS::AudioDecoderAMRWBResetFuzzTest(data, size);
    OHOS::AudioDecoderMP3ResetFuzzTest(data, size);
    return 0;
}