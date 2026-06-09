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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "securec.h"
#include "native_avcodec_videobase.h"

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

typedef struct {
    const char *prefix;
    const char *key;
    int prefixLen;
} KvDesc;

static const KvDesc ROI_KV_TABLE[] = {
    { "dqp:", OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, 4 },
    { "slb:", OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, 4 },
};
#define ROI_KV_COUNT (sizeof(ROI_KV_TABLE) / sizeof(ROI_KV_TABLE[0]))

static const size_t ROI_STR_MAX_LEN = 256;
static const uint32_t ROI_MAX_COUNT = 6;

static uint32_t CountExistingRois(const char *roiStr)
{
    uint32_t count = 0;
    size_t len = strlen(roiStr);
    for (size_t i = 0; i < len; i++) {
        if (roiStr[i] == ';') count++;
    }
    return count + (len > 0 && roiStr[len - 1] != ';');
}

static int BuildRoiSegmentStr(char *buffer, size_t bufferSize, OH_AVFormat *avFormat)
{
    int32_t top = 0;
    int32_t left = 0;
    int32_t bottom = 0;
    int32_t right = 0;
    OH_AVFormat_GetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, &top);
    OH_AVFormat_GetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, &left);
    OH_AVFormat_GetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, &bottom);
    OH_AVFormat_GetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, &right);

    int written = snprintf(buffer, bufferSize, "%d,%d-%d,%d", top, left, bottom, right);
    if (written < 0 || written >= (int)bufferSize) {
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
            buffer[writePos++] = '=';
            isFirstKv = false;
        } else {
            buffer[writePos++] = ',';
        }
        int n = snprintf(buffer + writePos, bufferSize - writePos, "%s%d", ROI_KV_TABLE[i].prefix, val);
        if (n < 0 || writePos + n >= (int)bufferSize) {
            return -1;
        }
        writePos += n;
    }
    buffer[writePos++] = ';';
    buffer[writePos] = '\0';
    return writePos;
}

