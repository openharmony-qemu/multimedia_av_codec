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

#ifndef __AV1_DEC_TYPE_DEF_H__ /* Macro sentry to avoid redundant including */
#define __AV1_DEC_TYPE_DEF_H__

#include <cstddef>
#include <cstdint>
#include "dav1d.h"

typedef unsigned int UINT32;
typedef unsigned char UINT8;

typedef struct TagAv1DecInArgs {
    UINT8 *pStream;
    UINT32 uiStreamLen;
    int64_t uiTimeStamp;
} AV1_DEC_INARGS;

typedef struct AV1ColorSpaceInfo {
    enum Dav1dColorPrimaries pri; ///< color primaries(av1)
    enum Dav1dTransferCharacteristics trc; ///< transfer characteristics (av1)
    enum Dav1dMatrixCoefficients mtrx; //// matrix coefficiients (av1)
    uint8_t color_range;
} AV1ColorSpaceInfo;
#endif // __AV1_DEC_TYPE_DEF_H__
