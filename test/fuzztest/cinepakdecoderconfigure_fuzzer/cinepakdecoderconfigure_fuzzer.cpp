/*
  * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define FUZZ_PROJECT_NAME "cinepakdecoderconfigure_fuzzer"
const size_t EXPECT_SIZE = 6;
namespace OHOS {

bool CinepakdecoderConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    bool result = true;
    FuzzedDataProvider fdp(data, size);
    VDecFuzzSample *vDecSample = new VDecFuzzSample();
    std::string filename = "/data/test/media/cinepak.cinepak";
    vDecSample->inpDir = filename.c_str();
    int32_t lengthMin = 96;
    int32_t lengthMax = 2048;
    int32_t frameRateMin = 1;
    int32_t frameRateMax = 60;
    vDecSample->defaultWidth = std::clamp(fdp.ConsumeIntegral<int32_t>(), lengthMin, lengthMax);
    vDecSample->defaultHeight = std::clamp(fdp.ConsumeIntegral<int32_t>(), lengthMin, lengthMax);
    vDecSample->defaultFrameRate = std::clamp(fdp.ConsumeIntegral<int32_t>(), frameRateMin, frameRateMax);
    std::vector<int32_t> rotations = {0, 90, 180, 270};
    size_t index = fdp.ConsumeIntegralInRange<size_t>(0, rotations.size() - 1);
    vDecSample->defaultRotation = rotations[index];
    std::vector<int32_t> pixelFormats = {1, 2, 3, 4, 5};
    size_t pfIndex = fdp.ConsumeIntegralInRange<size_t>(0, pixelFormats.size() - 1);
    vDecSample->defaultPixelFormat = pixelFormats[pfIndex];
    size_t maxSize = std::numeric_limits<size_t>::max();
    vDecSample->randomName = fdp.ConsumeRandomLengthString(maxSize);
    vDecSample->randomMime = fdp.ConsumeRandomLengthString(maxSize);
    if (vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.CINEPAK") != AV_ERR_OK) {
        delete vDecSample;
        return false;
    }
    if (vDecSample->ConfigureVideoDecoder() != AV_ERR_OK) {
        delete vDecSample;
        return false;
    }
    if (vDecSample->SetVideoDecoderCallback() != AV_ERR_OK) {
        delete vDecSample;
        return false;
    }
    if (vDecSample->StartVideoDecoder() != AV_ERR_OK) {
        delete vDecSample;
        return false;
    }
    vDecSample->WaitForEOS();
    vDecSample->Release();
    delete vDecSample;
    return result;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    (void)data;
    (void)size;
    OHOS::CinepakdecoderConfigureFuzzTest(data, size);
    return 0;
}