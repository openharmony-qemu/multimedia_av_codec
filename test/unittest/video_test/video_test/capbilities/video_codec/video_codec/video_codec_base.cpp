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
std::shared_ptr<VideoCodecBase> VideoCodecFactory::CreateVideoCodec(CodecType type)
{
    std::shared_ptr<VideoCodecBase> codec = nullptr;
    switch (type) {
        case VIDEO_HW_DECODER:
        case VIDEO_SW_DECODER:
            codec = std::make_shared<VideoDecoder>();
            break;
        case VIDEO_HW_ENCODER:
            codec = std::make_shared<VideoEncoder>();
            break;
        default:
            AVCODEC_LOGW("Not supported codec type");
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