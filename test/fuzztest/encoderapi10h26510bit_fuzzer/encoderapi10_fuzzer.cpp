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
#include <cstddef>
#include <cstdint>

#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"
#include "videoenc_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
#include <fstream>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "encoderapi11_fuzzer"

void SaveCorpus(const uint8_t *data, size_t size, const std::string& filename)
{
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(data), size);
        file.close();
    }
}
void RunNormalEncoder()
{
    auto vEncSample = make_unique<VEncNdkFuzzSample>();
    int32_t ret = vEncSample->CreateVideoEncoder();
    if (ret != 0) {
        return;
    }
    if (vEncSample->SetVideoEncoderCallback() != 0) {
        return;
    }
    if (vEncSample->ConfigureVideoEncoder() != 0) {
        return;
    }
    if (vEncSample->StartVideoEncoder() != 0) {
        return;
    }
    vEncSample->WaitForEOS();

    auto vEncSampleSurf = make_unique<VEncNdkFuzzSample>();
    vEncSampleSurf->surfInput = true;
    ret = vEncSampleSurf->CreateVideoEncoder();
    if (ret != 0) {
        return;
    }
    if (vEncSampleSurf->SetVideoEncoderCallback() != 0) {
        return;
    }
    if (vEncSampleSurf->ConfigureVideoEncoder() != 0) {
        return;
    }
    if (vEncSampleSurf->StartVideoEncoder() != 0) {
        return;
    }
    vEncSampleSurf->WaitForEOS();
}

bool g_needRunNormalEncoder = true;
namespace OHOS {
bool EncoderAPI10FuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    std::string filename = "/data/test/corpus-EncoderAPI10H26510BitFuzzTest";
    SaveCorpus(data, size, filename);
    if (g_needRunNormalEncoder) {
        g_needRunNormalEncoder = false;
        RunNormalEncoder();
    }
    bool result = false;
    FuzzedDataProvider fdp(data, size);
    int data1 = fdp.ConsumeIntegral<int32_t>();
    bool data2 = fdp.ConsumeBool();
    VEncNdkFuzzSample *vEncSample = new VEncNdkFuzzSample();
    vEncSample->fuzzMode = true;
    vEncSample->fuzzData = data;
    vEncSample->fuzzSize = size;
    vEncSample->surfInput = data2;
    int32_t ret = vEncSample->CreateVideoEncoder();
    if (ret != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return true;
    }
    if (vEncSample->SetVideoEncoderCallback() != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return true;        
    }
    if (vEncSample->ConfigureVideoEncoder() != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return true;        
    }
    if (vEncSample->StartVideoEncoder() != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return true;         
    }
    vEncSample->SetParameterFuzz(data1);
    vEncSample->WaitForEOS();
    delete vEncSample;
    vEncSample = nullptr;
    return result;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::EncoderAPI10FuzzTest(data, size);
    return 0;
}
