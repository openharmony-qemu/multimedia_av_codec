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
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "native_avcodec_base.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Preprocessor"};
}

namespace OHOS {
namespace MediaAVCodec {

Preprocessor::Preprocessor()
{
    AVCODEC_LOGI("Preprocessor created");
}

Preprocessor::~Preprocessor()
{
    AVCODEC_LOGI("Preprocessor destroyed");
}

int32_t Preprocessor::Configure(const Media::Format& format)
{
    CropParams cropParams;
    int32_t ret = ValidateCrop(format, cropParams);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    DownsamplingParams dsParams;
    ret = ValidateDownsampling(format, dsParams);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    DropFrameParams dropParams;
    ret = ValidateDropFrame(format, dropParams);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    if (cropParams.result == ConfigResult::ENABLED && dsParams.result == ConfigResult::ENABLED) {
        AVCODEC_LOGE("Crop and downsampling cannot be enabled simultaneously");
        return AVCS_ERR_INVALID_VAL;
    }

    if (cropParams.result == ConfigResult::ENABLED) {
        cropConfig_.left = cropParams.left;
        cropConfig_.right = cropParams.right;
        cropConfig_.top = cropParams.top;
        cropConfig_.bottom = cropParams.bottom;
        cropConfig_.enabled = true;
    } else if (cropParams.result == ConfigResult::DISABLED) {
        cropConfig_.enabled = false;
    }

    if (dsParams.result == ConfigResult::ENABLED) {
        downsamplingConfig_.width = dsParams.width;
        downsamplingConfig_.height = dsParams.height;
        downsamplingConfig_.enabled = true;
    } else if (dsParams.result == ConfigResult::DISABLED) {
        downsamplingConfig_.enabled = false;
    }

    if (dropParams.result == ConfigResult::ENABLED) {
        dropFrameConfig_.targetFrameRate = dropParams.targetFrameRate;
        dropFrameConfig_.enabled = true;
    } else if (dropParams.result == ConfigResult::DISABLED) {
        dropFrameConfig_.enabled = false;
    }

    AVCODEC_LOGI("Preprocessor configured: crop=%{public}d, ds=%{public}d, drop=%{public}d",
        cropConfig_.enabled, downsamplingConfig_.enabled, dropFrameConfig_.enabled);
    return AVCS_ERR_OK;
}

int32_t Preprocessor::ValidateCrop(const Media::Format& format, CropParams& params)
{
    int32_t left{0};
    int32_t right{0};
    int32_t top{0};
    int32_t bottom{0};
    bool hasLeft = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, left);
    bool hasRight = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, right);
    bool hasTop = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, top);
    bool hasBottom = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, bottom);

    bool hasAnyCrop = hasLeft || hasRight || hasTop || hasBottom;
    bool hasAllCrop = hasLeft && hasRight && hasTop && hasBottom;

    if (hasAnyCrop && !hasAllCrop) {
        AVCODEC_LOGE("Crop parameters must be all configured or all not configured");
        return AVCS_ERR_INVALID_VAL;
    }

    if (hasAllCrop) {
        if (left == 0 && right == 0 && top == 0 && bottom == 0) {
            params.result = ConfigResult::DISABLED;
        } else {
            if (!(0 <= left && left < right && 0 <= top && top < bottom)) {
                AVCODEC_LOGE("Invalid crop parameters: left=%{public}d, right=%{public}d, top=%{public}d, bottom=%{public}d",
                    left, right, top, bottom);
                return AVCS_ERR_INVALID_VAL;
            }
            params.left = left;
            params.right = right;
            params.top = top;
            params.bottom = bottom;
            params.result = ConfigResult::ENABLED;
        }
    } else {
        params.result = ConfigResult::NOT_SET;
    }

    return AVCS_ERR_OK;
}

int32_t Preprocessor::ValidateDownsampling(const Media::Format& format, DownsamplingParams& params)
{
    int32_t width{0};
    int32_t height{0};
    bool hasWidth = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, width);
    bool hasHeight = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT, height);

    if (hasWidth != hasHeight) {
        AVCODEC_LOGE("Downsampling parameters must be both configured or both not configured");
        return AVCS_ERR_INVALID_VAL;
    }

    if (hasWidth && hasHeight) {
        if (width == 0 && height == 0) {
            params.result = ConfigResult::DISABLED;
        } else {
            if (!(width > 0 && height > 0)) {
                AVCODEC_LOGE("Invalid downsampling parameters: width=%{public}d, height=%{public}d", width, height);
                return AVCS_ERR_INVALID_VAL;
            }
            params.width = width;
            params.height = height;
            params.result = ConfigResult::ENABLED;
        }
    } else {
        params.result = ConfigResult::NOT_SET;
    }

    return AVCS_ERR_OK;
}

int32_t Preprocessor::ValidateDropFrame(const Media::Format& format, DropFrameParams& params)
{
    int32_t targetFrameRate{0};
    if (format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DROP_TO_FRAME_RATE, targetFrameRate)) {
        if (targetFrameRate == 0) {
            params.result = ConfigResult::DISABLED;
        } else if (targetFrameRate > 0) {
            params.targetFrameRate = targetFrameRate;
            params.result = ConfigResult::ENABLED;
        } else {
            AVCODEC_LOGE("Invalid drop to frame rate: %{public}d", targetFrameRate);
            return AVCS_ERR_INVALID_VAL;
        }
    } else {
        params.result = ConfigResult::NOT_SET;
    }

    return AVCS_ERR_OK;
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

} // namespace MediaAVCodec
} // namespace OHOS