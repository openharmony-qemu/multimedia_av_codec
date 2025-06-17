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

#include "audio_sink.h"
#include "avcodec_trace.h"
#include "syspara/parameters.h"
#include "plugin/plugin_manager_v2.h"
#include "common/log.h"
#include "calc_max_amplitude.h"
#include "scoped_timer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "AudioSink" };
constexpr int64_t MAX_BUFFER_DURATION_US = 200000; // Max buffer duration is 200 ms
constexpr int64_t US_TO_MS = 1000; // 1000 us per ms
constexpr int64_t ANCHOR_UPDATE_PERIOD_US = 200000; // Update time anchor every 200 ms
constexpr int64_t DRAIN_TIME_DIFF_WARN_MS = 40;
constexpr int64_t DRAIN_TIME_DIFF_INFO_MS = 20;
constexpr int64_t AUDIO_SAMPLE_8_BIT = 1;
constexpr int64_t AUDIO_SAMPLE_16_BIT = 2;
constexpr int64_t AUDIO_SAMPLE_24_BIT = 3;
constexpr int64_t AUDIO_SAMPLE_32_BIT = 4;
constexpr int64_t SEC_TO_US = 1000 * 1000;
constexpr int64_t EOS_CALLBACK_WAIT_MS = 500;
constexpr int32_t BOOT_APP_UID = 1003;
constexpr int64_t INIT_PLUGIN_WARNING_MS = 20;
constexpr int64_t OVERTIME_WARNING_MS = 50;
constexpr int64_t FORMAT_CHANGE_MS = 100;
constexpr int64_t BUFFER_CONSUME_MS = 50;
}

namespace OHOS {
namespace Media {

const int32_t DEFAULT_BUFFER_QUEUE_SIZE = 8;
const int32_t APE_BUFFER_QUEUE_SIZE = 30;
const int64_t DEFAULT_PLAY_RANGE_VALUE = -1;
const int64_t MICROSECONDS_CONVERT_UNITS = 1000;

int64_t GetAudioLatencyFixDelay()
{
    constexpr uint64_t defaultValue = 120 * HST_USECOND;
    static uint64_t fixDelay = OHOS::system::GetUintParameter("debug.media_service.audio_sync_fix_delay", defaultValue);
    MEDIA_LOG_I("audio_sync_fix_delay, pid:%{public}d, fixdelay: " PUBLIC_LOG_U64, getprocpid(), fixDelay);
    return static_cast<int64_t>(fixDelay);
}

AudioSink::AudioSink()
{
    bool isRenderCallbackMode =
        OHOS::system::GetParameter("debug.media_service.audio.audiosink_callback", "1") == "1";
    bool isProcessInputMerged =
        OHOS::system::GetParameter("debug.media_service.audio.audiosink_processinput_merged", "1") == "1";
    MEDIA_LOG_I("AudioSink ctor isRenderCallbackMode: " PUBLIC_LOG_D32 ", isProcessInputMerged: " PUBLIC_LOG_D32,
        isRenderCallbackMode, isProcessInputMerged);
    isRenderCallbackMode_ = isRenderCallbackMode;
    isProcessInputMerged_ = isProcessInputMerged;
    syncerPriority_ = IMediaSynchronizer::AUDIO_SINK;
    fixDelay_ = GetAudioLatencyFixDelay();
    plugin_ = CreatePlugin();
}

AudioSink::AudioSink(bool isRenderCallbackMode, bool isProcessInputMerged)
    : isRenderCallbackMode_(isRenderCallbackMode), isProcessInputMerged_(isProcessInputMerged)
{
    MEDIA_LOG_I("AudioSink ctor default isRenderCallbackMode: " PUBLIC_LOG_D32
        ", isProcessInputMerged: " PUBLIC_LOG_D32, isRenderCallbackMode, isProcessInputMerged);
    syncerPriority_ = IMediaSynchronizer::AUDIO_SINK;
    fixDelay_ = GetAudioLatencyFixDelay();
    plugin_ = CreatePlugin();
}

AudioSink::~AudioSink()
{
    MEDIA_LOG_D("AudioSink dtor");
}

AudioSink::AudioSinkDataCallbackImpl::AudioSinkDataCallbackImpl(std::shared_ptr<AudioSink> sink): audioSink_(sink) {}

void AudioSink::AudioSinkDataCallbackImpl::OnWriteData(int32_t size, bool isAudioVivid)
{
    auto sink = audioSink_.lock();
    FALSE_RETURN_MSG(sink != nullptr, "audioSink_ is nullptr");
    AudioStandard::BufferDesc bufferDesc;
    MediaAVCodec::AVCodecTrace trace("AudioSink::OnWriteData");
    MEDIA_LOG_DD("GetBufferDesc in");
    Status ret = sink->GetBufferDesc(bufferDesc);
    FALSE_RETURN_MSG(ret == Status::OK, "GetBufferDesc fail, ret=" PUBLIC_LOG_D32, ret);
    bufferDesc.dataLength = 0;

    if (sink->IsInputBufferDataEnough(size, isAudioVivid)) {
        bool isCopySucess = sink->HandleAudioRenderRequest(static_cast<size_t>(size),
            isAudioVivid, bufferDesc);
        bufferDesc.dataLength = isCopySucess ? bufferDesc.dataLength : 0;
    }
    ret = sink->EnqueueBufferDesc(bufferDesc);
    sink->HandleAudioRenderRequestPost();
    FALSE_RETURN_MSG(ret == Status::OK, "enqueue failed, ret=" PUBLIC_LOG_D32, ret);
}

bool AudioSink::HandleAudioRenderRequest(size_t size, bool isAudioVivid, AudioStandard::BufferDesc &bufferDesc)
{
    FALSE_RETURN_V(!eosDraining_, false);
    bool isCopySucess = CopyDataToBufferDesc(static_cast<size_t>(size), isAudioVivid, bufferDesc);
    FALSE_RETURN_V_MSG_D(isCopySucess, false, "CopyDataToBufferDesc failed");
    UpdateAudioWriteTimeMayWait();
    SyncWriteByRenderInfo();
    UpdateAmplitude();
    return true;
}

void AudioSink::HandleAudioRenderRequestPost()
{
    std::lock_guard<std::mutex> lock(availBufferMutex_);
    if (appUid_ == BOOT_APP_UID && isEosBuffer_ && availOutputBuffers_.empty()) {
        std::unique_lock<std::mutex> eosCbLock(eosCbMutex_);
        hangeOnEosCb_ = true;
        eosCbCond_.wait_for(eosCbLock, std::chrono::milliseconds(EOS_CALLBACK_WAIT_MS),
            [this] () { return !hangeOnEosCb_; });
    }
    FALSE_RETURN_NOLOG(isEosBuffer_);
    FALSE_RETURN_NOLOG(!availOutputBuffers_.empty());
    auto cacheBuffer = availOutputBuffers_.front();
    FALSE_RETURN(cacheBuffer != nullptr);
    FALSE_RETURN(IsEosBuffer(cacheBuffer));
    availOutputBuffers_.pop();
    HandleEosBuffer(cacheBuffer);
}

Status AudioSink::GetBufferDesc(AudioStandard::BufferDesc &bufferDesc)
{
    FALSE_RETURN_V_MSG(plugin_ != nullptr, Status::ERROR_UNKNOWN, "GetBufferDesc audioSinkPlugin is nullptr");
    MEDIA_TRACE_DEBUG("AudioSink::GetBufferDesc");
    return plugin_->GetBufferDesc(bufferDesc);
}

Status AudioSink::EnqueueBufferDesc(const AudioStandard::BufferDesc &bufferDesc)
{
    FALSE_RETURN_V_MSG(plugin_ != nullptr, Status::ERROR_UNKNOWN, "Enqueue audioSinkPlugin is nullptr");
    MEDIA_TRACE_DEBUG("AudioSink::EnqueueBufferDesc");
    return plugin_->EnqueueBufferDesc(bufferDesc);
}

bool AudioSink::IsInputBufferDataEnough(int32_t size, bool isAudioVivid)
{
    std::lock_guard<std::mutex> lock(availBufferMutex_);
    if (!isAudioVivid) {
        maxCbDataSize_ = std::max(maxCbDataSize_, size);
    } else {
        // audioVivid use min(size, availDataSize) as maxDataSize to ensure that audioVivid dont use swapBuffers
        size_t availDataSize = availDataSize_.load();
        int32_t availDataSizeInt32 = availDataSize <= static_cast<size_t>(INT32_MAX) ?
            static_cast<int32_t>(availDataSize): INT32_MAX;
        maxCbDataSize_ = std::min(size, availDataSizeInt32);
    }
    DriveBufferCircle();
    std::unique_lock<std::mutex> formatLock(formatChangeMutex_);
    return availDataSize_.load() >= static_cast<size_t>(size) || isEosBuffer_ || formatChange_.load();
}

Status AudioSink::Init(std::shared_ptr<Meta>& meta, const std::shared_ptr<Pipeline::EventReceiver>& receiver)
{
    state_ = Pipeline::FilterState::INITIALIZED;
    Status ret = InitAudioSinkPlugin(meta, receiver);
    FALSE_RETURN_V(ret == Status::OK, ret);
    ret = InitAudioSinkInfo(meta);
    FALSE_RETURN_V(ret == Status::OK, ret);
    return Status::OK;
}

Status AudioSink::InitAudioSinkPlugin(std::shared_ptr<Meta>& meta,
    const std::shared_ptr<Pipeline::EventReceiver>& receiver)
{
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    FALSE_RETURN_V(meta != nullptr, Status::ERROR_NULL_POINTER);
    meta->SetData(Tag::APP_PID, appPid_);
    meta->SetData(Tag::APP_UID, appUid_);
    plugin_->SetEventReceiver(receiver);
    plugin_->SetParameter(meta);
    {
        ScopedTimer timer("InitAudioSinkPlugin", INIT_PLUGIN_WARNING_MS);
        plugin_->Init();
    }
    if (isRenderCallbackMode_) {
        audioSinkDataCallback_ = std::make_shared<AudioSinkDataCallbackImpl>(shared_from_this());
        Status ret = plugin_->SetRequestDataCallback(audioSinkDataCallback_);
        isRenderCallbackMode_ = ret == Status::OK ? true : false;
    }
    plugin_->Prepare();
    plugin_->SetMuted(isMuted_);
    return Status::OK;
}

Status AudioSink::InitAudioSinkInfo(std::shared_ptr<Meta>& meta)
{
    FALSE_RETURN_V(meta != nullptr, Status::ERROR_NULL_POINTER);
    meta->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate_);
    meta->GetData(Tag::AUDIO_SAMPLE_PER_FRAME, samplePerFrame_);
    meta->GetData(Tag::AUDIO_CHANNEL_COUNT, audioChannelCount_);
    if (samplePerFrame_ > 0 && sampleRate_ > 0) {
        playingBufferDurationUs_ = static_cast<int64_t>(samplePerFrame_) * SEC_TO_US / sampleRate_;
    }
    MEDIA_LOG_I("Audiosink playingBufferDurationUs_ = " PUBLIC_LOG_D64, playingBufferDurationUs_);
    std::string mime;
    bool mimeGetRes = meta->Get<Tag::MIME_TYPE>(mime);
    if (mimeGetRes && mime == "audio/x-ape") {
        isApe_ = true;
        MEDIA_LOG_I("AudioSink::Init is ape");
    }
    if (mimeGetRes && mime == "audio/flac") {
        isFlac_ = true;
        MEDIA_LOG_I("AudioSink::Init is flac");
    }

