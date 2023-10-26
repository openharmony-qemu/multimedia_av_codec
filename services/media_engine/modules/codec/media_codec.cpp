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
#include <shared_mutex>

#define INPUT_BUFFER_QUEUE_NAME "MediaCodecInputBufferQueue"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaCodec"};
} // namespace
namespace OHOS {
namespace MediaAVCodec {

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
        CHECK_AND_RETURN_LOG(codecCallback_ != nullptr, "codecCallback_ is nullptr");
        codecCallback_->OnError(errorType, errorCode);
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        CHECK_AND_RETURN_LOG(codecCallback_ != nullptr, "codecCallback_ is nullptr");
        std::unique_lock<std::shared_mutex> lock(outputMutex_);
        for (auto &val : outputMap_) {
            outputBufferQueueProducer_.DetachBuffer(val.sencond.buffer);
        }
        codecCallback_->OnOutputFormatChanged(format);
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        CHECK_AND_RETURN_LOG(outputBufferQueueProducer_ != nullptr, "outputBufferQueueProducer_ is nullptr");
        std::unique_lock<std::shared_mutex> lock(inputMutex_);
        uin64_t uniqueId = buffer->GetUniqueId();
        auto iter = outputMap_.find(uniqueId);
        if (iter == outputMap_.end()) {
            outputMap_[uniqueId].index = index;
            outputMap_[uniqueId].buffer = buffer;
            outputBufferQueueProducer_->AttachBuffer(&buffer);
            return;
        }
        iter->second.index = index;
        outputBufferQueueProducer_->PushBuffer(&buffer);
        return;
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        CHECK_AND_RETURN_LOG(inputBufferQueueConsumer_ != nullptr, "inputBufferQueueConsumer_ is nullptr");
        std::unique_lock<std::shared_mutex> lock(outputMutex_);
        uin64_t uniqueId = buffer->GetUniqueId();
        auto iter = inputMap_.find(uniqueId);
        if (iter == inputMap_.end()) {
            inputMap_[uniqueId].index = index;
            inputMap_[uniqueId].buffer = buffer;
            inputBufferQueueConsumer_->AttachBuffer(&buffer);
            return;
        }
        iter->second.index = index;
        inputBufferQueueConsumer_->ReleaseBuffer(&buffer);
        return;
    }

    int32_t SetInputBufferQueue(std::shared_ptr<AVBufferQueueConsumer> inputBufferQueueConsumer)
    {
        CHECK_AND_RETURN_LOG(inputBufferQueueConsumer_ != nullptr, "inputBufferQueueConsumer_ is nullptr");
        inputBufferQueueConsumer_ = inputBufferQueueConsumer;
        inputBufferNum_ = inputBufferQueueConsumer.size();
        return AVCS_ERR_OK;
    }

    int32_t SetOutputBufferQueue(std::shared_ptr<AVBufferQueueProducer> outputBufferQueueConsumer)
    {
        CHECK_AND_RETURN_LOG(outputBufferQueueProducer_ != nullptr, "inputBufferQueueConsumer_ is nullptr");
        outputBufferQueueProducer_ = outputBufferQueueProducer;
        outputBufferNum_ = outputBufferQueueProducer.size();
        return AVCS_ERR_OK;
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
            (void)inputBufferQueueConsumer_.DetachBuffer(val.sencond.buffer);
        }
        inputMap_.clear();

        for (auto &val : outputMap_) {
            (void)outputBufferQueueProducer_.DetachBuffer(val.sencond.buffer);
        }
        outputMap_.clear();
    }

private:
    CodecCallback *codecCallback_ = nullptr;
    std::shared_ptr<AVBufferQueueConsumer> inputBufferQueueConsumer_ = nullptr;
    std::shared_ptr<AVBufferQueueProducer> outputBufferQueueProducer_ = nullptr;
    std::unordered_map<uint64_t, BufferObj> inputMap_;
    std::unordered_map<uint64_t, BufferObj> outputMap_;
    int32_t inputBufferNum_;
    int32_t outputBufferNum_;
    std::shared_mutex_ inputMutex_;
    std::shared_mutex_ outputMutex_;
};

