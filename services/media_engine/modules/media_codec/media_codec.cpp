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
#include <iostream>
#include <sstream>
#include <ctime>
#include <fstream>
#include "avcodec_common.h"
#include "osal/task/autolock.h"
#include "plugin/plugin_manager_v2.h"
#include "avcodec_log.h"
#include "osal/utils/dump_buffer.h"
#include "avcodec_trace.h"
#include "bundle_mgr_interface.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "param_wrapper.h"
#include "access_token.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "tokenid_kit.h"

#ifdef SUPPORT_DRM
#include "imedia_key_session_service.h"
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
constexpr uint32_t LOG_LOW_FREQUENCY = 100;
const std::string DUMP_PARAM = "a";
const std::string DUMP_FILE_NAME = "player_audio_decoder_output.pcm";
const std::string DUMP_IO_LABLE = "sys.media.AVCodec.dump.enable";
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
            AVCODEC_LOGW("mediacodec was released, can not process input buffer");
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
      state_(CodecState::UNINITIALIZED),
      inputBytesSum_(0),
      outputBytesSum_(0),
      inputCount_(0),
      outputCount_(0)
{
}

MediaCodec::~MediaCodec()
{
    ResetIOStat();
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
    OHOS::system::GetStringParameter(DUMP_IO_LABLE, dumpIOEnable, "false");
    AVCODEC_LOGI("Init enter, mime:%{public}s", mime.c_str());
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
        state_ = CodecState::UNINITIALIZED;
        return (int32_t)Status::ERROR_INVALID_PARAMETER;
    }
    return (int32_t)Status::OK;
}

int32_t MediaCodec::Init(const std::string &name)
{
    AutoLock lock(stateMutex_);
    OHOS::system::GetStringParameter(DUMP_IO_LABLE, dumpIOEnable, "false");
    AVCODEC_LOGI("Init enter, name:%{public}s", name.c_str());
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Init");
    if (state_ != CodecState::UNINITIALIZED) {
        AVCODEC_LOGE("Init failed, state = %{public}s .", StateToString(state_).data());
        return (int32_t)Status::ERROR_INVALID_STATE;
    }
    AVCODEC_LOGI("state from %{public}s to INITIALIZING", StateToString(state_).data());
    state_ = CodecState::INITIALIZING;
    auto plugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(name);
    if (plugin == nullptr) {
        AVCODEC_LOGE("create pluign failed");
        state_ = CodecState::UNINITIALIZED;
        return (int32_t)Status::ERROR_INVALID_PARAMETER;
    }
    codecPlugin_ = std::reinterpret_pointer_cast<Plugins::CodecPlugin>(plugin);
    if (codecPlugin_->Init() != Status::OK) {
        AVCODEC_LOGE("pluign init failed");
        state_ = CodecState::UNINITIALIZED;
        return (int32_t)Status::ERROR_INVALID_PARAMETER;
    }
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

// LCOV_EXCL_START
void MediaCodec::IODump(const std::shared_ptr<Meta> &meta)
{
    std::time_t t = std::time(nullptr);
    std::tm *tm = std::localtime(&t);
    if (!tm) {
        return;
    }

    std::ostringstream common;
    common << "/data/media/";
    common << std::put_time(tm, "%H%M%S");
    common << "_" << reinterpret_cast<void*>(this);

    int32_t channels = 0;
    int32_t sampleRate = 0;
    meta->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    common << "_" << sampleRate;
    meta->GetData(Tag::AUDIO_CHANNEL_COUNT, channels);
    common << "_" << channels;

    std::ostringstream inputStr;
    std::ostringstream outputStr;
    inputStr << common.str() << "_input.bin";
    outputStr << common.str() << "_output.bin";

    dumpDataInputFs_ = std::make_shared<std::ofstream>(inputStr.str(), std::ios::binary);
    dumpDataOutputFs_ = std::make_shared<std::ofstream>(outputStr.str(), std::ios::binary);
}
// LCOV_EXCL_STOP

int32_t MediaCodec::Configure(const std::shared_ptr<Meta> &meta)
{
    AVCODEC_LOGI("MediaCodec::configure in");
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Configure");
    if (dumpIOEnable == "true") {
        IODump(meta);
        AVCODEC_LOGI("IODump in");
    }
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED, (int32_t)Status::ERROR_INVALID_STATE,
                             "state != inited");
    auto ret = codecPlugin_->SetParameter(meta);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "ret != ok");
    ret = codecPlugin_->SetDataCallback(this);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "ret != ok");
    state_ = CodecState::CONFIGURED;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer)
{
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::SetOutputBufferQueue");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
        (int32_t)Status::ERROR_INVALID_STATE, "state invalid");
    outputBufferQueueProducer_ = bufferQueueProducer;
    isBufferMode_ = true;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetCodecCallback(const std::shared_ptr<CodecCallback> &codecCallback)
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
        (int32_t)Status::ERROR_INVALID_STATE, "state invalid");
    CHECK_AND_RETURN_RET_LOG(codecCallback != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER,
        "codecCallback is nullptr");
    codecCallback_ = codecCallback;
    auto ret = codecPlugin_->SetDataCallback(this);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "ret != ok");
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetCodecCallback(const std::shared_ptr<AudioBaseCodecCallback> &codecCallback)
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
        (int32_t)Status::ERROR_INVALID_STATE, "state invalid");
    CHECK_AND_RETURN_RET_LOG(codecCallback != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER,
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
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED,
        (int32_t)Status::ERROR_INVALID_STATE, "state invalid");
    isSurfaceMode_ = true;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::Prepare()
{
    AVCODEC_LOGI("Prepare enter");
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Prepare");
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::FLUSHED, (int32_t)Status::ERROR_AGAIN,
        "state is flushed, no need prepare");
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::PREPARED, (int32_t)Status::OK, "already prepared");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::CONFIGURED, (int32_t)Status::ERROR_INVALID_STATE, "state invalid");
    if (isBufferMode_ && isSurfaceMode_) {
        AVCODEC_LOGE("state error");
        return (int32_t)Status::ERROR_UNKNOWN;
    }
    outputBufferCapacity_ = 0;
    auto ret = (int32_t)PrepareInputBufferQueue();
    CHECK_AND_RETURN_RET_LOG(ret == (int32_t)Status::OK, (int32_t)ret, "PrepareInputBufferQueue failed");
    ret = (int32_t)PrepareOutputBufferQueue();
    CHECK_AND_RETURN_RET_LOG(ret == (int32_t)Status::OK, (int32_t)ret, "PrepareOutputBufferQueue failed");
    state_ = CodecState::PREPARED;
    AVCODEC_LOGI("Prepare, ret = %{public}d", (int32_t)ret);
    return (int32_t)Status::OK;
}

