/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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
#include "scoped_timer.h"
#include "sync_fence.h"
#include "media_demuxer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "VideoDecoderAdapter" };
}

namespace OHOS {
namespace Media {
using namespace MediaAVCodec;
using FileType = OHOS::Media::Plugins::FileType;
const std::string VIDEO_INPUT_BUFFER_QUEUE_NAME = "VideoDecoderInputBufferQueue";
constexpr uint64_t DECODER_USAGE =
    BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_VIDEO_DECODER;
static const int64_t CODEC_START_WARNING_MS = 50;

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

class VideoConsumerListener : public IBufferConsumerListener {
public:
    explicit VideoConsumerListener(sptr<Surface> consumerSurface) : consumerSurface_(consumerSurface) {};

    ~VideoConsumerListener() override = default;
 
    void OnBufferAvailable() override
    {
        sptr<Surface> consumerSurface = consumerSurface_.promote();
        FALSE_RETURN_MSG(consumerSurface != nullptr, "consumerSurface_ is nullptr");
        sptr<SurfaceBuffer> surfaceBuffer = nullptr;
        sptr<SyncFence> fence = nullptr;
        int64_t timestamp = 0;
        OHOS::Rect damage = {};
        GSError err = consumerSurface->AcquireBuffer(surfaceBuffer, fence, timestamp, damage);
        FALSE_RETURN_MSG(err == GSERROR_OK, "AcquireBuffer failed, err:%{public}d", err);
        err = consumerSurface->ReleaseBuffer(surfaceBuffer, -1);
        FALSE_RETURN_MSG(err == GSERROR_OK, "ReleaseBuffer failed, err:%{public}d", err);
    }

private:
    wptr<Surface> consumerSurface_{nullptr};
};

VideoDecoderAdapter::VideoDecoderAdapter()
{
    MEDIA_LOG_D_SHORT("VideoDecoderAdapter instances create.");
}

VideoDecoderAdapter::~VideoDecoderAdapter()
{
    MEDIA_LOG_I_SHORT("~VideoDecoderAdapter()");
    FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec_ is nullptr");
    mediaCodec_->Release();
    std::unique_lock<std::mutex> lock(dtsQueMutex_);
    if (!inputBufferDtsQue_.empty()) {
        MEDIA_LOG_I("Clear dtsQue_, currrent size: " PUBLIC_LOG_U64, static_cast<uint64_t>(inputBufferDtsQue_.size()));
        inputBufferDtsQue_.clear();
    }
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
    return Status::OK;
}

Status VideoDecoderAdapter::Configure(const Format &format)
{
    MEDIA_LOG_I_SHORT("VideoDecoderAdapter->Configure.");
    std::shared_ptr<Media::Meta> metaInfo = const_cast<Format &>(format).GetMeta();
    FileType currentFileType = FileType::UNKNOW;
    if (metaInfo != nullptr && metaInfo->GetData(Media::Tag::MEDIA_FILE_TYPE, currentFileType)) {
        fileType_ = static_cast<int32_t>(currentFileType);
        MEDIA_LOG_I("Media file type " PUBLIC_LOG_D32, fileType_);
    }
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    Format formatCopy = format;
    std::shared_ptr<Media::Meta> metaInfoCopy = const_cast<Format &>(formatCopy).GetMeta();
    std::string codecMimeType_;
    metaInfo->GetData(Tag::MIME_TYPE, codecMimeType_);
    if (codecMimeType_ != MediaAVCodec::CodecMimeType::VIDEO_VC1 &&
        codecMimeType_ != MediaAVCodec::CodecMimeType::VIDEO_WMV3 &&
        codecMimeType_ != MediaAVCodec::CodecMimeType::VIDEO_RV30) {
        metaInfoCopy->Remove(std::string(MediaDescriptionKey::MD_KEY_CODEC_CONFIG));
    }
    int32_t ret = mediaCodec_->Configure(formatCopy);
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
    int32_t ret;
    {
        ScopedTimer timer("mediaCodec Start", CODEC_START_WARNING_MS);
        ret = mediaCodec_->Start();
    }
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
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status VideoDecoderAdapter::Stop()
{
    MEDIA_LOG_I_SHORT("Stop enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG(isConfigured_, Status::ERROR_INVALID_STATE, "mediaCodec_ is not configured");
    mediaCodec_->Stop();
    return Status::OK;
}

Status VideoDecoderAdapter::Flush()
{
    MEDIA_LOG_I_SHORT("Flush enter.");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_INVALID_STATE, "mediaCodec_ is nullptr");
    FALSE_RETURN_V_MSG(isConfigured_, Status::ERROR_INVALID_STATE, "mediaCodec_ is not configured");
    int32_t ret = mediaCodec_->Flush();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (inputBufferQueueConsumer_ != nullptr) {
            for (auto &buffer : bufferVector_) {
                inputBufferQueueConsumer_->DetachBuffer(buffer);
            }
            bufferVector_.clear();
            inputBufferQueueConsumer_->SetQueueSize(0);
        }
    }
    {
        std::unique_lock<std::mutex> lock(dtsQueMutex_);
        if (!inputBufferDtsQue_.empty()) {
            MEDIA_LOG_I("Clear dtsQue_, currrent size: " PUBLIC_LOG_U64,
                static_cast<uint64_t>(inputBufferDtsQue_.size()));
            inputBufferDtsQue_.clear();
        }
    }
    MEDIA_LOG_I_SHORT("Flush end.");
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
    isRenderStarted_ = false;
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
        if (ptsManagedFileTypes.find(static_cast<FileType>(fileType_)) != ptsManagedFileTypes.end()) {
            GetInputBufferDts(tmpBuffer);
        }
        FALSE_RETURN_MSG(tmpBuffer->meta_ != nullptr, "tmpBuffer is nullptr.");
        int32_t metaIndex;
        FALSE_RETURN_MSG(tmpBuffer->meta_->GetData(Tag::REGULAR_TRACK_ID, metaIndex), "get index failed.");
        uint32_t index = static_cast<uint32_t>(metaIndex);
        if (tmpBuffer->flag_ & (uint32_t)(Plugins::AVBufferFlag::EOS)) {
            tmpBuffer->memory_->SetSize(0);
        }
        FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec_ is nullptr.");
        if (!isRenderStarted_.load() && !(tmpBuffer->flag_& static_cast<uint32_t>(Plugins::AVBufferFlag::EOS))) {
            MEDIA_LOG_I("AquireAvailableInputBuffer for first frame,  index: %{public}u,  bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, tmpBuffer->GetUniqueId(),
                tmpBuffer->pts_, tmpBuffer->flag_);
            isRenderStarted_ = true;
        }
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

void VideoDecoderAdapter::GetInputBufferDts(std::shared_ptr<AVBuffer> &inputBuffer)
{
    FALSE_RETURN_MSG(inputBuffer != nullptr, "inputBuffer is nullptr.");
    std::unique_lock<std::mutex> lock(dtsQueMutex_);
    inputBufferDtsQue_.push_back(inputBuffer->dts_);
    MEDIA_LOG_D("Inputbuffer DTS: " PUBLIC_LOG_D64 " dtsQue_ size: " PUBLIC_LOG_U64,
        inputBuffer->dts_, static_cast<uint64_t>(inputBufferDtsQue_.size()));
}

void VideoDecoderAdapter::SetOutputBufferPts(std::shared_ptr<AVBuffer> &outputBuffer)
{
    FALSE_RETURN_MSG(outputBuffer != nullptr, "outputBuffer is nullptr.");
    std::unique_lock<std::mutex> lock(dtsQueMutex_);
    if (!inputBufferDtsQue_.empty()) {
        outputBuffer->pts_ = inputBufferDtsQue_.front();
        inputBufferDtsQue_.pop_front();
        MEDIA_LOG_D("Outputbuffer PTS: " PUBLIC_LOG_D64 " dtsQue_ size: " PUBLIC_LOG_U64,
            outputBuffer->pts_, static_cast<uint64_t>(inputBufferDtsQue_.size()));
    } else {
        MEDIA_LOG_W("DtsQue_ is empty.");
        outputBuffer->pts_ = outputBuffer->dts_;
    }
}

void VideoDecoderAdapter::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    AVCodecTrace trace("VideoDecoderAdapter::OnInputBufferAvailable");
    FALSE_RETURN_MSG(buffer != nullptr && buffer->meta_ != nullptr, "meta_ is nullptr.");
    buffer->meta_->SetData(Tag::REGULAR_TRACK_ID, static_cast<int32_t>(index));
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
        inputBufferQueueConsumer_->SetQueueSizeAndAttachBuffer(size, buffer, false);
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
    if (ptsManagedFileTypes.find(static_cast<FileType>(fileType_)) != ptsManagedFileTypes.end()) {
        SetOutputBufferPts(buffer);
    }
    FALSE_RETURN_MSG(callback_ != nullptr, "callback_ is nullptr");
    callback_->OnOutputBufferAvailable(index, buffer);
}

int32_t VideoDecoderAdapter::GetOutputFormat(Format &format)
{
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL,
        "GetOutputFormat mediaCodec_ is nullptr");
    return mediaCodec_->GetOutputFormat(format);
}

int32_t VideoDecoderAdapter::ReleaseOutputBuffer(uint32_t index, bool render, int64_t pts)
{
    AVCodecTrace trace("VideoDecoderAdapter::ReleaseOutputBuffer pts " + std::to_string(pts) + " " +
                       std::to_string(render));
    FALSE_RETURN_V(mediaCodec_ != nullptr, 0);
    FALSE_RETURN_V_NOLOG(!isPerfRecEnabled_, ReleaseOutputBufferWithPerfRecord(index, render));
    mediaCodec_->ReleaseOutputBuffer(index, render);
    return 0;
}

Status VideoDecoderAdapter::SetPerfRecEnabled(bool isPerfRecEnabled)
{
    isPerfRecEnabled_ = isPerfRecEnabled;
    return Status::OK;
}

int32_t VideoDecoderAdapter::ReleaseOutputBufferWithPerfRecord(uint32_t index, bool render)
{
    int64_t renderTime = CALC_EXPR_TIME_MS(mediaCodec_->ReleaseOutputBuffer(index, render));
    FALSE_RETURN_V_MSG(eventReceiver_ != nullptr, 0, "Report perf failed, callback is nullptr");
    FALSE_RETURN_V_NOLOG(perfRecorder_.Record(renderTime) == PerfRecorder::FULL, 0);
    eventReceiver_->
        OnDfxEvent({ "VRNDR", DfxEventType::DFX_INFO_PERF_REPORT, perfRecorder_.GetMainPerfData() });
    perfRecorder_.Reset();
    return 0;
}

int32_t VideoDecoderAdapter::RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs, int64_t pts)
{
    AVCodecTrace trace("RenderOutputBufferAtTime pts " + std::to_string(pts) + " time " +
                       std::to_string(renderTimestampNs));
    MEDIA_LOG_D_SHORT("VideoDecoderAdapter::RenderOutputBufferAtTime");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    mediaCodec_->RenderOutputBufferAtTime(index, renderTimestampNs);
    return 0;
}

