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
#include <shared_mutex>
#include "osal/task/autolock.h"
#include "plugin/plugin_manager.h"

namespace {
const std::string INPUT_BUFFER_QUEUE_NAME = "MediaCodecInputBufferQueue";
constexpr int32_t DEFAULT_BUFFER_NUM = 8;
constexpr int32_t TIME_OUT_MS = 500;
} // namespace

namespace OHOS {
namespace Media {
class InputBufferAvailableListener : public IConsumerListener {
public:
    explicit InputBufferAvailableListener(MediaCodec *mediaCodec)
    {
        mediaCodec_ = mediaCodec;
    }

    void OnBufferAvailable() override
    {
        mediaCodec_->ProcessInputBuffer();
    }

private:
    MediaCodec *mediaCodec_;
};

int32_t MediaCodec::Init(const std::string &mime, bool isEncoder)
{
    AutoLock lock(stateMutex_);
    if (state_ != CodecState::UNINITIALIZED) {
        MEDIA_LOG_E("Init failed, state = %{public}s .", StateToString(state_).data());
        return (int32_t)Status::ERROR_INVALID_STATE;
    }
    MEDIA_LOG_I("state from %{public}s to INITIALIZING", StateToString(state_).data());
    state_ = CodecState::INITIALIZING;
    Plugins::PluginType type;
    if (isEncoder) {
        type = Plugins::PluginType::AUDIO_ENCODER;
    } else {
        type = Plugins::PluginType::AUDIO_DECODER;
    }
    codecPlugin_ = CreatePlugin(mime, type);
    if (codecPlugin_ != nullptr) {
        MEDIA_LOG_I("codecPlugin_->Init()");
        auto ret = codecPlugin_->Init();
        FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "pluign init failed");
        state_ = CodecState::INITIALIZED;
    } else {
        MEDIA_LOG_I("createPlugin failed");
        return (int32_t)Status::ERROR_INVALID_PARAMETER;
    }
    return (int32_t)Status::OK;
}

int32_t MediaCodec::Init(const std::string &name)
{
    AutoLock lock(stateMutex_);
    MEDIA_LOG_I("MediaCodec::Init");
    if (state_ != CodecState::UNINITIALIZED) {
        MEDIA_LOG_E("Init failed, state = %{public}s .", StateToString(state_).data());
        return (int32_t)Status::ERROR_INVALID_STATE;
    }
    MEDIA_LOG_I("state from %{public}s to INITIALIZING", StateToString(state_).data());
    state_ = CodecState::INITIALIZING;
    Plugins::PluginType type = Plugins::PluginType::INVALID_TYPE;
    if (name.find("Encoder") != name.npos) {
        type = Plugins::PluginType::AUDIO_ENCODER;
    } else if (name.find("Decoder") != name.npos) {
        type = Plugins::PluginType::AUDIO_DECODER;
    }
    FALSE_RETURN_V(type != Plugins::PluginType::INVALID_TYPE, (int32_t)Status::ERROR_INVALID_PARAMETER);
    auto plugin = Plugins::PluginManager::Instance().CreatePlugin(name, type);
    FALSE_RETURN_V_MSG_E(plugin != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER, "create pluign failed");
    codecPlugin_ = std::reinterpret_pointer_cast<Plugins::CodecPlugin>(plugin);
    Status ret = codecPlugin_->Init();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)Status::ERROR_INVALID_PARAMETER, "pluign init failed");
    state_ = CodecState::INITIALIZED;
    return (int32_t)Status::OK;
}

std::shared_ptr<Plugins::CodecPlugin> MediaCodec::CreatePlugin(const std::string &mime, Plugins::PluginType pluginType)
{
    auto names = Plugins::PluginManager::Instance().ListPlugins(pluginType);
    std::string pluginName = "";
    for (auto &name : names) {
        auto info = Plugins::PluginManager::Instance().GetPluginInfo(pluginType, name);
        if (info == nullptr) {
            MEDIA_LOG_W("info is nullptr, mime:%{public}s name:%{public}s", mime.c_str(), name.c_str());
            continue;
        }
        auto capSet = info->inCaps;
        if (capSet.size() <= 0) {
            MEDIA_LOG_W("capSet size is 0, mime:%{public}s name:%{public}s", mime.c_str(), name.c_str());
            continue;
        }
        MEDIA_LOG_D("name::%{public}s mime:%{public}s mime:%{public}s", name.c_str(), mime.c_str(),
                    capSet[0].mime.c_str());
        if (mime.compare(capSet[0].mime) == 0) {
            pluginName = name;
            break;
        }
    }
    MEDIA_LOG_I("mime:%{public}s, pluginName:%{public}s", mime.c_str(), pluginName.c_str());
    if (!pluginName.empty()) {
        auto plugin = Plugins::PluginManager::Instance().CreatePlugin(pluginName, pluginType);
        return std::reinterpret_pointer_cast<Plugins::CodecPlugin>(plugin);
    } else {
        MEDIA_LOG_E("No plugins matching mime:%{public}s", mime.c_str());
    }
    return nullptr;
}

