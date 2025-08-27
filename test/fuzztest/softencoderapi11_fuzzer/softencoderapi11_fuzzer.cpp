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
#include "softvideoenc_api11_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "softencoderapi11_fuzzer"

namespace OHOS {
bool EncoderAPI11FuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    bool result = false;
    VEncAPI11FuzzSample *vEncSample = new VEncAPI11FuzzSample();
    FuzzedDataProvider fdp(data, size);
    auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
    vEncSample->fuzzData = remaining_data.data();
    vEncSample->fuzzSize = remaining_data.size();
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory("video/avc", true, SOFTWARE);
    string tmpCodecName = OH_AVCapability_GetName(cap);
    int32_t ret = vEncSample->CreateVideoEncoder(tmpCodecName.c_str());
    if (ret != 0) {
        delete vEncSample;
        return true;
    }
    vEncSample->SetVideoEncoderCallback();
    vEncSample->ConfigureVideoEncoder();
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
    OHOS::EncoderAPI11FuzzTest(data, size);
    return 0;
}