int32_t MediaCodec::Init(const std::string &mime, bool isEncoder)
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
    CHECK_AND_RETURN_RET_LOG(!codecname.empty(), AVCS_ERR_UNSUPPORT, "Create codec by mime failed: error mime type");
    int32_t ret = Init(codecname);
    AVCODEC_LOGI("Create codec by mime is successful");
    state_ = CodecState::INITIALIZED;
    return AVCS_ERR_OK;
}

int32_t MediaCodec::Init(const std::string &name)
{
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
    AVCODEC_LOGI("State from %{public}s to INITIALIZED", GetStatusDescription(state_).data());
    state_ = CodecState::INITIALIZED;
    return AVCS_ERR_OK;
}

int32_t MediaCodec::Configure(const std::shared_ptr<Meta> meta)
{
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURING,
                             AVCS_ERR_INVALID_STATE, "In invalid state");
    if (isVideo_) {
        codecAdapter_->SetParameter(meta);
    } else {
        codecPlugin_->SetParameter(meta);
    }
    AVCODEC_LOGI("State from %{public}s to CONFIGURING", GetStatusDescription(state_).data());
    state_ = CodecState::CONFIGURING;
    return AVCS_ERR_OK;
}

int32_t MediaCodec::SetCodecCallback(CodecCallback *codecCallback)
{
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURING,
                             AVCS_ERR_INVALID_STATE, "In invalid state");
    int32_t ret = AVCS_ERR_OK;
    if (isVideo_) {
        callbackAdapter_ = std::make_shared<VideoCodecCallback>(codecCallback);
        ret = codecAdapter_->SetCallback(callbackAdapter_);
    } else {
        codecCallback_ = codecCallback;
        codecPlugin_->SetDataCallback(this);
    }
    AVCODEC_LOGI("State from %{public}s to CONFIGURING", GetStatusDescription(state_).data());
    state_ = CodecState::CONFIGURING;
    return AVCS_ERR_OK;
}

int32_t MediaCodec::SetOutputBufferQueue(std::shared_ptr<AVBufferQueueProducer> bufferQueueProducer)
{
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::CONFIGURING, AVCS_ERR_INVALID_STATE, "In invalid state");

    outputBufferQueueProducer_ = bufferQueueProducer;
    isBufferMode_ = true;
    AVCODEC_LOGI("State from %{public}s to CONFIGURING", GetStatusDescription(state_).data());
    state_ = CodecState::CONFIGURING;
    return AVCS_ERR_OK;
}

std::shared_ptr<AVBufferQueueProducer> MediaCodec::GetInputBufferQueue()
{
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::CONFIGURING, nullptr, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(!isSurfaceMode_, nullptr, "In surface mode");

    isBufferMode_ = true;
    AVCODEC_LOGI("State from %{public}s to CONFIGURING", GetStatusDescription(state_).data());
    state_ = CodecState::CONFIGURING;
    return inputBufferQueueProducer_;
}

int32_t MediaCodec::SetOutputSurface(sptr<Surface> surface)
{
    CHECK_AND_RETURN_RET_LOG(isVideo_, AVCS_ERR_UNSUPPORT, "Audio codec not supported");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::CONFIGURING, AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(!isBufferMode_, nullptr, "In buffer mode");

    isSurfaceMode_ = true;
    AVCODEC_LOGI("State from %{public}s to CONFIGURING", GetStatusDescription(state_).data());
    state_ = CodecState::CONFIGURING;
    return codecAdapter_->SetOutputSurface(surface);
}

sptr<Surface> MediaCodec::GetInputSurface()
{
    CHECK_AND_RETURN_RET_LOG(isVideo_, AVCS_ERR_UNSUPPORT, "Audio codec not supported");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::CONFIGURING, nullptr, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(!isBufferMode_, nullptr, "In buffer mode");

    isSurfaceMode_ = true;
    AVCODEC_LOGI("State from %{public}s to CONFIGURING", GetStatusDescription(state_).data());
    state_ = CodecState::CONFIGURING;
    return codecAdapter_->CreateInputSurface();
}

