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
#include "videoenc_api11_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "encoderapi11_fuzzer"

namespace OHOS {
bool EncoderAPI11FuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    bool result = false;
    int32_t data_ = *reinterpret_cast<const int32_t *>(data);
    VEncAPI11FuzzSample *vEncSample = new VEncAPI11FuzzSample();
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory("video/avc", true, HARDWARE);
    string tmpCodecName = OH_AVCapability_GetName(cap);
    vEncSample->CreateVideoEncoder(tmpCodecName.c_str());
    vEncSample->SetVideoEncoderCallback();
    vEncSample->fuzzMode = true;
    vEncSample->ConfigureVideoEncoderFuzz(data_);
    vEncSample->StartVideoEncoder();
    vEncSample->SetParameter(data_);
    vEncSample->WaitForEOS();
    delete vEncSample;

    vEncSample = new VEncAPI11FuzzSample();
    vEncSample->CreateVideoEncoder(tmpCodecName.c_str());
    vEncSample->SetVideoEncoderCallback();
    vEncSample->surfInput = true;
    vEncSample->ConfigureVideoEncoderFuzz(data_);
    vEncSample->StartVideoEncoder();
    vEncSample->SetParameter(data_);
    vEncSample->WaitForEOS();
    delete vEncSample;

    return result;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::EncoderAPI11FuzzTest(data, size);
    return 0;
}
