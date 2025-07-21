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

namespace OHOS {
bool SwdecoderConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    bool result = false;
    FuzzedDataProvider fdp(data, size);
    int consumeWidth = fdp.ConsumeIntegral<int32_t>();
    int consumeHeight = fdp.ConsumeIntegral<int32_t>();
    int consumeFrameRate = fdp.ConsumeIntegral<int32_t>();
    int consumeRotation  = fdp.ConsumeIntegral<int32_t>();
    int consumePixelFormat = fdp.ConsumeIntegral<int32_t>();
    VDecFuzzSample *vDecSample = new VDecFuzzSample();
    vDecSample->inpDir = "/data/test/media/1280_720_30.m4v";
    vDecSample->defaultWidth = consumeWidth;
    vDecSample->defaultHeight = consumeHeight;
    vDecSample->defaultFrameRate = consumeFrameRate;
    vDecSample->defaultRotation = consumeRotation ;
    vDecSample->defaultPixelFormat = consumePixelFormat;
    vDecSample->enbleBlankFrame = fdp.ConsumeIntegral<int>();
    vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.MPEG4");
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
