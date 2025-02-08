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

#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"
#include "videoenc_api11_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "encoderapi11_fuzzer"

void RunNormalEncoder()
{
    auto vEncSample = make_unique<VEncAPI11FuzzSample>();
    int32_t ret = vEncSample->CreateVideoEncoder();
    if (ret != 0) {
        return;
    }
    vEncSample->SetVideoEncoderCallback();
    vEncSample->ConfigureVideoEncoder();
    vEncSample->StartVideoEncoder();
    vEncSample->WaitForEOS();

    auto vEncSampleSurf = make_unique<VEncAPI11FuzzSample>();
    vEncSample->surfInput = true;
    ret = vEncSampleSurf->CreateVideoEncoder();
    if (ret != 0) {
        return;
    }
    vEncSampleSurf->SetVideoEncoderCallback();
    vEncSampleSurf->ConfigureVideoEncoder();
    vEncSampleSurf->StartVideoEncoder();
    vEncSampleSurf->WaitForEOS();
}

bool g_needRunNormalEncoder = true;
namespace OHOS {
bool EncoderAPI11FuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    if (g_needRunNormalEncoder) {
        g_needRunNormalEncoder = false;
        RunNormalEncoder();
    }
    bool result = false;
    FuzzedDataProvider fdp(data, size);
    int data0 = fdp.ConsumeIntegral<int32_t>();
    int data1 = fdp.ConsumeIntegral<int32_t>();
    bool data2 = fdp.ConsumeBool();
    VEncAPI11FuzzSample *vEncSample = new VEncAPI11FuzzSample();
    vEncSample->fuzzData = data;
    vEncSample->fuzzSize = size;
    vEncSample->surfInput = data2;
    vEncSample->enableRepeat = fdp.ConsumeBool();
    vEncSample->setMaxCount = fdp.ConsumeBool();
    vEncSample->defaultRangeFlag = fdp.ConsumeIntegral<uint32_t>();
    vEncSample->DEFAULT_COLOR_PRIMARIES = fdp.ConsumeIntegral<uint32_t>();
    vEncSample->DEFAULT_TRANSFER_CHARACTERISTICS = fdp.ConsumeIntegral<uint32_t>();
    vEncSample->DEFAULT_MATRIX_COEFFICIENTS = fdp.ConsumeIntegral<uint32_t>();
    vEncSample->defaultKeyFrameInterval = fdp.ConsumeIntegral<uint32_t>();
    vEncSample->DEFAULT_BITRATE_MODE = fdp.ConsumeIntegral<uint32_t>();
    vEncSample->defaultBitrate = fdp.ConsumeIntegral<uint32_t>();
    vEncSample->defaultQuality = fdp.ConsumeIntegral<uint32_t>();
    vEncSample->defaultFrameAfter = fdp.ConsumeIntegral<int32_t>();
    vEncSample->defaultMaxCount = fdp.ConsumeIntegral<int32_t>();
    int32_t ret = vEncSample->CreateVideoEncoder();
    if (ret != 0) {
        delete vEncSample;
        return true;
    }
    vEncSample->SetVideoEncoderCallback();
    vEncSample->ConfigureVideoEncoderFuzz(data0);
    vEncSample->StartVideoEncoder();
    vEncSample->SetParameter(data1);
    vEncSample->WaitForEOS();
    delete vEncSample;
    return result;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::EncoderAPI11FuzzTest(data, size);
    return 0;
}
