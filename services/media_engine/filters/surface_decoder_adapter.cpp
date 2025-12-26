/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "surface_decoder_adapter.h"
#include <ctime>
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "meta/format.h"
#include "media_description.h"
#include "avcodec_trace.h"
#include "codec_capability_adapter.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER,
                                                "SurfaceDecoderAdapter" };
constexpr uint32_t BUFFER_IS_EOS = 1;
}

namespace OHOS {
namespace Media {

constexpr int64_t VARIABLE_INCREMENT_INTERVAL = 1;

class SurfaceDecoderAdapterCallback : public MediaAVCodec::MediaCodecCallback {
public:
    explicit SurfaceDecoderAdapterCallback(std::shared_ptr<SurfaceDecoderAdapter> surfaceDecoderAdapter)
        : surfaceDecoderAdapter_(std::move(surfaceDecoderAdapter))
    {
    }
    
    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) override
    {
        if (auto surfaceDecoderAdapter = surfaceDecoderAdapter_.lock()) {
            if (!transCoderErrorCbOnce_) {
                transCoderErrorCbOnce_ = true;
                surfaceDecoderAdapter->decoderAdapterCallback_->OnError(errorType, errorCode);
            }
        } else {
            MEDIA_LOG_I("invalid surfaceEncoderAdapter");
        }
    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format) override
    {
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        if (auto surfaceDecoderAdapter = surfaceDecoderAdapter_.lock()) {
            surfaceDecoderAdapter->OnInputBufferAvailable(index, buffer);
        } else {
            MEDIA_LOG_I("invalid surfaceDecoderAdapter");
        }
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        if (auto surfaceDecoderAdapter = surfaceDecoderAdapter_.lock()) {
            MEDIA_LOG_D("OnOutputBuffer flag " PUBLIC_LOG_D32, buffer->flag_);
            surfaceDecoderAdapter->OnOutputBufferAvailable(index, buffer);
        } else {
            MEDIA_LOG_I("invalid surfaceDecoderAdapter");
        }
    }

private:
    std::weak_ptr<SurfaceDecoderAdapter> surfaceDecoderAdapter_;
    bool transCoderErrorCbOnce_ = false;
};

class AVBufferAvailableListener : public OHOS::Media::IConsumerListener {
public:
    explicit AVBufferAvailableListener(std::shared_ptr<SurfaceDecoderAdapter> surfaceDecoderAdapter)
        : surfaceDecoderAdapter_(std::move(surfaceDecoderAdapter))
    {
    }

    void OnBufferAvailable() override
    {
        if (auto surfaceDecoderAdapter = surfaceDecoderAdapter_.lock()) {
            surfaceDecoderAdapter->AcquireAvailableInputBuffer();
        } else {
            MEDIA_LOG_I("invalid surfaceDecoderAdapter");
        }
    }
private:
    std::weak_ptr<SurfaceDecoderAdapter> surfaceDecoderAdapter_;
};

SurfaceDecoderAdapter::SurfaceDecoderAdapter()
{
    MEDIA_LOG_I("encoder adapter create");
}

SurfaceDecoderAdapter::~SurfaceDecoderAdapter()
{
    MEDIA_LOG_I("encoder adapter destroy");
    if (codecServer_) {
        codecServer_->Release();
    }
    codecServer_ = nullptr;
}

Status SurfaceDecoderAdapter::Init(const std::string &mime)
{
    MEDIA_LOG_I("Init mime: " PUBLIC_LOG_S, mime.c_str());
    codecServer_ = MediaAVCodec::VideoDecoderFactory::CreateByMime(mime);
    if (!codecServer_) {
        MEDIA_LOG_I("Create codecServer failed");
        return Status::ERROR_UNKNOWN;
    }
    if (!releaseBufferTask_) {
        releaseBufferTask_ = std::make_shared<Task>("SurfaceDecoder");
        FALSE_RETURN_V(releaseBufferTask_ != nullptr, Status::ERROR_NULL_POINTER);
        releaseBufferTask_->RegisterJob([this] {
            ReleaseBuffer();
            return 0;
        });
    }
    return Status::OK;
}

Status SurfaceDecoderAdapter::Configure(const Format &format)
{
    MEDIA_LOG_I("Configure");
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = codecServer_->Configure(format);
    return ret == 0 ? Status::OK : Status::ERROR_UNKNOWN;
}

int64_t SurfaceDecoderAdapter::GetFrameNum()
{
    return frameNum_.load();
}

int64_t SurfaceDecoderAdapter::GetLastBufferPts()
{
    return lastBufferPts_.load();
}

sptr<OHOS::Media::AVBufferQueueProducer> SurfaceDecoderAdapter::GetInputBufferQueue()
{
    MEDIA_LOG_I("GetInputBufferQueue");
    if (inputBufferQueue_ != nullptr && inputBufferQueue_-> GetQueueSize() > 0) {
        MEDIA_LOG_I("InputBufferQueue already create");
        return inputBufferQueueProducer_;
    }
    inputBufferQueue_ = AVBufferQueue::Create(0,
        MemoryType::UNKNOWN_MEMORY, "inputBufferQueue", true);
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    return inputBufferQueueProducer_;
}

Status SurfaceDecoderAdapter::SetDecoderAdapterCallback(
    const std::shared_ptr<DecoderAdapterCallback> &decoderAdapterCallback)
{
    MEDIA_LOG_I("SetDecoderAdapterCallback");
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> surfaceDecoderAdapterCallback =
        std::make_shared<SurfaceDecoderAdapterCallback>(shared_from_this());
    decoderAdapterCallback_ = decoderAdapterCallback;
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = codecServer_->SetCallback(surfaceDecoderAdapterCallback);
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceDecoderAdapter::SetOutputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("SetOutputSurface");
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = codecServer_->SetOutputSurface(surface);
    if (ret == 0) {
        MEDIA_LOG_I("SetOutputSurface success");
        return Status::OK;
    } else {
        MEDIA_LOG_I("SetOutputSurface fail");
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceDecoderAdapter::Start()
{
    MEDIA_LOG_I("Start");
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret;
    isThreadExit_ = false;
    if (releaseBufferTask_) {
        releaseBufferTask_->Start();
    }
    ret = codecServer_->Prepare();
    if (ret == 0) {
        MEDIA_LOG_I("Prepare success");
    } else {
        MEDIA_LOG_I("Prepare fail");
        return Status::ERROR_UNKNOWN;
    }
    ret = codecServer_->Start();
    if (ret == 0) {
        MEDIA_LOG_I("Start success");
        return Status::OK;
    } else {
        MEDIA_LOG_I("Start fail");
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceDecoderAdapter::Stop()
{
    MEDIA_LOG_I("Stop");
    if (releaseBufferTask_) {
        {
            std::unique_lock<std::mutex> lock(releaseBufferMutex_);
            isThreadExit_ = true;
        }
        releaseBufferCondition_.notify_all();
        releaseBufferTask_->Stop();
        MEDIA_LOG_I("releaseBufferTask_ Stop");
    }
    if (!codecServer_) {
        return Status::OK;
    }
    int32_t ret = codecServer_->Stop();
    MEDIA_LOG_I("codecServer_ Stop");
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceDecoderAdapter::Pause()
{
    MEDIA_LOG_I("Pause");
    return Status::OK;
}

Status SurfaceDecoderAdapter::Resume()
{
    MEDIA_LOG_I("Resume");
    return Status::OK;
}

Status SurfaceDecoderAdapter::Flush()
{
    MEDIA_LOG_I("Flush");
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = codecServer_->Flush();
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceDecoderAdapter::Release()
{
    MEDIA_LOG_I("Release");
    if (!codecServer_) {
        return Status::OK;
    }
    int32_t ret = codecServer_->Release();
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}
    
Status SurfaceDecoderAdapter::SetParameter(const Format &format)
{
    MEDIA_LOG_I("SetParameter");
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = codecServer_->SetParameter(format);
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

void SurfaceDecoderAdapter::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    FALSE_RETURN_MSG(buffer != nullptr && buffer->meta_ != nullptr, "meta_ is nullptr.");
    MEDIA_LOG_D("OnInputBufferAvailable enter. index: %{public}u, bufferid: %{public}" PRIu64", pts: %{public}" PRIu64
        ", flag: %{public}u", index, buffer->GetUniqueId(), buffer->pts_, buffer->flag_);
    buffer->meta_->SetData(Tag::REGULAR_TRACK_ID, static_cast<int32_t>(index));
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
    }
}

void SurfaceDecoderAdapter::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    MediaAVCodec::AVCodecTrace trace("OnOutputBufferAvailable " + std::to_string(index) + " " +
        std::to_string(buffer->pts_));
    {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        if ((buffer->flag_ & BUFFER_IS_EOS) == 1) {
            MEDIA_LOG_I("Buffer index: %{public}u" PRIu32 " flag: %{public}u" PRIu32, index, buffer->flag_);
            indexs_.push_back(index);
            eosBufferIndex_ = index;
        } else if (buffer->pts_ > lastBufferPts_.load()) {
            lastBufferPts_ = buffer->pts_;
            frameNum_.fetch_add(VARIABLE_INCREMENT_INTERVAL, std::memory_order_relaxed);
            indexs_.push_back(index);
        } else {
            MEDIA_LOG_W("Buffer drop index: " PUBLIC_LOG_U32 " pts: " PUBLIC_LOG_D64, index, buffer->pts_);
            dropIndexs_.push_back(index);
        }
    }
    releaseBufferCondition_.notify_all();
    MEDIA_LOG_D("OnOutputBufferAvailable end, index: " PUBLIC_LOG_U32 " pts: " PUBLIC_LOG_D64, index, buffer->pts_);
}

void SurfaceDecoderAdapter::AcquireAvailableInputBuffer()
{
    std::shared_ptr<AVBuffer> filledInputBuffer;
    Status ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
    if (ret != Status::OK) {
        MEDIA_LOG_E("AcquireBuffer fail");
        return;
    }
    FALSE_RETURN_MSG(filledInputBuffer->meta_ != nullptr, "filledInputBuffer meta is nullptr.");
    int32_t metaIndex;
    FALSE_RETURN_MSG(filledInputBuffer->meta_->GetData(Tag::REGULAR_TRACK_ID, metaIndex), "get index failed.");
    uint32_t index = static_cast<uint32_t>(metaIndex);
    FALSE_RETURN_MSG(codecServer_ != nullptr, "codecServer_ is nullptr.");
    if (codecServer_->QueueInputBuffer(index) != ERR_OK) {
        MEDIA_LOG_E("QueueInputBuffer failed, index: %{public}u,  bufferid: %{public}" PRIu64
            ", pts: %{public}" PRIu64", flag: %{public}u", index, filledInputBuffer->GetUniqueId(),
            filledInputBuffer->pts_, filledInputBuffer->flag_);
    } else {
        MEDIA_LOG_D("QueueInputBuffer success, index: %{public}u,  bufferid: %{public}" PRIu64
            ", pts: %{public}" PRIu64", flag: %{public}u", index, filledInputBuffer->GetUniqueId(),
            filledInputBuffer->pts_, filledInputBuffer->flag_);
    }
}

void SurfaceDecoderAdapter::ReleaseBuffer()
{
    MEDIA_LOG_I("ReleaseBuffer");
    while (!isThreadExit_) {
        std::vector<uint32_t> indexs;
        std::vector<uint32_t> dropIndexs;
        {
            std::unique_lock<std::mutex> lock(releaseBufferMutex_);
            releaseBufferCondition_.wait(lock, [this] {
                return isThreadExit_ || !indexs_.empty() || !dropIndexs_.empty();
            });
            indexs = indexs_;
            indexs_.clear();
            dropIndexs = dropIndexs_;
            dropIndexs_.clear();
        }
        for (auto &index : indexs) {
            MEDIA_LOG_D("Release buffer, index: " PUBLIC_LOG_U32, index);
            codecServer_->ReleaseOutputBuffer(index, true);
            if (index == eosBufferIndex_) {
                int64_t lastBufferPts = GetLastBufferPts();
                int64_t frameNum = GetFrameNum();
                MEDIA_LOG_I("lastBuffer PTS: " PUBLIC_LOG_D64 " frameNum: " PUBLIC_LOG_D64,
                    lastBufferPts, frameNum);
                decoderAdapterCallback_->OnBufferEos(lastBufferPts, frameNum);
            }
        }
        for (auto &dropIndex : dropIndexs) {
            MediaAVCodec::AVCodecTrace trace("ReleaseBuffer drop " + std::to_string(dropIndex));
            MEDIA_LOG_D("Drop buffer, index: " PUBLIC_LOG_U32, dropIndex);
            codecServer_->ReleaseOutputBuffer(dropIndex, false);
        }
    }
    MEDIA_LOG_I("ReleaseBuffer end");
}
} // namespace MEDIA
} // namespace OHOS
