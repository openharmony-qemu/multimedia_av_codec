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
#include "media_codec.h"
#include <shared_mutex>
#include "avcodec_common.h"
#include "common/log.h"
#include "osal/task/autolock.h"
#include "plugin/plugin_manager_v2.h"
#include "avcodec_log.h"
#include "osal/utils/dump_buffer.h"
#include "avcodec_trace.h"
#include "bundle_mgr_interface.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#ifdef SUPPORT_DRM
#include "i_keysession_service.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_AUDIO, "MediaCodec" };
const std::string INPUT_BUFFER_QUEUE_NAME = "MediaCodecInputBufferQueue";
constexpr int8_t RETRY = 3; // max retry count is 3
constexpr int32_t DEFAULT_BUFFER_NUM = 8;
constexpr int32_t TIME_OUT_MS = 50;
constexpr int32_t TIME_OUT_MS_INNER = 1;
constexpr uint32_t API_SUPPORT_AUDIO_FORMAT_CHANGED = 15;
constexpr uint32_t INVALID_API_VERSION = 0;
constexpr uint32_t API_VERSION_MOD = 1000;
const std::string DUMP_PARAM = "a";
const std::string DUMP_FILE_NAME = "player_audio_decoder_output.pcm";
} // namespace

namespace OHOS {
namespace Media {
class InputBufferAvailableListener : public IConsumerListener {
public:
    explicit InputBufferAvailableListener(std::shared_ptr<MediaCodec> mediaCodec)
    {
        mediaCodec_ = mediaCodec;
    }

    void OnBufferAvailable() override
    {
        auto realPtr = mediaCodec_.lock();
        if (realPtr != nullptr) {
            realPtr->ProcessInputBuffer();
        } else {
            MEDIA_LOG_W("mediacodec was released, can not process input buffer");
        }
    }

private:
    std::weak_ptr<MediaCodec> mediaCodec_;
};

MediaCodec::MediaCodec()
    : codecPlugin_(nullptr),
      inputBufferQueue_(nullptr),
      inputBufferQueueProducer_(nullptr),
      inputBufferQueueConsumer_(nullptr),
      outputBufferQueueProducer_(nullptr),
      isEncoder_(false),
      isSurfaceMode_(false),
      isBufferMode_(false),
      outputBufferCapacity_(0),
      state_(CodecState::UNINITIALIZED)
{
}

MediaCodec::~MediaCodec()
{
    state_ = CodecState::UNINITIALIZED;
    outputBufferCapacity_ = 0;
    if (codecPlugin_) {
        codecPlugin_ = nullptr;
    }
    if (inputBufferQueue_) {
        inputBufferQueue_ = nullptr;
    }
    if (inputBufferQueueProducer_) {
        inputBufferQueueProducer_ = nullptr;
    }
    if (inputBufferQueueConsumer_) {
        inputBufferQueueConsumer_ = nullptr;
    }
    if (outputBufferQueueProducer_) {
        outputBufferQueueProducer_ = nullptr;
    }
}

int32_t MediaCodec::Init(const std::string &mime, bool isEncoder)
{
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Init");
    MEDIA_LOG_I("Init enter, mime: " PUBLIC_LOG_S, mime.c_str());
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
    MEDIA_LOG_I("Init enter, name: " PUBLIC_LOG_S, name.c_str());
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Init");
    if (state_ != CodecState::UNINITIALIZED) {
        MEDIA_LOG_E("Init failed, state = %{public}s .", StateToString(state_).data());
        return (int32_t)Status::ERROR_INVALID_STATE;
    }
    MEDIA_LOG_I("state from %{public}s to INITIALIZING", StateToString(state_).data());
    state_ = CodecState::INITIALIZING;
    auto plugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(name);
    FALSE_RETURN_V_MSG_E(plugin != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER, "create pluign failed");
    codecPlugin_ = std::reinterpret_pointer_cast<Plugins::CodecPlugin>(plugin);
    Status ret = codecPlugin_->Init();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)Status::ERROR_INVALID_PARAMETER, "pluign init failed");
    state_ = CodecState::INITIALIZED;
    return (int32_t)Status::OK;
}

std::shared_ptr<Plugins::CodecPlugin> MediaCodec::CreatePlugin(const std::string &mime, Plugins::PluginType pluginType)
{
    auto plugin = Plugins::PluginManagerV2::Instance().CreatePluginByMime(pluginType, mime);
    if (plugin == nullptr) {
        return nullptr;
    }
    return std::reinterpret_pointer_cast<Plugins::CodecPlugin>(plugin);
}

