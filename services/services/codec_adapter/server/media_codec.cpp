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
#include "audio_codec_adapter.h"
#include "codecbase_adapter.h"
#include "codeclist_core.h"
#include "codeclist_utils.h"
#include "fcodec.h"
#include "hcodec_loader.h"

#define INPUT_BUFFER_QUEUE_NAME "MediaCodecInputBufferQueue"

namespace OHOS {
namespace Media {

class VideoCodecCallback : public AVCodecCallbackAdapter {
public:
    VideoCodecCallback(CodecCallback *codecCallback) : codecCallback_(codecCallback);
    ~VideoCodecCallback() override = default;

    using BufferObj = struct {
        std::shared_ptr<AVBuffer> buffer = nullptr;
        uint32_t index = UINT32_MAX;
    };

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        if (codecCallback_ == nullptr) {
            return;
        }
        codecCallback_->OnError(errorType, errorCode);
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        if (codecCallback_ == nullptr) {
            return;
        }
        codecCallback_->OnOutputFormatChanged(format);
        for (auto &val : outputMap_) {
            outputBufferQueueProducer_.DetachBuffer(val.sencond.buffer);
        }
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        uin64_t uniqueId = buffer->GetUniqueId();
        if (outputMap_.find(uniqueId) == outputMap_.end()) {
            outputMap_[uniqueId].index = index;
            outputBufferQueueProducer_->AttachBuffer(&outputBuffer);
            return;
        }
        outputMap_[uniqueId].index = index;
        outputBufferQueueProducer_->PushBuffer(&outputBuffer);
        return;
    }
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        uin64_t uniqueId = buffer->GetUniqueId();
        if (inputMap_.find(uniqueId) == inputMap_.end()) {
            inputMap_[uniqueId].index = index;
            inputBufferQueueConsumer_->AttachBuffer(&inputBuffer);
            return;
        }
        inputMap_[uniqueId].index = index;
        inputBufferQueueConsumer_->ReleaseBuffer(&inputBuffer);
        return;
    }

    Status SetInputBufferQueue(std::shared_ptr<AVBufferQueueConsumer> inputBufferQueueConsumer)
    {
        if (inputBufferQueueConsumer == nullptr) {
            return Status::ERROR_INVALID_PARAMETER;
        }
        inputBufferQueueConsumer_ = inputBufferQueueConsumer;
        inputBufferNum_ = inputBufferQueueConsumer.size();
        return Status::OK;
    }

    Status SetOutputBufferQueue(std::shared_ptr<AVBufferQueueProducer> outputBufferQueueConsumer)
    {
        if (outputBufferQueueProducer == nullptr) {
            return Status::ERROR_INVALID_PARAMETER;
        }
        outputBufferQueueProducer_ = outputBufferQueueProducer;
        outputBufferNum_ = outputBufferQueueProducer.size();
        return Status::OK;
    }

    uint32_t FindInputIndex(const std::shared_ptr<AVBuffer> &buffer)
    {
        uin64_t uniqueId = buffer->GetUniqueId();
        if (inputMap_.find(uniqueId) != inputMap_.end()) {
            return inputMap_[uniqueId].index;
        }
        return UINT32_MAX;
    }

    uint32_t FindOutputIndex(const std::shared_ptr<AVBuffer> &buffer)
    {
        uin64_t uniqueId = buffer->GetUniqueId();
        if (outputMap_.find(uniqueId) != outputMap_.end()) {
            return outputMap_[uniqueId].index;
        }
        return UINT32_MAX;
    }

    void ClearMap()
    {
        for (auto &val : inputMap_) {
            inputBufferQueueConsumer_.DetachBuffer(val.sencond.buffer);
        }
        for (auto &val : outputMap_) {
            outputBufferQueueProducer_.DetachBuffer(val.sencond.buffer);
        }
        outputMap_.clear();
        inputMap_.clear();
        // TODO, bufferQueue Clear;
    }

private:
    CodecCallback *codecCallback_ = nullptr;
    std::shared_ptr<AVBufferQueueConsumer> inputBufferQueueConsumer_ = nullptr;
    std::shared_ptr<AVBufferQueueProducer> outputBufferQueueProducer_ = nullptr;
    std::unordered_map<uint64_t, BufferObj> inputMap_;
    std::unordered_map<uint64_t, BufferObj> outputMap_;
    int32_t inputBufferNum_;
    int32_t outputBufferNum_;
};

