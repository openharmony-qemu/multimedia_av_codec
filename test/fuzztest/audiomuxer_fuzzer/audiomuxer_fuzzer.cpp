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
#include "audio_muxer_demo.h"
#define FUZZ_PROJECT_NAME "audiomuxer_fuzzer"

using namespace std;
using namespace OHOS::MediaAVCodec;
using namespace OHOS;
using namespace OHOS::MediaAVCodec::AudioBufferDemo;

namespace OHOS {

bool AudioMuxerAACFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ aac
    AVMuxerDemo* aMuxerBufferDemo = new AVMuxerDemo();
    aMuxerBufferDemo->InitFile("aac");
    auto res = aMuxerBufferDemo->RunCase(data, size);
    delete aMuxerBufferDemo;
    return res;
}

bool AudioMuxerMPEG3FuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ mpeg3
    AVMuxerDemo* aMuxerBufferDemo = new AVMuxerDemo();
    aMuxerBufferDemo->InitFile("mpeg3");
    auto res = aMuxerBufferDemo->RunCase(data, size);
    delete aMuxerBufferDemo;
    return res;
}

bool AudioMuxerAMRNBFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ amrnb
    AVMuxerDemo* aMuxerBufferDemo = new AVMuxerDemo();
    aMuxerBufferDemo->InitFile("amrnb");
    auto res = aMuxerBufferDemo->RunCase(data, size);
    delete aMuxerBufferDemo;
    return res;
}

bool AudioMuxerAMRWBFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ amrwb
    AVMuxerDemo* aMuxerBufferDemo = new AVMuxerDemo();
    aMuxerBufferDemo->InitFile("amrwb");
    auto res = aMuxerBufferDemo->RunCase(data, size);
    delete aMuxerBufferDemo;
    return res;
}

bool AudioMuxerJPGFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ jpg
    AVMuxerDemo* aMuxerBufferDemo = new AVMuxerDemo();
    aMuxerBufferDemo->InitFile("jpg");
    auto res = aMuxerBufferDemo->RunCase(data, size);
    delete aMuxerBufferDemo;
    return res;
}

}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioMuxerAACFuzzTest(data, size);
    OHOS::AudioMuxerMPEG3FuzzTest(data, size);
    OHOS::AudioMuxerAMRNBFuzzTest(data, size);
    OHOS::AudioMuxerAMRWBFuzzTest(data, size);
    OHOS::AudioMuxerJPGFuzzTest(data, size);
    return 0;
}