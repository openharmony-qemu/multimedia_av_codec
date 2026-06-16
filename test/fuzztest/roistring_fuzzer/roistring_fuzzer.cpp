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
using namespace std;

constexpr int32_t MAX_COORD = 10000;
constexpr uint32_t MAX_PARSE_CAPACITY = 8;

bool DoRoiStringFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int32_t)) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    int32_t top = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    int32_t left = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    int32_t bottom = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    int32_t right = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_COORD);
    int32_t dqpVal = fdp.ConsumeIntegralInRange<int32_t>(-60, 60);
    int32_t slbVal = fdp.ConsumeIntegralInRange<int32_t>(-2, 3);
    bool setDqp = fdp.ConsumeBool();
    bool setSlb = fdp.ConsumeBool();

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
    OH_VideoMetadata_AppendRoiString(&roiStr, fmt);
    if (roiStr != nullptr) {
        OH_AVFormat *fmt2 = OH_AVFormat_Create();
        if (fmt2 != nullptr) {
            OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, top + 1);
            OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, left + 1);
            OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, bottom + 1);
            OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, right + 1);
            OH_VideoMetadata_AppendRoiString(&roiStr, fmt2);
            OH_AVFormat_Destroy(fmt2);
        }
        uint32_t count = 0;
        OH_VideoMetadata_GetRoiCount(roiStr, &count);
        OH_AVFormat *parsed[MAX_PARSE_CAPACITY] = {};
        uint32_t parsedCount = 0;
        OH_VideoMetadata_ParseRoiString(roiStr, parsed, MAX_PARSE_CAPACITY, &parsedCount);
        for (uint32_t i = 0; i < parsedCount; i++) {
            OH_AVFormat_Destroy(parsed[i]);
        }
        free(roiStr);
    }
    OH_AVFormat_Destroy(fmt);

    // Parse raw fuzz data as roi string
    std::string rawStr(reinterpret_cast<const char *>(data), size);
    uint32_t rawCount = 0;
    OH_AVErrCode ret = OH_VideoMetadata_GetRoiCount(rawStr.c_str(), &rawCount);
    if (ret == AV_ERR_OK && rawCount > 0) {
        OH_AVFormat *rawParsed[MAX_PARSE_CAPACITY] = {};
        uint32_t rawParsedCount = 0;
        OH_VideoMetadata_ParseRoiString(rawStr.c_str(), rawParsed, MAX_PARSE_CAPACITY, &rawParsedCount);
        for (uint32_t i = 0; i < rawParsedCount; i++) {
            OH_AVFormat_Destroy(rawParsed[i]);
        }
    }

    // Nullptr test
    OH_VideoMetadata_AppendRoiString(nullptr, nullptr);
    OH_VideoMetadata_GetRoiCount(nullptr, nullptr);
    OH_VideoMetadata_ParseRoiString(nullptr, nullptr, 0, nullptr);

    return true;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    DoRoiStringFuzzTest(data, size);
    return 0;
}
