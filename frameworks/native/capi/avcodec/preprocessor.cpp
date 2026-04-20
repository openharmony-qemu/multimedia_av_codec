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

#include "preprocessor.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "securec.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodeclist_impl.h"
#include "meta/meta_key.h"
#include "native_avcodec_base.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Preprocessor"};

struct PlaneCopyInfo {
    uint32_t planeCount{0};
    uint32_t rowBytes[3] = {0};
    uint32_t rowCounts[3] = {0};
};

double RoundToTwoDecimals(double value)
{
    return std::round(value * 100.0) / 100.0; // 100.0: Round to 2 decimal places using round half up
}

bool GetPlaneCopyInfo(GraphicPixelFormat format, uint32_t width, uint32_t height, PlaneCopyInfo &copyInfo)
{
    switch (format) {
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888:
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010102: {
            copyInfo.planeCount = 1;
            copyInfo.rowBytes[0] = width * 4; // RGBA8888/RGBA1010102 both occupy 4 bytes per pixel.
            copyInfo.rowCounts[0] = height;
            return true;
        }
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP: {
            copyInfo.planeCount = 2; // 2: Y plane + interleaved UV plane
            copyInfo.rowBytes[0] = width;
            copyInfo.rowCounts[0] = height;
            copyInfo.rowBytes[1] = width;
            copyInfo.rowCounts[1] = (height + 1) / 2; // 2: UV height = Y height / 2
            return true;
        }
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010: {
            copyInfo.planeCount = 2; // 2: Y plane + interleaved UV plane
            copyInfo.rowBytes[0] = width * 2; // 2: 10-bit samples are stored in 16-bit containers.
            copyInfo.rowCounts[0] = height;
            copyInfo.rowBytes[1] = width * 2; // 2: 10-bit samples are stored in 16-bit containers.
            copyInfo.rowCounts[1] = (height + 1) / 2; // 2: UV height = Y height / 2
            return true;
        }
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P: {
            copyInfo.planeCount = 3; // 3: Y plane + U plane + V plane
            copyInfo.rowBytes[0] = width;
            copyInfo.rowCounts[0] = height;
            copyInfo.rowBytes[1] = (width + 1) / 2; // 2: UV width = Y width / 2
            copyInfo.rowCounts[1] = (height + 1) / 2; // 2: UV height = Y height / 2
            copyInfo.rowBytes[2] = copyInfo.rowBytes[1]; // 2: index
            copyInfo.rowCounts[2] = copyInfo.rowCounts[1]; // 2: index
            return true;
        }
        default:
            return false;
    }
}

bool CopyByPlanes(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output, uint8_t *src, uint8_t *dst)
{
    OH_NativeBuffer_Planes *srcPlanes = nullptr;
    OH_NativeBuffer_Planes *dstPlanes = nullptr;
    GSError srcErr = input->GetPlanesInfo(reinterpret_cast<void **>(&srcPlanes));
    GSError dstErr = output->GetPlanesInfo(reinterpret_cast<void **>(&dstPlanes));
    if (srcErr != GSERROR_OK || dstErr != GSERROR_OK || srcPlanes == nullptr || dstPlanes == nullptr) {
        return false;
    }

    uint32_t width = static_cast<uint32_t>(std::min(input->GetWidth(), output->GetWidth()));
    uint32_t height = static_cast<uint32_t>(std::min(input->GetHeight(), output->GetHeight()));
    PlaneCopyInfo copyInfo;
    if (!GetPlaneCopyInfo(static_cast<GraphicPixelFormat>(input->GetFormat()), width, height, copyInfo)) {
        return false;
    }
    if (srcPlanes->planeCount < copyInfo.planeCount || dstPlanes->planeCount < copyInfo.planeCount) {
        return false;
    }

    for (uint32_t plane = 0; plane < copyInfo.planeCount; ++plane) {
        uint32_t srcRowStride = srcPlanes->planes[plane].rowStride;
        uint32_t dstRowStride = dstPlanes->planes[plane].rowStride;
        uint32_t rowBytes = std::min({copyInfo.rowBytes[plane], srcRowStride, dstRowStride});
        CHECK_AND_RETURN_RET_LOG(rowBytes > 0, false, "Invalid row bytes for plane %{public}u", plane);

        auto *srcPlaneAddr = src + srcPlanes->planes[plane].offset;
        auto *dstPlaneAddr = dst + dstPlanes->planes[plane].offset;
        for (uint32_t row = 0; row < copyInfo.rowCounts[plane]; ++row) {
            memcpy_s(dstPlaneAddr + row * dstRowStride, rowBytes, srcPlaneAddr + row * srcRowStride, rowBytes);
        }
    }
    return true;
}
} // namespace
Preprocessor::Preprocessor(const std::string &mime) : mime_(mime)
{
    fastKitsInterface_.Retain();
}

