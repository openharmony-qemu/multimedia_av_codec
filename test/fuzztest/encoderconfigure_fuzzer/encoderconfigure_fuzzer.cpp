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
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "encoderconfigure_fuzzer"

namespace OHOS {
bool EncoderConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    bool result = false;
    int32_t data_ = *reinterpret_cast<const int32_t *>(data);
    VEncFuzzSample *vEncSample = new VEncFuzzSample();
    vEncSample->inpDir = "/data/test/media/1280_720_nv.yuv";
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory("video/avc", true, HARDWARE);
    string tmpCodecName = OH_AVCapability_GetName(cap);
    vEncSample->CreateVideoEncoder(tmpCodecName.c_str());
    vEncSample->SetVideoEncoderCallback();
    vEncSample->fuzzMode = true;
    vEncSample->ConfigureVideoEncoderFuzz(data_);
    vEncSample->StartVideoEncoder();
    vEncSample->WaitForEOS();
    delete vEncSample;
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