    return Status::OK;
}

sptr<AVBufferQueueProducer> AudioSink::GetBufferQueueProducer()
{
    if (state_ != Pipeline::FilterState::READY) {
        return nullptr;
    }
    return inputBufferQueueProducer_;
}

sptr<AVBufferQueueConsumer> AudioSink::GetBufferQueueConsumer()
{
    if (state_ != Pipeline::FilterState::READY) {
        return nullptr;
    }
    return inputBufferQueueConsumer_;
}

Status AudioSink::SetParameter(const std::shared_ptr<Meta>& meta)
{
    UpdateMediaTimeRange(meta);
    FALSE_RETURN_V(meta != nullptr, Status::ERROR_NULL_POINTER);
    meta->GetData(Tag::APP_PID, appPid_);
    meta->GetData(Tag::APP_UID, appUid_);
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    plugin_->SetParameter(meta);
    return Status::OK;
}

Status AudioSink::GetParameter(std::shared_ptr<Meta>& meta)
{
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    return plugin_->GetParameter(meta);
}

Status AudioSink::Prepare()
{
    state_ = Pipeline::FilterState::PREPARING;
    Status ret = PrepareInputBufferQueue();
    if (ret != Status::OK) {
        state_ = Pipeline::FilterState::INITIALIZED;
        return ret;
    }
    state_ = Pipeline::FilterState::READY;
    {
        AutoLock lock(eosMutex_);
        eosInterruptType_ = EosInterruptState::NONE;
        eosDraining_ = false;
    }
    return ret;
}

Status AudioSink::Start()
{
    Status ret = Status::OK;
    {
        ScopedTimer timer("AudioSinkPlugin Start", OVERTIME_WARNING_MS);
        FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
        ret = plugin_->Start();
    }
    if (ret != Status::OK) {
        MEDIA_LOG_I("AudioSink start error " PUBLIC_LOG_D32, ret);
        return ret;
    }
    isEos_ = false;
    state_ = Pipeline::FilterState::RUNNING;
    FALSE_RETURN_V_NOLOG(playerEventReceiver_ != nullptr, Status::OK);
    playerEventReceiver_->OnMemoryUsageEvent({"AUDIO_SINK_BQ",
        DfxEventType::DFX_INFO_MEMORY_USAGE, inputBufferQueue_->GetMemoryUsage()});
    return ret;
}

Status AudioSink::Stop()
{
    std::lock_guard<std::mutex> lockPlugin(pluginMutex_);
    playRangeStartTime_ = DEFAULT_PLAY_RANGE_VALUE;
    playRangeEndTime_ = DEFAULT_PLAY_RANGE_VALUE;
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    Status ret = plugin_->Stop();
    underrunDetector_.Reset();
    lagDetector_.Reset();
    ResetInfo();
    if (ret != Status::OK) {
        return ret;
    }
    state_ = Pipeline::FilterState::INITIALIZED;
    AutoLock lock(eosMutex_);
    if (eosInterruptType_ != EosInterruptState::NONE) {
        eosInterruptType_ = EosInterruptState::STOP;
    }
    return ret;
}

Status AudioSink::Pause()
{
    Status ret = Status::OK;
    underrunDetector_.Reset();
    lagDetector_.Reset();
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    if (appUid_ == BOOT_APP_UID) {
        if (eosTask_  != nullptr) {
            eosTask_->SubmitJobOnce([this] {
                ScopedTimer timer("AudioSinkPlugin Pause BOOT", OVERTIME_WARNING_MS);
                {
                    std::unique_lock<std::mutex> eosCbLock(eosCbMutex_);
                    hangeOnEosCb_ = false;
                    eosCbCond_.notify_all();
                }
                plugin_->PauseTransitent();
            });
        }
    } else if (isTransitent_ || (isEos_ && (isCalledBySystemApp_ || isLoop_))) {
        ScopedTimer timer("AudioSinkPlugin PauseTransitent", OVERTIME_WARNING_MS);
        ret = plugin_->PauseTransitent();
    } else {
        ScopedTimer timer("AudioSinkPlugin Pause", OVERTIME_WARNING_MS);
        ret = plugin_->Pause();
    }
    forceUpdateTimeAnchorNextTime_ = true;
    if (ret != Status::OK) {
        return ret;
    }
    state_ = Pipeline::FilterState::PAUSED;
    AutoLock lock(eosMutex_);
    if (eosInterruptType_ == EosInterruptState::INITIAL || eosInterruptType_ == EosInterruptState::RESUME) {
        eosInterruptType_ = EosInterruptState::PAUSE;
    }
    return ret;
}

Status AudioSink::Resume()
{
    lagDetector_.Reset();
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    Status ret = plugin_->Resume();
    if (ret != Status::OK) {
        MEDIA_LOG_I("AudioSink resume error " PUBLIC_LOG_D32, ret);
        return ret;
    }
    isEos_ = false;
    state_ = Pipeline::FilterState::RUNNING;
    AutoLock lock(eosMutex_);
    if (eosInterruptType_ == EosInterruptState::PAUSE) {
        eosInterruptType_ = EosInterruptState::RESUME;
        if (!eosDraining_ && eosTask_ != nullptr) {
            eosTask_->SubmitJobOnce([this] {
                HandleEosInner(false);
            });
        }
    }
    return ret;
}

Status AudioSink::Flush()
{
    std::lock_guard<std::mutex> lockPlugin(pluginMutex_);
    MEDIA_LOG_D("do audioSink flush");
    underrunDetector_.Reset();
    lagDetector_.Reset();
    
    {
        AutoLock lock(eosMutex_);
        eosInterruptType_ = EosInterruptState::NONE;
        eosDraining_ = false;
    }
    Status ret = Status::OK;
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    ret = plugin_->Flush();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "plugin flush failed");
    ResetInfo();
    return Status::OK;
}

Status AudioSink::Release()
{
    underrunDetector_.Reset();
    lagDetector_.Reset();
    ResetInfo();
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    return plugin_->Deinit();
}

Status AudioSink::SetPlayRange(int64_t start, int64_t end)
{
    MEDIA_LOG_I("SetPlayRange enter.");
    playRangeStartTime_ = start;
    playRangeEndTime_ = end;
    return Status::OK;
}

Status AudioSink::SetVolumeMode(int32_t mode)
{
    if (plugin_ == nullptr) {
        return Status::ERROR_NULL_POINTER;
    }
    return plugin_->SetVolumeMode(mode);
}

