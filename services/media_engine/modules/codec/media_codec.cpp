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

#include "media_codec.h"
#include <iomanip>                // TODO
#include <iostream>               // TODO
#include "av_shared_memory_ext.h" // TODO
#include "avcodec_errors.h"
#include "codeclist_core.h"
#include "codeclist_utils.h"
#include "codeclistbase.h"
#include "common/log.h"
#include "hcodec_adapter_loader.h"
#include <shared_mutex>

#ifdef LOG_DOMAIN
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002BAC
#endif
#define STATIC_CAST_CBADAPTER std::static_pointer_cast<VideoCodecCallback>(cbAdapter_)
namespace {
constexpr int32_t DEFAULT_BUFFER_NUM = 4;
constexpr int32_t TIME_OUT_MS = 8;
constexpr uint32_t INVAILD_INDEX = UINT32_MAX;
const std::string INPUT_BUFFER_QUEUE_NAME = "MediaCodecInputBufferQueue";
const std::map<OHOS::Media::CodecState, std::string> STATE_STR_MAP = {
    {OHOS::Media::CodecState::UNINITIALIZED, " UNINITIALIZED"},
    {OHOS::Media::CodecState::CONFIGURED, " CONFIGURED"},
    {OHOS::Media::CodecState::INITIALIZED, " INITIALIZED"},
    {OHOS::Media::CodecState::PREPARED, " PREPARED"},
    {OHOS::Media::CodecState::RUNNING, " RUNNING"},
    {OHOS::Media::CodecState::FLUSHED, " FLUSHED"},
    {OHOS::Media::CodecState::END_OF_STREAM, " END_OF_STREAM"},
    {OHOS::Media::CodecState::ERROR, " ERROR"},
};
} // namespace
namespace OHOS {
namespace Media {
using namespace MediaAVCodec;
using namespace Plugin;

class CodecConsumerListener : public IConsumerListener {
public:
    CodecConsumerListener(std::shared_ptr<MediaCodec> mediaCodec) : mediaCodec_(mediaCodec) {}

    void OnBufferAvailable() override
    {
        mediaCodec_.lock()->ProcessInputBuffer();
    }

private:
    std::weak_ptr<MediaCodec> mediaCodec_;
    // weak_ptr<VideoCodecCallback> cbAdapter_;
    // bool isVideo_;
};
class CodecProducerListener : public IRemoteStub<IProducerListener> {
public:
    CodecProducerListener(std::shared_ptr<MediaCodec> mediaCodec) : mediaCodec_(mediaCodec) {}

    void OnBufferAvailable() override
    {
        mediaCodec_.lock()->ProcessOutputBuffer();
    }

private:
    std::weak_ptr<MediaCodec> mediaCodec_;
    // weak_ptr<VideoCodecCallback> cbAdapter_;
    // bool isVideo_;
};

class VideoCodecCallback : public AVCodecCallbackAdapter {
public:
    VideoCodecCallback() : codecCallback_(nullptr) {}
    ~VideoCodecCallback() override = default;