Preprocessor::~Preprocessor()
{
    fastKitsInterface_.Release();
}

int32_t Preprocessor::Configure(const Media::Format &format)
{
    int32_t enableSyncMode = false;
    if (format.GetIntValue(Media::Tag::AV_CODEC_ENABLE_SYNC_MODE, enableSyncMode) && enableSyncMode) {
        AVCODEC_LOGE("Synchronous mode is not supported for primary/secondary encoder!");
        return AVCS_ERR_INVALID_VAL;
    }
    UpdateConfiguredValues(format, encoderConfig_);
    CropParams cropParams;
    DownsamplingParams dsParams;
    DropFrameParams dropParams;
    int32_t ret = ValidateCrop(format, cropParams);
    ret = (ret == AVCS_ERR_OK) ? ValidateDownsampling(format, dsParams) : ret;
    ret = (ret == AVCS_ERR_OK) ? ValidateDropFrame(format, dropParams) : ret;
    if (ret != AVCS_ERR_OK) {
        return ret;
    }
    if (cropParams.result == ConfigResult::ENABLED && dsParams.result == ConfigResult::ENABLED) {
        AVCODEC_LOGE("Crop and downsampling cannot be enabled simultaneously");
        return AVCS_ERR_INVALID_VAL;
    }
    if (cropParams.result != ConfigResult::NOT_SET) {
        cropConfig_ = CropConfig(cropParams);
    }
    if (dsParams.result != ConfigResult::NOT_SET) {
        downsamplingConfig_ = DownsamplingConfig(dsParams);
    }
    if (dropParams.result != ConfigResult::NOT_SET) {
        dropFrameConfig_ = DropFrameConfig(dropParams);
    }
    if (cropConfig_.enabled || downsamplingConfig_.enabled) {
        int32_t initRet = fastKitsInterface_.Init(static_cast<uint32_t>(encoderConfig_.pixelFormat));
        CHECK_AND_RETURN_RET_LOG(initRet == AVCS_ERR_OK, initRet, "Failed to init FastKitsInterface: %{public}d",
                                 initRet);
    }
    if (dropFrameConfig_.enabled) {
        dropFilter_.Configure(dropFrameConfig_.targetFrameRate, encoderConfig_.frameRate);
    }
    AVCODEC_LOGI("PreProcessor configured: crop=%{public}d, ds=%{public}d, drop=%{public}d", cropConfig_.enabled,
                 downsamplingConfig_.enabled, dropFrameConfig_.enabled);
    return AVCS_ERR_OK;
}

int32_t Preprocessor::ValidateCrop(const Media::Format &format, CropParams &params)
{
    params.result = ConfigResult::NOT_SET;
    int32_t left{0};
    int32_t right{0};
    int32_t top{0};
    int32_t bottom{0};
    bool hasLeft = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, left);
    bool hasRight = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, right);
    bool hasTop = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, top);
    bool hasBottom = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, bottom);
    if (!(hasLeft && hasRight && hasTop && hasBottom)) {
        return AVCS_ERR_OK;
    }
    Range encWidth;
    Range encHeight;
    if (!GetEncoderRange(format, encWidth, encHeight)) {
        AVCODEC_LOGE("Failed to get encoder capability for crop");
        return AVCS_ERR_INVALID_VAL;
    }
    if (left == 0 && top == 0 && right == encoderConfig_.width && bottom == encoderConfig_.height) {
        params.result = ConfigResult::DISABLED;
        return AVCS_ERR_OK;
    }
    int32_t cropWidth = right - left + 1;
    int32_t cropHeight = bottom - top + 1;
    if (!(0 <= left && left < right && right <= encoderConfig_.width && 0 <= top && top < bottom &&
          bottom <= encoderConfig_.height && encWidth.InRange(cropWidth) && encHeight.InRange(cropHeight))) {
        AVCODEC_LOGE("Invalid crop parameters: left=%{public}d, right=%{public}d, top=%{public}d, "
                     "bottom=%{public}d",
                     left, right, top, bottom);
        return AVCS_ERR_INVALID_VAL;
    }
    params.left = left;
    params.right = right;
    params.top = top;
    params.bottom = bottom;
    params.result = ConfigResult::ENABLED;
    return AVCS_ERR_OK;
}

