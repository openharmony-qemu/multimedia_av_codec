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

#include "securec.h"
#include <cstddef>
#include <cstdint>
#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
#define FUZZ_PROJECT_NAME "avcapability_fuzzer"

OH_AVCapability *cap = nullptr;
const int32_t *g_profiles = nullptr;
uint32_t g_profileNum = 0;
const int32_t *g_levels = nullptr;
uint32_t g_levelNum = 0;
const int32_t *g_pixFormat = nullptr;
uint32_t g_pixFormatNum = 0;
const int32_t *g_sampleRates = nullptr;
uint32_t g_sampleRateNum = 0;
int32_t g_widthAlignment = 0;
int32_t g_heightAlignment = 0;
OH_AVRange *g_rangeList = nullptr;
uint32_t g_rangeListNum = 0;

bool AvcapabilityFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    size_t maxSize = std::numeric_limits<size_t>::max();
    string mimeStr = fdp.ConsumeRandomLengthString(maxSize);
    bool isEncoder1 = fdp.ConsumeBool();
    bool isEncoder2 = fdp.ConsumeBool();
    int32_t level = fdp.ConsumeIntegral<int32_t>();
    int bitrateValue = fdp.ConsumeIntegralInRange(0, 3);
    OH_BitrateMode mode = static_cast<OH_BitrateMode>(bitrateValue);
    int32_t frameRate = fdp.ConsumeIntegral<int32_t>();
    int featureValue1 = fdp.ConsumeIntegralInRange(0, 2);
    OH_AVCapabilityFeature feature1 = static_cast<OH_AVCapabilityFeature>(featureValue1);
    int featureValue2 = fdp.ConsumeIntegralInRange(0, 2);
    OH_AVCapabilityFeature feature2 = static_cast<OH_AVCapabilityFeature>(featureValue2);
    OH_AVCodecCategory category = static_cast<OH_AVCodecCategory>(fdp.ConsumeIntegral<uint8_t>());

    OH_AVCodec_GetCapability(mimeStr.c_str(), isEncoder1);
    cap = OH_AVCodec_GetCapabilityByCategory(mimeStr.c_str(), isEncoder2, category);
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability_GetName(cap);
    OH_AVCapability_IsHardware(cap);
    OH_AVCapability_GetMaxSupportedInstances(cap);
    OH_AVCapability_GetSupportedProfiles(cap, &g_profiles, &g_profileNum);
    OH_AVCapability_GetSupportedLevelsForProfile(cap, fdp.ConsumeIntegral<int32_t>(), &g_levels, &g_levelNum);
    OH_AVCapability_AreProfileAndLevelSupported(cap, fdp.ConsumeIntegral<int32_t>(), level);
    OH_AVCapability_GetEncoderBitrateRange(cap, &range);
    OH_AVCapability_GetEncoderQualityRange(cap, &range);
    OH_AVCapability_GetEncoderComplexityRange(cap, &range);
    OH_AVCapability_IsEncoderBitrateModeSupported(cap, mode);
    OH_AVCapability_GetVideoSupportedPixelFormats(cap, &g_pixFormat, &g_pixFormatNum);
    OH_AVCapability_GetVideoWidthAlignment(cap, &g_widthAlignment);
    OH_AVCapability_GetVideoHeightAlignment(cap, &g_heightAlignment);
    OH_AVCapability_GetVideoWidthRangeForHeight(cap, fdp.ConsumeIntegral<int32_t>(), &range);
    OH_AVCapability_GetVideoHeightRangeForWidth(cap, fdp.ConsumeIntegral<int32_t>(), &range);
    OH_AVCapability_GetVideoWidthRange(cap, &range);
    OH_AVCapability_GetVideoHeightRange(cap, &range);
    OH_AVCapability_IsVideoSizeSupported(cap, fdp.ConsumeIntegral<int32_t>(), fdp.ConsumeIntegral<int32_t>());
    OH_AVCapability_GetVideoFrameRateRange(cap, &range);
    OH_AVCapability_GetVideoFrameRateRangeForSize(cap, fdp.ConsumeIntegral<int32_t>(), fdp.ConsumeIntegral<int32_t>(),
        &range);
    OH_AVCapability_AreVideoSizeAndFrameRateSupported(cap, fdp.ConsumeIntegral<int32_t>(),
        fdp.ConsumeIntegral<int32_t>(), frameRate);
    OH_AVCapability_IsFeatureSupported(cap, feature1);
    OH_AVCapability_GetFeatureProperties(cap, feature2);
    OH_AVCapability_GetAudioChannelCountRange(cap, &range);
    OH_AVCapability_GetAudioSupportedSampleRates(cap, &g_sampleRates, &g_sampleRateNum);
    OH_AVCapability_GetAudioSupportedSampleRateRanges(cap, &g_rangeList, &g_rangeListNum);
    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    AvcapabilityFuzzTest(data, size);
    return 0;
}