    using AVBufferObject = struct AVBufferObject {
        std::shared_ptr<AVBuffer> buffer = nullptr;
        uint32_t index = INVAILD_INDEX;
    };

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        FALSE_RETURN_MSG(codecCallback_ != nullptr, "codecCallback_ is nullptr");
        std::shared_lock<std::shared_mutex> lock(outputMutex_);
        codecCallback_->OnError(errorType, errorCode);
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        FALSE_RETURN_MSG(codecCallback_ != nullptr, "codecCallback_ is nullptr");
        std::lock_guard<std::shared_mutex> lock(outputMutex_);
        if (outputBufferQueueProducer_ != nullptr) {
            for (auto &val : outputMap_) {
                outputBufferQueueProducer_->DetachBuffer(val.second.buffer);
            }
        }
        outputMap_.clear();
        auto formatPtr = std::make_shared<Format>();
        *formatPtr = format;
        codecCallback_->OnOutputFormatChanged(formatPtr);
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        std::lock_guard<std::shared_mutex> lock(inputMutex_);
        uint64_t uniqueId = buffer->GetUniqueId();
        // static int32_t frameCount = 0;
        // std::cout << "out == Call Id: " << std::hex << (uniqueId >> 48) << "\n"
        //           << "out == Call Size: " << std::dec << buffer->memory_->GetSize() << "\n"
        //           << "out == Call frameCount = " << frameCount++ << ", index = " << index << "\n"
        //           << "out == Call Capacity: " << buffer->memory_->GetCapacity() << "\n\n";
        auto iter = outputMap_.find(uniqueId);
        Status ret = Status::OK;
        if (iter == outputMap_.end()) {
            outputMap_[uniqueId].index = index;
            outputMap_[uniqueId].buffer = buffer;
            if (!isSurfaceMode_) {
                ret = outputBufferQueueProducer_->AttachBuffer(buffer, true);
                FALSE_RETURN_MSG(ret == Status::OK, "output buffer queue producer AttachBuffer failed");
            } else {
                codecCallback_->OnSurfaceModeDataFilled(buffer);
            }
            return;
        }
        iter->second.index = index;
        iter->second.buffer = buffer;
        if (!isSurfaceMode_) {
            ret = outputBufferQueueProducer_->PushBuffer(buffer, true);
            FALSE_RETURN_MSG(ret == Status::OK, "output buffer queue producer PushBuffer failed");
        } else {
            codecCallback_->OnSurfaceModeDataFilled(buffer);
        }
        return;
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        FALSE_RETURN_MSG(inputBufferQueueConsumer_ != nullptr, "inputBufferQueueConsumer_ is nullptr");
        std::lock_guard<std::shared_mutex> lock(outputMutex_);
        uint64_t uniqueId = buffer->GetUniqueId();
        // static int32_t frameCount = 0;
        // std::cout << "in == Call Id: " << std::hex << (uniqueId >> 48) << "\n"
        //           << "in == Call frameCount: " << std::dec << frameCount++ << "\n"
        //           << "in == Call Capacity: " << buffer->memory_->GetCapacity() << "\n\n";
        Status ret = Status::OK;
        auto iter = inputMap_.find(uniqueId);
        if (iter == inputMap_.end()) {
            inputMap_[uniqueId].index = index;
            inputMap_[uniqueId].buffer = buffer;
            ; // TODO size ++
            ret = inputBufferQueueConsumer_->AttachBuffer(buffer, false);
            FALSE_RETURN_MSG(ret == Status::OK, "attach buffer queue consumer AttachBuffer failed");
            return;
        }
        iter->second.index = index;
        iter->second.buffer = buffer;
        ret = inputBufferQueueConsumer_->ReleaseBuffer(buffer);
        FALSE_RETURN_MSG(ret == Status::OK, "input buffer queue consumer ReleaseBuffer failed");
    }

    void SetCallback(std::shared_ptr<CodecCallback> &codecCallback)
    {
        FALSE_RETURN_MSG(codecCallback != nullptr, "codecCallback is nullptr");
        std::lock_guard<std::shared_mutex> lock(outputMutex_);
        codecCallback_ = codecCallback;
    }

    void SetInputBufferQueue(sptr<AVBufferQueueConsumer> inputBufferQueueConsumer)
    {
        FALSE_RETURN_MSG(inputBufferQueueConsumer != nullptr, "inputBufferQueueConsumer is nullptr");
        std::lock_guard<std::shared_mutex> lock(inputMutex_);
        inputBufferQueueConsumer_ = inputBufferQueueConsumer;
    }

    void SetOutputBufferQueue(sptr<AVBufferQueueProducer> outputBufferQueueProducer)
    {
        FALSE_RETURN_MSG(outputBufferQueueProducer != nullptr, "inputBufferQueueConsumer is nullptr");
        std::lock_guard<std::shared_mutex> lock(outputMutex_);
        outputBufferQueueProducer_ = outputBufferQueueProducer;
    }

    void SetIsSurface(bool isSurfaceMode)
    {
        isSurfaceMode_ = isSurfaceMode;
    }

    uint32_t FindInputIndex(const std::shared_ptr<AVBuffer> &buffer)
    {
        std::shared_lock<std::shared_mutex> lock(inputMutex_);
        auto iter = inputMap_.find(buffer->GetUniqueId());
        if (iter != inputMap_.end()) {
            return iter->second.index;
        }
        return INVAILD_INDEX;
    }

