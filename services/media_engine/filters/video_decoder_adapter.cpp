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

namespace OHOS {
namespace Media {
using namespace MediaAVCodec;
const std::string VIDEO_INPUT_BUFFER_QUEUE_NAME = "VideoDecoderInputBufferQueue";

VideoDecoderCallback::VideoDecoderCallback(std::shared_ptr<VideoDecoderAdapter> videoDecoder)
{
    MEDIA_LOG_I("VideoDecoderCallback instances create.");
    videoDecoderAdapter_ = videoDecoder;
}

VideoDecoderCallback::~VideoDecoderCallback()
{
    MEDIA_LOG_I("~VideoDecoderCallback()");
}

void VideoDecoderCallback::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
    if (auto videoDecoderAdapter = videoDecoderAdapter_.lock()) {
        videoDecoderAdapter->OnError(errorType, errorCode);
    } else {
        MEDIA_LOG_I("invalid videoDecoderAdapter");
    }
}

void VideoDecoderCallback::OnOutputFormatChanged(const MediaAVCodec::Format &format)
{
    if (auto videoDecoderAdapter = videoDecoderAdapter_.lock()) {
        videoDecoderAdapter->OnOutputFormatChanged(format);
    } else {
        MEDIA_LOG_I("invalid videoDecoderAdapter");
    }
}

void VideoDecoderCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (auto videoDecoderAdapter = videoDecoderAdapter_.lock()) {
        videoDecoderAdapter->OnInputBufferAvailable(index, buffer);
    } else {
        MEDIA_LOG_I("invalid videoDecoderAdapter");
    }
}

void VideoDecoderCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (auto videoDecoderAdapter = videoDecoderAdapter_.lock()) {
        videoDecoderAdapter->OnOutputBufferAvailable(index, buffer);
    } else {
        MEDIA_LOG_I("invalid videoDecoderAdapter");
    }
}

VideoDecoderAdapter::VideoDecoderAdapter()
{
    MEDIA_LOG_I("VideoDecoderAdapter instances create.");
}

VideoDecoderAdapter::~VideoDecoderAdapter()
{
    MEDIA_LOG_I("~VideoDecoderAdapter()");
    FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec_ is nullptr");
    mediaCodec_->Release();
}

Status VideoDecoderAdapter::Init(MediaAVCodec::AVCodecType type, bool isMimeType, const std::string &name)
{
    MEDIA_LOG_I("mediaCodec_->Init.");

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
        MEDIA_LOG_I("VideoDecoderAdapter::Init CreateByMime errorCode %{public}d", ret);
    } else {
        ret = MediaAVCodec::VideoDecoderFactory::CreateByName(name, format, mediaCodec_);
        MEDIA_LOG_I("VideoDecoderAdapter::Init CreateByName errorCode %{public}d", ret);
    }

    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    mediaCodecName_ = name;
    return Status::OK;
}

