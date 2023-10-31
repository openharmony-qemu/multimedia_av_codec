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
#include "common/log.h"
#include "codeclist_core.h"
#include "codeclist_utils.h"
#include <shared_mutex>


#ifndef CLIENT_SUPPORT_CODEC
#include "fcodec.h"
#include "hcodec_loader.h"
#endif

#define INPUT_BUFFER_QUEUE_NAME "MediaCodecInputBufferQueue"
#define DEFAULT_BUFFER_NUM 8
#define TIME_OUT_MS 8

namespace {
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

class VideoCodecCallback : public AVCodecCallbackAdapter {
public:
    VideoCodecCallback() : codecCallback_(nullptr) {}
    ~VideoCodecCallback() override = default;

    using BufferObj = struct BufferObj {
        std::shared_ptr<AVBuffer> buffer = nullptr;
        uint32_t index = UINT32_MAX;
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
        std::unique_lock<std::shared_mutex> lock(outputMutex_);
        for (auto &val : outputMap_) {
            outputBufferQueueProducer_->DetachBuffer(val.second.buffer);
        }
        Format formatNotConst = format;
        codecCallback_->OnOutputFormatChanged(formatNotConst.GetMeta());
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        FALSE_RETURN_MSG(outputBufferQueueProducer_ != nullptr, "outputBufferQueueProducer_ is nullptr");
        std::unique_lock<std::shared_mutex> lock(inputMutex_);
        uint64_t uniqueId = buffer->GetUniqueId();
        auto iter = outputMap_.find(uniqueId);
        if (iter == outputMap_.end()) {
            outputMap_[uniqueId].index = index;
            outputMap_[uniqueId].buffer = buffer;
            outputBufferQueueProducer_->AttachBuffer(buffer);
            return;
        }
        iter->second.index = index;
        outputBufferQueueProducer_->PushBuffer(buffer, false);
        return;
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        FALSE_RETURN_MSG(inputBufferQueueConsumer_ != nullptr, "inputBufferQueueConsumer_ is nullptr");
        std::unique_lock<std::shared_mutex> lock(outputMutex_);
        uint64_t uniqueId = buffer->GetUniqueId();
        auto iter = inputMap_.find(uniqueId);
        if (iter == inputMap_.end()) {
            inputMap_[uniqueId].index = index;
            inputMap_[uniqueId].buffer = buffer;
            inputBufferQueueConsumer_->AttachBuffer(buffer);
            return;
        }
        iter->second.index = index;
        inputBufferQueueConsumer_->ReleaseBuffer(buffer);
        return;
    }

    void SetCallback(std::shared_ptr<CodecCallback> &codecCallback)
    {
        FALSE_RETURN_MSG(codecCallback != nullptr, "codecCallback is nullptr");
        std::unique_lock<std::shared_mutex> lock(outputMutex_);
        codecCallback_ = codecCallback;
    }

    void SetInputBufferQueue(std::shared_ptr<AVBufferQueueConsumer> inputBufferQueueConsumer)
    {
        FALSE_RETURN_MSG(inputBufferQueueConsumer != nullptr, "inputBufferQueueConsumer is nullptr");
        std::unique_lock<std::shared_mutex> lock(inputMutex_);
        inputBufferQueueConsumer_ = inputBufferQueueConsumer;
    }

    void SetOutputBufferQueue(std::shared_ptr<AVBufferQueueProducer> outputBufferQueueProducer)
    {
        FALSE_RETURN_MSG(outputBufferQueueProducer != nullptr, "inputBufferQueueConsumer is nullptr");
        std::unique_lock<std::shared_mutex> lock(outputMutex_);
        outputBufferQueueProducer_ = outputBufferQueueProducer;
    }

    uint32_t FindInputIndex(const std::shared_ptr<AVBuffer> &buffer)
    {
        std::shared_lock<std::shared_mutex> lock(inputMutex_);
        auto iter = inputMap_.find(buffer->GetUniqueId());
        if (iter != inputMap_.end()) {
            return iter->second.index;
        }
        return UINT32_MAX;
    }

