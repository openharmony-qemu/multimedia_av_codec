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

#define FUZZ_PROJECT_NAME "wvc1swdecoderresource_fuzzer"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

static VDecFuzzSample *g_vDecSample = nullptr;
namespace OHOS {

void Release()
{
    g_vDecSample->Release();
    delete g_vDecSample;
    g_vDecSample = nullptr;
}

bool WVc1SwdecoderFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (!g_vDecSample) {
        g_vDecSample = new VDecFuzzSample();
        double frameRateMin = 1.0;
        double frameRateMax = 60.0;

        g_vDecSample->defaultWidth = std::clamp(fdp.ConsumeIntegral<uint32_t>(), 96u, 2048u);
        g_vDecSample->defaultHeight = std::clamp(fdp.ConsumeIntegral<uint32_t>(), 96u, 2048u);
        g_vDecSample->defaultFrameRate = std::clamp(fdp.ConsumeFloatingPoint<double>(), frameRateMin, frameRateMax);

        if (g_vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WVC1") != AV_ERR_OK) {
            Release();
            return false;
        }

        int32_t ret = g_vDecSample->ConfigureVideoDecoder();
        if (ret != AV_ERR_OK) {
            Release();
            return false;
        }
        ret = g_vDecSample->SetVideoDecoderCallback();
        if (ret != AV_ERR_OK) {
            Release();
            return false;
        }
        ret = g_vDecSample->Start();
        if (ret != AV_ERR_OK) {
            Release();
            return false;
        }
    }
    auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
    int ret = g_vDecSample->InputFuncFUZZ(remaining_data.data(), remaining_data.size());
    if (ret != AV_ERR_OK) {
        g_vDecSample->Flush();
        g_vDecSample->Stop();
        g_vDecSample->Reset();
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
#ifdef SUPPORT_CODEC_VC1
    OHOS::WVc1SwdecoderFuzzTest(data, size);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}