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
#include "avcodec_codec_name.h"
#include "avcodec_trace.h"
#include "plugin/plugin_manager_v2.h"
#include "osal/utils/dump_buffer.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_AUDIO, "MediaCodec" };
const std::string INPUT_BUFFER_QUEUE_NAME = "MediaCodecInputBufferQueue";
constexpr int32_t DEFAULT_BUFFER_NUM = 8;
constexpr int32_t TIME_OUT_MS = 500;
const std::string DUMP_PARAM = "a";
const std::string DUMP_FILE_NAME = "player_audio_decoder_output.pcm";
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

MediaCodec::MediaCodec()
    : codecPlugin_(nullptr),
      inputBufferQueue_(nullptr),
      inputBufferQueueProducer_(nullptr),
      inputBufferQueueConsumer_(nullptr),
      outputBufferQueueProducer_(nullptr),
      codecCallback_(nullptr),
      mediaCodecCallback_(nullptr),
      isEncoder_(false),
      isSurfaceMode_(false),
      isBufferMode_(false),
      outputBufferCapacity_(0),
      state_(CodecState::UNINITIALIZED)
{
}

MediaCodec::~MediaCodec()
{
    codecPlugin_ = nullptr;
    inputBufferQueue_ = nullptr;
    inputBufferQueueProducer_ = nullptr;
    inputBufferQueueConsumer_ = nullptr;
    outputBufferQueueProducer_ = nullptr;
    codecCallback_ = nullptr;
    mediaCodecCallback_ = nullptr;
    outputBufferCapacity_ = 0;
}

int32_t MediaCodec::Init(const std::string &mime, bool isEncoder)
{
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Init");
    AVCODEC_LOGI("Init enter, mime: %{public}s", mime.c_str());
    if (state_ != CodecState::UNINITIALIZED) {
        AVCODEC_LOGE("Init failed, state = %{public}s .", StateToString(state_).data());
        return (int32_t)Status::ERROR_INVALID_STATE;
    }
    AVCODEC_LOGI("state from %{public}s to INITIALIZING", StateToString(state_).data());
    state_ = CodecState::INITIALIZING;
    Plugins::PluginType type;
    if (isEncoder) {
        type = Plugins::PluginType::AUDIO_ENCODER;
    } else {
        type = Plugins::PluginType::AUDIO_DECODER;
    }
    codecPlugin_ = CreatePlugin(mime, type);
    if (codecPlugin_ != nullptr) {
        AVCODEC_LOGI("codecPlugin_->Init()");
        auto ret = codecPlugin_->Init();
        CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "pluign init failed");
        state_ = CodecState::INITIALIZED;
    } else {
        AVCODEC_LOGI("createPlugin failed");
        return (int32_t)Status::ERROR_INVALID_PARAMETER;
    }
    return (int32_t)Status::OK;
}

