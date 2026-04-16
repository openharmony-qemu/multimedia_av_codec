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

#include "smart_fluency_decoding_manager.h"
#include "avcodec_common.h"
#include "avcodec_log_ex.h"
#include "avcodec_mime_type.h"
#include "buffer/avallocator.h"
#include "media_description.h"
#include "meta/meta_key.h"
#include "meta/video_types.h"
#include "securec.h"
#ifdef SFD_DEBUG
#include "syspara/parameters.h"
#include <cstdlib>
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "SFDManager"};
constexpr int32_t MIN_CSD_SIZE = 4;
constexpr int32_t DEFAULT_CSD_COPY_SIZE = 256;
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
SmartFluencyDecodingManager::SmartFluencyDecodingManager(const std::string& codecMime, AVCodecType codecType)
    : codecMime_(codecMime), codecType_(codecType)
{
}

void SmartFluencyDecodingManager::SetCallbacks(std::function<void(uint32_t, std::shared_ptr<AVBuffer>)> inputCb,
                                               std::function<void(uint32_t)> outputCb)
{
    inputCb_ = std::move(inputCb);
    outputCb_ = std::move(outputCb);
}

void SmartFluencyDecodingManager::Configure(const Media::Format& format)
{
    if (!IsCodecSupported(codecMime_, codecType_)) {
        AVCODEC_LOGI_WITH_TAG("Skip SFD configuration (not supported)");
        return;
    }
    Media::Format sfdFormat = format;
#ifdef SFD_DEBUG
    ApplyDebugConfig(sfdFormat);
#endif
    if (sfd_ != nullptr) {
        builder_->SetFromFormat(sfdFormat);
        sfd_->UpdateDynamicConfig(sfdFormat);
        AVCODEC_LOGI_WITH_TAG("SFD dynamically updated via Configure.");
        return;
    }
    if (builder_ == nullptr) {
        builder_ = std::make_unique<SmartFluencyDecodingBuilder>();
        builder_->SetCodecMime(codecMime_);
        AVCODEC_LOGE_WITH_TAG("CSD_DEBUG: Builder created for mime=%{public}s", codecMime_.c_str());
    }
    builder_->SetFromFormat(sfdFormat);
    auto sfd = builder_->Build();

    if (sfd) {
        if (isCsdCached_ && !cachedCsdData_.empty()) {
            sfd->InitializeWithCsd(cachedCsdData_);
        }
        sfd_ = std::move(sfd);
        if (inputCb_ && outputCb_) {
            sfd_->SetCallbacks(inputCb_, outputCb_);
        }
        AVCODEC_LOGE_WITH_TAG("CSD_DEBUG: SFD initialized successfully");
    }
}

void SmartFluencyDecodingManager::OnInputBufferAvailable(uint32_t index, const std::shared_ptr<AVBuffer>& buffer)
{
    if (buffer == nullptr) {
        AVCODEC_LOGW_WITH_TAG("OnInputBufferAvailable: buffer is null for index %{public}u", index);
        return;
    }
    if (codecType_ == AVCODEC_TYPE_VIDEO_DECODER && !isCsdCached_) {
        std::lock_guard<std::mutex> csdLock(tempCsdMutex_);
        tempCsdBufferMap_[index] = buffer;
    }
    if (sfd_) {
        sfd_->SetInputBuffer(index, buffer);
    }
}

bool SmartFluencyDecodingManager::MakePreDecodeDecision(uint32_t index)
{
#ifdef SFD_DEBUG
    CheckAndTriggerDebugConfig();
#endif
    CacheCsdData(index);
    if (sfd_) {
        bool decision = sfd_->MakePreDecodeDecision(index);
        if (!decision) {
            AVCODEC_LOGE_WITH_TAG("CSD_DEBUG: Pre-decode drop decision for index %{public}u", index);
        }
        return decision;
    }
    return true;
}

bool SmartFluencyDecodingManager::MakePostDecodeDecision(uint32_t index, const std::shared_ptr<AVBuffer>& buffer, bool isSurfaceMode)
{
    if (buffer == nullptr) {
        AVCODEC_LOGE_WITH_TAG("MakePostDecodeDecision: buffer is null for index %{public}u", index);
        return true;
    }
    uint64_t uniqueId = buffer->GetUniqueId();
    if (isSurfaceMode && codecType_ == AVCODEC_TYPE_VIDEO_DECODER && buffer->memory_ != nullptr) {
        std::lock_guard<std::mutex> lock(surfaceBufMutex_);
        surfaceBufMap_[uniqueId] = buffer->memory_->GetSurfaceBuffer();
        buffer->memory_ = nullptr;
    }
    if (sfd_ && (buffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) == 0) {
        sptr<SurfaceBuffer> surfaceBuffer = nullptr;
        {
            std::lock_guard<std::mutex> lock(surfaceBufMutex_);
            auto it = surfaceBufMap_.find(uniqueId);
            if (it != surfaceBufMap_.end()) {
                surfaceBuffer = it->second;
            }
        }
        if (!sfd_->MakePostDecodeDecision(index, buffer->pts_, surfaceBuffer)) {
            AVCODEC_LOGE_WITH_TAG("CSD_DEBUG: Post-decode drop decision for index %{public}u, pts %{public}" PRId64,
                                 index, buffer->pts_);
            return false;
        }
    }
    return true;
}