int32_t MediaCodec::Configure(const std::shared_ptr<Meta> &meta)
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::INITIALIZED, (int32_t)Status::ERROR_INVALID_STATE);
    auto ret = codecPlugin_->SetParameter(meta);
    FALSE_RETURN_V(ret == Status::OK, (int32_t)ret);
    ret = codecPlugin_->SetDataCallback(this);
    FALSE_RETURN_V(ret == Status::OK, (int32_t)ret);
    state_ = CodecState::CONFIGURED;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer)
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
                   (int32_t)Status::ERROR_INVALID_STATE);
    outputBufferQueueProducer_ = bufferQueueProducer;
    isBufferMode_ = true;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetCodecCallback(const std::shared_ptr<CodecCallback> &codecCallback)
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
                   (int32_t)Status::ERROR_INVALID_STATE);
    FALSE_RETURN_V_MSG_E(codecCallback != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER,
                         "codecCallback is nullptr");
    codecCallback_ = codecCallback;
    auto ret = codecPlugin_->SetDataCallback(this);
    FALSE_RETURN_V(ret == Status::OK, (int32_t)ret);
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetOutputSurface(sptr<Surface> surface)
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
                   (int32_t)Status::ERROR_INVALID_STATE);
    isSurfaceMode_ = true;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::Prepare()
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ != CodecState::PREPARED, (int32_t)Status::OK);
    FALSE_RETURN_V(state_ == CodecState::CONFIGURED || state_ == CodecState::FLUSHED,
        (int32_t)Status::ERROR_INVALID_STATE);
    if (isBufferMode_ && isSurfaceMode_) {
        MEDIA_LOG_E("state error");
        return (int32_t)Status::ERROR_UNKNOWN;
    }
    outputBufferCapacity_ = 0;
    auto ret = (int32_t)PrepareInputBufferQueue();
    if (ret != (int32_t)Status::OK) {
        MEDIA_LOG_E("PrepareInputBufferQueue failed");
        return (int32_t)ret;
    }
    ret = (int32_t)PrepareOutputBufferQueue();
    if (ret != (int32_t)Status::OK) {
        MEDIA_LOG_E("PrepareOutputBufferQueue failed");
        return (int32_t)ret;
    }
    state_ = CodecState::PREPARED;
    MEDIA_LOG_I("Prepare, ret = %{public}d", (int32_t)ret);
    return (int32_t)Status::OK;
}

sptr<AVBufferQueueProducer> MediaCodec::GetInputBufferQueue()
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::PREPARED, sptr<AVBufferQueueProducer>());
    if (isSurfaceMode_) {
        return nullptr;
    }
    isBufferMode_ = true;
    return inputBufferQueueProducer_;
}

sptr<Surface> MediaCodec::GetInputSurface()
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::PREPARED, nullptr);
    if (isBufferMode_) {
        return nullptr;
    }
    isSurfaceMode_ = true;
    return nullptr;
}

int32_t MediaCodec::Start()
{
    AutoLock lock(stateMutex_);
    MEDIA_LOG_I("Start enter");
    FALSE_RETURN_V(state_ != CodecState::RUNNING, (int32_t)Status::OK);
    FALSE_RETURN_V(state_ == CodecState::PREPARED || state_ == CodecState::FLUSHED,
                   (int32_t)Status::ERROR_INVALID_STATE);
    state_ = CodecState::STARTING;
    auto ret = codecPlugin_->Start();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "plugin start failed");
    state_ = CodecState::RUNNING;
    return (int32_t)ret;
}

int32_t MediaCodec::Stop()
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ != CodecState::PREPARED, (int32_t)Status::OK);
    if (state_ == CodecState::UNINITIALIZED || state_ == CodecState::STOPPING || state_ == CodecState::RELEASING) {
        MEDIA_LOG_D("Stop, state_=%{public}s", StateToString(state_).data());
        return (int32_t)Status::OK;
    }
    FALSE_RETURN_V(state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM ||
                   state_ == CodecState::FLUSHED, (int32_t)Status::ERROR_INVALID_STATE);
    state_ = CodecState::STOPPING;
    auto ret = codecPlugin_->Stop();
    MEDIA_LOG_I("codec Stop, state from %{public}s to Stop", StateToString(state_).data());
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "plugin stop failed");
    ClearInputBuffer();
    state_ = CodecState::PREPARED;
    return (int32_t)ret;
}

