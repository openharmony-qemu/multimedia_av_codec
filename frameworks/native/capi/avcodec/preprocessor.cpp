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
#include "meta/meta_key.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "native_avcodec_base.h"
#include "avcodeclist_impl.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Preprocessor"};
}

namespace OHOS {
namespace MediaAVCodec {

int32_t Preprocessor::Configure(const Media::Format& format)
{
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
    AVCODEC_LOGI("PreProcessor configured: crop=%{public}d, ds=%{public}d, drop=%{public}d",
        cropConfig_.enabled, downsamplingConfig_.enabled, dropFrameConfig_.enabled);
    return AVCS_ERR_OK;
}

int32_t Preprocessor::ValidateCrop(const Media::Format& format, CropParams& params)
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

int32_t Preprocessor::ValidateDownsampling(const Media::Format& format, DownsamplingParams& params)
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

int32_t Preprocessor::ValidateDropFrame(const Media::Format& format, DropFrameParams& params)
{
    params.result = ConfigResult::NOT_SET;
    int32_t targetFrameRate = 0;
    if (format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DROP_TO_FRAME_RATE, targetFrameRate)) {
        return AVCS_ERR_OK;
    }
    if (targetFrameRate < 0 && encoderConfig_.frameRate <= 0) {
        AVCODEC_LOGE("Invalid drop frame rate: %{public}d, configured=%{public}.2f",
            targetFrameRate, encoderConfig_.frameRate);
        return AVCS_ERR_INVALID_VAL;
    }
    if (targetFrameRate == 0) {
        params.result = ConfigResult::DISABLED;
        return AVCS_ERR_OK;
    }
    if (targetFrameRate < encoderConfig_.frameRate) {
        params.targetFrameRate = static_cast<int32_t>(targetFrameRate);
        params.result = ConfigResult::ENABLED;
        return AVCS_ERR_OK;
    }
    AVCODEC_LOGE("Invalid drop frame rate: %{public}d, configured=%{public}.2f",
        targetFrameRate, encoderConfig_.frameRate);
    return AVCS_ERR_INVALID_VAL;
}

int32_t Preprocessor::SetParameter(const Media::Format& format)
{
    return Configure(format);
}

int32_t Preprocessor::Process(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output)
{
    if (ShouldDropFrame(0)) {
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

bool Preprocessor::ShouldDropFrame(int64_t pts)
{
    if (!dropFrameConfig_.enabled) {
        return false;
    }

    if (lastFramePts_ == 0) {
        lastFramePts_ = pts;
        return false;
    }

    int64_t frameInterval = 1000000 / dropFrameConfig_.targetFrameRate;
    if (pts - lastFramePts_ < frameInterval) {
        return true;
    }

    lastFramePts_ = pts;
    return false;
}

bool Preprocessor::IsCropEnabled() const
{
    return cropConfig_.enabled;
}

bool Preprocessor::IsDownsamplingEnabled() const
{
    return downsamplingConfig_.enabled;
}

const Preprocessor::CropConfig& Preprocessor::GetCropConfig() const
{
    return cropConfig_;
}

const Preprocessor::DownsamplingConfig& Preprocessor::GetDownsamplingConfig() const
{
    return downsamplingConfig_;
}

const Preprocessor::DropFrameConfig& Preprocessor::GetDropFrameConfig() const
{
    return dropFrameConfig_;
}

void Preprocessor::Crop(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output)
{

}

void Preprocessor::Downsampling(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output)
{

}

void Preprocessor::Copy(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> output)
{
    auto *src = reinterpret_cast<uint8_t *>(input->GetVirAddr());
    auto *dst = reinterpret_cast<uint8_t *>(output->GetVirAddr());
    size_t size = input->GetSize();
    if (src != nullptr && dst != nullptr && size > 0)
    {
        std::copy(src, src + size, dst);
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
            return is10BitData ? GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010102 :
                                                    GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888;
        }
        case VideoPixelFormat::NV12: {
            return is10BitData ? GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010 :
                                 GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP;
        }
        case VideoPixelFormat::NV21: {
            return is10BitData ? GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010 :
                                 GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP;
        }
        case VideoPixelFormat::RGBA1010102: {
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010102;
        }
        default:
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT;
    }
}

void Preprocessor::UpdateConfiguredValues(const Media::Format& format, EncoderConfig& config)
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
        config.frameRate = configuredFramerate;
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

bool Preprocessor::GetEncoderRange(const Media::Format& format, Range& encWidth, Range& encHeight)
{
    AVCodecListImpl codecList;
    (void)codecList.Init();
    CapabilityData* cap = codecList.GetCapability(std::string(mime_.data()), true, AVCodecCategory::AVCODEC_NONE);
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
    // TODO
    return 0;
}

int32_t Preprocessor::GetOutputHeight() const
{
    // TODO
    return 0;
}

} // namespace MediaAVCodec
} // namespace OHOS
