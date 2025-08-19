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
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "videodec_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
#define FUZZ_PROJECT_NAME "hevcswdecodersetparameter_fuzzer"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
static VDecFuzzSample *g_vDecSample = nullptr;
constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
constexpr double DEFAULT_FRAME_RATE = 30.0;
namespace OHOS {
bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    if (!g_vDecSample) {
        g_vDecSample = new VDecFuzzSample();
        g_vDecSample->fuzzData = data;
        g_vDecSample->fuzzSize = size;
        g_vDecSample->defaultWidth = DEFAULT_WIDTH;
        g_vDecSample->defaultHeight = DEFAULT_HEIGHT;
        g_vDecSample->defaultFrameRate = DEFAULT_FRAME_RATE;
        g_vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.HEVC");
        g_vDecSample->ConfigureVideoDecoder();
        g_vDecSample->SetVideoDecoderCallback();
        g_vDecSample->StartVideoDecoder();

        OH_AVFormat *format = OH_AVFormat_CreateVideoFormat("video/hevc", DEFAULT_WIDTH, DEFAULT_HEIGHT);
        FuzzedDataProvider fdp(data, size);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, fdp.ConsumeIntegral<int32_t>());
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_DURATION, fdp.ConsumeIntegral<int64_t>());
        OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, fdp.ConsumeFloatingPoint<double>());

        g_vDecSample->SetParameter(format);
        OH_AVFormat_Destroy(format);

        g_vDecSample->WaitForEOS();
        delete g_vDecSample;
        g_vDecSample = nullptr;
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
