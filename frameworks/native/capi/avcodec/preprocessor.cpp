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
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
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
bool IsEvenNumber(int32_t value)
{
    constexpr int32_t checkEvenNumber = 2;
    return (value % checkEvenNumber == 0);
}
}
namespace OHOS {
namespace MediaAVCodec {
namespace {
using namespace OHOS;
namespace V2_2 = OHOS::HDI::Display::Graphic::Common::V2_2;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Preprocessor"};
constexpr int32_t BUFFER_REQUEST_STRIDE_ALIGNMENT = 32;
constexpr double ROUND_TO_TWO_DECIMALS_FACTOR = 100.0;
constexpr int32_t RGBA_BYTES_PER_PIXEL = 4;
constexpr int32_t P010_BYTES_PER_SAMPLE = 2;
constexpr int32_t YUV_SUBSAMPLE = 2;
constexpr int32_t INVALID_ROW_BYTES = -1;
constexpr int32_t DISABLE_CROP_LEFT = 0;
constexpr int32_t DISABLE_CROP_TOP = 0;
constexpr int32_t DISABLE_CROP_RIGHT = 0;
constexpr int32_t DISABLE_CROP_BOTTOM = 0;
constexpr int32_t DISABLE_DOWNSAMPLING_WIDTH = 0;
constexpr int32_t DISABLE_DOWNSAMPLING_HEIGHT = 0;
constexpr int32_t DEFAULT_METADATA_OFFSET = 0;
constexpr double IDENTITY_METADATA_SCALE = 1.0;
constexpr auto FAST_KIT_ENABLE = true;

struct RoiEntry {
    int32_t top{0};
    int32_t left{0};
    int32_t bottom{0};
    int32_t right{0};
    std::string_view param;
    bool hasParam{false};
};

std::optional<int32_t> ParseInt32(std::string_view str)
{
    int32_t value = 0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    return ec == std::errc{} && ptr == (str.data() + str.size()) ? std::optional<int32_t>(value) : std::nullopt;
}

std::optional<RoiEntry> ParseRoiEntry(std::string_view entry)
{
    size_t paramPos = entry.find('=');
    size_t coordinateEnd = paramPos == std::string_view::npos ? entry.size() : paramPos;
    size_t firstComma = entry.find(',');
    if (firstComma == std::string_view::npos || firstComma >= coordinateEnd) {
        return std::nullopt;
    }
    size_t dashSearchStart = firstComma + 1;
    if (dashSearchStart < coordinateEnd && entry[dashSearchStart] == '-') {
        ++dashSearchStart;
    }
    size_t dash = entry.find('-', dashSearchStart);
    size_t secondComma = dash == std::string_view::npos ? std::string_view::npos : entry.find(',', dash + 1);
    if (dash == std::string_view::npos || dash >= coordinateEnd || secondComma == std::string_view::npos ||
        secondComma >= coordinateEnd) {
        return std::nullopt;
    }

    auto top = ParseInt32(entry.substr(0, firstComma));
    auto left = ParseInt32(entry.substr(firstComma + 1, dash - firstComma - 1));
    auto bottom = ParseInt32(entry.substr(dash + 1, secondComma - dash - 1));
    auto right = ParseInt32(entry.substr(secondComma + 1, coordinateEnd - secondComma - 1));
    if (!top || !left || !bottom || !right) {
        return std::nullopt;
    }
    return RoiEntry{*top, *left, *bottom, *right,
                    paramPos == std::string_view::npos ? std::string_view{} : entry.substr(paramPos + 1),
                    paramPos != std::string_view::npos};
}

double RoundToTwoDecimals(double value)
{
    return std::round(value * ROUND_TO_TWO_DECIMALS_FACTOR) / ROUND_TO_TWO_DECIMALS_FACTOR;
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
            return width * RGBA_BYTES_PER_PIXEL;
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010:
            return width * P010_BYTES_PER_SAMPLE;
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P:
            return width;
        default:
            return INVALID_ROW_BYTES;
    }
}

bool IsGeometrySupported(OHOS::GraphicPixelFormat format, int32_t width, int32_t height)
{
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (FAST_KIT_ENABLE &&
        !OHOS::MediaAVCodec::PreProcessing::FastKitsInterface::IsFormatSupported(static_cast<int32_t>(format))) {
        return false;
    } else if (!FAST_KIT_ENABLE &&
               !OHOS::MediaAVCodec::PreProcessing::CpuImageProcessor::IsFormatSupported(static_cast<int32_t>(format))) {
        return false;
    }
    return !IsYuv420Format(format) || (width % YUV_SUBSAMPLE == 0 && height % YUV_SUBSAMPLE == 0);
}

std::string TransformRoiMetadata(const std::string &roi, double scaleX, double scaleY,
                                 int32_t offsetTop, int32_t offsetLeft)
{
    std::string result;
    std::string_view roiView(roi);
    for (size_t start = 0; start <= roiView.size();) {
        size_t end = roiView.find(';', start);
        std::string_view segment = roiView.substr(start, end == std::string_view::npos ? end : end - start);
        auto entry = ParseRoiEntry(segment);
        if (entry) {
            int32_t newTop = static_cast<int32_t>(std::round(entry->top * scaleY)) - offsetTop;
            int32_t newLeft = static_cast<int32_t>(std::round(entry->left * scaleX)) - offsetLeft;
            int32_t newBottom = static_cast<int32_t>(std::round(entry->bottom * scaleY)) - offsetTop;
            int32_t newRight = static_cast<int32_t>(std::round(entry->right * scaleX)) - offsetLeft;
            if (!result.empty()) {
                result += ";";
            }
            result += std::to_string(newTop) + "," + std::to_string(newLeft) + "-" +
                      std::to_string(newBottom) + "," + std::to_string(newRight);
            if (entry->hasParam) {
                result += "=";
                result.append(entry->param.data(), entry->param.size());
            }
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    if (!result.empty()) {
        result += ";";
    }
    return result;
}

} // namespace

Preprocessor::Preprocessor(const std::string &mime) : mime_(mime)
{
    if (FAST_KIT_ENABLE) {
        fastKitsInterface_.Retain();
    }
}

Preprocessor::~Preprocessor()
{
    if (FAST_KIT_ENABLE) {
        fastKitsInterface_.Release();
    }
}

bool Preprocessor::IsAvailable()
{
    if (!FAST_KIT_ENABLE) {
        return false;
    }
    auto &fastKits = PreProcessing::FastKitsInterface::GetInstance();
    fastKits.Retain();
    bool loaded = fastKits.IsLoaded();
    fastKits.Release();
    return loaded;
}

int32_t Preprocessor::ValidateConfiguration(const Media::Format &format, bool configuring)
{
    CHECK_AND_RETURN_RET_LOG(UsingAsyncMode(format), AVCS_ERR_INVALID_VAL,
                             "Preprocessor encoder does not support synchronous mode!");

    UpdateConfiguredValues(format, configuring);
    int32_t ret = ValidateCrop(format);
    ret = (ret == AVCS_ERR_OK) ? ValidateDownsampling(format) : ret;
    ret = (ret == AVCS_ERR_OK) ? ValidateDropFrame(format) : ret;
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    if (cropParams_.result == ConfigResult::ENABLED && downsamplingParams_.result == ConfigResult::ENABLED) {
        AVCODEC_LOGE("Crop and downsampling cannot be enabled simultaneously");
        return AVCS_ERR_INVALID_VAL;
    }
    if (cropParams_.result == ConfigResult::ENABLED && IsYuv420Format(encoderParams_.pixelFormat)) {
        int32_t cropWidth = cropParams_.right - cropParams_.left + 1;
        int32_t cropHeight = cropParams_.bottom - cropParams_.top + 1;
        bool invalid = (!IsEvenNumber(cropParams_.left) || !IsEvenNumber(cropParams_.top) ||
                        !IsEvenNumber(cropWidth) || !IsEvenNumber(cropHeight));
        CHECK_AND_RETURN_RET_LOG(!invalid, AVCS_ERR_INVALID_VAL,
            "For YUV pixel format input, crop left-top coordinates and crop width, height must be even numbers. "
            "(left, top)=(%{public}d, %{public}d), width=%{public}d, height=%{public}d",
            cropParams_.left, cropParams_.top, cropWidth, cropHeight);
    }
    if (downsamplingParams_.result == ConfigResult::ENABLED && IsYuv420Format(encoderParams_.pixelFormat)) {
        bool invalid = !IsEvenNumber(downsamplingParams_.width) || !IsEvenNumber(downsamplingParams_.height);
        CHECK_AND_RETURN_RET_LOG(!invalid, AVCS_ERR_INVALID_VAL,
            "For YUV pixel format input, target width and height of downsampling must be even numbers."
            "width=%{public}d, height=%{public}d", downsamplingParams_.width, downsamplingParams_.height);
    }
    return AVCS_ERR_OK;
}

GraphicPixelFormat Preprocessor::GetPixelFormatFromConfiguration(const Media::Format &format) const
{
    int32_t pixelFormat = static_cast<int32_t>(VideoPixelFormat::NV12);
    (void)format.GetIntValue(Media::Tag::VIDEO_PIXEL_FORMAT, pixelFormat);
    int32_t profile = static_cast<int32_t>(HEVCProfile::HEVC_PROFILE_UNKNOW);
    (void)format.GetIntValue(Media::Tag::MEDIA_PROFILE, profile);
    return ConvertVideoPixelFormat2GraphicPixelFormat(static_cast<VideoPixelFormat>(pixelFormat), profile);
}

GraphicPixelFormat Preprocessor::GetPixelFormatFromParameter(const Media::Format &format) const
{
    bool isConfigured10Bit = (encoderConfig_.pixelFormat == GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010 ||
                              encoderConfig_.pixelFormat == GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010 ||
                              encoderConfig_.pixelFormat == GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010102 ||
                              encoderConfig_.pixelFormat == GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010108 ||
                              encoderConfig_.pixelFormat == GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RAW10);
    int32_t profile = static_cast<int32_t>(HEVCProfile::HEVC_PROFILE_UNKNOW);
    bool hasProfile = format.GetIntValue(Media::Tag::MEDIA_PROFILE, profile);
    if (!hasProfile && isConfigured10Bit) {
        profile = static_cast<int32_t>(HEVCProfile::HEVC_PROFILE_MAIN_10);
    }
    int32_t pixelFormat = static_cast<int32_t>(VideoPixelFormat::NV12);
    bool hasPixelFormat = format.GetIntValue(Media::Tag::VIDEO_PIXEL_FORMAT, pixelFormat);
    if (hasPixelFormat) {
        return ConvertVideoPixelFormat2GraphicPixelFormat(static_cast<VideoPixelFormat>(pixelFormat), profile);
    }

    return encoderConfig_.pixelFormat;
}

int32_t Preprocessor::Configure(const Media::Format &format)
{
    if (cropParams_.result != ConfigResult::NOT_SET) {
        cropConfig_ = CropConfig(cropParams_);
    }
    if (downsamplingParams_.result != ConfigResult::NOT_SET) {
        downsamplingConfig_ = DownsamplingConfig(downsamplingParams_);
    }
    if (dropParams_.result != ConfigResult::NOT_SET) {
        dropFrameConfig_ = DropFrameConfig(dropParams_);
    }
    if (FAST_KIT_ENABLE && (cropConfig_.enabled || downsamplingConfig_.enabled)) {
        int32_t initRet = fastKitsInterface_.Init(static_cast<uint32_t>(encoderParams_.pixelFormat));
        CHECK_AND_RETURN_RET_LOG(initRet == AVCS_ERR_OK, initRet, "Failed to init FastKitsInterface: %{public}d",
                                 initRet);
    }
    if (dropFrameConfig_.enabled) {
        dropFilter_.Configure(dropFrameConfig_.targetFrameRate, encoderParams_.frameRate);
    }

    encoderConfig_.width = encoderParams_.width;
    encoderConfig_.height = encoderParams_.height;
    encoderConfig_.pixelFormat = encoderParams_.pixelFormat;
    encoderConfig_.frameRate = encoderParams_.frameRate;

    AVCODEC_LOGI("PreProcessor configured: crop=%{public}d, ds=%{public}d, drop=%{public}d", cropConfig_.enabled,
                 downsamplingConfig_.enabled, dropFrameConfig_.enabled);
    return AVCS_ERR_OK;
}

bool Preprocessor::UsingAsyncMode(const Media::Format &format) const
{
    int32_t asyncMode = 0;
    (void)format.GetIntValue(Media::Tag::AV_CODEC_ENABLE_SYNC_MODE, asyncMode);
    return asyncMode == 0;
}

int32_t Preprocessor::ValidateCrop(const Media::Format &format)
{
    cropParams_.result = ConfigResult::NOT_SET;
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
    if (!hasAny) { // none of crop values configured, keep current status
        return AVCS_ERR_OK;
    }
    CHECK_AND_RETURN_RET_LOG(IsAvailable(), AVCS_ERR_UNSUPPORT, "Crop are not supported");
    CHECK_AND_RETURN_RET_LOG(!hasAny || hasAll, AVCS_ERR_INVALID_VAL,
                             "Incomplete crop parameters: hasLeft=%{public}d, hasRight=%{public}d, "
                             "hasTop=%{public}d, hasBottom=%{public}d",
                             hasLeft, hasRight, hasTop, hasBottom);
    // all crop values are 0: disable crop
    if (left == DISABLE_CROP_LEFT && top == DISABLE_CROP_TOP && right == DISABLE_CROP_RIGHT &&
        bottom == DISABLE_CROP_BOTTOM) {
        cropParams_.result = ConfigResult::DISABLED;
        AVCODEC_LOGI("Crop disabled by zero crop parameters");
        return AVCS_ERR_OK;
    }
    Range encWidth;
    Range encHeight;
    CHECK_AND_RETURN_RET_LOG(GetEncoderRange(format, encWidth, encHeight), AVCS_ERR_INVALID_VAL,
                             "Failed to get encoder capability for crop");
    constexpr int32_t cropRectInclusiveOffset = 1;
    int32_t cropWidth = right - left + cropRectInclusiveOffset;
    int32_t cropHeight = bottom - top + cropRectInclusiveOffset;
    bool rangeValid = (0 <= left && left < right && right < encoderParams_.width) &&
                      (0 <= top && top < bottom && bottom < encoderParams_.height) &&
                      (encWidth.InRange(cropWidth) && encHeight.InRange(cropHeight));
    CHECK_AND_RETURN_RET_LOG(rangeValid, AVCS_ERR_INVALID_VAL,
        "Invalid crop parameters: left=%{public}d, right=%{public}d, top=%{public}d, bottom=%{public}d",
        left, right, top, bottom);
    cropParams_.left = left;
    cropParams_.right = right;
    cropParams_.top = top;
    cropParams_.bottom = bottom;
    cropParams_.result = ConfigResult::ENABLED;
    return AVCS_ERR_OK;
}

int32_t Preprocessor::ValidateDownsampling(const Media::Format &format)
{
    downsamplingParams_.result = ConfigResult::NOT_SET;
    int32_t width{0};
    int32_t height{0};
    bool hasWidth = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, width);
    bool hasHeight = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT, height);
    if (!hasWidth && !hasHeight) {
        return AVCS_ERR_OK;
    }
    if (!IsAvailable()) {
        AVCODEC_LOGE("Downsampling are not supported");
        return AVCS_ERR_UNSUPPORT;
    }
    if (hasWidth != hasHeight) {
        return AVCS_ERR_INVALID_VAL;
    }
    if (width == DISABLE_DOWNSAMPLING_WIDTH && height == DISABLE_DOWNSAMPLING_HEIGHT) {
        downsamplingParams_.result = ConfigResult::DISABLED;
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
    if (!(minWidth > 0 && minHeight > 0 && minWidth <= width && width <= encoderParams_.width &&
          encoderParams_.width <= maxWidth && minHeight <= height && height <= encoderParams_.height &&
          encoderParams_.height <= maxHeight)) {
        AVCODEC_LOGE("Invalid downsampling parameters: width=%{public}d, height=%{public}d, "
                     "valid range: width [%{public}d, %{public}d], height [%{public}d, %{public}d], "
                     "encoder configured: %{public}dx%{public}d",
                     width, height, minWidth, maxWidth, minHeight, maxHeight, encoderParams_.width,
                     encoderParams_.height);
        return AVCS_ERR_INVALID_VAL;
    }
    downsamplingParams_.width = width;
    downsamplingParams_.height = height;
    downsamplingParams_.result = ConfigResult::ENABLED;
    return AVCS_ERR_OK;
}

int32_t Preprocessor::ValidateDropFrame(const Media::Format &format)
{
    dropParams_.result = ConfigResult::NOT_SET;
    double targetFrameRate = 0;
    if (!format.GetDoubleValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DROP_TO_FRAME_RATE, targetFrameRate)) {
        return AVCS_ERR_OK;
    }
    targetFrameRate = RoundToTwoDecimals(targetFrameRate);
    if (targetFrameRate < -std::numeric_limits<double>::epsilon() ||
        encoderParams_.frameRate < -std::numeric_limits<double>::epsilon()) {
        AVCODEC_LOGE("Invalid drop frame rate: %{public}.2f, configured=%{public}.2f", targetFrameRate,
                     encoderParams_.frameRate);
        return AVCS_ERR_INVALID_VAL;
    }
    if (std::fabs(targetFrameRate) < std::numeric_limits<double>::epsilon()) {
        dropParams_.result = ConfigResult::DISABLED;
        return AVCS_ERR_OK;
    }
    if ((targetFrameRate - encoderParams_.frameRate) < -std::numeric_limits<double>::epsilon()) {
        dropParams_.targetFrameRate = targetFrameRate;
        dropParams_.result = ConfigResult::ENABLED;
        return AVCS_ERR_OK;
    }
    AVCODEC_LOGE("Invalid drop frame rate: %{public}.2f, configured=%{public}.2f", targetFrameRate,
                 encoderParams_.frameRate);
    return AVCS_ERR_INVALID_VAL;
}

int32_t Preprocessor::SetParameter(const Media::Format &format)
{
    return Configure(format);
}

int32_t Preprocessor::Process(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output, uint64_t pts)
{
    CHECK_AND_RETURN_RET_LOG(input && output, AVCS_ERR_INVALID_VAL,
                             "Invalid buffer: input is %{public}s, output is %{public}s", input ? "valid" : "nullptr",
                             output ? "valid" : "nullptr");
    if (IsCropEnabled()) {
        int32_t ret = Crop(input, output);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Crop failed: %{public}d", ret);
        UpdateOutputMetadata(input, output,
                             {IDENTITY_METADATA_SCALE, IDENTITY_METADATA_SCALE, cropConfig_.top,
                              cropConfig_.left});
        return ret;
    }
    if (IsDownsamplingEnabled()) {
        int32_t ret = Downsampling(input, output);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Downsampling failed: %{public}d", ret);
        UpdateOutputMetadata(input, output,
                             {static_cast<double>(output->GetWidth()) / input->GetWidth(),
                              static_cast<double>(output->GetHeight()) / input->GetHeight(),
                              DEFAULT_METADATA_OFFSET, DEFAULT_METADATA_OFFSET});
        return ret;
    }
    int32_t ret = Copy(input, output);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Copy failed: %{public}d", ret);
    UpdateOutputMetadata(input, output,
                         {IDENTITY_METADATA_SCALE, IDENTITY_METADATA_SCALE, DEFAULT_METADATA_OFFSET,
                          DEFAULT_METADATA_OFFSET});
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
    CHECK_AND_RETURN_RET_LOG(FAST_KIT_ENABLE, AVCS_ERR_UNSUPPORT, "Crop requires FastKit");
    PreProcessing::CropRect rect{cropConfig_.top, cropConfig_.left, cropConfig_.bottom, cropConfig_.right};
    int32_t ret = fastKitsInterface_.Crop(input, output, rect);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "FastKit crop failed: %{public}d", ret);
    return AVCS_ERR_OK;
}