Status MediaCodec::Init(const std::string &mime, bool isEncoder)
{
    if (state_ != CodecState::UNINITIALIZED) {
        return Status::INVALID_STATE;
    }

    std::shared_ptr<CodecListCore> codecListCore = std::make_shared<CodecListCore>();
    std::string codecname;
    Format format;
    format.PutStringValue("codec_mime", mime);
    if (isEncoder) {
        codecname = codecListCore->FindEncoder(format);
    } else {
        codecname = codecListCore->FindDecoder(format);
    }
    CHECK_AND_RETURN_RET_LOG(!codecname.empty(), Status::ERROR_UNSUPPORTED_FORMAT,
                             "Create codec by mime failed: error mime type");
    Status ret = Init(codecname);
    AVCODEC_LOGI("Create codec by mime is successful");

    state_ = CodecState::INITIALIZED;
    return Status::OK;
}

Status MediaCodec::Init(const std::string &name)
{
    if (state_ != CodecState::UNINITIALIZED) {
        return Status::INVALID_STATE;
    }

    std::shared_ptr<CodecListCore> codecListCore = std::make_shared<CodecListCore>();
    CodecType codecType = codecListCore->FindCodecType(name);
    switch (codecType) {
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
        case CodecType::AVCODEC_AUDIO_CODEC: {
            isVideo_ = false;
            codecPlugin_ = std::make_shared<CodecPlugin>();
            break;
        }
        default: {
            AVCODEC_LOGE("Create codec %{public}s failed", name.c_str());
            return codec;
        }
    }
    AVCODEC_LOGI("Create codec %{public}s successful", name.c_str());

    state_ = CodecState::INITIALIZED;
    return Status::OK;
}

Status MediaCodec::Configure(const std::shared_ptr<Meta> meta)
{
    if (state_ != CodecState::INITIALIZED && state_ != CodecState::CONFIGURING) {
        return Status::IVALID_STATE;
    }
    if (isVideo_) {
        codecAdapter_->SetParameter(meta);
    } else {
        codecPlugin_->SetParameter(meta);
    }
    state_ = CodecState::CONFIGURING;
    return Status::OK;
}

Status MediaCodec::SetOutputBufferQueue(std::shared_ptr<AVBufferQueueProducer> bufferQueueProducer)
{
    if (state_ != CodecState::INITIALIZED && state_ != CodecState::CONFIGURING) {
        return Status::IVALID_STATE;
    }
    outputBufferQueueProducer_ = bufferQueueProducer;
    isBufferMode_ = true;
    state_ = CodecState::CONFIGURING;
    return Status::OK;
}

Status MediaCodec::SetCodecCallback(CodecCallback *codecCallback)
{
    if (state_ != CodecState::INITIALIZED && state_ != CodecState::CONFIGURING) {
        return Status::IVALID_STATE;
    }
    Status ret = Status::OK;
    if (isVideo_) {
        callbackAdapter_ = std::make_shared<VideoCodecCallback>(codecCallback);
        ret = codecAdapter_->SetCallback(callbackAdapter_);
    } else {
        codecCallback_ = codecCallback;
        codecPlugin_->SetDataCallback(this);
    }
    state_ = CodecState::CONFIGURING;
    return Status::OK;
}

Status MediaCodec::SetOutputSurface(sptr<Surface> surface)
{
    if (state_ != CodecState::INITIALIZED && state_ != CodecState::CONFIGURING) {
        return Status::IVALID_STATE;
    }
    state_ = CodecState::CONFIGURING;
    Status ret = Status::OK;
    isSurfaceMode_ = true;
    if (isVideo_) {
        ret = codecAdapter_->SetOutputSurface(surface);
    }
    return ret;
}

Status MediaCodec::Prepare()
{
    if (state_ != CodecState::CONFIGURING) {
        return Status::IVALID_STATE;
    }
    if (isBufferMode_ && isSurfaceMode_) {
        return Status::ERROR;
    }
    Status ret = Status::OK;
    ret = PrepareInputBufferQueue();
    if (ret != Status::OK) {
        return ret;
    }
    ret = PrepareOutputBufferQueue();
    if (ret != Status::OK) {
        return ret;
    }
    state_ = CodecState::CONFIGURED;
    return ret;
}

std::shared_ptr<AVBufferQueueProducer> MediaCodec::GetInputBufferQueue()
{
    if (state_ != CodecState::CONFIGURED) {
        return nullptr;
    }
    if (isSurfaceMode_) {
        return nullptr;
    }
    isBufferMode_ = true;
    return inputBufferQueueProducer_;
}

