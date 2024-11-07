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
#include "hevcserverdec_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "hevcswdecoderconfigure_fuzzer"

namespace OHOS {
bool HevcSwdecoderConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    bool result = false;
    int32_t data_ = *reinterpret_cast<const int32_t *>(data);
    VDecServerSample *vDecSample = new VDecServerSample();
    vDecSample->kWidth = data_;
    vDecSample->kHeight = data_;
    vDecSample->kFormat = data_;
    vDecSample->kRotation = data_;
    vDecSample->kFormatRate = data_;
    vDecSample->RunVideoServerDecoder();
    vDecSample->WaitForEos();

    delete vDecSample;
    return result;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::HevcSwdecoderConfigureFuzzTest(data, size);
    return 0;
}
