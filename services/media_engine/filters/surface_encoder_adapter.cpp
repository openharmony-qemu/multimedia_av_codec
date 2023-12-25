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

constexpr uint32_t TIME_OUT_MS = 100;
constexpr int32_t US_TO_MS = 1000;

namespace OHOS {
namespace Media {

class SurfaceEncoderAdapterCallback : public MediaAVCodec::MediaCodecCallback {
public:
    explicit SurfaceEncoderAdapterCallback(std::shared_ptr<SurfaceEncoderAdapter> surfaceEncoderAdapter)
        : surfaceEncoderAdapter_(std::move(surfaceEncoderAdapter))
    {
    }
    
    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
    {
        surfaceEncoderAdapter_->encoderAdapterCallback_->OnError(errorType, errorCode);
    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format)
    {
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
    {
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
    {
        surfaceEncoderAdapter_->OnOutputBufferAvailable(index, buffer);
    }

private:
    std::shared_ptr<SurfaceEncoderAdapter> surfaceEncoderAdapter_;
};

SurfaceEncoderAdapter::SurfaceEncoderAdapter()
{
}

SurfaceEncoderAdapter::~SurfaceEncoderAdapter() {
}

Status SurfaceEncoderAdapter::Init(const std::string &mime, bool isEncoder)
{
    MEDIA_LOG_I("Init mime: " PUBLIC_LOG_S, mime.c_str());
    if (!codecServer_) {
        codecServer_ = MediaAVCodec::CodecServer::Create();
    }
    if (!releaseBufferTask_) {
        releaseBufferTask_ = std::make_shared<Task>("SurfaceEncoder");
        releaseBufferTask_->RegisterJob([this] { ReleaseBuffer(); });
    }
    int32_t ret = codecServer_->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER, true, mime);
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Configure(const std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("Configure");
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
    int32_t ret = codecServer_->Configure(format);
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer)
{
    MEDIA_LOG_I("SetOutputBufferQueue");
    outputBufferQueueProducer_ = bufferQueueProducer;
    return Status::OK;
}

Status SurfaceEncoderAdapter::SetEncoderAdapterCallback(
    const std::shared_ptr<EncoderAdapterCallback> &encoderAdapterCallback)
{
    MEDIA_LOG_I("SetEncoderAdapterCallback");
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> surfaceEncoderAdapterCallback =
        std::make_shared<SurfaceEncoderAdapterCallback>(shared_from_this());
    encoderAdapterCallback_ = encoderAdapterCallback;
    int32_t ret = codecServer_->SetCallback(surfaceEncoderAdapterCallback);
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::SetInputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("GetInputSurface");
    MediaAVCodec::CodecServer *codecServerPtr = (MediaAVCodec::CodecServer *)(codecServer_.get());
    int32_t ret = codecServerPtr->SetInputSurface(surface);
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Start()
{
    MEDIA_LOG_I("Start");
    int32_t ret;
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
    MEDIA_LOG_I("Stop");
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    const int64_t SEC_TO_NS = 1000000000;
    stopTime_ = (uint64_t)timestamp.tv_sec * SEC_TO_NS + (uint64_t)timestamp.tv_nsec;
    MEDIA_LOG_I("Stop time: " PUBLIC_LOG_D64, stopTime_);

    std::unique_lock<std::mutex> lock(stopMutex_);
    stopCondition_.wait(lock);
    if (releaseBufferTask_) {
        releaseBufferTask_->Stop();
        MEDIA_LOG_I("releaseBufferTask_ Stop");
    }
    int32_t ret = codecServer_->Stop();
    MEDIA_LOG_I("codecServer_ Stop");
    isStart_ = false;
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Pause()
{
    MEDIA_LOG_I("Pause");
    return Status::OK;
}

Status SurfaceEncoderAdapter::Resume()
{
    MEDIA_LOG_I("Resume");
    return Status::OK;
}

Status SurfaceEncoderAdapter::Flush()
{
    MEDIA_LOG_I("Flush");
    int32_t ret = codecServer_->Flush();
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Reset()
{
    MEDIA_LOG_I("Reset");
    int32_t ret = codecServer_->Reset();
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Release()
{
    MEDIA_LOG_I("Release");
    int32_t ret = codecServer_->Release();
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::NotifyEos()
{
    MEDIA_LOG_I("NotifyEos");
    int32_t ret = codecServer_->NotifyEos();
    if (ret == 0) {
        return Status::OK;
    } else {
        return Status::ERROR_UNKNOWN;
    }
}
    
Status SurfaceEncoderAdapter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
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
    MEDIA_LOG_I("GetOutputFormat is not supported");
    return nullptr;
}

void SurfaceEncoderAdapter::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    MEDIA_LOG_I("OnOutputBufferAvailable buffer->pts" PUBLIC_LOG_D64, buffer->pts_);
    if (!isStart_) {
        std::unique_lock<std::mutex> lock(releaseBufferMutex_);
        releaseBufferIndex_ = index;
        releaseBufferCondition_.notify_all();
        return;
    }
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
    avBufferConfig.memoryFlag = MemoryFlag::MEMORY_READ_WRITE;
    Status status = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
    if (status != Status::OK) {
        MEDIA_LOG_I("RequestBuffer fail.");
        return;
    }
    std::shared_ptr<AVMemory> &bufferMem = emptyOutputBuffer->memory_;
    if (emptyOutputBuffer->memory_ == nullptr) {
        MEDIA_LOG_I("emptyOutputBuffer->memory_ is nullptr");
        return;
    }
    bufferMem->Write(buffer->memory_->GetAddr(), size, 0);
    *(emptyOutputBuffer->meta_) = *(buffer->meta_);
    emptyOutputBuffer->pts_ = (buffer->pts_ - startBufferTime_) / US_TO_MS;
    outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, true);
    std::unique_lock<std::mutex> lock(releaseBufferMutex_);
    releaseBufferIndex_ = index;
    releaseBufferCondition_.notify_all();
    MEDIA_LOG_I("OnOutputBufferAvailable end");
}

void SurfaceEncoderAdapter::ReleaseBuffer()
{
    MEDIA_LOG_I("ReleaseBuffer");
    std::unique_lock<std::mutex> lock(releaseBufferMutex_);
    releaseBufferCondition_.wait(lock);
    codecServer_->ReleaseOutputBuffer(releaseBufferIndex_, false);
}
} // namespace MEDIA
} // namespace OHOS