int32_t MediaCodec::Configure(const std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("MediaCodec::configure in");
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Configure");
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
    MediaAVCodec::AVCodecTrace trace("MediaCodec::SetOutputBufferQueue");
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

int32_t MediaCodec::SetCodecCallback(const std::shared_ptr<AudioBaseCodecCallback> &codecCallback)
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
                   (int32_t)Status::ERROR_INVALID_STATE);
    FALSE_RETURN_V_MSG_E(codecCallback != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER,
                         "codecCallback is nullptr");
    mediaCodecCallback_ = codecCallback;
    uint32_t apiVersion = GetApiVersion();
    isSupportAudioFormatChanged_ =
        ((apiVersion == INVALID_API_VERSION) || (apiVersion >= API_SUPPORT_AUDIO_FORMAT_CHANGED));
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
    MEDIA_LOG_I("Prepare enter");
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Prepare");
    FALSE_RETURN_V_MSG_W(state_ != CodecState::FLUSHED, (int32_t)Status::ERROR_AGAIN,
        "state is flushed, no need prepare");
    FALSE_RETURN_V(state_ != CodecState::PREPARED, (int32_t)Status::OK);
    FALSE_RETURN_V(state_ == CodecState::CONFIGURED,
        (int32_t)Status::ERROR_INVALID_STATE);
    if (isBufferMode_ && isSurfaceMode_) {
        MEDIA_LOG_E("state error");
        return (int32_t)Status::ERROR_UNKNOWN;
    }
    outputBufferCapacity_ = 0;
    auto ret = (int32_t)PrepareInputBufferQueue();
    CHECK_AND_RETURN_RET_LOG(ret == (int32_t)Status::OK, (int32_t)ret, "PrepareInputBufferQueue failed");
    ret = (int32_t)PrepareOutputBufferQueue();
    CHECK_AND_RETURN_RET_LOG(ret == (int32_t)Status::OK, (int32_t)ret, "PrepareOutputBufferQueue failed");
    state_ = CodecState::PREPARED;
    MEDIA_LOG_I("Prepare, ret = %{public}d", (int32_t)ret);
    return (int32_t)Status::OK;
}

sptr<AVBufferQueueProducer> MediaCodec::GetInputBufferQueue()
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::PREPARED, sptr<AVBufferQueueProducer>());
    CHECK_AND_RETURN_RET_LOG(!isSurfaceMode_, nullptr, "GetInputBufferQueue isSurfaceMode_");
    isBufferMode_ = true;
    return inputBufferQueueProducer_;
}

sptr<AVBufferQueueConsumer> MediaCodec::GetInputBufferQueueConsumer()
{
    AutoLock lock(stateMutex_);
    // Case: to enable to set listener to input bufferqueue consumer. i.e. input bufferqueue updating by ChangePlugin.
    FALSE_RETURN_V(state_ == CodecState::PREPARED || state_ == CodecState::RUNNING ||
                   state_ == CodecState::FLUSHED || state_ == CodecState::END_OF_STREAM, sptr<AVBufferQueueConsumer>());
    CHECK_AND_RETURN_RET_LOG(!isSurfaceMode_, nullptr, "GetInputBufferQueueConsumer isSurfaceMode_");
    isBufferMode_ = true;
    return inputBufferQueueConsumer_;
}

sptr<AVBufferQueueProducer> MediaCodec::GetOutputBufferQueueProducer()
{
    AutoLock lock(stateMutex_);
    // Case: to enable to set listener to output bufferqueue producer. i.e. output bufferqueue updating by ChangePlugin.
    FALSE_RETURN_V(state_ == CodecState::PREPARED || state_ == CodecState::RUNNING ||
                   state_ == CodecState::FLUSHED || state_ == CodecState::END_OF_STREAM, sptr<AVBufferQueueProducer>());
    CHECK_AND_RETURN_RET_LOG(!isSurfaceMode_, nullptr, "GetOutputBufferQueueProducer isSurfaceMode_");

    return outputBufferQueueProducer_;
}