Status AudioSink::SetVolume(float volume)
{
    if (plugin_ == nullptr) {
        return Status::ERROR_NULL_POINTER;
    }
    if (volume < 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    volume_ = volume;
    return plugin_->SetVolume(volume);
}

int32_t AudioSink::SetVolumeWithRamp(float targetVolume, int32_t duration)
{
    MEDIA_LOG_I("SetVolumeWithRamp");
    FALSE_RETURN_V(plugin_ != nullptr, static_cast<int32_t>(Status::ERROR_NULL_POINTER));
    return plugin_->SetVolumeWithRamp(targetVolume, duration);
}

Status AudioSink::SetIsTransitent(bool isTransitent)
{
    MEDIA_LOG_I("SetIsTransitent");
    isTransitent_ = isTransitent;
    return Status::OK;
}

Status AudioSink::Freeze()
{
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    plugin_->Freeze();
    return Status::OK;
}

Status AudioSink::UnFreeze()
{
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    plugin_->UnFreeze();
    return Status::OK;
}

Status AudioSink::PrepareInputBufferQueue()
{
    if (inputBufferQueue_ != nullptr && inputBufferQueue_-> GetQueueSize() > 0) {
        MEDIA_LOG_I("InputBufferQueue already create");
        return Status::ERROR_INVALID_OPERATION;
    }
    int32_t inputBufferSize = isApe_ ? APE_BUFFER_QUEUE_SIZE : DEFAULT_BUFFER_QUEUE_SIZE;
    MemoryType memoryType = MemoryType::SHARED_MEMORY;
#ifndef MEDIA_OHOS
    memoryType = MemoryType::VIRTUAL_MEMORY;
#endif
    bufferMemoryType_ = memoryType;
    MEDIA_LOG_I("PrepareInputBufferQueue ");
    inputBufferQueue_ = AVBufferQueue::Create(inputBufferSize, memoryType, INPUT_BUFFER_QUEUE_NAME);
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    return Status::OK;
}

std::shared_ptr<Plugins::AudioSinkPlugin> AudioSink::CreatePlugin()
{
    auto plugin = Plugins::PluginManagerV2::Instance().CreatePluginByMime(Plugins::PluginType::AUDIO_SINK, "audio/raw");
    if (plugin == nullptr) {
        return nullptr;
    }
    return std::reinterpret_pointer_cast<Plugins::AudioSinkPlugin>(plugin);
}

void AudioSink::UpdateAudioWriteTimeMayWait()
{
    if (latestBufferDuration_ <= 0) {
        return;
    }
    if (latestBufferDuration_ > MAX_BUFFER_DURATION_US) {
        latestBufferDuration_ = MAX_BUFFER_DURATION_US; // wait at most MAX_DURATION
    }
    int64_t timeNow = Plugins::HstTime2Us(SteadyClock::GetCurrentTimeNanoSec());
    if (!lastBufferWriteSuccess_) {
        int64_t writeSleepTime = latestBufferDuration_ - (timeNow - lastBufferWriteTime_);
        MEDIA_LOG_W("Last buffer write fail, sleep time is " PUBLIC_LOG_D64 "us", writeSleepTime);
        if (writeSleepTime > 0) {
            usleep(writeSleepTime);
            timeNow = Plugins::HstTime2Us(SteadyClock::GetCurrentTimeNanoSec());
        }
    }
    lastBufferWriteTime_ = timeNow;
}

void AudioSink::SetThreadGroupId(const std::string& groupId)
{
    eosTask_ = std::make_unique<Task>("OS_EOSa", groupId, TaskType::AUDIO, TaskPriority::HIGH, false);
}

void AudioSink::HandleEosInner(bool drain)
{
    AutoLock lock(eosMutex_);
    eosDraining_ = true; // start draining task
    switch (eosInterruptType_) {
        case EosInterruptState::INITIAL: // No user operation during EOS drain, complete drain normally
            break;
        case EosInterruptState::RESUME: // EOS drain is resumed after pause, do necessary changes
            if (drain) {
                // pause and resume happened before this task, audiosink latency should be updated
                drain = false;
            }
            eosInterruptType_ = EosInterruptState::INITIAL; // Reset EOS draining state
            break;
        default: // EOS drain is interrupted by pause or stop, and not resumed
            MEDIA_LOG_W("Drain audiosink interrupted");
            eosDraining_ = false; // abort draining task
            return;
    }
    FALSE_RETURN(plugin_ != nullptr);
    if (drain || !plugin_->IsOffloading()) {
        MEDIA_LOG_I("Drain audiosink and report EOS");
        DrainAndReportEosEvent();
        return;
    }
    uint64_t latency = 0;
    if (plugin_->GetLatency(latency) != Status::OK) {
        MEDIA_LOG_W("Failed to get latency, drain audiosink directly");
        DrainAndReportEosEvent();
        return;
    }
    if (eosTask_ == nullptr) {
        MEDIA_LOG_W("Drain audiosink, eosTask_ is nullptr");
        DrainAndReportEosEvent();
        return;
    }
    MEDIA_LOG_I("Drain audiosink wait latency = " PUBLIC_LOG_U64, latency);
    eosTask_->SubmitJobOnce([this] {
            HandleEosInner(true);
        }, latency, false);
}

void AudioSink::DrainAndReportEosEvent()
{
    FALSE_RETURN(plugin_ != nullptr);
    plugin_->Drain();
    eosInterruptType_ = EosInterruptState::NONE;
    eosDraining_ = false; // finish draining task
    isEos_ = true;
    auto syncCenter = syncCenter_.lock();
    if (syncCenter) {
        syncCenter->ReportEos(this);
    }
    Event event {
        .srcFilter = "AudioSink",
        .type = EventType::EVENT_COMPLETE,
    };
    FALSE_RETURN(playerEventReceiver_ != nullptr);
    playerEventReceiver_->OnEvent(event);
}

void AudioSink::CheckUpdateState(char *frame, uint64_t replyBytes, int32_t format)
{
    FALSE_RETURN(frame != nullptr && replyBytes != 0);
    auto currentMaxAmplitude = OHOS::Media::CalcMaxAmplitude::UpdateMaxAmplitude(frame, replyBytes, format);
    currentMaxAmplitude_ = currentMaxAmplitude_ > currentMaxAmplitude ? currentMaxAmplitude_ : currentMaxAmplitude;
}

void AudioSink::UpdateAmplitude()
{
    AutoLock amplitudeLock(amplitudeMutex_);
    maxAmplitude_ = currentMaxAmplitude_ > maxAmplitude_ ? currentMaxAmplitude_ : maxAmplitude_;
}

float AudioSink::GetMaxAmplitude()
{
    AutoLock amplitudeLock(amplitudeMutex_);
    auto ret = maxAmplitude_;
    maxAmplitude_ = 0;
    return ret;
}

void AudioSink::CalcMaxAmplitude(std::shared_ptr<AVBuffer> filledOutputBuffer)
{
    FALSE_RETURN(filledOutputBuffer != nullptr);
    auto mem = filledOutputBuffer->memory_;
    FALSE_RETURN(mem != nullptr);
    auto srcBuffer = mem->GetAddr();
    auto destBuffer = const_cast<uint8_t *>(srcBuffer);
    auto srcLength = mem->GetSize();
    size_t destLength = static_cast<size_t>(srcLength);
    int32_t format = plugin_->GetSampleFormat();
    CheckUpdateState(reinterpret_cast<char *>(destBuffer), destLength, format);
}

bool AudioSink::DropApeBuffer(std::shared_ptr<AVBuffer> filledOutputBuffer)
{
    if (!isApe_ || seekTimeUs_ == HST_TIME_NONE) {
        return false;
    }
    if (filledOutputBuffer->pts_ < seekTimeUs_) {
        MEDIA_LOG_D("Drop ape buffer pts = " PUBLIC_LOG_D64, filledOutputBuffer->pts_);
        inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
        return true;
    } else {
        seekTimeUs_ = HST_TIME_NONE;
    }
    return false;
}

void AudioSink::ClearInputBuffer()
{
    MEDIA_LOG_D("AudioSink::ClearInputBuffer enter");
    if (!inputBufferQueueConsumer_) {
        return;
    }
    std::shared_ptr<AVBuffer> filledInputBuffer;
    Status ret = Status::OK;
    while (ret == Status::OK) {
        ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            MEDIA_LOG_I("AudioSink::ClearInputBuffer clear input Buffer");
            return;
        }
        inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
    }
}

int32_t AudioSink::GetSampleFormatBytes()
{
    int32_t format = 0;
    FALSE_RETURN_V(plugin_ != nullptr, static_cast<int32_t>(Status::ERROR_NULL_POINTER));
    switch (plugin_->GetSampleFormat()) {
        case AudioSampleFormat::SAMPLE_U8:
            format = AUDIO_SAMPLE_8_BIT;
            break;
        case AudioSampleFormat::SAMPLE_S16LE:
            format = AUDIO_SAMPLE_16_BIT;
            break;
        case AudioSampleFormat::SAMPLE_S24LE:
            format = AUDIO_SAMPLE_24_BIT;
            break;
        case AudioSampleFormat::SAMPLE_S32LE:
        case AudioSampleFormat::SAMPLE_F32LE:
            format = AUDIO_SAMPLE_32_BIT;
            break;
        default:
            break;
    }
    return format;
}

