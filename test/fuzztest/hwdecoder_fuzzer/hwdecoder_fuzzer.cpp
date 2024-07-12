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
#define FUZZ_PROJECT_NAME "hwdecoder_fuzzer"

static VDecFuzzSample *g_vDecSample = nullptr;
constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
constexpr uint32_t SPS_SIZE = 0x19;
constexpr uint32_t PPS_SIZE = 0x05;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr uint8_t SPS[SPS_SIZE + START_CODE_SIZE] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, 0xAC,
                                                     0xB4, 0x03, 0xC0, 0x11, 0x3F, 0x2E, 0x02, 0x20, 0x00,
                                                     0x00, 0x03, 0x00, 0x20, 0x00, 0x00, 0x07, 0x81, 0xE3,
                                                     0x06, 0x54};
constexpr uint8_t PPS[PPS_SIZE + START_CODE_SIZE] = {0x00, 0x00, 0x00, 0x01, 0x68, 0xEF, 0x0F, 0x2C, 0x8B};
bool g_isSurfMode = true;

void RunNormalDecoder()
{
    VDecFuzzSample *vDecSample = new VDecFuzzSample();
    vDecSample->defaultWidth = DEFAULT_WIDTH;
    vDecSample->defaultHeight = DEFAULT_HEIGHT;
    vDecSample->CreateVideoDecoder();
    vDecSample->ConfigureVideoDecoder();
    vDecSample->SetVideoDecoderCallback();
    vDecSample->StartVideoDecoder();
    vDecSample->WaitForEOS();
    delete vDecSample;

    vDecSample = new VDecFuzzSample();
    vDecSample->isSurfMode = true;
    vDecSample->defaultWidth = DEFAULT_WIDTH;
    vDecSample->defaultHeight = DEFAULT_HEIGHT;
    vDecSample->CreateVideoDecoder();
    vDecSample->ConfigureVideoDecoder();
    vDecSample->SetVideoDecoderCallback();
    vDecSample->StartVideoDecoder();
    vDecSample->WaitForEOS();
    delete vDecSample;
}

bool g_needRunNormalDecoder = true;
namespace OHOS {
bool HwdecoderFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    if (g_needRunNormalDecoder) {
        g_needRunNormalDecoder = false;
        RunNormalDecoder();
    }
    int32_t data_ = *reinterpret_cast<const int32_t *>(data);
    if (!g_vDecSample) {
        g_vDecSample = new VDecFuzzSample();
        g_vDecSample->defaultWidth = DEFAULT_WIDTH;
        g_vDecSample->defaultHeight = DEFAULT_HEIGHT;
        g_vDecSample->CreateVideoDecoder();
        g_vDecSample->ConfigureVideoDecoder();
        g_vDecSample->SetVideoDecoderCallback();
        g_vDecSample->Start();
        g_vDecSample->InputFuncFUZZ(SPS, SPS_SIZE + START_CODE_SIZE);
        g_vDecSample->InputFuncFUZZ(PPS, PPS_SIZE + START_CODE_SIZE);
    }
    OH_AVErrCode ret = g_vDecSample->InputFuncFUZZ(data, size);
    g_vDecSample->SetParameter(data_);
    if (ret == AV_ERR_NO_MEMORY) {
        g_vDecSample->Flush();
        g_vDecSample->Stop();
        g_vDecSample->Reset();
        delete g_vDecSample;
        g_vDecSample = nullptr;
        return false;
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::HwdecoderFuzzTest(data, size);
    return 0;
}
