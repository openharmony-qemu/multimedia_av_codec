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
#include "native_avformat.h"
#include "videoenc_sample.h"
#include "native_avcapability.h"
#include <fuzzer/FuzzedDataProvider.h>
#define FUZZ_PROJECT_NAME "encodersetparameter_fuzzer"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
static VEncFuzzSample *vEncSample = nullptr;
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
constexpr double DEFAULT_FRAME_RATE = 30.0;
namespace OHOS {
void SetRandomValue(const uint8_t *data, size_t size)
{
    OH_AVFormat *format = OH_AVFormat_CreateVideoFormat("video/avc", DEFAULT_WIDTH, DEFAULT_HEIGHT);
    FuzzedDataProvider fdp(data, size);
    int intData0 = fdp.ConsumeIntegral<int32_t>();
    int intData1 = fdp.ConsumeIntegral<int32_t>();
    int intData2 = fdp.ConsumeIntegral<int32_t>();
    int intData3 = fdp.ConsumeIntegral<int32_t>();
    int intData4 = fdp.ConsumeIntegral<int32_t>();
    int intData5 = fdp.ConsumeIntegral<int32_t>();
    int intData6 = fdp.ConsumeIntegral<int32_t>();
    int intData7 = fdp.ConsumeIntegral<int32_t>();
    int intData8 = fdp.ConsumeIntegral<int32_t>();
    int longData = fdp.ConsumeIntegral<int64_t>();
    double doubleData = fdp.ConsumeFloatingPoint<double>();

    int intData9 = fdp.ConsumeIntegral<int32_t>();
    int longData1 = fdp.ConsumeIntegral<int64_t>();
    double doubleData1 = fdp.ConsumeFloatingPoint<double>();
    std::string randomIntKey = fdp.ConsumeRandomLengthString();
    std::string randomLongKey = fdp.ConsumeRandomLengthString();
    std::string randomDoubleKey = fdp.ConsumeRandomLengthString();

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, intData0);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, intData1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, intData2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, intData3);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, intData4);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, intData5);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, intData6);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, intData7);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, intData8);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_DURATION, longData);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, doubleData);

    OH_AVFormat_SetIntValue(format, randomIntKey.c_str(), intData9);
    OH_AVFormat_SetLongValue(format, randomLongKey.c_str(), longData1);
    OH_AVFormat_SetDoubleValue(format, randomDoubleKey.c_str(), doubleData1);

    vEncSample->SetParameter(format);
    OH_AVFormat_Destroy(format);
}
bool EncoderSetparameterFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    if (!vEncSample) {
        vEncSample = new VEncFuzzSample();
        vEncSample->defaultWidth = DEFAULT_WIDTH;
        vEncSample->defaultHeight = DEFAULT_HEIGHT;
        vEncSample->defaultFrameRate = DEFAULT_FRAME_RATE;
        vEncSample->fuzzMode = true;
        OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory("video/avc", true, HARDWARE);
        string tmpCodecName = OH_AVCapability_GetName(cap);
        vEncSample->CreateVideoEncoder(tmpCodecName.c_str());
        vEncSample->SetVideoEncoderCallback();
        vEncSample->ConfigureVideoEncoder();
        vEncSample->Start();
    }
    SetRandomValue(data, size);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::EncoderSetparameterFuzzTest(data, size);
    return 0;
}