sptr<AVBufferQueueProducer> MediaCodec::GetInputBufferQueue()
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::PREPARED, sptr<AVBufferQueueProducer>(), "state invalid");
    CHECK_AND_RETURN_RET_LOG(!isSurfaceMode_, nullptr, "GetInputBufferQueue isSurfaceMode_");
    isBufferMode_ = true;
    return inputBufferQueueProducer_;
}

sptr<AVBufferQueueConsumer> MediaCodec::GetInputBufferQueueConsumer()
{
    AutoLock lock(stateMutex_);
    // Case: to enable to set listener to input bufferqueue consumer. i.e. input bufferqueue updating by ChangePlugin.
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::PREPARED || state_ == CodecState::RUNNING ||
        state_ == CodecState::FLUSHED || state_ == CodecState::END_OF_STREAM,
        sptr<AVBufferQueueConsumer>(), "state invalid");
    CHECK_AND_RETURN_RET_LOG(!isSurfaceMode_, nullptr, "GetInputBufferQueueConsumer isSurfaceMode_");
    isBufferMode_ = true;
    return inputBufferQueueConsumer_;
}

sptr<AVBufferQueueProducer> MediaCodec::GetOutputBufferQueueProducer()
{
    AutoLock lock(stateMutex_);
    // Case: to enable to set listener to output bufferqueue producer. i.e. output bufferqueue updating by ChangePlugin.
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::PREPARED || state_ == CodecState::RUNNING ||
        state_ == CodecState::FLUSHED || state_ == CodecState::END_OF_STREAM,
        sptr<AVBufferQueueProducer>(), "state invalid");
    CHECK_AND_RETURN_RET_LOG(!isSurfaceMode_, nullptr, "GetOutputBufferQueueProducer isSurfaceMode_");

    return outputBufferQueueProducer_;
}

void MediaCodec::ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed)
{
    uint32_t bufferStatus = static_cast<uint32_t>(InOutPortBufferStatus::INIT_IGNORE_RET);
    ProcessInputBufferInner(isTriggeredByOutPort, isFlushed, bufferStatus);
}

