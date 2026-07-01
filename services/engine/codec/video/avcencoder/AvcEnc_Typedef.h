/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef __AVC_ENC_TYPE_DEF_H__ /* Macro sentry to avoid redundant including */
#define __AVC_ENC_TYPE_DEF_H__

#define IV_MAX_RAW_COMPONENTS 4
typedef void *AVC_ENC_HANDLE;

// Log level
typedef enum {
    IHW264VIDEO_ALG_LOG_DEBUG = 0,  // print debug info, used for developer debug
    IHW264VIDEO_ALG_LOG_INFO,       // log for help
    IHW264VIDEO_ALG_LOG_WARNING,    // log for warning
    IHW264VIDEO_ALG_LOG_ERROR,      // log for error
} IHW264VIDEO_ALG_LOG_LEVEL;

typedef void (*IHW264E_VIDEO_ALG_LOG_FXN)(uint32_t uiChannelID, IHW264VIDEO_ALG_LOG_LEVEL eLevel, uint8_t *pszMsg, ...);

typedef enum {
    YUV_420P     = 0x0,
    YUV_420SP_UV = 0x1,
    YUV_420SP_VU = 0x2,
} COLOR_FORMAT;

typedef enum {
    MODE_CQP     = 0x0,
    MODE_CBR     = 0x1,
    MODE_VBR     = 0x2,
} ENC_MODE;

typedef enum {
    PROFILE_BASE        = 0x0,
    PROFILE_MAIN        = 0x1,
    PROFILE_HIGH        = 0x2,
    PROFILE_SIMPLE      = 0x3,
    PROFILE_ADVSIMPLE   = 0x4,
} ENC_PROFILE;

typedef struct TagAvcEncInitParam {
    uint32_t uiChannelID = 0;
    IHW264E_VIDEO_ALG_LOG_FXN logFxn = nullptr;
    uint32_t  width = 0;
    uint32_t  height = 0;
    ENC_MODE  encMode = ENC_MODE::MODE_CBR;
    uint32_t  frameRate = 0;
    uint32_t  bitrate = 0;
    uint32_t  qp = 0;
    uint32_t  qpMax = 0;
    uint32_t  qpMin = 0;
    uint32_t  iperiod = 0;
    COLOR_FORMAT colorFmt = COLOR_FORMAT::YUV_420P;
    uint8_t   range = 0;
    uint8_t   primaries = 0;
    uint8_t   transfer = 0;
    uint8_t   matrix = 0;
    uint32_t  level = 0;
    ENC_PROFILE profile = ENC_PROFILE::PROFILE_BASE;
} AVC_ENC_INIT_PARAM;

typedef struct ReconfigArgs {
    uint32_t  bitrate = 0;
    uint32_t  idrRequest = 0;
} RECONFIG_ARGS;

typedef struct TagAvcEncInArgs {
    COLOR_FORMAT colorFmt = COLOR_FORMAT::YUV_420P;
    uint64_t timestamp = 0;
    void *inputBufs[IV_MAX_RAW_COMPONENTS] = {nullptr};  // YUV address, store YUV in order
    uint32_t width[IV_MAX_RAW_COMPONENTS] = {0};
    uint32_t height[IV_MAX_RAW_COMPONENTS] = {0};
    uint32_t stride[IV_MAX_RAW_COMPONENTS] = {0};
    RECONFIG_ARGS configArgs;
} AVC_ENC_INARGS;

typedef struct TagAvcEncOutArgs {
    uint32_t encodedFrameType = 0;
    uint32_t isLast = 0;
    void *streamBuf = nullptr;
    uint32_t size = 0;
    uint32_t bytes = 0;
    uint64_t timestamp = 0;
} AVC_ENC_OUTARGS;
#endif // __AVC_ENC_TYPE_DEF_H__