void SmartFluencyDecodingManager::ExtractCsdData(const std::shared_ptr<AVBuffer>& buffer)
{
    if (!buffer || !buffer->memory_) {
        AVCODEC_LOGD_WITH_TAG("CSD_DEBUG: ExtractCsdData failed, buffer or memory is null");
        return;
    }
    uint8_t* data = buffer->memory_->GetAddr();
    int32_t size = buffer->memory_->GetSize();
    if (!data || size <= MIN_CSD_SIZE) {
        AVCODEC_LOGD_WITH_TAG("CSD_DEBUG: ExtractCsdData failed, data is null or size (=%{public}d) <= MIN (%{public}d)",
                              size, MIN_CSD_SIZE);
        return;
    }
    int32_t currentSize = static_cast<int32_t>(cachedCsdData_.size());
    if (currentSize >= DEFAULT_CSD_COPY_SIZE) {
        return;
    }
    int32_t needed = DEFAULT_CSD_COPY_SIZE - currentSize;
    int32_t copySize = std::min(size, needed);
    if (buffer->flag_ == AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        cachedCsdData_.insert(cachedCsdData_.end(), data, data + copySize);
        AVCODEC_LOGI_WITH_TAG("CSD_DEBUG: Appended %{public}d bytes CODEC_DATA. Total CSD size: %{public}zu",
                              copySize, cachedCsdData_.size());
        return;
    }
    if (buffer->flag_ & (AVCODEC_BUFFER_FLAG_CODEC_DATA | AVCODEC_BUFFER_FLAG_SYNC_FRAME)) {
        cachedCsdData_.insert(cachedCsdData_.end(), data, data + copySize);
        AVCODEC_LOGI_WITH_TAG("CSD_DEBUG: Fast appended %{public}d bytes with flags %{public}u. Total CSD size: %{public}zu",
                              copySize, buffer->flag_, cachedCsdData_.size());
        return;
    }
    AVCODEC_LOGD_WITH_TAG("CSD_DEBUG: ExtractCsdData didn't match flags, buffer->flag_=%{public}u", buffer->flag_);
}

void SmartFluencyDecodingManager::CacheCsdData(uint32_t index)
{
    if (codecType_ != AVCODEC_TYPE_VIDEO_DECODER || isCsdCached_) {
        return;
    }
    std::shared_ptr<AVBuffer> csdAttemptBuffer = nullptr;
    {
        std::lock_guard<std::mutex> csdLock(tempCsdMutex_);
        auto it = tempCsdBufferMap_.find(index);
        if (it != tempCsdBufferMap_.end()) {
            csdAttemptBuffer = it->second;
            tempCsdBufferMap_.erase(it);
        }
    }
    if (csdAttemptBuffer) {
        ExtractCsdData(csdAttemptBuffer);
        bool isEnoughSize = (cachedCsdData_.size() >= DEFAULT_CSD_COPY_SIZE);
        bool isSyncFrame = ((csdAttemptBuffer->flag_ & AVCODEC_BUFFER_FLAG_SYNC_FRAME) != 0);

        if (!cachedCsdData_.empty() && (isEnoughSize || isSyncFrame)) {
            isCsdCached_ = true;
            // ´òÓ¡Ç° 16 ×Ö½Ú
            std::string hexStr;
            char buf[8] = {0};
            size_t printSize = std::min(cachedCsdData_.size(), static_cast<size_t>(16));
            for (size_t i = 0; i < printSize; ++i) {
                snprintf(buf, sizeof(buf), "%02X ", cachedCsdData_[i]);
                hexStr += buf;
            }
            AVCODEC_LOGI_WITH_TAG("CSD_DEBUG: *** CSD Finalized *** (Size: %{public}zu), First %{public}zu bytes: [%{public}s]",
                                  cachedCsdData_.size(), printSize, hexStr.c_str());
            std::lock_guard<std::mutex> csdLock(tempCsdMutex_);
            tempCsdBufferMap_.clear();
        }
    }
}