void MediaCodec::ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed)
{
    MEDIA_LOG_D("ProcessInputBufferInnerr isTriggeredByOutPort:" PUBLIC_LOG_D32 ", isFlushed:" PUBLIC_LOG_D32
        ", isOutputBufferAvailable:" PUBLIC_LOG_D32 ", inputBufferQueueSize:" PUBLIC_LOG_U32
        ", inputBufferEosStatus:" PUBLIC_LOG_U32, isTriggeredByOutPort, isFlushed, isOutputBufferAvailable_.load(),
        inputBufferQueueConsumer_->GetFilledBufferSize(), inputBufferEosStatus_.load());
    FALSE_RETURN_MSG(!isFlushed, "ProcessInputBufferInner isFlushed true");
    FALSE_RETURN_MSG(inputBufferQueueConsumer_->GetFilledBufferSize() != 0 || !isOutputBufferAvailable_.load(),
        "ProcessInputBufferInner ignore");

    MediaAVCodec::AVCodecTrace trace("MediaCodec::ProcessInputBufferInner");
    Status ret = Status::OK;

    // The last process failed to RequestBuffer from outputBufferQueueProducer, perform HandleOutputBufferInner firstly.
    if (!isOutputBufferAvailable_.load() || inputBufferEosStatus_.load() != 0) {
    CHECK_AND_RETURN_LOGD(HandleOutputBufferInner(ret), "S1 output buffer unavailable, ret:%{public}d", ret);
        inputBufferEosStatus_.store(0);
    }

    bool isProcessingNeeded = false;
    uint32_t eosStatus = 0;
    ret = Status::OK;
    HandleInputBufferInner(eosStatus, isProcessingNeeded, ret);
    CHECK_AND_RETURN_LOG(isProcessingNeeded, "ProcessInputBufferInner failed %{public}d", static_cast<int32_t>(ret));

    inputBufferEosStatus_ = eosStatus;
    CHECK_AND_RETURN_LOGD(HandleOutputBufferInner(ret), "S2 output buffer unavailable, ret:%{public}d", ret);
    inputBufferEosStatus_.store(0);
}

bool MediaCodec::HandleOutputBufferInner(Status &ret)
{
    MediaAVCodec::AVCodecTrace trace("MediaCodec::HandleOutputBufferInner");
    bool isBufferAvailable = false;
    do {
        isBufferAvailable = false;
        ret = HandleOutputBufferOnce(isBufferAvailable, inputBufferEosStatus_.load(), false);
    } while (ret == Status::ERROR_AGAIN);
    MEDIA_LOG_D("HandleOutputBufferInner ret:%{public}d, isBufferAvailable:%{public}d", ret, isBufferAvailable);
    isOutputBufferAvailable_.store(isBufferAvailable);
    return isBufferAvailable;
}

sptr<Surface> MediaCodec::GetInputSurface()
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::PREPARED, nullptr);
    CHECK_AND_RETURN_RET_LOG(!isBufferMode_, nullptr, "GetInputBufferQueue isBufferMode_");
    isSurfaceMode_ = true;
    return nullptr;
}

int32_t MediaCodec::Start()
{
    AutoLock lock(stateMutex_);
    MEDIA_LOG_I("Start enter");
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Start");
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
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Stop");
    MEDIA_LOG_I("Stop enter");
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
    inputBufferEosStatus_.store(0);
    isOutputBufferAvailable_.store(true);
    state_ = CodecState::PREPARED;
    return (int32_t)ret;
}

int32_t MediaCodec::Flush()
{
    AutoLock lock(stateMutex_);
    MEDIA_LOG_I("Flush enter");
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
    inputBufferEosStatus_.store(0);
    isOutputBufferAvailable_.store(true);
    state_ = CodecState::FLUSHED;
    return (int32_t)ret;
}

int32_t MediaCodec::Reset()
{
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Reset");
    MEDIA_LOG_I("Reset enter");
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
    ClearInputBuffer();
    inputBufferEosStatus_.store(0);
    isOutputBufferAvailable_.store(true);
    state_ = CodecState::INITIALIZED;
    return (int32_t)ret;
}

