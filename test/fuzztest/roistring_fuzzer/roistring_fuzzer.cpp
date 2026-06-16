/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include <cstdlib>
#include <cstring>
#include <fuzzer/FuzzedDataProvider.h>
#include "native_avcodec_videobase.h"
#include "native_avformat.h"

#define FUZZ_PROJECT_NAME "roistring_fuzzer"

namespace OHOS {
namespace RoiStringFuzzer {

constexpr int32_t MAX_COORD = 10000;
constexpr uint32_t MAX_PARSE_CAPACITY = 8;

static void DestroyFormats(OH_AVFormat **formats, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        OH_AVFormat_Destroy(formats[i]);
    }
}

// Create a format with fuzzed coordinates, optionally with kv pairs
bool AppendRoiStringFuzzTest(FuzzedDataProvider *provider)
{
    int32_t top = provider->ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    int32_t left = provider->ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    int32_t bottom = provider->ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    int32_t right = provider->ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    bool setDqp = provider->ConsumeBool();
    bool setSlb = provider->ConsumeBool();
    int32_t dqpVal = provider->ConsumeIntegralInRange<int32_t>(-60, 60);
    int32_t slbVal = provider->ConsumeIntegralInRange<int32_t>(-2, 3);
    bool appendSecond = provider->ConsumeBool();

    OH_AVFormat *fmt = OH_AVFormat_Create();
    if (fmt == nullptr) {
        return false;
    }
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, top);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, left);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, bottom);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, right);
    if (setDqp) {
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, dqpVal);
    }
    if (setSlb) {
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, slbVal);
    }

    char *roiStr = nullptr;
    OH_AVErrCode ret = OH_VideoMetadata_AppendRoiString(&roiStr, fmt);
    if (ret == AV_ERR_OK && roiStr != nullptr && appendSecond) {
        OH_AVFormat *fmt2 = OH_AVFormat_Create();
        if (fmt2 != nullptr) {
            OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, top + 1);
            OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, left + 1);
            OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, bottom + 1);
            OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, right + 1);
            OH_VideoMetadata_AppendRoiString(&roiStr, fmt2);
            OH_AVFormat_Destroy(fmt2);
        }
        if (roiStr != nullptr) {
            uint32_t count = 0;
            OH_VideoMetadata_GetRoiCount(roiStr, &count);
            OH_AVFormat *parsed[MAX_PARSE_CAPACITY] = {};
            uint32_t parsedCount = 0;
            OH_VideoMetadata_ParseRoiString(roiStr, parsed, MAX_PARSE_CAPACITY, &parsedCount);
            DestroyFormats(parsed, parsedCount);
        }
    }
    if (roiStr != nullptr) {
        free(roiStr);
    }
    OH_AVFormat_Destroy(fmt);
    return true;
}

// Also test with invalid coords (negative, top>=bottom, left>=right)
bool AppendRoiStringInvalidFuzzTest(FuzzedDataProvider *provider)
{
    int32_t top = provider->ConsumeIntegralInRange<int32_t>(-10, MAX_COORD);
    int32_t left = provider->ConsumeIntegralInRange<int32_t>(-10, MAX_COORD);
    int32_t bottom = provider->ConsumeIntegralInRange<int32_t>(-10, MAX_COORD);
    int32_t right = provider->ConsumeIntegralInRange<int32_t>(-10, MAX_COORD);

    OH_AVFormat *fmt = OH_AVFormat_Create();
    if (fmt == nullptr) {
        return false;
    }
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, top);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, left);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, bottom);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, right);

    char *roiStr = nullptr;
    OH_VideoMetadata_AppendRoiString(&roiStr, fmt);
    if (roiStr != nullptr) {
        free(roiStr);
    }
    OH_AVFormat_Destroy(fmt);
    return true;
}

