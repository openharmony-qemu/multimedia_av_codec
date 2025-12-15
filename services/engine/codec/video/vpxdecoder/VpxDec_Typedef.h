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

typedef enum vpx_color_space {
    VPX_CS_UNKNOWN = 0,
    VPX_CS_BT_601 = 1,
    VPX_CS_BT_709 = 2,
    VPX_CS_SMPTE_170 = 3,
    VPX_CS_SMPTE_240 = 4,
    VPX_CS_BT_2020 = 5,
    VPX_CS_RESERVED = 6,
    VPX_CS_SRGB = 7
} VpxColorSpace;

typedef enum vpx_color_range {
    VPX_CR_STUDIO_RANGE = 0,
    VPX_CR_FULL_RANGE = 1
} VpxColorRange;

typedef struct TagVpxDecInArgs {
    UINT8 *pStream;
    UINT32 uiStreamLen;
    UINT64 uiTimeStamp;
} VpxDecInArgs;

typedef enum vpx_img_fmt {
    VPX_IMG_FMT_NONE,
    VPX_IMG_FMT_YV12 = 0x301, /**< planar YVU */
    VPX_IMG_FMT_I420 = 0x102,
    VPX_IMG_FMT_I422 = 0x105,
    VPX_IMG_FMT_I444 = 0x106,
    VPX_IMG_FMT_I440 = 0x107,
    VPX_IMG_FMT_NV12 = 0x109,
    VPX_IMG_FMT_I42016 = 0x902,
    VPX_IMG_FMT_I42216 = 0x905,
    VPX_IMG_FMT_I44416 = 0x906,
    VPX_IMG_FMT_I44016 = 0x907
} VpxImageFmt;

typedef struct vpx_image {
    VpxImageFmt fmt;       /**< Image Format */
    VpxColorSpace cs;    /**< Color Space */
    VpxColorRange range; /**< Color Range */

    /* Image storage dimensions */
    UINT32 w;         /**< Stored image width */
    UINT32 h;         /**< Stored image height */
    UINT32 bit_depth; /**< Stored image bit-depth */

    /* Image display dimensions */
    UINT32 d_w; /**< Displayed image width */
    UINT32 d_h; /**< Displayed image height */

    /* Image intended rendering dimensions */
    UINT32 r_w; /**< Intended rendering image width */
    UINT32 r_h; /**< Intended rendering image height */

    /* Chroma subsampling info */
    UINT32 x_chroma_shift; /**< subsampling order, X */
    UINT32 y_chroma_shift; /**< subsampling order, Y */

/* Image data pointers. */
#define VPX_PLANE_PACKED 0  /**< To be used for all packed formats */
#define VPX_PLANE_Y 0       /**< Y (Luminance) plane */
#define VPX_PLANE_U 1       /**< U (Chroma) plane */
#define VPX_PLANE_V 2       /**< V (Chroma) plane */
#define VPX_PLANE_ALPHA 3   /**< A (Transparency) plane */
    UINT8 *planes[4]; /**< pointer to the top left pixel for each plane */
    int stride[4];            /**< stride between rows for each plane */

    int bps; /**< bits per sample (for packed formats) */

    /*!\brief The following member may be set by the application to associate
     * data with this image.
     */
    void *user_priv;

    /* The following members should be treated as private. */
    UINT8 *img_data; /**< private */
    int img_data_owner;      /**< private */
    int self_allocd;         /**< private */

    void *fb_priv; /**< Frame buffer data associated with the image. */
} VpxImage;

#endif // __VPX_DEC_TYPE_DEF_H__