int32_t Preprocessor::Downsampling(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output)
{
    if (!CheckOrReallocOutSurfaceBuffer(downsamplingConfig_.width, downsamplingConfig_.height,
                                        input->GetFormat(), output)) {
        AVCODEC_LOGE("Realloc buffer failed");
        return AVCS_ERR_NO_MEMORY;
    }
    CHECK_AND_RETURN_RET_LOG(FAST_KIT_ENABLE, AVCS_ERR_UNSUPPORT, "Downsampling requires FastKit");
    int32_t ret = fastKitsInterface_.DownSample(input, output);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "FastKit downsampling failed: %{public}d", ret);
    return AVCS_ERR_OK;
}

int32_t Preprocessor::Copy(sptr<SurfaceBuffer> input, sptr<SurfaceBuffer> &output)
{
    if (!CheckOrReallocOutSurfaceBuffer(input->GetWidth(), input->GetHeight(), input->GetFormat(), output)) {
        AVCODEC_LOGE("Realloc buffer failed");
        return AVCS_ERR_NO_MEMORY;
    }
    if (FAST_KIT_ENABLE) {
        PreProcessing::CropRect rect{0, 0, input->GetHeight() - 1, input->GetWidth() - 1};
        int32_t ret = fastKitsInterface_.Crop(input, output, rect);
#ifdef SUPPORT_VIDEO_ENCODER_PREPROCESSING
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "FastKit copy failed: %{public}d", ret);
        return AVCS_ERR_OK;
#else
        if (ret == AVCS_ERR_OK) {
            return AVCS_ERR_OK;
        }
#endif
    }
    PreProcessing::CpuImageProcessRequest request;
    request.input = input;
    request.output = output;
    request.operation = PreProcessing::CpuImageOperation::COPY;
    int32_t ret = PreProcessing::CpuImageProcessor::Process(request);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Copy failed: %{public}d", ret);
    return AVCS_ERR_OK;
}

