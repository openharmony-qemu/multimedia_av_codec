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
#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
#define FUZZ_PROJECT_NAME "avcapability_fuzzer"

OH_AVCapability *cap = nullptr;
const int32_t *profiles = nullptr;
uint32_t profileNum = 0;
const int32_t *g_levels = nullptr;
uint32_t levelNum = 0;
OH_AVRange *bitrateRange;
OH_AVRange *qualityRange;
OH_AVRange *complexityRange;
const int32_t *g_pixFormat = nullptr;
uint32_t g_pixFormatNum = 0;
int32_t *g_widthAlignment = nullptr;
int32_t *g_heightAlignment = nullptr;
OH_AVRange *widthRange;
OH_AVRange *heightRange;
OH_AVRange *frameRateRange;

bool AvcapabilityFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    size_t maxSize = std::numeric_limits<size_t>::max();
    string mimeStr = fdp.ConsumeRandomLengthString(maxSize);
    bool isEncoder = fdp.ConsumeBool();
    int32_t profile = fdp.ConsumeIntegral<int32_t>();
    int32_t level = fdp.ConsumeIntegral<int32_t>();
    int bitrateValue = fdp.ConsumeIntegralInRange(0, 3);
    OH_BitrateMode mode = static_cast<OH_BitrateMode>(bitrateValue);
    int32_t height = fdp.ConsumeIntegral<int32_t>();
    int32_t width = fdp.ConsumeIntegral<int32_t>();
    int32_t frameRate = fdp.ConsumeIntegral<int32_t>();
    int featureValue = fdp.ConsumeIntegralInRange(0, 2);
    OH_AVCapabilityFeature feature = static_cast<OH_AVCapabilityFeature>(featureValue);
    OH_AVCodecCategory category = static_cast<OH_AVCodecCategory>(fdp.ConsumeIntegral<uint8_t>());

    OH_AVCodec_GetCapability(mimeStr.c_str(), isEncoder);
    cap = OH_AVCodec_GetCapabilityByCategory(mimeStr.c_str(), isEncoder, category);
    if (cap != nullptr) {
        OH_AVCapability_GetName(cap);
        OH_AVCapability_IsHardware(cap);
        OH_AVCapability_GetMaxSupportedInstances(cap);
        OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profileNum);
        OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &g_levels, &levelNum);
        OH_AVCapability_AreProfileAndLevelSupported(cap, profile, level);
        OH_AVCapability_GetEncoderBitrateRange(cap, bitrateRange);
        OH_AVCapability_GetEncoderQualityRange(cap, qualityRange);
        OH_AVCapability_GetEncoderComplexityRange(cap, complexityRange);
        OH_AVCapability_IsEncoderBitrateModeSupported(cap, mode);
        OH_AVCapability_GetVideoSupportedPixelFormats(cap, &g_pixFormat, &g_pixFormatNum);
        OH_AVCapability_GetVideoWidthAlignment(cap, g_widthAlignment);
        OH_AVCapability_GetVideoHeightAlignment(cap, g_heightAlignment);
        OH_AVCapability_GetVideoWidthRangeForHeight(cap, height, widthRange);
        OH_AVCapability_GetVideoHeightRangeForWidth(cap, width, heightRange);
        OH_AVCapability_GetVideoWidthRange(cap, widthRange);
        OH_AVCapability_GetVideoHeightRange(cap, heightRange);
        OH_AVCapability_IsVideoSizeSupported(cap, width, height);
        OH_AVCapability_GetVideoFrameRateRange(cap, frameRateRange);
        OH_AVCapability_GetVideoFrameRateRangeForSize(cap, width, height, frameRateRange);
        OH_AVCapability_AreVideoSizeAndFrameRateSupported(cap, width, height, frameRate);
        OH_AVCapability_IsFeatureSupported(cap, feature);
        OH_AVCapability_GetFeatureProperties(cap, feature);
    }
    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    AvcapabilityFuzzTest(data, size);
    return 0;
}