int32_t Preprocessor::ValidateDownsampling(const Media::Format &format, DownsamplingParams &params)
{
    params.result = ConfigResult::NOT_SET;
    int32_t width{0};
    int32_t height{0};
    bool hasWidth = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, width);
    bool hasHeight = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT, height);
    if (!(hasWidth && hasHeight)) {
        return AVCS_ERR_OK;
    }
    if (width == 0 && height == 0) {
        params.result = ConfigResult::DISABLED;
        return AVCS_ERR_OK;
    }
    Range encWidth;
    Range encHeight;
    if (!GetEncoderRange(format, encWidth, encHeight)) {
        AVCODEC_LOGE("Failed to get encoder capability for downsampling");
        return AVCS_ERR_INVALID_VAL;
    }
    int32_t minWidth = std::max((dAlgoWidthMin_ > 0) ? dAlgoWidthMin_ : encWidth.minVal, encWidth.minVal);
    int32_t maxWidth = (dAlgoWidthMax_ > 0) ? std::min(dAlgoWidthMax_, encWidth.maxVal) : encWidth.maxVal;
    int32_t minHeight = std::max((dAlgoHeightMin_ > 0) ? dAlgoHeightMin_ : encHeight.minVal, encHeight.minVal);
    int32_t maxHeight = (dAlgoHeightMax_ > 0) ? std::min(dAlgoHeightMax_, encHeight.maxVal) : encHeight.maxVal;
    if (!(minWidth > 0 && minHeight > 0 && minWidth <= width && width <= encoderConfig_.width &&
          encoderConfig_.width <= maxWidth && minHeight <= height && height <= encoderConfig_.height &&
          encoderConfig_.height <= maxHeight)) {
        AVCODEC_LOGE("Invalid downsampling parameters: width=%{public}d, height=%{public}d", width, height);
        return AVCS_ERR_INVALID_VAL;
    }
    params.width = width;
    params.height = height;
    params.result = ConfigResult::ENABLED;
    return AVCS_ERR_OK;
}

int32_t Preprocessor::ValidateDropFrame(const Media::Format &format, DropFrameParams &params)
{
    params.result = ConfigResult::NOT_SET;
    double targetFrameRate = 0;
    if (!format.GetDoubleValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DROP_TO_FRAME_RATE, targetFrameRate)) {
        return AVCS_ERR_OK;
    }
    targetFrameRate = RoundToTwoDecimals(targetFrameRate);
    if (targetFrameRate < 0 && encoderConfig_.frameRate <= 0) {
        AVCODEC_LOGE("Invalid drop frame rate: %{public}.2f, configured=%{public}.2f", targetFrameRate,
                     encoderConfig_.frameRate);
        return AVCS_ERR_INVALID_VAL;
    }
    if (targetFrameRate == 0) {
        params.result = ConfigResult::DISABLED;
        return AVCS_ERR_OK;
    }
    if (targetFrameRate < encoderConfig_.frameRate) {
        params.targetFrameRate = targetFrameRate;
        params.result = ConfigResult::ENABLED;
        return AVCS_ERR_OK;
    }
    AVCODEC_LOGE("Invalid drop frame rate: %{public}.2f, configured=%{public}.2f", targetFrameRate,
                 encoderConfig_.frameRate);
    return AVCS_ERR_INVALID_VAL;
}

int32_t Preprocessor::SetParameter(const Media::Format &format)
{
    return Configure(format);
}

