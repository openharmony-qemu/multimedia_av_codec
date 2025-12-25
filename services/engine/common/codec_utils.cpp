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
#include "avcodec_trace.h"
#include "media_description.h"
#include "v1_0/cm_color_space.h"
namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CODEC_UTILS"};
constexpr uint32_t INDEX_ARRAY = 2;
constexpr uint32_t WAIT_FENCE_MS = 1000;
constexpr int32_t DOUBLE = 2;
std::map<VideoPixelFormat, AVPixelFormat> g_pixelFormatMap = {
    {VideoPixelFormat::YUVI420, AV_PIX_FMT_YUV420P},
    {VideoPixelFormat::NV12, AV_PIX_FMT_NV12},
    {VideoPixelFormat::NV21, AV_PIX_FMT_NV21},
    {VideoPixelFormat::RGBA, AV_PIX_FMT_RGBA},
};
std::map<ColorPrimary, CM_ColorPrimaries> g_colorPrimariesMap = {
    {COLOR_PRIMARY_BT709, COLORPRIMARIES_BT709},
    {COLOR_PRIMARY_BT601_625, COLORPRIMARIES_BT601_P},
    {COLOR_PRIMARY_BT601_525, COLORPRIMARIES_BT601_N},
    {COLOR_PRIMARY_BT2020, COLORPRIMARIES_BT2020},
    {COLOR_PRIMARY_P3DCI, COLORPRIMARIES_P3_DCI},
    {COLOR_PRIMARY_P3D65, COLORPRIMARIES_P3_D65},
};
std::map<TransferCharacteristic, CM_TransFunc> g_transFuncMap = {
    {TRANSFER_CHARACTERISTIC_BT709, TRANSFUNC_BT709},
    {TRANSFER_CHARACTERISTIC_BT601, TRANSFUNC_BT709},
    {TRANSFER_CHARACTERISTIC_LINEAR, TRANSFUNC_LINEAR},
    {TRANSFER_CHARACTERISTIC_IEC_61966_2_1, TRANSFUNC_SRGB},
    {TRANSFER_CHARACTERISTIC_BT2020_10BIT, TRANSFUNC_BT709},
    {TRANSFER_CHARACTERISTIC_BT2020_12BIT, TRANSFUNC_BT709},
    {TRANSFER_CHARACTERISTIC_PQ, TRANSFUNC_PQ},
    {TRANSFER_CHARACTERISTIC_HLG, TRANSFUNC_HLG},
};
std::map<MatrixCoefficient, CM_Matrix> g_matrixMap = {
    {MATRIX_COEFFICIENT_BT709, MATRIX_BT709},
    {MATRIX_COEFFICIENT_BT601_625, MATRIX_BT601_P},
    {MATRIX_COEFFICIENT_BT601_525, MATRIX_BT601_N},
    {MATRIX_COEFFICIENT_BT2020_NCL, MATRIX_BT2020},
    {MATRIX_COEFFICIENT_ICTCP, MATRIX_BT2100_ICTCP},
};
} // namespace

using namespace OHOS::Media;
// for surface parameter check
bool IsValidPixelFormat(int32_t val)
{
    return val >= static_cast<int32_t>(VideoPixelFormat::YUVI420) &&
           val <= static_cast<int32_t>(VideoPixelFormat::RGBA) &&
           val != static_cast<int32_t>(VideoPixelFormat::SURFACE_FORMAT);
}

bool IsValidScaleType(int32_t val)
{
    return val == static_cast<int32_t>(ScalingMode::SCALING_MODE_SCALE_TO_WINDOW) ||
           val == static_cast<int32_t>(ScalingMode::SCALING_MODE_SCALE_CROP);
}

bool IsValidRotation(int32_t val)
{
    return val == static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_0) ||
           val == static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_90) ||
           val == static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_180) ||
           val == static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_270);
}

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

int32_t ConvertVideoFrame(std::shared_ptr<Scale> *scale,
                          uint8_t **srcData, int32_t *srcLineSize, AVPixelFormat srcPixFmt,
                          int32_t srcWidth, int32_t srcHeight,
                          uint8_t **dstData, int32_t *dstLineSize, AVPixelFormat dstPixFmt)
{
    if (*scale == nullptr) {
        *scale = std::make_shared<Scale>();
        ScalePara scalePara{srcWidth, srcHeight, srcPixFmt,
                            srcWidth, srcHeight, dstPixFmt};
        CHECK_AND_RETURN_RET_LOG((*scale)->Init(scalePara, dstData, dstLineSize) == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
                                 "Scale init error");
    }
    return (*scale)->Convert(srcData, srcLineSize, dstData, dstLineSize);
}

