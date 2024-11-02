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
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "videodec_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "hevcswdecoderconfigure_fuzzer"
static VDecFuzzSample *vDecSample = nullptr;

namespace OHOS {
bool HevcSwdecoderConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    bool result = false;
    int32_t data_ = *reinterpret_cast<const int32_t *>(data);
    vDecSample = new VDecFuzzSample();
    vDecSample->inpDir = "/data/test/media/1920_1080_30.h265";
    vDecSample->defaultWidth = data_;
    vDecSample->defaultHeight = data_;
    vDecSample->defaultFrameRate = data_;
    vDecSample->defaultRotation = data_;
    vDecSample->defaultPixelFormat = data_;
    if (vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.HEVC") == AV_ERR_OK) {
        vDecSample->ConfigureVideoDecoder();
        vDecSample->SetVideoDecoderCallback();
        vDecSample->StartVideoDecoder();
        vDecSample->WaitForEOS();
        vDecSample->Release();
    }
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