int32_t MediaCodec::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::CONFIGURING, AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(!isBufferMode_ || !isSurfaceMode_, AVCS_ERR_UNKNOWN,
                             "Cannot handle both modes at the same time");

    int32_t ret = PrepareInputBufferQueue();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");

    ret = PrepareOutputBufferQueue();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");

    AVCODEC_LOGI("State from %{public}s to CONFIGURED", GetStatusDescription(state_).data());
    state_ = CodecState::CONFIGURED;
    return ret;
}

int32_t MediaCodec::Start()
{
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::CONFIGURED || state_ == CodecState::FLUSHED, AVCS_ERR_INVALID_STATE,
                             "In invalid state");
    int32_t ret = AVCS_ERR_OK;
    state_ = CodecState::STARTING;
    if (isVideo_) {
        ret = codecAdapter_->Start();
    } else {
        ret = codecPlugin_->Start();
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");
    AVCODEC_LOGI("State from %{public}s to STARTED", GetStatusDescription(state_).data());
    state_ = CodecState::STARTED;
    return ret;
}

int32_t MediaCodec::Stop()
{
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::INITIALIZED && state_ != CodecState::CONFIGURING &&
                                 state_ != CodecState::CONFIGURED && state_ != CodecState::STOPPING,
                             AVCS_ERR_OK, "No need to handle the current state");
    int32_t ret = AVCS_ERR_OK;
    state_ = CodecState::STOPPING;
    if (isVideo_) {
        ret = codecAdapter_->Stop();
    } else {
        ret = codecPlugin_->Stop();
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");
    AVCODEC_LOGI("State from %{public}s to CONFIGURED", GetStatusDescription(state_).data());
    state_ = CodecState::CONFIGURED;
    return ret;
}

int32_t MediaCodec::Flush()
{
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::FLUSHED, AVCS_ERR_OK, "No need to handle the current state");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::STARTING, AVCS_ERR_INVALID_STATE, "In invalid state");
    int32_t ret = AVCS_ERR_OK;
    state_ = CodecState::FLUSHING;
    if (isVideo_) {
        ret = codecAdapter_->Flush();
    } else {
        ret = codecPlugin_->Flush();
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");
    AVCODEC_LOGI("State from %{public}s to FLUSHED", GetStatusDescription(state_).data());
    state_ = CodecState::FLUSHED;
    return ret;
}

int32_t MediaCodec::Reset()
{
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::UNINITIALIZED, AVCS_ERR_OK, "No need to handle the current state");

    int32_t ret = AVCS_ERR_OK;
    if (isVideo_) {
        ret = codecAdapter_->Reset();
    } else {
        ret = codecPlugin_->Reset();
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");
    AVCODEC_LOGI("State from %{public}s to UNINITIALIZED", GetStatusDescription(state_).data());
    state_ = CodecState::UNINITIALIZED;
    return ret;
}

