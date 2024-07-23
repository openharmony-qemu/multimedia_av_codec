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
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "encoderapi11_fuzzer"

void RunNormalEncoder()
{
    auto vEncSample = make_unique<VEncAPI11FuzzSample>();
    vEncSample->CreateVideoEncoder();
    vEncSample->SetVideoEncoderCallback();
    vEncSample->ConfigureVideoEncoder();
    vEncSample->StartVideoEncoder();
    vEncSample->WaitForEOS();

    auto vEncSampleSurf = make_unique<VEncAPI11FuzzSample>();
    vEncSample->surfInput = true;
    vEncSampleSurf->CreateVideoEncoder();
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
    int32_t data2 = *reinterpret_cast<const int32_t *>(data);
    VEncAPI11FuzzSample *vEncSample = new VEncAPI11FuzzSample();

    vEncSample->CreateVideoEncoder();
    vEncSample->SetVideoEncoderCallback();
    vEncSample->fuzzMode = true;
    vEncSample->ConfigureVideoEncoderFuzz(data2);
    vEncSample->StartVideoEncoder();
    vEncSample->SetParameter(data2);
    vEncSample->WaitForEOS();
    delete vEncSample;

    vEncSample = new VEncAPI11FuzzSample();
    vEncSample->CreateVideoEncoder();
    vEncSample->SetVideoEncoderCallback();
    vEncSample->surfInput = true;
    vEncSample->ConfigureVideoEncoderFuzz(data2);
    vEncSample->StartVideoEncoder();
    vEncSample->SetParameter(data2);
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
