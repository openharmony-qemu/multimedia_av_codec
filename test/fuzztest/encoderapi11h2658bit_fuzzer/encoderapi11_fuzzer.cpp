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
#include "videoenc_api11_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
#include <fstream>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "encoderapi11_fuzzer"

VEncAPI11FuzzSample *g_vEncSample = nullptr;

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
    auto vEncSample = make_unique<VEncAPI11FuzzSample>();
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

    auto vEncSampleSurf = make_unique<VEncAPI11FuzzSample>();
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

bool ReleaseSample()
{
    delete g_vEncSample;
    g_vEncSample = nullptr;
    return true;
}

bool g_needRunNormalEncoder = true;
namespace OHOS {
bool EncoderAPI11FuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    std::string filename = "/data/test/corpus-EncoderAPI11H2658BitFuzzTest";
    SaveCorpus(data, size, filename);
    if (g_needRunNormalEncoder) {
        g_needRunNormalEncoder = false;
        RunNormalEncoder();
    }
    FuzzedDataProvider fdp(data, size);
    int data1 = fdp.ConsumeIntegral<int32_t>();
    bool data2 = fdp.ConsumeBool();
    g_vEncSample = new VEncAPI11FuzzSample();
    g_vEncSample->fuzzData = data;
    g_vEncSample->fuzzSize = size;
    g_vEncSample->surfInput = data2;
    g_vEncSample->fuzzMode = true;
    g_vEncSample->enableRepeat = fdp.ConsumeBool();
    g_vEncSample->setMaxCount = fdp.ConsumeBool();
    g_vEncSample->defaultRangeFlag = fdp.ConsumeIntegral<uint32_t>();
    g_vEncSample->defaultColorPrimaries = fdp.ConsumeIntegral<uint32_t>();
    g_vEncSample->defaultTransferCharacteristics = fdp.ConsumeIntegral<uint32_t>();
    g_vEncSample->defaultMatarixCoefficients = fdp.ConsumeIntegral<uint32_t>();
    g_vEncSample->defaultKeyFrameInterval = fdp.ConsumeIntegral<uint32_t>();
    g_vEncSample->defaultBitrateMode = fdp.ConsumeIntegral<uint32_t>();
    g_vEncSample->defaultBitRate = fdp.ConsumeIntegral<uint32_t>();
    g_vEncSample->defaultQuality = fdp.ConsumeIntegral<uint32_t>();
    g_vEncSample->defaultFrameAfter = fdp.ConsumeIntegral<int32_t>();
    g_vEncSample->defaultMaxCount = fdp.ConsumeIntegral<int32_t>();
    if (g_vEncSample->CreateVideoEncoder() != 0) {
        return ReleaseSample();
    }
    if (g_vEncSample->SetVideoEncoderCallback() != 0) {
        return ReleaseSample();
    }
    if (g_vEncSample->ConfigureVideoEncoder() != 0) {
        return ReleaseSample();        
    }
    if (g_vEncSample->StartVideoEncoder() != 0) {
        return ReleaseSample();        
    }
    g_vEncSample->SetParameter(data1);
    g_vEncSample->WaitForEOS();
    return ReleaseSample();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::EncoderAPI11FuzzTest(data, size);
    return 0;
}