void MediaCodec::ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed, uint32_t &bufferStatus)
{
    CHECK_AND_RETURN_LOGD(inputBufferQueueConsumer_ != nullptr, "inputBufferQueueConsumer is nullptr!");
    uint32_t filledBufferSize = inputBufferQueueConsumer_->GetFilledBufferSize();
    uint32_t eosStatus = inputBufferEosStatus_.load();
    bool isOutAvail = isOutputBufferAvailable_.load();
    bufferStatus = static_cast<uint32_t>(InOutPortBufferStatus::INIT_IGNORE_RET);
    MediaAVCodec::AVCodecTrace trace(std::string("MediaCodec::ProcessInputBufferInner:") +
        (isTriggeredByOutPort ? "1" : "0") + "," + (isFlushed ? "1" : "0") + "," + std::to_string(filledBufferSize) +
        "," + (isOutAvail ? "1" : "0") + "," + std::to_string(eosStatus));
    AVCODEC_LOGD("ProcessInputBufferInnerr isTriggeredByOutPort:%{public}" PRId32 ", isFlushed:%{public}" PRId32
        ", isOutAvail:%{public}" PRId32 ", filledBufferSize:%{public}" PRIu32 ", eosStatus:%{public}" PRIu32,
        isTriggeredByOutPort, isFlushed, isOutAvail, filledBufferSize, eosStatus);
    CHECK_AND_RETURN_LOGD(!isFlushed, "ProcessInputBufferInner isFlushed true");
    if (filledBufferSize == 0 && isOutputBufferAvailable_.load() && eosStatus == 0) {
        bufferStatus = static_cast<uint32_t>(InOutPortBufferStatus::OUTPORT_AVAIL);
        AVCODEC_LOGI_LIMIT(LOG_LOW_FREQUENCY, "ProcessInputBufferInner ignore");
        return;
    }

    Status ret = Status::OK;
    // The last process failed to RequestBuffer from outputBufferQueueProducer, perform HandleOutputBufferInner firstly.
    if (!isOutAvail || eosStatus != 0) {
        CHECK_AND_RETURN_LOGD(HandleOutputBufferInner(ret, bufferStatus, filledBufferSize, eosStatus),
            "HandleOutputBufferInner S1, ret:%{public}d", static_cast<int32_t>(ret));
        inputBufferEosStatus_.store(0);
    }

    bool isProcessingNeeded = false;
    eosStatus = 0;
    ret = Status::OK;
    HandleInputBufferInner(eosStatus, isProcessingNeeded, ret);
    filledBufferSize = inputBufferQueueConsumer_->GetFilledBufferSize();
    if (!isProcessingNeeded) {
        if (bufferStatus != static_cast<uint32_t>(InOutPortBufferStatus::INIT_IGNORE_RET)) {
            filledBufferSize > 0 ? (bufferStatus |= static_cast<uint32_t>(InOutPortBufferStatus::INPORT_AVAIL)) :
                (bufferStatus &= ~static_cast<uint32_t>(InOutPortBufferStatus::INPORT_AVAIL));
        } else {
            // Normally this case happen with low probability, assuming OutputBuffer available not make negative effect
            bufferStatus = static_cast<uint32_t>(InOutPortBufferStatus::OUTPORT_AVAIL) |
                (filledBufferSize > 0 ? static_cast<uint32_t>(InOutPortBufferStatus::INPORT_AVAIL) : 0);
        }
        AVCODEC_LOGI("HandleInputBufferInner failed, ret:%{public}d, bufferStatus:0x%{public}x",
            static_cast<int32_t>(ret), bufferStatus);
        return;
    }

    inputBufferEosStatus_.store(eosStatus);
    CHECK_AND_RETURN_LOGD(HandleOutputBufferInner(ret, bufferStatus, filledBufferSize, eosStatus),
        "HandleOutputBufferInner S2, ret:%{public}d", static_cast<int32_t>(ret));
    inputBufferEosStatus_.store(0);
}