int32_t MemWritePlaneDataStride(const std::shared_ptr<AVMemory> &memory, uint8_t* srcData, int32_t srcStride,
                                int32_t dstStride, int32_t height)
{
    CHECK_AND_RETURN_RET_LOG(memory != nullptr && srcData != nullptr, AVCS_ERR_INVALID_VAL,
        "memory or srcData is nullptr");
    int32_t srcPos = 0;
    int32_t dstPos = memory->GetSize();
    int32_t writeSize = srcStride > dstStride ? dstStride : srcStride;
    for (int32_t colNum = 0; colNum < height; colNum++) {
        CHECK_AND_RETURN_RET_LOG(memory->Write(srcData + srcPos, writeSize, dstPos) > 0,
            AVCS_ERR_NO_MEMORY, "memory Write Data failed");
        dstPos += dstStride;
        srcPos += srcStride;
    }
    CHECK_AND_RETURN_RET_LOG(memory->SetSize(dstPos) == Status::OK, AVCS_ERR_NO_MEMORY, "memory SetSize failed");
    return AVCS_ERR_OK;
}

int32_t WriteYuvDataStride(const std::shared_ptr<AVMemory> &memory, uint8_t **scaleData, const int32_t *scaleLineSize,
                           int32_t stride, const Format &format)
{
    int32_t height;
    int32_t fmt;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, fmt);
    VideoPixelFormat pixFmt = static_cast<VideoPixelFormat>(fmt);
    CHECK_AND_RETURN_RET_LOG(pixFmt == VideoPixelFormat::YUVI420 || pixFmt == VideoPixelFormat::NV12 ||
                                 pixFmt == VideoPixelFormat::NV21,
                             AVCS_ERR_UNSUPPORT, "pixFmt: %{public}d do not support", pixFmt);
    std::string traceTitle = "stride(" + std::to_string(stride) + ")_height(" + std::to_string(height) + ")";
    AVCodecTrace trace("WriteYuvByStride_pixfmt(" + std::to_string(fmt) + ")_" + traceTitle);
    int32_t ret = MemWritePlaneDataStride(memory, scaleData[0], scaleLineSize[0], stride, height);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "memory Write Data failed");
    stride = ((stride + UV_SCALE_FACTOR - 1) / UV_SCALE_FACTOR) * UV_SCALE_FACTOR;
    height = ((height + UV_SCALE_FACTOR - 1) / UV_SCALE_FACTOR) * UV_SCALE_FACTOR;
    if (pixFmt == VideoPixelFormat::YUVI420) {
        ret = MemWritePlaneDataStride(memory, scaleData[1], scaleLineSize[1],
            static_cast<int32_t>(stride / UV_SCALE_FACTOR), static_cast<int32_t>(height / UV_SCALE_FACTOR));
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "memory Write Data failed");
        ret = MemWritePlaneDataStride(memory, scaleData[INDEX_ARRAY], scaleLineSize[1],
            static_cast<int32_t>(stride / UV_SCALE_FACTOR), static_cast<int32_t>(height / UV_SCALE_FACTOR));
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "memory Write Data failed");
    } else if ((pixFmt == VideoPixelFormat::NV12) || (pixFmt == VideoPixelFormat::NV21)) {
        ret = MemWritePlaneDataStride(memory, scaleData[1], scaleLineSize[1], stride,
            static_cast<int32_t>(height / UV_SCALE_FACTOR));
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "memory Write Data failed");
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
    std::string traceTitle = "stride(" + std::to_string(stride) + ")_height(" + std::to_string(height) + ")";
    AVCodecTrace trace("WriteRgbByStride_" + traceTitle);
    for (int32_t colNum = 0; colNum < height; colNum++) {
        CHECK_AND_RETURN_RET_LOG(memory->Write(scaleData[0] + srcPos, writeSize, dstPos) > 0,
            AVCS_ERR_NO_MEMORY, "memory Write Data failed");
        dstPos += stride;
        srcPos += dataSize;
    }
    AVCODEC_LOGD("WriteRgbDataStride success");
    return AVCS_ERR_OK;
}

