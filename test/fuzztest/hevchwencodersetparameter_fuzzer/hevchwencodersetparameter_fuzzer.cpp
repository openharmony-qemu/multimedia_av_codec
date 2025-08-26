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
#include <fuzzer/FuzzedDataProvider.h>
#include <fstream>

#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"
#include "videoenc_api11_sample.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "hwencodersetparam_fuzzer"

VEncAPI11FuzzSample *g_vEncSample = nullptr;

void SaveCorpus(const uint8_t *data, size_t size, const std::string& filename)
{
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(data), size);
        file.close();
    }
}

bool ReleaseSample()
{
    delete g_vEncSample;
    g_vEncSample = nullptr;
    return true;
}

namespace OHOS {
bool HwEncoderSetParamFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    std::string filename = "/data/test/corpus-EncoderAPI11FuzzTest";
    SaveCorpus(data, size, filename);
    FuzzedDataProvider fdp(data, size);
    bool data2 = fdp.ConsumeBool();
    auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
    int64_t maxBite = 100000000;
    int32_t sqrFactor = 32;
    g_vEncSample = new VEncAPI11FuzzSample();
    g_vEncSample->fuzzData = remaining_data.data();
    g_vEncSample->fuzzSize = remaining_data.size();
    g_vEncSample->surfInput = data2;
    g_vEncSample->fuzzMode = true;
    g_vEncSample->defaultBitrateMode = SQR;
    g_vEncSample->factorEnable = true;
    g_vEncSample->maxbiteEnable = true;
    g_vEncSample->defaultSqrFactor = sqrFactor;
    g_vEncSample->defaultMaxBitrate = maxBite;
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory("video/hevc", true, HARDWARE);
    string tmpCodecName = OH_AVCapability_GetName(cap);
    if (g_vEncSample->CreateVideoEncoder(tmpCodecName.c_str()) != AV_ERR_OK) {
        return ReleaseSample();
    }
    if (g_vEncSample->SetVideoEncoderCallback() != AV_ERR_OK) {
        return ReleaseSample();
    }
    if (g_vEncSample->ConfigureVideoEncoder() != AV_ERR_OK) {
        return ReleaseSample();
    }
    if (g_vEncSample->StartVideoEncoder() != AV_ERR_OK) {
        return ReleaseSample();
    }
    g_vEncSample->SetParameter();
    g_vEncSample->WaitForEOS();
    return ReleaseSample();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::HwEncoderSetParamFuzzTest(data, size);
    return 0;
}