int32_t MediaCodec::Init(const std::string &name)
{
    AutoLock lock(stateMutex_);
    AVCODEC_LOGI("Init enter, name: %{public}s", name.c_str());
    AVCODEC_LOGI("MediaCodec::Init");
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Init");
    if (state_ != CodecState::UNINITIALIZED) {
        AVCODEC_LOGE("Init failed, state = %{public}s .", StateToString(state_).data());
        return (int32_t)Status::ERROR_INVALID_STATE;
    }
    AVCODEC_LOGI("state from %{public}s to INITIALIZING", StateToString(state_).data());
    state_ = CodecState::INITIALIZING;
    auto plugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(name);
    CHECK_AND_RETURN_RET_LOG(plugin != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER, "create pluign failed");
    codecPlugin_ = std::reinterpret_pointer_cast<Plugins::CodecPlugin>(plugin);
    Status ret = codecPlugin_->Init();
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)Status::ERROR_INVALID_PARAMETER, "pluign init failed");
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
    AVCODEC_LOGI("MediaCodec::configure in");
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Configure");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED, (int32_t)Status::ERROR_INVALID_STATE,
        "state is not INITIALIZED");
    auto ret = codecPlugin_->SetParameter(meta);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "SetParameter failed.");
    ret = codecPlugin_->SetDataCallback(this);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "SetDataCallback failed.");
    state_ = CodecState::CONFIGURED;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer)
{
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::SetOutputBufferQueue");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
        (int32_t)Status::ERROR_INVALID_STATE, "state is not INITIALIZED or CONFIGURED");
    outputBufferQueueProducer_ = bufferQueueProducer;
    isBufferMode_ = true;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetCodecCallback(const std::shared_ptr<CodecCallback> &codecCallback)
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
        (int32_t)Status::ERROR_INVALID_STATE, "state != INITIALIZED and state != CONFIGURED");
    CHECK_AND_RETURN_RET_LOG(codecCallback != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER,
                             "codecCallback is nullptr");
    codecCallback_ = codecCallback;
    auto ret = codecPlugin_->SetDataCallback(this);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "ret != Status::OK");
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetCodecCallback(const std::shared_ptr<AudioBaseCodecCallback> &codecCallback)
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
        (int32_t)Status::ERROR_INVALID_STATE, "state != INITIALIZED and state != CONFIGURED");
    CHECK_AND_RETURN_RET_LOG(codecCallback != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER,
                             "codecCallback is nullptr");
    mediaCodecCallback_ = codecCallback;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetOutputSurface(sptr<Surface> surface)
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
        (int32_t)Status::ERROR_INVALID_STATE, "state != INITIALIZED and state != CONFIGURED");
    isSurfaceMode_ = true;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::Prepare()
{
    AVCODEC_LOGI("Prepare enter");
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Prepare");
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::PREPARED, (int32_t)Status::OK, "state != PREPARED");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::CONFIGURED || state_ == CodecState::FLUSHED,
        (int32_t)Status::ERROR_INVALID_STATE, "state != CONFIGURED and state != FLUSHED");
    if (isBufferMode_ && isSurfaceMode_) {
        AVCODEC_LOGE("state error");
        return (int32_t)Status::ERROR_UNKNOWN;
    }
    outputBufferCapacity_ = 0;
    auto ret = (int32_t)PrepareInputBufferQueue();
    if (ret != (int32_t)Status::OK) {
        AVCODEC_LOGE("PrepareInputBufferQueue failed");
        return (int32_t)ret;
    }
    ret = (int32_t)PrepareOutputBufferQueue();
    if (ret != (int32_t)Status::OK) {
        AVCODEC_LOGE("PrepareOutputBufferQueue failed");
        return (int32_t)ret;
    }
    state_ = CodecState::PREPARED;
    AVCODEC_LOGI("Prepare, ret = %{public}d", (int32_t)ret);
    return (int32_t)Status::OK;
}

sptr<AVBufferQueueProducer> MediaCodec::GetInputBufferQueue()
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::PREPARED, sptr<AVBufferQueueProducer>(), "state != PREPARED");
    if (isSurfaceMode_) {
        return nullptr;
    }
    isBufferMode_ = true;
    return inputBufferQueueProducer_;
}

sptr<Surface> MediaCodec::GetInputSurface()
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::PREPARED, nullptr, "state != PREPARED");
    if (isBufferMode_) {
        return nullptr;
    }
    isSurfaceMode_ = true;
    return nullptr;
}

int32_t MediaCodec::Start()
{
    AutoLock lock(stateMutex_);
    AVCODEC_LOGI("Start enter");
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Start");
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::RUNNING, (int32_t)Status::OK, "state != RUNNING");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::PREPARED || state_ == CodecState::FLUSHED,
        (int32_t)Status::ERROR_INVALID_STATE, "state != PREPARED and state != FLUSHED");
    state_ = CodecState::STARTING;
    auto ret = codecPlugin_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin start failed");
    state_ = CodecState::RUNNING;
    return (int32_t)ret;
}

