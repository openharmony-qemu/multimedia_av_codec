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

#define FUZZ_PROJECT_NAME "wvc1swdecodersetparameter_fuzzer"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

constexpr uint32_t DEFAULT_WIDTH = 720;
constexpr uint32_t DEFAULT_HEIGHT = 480;
constexpr double DEFAULT_FRAME_RATE = 30.0;

void Release(VDecFuzzSample *&sample)
{
    if (sample != nullptr) {
        sample->Release();
        delete sample;
        sample = nullptr;
    }
}

static OH_AVFormat *SetFormatValue(FuzzedDataProvider &fdp)
{
    OH_AVFormat *format = OH_AVFormat_CreateVideoFormat("video/wvc1", DEFAULT_WIDTH, DEFAULT_HEIGHT);
    if (format == nullptr) {
        return nullptr;
    }

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
    return format;
}

namespace OHOS {
bool WVc1SwdecoderFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    VDecFuzzSample *vDecSample = new VDecFuzzSample();
    
    vDecSample->defaultWidth = fdp.ConsumeIntegral<int32_t>();
    vDecSample->defaultHeight = fdp.ConsumeIntegral<int32_t>();
    vDecSample->defaultFrameRate = fdp.ConsumeFloatingPoint<double>();
    vDecSample->isSurfMode = fdp.ConsumeBool();

    if (vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WVC1") != AV_ERR_OK ||
        vDecSample->ConfigureVideoDecoder() != AV_ERR_OK ||
        vDecSample->SetVideoDecoderCallback() != AV_ERR_OK ||
        vDecSample->Start() != AV_ERR_OK) {
        Release(vDecSample);
        return false;
    }

    OH_AVFormat *format = SetFormatValue(fdp);
    if (format != nullptr) {
        vDecSample->SetParameter(format);
        OH_AVFormat_Destroy(format);
    }
    Release(vDecSample);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
#ifdef SUPPORT_CODEC_VC1
    OHOS::WVc1SwdecoderFuzzTest(data, size);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