int32_t MediaCodec::Flush()
{
    AutoLock lock(stateMutex_);
    if (state_ == CodecState::FLUSHED) {
        MEDIA_LOG_W("Flush, state is already flushed, state_=%{public}s .", StateToString(state_).data());
        return (int32_t)Status::OK;
    }
    if (state_ != CodecState::RUNNING && state_ != CodecState::END_OF_STREAM) {
        MEDIA_LOG_E("Flush failed, state =%{public}s", StateToString(state_).data());
        return (int32_t)Status::ERROR_INVALID_STATE;
    }
    MEDIA_LOG_I("Flush, state from %{public}s to FLUSHING", StateToString(state_).data());
    state_ = CodecState::FLUSHING;
    inputBufferQueueProducer_->Clear();
    auto ret = codecPlugin_->Flush();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "plugin flush failed");
    ClearInputBuffer();
    state_ = CodecState::FLUSHED;
    return (int32_t)ret;
}

int32_t MediaCodec::Reset()
{
    AutoLock lock(stateMutex_);
    if (state_ == CodecState::UNINITIALIZED || state_ == CodecState::RELEASING) {
        MEDIA_LOG_W("adapter reset, state is already released, state =%{public}s .", StateToString(state_).data());
        return (int32_t)Status::OK;
    }
    if (state_ == CodecState::INITIALIZING) {
        MEDIA_LOG_W("adapter reset, state is initialized, state =%{public}s .", StateToString(state_).data());
        state_ = CodecState::INITIALIZED;
        return (int32_t)Status::OK;
    }
    auto ret = codecPlugin_->Reset();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "plugin reset failed");
    state_ = CodecState::INITIALIZED;
    return (int32_t)ret;
}

int32_t MediaCodec::Release()
{
    AutoLock lock(stateMutex_);
    if (state_ == CodecState::UNINITIALIZED || state_ == CodecState::RELEASING) {
        MEDIA_LOG_W("codec Release, state isnot completely correct, state =%{public}s .", StateToString(state_).data());
        return (int32_t)Status::OK;
    }

    if (state_ == CodecState::INITIALIZING) {
        MEDIA_LOG_W("codec Release, state isnot completely correct, state =%{public}s .", StateToString(state_).data());
        state_ = CodecState::RELEASING;
        return (int32_t)Status::OK;
    }
    MEDIA_LOG_I("codec Release, state from %{public}s to RELEASING", StateToString(state_).data());
    state_ = CodecState::RELEASING;
    auto ret = codecPlugin_->Release();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "plugin release failed");
    state_ = CodecState::UNINITIALIZED;
    return (int32_t)ret;
}

int32_t MediaCodec::NotifyEos()
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ != CodecState::END_OF_STREAM, (int32_t)Status::OK);
    FALSE_RETURN_V(state_ == CodecState::RUNNING, (int32_t)Status::ERROR_INVALID_STATE);
    state_ = CodecState::END_OF_STREAM;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(parameter != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V(state_ != CodecState::UNINITIALIZED && state_ != CodecState::INITIALIZED &&
                   state_ != CodecState::PREPARED, (int32_t)Status::ERROR_INVALID_STATE);
    auto ret = codecPlugin_->SetParameter(parameter);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "plugin set parameter failed");
    return (int32_t)ret;
}

int32_t MediaCodec::GetOutputFormat(std::shared_ptr<Meta> &parameter)
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V_MSG_E(state_ != CodecState::UNINITIALIZED, (int32_t)Status::ERROR_INVALID_STATE,
                         "status incorrect,get output format failed.");
    FALSE_RETURN_V(codecPlugin_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE);
    FALSE_RETURN_V(parameter != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER);
    auto ret = codecPlugin_->GetParameter(parameter);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "plugin get parameter failed");
    return (int32_t)ret;
}

