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

#include "video_codec_base.h"
#include "av_codec_sample_log.h"
#include "native_avcapability.h"

#include "video_decoder.h"
#include "video_encoder.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoCodecBase"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
std::shared_ptr<VideoCodecBase> VideoCodecFactory::CreateVideoCodec(CodecType type, CodecRunMode mode)
{
    std::shared_ptr<VideoCodecBase> codec = nullptr;
    switch (type) {
        case VIDEO_HW_DECODER:
        case VIDEO_SW_DECODER:
            codec = CreateVideoDecoder(mode);
            break;
        case VIDEO_HW_ENCODER:
        case VIDEO_SW_ENCODER:
            codec = CreateVideoEncoder(mode);
            break;
        default:
            AVCODEC_LOGW("Not supported codec type, %{public}d", type);
    }
    return codec;
}

std::shared_ptr<VideoCodecBase> VideoCodecFactory::CreateVideoDecoder(CodecRunMode mode)
{
    std::shared_ptr<VideoCodecBase> codec = nullptr;
    switch (mode) {
        case API10_SURFACE:
            codec = std::make_shared<VideoDecoderAPI10Surface>();
            break;
        case API10_BUFFER:
            codec = std::make_shared<VideoDecoderAPI10Buffer>();
            break;
        case API11_SURFACE:
            codec = std::make_shared<VideoDecoderAPI11Surface>();
            break;
        case API11_BUFFER:
            codec = std::make_shared<VideoDecoderAPI11Buffer>();
            break;
        default:
            AVCODEC_LOGW("Not supported codec run mode: %{public}d", mode);
    }
    return codec;
}

std::shared_ptr<VideoCodecBase> VideoCodecFactory::CreateVideoEncoder(CodecRunMode mode)
{
    std::shared_ptr<VideoCodecBase> codec = nullptr;
    switch (mode) {
        case API10_SURFACE:
            codec = std::make_shared<VideoEncoderAPI10Surface>();
            break;
        case API10_BUFFER:
            codec = std::make_shared<VideoEncoderAPI10Buffer>();
            break;
        case API11_SURFACE:
            codec = std::make_shared<VideoEncoderAPI11Surface>();
            break;
        case API11_BUFFER:
            codec = std::make_shared<VideoEncoderAPI11Buffer>();
            break;
        default:
            AVCODEC_LOGW("Not supported codec run mode: %{public}d", mode);
    }
    return codec;
}

std::string VideoCodecBase::GetCodecName(const std::string &codecMime, bool isEncoder, bool isSoftware)
{
    auto capability = OH_AVCodec_GetCapabilityByCategory(codecMime.c_str(), isEncoder,
        isSoftware ? OH_AVCodecCategory::SOFTWARE : OH_AVCodecCategory::HARDWARE);
    return OH_AVCapability_GetName(capability);
}
} // Sample
} // MediaAVCodec
} // OHOS