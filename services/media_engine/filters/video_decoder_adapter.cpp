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
AVBufferAvailableListener::AVBufferAvailableListener(std::shared_ptr<VideoDecoderAdapter> videoDecoder)
{
    MEDIA_LOG_I("AVBufferAvailableListener instances create.");
    videoDecoder_ = videoDecoder;
}

AVBufferAvailableListener::~AVBufferAvailableListener()
{
    MEDIA_LOG_I("~AVBufferAvailableListener()");
}

void AVBufferAvailableListener::OnBufferAvailable()
{
    if (auto videoDecoder = videoDecoder_.lock()) {
        videoDecoder->AquireAvailableInputBuffer();
    } else {
        MEDIA_LOG_I("invalid videoDecoder");
    }
}

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
    if (!isThreadExit_) {
        Stop();
    }
    FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec_ is nullptr");
    mediaCodec_->Release();
}

int32_t VideoDecoderAdapter::Init(MediaAVCodec::AVCodecType type, bool isMimeType, const std::string &name)
{
    MEDIA_LOG_I("mediaCodec_->Init.");
    if (isMimeType) {
        mediaCodec_ = MediaAVCodec::VideoDecoderFactory::CreateByMime(name);
    } else {
        mediaCodec_ = MediaAVCodec::VideoDecoderFactory::CreateByName(name);
    }

    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t VideoDecoderAdapter::Configure(const Format &format)
{
    MEDIA_LOG_I("VideoDecoderAdapter->Configure.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->Configure(format);
}

int32_t VideoDecoderAdapter::SetParameter(const Format &format)
{
    MEDIA_LOG_I("SetParameter enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->SetParameter(format);
}

int32_t VideoDecoderAdapter::Start()
{
    MEDIA_LOG_I("Start enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG_E(isThreadExit_, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL,
        "Process has been started already, neet to stop it first.");
    isThreadExit_ = false;
    isPaused_ = false;
    readThread_ = std::make_unique<std::thread>(&VideoDecoderAdapter::RenderLoop, this);
    pthread_setname_np(readThread_->native_handle(), "RenderLoop");
    return mediaCodec_->Start();
}

int32_t VideoDecoderAdapter::Pause()
{
    MEDIA_LOG_I("Pause enter.");
    isPaused_ = true;
    condBufferAvailable_.notify_all();
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t VideoDecoderAdapter::Stop()
{
    MEDIA_LOG_I("Stop enter.");
    if (mediaCodec_ != nullptr) {
        mediaCodec_->Stop();
    } else {
        MEDIA_LOG_W("mediaCodec_ is nullptr");
    }
    FALSE_RETURN_V_MSG_E(!isThreadExit_, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL,
        "Process has been stopped already, need to start if first.");
    isThreadExit_ = true;
    condBufferAvailable_.notify_all();
    if (readThread_ != nullptr && readThread_->joinable()) {
        readThread_->join();
        readThread_ = nullptr;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t VideoDecoderAdapter::Resume()
{
    MEDIA_LOG_I("Resume enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    isPaused_ = false;
    condBufferAvailable_.notify_all();
    return mediaCodec_->Start();
}

int32_t VideoDecoderAdapter::Flush()
{
    MEDIA_LOG_I("Flush enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    mediaCodec_->Flush();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        indexs_.clear();
    }

    for (auto &buffer : bufferVector_) {
        inputBufferQueueConsumer_->DetachBuffer(buffer);
    }
    bufferVector_.clear();
    inputBufferQueueConsumer_->SetQueueSize(0);
 
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t VideoDecoderAdapter::Reset()
{
    MEDIA_LOG_I("Reset enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    mediaCodec_->Reset();
    if (!isThreadExit_) {
        Stop();
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t VideoDecoderAdapter::Release()
{
    MEDIA_LOG_I("Release enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->Release();
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

sptr<AVBufferQueueProducer> VideoDecoderAdapter::GetInputBufferQueue()
{
    if (inputBufferQueue_ != nullptr && inputBufferQueue_-> GetQueueSize() > 0) {
        MEDIA_LOG_W("InputBufferQueue already create");
        return inputBufferQueueProducer_;
    }
    inputBufferQueue_ = AVBufferQueue::Create(0,
        MemoryType::SHARED_MEMORY, VIDEO_INPUT_BUFFER_QUEUE_NAME, true);
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
    MEDIA_LOG_I("InputBufferQueue setlistener");
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    return inputBufferQueueProducer_;
}

void VideoDecoderAdapter::AquireAvailableInputBuffer()
{
    AVCodecTrace trace("VideoDecoderAdapter::AquireAvailableInputBuffer");
    std::shared_ptr<AVBuffer> tmpBuffer;
    if (inputBufferQueueConsumer_->AcquireBuffer(tmpBuffer) == Status::OK) {
        FALSE_RETURN_MSG(tmpBuffer->meta_ != nullptr, "tmpBuffer is nullptr.");
        uint32_t index;
        FALSE_RETURN_MSG(tmpBuffer->meta_->GetData(Tag::REGULAR_TRACK_ID, index), "get index failed.");
        if (tmpBuffer->flag_ & (uint32_t)(Plugins::AVBufferFlag::EOS)) {
            MEDIA_LOG_I("ReleaseBuffer for eos, index: %{public}u,  bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, tmpBuffer->GetUniqueId(),
                tmpBuffer->pts_, tmpBuffer->flag_);
            Event event {
                .srcFilter = "VideoSink",
                .type = EventType::EVENT_COMPLETE,
            };
            FALSE_RETURN(eventReceiver_  != nullptr);
            eventReceiver_ ->OnEvent(event);
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

void VideoDecoderAdapter::RenderLoop()
{
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condBufferAvailable_.wait(lock, [this] { return (!indexs_.empty() && !isPaused_) || isThreadExit_; });
            if (isThreadExit_) {
                MEDIA_LOG_I("Exit RenderLoop read thread.");
                break;
            }
            task = std::move(indexs_.front());
            indexs_.pop_front();
        }
        task();
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
    FALSE_RETURN_MSG(buffer != nullptr, "buffer is nullptr");
    FALSE_RETURN_MSG(callback_ != nullptr, "callback_ is nullptr");
    MEDIA_LOG_D("OnOutputBufferAvailable start. index: %{public}u, bufferid: %{public}" PRIu64 ", pts: %{public}" PRIu64
        ", flag: %{public}u", index, buffer->GetUniqueId(), buffer->pts_, buffer->flag_);
    callback_->OnOutputBufferAvailable(index, buffer);
}

int32_t VideoDecoderAdapter::GetOutputFormat(Format &format)
{
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL,
        "GetOutputFormat mediaCodec_ is nullptr");
    return mediaCodec_->GetOutputFormat(format);
}

int32_t VideoDecoderAdapter::ReleaseOutputBuffer(uint32_t index, std::shared_ptr<Pipeline::VideoSink> videoSink,
    std::shared_ptr<AVBuffer> &outputBuffer, bool doSync)
{
    AVCodecTrace trace("VideoDecoderAdapter::ReleaseOutputBuffer");
    auto task = [this, index, videoSink, outputBuffer, doSync]() {
        if (doSync) {
            bool render = videoSink->DoSyncWrite(outputBuffer);
            mediaCodec_->ReleaseOutputBuffer(index, render);
            MEDIA_LOG_D("Video release output buffer pts: %{public}" PRIu64 ", render: %{public}i",
                (outputBuffer == nullptr ? -1 : outputBuffer->pts_), render);
        } else {
            mediaCodec_->ReleaseOutputBuffer(index, false);
        }
    };

    {
        std::lock_guard<std::mutex> lock(mutex_);
        indexs_.push_back(std::move(task));
    }
    condBufferAvailable_.notify_one();
    return 0;
}

int32_t VideoDecoderAdapter::SetOutputSurface(sptr<Surface> videoSurface)
{
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
} // namespace Media
} // namespace OHOS