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

#include "native_avcodec_videobase.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "securec.h"
#include "avcodec_log.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *OH_MD_KEY_VIDEO_METADATA_ROI_TOP = "video_metadata_roi_top";
const char *OH_MD_KEY_VIDEO_METADATA_ROI_LEFT = "video_metadata_roi_left";
const char *OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM = "video_metadata_roi_bottom";
const char *OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT = "video_metadata_roi_right";
const char *OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP = "video_metadata_roi_deltaqp";
const char *OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL = "video_metadata_roi_sem_label";

#ifdef __cplusplus
}
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "NativeAvcodecVideoBase"};

struct KvDesc {
    const char *keyName;
    const char *key;
    int keyNameLen;
};

const KvDesc ROI_KV_TABLE[] = {
    { "dqp", OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, 3 },
    { "slb", OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, 3 },
};
constexpr uint32_t ROI_KV_COUNT = sizeof(ROI_KV_TABLE) / sizeof(ROI_KV_TABLE[0]);
constexpr size_t ROI_STR_MAX_LEN = 256;

struct RoiRectangleData {
    int32_t top;
    int32_t left;
    int32_t bottom;
    int32_t right;
};
}

class RoiStringHelper {
public:
    static OH_AVErrCode AppendRoiString(char **roiStrInOut, OH_AVFormat *format);
    static OH_AVErrCode GetRoiCount(const char *roiStr, uint32_t *outCount);
    static OH_AVErrCode ParseRoiString(const char *roiStr, OH_AVFormat **outFormats,
                                       uint32_t maxCapacity, uint32_t *outCount);

private:
    RoiStringHelper() = delete;
    ~RoiStringHelper() = delete;

    static bool ParseInt32(const char *str, int32_t *outVal, const char **outEndPtr);
    static bool IsValidRectangle(int32_t top, int32_t left, int32_t bottom, int32_t right);
    static bool ParseRectangleSegment(const char *start, RoiRectangleData *data, const char **outEndPtr);
    static void SetRectangleValues(OH_AVFormat *avFormat, const RoiRectangleData *data);
    static uint32_t CountExistingRois(const char *roiStr);
    static int BuildRoiSegmentStr(char *buffer, size_t bufferSize, OH_AVFormat *avFormat);
    static OH_AVErrCode AppendSegmentToRoiStr(char **roiStrInOut, const char *segment);
    static bool ValidateAndParseRoiKvpairs(OH_AVFormat *avFormat, const char *eqPos, const char *segEnd);
    static OH_AVFormat *ParseOneRoiSegmentToFormat(const char *start, const char *segEnd);
    static bool IsSameRoiFormat(OH_AVFormat *a, OH_AVFormat *b);
};

bool RoiStringHelper::ParseInt32(const char *str, int32_t *outVal, const char **outEndPtr)
{
    errno = 0;
    char *endPtr = nullptr;
    long val = strtol(str, &endPtr, 10);
    if (errno == ERANGE || endPtr == str || val < INT32_MIN || val > INT32_MAX) {
        return false;
    }
    *outVal = static_cast<int32_t>(val);
    *outEndPtr = endPtr;
    return true;
}

bool RoiStringHelper::IsValidRectangle(int32_t top, int32_t left, int32_t bottom, int32_t right)
{
    return top >= 0 && left >= 0 && bottom >= 0 && right >= 0 && top < bottom && left < right;
}

bool RoiStringHelper::ParseRectangleSegment(const char *start, RoiRectangleData *data, const char **outEndPtr)
{
    const char *endPtr = nullptr;
    if (!ParseInt32(start, &data->top, &endPtr) || *endPtr != ',') {
        return false;
    }
    if (!ParseInt32(endPtr + 1, &data->left, &endPtr) || *endPtr != '-') {
        return false;
    }
    if (!ParseInt32(endPtr + 1, &data->bottom, &endPtr) || *endPtr != ',') {
        return false;
    }
    if (!ParseInt32(endPtr + 1, &data->right, &endPtr)) {
        return false;
    }
    char nextChar = *endPtr;
    if (nextChar != '=' && nextChar != ';' && nextChar != '\0') {
        return false;
    }
    if (!IsValidRectangle(data->top, data->left, data->bottom, data->right)) {
        return false;
    }
    *outEndPtr = endPtr;
    return true;
}

void RoiStringHelper::SetRectangleValues(OH_AVFormat *avFormat, const RoiRectangleData *data)
{
    OH_AVFormat_SetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, data->top);
    OH_AVFormat_SetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, data->left);
    OH_AVFormat_SetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, data->bottom);
    OH_AVFormat_SetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, data->right);
}