    uint32_t FindOutputIndex(const std::shared_ptr<AVBuffer> &buffer)
    {
        std::shared_lock<std::shared_mutex> lock(outputMutex_);
        auto iter = outputMap_.find(buffer->GetUniqueId());
        if (iter != outputMap_.end()) {
            return iter->second.index;
        }
        return INVAILD_INDEX;
    }

    void ClearBuffer()
    {
        std::scoped_lock lock(outputMutex_, inputMutex_);
        for (auto &val : inputMap_) {
            (void)inputBufferQueueConsumer_->DetachBuffer(val.second.buffer);
        }
        inputMap_.clear();

        for (auto &val : outputMap_) {
            (void)outputBufferQueueProducer_->DetachBuffer(val.second.buffer);
        }
        outputMap_.clear();
    }

private:
    std::shared_ptr<CodecCallback> codecCallback_ = nullptr;
    std::shared_ptr<AVBufferQueue> inputBufferQueue_ = nullptr;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_ = nullptr;
    sptr<AVBufferQueueProducer> outputBufferQueueProducer_ = nullptr;
    std::unordered_map<uint64_t, AVBufferObject> inputMap_;
    std::unordered_map<uint64_t, AVBufferObject> outputMap_;
    std::shared_mutex inputMutex_;
    std::shared_mutex outputMutex_;
    bool isSurfaceMode_ = false;
};

Status MediaCodec::Init(const std::string &mime, bool isEncoder)
{
    std::shared_ptr<CodecListCore> codecListCore = std::make_shared<CodecListCore>();
    std::string codecname;
    Format format;
    format.PutStringValue("codec_mime", mime);
    if (isEncoder) {
        codecname = codecListCore->FindEncoder(format);
    } else {
        codecname = codecListCore->FindDecoder(format);
    }
    isVideoEncoder_ = isEncoder;
    isVideoEncoderChecked_ = true;
    FALSE_RETURN_V_MSG(!codecname.empty(), Status::ERROR_INVALID_OPERATION,
                       "Create codec by mime failed: error mime type");
    return Init(codecname);
}

Status MediaCodec::Init(const std::string &name)
{
    std::shared_ptr<CodecListCore> codecListCore = std::make_shared<CodecListCore>();
    CodecType codecType = codecListCore->FindCodecType(name);
    switch (codecType) {
        case CodecType::AVCODEC_HCODEC: {
            isVideo_ = true;
            codecAdapter_ = HCodecAdapterLoader::CreateByName(name);
            break;
        }
        // case CodecType::AVCODEC_VIDEO_CODEC: {
        //     isVideo_ = true;
        //     codecAdapter_ = std::make_shared<Codec::FCodec>(name);
        //     break;
        // }
        case CodecType::AVCODEC_AUDIO_CODEC: {
            isVideo_ = false;
            // codecPlugin_ = std::make_shared<CodecPlugin>();
            break;
        }
        default: {
            MEDIA_LOG_E("Create codec %{public}s failed", name.c_str());
            return Status::ERROR_UNSUPPORTED_FORMAT;
        }
    }
    Status ret = CheckIsEncoder(name);
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");

    MEDIA_LOG_I("Create codec %{public}s successful", name.c_str());
    MEDIA_LOG_I("State from %{public}s to INITIALIZED", GetStatusDescription(state_).data());
    state_ = CodecState::INITIALIZED;
    return Status::OK;
}

Status MediaCodec::Configure(const std::shared_ptr<MediaAVCodec::Format> &meta)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
                       Status::ERROR_INVALID_STATE, "In invalid state");
    if (isVideo_) {
        // Format format;
        // format.SetMeta(*meta);
        codecAdapter_->Configure(*meta);
    } else {
        // codecPlugin_->SetParameter(meta);
    }
    MEDIA_LOG_I("State from %{public}s to CONFIGURED", GetStatusDescription(state_).data());
    state_ = CodecState::CONFIGURED;
    return Status::OK;
}

