/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "videodec_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "fcodecsurfaceconfigure_fuzzer"
const size_t EXPECT_SIZE = 6;

namespace OHOS {
bool FCodecSurfaceConfigureFuzzer(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    if (size < EXPECT_SIZE) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    uint32_t decision = fdp.ConsumeIntegral<uint32_t>();
    VDecFuzzSample *vDecSample = new VDecFuzzSample();
    if (decision % 2 == 0) {
        vDecSample->isSurfaceMode = true;
    } else {
        vDecSample->isSurfaceMode = false;
    }
    vDecSample->RunVideoDec();
    delete vDecSample;
    return false;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FCodecSurfaceConfigureFuzzer(data, size);
    return 0;
}