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
#include "syncvideoenc_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
#include <fstream>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "syncencoder_fuzzer"


OH_AVCapability *cap = nullptr;
constexpr int32_t ONE = 1;
constexpr int32_t TWO = 2;
string g_codeName = "";
VEncSyncSample *vEncSample = nullptr;

void SaveCorpus(const uint8_t *data, size_t size, const std::string& filename)
{
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(data), size);
        file.close();
    }
}

string GetCodeName(const char* mimeName, OH_AVCodecCategory category)
{
    cap = OH_AVCodec_GetCapabilityByCategory(mimeName, true, category);
    if (cap == nullptr) {
        return "";
    }
    return OH_AVCapability_GetName(cap);
}

bool ReleaseSample()
{
    delete vEncSample;
    vEncSample = nullptr;
    return true;  
}

void CodecType()
{
    if (vEncSample->codecType == ONE) {
        g_codeName = GetCodeName(OH_AVCODEC_MIMETYPE_VIDEO_AVC, HARDWARE);
    } else if (vEncSample->codecType == TWO) {
        g_codeName = GetCodeName(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, HARDWARE);
    }
}

namespace OHOS {
bool EncoderSyncFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    std::string filename = "/data/test/corpus-EncoderSyncFuzzTest";
    SaveCorpus(data, size, filename);
    FuzzedDataProvider fdp(data, size);
    int data1 = fdp.ConsumeIntegral<int32_t>();
    vEncSample = new VEncSyncSample();
    vEncSample->codecType = fdp.ConsumeIntegralInRange<int32_t>(ONE, TWO);
    CodecType();
    if (g_codeName == "") {
        return false;
    }
    vEncSample->fuzzData = data;
    vEncSample->fuzzSize = size;
    vEncSample->surfInput = fdp.ConsumeBool();
    vEncSample->fuzzMode = true;
    vEncSample->enbleSyncMode = fdp.ConsumeIntegral<int32_t>();
    vEncSample->syncInputWaitTime = fdp.ConsumeIntegral<int64_t>();
    vEncSample->syncOutputWaitTime = 1;
    int32_t intval = fdp.ConsumeIntegral<uint32_t>();
    int32_t ret = vEncSample->CreateVideoEncoder(g_codeName.c_str());
    if (ret != 0) {
        return ReleaseSample();
    }
    if (vEncSample->ConfigureVideoEncoderFuzz(intval) != 0) {
        return ReleaseSample();
    }
    if (vEncSample->surfInput) {
        vEncSample->CreateSurface();
    }
    if (vEncSample->enbleSyncMode == 0) {
        return ReleaseSample();     
    }
    if (vEncSample->Start() != 0) {
        return ReleaseSample();
    }
    if (vEncSample->surfInput) {
        vEncSample->InputFuncSurfaceFuzz();
    } else {
        vEncSample->SyncInputFuncFuzz();
    }
    vEncSample->SyncOutputFuncFuzz();
    vEncSample->SetParameter(data1);
    return ReleaseSample();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::EncoderSyncFuzzTest(data, size);
    return 0;
}