int32_t MediaCodec::Release()
{
    int32_t ret = AVCS_ERR_OK;
    if (state_ == CodecState::RELEASING || state_ == CodecState::UNINITIALIZED) {
        return AVCS_ERR_OK;
    }
    if (state_ == CodecState::CONFIGURING || state_ == CodecState::CONFIGURED) {
        state_ = CodecState::UNINITIALIZED;
        return AVCS_ERR_OK;
    }
    state_ = CodecState::RELEASING;
    if (isVideo_) {
        ret = codecAdapter_->Release();
    } else {
        ret = codecPlugin_->Release();
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");
    AVCODEC_LOGI("State from %{public}s to UNINITIALIZED", GetStatusDescription(state_).data());
    state_ = CodecState::UNINITIALIZED;
    return ret;
}

int32_t MediaCodec::NotifyEOS()
{
    CHECK_AND_RETURN_RET_LOG(isVideo_, AVCS_ERR_UNSUPPORT, "Audio codec not supported");
    return codecAdapter_->NotifyEOS();
}

int32_t MediaCodec::SetParameter(const std::shared_ptr<Meta> parameter)
{
    int32_t ret = AVCS_ERR_OK;
    if (isVideo_) {
        ret = codecAdapter_->SetParameter(parameter);
    } else {
        ret = codecPlugin_->SetParameter(parameter);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");
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

int32_t MediaCodec::PrepareInputBufferQueue()
{
    int32_t ret = AVCS_ERR_OK;
    std::vector<std::shared_ptr<AVBuffer>> inputBuffers;
    ret = codecPlugin_->GetInputBuffers(&inputBuffers);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");
    if (inputBuffers.size() == 0) {
        int inputBufferSize = 5;
        inputBufferQueue_ = AVBufferQueue::Create(INPUT_BUFFER_QUEUE_NAME, inputBufferSize);
        Meta inputBufferConfig = std::make_shared<Meta>();

        ret = codecPlugin_->GetParameter(inputBufferConfig);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");
        int32_t capacity = inputBufferConfig.get("bufferSize");

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

int32_t MediaCodec::PrepareOutputBufferQueue()
{
    int32_t ret = AVCS_ERR_OK;
    std::vector<std::shared_ptr<AVBuffer>> outputBuffers;
    ret = codecPlugin_->GetOutputBuffers(&outputBuffers);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");
    if (outputBuffers.size() == 0) {
        int outputBufferSize = 5;
        Meta outputBufferConfig = std::make_shared<Meta>();
        ret = codecPlugin_->GetParameter(outputBufferConfig);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "The operation failed");
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
    int32_t ret = AVCS_ERR_OK;
    std::shared_ptr<AVBuffer> filledInputBuffer = std::make_shared<AVBuffer>();
    ret = inputBufferQueueConsumer_->AcquireBuffer(&filledInputBuffer);
    CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "The operation failed");
    if (state_ != CodecState::STARTED) {
        inputBufferQueueConsumer_->ReleaseBuffer(&filledInputBuffer);
        return;
    }
    if (isVideo_) {
        ret = codecAdapter_->QueueInputBuffer(callbackAdapter_->FindInputIndex(filledInputBuffer), filledInputBuffer);
    } else {
        ret = codecPlugin_->QueueInputBuffer(&filledInputBuffer);
    }
    if (ret != AVCS_ERR_OK) {
        inputBufferQueueConsumer_->ReleaseBuffer(&filledInputBuffer);
        return;
    }
    while (true) {
        std::shared_ptr<AVBuffer> emptyOutputBuffer = std::make_shared<AVBuffer>();
        AVBufferConfig avBufferConfig = new AVBufferConfig();
        ret = outputBufferQueueProducer_->RequestBuffer(&emptyOutputBuffer, &avBufferConfig);
        CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "The operation failed");
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
        CHECK_AND_CONTINUE_LOG(ret != Status::NOT_ENOUGH_DATA, "Not enough data");
        if (ret != AVCS_ERR_OK) {
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

const std::string &MediaCodec::GetStatusDescription(const CodecState &state)
{
    static const std::string ILLEGAL_STATE = "CODEC_STATUS_ILLEGAL";
    if (status < OHOS::MediaAVCodec::CodecServer::UNINITIALIZED || status > OHOS::MediaAVCodec::CodecServer::ERROR) {
        return ILLEGAL_STATE;
    }
    std::map<CodecState, std::string> GetStatusDescription = {
        {CodecState::RELEASED, " RELEASED"},         {CodecState::INITIALIZED, " INITIALIZED"},
        {CodecState::FLUSHED, " FLUSHED"},           {CodecState::RUNNING, " RUNNING"},
        {CodecState::INITIALIZING, " INITIALIZING"}, {CodecState::STARTING, " STARTING"},
        {CodecState::STOPPING, " STOPPING"},         {CodecState::FLUSHING, " FLUSHING"},
        {CodecState::RESUMING, " RESUMING"},         {CodecState::RELEASING, " RELEASING"},
    };
    return stateStrMap[state];
}
} // namespace MediaAVCodec
} // namespace OHOS