bool AudioSink::IsBufferAvailable(std::shared_ptr<AVBuffer> &buffer, size_t &cacheBufferSize)
{
    FALSE_RETURN_V_MSG_D(buffer != nullptr && buffer->memory_ != nullptr, false, "buffer is null.");
    int32_t bufferSize = buffer->memory_->GetSize();
    FALSE_RETURN_V_MSG_D(bufferSize >= currentQueuedBufferOffset_, false, "buffer is empty, skip this buffer.");
    cacheBufferSize = static_cast<size_t>(bufferSize - currentQueuedBufferOffset_);
    return true;
}

bool AudioSink::CopyBufferData(AudioStandard::BufferDesc &bufferDesc, std::shared_ptr<AVBuffer> &buffer,
    size_t &size, size_t &cacheBufferSize, int64_t &bufferPts)
{
    FALSE_RETURN_V_MSG(bufferDesc.buffer != nullptr, false, "Audio bufferDesc is nullptr");
    FALSE_RETURN_V_MSG(buffer != nullptr && buffer->memory_ != nullptr, false, "AVBuffer is nullptr");
    size_t availableSize = cacheBufferSize > size ? size : cacheBufferSize;
    auto ret = memcpy_s(bufferDesc.buffer + bufferDesc.dataLength, availableSize,
        buffer->memory_->GetAddr() + currentQueuedBufferOffset_, availableSize);
    FALSE_RETURN_V_MSG(ret == 0, false, "copy from cache buffer may fail.");
    bufferPts = (bufferPts == HST_TIME_NONE) ? buffer->pts_ : bufferPts;
    bufferDesc.dataLength += availableSize;
    availDataSize_.fetch_sub(availableSize);
    if (cacheBufferSize > size) {
        currentQueuedBufferOffset_ += static_cast<int32_t>(size);
        size = 0;
        return false;
    }
    currentQueuedBufferOffset_ = 0;
    size -= cacheBufferSize;
    return true;
}

bool AudioSink::CopyAudioVividBufferData(AudioStandard::BufferDesc &bufferDesc, std::shared_ptr<AVBuffer> &buffer,
    size_t &size, size_t &cacheBufferSize, int64_t &bufferPts)
{
    FALSE_RETURN_V_MSG(bufferDesc.buffer != nullptr, false, "Audio bufferDesc is nullptr");
    FALSE_RETURN_V_MSG(buffer != nullptr && buffer->memory_ != nullptr, false, "AVBuffer is nullptr");
    auto ret = memcpy_s(bufferDesc.buffer + bufferDesc.dataLength, cacheBufferSize,
        buffer->memory_->GetAddr() + currentQueuedBufferOffset_, cacheBufferSize);
    FALSE_RETURN_V_MSG(ret == 0, false, "copy from cache buffer may fail.");
    bufferPts = (bufferPts == HST_TIME_NONE) ? buffer->pts_ : bufferPts;
    bufferDesc.dataLength += cacheBufferSize;
    size -= cacheBufferSize;
    availDataSize_.fetch_sub(cacheBufferSize);
    currentQueuedBufferOffset_ = 0;
    auto meta = buffer->meta_;
    std::vector<uint8_t> metaData;
    meta->GetData(Tag::OH_MD_KEY_AUDIO_VIVID_METADATA, metaData);
    if (metaData.size() == bufferDesc.metaLength && bufferDesc.metaLength > 0) {
        ret = memcpy_s(bufferDesc.metaBuffer, bufferDesc.metaLength,
            metaData.data(), bufferDesc.metaLength);
        FALSE_RETURN_V_MSG(ret == 0, false, "copy from cache buffer may fail.");
    }
    return true;
}

bool AudioSink::IsBufferDataDrained(AudioStandard::BufferDesc &bufferDesc, std::shared_ptr<AVBuffer> &buffer,
    size_t &size, size_t &cacheBufferSize, bool isAudioVivid, int64_t &bufferPts)
{
    FALSE_RETURN_V_MSG(cacheBufferSize <= size || !isAudioVivid, false, "copy from cache buffer may fail.");
    MEDIA_TRACE_DEBUG("AudioSink::CopyBuffer");
    bool ret = isAudioVivid ? CopyAudioVividBufferData(bufferDesc, buffer, size, cacheBufferSize, bufferPts) :
        CopyBufferData(bufferDesc, buffer, size, cacheBufferSize, bufferPts);
    return ret;
}

bool AudioSink::CopyDataToBufferDesc(size_t size, bool isAudioVivid, AudioStandard::BufferDesc &bufferDesc)
{
    FALSE_RETURN_V_MSG(size != 0 && size == bufferDesc.bufLength, false,
        "bufferDesc or request size is unavailable");
    std::lock_guard<std::mutex> lock(availBufferMutex_);
    MEDIA_TRACE_DEBUG("AudioSink::CopyDataToBufferDesc");
    int64_t bufferPts = HST_TIME_NONE;
    do {
        bool isSwapBuffer = false;
        FALSE_RETURN_V_MSG(!swapOutputBuffers_.empty() || !availOutputBuffers_.empty(), false, "buffer queue is empty");
        std::shared_ptr<AVBuffer> cacheBuffer;
        if (!swapOutputBuffers_.empty()) {
            cacheBuffer = swapOutputBuffers_.front();
            isSwapBuffer = true;
        } else {
            cacheBuffer = availOutputBuffers_.front();
        }
        if (MuteAudioBuffer(size, bufferDesc, IsEosBuffer(cacheBuffer)) != Status::OK) {
            break;
        }
        size_t cacheBufferSize = 0;
        if (IsBufferAvailable(cacheBuffer, cacheBufferSize)) {
            if (!IsBufferDataDrained(bufferDesc, cacheBuffer, size, cacheBufferSize, isAudioVivid, bufferPts)) {
                break;
            }
            CalcMaxAmplitude(cacheBuffer);
        }
        ReleaseCacheBuffer(isSwapBuffer);
    } while (size > 0 && !isAudioVivid);
    if (bufferPts >= 0) {
        int64_t bufferDuration = CalculateBufferDuration(bufferDesc.dataLength);
        innerSynchroizer_->UpdateCurrentBufferInfo(bufferPts, bufferDuration);
        return true;
    }
    return false;
}

Status AudioSink::MuteAudioBuffer(size_t size, AudioStandard::BufferDesc &bufferDesc, bool isEos)
{
    std::unique_lock<std::mutex> lock(formatChangeMutex_);
    FALSE_RETURN_V_NOLOG(formatChange_ || isEos, Status::OK);
    MEDIA_LOG_I("AudioSink mute audio buffer isEos %{public}d size %{public}zu dataLength %{public}zu"
        "bufferLength %{public}zu", isEos, size, bufferDesc.dataLength, bufferDesc.bufLength);
    FALSE_RETURN_V_NOLOG(plugin_ && size > 0, Status::ERROR_INVALID_STATE);
    (void)plugin_->MuteAudioBuffer(bufferDesc.buffer, bufferDesc.dataLength, size);
    return isEos ? Status::ERROR_INVALID_STATE : Status::OK;
}

int64_t AudioSink::CalculateBufferDuration(int64_t writeDataSize)
{
    int32_t format = GetSampleFormatBytes();
    FALSE_RETURN_V(format > 0 && audioChannelCount_ > 0, 0);
    int64_t sampleDataNums = writeDataSize / format / audioChannelCount_;
    FALSE_RETURN_V(sampleRate_ > 0, 0);
    int64_t sampleDataDuration = sampleDataNums * SEC_TO_US / sampleRate_;
    return sampleDataDuration;
}

void AudioSink::AudioDataSynchroizer::UpdateCurrentBufferInfo(int64_t bufferPts, int64_t bufferDuration)
{
    bufferDuration_ = bufferDuration;
    startPTS_ = startPTS_ == HST_TIME_NONE ? bufferPts : startPTS_;
    lastBufferPTS_ = lastBufferPTS_ == HST_TIME_NONE ? bufferPts : lastBufferPTS_;
    curBufferPTS_ = bufferPts;
}

void AudioSink::AudioDataSynchroizer::UpdateLastBufferPTS(int64_t bufferOffset, float speed)
{
    MEDIA_LOG_DD("lastBuffer Info: lastBufferPTS_ is " PUBLIC_LOG_D64 " lastBufferOffset_ is " PUBLIC_LOG_D64
        " and compensateDuration_ is " PUBLIC_LOG_D64, lastBufferPTS_, lastBufferOffset_, compensateDuration_);
    curBufferPTS_ = curBufferPTS_ == HST_TIME_NONE ? 0 : curBufferPTS_;
    int64_t tempPTS = curBufferPTS_ + lastBufferOffset_ + bufferDuration_;
    if (tempPTS < lastBufferPTS_) {
        MEDIA_LOG_W("audio pts is not increasing, last pts: " PUBLIC_LOG_D64 ", current pts: " PUBLIC_LOG_D64,
            lastBufferPTS_, tempPTS);
    }
    lastBufferPTS_ = tempPTS;
    lastBufferOffset_ = bufferOffset;
    sumDuration_ += bufferDuration_;
    FALSE_RETURN_MSG(speed != 0, "speed is 0");
    compensateDuration_ += bufferDuration_ - bufferDuration_ / speed;
}