bool MediaCodec::HandleOutputBufferInner(Status &ret, uint32_t &bufferStatus, uint32_t filledBufferSize,
    uint32_t eosStatus)
{
    MEDIA_TRACE_DEBUG(std::string("MediaCodec::HandleOutputBufferInner:") + std::to_string(eosStatus));
    bool isBufferAvailable = false;
    do {
        isBufferAvailable = false;
        ret = HandleOutputBufferOnce(isBufferAvailable, eosStatus, false);
    } while (ret == Status::ERROR_AGAIN);

    isOutputBufferAvailable_.store(isBufferAvailable);

    bool isGoingOn = isBufferAvailable;
    bufferStatus = (filledBufferSize > 0) ? static_cast<uint32_t>(InOutPortBufferStatus::INPORT_AVAIL) : 0;
    if (!isBufferAvailable) {
        bufferStatus |= (eosStatus == MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS) ?
            static_cast<uint32_t>(InOutPortBufferStatus::OUT_EOS_START) : bufferStatus;
    } else {
        bufferStatus |= static_cast<uint32_t>(InOutPortBufferStatus::OUTPORT_AVAIL);
        if (eosStatus == MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS) {
            bufferStatus |= static_cast<uint32_t>(InOutPortBufferStatus::OUT_EOS_START) |
                static_cast<uint32_t>(InOutPortBufferStatus::OUT_EOS_DONE);
            inputBufferEosStatus_.store(0);
            isGoingOn = false;
            AVCODEC_LOGI("HandleOutputBufferInner OUT_EOS_DONE");
        }
    }

    AVCODEC_LOGD("HandleOutputBufferInner ret:%{public}" PRId32 ", isGoingOn:%{public}" PRId32
        ", isBufferAvailable:%{public}" PRId32 ", eosStatus:%{public}" PRIu32 ", bufferStatus:0x%{public}" PRIx32,
        static_cast<int32_t>(ret), isGoingOn, isBufferAvailable, eosStatus, bufferStatus);

    return isGoingOn;
}

void MediaCodec::ResetBufferStatusInfo()
{
    if (cachedOutputBuffer_ && outputBufferQueueProducer_) {
        outputBufferQueueProducer_->PushBuffer(cachedOutputBuffer_, false);
        cachedOutputBuffer_ = nullptr;
    }
    inputBufferEosStatus_.store(0);
    isOutputBufferAvailable_.store(true);
}

sptr<Surface> MediaCodec::GetInputSurface()
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::PREPARED, nullptr, "state invalid");
    CHECK_AND_RETURN_RET_LOG(!isBufferMode_, nullptr, "GetInputBufferQueue isBufferMode_");
    isSurfaceMode_ = true;
    return nullptr;
}

int32_t MediaCodec::Start()
{
    AutoLock lock(stateMutex_);
    AVCODEC_LOGI("Start enter");
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Start");
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::RUNNING, (int32_t)Status::OK, "already running");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::PREPARED || state_ == CodecState::FLUSHED,
        (int32_t)Status::ERROR_INVALID_STATE, "state invalid");
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
    CHECK_AND_RETURN_RET_LOG(codecPlugin_ != nullptr, (int32_t)Status::OK, "codecPlugin_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::PREPARED, (int32_t)Status::OK, "already stop");
    if (state_ == CodecState::UNINITIALIZED || state_ == CodecState::STOPPING || state_ == CodecState::RELEASING) {
        AVCODEC_LOGD("Stop, state_=%{public}s", StateToString(state_).data());
        return (int32_t)Status::OK;
    }
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM ||
        state_ == CodecState::FLUSHED, (int32_t)Status::ERROR_INVALID_STATE, "state invalid");
    ResetIOStat();
    state_ = CodecState::STOPPING;
    auto ret = codecPlugin_->Stop();
    AVCODEC_LOGI("codec Stop, state from %{public}s to Stop", StateToString(state_).data());
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin stop failed");
    ClearInputBuffer();
    ResetBufferStatusInfo();
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
    ResetIOStat();
    state_ = CodecState::FLUSHING;
    inputBufferQueueProducer_->Clear();
    auto ret = codecPlugin_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin flush failed");
    ClearInputBuffer();
    ResetBufferStatusInfo();
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
    ResetBufferStatusInfo();
    ClearBufferQueue();
    // LCOV_EXCL_START
    if (dumpDataInputFs_ != nullptr && dumpDataInputFs_->is_open()) {
        dumpDataInputFs_->flush();
        dumpDataInputFs_->close();
    }
    if (dumpDataOutputFs_ != nullptr && dumpDataOutputFs_->is_open()) {
        dumpDataOutputFs_->flush();
        dumpDataOutputFs_->close();
    }
    // LCOV_EXCL_STOP
    ResetIOStat();
    state_ = CodecState::INITIALIZED;
    return (int32_t)ret;
}

