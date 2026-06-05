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
#include "native_avcodec_videobase.h"

OH_AVErrCode OH_VideoMetadata_AppendRoiString(char **roiStrInOut, OH_AVFormat *format) {
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

    int32_t dqp = 0, semLabel = 0;
    bool hasDqp = OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &dqp);
    bool hasSemLabel = OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, &semLabel);

    char currentRoiStr[128] = {0};
    int written = 0;

    if (hasDqp && hasSemLabel) {
        written = snprintf(currentRoiStr, sizeof(currentRoiStr), "%d,%d-%d,%d=dqp:%d,slb:%d;", 
                           top, left, bottom, right, dqp, semLabel);
    } else if (hasDqp) {
        written = snprintf(currentRoiStr, sizeof(currentRoiStr), "%d,%d-%d,%d=dqp:%d;", 
                           top, left, bottom, right, dqp);
    } else if (hasSemLabel) {
        written = snprintf(currentRoiStr, sizeof(currentRoiStr), "%d,%d-%d,%d=slb:%d;", 
                           top, left, bottom, right, semLabel);
    } else {
        written = snprintf(currentRoiStr, sizeof(currentRoiStr), "%d,%d-%d,%d;", 
                           top, left, bottom, right);
    }

    if (written < 0 || written >= (int)sizeof(currentRoiStr)) {
        return AV_ERR_UNKNOWN; 
    }

    size_t currentLen = strlen(currentRoiStr);
    if (*roiStrInOut == NULL) {
        *roiStrInOut = (char *)malloc(currentLen + 1);
        if (*roiStrInOut == NULL) {
            return AV_ERR_NO_MEMORY;
        }
        strcpy(*roiStrInOut, currentRoiStr);
    } else {
        size_t oldLen = strlen(*roiStrInOut);
        char *newStr = (char *)realloc(*roiStrInOut, oldLen + currentLen + 1);
        if (newStr == NULL) {
            return AV_ERR_NO_MEMORY; 
        }
        strcat(newStr, currentRoiStr);
        *roiStrInOut = newStr;
    }

    return AV_ERR_OK;
}

OH_AVErrCode OH_VideoMetadata_GetRoiCount(const char *roiStr, uint32_t *outCount) {
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

OH_AVErrCode OH_VideoMetadata_ParseRoiString(const char *roiStr, OH_AVFormat **outFormats, uint32_t maxCapacity, uint32_t *outCount) {
    if (roiStr == NULL || outFormats == NULL || outCount == NULL) {
        return AV_ERR_INVALID_VAL;
    }

    uint32_t parsedNum = 0;
    const char *p = roiStr;

    while (*p != '\0' && parsedNum < maxCapacity) {
        int top = 0, left = 0, bottom = 0, right = 0;
        const char *segmentEnd = strchr(p, ';');

        if (sscanf(p, "%d,%d-%d,%d", &top, &left, &bottom, &right) == 4) {
            
            OH_AVFormat *newFormat = OH_AVFormat_Create();
            if (newFormat == NULL) {
                break;
            }

            OH_AVFormat_SetIntValue(newFormat, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, top);
            OH_AVFormat_SetIntValue(newFormat, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, left);
            OH_AVFormat_SetIntValue(newFormat, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, bottom);
            OH_AVFormat_SetIntValue(newFormat, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, right);

            const char *eqPos = strchr(p, '=');
            if (eqPos != NULL && (segmentEnd == NULL || eqPos < segmentEnd)) {
                if (strstr(eqPos, "dqp:") != NULL || strstr(eqPos, "slb:") != NULL) {
                    const char *dqpStr = strstr(eqPos, "dqp:");
                    if (dqpStr != NULL && (segmentEnd == NULL || dqpStr < segmentEnd)) {
                        int dqpVal = 0;
                        if (sscanf(dqpStr, "dqp:%d", &dqpVal) == 1) {
                            OH_AVFormat_SetIntValue(newFormat, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, dqpVal);
                        }
                    }

                    const char *slbStr = strstr(eqPos, "slb:");
                    if (slbStr != NULL && (segmentEnd == NULL || slbStr < segmentEnd)) {
                        int slbVal = 0;
                        if (sscanf(slbStr, "slb:%d", &slbVal) == 1) {
                            OH_AVFormat_SetIntValue(newFormat, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, slbVal);
                        }
                    }
                } else {
                    int dqpVal = 0;
                    if (sscanf(eqPos, "=%d", &dqpVal) == 1) {
                        OH_AVFormat_SetIntValue(newFormat, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, dqpVal);
                    }
                }
            }

            outFormats[parsedNum] = newFormat;
            parsedNum++;
        }

        if (!segmentEnd) {
            break;
        }
        p = segmentEnd + 1;
    }

    *outCount = parsedNum;
    return AV_ERR_OK;
}