// Test format without required keys
bool AppendRoiStringMissingKeysFuzzTest(FuzzedDataProvider *provider)
{
    bool skipTop = provider->ConsumeBool();
    bool skipLeft = provider->ConsumeBool();
    bool skipBottom = provider->ConsumeBool();
    bool skipRight = provider->ConsumeBool();

    OH_AVFormat *fmt = OH_AVFormat_Create();
    if (fmt == nullptr) {
        return false;
    }
    if (!skipTop) {
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, 0);
    }
    if (!skipLeft) {
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, 0);
    }
    if (!skipBottom) {
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, 100);
    }
    if (!skipRight) {
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, 100);
    }

    char *roiStr = nullptr;
    OH_VideoMetadata_AppendRoiString(&roiStr, fmt);
    if (roiStr != nullptr) {
        free(roiStr);
    }
    OH_AVFormat_Destroy(fmt);
    return true;
}

// Use raw fuzz data as ROI string input for GetRoiCount/ParseRoiString
bool ParseRoiStringFuzzTest(const uint8_t *data, size_t size)
{
    std::string roiStr(reinterpret_cast<const char *>(data), size);
    uint32_t count = 0;
    OH_AVErrCode ret = OH_VideoMetadata_GetRoiCount(roiStr.c_str(), &count);
    if (ret == AV_ERR_OK && count > 0) {
        OH_AVFormat *parsed[MAX_PARSE_CAPACITY] = {};
        uint32_t parsedCount = 0;
        OH_VideoMetadata_ParseRoiString(roiStr.c_str(), parsed, MAX_PARSE_CAPACITY, &parsedCount);
        DestroyFormats(parsed, parsedCount);
    }
    return true;
}

// Test with pre-built valid ROI strings combined with fuzz data
bool ParseRoiStringMixedFuzzTest(FuzzedDataProvider *provider)
{
    std::string baseStr;
    int32_t segmentCount = provider->ConsumeIntegralInRange<int32_t>(1, 4);
    for (int32_t i = 0; i < segmentCount; i++) {
        int32_t top = provider->ConsumeIntegralInRange<int32_t>(0, 50);
        int32_t left = provider->ConsumeIntegralInRange<int32_t>(0, 50);
        int32_t bottom = provider->ConsumeIntegralInRange<int32_t>(top + 1, 100);
        int32_t right = provider->ConsumeIntegralInRange<int32_t>(left + 1, 100);
        baseStr += std::to_string(top) + "," + std::to_string(left) + "-" +
                   std::to_string(bottom) + "," + std::to_string(right);

        bool hasKv = provider->ConsumeBool();
        if (hasKv) {
            bool useNewFormat = provider->ConsumeBool();
            if (useNewFormat) {
                baseStr += "=dqp:" + std::to_string(provider->ConsumeIntegralInRange<int32_t>(-51, 51));
                bool alsoSlb = provider->ConsumeBool();
                if (alsoSlb) {
                    baseStr += ",slb:" + std::to_string(provider->ConsumeIntegralInRange<int32_t>(0, 1));
                }
            } else {
                baseStr += "=" + std::to_string(provider->ConsumeIntegralInRange<int32_t>(-51, 51));
            }
        }
        baseStr += ";";
    }

    uint32_t count = 0;
    OH_AVErrCode ret = OH_VideoMetadata_GetRoiCount(baseStr.c_str(), &count);
    if (ret == AV_ERR_OK && count > 0) {
        OH_AVFormat *parsed[MAX_PARSE_CAPACITY] = {};
        uint32_t parsedCount = 0;
        OH_VideoMetadata_ParseRoiString(baseStr.c_str(), parsed, MAX_PARSE_CAPACITY, &parsedCount);
        DestroyFormats(parsed, parsedCount);
    }
    return true;
}