int32_t MediaCodec::Release()
{
    AutoLock lock(stateMutex_);
    MediaAVCodec::AVCodecTrace trace("MediaCodec::Release");
    AVCODEC_LOGI("Release enter");
    CHECK_AND_RETURN_RET_LOG(codecPlugin_ != nullptr, (int32_t)Status::OK, "codecPlugin_ is nullptr");
    if (state_ == CodecState::UNINITIALIZED || state_ == CodecState::RELEASING) {
        AVCODEC_LOGW("codec Release, state is not correct, state =%{public}s .", StateToString(state_).data());
        return (int32_t)Status::OK;
    }

    if (state_ == CodecState::INITIALIZING) {
        AVCODEC_LOGW("codec Release, state is not correct, state =%{public}s .", StateToString(state_).data());
        state_ = CodecState::RELEASING;
        return (int32_t)Status::OK;
    }
    AVCODEC_LOGI("codec Release, state from %{public}s to RELEASING", StateToString(state_).data());
    ResetIOStat();
    state_ = CodecState::RELEASING;
    auto ret = codecPlugin_->Release();
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin release failed");
    codecPlugin_ = nullptr;
    ResetBufferStatusInfo();
    ClearBufferQueue();
    // LCOV_EXCL_START
    if (dumpDataInputFs_ != nullptr && dumpDataInputFs_->is_open()) {
        dumpDataInputFs_->flush();
        dumpDataInputFs_->close();
    }
    if (dumpDataOutputFs_ != nullptr && dumpDataOutputFs_->is_open()) {
        dumpDataOutputFs_->flush();
        dumpDataOutputFs_->close();
    }
    // LCOV_EXCL_STOP
    state_ = CodecState::UNINITIALIZED;
    return (int32_t)ret;
}

int32_t MediaCodec::NotifyEos()
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::END_OF_STREAM, (int32_t)Status::OK, "already eos");
    CHECK_AND_RETURN_RET_LOG(state_ == CodecState::RUNNING, (int32_t)Status::ERROR_INVALID_STATE, "state invalid");
    state_ = CodecState::END_OF_STREAM;
    return (int32_t)Status::OK;
}

int32_t MediaCodec::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(parameter != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER, "input nullptr");
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::UNINITIALIZED && state_ != CodecState::INITIALIZED &&
        state_ != CodecState::PREPARED, (int32_t)Status::ERROR_INVALID_STATE, "state invalid");
    auto ret = codecPlugin_->SetParameter(parameter);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, (int32_t)ret, "plugin set parameter failed");
    return (int32_t)ret;
}

void MediaCodec::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    (void)instanceId;
    auto tid = gettid();
    CHECK_AND_RETURN_LOGW(!isDump || tid > 0, "MediaCodec Can not dump: gettid failed");
    dumpPrefix_ = std::to_string(tid);
    isDump_ = isDump;
}

int32_t MediaCodec::GetOutputFormat(std::shared_ptr<Meta> &parameter)
{
    AutoLock lock(stateMutex_);
    CHECK_AND_RETURN_RET_LOG(state_ != CodecState::UNINITIALIZED, (int32_t)Status::ERROR_INVALID_STATE,
                             "status incorrect,get output format failed.");
    CHECK_AND_RETURN_RET_LOG(codecPlugin_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "plugin nullptr");
    CHECK_AND_RETURN_RET_LOG(parameter != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER, "input nullptr");
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
    CHECK_AND_RETURN_RET_LOG(inputBufferConfig != nullptr, Status::ERROR_UNKNOWN, "inputBufferConfig is nullptr");
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
        CHECK_AND_RETURN_RET_LOG(inputBuffer != nullptr, Status::ERROR_UNKNOWN,
            "inputBuffer is nullptr");
        CHECK_AND_RETURN_RET_LOG(inputBufferQueueProducer_ != nullptr, Status::ERROR_UNKNOWN,
            "inputBufferQueueProducer_ is nullptr");
        inputBufferQueueProducer_->AttachBuffer(inputBuffer, false);
        AVCODEC_LOGI("Attach intput buffer. index: %{public}d, bufferId: %{public}" PRIu64,
            i, inputBuffer->GetUniqueId());
        inputBufferVector_.push_back(inputBuffer);
    }
    return Status::OK;
}

// LCOV_EXCL_START
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
    CHECK_AND_RETURN_RET_LOG(drmRes == 0, Status::ERROR_DRM_DECRYPT_FAILED, "DrmAudioCencDecrypt return error");

    // 5. copy decrypted data from drm output buffer back
    drmRes = memcpy_s(filledInputBuffer->memory_->GetAddr(), bufSize,
        drmOutBuf->memory_->GetAddr(), bufSize);
    CHECK_AND_RETURN_RET_LOG(drmRes == 0, Status::ERROR_UNKNOWN, "memcpy_s drmOutBuf failed");
    return Status::OK;
}