int32_t MediaCodec::Stop()
{
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Stop");
    AVCODEC_LOGI("Stop enter");
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::PREPARED, (int32_t)Status::OK, "state != PREPARED");
    if (state_ == CodecState::UNINITIALIZED || state_ == CodecState::STOPPING || state_ == CodecState::RELEASING) {
        AVCODEC_LOGD("Stop, state_=%{public}s", StateToString(state_).data());
        return (int32_t)Status::OK;
    }
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM ||
        state_ == CodecState::FLUSHED, (int32_t)Status::ERROR_INVALID_STATE, "state != RUNNING, EOS or FLUSHED");
    state_ = CodecState::STOPPING;
    auto ret = codecPlugin_->Stop();
    AVCODEC_LOGI("codec Stop, state from %{public}s to Stop", StateToString(state_).data());
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin stop failed");
    ClearInputBuffer();
    state_ = CodecState::PREPARED;
    return (int32_t)ret;
}

int32_t MediaCodec::Flush()
{
    AutoLock lock(stateMutex_);
    AVCODEC_LOGI("Flush enter");
    if (state_ == CodecState::FLUSHED) {
        AVCODEC_LOGW("Flush, state is already flushed, state_=%{public}s .", StateToString(state_).data());
        return (int32_t)Status::OK;
    }
    if (state_ != CodecState::RUNNING && state_ != CodecState::END_OF_STREAM) {
        AVCODEC_LOGE("Flush failed, state =%{public}s", StateToString(state_).data());
        return (int32_t)Status::ERROR_INVALID_STATE;
    }
    AVCODEC_LOGI("Flush, state from %{public}s to FLUSHING", StateToString(state_).data());
    state_ = CodecState::FLUSHING;
    inputBufferQueueProducer_->Clear();
    auto ret = codecPlugin_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin flush failed");
    ClearInputBuffer();
    state_ = CodecState::FLUSHED;
    return (int32_t)ret;
}

int32_t MediaCodec::Reset()
{
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Reset");
    AVCODEC_LOGI("Reset enter");
    if (state_ == CodecState::UNINITIALIZED || state_ == CodecState::RELEASING) {
        AVCODEC_LOGW("adapter reset, state is already released, state =%{public}s .", StateToString(state_).data());
        return (int32_t)Status::OK;
    }
    if (state_ == CodecState::INITIALIZING) {
        AVCODEC_LOGW("adapter reset, state is initialized, state =%{public}s .", StateToString(state_).data());
        state_ = CodecState::INITIALIZED;
        return (int32_t)Status::OK;
    }
    auto ret = codecPlugin_->Reset();
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin reset failed");
    ClearInputBuffer();
    state_ = CodecState::INITIALIZED;
    return (int32_t)ret;
}

int32_t MediaCodec::Release()
{
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Release");
    AVCODEC_LOGI("Release enter");
    if (state_ == CodecState::UNINITIALIZED || state_ == CodecState::RELEASING) {
        AVCODEC_LOGW("codec Release, state isnot completely correct, state =%{public}s .",
                     StateToString(state_).data());
        return (int32_t)Status::OK;
    }

    if (state_ == CodecState::INITIALIZING) {
        AVCODEC_LOGW("codec Release, state isnot completely correct, state =%{public}s .",
                     StateToString(state_).data());
        state_ = CodecState::RELEASING;
        return (int32_t)Status::OK;
    }
    AVCODEC_LOGI("codec Release, state from %{public}s to RELEASING", StateToString(state_).data());
    state_ = CodecState::RELEASING;
    auto ret = codecPlugin_->Release();
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin release failed");
    codecPlugin_ = nullptr;
    ClearBufferQueue();
    state_ = CodecState::UNINITIALIZED;
    return (int32_t)ret;
}

