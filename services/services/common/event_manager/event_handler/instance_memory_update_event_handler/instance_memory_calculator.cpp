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
#include <map>
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

    bool operator < (const CalculatorParameter& other) const
    {
        return (codecType < other.codecType) &&
               (mimeType < other.mimeType) &&
               (pixelFormat < other.pixelFormat) &&
               (bitDepth < other.bitDepth) &&
               (isHardware < other.isHardware) &&
               (enablePostProcessing < other.enablePostProcessing);
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
        linearSlope = 9.616;
        linearIntercept = 29596;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        linearSlope = 9.270;
        linearIntercept = 43637;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_5_1) {
        linearSlope = 9.319;
        linearIntercept = 55833;
    }
    linearSlope = 9.319;
    linearIntercept = 129228;
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareDecoderHevc10BitYUV420PostProcessing(uint32_t blockSize)
{
    auto linearSlope = 0.0;
    auto linearIntercept = 0U;
    if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1) {
        linearSlope = 11.428;
        linearIntercept = 64363;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        linearSlope = 10.999;
        linearIntercept = 78945;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_5_1) {
        linearSlope = 11.149;
        linearIntercept = 90380;
    }
    linearSlope = 11.048;
    linearIntercept = 167618;
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareDecoderVvc10BitYUV420(uint32_t blockSize)
{
    auto linearSlope = 0.0;
    auto linearIntercept = 0U;
    if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1) {
        linearSlope = 4.469;
        linearIntercept = 13035;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        linearSlope = 4.175;
        linearIntercept = 20644;
    }
    linearSlope = 4.221;
    linearIntercept = 32769;
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
        linearSlope = 5.702;
        linearIntercept = 28934;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        linearSlope = 5.478;
        linearIntercept = 42493;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_5_1) {
        linearSlope = 5.513;
        linearIntercept = 54700;
    }
    linearSlope = 5.487;
    linearIntercept = 129299;
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareEncoderHevc10BitYUV420(uint32_t blockSize)
{
    auto linearSlope = 5.930;
    auto linearIntercept = 40394;
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareEncoder8BitRGBA(uint32_t blockSize)
{
    auto linearSlope = 7.518;
    auto linearIntercept = 6475;
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t HardwareEncoderYUV420(uint32_t blockSize)
{
    auto linearSlope = 4.541;
    auto linearIntercept = 7989;
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t SoftwareDecoderAvcRGBA(uint32_t blockSize)
{
    auto linearSlope = 6.922;
    auto linearIntercept = 10293;
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t SoftwareDecoderAvcYUV420(uint32_t blockSize)
{
    auto linearSlope = 3.273;
    auto linearIntercept = 10659;
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

uint32_t SoftwareDecoderHevcYUV420(uint32_t blockSize)
{
    auto linearSlope = 1.510;
    auto linearIntercept = 277115;
    return static_cast<uint32_t>(linearSlope * blockSize + linearIntercept);
}

const std::map<CalculatorParameter, uint32_t (*)(uint32_t)> CALCULATOR_MAP = {
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

    if (pixelFormatStr == "nv12_10bit" ||
        pixelFormatStr == "nv21_10bit" ||
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