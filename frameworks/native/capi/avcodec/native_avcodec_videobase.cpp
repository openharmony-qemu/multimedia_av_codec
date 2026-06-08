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
    int32_t top;
    int32_t left;
    int32_t bottom;
    int32_t right;
    bool hasDqpVal;
    int32_t dqpVal;
    bool hasSemLabelVal;
    int32_t semLabelVal;
} RoiSegmentData;

static const size_t ROI_STR_MAX_LEN = 256;
static const uint32_t ROI_MAX_COUNT = 6;

static uint32_t CountExistingRois(const char *roiStr)
{
    uint32_t count = 0;
    const char *p = roiStr;
    while (*p != '\0') {
        if (*p == ';') {
            count++;
        }
        p++;
    }
    size_t len = strlen(roiStr);
    if (len > 0 && roiStr[len - 1] != ';') {
        count++;
    }
    return count;
}

static int BuildRoiSegmentStr(char *buf, size_t bufSize, const RoiSegmentData *data)
{
    int written = 0;
    if (data->hasDqpVal && data->hasSemLabelVal) {
        written = snprintf(buf, bufSize, "%d,%d-%d,%d=dqp:%d,slb:%d;",
                           data->top, data->left, data->bottom, data->right, data->dqpVal, data->semLabelVal);
    } else if (data->hasDqpVal) {
        written = snprintf(buf, bufSize, "%d,%d-%d,%d=dqp:%d;",
                           data->top, data->left, data->bottom, data->right, data->dqpVal);
    } else if (data->hasSemLabelVal) {
        written = snprintf(buf, bufSize, "%d,%d-%d,%d=slb:%d;",
                           data->top, data->left, data->bottom, data->right, data->semLabelVal);
    } else {
        written = snprintf(buf, bufSize, "%d,%d-%d,%d;",
                           data->top, data->left, data->bottom, data->right);
    }
    return written;
}

static OH_AVErrCode AppendSegmentToRoiStr(char **roiStrInOut, const char *segment)
{
    size_t segLen = strlen(segment);
    if (*roiStrInOut == NULL) {
        size_t bufSize = segLen + 1;
        *roiStrInOut = (char *)malloc(bufSize);
        if (*roiStrInOut == NULL) {
            return AV_ERR_NO_MEMORY;
        }
        if (memcpy_s(*roiStrInOut, bufSize, segment, segLen + 1) != EOK) {
            free(*roiStrInOut);
            *roiStrInOut = NULL;
            return AV_ERR_UNKNOWN;
        }
        return AV_ERR_OK;
    }

    size_t oldLen = strlen(*roiStrInOut);
    size_t extraLen = 0;
    if (oldLen > 0 && (*roiStrInOut)[oldLen - 1] != ';') {
        extraLen = 1;
    }
    size_t newBufSize = oldLen + extraLen + segLen + 1;
    char *newStr = (char *)malloc(newBufSize);
    if (newStr == NULL) {
        return AV_ERR_NO_MEMORY;
    }
    if (memcpy_s(newStr, newBufSize, *roiStrInOut, oldLen) != EOK) {
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

OH_AVErrCode OH_VideoMetadata_AppendRoiString(char **roiStrInOut, OH_AVFormat *format)
{
    if (roiStrInOut == NULL || format == NULL) {
        return AV_ERR_INVALID_VAL;
    }

    int32_t top = 0, left = 0, bottom = 0, right = 0;
    if (!OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, &top) ||
        !OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, &left) ||
        !OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, &bottom) ||
        !OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, &right)) {
        return AV_ERR_INVALID_VAL;
    }

    int32_t dqpVal = 0, semLabelVal = 0;
    bool hasDqpVal = OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &dqpVal);
    bool hasSemLabelVal = OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, &semLabelVal);

    if (*roiStrInOut != NULL && CountExistingRois(*roiStrInOut) >= ROI_MAX_COUNT) {
        return AV_ERR_INVALID_VAL;
    }

    RoiSegmentData data = {top, left, bottom, right, hasDqpVal, dqpVal, hasSemLabelVal, semLabelVal};
    char segmentBuf[128] = {0};
    int written = BuildRoiSegmentStr(segmentBuf, sizeof(segmentBuf), &data);
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
    const char *p = roiStr;

    while (*p != '\0') {
        int top, left, bottom, right;
        if (sscanf(p, "%d,%d-%d,%d", &top, &left, &bottom, &right) == 4) {
            count++;
        }

        const char *end = strchr(p, ';');
        if (!end) {
            break;
        }
        p = end + 1;
    }

    *outCount = count;
    return AV_ERR_OK;
}

static void ParseRoiKvpairsToFormat(OH_AVFormat *fmt, const char *eqPos, const char *segEnd)
{
    bool hasDqpPrefix = false;
    bool hasSlbPrefix = false;

    const char *dqpPos = strstr(eqPos, "dqp:");
    if (dqpPos != NULL && (segEnd == NULL || dqpPos < segEnd)) {
        hasDqpPrefix = true;
    }
    const char *slbPos = strstr(eqPos, "slb:");
    if (slbPos != NULL && (segEnd == NULL || slbPos < segEnd)) {
        hasSlbPrefix = true;
    }

    if (hasDqpPrefix || hasSlbPrefix) {
        if (hasDqpPrefix) {
            int32_t dqpVal = 0;
            if (sscanf(dqpPos, "dqp:%d", &dqpVal) == 1) {
                OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, dqpVal);
            }
        }
        if (hasSlbPrefix) {
            int32_t semLabelVal = 0;
            if (sscanf(slbPos, "slb:%d", &semLabelVal) == 1) {
                OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, semLabelVal);
            }
        }
    } else {
        int32_t dqpVal = 0;
        if (sscanf(eqPos, "=%d", &dqpVal) == 1) {
            OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, dqpVal);
        }
    }
}

static OH_AVFormat *ParseOneRoiSegmentToFormat(const char *start, const char *segEnd)
{
    int top = 0, left = 0, bottom = 0, right = 0;
    if (sscanf(start, "%d,%d-%d,%d", &top, &left, &bottom, &right) != 4) {
        return NULL;
    }

    OH_AVFormat *fmt = OH_AVFormat_Create();
    if (fmt == NULL) {
        return NULL;
    }

    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, top);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, left);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, bottom);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, right);

    const char *eqPos = strchr(start, '=');
    if (eqPos != NULL && (segEnd == NULL || eqPos < segEnd)) {
        ParseRoiKvpairsToFormat(fmt, eqPos, segEnd);
    }
    return fmt;
}

OH_AVErrCode OH_VideoMetadata_ParseRoiString(const char *roiStr, OH_AVFormat **outFormats,
                                             uint32_t maxCapacity, uint32_t *outCount)
{
    if (roiStr == NULL || outFormats == NULL || outCount == NULL) {
        return AV_ERR_INVALID_VAL;
    }

    uint32_t parsedNum = 0;
    const char *p = roiStr;

    while (*p != '\0' && parsedNum < maxCapacity) {
        const char *segEnd = strchr(p, ';');
        OH_AVFormat *fmt = ParseOneRoiSegmentToFormat(p, segEnd);
        if (fmt != NULL) {
            outFormats[parsedNum] = fmt;
            parsedNum++;
        }
        if (segEnd == NULL) {
            break;
        }
        p = segEnd + 1;
    }

    *outCount = parsedNum;
    return AV_ERR_OK;
}
