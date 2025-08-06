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
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "videodec_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "swdecoderconfigure_fuzzer"
const size_t EXPECT_SIZE = 6;
namespace OHOS {
bool SwdecoderConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    bool result = false;
    FuzzedDataProvider fdp(data, size);
    VDecFuzzSample *vDecSample = new VDecFuzzSample();
    vDecSample->inpDir = "/data/test/media/1280_720_30_10Mb.h264";
    vDecSample->defaultWidth = std::clamp(fdp.ConsumeIntegral<uint32_t>(), 176u, 4096u);
    vDecSample->defaultHeight = std::clamp(fdp.ConsumeIntegral<uint32_t>(), 176u, 4096u);
    vDecSample->defaultFrameRate = std::clamp(fdp.ConsumeIntegral<uint32_t>(), 1u, 1000u);
    std::vector<uint32_t> rotations = {0, 90, 180, 270};
    size_t index = fdp.ConsumeIntegralInRange<uint32_t>(0, rotations.size() - 1);
    vDecSample->defaultRotation = rotations[index];
    std::vector<uint32_t> pixelFormats = {1, 2, 3, 4, 5};
    size_t pfIndex = fdp.ConsumeIntegralInRange<uint32_t>(0, pixelFormats.size() - 1);
    vDecSample->defaultPixelFormat = pixelFormats[pfIndex];
    size_t maxSize = std::numeric_limits<size_t>::max();
    vDecSample->randomName = fdp.ConsumeRandomLengthString(maxSize);
    vDecSample->randomMime = fdp.ConsumeRandomLengthString(maxSize);
    if (vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC") != 0) {
        delete vDecSample;
        vDecSample = nullptr;
        return false;
    }
    if (vDecSample->ConfigureVideoDecoder() != 0) {
        delete vDecSample;
        vDecSample = nullptr;
        return false;        
    }
    if (vDecSample->SetVideoDecoderCallback() != 0) {
        delete vDecSample;
        vDecSample = nullptr;
        return false;         
    }
    if (vDecSample->StartVideoDecoder() != 0) {
        delete vDecSample;
        vDecSample = nullptr;
        return false;         
    }
    vDecSample->WaitForEOS();
    vDecSample->Release();
    delete vDecSample;
    vDecSample = nullptr;
    return result;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SwdecoderConfigureFuzzTest(data, size);
    return 0;
}