// Test with intentionally invalid ROI strings
bool ParseRoiStringInvalidFuzzTest(FuzzedDataProvider *provider)
{
    int32_t invalidType = provider->ConsumeIntegralInRange<int32_t>(0, 6);
    std::string invalidStr;
    switch (invalidType) {
        case 0:
            invalidStr = "abc";
            break;
        case 1:
            invalidStr = "10,20-5,30;";
            break;
        case 2:
            invalidStr = "10,50-30,20;";
            break;
        case 3:
            invalidStr = "10,20-30,40=dqp:abc;";
            break;
        case 4:
            invalidStr = "10,20-30,40=slb:1t;";
            break;
        case 5:
            invalidStr = "10,20-30,40=60;";
            break;
        case 6:
            invalidStr = "10,20-30,40=dqp:5,dqp:3;";
            break;
        default:
            invalidStr = "";
            break;
    }

    uint32_t count = 0;
    OH_VideoMetadata_GetRoiCount(invalidStr.c_str(), &count);
    OH_AVFormat *parsed[MAX_PARSE_CAPACITY] = {};
    uint32_t parsedCount = 0;
    OH_VideoMetadata_ParseRoiString(invalidStr.c_str(), parsed, MAX_PARSE_CAPACITY, &parsedCount);
    DestroyFormats(parsed, parsedCount);
    return true;
}

// Test GetRoiCount/ParseRoiString with nullptr params
bool ParseRoiStringNullFuzzTest()
{
    uint32_t count = 0;
    OH_VideoMetadata_GetRoiCount(nullptr, &count);
    OH_VideoMetadata_GetRoiCount("0,0-10,10;", nullptr);

    OH_AVFormat *formats[MAX_PARSE_CAPACITY] = {};
    uint32_t parsedCount = 0;
    OH_VideoMetadata_ParseRoiString(nullptr, formats, MAX_PARSE_CAPACITY, &parsedCount);
    OH_VideoMetadata_ParseRoiString("0,0-10,10;", nullptr, MAX_PARSE_CAPACITY, &parsedCount);
    OH_VideoMetadata_ParseRoiString("0,0-10,10;", formats, MAX_PARSE_CAPACITY, nullptr);
    return true;
}

// Test AppendRoiString with nullptr params
bool AppendRoiStringNullFuzzTest()
{
    OH_VideoMetadata_AppendRoiString(nullptr, nullptr);
    char *roiStr = nullptr;
    OH_VideoMetadata_AppendRoiString(&roiStr, nullptr);
    OH_AVFormat *fmt = OH_AVFormat_Create();
    if (fmt != nullptr) {
        OH_VideoMetadata_AppendRoiString(nullptr, fmt);
        OH_AVFormat_Destroy(fmt);
    }
    return true;
}

// Test empty string
bool ParseRoiStringEmptyFuzzTest()
{
    uint32_t count = 0;
    OH_VideoMetadata_GetRoiCount("", &count);

    OH_AVFormat *formats[MAX_PARSE_CAPACITY] = {};
    uint32_t parsedCount = 0;
    OH_VideoMetadata_ParseRoiString("", formats, MAX_PARSE_CAPACITY, &parsedCount);
    return true;
}

} // namespace RoiStringFuzzer
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t)) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);

    static uint8_t nullTestFlag = 0;
    if (nullTestFlag == 0) {
        OHOS::RoiStringFuzzer::AppendRoiStringNullFuzzTest();
        OHOS::RoiStringFuzzer::ParseRoiStringNullFuzzTest();
        OHOS::RoiStringFuzzer::ParseRoiStringEmptyFuzzTest();
        nullTestFlag = 1;
    }

    int32_t category = fdp.ConsumeIntegralInRange<int32_t>(0, 5);
    switch (category) {
        case 0:
            OHOS::RoiStringFuzzer::AppendRoiStringFuzzTest(&fdp);
            break;
        case 1:
            OHOS::RoiStringFuzzer::AppendRoiStringInvalidFuzzTest(&fdp);
            break;
        case 2:
            OHOS::RoiStringFuzzer::AppendRoiStringMissingKeysFuzzTest(&fdp);
            break;
        case 3:
            OHOS::RoiStringFuzzer::ParseRoiStringFuzzTest(data, size);
            break;
        case 4:
            OHOS::RoiStringFuzzer::ParseRoiStringMixedFuzzTest(&fdp);
            break;
        case 5:
            OHOS::RoiStringFuzzer::ParseRoiStringInvalidFuzzTest(&fdp);
            break;
        default:
            break;
    }
    return 0;
}