int32_t MediaCodec::NotifyEos()
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::END_OF_STREAM, (int32_t)Status::OK, "state not EOS");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::RUNNING, (int32_t)Status::ERROR_INVALID_STATE, "state not RUNNING");
    state_ = CodecState::END_OF_STREAM;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(parameter != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER, "parameter is nullptr");
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::UNINITIALIZED && state_ != CodecState::INITIALIZED &&
        state_ != CodecState::PREPARED, (int32_t)Status::ERROR_INVALID_STATE, "state is invalid");
    auto ret = codecPlugin_->SetParameter(parameter);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin set parameter failed");
    return (int32_t)ret;
}

void MediaCodec::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    if (isDump && instanceId == 0) {
        AVCODEC_LOGW("Cannot dump with instanceId 0.");
        return;
    }
    dumpPrefix_ = std::to_string(instanceId);
    isDump_ = isDump;
}

int32_t MediaCodec::GetOutputFormat(std::shared_ptr<Meta> &parameter)
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::UNINITIALIZED, (int32_t)Status::ERROR_INVALID_STATE,
                             "status incorrect,get output format failed.");
    CHECK_AND_RETURN_RET_LOG(codecPlugin_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "codecPlugin_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(parameter != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER, "parameter is nullptr");
    auto ret = codecPlugin_->GetParameter(parameter);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin get parameter failed");
    return (int32_t)ret;
}

Status MediaCodec::AttachBufffer()
{
    AVCODEC_LOGI("AttachBufffer enter");
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
    CHECK_AND_RETURN_RET_LOG(inputBufferQueue_ != nullptr, Status::ERROR_UNKNOWN, "inputBufferQueue_ is nullptr");
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    std::shared_ptr<Meta> inputBufferConfig = std::make_shared<Meta>();
    CHECK_AND_RETURN_RET_LOG(codecPlugin_ != nullptr, Status::ERROR_UNKNOWN, "codecPlugin_ is nullptr");
    auto ret = codecPlugin_->GetParameter(inputBufferConfig);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret, "attachBufffer failed, plugin get param error");
    int32_t capacity = 0;
    CHECK_AND_RETURN_RET_LOG(inputBufferConfig != nullptr, Status::ERROR_UNKNOWN,
        "inputBufferConfig is nullptr");
    CHECK_AND_RETURN_RET_LOG(inputBufferConfig->Get<Tag::AUDIO_MAX_INPUT_SIZE>(capacity),
        Status::ERROR_INVALID_PARAMETER, "get AUDIO_MAX_INPUT_SIZE failed");
    for (int i = 0; i < inputBufferNum; i++) {
        std::shared_ptr<AVAllocator> avAllocator;
#ifndef MEDIA_OHOS
        AVCODEC_LOGD("CreateVirtualAllocator,i=%{public}d capacity=%{public}d", i, capacity);
        avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
#else
        AVCODEC_LOGD("CreateSharedAllocator,i=%{public}d capacity=%{public}d", i, capacity);
        avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
#endif
        std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
        CHECK_AND_RETURN_RET_LOG(inputBuffer != nullptr, Status::ERROR_UNKNOWN, "inputBuffer is nullptr");
        CHECK_AND_RETURN_RET_LOG(inputBufferQueueProducer_ != nullptr, Status::ERROR_UNKNOWN,
            "inputBufferQueueProducer_ is nullptr");
        inputBufferQueueProducer_->AttachBuffer(inputBuffer, false);
        AVCODEC_LOGI("Attach intput buffer. index: %{public}d, bufferId: %{public}" PRIu64,
            i, inputBuffer->GetUniqueId());
        inputBufferVector_.push_back(inputBuffer);
    }
    return Status::OK;
}

