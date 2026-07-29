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

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fuzzer/FuzzedDataProvider.h>
#include "native_avcodec_videobase.h"
#include "native_avformat.h"

#define FUZZ_PROJECT_NAME "roistring_fuzzer"

constexpr int32_t CATEGORY_MAX = 5;
constexpr int32_t INVALID_TYPE_MAX = 6;

enum FuzzCategory : int32_t {
    CAT_APPEND_ROI = 0,
    CAT_APPEND_INVALID = 1,
    CAT_APPEND_MISSING_KEYS = 2,
    CAT_PARSE_RAW = 3,
    CAT_PARSE_MIXED = 4,
    CAT_PARSE_INVALID = 5,
};

enum InvalidType : int32_t {
    INV_NON_NUMERIC = 0,
    INV_BOTTOM_LESS_TOP = 1,
    INV_RIGHT_LESS_LEFT = 2,
    INV_DQP_NON_NUMERIC = 3,
    INV_SLB_NON_NUMERIC = 4,
    INV_OLD_FORMAT_OUT_OF_RANGE = 5,
    INV_DQP_DUPLICATE = 6,
};

namespace OHOS {
namespace RoiStringFuzzer {

constexpr int32_t MAX_COORD = 10000;
constexpr int32_t MAX_PARSE_CAPACITY = 8;
constexpr int32_t INVALID_COORD_MIN = -10;
constexpr int32_t DEFAULT_VALID_COORD = 100;
constexpr int32_t DQP_FUZZ_MIN = -60;
constexpr int32_t DQP_FUZZ_MAX = 60;
constexpr int32_t DQP_MIN_VAL = -51;
constexpr int32_t DQP_MAX_VAL = 51;
constexpr int32_t SLB_FUZZ_MIN = -2;
constexpr int32_t SLB_FUZZ_MAX = 3;
constexpr int32_t SLB_MIN_VAL = 0;
constexpr int32_t SLB_MAX_VAL = 1;
constexpr int32_t MIN_SEGMENT_COUNT = 1;
constexpr int32_t MAX_SEGMENT_COUNT = 4;
constexpr int32_t MIXED_COORD_MAX = 50;
constexpr int32_t COORD_INCREMENT = 1;

static void DestroyFormats(OH_AVFormat **formats, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        OH_AVFormat_Destroy(formats[i]);
    }
}

static std::string BuildCoordStr(int32_t top, int32_t left,
                                 int32_t bottom, int32_t right)
{
    return std::to_string(top) + "," + std::to_string(left) + "-" +
           std::to_string(bottom) + "," + std::to_string(right);
}

static std::string BuildKvStr(FuzzedDataProvider *provider)
{
    std::string kvStr;
    bool useNewFormat = provider->ConsumeBool();
    if (useNewFormat) {
        int32_t dqp = provider->ConsumeIntegralInRange<int32_t>(
            DQP_MIN_VAL, DQP_MAX_VAL);
        kvStr += "=dqp:" + std::to_string(dqp);
        if (provider->ConsumeBool()) {
            int32_t slb = provider->ConsumeIntegralInRange<int32_t>(
                SLB_MIN_VAL, SLB_MAX_VAL);
            kvStr += ",slb:" + std::to_string(slb);
        }
    } else {
        int32_t dqp = provider->ConsumeIntegralInRange<int32_t>(
            DQP_MIN_VAL, DQP_MAX_VAL);
        kvStr += "=" + std::to_string(dqp);
    }
    return kvStr;
}

static void ParseAndDestroyRoiStr(const std::string &roiStr)
{
    uint32_t count = 0;
    OH_AVErrCode ret = OH_VideoMetadata_GetRoiCount(roiStr.c_str(), &count);
    if (ret != AV_ERR_OK || count == 0) {
        return;
    }
    OH_AVFormat *parsed[MAX_PARSE_CAPACITY] = {};
    uint32_t parsedCount = 0;
    OH_VideoMetadata_ParseRoiString(
        roiStr.c_str(), parsed, MAX_PARSE_CAPACITY, &parsedCount);
    DestroyFormats(parsed, parsedCount);
}

static void AppendSecondAndParse(char **roiStr, int32_t top, int32_t left,
                                 int32_t bottom, int32_t right)
{
    OH_AVFormat *fmt2 = OH_AVFormat_Create();
    if (fmt2 == nullptr) {
        return;
    }
    OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_TOP,
                            top + COORD_INCREMENT);
    OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT,
                            left + COORD_INCREMENT);
    OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM,
                            bottom + COORD_INCREMENT);
    OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT,
                            right + COORD_INCREMENT);
    OH_VideoMetadata_AppendRoiString(roiStr, fmt2);
    OH_AVFormat_Destroy(fmt2);
}

bool AppendRoiStringFuzzTest(FuzzedDataProvider *provider)
{
    int32_t top = provider->ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    int32_t left = provider->ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    int32_t bottom = provider->ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    int32_t right = provider->ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    bool setDqp = provider->ConsumeBool();
    bool setSlb = provider->ConsumeBool();
    int32_t dqpVal = provider->ConsumeIntegralInRange<int32_t>(
        DQP_FUZZ_MIN, DQP_FUZZ_MAX);
    int32_t slbVal = provider->ConsumeIntegralInRange<int32_t>(
        SLB_FUZZ_MIN, SLB_FUZZ_MAX);
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
        AppendSecondAndParse(&roiStr, top, left, bottom, right);
        if (roiStr != nullptr) {
            std::string str(roiStr);
            ParseAndDestroyRoiStr(str);
        }
    }
    if (roiStr != nullptr) {
        free(roiStr);
    }
    OH_AVFormat_Destroy(fmt);
    return true;
}