void MediaCodec::HandleAudioCencDecryptError()
{
    AVCODEC_LOGE("MediaCodec DrmAudioCencDecrypt failed.");
    auto realPtr = mediaCodecCallback_.lock();
    if (realPtr != nullptr) {
        realPtr->OnError(CodecErrorType::CODEC_DRM_DECRYTION_FAILED,
            static_cast<int32_t>(Status::ERROR_DRM_DECRYPT_FAILED));
    }
}
// LCOV_EXCL_STOP

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
    sptr<IConsumerListener> listener = new InputBufferAvailableListener(shared_from_this());
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
                                 (int32_t)Status::ERROR_INVALID_PARAMETER, "get AUDIO_MAX_OUTPUT_SIZE fail");
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
    MEDIA_TRACE_DEBUG("MediaCodec::ProcessInputBuffer");

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
    MEDIA_TRACE_DEBUG_POSTFIX("MediaCodec::HandleInputBufferInner", "1");

    std::shared_ptr<AVBuffer> filledInputBuffer;
    if (state_ != CodecState::RUNNING) {
        AVCODEC_LOGI("HandleInputBufferInner status changed, current status is not running");
        ret = Status::ERROR_INVALID_STATE;
        return;
    }
    {
        MEDIA_TRACE_DEBUG_POSTFIX("MediaCodec::HandleInputBufferInner-AcquireBuffer", "2");
        ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
        CHECK_AND_RETURN_LOG(ret == Status::OK && filledInputBuffer, "HandleInputBufferInner AcquireBuffer fail");
    }
    if (state_ != CodecState::RUNNING) {
        AVCODEC_LOGW("HandleInputBufferInner not running, ReleaseBuffer name:MediaCodecInputBufferQueue");
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
        ret = Status::ERROR_INVALID_STATE;
        return;
    }

    if (!filledInputBuffer || !filledInputBuffer->memory_ || !filledInputBuffer->memory_->GetAddr()) {
        ret = Status::ERROR_NULL_POINTER;
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
        AVCODEC_LOGE("Plugin queueInputBuffer failed, state_:%{public}d, ret:%{public}d", state_.load(), ret);
        return;
    }
    isProcessingNeeded = true;
    eosStatus = flag;
}

// LCOV_EXCL_START
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
// LCOV_EXCL_STOP

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
        if (ret != Status::OK) {
            AVCODEC_LOGE("codecPlugin SetParameter ret %{public}d", ret);
            return ret;
        }
        ret = codecPlugin_->Init();
        if (ret != Status::OK) {
            AVCODEC_LOGE("codecPlugin Init ret %{public}d", ret);
            return ret;
        }
        ret = codecPlugin_->SetDataCallback(this);
        if (ret != Status::OK) {
            AVCODEC_LOGE("codecPlugin SetDataCallback ret %{public}d", ret);
            return ret;
        }
    }

    // discard undecoded data and unconsumed decoded data.
    if (inputBufferQueueProducer_) {
        inputBufferQueueProducer_->Clear();
    }

    CHECK_AND_RETURN_RET_LOG(inputBufferQueue_ != nullptr, Status::ERROR_UNKNOWN, "inputBufferQueue_ is nullptr");
    ClearInputBuffer();
    ResetBufferStatusInfo();

    AutoLock lock(stateMutex_);
    PrepareInputBufferQueue();
    PrepareOutputBufferQueue();
    if (state_ == CodecState::RUNNING) {
        ret = codecPlugin_->Start();
        if (ret != Status::OK) {
            AVCODEC_LOGE("codecPlugin Start ret %{public}d", ret);
        }
    }
    ResetIOStat();

    return ret;
}

Status MediaCodec::HandleOutputBuffer(uint32_t eosStatus)
{
    AVCODEC_LOGD("HandleOutputBuffer enter");
    bool isBufferAvailable = false;
    Status ret = HandleOutputBufferOnce(isBufferAvailable, eosStatus, true);
    AVCODEC_LOGD("HandleOutputBuffer ret:%{public}d, isBufferAvailable:%{public}d", ret, isBufferAvailable);
    return ret;
}