static OH_AVErrCode AppendSegmentToRoiStr(char **roiStrInOut, const char *segment)
{
    size_t segLen = strlen(segment);
    const char *oldStr = (*roiStrInOut != NULL) ? *roiStrInOut : "";
    size_t oldLen = strlen(oldStr);
    size_t extraLen = (oldLen > 0 && oldStr[oldLen - 1] != ';') ? 1 : 0;
    size_t newBufferSize = oldLen + extraLen + segLen + 1;

    char *newStr = (char *)malloc(newBufferSize);
    if (newStr == NULL) {
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

static bool ValidateAndParseRoiKvpairs(OH_AVFormat *avFormat, const char *eqPos, const char *segEnd)
{
    const char *curPtr = eqPos + 1;
    bool isCommaPending = false;
    bool isKvSet[ROI_KV_COUNT] = {false};
    bool isAnyKvPrefix = false;
    while (*curPtr != '\0' && (segEnd == NULL || curPtr < segEnd)) {
        uint32_t matchedIdx = ROI_KV_COUNT;
        for (uint32_t i = 0; i < ROI_KV_COUNT; i++) {
            if (strncmp(curPtr, ROI_KV_TABLE[i].prefix, ROI_KV_TABLE[i].prefixLen) != 0) {
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
        int32_t val = 0;
        int n = 0;
        if (sscanf(curPtr + ROI_KV_TABLE[matchedIdx].prefixLen, "%d%n", &val, &n) != 1) {
            return false;
        }
        OH_AVFormat_SetIntValue(avFormat, ROI_KV_TABLE[matchedIdx].key, val);
        curPtr += ROI_KV_TABLE[matchedIdx].prefixLen + n;
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
    int32_t dqpVal = 0;
    int n = 0;
    if (sscanf(eqPos, "=%d%n", &dqpVal, &n) != 1) {
        return false;
    }
    if (eqPos[n] != ';' && eqPos[n] != '\0') {
        return false;
    }
    OH_AVFormat_SetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, dqpVal);
    return true;
}

static OH_AVFormat *ParseOneRoiSegmentToFormat(const char *start, const char *segEnd)
{
    int top = 0;
    int left = 0;
    int bottom = 0;
    int right = 0;
    int consumed = 0;
    if (sscanf(start, "%d,%d-%d,%d%n", &top, &left, &bottom, &right, &consumed) != 4) {
        return NULL;
    }
    char nextChar = start[consumed];
    if (nextChar != '=' && nextChar != ';' && nextChar != '\0') {
        return NULL;
    }
    OH_AVFormat *avFormat = OH_AVFormat_Create();
    if (avFormat == NULL) {
        return NULL;
    }
    OH_AVFormat_SetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, top);
    OH_AVFormat_SetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, left);
    OH_AVFormat_SetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, bottom);
    OH_AVFormat_SetIntValue(avFormat, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, right);

    const char *eqPos = strchr(start, '=');
    if (eqPos != NULL && (segEnd == NULL || eqPos < segEnd)) {
        if (!ValidateAndParseRoiKvpairs(avFormat, eqPos, segEnd)) {
            OH_AVFormat_Destroy(avFormat);
            return NULL;
        }
    }
    return avFormat;
}

OH_AVErrCode OH_VideoMetadata_AppendRoiString(char **roiStrInOut, OH_AVFormat *format)
{
    if (roiStrInOut == NULL || format == NULL) {
        return AV_ERR_INVALID_VAL;
    }

    int32_t val = 0;
    if (!OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, &val) ||
        !OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, &val) ||
        !OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, &val) ||
        !OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, &val)) {
        return AV_ERR_INVALID_VAL;
    }

    if (*roiStrInOut != NULL && CountExistingRois(*roiStrInOut) >= ROI_MAX_COUNT) {
        return AV_ERR_INVALID_VAL;
    }

    char segmentBuf[128] = {0};
    int written = BuildRoiSegmentStr(segmentBuf, sizeof(segmentBuf), format);
    if (written < 0 || written >= (int)sizeof(segmentBuf)) {
        return AV_ERR_UNKNOWN;
    }

    size_t segLen = strlen(segmentBuf);
    size_t newTotalLen = segLen;
    if (*roiStrInOut != NULL) {
        size_t oldLen = strlen(*roiStrInOut);
        size_t extraLen = (oldLen > 0 && (*roiStrInOut)[oldLen - 1] != ';') ? 1 : 0;
        newTotalLen = oldLen + extraLen + segLen;
    }
    if (newTotalLen > ROI_STR_MAX_LEN) {
        return AV_ERR_INVALID_VAL;
    }

    return AppendSegmentToRoiStr(roiStrInOut, segmentBuf);
}

OH_AVErrCode OH_VideoMetadata_GetRoiCount(const char *roiStr, uint32_t *outCount)
{
    if (roiStr == NULL || outCount == NULL) {
        return AV_ERR_INVALID_VAL;
    }

    uint32_t count = 0;
    const char *curPtr = roiStr;
    while (*curPtr != '\0') {
        const char *segEnd = strchr(curPtr, ';');
        OH_AVFormat *avFormat = ParseOneRoiSegmentToFormat(curPtr, segEnd);
        if (avFormat != NULL) {
            count++;
            OH_AVFormat_Destroy(avFormat);
        }
        if (segEnd == NULL) {
            break;
        }
        curPtr = segEnd + 1;
    }
    *outCount = count;
    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoMetadata_ParseRoiString(const char *roiStr, OH_AVFormat **outFormats,
                                             uint32_t maxCapacity, uint32_t *outCount)
{
    if (roiStr == NULL || outFormats == NULL || outCount == NULL) {
        return AV_ERR_INVALID_VAL;
    }

    uint32_t parsedNum = 0;
    const char *curPtr = roiStr;
    while (*curPtr != '\0' && parsedNum < maxCapacity) {
        const char *segEnd = strchr(curPtr, ';');
        OH_AVFormat *avFormat = ParseOneRoiSegmentToFormat(curPtr, segEnd);
        if (avFormat != NULL) {
            outFormats[parsedNum++] = avFormat;
        }
        if (segEnd == NULL) {
            break;
        }
        curPtr = segEnd + 1;
    }
    *outCount = parsedNum;
    return AV_ERR_OK;
}