Status MediaCodec::AttachBufffer()
{
    int inputBufferNum = DEFAULT_BUFFER_NUM;
    MemoryType memoryType;
#ifndef MEDIA_OHOS
    memoryType = MemoryType::VIRTUAL_MEMORY;
#else
    memoryType = MemoryType::SHARED_MEMORY;
#endif
    inputBufferQueue_ = AVBufferQueue::Create(inputBufferNum, memoryType, INPUT_BUFFER_QUEUE_NAME);
    FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, Status::ERROR_UNKNOWN,
                         "inputBufferQueue_ is nullptr");
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    std::shared_ptr<Meta> inputBufferConfig = std::make_shared<Meta>();
    FALSE_RETURN_V_MSG_E(codecPlugin_ != nullptr, Status::ERROR_UNKNOWN, "codecPlugin_ is nullptr");
    auto ret = codecPlugin_->GetParameter(inputBufferConfig);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "attachBufffer failed, plugin get param error");
    int32_t capacity = 0;
    FALSE_RETURN_V_MSG_E(inputBufferConfig != nullptr, Status::ERROR_UNKNOWN,
                         "inputBufferConfig is nullptr");
    FALSE_RETURN_V(inputBufferConfig->Get<Tag::AUDIO_MAX_INPUT_SIZE>(capacity),
                   Status::ERROR_INVALID_PARAMETER);
    for (int i = 0; i < inputBufferNum; i++) {
        std::shared_ptr<AVAllocator> avAllocator;
#ifndef MEDIA_OHOS
        MEDIA_LOG_D("CreateVirtualAllocator,i=%{public}d capacity=%{public}d", i, capacity);
        avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
#else
        MEDIA_LOG_D("CreateSharedAllocator,i=%{public}d capacity=%{public}d", i, capacity);
        avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
#endif
        std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
        FALSE_RETURN_V_MSG_E(inputBufferQueueProducer_ != nullptr, Status::ERROR_UNKNOWN,
                             "inputBufferQueueProducer_ is nullptr");
        inputBufferQueueProducer_->AttachBuffer(inputBuffer, false);
    }
    return Status::OK;
}

int32_t MediaCodec::PrepareInputBufferQueue()
{
    std::vector<std::shared_ptr<AVBuffer>> inputBuffers;
    FALSE_RETURN_V_MSG_E(codecPlugin_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "codecPlugin_ is nullptr");
    auto ret = codecPlugin_->GetInputBuffers(inputBuffers);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "pluign getInputBuffers failed");
    if (inputBuffers.empty()) {
        ret = AttachBufffer();
        if (ret != Status::OK) {
            MEDIA_LOG_E("GetParameter failed");
            return (int32_t)ret;
        }
    } else {
        inputBufferQueue_ =
            AVBufferQueue::Create(inputBuffers.size(), MemoryType::HARDWARE_MEMORY, INPUT_BUFFER_QUEUE_NAME);
        FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, (int32_t)Status::ERROR_UNKNOWN,
                             "inputBufferQueue_ is nullptr");
        inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
        for (uint32_t i = 0; i < inputBuffers.size(); i++) {
            inputBufferQueueProducer_->AttachBuffer(inputBuffers[i], false);
        }
    }
    FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "inputBufferQueue_ is nullptr");
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    sptr<IConsumerListener> listener = new InputBufferAvailableListener(this);
    FALSE_RETURN_V_MSG_E(inputBufferQueueConsumer_ != nullptr, (int32_t)Status::ERROR_UNKNOWN,
                         "inputBufferQueueConsumer_ is nullptr");
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    return (int32_t)ret;
}

int32_t MediaCodec::PrepareOutputBufferQueue()
{
    std::vector<std::shared_ptr<AVBuffer>> outputBuffers;
    FALSE_RETURN_V_MSG_E(codecPlugin_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "codecPlugin_ is nullptr");
    auto ret = codecPlugin_->GetOutputBuffers(outputBuffers);
    if (ret != Status::OK) {
        MEDIA_LOG_E("GetOutputBuffers failed");
        return (int32_t)ret;
    }
    if (outputBuffers.empty()) {
        int outputBufferNum = 30;
        std::shared_ptr<Meta> outputBufferConfig = std::make_shared<Meta>();
        ret = codecPlugin_->GetParameter(outputBufferConfig);
        if (ret != Status::OK) {
            MEDIA_LOG_E("GetParameter failed");
            return (int32_t)ret;
        }
        FALSE_RETURN_V_MSG_E(outputBufferConfig != nullptr, (int32_t)Status::ERROR_INVALID_STATE,
                             "outputBufferConfig is nullptr");
        FALSE_RETURN_V(outputBufferConfig->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(outputBufferCapacity_),
                       (int32_t)Status::ERROR_INVALID_PARAMETER);
        for (int i = 0; i < outputBufferNum; i++) {
            auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            std::shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputBufferCapacity_);
            FALSE_RETURN_V_MSG_E(outputBufferQueueProducer_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE,
                                 "outputBufferQueueProducer_ is nullptr");
            outputBufferQueueProducer_->AttachBuffer(outputBuffer, false);
        }
    } else {
        FALSE_RETURN_V_MSG_E(outputBufferQueueProducer_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE,
                             "outputBufferQueueProducer_ is nullptr");
        for (uint32_t i = 0; i < outputBuffers.size(); i++) {
            outputBufferQueueProducer_->AttachBuffer(outputBuffers[i], false);
        }
    }
    return (int32_t)ret;
}