Status MediaCodec::SetCodecCallback(std::shared_ptr<CodecCallback> &codecCallback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
                       Status::ERROR_INVALID_STATE, "In invalid state");
    Status ret = Status::OK;
    codecCallback_ = codecCallback;
    if (isVideo_) {
        if (cbAdapter_ == nullptr) {
            cbAdapter_ = std::make_shared<VideoCodecCallback>();
            ret = codecAdapter_->SetCallback(cbAdapter_) == 0 ? Status::OK : Status::ERROR_UNKNOWN;
            FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
        }
        STATIC_CAST_CBADAPTER->SetCallback(codecCallback_);
    } else {
        // ret = codecPlugin_->SetDataCallback(this);
    }
    return ret;
}

Status MediaCodec::SetOutputSurface(sptr<Surface> &surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(isVideo_, Status::ERROR_INVALID_OPERATION, "Audio codec not supported");
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
                       Status::ERROR_INVALID_STATE, "In invalid state");
    isSurfaceMode_ = codecAdapter_->SetOutputSurface(surface) == 0;
    return isSurfaceMode_ ? Status::OK : Status::ERROR_UNKNOWN;
}

Status MediaCodec::SetOutputBufferQueue(sptr<AVBufferQueueProducer> &bufferQueueProducer)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
                       Status::ERROR_INVALID_STATE, "In invalid state");
    FALSE_RETURN_V_MSG(!isSurfaceMode_, Status::ERROR_INVALID_STATE, "In surface mode");
    if (isVideo_) {
        if (isVideoEncoder_ || !isSurfaceMode_) {
            outputBufferQueueProducer_ = bufferQueueProducer;
            return Status::OK;
        }
        return Status::ERROR_INVALID_STATE;
    }
    outputBufferQueueProducer_ = bufferQueueProducer;
    return Status::OK;
}

sptr<Surface> MediaCodec::GetInputSurface()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(isVideo_, nullptr, "Audio codec not supported");
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, nullptr,
                       "In invalid state");
    auto surface = codecAdapter_->CreateInputSurface();
    isSurfaceMode_ = surface != nullptr;
    return surface;
}

sptr<AVBufferQueueProducer> MediaCodec::GetInputBufferQueue()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(state_ == CodecState::PREPARED || state_ == CodecState::RUNNING ||
                           state_ == CodecState::FLUSHED || state_ == CodecState::END_OF_STREAM,
                       nullptr, "In invalid state");
    // FALSE_RETURN_V_MSG(!isSurfaceMode_, nullptr, "In surface mode");

    if (isVideo_) {
        if (!isVideoEncoder_ || !isSurfaceMode_) {
            return inputBufferQueueProducer_;
        }
        return nullptr;
    }
    return inputBufferQueueProducer_;
}

Status MediaCodec::Prepare()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE, "In invalid state");
    FALSE_RETURN_V_MSG(isSurfaceMode_ || outputBufferQueueProducer_ != nullptr, Status::ERROR_INVALID_OPERATION,
                       "Invalid output bufferqueue");
    FALSE_RETURN_V_MSG(codecCallback_ != nullptr, Status::ERROR_INVALID_OPERATION, "not set callback");
    Status ret = Status::OK;
    if (isVideo_) {
        FALSE_RETURN_V_MSG(cbAdapter_ != nullptr, Status::ERROR_INVALID_OPERATION, "not set callback adapter");
        STATIC_CAST_CBADAPTER->SetIsSurface(isSurfaceMode_);
        if (isVideoEncoder_ || !isSurfaceMode_) {
            STATIC_CAST_CBADAPTER->SetOutputBufferQueue(outputBufferQueueProducer_);
        }
        ret = PrepareInputBufferQueueAdapter();
        FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");

        int32_t retPrepare =
            AVCSErrorToOHAVErrCode(static_cast<OHOS::MediaAVCodec::AVCodecServiceErrCode>(codecAdapter_->Prepare()));
        FALSE_RETURN_V_MSG(retPrepare == 0, Status::ERROR_UNKNOWN, "The operation failed, error codec: " PUBLIC_LOG_D32,
                           retPrepare);
    } else {
        // ret = codecPlugin_->Prepare();
        // FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");

        ret = PrepareInputBufferQueue();
        FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");

        ret = PrepareOutputBufferQueue();
        FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
    }

    MEDIA_LOG_I("State from %{public}s to PREPARED", GetStatusDescription(state_).data());
    state_ = CodecState::PREPARED;
    return ret;
}

