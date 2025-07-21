/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "avcodec_trace.h"
#include "avcodec_sysevent.h"
#include "common/log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_RECORDER, "SurfaceEncoderAdapter" };
}

constexpr uint32_t TIME_OUT_MS = 1000;
constexpr uint32_t NS_PER_US = 1000;
constexpr int64_t SEC_TO_NS = 1000000000;
constexpr uint32_t STOP_TIME_OUT_MS = 2000;
//Codec wait timeout with no video frame received
constexpr uint32_t AVCODEC_ERR_TIMEOUT_NO_FRAME_RECEIVED = 50001 ;
namespace OHOS {
namespace Media {

using namespace OHOS::MediaAVCodec;
class SurfaceEncoderAdapterCallback : public MediaAVCodec::MediaCodecCallback {
public:
    explicit SurfaceEncoderAdapterCallback(std::shared_ptr<SurfaceEncoderAdapter> surfaceEncoderAdapter)
        : surfaceEncoderAdapter_(std::move(surfaceEncoderAdapter))
    {
    }

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) override
    {
        if (auto surfaceEncoderAdapter = surfaceEncoderAdapter_.lock()) {
            if (surfaceEncoderAdapter->GetIsTransCoderMode() && transCoderErrorCbOnce_) {
                return;
            }
            if (surfaceEncoderAdapter->GetIsTransCoderMode()) {
                transCoderErrorCbOnce_ = true;
            }
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
    bool transCoderErrorCbOnce_ = false;
};

class DroppedFramesCallback : public MediaAVCodec::MediaCodecParameterWithAttrCallback {
public:
    explicit DroppedFramesCallback(std::shared_ptr<SurfaceEncoderAdapter> surfaceEncoderAdapter)
        : surfaceEncoderAdapter_(std::move(surfaceEncoderAdapter))
    {
    }

    void OnInputParameterWithAttrAvailable(uint32_t index, std::shared_ptr<Format> attribute,
        std::shared_ptr<Format> parameter) override
    {
        if (auto surfaceEncoderAdapter = surfaceEncoderAdapter_.lock()) {
            surfaceEncoderAdapter->OnInputParameterWithAttrAvailable(index, attribute, parameter);
        } else {
            MEDIA_LOG_I("invalid surfaceEncoderAdapter");
        }
    }

private:
    std::weak_ptr<SurfaceEncoderAdapter> surfaceEncoderAdapter_;
};

SurfaceEncoderAdapter::SurfaceEncoderAdapter()
{
    MEDIA_LOG_I("encoder adapter create");
}

SurfaceEncoderAdapter::~SurfaceEncoderAdapter()
{
    MEDIA_LOG_I("encoder adapter destroy");
    if (codecServer_) {
        codecServer_->Release();
    }
    codecServer_ = nullptr;
}

Status SurfaceEncoderAdapter::Init(const std::string &mime, bool isEncoder)
{
    MEDIA_LOG_I("Init mime: " PUBLIC_LOG_S, mime.c_str());
    codecMimeType_ = mime;
    Format format;
    std::shared_ptr<Media::Meta> callerInfo = std::make_shared<Media::Meta>();
    callerInfo->SetData(Media::Tag::AV_CODEC_FORWARD_CALLER_PID, appPid_);
    callerInfo->SetData(Media::Tag::AV_CODEC_FORWARD_CALLER_UID, appUid_);
    callerInfo->SetData(Media::Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, bundleName_);
    format.SetMeta(callerInfo);
    int32_t ret = MediaAVCodec::VideoEncoderFactory::CreateByMime(mime, format, codecServer_);
    MEDIA_LOG_I("AVCodecVideoEncoderImpl::Init CreateByMime errorCode %{public}d", ret);
    if (!codecServer_) {
        MEDIA_LOG_I("Create codecServer failed");
        SetFaultEvent("SurfaceEncoderAdapter::Init Create codecServer failed", ret);
        return Status::ERROR_UNKNOWN;
    }
    if (!releaseBufferTask_) {
        releaseBufferTask_ = std::make_shared<Task>("SurfaceEncoder",  "", TaskType::SINGLETON);
        releaseBufferTask_->RegisterJob([this] {
            ReleaseBuffer();
            return 0;
        });
    }
    return Status::OK;
}

void SurfaceEncoderAdapter::ConfigureGeneralFormat(MediaAVCodec::Format &format, const std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("ConfigureGeneralFormat");
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
        format.PutLongValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, mediaBitrate);
    }
    if (meta->Find(Tag::VIDEO_FRAME_RATE) != meta->end()) {
        double videoFrameRate;
        meta->Get<Tag::VIDEO_FRAME_RATE>(videoFrameRate);
        FALSE_LOG_MSG(videoFrameRate > 0, "videoFrameRate get invalid");
        videoFrameRate_ = static_cast<int32_t>(videoFrameRate);
        MEDIA_LOG_I("videoFrameRate_: %{public}d", videoFrameRate_);
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
}

void SurfaceEncoderAdapter::ConfigureEnableFormat(MediaAVCodec::Format &format, const std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("ConfigureEnableFormat");
    if (meta->Find(Tag::VIDEO_ENCODER_ENABLE_WATERMARK) != meta->end()) {
        bool enableWatermark = false;
        meta->Get<Tag::VIDEO_ENCODER_ENABLE_WATERMARK>(enableWatermark);
        format.PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, enableWatermark);
    }
}

Status SurfaceEncoderAdapter::Configure(const std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("Configure");
    MediaAVCodec::AVCodecTrace trace("SurfaceEncoderAdapter::Configure");
    MediaAVCodec::Format format = MediaAVCodec::Format();
    ConfigureGeneralFormat(format, meta);
    ConfigureAboutRGBA(format, meta);
    ConfigureAboutEnableTemporalScale(format, meta);
    ConfigureEnableFormat(format, meta);
    if (!codecServer_) {
        SetFaultEvent("SurfaceEncoderAdapter::Configure, CodecServer is null");
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = static_cast<int32_t>(Status::OK);
    if (!isTransCoderMode) {
        std::shared_ptr<MediaAVCodec::MediaCodecParameterWithAttrCallback> droppedFramesCallback =
        std::make_shared<DroppedFramesCallback>(shared_from_this());
        ret = codecServer_->SetCallback(droppedFramesCallback);
        if (ret != 0) {
            MEDIA_LOG_I("Set dropped Frames Callback failed");
            SetFaultEvent("DroppedFramesCallback::DroppedFramesCallback error", ret);
            return Status::ERROR_UNKNOWN;
        }
    }
    if (isTransCoderMode) {
        format.PutIntValue(Tag::VIDEO_FRAME_RATE_ADAPTIVE_MODE, true);
        bool isSetEnable = meta->Get<Tag::AV_TRANSCODER_ENABLE_B_FRAME>(enableBFrame_);
        MEDIA_LOG_I("Transcoder enable B frame: %{public}d", static_cast<int32_t>(enableBFrame_));
        if (isSetEnable) {
            format.PutIntValue(Tag::VIDEO_ENCODER_ENABLE_B_FRAME, static_cast<int32_t>(enableBFrame_));
        }
    }
    if (!isTransCoderMode) {
        format.PutIntValue(Tag::VIDEO_ENCODER_ENABLE_B_FRAME, enableBFrame_);
    }
    ret = codecServer_->Configure(format);
    if (ret != 0) {
        SetFaultEvent("SurfaceEncoderAdapter::Configure error", ret);
    }
    return ret == 0 ? Status::OK : Status::ERROR_UNKNOWN;
}

Status SurfaceEncoderAdapter::SetWatermark(std::shared_ptr<AVBuffer> &waterMarkBuffer)
{
    MEDIA_LOG_I("SetWaterMark");
    if (!codecServer_) {
        MEDIA_LOG_I("CodecServer is null");
        SetFaultEvent("SurfaceEncoderAdapter::setWatermark, CodecServer is null");
        return Status::ERROR_UNKNOWN;
    }
    int ret = codecServer_->SetCustomBuffer(waterMarkBuffer);
    if (ret != 0) {
        MEDIA_LOG_E("SetCustomBuffer error");
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status SurfaceEncoderAdapter::SetVideoEnableBFrame(bool &enableBFrame)
{
    MEDIA_LOG_I("SurfaceEncoderAdapter::SetVideoEnableBFrame in, enableBFrame is: %{public}d", enableBFrame);
    if (!codecServer_) {
        MEDIA_LOG_I("CodecServer is null");
        SetFaultEvent("SurfaceEncoderAdapter::SetVideoEnableBFrame, CodecServer is null");
        return Status::ERROR_UNKNOWN;
    }
    enableBFrame_ = enableBFrame;
    return Status::OK;
}

Status SurfaceEncoderAdapter::SetStopTime()
{
    GetCurrentTime(stopTime_);
    MEDIA_LOG_I("SetStopTime: " PUBLIC_LOG_D64, stopTime_);
    return Status::OK;
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
    if (!codecServer_) {
        SetFaultEvent("SurfaceEncoderAdapter::SetEncoderAdapterCallback, CodecServer is null");
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = codecServer_->SetCallback(surfaceEncoderAdapterCallback);
    if (ret == 0) {
        return Status::OK;
    } else {
        SetFaultEvent("SurfaceEncoderAdapter::SetEncoderAdapterCallback error", ret);
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::SetEncoderAdapterKeyFramePtsCallback(
    const std::shared_ptr<EncoderAdapterKeyFramePtsCallback> &encoderAdapterKeyFramePtsCallback)
{
    MEDIA_LOG_I("SetEncoderAdapterKeyFramePtsCallback");
    encoderAdapterKeyFramePtsCallback_ = encoderAdapterKeyFramePtsCallback;
    return Status::OK;
}

Status SurfaceEncoderAdapter::SetInputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("GetInputSurface");
    if (!codecServer_) {
        SetFaultEvent("SurfaceEncoderAdapter::SetInputSurface, CodecServer is null");
        return Status::ERROR_UNKNOWN;
    }
    MediaAVCodec::CodecServer *codecServerPtr = (MediaAVCodec::CodecServer *)(codecServer_.get());
    int32_t ret = codecServerPtr->SetInputSurface(surface);
    if (ret == 0) {
        return Status::OK;
    } else {
        SetFaultEvent("SurfaceEncoderAdapter::SetInputSurface error", ret);
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::SetTransCoderMode()
{
    MEDIA_LOG_I("SetTransCoderMode");
    isTransCoderMode = true;
    return Status::OK;
}

sptr<Surface> SurfaceEncoderAdapter::GetInputSurface()
{
    FALSE_RETURN_V_MSG(codecServer_ != nullptr, nullptr, "codecServer_ is nullptr");
    return codecServer_->CreateInputSurface();
}

Status SurfaceEncoderAdapter::Start()
{
    MEDIA_LOG_I("Start");
    MediaAVCodec::AVCodecTrace trace("SurfaceEncoderAdapter::Start");
    if (!codecServer_) {
        SetFaultEvent("SurfaceEncoderAdapter::Start, CodecServer is null");
        return Status::ERROR_UNKNOWN;
    }
    Clear();
    int32_t ret;
    isThreadExit_ = false;
    hasReceivedEOS_ = false;
    if (releaseBufferTask_) {
        releaseBufferTask_->Start();
    }
    ret = codecServer_->Start();
    isStart_ = true;
    isStartKeyFramePts_ = true;
    if (ret == 0) {
        curState_ = ProcessStateCode::RECORDING;
        return Status::OK;
    } else {
        SetFaultEvent("SurfaceEncoderAdapter::Start error", ret);
        curState_ = ProcessStateCode::ERROR;
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Stop()
{
    MEDIA_LOG_I("Stop");
    MediaAVCodec::AVCodecTrace trace("SurfaceEncoderAdapter::Stop");
    if (stopTime_ < 0) {
        GetCurrentTime(stopTime_);
    }
    isStopKeyFramePts_ = true;
    MEDIA_LOG_I("Stop time: " PUBLIC_LOG_D64, stopTime_);
    // operate stop when it is paused state.
    if (curState_ == ProcessStateCode::PAUSED && !isTransCoderMode) {
        stopTime_ = pauseTime_;
        // current frame is not the last frame before the pasue time, wait for stop
        if (currentKeyFramePts_ <= pauseTime_ - (SEC_TO_NS / videoFrameRate_)) {
            MEDIA_LOG_D("paused state -> stop, wait for stop.");
            HandleWaitforStop();
        }
        // else stop directly
        AddStopPts();
    }
    // operate stop when it is recording state.
    if (curState_ == ProcessStateCode::RECORDING && !isTransCoderMode) {
        MEDIA_LOG_D("recording state -> stop, wait for stop.");
        HandleWaitforStop();
        AddStopPts();
    }

    if (releaseBufferTask_) {
        {
            std::lock_guard<std::mutex> lock(releaseBufferMutex_);
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
    isStart_ = false;
    if (ret == 0) {
        curState_ = ProcessStateCode::STOPPED;
        return Status::OK;
    } else {
        SetFaultEvent("SurfaceEncoderAdapter::Stop error", ret);
        curState_ = ProcessStateCode::ERROR;
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Pause()
{
    MEDIA_LOG_I("Pause");
    MediaAVCodec::AVCodecTrace trace("SurfaceEncoderAdapter::Pause");
    if (isTransCoderMode) {
        return Status::OK;
    }
    std::lock_guard<std::mutex> lock(checkFramesMutex_);
    GetCurrentTime(pauseTime_);
    FALSE_RETURN_V_MSG(pauseTime_ > 0, Status::ERROR_UNKNOWN, "GetCurrentTime pauseTime_ <= 0");
    MEDIA_LOG_I("Pause time: " PUBLIC_LOG_D64, pauseTime_);
    if (pauseResumeQueue_.empty() ||
        (pauseResumeQueue_.back().second == StateCode::RESUME && pauseResumeQueue_.back().first <= pauseTime_)) {
        pauseResumeQueue_.push_back({pauseTime_, StateCode::PAUSE});
        pauseResumeQueue_.push_back({std::numeric_limits<int64_t>::max(), StateCode::RESUME});
        pauseResumePts_.push_back({pauseTime_, StateCode::PAUSE});
        pauseResumePts_.push_back({std::numeric_limits<int64_t>::max(), StateCode::RESUME});
    }
    curState_ = ProcessStateCode::PAUSED;
    return Status::OK;
}

Status SurfaceEncoderAdapter::Resume()
{
    MEDIA_LOG_I("Resume");
    MediaAVCodec::AVCodecTrace trace("SurfaceEncoderAdapter::Resume");
    if (isTransCoderMode) {
        isResume_ = true;
        return Status::OK;
    }
    std::lock_guard<std::mutex> lock(checkFramesMutex_);
    GetCurrentTime(resumeTime_);
    FALSE_RETURN_V_MSG(resumeTime_ > 0, Status::ERROR_UNKNOWN, "GetCurrentTime resumeTime_ <= 0");
    MEDIA_LOG_I("resume time: " PUBLIC_LOG_D64, resumeTime_);
    if (pauseResumeQueue_.empty()) {
        MEDIA_LOG_I("Status Error, no pause before resume");
        return Status::ERROR_UNKNOWN;
    }
    if (pauseResumeQueue_.back().second == StateCode::RESUME) {
        pauseResumeQueue_.back().first = std::min(resumeTime_, pauseResumeQueue_.back().first);
        pauseResumePts_.back().first = std::min(resumeTime_, pauseResumePts_.back().first);
    }
    if (pauseTime_ != -1) {
        totalPauseTime_ = totalPauseTime_ + resumeTime_ - pauseTime_;
        MEDIA_LOG_I("total pause time: " PUBLIC_LOG_D64, totalPauseTime_);
        // total pause time (without checkFramesPauseTime)
        totalPauseTimeQueue_.push_back(totalPauseTime_);
    }

    curState_ = ProcessStateCode::RECORDING;
    pauseTime_ = -1;
    resumeTime_ = -1;
    return Status::OK;
}

Status SurfaceEncoderAdapter::Flush()
{
    MEDIA_LOG_I("Flush");
    if (!codecServer_) {
        SetFaultEvent("SurfaceEncoderAdapter::Flush, CodecServer is null");
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = codecServer_->Flush();
    if (ret == 0) {
        return Status::OK;
    } else {
        SetFaultEvent("SurfaceEncoderAdapter::Flush error", ret);
        curState_ = ProcessStateCode::ERROR;
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Reset()
{
    MEDIA_LOG_I("Reset");
    MediaAVCodec::AVCodecTrace trace("SurfaceEncoderAdapter::Reset");
    if (!codecServer_) {
        return Status::OK;
    }
    int32_t ret = codecServer_->Reset();
    Clear();
    if (ret == 0) {
        curState_ = ProcessStateCode::IDLE;
        return Status::OK;
    } else {
        SetFaultEvent("SurfaceEncoderAdapter::Reset error", ret);
        curState_ = ProcessStateCode::ERROR;
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::Release()
{
    MEDIA_LOG_I("Release");
    MediaAVCodec::AVCodecTrace trace("SurfaceEncoderAdapter::Release");
    if (!codecServer_) {
        return Status::OK;
    }
    int32_t ret = codecServer_->Release();
    if (ret == 0) {
        return Status::OK;
    } else {
        SetFaultEvent("SurfaceEncoderAdapter::Release error", ret);
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::NotifyEos(int64_t pts)
{
    MEDIA_LOG_I("NotifyEos");
    if (!codecServer_) {
        SetFaultEvent("SurfaceEncoderAdapter::NotifyEos, CodecServer is null");
        return Status::ERROR_UNKNOWN;
    }
    int32_t ret = 0;
    MEDIA_LOG_I("lastBuffer PTS: " PUBLIC_LOG_D64 " current PTS: " PUBLIC_LOG_D64, pts, currentPts_.load());
    eosPts_ = pts;
    ret = codecServer_->NotifyEos();
    if (ret == 0) {
        return Status::OK;
    } else {
        SetFaultEvent("SurfaceEncoderAdapter::NotifyEos error", ret);
        return Status::ERROR_UNKNOWN;
    }
}

Status SurfaceEncoderAdapter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    MediaAVCodec::AVCodecTrace trace("SurfaceEncoderAdapter::SetParameter");
    if (!codecServer_) {
        SetFaultEvent("SurfaceEncoderAdapter::SetParameter, CodecServer is null");
        return Status::ERROR_UNKNOWN;
    }
    MediaAVCodec::Format format = MediaAVCodec::Format();
    int32_t ret = codecServer_->SetParameter(format);
    if (ret == 0) {
        return Status::OK;
    } else {
        SetFaultEvent("SurfaceEncoderAdapter::SetParameter error", ret);
        return Status::ERROR_UNKNOWN;
    }
}

std::shared_ptr<Meta> SurfaceEncoderAdapter::GetOutputFormat()
{
    MEDIA_LOG_I("GetOutputFormat is not supported");
    return nullptr;
}

void SurfaceEncoderAdapter::TransCoderOnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    int32_t size = buffer->memory_->GetSize();
    std::shared_ptr<AVBuffer> emptyOutputBuffer;
    AVBufferConfig avBufferConfig;
    avBufferConfig.size = size;
    avBufferConfig.memoryType = MemoryType::SHARED_MEMORY;
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
    bufferMem->SetSize(size);
    *(emptyOutputBuffer->meta_) = *(buffer->meta_);
    emptyOutputBuffer->pts_ = buffer->pts_;
    emptyOutputBuffer->flag_ = buffer->flag_;
    outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, true);
    {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        indexs_.push_back(index);
    }
    releaseBufferCondition_.notify_all();
    MEDIA_LOG_D("OnOutputBufferAvailable end");
}

void SurfaceEncoderAdapter::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    MEDIA_LOG_D("OnOutputBufferAvailable buffer->pts" PUBLIC_LOG_D64, buffer->pts_);
    currentPts_ = currentPts_.load() < buffer->pts_? buffer->pts_ : currentPts_.load();
    MediaAVCodec::AVCodecTrace trace("SurfaceEncoderAdapter::OnOutputBufferAvailable");
    if (isTransCoderMode) {
        TransCoderOnOutputBufferAvailable(index, buffer);
        return;
    }
    FALSE_RETURN_MSG(buffer->memory_ != nullptr, "buffer->memory_ is nullptr, OnOutputBufferAvailable fail");
    int32_t size = buffer->memory_->GetSize();
    std::shared_ptr<AVBuffer> outputBuffer;
    AVBufferConfig avBufferConfig;
    avBufferConfig.size = size;
    avBufferConfig.memoryType = MemoryType::SHARED_MEMORY;
    avBufferConfig.memoryFlag = MemoryFlag::MEMORY_READ_WRITE;
    Status status = outputBufferQueueProducer_->RequestBuffer(outputBuffer, avBufferConfig, TIME_OUT_MS);
    FALSE_RETURN_MSG(status == Status::OK, "RequestBuffer fail.");
    FALSE_RETURN_MSG(outputBuffer != nullptr, "outputBuffer is nullptr.");
    std::shared_ptr<AVMemory> &bufferMem = outputBuffer->memory_;
    FALSE_RETURN_MSG(outputBuffer->memory_ != nullptr, "outputBuffer->memory_ is nullptr");
    bufferMem->Write(buffer->memory_->GetAddr(), size, 0);
    *(outputBuffer->meta_) = *(buffer->meta_);
    outputBuffer->pts_ = buffer->pts_ / NS_PER_US;
    outputBuffer->flag_ = buffer->flag_;
    outputBufferQueueProducer_->PushBuffer(outputBuffer, true);
    {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        indexs_.push_back(index);
    }
    releaseBufferCondition_.notify_all();
    if (buffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
        MEDIA_LOG_I("EOS received, ready to stop.");
        hasReceivedEOS_ = true;
        std::unique_lock<std::mutex> lock(stopMutex_);
        stopCondition_.notify_all();
    }
}

void SurfaceEncoderAdapter::ReleaseBuffer()
{
    MEDIA_LOG_I("ReleaseBuffer");
    while (true) {
        if (isThreadExit_) {
            MEDIA_LOG_I("Exit ReleaseBuffer thread.");
            break;
        }
        std::vector<uint32_t> indexs;
        {
            std::unique_lock<std::mutex> lock(releaseBufferMutex_);
            releaseBufferCondition_.wait(lock, [this] {
                return isThreadExit_ || !indexs_.empty();
            });
            indexs = indexs_;
            indexs_.clear();
        }
        for (auto &index : indexs) {
            codecServer_->ReleaseOutputBuffer(index);
        }
    }
    MEDIA_LOG_I("ReleaseBuffer end");
}

void SurfaceEncoderAdapter::ConfigureAboutRGBA(MediaAVCodec::Format &format, const std::shared_ptr<Meta> &meta)
{
    Plugins::VideoPixelFormat pixelFormat = Plugins::VideoPixelFormat::NV12;
    if (meta->Find(Tag::VIDEO_PIXEL_FORMAT) != meta->end()) {
        meta->Get<Tag::VIDEO_PIXEL_FORMAT>(pixelFormat);
    }
    format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(pixelFormat));

    if (meta->Find(Tag::VIDEO_ENCODE_BITRATE_MODE) != meta->end()) {
        Plugins::VideoEncodeBitrateMode videoEncodeBitrateMode;
        meta->Get<Tag::VIDEO_ENCODE_BITRATE_MODE>(videoEncodeBitrateMode);
        format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, videoEncodeBitrateMode);
    }
}

void SurfaceEncoderAdapter::ConfigureAboutEnableTemporalScale(MediaAVCodec::Format &format,
    const std::shared_ptr<Meta> &meta)
{
    if (meta->Find(Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY) != meta->end()) {
        bool enableTemporalScale;
        meta->Get<Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY>(enableTemporalScale);
        if (!enableTemporalScale) {
            MEDIA_LOG_I("video encoder enableTemporalScale is false!");
            return;
        }
        OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true);
        bool isSupported = OH_AVCapability_IsFeatureSupported(capability, VIDEO_ENCODER_TEMPORAL_SCALABILITY);
        if (isSupported) {
            MEDIA_LOG_I("VIDEO_ENCODER_TEMPORAL_SCALABILITY is supported!");
            format.PutIntValue(MediaAVCodec::MediaDescriptionKey::OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY,
                1);
        } else {
            MEDIA_LOG_I("VIDEO_ENCODER_TEMPORAL_SCALABILITY is not supported!");
        }
    }
}

void SurfaceEncoderAdapter::SetFaultEvent(const std::string &errMsg, int32_t ret)
{
    SetFaultEvent(errMsg + ", ret = " + std::to_string(ret));
}

void SurfaceEncoderAdapter::SetFaultEvent(const std::string &errMsg)
{
    VideoCodecFaultInfo videoCodecFaultInfo;
    videoCodecFaultInfo.appName = bundleName_;
    videoCodecFaultInfo.instanceId = std::to_string(instanceId_);
    videoCodecFaultInfo.callerType = "player_framework";
    videoCodecFaultInfo.videoCodec = codecMimeType_;
    videoCodecFaultInfo.errMsg = errMsg;
    FaultVideoCodecEventWrite(videoCodecFaultInfo);
}

void SurfaceEncoderAdapter::SetCallingInfo(int32_t appUid, int32_t appPid,
    const std::string &bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;
}

void SurfaceEncoderAdapter::OnInputParameterWithAttrAvailable(uint32_t index, std::shared_ptr<Format> &attribute,
    std::shared_ptr<Format> &parameter)
{
    MediaAVCodec::AVCodecTrace trace("SurfaceEncoderAdapter::OnInputParameterWithAttrAvailable");
    if (isTransCoderMode) {
        MEDIA_LOG_D("isTransCoderMode");
        parameter->PutIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, false);
        codecServer_->QueueInputParameter(index);
        return;
    }
    std::lock_guard<std::mutex> lock(checkFramesMutex_);
    int64_t currentPts = 0;
    attribute->GetLongValue(Tag::MEDIA_TIME_STAMP, currentPts);
    MEDIA_LOG_D("OnInputParameterWithAttrAvailable currentPts " PUBLIC_LOG_D64, currentPts);
    
    bool isDroppedFrames = CheckFrames(currentPts, checkFramesPauseTime_);
    MEDIA_LOG_D("OnInputParameterWithAttrAvailable checkFramesPauseTime " PUBLIC_LOG_D64, checkFramesPauseTime_);
    MEDIA_LOG_D("OnInputParameterWithAttrAvailable isDroppedFrames = " PUBLIC_LOG_S,
        isDroppedFrames ? "true" : "false");
    {
        std::lock_guard<std::mutex> mappingLock(mappingPtsMutex_);
        // adjustPts means timestamps after resume time are translated to corresponding ones after pause time
        int64_t adjustPts = currentPts - totalPauseTimeQueue_[0] + checkFramesPauseTime_;
        MEDIA_LOG_D("OnInputParameterWithAttrAvailable adjustPts " PUBLIC_LOG_D64, adjustPts);
        MEDIA_LOG_D("OnInputParameterWithAttrAvailable totalPauseTimeQueue_[0] " PUBLIC_LOG_D64,
                    totalPauseTimeQueue_[0]);
        if (!isDroppedFrames) {
            if (startBufferTime_ == -1) {
                startBufferTime_ = currentPts;
            }
            int64_t mappingTime = adjustPts - startBufferTime_;
            MEDIA_LOG_D("OnInputParameterWithAttrAvailable mappingTime = " PUBLIC_LOG_D64, mappingTime);
            preKeyFramePts_ = currentKeyFramePts_;
            currentKeyFramePts_ = currentPts;
            AddStartPts(currentPts);
            AddPauseResumePts(currentPts);
            parameter->PutLongValue(Tag::VIDEO_ENCODE_SET_FRAME_PTS, mappingTime);
        }
        lastBufferTime_ = currentPts;
    }
    parameter->PutIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, isDroppedFrames);
    codecServer_->QueueInputParameter(index);
    FALSE_RETURN_MSG(videoFrameRate_ != 0, "videoFrameRate_ = 0, invalid value.");
    if (stopTime_ != -1 && currentPts > stopTime_ - (SEC_TO_NS / videoFrameRate_)) {
        MEDIA_LOG_I("currentPts > stopTime, send EOS.");
        int32_t ret = codecServer_->NotifyEos();
        if (ret != 0) {
            MEDIA_LOG_E("OnInputParameterWithAttrAvailable codecServer_->NotifyEos() failed!");
        }
    }
}

bool SurfaceEncoderAdapter::CheckFrames(int64_t currentPts, int64_t &checkFramesPauseTime)
{
    if (pauseResumeQueue_.empty()) {
        return false;
    }
    auto stateCode = pauseResumeQueue_[0].second;
    MEDIA_LOG_D("CheckFrames stateCode: " PUBLIC_LOG_D32
        " time:" PUBLIC_LOG_D64, static_cast<int32_t>(stateCode), pauseResumeQueue_[0].first);
    // means not dropped frames when less than pause time
    if (stateCode == StateCode::PAUSE && currentPts < pauseResumeQueue_[0].first) {
        return false;
    }
    // means dropped frames when less than resume time
    if (stateCode == StateCode::RESUME && currentPts < pauseResumeQueue_[0].first) {
        return true;
    }
    // pop expire nodes
    // currentpts bigger than the first two node, it means the pause and resume node are useless
    if (pauseResumeQueue_.size() > 1
        && currentPts >= pauseResumeQueue_[0].first
        && currentPts >= pauseResumeQueue_[1].first) {
        if (pauseResumeQueue_[0].second == StateCode::RESUME && !totalPauseTimeQueue_.empty()) {
            // pop totalPauseTime
            totalPauseTimeQueue_.pop_front();
        }
        pauseResumeQueue_.pop_front();
        MEDIA_LOG_I("pauseResumeQueue_ pop expire nodes");
        return CheckFrames(currentPts, checkFramesPauseTime);
    }
    if (stateCode == StateCode::RESUME && currentPts >= pauseResumeQueue_[0].first) {
        if (!totalPauseTimeQueue_.empty()) {
            // after resume pop totalPauseTime
            totalPauseTimeQueue_.pop_front();
        }
        // resumetime之后第一帧与resumetime的差
        checkFramesPauseTime = checkFramesPauseTime - (currentPts - pauseResumeQueue_[0].first);
    }
    pauseResumeQueue_.pop_front();
    return CheckFrames(currentPts, checkFramesPauseTime);
}

void SurfaceEncoderAdapter::GetCurrentTime(int64_t &currentTime)
{
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    currentTime = static_cast<int64_t>(timestamp.tv_sec) * SEC_TO_NS + static_cast<int64_t>(timestamp.tv_nsec);
}

void SurfaceEncoderAdapter::AddStartPts(int64_t currentPts)
{
    // start time
    if (isStartKeyFramePts_) {
        keyFramePts_ += std::to_string(currentPts / NS_PER_US) + ",";
        isStartKeyFramePts_ = false;
        if (encoderAdapterKeyFramePtsCallback_) {
            encoderAdapterKeyFramePtsCallback_->OnReportFirstFramePts(currentPts);
        } else {
            MEDIA_LOG_E("encoderAdapterKeyFramePtsCallback_ is null, can't report firstFramePts");
        }
        MEDIA_LOG_I("AddStartPts success %{public}s end", keyFramePts_.c_str());
    }
}

void SurfaceEncoderAdapter::AddStopPts()
{
    // stop time
    MEDIA_LOG_D("AddStopPts enter");
    if (isStopKeyFramePts_) {
        if (currentKeyFramePts_ > stopTime_) {
            keyFramePts_ += std::to_string(preKeyFramePts_ / NS_PER_US);
            MEDIA_LOG_I("AddStopPts preKeyFramePts_ %{public}s end", keyFramePts_.c_str());
        } else {
            keyFramePts_ += std::to_string(currentKeyFramePts_ / NS_PER_US);
            MEDIA_LOG_I("AddStopPts currentKeyFramePts_ %{public}s end", keyFramePts_.c_str());
        }
        isStopKeyFramePts_ = false;
        if (encoderAdapterKeyFramePtsCallback_) {
            encoderAdapterKeyFramePtsCallback_->OnReportKeyFramePts(keyFramePts_);
        }
        keyFramePts_.clear();
    }
}

bool SurfaceEncoderAdapter::AddPauseResumePts(int64_t currentPts)
{
    if (pauseResumePts_.empty()) {
        return false;
    }
    auto stateCode = pauseResumePts_[0].second;
    MEDIA_LOG_D("CheckFrames stateCode: " PUBLIC_LOG_D32
        " time:" PUBLIC_LOG_D64, static_cast<int32_t>(stateCode), pauseResumePts_[0].first);
    // means not dropped frames when less than pause time
    if (stateCode == StateCode::PAUSE && currentPts < pauseResumePts_[0].first) {
        return false;
    }
    // means dropped frames when less than resume time
    if (stateCode == StateCode::RESUME && currentPts < pauseResumePts_[0].first) {
        return true;
    }
    if (stateCode == StateCode::PAUSE) {
        MEDIA_LOG_D("AddPausePts %{public}s start", keyFramePts_.c_str());
        keyFramePts_ += std::to_string(preKeyFramePts_ / NS_PER_US) + ",";
        MEDIA_LOG_D("AddPausePts %{public}s end", keyFramePts_.c_str());
    }
    if (stateCode == StateCode::RESUME) {
        MEDIA_LOG_D("AddResumePts %{public}s start", keyFramePts_.c_str());
        keyFramePts_ += std::to_string(currentKeyFramePts_ / NS_PER_US) + ",";
        if (encoderAdapterKeyFramePtsCallback_) {
            encoderAdapterKeyFramePtsCallback_->OnReportFirstFramePts(currentKeyFramePts_);
        } else {
            MEDIA_LOG_E("encoderAdapterKeyFramePtsCallback_ is null, can't report firstFramePts");
        }
        MEDIA_LOG_D("AddResumePts %{public}s end", keyFramePts_.c_str());
    }
    pauseResumePts_.pop_front();
    return AddPauseResumePts(currentPts);
}

void SurfaceEncoderAdapter::HandleWaitforStop()
{
    // Determine whether to end directly or wait STOP_TIME_OUT_MS
    std::unique_lock<std::mutex> lock(stopMutex_);
    if (hasReceivedEOS_) {
        MEDIA_LOG_I("SurfaceEncoderAdapter::HandleWaitforStop, EOS has received, directly stop");
        return;
    }
    std::cv_status waitStatus = stopCondition_.wait_for(lock, std::chrono::milliseconds(STOP_TIME_OUT_MS));
    // Waiting timeout with no video frame received
    if (waitStatus == std::cv_status::timeout && currentKeyFramePts_ == -1) {
        MEDIA_LOG_E("Codec wait timeout with no video frame received");
        encoderAdapterCallback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL,
                                         AVCODEC_ERR_TIMEOUT_NO_FRAME_RECEIVED);
    }
}

void SurfaceEncoderAdapter::Clear()
{
    MEDIA_LOG_I("SurfaceEncoderAdapter::Clear enter");
    startBufferTime_ = -1;
    stopTime_ = -1;
    pauseTime_ = -1;
    resumeTime_ = -1;
    totalPauseTime_ = 0;
    isStart_ = false;
    isResume_ = false;
    isStartKeyFramePts_ = false;
    pauseResumeQueue_.clear();
    pauseResumePts_.clear();
    totalPauseTimeQueue_ = {0};
    checkFramesPauseTime_ = 0;
    currentPts_ = -1;
    currentKeyFramePts_ = -1;
    preKeyFramePts_ = -1;
}

bool SurfaceEncoderAdapter::GetIsTransCoderMode()
{
    return isTransCoderMode;
}
} // namespace MEDIA
} // namespace OHOS
