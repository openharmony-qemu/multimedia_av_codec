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
const int64_t EXPECT_SIZE = 6;
const size_t WIDTH_SIZE = 1;
const size_t HEIGHT_SIZE = 2;
const size_t FRAME_RATE_SIZE = 3;
const size_t ROTATION_SIZE = 4;
const size_t PIXELFORMAT_SIZE = 5;
namespace OHOS {
bool HevcSwdecoderConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    bool result = false;
    VDecServerSample *vDecSample = new VDecServerSample();
    vDecSample->kWidth = data[size - WIDTH_SIZE];
    vDecSample->kHeight = data[size - HEIGHT_SIZE];
    vDecSample->kFormat = data[size - PIXELFORMAT_SIZE];
    vDecSample->kRotation = data[size - ROTATION_SIZE];
    vDecSample->kFormatRate = data[size - FRAME_RATE_SIZE];
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
