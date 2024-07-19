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
#include "audio_decoder_reset_demo.h"
#define FUZZ_PROJECT_NAME "audiodecoderreset_fuzzer"

using namespace std;
using namespace OHOS::MediaAVCodec;
using namespace OHOS;
using namespace OHOS::MediaAVCodec::AudioBufferDemo;

namespace OHOS {

bool AudioDecoderFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ OH_AudioCodec_CreateByMime
    std::string codecdata(reinterpret_cast<const char*>(data), size);
    OH_AVCodec *source =  OH_AudioCodec_CreateByMime(codecdata.c_str(), true);
    if (source) {
        OH_AudioCodec_Destroy(source);
    }
    OH_AVCodec *sourcename =  OH_AudioCodec_CreateByName(codecdata.c_str());
    if (sourcename) {
        OH_AudioCodec_Destroy(sourcename);
    }
    return true;
}

bool AudioDecoderAACResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ lbvc
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
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
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
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
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
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
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
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
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
    aDecBufferDemo->InitFile("amrwb");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderVividResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ vivid
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
    aDecBufferDemo->InitFile("vivid");
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
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
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
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
    aDecBufferDemo->InitFile("g711mu");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderAPEResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ ape
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
    aDecBufferDemo->InitFile("ape");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderLBVCResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ lbvc
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
    aDecBufferDemo->InitFile("lbvc");
    auto res = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return res;
}

bool AudioDecoderMP3ResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ lbvc
    ADecBufferDemo* aDecBufferDemo = new ADecBufferDemo();
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
    OHOS::AudioDecoderFuzzTest(data, size);
    OHOS::AudioDecoderAACResetFuzzTest(data, size);
    OHOS::AudioDecoderFlacResetFuzzTest(data, size);
    OHOS::AudioDecoderAPEResetFuzzTest(data, size);
    OHOS::AudioDecoderG711ResetFuzzTest(data, size);
    OHOS::AudioDecoderOPUSResetFuzzTest(data, size);
    OHOS::AudioDecoderLBVCResetFuzzTest(data, size);
    OHOS::AudioDecoderVividResetFuzzTest(data, size);
    OHOS::AudioDecoderAMRNBResetFuzzTest(data, size);
    OHOS::AudioDecoderAMRWBResetFuzzTest(data, size);
    OHOS::AudioDecoderMP3ResetFuzzTest(data, size);
    return 0;
}