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
#include "av1serverdec_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "av1swdecoderconfigure_fuzzer"
const size_t EXPECT_SIZE = 24;
namespace OHOS {
bool Av1SwdecoderConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    VDecServerSample *vDecSample = new VDecServerSample();
    vDecSample->kWidth = fdp.ConsumeIntegral<int32_t>();
    vDecSample->kHeight = fdp.ConsumeIntegral<int32_t>();
    vDecSample->kFormat = fdp.ConsumeIntegral<int32_t>();
    vDecSample->kRotation = fdp.ConsumeIntegral<int32_t>();
    vDecSample->kFormatRate = fdp.ConsumeIntegral<int32_t>();
    auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
    vDecSample->fuzzData = remaining_data.data();
    vDecSample->fuzzSize = remaining_data.size();
    vDecSample->RunVideoServerDecoder();
    vDecSample->WaitForEos();

    delete vDecSample;
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Av1SwdecoderConfigureFuzzTest(data, size);
    return 0;
}