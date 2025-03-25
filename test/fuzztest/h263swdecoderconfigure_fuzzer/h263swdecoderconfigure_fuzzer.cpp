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
#define FUZZ_PROJECT_NAME "h263swdecoderconfigure_fuzzer"
const size_t EXPECT_SIZE = 6;
const size_t WIDTH_SIZE = 1;
const size_t HEIGHT_SIZE = 2;
const size_t FRAME_RATE_SIZE = 3;
const size_t ROTATION_SIZE = 4;
const size_t PIXELFORMAT_SIZE = 5;
namespace OHOS {
bool H263SwdecoderConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    bool result = false;
    FuzzedDataProvider fdp(data, size);
    VDecFuzzSample *vDecSample = new VDecFuzzSample();
    vDecSample->inpDir = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->defaultWidth = data[size - WIDTH_SIZE];
    vDecSample->defaultHeight = data[size - HEIGHT_SIZE];
    vDecSample->defaultFrameRate = data[size - FRAME_RATE_SIZE];
    vDecSample->defaultRotation = data[size - ROTATION_SIZE];
    vDecSample->defaultPixelFormat = data[size - PIXELFORMAT_SIZE];
    size_t maxSize = std::numeric_limits<size_t>::max();
    vDecSample->randomName = fdp.ConsumeRandomLengthString(maxSize);
    vDecSample->randomMime = fdp.ConsumeRandomLengthString(maxSize);
    vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263");
    vDecSample->ConfigureVideoDecoder();
    vDecSample->SetVideoDecoderCallback();
    vDecSample->StartVideoDecoder();
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
    OHOS::H263SwdecoderConfigureFuzzTest(data, size);
    return 0;
}