void Preprocessor::UpdateOutputMetadata(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                                        const RoiTransform &roiTransform)
{
    CopyInputMetadata(input, output);
    UpdateRoiMetadata(input, output, roiTransform);
}

void Preprocessor::CopyInputMetadata(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output)
{
    CHECK_AND_RETURN_LOG(input != nullptr && output != nullptr, "Invalid buffer for metadata copy");

    std::vector<uint32_t> keys;
    GSError ret = input->ListMetadataKeys(keys);
    if (ret != GSERROR_OK) {
        return;
    }

    for (uint32_t key : keys) {
        if (key == V2_2::ATTRKEY_ROI_METADATA) {
            continue;
        }
        std::vector<uint8_t> value;
        ret = input->GetMetadata(key, value);
        if (ret != GSERROR_OK) {
            AVCODEC_LOGW("Get metadata failed: key=%{public}u, ret=%{public}d", key, ret);
            continue;
        }
        ret = output->SetMetadata(key, value, false);
        if (ret != GSERROR_OK) {
            AVCODEC_LOGW("Set metadata failed: key=%{public}u, ret=%{public}d", key, ret);
        }
    }
}

void Preprocessor::UpdateRoiMetadata(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                                     const RoiTransform &roiTransform)
{
    CHECK_AND_RETURN_LOG(input != nullptr && output != nullptr, "Invalid buffer for roi metadata!");

    std::vector<uint8_t> roiData;
    if (input->GetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData) != GSERROR_OK || roiData.empty()) {
        return;
    }
    auto nullPos = std::find(roiData.begin(), roiData.end(), '\0');
    std::string roi(roiData.begin(), nullPos);
    std::string transformed = TransformRoiMetadata(roi, roiTransform.scaleX, roiTransform.scaleY,
                                                   roiTransform.offsetTop, roiTransform.offsetLeft);
    std::vector<uint8_t> outputRoi(transformed.begin(), transformed.end());
    outputRoi.push_back('\0');
    GSError ret = output->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, outputRoi, false);
    CHECK_AND_RETURN_LOG(ret == GSERROR_OK, "Set roi metadata failed: %{public}d", ret);
}

