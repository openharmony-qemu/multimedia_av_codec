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

const size_t THRESHOLD = 10;
static const uint8_t* RAW_DATA = nullptr;
static size_t g_dataSize = 0;
static size_t g_pos;
typedef void (*TestFuncs)();

template<class T>
T GetData()
{
    T object {};
    size_t objectSize = sizeof(object);
    if (RAW_DATA == nullptr || objectSize > g_dataSize - g_pos) {
        return object;
    }
    errno_t ret = memcpy_s(&object, objectSize, RAW_DATA + g_pos, objectSize);
    if (ret != EOK) {
        return {};
    }
    g_pos += objectSize;
    return object;
}

template<class T>
uint32_t GetArrLength(T& arr)
{
    if (arr == nullptr) {
        return 0;
    }
    return sizeof(arr) / sizeof(arr[0]);
}

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

TestFuncs g_testFuncs[] = {
    AudioDecoderFuzzTest,
    AudioDecoderAACResetFuzzTest,
    AudioDecoderFlacResetFuzzTest,
    AudioDecoderAPEResetFuzzTest,
    AudioDecoderG711ResetFuzzTest,
    AudioDecoderOPUSResetFuzzTest,
    AudioDecoderLBVCResetFuzzTest,
    AudioDecoderVividResetFuzzTest,
    AudioDecoderAMRNBResetFuzzTest,
    AudioDecoderAMRWBResetFuzzTest,
    AudioDecoderMP3ResetFuzzTest,
};

bool FuzzTest(const uint8_t* rawData, size_t size)
{
    if (rawData == nullptr) {
        return false;
    }

    // initialize data
    RAW_DATA = rawData;
    g_dataSize = size;
    g_pos = 0;

    uint32_t code = GetData<uint32_t>();
    uint32_t len = GetArrLength(g_testFuncs);
    if (len > 0) {
        g_testFuncs[code % len]();
    } else {
        return false;
    }

    return true;
}

}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (size < OHOS::THRESHOLD) {
        return 0;
    }

    OHOS::FuzzTest(data, size);
    return 0;
}