sptr<Surface> MediaCodec::GetInputSurface()
{
    if (state_ != CodecState::CONFIGURED) {
        return nullptr;
    }
    if (isBufferMode_) {
        return nullptr;
    }
    isSurfaceMode_ = true;
    if (isVideo_) {
        return codecAdapter_->CreateInputSurface();
    }
    return nullptr;
}

Status MediaCodec::Start()
{
    if (state_ != CodecState::CONFIGURED && state_ != CodecState::FLUSHED) {
        return Status::INVALID_STATE;
    }
    Status ret = Status::OK;
    state_ = CodecState::STARTING;
    if (isVideo_) {
        ret = codecAdapter_->Start();
    } else {
        ret = codecPlugin_->Start();
    }
    if (ret != Status::OK) {
        return ret;
    }
    state_ = CodecState::STARTED;
    return ret;
}

Status MediaCodec::Stop()
{
    if (state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURING || state_ == CodecState::CONFIGURED ||
        state_ == CodecState::STOPPING) {
        return Status::OK;
    }
    Status ret = Status::OK;
    state_ = CodecState::STOPPING;
    if (isVideo_) {
        ret = codecAdapter_->Stop();
    } else {
        ret = codecPlugin_->Stop();
    }
    if (ret != Status::OK) {
        return ret;
    }
    state_ = CodecState::CONFIGURED;
    return ret;
}

Status MediaCodec::Flush()
{
    if (state_ == CodecState::FLUSHED) {
        return Status::OK;
    }
    if (state_ != CodecState::STARTING) {
        return Status::INVALID_STATE;
    }
    Status ret = Status::OK;
    state_ = CodecState::FLUSHING;
    if (isVideo_) {
        ret = codecAdapter_->Flush();
    } else {
        ret = codecPlugin_->Flush();
    }
    if (ret != Status::OK) {
        return ret;
    }
    state_ = CodecState::FLUSHED;
    return ret;
}

Status MediaCodec::Reset()
{
    Status ret = Status::OK;
    if (state_ == CodecState::RELEASING || state_ == CodecState::UNINITIALIZED) {
        return Status::OK;
    }
    if (state_ == CodecState::CONFIGURING || state_ == CodecState::CONFIGURED) {
        state_ = CodecState::UNINITIALIZED;
        return Status::OK;
    }
    if (isVideo_) {
        ret = codecAdapter_->Reset();
    } else {
        ret = codecPlugin_->Reset();
    }
    if (ret != Status::OK) {
        return ret;
    }
    state_ = CodecState::UNINITIALIZED;
    return ret;
}

Status MediaCodec::Release()
{
    Status ret = Status::OK;
    if (state_ == CodecState::RELEASING || state_ == CodecState::UNINITIALIZED) {
        return Status::OK;
    }
    if (state_ == CodecState::CONFIGURING || state_ == CodecState::CONFIGURED) {
        state_ = CodecState::UNINITIALIZED;
        return Status::OK;
    }
    state_ = CodecState::RELEASING;
    if (isVideo_) {
        ret = codecAdapter_->Release();
    } else {
        ret = codecPlugin_->Release();
    }
    if (ret != Status::OK) {
        return ret;
    }
    state_ = CodecState::UNINITIALIZED;
    return ret;
}

Status MediaCodec::NotifyEOS()
{
    Status ret = Status::OK;
    if (isVideo_) {
        ret = codecAdapter_->NotifyEOS();
    }
    return ret;
}

Status MediaCodec::SetParameter(const std::shared_ptr<Meta> parameter)
{
    Status ret = Status::OK;
    if (isVideo_) {
        ret = codecAdapter_->SetParameter(parameter);
    } else {
        ret = codecPlugin_->SetParameter(parameter);
    }
    return ret;
}

std::shared_ptr<Meta> MediaCodec::GetOutputFormat()
{
    std::shared_ptr<Meta> outputFormat = std::make_shared<Meta>();
    if (isVideo_) {
        codecAdapter_->GetOutputFormat(outputFormat);
    } else {
        codecPlugin_->GetParameter(outputFormat);
    }
    return outputFormat;
}

