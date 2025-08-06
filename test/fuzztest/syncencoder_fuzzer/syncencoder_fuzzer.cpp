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
string g_codeName = "";
VEncSyncSample *g_vEncSample = nullptr;
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

bool ReleaseSample()
{
    delete g_vEncSample;
    g_vEncSample = nullptr;
    return false; 
}

void CodeType()
{
    if (g_vEncSample->codecType == ONE) {
        g_codeName = GetCodeName(OH_AVCODEC_MIMETYPE_VIDEO_AVC, HARDWARE);
    } else if (g_vEncSample->codecType == TWO) {
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
    g_vEncSample = new VEncSyncSample();
    g_vEncSample->codecType = fdp.ConsumeIntegralInRange<int32_t>(ONE, TWO);
    CodeType();
    bool isRgba1010102 = fdp.ConsumeBool();
    if (isRgba1010102) {
        g_vEncSample->DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_RGBA1010102;
    }
    g_vEncSample->fuzzData = data;
    g_vEncSample->fuzzSize = size;
    g_vEncSample->surfInput = fdp.ConsumeBool();
    g_vEncSample->fuzzMode = true;
    g_vEncSample->enbleBFrameMode = fdp.ConsumeIntegral<int32_t>();
    g_vEncSample->enbleSyncMode = 1;
    g_vEncSample->syncInputWaitTime = fdp.ConsumeIntegral<int64_t>();
    g_vEncSample->syncOutputWaitTime = 1;
    g_vEncSample->enableRepeat = fdp.ConsumeBool();
    g_vEncSample->setMaxCount = fdp.ConsumeBool();
    g_vEncSample->defaultKeyFrameInterval = fdp.ConsumeIntegral<uint32_t>();
    g_vEncSample->DEFAULT_BITRATE_MODE = fdp.ConsumeIntegral<uint32_t>();
    g_vEncSample->defaultQuality = fdp.ConsumeIntegral<uint32_t>();
    if (g_vEncSample->CreateVideoEncoder(g_codeName.c_str()) != 0) {
        return ReleaseSample();
    }
    if (g_vEncSample->ConfigureVideoEncoder() != 0) {
        return ReleaseSample(); 
    }
    if (g_vEncSample->surfInput) {
        g_vEncSample->CreateSurface();
    }
    if (g_vEncSample->Start() != 0) {
        return ReleaseSample();
    }
    if (g_vEncSample->surfInput) {
        g_vEncSample->InputFuncSurfaceFuzz();
    } else {
        g_vEncSample->SyncInputFuncFuzz();
    }
    g_vEncSample->SyncOutputFuncFuzz();
    g_vEncSample->SetParameter(data1);
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
