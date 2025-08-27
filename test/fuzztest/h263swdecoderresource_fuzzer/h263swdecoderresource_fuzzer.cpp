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
#include <fuzzer/FuzzedDataProvider.h>
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "videodec_sample.h"

#define FUZZ_PROJECT_NAME "h263swdecoderresource_fuzzer"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

static VDecFuzzSample *vDecSample = nullptr;

namespace OHOS {

void Release()
{
    vDecSample->Release();
    delete vDecSample;
    vDecSample = nullptr;
}
bool H263SwdecoderFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (!vDecSample) {
        vDecSample = new VDecFuzzSample();
        double frameRateMin = 1.0;
        double frameRateMax = 1000.0;
        vDecSample->defaultWidth = std::clamp(fdp.ConsumeIntegral<uint32_t>(), 176u, 4096u);
        vDecSample->defaultHeight = std::clamp(fdp.ConsumeIntegral<uint32_t>(), 176u, 4096u);
        vDecSample->defaultFrameRate = std::clamp(fdp.ConsumeFloatingPoint<double>(), frameRateMin, frameRateMax);
        if (vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263") != AV_ERR_OK) {
            Release();
            return false;
        }
        int32_t ret = vDecSample->ConfigureVideoDecoder();
        if (ret != AV_ERR_OK) {
            Release();
            return false;
        }
        ret = vDecSample->SetVideoDecoderCallback();
        if (ret != AV_ERR_OK) {
            Release();
            return false;
        }
        ret = vDecSample->Start();
        if (ret != AV_ERR_OK) {
            Release();
            return false;
        }
    }
    auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
    OH_AVErrCode ret = vDecSample->InputFuncFUZZ(remaining_data.data(), remaining_data.size());
    if (ret != AV_ERR_OK) {
        vDecSample->Flush();
        vDecSample->Stop();
        vDecSample->Reset();
        Release();
        return false;
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::H263SwdecoderFuzzTest(data, size);
    return 0;
}