int32_t MediaCodec::Release()
{
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Release");
    MEDIA_LOG_I("Release enter");
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
    codecPlugin_ = nullptr;
    ClearBufferQueue();
    inputBufferEosStatus_.store(0);
    isOutputBufferAvailable_.store(true);
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

void MediaCodec::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    if (isDump && instanceId == 0) {
        MEDIA_LOG_W("Cannot dump with instanceId 0.");
        return;
    }
    dumpPrefix_ = std::to_string(instanceId);
    isDump_ = isDump;
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
    MEDIA_LOG_I("AttachBufffer enter");
    int inputBufferNum = DEFAULT_BUFFER_NUM;
    MemoryType memoryType;
#ifndef MEDIA_OHOS
    memoryType = MemoryType::VIRTUAL_MEMORY;
#else
    memoryType = MemoryType::SHARED_MEMORY;
#endif
    if (inputBufferQueue_ == nullptr) {
        inputBufferQueue_ = AVBufferQueue::Create(inputBufferNum, memoryType, INPUT_BUFFER_QUEUE_NAME);
    }
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
        FALSE_RETURN_V_MSG_E(inputBuffer != nullptr, Status::ERROR_UNKNOWN,
                             "inputBuffer is nullptr");
        FALSE_RETURN_V_MSG_E(inputBufferQueueProducer_ != nullptr, Status::ERROR_UNKNOWN,
                             "inputBufferQueueProducer_ is nullptr");
        inputBufferQueueProducer_->AttachBuffer(inputBuffer, false);
        MEDIA_LOG_I("Attach intput buffer. index: %{public}d, bufferId: %{public}" PRIu64,
            i, inputBuffer->GetUniqueId());
        inputBufferVector_.push_back(inputBuffer);
    }
    return Status::OK;
}

Status MediaCodec::AttachDrmBufffer(std::shared_ptr<AVBuffer> &drmInbuf, std::shared_ptr<AVBuffer> &drmOutbuf,
    uint32_t size)
{
    MEDIA_LOG_D("AttachDrmBufffer");
    std::shared_ptr<AVAllocator> avAllocator;
    avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    FALSE_RETURN_V_MSG_E(avAllocator != nullptr, Status::ERROR_UNKNOWN,
        "avAllocator is nullptr");

    drmInbuf = AVBuffer::CreateAVBuffer(avAllocator, size);
    FALSE_RETURN_V_MSG_E(drmInbuf != nullptr, Status::ERROR_UNKNOWN,
        "drmInbuf is nullptr");
    FALSE_RETURN_V_MSG_E(drmInbuf->memory_ != nullptr, Status::ERROR_UNKNOWN,
        "drmInbuf->memory_ is nullptr");
    drmInbuf->memory_->SetSize(size);

    drmOutbuf = AVBuffer::CreateAVBuffer(avAllocator, size);
    FALSE_RETURN_V_MSG_E(drmOutbuf != nullptr, Status::ERROR_UNKNOWN,
        "drmOutbuf is nullptr");
    FALSE_RETURN_V_MSG_E(drmOutbuf->memory_ != nullptr, Status::ERROR_UNKNOWN,
        "drmOutbuf->memory_ is nullptr");
    drmOutbuf->memory_->SetSize(size);
    return Status::OK;
}

Status MediaCodec::DrmAudioCencDecrypt(std::shared_ptr<AVBuffer> &filledInputBuffer)
{
    MEDIA_LOG_D("DrmAudioCencDecrypt enter");
    Status ret = Status::OK;

    // 1. allocate drm buffer
    uint32_t bufSize = static_cast<uint32_t>(filledInputBuffer->memory_->GetSize());
    if (bufSize == 0) {
        MEDIA_LOG_D("MediaCodec DrmAudioCencDecrypt input buffer size equal 0");
        return ret;
    }
    std::shared_ptr<AVBuffer> drmInBuf;
    std::shared_ptr<AVBuffer> drmOutBuf;
    ret = AttachDrmBufffer(drmInBuf, drmOutBuf, bufSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "AttachDrmBufffer failed");

    // 2. copy data to drm input buffer
    int32_t drmRes = memcpy_s(drmInBuf->memory_->GetAddr(), bufSize,
        filledInputBuffer->memory_->GetAddr(), bufSize);
    FALSE_RETURN_V_MSG_E(drmRes == 0, Status::ERROR_UNKNOWN, "memcpy_s drmInBuf failed");
    if (filledInputBuffer->meta_ != nullptr) {
        *(drmInBuf->meta_) = *(filledInputBuffer->meta_);
    }
    // 4. decrypt
    drmRes = drmDecryptor_->DrmAudioCencDecrypt(drmInBuf, drmOutBuf, bufSize);
    FALSE_RETURN_V_MSG_E(drmRes == 0, Status::ERROR_DRM_DECRYPT_FAILED, "DrmAudioCencDecrypt return error");

    // 5. copy decrypted data from drm output buffer back
    drmRes = memcpy_s(filledInputBuffer->memory_->GetAddr(), bufSize,
        drmOutBuf->memory_->GetAddr(), bufSize);
    FALSE_RETURN_V_MSG_E(drmRes == 0, Status::ERROR_UNKNOWN, "memcpy_s drmOutBuf failed");
    return Status::OK;
}