uint32_t RoiStringHelper::CountExistingRois(const char *roiStr)
{
    uint32_t count = 0;
    size_t len = strlen(roiStr);
    for (size_t i = 0; i < len; i++) {
        if (roiStr[i] == ';') {
            count++;
        }
    }
    return count + (len > 0 && roiStr[len - 1] != ';');
}

int RoiStringHelper::BuildRoiSegmentStr(char *buffer, size_t bufferSize, OH_AVFormat *avFormat)
{
    int32_t top = 0;
    int32_t left = 0;
    int32_t bottom = 0;
    int32_t right = 0;
    OH_AVFormat_GetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, &top);
    OH_AVFormat_GetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, &left);
    OH_AVFormat_GetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, &bottom);
    OH_AVFormat_GetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, &right);

    int written = snprintf_s(buffer, bufferSize, bufferSize - 1, "%d,%d-%d,%d", top, left, bottom, right);
    if (written < 0) {
        return written;
    }

    int writePos = written;
    bool isFirstKv = true;
    for (uint32_t i = 0; i < ROI_KV_COUNT; i++) {
        int32_t val = 0;
        if (!OH_AVFormat_GetIntValue(avFormat, ROI_KV_TABLE[i].key, &val)) {
            continue;
        }
        if (isFirstKv) {
            if (writePos >= static_cast<int>(bufferSize)) {
                return -1;
            }
            buffer[writePos++] = '=';
            isFirstKv = false;
        } else {
            if (writePos >= static_cast<int>(bufferSize)) {
                return -1;
            }
            buffer[writePos++] = ',';
        }
        int n = snprintf_s(buffer + writePos, bufferSize - writePos, bufferSize - writePos - 1,
                           "%s:%d", ROI_KV_TABLE[i].keyName, val);
        if (n < 0) {
            return -1;
        }
        writePos += n;
    }
    if (writePos + 1 >= static_cast<int>(bufferSize)) {
        return -1;
    }
    buffer[writePos++] = ';';
    buffer[writePos] = '\0';
    return writePos;
}

OH_AVErrCode RoiStringHelper::AppendSegmentToRoiStr(char **roiStrInOut, const char *segment)
{
    size_t segLen = strlen(segment);
    const char *oldStr = (*roiStrInOut != nullptr) ? *roiStrInOut : "";
    size_t oldLen = strlen(oldStr);
    size_t extraLen = (oldLen > 0 && oldStr[oldLen - 1] != ';') ? 1 : 0;
    size_t newBufferSize = oldLen + extraLen + segLen + 1;

    char *newStr = static_cast<char *>(malloc(newBufferSize));
    if (newStr == nullptr) {
        return AV_ERR_NO_MEMORY;
    }
    if (oldLen > 0 && memcpy_s(newStr, newBufferSize, oldStr, oldLen) != EOK) {
        free(newStr);
        return AV_ERR_UNKNOWN;
    }
    if (extraLen > 0) {
        newStr[oldLen] = ';';
    }
    if (memcpy_s(newStr + oldLen + extraLen, segLen + 1, segment, segLen + 1) != EOK) {
        free(newStr);
        return AV_ERR_UNKNOWN;
    }
    free(*roiStrInOut);
    *roiStrInOut = newStr;
    return AV_ERR_OK;
}

bool RoiStringHelper::ValidateAndParseRoiKvpairs(OH_AVFormat *avFormat, const char *eqPos, const char *segEnd)
{
    const char *curPtr = eqPos + 1;
    bool isCommaPending = false;
    bool isKvSet[ROI_KV_COUNT] = {false};
    bool isAnyKvPrefix = false;
    while (*curPtr != '\0' && (segEnd == nullptr || curPtr < segEnd)) {
        uint32_t matchedIdx = ROI_KV_COUNT;
        for (uint32_t i = 0; i < ROI_KV_COUNT; i++) {
            if (strncmp(curPtr, ROI_KV_TABLE[i].keyName, ROI_KV_TABLE[i].keyNameLen) != 0 ||
                curPtr[ROI_KV_TABLE[i].keyNameLen] != ':') {
                continue;
            }
            matchedIdx = i;
            break;
        }
        if (matchedIdx == ROI_KV_COUNT) {
            break;
        }
        if (isKvSet[matchedIdx]) {
            return false;
        }
        isCommaPending = false;
        isKvSet[matchedIdx] = true;
        isAnyKvPrefix = true;
        const char *numEndPtr = nullptr;
        int32_t val = 0;
        if (!ParseInt32(curPtr + ROI_KV_TABLE[matchedIdx].keyNameLen + 1, &val, &numEndPtr)) {
            return false;
        }
        OH_AVFormat_SetIntValue(avFormat, ROI_KV_TABLE[matchedIdx].key, val);
        curPtr = numEndPtr;
        if (*curPtr == ',') {
            curPtr++;
            isCommaPending = true;
        } else if (*curPtr != ';' && *curPtr != '\0') {
            return false;
        }
    }
    if (isAnyKvPrefix) {
        return !isCommaPending && (*curPtr == ';' || *curPtr == '\0');
    }
    const char *dqpEndPtr = nullptr;
    int32_t dqpVal = 0;
    if (!ParseInt32(eqPos + 1, &dqpVal, &dqpEndPtr) || (*dqpEndPtr != ';' && *dqpEndPtr != '\0')) {
        return false;
    }
    OH_AVFormat_SetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, dqpVal);
    return true;
}

