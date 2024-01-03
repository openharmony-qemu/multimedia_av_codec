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
    videoDecoder_ = videoDecoder;
}

AVBufferAvailableListener::~AVBufferAvailableListener()
{
}

void AVBufferAvailableListener::OnBufferAvailable()
{
    videoDecoder_->AquireAvailableInputBuffer();
}

VideoDecoderCallback::VideoDecoderCallback(std::shared_ptr<VideoDecoderAdapter> videoDecoder)
{
    MEDIA_LOG_I("VideoDecoderCallback::VideoDecoderCallback");
    videoDecoderAdapter_ = videoDecoder;
}

VideoDecoderCallback::~VideoDecoderCallback()
{
}

void VideoDecoderCallback::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
}

void VideoDecoderCallback::OnOutputFormatChanged(const MediaAVCodec::Format &format)
{
}

void VideoDecoderCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    videoDecoderAdapter_->OnInputBufferAvailable(index, buffer);
}

void VideoDecoderCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    videoDecoderAdapter_->OnOutputBufferAvailable(index, buffer);
}

VideoDecoderAdapter::VideoDecoderAdapter()
{
    MEDIA_LOG_I("VideoDecoderAdapter instances create.");
}

VideoDecoderAdapter::~VideoDecoderAdapter()
{
    mediaCodec_->Release();
    if (!isThreadExit_) {
        Stop();
    }
}

int32_t VideoDecoderAdapter::Init(MediaAVCodec::AVCodecType type, bool isMimeType, const std::string &name)
{
    MEDIA_LOG_I("mediaCodec_->Init.");
    mediaCodec_ = MediaAVCodec::VideoDecoderFactory::CreateByMime(name);
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
    MEDIA_LOG_I("VideoDecoderAdapter->SetParameter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->SetParameter(format);
}

int32_t VideoDecoderAdapter::Start()
{
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG_E(isThreadExit_, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL,
        "Process has been started already, neet to stop it first.");
    isThreadExit_ = false;
    readThread_ = std::make_unique<std::thread>(&VideoDecoderAdapter::RenderLoop, this);
    return mediaCodec_->Start();
}

int32_t VideoDecoderAdapter::Stop()
{
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    mediaCodec_->Stop();
    FALSE_RETURN_V_MSG_E(!isThreadExit_, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL,
        "Process has been stopped already, need to start if first.");
    isThreadExit_ = true;
    if (readThread_ != nullptr && readThread_->joinable()) {
        readThread_->join();
        readThread_ = nullptr;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t VideoDecoderAdapter::Flush()
{
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->Flush();
}

int32_t VideoDecoderAdapter::Reset()
{
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    mediaCodec_->Reset();
    if (!isThreadExit_) {
        Stop();
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t VideoDecoderAdapter::Release()
{
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
    std::shared_ptr<AVBuffer> tmpBuffer;
    if (inputBufferQueueConsumer_->AcquireBuffer(tmpBuffer) == Status::OK) {
        FALSE_RETURN_MSG(tmpBuffer->meta_ != nullptr, "tmpBuffer is nullptr.");
        uint32_t index;
        FALSE_RETURN_MSG(tmpBuffer->meta_->GetData(Tag::REGULAR_TRACK_ID, index), "get index failed.");
        if (tmpBuffer->flag_ & (uint32_t)(Plugins::AVBufferFlag::EOS)) {
            inputBufferQueueConsumer_->ReleaseBuffer(tmpBuffer);
            return;
        }
        if (mediaCodec_->QueueInputBuffer(index) != ERR_OK) {
            MEDIA_LOG_E("QueueInputBuffer failed index: %{public}u,  bufferid: %{public}" PRIu64,
                index, tmpBuffer->GetUniqueId());
        } else {
            MEDIA_LOG_D("QueueInputBuffer success index: %{public}u,  bufferid: %{public}" PRIu64,
                index, tmpBuffer->GetUniqueId());
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
            MEDIA_LOG_E("IsBufferInQueue ReleaseBuffer failed. index: %{public}u, bufferid: %{public}" PRIu64,
                index, buffer->GetUniqueId());
        }
    } else {
        uint32_t size = inputBufferQueueConsumer_->GetQueueSize() + 1;
        MEDIA_LOG_I("AttachBuffer enter. index: %{public}u,  size: %{public}u , bufferid: %{public}" PRIu64,
            index, size, buffer->GetUniqueId());
        inputBufferQueueConsumer_->SetQueueSize(size);
        inputBufferQueueConsumer_->AttachBuffer(buffer, false);
    }
}

void VideoDecoderAdapter::RenderLoop()
{
    while (true) {
        if (isThreadExit_) {
            MEDIA_LOG_I("Exit RenderLoop read thread.");
            break;
        }
        uint32_t index;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condBufferAvailable_.wait(lock, [this] { return !indexs_.empty(); });
            index = indexs_[0];
            indexs_.erase(indexs_.begin());
        }
        mediaCodec_->ReleaseOutputBuffer(index, true);
    }
}

void VideoDecoderAdapter::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    FALSE_RETURN_MSG(callback_ != nullptr, "callback_ is nullptr");
    callback_->OnOutputBufferAvailable(index, buffer);
}

int32_t VideoDecoderAdapter::ReleaseOutputBuffer(uint32_t index, bool render)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        indexs_.push_back(index);
    }
    condBufferAvailable_.notify_one();
    return 0;
}

int32_t VideoDecoderAdapter::SetOutputSurface(sptr<Surface> videoSurface)
{
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->SetOutputSurface(videoSurface);
}

#ifdef SUPPORT_DRM
int32_t VideoDecoderAdapter::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG(keySession != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    return mediaCodec_->SetDecryptConfig(keySession, svpFlag);
}
#endif
} // namespace Media
} // namespace OHOS