int32_t WriteYuvData(const std::shared_ptr<AVMemory> &memory, uint8_t **scaleData, const int32_t *scaleLineSize,
                     int32_t &height, VideoPixelFormat &pixFmt)
{
    AVCODEC_SYNC_TRACE;
    int32_t ySize = static_cast<int32_t>(scaleLineSize[0] * height);      // yuv420: 411 nv21
    int32_t uvSize = static_cast<int32_t>(scaleLineSize[1] * height / 2); // 2
    int32_t frameSize = 0;
    if (pixFmt == VideoPixelFormat::YUVI420) {
        frameSize = ySize + (uvSize * 2); // 2
    } else if (pixFmt == VideoPixelFormat::NV21 || pixFmt == VideoPixelFormat::NV12) {
        frameSize = ySize + uvSize;
    }
    CHECK_AND_RETURN_RET_LOG(memory->GetCapacity() >= frameSize, AVCS_ERR_NO_MEMORY,
                             "output buffer size is not enough: real[%{public}d], need[%{public}u]",
                             memory->GetCapacity(), frameSize);
    if (pixFmt == VideoPixelFormat::YUVI420) {
        CHECK_AND_RETURN_RET_LOG(memory->Write(scaleData[0], ySize) > 0,
            AVCS_ERR_NO_MEMORY, "memory Write Data failed");
        CHECK_AND_RETURN_RET_LOG(memory->Write(scaleData[1], uvSize) > 0,
            AVCS_ERR_NO_MEMORY, "memory Write Data failed");
        CHECK_AND_RETURN_RET_LOG(memory->Write(scaleData[INDEX_ARRAY], uvSize) > 0,
            AVCS_ERR_NO_MEMORY, "memory Write Data failed");
    } else if ((pixFmt == VideoPixelFormat::NV12) || (pixFmt == VideoPixelFormat::NV21)) {
        CHECK_AND_RETURN_RET_LOG(memory->Write(scaleData[0], ySize) > 0,
            AVCS_ERR_NO_MEMORY, "memory Write Data failed");
        CHECK_AND_RETURN_RET_LOG(memory->Write(scaleData[1], uvSize) > 0,
            AVCS_ERR_NO_MEMORY, "memory Write Data failed");
    } else {
        return AVCS_ERR_UNSUPPORT;
    }
    return AVCS_ERR_OK;
}

int32_t WriteRgbData(const std::shared_ptr<AVMemory> &memory, uint8_t **scaleData, const int32_t *scaleLineSize,
                     int32_t &height)
{
    AVCODEC_SYNC_TRACE;
    int32_t frameSize = static_cast<int32_t>(scaleLineSize[0] * height);
    CHECK_AND_RETURN_RET_LOG(memory->GetCapacity() >= frameSize, AVCS_ERR_NO_MEMORY,
                             "output buffer size is not enough: real[%{public}d], need[%{public}u]",
                             memory->GetCapacity(), frameSize);
    CHECK_AND_RETURN_RET_LOG(memory->Write(scaleData[0], frameSize) > 0,
        AVCS_ERR_NO_MEMORY, "memory Write Data failed");
    return AVCS_ERR_OK;
}

