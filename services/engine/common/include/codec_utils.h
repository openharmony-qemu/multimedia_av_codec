/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef CODEC_UTILS_H
#define CODEC_UTILS_H

#include "av_common.h"
#include "avcodec_errors.h"
#include "buffer/avbuffer.h"
#include "fsurface_memory.h"
#include "meta/format.h"
#include "surface.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
};
namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
using AVMemory = Media::AVMemory;
using Format = Media::Format;
const int32_t VIDEO_ALIGN_SIZE = 16;
constexpr uint32_t VIDEO_PIX_DEPTH_RGBA = 4;
struct ScalePara {
    int32_t srcWidth = 0;
    int32_t srcHeight = 0;
    AVPixelFormat srcFfFmt = AVPixelFormat::AV_PIX_FMT_NONE;
    int32_t dstWidth = 0;
    int32_t dstHeight = 0;
    AVPixelFormat dstFfFmt = AVPixelFormat::AV_PIX_FMT_RGBA;
    int32_t align = VIDEO_ALIGN_SIZE;
};
struct Scale {
public:
    int32_t Init(const ScalePara &scalePara, uint8_t **dstData, int32_t *dstLineSize);
    int32_t Convert(uint8_t **srcData, const int32_t *srcLineSize, uint8_t **dstData, int32_t *dstLineSize);

private:
    ScalePara scalePara_;
    std::shared_ptr<SwsContext> swsCtx_ = nullptr;
};
struct SurfaceInfo {
    uint32_t surfaceStride = 0;
    int32_t surfaceFence = 0;
    uint8_t **scaleData = nullptr;
    int32_t *scaleLineSize = nullptr;
};
GraphicPixelFormat TranslateSurfaceFormat(const VideoPixelFormat &surfaceFormat);
VideoPixelFormat ConvertPixelFormatFromFFmpeg(int32_t ffmpegPixelFormat);
AVPixelFormat ConvertPixelFormatToFFmpeg(VideoPixelFormat pixelFormat);
GraphicTransformType TranslateSurfaceRotation(const VideoRotation &rotation);
int32_t ConvertVideoFrame(std::shared_ptr<Scale> *scale, std::shared_ptr<AVFrame> frame, uint8_t **dstData,
                          int32_t *dstLineSize, AVPixelFormat dstPixFmt);
int32_t WriteSurfaceData(const std::shared_ptr<AVMemory> &memory, struct SurfaceInfo &surfaceInfo,
                         const Format &format);
int32_t WriteBufferData(const std::shared_ptr<AVMemory> &memory, uint8_t **scaleData, int32_t *scaleLineSize,
                        const Format &format);

std::string AVStrError(int errnum);
bool IsYuvFormat(VideoPixelFormat &format);
bool IsRgbFormat(VideoPixelFormat &format);
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif