/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <functional>
#include <map>
#include "meta/meta_key.h"
#include "avcodec_info.h"
#include "instance_memory_update_event_handler.h"
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
    } else {
        return CalculatorParameterPixelFormat::YUV420;
    }
}

struct CalculatorParameter {
    AVCodecType codecType = AVCODEC_TYPE_VIDEO_DECODER;
    std::string_view mimeType = CodecMimeType::VIDEO_AVC;
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

constexpr static CalculatorParameter HardwareDecoderHevc10BitYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    false
};

constexpr static CalculatorParameter HardwareDecoderHevc10BitYUV420PostProcessingParameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    true
};

constexpr static CalculatorParameter HardwareDecoderVvc10BitYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_VVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    false
};

constexpr static CalculatorParameter HardwareDecoderVvcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_VVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

constexpr static CalculatorParameter HardwareDecoderAvcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_AVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

constexpr static CalculatorParameter HardwareDecoderHevcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

constexpr static CalculatorParameter HardwareEncoderHevc10BitYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_HEVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_10,
    true,
    false
};

constexpr static CalculatorParameter HardwareEncoderAvcRGBAParameter = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_AVC,
    CalculatorParameterPixelFormat::RGBA,
    BitDepth::BIT_8,
    true,
    false
};

constexpr static CalculatorParameter HardwareEncoderHevcRGBAParameter = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_HEVC,
    CalculatorParameterPixelFormat::RGBA,
    BitDepth::BIT_8,
    true,
    false
};

constexpr static CalculatorParameter HardwareEncoderAvcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_AVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

constexpr static CalculatorParameter HardwareEncoderHevcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_ENCODER,
    CodecMimeType::VIDEO_HEVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    true,
    false
};

constexpr static CalculatorParameter SoftwareDecoderAvcRGBAParameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_AVC,
    CalculatorParameterPixelFormat::RGBA,
    BitDepth::BIT_8,
    false,
    false
};

constexpr static CalculatorParameter SoftwareDecoderAvcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_AVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    false,
    false
};

constexpr static CalculatorParameter SoftwareDecoderHevcYUV420Parameter = {
    AVCODEC_TYPE_VIDEO_DECODER,
    CodecMimeType::VIDEO_HEVC,
    CalculatorParameterPixelFormat::YUV420,
    BitDepth::BIT_8,
    false,
    false
};

constexpr uint32_t HARDWARED_ECODER_LEVEL_LEVEL_3_1 = 3762;
constexpr uint32_t HARDWARED_ECODER_LEVEL_LEVEL_4_1 = 8036;
constexpr uint32_t HARDWARED_ECODER_LEVEL_LEVEL_5_1 = 36686;

uint32_t HardwareDecoderHevc10BitYUV420(uint32_t blockSize)
{
    if (blockSize <= HARDWARED_ECODER_LEVEL_LEVEL_3_1) {
        return 0.0094 * blockSize + 28.902;
    } else if (blockSize <= HARDWARED_ECODER_LEVEL_LEVEL_4_1) {
        return 0.0091 * blockSize + 42.614;
    } else if (blockSize <= HARDWARED_ECODER_LEVEL_LEVEL_5_1) {
        return 0.0091 * blockSize + 54.524;
    }
    return 0.0091 * blockSize + 126.2;
}

uint32_t HardwareDecoderHevc10BitYUV420PostProcessing(uint32_t blockSize)
{
    AVCODEC_LOGW("Unsupported");
    return 0;
}

uint32_t HardwareDecoderVvc10BitYUV420(uint32_t blockSize)
{
    AVCODEC_LOGW("Unsupported");
    return 0;
}

uint32_t HardwareDecoderVvcYUV420(uint32_t blockSize)
{
    AVCODEC_LOGW("Unsupported");
    return 0;
}

uint32_t HardwareDecoderYUV420(uint32_t blockSize)
{
    if (blockSize <= HARDWARED_ECODER_LEVEL_LEVEL_3_1) {
        return 0.0056 * blockSize + 28.256;
    } else if (blockSize <= HARDWARED_ECODER_LEVEL_LEVEL_4_1) {
        return 0.0053 * blockSize + 41.502;
    } else if (blockSize <= HARDWARED_ECODER_LEVEL_LEVEL_5_1) {
        return 0.0054 * blockSize + 53.418;
    }
    return 0.0054 * blockSize + 126.27;
}

uint32_t HardwareEncoderHevc10BitYUV420(uint32_t blockSize)
{
    return 0.0058 * blockSize + 39.448;
}

uint32_t HardwareEncoder8BitRGBA(uint32_t blockSize)
{
    return 0.0073 * blockSize + 6.3228;
}

uint32_t HardwareEncoderYUV420(uint32_t blockSize)
{
    return 0.0044 * blockSize + 7.8013;
}

uint32_t SoftwareDecoderAvcRGBA(uint32_t blockSize)
{
    return 0.0068 * blockSize + 10.052;
}

uint32_t SoftwareDecoderAvcYUV420(uint32_t blockSize)
{
    return 0.0032 * blockSize + 10.409;
}

uint32_t SoftwareDecoderHevcYUV420(uint32_t blockSize)
{
    return 0.0015 * blockSize + 270.62;
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
    meta.GetData(Media::Tag::VIDEO_PIXEL_FORMAT, pixelFormat);
    meta.GetData(Media::Tag::MIME_TYPE, calculatorParameter.mimeType);
    meta.GetData(EventInfoExtentedKey::CODEC_TYPE.data(), calculatorParameter.codecType);
    meta.GetData(EventInfoExtentedKey::IS_HARDWARE.data(), calculatorParameter.isHardware);
    meta.GetData(EventInfoExtentedKey::BIT_DEPTH.data(), calculatorParameter.bitDepth);
    meta.GetData(EventInfoExtentedKey::ENABLE_POST_PROCESSING.data(), calculatorParameter.enablePostProcessing);
    calculatorParameter.pixelFormat = VideoPixelFormat2CalculatorParameterPixelFormat(pixelFormat);

    auto it = CALCULATOR_MAP.find(calculatorParameter);
    return (it != CALCULATOR_MAP.end()) ?
        static_cast<CalculatorType>(it->second) :
        [] (uint32_t blockSize) -> uint32_t { return 0; };
}
} // namespace MediaAVCodec
} // namespace OHOS