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
#include "double_utils.h"
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
#include <limits>
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "SFDManager"};
constexpr int32_t MIN_CSD_SIZE = 4;
constexpr int32_t DEFAULT_CSD_COPY_SIZE = 4096;

#ifdef SFD_DEBUG
constexpr const char* DEBUG_PARAM_MODE = "sfd.debug.mode";
constexpr const char* DEBUG_PARAM_RATIO = "sfd.debug.ratio";
constexpr const char* DEBUG_PARAM_SPEED = "sfd.debug.speed";
constexpr const char* DEBUG_PARAM_TRIGGER_FRAME = "sfd.debug.trigger_frame";
constexpr const char* DEBUG_PARAM_NEW_MODE = "sfd.debug.new_mode";
constexpr const char* DEBUG_PARAM_NEW_RATIO = "sfd.debug.new_ratio";
constexpr const char* DEBUG_PARAM_NEW_SPEED = "sfd.debug.new_speed";
constexpr int32_t BASE_DECIMAL = 10;

int32_t GetSysParamInt(const char* key, int32_t defValue)
{
    std::string valStr = OHOS::system::GetParameter(key, std::to_string(defValue));
    char* end = nullptr;
    int64_t val = std::strtoll(valStr.c_str(), &end, BASE_DECIMAL);
    if (end == valStr.c_str() || *end != '\0') {
        AVCODEC_LOGW("Invalid debug param format for %{public}s, fallback to default", key);
        return defValue;
    }
    if (val < std::numeric_limits<int32_t>::min() || val > std::numeric_limits<int32_t>::max()) {
        AVCODEC_LOGW("Debug param %{public}s out of int32_t range, fallback to default", key);
        return defValue;
    }
    return static_cast<int32_t>(val);
}

double GetSysParamDouble(const char* key, double defValue)
{
    std::string valStr = OHOS::system::GetParameter(key, std::to_string(defValue));
    char* end = nullptr;
    double val = std::strtod(valStr.c_str(), &end);
    if (end == valStr.c_str() || *end != '\0') {
        AVCODEC_LOGW("Invalid debug param format for %{public}s, fallback to default", key);
        return defValue;
    }
    return val;
}
#endif
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
SmartFluencyDecodingManager::SmartFluencyDecodingManager(const std::string& codecMime, AVCodecType codecType)
    : codecMime_(codecMime), codecType_(codecType)
{
}

void SmartFluencyDecodingManager::SetCallbacks(SmartFluencyDecoding::AsyncDropInputCallback inputCb,
                                               SmartFluencyDecoding::AsyncDropOutputCallback outputCb)
{
    inputCb_ = std::move(inputCb);
    outputCb_ = std::move(outputCb);
    if (sfd_ != nullptr) {
        sfd_->SetCallbacks(inputCb_, outputCb_);
        AVCODEC_LOGD_WITH_TAG("Callbacks dynamically synced to SFD instance");
    }
}

void SmartFluencyDecodingManager::Configure(const Media::Format& format)
{
    CHECK_AND_RETURN_LOG_WITH_TAG(IsCodecSupported(codecMime_, codecType_),
        "Skip SFD configuration (not supported)");
    Media::Format sfdFormat = format;
#ifdef SFD_DEBUG
    ApplyDebugConfig(sfdFormat);
#endif
    if (sfd_ != nullptr) {
        builder_->SetFromFormat(sfdFormat);
        sfd_->UpdateDynamicConfig(sfdFormat);
        {
            std::lock_guard<std::mutex> csdLock(csdMutex_);
            if (isCsdCached_ && !cachedCsdData_.empty()) {
                sfd_->InjectCsdData(cachedCsdData_);
                sfd_->ReplayCsdToAnalyzer();
            }
        }
        AVCODEC_LOGI_WITH_TAG("SFD dynamically updated via Configure.");
        return;
    }
    if (builder_ == nullptr) {
        builder_ = std::make_unique<SmartFluencyDecodingBuilder>();
        builder_->SetCodecMime(codecMime_);
        AVCODEC_LOGD_WITH_TAG("Builder created for mime=%{public}s", codecMime_.c_str());
    }
    builder_->SetFromFormat(sfdFormat);
    auto sfd = builder_->Build();
    if (sfd) {
        {
            std::lock_guard<std::mutex> csdLock(csdMutex_);
            if (isCsdCached_ && !cachedCsdData_.empty()) {
                sfd->InjectCsdData(cachedCsdData_);
                sfd->ReplayCsdToAnalyzer();
            }
        }
        sfd->SetTag(this->GetTag());
        sfd->SetCallbacks(inputCb_, outputCb_);
        sfd_ = std::move(sfd);
        AVCODEC_LOGI_WITH_TAG("SFD initialized successfully");
    }
}

