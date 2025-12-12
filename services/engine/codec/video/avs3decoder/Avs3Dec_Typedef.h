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

#ifndef __AVS3_DEC_TYPE_DEF_H__ /* Macro sentry to avoid redundant including */
#define __AVS3_DEC_TYPE_DEF_H__

#define MAX_RPLS  32
#define MAX_REFS  17

typedef void *AVS3_DEC_HANDLE;

typedef struct Uavs3dComRpl {
    int num;
    int active;
} ComRpl;

#define FRAME_MAX_PLANES 3
typedef struct Uavs3dIoFrm {
    void* priv;
    int     width[FRAME_MAX_PLANES];                /* width (in unit of pixel) */
    int     height[FRAME_MAX_PLANES];               /* height (in unit of pixel) */
    int     stride[FRAME_MAX_PLANES];               /* buffer stride (in unit of byte) */
    void* buffer[FRAME_MAX_PLANES];               /* address of each plane */

    /* frame info */
    long long   ptr;
    long long   pts;
    long long   dtr;
    long long   dts;
    int         type;
    long long   refpic[2][16];

    /* bitstream */
    unsigned char *bs;
} Uavs3dIoFrm;

#define AVS3_INTER_PIC_START_CODE    0xB6
#define AVS3_INTRA_PIC_START_CODE    0xB3
#define AVS3_SEQ_START_CODE          0xB0
#define AVS3_FIRST_SLICE_START_CODE  0x00
#define AVS3_SEQ_END_CODE            0xB1
#define AVS3_NAL_START_CODE          0x010000

#endif // __AVS3_DEC_TYPE_DEF_H__
