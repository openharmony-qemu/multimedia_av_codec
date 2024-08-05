/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef __HEVC_DEC_TYPE_DEF_H__ /* Macro sentry to avoid redundant including */
#define __HEVC_DEC_TYPE_DEF_H__

typedef void *HEVC_DEC_HANDLE;

typedef signed char INT8;
typedef signed int INT32;
typedef unsigned char UINT8;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;

// Log level
typedef enum {
    IHW265VIDEO_ALG_LOG_ERROR = 0,  // log for error
    IHW265VIDEO_ALG_LOG_WARNING,    // log for waring
    IHW265VIDEO_ALG_LOG_INFO,       // log for help
    IHW265VIDEO_ALG_LOG_DEBUG       // print debug info, used for developer debug
} IHW265VIDEO_ALG_LOG_LEVEL;

typedef void (*IHW265D_VIDEO_ALG_LOG_FXN)(UINT32 uiChannelID, IHW265VIDEO_ALG_LOG_LEVEL eLevel, INT8 *pszMsg, ...);

typedef struct TagHevcDecInitParam {
    UINT32 uiChannelID;  // [in] channel ID, used for channel info
    IHW265D_VIDEO_ALG_LOG_FXN logFxn;  // log output callback function
} HEVC_DEC_INIT_PARAM;

typedef struct TagHevcDecInArgs {
    UINT8 *pStream;
    UINT32 uiStreamLen;
    UINT64 uiTimeStamp;
} HEVC_DEC_INARGS;

typedef struct TagHevcDecOutArgs {
    UINT32 uiDecWidth;
    UINT32 uiDecHeight;
    UINT32 uiDecStride;
    UINT32 uiDecBitDepth;
    UINT64 uiTimeStamp;

    UINT8 *pucOutYUV[3];  // YUV address, store YUV in order
} HEVC_DEC_OUTARGS;

#endif // __HEVC_DEC_TYPE_DEF_H__