int32_t WriteSurfaceData(const std::shared_ptr<AVMemory> &memory, struct SurfaceInfo &surfaceInfo, const Format &format)
{
    int32_t height;
    int32_t fmt;
    CHECK_AND_RETURN_RET_LOG(format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height) && height > 0,
                             AVCS_ERR_INVALID_VAL, "Invalid height %{public}d!", height);
    CHECK_AND_RETURN_RET_LOG(format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, fmt) &&
                             fmt >= static_cast<int32_t>(VideoPixelFormat::YUV420P) &&
                             fmt <= static_cast<int32_t>(VideoPixelFormat::RGBA),
                             AVCS_ERR_INVALID_VAL, "Cannot get pixel format");
    VideoPixelFormat pixFmt = static_cast<VideoPixelFormat>(fmt);
    AVCODEC_SYNC_TRACE;
    if (surfaceInfo.surfaceFence != nullptr) {
        int32_t waitRes = surfaceInfo.surfaceFence->Wait(WAIT_FENCE_MS);
        EXPECT_AND_LOGD(waitRes != 0, "wait fence time out, cost more than %{public}u ms", WAIT_FENCE_MS);
    }
    uint32_t yScaleLineSize = static_cast<uint32_t>(surfaceInfo.scaleLineSize[0]);
    uint32_t uScaleLineSize = static_cast<uint32_t>(surfaceInfo.scaleLineSize[1]);
    if (IsYuvFormat(pixFmt)) {
        if (surfaceInfo.surfaceStride != yScaleLineSize || (uScaleLineSize * DOUBLE) != surfaceInfo.surfaceStride) {
            return WriteYuvDataStride(memory, surfaceInfo.scaleData, surfaceInfo.scaleLineSize,
                                      surfaceInfo.surfaceStride, format);
        }
        return WriteYuvData(memory, surfaceInfo.scaleData, surfaceInfo.scaleLineSize, height, pixFmt);
    } else if (IsRgbFormat(pixFmt)) {
        if (surfaceInfo.surfaceStride != yScaleLineSize) {
            return WriteRgbDataStride(memory, surfaceInfo.scaleData, surfaceInfo.scaleLineSize,
                                      surfaceInfo.surfaceStride, format);
        }
        return WriteRgbData(memory, surfaceInfo.scaleData, surfaceInfo.scaleLineSize, height);
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
    CHECK_AND_RETURN_RET_LOG(format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height) && height > 0,
                             AVCS_ERR_INVALID_VAL, "Invalid height %{public}d!", height);
    CHECK_AND_RETURN_RET_LOG(format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width) && width > 0,
                             AVCS_ERR_INVALID_VAL, "Invalid width %{public}d!", width);
    CHECK_AND_RETURN_RET_LOG(format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, fmt) &&
                             fmt >= static_cast<int32_t>(VideoPixelFormat::YUV420P) &&
                             fmt <= static_cast<int32_t>(VideoPixelFormat::RGBA),
                             AVCS_ERR_INVALID_VAL, "Cannot get pixel format");
    VideoPixelFormat pixFmt = static_cast<VideoPixelFormat>(fmt);
    AVCODEC_SYNC_TRACE;
    if (IsYuvFormat(pixFmt)) {
        if (scaleLineSize[0] != width || (scaleLineSize[1] * DOUBLE) != width) {
            return WriteYuvDataStride(memory, scaleData, scaleLineSize, width, format);
        }
        return WriteYuvData(memory, scaleData, scaleLineSize, height, pixFmt);
    } else if (IsRgbFormat(pixFmt)) {
        if (scaleLineSize[0] != width) {
            return WriteRgbDataStride(memory, scaleData, scaleLineSize, width * VIDEO_PIX_DEPTH_RGBA, format);
        }
        return WriteRgbData(memory, scaleData, scaleLineSize, height);
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
        case VideoPixelFormat::YUVI420: {
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

int32_t ConvertParamsToColorSpaceInfo(uint32_t fullRangeFlag, uint32_t colorPrimaries,
                                      uint32_t transferCharacteristic, uint32_t matrixCoeffs,
                                      std::vector<uint8_t> &colorSpaceInfoData)
{
    colorSpaceInfoData.resize(sizeof(CM_ColorSpaceInfo));
    CM_ColorSpaceInfo* colorSpaceInfo = reinterpret_cast<CM_ColorSpaceInfo*>(colorSpaceInfoData.data());
    CHECK_AND_RETURN_RET_LOG(colorSpaceInfo != nullptr, AVCS_ERR_INVALID_VAL, "colorSpaceInfo is nullptr");
    if (!g_colorPrimariesMap.count(static_cast<ColorPrimary>(colorPrimaries))) {
        AVCODEC_LOGE("unsupported colorPrimaries: %{public}u", colorPrimaries);
        return AVCS_ERR_UNSUPPORT;
    }
    if (!g_transFuncMap.count(static_cast<TransferCharacteristic>(transferCharacteristic))) {
        AVCODEC_LOGE("unsupported transferCharacteristic: %{public}u", transferCharacteristic);
        return AVCS_ERR_UNSUPPORT;
    }
    if (!g_matrixMap.count(static_cast<MatrixCoefficient>(matrixCoeffs))) {
        AVCODEC_LOGE("unsupported matrixCoeffs: %{public}u", matrixCoeffs);
        return AVCS_ERR_UNSUPPORT;
    }
    colorSpaceInfo->primaries = g_colorPrimariesMap[static_cast<ColorPrimary>(colorPrimaries)];
    colorSpaceInfo->transfunc = g_transFuncMap[static_cast<TransferCharacteristic>(transferCharacteristic)];
    colorSpaceInfo->matrix = g_matrixMap[static_cast<MatrixCoefficient>(matrixCoeffs)];
    if (fullRangeFlag) {
        colorSpaceInfo->range = RANGE_FULL;
    } else {
        colorSpaceInfo->range = RANGE_LIMITED;
    }
    return AVCS_ERR_OK;
}

uint32_t GetMetaDataTypeByTransFunc(uint32_t transferCharacteristic)
{
    switch (static_cast<TransferCharacteristic>(transferCharacteristic)) {
        case TRANSFER_CHARACTERISTIC_PQ: {
            return static_cast<uint32_t>(CM_VIDEO_HDR10);
        }
        case TRANSFER_CHARACTERISTIC_HLG: {
            return static_cast<uint32_t>(CM_VIDEO_HLG);
        }
        default: {
            return static_cast<uint32_t>(CM_METADATA_NONE);
        }
    }
    return static_cast<uint32_t>(CM_METADATA_NONE);
}

bool IsYuvFormat(VideoPixelFormat &format)
{
    return (format == VideoPixelFormat::YUVI420 || format == VideoPixelFormat::NV12 ||
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