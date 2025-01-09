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

#include "instance_memory_update_event_handler.h"
#include <functional>
#include <unordered_map>
#include "meta/meta_key.h"
#include "avcodec_info.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "InstanceMemoryCalculator"};
}

namespace OHOS {
namespace MediaAVCodec {
enum class CalculatorParameterPixelFormat {
    YUV420,
    RGBA,
};

enum class BitDepth : int32_t {
    BIT_8,
    BIT_10,
};

CalculatorParameterPixelFormat VideoPixelFormat2CalculatorParameterPixelFormat(VideoPixelFormat format)
{
    if (format == VideoPixelFormat::RGBA) {
        return CalculatorParameterPixelFormat::RGBA;
    }
    return CalculatorParameterPixelFormat::YUV420;
}

struct CalculatorParameter {
    AVCodecType codecType = AVCODEC_TYPE_VIDEO_DECODER;
    std::string mimeType = CodecMimeType::VIDEO_AVC.data();
    CalculatorParameterPixelFormat pixelFormat = CalculatorParameterPixelFormat::YUV420;
    BitDepth bitDepth = BitDepth::BIT_8;
    bool isHardware = true;
    bool enablePostProcessing = false;
};

struct CalculatorParamterHash {
    std::size_t operator()(const CalculatorParameter &param) const
    {
        return (std::hash<int32_t>()(static_cast<int32_t>(param.codecType))         << 0) ^ // 0: Hash offset
               (std::hash<std::string>()(param.mimeType)                            << 1) ^ // 1: Hash offset
               (std::hash<int32_t>()(static_cast<int32_t>(param.pixelFormat))       << 2) ^ // 2: Hash offset
               (std::hash<int32_t>()(static_cast<int32_t>(param.bitDepth))          << 3) ^ // 3: Hash offset
               (std::hash<bool>()(static_cast<int32_t>(param.isHardware))           << 4) ^ // 4: Hash offset
               (std::hash<bool>()(static_cast<int32_t>(param.enablePostProcessing)) << 5);  // 5: Hash offset
    }
};

struct CalculatorParamterEqual {
    bool operator()(const CalculatorParameter &lhs, const CalculatorParameter &rhs) const
    {
        return (lhs.codecType            == rhs.codecType           ) &&
               (lhs.mimeType             == rhs.mimeType            ) &&
               (lhs.pixelFormat          == rhs.pixelFormat         ) &&
               (lhs.bitDepth             == rhs.bitDepth            ) &&
               (lhs.isHardware           == rhs.isHardware          ) &&
               (lhs.enablePostProcessing == rhs.enablePostProcessing);
    }
};

static const CalculatorParameter HARDWARE_DECODER_HEVC_10BIT_YUV420_PARAMETER = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    false
};

static const CalculatorParameter HARDWARE_DECODER_HEVC_10BIT_YUV420_POSTPROCESSING_PARAMETER = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    true
};

static const CalculatorParameter HARDWARE_DECODER_VVC_10BIT_YUV420_PARAMETER = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_VVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    false
};

static const CalculatorParameter HARDWARE_DECODER_VVC_YUV420_PARAMETER = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_VVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

static const CalculatorParameter HARDWARE_DECODER_AVC_YUV420_PARAMETER = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_AVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

static const CalculatorParameter HARDWARE_DECODER_HEVC_YUV420_PARAMETER = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

static const CalculatorParameter HARDWARE_ENCODER_HEVC_10BIT_YUV420_PARAMETER = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    false
};

static const CalculatorParameter HARDWARE_ENCODER_AVC_RGBA_PARAMETER = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_AVC.data(),
    CalculatorParameterPixelFormat::RGBA,
    BitDepth::BIT_8,
    true,
    false
};

static const CalculatorParameter HARDWARE_ENCODER_HEVC_RGBA_PARAMETER = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::RGBA,
    BitDepth::BIT_8,
    true,
    false
};

static const CalculatorParameter HARDWARE_ENCODER_AVC_YUV420_PARAMETER = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_AVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

static const CalculatorParameter HARDWARE_ENCODER_HEVC_YUV420_PARAMETER = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

static const CalculatorParameter SOFTWARE_DECODER_AVC_RGBA_PARAMETER = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_AVC.data(),
    CalculatorParameterPixelFormat::RGBA,
    BitDepth::BIT_8,
    false,
    false
};

static const CalculatorParameter SOFTWARE_DECODER_AVC_YUV420_PARAMETER = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_AVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    false,
    false
};

static const CalculatorParameter SOFTWARE_DECODER_HEVC_YUV420_PARAMETER = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    false,
    false
};

constexpr uint32_t BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1 = 3762;
constexpr uint32_t BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1 = 8036;
constexpr uint32_t BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_5_1 = 36686;

/*******  The unit of memory returned by all calculator is KB  ******/

