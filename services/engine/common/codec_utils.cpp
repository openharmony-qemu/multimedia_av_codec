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

#include "codec_utils.h"
#include "avcodec_log.h"
#include "media_description.h"
namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "FCodec"};
constexpr uint32_t INDEX_ARRAY = 2;
std::map<VideoPixelFormat, AVPixelFormat> g_pixelFormatMap = {
    {VideoPixelFormat::YUV420P, AV_PIX_FMT_YUV420P},
    {VideoPixelFormat::NV12, AV_PIX_FMT_NV12},
    {VideoPixelFormat::NV21, AV_PIX_FMT_NV21},
    {VideoPixelFormat::RGBA, AV_PIX_FMT_RGBA},
};
} // namespace

using namespace OHOS::Media;
int32_t ConvertVideoFrame(std::shared_ptr<Scale> *scale, std::shared_ptr<AVFrame> frame, uint8_t **dstData,
                          int32_t *dstLineSize, AVPixelFormat dstPixFmt)
{
    if (*scale == nullptr) {
        *scale = std::make_shared<Scale>();
        ScalePara scalePara{static_cast<int32_t>(frame->width),        static_cast<int32_t>(frame->height),
                            static_cast<AVPixelFormat>(frame->format), static_cast<int32_t>(frame->width),
                            static_cast<int32_t>(frame->height),       dstPixFmt};
        CHECK_AND_RETURN_RET_LOG((*scale)->Init(scalePara, dstData, dstLineSize) == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
                                 "Scale init error");
    }
    return (*scale)->Convert(frame->data, frame->linesize, dstData, dstLineSize);
}

int32_t WriteYuvDataStride(const std::shared_ptr<AVMemory> &memory, uint8_t **scaleData, const int32_t *scaleLineSize,
                           int32_t stride, const Format &format)
{
    int32_t height;
    int32_t fmt;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, fmt);
    VideoPixelFormat pixFmt = static_cast<VideoPixelFormat>(fmt);
    CHECK_AND_RETURN_RET_LOG(pixFmt == VideoPixelFormat::YUV420P || pixFmt == VideoPixelFormat::NV12 ||
                                 pixFmt == VideoPixelFormat::NV21,
                             AVCS_ERR_UNSUPPORT, "pixFmt: %{public}d do not support", pixFmt);
    int32_t srcPos = 0;
    int32_t dstPos = 0;
    int32_t dataSize = scaleLineSize[0];
    int32_t writeSize = dataSize > stride ? stride : dataSize;
    for (int32_t colNum = 0; colNum < height; colNum++) {
        memory->Write(scaleData[0] + srcPos, writeSize, dstPos);
        dstPos += stride;
        srcPos += dataSize;
    }
    srcPos = 0;
    if (pixFmt == VideoPixelFormat::YUV420P) {
        dataSize = scaleLineSize[1];
        writeSize = dataSize > (stride >> 1) ? (stride >> 1) : dataSize;
        for (int32_t colNum = 0; colNum < (height >> 1); colNum++) {
            memory->Write(scaleData[1] + srcPos, writeSize, dstPos);
            dstPos += (stride >> 1);
            srcPos += dataSize;
        }
        srcPos = 0;
        for (int32_t colNum = 0; colNum < (height >> 1); colNum++) {
            memory->Write(scaleData[INDEX_ARRAY] + srcPos, writeSize, dstPos);
            dstPos += (stride >> 1);
            srcPos += dataSize;
        }
    } else if ((pixFmt == VideoPixelFormat::NV12) || (pixFmt == VideoPixelFormat::NV21)) {
        dataSize = scaleLineSize[1];
        writeSize = dataSize > stride ? stride : dataSize;
        for (int32_t colNum = 0; colNum < (height >> 1); colNum++) {
            memory->Write(scaleData[1] + srcPos, writeSize, dstPos);
            dstPos += stride;
            srcPos += dataSize;
        }
    }
    AVCODEC_LOGD("WriteYuvDataStride success");
    return AVCS_ERR_OK;
}

int32_t WriteRgbDataStride(const std::shared_ptr<AVMemory> &memory, uint8_t **scaleData, const int32_t *scaleLineSize,
                           int32_t stride, const Format &format)
{
    int32_t height;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    int32_t srcPos = 0;
    int32_t dstPos = 0;
    int32_t dataSize = scaleLineSize[0];
    int32_t writeSize = dataSize > stride ? stride : dataSize;
    for (int32_t colNum = 0; colNum < height; colNum++) {
        memory->Write(scaleData[0] + srcPos, writeSize, dstPos);
        dstPos += stride;
        srcPos += dataSize;
    }

    AVCODEC_LOGD("WriteRgbDataStride success");
    return AVCS_ERR_OK;
}