void AudioSink::UpdateRenderInfo()
{
    timespec time;
    uint32_t position;
    MEDIA_TRACE_DEBUG("AudioSink::UpdateRenderInfo");
    FALSE_RETURN(plugin_ != nullptr);
    FALSE_RETURN_MSG(plugin_->GetAudioPosition(time, position), "GetAudioPosition from audioRender failed");
    int64_t currentRenderClockTime = time.tv_sec * SEC_TO_US + time.tv_nsec / US_TO_MS; // convert to us
    FALSE_RETURN(sampleRate_ > 0);
    int64_t currentRenderPTS = static_cast<int64_t>(position) * SEC_TO_US / sampleRate_;
    MEDIA_LOG_DD("currentRenderPTS is " PUBLIC_LOG_D64 " and currentRenderClockTime is " PUBLIC_LOG_D64,
        currentRenderPTS, currentRenderClockTime);
    innerSynchroizer_->OnRenderPositionUpdated(currentRenderPTS, currentRenderClockTime);
}

void AudioSink::AudioDataSynchroizer::OnRenderPositionUpdated(int64_t currentRenderPTS, int64_t currentRenderClockTime)
{
    currentRenderClockTime_ = currentRenderClockTime;
    currentRenderPTS_ = currentRenderPTS;
}

int64_t AudioSink::AudioDataSynchroizer::CalculateAudioLatency()
{
    int64_t latency = 0;
    if (lastBufferPTS_ == startPTS_) {
        return latency;
    }
    int64_t nowClockTime = Plugins::HstTime2Us(SteadyClock::GetCurrentTimeNanoSec());
    MEDIA_LOG_DD("PTS diff is " PUBLIC_LOG_D64, ((sumDuration_ - compensateDuration_) -
        currentRenderPTS_));
    latency = ((sumDuration_ - compensateDuration_) - currentRenderPTS_) -
        (nowClockTime - currentRenderClockTime_);
    FALSE_RETURN_V_MSG(latency >= 0, 0, "calculate latency failed");
    return latency;
}

int64_t AudioSink::AudioDataSynchroizer::GetLastBufferPTS() const
{
    return lastBufferPTS_;
}

int64_t AudioSink::AudioDataSynchroizer::GetLastReportedClockTime() const
{
    return lastReportedClockTime_;
}

int64_t AudioSink::AudioDataSynchroizer::GetBufferDuration() const
{
    return bufferDuration_;
}

void AudioSink::AudioDataSynchroizer::UpdateReportTime(int64_t nowClockTime)
{
    lastReportedClockTime_ = nowClockTime;
}

void AudioSink::AudioDataSynchroizer::Reset()
{
    lastBufferPTS_ = HST_TIME_NONE;
    bufferDuration_ = 0;
    currentRenderClockTime_ = 0;
    currentRenderPTS_ = 0;
    lastReportedClockTime_ = HST_TIME_NONE;
    startPTS_ = HST_TIME_NONE;
    curBufferPTS_ = HST_TIME_NONE;
    lastBufferOffset_ = 0;
    compensateDuration_ = 0;
    sumDuration_ = 0;
}

bool AudioSink::IsTimeAnchorNeedUpdate()
{
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN_V(syncCenter != nullptr, false);
    int64_t lastAnchorClockTime = innerSynchroizer_->GetLastReportedClockTime();
    int64_t nowCt = syncCenter->GetClockTimeNow();
    bool needUpdate = forceUpdateTimeAnchorNextTime_ ||
        (lastAnchorClockTime == HST_TIME_NONE) ||
        (nowCt - lastAnchorClockTime >= ANCHOR_UPDATE_PERIOD_US);
    FALSE_RETURN_V_MSG_D(needUpdate, false, "No need to update time anchor this time.");
    UpdateRenderInfo();
    int64_t latency = innerSynchroizer_->CalculateAudioLatency();
    MEDIA_LOG_DD("Calculate latency = " PUBLIC_LOG_U64, latency);
    int64_t lastBufferPTS = innerSynchroizer_->GetLastBufferPTS();
    int64_t lastBufferDuration = innerSynchroizer_->GetBufferDuration();
    Pipeline::IMediaSyncCenter::IMediaTime iMediaTime = {lastBufferPTS - firstPts_, lastBufferPTS,
        lastBufferDuration};
    syncCenter->UpdateTimeAnchor(nowCt, latency + fixDelay_, iMediaTime, this);
    MEDIA_LOG_I("AudioSink fixDelay_: " PUBLIC_LOG_D64
        " us, latency: " PUBLIC_LOG_D64
        " us, pts-f: " PUBLIC_LOG_D64
        " us, nowCt: " PUBLIC_LOG_D64 " us",
        fixDelay_, latency, lastBufferPTS - firstPts_, nowCt);
    forceUpdateTimeAnchorNextTime_ = false;
    innerSynchroizer_->UpdateReportTime(nowCt);
    return true;
}

void AudioSink::SyncWriteByRenderInfo()
{
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN(syncCenter != nullptr);
    if (firstPts_ == HST_TIME_NONE) {
        if (syncCenter && syncCenter->GetMediaStartPts() != HST_TIME_NONE) {
            firstPts_ = syncCenter->GetMediaStartPts();
        } else {
            firstPts_ = innerSynchroizer_->GetLastBufferPTS();
        }
    }
    bool anchorUpdated = IsTimeAnchorNeedUpdate();
    innerSynchroizer_->UpdateLastBufferPTS(CalculateBufferDuration(currentQueuedBufferOffset_), speed_);
    latestBufferDuration_ = innerSynchroizer_->GetBufferDuration() / speed_;
    if (anchorUpdated) {
        bufferDurationSinceLastAnchor_ = latestBufferDuration_;
    } else {
        bufferDurationSinceLastAnchor_ += latestBufferDuration_;
    }
    underrunDetector_.SetLastAudioBufferDuration(bufferDurationSinceLastAnchor_);
    if (syncCenter) {
        syncCenter->SetLastAudioBufferDuration(bufferDurationSinceLastAnchor_);
    }
}

void AudioSink::ReleaseCacheBuffer(bool isSwapBuffer)
{
    if (isSwapBuffer) {
        FALSE_RETURN_MSG(!swapOutputBuffers_.empty(), "swapOutputBuffers_ has no buffer");
        swapOutputBuffers_.pop();
        std::unique_lock<std::mutex> formatLock(formatChangeMutex_);
        FALSE_RETURN_NOLOG(formatChange_.load() && swapOutputBuffers_.empty() && availOutputBuffers_.empty());
        formatChange_ = false;
        formatChangeCond_.notify_all();
        return;
    }
    auto buffer = availOutputBuffers_.front();
    availOutputBuffers_.pop();
    FALSE_RETURN_MSG(buffer != nullptr, "release buffer, but buffer is null");
    MEDIA_LOG_DD("the pts is " PUBLIC_LOG_D64 " buffer is release", buffer->pts_);
    Status ret = inputBufferQueueConsumer_->ReleaseBuffer(buffer);
    FALSE_RETURN_MSG(ret == Status::OK, "release avbuffer failed");
    std::unique_lock<std::mutex> formatLock(formatChangeMutex_);
    FALSE_RETURN_NOLOG(formatChange_.load() && swapOutputBuffers_.empty() && availOutputBuffers_.empty());
    formatChange_ = false;
    formatChangeCond_.notify_all();
    return;
}

void AudioSink::ResetInfo()
{
    if (appUid_ == BOOT_APP_UID) {
        std::unique_lock<std::mutex> eosCbLock(eosCbMutex_);
        hangeOnEosCb_ = false;
        eosCbCond_.notify_all();
    }
    innerSynchroizer_->Reset();
    maxAmplitude_ = 0;
    currentMaxAmplitude_ = 0;
    currentQueuedBufferOffset_ = 0;
    forceUpdateTimeAnchorNextTime_ = true;
    isEosBuffer_ = false;
    availDataSize_.store(0);
    ClearAvailableOutputBuffers();
    ClearInputBuffer();
}

void AudioSink::ClearAvailableOutputBuffers()
{
    FALSE_RETURN(inputBufferQueueConsumer_ != nullptr);
    std::lock_guard<std::mutex> lock(availBufferMutex_);
    while (!swapOutputBuffers_.empty()) {
        swapOutputBuffers_.pop();
    }
    while (!availOutputBuffers_.empty()) {
        ReleaseCacheBuffer();
    }
    std::unique_lock<std::mutex> formatLock(formatChangeMutex_);
    formatChange_ = false;
    formatChangeCond_.notify_all();
}