Status MediaCodec::HandleOutputBufferOnce(bool &isBufferAvailable, uint32_t eosStatus, bool isSync)
{
    MEDIA_TRACE_DEBUG_POSTFIX("MediaCodec::HandleOutputBufferOnce-isSync:" + std::to_string(isSync), "1");
    Status ret = Status::OK;
    std::shared_ptr<AVBuffer> emptyOutputBuffer;
    AVBufferConfig avBufferConfig;
    if (cachedOutputBuffer_) {
        std::swap(emptyOutputBuffer, cachedOutputBuffer_);
    } else {
        if (isSync) {
            MediaAVCodec::AVCodecTrace traceRequestBuffer("MediaCodec::HandleOutputBufferOnce-RequestBuffer-sync");
            do {
                ret = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
            } while (ret != Status::OK && state_ == CodecState::RUNNING);
        } else {
            MEDIA_TRACE_DEBUG_POSTFIX("MediaCodec::HandleOutputBufferOnce-RequestBuffer-async", "2");
            ret = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS_INNER);
        }
    }

    if (emptyOutputBuffer && emptyOutputBuffer->memory_ && emptyOutputBuffer->memory_->GetAddr()) {
        emptyOutputBuffer->flag_ = eosStatus;
        isBufferAvailable = true;
    } else if (state_ != CodecState::RUNNING) {
        return Status::OK;
    } else {
        return Status::ERROR_NULL_POINTER;
    }

    ret = CodePluginOutputBuffer(emptyOutputBuffer);
    if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
        AVCODEC_LOGD("HandleOutputBufferOnce QueueOutputBuffer ERROR_NOT_ENOUGH_DATA");
        // To cache the empty OutputBuffer returned by RequestBuffer to improve performance
        std::swap(emptyOutputBuffer, cachedOutputBuffer_);
    } else if (ret == Status::ERROR_AGAIN) {
        AVCODEC_LOGD("HandleOutputBufferOnce The output data is not completely read, needs to be read again");
    } else if (ret == Status::END_OF_STREAM) {
        AVCODEC_LOGI("HandleOutputBufferOnce QueueOutputBuffer END_OF_STREAM");
    } else if (ret != Status::OK) {
        AVCODEC_LOGE("HandleOutputBufferOnce QueueOutputBuffer error");
        outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, false);
    }
    return ret;
}

void MediaCodec::OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    CHECK_AND_RETURN_LOG(inputBuffer != nullptr, "OnInputBufferDone fail, inputBuffer nullptr");
    MediaAVCodec::AVCodecTrace trace(("MediaCodec::OnInputBufferDone:") +
        std::to_string(inputBuffer->flag_) + "," + std::to_string(inputBuffer->pts_) +
        "," + std::to_string(inputBuffer->duration_));
    if (dumpIOEnable == "true" && dumpDataInputFs_) {
        if (dumpDataInputFs_->is_open() && inputBuffer->memory_->GetAddr()) {
            AVCODEC_LOGD("dumpIOE writing");
            dumpDataInputFs_->write(reinterpret_cast<const char*>(inputBuffer->memory_->GetAddr() +
                                    inputBuffer->memory_->GetOffset()), inputBuffer->memory_->GetSize());
        }
    }
    Status ret = inputBufferQueueConsumer_->ReleaseBuffer(inputBuffer);
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " OnInputBufferDone, buffer->pts:%{public}" PRId64,
        FAKE_POINTER(this), inputBuffer->pts_);
    CHECK_AND_RETURN_LOG(ret == Status::OK, "OnInputBufferDone fail");
}

void MediaCodec::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer)
{
    CHECK_AND_RETURN_LOG(outputBuffer != nullptr, "OnOutputBufferDone fail, outputBuffer nullptr");
    MediaAVCodec::AVCodecTrace trace(("MediaCodec::OnOutputBufferDone:") +
        std::to_string(outputBuffer->flag_) + "," + std::to_string(outputBuffer->pts_) +
        "," + std::to_string(outputBuffer->duration_));
    // LCOV_EXCL_START
    if (isDump_) {
        DumpAVBufferToFile(DUMP_PARAM, dumpPrefix_ + DUMP_FILE_NAME, outputBuffer);
    }
    if (dumpIOEnable == "true" && dumpDataOutputFs_) {
        if (dumpDataOutputFs_->is_open() && outputBuffer->memory_->GetAddr()) {
            AVCODEC_LOGD("dumpIOE writing");
            dumpDataOutputFs_->write(reinterpret_cast<const char*>(outputBuffer->memory_->GetAddr() +
                                     outputBuffer->memory_->GetOffset()), outputBuffer->memory_->GetSize());
        }
    }
    // LCOV_EXCL_STOP
    Status ret = outputBufferQueueProducer_->PushBuffer(outputBuffer, true);
    auto realPtr = mediaCodecCallback_.lock();
    if (realPtr != nullptr) {
        realPtr->OnOutputBufferDone(outputBuffer);
    }
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " OnOutputBufferDone, buffer->pts:%{public}" PRId64,
        FAKE_POINTER(this), outputBuffer->pts_);
    CHECK_AND_RETURN_LOG(ret == Status::OK, "OnOutputBufferDone fail");
}

