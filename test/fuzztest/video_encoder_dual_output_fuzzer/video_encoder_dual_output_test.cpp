/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <cstring>
#include <fuzzer/FuzzedDataProvider.h>
#include "video_enc_dual_output_sample.h"

using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "video_encoder_dual_output_test"

// ======================== Helpers ========================

const char *PickMime(FuzzedDataProvider &fdp)
{
    return fdp.ConsumeBool() ? OH_AVCODEC_MIMETYPE_VIDEO_HEVC : OH_AVCODEC_MIMETYPE_VIDEO_AVC;
}

// ======================== Fuzz Categories ========================

void LifecycleFuzzEntry(FuzzedDataProvider *fdp)
{
    VideoEncDualOutputSample sample;
    sample.LifecycleFuzz(*fdp);
}

void CreateParamFuzz(FuzzedDataProvider *fdp)
{
    const char *mime = PickMime(*fdp);
    OH_AVCodec *codec = nullptr;
    OH_VideoEncoder_CreatePrimaryWithPreproc(mime, &codec);
    if (codec != nullptr) {
        OH_VideoEncoder_Destroy(codec);
    }
    OH_VideoEncoder_CreatePrimaryWithPreproc(mime, nullptr);
    OH_VideoEncoder_CreateSecondaryFromPrimary(nullptr, &codec);

    OH_AVCodec *primary = nullptr;
    OH_AVErrCode ret = OH_VideoEncoder_CreatePrimaryWithPreproc(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, &primary);
    if (ret == AV_ERR_OK && primary != nullptr) {
        OH_VideoEncoder_CreateSecondaryFromPrimary(primary, nullptr);
        OH_AVCodec *fake = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
        if (fake != nullptr) {
            OH_VideoEncoder_CreateSecondaryFromPrimary(fake, &codec);
            OH_VideoEncoder_Destroy(fake);
        }
        OH_VideoEncoder_Destroy(primary);
    }
}

// ======================== Entry ========================

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    int32_t category = fdp.ConsumeIntegral<int32_t>() % 2; // 0=lifecycle, 1=create-param
    if (category == 0) {
        LifecycleFuzzEntry(&fdp);
    } else {
        CreateParamFuzz(&fdp);
    }
    return 0;
}
