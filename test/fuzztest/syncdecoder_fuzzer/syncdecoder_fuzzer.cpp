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
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "syncdecoder_sample.h"
#include "native_avcapability.h"
#include <fuzzer/FuzzedDataProvider.h>
#include <fstream>
#include <cstring>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "syncdecoder_fuzzer"

constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
constexpr double DEFAULT_FRAME_RATE = 30.0;
constexpr int32_t ONE = 1;
constexpr int32_t TWO = 2;
constexpr int32_t THREE = 3;
constexpr int32_t FOUR = 4;
constexpr int32_t FIVE = 5;
constexpr int32_t SIX = 6;
constexpr int32_t SEVEN = 7;
constexpr int32_t EIGHT = 8;
OH_AVCapability *cap = nullptr;
VDecSyncSample *g_vDecSample = nullptr;
string g_codeName = "";

namespace OHOS {
void SaveCorpus(const uint8_t *data, size_t size, const std::string& filename)
{
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(data), size);
        file.close();
    }
}

string GetcodeName(const char* mimeName, OH_AVCodecCategory category)
{
    cap = OH_AVCodec_GetCapabilityByCategory(mimeName, false, category);
    if (cap == nullptr) {
        return "";
    }
    return OH_AVCapability_GetName(cap);
}

void CodepType()
{
    if (g_vDecSample->codecType == ONE) {
        g_codeName = GetcodeName(OH_AVCODEC_MIMETYPE_VIDEO_AVC, HARDWARE);
    } else if (g_vDecSample->codecType == TWO) {
        g_codeName = GetcodeName(OH_AVCODEC_MIMETYPE_VIDEO_AVC, SOFTWARE);
    } else if (g_vDecSample->codecType == THREE) {
        g_codeName = GetcodeName(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, HARDWARE);
    } else if (g_vDecSample->codecType == FOUR) {
        g_codeName = GetcodeName(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, SOFTWARE);
    } else if (g_vDecSample->codecType == FIVE) {
        g_codeName = GetcodeName(OH_AVCODEC_MIMETYPE_VIDEO_VVC, HARDWARE);
    } else if (g_vDecSample->codecType == SIX) {
        g_codeName = GetcodeName(OH_AVCODEC_MIMETYPE_VIDEO_H263, SOFTWARE);
    } else if (g_vDecSample->codecType == SEVEN) {
        g_codeName = GetcodeName(OH_AVCODEC_MIMETYPE_VIDEO_MPEG2, SOFTWARE);
    } else if (g_vDecSample->codecType == EIGHT) {
        g_codeName = GetcodeName(OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2, SOFTWARE);
    }
}

bool DecoderSyncFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    std::string filename = "/data/test/corpus-SyncDecoderFuzzTest";
    SaveCorpus(data, size, filename);
    FuzzedDataProvider fdp(data, size);
    int data0 = fdp.ConsumeIntegral<int32_t>();
    g_vDecSample = new VDecSyncSample();
    g_vDecSample->codecType = fdp.ConsumeIntegralInRange<int32_t>(ONE, EIGHT);
    CodepType();
    g_vDecSample->sfOutput = fdp.ConsumeBool();
    g_vDecSample->defaultWidth = DEFAULT_WIDTH;
    g_vDecSample->defaultHeight = DEFAULT_HEIGHT;
    g_vDecSample->defaultFrameRate = DEFAULT_FRAME_RATE;
    g_vDecSample->enbleSyncMode = 1;
    g_vDecSample->enbleBlankFrame = fdp.ConsumeIntegral<int>();
    g_vDecSample->syncInputWaitTime = fdp.ConsumeIntegral<int64_t>();
    g_vDecSample->syncOutputWaitTime = 1;
    g_vDecSample->renderTimestampNs = fdp.ConsumeIntegral<int64_t>();
    g_vDecSample->isRenderAttime = fdp.ConsumeBool();
    int32_t ret = g_vDecSample->CreateVideoDecoder(g_codeName);
    if (ret != AV_ERR_OK) {
        delete g_vDecSample;
        g_vDecSample = nullptr;
        return false;
    }
    if (g_vDecSample->ConfigureVideoDecoder() !=AV_ERR_OK) {
        delete g_vDecSample;
        g_vDecSample = nullptr;
        return false;
    }
    if (g_vDecSample->sfOutput) {
        g_vDecSample->DecodeSetSurface();
    }
    if (g_vDecSample->Start() != AV_ERR_OK) {
        delete g_vDecSample;
        g_vDecSample = nullptr;
        return false;
    }
    g_vDecSample->SyncInputFuncFuzz(data, size);
    g_vDecSample->SyncOutputFuncFuzz();
    g_vDecSample->SetParameter(data0);
    g_vDecSample->Flush();
    g_vDecSample->Stop();
    g_vDecSample->Reset();
    delete g_vDecSample;
    g_vDecSample = nullptr;
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DecoderSyncFuzzTest(data, size);
    return 0;
}
