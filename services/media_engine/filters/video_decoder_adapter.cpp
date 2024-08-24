/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#define MEDIA_PIPELINE

#include <malloc.h>
#include <map>
#include <unistd.h>
#include <vector>
#include "avcodec_video_decoder.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "common/log.h"
#include "media_description.h"
#include "surface_type.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "meta/meta_key.h"
#include "meta/meta.h"
#include "video_decoder_adapter.h"
#include "avcodec_sysevent.h"
#include "media_core.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "VideoDecoderAdapter" };
}

namespace OHOS {
namespace Media {
using namespace MediaAVCodec;
const std::string VIDEO_INPUT_BUFFER_QUEUE_NAME = "VideoDecoderInputBufferQueue";
//Threshold for frame lag detection, set to 100 milliseconds.
const int64_t LAG_LIMIT_TIME = 100;

VideoDecoderCallback::VideoDecoderCallback(std::shared_ptr<VideoDecoderAdapter> videoDecoder)
{
    MEDIA_LOG_D_SHORT("VideoDecoderCallback instances create.");
    videoDecoderAdapter_ = videoDecoder;
}

VideoDecoderCallback::~VideoDecoderCallback()
{
    MEDIA_LOG_D_SHORT("~VideoDecoderCallback()");
}

void VideoDecoderCallback::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
    if (auto videoDecoderAdapter = videoDecoderAdapter_.lock()) {
        videoDecoderAdapter->OnError(errorType, errorCode);
    } else {
        MEDIA_LOG_I_SHORT("invalid videoDecoderAdapter");
    }
}

void VideoDecoderCallback::OnOutputFormatChanged(const MediaAVCodec::Format &format)
{
    if (auto videoDecoderAdapter = videoDecoderAdapter_.lock()) {
        videoDecoderAdapter->OnOutputFormatChanged(format);
    } else {
        MEDIA_LOG_I_SHORT("invalid videoDecoderAdapter");
    }
}

void VideoDecoderCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (auto videoDecoderAdapter = videoDecoderAdapter_.lock()) {
        videoDecoderAdapter->OnInputBufferAvailable(index, buffer);
    } else {
        MEDIA_LOG_I_SHORT("invalid videoDecoderAdapter");
    }
}

void VideoDecoderCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (auto videoDecoderAdapter = videoDecoderAdapter_.lock()) {
        videoDecoderAdapter->OnOutputBufferAvailable(index, buffer);
    } else {
        MEDIA_LOG_I_SHORT("invalid videoDecoderAdapter");
    }
}

VideoDecoderAdapter::VideoDecoderAdapter()
{
    MEDIA_LOG_D_SHORT("VideoDecoderAdapter instances create.");
}

VideoDecoderAdapter::~VideoDecoderAdapter()
{
    MEDIA_LOG_I_SHORT("~VideoDecoderAdapter()");
    FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec_ is nullptr");
    mediaCodec_->Release();
}

Status VideoDecoderAdapter::Init(MediaAVCodec::AVCodecType type, bool isMimeType, const std::string &name)
{
    MEDIA_LOG_I_SHORT("mediaCodec_->Init.");

    Format format;
    int32_t ret;
    std::shared_ptr<Media::Meta> callerInfo = std::make_shared<Media::Meta>();
    callerInfo->SetData(Media::Tag::AV_CODEC_FORWARD_CALLER_PID, appPid_);
    callerInfo->SetData(Media::Tag::AV_CODEC_FORWARD_CALLER_UID, appUid_);
    callerInfo->SetData(Media::Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, bundleName_);
    format.SetMeta(callerInfo);
    mediaCodecName_ = "";
    if (isMimeType) {
        ret = MediaAVCodec::VideoDecoderFactory::CreateByMime(name, format, mediaCodec_);
        MEDIA_LOG_I_SHORT("VideoDecoderAdapter::Init CreateByMime errorCode %{public}d", ret);
    } else {
        ret = MediaAVCodec::VideoDecoderFactory::CreateByName(name, format, mediaCodec_);
        MEDIA_LOG_I_SHORT("VideoDecoderAdapter::Init CreateByName errorCode %{public}d", ret);
    }

    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    mediaCodecName_ = name;
    currentTime_ = -1;
    return Status::OK;
}