int32_t WriteYuvData(const std::shared_ptr<AVMemory> &memory, uint8_t **scaleData, const int32_t *scaleLineSize,
                     int32_t &height, VideoPixelFormat &pixFmt)
{
    int32_t ySize = static_cast<int32_t>(scaleLineSize[0] * height);      // yuv420: 411 nv21
    int32_t uvSize = static_cast<int32_t>(scaleLineSize[1] * height / 2); // 2
    int32_t frameSize = 0;
    if (pixFmt == VideoPixelFormat::YUV420P) {
        frameSize = ySize + (uvSize * 2); // 2
    } else if (pixFmt == VideoPixelFormat::NV21 || pixFmt == VideoPixelFormat::NV12) {
        frameSize = ySize + uvSize;
    }
    CHECK_AND_RETURN_RET_LOG(memory->GetCapacity() >= frameSize, AVCS_ERR_NO_MEMORY,
                             "output buffer size is not enough: real[%{public}d], need[%{public}u]",
                             memory->GetCapacity(), frameSize);
    if (pixFmt == VideoPixelFormat::YUV420P) {
        memory->Write(scaleData[0], ySize);
        memory->Write(scaleData[1], uvSize);
        memory->Write(scaleData[2], uvSize); // 2
    } else if ((pixFmt == VideoPixelFormat::NV12) || (pixFmt == VideoPixelFormat::NV21)) {
        memory->Write(scaleData[0], ySize);
        memory->Write(scaleData[1], uvSize);
    } else {
        return AVCS_ERR_UNSUPPORT;
    }
    return AVCS_ERR_OK;
}

int32_t WriteRgbData(const std::shared_ptr<AVMemory> &memory, uint8_t **scaleData, const int32_t *scaleLineSize,
                     int32_t &height)
{
    int32_t frameSize = static_cast<int32_t>(scaleLineSize[0] * height);
    CHECK_AND_RETURN_RET_LOG(memory->GetCapacity() >= frameSize, AVCS_ERR_NO_MEMORY,
                             "output buffer size is not enough: real[%{public}d], need[%{public}u]",
                             memory->GetCapacity(), frameSize);
    memory->Write(scaleData[0], frameSize);
    return AVCS_ERR_OK;
}

int32_t WriteSurfaceData(const std::shared_ptr<AVMemory> &memory, struct SurfaceInfo &surfaceInfo, const Format &format)
{
    int32_t height;
    int32_t fmt;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, fmt);
    VideoPixelFormat pixFmt = static_cast<VideoPixelFormat>(fmt);

    sptr<SyncFence> autoFence = new (std::nothrow) SyncFence(surfaceInfo.surfaceFence);
    if (autoFence != nullptr) {
        autoFence->Wait(100); // 100ms
    }

    if (IsYuvFormat(pixFmt)) {
        if (surfaceInfo.surfaceStride % surfaceInfo.scaleLineSize[0]) {
            return WriteYuvDataStride(memory, surfaceInfo.scaleData, surfaceInfo.scaleLineSize,
                                      surfaceInfo.surfaceStride, format);
        }
        WriteYuvData(memory, surfaceInfo.scaleData, surfaceInfo.scaleLineSize, height, pixFmt);
    } else if (IsRgbFormat(pixFmt)) {
        if (surfaceInfo.surfaceStride % surfaceInfo.scaleLineSize[0]) {
            return WriteRgbDataStride(memory, surfaceInfo.scaleData, surfaceInfo.scaleLineSize,
                                      surfaceInfo.surfaceStride, format);
        }
        WriteRgbData(memory, surfaceInfo.scaleData, surfaceInfo.scaleLineSize, height);
    } else {
        AVCODEC_LOGE("Fill frame buffer failed : unsupported pixel format: %{public}d", pixFmt);
        return AVCS_ERR_UNSUPPORT;
    }
    return AVCS_ERR_OK;
}