Status VideoDecoderAdapter::Configure(const Format &format)
{
    MEDIA_LOG_I("VideoDecoderAdapter->Configure.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    int32_t ret = mediaCodec_->Configure(format);
    isConfigured_ = ret == AVCodecServiceErrCode::AVCS_ERR_OK;
    return isConfigured_ ? Status::OK : Status::ERROR_INVALID_DATA;
}

int32_t VideoDecoderAdapter::SetParameter(const Format &format)
{
    MEDIA_LOG_I("SetParameter enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->SetParameter(format);
}

Status VideoDecoderAdapter::Start()
{
    MEDIA_LOG_I("Start enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG(isConfigured_, Status::ERROR_INVALID_STATE, "mediaCodec_ is not configured");
    int32_t ret = mediaCodec_->Start();
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status VideoDecoderAdapter::Stop()
{
    MEDIA_LOG_I("Stop enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG(isConfigured_, Status::ERROR_INVALID_STATE, "mediaCodec_ is not configured");
    mediaCodec_->Stop();
    return Status::OK;
}

Status VideoDecoderAdapter::Flush()
{
    MEDIA_LOG_I("Flush enter.");
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
    MEDIA_LOG_I("Reset enter.");
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
    MEDIA_LOG_I("Release enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    int32_t ret = mediaCodec_->Release();
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE; 
}

int32_t VideoDecoderAdapter::SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback)
{
    MEDIA_LOG_I("SetCallback enter.");
    callback_ = callback;
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> mediaCodecCallback
        = std::make_shared<VideoDecoderCallback>(shared_from_this());
    return mediaCodec_->SetCallback(mediaCodecCallback);
}

void VideoDecoderAdapter::PrepareInputBufferQueue()
{
    if (inputBufferQueue_ != nullptr && inputBufferQueue_-> GetQueueSize() > 0) {
        MEDIA_LOG_W("InputBufferQueue already create");
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
        MEDIA_LOG_E("inputBufferQueueConsumer_ is null");
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
        if (mediaCodec_->QueueInputBuffer(index) != ERR_OK) {
            MEDIA_LOG_E("QueueInputBuffer failed, index: %{public}u,  bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, tmpBuffer->GetUniqueId(),
                tmpBuffer->pts_, tmpBuffer->flag_);
        } else {
            MEDIA_LOG_D("QueueInputBuffer success, index: %{public}u,  bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, tmpBuffer->GetUniqueId(),
                tmpBuffer->pts_, tmpBuffer->flag_);
        }
    } else {
        MEDIA_LOG_E("AcquireBuffer failed.");
    }
}

void VideoDecoderAdapter::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    FALSE_RETURN_MSG(buffer != nullptr && buffer->meta_ != nullptr, "meta_ is nullptr.");
    buffer->meta_->SetData(Tag::REGULAR_TRACK_ID, index);
    if (inputBufferQueueConsumer_ == nullptr) {
        MEDIA_LOG_E("inputBufferQueueConsumer_ is null");
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    if (inputBufferQueueConsumer_->IsBufferInQueue(buffer)) {
        if (inputBufferQueueConsumer_->ReleaseBuffer(buffer) != Status::OK) {
            MEDIA_LOG_E("IsBufferInQueue ReleaseBuffer failed. index: %{public}u, bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, buffer->GetUniqueId(),
                buffer->pts_, buffer->flag_);
        } else {
            MEDIA_LOG_D("IsBufferInQueue ReleaseBuffer success. index: %{public}u, bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, buffer->GetUniqueId(),
                buffer->pts_, buffer->flag_);
        }
    } else {
        uint32_t size = inputBufferQueueConsumer_->GetQueueSize() + 1;
        MEDIA_LOG_I("AttachBuffer enter. index: %{public}u,  size: %{public}u , bufferid: %{public}" PRIu64,
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
        MEDIA_LOG_D("OnOutputBufferAvailable start. index: %{public}u, bufferid: %{public}" PRIu64
            ", pts: %{public}" PRIu64 ", flag: %{public}u", index, buffer->GetUniqueId(), buffer->pts_, buffer->flag_);
    } else {
        MEDIA_LOG_D("OnOutputBufferAvailable start. buffer is nullptr, index: %{public}u", index);
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
    mediaCodec_->ReleaseOutputBuffer(index, render);
    return 0;
}

int32_t VideoDecoderAdapter::SetOutputSurface(sptr<Surface> videoSurface)
{
    MEDIA_LOG_I("VideoDecoderAdapter::SetOutputSurface");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->SetOutputSurface(videoSurface);
}

int32_t VideoDecoderAdapter::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
#ifdef SUPPORT_DRM
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG(keySession != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->SetDecryptConfig(keySession, svpFlag);
#else
    return 0;
#endif
}

void VideoDecoderAdapter::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver)
{
    eventReceiver_ = receiver;
}

void VideoDecoderAdapter::SetCallingInfo(int32_t appUid, int32_t appPid, std::string bundleName)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
}

void VideoDecoderAdapter::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D("VideoDecoderAdapter::OnDumpInfo called.");
    std::string dumpString;
    dumpString += "VideoDecoderAdapter media codec name is:" + mediaCodecName_ + "\n";
    dumpString += "VideoDecoderAdapter buffer size is:" + std::to_string(inputBufferQueue_->GetQueueSize()) + "\n";
    if (fd < 0) {
        MEDIA_LOG_E("VideoDecoderAdapter::OnDumpInfo fd is invalid.");
        return;
    }
    int ret = write(fd, dumpString.c_str(), dumpString.size());
    if (ret < 0) {
        MEDIA_LOG_E("VideoDecoderAdapter::OnDumpInfo write failed.");
        return;
    }
}
} // namespace Media
} // namespace OHOS