void MediaCodec::HandleAudioCencDecryptError()
{
    MEDIA_LOG_E("MediaCodec DrmAudioCencDecrypt failed.");
    auto realPtr = mediaCodecCallback_.lock();
    if (realPtr != nullptr) {
        realPtr->OnError(CodecErrorType::CODEC_DRM_DECRYTION_FAILED,
            static_cast<int32_t>(Status::ERROR_DRM_DECRYPT_FAILED));
    }
}

int32_t MediaCodec::PrepareInputBufferQueue()
{
    MEDIA_LOG_I("PrepareInputBufferQueue enter");
    std::vector<std::shared_ptr<AVBuffer>> inputBuffers;
    MediaAVCodec::AVCodecTrace trace("MediaCodec::PrepareInputBufferQueue");
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
        if (inputBufferQueue_ == nullptr) {
            inputBufferQueue_ =
                AVBufferQueue::Create(inputBuffers.size(), MemoryType::HARDWARE_MEMORY, INPUT_BUFFER_QUEUE_NAME);
        }
        FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, (int32_t)Status::ERROR_UNKNOWN,
                             "inputBufferQueue_ is nullptr");
        inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
        for (uint32_t i = 0; i < inputBuffers.size(); i++) {
            inputBufferQueueProducer_->AttachBuffer(inputBuffers[i], false);
            inputBufferVector_.push_back(inputBuffers[i]);
        }
    }
    FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "inputBufferQueue_ is nullptr");
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    sptr<IConsumerListener> listener = new InputBufferAvailableListener(shared_from_this());
    FALSE_RETURN_V_MSG_E(inputBufferQueueConsumer_ != nullptr, (int32_t)Status::ERROR_UNKNOWN,
                         "inputBufferQueueConsumer_ is nullptr");
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    return (int32_t)ret;
}

int32_t MediaCodec::PrepareOutputBufferQueue()
{
    MEDIA_LOG_I("PrepareOutputBufferQueue enter");
    std::vector<std::shared_ptr<AVBuffer>> outputBuffers;
    MediaAVCodec::AVCodecTrace trace("MediaCodec::PrepareOutputBufferQueue");
    FALSE_RETURN_V_MSG_E(codecPlugin_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "codecPlugin_ is nullptr");
    auto ret = codecPlugin_->GetOutputBuffers(outputBuffers);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "GetOutputBuffers failed");
    FALSE_RETURN_V_MSG_E(outputBufferQueueProducer_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE,
                         "outputBufferQueueProducer_ is nullptr");
    if (outputBuffers.empty()) {
        int outputBufferNum = 30;
        std::shared_ptr<Meta> outputBufferConfig = std::make_shared<Meta>();
        ret = codecPlugin_->GetParameter(outputBufferConfig);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret, "GetParameter failed");
        FALSE_RETURN_V_MSG_E(outputBufferConfig != nullptr, (int32_t)Status::ERROR_INVALID_STATE,
                             "outputBufferConfig is nullptr");
        FALSE_RETURN_V(outputBufferConfig->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(outputBufferCapacity_),
                       (int32_t)Status::ERROR_INVALID_PARAMETER);
        for (int i = 0; i < outputBufferNum; i++) {
            auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            std::shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputBufferCapacity_);
            FALSE_RETURN_V_MSG_E(outputBuffer != nullptr, (int32_t)Status::ERROR_INVALID_STATE,
                                 "outputBuffer is nullptr");
            if (outputBufferQueueProducer_->AttachBuffer(outputBuffer, false) == Status::OK) {
                MEDIA_LOG_D("Attach output buffer. index: %{public}d, bufferId: %{public}" PRIu64, i,
                            outputBuffer->GetUniqueId());
                outputBufferVector_.push_back(outputBuffer);
            }
        }
    } else {
        for (uint32_t i = 0; i < outputBuffers.size(); i++) {
            if (outputBufferQueueProducer_->AttachBuffer(outputBuffers[i], false) == Status::OK) {
                MEDIA_LOG_D("Attach output buffer. index: %{public}d, bufferId: %{public}" PRIu64, i,
                            outputBuffers[i]->GetUniqueId());
                outputBufferVector_.push_back(outputBuffers[i]);
            }
        }
    }
    FALSE_RETURN_V_MSG_E(outputBufferVector_.size() > 0, (int32_t)Status::ERROR_INVALID_STATE, "Attach no buffer");
    return (int32_t)ret;
}