void AudioSink::GetAvailableOutputBuffers()
{
    FALSE_RETURN(inputBufferQueueConsumer_ != nullptr);
    std::lock_guard<std::mutex> lock(availBufferMutex_);
    std::shared_ptr<AVBuffer> filledInputBuffer;
    Status ret = Status::OK;
    MEDIA_TRACE_DEBUG("AudioSink::GetBufferFromUpstream");
    while (ret == Status::OK) {
        ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
        if (ret != Status::OK || filledInputBuffer == nullptr) {
            break;
        }
        if (filledInputBuffer->memory_ == nullptr || filledInputBuffer->pts_ < 0) {
            inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
            continue;
        }
        if (DropApeBuffer(filledInputBuffer)) {
            break;
        }
        if (IsEosBuffer(filledInputBuffer)) {
            MEDIA_LOG_I("AudioSink Recv EOS");
            isEosBuffer_ = true;
        }
        availOutputBuffers_.push(filledInputBuffer);
        MEDIA_LOG_DD("the pts is " PUBLIC_LOG_D64 " buffer is push", filledInputBuffer->pts_);
        availDataSize_.fetch_add(filledInputBuffer->memory_->GetSize());
    }
    DriveBufferCircle();
    return;
}

void AudioSink::SetIsInPrePausing(bool isInPrePausing)
{
    MediaAVCodec::AVCodecTrace trace("AudioSink::SetIsInPrePausing" + std::to_string(isInPrePausing));
    isInPrePausing_.store(isInPrePausing, std::memory_order_relaxed);
    FALSE_RETURN_NOLOG(isInPrePausing && (isRenderCallbackMode_ && !isAudioDemuxDecodeAsync_));

    std::lock_guard<std::mutex> lock(availBufferMutex_);
    DriveBufferCircle();
}

void AudioSink::DriveBufferCircle()
{
    FALSE_RETURN_NOLOG(!isEosBuffer_);
    FALSE_RETURN_NOLOG(!availOutputBuffers_.empty() && inputBufferQueue_ != nullptr);
    FALSE_RETURN_NOLOG(availOutputBuffers_.size() >= inputBufferQueue_->GetQueueSize());
    size_t availDataSize = availDataSize_.load();
    int32_t availDataSizeInt32 = availDataSize <= static_cast<size_t>(INT32_MAX) ?
        static_cast<int32_t>(availDataSize): INT32_MAX;
    bool isInPrePausing = isInPrePausing_.load(std::memory_order_relaxed);
    FALSE_RETURN_NOLOG(isInPrePausing || availDataSizeInt32 < maxCbDataSize_);
    std::shared_ptr<AVBuffer> oldestBuffer = availOutputBuffers_.front();
    FALSE_RETURN_MSG(oldestBuffer != nullptr && oldestBuffer->memory_->GetSize() > 0, "buffer or memory is nullptr");
    std::shared_ptr<AVBuffer> swapBuffer = CopyBuffer(oldestBuffer);
    FALSE_RETURN_MSG(swapBuffer != nullptr, "CopyBuffer failed, swapBuffer is nullptr");
    availOutputBuffers_.pop();
    swapOutputBuffers_.push(swapBuffer);
    if (isInPrePausing) {
        MEDIA_LOG_I("DriveBufferCircle availOutputBuffers_ size:%{public}d",
            static_cast<int>(availOutputBuffers_.size()));
    }
    FALSE_RETURN_MSG(inputBufferQueueConsumer_ != nullptr, "bufferQueue consumer is nullptr");
    inputBufferQueueConsumer_->ReleaseBuffer(oldestBuffer);
}

std::shared_ptr<AVBuffer> AudioSink::CopyBuffer(const std::shared_ptr<AVBuffer> buffer)
{
    FALSE_RETURN_V_MSG_E(buffer != nullptr && buffer->memory_->GetSize() > 0, nullptr, "buffer or memory is nullptr");
    std::shared_ptr<Meta> meta = buffer->meta_;
    std::vector<uint8_t> metaData;
    FALSE_RETURN_V_MSG_W(meta == nullptr || !meta->GetData(Tag::OH_MD_KEY_AUDIO_VIVID_METADATA, metaData), nullptr,
        "copy buffer not support for audiovivid");
    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = static_cast<int32_t>(buffer->memory_->GetSize());
    avBufferConfig.memoryType = bufferMemoryType_;
    std::shared_ptr<AVBuffer> swapBuffer = AVBuffer::CreateAVBuffer(avBufferConfig);
    FALSE_RETURN_V_MSG_E(swapBuffer != nullptr && swapBuffer->memory_ != nullptr, nullptr, "create swapBuffer failed");
    swapBuffer->pts_ = buffer->pts_;
    swapBuffer->dts_ = buffer->dts_;
    swapBuffer->duration_ = buffer->duration_;
    swapBuffer->flag_ = buffer->flag_;
    FALSE_RETURN_V_MSG_E(swapBuffer->memory_->GetCapacity() >= buffer->memory_->GetSize(), nullptr, "no enough memory");
    errno_t res = memcpy_s(swapBuffer->memory_->GetAddr(),
        swapBuffer->memory_->GetCapacity(),
        buffer->memory_->GetAddr(),
        buffer->memory_->GetSize());
    FALSE_RETURN_V_MSG_E(res == EOK, nullptr, "copy data failed");
    swapBuffer->memory_->SetSize(buffer->memory_->GetSize());
    return swapBuffer;
}

void AudioSink::WriteDataToRender(std::shared_ptr<AVBuffer> &filledOutputBuffer)
{
    FALSE_RETURN(DropApeBuffer(filledOutputBuffer) == false);
    FALSE_RETURN(filledOutputBuffer != nullptr);
    if (IsEosBuffer(filledOutputBuffer)) {
        HandleEosBuffer(filledOutputBuffer);
        return;
    }
    FALSE_RETURN(plugin_ != nullptr);
    UpdateAudioWriteTimeMayWait();
    DoSyncWrite(filledOutputBuffer);
    if (calMaxAmplitudeCbStatus_) {
        CalcMaxAmplitude(filledOutputBuffer);
        UpdateAmplitude();
    } else {
        maxAmplitude_ = 0.0f;
    }
    lastBufferWriteSuccess_ = (plugin_->Write(filledOutputBuffer) == Status::OK);
    int64_t nowClockTime = 0;
    GetSyncCenterClockTime(nowClockTime);
    auto audioWriteMs = plugin_->GetWriteDurationMs();
    lagDetector_.UpdateDrainTimeGroup(
        { lastAnchorClockTime_, bufferDurationSinceLastAnchor_, audioWriteMs, nowClockTime });
    PerfRecord(audioWriteMs);
    lagDetector_.CalcLag(filledOutputBuffer);
    MEDIA_LOG_DD("audio DrainOutputBuffer pts = " PUBLIC_LOG_D64, filledOutputBuffer->pts_);
    numFramesWritten_++;
    FALSE_RETURN(inputBufferQueueConsumer_ != nullptr);
    inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
}

bool AudioSink::IsEosBuffer(std::shared_ptr<AVBuffer> &filledOutputBuffer)
{
    FALSE_RETURN_V(filledOutputBuffer != nullptr, false);
    return (filledOutputBuffer->flag_ & BUFFER_FLAG_EOS) ||
        ((playRangeEndTime_ != DEFAULT_PLAY_RANGE_VALUE) &&
        (filledOutputBuffer->pts_ > playRangeEndTime_ * MICROSECONDS_CONVERT_UNITS));
}

void AudioSink::HandleEosBuffer(std::shared_ptr<AVBuffer> &filledOutputBuffer)
{
    FALSE_RETURN(inputBufferQueueConsumer_ != nullptr);
    inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
    AutoLock eosLock(eosMutex_);
    // avoid submit handle eos task multiple times
    FALSE_RETURN(!eosDraining_);
    eosInterruptType_ = EosInterruptState::INITIAL;
    if (eosTask_ == nullptr) {
        DrainAndReportEosEvent();
        return;
    }
    MEDIA_LOG_I("DrainOutputBuffer Recv EOS PTS");
    eosTask_->SubmitJobOnce([this] {
        HandleEosInner(false);
    });
    return;
}

void AudioSink::DrainOutputBuffer(bool flushed)
{
    std::lock_guard<std::mutex> lock(pluginMutex_);
    FALSE_RETURN(plugin_ != nullptr && inputBufferQueueConsumer_ != nullptr);

    if (isRenderCallbackMode_) {
        if (state_ != Pipeline::FilterState::RUNNING) {
            MEDIA_LOG_W("DrainOutputBuffer ignore temporarily for not RUNNINT state: " PUBLIC_LOG_D32
                ", isProcessInputMerged: " PUBLIC_LOG_D32, state_.load(), isProcessInputMerged_);

            /*
            * As the AudioRender START and PAUSE procedure may consume a long time about 200 ms,
            * if the consumption of inputbuffer queue is not excuted in AudioSinkFilter's task working thread,
            * this RETURN ACTION will cause the filled buffers not been consumed,
            * which cause the upstream filter RequesetBuffer failed and audio track playback stuck.
            */
            if (!isProcessInputMerged_) {
                return;
            }
        }
        GetAvailableOutputBuffers();
    } else {
        std::shared_ptr<AVBuffer> filledOutputBuffer = nullptr;
        Status ret = inputBufferQueueConsumer_->AcquireBuffer(filledOutputBuffer);
        FALSE_RETURN(ret == Status::OK && filledOutputBuffer != nullptr);
        if (state_ != Pipeline::FilterState::RUNNING || flushed) {
            MEDIA_LOG_W("DrainOutputBuffer, drop audio buffer pts = " PUBLIC_LOG_D64 ", state: " PUBLIC_LOG_D32
                ", flushed: " PUBLIC_LOG_D32, filledOutputBuffer->pts_, state_.load(), flushed);

            /*
             * As START and PAUSE procedure of AudioDecoderFilter and AudioSinkFilter run concurrently
             * in different working thread, AudioSinkFilter may change to RUNNING state after AudioDecoderFilter
             * or AudioSinkFilter may change to PAUSED state before AudioDecoderFilter.
             * This ReleaseBuffer ACTION will cause audio buffer droped, which may cause audio discontinuity.
             *
             * So, How to deal with this case?
             * 1. Try to cache buffers in non RUNNING state, prioritize consuming the cached buffers in RUNNING state?
             * 2. There may be some buffers that cannot be processed, i.e. do Pause/Resume repeatedly around the ending
             * and cause the EOS buffer being cached which may cause that the Audio Track can't end.
             */
            inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
            return;
        }
        WriteDataToRender(filledOutputBuffer);
    }
}