Status MediaCodec::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG((state_ == CodecState::PREPARED || state_ == CodecState::FLUSHED) &&
                           inputBufferQueueProducer_ != nullptr,
                       Status::ERROR_INVALID_STATE, "In invalid state");
    Status ret = Status::OK;
    if (isVideo_) {
        ret = codecAdapter_->Start() == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    } else {
        // ret = codecPlugin_->Start();
    }
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
    MEDIA_LOG_I("State from %{public}s to RUNNING", GetStatusDescription(state_).data());
    state_ = CodecState::RUNNING;
    return ret;
}

Status MediaCodec::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM ||
                           state_ == CodecState::FLUSHED,
                       Status::ERROR_INVALID_STATE, "In invalid state");
    Status ret = Status::OK;
    if (isVideo_) {
        ret = codecAdapter_->Stop() == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    } else {
        // ret = codecPlugin_->Stop();
    }
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
    MEDIA_LOG_I("State from %{public}s to PREPARED", GetStatusDescription(state_).data());
    state_ = CodecState::PREPARED;
    return ret;
}

Status MediaCodec::Flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM,
                       Status::ERROR_INVALID_STATE, "In invalid state");
    Status ret = Status::OK;
    if (isVideo_) {
        ret = codecAdapter_->Flush() == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    } else {
        // ret = codecPlugin_->Flush();
    }
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
    MEDIA_LOG_I("State from %{public}s to FLUSHED", GetStatusDescription(state_).data());
    state_ = CodecState::FLUSHED;
    return ret;
}

Status MediaCodec::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(state_ != CodecState::UNINITIALIZED, Status::ERROR_INVALID_STATE, "In invalid state");

    Status ret = Status::OK;
    if (isVideo_) {
        ret = codecAdapter_->Reset() == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    } else {
        // ret = codecPlugin_->Reset();
    }
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
    MEDIA_LOG_I("State from %{public}s to UNINITIALIZED", GetStatusDescription(state_).data());
    inputBufferQueueProducer_ = nullptr;
    state_ = INITIALIZED;
    return ret;
}

Status MediaCodec::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    Status ret = Status::OK;
    if (isVideo_) {
        ret = codecAdapter_->Release() == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    } else {
        // ret = codecPlugin_->Release();
    }
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
    MEDIA_LOG_I("State from %{public}s to UNINITIALIZED", GetStatusDescription(state_).data());
    inputBufferQueueProducer_ = nullptr;
    state_ = CodecState::UNINITIALIZED;
    return ret;
}

Status MediaCodec::SurfaceModeReturnBuffer(std::shared_ptr<AVBuffer> &buffer, bool available)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(isVideo_, Status::ERROR_INVALID_OPERATION, "Audio codec not supported");
    FALSE_RETURN_V_MSG(state_ == CodecState::RUNNING, Status::ERROR_INVALID_STATE, "In invalid state");

    if (available) {
        int32_t ret = codecAdapter_->RenderOutputBuffer(STATIC_CAST_CBADAPTER->FindOutputIndex(buffer));
        FALSE_RETURN_V_MSG(ret == 0, Status::ERROR_INVALID_OPERATION, "The operation failed");
    } else {
        int32_t ret = codecAdapter_->ReleaseOutputBuffer(STATIC_CAST_CBADAPTER->FindOutputIndex(buffer));
        FALSE_RETURN_V_MSG(ret == 0, Status::ERROR_INVALID_OPERATION, "The operation failed");
    }

    return Status::OK;
}

Status MediaCodec::NotifyEOS()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(isVideo_, Status::ERROR_INVALID_OPERATION, "Audio codec not supported");
    FALSE_RETURN_V_MSG(state_ == CodecState::RUNNING, Status::ERROR_INVALID_STATE, "In invalid state");

    int32_t ret = codecAdapter_->NotifyEos();
    FALSE_RETURN_V_MSG(ret == 0, Status::ERROR_INVALID_OPERATION, "The operation failed");

    MEDIA_LOG_I("State from %{public}s to END_OF_STREAM", GetStatusDescription(state_).data());
    state_ = CodecState::END_OF_STREAM;
    return Status::OK;
}