void MediaCodec::ProcessInputBuffer()
{
    MEDIA_LOG_D("ProcessInputBuffer enter");
    MediaAVCodec::AVCodecTrace trace("MediaCodec::ProcessInputBuffer");

    bool isProcessingNeeded = false;
    Status ret = Status::OK;
    uint32_t eosStatus = 0;
    HandleInputBufferInner(eosStatus, isProcessingNeeded, ret);
    CHECK_AND_RETURN_LOG(isProcessingNeeded, "ProcessInputBuffer failed %{public}d", static_cast<int32_t>(ret));

    do {
        ret = HandleOutputBuffer(eosStatus);
    } while (ret == Status::ERROR_AGAIN);
}

void MediaCodec::HandleInputBufferInner(uint32_t &eosStatus, bool &isProcessingNeeded, Status &ret)
{
    MediaAVCodec::AVCodecTrace traceHandleInputBufferInner("MediaCodec::HandleInputBufferInner");

    std::shared_ptr<AVBuffer> filledInputBuffer;
    if (state_ != CodecState::RUNNING) {
        MEDIA_LOG_I("HandleInputBufferInner status changed, current status is not running");
        ret = Status::ERROR_INVALID_STATE;
        return;
    }
    {
        MediaAVCodec::AVCodecTrace traceAcquireBuffer("MediaCodec::HandleInputBufferInner-AcquireBuffer");
        ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            MEDIA_LOG_E("HandleInputBufferInner AcquireBuffer fail");
            return;
        }
    }
    if (state_ != CodecState::RUNNING) {
        MEDIA_LOG_D("HandleInputBufferInner not running, ReleaseBuffer name:MediaCodecInputBufferQueue");
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        ret = Status::ERROR_INVALID_STATE;
        return;
    }
    uint32_t flag = filledInputBuffer->flag_;
    int8_t retryCount = 0;
    do {
        if (drmDecryptor_ != nullptr) {
            MediaAVCodec::AVCodecTrace trace("MediaCodec::HandleInputBufferInner-DrmAudioCencDecrypt");
            ret = DrmAudioCencDecrypt(filledInputBuffer);
            if (ret != Status::OK) {
                HandleAudioCencDecryptError();
                break;
            }
        }

        ret = CodePluginInputBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            retryCount++;
            continue;
        }
    } while (ret != Status::OK && retryCount < RETRY && state_ == CodecState::RUNNING);

    if (ret != Status::OK || state_ != CodecState::RUNNING) {
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        MEDIA_LOG_E("Plugin queueInputBuffer failed, state_:%{public}d, ret:%{public}d", state_.load(), ret);
        return;
    }
    isProcessingNeeded = true;
    eosStatus = flag;
}

#ifdef SUPPORT_DRM
int32_t MediaCodec::SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
    MEDIA_LOG_I("MediaCodec::SetAudioDecryptionConfig");
    if (drmDecryptor_ == nullptr) {
        drmDecryptor_ = std::make_shared<MediaAVCodec::CodecDrmDecrypt>();
    }
    FALSE_RETURN_V_MSG_E(drmDecryptor_ != nullptr, (int32_t)Status::ERROR_NO_MEMORY, "drmDecryptor is nullptr");
    drmDecryptor_->SetDecryptionConfig(keySession, svpFlag);
    return (int32_t)Status::OK;
}
#else
int32_t MediaCodec::SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
    MEDIA_LOG_I("MediaCodec::SetAudioDecryptionConfig, Not support");
    (void)keySession;
    (void)svpFlag;
    return (int32_t)Status::OK;
}
#endif

Status MediaCodec::ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta)
{
    Status ret = Status::OK;
    Plugins::PluginType type;
    if (isEncoder) {
        type = Plugins::PluginType::AUDIO_ENCODER;
    } else {
        type = Plugins::PluginType::AUDIO_DECODER;
    }

    {
        AutoLock pluginLock(codecPluginMutex_);
        if (codecPlugin_ != nullptr) {
            codecPlugin_->Release();
            codecPlugin_ = nullptr;
        }
        codecPlugin_ = CreatePlugin(mime, type);

        CHECK_AND_RETURN_RET_LOG(codecPlugin_ != nullptr, Status::ERROR_INVALID_PARAMETER, "createPlugin failed");
        ret = codecPlugin_->SetParameter(meta);
        MEDIA_LOG_I("codecPlugin SetParameter ret %{public}d", ret);
        ret = codecPlugin_->Init();
        MEDIA_LOG_I("codecPlugin Init ret %{public}d", ret);
        ret = codecPlugin_->SetDataCallback(this);
        MEDIA_LOG_I("codecPlugin SetDataCallback ret %{public}d", ret);
    }

    // discard undecoded data and unconsumed decoded data.
    inputBufferQueueProducer_->Clear();
    FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, Status::ERROR_UNKNOWN, "inputBufferQueue_ is nullptr");
    ClearInputBuffer();
    inputBufferEosStatus_.store(0);
    isOutputBufferAvailable_.store(true);

    PrepareInputBufferQueue();
    PrepareOutputBufferQueue();
    if (state_ == CodecState::RUNNING) {
        ret = codecPlugin_->Start();
        MEDIA_LOG_I("codecPlugin Start ret %{public}d", ret);
    }

    return ret;
}