Status MediaCodec::PrepareInputBufferQueue()
{
    Status ret = Status::OK;
    std::vector<std::shared_ptr<AVBuffer>> inputBuffers;
    ret = codecPlugin_->GetInputBuffers(&inputBuffers);
    if (ret != Status::OK) {
        return ret;
    }
    if (inputBuffers.size() == 0) {
        int inputBufferSize = 5;
        inputBufferQueue_ = AVBufferQueue::Create(INPUT_BUFFER_QUEUE_NAME, inputBufferSize);
        Meta inputBufferConfig = std::make_shared<Meta>();
        ret = codecPlugin_->GetParameter(inputBufferConfig);
        int32_t capacity = inputBufferConfig.get("bufferSize");
        if (ret != Status::OK) {
            return ret;
        }
        for (int i = 0; i < inputBufferSize; i++) {
            std::shared_ptr<AVAllocator> avAllocator =
                AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
            inputBufferQueue_->AttachBuffer(inputBuffer);
        }
    } else {
        inputBufferQueue_ = AVBufferQueue::Create(INPUT_BUFFER_QUEUE_NAME, inputBuffers.size());
        for (int i = 0; i < inputBuffers.size(); i++) {
            inputBufferQueue_->AttachBuffer(inputBuffers[i]);
        }
    }
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    inputBufferQueueConsumer_->SetConsumerListener([this] { ProcessInputBuffer(); });
    return ret;
}

Status MediaCodec::PrepareOutputBufferQueue()
{
    Status ret = Status::OK;
    std::vector<std::shared_ptr<AVBuffer>> outputBuffers;
    ret = codecPlugin_->GetOutputBuffers(&outputBuffers);
    if (ret != Status::OK) {
        return ret;
    }
    if (outputBuffers.size() == 0) {
        int outputBufferSize = 5;
        Meta outputBufferConfig = std::make_shared<Meta>();
        ret = codecPlugin_->GetParameter(outputBufferConfig);
        if (ret != Status::OK) {
            return ret;
        }
        for (int i = 0; i < outputBufferSize; i++) {
            std::shared_ptr<AVAllocator> avAllocator =
                AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            std::shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
            outputBufferQueueProducer_->AttachBuffer(outputBuffer);
        }
    } else {
        for (int i = 0; i < outputBuffers.size(); i++) {
            outputBufferQueueProducer_->AttachBuffer(outputBuffers[i]);
        }
    }
    return ret;
}

void MediaCodec::ProcessInputBuffer()
{
    // lock();
    Status ret = Status::OK;
    std::shared_ptr<AVBuffer> filledInputBuffer = std::make_shared<AVBuffer>();
    ret = inputBufferQueueConsumer_->AcquireBuffer(&filledInputBuffer);
    if (ret != Status::OK) {
        return;
    }
    if (state_ != CodecState::STARTED) {
        inputBufferQueueConsumer_->ReleaseBuffer(&filledInputBuffer);
        return;
    }
    if (isVideo_) {
        ret = codecAdapter_->QueueInputBuffer(callbackAdapter_->FindInputIndex(filledInputBuffer), filledInputBuffer);
    } else {
        ret = codecPlugin_->QueueInputBuffer(&filledInputBuffer);
    }
    if (ret != Status::OK) {
        inputBufferQueueConsumer_->ReleaseBuffer(&filledInputBuffer);
        return;
    }
    while (true) {
        std::shared_ptr<AVBuffer> emptyOutputBuffer = std::make_shared<AVBuffer>();
        AVBufferConfig avBufferConfig = new AVBufferConfig();
        ret = outputBufferQueueProducer_->RequestBuffer(&emptyOutputBuffer, &avBufferConfig);
        if (ret != Status::OK) {
            return;
        }
        if (isVideo_) {
            if (isSurfaceMode_) {
                ret = codecAdapter_->RenderOutputBuffer(callbackAdapter_->FindOutputIndex(emptyOutputBuffer));
            } else {
                ret = codecAdapter_->QueueInputBuffer(callbackAdapter_->FindOutputIndex(emptyOutputBuffer),
                                                      emptyOutputBuffer);
            }
        } else {
            ret = codecPlugin_->QueueOutputBuffer(&emptyOutputBuffer);
        }
        if (ret == Status::NOT_ENOUGH_DATA) {
            continue;
        } else if (ret != Status::OK) {
            outputBufferProducer_->CancelBuffer(&emptyOutputBuffer);
            return;
        }
        return;
    }
}

void MediaCodec::OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    inputBufferQueueConsumer_->ReleaseBuffer(&inputBuffer);
}

void MediaCodec::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer)
{
    outputBufferQueueProducer_->PushBuffer(&outputBuffer);
}

void MediaCodec::OnEvent(const std::shared_ptr<PluginEvent> event) {}

} // namespace Media
} // namespace OHOS