bool Preprocessor::IsHevc10BitData(HEVCProfile profile) const
{
    return profile == HEVC_PROFILE_MAIN_10 && mime_ == "video/hevc";
}

GraphicPixelFormat Preprocessor::ConvertVideoPixelFormat2GraphicPixelFormat(VideoPixelFormat pixelFormat,
                                                                            int32_t profile) const
{
    bool is10BitData = IsHevc10BitData(static_cast<HEVCProfile>(profile));
    switch (pixelFormat) {
        case VideoPixelFormat::YUVI420:
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P;
        case VideoPixelFormat::RGBA:
            return is10BitData ? GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010102
                               : GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888;
        case VideoPixelFormat::NV21:
            return is10BitData ? GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010
                               : GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP;
        case VideoPixelFormat::RGBA1010102:
            return GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_1010102;
        case VideoPixelFormat::NV12:
        default:
            return is10BitData ? GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010
                               : GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP;
    }
}

void Preprocessor::UpdateConfiguredValues(const Media::Format &format, bool configuring)
{
    int32_t configuredWidth = 0;
    int32_t configuredHeight = 0;
    double configuredFramerate = 0;
    if (format.GetIntValue(Media::Tag::VIDEO_WIDTH, configuredWidth) &&
        format.GetIntValue(Media::Tag::VIDEO_HEIGHT, configuredHeight)) {
        encoderParams_.width = configuredWidth;
        encoderParams_.height = configuredHeight;
    }

    if (format.GetDoubleValue(Media::Tag::VIDEO_FRAME_RATE, configuredFramerate)) {
        encoderParams_.frameRate = RoundToTwoDecimals(configuredFramerate);
    }

    encoderParams_.pixelFormat =
        configuring ? GetPixelFormatFromConfiguration(format) : GetPixelFormatFromParameter(format);
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
    auto targetGraphicFormat = static_cast<GraphicPixelFormat>(targetFormat);
    if (!IsGeometrySupported(targetGraphicFormat, targetWidth, targetHeight)) {
        AVCODEC_LOGE("Unsupported target geometry: %{public}dx%{public}d format=%{public}d",
                     targetWidth, targetHeight, targetFormat);
        return false;
    }
    int32_t minStride = GetMinRowBytes(targetGraphicFormat, targetWidth);
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
