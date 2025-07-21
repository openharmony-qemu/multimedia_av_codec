/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
string codeName = "";
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

void CodeType()
{
    if (vEncSample->codecType == 1) {
        codeName = GetCodeName(OH_AVCODEC_MIMETYPE_VIDEO_AVC, HARDWARE);
    } else if (vEncSample->codecType == 2) {
        codeName = GetCodeName(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, HARDWARE);
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
    bool data2 = fdp.ConsumeBool();
    vEncSample = new VEncSyncSample();
    vEncSample->codecType = fdp.ConsumeIntegralInRange<int32_t>(1, 2);
    CodeType();
    bool isRgba1010102 = fdp.ConsumeBool();
    if (isRgba1010102) {
        vEncSample->DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_RGBA1010102;
    }
    vEncSample->fuzzData = data;
    vEncSample->fuzzSize = size;
    vEncSample->SURF_INPUT = data2;
    vEncSample->fuzzMode = true;
    vEncSample->enbleBFrameMode = fdp.ConsumeIntegral<int32_t>();
    vEncSample->enbleSyncMode = fdp.ConsumeIntegral<int32_t>();
    vEncSample->syncInputWaitTime = fdp.ConsumeIntegral<int64_t>();
    vEncSample->syncOutputWaitTime = fdp.ConsumeIntegral<int64_t>();
    vEncSample->enableRepeat = fdp.ConsumeBool();
    vEncSample->setMaxCount = fdp.ConsumeBool();
    vEncSample->DEFAULT_KEY_FRAME_INTERVAL = fdp.ConsumeIntegral<uint32_t>();
    vEncSample->DEFAULT_BITRATE_MODE = fdp.ConsumeIntegral<uint32_t>();
    vEncSample->DEFAULT_QUALITY = fdp.ConsumeIntegral<uint32_t>();
    int32_t ret = vEncSample->CreateVideoEncoder(codeName.c_str());
    if (ret != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return true;
    }
    if (vEncSample->enbleSyncMode == 0) {
        vEncSample->SetVideoEncoderCallback();
    }
    vEncSample->ConfigureVideoEncoder();
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