    uint32_t FindOutputIndex(const std::shared_ptr<AVBuffer> &buffer)
    {
        std::shared_lock<std::shared_mutex> lock(outputMutex_);
        auto iter = outputMap_.find(buffer->GetUniqueId());
        if (iter != outputMap_.end()) {
            return iter->second.index;
        }
        return UINT32_MAX;
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
    std::shared_ptr<AVBufferQueueConsumer> inputBufferQueueConsumer_ = nullptr;
    std::shared_ptr<AVBufferQueueProducer> outputBufferQueueProducer_ = nullptr;
    std::unordered_map<uint64_t, BufferObj> inputMap_;
    std::unordered_map<uint64_t, BufferObj> outputMap_;
    std::shared_mutex inputMutex_;
    std::shared_mutex outputMutex_;
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
    FALSE_RETURN_V_MSG(!codecname.empty(), Status::ERROR_INVALID_OPERATION,
                             "Create codec by mime failed: error mime type");
    return Init(codecname);
}

Status MediaCodec::Init(const std::string &name)
{
    std::shared_ptr<CodecListCore> codecListCore = std::make_shared<CodecListCore>();
    CodecType codecType = codecListCore->FindCodecType(name);
    switch (codecType) {
#ifndef CLIENT_SUPPORT_CODEC
        case CodecType::AVCODEC_HCODEC: {
            isVideo_ = true;
            codecAdapter_ = HCodecLoader::CreateByName(name);
            break;
        }
        case CodecType::AVCODEC_VIDEO_CODEC: {
            isVideo_ = true;
            codecAdapter_ = std::make_shared<Codec::FCodec>(name);
            break;
        }
#endif
        case CodecType::AVCODEC_AUDIO_CODEC: {
            isVideo_ = false;
            // codecPlugin_ = std::make_shared<CodecPlugin>();
            break;
        }
        default: {
            AVCODEC_LOGE("Create codec %{public}s failed", name.c_str());
            return Status::ERROR_UNSUPPORTED_FORMAT;
        }
    }
    MEDIA_LOG_I("Create codec %{public}s successful", name.c_str());
    MEDIA_LOG_I("State from %{public}s to INITIALIZED", GetStatusDescription(state_).data());
    state_ = CodecState::INITIALIZED;
    return Status::OK;
}

Status MediaCodec::Configure(const std::shared_ptr<Meta> &meta)
{
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE,
                             "In invalid state");
    if (isVideo_) {
        Format format;
        format.SetMeta(*meta);
        codecAdapter_->SetParameter(format);
    } else {
        // codecPlugin_->SetParameter(meta);
    }
    MEDIA_LOG_I("State from %{public}s to CONFIGURED", GetStatusDescription(state_).data());
    state_ = CodecState::CONFIGURED;
    return Status::OK;
}

Status MediaCodec::SetCodecCallback(std::shared_ptr<CodecCallback> &codecCallback)
{
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE,
                             "In invalid state");
    Status ret = Status::OK;
    codecCallback_ = codecCallback;
    if (isVideo_) {
        if (cbAdapter_ == nullptr) {
            cbAdapter_ = std::make_shared<VideoCodecCallback>();
            ret = codecAdapter_->SetCallback(cbAdapter_) == 0 ? Status::OK : Status::ERROR_UNKNOWN;
            FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
        }
        std::static_pointer_cast<VideoCodecCallback>(cbAdapter_)->SetCallback(codecCallback_);
    } else {
        // ret = codecPlugin_->SetDataCallback(this);
    }
    return ret;
}

Status MediaCodec::SetOutputBufferQueue(std::shared_ptr<AVBufferQueueProducer> &bufferQueueProducer)
{
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE,
                             "In invalid state");
    FALSE_RETURN_V_MSG(!isSurfaceMode_, Status::ERROR_INVALID_STATE, "In surface mode");
    outputBufferQueueProducer_ = bufferQueueProducer;
    isBufferMode_ = true;
    return Status::OK;
}

Status MediaCodec::SetOutputSurface(sptr<Surface> &surface)
{
    FALSE_RETURN_V_MSG(isVideo_, Status::ERROR_INVALID_OPERATION, "Audio codec not supported");
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE,
                             "In invalid state");
    FALSE_RETURN_V_MSG(!isBufferMode_, Status::ERROR_INVALID_STATE, "In buffer mode");

    isSurfaceMode_ = true;
    return codecAdapter_->SetOutputSurface(surface) == 0 ? Status::OK : Status::ERROR_UNKNOWN;
}