Status MediaCodec::AttachDrmBufffer(std::shared_ptr<AVBuffer> &drmInbuf, std::shared_ptr<AVBuffer> &drmOutbuf,
    uint32_t size)
{
    AVCODEC_LOGD("AttachDrmBufffer");
    std::shared_ptr<AVAllocator> avAllocator;
    avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    CHECK_AND_RETURN_RET_LOG(avAllocator != nullptr, Status::ERROR_UNKNOWN,
        "avAllocator is nullptr");

    drmInbuf = AVBuffer::CreateAVBuffer(avAllocator, size);
    CHECK_AND_RETURN_RET_LOG(drmInbuf != nullptr, Status::ERROR_UNKNOWN,
        "drmInbuf is nullptr");
    CHECK_AND_RETURN_RET_LOG(drmInbuf->memory_ != nullptr, Status::ERROR_UNKNOWN,
        "drmInbuf->memory_ is nullptr");
    drmInbuf->memory_->SetSize(size);

    drmOutbuf = AVBuffer::CreateAVBuffer(avAllocator, size);
    CHECK_AND_RETURN_RET_LOG(drmOutbuf != nullptr, Status::ERROR_UNKNOWN,
        "drmOutbuf is nullptr");
    CHECK_AND_RETURN_RET_LOG(drmOutbuf->memory_ != nullptr, Status::ERROR_UNKNOWN,
        "drmOutbuf->memory_ is nullptr");
    drmOutbuf->memory_->SetSize(size);
    return Status::OK;
}

Status MediaCodec::DrmAudioCencDecrypt(std::shared_ptr<AVBuffer> &filledInputBuffer)
{
    AVCODEC_LOGD("DrmAudioCencDecrypt enter");
    Status ret = Status::OK;

    // 1. allocate drm buffer
    uint32_t bufSize = static_cast<uint32_t>(filledInputBuffer->memory_->GetSize());
    if (bufSize == 0) {
        AVCODEC_LOGD("MediaCodec DrmAudioCencDecrypt input buffer size equal 0");
        return ret;
    }
    std::shared_ptr<AVBuffer> drmInBuf;
    std::shared_ptr<AVBuffer> drmOutBuf;
    ret = AttachDrmBufffer(drmInBuf, drmOutBuf, bufSize);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, Status::ERROR_UNKNOWN, "AttachDrmBufffer failed");

    // 2. copy data to drm input buffer
    int32_t drmRes = memcpy_s(drmInBuf->memory_->GetAddr(), bufSize,
        filledInputBuffer->memory_->GetAddr(), bufSize);
    CHECK_AND_RETURN_RET_LOG(drmRes == 0, Status::ERROR_UNKNOWN, "memcpy_s drmInBuf failed");
    if (filledInputBuffer->meta_ != nullptr) {
        *(drmInBuf->meta_) = *(filledInputBuffer->meta_);
    }
    // 4. decrypt
    drmRes = drmDecryptor_->DrmAudioCencDecrypt(drmInBuf, drmOutBuf, bufSize);
    CHECK_AND_RETURN_RET_LOG(drmRes == 0, Status::ERROR_UNKNOWN, "DrmAudioCencDecrypt return error");

    // 5. copy decrypted data from drm output buffer back
    drmRes = memcpy_s(filledInputBuffer->memory_->GetAddr(), bufSize,
        drmOutBuf->memory_->GetAddr(), bufSize);
    CHECK_AND_RETURN_RET_LOG(drmRes == 0, Status::ERROR_UNKNOWN, "memcpy_s drmOutBuf failed");
    return Status::OK;
}