Status MediaCodec::SetParameter(const std::shared_ptr<MediaAVCodec::Format> &parameter)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(state_ != CodecState::UNINITIALIZED && state_ != CodecState::INITIALIZED &&
                           state_ != CodecState::PREPARED,
                       Status::ERROR_INVALID_STATE, "In invalid state");
    Status ret = Status::OK;
    if (isVideo_) {
        Format format;
        // format.SetMeta(*parameter);
        ret = codecAdapter_->SetParameter(format) == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    } else {
        // ret = codecPlugin_->SetParameter(parameter);
    }
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
    return ret;
}

std::shared_ptr<MediaAVCodec::Format> MediaCodec::GetOutputFormat()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG(state_ != CodecState::UNINITIALIZED, nullptr, "In invalid state");
    std::shared_ptr<MediaAVCodec::Format> outputFormat = nullptr;
    if (isVideo_) {
        Format format;
        codecAdapter_->GetOutputFormat(format);
        // outputFormat = format.GetMeta();
    } else {
        outputFormat = std::make_shared<MediaAVCodec::Format>();
        // codecPlugin_->GetParameter(outputFormat);
    }
    return outputFormat;
}

Status MediaCodec::PrepareInputBufferQueue()
{
    Status ret = Status::OK;
    std::vector<std::shared_ptr<AVBuffer>> inputBuffers;
    // ret = codecPlugin_->GetInputBuffers(inputBuffers);
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
    if (inputBuffers.size() == 0) {
        int inputBufferNum = DEFAULT_BUFFER_NUM;
        inputBufferQueue_ = AVBufferQueue::Create(inputBufferNum, MemoryType::SHARED_MEMORY, INPUT_BUFFER_QUEUE_NAME);
        inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
        std::shared_ptr<MediaAVCodec::Format> inputBufferConfig = std::make_shared<MediaAVCodec::Format>();
        // ret = codecPlugin_->GetParameter(inputBufferConfig);
        FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
        int32_t capacity = 0;
        // inputBufferConfig->GetData("max_input_size", capacity);
        for (int i = 0; i < inputBufferNum; i++) {
            std::shared_ptr<AVAllocator> avAllocator =
                AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
            inputBufferQueueProducer_->AttachBuffer(inputBuffer, false);
        }
    } else {
        inputBufferQueue_ =
            AVBufferQueue::Create(inputBuffers.size(), MemoryType::HARDWARE_MEMORY, INPUT_BUFFER_QUEUE_NAME);
        inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
        for (auto &buffer : inputBuffers) {
            inputBufferQueueProducer_->AttachBuffer(buffer, false);
        }
    }
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    sptr<IConsumerListener> csListener = new CodecConsumerListener(shared_from_this());
    inputBufferQueueConsumer_->SetBufferAvailableListener(csListener);
    return ret;
}

Status MediaCodec::PrepareInputBufferQueueAdapter()
{
    inputBufferQueue_ = AVBufferQueue::Create(DEFAULT_BUFFER_NUM, MemoryType::UNKNOWN_MEMORY, INPUT_BUFFER_QUEUE_NAME);
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();

    sptr<IConsumerListener> csListener = new CodecConsumerListener(shared_from_this());
    inputBufferQueueConsumer_->SetBufferAvailableListener(csListener);

    STATIC_CAST_CBADAPTER->SetInputBufferQueue(inputBufferQueueConsumer_);
    return Status::OK;
}

Status MediaCodec::PrepareOutputBufferQueue()
{
    Status ret = Status::OK;
    std::vector<std::shared_ptr<AVBuffer>> outputBuffers;
    // ret = codecPlugin_->GetOutputBuffers(outputBuffers);
    FALSE_RETURN_V_MSG(ret == Status::OK, Status::ERROR_INVALID_STATE, "The operation failed");
    if (outputBuffers.size() == 0) {
        int32_t outputBufferNum = DEFAULT_BUFFER_NUM;
        std::shared_ptr<MediaAVCodec::Format> outputBufferConfig = std::make_shared<MediaAVCodec::Format>();
        // ret = codecPlugin_->GetParameter(outputBufferConfig);
        FALSE_RETURN_V_MSG(ret == Status::OK, Status::ERROR_INVALID_STATE, "The operation failed");
        int32_t capacity = 0;
        // ret = outputBufferConfig->GetData("max_output_size", capacity) ? Status::OK : Status::ERROR_UNKNOWN;
        for (int32_t i = 0; i < outputBufferNum; ++i) {
            std::shared_ptr<AVAllocator> avAllocator =
                AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            std::shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
            outputBufferQueueProducer_->AttachBuffer(outputBuffer, true);
        }
    } else {
        for (auto &buffer : outputBuffers) {
            outputBufferQueueProducer_->AttachBuffer(buffer, true);
        }
    }

    sptr<IProducerListener> pdListener = new CodecProducerListener(shared_from_this());
    outputBufferQueueProducer_->SetBufferAvailableListener(pdListener);
    return ret;
}

