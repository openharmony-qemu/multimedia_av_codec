/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "videoenc_sample.h"
#include "native_avcapability.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "encoderconfigure_fuzzer"

void RunNormalEncoder()
{
    VEncFuzzSample *vEncSample = new VEncFuzzSample();
    vEncSample->inpDir = "/data/test/media/1280_720_nv.yuv";
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory("video/avc", true, HARDWARE);
    string tmpCodecName = OH_AVCapability_GetName(cap);
    int32_t ret = vEncSample->CreateVideoEncoder(tmpCodecName.c_str());
    if (ret != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return;
    }
    if (vEncSample->SetVideoEncoderCallback() != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return;        
    }
    if (vEncSample->ConfigureVideoEncoder() != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return;        
    }
    if (vEncSample->StartVideoEncoder() != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return;        
    }
    vEncSample->WaitForEOS();
    delete vEncSample;
}
bool g_needRunNormalEncoder = true;
namespace OHOS {
bool EncoderConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    if (g_needRunNormalEncoder) {
        g_needRunNormalEncoder = false;
        RunNormalEncoder();
    }
    FuzzedDataProvider fdp(data, size);
    bool result = false;
    int32_t intval = fdp.ConsumeIntegral<uint32_t>();
    VEncFuzzSample *vEncSample = new VEncFuzzSample();
    vEncSample->inpDir = "/data/test/media/1280_720_nv.yuv";
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory("video/avc", true, HARDWARE);
    string tmpCodecName = OH_AVCapability_GetName(cap);
    int32_t ret = vEncSample->CreateVideoEncoder(tmpCodecName.c_str());
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
    vEncSample->fuzzMode = true;
    if (vEncSample->ConfigureVideoEncoderFuzz(intval) != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return true;         
    }
    if (vEncSample->StartVideoEncoder() != 0) {
        delete vEncSample;
        vEncSample = nullptr;
        return true;          
    }
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
    OHOS::EncoderConfigureFuzzTest(data, size);
    return 0;
}
