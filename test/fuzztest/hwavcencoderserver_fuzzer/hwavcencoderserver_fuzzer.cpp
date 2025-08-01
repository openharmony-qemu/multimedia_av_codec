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
#include "serverenc_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "hwavcencoderserver_fuzzer"

namespace OHOS {
bool HwavcencoderServerFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    VEncServerSample *vEncSample = new VEncServerSample();
    vEncSample->fuzzData = data;
    vEncSample->fuzzSize = size;
    int32_t  lengthMin = 176;
    int32_t  lengthMax = 4096;
    int32_t  frameRateMin = 1;
    int32_t  frameRateMax = 1000;
    vEncSample->defaultWidth = std::clamp(fdp.ConsumeIntegral<int32_t>(), lengthMin, lengthMax);
    vEncSample->defaultHeight = std::clamp(fdp.ConsumeIntegral<int32_t>(), lengthMin, lengthMax);
    vEncSample->defaultFrameRate = std::clamp(fdp.ConsumeIntegral<int32_t>(), frameRateMin, frameRateMax);
    std::vector<int32_t> pixelFormats = { 1, 2, 3, 4, 5 };
    size_t pfIndex = fdp.ConsumeIntegralInRange<size_t>(0, pixelFormats.size() - 1);
    vEncSample->defaultPixelFormat = pixelFormats[pfIndex];
    vEncSample->RunVideoServerDecoder();
    vEncSample->WaitForEos();
    delete vEncSample;
    return false;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::HwavcencoderServerFuzzTest(data, size);
    return 0;
}