int32_t VideoDecoderAdapter::SetOutputSurface(sptr<Surface> videoSurface)
{
    MEDIA_LOG_I_SHORT("VideoDecoderAdapter::SetOutputSurface");
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL, "mediaCodec_ is nullptr");
    
    if (videoSurface == nullptr) {
        InitDefaultSurface();
        return mediaCodec_->SetOutputSurface(producerSurface_);
    }

    auto status = mediaCodec_->SetOutputSurface(videoSurface);
    FALSE_RETURN_V_NOLOG(status != MSERR_OK, status);
    producerSurface_ = nullptr;
    consumerSurface_ = nullptr;
    return MSERR_OK;
}

void VideoDecoderAdapter::InitDefaultSurface()
{
    MEDIA_LOG_D("InitDefaultSurface enter");
    consumerSurface_ = Surface::CreateSurfaceAsConsumer("VirtualVideoSurface");
    FALSE_RETURN_MSG(consumerSurface_ != nullptr, "InitSurface create consumer surface failed.");
 
    GSError err = consumerSurface_->SetDefaultUsage(DECODER_USAGE);
    FALSE_RETURN_MSG(err == GSERROR_OK, "InitSurface SetDefaultUsage failed.");
    sptr<IBufferConsumerListener> listener = new VideoConsumerListener(consumerSurface_);
    err = consumerSurface_->RegisterConsumerListener(listener);
    FALSE_RETURN_MSG(err == GSERROR_OK, "InitSurface RegisterConsumerListener failed.");
 
    sptr<IBufferProducer> producer = consumerSurface_->GetProducer();
    producerSurface_ = Surface::CreateSurfaceAsProducer(producer);
    FALSE_RETURN_MSG(producerSurface_ != nullptr, "InitSurface create producer surface failed.");
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

bool VideoDecoderAdapter::IsHwDecoder()
{
    OHOS::MediaAVCodec::Format format = OHOS::MediaAVCodec::Format();
    mediaCodec_->GetCodecInfo(format);
    int32_t isHardware = 0;
    format.GetIntValue(Media::Tag::MEDIA_IS_HARDWARE, isHardware);
    MEDIA_LOG_I("isHardware: %{public}d ", isHardware);
    return isHardware ==1 ? true : false;
}

void VideoDecoderAdapter::NotifyMemoryExchange(bool exchangeFlag)
{
    FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec_ is nullptr");
    AVCodecTrace trace("VideoDecoderAdapter::NotifyMemoryExchange " + std::to_string(exchangeFlag));
    MEDIA_LOG_I("Memory exchange %{public}d begin", exchangeFlag);
    mediaCodec_->NotifyMemoryExchange(exchangeFlag);
    MEDIA_LOG_I("Memory exchange %{public}d end", exchangeFlag);
}
} // namespace Media
} // namespace OHOS