bool AppendRoiStringInvalidFuzzTest(FuzzedDataProvider *provider)
{
    int32_t top = provider->ConsumeIntegralInRange<int32_t>(
        INVALID_COORD_MIN, MAX_COORD);
    int32_t left = provider->ConsumeIntegralInRange<int32_t>(
        INVALID_COORD_MIN, MAX_COORD);
    int32_t bottom = provider->ConsumeIntegralInRange<int32_t>(
        INVALID_COORD_MIN, MAX_COORD);
    int32_t right = provider->ConsumeIntegralInRange<int32_t>(
        INVALID_COORD_MIN, MAX_COORD);

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
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM,
                                DEFAULT_VALID_COORD);
    }
    if (!skipRight) {
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT,
                                DEFAULT_VALID_COORD);
    }

    char *roiStr = nullptr;
    OH_VideoMetadata_AppendRoiString(&roiStr, fmt);
    if (roiStr != nullptr) {
        free(roiStr);
    }
    OH_AVFormat_Destroy(fmt);
    return true;
}

bool ParseRoiStringFuzzTest(const uint8_t *data, size_t size)
{
    std::string roiStr(reinterpret_cast<const char *>(data), size);
    ParseAndDestroyRoiStr(roiStr);
    return true;
}

bool ParseRoiStringMixedFuzzTest(FuzzedDataProvider *provider)
{
    std::string baseStr;
    int32_t segmentCount = provider->ConsumeIntegralInRange<int32_t>(
        MIN_SEGMENT_COUNT, MAX_SEGMENT_COUNT);
    for (int32_t i = 0; i < segmentCount; i++) {
        int32_t top = provider->ConsumeIntegralInRange<int32_t>(
            0, MIXED_COORD_MAX);
        int32_t left = provider->ConsumeIntegralInRange<int32_t>(
            0, MIXED_COORD_MAX);
        int32_t bottom = provider->ConsumeIntegralInRange<int32_t>(
            top + COORD_INCREMENT, DEFAULT_VALID_COORD);
        int32_t right = provider->ConsumeIntegralInRange<int32_t>(
            left + COORD_INCREMENT, DEFAULT_VALID_COORD);
        baseStr += BuildCoordStr(top, left, bottom, right);
        if (provider->ConsumeBool()) {
            baseStr += BuildKvStr(provider);
        }
        baseStr += ";";
    }
    ParseAndDestroyRoiStr(baseStr);
    return true;
}

static std::string GetInvalidStr(int32_t invalidType)
{
    switch (invalidType) {
        case INV_NON_NUMERIC:
            return "abc";
        case INV_BOTTOM_LESS_TOP:
            return "10,20-5,30;";
        case INV_RIGHT_LESS_LEFT:
            return "10,50-30,20;";
        case INV_DQP_NON_NUMERIC:
            return "10,20-30,40=dqp:abc;";
        case INV_SLB_NON_NUMERIC:
            return "10,20-30,40=slb:1t;";
        case INV_OLD_FORMAT_OUT_OF_RANGE:
            return "10,20-30,40=60;";
        case INV_DQP_DUPLICATE:
            return "10,20-30,40=dqp:5,dqp:3;";
        default:
            return "";
    }
}

bool ParseRoiStringInvalidFuzzTest(FuzzedDataProvider *provider)
{
    int32_t invalidType = provider->ConsumeIntegralInRange<int32_t>(
        0, INVALID_TYPE_MAX);
    std::string invalidStr = GetInvalidStr(invalidType);
    ParseAndDestroyRoiStr(invalidStr);
    return true;
}

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

    int32_t category = fdp.ConsumeIntegralInRange<int32_t>(0, CATEGORY_MAX);
    switch (category) {
        case CAT_APPEND_ROI:
            OHOS::RoiStringFuzzer::AppendRoiStringFuzzTest(&fdp);
            break;
        case CAT_APPEND_INVALID:
            OHOS::RoiStringFuzzer::AppendRoiStringInvalidFuzzTest(&fdp);
            break;
        case CAT_APPEND_MISSING_KEYS:
            OHOS::RoiStringFuzzer::AppendRoiStringMissingKeysFuzzTest(&fdp);
            break;
        case CAT_PARSE_RAW: {
            auto remaining = fdp.ConsumeRemainingBytes<uint8_t>();
            OHOS::RoiStringFuzzer::ParseRoiStringFuzzTest(remaining.data(), remaining.size());
            break;
        }
        case CAT_PARSE_MIXED:
            OHOS::RoiStringFuzzer::ParseRoiStringMixedFuzzTest(&fdp);
            break;
        case CAT_PARSE_INVALID:
            OHOS::RoiStringFuzzer::ParseRoiStringInvalidFuzzTest(&fdp);
            break;
        default:
            break;
    }
    return 0;
}
