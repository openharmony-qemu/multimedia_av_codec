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
#include <limits>
#include <regex>
#include <string>
#include <vector>
#include "securec.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodeclist_impl.h"
#include "meta/meta_key.h"
#include "native_avcodec_base.h"
#include "syspara/parameters.h"
#include "v2_2/buffer_handle_meta_key_type.h"

namespace {
using namespace OHOS;
namespace V2_2 = OHOS::HDI::Display::Graphic::Common::V2_2;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Preprocessor"};
constexpr int32_t BUFFER_REQUEST_STRIDE_ALIGNMENT = 32;
auto fastKitEnable = OHOS::system::GetBoolParameter("persist.OHOS.MediaAVCodec.FastKit.Enable", false);

double RoundToTwoDecimals(double value)
{
    return std::round(value * 100.0) / 100.0; // 100.0: Round to 2 decimal digits using round half up
}

bool IsYuv420Format(OHOS::GraphicPixelFormat format)
{
    return format == GRAPHIC_PIXEL_FMT_YCBCR_420_SP || format == GRAPHIC_PIXEL_FMT_YCRCB_420_SP ||
           format == GRAPHIC_PIXEL_FMT_YCBCR_P010 || format == GRAPHIC_PIXEL_FMT_YCRCB_P010 ||
           format == GRAPHIC_PIXEL_FMT_YCBCR_420_P;
}

int32_t GetMinRowBytes(OHOS::GraphicPixelFormat format, int32_t width)
{
    switch (format) {
        case GRAPHIC_PIXEL_FMT_RGBA_8888:
        case GRAPHIC_PIXEL_FMT_RGBA_1010102:
            return width * 4; // 4: bytes per packed RGBA pixel
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010:
            return width * 2; // 2: 10-bit samples stored in 16-bit containers
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P:
            return width;
        default:
            return -1;
    }
}

bool IsGeometrySupported(OHOS::GraphicPixelFormat format, int32_t width, int32_t height)
{
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (!OHOS::MediaAVCodec::PreProcessing::CpuImageProcessor::IsFormatSupported(static_cast<int32_t>(format))) {
        return false;
    }
    return !IsYuv420Format(format) || (width % 2 == 0 && height % 2 == 0);
}

std::string TransformRoiMetadata(const std::string &roi, double scaleX, double scaleY,
                                 int32_t offsetTop, int32_t offsetLeft)
{
    static const std::regex ROI_PATTERN(R"((?:^|;)(-?\d+),(-?\d+)-(-?\d+),(-?\d+)(?:=([^;]*))?;)");
    std::smatch match;
    std::string temp = roi;
    std::string result;
    while (std::regex_search(temp, match, ROI_PATTERN)) {
        int32_t top = std::stoi(match[1].str());
        int32_t left = std::stoi(match[2].str());
        int32_t bottom = std::stoi(match[3].str());
        int32_t right = std::stoi(match[4].str());
        int32_t newTop = static_cast<int32_t>(std::round(top * scaleY)) - offsetTop;
        int32_t newLeft = static_cast<int32_t>(std::round(left * scaleX)) - offsetLeft;
        int32_t newBottom = static_cast<int32_t>(std::round(bottom * scaleY)) - offsetTop;
        int32_t newRight = static_cast<int32_t>(std::round(right * scaleX)) - offsetLeft;
        if (!result.empty()) {
            result += ";";
        }
        result += std::to_string(newTop) + "," + std::to_string(newLeft) + "-" +
                  std::to_string(newBottom) + "," + std::to_string(newRight);
        if (match[5].matched) {
            result += "=" + match[5].str();
        }
        temp = match.suffix().str();
    }
    if (!result.empty()) {
        result += ";";
    }
    return result;
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
Preprocessor::Preprocessor(const std::string &mime) : mime_(mime)
{
    if (fastKitEnable) {
        fastKitsInterface_.Retain();
    }
}

Preprocessor::~Preprocessor()
{
    if (fastKitEnable) {
        fastKitsInterface_.Release();
    }
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
    if (fastKitEnable && (cropConfig_.enabled || downsamplingConfig_.enabled)) {
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

    bool hasAny = hasLeft || hasRight || hasTop || hasBottom;
    bool hasAll = hasLeft && hasRight && hasTop && hasBottom;

    if (!hasAny) {              // If none of the 4 values (left, right, top and bottom) is configured,
        return AVCS_ERR_OK;     // keep current status.
    }

    if (hasAny && !hasAll) {            // At least one of the 4 values is not configured,
        AVCODEC_LOGE("Incomplete crop parameters: hasLeft=%{public}d, hasRight=%{public}d, "
                     "hasTop=%{public}d, hasBottom=%{public}d",
                     hasLeft, hasRight, hasTop, hasBottom);
        return AVCS_ERR_INVALID_VAL;    // it is not allowed.
    }

    // left, right, top and bottom are all configured
    if (left == 0 && top == 0 && right == 0 && bottom == 0) {
        params.result = ConfigResult::DISABLED;
        AVCODEC_LOGI("Crop disabled by zero crop parameters");
        return AVCS_ERR_OK;
    }

    Range encWidth;
    Range encHeight;
    if (!GetEncoderRange(format, encWidth, encHeight)) {
        AVCODEC_LOGE("Failed to get encoder capability for crop");
        return AVCS_ERR_INVALID_VAL;
    }
    int32_t cropWidth = right - left + 1;
    int32_t cropHeight = bottom - top + 1;
    bool rangeValid = (0 <= left && left < right && right < encoderConfig_.width) &&
                      (0 <= top && top < bottom && bottom < encoderConfig_.height) &&
                      (encWidth.InRange(cropWidth) && encHeight.InRange(cropHeight));
    if (!rangeValid) {
        AVCODEC_LOGE("Invalid crop parameters: left=%{public}d, right=%{public}d, top=%{public}d, bottom=%{public}d",
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
    if (hasWidth != hasHeight) {
        return AVCS_ERR_INVALID_VAL;
    }
    if (!hasWidth && !hasHeight) {
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
    if (targetFrameRate < -std::numeric_limits<double>::epsilon() ||
        encoderConfig_.frameRate < -std::numeric_limits<double>::epsilon()) {
        AVCODEC_LOGE("Invalid drop frame rate: %{public}.2f, configured=%{public}.2f", targetFrameRate,
                     encoderConfig_.frameRate);
        return AVCS_ERR_INVALID_VAL;
    }
    if (std::fabs(targetFrameRate) < std::numeric_limits<double>::epsilon()) {
        params.result = ConfigResult::DISABLED;
        return AVCS_ERR_OK;
    }
    if ((targetFrameRate - encoderConfig_.frameRate) < -std::numeric_limits<double>::epsilon()) {
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

int32_t Preprocessor::Process(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output, uint64_t pts)
{
    CHECK_AND_RETURN_RET_LOG(input, AVCS_ERR_INVALID_VAL,
                             "Invalid buffer: input=%{public}p, output=%{public}p", input.GetRefPtr(),
                             output.GetRefPtr());
    CHECK_AND_RETURN_RET_LOG(!output || input->GetFormat() == output->GetFormat(), AVCS_ERR_INVALID_VAL,
                             "Format mismatch: input=%{public}d, output=%{public}d", input->GetFormat(),
                             output ? output->GetFormat() : -1);
    if (IsCropEnabled()) {
        int32_t ret = Crop(input, output);
        if (ret == AVCS_ERR_OK) {
            UpdateRoiMetadata(input, output, 1.0, 1.0, cropConfig_.top, cropConfig_.left);
            return ret;
        }
        AVCODEC_LOGW("Crop failed: %{public}d, fallback to copy", ret);
        ret = Copy(input, output);
        if (ret == AVCS_ERR_OK) {
            UpdateRoiMetadata(input, output, 1.0, 1.0, 0, 0);
        }
        return ret;
    }
    if (IsDownsamplingEnabled()) {
        int32_t ret = Downsampling(input, output);
        if (ret == AVCS_ERR_OK) {
            UpdateRoiMetadata(input, output, static_cast<double>(output->GetWidth()) / input->GetWidth(),
                              static_cast<double>(output->GetHeight()) / input->GetHeight(), 0, 0);
            return ret;
        }
        AVCODEC_LOGW("Downsampling failed: %{public}d, fallback to copy", ret);
        ret = Copy(input, output);
        if (ret == AVCS_ERR_OK) {
            UpdateRoiMetadata(input, output, 1.0, 1.0, 0, 0);
        }
        return ret;
    }
    int32_t ret = Copy(input, output);
    if (ret == AVCS_ERR_OK) {
        UpdateRoiMetadata(input, output, 1.0, 1.0, 0, 0);
    }
    return ret;
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

void Preprocessor::SetOnBufferReallocCallback(std::function<void(sptr<SurfaceBuffer>, sptr<SurfaceBuffer>)> callback)
{
    onOutSurfaceBufferReallocCb_ = callback;
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

int32_t Preprocessor::Crop(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output)
{
    if (!CheckOrReallocOutSurfaceBuffer(cropConfig_.width, cropConfig_.height, input->GetFormat(), output)) {
        AVCODEC_LOGE("Realloc buffer failed");
        return AVCS_ERR_NO_MEMORY;
    }
    if (fastKitEnable) {
        PreProcessing::CropRect rect{cropConfig_.top, cropConfig_.left, cropConfig_.bottom, cropConfig_.right};
        int32_t ret = fastKitsInterface_.Crop(input, output, rect);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "FastKit crop failed: %{public}d", ret);
        return AVCS_ERR_OK;
    }
    PreProcessing::CpuImageProcessRequest request;
    request.input = input;
    request.output = output;
    request.operation = PreProcessing::CpuImageOperation::CROP;
    request.crop = {cropConfig_.left, cropConfig_.top, cropConfig_.width, cropConfig_.height};
    int32_t ret = PreProcessing::CpuImageProcessor::Process(request);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Crop failed: %{public}d", ret);
    return AVCS_ERR_OK;
}

int32_t Preprocessor::Downsampling(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output)
{
    if (!CheckOrReallocOutSurfaceBuffer(downsamplingConfig_.width, downsamplingConfig_.height,
                                        input->GetFormat(), output)) {
        AVCODEC_LOGE("Realloc buffer failed");
        return AVCS_ERR_NO_MEMORY;
    }
    if (fastKitEnable) {
        int32_t ret = fastKitsInterface_.DownSample(input, output);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "FastKit downsampling failed: %{public}d", ret);
        return AVCS_ERR_OK;
    }
    PreProcessing::CpuImageProcessRequest request;
    request.input = input;
    request.output = output;
    request.operation = PreProcessing::CpuImageOperation::DOWNSAMPLE;
    int32_t ret = PreProcessing::CpuImageProcessor::Process(request);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Downsampling failed: %{public}d", ret);
    return AVCS_ERR_OK;
}

int32_t Preprocessor::Copy(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output)
{
    if (!CheckOrReallocOutSurfaceBuffer(input->GetWidth(), input->GetHeight(), input->GetFormat(), output)) {
        AVCODEC_LOGE("Realloc buffer failed");
        return AVCS_ERR_NO_MEMORY;
    }
    PreProcessing::CpuImageProcessRequest request;
    request.input = input;
    request.output = output;
    request.operation = PreProcessing::CpuImageOperation::COPY;
    int32_t ret = PreProcessing::CpuImageProcessor::Process(request);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Copy failed: %{public}d", ret);
    return AVCS_ERR_OK;
}

void Preprocessor::UpdateRoiMetadata(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                                     double scaleX, double scaleY, int32_t offsetTop, int32_t offsetLeft)
{
    CHECK_AND_RETURN_LOG(input != nullptr && output != nullptr,
                         "Invalid buffer for roi metadata: input=%{public}p, output=%{public}p",
                         input.GetRefPtr(), output.GetRefPtr());

    std::vector<uint8_t> roiData;
    if (input->GetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData) != GSERROR_OK || roiData.empty()) {
        return;
    }
    auto nullPos = std::find(roiData.begin(), roiData.end(), '\0');
    std::string roi(roiData.begin(), nullPos);
    std::string transformed = TransformRoiMetadata(roi, scaleX, scaleY, offsetTop, offsetLeft);
    std::vector<uint8_t> outputRoi(transformed.begin(), transformed.end());
    outputRoi.push_back('\0');
    GSError ret = output->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, outputRoi);
    CHECK_AND_RETURN_LOG(ret == GSERROR_OK, "Set roi metadata failed: %{public}d", ret);
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
    CapabilityData *cap = codecList.GetCapability(std::string(mime_.c_str()), true, AVCodecCategory::AVCODEC_NONE);
    if (cap == nullptr) {
        return false;
    }
    encWidth = cap->width;
    encHeight = cap->height;
    return true;
}

bool Preprocessor::CheckOrReallocOutSurfaceBuffer(int32_t targetWidth, int32_t targetHeight,
                                                  int32_t targetFormat, sptr<SurfaceBuffer> &output)
{
    if (!IsGeometrySupported(encoderConfig_.pixelFormat, targetWidth, targetHeight)) {
        AVCODEC_LOGE("Unsupported target geometry: %{public}dx%{public}d format=%{public}d",
                     targetWidth, targetHeight, encoderConfig_.pixelFormat);
        return false;
    }
    int32_t minStride = GetMinRowBytes(encoderConfig_.pixelFormat, targetWidth);
    if (minStride <= 0) {
        return false;
    }
    bool needRealloc = !output || output->GetWidth() != targetWidth || output->GetHeight() != targetHeight ||
                       output->GetFormat() != targetFormat ||
                       output->GetVirAddr() == nullptr ||
                       output->GetStride() < minStride;
    if (needRealloc) {
        sptr<SurfaceBuffer> newBuffer = SurfaceBuffer::Create();
        if (!newBuffer) {
            return false;
        }
        constexpr auto DEFAULT_USAGE = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_MMZ_CACHE;
        BufferRequestConfig config = {
            .width = targetWidth,
            .height = targetHeight,
            .strideAlignment = BUFFER_REQUEST_STRIDE_ALIGNMENT,
            .format = targetFormat,
            .usage = output ? output->GetUsage() | DEFAULT_USAGE : DEFAULT_USAGE,
            .timeout = 0,
        };
        auto ret = newBuffer->Alloc(config);
        if (ret != GSERROR_OK) {
            AVCODEC_LOGE("Alloc output buffer failed: %{public}d", ret);
            return false;
        }
        if (output && onOutSurfaceBufferReallocCb_) {
            onOutSurfaceBufferReallocCb_(output, newBuffer);
        }
        output = newBuffer;
    }
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