bool RoiStringHelper::IsSameRoiFormat(OH_AVFormat *a, OH_AVFormat *b)
{
    int32_t aTop = 0;
    int32_t aLeft = 0;
    int32_t aBottom = 0;
    int32_t aRight = 0;
    int32_t bTop = 0;
    int32_t bLeft = 0;
    int32_t bBottom = 0;
    int32_t bRight = 0;
    OH_AVFormat_GetIntValue(a, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, &aTop);
    OH_AVFormat_GetIntValue(a, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, &aLeft);
    OH_AVFormat_GetIntValue(a, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, &aBottom);
    OH_AVFormat_GetIntValue(a, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, &aRight);
    OH_AVFormat_GetIntValue(b, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, &bTop);
    OH_AVFormat_GetIntValue(b, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, &bLeft);
    OH_AVFormat_GetIntValue(b, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, &bBottom);
    OH_AVFormat_GetIntValue(b, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, &bRight);
    if (aTop != bTop || aLeft != bLeft || aBottom != bBottom || aRight != bRight) {
        return false;
    }
    for (uint32_t i = 0; i < ROI_KV_COUNT; i++) {
        int32_t aVal = 0;
        int32_t bVal = 0;
        bool aHas = OH_AVFormat_GetIntValue(a, ROI_KV_TABLE[i].key, &aVal);
        bool bHas = OH_AVFormat_GetIntValue(b, ROI_KV_TABLE[i].key, &bVal);
        if (aHas != bHas || (aHas && aVal != bVal)) {
            return false;
        }
    }
    return true;
}

OH_AVFormat *RoiStringHelper::ParseOneRoiSegmentToFormat(const char *start, const char *segEnd)
{
    RoiRectangleData data = {};
    const char *endPtr = nullptr;
    if (!ParseRectangleSegment(start, &data, &endPtr)) {
        return nullptr;
    }
    OH_AVFormat *avFormat = OH_AVFormat_Create();
    if (avFormat == nullptr) {
        return nullptr;
    }
    SetRectangleValues(avFormat, &data);
    const char *eqPos = strchr(start, '=');
    if (eqPos != nullptr && (segEnd == nullptr || eqPos < segEnd)) {
        if (!ValidateAndParseRoiKvpairs(avFormat, eqPos, segEnd)) {
            OH_AVFormat_Destroy(avFormat);
            return nullptr;
        }
    }
    return avFormat;
}

OH_AVErrCode RoiStringHelper::AppendRoiString(char **roiStrInOut, OH_AVFormat *format)
{
    if (roiStrInOut == nullptr || format == nullptr) {
        return AV_ERR_INVALID_VAL;
    }

    int32_t val = 0;
    if (!OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, &val) ||
        !OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, &val) ||
        !OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, &val) ||
        !OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, &val)) {
        return AV_ERR_INVALID_VAL;
    }

    char segmentBuf[128] = {0};
    int written = BuildRoiSegmentStr(segmentBuf, sizeof(segmentBuf), format);
    if (written < 0) {
        return AV_ERR_UNKNOWN;
    }

    size_t segLen = strlen(segmentBuf);
    size_t newTotalLen = segLen;
    if (*roiStrInOut != nullptr) {
        size_t oldLen = strlen(*roiStrInOut);
        size_t extraLen = (oldLen > 0 && (*roiStrInOut)[oldLen - 1] != ';') ? 1 : 0;
        newTotalLen = oldLen + extraLen + segLen;
    }
    if (newTotalLen > ROI_STR_MAX_LEN) {
        return AV_ERR_INVALID_VAL;
    }

    return AppendSegmentToRoiStr(roiStrInOut, segmentBuf);
}