int32_t MediaCodec::PrepareInputBufferQueue()
{
    AVCODEC_LOGI("PrepareInputBufferQueue enter");
    std::vector<std::shared_ptr<AVBuffer>> inputBuffers;
    MediaAVCodec::AVCodecTrace trace("MediaCodec::PrepareInputBufferQueue");
    CHECK_AND_RETURN_RET_LOG(codecPlugin_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "codecPlugin_ is nullptr");
    auto ret = codecPlugin_->GetInputBuffers(inputBuffers);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "pluign getInputBuffers failed");
    if (inputBuffers.empty()) {
        ret = AttachBufffer();
        if (ret != Status::OK) {
            AVCODEC_LOGE("GetParameter failed");
            return (int32_t)ret;
        }
    } else {
        if (inputBufferQueue_ == nullptr) {
            inputBufferQueue_ =
                AVBufferQueue::Create(inputBuffers.size(), MemoryType::HARDWARE_MEMORY, INPUT_BUFFER_QUEUE_NAME);
        }
        CHECK_AND_RETURN_RET_LOG(inputBufferQueue_ != nullptr, (int32_t)Status::ERROR_UNKNOWN,
                                 "inputBufferQueue_ is nullptr");
        inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
        for (uint32_t i = 0; i < inputBuffers.size(); i++) {
            inputBufferQueueProducer_->AttachBuffer(inputBuffers[i], false);
            inputBufferVector_.push_back(inputBuffers[i]);
        }
    }
    CHECK_AND_RETURN_RET_LOG(inputBufferQueue_ != nullptr, (int32_t)Status::ERROR_UNKNOWN,
                             "inputBufferQueue_ is nullptr");
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    sptr<IConsumerListener> listener = new InputBufferAvailableListener(this);
    CHECK_AND_RETURN_RET_LOG(inputBufferQueueConsumer_ != nullptr, (int32_t)Status::ERROR_UNKNOWN,
                             "inputBufferQueueConsumer_ is nullptr");
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    return (int32_t)ret;
}

int32_t MediaCodec::PrepareOutputBufferQueue()
{
    AVCODEC_LOGI("PrepareOutputBufferQueue enter");
    std::vector<std::shared_ptr<AVBuffer>> outputBuffers;
    MediaAVCodec::AVCodecTrace trace("MediaCodec::PrepareOutputBufferQueue");
    CHECK_AND_RETURN_RET_LOG(codecPlugin_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "codecPlugin_ is nullptr");
    auto ret = codecPlugin_->GetOutputBuffers(outputBuffers);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "GetOutputBuffers failed");
    CHECK_AND_RETURN_RET_LOG(outputBufferQueueProducer_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE,
                             "outputBufferQueueProducer_ is nullptr");
    if (outputBuffers.empty()) {
        int outputBufferNum = 30;
        std::shared_ptr<Meta> outputBufferConfig = std::make_shared<Meta>();
        ret = codecPlugin_->GetParameter(outputBufferConfig);
        CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "GetParameter failed");
        CHECK_AND_RETURN_RET_LOG(outputBufferConfig != nullptr, (int32_t)Status::ERROR_INVALID_STATE,
            "outputBufferConfig is nullptr");
        CHECK_AND_RETURN_RET_LOG(outputBufferConfig->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(outputBufferCapacity_),
            (int32_t)Status::ERROR_INVALID_PARAMETER, "get AUDIO_MAX_OUTPUT_SIZE failed");
        for (int i = 0; i < outputBufferNum; i++) {
            auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            std::shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputBufferCapacity_);
            CHECK_AND_RETURN_RET_LOG(outputBuffer != nullptr, (int32_t)Status::ERROR_INVALID_STATE,
                                     "outputBuffer is nullptr");
            if (outputBufferQueueProducer_->AttachBuffer(outputBuffer, false) == Status::OK) {
                AVCODEC_LOGD("Attach output buffer. index: %{public}d, bufferId: %{public}" PRIu64, i,
                            outputBuffer->GetUniqueId());
                outputBufferVector_.push_back(outputBuffer);
            }
        }
    } else {
        for (uint32_t i = 0; i < outputBuffers.size(); i++) {
            if (outputBufferQueueProducer_->AttachBuffer(outputBuffers[i], false) == Status::OK) {
                AVCODEC_LOGD("Attach output buffer. index: %{public}d, bufferId: %{public}" PRIu64, i,
                            outputBuffers[i]->GetUniqueId());
                outputBufferVector_.push_back(outputBuffers[i]);
            }
        }
    }
    CHECK_AND_RETURN_RET_LOG(outputBufferVector_.size() > 0, (int32_t)Status::ERROR_INVALID_STATE, "Attach no buffer");
    return (int32_t)ret;
}