uint32_t HardwareDecoderHevc10BitYUV420(uint32_t blockSize)
{
    auto linearSlope = 0.0;
    auto linearIntercept = 0U;
    if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1) {
        linearSlope = 9.616;        // 9.616: HardwareDecoderHevc10BitYUV420 level1-3.1 slope
        linearIntercept = 29596;    // 29596: HardwareDecoderHevc10BitYUV420 level1-3.1 intercept
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        linearSlope = 9.270;        // 9.270: HardwareDecoderHevc10BitYUV420 level3.1-4.1 slope
        linearIntercept = 43637;    // 43637: HardwareDecoderHevc10BitYUV420 level3.1-4.1 intercept
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_5_1) {
        linearSlope = 9.319;        // 9.319: HardwareDecoderHevc10BitYUV420 level4.1-5.1 slope
        linearIntercept = 55833;    // 55833: HardwareDecoderHevc10BitYUV420 level4.1-5.1 intercept
    } else {
        linearSlope = 9.319;        // 9.319:  HardwareDecoderHevc10BitYUV420 level15.1+ slope
        linearIntercept = 129228;   // 129228: HardwareDecoderHevc10BitYUV420 level15.1+ intercept
    }
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareDecoderHevc10BitYUV420PostProcessing(uint32_t blockSize)
{
    auto linearSlope = 0.0;
    auto linearIntercept = 0U;
    if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1) {
        linearSlope = 11.428;       // 11.428: HardwareDecoderHevc10BitYUV420PostProcessing level1-3.1 slope
        linearIntercept = 64363;    // 64363:  HardwareDecoderHevc10BitYUV420PostProcessing level1-3.1 intercept
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        linearSlope = 10.999;       // 10.999: HardwareDecoderHevc10BitYUV420PostProcessing level3.1-4.1 slope
        linearIntercept = 78945;    // 78945:  HardwareDecoderHevc10BitYUV420PostProcessing level3.1-4.1 intercept
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_5_1) {
        linearSlope = 11.149;       // 11.149: HardwareDecoderHevc10BitYUV420PostProcessing level4.1-5.1 slope
        linearIntercept = 90380;    // 90380:  HardwareDecoderHevc10BitYUV420PostProcessing level4.1-5.1 intercept
    } else {
        linearSlope = 11.048;       // 11.048: HardwareDecoderHevc10BitYUV420PostProcessing level15.1+ slope
        linearIntercept = 167618;   // 167618: HardwareDecoderHevc10BitYUV420PostProcessing level15.1+ intercept
    }
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareDecoderVvc10BitYUV420(uint32_t blockSize)
{
    auto linearSlope = 0.0;
    auto linearIntercept = 0U;
    if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1) {
        linearSlope = 4.469;        // 4.469: HardwareDecoderVvc10BitYUV420 level1-3.1 slope
        linearIntercept = 13035;    // 13035: HardwareDecoderVvc10BitYUV420 level1-3.1 intercept
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        linearSlope = 4.175;        // 4.175: HardwareDecoderVvc10BitYUV420 level3.1-4.1 slope
        linearIntercept = 20644;    // 20644: HardwareDecoderVvc10BitYUV420 level3.1-4.1 intercept
    } else {
        linearSlope = 4.221;        // 4.221: HardwareDecoderVvc10BitYUV420 level4.1+ slope
        linearIntercept = 32769;    // 32769: HardwareDecoderVvc10BitYUV420 level4.1+ intercept
    }
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareDecoderVvcYUV420(uint32_t blockSize)
{
    AVCODEC_LOGD("Unsupported");
    return 0;
}

