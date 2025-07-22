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


namespace OHOS {
bool EncoderSyncFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    std::string filename = "/data/test/corpus-EncoderSyncFuzzTest";
    SaveCorpus(data, size, filename);
    FuzzedDataProvider fdp(data, size);
    string codeName = "";
    int data1 = fdp.ConsumeIntegral<int32_t>();
    bool data2 = fdp.ConsumeBool();
    VEncSyncSample *vEncSample = new VEncSyncSample();
    vEncSample->codecType = fdp.ConsumeIntegralInRange<int32_t>(ONE, TWO);
    if (vEncSample->codecType == ONE) {
        codeName = GetCodeName(OH_AVCODEC_MIMETYPE_VIDEO_AVC, HARDWARE);
    } else if (vEncSample->codecType == TWO) {
        codeName = GetCodeName(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, HARDWARE);
    }
    if (codeName == "") {
        return false;
    }
    vEncSample->fuzzData = data;
    vEncSample->fuzzSize = size;
    vEncSample->surfInput = data2;
    vEncSample->fuzzMode = true;
    vEncSample->enbleSyncMode = fdp.ConsumeIntegral<int32_t>();
    vEncSample->syncInputWaitTime = fdp.ConsumeIntegral<int64_t>();
    vEncSample->syncOutputWaitTime = fdp.ConsumeIntegral<int64_t>();
    int32_t intval = fdp.ConsumeIntegral<uint32_t>();
    int32_t ret = vEncSample->CreateVideoEncoder(codeName.c_str());
    if (ret != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return true;
    }
    if (vEncSample->enbleSyncMode == 0) {
        vEncSample->SetVideoEncoderCallback();
    }
    vEncSample->ConfigureVideoEncoderFuzz(intval);
    vEncSample->StartVideoEncoder();
    vEncSample->SetParameter(data1);
    vEncSample->WaitForEOS();
    delete vEncSample;
    vEncSample = nullptr;
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::EncoderSyncFuzzTest(data, size);
    return 0;
}
