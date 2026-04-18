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

#include "smart_fluency_decoding_builder.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "buffer/avallocator.h"
#include "media_description.h"
#include "meta/meta_key.h"
#include "meta/video_types.h"
#include "smart_fluency_decoding.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "SFDBuilder"};
constexpr double DEFAULT_PLAYBACK_SPEED = 1.0;
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
namespace SFD {
SmartFluencyDecodingBuilder::SmartFluencyDecodingBuilder()
{
}

SmartFluencyDecodingBuilder::~SmartFluencyDecodingBuilder() = default;

SmartFluencyDecodingBuilder& SmartFluencyDecodingBuilder::SetVideoWidth(int32_t width)
{
    config_.width = width;
    return *this;
}

SmartFluencyDecodingBuilder& SmartFluencyDecodingBuilder::SetVideoHeight(int32_t height)
{
    config_.height = height;
    return *this;
}

SmartFluencyDecodingBuilder& SmartFluencyDecodingBuilder::SetCodecMime(const std::string& mime)
{
    config_.codecType = ParseCodecType(mime);
    return *this;
}

SFDCodecType SmartFluencyDecodingBuilder::ParseCodecType(const std::string& mime)
{
    if (mime == AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_AVC) {
        return SFDCodecType::AVC;
    } else if (mime == AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) {
        return SFDCodecType::HEVC;
    } else if (mime == AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_VVC) {
        return SFDCodecType::VVC;
    }
    return SFDCodecType::INVALID;
}

SmartFluencyDecodingBuilder& SmartFluencyDecodingBuilder::UpdatePlaybackSpeed(double speed)
{
    playbackSpeed_ = speed;
    return *this;
}

SmartFluencyDecodingBuilder& SmartFluencyDecodingBuilder::SetFromFormat(const Format& format)
{
    int32_t width = 0;
    if (format.GetIntValue(Tag::VIDEO_WIDTH, width) && width > 0) {
        SetVideoWidth(width);
    }
    int32_t height = 0;
    if (format.GetIntValue(Tag::VIDEO_HEIGHT, height) && height > 0) {
        SetVideoHeight(height);
    }
    RetentionStrategyType parsedMode = RetentionStrategyType::INVALID;
    std::optional<double> parsedRatio = std::nullopt;
    SmartFluencyDecoding::ParseRetentionConfig(format, parsedMode, parsedRatio);
    config_.initMode = parsedMode;
    config_.retentionRatio = parsedRatio;
    double speed = DEFAULT_PLAYBACK_SPEED;
    if (format.GetDoubleValue(Tag::VIDEO_DECODER_SPEED, speed)) {
        UpdatePlaybackSpeed(speed);
    }
    return *this;
}

std::unique_ptr<SmartFluencyDecoding> SmartFluencyDecodingBuilder::Build()
{
    if (!ValidateParams()) {
        return nullptr;
    }
    if (config_.initMode == RetentionStrategyType::INVALID) {
        return nullptr;
    }
    auto instance = std::make_unique<SmartFluencyDecoding>();
    CHECK_AND_RETURN_RET_LOG(instance != nullptr, nullptr, "Failed to create SmartFluencyDecoding instance");
    int32_t ret = instance->Initialize(config_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Failed to initialize SmartFluencyDecoding"
        ",ret=%{public}d", ret);
    if (playbackSpeed_.has_value()) {
        instance->UpdatePlaybackSpeed(playbackSpeed_.value());
    }
    AVCODEC_LOGI("SmartFluencyDecoding built successfully: %{public}dx%{public}d, mode=%{public}d",
        config_.width, config_.height, static_cast<int32_t>(config_.initMode));
    return instance;
}

bool SmartFluencyDecodingBuilder::ValidateParams()
{
    CHECK_AND_RETURN_RET_LOG(config_.width > 0, false, "Invalid or missing video width");
    CHECK_AND_RETURN_RET_LOG(config_.height > 0, false, "Invalid or missing video height");
    CHECK_AND_RETURN_RET_LOG(config_.codecType != SFDCodecType::INVALID, false,
        "Invalid or missing codec type");
    return true;
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