void SmartFluencyDecodingManager::OnInputBufferAvailable(uint32_t index, const std::shared_ptr<AVBuffer>& buffer)
{
    CHECK_AND_RETURN_LOG_WITH_TAG(buffer != nullptr, "buffer is null for index %{public}u", index);
    if (codecType_ == AVCODEC_TYPE_VIDEO_DECODER) {
        std::lock_guard<std::mutex> csdLock(csdMutex_);
        if (!isCsdCached_) {
            tempCsdBufferMap_[index] = buffer;
        }
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
#ifdef SFD_DEBUG
        if (!decision) {
            AVCODEC_LOGD_WITH_TAG("Pre-decode drop decision for index %{public}u", index);
        }
#endif
        return decision;
    }
    AVCODEC_LOGD_WITH_TAG("SFD is not active, skipping decision for index %{public}u", index);
    return true;
}

bool SmartFluencyDecodingManager::MakePostDecodeDecision(uint32_t index,
    const std::shared_ptr<AVBuffer>& buffer, bool isSurfaceMode)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(buffer != nullptr, true, "buffer is null for index %{public}u", index);
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
        bool mvCacheInvalidated = false;
        buffer->meta_->GetData(Tag::VIDEO_DECODER_IS_MV_CACHE_INVALIDATED, mvCacheInvalidated);
        if (!sfd_->MakePostDecodeDecision(index, buffer->pts_, surfaceBuffer, mvCacheInvalidated)) {
#ifdef SFD_DEBUG
            AVCODEC_LOGD_WITH_TAG("Post-decode drop decision for index %{public}u, pts %{public}" PRId64,
                index, buffer->pts_);
#endif
            return false;
        }
    }
    return true;
}

void SmartFluencyDecodingManager::ExtractCsdData(const std::shared_ptr<AVBuffer>& buffer)
{
    CHECK_AND_RETURN_LOG_WITH_TAG(buffer != nullptr && buffer->memory_ != nullptr,
        "ExtractCsdData failed, buffer or memory is null");
    uint8_t* data = buffer->memory_->GetAddr();
    int32_t size = buffer->memory_->GetSize();
    CHECK_AND_RETURN_LOG_WITH_TAG(data != nullptr && size >= MIN_CSD_SIZE,
        "ExtractCsdData failed, data is null or size (=%{public}d) < MIN (%{public}d)", size, MIN_CSD_SIZE);
    int32_t currentSize = static_cast<int32_t>(cachedCsdData_.size());
    CHECK_AND_RETURN_LOG_WITH_TAG(currentSize < DEFAULT_CSD_COPY_SIZE,
        "CSD cache is already full (%{public}d bytes)", currentSize);
    int32_t needed = DEFAULT_CSD_COPY_SIZE - currentSize;
    int32_t copySize = std::min(size, needed);
    if (buffer->flag_ == AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        cachedCsdData_.insert(cachedCsdData_.end(), data, data + copySize);
        AVCODEC_LOGI_WITH_TAG("Appended %{public}d bytes CODEC_DATA. Total CSD size: %{public}zu",
            copySize, cachedCsdData_.size());
        return;
    }
    if (buffer->flag_ & (AVCODEC_BUFFER_FLAG_CODEC_DATA | AVCODEC_BUFFER_FLAG_SYNC_FRAME)) {
        cachedCsdData_.insert(cachedCsdData_.end(), data, data + copySize);
        AVCODEC_LOGI_WITH_TAG("Fast appended %{public}d bytes with flags %{public}u. Total CSD size: %{public}zu",
            copySize, buffer->flag_, cachedCsdData_.size());
        return;
    }
    AVCODEC_LOGD_WITH_TAG("ExtractCsdData didn't match flags, buffer->flag_=%{public}u", buffer->flag_);
}

void SmartFluencyDecodingManager::CacheCsdData(uint32_t index)
{
    std::lock_guard<std::mutex> csdLock(csdMutex_);
    if (codecType_ != AVCODEC_TYPE_VIDEO_DECODER || isCsdCached_) {
        return;
    }
    std::shared_ptr<AVBuffer> csdAttemptBuffer = nullptr;
    auto it = tempCsdBufferMap_.find(index);
    if (it != tempCsdBufferMap_.end()) {
        csdAttemptBuffer = it->second;
        tempCsdBufferMap_.erase(it);
    }
    if (csdAttemptBuffer) {
        ExtractCsdData(csdAttemptBuffer);
        bool isEnoughSize = (cachedCsdData_.size() >= DEFAULT_CSD_COPY_SIZE);
        bool isSyncFrame = ((csdAttemptBuffer->flag_ & AVCODEC_BUFFER_FLAG_SYNC_FRAME) != 0);
        if (!cachedCsdData_.empty() && (isEnoughSize || isSyncFrame)) {
            isCsdCached_ = true;
            tempCsdBufferMap_.clear();
        }
    }
}

