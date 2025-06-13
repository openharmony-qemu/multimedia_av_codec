/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
    vDecSample->defaultWidth = fdp.ConsumeIntegral<uint32_t>();
    vDecSample->defaultHeight = fdp.ConsumeIntegral<uint32_t>();
    vDecSample->defaultFrameRate = fdp.ConsumeIntegral<uint32_t>();
    vDecSample->defaultRotation = fdp.ConsumeIntegral<uint32_t>();
    vDecSample->defaultPixelFormat = fdp.ConsumeIntegral<uint32_t>();
    size_t maxSize = std::numeric_limits<size_t>::max();
    vDecSample->randomName = fdp.ConsumeRandomLengthString(maxSize);
    vDecSample->randomMime = fdp.ConsumeRandomLengthString(maxSize);
    vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
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
    OHOS::SwdecoderConfigureFuzzTest(data, size);
    return 0;
}