sptr<Surface> MediaCodec::GetInputSurface()
{
    FALSE_RETURN_V_MSG(isVideo_, nullptr, "Audio codec not supported");
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, nullptr, "In invalid state");
    FALSE_RETURN_V_MSG(!isBufferMode_, nullptr, "In buffer mode");

    isSurfaceMode_ = true;
    return codecAdapter_->CreateInputSurface();
}

std::shared_ptr<AVBufferQueueProducer> MediaCodec::GetInputBufferQueue()
{
    FALSE_RETURN_V_MSG(state_ == CodecState::PREPARED || state_ == CodecState::RUNNING ||
                                 state_ == CodecState::FLUSHED || state_ == CodecState::END_OF_STREAM,
                             nullptr, "In invalid state");
    FALSE_RETURN_V_MSG(!isSurfaceMode_, nullptr, "In surface mode");

    isBufferMode_ = true;
    return inputBufferQueueProducer_;
}

Status MediaCodec::Prepare()
{
    FALSE_RETURN_V_MSG(state_ == CodecState::INITIALIZED, Status::ERROR_INVALID_STATE, "In invalid state");
    FALSE_RETURN_V_MSG(!isBufferMode_ || !isSurfaceMode_, Status::ERROR_UNKNOWN,
                             "Cannot handle both modes at the same time");

    Status ret = PrepareInputBufferQueue();
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");

    ret = PrepareOutputBufferQueue();
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");

    FALSE_RETURN_V_MSG(codecCallback_ != nullptr, Status::ERROR_INVALID_OPERATION, "not set callback");
    if (isVideo_) {
        FALSE_RETURN_V_MSG(cbAdapter_ != nullptr, Status::ERROR_INVALID_OPERATION, "The operation failed");
        std::static_pointer_cast<VideoCodecCallback>(cbAdapter_)->SetOutputBufferQueue(outputBufferQueueProducer_);
        ret = codecAdapter_->Prepare() == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    }

    MEDIA_LOG_I("State from %{public}s to PREPARED", GetStatusDescription(state_).data());
    state_ = CodecState::PREPARED;
    return ret;
}

Status MediaCodec::Start()
{
    FALSE_RETURN_V_MSG((state_ == CodecState::PREPARED) && inputBufferQueueProducer_ != nullptr, Status::ERROR_INVALID_STATE,
                             "In invalid state");
    Status ret = Status::OK;
    if (isVideo_) {
        ret = codecAdapter_->Start() == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    } else {
        // ret = codecPlugin_->Start();
    }
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
    MEDIA_LOG_I("State from %{public}s to STARTED", GetStatusDescription(state_).data());
    state_ = CodecState::RUNNING;
    return ret;
}