int32_t Preprocessor::Process(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output, uint64_t pts)
{
    CHECK_AND_RETURN_RET_LOG(input && output, AVCS_ERR_INVALID_VAL,
                             "Invalid buffer: input=%{public}p, output=%{public}p", input.GetRefPtr(),
                             output.GetRefPtr());
    if (ShouldDropFrame(pts)) {
        return AVCS_ERR_OK;
    }

    if (IsCropEnabled()) {
        Crop(input, output);
    } else if (IsDownsamplingEnabled()) {
        Downsampling(input, output);
    } else {
        Copy(input, output);
    }

    return AVCS_ERR_OK;
}

bool Preprocessor::ShouldDropFrame(uint64_t pts)
{
    if (!dropFrameConfig_.enabled) {
        return false;
    }
    return dropFilter_.ShouldDropFrame(pts);
}

void Preprocessor::FlushTimeStamp()
{
    if (!dropFrameConfig_.enabled) {
        return;
    }
    return dropFilter_.FlushTimeStamp();
}

bool Preprocessor::IsCropEnabled() const
{
    return cropConfig_.enabled;
}

bool Preprocessor::IsDownsamplingEnabled() const
{
    return downsamplingConfig_.enabled;
}

const Preprocessor::CropConfig &Preprocessor::GetCropConfig() const
{
    return cropConfig_;
}

const Preprocessor::DownsamplingConfig &Preprocessor::GetDownsamplingConfig() const
{
    return downsamplingConfig_;
}

const Preprocessor::DropFrameConfig &Preprocessor::GetDropFrameConfig() const
{
    return dropFrameConfig_;
}

void Preprocessor::Crop(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output)
{
    PreProcessing::CropRect rect{cropConfig_.top, cropConfig_.left, cropConfig_.bottom, cropConfig_.right};
    int32_t ret = fastKitsInterface_.Crop(input, output, rect);
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Crop failed: %{public}d", ret);
    }
}

void Preprocessor::Downsampling(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output)
{
    int32_t ret = fastKitsInterface_.DownSample(input, output);
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Downsampling failed: %{public}d", ret);
    }
}

void Preprocessor::Copy(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output)
{
    auto *src = static_cast<uint8_t *>(input->GetVirAddr());
    auto *dst = static_cast<uint8_t *>(output->GetVirAddr());
    CHECK_AND_RETURN_LOG(src != nullptr && dst != nullptr,
                         "Invalid vir addr: input=%{public}p, output=%{public}p", src, dst);
    EXPECT_AND_LOGW(input->GetFormat() != output->GetFormat(),
                    "SurfaceBuffer format mismatch, input=%{public}d, output=%{public}d",
                    input->GetFormat(), output->GetFormat());

    if (CopyByPlanes(input, output, src, dst)) {
        return;
    }

    int32_t inputStride = input->GetStride();
    int32_t outputStride = output->GetStride();
    CHECK_AND_RETURN_LOG(inputStride > 0 && outputStride > 0, "Invalid stride: input=%{public}d, output=%{public}d",
                         inputStride, outputStride);

    uint32_t inputSize = input->GetSize();
    uint32_t outputSize = output->GetSize();
    uint32_t copySize = std::min(inputSize, outputSize);
    if (inputStride == outputStride) {
        memcpy_s(dst, copySize, src, copySize);
        return;
    }

    uint32_t rowBytes = std::min(static_cast<uint32_t>(std::min(inputStride, outputStride)), copySize);
    CHECK_AND_RETURN_LOG(rowBytes > 0, "Invalid row bytes, inputSize=%{public}u, outputSize=%{public}u",
                         inputSize, outputSize);
    uint32_t rowCount = std::min(inputSize / static_cast<uint32_t>(inputStride),
                                 outputSize / static_cast<uint32_t>(outputStride));
    for (uint32_t row = 0; row < rowCount; ++row) {
        memcpy_s(dst + row * outputStride, rowBytes, src + row * inputStride, rowBytes);
    }
}

bool Preprocessor::IsHevc10BitData(HEVCProfile profile)
{
    return profile == HEVC_PROFILE_MAIN_10 && mime_ == "video/hevc";
}

