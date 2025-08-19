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
#include "videodec_inner_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "native_avcapability.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "hwdecoderinner_fuzzer"
VDecNdkInnerFuzzSample *g_vDecSample = nullptr;

namespace OHOS {
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
    delete g_vDecSample;
    g_vDecSample = nullptr;
    return true;
}

bool HwdecoderInnerFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    std::string filename = "/data/test/corpus-HwdecoderInnerFuzzTest";
    SaveCorpus(data, size, filename);
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    if (cap == nullptr) {
        return false;
    }
    string codecName = OH_AVCapability_GetName(cap);
    FuzzedDataProvider fdp(data, size);
    g_vDecSample = new VDecNdkInnerFuzzSample();
    g_vDecSample->isP3Full = fdp.ConsumeBool();
    if (g_vDecSample->isP3Full) {
        g_vDecSample->defaultColorspace = static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_FULL);
    }
    g_vDecSample->sfOutput = fdp.ConsumeBool();
    int32_t ret = g_vDecSample->CreateByName(codecName);
    if (ret != AV_ERR_OK) {
        return ReleaseSample();
    }
    ret = g_vDecSample->Configure();
    if (ret != AV_ERR_OK) {
        return ReleaseSample();
    }
    if (g_vDecSample->SetCallback() != AV_ERR_OK) {
        return ReleaseSample();
    }
    if (g_vDecSample->sfOutput) {
        g_vDecSample->SetOutputSurface();
    }
    g_vDecSample->Prepare();
    ret = g_vDecSample->Start();
    if (ret != AV_ERR_OK) {
        return ReleaseSample();
    }
    g_vDecSample->InputFuncFUZZ(data, size);
    g_vDecSample->Flush();
    g_vDecSample->Stop();
    g_vDecSample->Reset();
    return ReleaseSample();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::HwdecoderInnerFuzzTest(data, size);
    return 0;
}
