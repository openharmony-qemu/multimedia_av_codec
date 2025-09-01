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
#include <fuzzer/FuzzedDataProvider.h>
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "videodec_api11_sample.h"

#define FUZZ_PROJECT_NAME "swdecoderresource_fuzzer"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
const size_t EXPECT_SIZE = 20;
static VDecFuzzSample *g_vDecSample = nullptr;

namespace OHOS {
bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
     if (size < EXPECT_SIZE) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    if (!g_vDecSample) {
        g_vDecSample = new VDecFuzzSample();
        g_vDecSample->defaultWidth = fdp.ConsumeIntegral<uint32_t>();
        g_vDecSample->defaultHeight = fdp.ConsumeIntegral<uint32_t>();
        g_vDecSample->defaultFrameRate = fdp.ConsumeIntegral<uint32_t>();
        g_vDecSample->enbleBlankFrame = fdp.ConsumeIntegral<int>();
        g_vDecSample->CreateVideoDecoder();
        g_vDecSample->ConfigureVideoDecoder();
        g_vDecSample->SetVideoDecoderCallback();
        g_vDecSample->Start();
    }
    auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
    OH_AVErrCode ret = g_vDecSample->InputFuncFUZZ(remaining_data.data(), remaining_data.size());
    if (ret != AV_ERR_OK) {
        g_vDecSample->Flush();
        g_vDecSample->Stop();
        g_vDecSample->Reset();
        g_vDecSample->Release();
        delete g_vDecSample;
        g_vDecSample = nullptr;
        return false;
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}
