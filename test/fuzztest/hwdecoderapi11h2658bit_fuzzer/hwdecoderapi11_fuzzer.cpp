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
#include "videodec_api11_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
#include <fstream>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "hwdecoderapi11_fuzzer"

static VDecApi11FuzzSample *g_vDecSample = nullptr;
constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
constexpr double DEFAULT_FRAME_RATE = 30.0;
constexpr uint32_t SPS_SIZE = 0x19;
constexpr uint32_t PPS_SIZE = 0x05;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr uint8_t SPS[SPS_SIZE + START_CODE_SIZE] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, 0xAC,
                                                     0xB4, 0x03, 0xC0, 0x11, 0x3F, 0x2E, 0x02, 0x20, 0x00,
                                                     0x00, 0x03, 0x00, 0x20, 0x00, 0x00, 0x07, 0x81, 0xE3,
                                                     0x06, 0x54};
constexpr uint8_t PPS[PPS_SIZE + START_CODE_SIZE] = {0x00, 0x00, 0x00, 0x01, 0x68, 0xEF, 0x0F, 0x2C, 0x8B};
bool g_isSurfMode = true;

namespace OHOS {
void RunNormalDecoder()
{
    VDecApi11FuzzSample *vDecSample = new VDecApi11FuzzSample();
    vDecSample->defaultWidth = DEFAULT_WIDTH;
    vDecSample->defaultHeight = DEFAULT_HEIGHT;
    vDecSample->defaultFrameRate = DEFAULT_FRAME_RATE;
    int32_t ret = vDecSample->CreateVideoDecoder();
    if (ret != AV_ERR_OK) {
        delete vDecSample;
        vDecSample = nullptr;
        return;
    }
    vDecSample->ConfigureVideoDecoder();
    vDecSample->SetVideoDecoderCallback();
    vDecSample->StartVideoDecoder();
    vDecSample->WaitForEOS();
    delete vDecSample;

    vDecSample = new VDecApi11FuzzSample();
    vDecSample->isSurfMode = true;
    vDecSample->defaultWidth = DEFAULT_WIDTH;
    vDecSample->defaultHeight = DEFAULT_HEIGHT;
    vDecSample->defaultFrameRate = DEFAULT_FRAME_RATE;
    ret = vDecSample->CreateVideoDecoder();
    if (ret != AV_ERR_OK) {
        delete vDecSample;
        vDecSample = nullptr;
        return;
    }
    vDecSample->ConfigureVideoDecoder();
    vDecSample->SetVideoDecoderCallback();
    vDecSample->StartVideoDecoder();
    vDecSample->WaitForEOS();
    delete vDecSample;
}

bool ReleaseSample()
{
    delete g_vDecSample;
    g_vDecSample = nullptr;
    return true;
}

bool g_needRunNormalDecoder = true;
bool HwdecoderApi11FuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    if (g_needRunNormalDecoder) {
        g_needRunNormalDecoder = false;
        RunNormalDecoder();
    }
    FuzzedDataProvider fdp(data, size);
    int data0 = fdp.ConsumeIntegral<int32_t>();
    int data1 = fdp.ConsumeIntegral<int32_t>();
    if (!g_vDecSample) {
        g_vDecSample = new VDecApi11FuzzSample();
        g_vDecSample->defaultWidth = DEFAULT_WIDTH;
        g_vDecSample->defaultHeight = DEFAULT_HEIGHT;
        g_vDecSample->defaultFrameRate = DEFAULT_FRAME_RATE;
        g_vDecSample->renderTimestampNs = fdp.ConsumeIntegral<int64_t>();
        g_vDecSample->isRenderAttime = fdp.ConsumeBool();
        auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
        int32_t ret = g_vDecSample->CreateVideoDecoder();
        if (ret != AV_ERR_OK) {
            return ReleaseSample();
        }
        if (g_vDecSample->ConfigureVideoDecoder() != AV_ERR_OK) {
            return ReleaseSample();
        }
        if (g_vDecSample->SetVideoDecoderCallback() != AV_ERR_OK) {
            return ReleaseSample();
        }
        if (g_vDecSample->Start() != AV_ERR_OK) {
            return ReleaseSample();
        }
        g_vDecSample->InputFuncFUZZ(SPS, SPS_SIZE + START_CODE_SIZE);
        g_vDecSample->InputFuncFUZZ(PPS, PPS_SIZE + START_CODE_SIZE);
        g_vDecSample->InputFuncFUZZ(remaining_data.data(), remaining_data.size());
        g_vDecSample->SetParameter(data0, data1);
        g_vDecSample->Flush();
        g_vDecSample->Stop();
        g_vDecSample->Reset();
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
    OHOS::HwdecoderApi11FuzzTest(data, size);
    return 0;
}