void MediaCodec::ProcessInputBuffer()
{
    AVCODEC_LOGD("ProcessInputBuffer enter");
    MediaAVCodec::AVCodecTrace trace("MediaCodec::ProcessInputBuffer");
    Status ret;
    uint32_t eosStatus = 0;
    std::shared_ptr<AVBuffer> filledInputBuffer;
    if (state_ != CodecState::RUNNING) {
        AVCODEC_LOGE("status changed, current status is not running in ProcessInputBuffer");
        return;
    }
    ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
    if (ret != Status::OK) {
        AVCODEC_LOGE("ProcessInputBuffer AcquireBuffer fail");
        return;
    }
    if (state_ != CodecState::RUNNING) {
        AVCODEC_LOGD("ProcessInputBuffer ReleaseBuffer name:MediaCodecInputBufferQueue");
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        return;
    }
    const int8_t RETRY = 3; // max retry count is 3
    int8_t retryCount = 0;
    do {
        if (drmDecryptor_ != nullptr) {
            ret = DrmAudioCencDecrypt(filledInputBuffer);
            if (ret != Status::OK) {
                AVCODEC_LOGE("MediaCodec DrmAudioCencDecrypt failed.");
                break;
            }
        }

        ret = codecPlugin_->QueueInputBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            retryCount++;
            continue;
        }
    } while (ret != Status::OK && retryCount < RETRY);

    if (ret != Status::OK) {
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        AVCODEC_LOGE("Plugin queueInputBuffer failed.");
        return;
    }
    eosStatus = filledInputBuffer->flag_;
    do {
        ret = HandleOutputBuffer(eosStatus);
    } while (ret == Status::ERROR_AGAIN);
}

#ifdef SUPPORT_DRM
int32_t MediaCodec::SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
    AVCODEC_LOGI("MediaCodec::SetAudioDecryptionConfig");
    if (drmDecryptor_ == nullptr) {
        drmDecryptor_ = std::make_shared<MediaAVCodec::CodecDrmDecrypt>();
    }
    CHECK_AND_RETURN_RET_LOG(drmDecryptor_ != nullptr, (int32_t)Status::ERROR_NO_MEMORY, "drmDecryptor is nullptr");
    drmDecryptor_->SetDecryptionConfig(keySession, svpFlag);
    return (int32_t)Status::OK;
}
#else
int32_t MediaCodec::SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
    AVCODEC_LOGI("MediaCodec::SetAudioDecryptionConfig, Not support");
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
    if (codecPlugin_ != nullptr) {
        codecPlugin_->Release();
        codecPlugin_ = nullptr;
    }
    codecPlugin_ = CreatePlugin(mime, type);
    if (codecPlugin_ != nullptr) {
        ret = codecPlugin_->SetParameter(meta);
        AVCODEC_LOGI("codecPlugin SetParameter ret %{public}d", ret);
        ret = codecPlugin_->Init();
        AVCODEC_LOGI("codecPlugin Init ret %{public}d", ret);
        ret = codecPlugin_->SetDataCallback(this);
        AVCODEC_LOGI("codecPlugin SetDataCallback ret %{public}d", ret);
        PrepareInputBufferQueue();
        PrepareOutputBufferQueue();
        if (state_ == CodecState::RUNNING) {
            ret = codecPlugin_->Start();
            AVCODEC_LOGI("codecPlugin Start ret %{public}d", ret);
        }
    } else {
        AVCODEC_LOGI("createPlugin failed");
        return Status::ERROR_INVALID_PARAMETER;
    }
    return ret;
}