Status AudioSink::SetPerfRecEnabled(bool isPerfRecEnabled)
{
    isPerfRecEnabled_ = isPerfRecEnabled;
    return Status::OK;
}

void AudioSink::PerfRecord(int64_t audioWriteMs)
{
    FALSE_RETURN_NOLOG(isPerfRecEnabled_);
    FALSE_RETURN_MSG(playerEventReceiver_ != nullptr, "Report perf failed, event receiver is nullptr");
    FALSE_RETURN_NOLOG(perfRecorder_.Record(audioWriteMs) == PerfRecorder::FULL);
    playerEventReceiver_->OnDfxEvent(
        { "ASINK", DfxEventType::DFX_INFO_PERF_REPORT, perfRecorder_.GetMainPerfData() });
    perfRecorder_.Reset();
}

void AudioSink::ResetSyncInfo()
{
    lastAnchorClockTime_ = HST_TIME_NONE;
    forceUpdateTimeAnchorNextTime_ = true;
}

void AudioSink::UnderrunDetector::Reset()
{
    AutoLock lock(mutex_);
    lastClkTime_ = HST_TIME_NONE;
    lastLatency_ = HST_TIME_NONE;
    lastBufferDuration_ = HST_TIME_NONE;
}

void AudioSink::UnderrunDetector::SetEventReceiver(std::weak_ptr<Pipeline::EventReceiver> eventReceiver)
{
    eventReceiver_ = eventReceiver;
}

void AudioSink::UnderrunDetector::UpdateBufferTimeNoLock(int64_t clkTime, int64_t latency)
{
    lastClkTime_ = clkTime;
    lastLatency_ = latency;
}

void AudioSink::UnderrunDetector::SetLastAudioBufferDuration(int64_t durationUs)
{
    AutoLock lock(mutex_);
    lastBufferDuration_ = durationUs;
}

void AudioSink::UnderrunDetector::DetectAudioUnderrun(int64_t clkTime, int64_t latency)
{
    if (lastClkTime_ == HST_TIME_NONE) {
        AutoLock lock(mutex_);
        UpdateBufferTimeNoLock(clkTime, latency);
        return;
    }
    int64_t underrunTimeUs = 0;
    {
        AutoLock lock(mutex_);
        int64_t elapsedClk = clkTime - lastClkTime_;
        underrunTimeUs = elapsedClk - (lastLatency_ + lastBufferDuration_);
        UpdateBufferTimeNoLock(clkTime, latency);
    }
    if (underrunTimeUs > 0) {
        MEDIA_LOG_D("AudioSink maybe underrun, underrunTimeUs=" PUBLIC_LOG_D64, underrunTimeUs);
        auto eventReceiver = eventReceiver_.lock();
        FALSE_RETURN(eventReceiver != nullptr);
        eventReceiver->OnDfxEvent({"AudioSink", DfxEventType::DFX_INFO_PLAYER_AUDIO_LAG, underrunTimeUs / US_TO_MS});
    }
}

bool AudioSink::AudioLagDetector::CalcLag(std::shared_ptr<AVBuffer> buffer)
{
    (void)buffer;
    int64_t maxMediaTime = lastDrainTimeGroup_.anchorDuration + lastDrainTimeGroup_.lastAnchorPts + latency_;
    auto currentMediaTime = lastDrainTimeGroup_.nowClockTime;
    auto writeTimeMs = lastDrainTimeGroup_.writeDuration;

    MEDIA_LOG_DD("maxMediaTime " PUBLIC_LOG_D64 " currentMediaTime " PUBLIC_LOG_D64 " latency_ " PUBLIC_LOG_D64,
        maxMediaTime, currentMediaTime, latency_);

    if (maxMediaTime < currentMediaTime) {
        MEDIA_LOG_W("renderer write cost " PUBLIC_LOG_D64, writeTimeMs);
    }
    
    // Calc time delays except plugin write
    auto currentTimeMs = Plugins::GetCurrentMillisecond();
    auto totalTimeDiff = lastDrainTimeMs_ == 0 ? 0 : currentTimeMs - lastDrainTimeMs_;
    lastDrainTimeMs_ = currentTimeMs;
    auto drainTimeDiff = totalTimeDiff - writeTimeMs;
    if (drainTimeDiff > DRAIN_TIME_DIFF_WARN_MS) {
        MEDIA_LOG_W("Audio drain cost " PUBLIC_LOG_D64, drainTimeDiff);
    } else if (drainTimeDiff > DRAIN_TIME_DIFF_INFO_MS) {
        MEDIA_LOG_I("Audio drain cost " PUBLIC_LOG_D64, drainTimeDiff);
    } else {
        MEDIA_LOG_DD("Audio drain cost " PUBLIC_LOG_D64, drainTimeDiff);
    }

    return maxMediaTime < currentMediaTime;
}

void AudioSink::AudioLagDetector::Reset()
{
    latency_ = 0;
    lastDrainTimeMs_ = 0;
    lastDrainTimeGroup_.lastAnchorPts = 0;
    lastDrainTimeGroup_.anchorDuration = 0;
    lastDrainTimeGroup_.writeDuration = 0;
    lastDrainTimeGroup_.nowClockTime = 0;
}

bool AudioSink::GetSyncCenterClockTime(int64_t &clockTime)
{
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN_V(syncCenter != nullptr, false);
    clockTime = syncCenter->GetClockTimeNow();
    return true;
}

void AudioSink::AudioLagDetector::UpdateDrainTimeGroup(AudioDrainTimeGroup group)
{
    lastDrainTimeGroup_.lastAnchorPts = group.lastAnchorPts;
    lastDrainTimeGroup_.anchorDuration = group.anchorDuration;
    lastDrainTimeGroup_.writeDuration = group.writeDuration;
    lastDrainTimeGroup_.nowClockTime = group.nowClockTime;
}

bool AudioSink::UpdateTimeAnchorIfNeeded(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN_V(syncCenter != nullptr, false);
    int64_t nowCt = syncCenter->GetClockTimeNow();
    bool needUpdate = forceUpdateTimeAnchorNextTime_ ||
        (lastAnchorClockTime_ == HST_TIME_NONE) ||
        (nowCt - lastAnchorClockTime_ >= ANCHOR_UPDATE_PERIOD_US);
    if (!needUpdate) {
        MEDIA_LOG_DD("No need to update time anchor this time.");
        return false;
    }
    uint64_t latency = 0;
    FALSE_LOG_MSG(plugin_->GetLatency(latency) == Status::OK, "failed to get latency");
    Pipeline::IMediaSyncCenter::IMediaTime iMediaTime = {buffer->pts_ - firstPts_, buffer->pts_, buffer->duration_};
    syncCenter->UpdateTimeAnchor(nowCt, latency + fixDelay_, iMediaTime, this);
    lagDetector_.SetLatency(latency + fixDelay_);
    MEDIA_LOG_I("AudioSink fixDelay_: " PUBLIC_LOG_D64
        " us, latency: " PUBLIC_LOG_D64
        " us, pts-f: " PUBLIC_LOG_D64
        " us, pts: " PUBLIC_LOG_D64
        " us, nowCt: " PUBLIC_LOG_D64 " us",
        fixDelay_, latency, buffer->pts_ - firstPts_, buffer->pts_, nowCt);
    forceUpdateTimeAnchorNextTime_ = false;
    lastAnchorClockTime_ = nowCt;
    return true;
}