void MediaCodec::ProcessInputBuffer()
{
    Status ret;
    uint32_t eosStatus = 0;
    std::shared_ptr<AVBuffer> filledInputBuffer;
    ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
    if (ret != Status::OK) {
        MEDIA_LOG_E("ProcessInputBuffer AcquireBuffer fail");
        return;
    }
    if (state_ != CodecState::RUNNING) {
        MEDIA_LOG_E("ProcessInputBuffer ReleaseBuffer name:MediaCodecInputBufferQueue");
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        return;
    }
    const int8_t RETRY = 3; // max retry count is 3
    int8_t retryCount = 0;
    do {
        ret = codecPlugin_->QueueInputBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            retryCount++;
            continue;
        }
    } while (ret != Status::OK && retryCount < RETRY);

    if (ret != Status::OK) {
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        MEDIA_LOG_E("Plugin queueInputBuffer failed.");
        return;
    }
    eosStatus = filledInputBuffer->flag_;
    do {
        ret = HandleOutputBuffer(eosStatus);
    } while (ret == Status::ERROR_AGAIN);
}

Status MediaCodec::HandleOutputBuffer(uint32_t eosStatus)
{
    Status ret = Status::OK;
    std::shared_ptr<AVBuffer> emptyOutputBuffer;
    AVBufferConfig avBufferConfig;
    do {
        ret = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
    } while (ret != Status::OK);
    if (emptyOutputBuffer) {
        emptyOutputBuffer->flag_ = eosStatus;
    } else {
        return Status::ERROR_NULL_POINTER;
    }
    ret = codecPlugin_->QueueOutputBuffer(emptyOutputBuffer);
    if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
        MEDIA_LOG_D("QueueOutputBuffer ERROR_NOT_ENOUGH_DATA");
        outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, false);
    } else if (ret == Status::ERROR_AGAIN) {
        MEDIA_LOG_D("The output data is not completely read, needs to be read again");
    } else if (ret == Status::END_OF_STREAM) {
        MEDIA_LOG_D("HandleOutputBuffer END_OF_STREAM");
    } else if (ret != Status::OK) {
        MEDIA_LOG_E("QueueOutputBuffer error");
        outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, false);
    }
    return ret;
}

void MediaCodec::ClearInputBuffer()
{
    std::shared_ptr<AVBuffer> filledInputBuffer;
    Status ret = Status::OK;
    while (ret == Status::OK) {
        ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            MEDIA_LOG_I("clear input Buffer");
            return;
        }
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
    }
}

void MediaCodec::OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    Status ret = inputBufferQueueConsumer_->ReleaseBuffer(inputBuffer);
    FALSE_RETURN_MSG(ret == Status::OK, "OnInputBufferDone fail");
}

void MediaCodec::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer)
{
    Status ret = outputBufferQueueProducer_->PushBuffer(outputBuffer, true);
    FALSE_RETURN_MSG(ret == Status::OK, "OnOutputBufferDone fail");
}

void MediaCodec::OnEvent(const std::shared_ptr<Plugins::PluginEvent> event) {}

std::string MediaCodec::StateToString(CodecState state)
{
    std::map<CodecState, std::string> stateStrMap = {
        {CodecState::UNINITIALIZED, " UNINITIALIZED"},
        {CodecState::INITIALIZED, " INITIALIZED"},
        {CodecState::FLUSHED, " FLUSHED"},
        {CodecState::RUNNING, " RUNNING"},
        {CodecState::INITIALIZING, " INITIALIZING"},
        {CodecState::STARTING, " STARTING"},
        {CodecState::STOPPING, " STOPPING"},
        {CodecState::FLUSHING, " FLUSHING"},
        {CodecState::RESUMING, " RESUMING"},
        {CodecState::RELEASING, " RELEASING"},
    };
    return stateStrMap[state];
}
} // namespace Media
} // namespace OHOS
