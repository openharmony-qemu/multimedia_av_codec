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
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "videodec_sample.h"

#define FUZZ_PROJECT_NAME "swdecoderresource_fuzzer"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

static VDecFuzzSample *vDecSample = nullptr;
constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
constexpr double DEFAULT_FRAME_RATE = 30.0;

namespace OHOS {
bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (!vDecSample) {
        vDecSample = new VDecFuzzSample();
        vDecSample->defaultWidth = DEFAULT_WIDTH;
        vDecSample->defaultHeight = DEFAULT_HEIGHT;
        vDecSample->defaultFrameRate = DEFAULT_FRAME_RATE;
        vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
        vDecSample->ConfigureVideoDecoder();
        vDecSample->SetVideoDecoderCallback();
        vDecSample->Start();
    }
    OH_AVErrCode ret = vDecSample->InputFuncFUZZ(data, size);
    if (ret != AV_ERR_OK) {
        vDecSample->Release();
        delete vDecSample;
        vDecSample = nullptr;
        return false;
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}