uint32_t HardwareDecoderYUV420(uint32_t blockSize)
{
    auto linearSlope = 0.0;
    auto linearIntercept = 0U;
    if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1) {
        linearSlope = 5.702;        // 5.702: HardwareDecoderYUV420 level1-3.1 slope
        linearIntercept = 28934;    // 28934: HardwareDecoderYUV420 level1-3.1 intercept
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        linearSlope = 5.478;        // 5.478: HardwareDecoderYUV420 level3.1-4.1 slope
        linearIntercept = 42493;    // 42493: HardwareDecoderYUV420 level3.1-4.1 intercept
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_5_1) {
        linearSlope = 5.513;        // 5.513: HardwareDecoderYUV420 level4.1-5.1 slope
        linearIntercept = 54700;    // 54700: HardwareDecoderYUV420 level4.1-5.1 intercept
    } else {
        linearSlope = 5.487;        // 5.487:  HardwareDecoderYUV420 level15.1+ slope
        linearIntercept = 129299;   // 129299: HardwareDecoderYUV420 level15.1+ intercept
    }
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareEncoderHevc10BitYUV420(uint32_t blockSize)
{
    auto linearSlope = 5.930;       // 5.930: HardwareEncoderHevc10BitYUV420 slope
    auto linearIntercept = 40394;   // 40394: HardwareEncoderHevc10BitYUV420 intercept
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareEncoder8BitRGBA(uint32_t blockSize)
{
    auto linearSlope = 7.518;       // 7.518: HardwareEncoder8BitRGBA slope
    auto linearIntercept = 6475;    // 6475:  HardwareEncoder8BitRGBA intercept
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareEncoderYUV420(uint32_t blockSize)
{
    auto linearSlope = 4.541;       // 4.541: HardwareEncoderYUV420 slope
    auto linearIntercept = 7989;    // 7989:  HardwareEncoderYUV420 intercept
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t SoftwareDecoderAvcRGBA(uint32_t blockSize)
{
    auto linearSlope = 6.922;       // 6.922: SoftwareDecoderAvcRGBA slope
    auto linearIntercept = 10293;   // 10293: SoftwareDecoderAvcRGBA intercept
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t SoftwareDecoderAvcYUV420(uint32_t blockSize)
{
    auto linearSlope = 3.273;       // 3.273: SoftwareDecoderAvcYUV420 slope
    auto linearIntercept = 10659;   // 10659: SoftwareDecoderAvcYUV420 intercept
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t SoftwareDecoderHevcYUV420(uint32_t blockSize)
{
    auto linearSlope = 1.510;       // 1.510:  SoftwareDecoderHevcYUV420 slope
    auto linearIntercept = 277115;  // 277115: SoftwareDecoderHevcYUV420 intercept
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

const std::unordered_map<CalculatorParameter, uint32_t (*)(uint32_t),
                         CalculatorParamterHash, CalculatorParamterEqual> CALCULATOR_MAP = {
    {HARDWARE_DECODER_HEVC_10BIT_YUV420_PARAMETER, HardwareDecoderHevc10BitYUV420},
    {HARDWARE_DECODER_HEVC_10BIT_YUV420_POSTPROCESSING_PARAMETER, HardwareDecoderHevc10BitYUV420PostProcessing},
    {HARDWARE_DECODER_VVC_10BIT_YUV420_PARAMETER, HardwareDecoderVvc10BitYUV420},
    {HARDWARE_DECODER_VVC_YUV420_PARAMETER, HardwareDecoderVvcYUV420},
    {HARDWARE_DECODER_AVC_YUV420_PARAMETER, HardwareDecoderYUV420},
    {HARDWARE_DECODER_HEVC_YUV420_PARAMETER, HardwareDecoderYUV420},
    {HARDWARE_ENCODER_HEVC_10BIT_YUV420_PARAMETER, HardwareEncoderHevc10BitYUV420},
    {HARDWARE_ENCODER_AVC_RGBA_PARAMETER, HardwareEncoder8BitRGBA},
    {HARDWARE_ENCODER_HEVC_RGBA_PARAMETER, HardwareEncoder8BitRGBA},
    {HARDWARE_ENCODER_AVC_YUV420_PARAMETER, HardwareEncoderYUV420},
    {HARDWARE_ENCODER_HEVC_YUV420_PARAMETER, HardwareEncoderYUV420},
    {SOFTWARE_DECODER_AVC_RGBA_PARAMETER, SoftwareDecoderAvcRGBA},
    {SOFTWARE_DECODER_AVC_YUV420_PARAMETER, SoftwareDecoderAvcYUV420},
    {SOFTWARE_DECODER_HEVC_YUV420_PARAMETER, SoftwareDecoderHevcYUV420}
};

std::optional<CalculatorType> InstanceMemoryUpdateEventHandler::GetCalculator(const Media::Meta &meta)
{
    VideoPixelFormat pixelFormat;
    CalculatorParameter calculatorParameter;
    std::string pixelFormatStr;
    int32_t profile = 0;
    meta.GetData(Media::Tag::VIDEO_PIXEL_FORMAT, pixelFormat);
    meta.GetData(Media::Tag::MIME_TYPE, calculatorParameter.mimeType);
    meta.GetData(EventInfoExtentedKey::CODEC_TYPE.data(), calculatorParameter.codecType);
    meta.GetData(EventInfoExtentedKey::IS_HARDWARE.data(), calculatorParameter.isHardware);
    meta.GetData(EventInfoExtentedKey::ENABLE_POST_PROCESSING.data(), calculatorParameter.enablePostProcessing);
    meta.GetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), pixelFormatStr);
    meta.GetData(Media::Tag::MEDIA_PROFILE, profile);
    calculatorParameter.pixelFormat = VideoPixelFormat2CalculatorParameterPixelFormat(pixelFormat);

    if (pixelFormatStr == "NV12_10bit" ||
        pixelFormatStr == "NV21_10bit" ||
        (calculatorParameter.mimeType == CodecMimeType::VIDEO_HEVC.data() && profile == HEVC_PROFILE_MAIN_10)) {
        calculatorParameter.bitDepth = BitDepth::BIT_10;
    }

    auto it = CALCULATOR_MAP.find(calculatorParameter);
    return (it != CALCULATOR_MAP.end()) ?
        static_cast<CalculatorType>(it->second) :
        [] (uint32_t blockSize) -> uint32_t { return 0; };
}
} // namespace MediaAVCodec
} // namespace OHOS