Status MediaCodec::HandleOutputBuffer(uint32_t eosStatus)
{
    AVCODEC_LOGD("HandleOutputBuffer enter");
    Status ret = Status::OK;
    std::shared_ptr<AVBuffer> emptyOutputBuffer;
    AVBufferConfig avBufferConfig;
    do {
        ret = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
    } while (ret != Status::OK && state_ == CodecState::RUNNING);

    if (emptyOutputBuffer) {
        emptyOutputBuffer->flag_ = eosStatus;
    } else if (state_ != CodecState::RUNNING) {
        return Status::OK;
    } else {
        return Status::ERROR_NULL_POINTER;
    }
    ret = codecPlugin_->QueueOutputBuffer(emptyOutputBuffer);
    if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
        AVCODEC_LOGD("QueueOutputBuffer ERROR_NOT_ENOUGH_DATA");
        outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, false);
    } else if (ret == Status::ERROR_AGAIN) {
        AVCODEC_LOGD("The output data is not completely read, needs to be read again");
    } else if (ret == Status::END_OF_STREAM) {
        AVCODEC_LOGD("HandleOutputBuffer END_OF_STREAM");
    } else if (ret != Status::OK) {
        AVCODEC_LOGE("QueueOutputBuffer error");
        outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, false);
    }
    return ret;
}

void MediaCodec::OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    MediaAVCodec::AVCodecTrace trace("MediaCodec::OnInputBufferDone");
    Status ret = inputBufferQueueConsumer_->ReleaseBuffer(inputBuffer);
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " OnInputBufferDone, buffer->pts %{public}lld",
        FAKE_POINTER(this), inputBuffer->pts_);
    CHECK_AND_RETURN_LOG(ret == Status::OK, "OnInputBufferDone fail");
}

void MediaCodec::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer)
{
    MediaAVCodec::AVCodecTrace trace("MediaCodec::OnOutputBufferDone");
    if (isDump_) {
        DumpAVBufferToFile(DUMP_PARAM, dumpPrefix_ + DUMP_FILE_NAME, outputBuffer);
    }
    Status ret = outputBufferQueueProducer_->PushBuffer(outputBuffer, true);
    if (mediaCodecCallback_) {
        mediaCodecCallback_->OnOutputBufferDone(outputBuffer);
    }
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " OnOutputBufferDone, buffer->pts %{public}lld",
        FAKE_POINTER(this), outputBuffer->pts_);
    CHECK_AND_RETURN_LOG(ret == Status::OK, "OnOutputBufferDone fail");
}

void MediaCodec::ClearBufferQueue()
{
    AVCODEC_LOGI("ClearBufferQueue called.");
    if (inputBufferQueueProducer_ != nullptr) {
        for (auto &buffer : inputBufferVector_) {
            inputBufferQueueProducer_->DetachBuffer(buffer);
        }
        inputBufferVector_.clear();
        inputBufferQueueProducer_->SetQueueSize(0);
    }
    if (outputBufferQueueProducer_ != nullptr) {
        for (auto &buffer : outputBufferVector_) {
            outputBufferQueueProducer_->DetachBuffer(buffer);
        }
        outputBufferVector_.clear();
        outputBufferQueueProducer_->SetQueueSize(0);
    }
}

void MediaCodec::ClearInputBuffer()
{
    MediaAVCodec::AVCodecTrace trace("MediaCodec::ClearInputBuffer");
    AVCODEC_LOGD("ClearInputBuffer enter");
    if (!inputBufferQueueConsumer_) {
        return;
    }
    std::shared_ptr<AVBuffer> filledInputBuffer;
    Status ret = Status::OK;
    while (ret == Status::OK) {
        ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            AVCODEC_LOGI("clear input Buffer");
            return;
        }
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
    }
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

void MediaCodec::OnDumpInfo(int32_t fd)
{
    AVCODEC_LOGD("MediaCodec::OnDumpInfo called.");
    if (fd < 0) {
        AVCODEC_LOGE("MediaCodec::OnDumpInfo fd is invalid.");
        return;
    }
    std::string dumpString;
    dumpString += "MediaCodec plugin name: " + codecPluginName_ + "\n";
    dumpString += "MediaCodec buffer size is:" + std::to_string(inputBufferQueue_->GetQueueSize()) + "\n";
    int ret = write(fd, dumpString.c_str(), dumpString.size());
    if (ret < 0) {
        AVCODEC_LOGE("MediaCodec::OnDumpInfo write failed.");
        return;
    }
}
} // namespace Media
} // namespace OHOS
