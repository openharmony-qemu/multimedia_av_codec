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
#include "hevcserverdec_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "swdecoderservernormal_fuzzer"
const size_t EXPECT_SIZE = 26;

namespace OHOS {
bool SwdecoderServerNormalFuzzTest(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    VDecServerSample *vDecSample = new VDecServerSample();
    vDecSample->defaultWidth = std::clamp(fdp.ConsumeIntegral<uint32_t>(), 2u, 1920u);
    vDecSample->defaultHeight = std::clamp(fdp.ConsumeIntegral<uint32_t>(), 2u, 1920u);
    vDecSample->defaultFrameRate = std::clamp(fdp.ConsumeIntegral<uint32_t>(), 1u, 30u);
    std::vector<uint32_t> rotations = {0, 90, 180, 270};
    size_t index = fdp.ConsumeIntegralInRange<uint32_t>(0, rotations.size() - 1);
    vDecSample->defaultRotation = rotations[index];
    std::vector<uint32_t> pixelFormats = {1, 2, 3, 4, 5};
    size_t pfIndex = fdp.ConsumeIntegralInRange<uint32_t>(0, pixelFormats.size() - 1);
    vDecSample->defaultPixelFormat = pixelFormats[pfIndex];
    bool needRotation = fdp.ConsumeBool();
    if (needRotation) {
        vDecSample->inpDir = "/data/test/media/720_1280_25_avcc.h265";
    } else {
        vDecSample->inpDir = "/data/test/media/720_1280_25_avcc.hdr.h265";
    }
    vDecSample->isSurfMode = fdp.ConsumeBool();
    auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
    vDecSample->fuzzData = remaining_data.data();
    vDecSample->fuzzSize = remaining_data.size();
    vDecSample->RunVideoServerDecoder();
    vDecSample->WaitForEos();
    delete vDecSample;
    return false;
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SwdecoderServerNormalFuzzTest(data, size);
    return 0;
}
