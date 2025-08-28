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

#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"
#include "videoenc_inner_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "encoderinner_fuzzer"

VEncNdkInnerFuzzSample *g_vEncSample = nullptr;

bool ReleaseSample()
{
    delete g_vEncSample;
    g_vEncSample = nullptr;
    return false;
}

namespace OHOS {
BufferRequestConfig bufferConfig = {
    .width = 128,
    .height = 72,
    .strideAlignment = 0x8,
    .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
    .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
    .timeout = 0,
};

bool EncoderInnerFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    g_vEncSample = new VEncNdkInnerFuzzSample();
    string gCodecMime = "video/avc";
    string gCodecName = "";
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(gCodecMime.c_str(), true, HARDWARE);
    if (cap == nullptr) {
        return ReleaseSample();
    }
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (strcmp(tmpCodecName, "") == 0) {
        return ReleaseSample();
    }
    gCodecName = tmpCodecName;
    if (!g_vEncSample->GetWaterMarkCapability(gCodecMime)) {
        return ReleaseSample();
    }
    g_vEncSample->inpDir = "/data/test/corpus/1280_720_nv.yuv";
    g_vEncSample->surfaceInput = true;
    g_vEncSample->enableWaterMark = true;
    constexpr uint32_t doubleValue = 2;
    g_vEncSample->videoCoordinateX = (g_vEncSample->defaultWidth - bufferConfig.width) / doubleValue;
    g_vEncSample->videoCoordinateY = (g_vEncSample->defaultHeight - bufferConfig.height) / doubleValue;
    g_vEncSample->videoCoordinateWidth = bufferConfig.width;
    g_vEncSample->videoCoordinateHeight = bufferConfig.height;
    FuzzedDataProvider fdp(data, size);
    auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
    if (g_vEncSample->CreateByName(gCodecName) != AV_ERR_OK) {
        return ReleaseSample();
    }
    if (g_vEncSample->SetCallback() != AV_ERR_OK) {
        return ReleaseSample();
    }
    if (g_vEncSample->Configure() != AV_ERR_OK) {
        return ReleaseSample();
    }
    if (g_vEncSample->SetCustomBuffer(bufferConfig, remaining_data.data(), remaining_data.size()) != AV_ERR_OK) {
        return ReleaseSample();
    }
    if (g_vEncSample->StartVideoEncoder() != AV_ERR_OK) {
        return ReleaseSample();
    }
    g_vEncSample->WaitForEOS();
    return ReleaseSample();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::EncoderInnerFuzzTest(data, size);
    return 0;
}
