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

#ifndef __VPX_DEC_TYPE_DEF_H__ /* Macro sentry to avoid redundant including */
#define __VPX_DEC_TYPE_DEF_H__

#include <cstddef>
#include <cstdint>
#include <csetjmp>
#ifndef DECLARE_ALIGNED
#if defined(__GNUC__) || defined(__SUNPRO_C)
#define DECLARE_ALIGNED(n, typ, val) typ val __attribute__((aligned(n)))
#elif defined(_MSC_VER)
#define DECLARE_ALIGNED(n, typ, val) __declspec(align(n)) typ val
#else
#warning No alignment directives known for this compiler.
#define DECLARE_ALIGNED(n, typ, val) typ val
#endif
#endif

#define MAX_FB_MT_DEC 32

typedef int INT32;
typedef unsigned int UINT32;
typedef unsigned char UINT8;
typedef void *VPX_DEC_HANDLE;
typedef unsigned long long UINT64;

typedef struct TagVpxDecInArgs {
    UINT8 *pStream;
    UINT32 uiStreamLen;
    UINT64 uiTimeStamp;
} VpxDecInArgs;
#endif // __VPX_DEC_TYPE_DEF_H__