void SmartFluencyDecodingManager::OnOutputBufferUnbinded()
{
    std::lock_guard<std::mutex> lock(surfaceBufMutex_);
    surfaceBufMap_.clear();
}

void SmartFluencyDecodingManager::OnDecodingPerformanceUpdate(double speed, double decFps)
{
    if (sfd_) {
        sfd_->OnDecodingPerformanceUpdate(speed, decFps);
    }
}

void SmartFluencyDecodingManager::Flush()
{
    if (sfd_) {
        sfd_->Flush();
    }
}

bool SmartFluencyDecodingManager::IsSfdActive() const
{
    return sfd_ != nullptr;
}

void SmartFluencyDecodingManager::EnableMVOutput(Media::Format& format)
{
    if (sfd_ != nullptr ||
        codecMime_ == MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC ||
        codecMime_ == MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_VVC) {
        format.PutIntValue(Media::Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, 1);
    }
}

bool SmartFluencyDecodingManager::IsCodecSupported(const std::string& codecMime, AVCodecType codecType)
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

void SmartFluencyDecodingManager::CleanResources()
{
    isCsdCached_ = false;
    cachedCsdData_.clear();
    {
        std::lock_guard<std::mutex> csdLock(tempCsdMutex_);
        tempCsdBufferMap_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(surfaceBufMutex_);
        surfaceBufMap_.clear();
    }
    if (sfd_) {
        sfd_ = nullptr;
    }
#ifdef SFD_DEBUG
    inputFrameCount_ = 0;
    isDebugConfigInitialized_ = false;
#endif
}

#ifdef SFD_DEBUG
void SmartFluencyDecodingManager::ApplyDebugConfig(Media::Format& format)
{
    if (!isDebugConfigInitialized_) {
        configRetentionMode_ =
            std::strtol(OHOS::system::GetParameter("sfd.debug.mode", "-1").c_str(), nullptr, 10);
        configRetentionRatio_ =
            std::strtod(OHOS::system::GetParameter("sfd.debug.ratio", "-1.0").c_str(), nullptr);
        configDecSpeed_ = std::strtod(OHOS::system::GetParameter("sfd.debug.speed", "-1.0").c_str(), nullptr);

        triggerFrameCnt_ =
            std::strtol(OHOS::system::GetParameter("sfd.debug.trigger_frame", "-1").c_str(), nullptr, 10);
        newRetentionMode_ =
            std::strtol(OHOS::system::GetParameter("sfd.debug.new_mode", "-1").c_str(), nullptr, 10);
        newRetentionRatio_ =
            std::strtod(OHOS::system::GetParameter("sfd.debug.new_ratio", "-1.0").c_str(), nullptr);
        newDecSpeed_ = std::strtod(OHOS::system::GetParameter("sfd.debug.new_speed", "-1.0").c_str(), nullptr);
        inputFrameCount_ = 0;
        isDebugConfigInitialized_ = true;
        AVCODEC_LOGI_WITH_TAG("SFD Debug: Conf: %{public}d, %{public}.2f, %{public}.2f; Trg:%{public}d -> "
            "%{public}d, %{public}.2f, %{public}.2f", configRetentionMode_, configRetentionRatio_, configDecSpeed_,
            triggerFrameCnt_, newRetentionMode_, newRetentionRatio_, newDecSpeed_);
    }

    if (configRetentionMode_ >= 0) {
        format.PutIntValue(Media::Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, configRetentionMode_);
    }
    if (configRetentionRatio_ >= 0.0) {
        format.PutDoubleValue(Media::Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, configRetentionRatio_);
    }
    if (configDecSpeed_ >= 0.0) {
        format.PutDoubleValue(Media::Tag::VIDEO_DECODER_SPEED, configDecSpeed_);
    }
}

void SmartFluencyDecodingManager::CheckAndTriggerDebugConfig()
{
    inputFrameCount_++;
    if (triggerFrameCnt_ > 0 && inputFrameCount_ == static_cast<uint32_t>(triggerFrameCnt_)) {
        if (newRetentionMode_ >= 0) {
            configRetentionMode_ = newRetentionMode_;
        }
        if (newRetentionRatio_ >= 0.0) {
            configRetentionRatio_ = newRetentionRatio_;
        };
        if (newDecSpeed_ >= 0.0) {
            configDecSpeed_ = newDecSpeed_;
        }
        AVCODEC_LOGI_WITH_TAG("SFD Debug: Trg at %{public}u. M:%{public}d, R:%{public}.2f, S:%{public}.2f",
            inputFrameCount_, newRetentionMode_, newRetentionRatio_, newDecSpeed_);
        Media::Format emptyFormat;
        Configure(emptyFormat);
    }
}
#endif
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