Status MediaCodec::HandleOutputBuffer(uint32_t eosStatus)
{
    MEDIA_LOG_D("HandleOutputBuffer enter");
    bool isBufferAvailable = false;
    Status ret = HandleOutputBufferOnce(isBufferAvailable, eosStatus, true);
    MEDIA_LOG_D("HandleOutputBuffer ret:%{public}d, isBufferAvailable:%{public}d", ret, isBufferAvailable);
    return ret;
}

Status MediaCodec::HandleOutputBufferOnce(bool &isBufferAvailable, uint32_t eosStatus, bool isSync)
{
    MediaAVCodec::AVCodecTrace traceHandleOutputBufferOnce("MediaCodec::HandleOutputBufferOnce");
    Status ret = Status::OK;
    std::shared_ptr<AVBuffer> emptyOutputBuffer;
    AVBufferConfig avBufferConfig;
    {
        if (isSync) {
            MediaAVCodec::AVCodecTrace traceRequestBuffer("MediaCodec::HandleOutputBufferOnce-RequestBuffer-sync");
            do {
                ret = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
            } while (ret != Status::OK && state_ == CodecState::RUNNING);
        } else {
            MediaAVCodec::AVCodecTrace traceRequestBuffer("MediaCodec::HandleOutputBufferOnce-RequestBuffer-async");
            ret = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS_INNER);
        }
    }
    if (emptyOutputBuffer) {
        emptyOutputBuffer->flag_ = eosStatus;
        isBufferAvailable = true;
    } else if (state_ != CodecState::RUNNING) {
        return Status::OK;
    } else {
        return Status::ERROR_NULL_POINTER;
    }

    ret = CodePluginOutputBuffer(emptyOutputBuffer);
    if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
        MEDIA_LOG_D("HandleOutputBufferOnce QueueOutputBuffer ERROR_NOT_ENOUGH_DATA");
        outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, false);
    } else if (ret == Status::ERROR_AGAIN) {
        MEDIA_LOG_D("HandleOutputBufferOnce The output data is not completely read, needs to be read again");
    } else if (ret == Status::END_OF_STREAM) {
        MEDIA_LOG_D("HandleOutputBufferOnce QueueOutputBuffer END_OF_STREAM");
    } else if (ret != Status::OK) {
        MEDIA_LOG_E("HandleOutputBufferOnce QueueOutputBuffer error");
        outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, false);
    }
    return ret;
}

void MediaCodec::OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    MediaAVCodec::AVCodecTrace trace("MediaCodec::OnInputBufferDone");
    Status ret = inputBufferQueueConsumer_->ReleaseBuffer(inputBuffer);
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " OnInputBufferDone, buffer->pts:" PUBLIC_LOG_D64,
        FAKE_POINTER(this), inputBuffer->pts_);
    FALSE_RETURN_MSG(ret == Status::OK, "OnInputBufferDone fail");
}

void MediaCodec::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer)
{
    MediaAVCodec::AVCodecTrace trace("MediaCodec::OnOutputBufferDone");
    if (isDump_) {
        DumpAVBufferToFile(DUMP_PARAM, dumpPrefix_ + DUMP_FILE_NAME, outputBuffer);
    }
    Status ret = outputBufferQueueProducer_->PushBuffer(outputBuffer, true);
    auto realPtr = mediaCodecCallback_.lock();
    if (realPtr != nullptr) {
        realPtr->OnOutputBufferDone(outputBuffer);
    }
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " OnOutputBufferDone, buffer->pts:" PUBLIC_LOG_D64,
        FAKE_POINTER(this), outputBuffer->pts_);
    FALSE_RETURN_MSG(ret == Status::OK, "OnOutputBufferDone fail");
}

