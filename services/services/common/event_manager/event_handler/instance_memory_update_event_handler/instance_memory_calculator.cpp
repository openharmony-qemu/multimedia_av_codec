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

static CalculatorParameter HardwareDecoderHevc10BitYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    false
};

static CalculatorParameter HardwareDecoderHevc10BitYUV420PostProcessingParameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    true
};

static CalculatorParameter HardwareDecoderVvc10BitYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_VVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    false
};

static CalculatorParameter HardwareDecoderVvcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_VVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

static CalculatorParameter HardwareDecoderAvcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_AVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

static CalculatorParameter HardwareDecoderHevcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

static CalculatorParameter HardwareEncoderHevc10BitYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    false
};

static CalculatorParameter HardwareEncoderAvcRGBAParameter = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_AVC.data(),
    CalculatorParameterPixelFormat::RGBA,
    BitDepth::BIT_8,
    true,
    false
};

static CalculatorParameter HardwareEncoderHevcRGBAParameter = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::RGBA,
    BitDepth::BIT_8,
    true,
    false
};

static CalculatorParameter HardwareEncoderAvcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_AVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

static CalculatorParameter HardwareEncoderHevcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_HEVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

static CalculatorParameter SoftwareDecoderAvcRGBAParameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_AVC.data(),
    CalculatorParameterPixelFormat::RGBA,
    BitDepth::BIT_8,
    false,
    false
};

static CalculatorParameter SoftwareDecoderAvcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_AVC.data(),
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    false,
    false
};

static CalculatorParameter SoftwareDecoderHevcYUV420Parameter = {
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
    if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1) {
        return 9.616 * blockSize + 29596;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        return 9.270 * blockSize + 43637;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_5_1) {
        return 9.319 * blockSize + 55833;
    }
    return 9.319 * blockSize + 129228;
}

uint32_t HardwareDecoderHevc10BitYUV420PostProcessing(uint32_t blockSize)
{
    if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1) {
        return 11.428 * blockSize + 64363;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        return 10.999 * blockSize + 78945;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_5_1) {
        return 11.149 * blockSize + 90380;
    }
    return 11.048 * blockSize + 167618;
}

uint32_t HardwareDecoderVvc10BitYUV420(uint32_t blockSize)
{
    if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1) {
        return 4.469 * blockSize + 13035;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        return 4.175 * blockSize + 20644;
    }
    return 4.221 * blockSize + 32769;
}

uint32_t HardwareDecoderVvcYUV420(uint32_t blockSize)
{
    AVCODEC_LOGD("Unsupported");
    return 0;
}

uint32_t HardwareDecoderYUV420(uint32_t blockSize)
{
    if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_3_1) {
        return 5.702 * blockSize + 28934;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_4_1) {
        return 5.478 * blockSize + 42493;
    } else if (blockSize <= BLOCK_SIZE_HARDWARED_PROFILE_LEVEL_5_1) {
        return 5.513 * blockSize + 54700;
    }
    return 5.487 * blockSize + 129299;
}

uint32_t HardwareEncoderHevc10BitYUV420(uint32_t blockSize)
{
    return 5.930 * blockSize + 40394;
}

uint32_t HardwareEncoder8BitRGBA(uint32_t blockSize)
{
    return 7.518 * blockSize + 6475;
}

uint32_t HardwareEncoderYUV420(uint32_t blockSize)
{
    return 4.541 * blockSize + 7989;
}

uint32_t SoftwareDecoderAvcRGBA(uint32_t blockSize)
{
    return 6.922 * blockSize + 10293;
}

uint32_t SoftwareDecoderAvcYUV420(uint32_t blockSize)
{
    return 3.273 * blockSize + 10659;
}

uint32_t SoftwareDecoderHevcYUV420(uint32_t blockSize)
{
    return 1.510 * blockSize + 277115;
}

const std::map<CalculatorParameter, uint32_t (*)(uint32_t)> CALCULATOR_MAP = {
    {HardwareDecoderHevc10BitYUV420Parameter, HardwareDecoderHevc10BitYUV420},
    {HardwareDecoderHevc10BitYUV420PostProcessingParameter, HardwareDecoderHevc10BitYUV420PostProcessing},
    {HardwareDecoderVvc10BitYUV420Parameter, HardwareDecoderVvc10BitYUV420},
    {HardwareDecoderVvcYUV420Parameter, HardwareDecoderVvcYUV420},
    {HardwareDecoderAvcYUV420Parameter, HardwareDecoderYUV420},
    {HardwareDecoderHevcYUV420Parameter, HardwareDecoderYUV420},
    {HardwareEncoderHevc10BitYUV420Parameter, HardwareEncoderHevc10BitYUV420},
    {HardwareEncoderAvcRGBAParameter, HardwareEncoder8BitRGBA},
    {HardwareEncoderHevcRGBAParameter, HardwareEncoder8BitRGBA},
    {HardwareEncoderAvcYUV420Parameter, HardwareEncoderYUV420},
    {HardwareEncoderHevcYUV420Parameter, HardwareEncoderYUV420},
    {SoftwareDecoderAvcRGBAParameter, SoftwareDecoderAvcRGBA},
    {SoftwareDecoderAvcYUV420Parameter, SoftwareDecoderAvcYUV420},
    {SoftwareDecoderHevcYUV420Parameter, SoftwareDecoderHevcYUV420}
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