GraphicPixelFormat Preprocessor::ConvertVideoPixelFormat2GraphicPixelFormat(VideoPixelFormat pixelFormat,
                                                                            int32_t profile)
{
    bool is10BitData = IsHevc10BitData(static_cast<HEVCProfile>(profile));
    switch (pixelFormat) {
        case VideoPixelFormat::YUVI420: {
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P;
        }
        case VideoPixelFormat::RGBA: {
            return is10BitData ? GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010102
                               : GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888;
        }
        case VideoPixelFormat::NV12: {
            return is10BitData ? GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010
                               : GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP;
        }
        case VideoPixelFormat::NV21: {
            return is10BitData ? GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010
                               : GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP;
        }
        case VideoPixelFormat::RGBA1010102: {
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010102;
        }
        default:
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT;
    }
}

void Preprocessor::UpdateConfiguredValues(const Media::Format &format, EncoderConfig &config)
{
    int32_t configuredWidth = 0;
    int32_t configuredHeight = 0;
    double configuredFramerate = 0;
    if (format.GetIntValue(Media::Tag::VIDEO_WIDTH, configuredWidth) &&
        format.GetIntValue(Media::Tag::VIDEO_HEIGHT, configuredHeight)) {
        config.width = configuredWidth;
        config.height = configuredHeight;
    }

    if (format.GetDoubleValue(Media::Tag::VIDEO_FRAME_RATE, configuredFramerate)) {
        config.frameRate = RoundToTwoDecimals(configuredFramerate);
    }

    int32_t configuredPixelFormat = 0;
    if (format.GetIntValue(Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, configuredPixelFormat)) {
        config.pixelFormat = static_cast<GraphicPixelFormat>(configuredPixelFormat);
    } else {
        if (format.GetIntValue(Media::Tag::VIDEO_PIXEL_FORMAT, configuredPixelFormat)) {
            int32_t profile = -1;
            (void)format.GetIntValue(Media::Tag::MEDIA_PROFILE, profile);
            config.pixelFormat = ConvertVideoPixelFormat2GraphicPixelFormat(
                static_cast<VideoPixelFormat>(configuredPixelFormat), profile);
        } else {
            config.pixelFormat = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP;
        }
    }
}

bool Preprocessor::GetEncoderRange(const Media::Format &format, Range &encWidth, Range &encHeight)
{
    AVCodecListImpl codecList;
    (void)codecList.Init();
    CapabilityData *cap = codecList.GetCapability(std::string(mime_.data()), true, AVCodecCategory::AVCODEC_NONE);
    if (cap == nullptr) {
        return false;
    }
    encWidth = cap->width;
    encHeight = cap->height;
    return true;
}

int32_t Preprocessor::GetConfiguredWidth() const
{
    return encoderConfig_.width;
}

int32_t Preprocessor::GetConfiguredHeight() const
{
    return encoderConfig_.height;
}

GraphicPixelFormat Preprocessor::GetConfiguredPixelFormat() const
{
    return encoderConfig_.pixelFormat;
}

int32_t Preprocessor::GetOutputWidth() const
{
    if (IsCropEnabled()) {
        return cropConfig_.width;
    }
    if (IsDownsamplingEnabled()) {
        return downsamplingConfig_.width;
    }
    return encoderConfig_.width;
}

int32_t Preprocessor::GetOutputHeight() const
{
    if (IsCropEnabled()) {
        return cropConfig_.height;
    }
    if (IsDownsamplingEnabled()) {
        return downsamplingConfig_.height;
    }
    return encoderConfig_.height;
}

int32_t Preprocessor::GetCropLeft() const
{
    return cropConfig_.left;
}

int32_t Preprocessor::GetCropRight() const
{
    return cropConfig_.right;
}

int32_t Preprocessor::GetCropTop() const
{
    return cropConfig_.top;
}

int32_t Preprocessor::GetCropBottom() const
{
    return cropConfig_.bottom;
}

int32_t Preprocessor::GetDownsamplingWidth() const
{
    return downsamplingConfig_.width;
}

int32_t Preprocessor::GetDownsamplingHeight() const
{
    return downsamplingConfig_.height;
}

} // namespace MediaAVCodec
} // namespace OHOS
