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
#include "videodec_sample.h"
#define FUZZ_PROJECT_NAME "swdecodersetparameter_fuzzer"
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
    if (size < sizeof(int64_t)) {
        return false;
    }
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
    OH_AVFormat *format = OH_AVFormat_CreateVideoFormat("video/avc", DEFAULT_WIDTH, DEFAULT_HEIGHT);
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    double doubleData = *reinterpret_cast<const double *>(data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_DURATION, longData);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, doubleData);

    vDecSample->SetParameter(format);
    OH_AVFormat_Destroy(format);
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
