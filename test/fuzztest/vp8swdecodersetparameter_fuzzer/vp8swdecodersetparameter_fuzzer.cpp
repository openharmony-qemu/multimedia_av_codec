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
#include <fuzzer/FuzzedDataProvider.h>
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "videodec_sample.h"
#define FUZZ_PROJECT_NAME "vp8swdecodersetparameter_fuzzer"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
static VDecFuzzSample *g_vDecSample = nullptr;
constexpr uint32_t DEFAULT_WIDTH = 720;
constexpr uint32_t DEFAULT_HEIGHT = 480;
constexpr double DEFAULT_FRAME_RATE = 30.0;
const size_t EXPECT_SIZE = 64;
namespace OHOS {
bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    if (!g_vDecSample) {
        g_vDecSample = new VDecFuzzSample();
        FuzzedDataProvider fdp(data, size);
        int32_t bitrate = fdp.ConsumeIntegral<int32_t>();
        int32_t maxInputSize = fdp.ConsumeIntegral<int32_t>();
        int32_t width = fdp.ConsumeIntegral<int32_t>();
        int32_t height = fdp.ConsumeIntegral<int32_t>();
        int32_t pixFormat = fdp.ConsumeIntegral<int32_t>();
        int32_t biteMode = fdp.ConsumeIntegral<int32_t>();
        int32_t profile = fdp.ConsumeIntegral<int32_t>();
        int32_t frameInterval = fdp.ConsumeIntegral<int32_t>();
        int32_t rotaTion = fdp.ConsumeIntegral<int32_t>();
        int64_t duraTion = fdp.ConsumeIntegral<int64_t>();
        double  frameRate = fdp.ConsumeFloatingPoint<double>();

        auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
        g_vDecSample->fuzzData = remaining_data.data();
        g_vDecSample->fuzzSize = remaining_data.size();
        g_vDecSample->defaultWidth = DEFAULT_WIDTH;
        g_vDecSample->defaultHeight = DEFAULT_HEIGHT;
        g_vDecSample->defaultFrameRate = DEFAULT_FRAME_RATE;
        g_vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8");
        g_vDecSample->ConfigureVideoDecoder();
        g_vDecSample->SetVideoDecoderCallback();
        g_vDecSample->StartVideoDecoder();

        OH_AVFormat *format = OH_AVFormat_CreateVideoFormat("video/vp8", DEFAULT_WIDTH, DEFAULT_HEIGHT);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, bitrate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, maxInputSize);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, width);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, height);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, pixFormat);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, biteMode);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, profile);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, frameInterval);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, rotaTion);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_DURATION, duraTion);
        OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, frameRate);

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
#ifdef SUPPORT_CODEC_VP8
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
