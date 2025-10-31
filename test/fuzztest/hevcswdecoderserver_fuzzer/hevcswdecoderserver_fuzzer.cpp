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
#include <fuzzer/FuzzedDataProvider.h>
#include "hevcserverdec_sample.h"
#include "native_avcodec_videodecoder.h"
#include "native_avmemory.h"
#include "native_avformat.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "swdecoderserver_fuzzer"
const size_t EXPECT_SIZE = 8;
OH_AVCapability *cap_hevc = nullptr;
static string g_codecNameHevc = "";
OH_AVFormat *format;
OH_AVCodec *vdec_ = NULL;

namespace OHOS {
void GetDescription()
{
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, SOFTWARE);
    g_codecNameHevc = OH_AVCapability_GetName(cap_hevc);
    vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHevc.c_str());
    format = OH_VideoDecoder_GetOutputDescription(vdec_);
    int firstCallBackKey = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_NATIVE_BUFFER_FORMAT, &firstCallBackKey);
    const OH_NativeBuffer_Format *pixlFormats = nullptr;
    uint32_t pixlFormatNum = 0;
    const char *avcodecMmType = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
    OH_AVCapability *capability = OH_AVCodec_GetCapability(avcodecMmType, false);
    OH_AVCapability_GetVideoSupportedNativeBufferFormats(capability, &pixlFormats, &pixlFormatNum);
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    if (vdec_ != nullptr) {
        OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }
}
bool g_needRunDescription = true;
bool SwdecoderServerFuzzTest(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    if (g_needRunDescription) {
        g_needRunDescription = false;
        GetDescription();
    }
    FuzzedDataProvider fdp(data, size);
    bool needFlush = fdp.ConsumeBool();
    bool needStop = fdp.ConsumeBool();
    bool needReset = fdp.ConsumeBool();
    bool needRestart = fdp.ConsumeBool();
    auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
    VDecServerSample *vDecSample = new VDecServerSample();
    vDecSample->fuzzData = remaining_data.data();
    vDecSample->fuzzSize = remaining_data.size();
    vDecSample->RunVideoServerDecoder();
    if (needFlush) {
        vDecSample->Flush();
        if (needRestart) {
            vDecSample->Start();
        }
    }
    if (needStop) {
        vDecSample->Stop();
        if (needRestart) {
            vDecSample->Start();
        }
    }
    if (needReset) {
        vDecSample->Reset();
        if (needRestart) {
            vDecSample->ConfigServerDecoder();
            vDecSample->Start();
        }
    }
    vDecSample->WaitForEos();
    delete vDecSample;
    return false;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SwdecoderServerFuzzTest(data, size);
    return 0;
}