Status VideoDecoderAdapter::Configure(const Format &format)
{
    MEDIA_LOG_I_SHORT("VideoDecoderAdapter->Configure.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    int32_t ret = mediaCodec_->Configure(format);
    isConfigured_ = ret == AVCodecServiceErrCode::AVCS_ERR_OK;
    return isConfigured_ ? Status::OK : Status::ERROR_INVALID_DATA;
}

int32_t VideoDecoderAdapter::SetParameter(const Format &format)
{
    MEDIA_LOG_D_SHORT("SetParameter enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->SetParameter(format);
}

Status VideoDecoderAdapter::Start()
{
    MEDIA_LOG_I_SHORT("Start enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG(isConfigured_, Status::ERROR_INVALID_STATE, "mediaCodec_ is not configured");
    int32_t ret = mediaCodec_->Start();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        std::string instanceId = std::to_string(instanceId_);
        struct VideoCodecFaultInfo videoCodecFaultInfo;
        videoCodecFaultInfo.appName = bundleName_;
        videoCodecFaultInfo.instanceId = instanceId;
        videoCodecFaultInfo.callerType = "player_framework";
        videoCodecFaultInfo.videoCodec = mediaCodecName_;
        videoCodecFaultInfo.errMsg = "mediaCodec_ start failed";
        FaultVideoCodecEventWrite(videoCodecFaultInfo);
    }
    currentTime_ = GetCurrentMillisecond();
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status VideoDecoderAdapter::Stop()
{
    MEDIA_LOG_I_SHORT("Stop enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG(isConfigured_, Status::ERROR_INVALID_STATE, "mediaCodec_ is not configured");
    mediaCodec_->Stop();
    currentTime_ = -1;
    return Status::OK;
}

Status VideoDecoderAdapter::Flush()
{
    MEDIA_LOG_I_SHORT("Flush enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG(isConfigured_, Status::ERROR_INVALID_STATE, "mediaCodec_ is not configured");
    int32_t ret = mediaCodec_->Flush();
    std::unique_lock<std::mutex> lock(mutex_);
    if (inputBufferQueueConsumer_ != nullptr) {
        for (auto &buffer : bufferVector_) {
            inputBufferQueueConsumer_->DetachBuffer(buffer);
        }
        bufferVector_.clear();
        inputBufferQueueConsumer_->SetQueueSize(0);
    }
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status VideoDecoderAdapter::Reset()
{
    MEDIA_LOG_I_SHORT("Reset enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    mediaCodec_->Reset();
    isConfigured_ = false;
    std::unique_lock<std::mutex> lock(mutex_);
    if (inputBufferQueueConsumer_ != nullptr) {
        for (auto &buffer : bufferVector_) {
            inputBufferQueueConsumer_->DetachBuffer(buffer);
        }
        bufferVector_.clear();
        inputBufferQueueConsumer_->SetQueueSize(0);
    }
    return Status::OK;
}

Status VideoDecoderAdapter::Release()
{
    MEDIA_LOG_I_SHORT("Release enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    int32_t ret = mediaCodec_->Release();
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

void VideoDecoderAdapter::ResetRenderTime()
{
    currentTime_ = -1;
}

int32_t VideoDecoderAdapter::SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback)
{
    MEDIA_LOG_D_SHORT("SetCallback enter.");
    callback_ = callback;
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> mediaCodecCallback
        = std::make_shared<VideoDecoderCallback>(shared_from_this());
    return mediaCodec_->SetCallback(mediaCodecCallback);
}

void VideoDecoderAdapter::PrepareInputBufferQueue()
{
    if (inputBufferQueue_ != nullptr && inputBufferQueue_-> GetQueueSize() > 0) {
        MEDIA_LOG_W_SHORT("InputBufferQueue already create");
        return;
    }
    inputBufferQueue_ = AVBufferQueue::Create(0,
        MemoryType::UNKNOWN_MEMORY, VIDEO_INPUT_BUFFER_QUEUE_NAME, true);
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
}

sptr<AVBufferQueueProducer> VideoDecoderAdapter::GetBufferQueueProducer()
{
    return inputBufferQueueProducer_;
}

sptr<AVBufferQueueConsumer> VideoDecoderAdapter::GetBufferQueueConsumer()
{
    return inputBufferQueueConsumer_;
}

void VideoDecoderAdapter::AquireAvailableInputBuffer()
{
    AVCodecTrace trace("VideoDecoderAdapter::AquireAvailableInputBuffer");
    if (inputBufferQueueConsumer_ == nullptr) {
        MEDIA_LOG_E_SHORT("inputBufferQueueConsumer_ is null");
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    std::shared_ptr<AVBuffer> tmpBuffer;
    if (inputBufferQueueConsumer_->AcquireBuffer(tmpBuffer) == Status::OK) {
        FALSE_RETURN_MSG(tmpBuffer->meta_ != nullptr, "tmpBuffer is nullptr.");
        uint32_t index;
        FALSE_RETURN_MSG(tmpBuffer->meta_->GetData(Tag::REGULAR_TRACK_ID, index), "get index failed.");
        if (tmpBuffer->flag_ & (uint32_t)(Plugins::AVBufferFlag::EOS)) {
            tmpBuffer->memory_->SetSize(0);
        }
        FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec_ is nullptr.");
        int32_t ret = mediaCodec_->QueueInputBuffer(index);
        if (ret != ERR_OK) {
            MEDIA_LOG_E_SHORT("QueueInputBuffer failed, index: %{public}u,  bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u, errCode: %{public}d", index, tmpBuffer->GetUniqueId(),
                tmpBuffer->pts_, tmpBuffer->flag_, ret);
            if (ret == AVCS_ERR_DECRYPT_FAILED) {
                eventReceiver_->OnEvent({"video_decoder_adapter", EventType::EVENT_ERROR,
                    MSERR_DRM_VERIFICATION_FAILED});
            }
        } else {
            MEDIA_LOG_D_SHORT("QueueInputBuffer success, index: %{public}u,  bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, tmpBuffer->GetUniqueId(),
                tmpBuffer->pts_, tmpBuffer->flag_);
        }
    } else {
        MEDIA_LOG_E_SHORT("AcquireBuffer failed.");
    }
}

void VideoDecoderAdapter::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    FALSE_RETURN_MSG(buffer != nullptr && buffer->meta_ != nullptr, "meta_ is nullptr.");
    buffer->meta_->SetData(Tag::REGULAR_TRACK_ID, index);
    if (inputBufferQueueConsumer_ == nullptr) {
        MEDIA_LOG_E_SHORT("inputBufferQueueConsumer_ is null");
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    if (inputBufferQueueConsumer_->IsBufferInQueue(buffer)) {
        if (inputBufferQueueConsumer_->ReleaseBuffer(buffer) != Status::OK) {
            MEDIA_LOG_E_SHORT("IsBufferInQueue ReleaseBuffer failed. index: %{public}u, bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, buffer->GetUniqueId(),
                buffer->pts_, buffer->flag_);
        } else {
            MEDIA_LOG_D_SHORT("IsBufferInQueue ReleaseBuffer success. index: %{public}u, bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, buffer->GetUniqueId(),
                buffer->pts_, buffer->flag_);
        }
    } else {
        uint32_t size = inputBufferQueueConsumer_->GetQueueSize() + 1;
        MEDIA_LOG_D_SHORT("AttachBuffer enter. index: %{public}u,  size: %{public}u , bufferid: %{public}" PRIu64,
            index, size, buffer->GetUniqueId());
        inputBufferQueueConsumer_->SetQueueSize(size);
        inputBufferQueueConsumer_->AttachBuffer(buffer, false);
        bufferVector_.push_back(buffer);
    }
}

void VideoDecoderAdapter::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
    FALSE_RETURN_MSG(callback_ != nullptr, "OnError callback_ is nullptr");
    callback_->OnError(errorType, errorCode);
}

void VideoDecoderAdapter::OnOutputFormatChanged(const MediaAVCodec::Format &format)
{
    FALSE_RETURN_MSG(callback_ != nullptr, "OnOutputFormatChanged callback_ is nullptr");
    callback_->OnOutputFormatChanged(format);
}

void VideoDecoderAdapter::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    AVCodecTrace trace("VideoDecoderAdapter::OnOutputBufferAvailable");
    if (buffer != nullptr) {
        MEDIA_LOG_D_SHORT("OnOutputBufferAvailable start. index: %{public}u, bufferid: %{public}" PRIu64
            ", pts: %{public}" PRIu64 ", flag: %{public}u", index, buffer->GetUniqueId(), buffer->pts_, buffer->flag_);
    } else {
        MEDIA_LOG_D_SHORT("OnOutputBufferAvailable start. buffer is nullptr, index: %{public}u", index);
    }
    FALSE_RETURN_MSG(buffer != nullptr, "buffer is nullptr");
    FALSE_RETURN_MSG(callback_ != nullptr, "callback_ is nullptr");
    callback_->OnOutputBufferAvailable(index, buffer);
}

int32_t VideoDecoderAdapter::GetOutputFormat(Format &format)
{
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL,
        "GetOutputFormat mediaCodec_ is nullptr");
    return mediaCodec_->GetOutputFormat(format);
}

int32_t VideoDecoderAdapter::ReleaseOutputBuffer(uint32_t index, bool render)
{
    MEDIA_LOG_I_SHORT("VideoDecoderAdapter::ReleaseOutputBuffer");
    mediaCodec_->ReleaseOutputBuffer(index, render);
    if (render && currentTime_ != -1) {
        int64_t currentTime = GetCurrentMillisecond();
        int64_t diffTime = currentTime - currentTime_;
        if (diffTime > LAG_LIMIT_TIME) {
            lagTimes_++;
            maxLagDuration_ = maxLagDuration_ > diffTime ? maxLagDuration_ : diffTime;
            totalLagDuration_ += diffTime;
        }
        currentTime_ = currentTime;
    }
    return 0;
}

int32_t VideoDecoderAdapter::RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs)
{
    AVCodecTrace trace("VideoDecoderAdapter::RenderOutputBufferAtTime");
    MEDIA_LOG_D_SHORT("VideoDecoderAdapter::RenderOutputBufferAtTime");
    mediaCodec_->RenderOutputBufferAtTime(index, renderTimestampNs);
    return 0;
}

Status VideoDecoderAdapter::GetLagInfo(int32_t& lagTimes, int32_t& maxLagDuration, int32_t& avgLagDuration)
{
    lagTimes = lagTimes_;
    maxLagDuration = static_cast<int32_t>(maxLagDuration_);
    if (lagTimes_ != 0) {
        avgLagDuration = static_cast<int32_t>(totalLagDuration_ / lagTimes_);
    } else {
        avgLagDuration = 0;
    }
    return Status::OK;
}

int64_t VideoDecoderAdapter::GetCurrentMillisecond()
{
    std::chrono::system_clock::duration duration = std::chrono::system_clock::now().time_since_epoch();
    int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return time;
}

int32_t VideoDecoderAdapter::SetOutputSurface(sptr<Surface> videoSurface)
{
    MEDIA_LOG_I_SHORT("VideoDecoderAdapter::SetOutputSurface");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->SetOutputSurface(videoSurface);
}

int32_t VideoDecoderAdapter::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
#ifdef SUPPORT_DRM
    if (mediaCodec_ == nullptr) {
        MEDIA_LOG_E_SHORT("mediaCodec_ is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    if (keySession == nullptr) {
        MEDIA_LOG_E_SHORT("keySession is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    return mediaCodec_->SetDecryptConfig(keySession, svpFlag);
#else
    return 0;
#endif
}

void VideoDecoderAdapter::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver)
{
    eventReceiver_ = receiver;
}

void VideoDecoderAdapter::SetCallingInfo(int32_t appUid, int32_t appPid, std::string bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;
}

void VideoDecoderAdapter::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D_SHORT("VideoDecoderAdapter::OnDumpInfo called.");
    std::string dumpString;
    dumpString += "VideoDecoderAdapter media codec name is:" + mediaCodecName_ + "\n";
    if (inputBufferQueue_ != nullptr) {
        dumpString += "VideoDecoderAdapter buffer size is:" + std::to_string(inputBufferQueue_->GetQueueSize()) + "\n";
    }
    if (fd < 0) {
        MEDIA_LOG_E_SHORT("VideoDecoderAdapter::OnDumpInfo fd is invalid.");
        return;
    }
    int ret = write(fd, dumpString.c_str(), dumpString.size());
    if (ret < 0) {
        MEDIA_LOG_E_SHORT("VideoDecoderAdapter::OnDumpInfo write failed.");
        return;
    }
}
} // namespace Media
} // namespace OHOS