OH_AVErrCode RoiStringHelper::GetRoiCount(const char *roiStr, uint32_t *outCount)
{
    if (roiStr == nullptr || outCount == nullptr) {
        return AV_ERR_INVALID_VAL;
    }

    uint32_t estimatedCount = CountExistingRois(roiStr);
    if (estimatedCount == 0) {
        *outCount = 0;
        return AV_ERR_OK;
    }

    if (estimatedCount > SIZE_MAX / sizeof(OH_AVFormat *)) {
        return AV_ERR_INVALID_VAL;
    }

    size_t allocSize = sizeof(OH_AVFormat *) * estimatedCount;
    OH_AVFormat **tempFormats = static_cast<OH_AVFormat **>(malloc(allocSize));
    if (tempFormats == nullptr) {
        return AV_ERR_NO_MEMORY;
    }

    uint32_t actualCount = 0;
    OH_AVErrCode ret = ParseRoiString(roiStr, tempFormats, estimatedCount, &actualCount);
    if (ret != AV_ERR_OK) {
        free(tempFormats);
        return ret;
    }

    for (uint32_t i = 0; i < actualCount; i++) {
        OH_AVFormat_Destroy(tempFormats[i]);
    }
    free(tempFormats);
    *outCount = actualCount;
    return AV_ERR_OK;
}

OH_AVErrCode RoiStringHelper::ParseRoiString(const char *roiStr, OH_AVFormat **outFormats,
    uint32_t maxCapacity, uint32_t *outCount)
{
    if (roiStr == nullptr || outFormats == nullptr || outCount == nullptr) {
        return AV_ERR_INVALID_VAL;
    }

    uint32_t parsedNum = 0;
    const char *curPtr = roiStr;
    while (*curPtr != '\0' && parsedNum < maxCapacity) {
        const char *segEnd = strchr(curPtr, ';');
        OH_AVFormat *avFormat = ParseOneRoiSegmentToFormat(curPtr, segEnd);
        if (avFormat != nullptr) {
            outFormats[parsedNum++] = avFormat;
        }
        if (segEnd == nullptr) {
            break;
        }
        curPtr = segEnd + 1;
    }

    uint32_t uniqueNum = 0;
    for (uint32_t i = 0; i < parsedNum; i++) {
        bool isDuplicate = false;
        for (uint32_t j = 0; j < uniqueNum; j++) {
            if (IsSameRoiFormat(outFormats[i], outFormats[j])) {
                isDuplicate = true;
                break;
            }
        }
        if (isDuplicate) {
            OH_AVFormat_Destroy(outFormats[i]);
        } else {
            outFormats[uniqueNum++] = outFormats[i];
        }
    }
    *outCount = uniqueNum;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoMetadata_AppendRoiString(char **roiStrInOut, OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(roiStrInOut != nullptr, AV_ERR_INVALID_VAL, "roiStrInOut is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "format is nullptr!");

    OH_AVErrCode ret = RoiStringHelper::AppendRoiString(roiStrInOut, format);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Append roi string failed, ret=%{public}d!", ret);
    AVCODEC_LOGD("Append roi string success!");
    return ret;
}

OH_AVErrCode OH_VideoMetadata_GetRoiCount(const char *roiStr, uint32_t *outCount)
{
    CHECK_AND_RETURN_RET_LOG(roiStr != nullptr, AV_ERR_INVALID_VAL, "roiStr is nullptr!");
    CHECK_AND_RETURN_RET_LOG(outCount != nullptr, AV_ERR_INVALID_VAL, "outCount is nullptr!");

    OH_AVErrCode ret = RoiStringHelper::GetRoiCount(roiStr, outCount);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Get roi count failed, ret=%{public}d!", ret);
    AVCODEC_LOGD("Get roi count success, count=%{public}u!", *outCount);
    return ret;
}

OH_AVErrCode OH_VideoMetadata_ParseRoiString(const char *roiStr, OH_AVFormat **outFormats,
                                             uint32_t maxCapacity, uint32_t *outCount)
{
    CHECK_AND_RETURN_RET_LOG(roiStr != nullptr, AV_ERR_INVALID_VAL, "roiStr is nullptr!");
    CHECK_AND_RETURN_RET_LOG(outFormats != nullptr, AV_ERR_INVALID_VAL, "outFormats is nullptr!");
    CHECK_AND_RETURN_RET_LOG(outCount != nullptr, AV_ERR_INVALID_VAL, "outCount is nullptr!");

    OH_AVErrCode ret = RoiStringHelper::ParseRoiString(roiStr, outFormats, maxCapacity, outCount);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Parse roi string failed, ret=%{public}d!", ret);
    AVCODEC_LOGD("Parse roi string success, count=%{public}u!", *outCount);
    return ret;
}