Status MediaCodec::Stop()
{
    FALSE_RETURN_V_MSG(state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM,
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

Status MediaCodec::NotifyEOS()
{
    FALSE_RETURN_V_MSG(isVideo_, Status::ERROR_INVALID_OPERATION, "Audio codec not supported");
    FALSE_RETURN_V_MSG(state_ == CodecState::RUNNING, Status::ERROR_INVALID_STATE, "In invalid state");

    int32_t ret = codecAdapter_->NotifyEos();
    FALSE_RETURN_V_MSG(ret == 0, Status::ERROR_INVALID_OPERATION, "The operation failed");

    MEDIA_LOG_I("State from %{public}s to END_OF_STREAM", GetStatusDescription(state_).data());
    state_ = CodecState::END_OF_STREAM;
    return Status::OK;
}

Status MediaCodec::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    FALSE_RETURN_V_MSG(state_ != CodecState::UNINITIALIZED && state_ != CodecState::INITIALIZED &&
                                 state_ != CodecState::PREPARED,
                             Status::ERROR_INVALID_STATE, "In invalid state");
    Status ret = Status::OK;
    if (isVideo_) {
        Format format;
        format.SetMeta(*parameter);
        ret = codecAdapter_->SetParameter(format) == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    } else {
        // ret = codecPlugin_->SetParameter(parameter);
    }
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
    return ret;
}

std::shared_ptr<Meta> MediaCodec::GetOutputFormat()
{
    FALSE_RETURN_V_MSG(state_ != CodecState::UNINITIALIZED, nullptr, "In invalid state");
    std::shared_ptr<Meta> outputFormat = nullptr;
    if (isVideo_) {
        Format format;
        codecAdapter_->GetOutputFormat(format);
        outputFormat = format.GetMeta();
    } else {
        outputFormat = std::make_shared<Meta>();
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
        std::shared_ptr<Meta> inputBufferConfig = std::make_shared<Meta>();
        // ret = codecPlugin_->GetParameter(inputBufferConfig);
        FALSE_RETURN_V_MSG(ret == Status::OK, ret, "The operation failed");
        int32_t capacity = 0;
        inputBufferConfig->GetData("max_input_size", capacity);
        for (int i = 0; i < inputBufferNum; i++) {
            std::shared_ptr<AVAllocator> avAllocator =
                AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
            inputBufferQueueProducer_->AttachBuffer(inputBuffer);
        }
    } else {
        inputBufferQueue_ =
            AVBufferQueue::Create(inputBuffers.size(), MemoryType::HARDWARE_MEMORY, INPUT_BUFFER_QUEUE_NAME);
        inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
        for (auto &buffer : inputBuffers) {
            inputBufferQueueProducer_->AttachBuffer(buffer);
        }
    }
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    std::shared_ptr<IAVBufferAvailableListener> listener = std::shared_ptr<IAVBufferAvailableListener>(this);
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    std::static_pointer_cast<VideoCodecCallback>(cbAdapter_)->SetInputBufferQueue(inputBufferQueueConsumer_);
    return ret;
}

Status MediaCodec::PrepareOutputBufferQueue()
{
    Status ret = Status::OK;
    std::vector<std::shared_ptr<AVBuffer>> outputBuffers;
    // ret = codecPlugin_->GetOutputBuffers(outputBuffers);
    FALSE_RETURN_V_MSG(ret == Status::OK, Status::ERROR_INVALID_STATE, "The operation failed");
    if (outputBuffers.size() == 0) {
        int32_t outputBufferNum = DEFAULT_BUFFER_NUM;
        std::shared_ptr<Meta> outputBufferConfig = std::make_shared<Meta>();
        // ret = codecPlugin_->GetParameter(outputBufferConfig);
        FALSE_RETURN_V_MSG(ret == Status::OK, Status::ERROR_INVALID_STATE, "The operation failed");
        int32_t capacity = 0;
        ret = outputBufferConfig->GetData("max_output_size", capacity) ? Status::OK : Status::ERROR_UNKNOWN;
        for (int32_t i = 0; i < outputBufferNum; ++i) {
            std::shared_ptr<AVAllocator> avAllocator =
                AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            std::shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
            outputBufferQueueProducer_->AttachBuffer(outputBuffer);
        }
    } else {
        for (auto &buffer : outputBuffers) {
            outputBufferQueueProducer_->AttachBuffer(buffer);
        }
    }
    return ret;
}

void MediaCodec::ProcessInputBuffer()
{
    Status ret = Status::OK;
    std::shared_ptr<AVBuffer> filledInputBuffer;
    ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer) == 0 ? Status::OK : Status::ERROR_UNKNOWN;
    FALSE_RETURN_MSG(ret == Status::OK, "The operation failed");
    if (state_ != CodecState::RUNNING) {
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        return;
    }
    // ret = codecPlugin_->QueueInputBuffer(filledInputBuffer);
    if (ret != Status::OK) {
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        return;
    }
    std::shared_ptr<AVBuffer> emptyOutputBuffer;
    while (true) {
        emptyOutputBuffer = nullptr;
        AVBufferConfig avBufferConfig;
        ret = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS) == 0
                  ? Status::OK
                  : Status::ERROR_UNKNOWN;
        filledInputBuffer = nullptr;
        ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer) == 0 ? Status::OK : Status::ERROR_UNKNOWN;
        FALSE_RETURN_MSG(ret == Status::OK, "The operation failed");
        // ret = codecPlugin_->QueueOutputBuffer(emptyOutputBuffer);
        CHECK_AND_CONTINUE_LOG(ret == Status::ERROR_NO_MEMORY, "Not enough data");
        if (ret != Status::OK) {
            outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, true);
            return;
        }
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

void MediaCodec::OnBufferAvailable(std::shared_ptr<AVBufferQueue> &outBuffer)
{
    (void)outBuffer;
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