int64_t AudioSink::DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    bool render = true; // audio sink always report time anchor and do not drop
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN_V(syncCenter != nullptr, 0);
    if (firstPts_ == HST_TIME_NONE) {
        if (syncCenter && syncCenter->GetMediaStartPts() != HST_TIME_NONE) {
            firstPts_ = syncCenter->GetMediaStartPts();
        } else {
            firstPts_ = buffer->pts_;
        }
        MEDIA_LOG_I("audio DoSyncWrite set firstPts = " PUBLIC_LOG_D64, firstPts_);
    }
    bool anchorUpdated = UpdateTimeAnchorIfNeeded(buffer);
    latestBufferDuration_ = CalcBufferDuration(buffer) / speed_;
    if (anchorUpdated) {
        bufferDurationSinceLastAnchor_ = latestBufferDuration_;
    } else {
        bufferDurationSinceLastAnchor_ += latestBufferDuration_;
    }
    underrunDetector_.SetLastAudioBufferDuration(bufferDurationSinceLastAnchor_);
    if (syncCenter) {
        syncCenter->SetLastAudioBufferDuration(bufferDurationSinceLastAnchor_);
    }
    return render ? 0 : -1;
}

int64_t AudioSink::CalcBufferDuration(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr && buffer->memory_ != nullptr && sampleRate_ != 0 && audioChannelCount_ != 0, 0);
    int64_t size = static_cast<int64_t>(buffer->memory_->GetSize());
    int32_t format = GetSampleFormatBytes();
    FALSE_RETURN_V(format > 0 && audioChannelCount_ > 0, 0);
    return SEC_TO_US * size / format / sampleRate_ / audioChannelCount_;
}

Status AudioSink::SetSpeed(float speed)
{
    if (plugin_ == nullptr) {
        return Status::ERROR_NULL_POINTER;
    }
    if (speed <= 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    auto ret = plugin_->SetSpeed(speed);
    if (ret == Status::OK) {
        speed_ = speed;
    }
    forceUpdateTimeAnchorNextTime_ = true;
    return ret;
}

Status AudioSink::SetAudioEffectMode(int32_t effectMode)
{
    MEDIA_LOG_I("SetAudioEffectMode");
    if (plugin_ == nullptr) {
        return Status::ERROR_NULL_POINTER;
    }
    effectMode_ = effectMode;
    return plugin_->SetAudioEffectMode(effectMode);
}

Status AudioSink::GetAudioEffectMode(int32_t &effectMode)
{
    MEDIA_LOG_I("GetAudioEffectMode");
    if (plugin_ == nullptr) {
        return Status::ERROR_NULL_POINTER;
    }
    return plugin_->GetAudioEffectMode(effectMode);
}

bool AudioSink::OnNewAudioMediaTime(int64_t mediaTimeUs)
{
    bool render = true;
    if (firstAudioAnchorTimeMediaUs_ == Plugins::HST_TIME_NONE) {
        firstAudioAnchorTimeMediaUs_ = mediaTimeUs;
    }
    int64_t nowUs = 0;
    auto syncCenter = syncCenter_.lock();
    if (syncCenter) {
        nowUs = syncCenter->GetClockTimeNow();
    }
    int64_t pendingTimeUs = getPendingAudioPlayoutDurationUs(nowUs);
    render = syncCenter->UpdateTimeAnchor(nowUs, pendingTimeUs, {mediaTimeUs, mediaTimeUs, mediaTimeUs}, this);
    return render;
}

int64_t AudioSink::getPendingAudioPlayoutDurationUs(int64_t nowUs)
{
    int64_t writtenSamples = numFramesWritten_ * samplePerFrame_;
    const int64_t numFramesPlayed = plugin_->GetPlayedOutDurationUs(nowUs);
    FALSE_RETURN_V(sampleRate_ > 0, 0);
    int64_t pendingUs = (writtenSamples - numFramesPlayed) * HST_MSECOND / sampleRate_;
    MEDIA_LOG_DD("pendingUs: " PUBLIC_LOG_D64, pendingUs);
    if (pendingUs < 0) {
        pendingUs = 0;
    }
    return pendingUs;
}

int64_t AudioSink::getDurationUsPlayedAtSampleRate(uint32_t numFrames)
{
    std::shared_ptr<Meta> parameter;
    plugin_->GetParameter(parameter);
    int32_t sampleRate = 0;
    if (parameter) {
        parameter->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    }
    if (sampleRate == 0) {
        MEDIA_LOG_W("cannot get sampleRate");
        return 0;
    }
    return (int64_t)(static_cast<int32_t>(numFrames) * HST_MSECOND / sampleRate);
}

void AudioSink::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver)
{
    FALSE_RETURN(receiver != nullptr);
    playerEventReceiver_ = receiver;
    FALSE_RETURN(plugin_ != nullptr);
    plugin_->SetEventReceiver(receiver);
}

void AudioSink::SetSyncCenter(std::shared_ptr<Pipeline::MediaSyncManager> syncCenter)
{
    syncCenter_ = syncCenter;
    MediaSynchronousSink::Init();
}

Status AudioSink::ChangeTrack(std::shared_ptr<Meta>& meta, const std::shared_ptr<Pipeline::EventReceiver>& receiver)
{
    MEDIA_LOG_I("GetAudioEffectMode ChangeTrack. ");
    std::lock_guard<std::mutex> lock(pluginMutex_);
    if (plugin_) {
        plugin_->Stop();
        plugin_->Deinit();
        plugin_ = nullptr;
    }
    plugin_ = CreatePlugin();
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    Status ret = Status::OK;
    ret = InitAudioSinkPlugin(meta, receiver);
    FALSE_RETURN_V(ret == Status::OK, ret);

    ret = InitAudioSinkInfo(meta);
    FALSE_RETURN_V(ret == Status::OK, ret);

    ret = SetAudioSinkPluginParameters();
    FALSE_RETURN_V(ret == Status::OK, ret);

    forceUpdateTimeAnchorNextTime_ = true;

    return Status::OK;
}

Status AudioSink::HandleFormatChange(std::shared_ptr<Meta>& meta,
    const std::shared_ptr<Pipeline::EventReceiver>& receiver)
{
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_INVALID_STATE);
    ScopedTimer timer("HandleFormatChange", FORMAT_CHANGE_MS);
    FALSE_RETURN_V_NOLOG(meta && plugin_ && plugin_->IsFormatSupported(meta), Status::ERROR_INVALID_PARAMETER);
    WaitForAllBufferConsumed();
    FALSE_RETURN_V_MSG(!isInterruptNeeded_, Status::OK, "Abandon format change operation becaused of interruption");
    plugin_->Drain();
    Flush();
    return ChangeTrack(meta, receiver);
}

void AudioSink::WaitForAllBufferConsumed()
{
    ScopedTimer timer("WaitForAllBufferConsumed", BUFFER_CONSUME_MS);
    MediaAVCodec::AVCodecTrace trace("AudioSink::WaitForAllBufferConsumed");
    std::unique_lock<std::mutex> formatLock(formatChangeMutex_);
    formatChange_ = true;
    formatChangeCond_.wait(formatLock,
        [this]() { return isInterruptNeeded_ || (swapOutputBuffers_.empty() && availOutputBuffers_.empty()); });
}

Status AudioSink::SetAudioSinkPluginParameters()
{
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    Status ret = Status::OK;
    if (volume_ >= 0) {
        plugin_->SetVolume(volume_);
    }
    if (speed_ >= 0) {
        plugin_->SetSpeed(speed_);
    }
    if (effectMode_ >= 0) {
        plugin_->SetAudioEffectMode(effectMode_);
    }
    if (state_ == Pipeline::FilterState::RUNNING) {
        ret = plugin_->Start();
    }
    return ret;
}

Status AudioSink::SetMuted(bool isMuted)
{
    isMuted_ = isMuted;
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    return plugin_->SetMuted(isMuted);
}

int32_t AudioSink::SetMaxAmplitudeCbStatus(bool status)
{
    calMaxAmplitudeCbStatus_ = status;
    MEDIA_LOG_I("audio SetMaxAmplitudeCbStatus  = " PUBLIC_LOG_D32, calMaxAmplitudeCbStatus_);
    return 0;
}

Status AudioSink::SetSeekTime(int64_t seekTime)
{
    MEDIA_LOG_I("AudioSink SetSeekTime pts = " PUBLIC_LOG_D64, seekTime);
    seekTimeUs_ = seekTime;
    return Status::OK;
}

void AudioSink::OnInterrupted(bool isInterruptNeeded)
{
    MEDIA_LOG_D("OnInterrupted %{public}d", isInterruptNeeded);
    {
        std::unique_lock<std::mutex> lock(formatChangeMutex_);
        isInterruptNeeded_ = isInterruptNeeded;
        formatChangeCond_.notify_all();
    }
    if (plugin_ != nullptr) {
        plugin_->SetInterruptState(isInterruptNeeded);
    }
}

Status AudioSink::SetIsCalledBySystemApp(bool isCalledBySystemApp)
{
    MEDIA_LOG_I("AudioSink isCalledBySystemApp = " PUBLIC_LOG_D32, isCalledBySystemApp);
    isCalledBySystemApp_ = isCalledBySystemApp;
    return Status::OK;
}

Status AudioSink::SetLooping(bool loop)
{
    isLoop_ = loop;
    MEDIA_LOG_I("AudioSink SetLooping isLoop_ = " PUBLIC_LOG_D32, isLoop_);
    return Status::OK;
}

Status AudioSink::SetAudioHapticsSyncId(int32_t syncId)
{
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    return plugin_->SetAudioHapticsSyncId(syncId);
}
} // namespace MEDIA
} // namespace OHOS