void MediaCodec::ClearBufferQueue()
{
    AVCODEC_LOGI("ClearBufferQueue called.");
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
    inputBufferQueue_ = nullptr;
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

void MediaCodec::OnEvent(const std::shared_ptr<Plugins::PluginEvent> event)
{
    if (event->type != Plugins::PluginEventType::AUDIO_OUTPUT_FORMAT_CHANGED) {
        return;
    }

    if (!isSupportAudioFormatChanged_) {
        AVCODEC_LOGW("receive audio format changed but api version is low");
        return;
    }

    auto realPtr = mediaCodecCallback_.lock();
    if (realPtr != nullptr) {
        std::shared_ptr<Meta> format = std::make_shared<Meta>(AnyCast<Meta>(event->param));
        realPtr->OnOutputFormatChanged(format);
    } else {
        AVCODEC_LOGE("receive AUDIO_OUTPUT_FORMAT_CHANGED, but lock callback fail");
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

// LCOV_EXCL_START
uint32_t MediaCodec::GetApiVersion()
{
    uint32_t apiVersion = INVALID_API_VERSION;
    using namespace OHOS::Security::AccessToken;
    AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(callerToken);
    if (tokenType != TOKEN_HAP) {
        return apiVersion;
    }
    OHOS::sptr<OHOS::ISystemAbilityManager> systemAbilityManager =
        OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    OHOS::sptr<OHOS::IRemoteObject> remoteObject =
        systemAbilityManager->CheckSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    sptr<AppExecFwk::IBundleMgr> iBundleMgr = OHOS::iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    if (iBundleMgr == nullptr) {
        AVCODEC_LOGW("GetApiVersion IBundleMgr is nullptr");
        return apiVersion;
    }
    AppExecFwk::BundleInfo bundleInfo;
    if (iBundleMgr->GetBundleInfoForSelf(0, bundleInfo) == ERR_OK) {
        apiVersion = bundleInfo.targetVersion % API_VERSION_MOD;
        AVCODEC_LOGI("GetApiVersion targetVersion: %{public}u", bundleInfo.targetVersion);
    } else {
        AVCODEC_LOGW("GetApiVersion failed, call by SA or test maybe");
    }
    return apiVersion;
}
// LCOV_EXCL_STOP

Status MediaCodec::CodePluginInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    AutoLock pluginLock(codecPluginMutex_);
    if (codecPlugin_ != nullptr) {
        MEDIA_TRACE_DEBUG(std::string("MediaCodec::CodePluginInputBuffer-QueueInputBuffer:") +
            std::to_string(inputBuffer->flag_) + "," + std::to_string(inputBuffer->pts_) +
            "," + std::to_string(inputBuffer->duration_));

        if (inputBuffer && inputBuffer->memory_) {
            ++inputCount_;
            inputBytesSum_ += inputBuffer->memory_->GetSize();
        }

        return codecPlugin_->QueueInputBuffer(inputBuffer);
    } else {
        AVCODEC_LOGE("plugin is null");
        return Status::ERROR_UNKNOWN;
    }
}

Status MediaCodec::CodePluginOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    AutoLock pluginLock(codecPluginMutex_);
    CHECK_AND_RETURN_RET_LOG(codecPlugin_ != nullptr, Status::ERROR_INVALID_STATE, "plugin is null");

    MEDIA_TRACE_DEBUG("MediaCodec::CodePluginOutputBuffer");
    auto res = codecPlugin_->QueueOutputBuffer(outputBuffer);

    if (outputBuffer && outputBuffer->memory_) {
        ++outputCount_;
        outputBytesSum_ += outputBuffer->memory_->GetSize();
    }

    return res;
}

void MediaCodec::ResetIOStat()
{
    if (state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM) {
        AVCODEC_LOGI("MediaCodec::ResetIOStat, input: %{public}" PRId64
                     " bytes in %{public}zu times, output: %{public}" PRId64 " bytes in "
                     "%{public}zu times utill last run",
            inputBytesSum_,
            inputCount_,
            outputBytesSum_,
            outputCount_);
    }

    inputBytesSum_ = 0;
    outputBytesSum_ = 0;
    inputCount_ = 0;
    outputCount_ = 0;
}

} // namespace Media
} // namespace OHOS