void SmartFluencyDecodingManager::OnOutputBufferUnbinded()
{
    std::lock_guard<std::mutex> lock(surfaceBufMutex_);
    surfaceBufMap_.clear();
}

void SmartFluencyDecodingManager::Flush()
{
    if (sfd_) {
        sfd_->Flush();
    }
}

bool SmartFluencyDecodingManager::ShouldReportDfx() const
{
    if (sfd_ == nullptr) {
        return false;
    }
    return sfd_->ShouldReportDfx();
}

void SmartFluencyDecodingManager::EnableMVOutput(Media::Format& format)
{
    if (sfd_ != nullptr && sfd_->RequiresMvAnalysis()) {
        format.PutIntValue(Media::Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, 1);
    }
}

bool SmartFluencyDecodingManager::IsCodecSupported(const std::string& codecMime, AVCodecType codecType, bool isSecure)
{
    if (codecType != MediaAVCodec::AVCODEC_TYPE_VIDEO_DECODER || isSecure) {
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
    {
        std::lock_guard<std::mutex> csdLock(csdMutex_);
        isCsdCached_ = false;
        cachedCsdData_.clear();
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
    AVCODEC_LOGI_WITH_TAG("resources cleaned");
}

void SmartFluencyDecodingManager::OnDecInfoUpdate(double sourceFps, double speed)
{
    if (sfd_) {
        sfd_->OnDecInfoUpdate(sourceFps, speed);
    }
}

#ifdef SFD_DEBUG
void SmartFluencyDecodingManager::ApplyDebugConfig(Media::Format& format)
{
    if (!isDebugConfigInitialized_) {
        configMode_ = GetSysParamInt(DEBUG_PARAM_MODE, -1);
        configRatio_ = GetSysParamDouble(DEBUG_PARAM_RATIO, -1.0);
        configDecSpeed_ = GetSysParamDouble(DEBUG_PARAM_SPEED, -1.0);
        triggerFrameCnt_ = GetSysParamInt(DEBUG_PARAM_TRIGGER_FRAME, -1);
        newMode_ = GetSysParamInt(DEBUG_PARAM_NEW_MODE, -1);
        newRatio_ = GetSysParamDouble(DEBUG_PARAM_NEW_RATIO, -1.0);
        newDecSpeed_ = GetSysParamDouble(DEBUG_PARAM_NEW_SPEED, -1.0);
        inputFrameCount_ = 0;
        isDebugConfigInitialized_ = true;
        AVCODEC_LOGI_WITH_TAG("SFD Debug: Conf: %{public}d, %{public}.3f, %{public}.3f; Trg:%{public}d -> "
            "%{public}d, %{public}.3f, %{public}.3f", configMode_, configRatio_, configDecSpeed_,
            triggerFrameCnt_, newMode_, newRatio_, newDecSpeed_);
    }
    if (configMode_ >= 0) {
        format.PutIntValue(Media::Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, configMode_);
    }
    if (DoubleUtils::IsGreaterOrEqual(configRatio_, 0.0)) {
        format.PutDoubleValue(Media::Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, configRatio_);
    }
    if (DoubleUtils::IsGreaterOrEqual(configDecSpeed_, 0.0)) {
        format.PutDoubleValue(Media::Tag::VIDEO_DECODER_SPEED, configDecSpeed_);
    }
}

void SmartFluencyDecodingManager::CheckAndTriggerDebugConfig()
{
    inputFrameCount_++;
    if (triggerFrameCnt_ > 0 && inputFrameCount_ == static_cast<uint32_t>(triggerFrameCnt_)) {
        if (newMode_ >= 0) {
            configMode_ = newMode_;
        }
        if (DoubleUtils::IsGreaterOrEqual(newRatio_, 0.0)) {
            configRatio_ = newRatio_;
        };
        if (DoubleUtils::IsGreaterOrEqual(newDecSpeed_, 0.0)) {
            configDecSpeed_ = newDecSpeed_;
        }
        AVCODEC_LOGI_WITH_TAG("SFD Debug: Trg at %{public}u. M:%{public}d, R:%{public}.3f, S:%{public}.3f",
            inputFrameCount_, newMode_, newRatio_, newDecSpeed_);
        Media::Format emptyFormat;
        Configure(emptyFormat);
    }
}
#endif
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