Status MediaCodec::CheckIsEncoder(const std::string &name)
{
    if (isVideoEncoderChecked_ || !isVideo_) {
        return Status::OK;
    }
    std::vector<CapabilityData> caps;
    // int32_t ret = GetCapabilityList(caps);
    // FALSE_RETURN_V_MSG(ret == 0, Status::ERROR_UNKNOWN, "The operation failed");
    for (auto &val : caps) {
        if (name == val.codecName) {
            isVideoEncoder_ = val.codecType == AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER;
            return Status::OK;
        }
    }
    return Status::ERROR_INVALID_PARAMETER;
}

void MediaCodec::ProcessInputBuffer()
{
    Status ret = Status::OK;
    std::shared_ptr<AVBuffer> filledInputBuffer;
    ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
    FALSE_RETURN_MSG(ret == Status::OK, "The operation failed");
    if (state_ != CodecState::RUNNING) {
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        return;
    }
    if (isVideo_) {
        uint32_t index = STATIC_CAST_CBADAPTER->FindInputIndex(filledInputBuffer);
        FALSE_RETURN_MSG(index != INVAILD_INDEX, "Get invalid buffer");
        int32_t adapterRet = codecAdapter_->QueueInputBuffer(index, filledInputBuffer);
        FALSE_RETURN_MSG(adapterRet == 0, "The operation failed");
        // static int32_t frameCount = 0;
        // std::cout << "push input buffer id = " << std::hex << (filledInputBuffer->GetUniqueId() >> 48) << "\n"
        //           << "push input buffer frameCount = " << std::dec << frameCount++ << ", index = " << index <<
        //           "\n\n";
    } else {
        // ret = codecPlugin_->QueueInputBuffer(filledInputBuffer);
    }
    if (ret != Status::OK) {
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        return;
    }
}

void MediaCodec::ProcessOutputBuffer()
{
    std::shared_ptr<AVBuffer> emptyOutputBuffer = nullptr;
    AVBufferConfig avBufferConfig;
    avBufferConfig.size = 1;
    avBufferConfig.memoryFlag = MemoryFlag::MEMORY_READ_WRITE;
    Status ret = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
    FALSE_RETURN_MSG(ret == Status::OK, "The operation failed");
    if (isVideo_) {
        uint32_t index = STATIC_CAST_CBADAPTER->FindOutputIndex(emptyOutputBuffer);
        FALSE_RETURN_MSG(index != INVAILD_INDEX, "Get invalid buffer");
        ret = codecAdapter_->ReleaseOutputBuffer(index) == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    } else {
        // ret = codecPlugin_->QueueOutputBuffer(emptyOutputBuffer);
    }
    if (ret == Status::ERROR_NO_MEMORY) {
        MEDIA_LOG_I("Not enough data");
    }
    if (ret != Status::OK) {
        outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, false);
        return;
    }
}

void MediaCodec::OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    inputBufferQueueConsumer_->ReleaseBuffer(inputBuffer);
}

void MediaCodec::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer)
{
    outputBufferQueueProducer_->PushBuffer(outputBuffer, false);
}

void MediaCodec::OnEvent(const std::shared_ptr<PluginEvent> event)
{
    (void)event;
}

const std::string &MediaCodec::GetStatusDescription(const CodecState &state)
{
    static const std::string ILLEGAL_STATE = "CODEC_STATE_ILLEGAL";
    auto iter = STATE_STR_MAP.find(state);
    if (iter == STATE_STR_MAP.end()) {
        return ILLEGAL_STATE;
    }
    return iter->second;
}
} // namespace Media
} // namespace OHOS