void MediaCodec::ClearBufferQueue()
{
    MEDIA_LOG_I("ClearBufferQueue called.");
    if (inputBufferQueueProducer_ != nullptr) {
        for (const auto &buffer : inputBufferVector_) {
            inputBufferQueueProducer_->DetachBuffer(buffer);
        }
        inputBufferVector_.clear();
        inputBufferQueueProducer_->SetQueueSize(0);
    }
    if (outputBufferQueueProducer_ != nullptr) {
        for (const auto &buffer : outputBufferVector_) {
            outputBufferQueueProducer_->DetachBuffer(buffer);
        }
        outputBufferVector_.clear();
        outputBufferQueueProducer_->SetQueueSize(0);
    }
}

void MediaCodec::ClearInputBuffer()
{
    MediaAVCodec::AVCodecTrace trace("MediaCodec::ClearInputBuffer");
    MEDIA_LOG_D("ClearInputBuffer enter");
    if (!inputBufferQueueConsumer_) {
        return;
    }
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

void MediaCodec::OnEvent(const std::shared_ptr<Plugins::PluginEvent> event)
{
    if (event->type != Plugins::PluginEventType::AUDIO_OUTPUT_FORMAT_CHANGED) {
        return;
    }

    if (!isSupportAudioFormatChanged_) {
        MEDIA_LOG_W("receive audio format changed but api version is low");
        return;
    }

    auto realPtr = mediaCodecCallback_.lock();
    if (realPtr != nullptr) {
        std::shared_ptr<Meta> format = std::make_shared<Meta>(AnyCast<Meta>(event->param));
        realPtr->OnOutputFormatChanged(format);
    } else {
        MEDIA_LOG_E("receive AUDIO_OUTPUT_FORMAT_CHANGED, but lock callback fail");
    }
}

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

void MediaCodec::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D("MediaCodec::OnDumpInfo called.");
    if (fd < 0) {
        MEDIA_LOG_E("MediaCodec::OnDumpInfo fd is invalid.");
        return;
    }
    std::string dumpString;
    dumpString += "MediaCodec plugin name: " + codecPluginName_ + "\n";
    dumpString += "MediaCodec buffer size is:" + std::to_string(inputBufferQueue_->GetQueueSize()) + "\n";
    int ret = write(fd, dumpString.c_str(), dumpString.size());
    if (ret < 0) {
        MEDIA_LOG_E("MediaCodec::OnDumpInfo write failed.");
        return;
    }
}

uint32_t MediaCodec::GetApiVersion()
{
    uint32_t apiVersion = INVALID_API_VERSION;
    OHOS::sptr<OHOS::ISystemAbilityManager> systemAbilityManager =
        OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    OHOS::sptr<OHOS::IRemoteObject> remoteObject =
        systemAbilityManager->CheckSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    sptr<AppExecFwk::IBundleMgr> iBundleMgr = OHOS::iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    if (iBundleMgr == nullptr) {
        MEDIA_LOG_W("GetApiVersion IBundleMgr is nullptr");
        return apiVersion;
    }
    AppExecFwk::BundleInfo bundleInfo;
    if (iBundleMgr->GetBundleInfoForSelf(0, bundleInfo) == ERR_OK) {
        apiVersion = bundleInfo.targetVersion % API_VERSION_MOD;
        MEDIA_LOG_I("GetApiVersion targetVersion: %{public}u", bundleInfo.targetVersion);
    } else {
        MEDIA_LOG_W("GetApiVersion failed, call by SA or test maybe");
    }
    return apiVersion;
}

Status MediaCodec::CodePluginInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    AutoLock pluginLock(codecPluginMutex_);
    if (codecPlugin_ != nullptr) {
        MediaAVCodec::AVCodecTrace traceQueueInputBuffer("MediaCodec::HandleInputBufferInner-QueueInputBuffer");
        return codecPlugin_->QueueInputBuffer(inputBuffer);
    } else {
        MEDIA_LOG_E("plugin is null");
        return Status::ERROR_UNKNOWN;
    }
}

Status MediaCodec::CodePluginOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    AutoLock pluginLock(codecPluginMutex_);
    FALSE_RETURN_V_MSG_E(codecPlugin_ != nullptr, Status::ERROR_INVALID_STATE, "plugin is null");

    MediaAVCodec::AVCodecTrace traceQueueOutputBuffer("MediaCodec::HandleOutputBufferOnce-QueueOutputBuffer");
    return codecPlugin_->QueueOutputBuffer(outputBuffer);
}

} // namespace Media
} // namespace OHOS