int32_t WriteBufferData(const std::shared_ptr<AVMemory> &memory, uint8_t **scaleData, int32_t *scaleLineSize,
                        const Format &format)
{
    int32_t height;
    int32_t width;
    int32_t fmt;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, fmt);
    VideoPixelFormat pixFmt = static_cast<VideoPixelFormat>(fmt);

    if (IsYuvFormat(pixFmt)) {
        if (scaleLineSize[0] % width) {
            return WriteYuvDataStride(memory, scaleData, scaleLineSize, width, format);
        }
        WriteYuvData(memory, scaleData, scaleLineSize, height, pixFmt);
    } else if (IsRgbFormat(pixFmt)) {
        if (scaleLineSize[0] % width) {
            return WriteRgbDataStride(memory, scaleData, scaleLineSize, width * VIDEO_PIX_DEPTH_RGBA, format);
        }
        WriteRgbData(memory, scaleData, scaleLineSize, height);
    } else {
        AVCODEC_LOGE("Fill frame buffer failed : unsupported pixel format: %{public}d", pixFmt);
        return AVCS_ERR_UNSUPPORT;
    }
    return AVCS_ERR_OK;
}

std::string AVStrError(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}

GraphicTransformType TranslateSurfaceRotation(const VideoRotation &rotation)
{
    switch (rotation) {
        case VideoRotation::VIDEO_ROTATION_90: {
            return GRAPHIC_ROTATE_270;
        }
        case VideoRotation::VIDEO_ROTATION_180: {
            return GRAPHIC_ROTATE_180;
        }
        case VideoRotation::VIDEO_ROTATION_270: {
            return GRAPHIC_ROTATE_90;
        }
        default:
            return GRAPHIC_ROTATE_NONE;
    }
}

GraphicPixelFormat TranslateSurfaceFormat(const VideoPixelFormat &surfaceFormat)
{
    switch (surfaceFormat) {
        case VideoPixelFormat::YUV420P: {
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P;
        }
        case VideoPixelFormat::RGBA: {
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888;
        }
        case VideoPixelFormat::NV12: {
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP;
        }
        case VideoPixelFormat::NV21: {
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP;
        }
        default:
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT;
    }
}

VideoPixelFormat ConvertPixelFormatFromFFmpeg(int32_t ffmpegPixelFormat)
{
    auto iter = std::find_if(
        g_pixelFormatMap.begin(), g_pixelFormatMap.end(),
        [&](const std::pair<VideoPixelFormat, AVPixelFormat> &tmp) -> bool { return tmp.second == ffmpegPixelFormat; });
    return iter == g_pixelFormatMap.end() ? VideoPixelFormat::UNKNOWN : iter->first;
}

AVPixelFormat ConvertPixelFormatToFFmpeg(VideoPixelFormat pixelFormat)
{
    auto iter = std::find_if(
        g_pixelFormatMap.begin(), g_pixelFormatMap.end(),
        [&](const std::pair<VideoPixelFormat, AVPixelFormat> &tmp) -> bool { return tmp.first == pixelFormat; });
    return iter == g_pixelFormatMap.end() ? AV_PIX_FMT_NONE : iter->second;
}

bool IsYuvFormat(VideoPixelFormat &format)
{
    return (format == VideoPixelFormat::YUV420P || format == VideoPixelFormat::NV12 ||
            format == VideoPixelFormat::NV21);
}

bool IsRgbFormat(VideoPixelFormat &format)
{
    return (format == VideoPixelFormat::RGBA);
}

int32_t Scale::Init(const ScalePara &scalePara, uint8_t **dstData, int32_t *dstLineSize)
{
    scalePara_ = scalePara;
    if (swsCtx_ != nullptr) {
        return AVCS_ERR_OK;
    }
    auto swsContext =
        sws_getContext(scalePara_.srcWidth, scalePara_.srcHeight, scalePara_.srcFfFmt, scalePara_.dstWidth,
                       scalePara_.dstHeight, scalePara_.dstFfFmt, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    if (swsContext == nullptr) {
        return AVCS_ERR_UNKNOWN;
    }
    swsCtx_ = std::shared_ptr<SwsContext>(swsContext, [](struct SwsContext *ptr) {
        if (ptr != nullptr) {
            sws_freeContext(ptr);
        }
    });
    auto ret = av_image_alloc(dstData, dstLineSize, scalePara_.dstWidth, scalePara_.dstHeight, scalePara_.dstFfFmt,
                              scalePara_.align);
    if (ret < 0) {
        return AVCS_ERR_UNKNOWN;
    }
    for (int32_t i = 0; dstLineSize[i] > 0; i++) {
        if (dstData[i] && !dstLineSize[i]) {
            return AVCS_ERR_UNKNOWN;
        }
    }
    return AVCS_ERR_OK;
}

int32_t Scale::Convert(uint8_t **srcData, const int32_t *srcLineSize, uint8_t **dstData, int32_t *dstLineSize)
{
    auto res = sws_scale(swsCtx_.get(), srcData, srcLineSize, 0, scalePara_.srcHeight, dstData, dstLineSize);
    if (res < 0) {
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS