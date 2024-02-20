/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "surface_encoder_adapter.h"
#include <ctime>
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "codec_server.h"
#include "meta/format.h"
#include "media_description.h"

constexpr uint32_t TIME_OUT_MS = 1000;

namespace OHOS {
namespace Media {

class SurfaceEncoderAdapterCallback : public MediaAVCodec::MediaCodecCallback {
public:
    explicit SurfaceEncoderAdapterCallback(std::shared_ptr<SurfaceEncoderAdapter> surfaceEncoderAdapter)
        : surfaceEncoderAdapter_(std::move(surfaceEncoderAdapter))
    {
    }

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) override
    {
        if (auto surfaceEncoderAdapter = surfaceEncoderAdapter_.lock()) {
            surfaceEncoderAdapter->encoderAdapterCallback_->OnError(errorType, errorCode);
        } else {
            MEDIA_LOG_I("invalid surfaceEncoderAdapter");
        }
    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format) override
    {
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        if (auto surfaceEncoderAdapter = surfaceEncoderAdapter_.lock()) {
            surfaceEncoderAdapter->OnOutputBufferAvailable(index, buffer);
        } else {
            MEDIA_LOG_I("invalid surfaceEncoderAdapter");
        }
    }

private:
    std::weak_ptr<SurfaceEncoderAdapter> surfaceEncoderAdapter_;
};

SurfaceEncoderAdapter::SurfaceEncoderAdapter()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "encoder adapter create", logTag_.c_str());
}

SurfaceEncoderAdapter::~SurfaceEncoderAdapter()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "encoder adapter destroy", logTag_.c_str());
    if (codecServer_) {
        codecServer_->Release();
    }
    codecServer_ = nullptr;
}

Status SurfaceEncoderAdapter::Init(const std::string &mime, bool isEncoder)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Init mime: " PUBLIC_LOG_S, logTag_.c_str(), mime.c_str());
    if (!codecServer_) {
        codecServer_ = MediaAVCodec::VideoEncoderFactory::CreateByMime(mime);
        if (!codecServer_) {
            MEDIA_LOG_I(PUBLIC_LOG_S "Create codecServer failed", logTag_.c_str());
            return Status::ERROR_UNKNOWN;
        }
    }
    if (!releaseBufferTask_) {
        releaseBufferTask_ = std::make_shared<Task>("SurfaceEncoder");
        releaseBufferTask_->RegisterJob([this] { ReleaseBuffer(); });
    }
    return Status::OK;
}

void SurfaceEncoderAdapter::SetLogTag(std::string logTag)
{
    logTag_ = std::move(logTag);
}

Status SurfaceEncoderAdapter::Configure(const std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Configure", logTag_.c_str());
    MediaAVCodec::Format format = MediaAVCodec::Format();
    if (meta->Find(Tag::VIDEO_WIDTH) != meta->end()) {
        int32_t videoWidth;
        meta->Get<Tag::VIDEO_WIDTH>(videoWidth);
        format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_WIDTH, videoWidth);
    }
    if (meta->Find(Tag::VIDEO_HEIGHT) != meta->end()) {
        int32_t videoHeight;
        meta->Get<Tag::VIDEO_HEIGHT>(videoHeight);
        format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_HEIGHT, videoHeight);
    }
    if (meta->Find(Tag::VIDEO_CAPTURE_RATE) != meta->end()) {
        double videoCaptureRate;
        meta->Get<Tag::VIDEO_CAPTURE_RATE>(videoCaptureRate);
        format.PutDoubleValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CAPTURE_RATE, videoCaptureRate);
    }
    if (meta->Find(Tag::MEDIA_BITRATE) != meta->end()) {
        int64_t mediaBitrate;
        meta->Get<Tag::MEDIA_BITRATE>(mediaBitrate);
        format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, mediaBitrate);
    }
    if (meta->Find(Tag::VIDEO_FRAME_RATE) != meta->end()) {
        double videoFrameRate;
        meta->Get<Tag::VIDEO_FRAME_RATE>(videoFrameRate);
        format.PutDoubleValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_FRAME_RATE, videoFrameRate);
    }
    if (meta->Find(Tag::MIME_TYPE) != meta->end()) {
        std::string mimeType;
        meta->Get<Tag::MIME_TYPE>(mimeType);
        format.PutStringValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME, mimeType);
    }
    if (meta->Find(Tag::VIDEO_H265_PROFILE) != meta->end()) {
        Plugins::HEVCProfile h265Profile;
        meta->Get<Tag::VIDEO_H265_PROFILE>(h265Profile);
        format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PROFILE, h265Profile);
    }
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = codecServer_->Configure(format);
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "SetOutputBufferQueue", logTag_.c_str());
    outputBufferQueueProducer_ = bufferQueueProducer;
    return Status::OK;
}

Status SurfaceEncoderAdapter::SetEncoderAdapterCallback(
    const std::shared_ptr<EncoderAdapterCallback> &encoderAdapterCallback)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "SetEncoderAdapterCallback", logTag_.c_str());
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> surfaceEncoderAdapterCallback =
        std::make_shared<SurfaceEncoderAdapterCallback>(shared_from_this());
    encoderAdapterCallback_ = encoderAdapterCallback;
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = codecServer_->SetCallback(surfaceEncoderAdapterCallback);
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::SetInputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "GetInputSurface", logTag_.c_str());
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    MediaAVCodec::CodecServer *codecServerPtr = (MediaAVCodec::CodecServer *)(codecServer_.get());
    int32_t ret = codecServerPtr->SetInputSurface(surface);
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

sptr<Surface> SurfaceEncoderAdapter::GetInputSurface()
{
    return codecServer_->CreateInputSurface();
}

Status SurfaceEncoderAdapter::Start()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Start", logTag_.c_str());
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret;
    isThreadExit_ = false;
    if (releaseBufferTask_) {
        releaseBufferTask_->Start();
    }
    ret = codecServer_->Start();
    isStart_ = true;
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Stop()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Stop", logTag_.c_str());
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    const int64_t SEC_TO_NS = 1000000000;
    stopTime_ = static_cast<uint64_t>(timestamp.tv_sec) * SEC_TO_NS + static_cast<uint64_t>(timestamp.tv_nsec);
    MEDIA_LOG_I(PUBLIC_LOG_S "Stop time: " PUBLIC_LOG_D64, logTag_.c_str(), stopTime_);

    if (isStart_) {
        std::unique_lock<std::mutex> lock(stopMutex_);
        stopCondition_.wait_for(lock, std::chrono::milliseconds(TIME_OUT_MS));
    }
    if (releaseBufferTask_) {
        isThreadExit_ = true;
        releaseBufferCondition_.notify_all();
        releaseBufferTask_->Stop();
        MEDIA_LOG_I(PUBLIC_LOG_S "releaseBufferTask_ Stop", logTag_.c_str());
    }
    if (!codecServer_) {
        return Status::OK;
    }
    int32_t ret = codecServer_->Stop();
    MEDIA_LOG_I(PUBLIC_LOG_S "codecServer_ Stop", logTag_.c_str());
    isStart_ = false;
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Pause()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Pause", logTag_.c_str());
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    const int64_t SEC_TO_NS = 1000000000;
    pauseTime_ = static_cast<uint64_t>(timestamp.tv_sec) * SEC_TO_NS + static_cast<uint64_t>(timestamp.tv_nsec);
    return Status::OK;
}

Status SurfaceEncoderAdapter::Resume()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Resume", logTag_.c_str());
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    const int64_t SEC_TO_NS = 1000000000;
    int64_t resumeTime = static_cast<uint64_t>(timestamp.tv_sec) * SEC_TO_NS + static_cast<uint64_t>(timestamp.tv_nsec);
    totalPauseTime_ = totalPauseTime_ + resumeTime - pauseTime_;
    return Status::OK;
}

Status SurfaceEncoderAdapter::Flush()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Flush", logTag_.c_str());
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

Status SurfaceEncoderAdapter::Reset()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Reset", logTag_.c_str());
    if (!codecServer_) {
        return Status::OK;
    }
    int32_t ret = codecServer_->Reset();
    startBufferTime_ = -1;
    stopTime_ = -1;
    pauseTime_ = -1;
    totalPauseTime_ = 0;
    isStart_ = false;
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Release()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Release", logTag_.c_str());
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

Status SurfaceEncoderAdapter::NotifyEos()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "NotifyEos", logTag_.c_str());
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = codecServer_->NotifyEos();
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}
    
Status SurfaceEncoderAdapter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "SetParameter", logTag_.c_str());
    if (!codecServer_) {
        return Status::ERROR_UNKNOWN;
    }
    MediaAVCodec::Format format = MediaAVCodec::Format();
    int32_t ret = codecServer_->SetParameter(format);
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

std::shared_ptr<Meta> SurfaceEncoderAdapter::GetOutputFormat()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "GetOutputFormat is not supported", logTag_.c_str());
    return nullptr;
}

void SurfaceEncoderAdapter::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnOutputBufferAvailable buffer->pts" PUBLIC_LOG_D64, logTag_.c_str(), buffer->pts_);
    if (stopTime_ != -1 && buffer->pts_ > stopTime_) {
        MEDIA_LOG_I("buffer->pts > stopTime, ready to stop");
        std::unique_lock<std::mutex> lock(stopMutex_);
        stopCondition_.notify_all();
    }
    if (startBufferTime_ == -1 && buffer->pts_ != 0) {
        startBufferTime_ = buffer->pts_;
    }

    int32_t size = buffer->memory_->GetSize();
    std::shared_ptr<AVBuffer> emptyOutputBuffer;
    AVBufferConfig avBufferConfig;
    avBufferConfig.size = size;
    avBufferConfig.memoryType = MemoryType::SHARED_MEMORY;
    avBufferConfig.memoryFlag = MemoryFlag::MEMORY_READ_WRITE;
    Status status = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
    if (status != Status::OK) {
        MEDIA_LOG_I(PUBLIC_LOG_S "RequestBuffer fail.", logTag_.c_str());
        return;
    }
    std::shared_ptr<AVMemory> &bufferMem = emptyOutputBuffer->memory_;
    if (emptyOutputBuffer->memory_ == nullptr) {
        MEDIA_LOG_I(PUBLIC_LOG_S "emptyOutputBuffer->memory_ is nullptr", logTag_.c_str());
        return;
    }
    bufferMem->Write(buffer->memory_->GetAddr(), size, 0);
    *(emptyOutputBuffer->meta_) = *(buffer->meta_);
    emptyOutputBuffer->pts_ = buffer->pts_ - startBufferTime_ - totalPauseTime_;
    emptyOutputBuffer->flag_ = buffer->flag_;
    outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, true);
    {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        indexs_.push_back(index);
    }
    releaseBufferCondition_.notify_all();
    MEDIA_LOG_I(PUBLIC_LOG_S "OnOutputBufferAvailable end", logTag_.c_str());
}

void SurfaceEncoderAdapter::ReleaseBuffer()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "ReleaseBuffer", logTag_.c_str());
    while (true) {
        if (isThreadExit_) {
            MEDIA_LOG_I(PUBLIC_LOG_S "Exit ReleaseBuffer thread.", logTag_.c_str());
            break;
        }
        std::vector<uint32_t> indexs;
        {
            std::unique_lock<std::mutex> lock(releaseBufferMutex_);
            releaseBufferCondition_.wait(lock);
            indexs = indexs_;
            indexs_.clear();
        }
        for (auto &index : indexs) {
            codecServer_->ReleaseOutputBuffer(index);
        }
    }
    MEDIA_LOG_I(PUBLIC_LOG_S "ReleaseBuffer end", logTag_.c_str());
}
} // namespace MEDIA
} // namespace OHOS