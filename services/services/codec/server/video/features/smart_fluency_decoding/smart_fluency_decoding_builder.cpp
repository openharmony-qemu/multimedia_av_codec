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
    params_.width = width;
    return *this;
}

SmartFluencyDecodingBuilder& SmartFluencyDecodingBuilder::SetVideoHeight(int32_t height)
{
    params_.height = height;
    return *this;
}

SmartFluencyDecodingBuilder& SmartFluencyDecodingBuilder::SetCodecMime(const std::string& mime)
{
    params_.codecType = ParseCodecType(mime);
    return *this;
}

SFDCodecType SmartFluencyDecodingBuilder::ParseCodecType(const std::string& mime)
{
    if (mime == AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_AVC || mime == "video/h264") {
        return SFDCodecType::AVC;
    } else if (mime == AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC || mime == "video/h265") {
        return SFDCodecType::HEVC;
    } else if (mime == AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_VVC || mime == "video/h266") {
        return SFDCodecType::VVC;
    }
    return SFDCodecType::INVALID;
}


SmartFluencyDecodingBuilder& SmartFluencyDecodingBuilder::UpdatePlaybackSpeed(double speed)
{
    playbackSpeed_ = speed;
    return *this;
}

bool SmartFluencyDecodingBuilder::IsCodecSupported(const std::string& codecMime, int32_t codecType)
{
    if (codecType != MediaAVCodec::AVCODEC_TYPE_VIDEO_DECODER) {
        return false;
    }

    if (codecMime != MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_AVC &&
        codecMime != MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC &&
        codecMime != MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_VVC) {
        return false;
    }
    return true;
}

void SmartFluencyDecodingBuilder::EnableMVOutput(Format& format)
{
    format.PutIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, 1);
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
    int32_t extMode = -1;
    bool hasMode = format.GetIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, extMode);

    double ratio = 0.0;
    bool hasRatio = format.GetDoubleValue(Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, ratio) && IsRetentionRateValid(ratio);
    if (hasRatio) {
        params_.retentionRatio = ratio;
    }

    if (!hasMode) {
        params_.initMode = hasRatio ? FrameDropMode::UNIFORM_CUSTOM : FrameDropMode::INVALID;
    } else {
        switch (extMode) {
            case static_cast<int32_t>(Media::Plugins::FrameRetentionMode::FULL):
                params_.initMode = FrameDropMode::FULL;
                break;
            case static_cast<int32_t>(Media::Plugins::FrameRetentionMode::ADAPTIVE):
                params_.initMode = FrameDropMode::ADAPTIVE;
                break;
            case static_cast<int32_t>(Media::Plugins::FrameRetentionMode::UNIFORM):
                params_.initMode = hasRatio ? FrameDropMode::UNIFORM_CUSTOM : FrameDropMode::UNIFORM_DEFAULT;
                break;
            default:
                params_.initMode = FrameDropMode::INVALID;
                break;
        }
    }

    if (hasRatio && (params_.initMode == FrameDropMode::ADAPTIVE || params_.initMode == FrameDropMode::FULL)) {
        AVCODEC_LOGW("Adaptive or Full mode with retentionRate will be ignored");
    }
    double speed = 1.0;
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

    if (params_.initMode == FrameDropMode::INVALID) {
        return nullptr;
    }

    auto instance = std::make_unique<SmartFluencyDecoding>();
    if (!instance) {
        AVCODEC_LOGE("Build failed: Failed to create SmartFluencyDecoding instance");
        return nullptr;
    }

    int32_t ret = instance->Initialize(params_);

    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Build failed: Failed to initialize SmartFluencyDecoding, ret=%{public}d", ret);
        return nullptr;
    }

    if (playbackSpeed_.has_value()) {
        instance->UpdatePlaybackSpeed(playbackSpeed_.value());
    }

    AVCODEC_LOGI("SmartFluencyDecoding built successfully: %{public}dx%{public}d, mode=%{public}d",
                 params_.width, params_.height, static_cast<int32_t>(params_.initMode));
    
    return instance;
}


bool SmartFluencyDecodingBuilder::ValidateParams()
{
    if (params_.width <= 0) {
        AVCODEC_LOGE("Build failed: Invalid or missing video width");
        return false;
    }
    if (params_.height <= 0) {
        AVCODEC_LOGE("Build failed: Invalid or missing video height");
        return false;
    }
    if (params_.codecType == SFDCodecType::INVALID) {
        AVCODEC_LOGE("Build failed: Invalid or missing codec type");
        return false;
    }
    return true;
}

bool SmartFluencyDecodingBuilder::IsRetentionRateValid(double rate)
{
    if (rate < 0.01) {
        AVCODEC_LOGW("retentionRate invalid: value=%{public}.2f (must be >= 0.01)", rate);
        return false;
    }
    if (rate > 1.0) {
        AVCODEC_LOGW("retentionRate invalid: value=%{public}.2f (must be <= 1.0)", rate);
        return false;
    }
    return true;
}


std::vector<uint8_t> SmartFluencyDecodingBuilder::ExtractCsdData(const std::shared_ptr<Media::AVBuffer>& buffer)
{
    std::vector<uint8_t> csdData;
    if (!buffer || !buffer->memory_) return csdData;
    
    uint8_t* data = buffer->memory_->GetAddr();
    int32_t size = buffer->memory_->GetCapacity();
    if (!data || size <= 4) return csdData;

    // Scenario A: Pure CODEC_DATA Buffer, full copy
    if (buffer->flag_ == AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        csdData.assign(data, data + size);
        return csdData;
    }

    // Scenario B: Mixed CODEC_DATA or SYNC_FRAME, copy first 256 bytes
    if (buffer->flag_ & (AVCODEC_BUFFER_FLAG_CODEC_DATA | AVCODEC_BUFFER_FLAG_SYNC_FRAME)) {
        int32_t copySize = std::min(size, 256);
        csdData.assign(data, data + copySize);
        AVCODEC_LOGI("Fast extracted %{public}d bytes as CSD from buffer with flags %{public}u", copySize, buffer->flag_);
    }
    
    return csdData;
}

void SmartFluencyDecodingBuilder::ReplayCsdData(SmartFluencyDecoding* sfd, const std::vector<uint8_t>& csdData)
{
    if (!sfd || csdData.empty()) return;

    std::shared_ptr<Media::AVAllocator> allocator = Media::AVAllocatorFactory::CreateSharedAllocator(Media::MEMORY_READ_WRITE);
    if (!allocator) {
        AVCODEC_LOGE("Failed to create shared allocator for replay FakeBuffer");
        return;
    }
    
    auto fakeBuffer = Media::AVBuffer::CreateAVBuffer(allocator, static_cast<int32_t>(csdData.size()));
    if (fakeBuffer && fakeBuffer->memory_) {
        // 直接执行内存拷贝
        int32_t capacity = fakeBuffer->memory_->GetCapacity();
        if (capacity >= static_cast<int32_t>(csdData.size())) {
            memcpy(fakeBuffer->memory_->GetAddr(), csdData.data(), csdData.size());
            fakeBuffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;
            
            // 提交给已初始化的 SFD 进行前置探测（仅作用于底层解码器解析属性）
            sfd->InitializeWithCsd(fakeBuffer);
            AVCODEC_LOGI("SFD Mid-stream enabled: Replayed 256-bytes deep-copied CSD data successfully.");
        }
    }
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
