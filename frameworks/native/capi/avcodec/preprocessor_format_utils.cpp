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

#include "preprocessor_format_utils.h"

namespace OHOS {
namespace MediaAVCodec {

bool GraphicPixelFmtToVideoPixelFmt(GraphicPixelFormat graphicFmt, VideoPixelFormat& outVideoFmt)
{
    switch (graphicFmt) {
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P:
            outVideoFmt = VideoPixelFormat::YUVI420;
            return true;
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888:
            outVideoFmt = VideoPixelFormat::RGBA;
            return true;
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
            outVideoFmt = VideoPixelFormat::NV12;
            return true;
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
            outVideoFmt = VideoPixelFormat::NV21;
            return true;
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010102:
            outVideoFmt = VideoPixelFormat::RGBA1010102;
            return true;
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010:
            outVideoFmt = VideoPixelFormat::NV12;
            return true;
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010:
            outVideoFmt = VideoPixelFormat::NV21;
            return true;
        default:
            return false;
    }
}

} // namespace MediaAVCodec
} // namespace OHOS
