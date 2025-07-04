/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "syspara/parameters.h"
#include "plugin/plugin_manager_v2.h"
#include "common/log.h"
#include "calc_max_amplitude.h"

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
}

namespace OHOS {
namespace Media {

const int32_t DEFAULT_BUFFER_QUEUE_SIZE = 8;
const int32_t APE_BUFFER_QUEUE_SIZE = 32;
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
    MEDIA_LOG_I("AudioSink ctor");
    syncerPriority_ = IMediaSynchronizer::AUDIO_SINK;
    fixDelay_ = GetAudioLatencyFixDelay();
    plugin_ = CreatePlugin();
}

AudioSink::~AudioSink()
{
    MEDIA_LOG_I("AudioSink dtor");
}

Status AudioSink::Init(std::shared_ptr<Meta>& meta, const std::shared_ptr<Pipeline::EventReceiver>& receiver)
{
    state_ = Pipeline::FilterState::INITIALIZED;
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    if (meta != nullptr) {
        meta->SetData(Tag::APP_PID, appPid_);
        meta->SetData(Tag::APP_UID, appUid_);
    }
    plugin_->SetEventReceiver(receiver);
    plugin_->SetParameter(meta);
    plugin_->Init();
    plugin_->Prepare();
    plugin_->SetMuted(isMuted_);
    meta->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate_);
    meta->GetData(Tag::AUDIO_SAMPLE_PER_FRAME, samplePerFrame_);
    meta->GetData(Tag::AUDIO_CHANNEL_COUNT, audioChannelCount_);
    if (samplePerFrame_ > 0 && sampleRate_ > 0) {
        playingBufferDurationUs_ = samplePerFrame_ * 1000000 / sampleRate_; // 1000000 usec per sec
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

bool AudioSink::NeedImmediateRender()
{
    return isApe_ || isFlac_;
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
    if (meta != nullptr) {
        meta->GetData(Tag::APP_PID, appPid_);
        meta->GetData(Tag::APP_UID, appUid_);
    }
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    plugin_->SetParameter(meta);
    return Status::OK;
}

Status AudioSink::GetParameter(std::shared_ptr<Meta>& meta)
{
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
    Status ret = plugin_->Start();
    if (ret != Status::OK) {
        MEDIA_LOG_I("AudioSink start error " PUBLIC_LOG_D32, ret);
        return ret;
    }
    isEos_ = false;
    state_ = Pipeline::FilterState::RUNNING;
    return ret;
}

Status AudioSink::Stop()
{
    playRangeStartTime_ = DEFAULT_PLAY_RANGE_VALUE;
    playRangeEndTime_ = DEFAULT_PLAY_RANGE_VALUE;
    Status ret = plugin_->Stop();
    underrunDetector_.Reset();
    lagDetector_.Reset();
    forceUpdateTimeAnchorNextTime_ = true;
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
    if (isTransitent_ || (isEos_ && (isCalledBySystemApp_ || isLoop_))) {
        ret = plugin_->PauseTransitent();
    } else {
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
    Status ret = plugin_->Resume();
    if (ret != Status::OK) {
        MEDIA_LOG_I("AudioSink resume error " PUBLIC_LOG_D32, ret);
        return ret;
    }
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
    underrunDetector_.Reset();
    lagDetector_.Reset();
    {
        AutoLock lock(eosMutex_);
        eosInterruptType_ = EosInterruptState::NONE;
        eosDraining_ = false;
    }
    forceUpdateTimeAnchorNextTime_ = true;
    return plugin_->Flush();
}

Status AudioSink::Release()
{
    underrunDetector_.Reset();
    lagDetector_.Reset();
    return plugin_->Deinit();
}

Status AudioSink::SetPlayRange(int64_t start, int64_t end)
{
    MEDIA_LOG_I("SetPlayRange enter.");
    playRangeStartTime_ = start;
    playRangeEndTime_ = end;
    return Status::OK;
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
    MEDIA_LOG_I("AudioSink::SetVolumeWithRamp entered. ");
    return plugin_->SetVolumeWithRamp(targetVolume, duration);
}

Status AudioSink::SetIsTransitent(bool isTransitent)
{
    MEDIA_LOG_I("AudioSink::SetIsTransitent entered. ");
    isTransitent_ = isTransitent;
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
    plugin_->Drain();
    plugin_->PauseTransitent();
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

bool AudioSink::DropApeBuffer(std::shared_ptr<AVBuffer> filledOutputBuffer)
{
    if (!isApe_ || seekTimeUs_ == HST_TIME_NONE) {
        return false;
    }
    FALSE_RETURN_V(inputBufferQueueConsumer_ != nullptr, false);
    if (filledOutputBuffer->pts_ < seekTimeUs_) {
        MEDIA_LOG_D("Drop ape buffer pts = " PUBLIC_LOG_D64, filledOutputBuffer->pts_);
        inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
        return true;
    } else {
        seekTimeUs_ = HST_TIME_NONE;
    }
    return false;
}
void AudioSink::CheckUpdateState(char *frame, uint64_t replyBytes, int32_t format)
{
    FALSE_RETURN(frame != nullptr && replyBytes != 0);
    auto currentMaxAmplitude = OHOS::Media::CalcMaxAmplitude::UpdateMaxAmplitude(frame, replyBytes, format);
    AutoLock amplitudeLock(amplitudeMutex_);
    if (currentMaxAmplitude > maxAmplitude_) {
        maxAmplitude_ = currentMaxAmplitude;
    }
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

void AudioSink::DrainOutputBuffer(bool flushed)
{
    std::lock_guard<std::mutex> lock(pluginMutex_);
    std::shared_ptr<AVBuffer> filledOutputBuffer = nullptr;
    FALSE_RETURN(plugin_ != nullptr && inputBufferQueueConsumer_ != nullptr);
    Status ret = inputBufferQueueConsumer_->AcquireBuffer(filledOutputBuffer);
    FALSE_RETURN(ret == Status::OK && filledOutputBuffer != nullptr);
    if (state_ != Pipeline::FilterState::RUNNING || flushed) {
        MEDIA_LOG_E("Drop buffer pts = " PUBLIC_LOG_D64, filledOutputBuffer->pts_);
        inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
        return;
    }
    if ((filledOutputBuffer->flag_ & BUFFER_FLAG_EOS) ||
        ((playRangeEndTime_ != DEFAULT_PLAY_RANGE_VALUE) &&
        (filledOutputBuffer->pts_ > playRangeEndTime_ * MICROSECONDS_CONVERT_UNITS))) {
        inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
        AutoLock eosLock(eosMutex_);
        // avoid submit handle eos task multiple times
        FALSE_RETURN(!eosDraining_);
        eosInterruptType_ = EosInterruptState::INITIAL;
        if (eosTask_ == nullptr) {
            DrainAndReportEosEvent();
            return;
        }
        eosTask_->SubmitJobOnce([this] {
            HandleEosInner(false);
        });
        return;
    }
    FALSE_RETURN(DropApeBuffer(filledOutputBuffer) == false);
    UpdateAudioWriteTimeMayWait();
    DoSyncWrite(filledOutputBuffer);
    if (calMaxAmplitudeCbStatus_) {
        CalcMaxAmplitude(filledOutputBuffer);
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
    MEDIA_LOG_D("audio DrainOutputBuffer pts = " PUBLIC_LOG_D64, filledOutputBuffer->pts_);
    numFramesWritten_++;
    inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
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
    int64_t maxMediaTime = lastDrainTimeGroup_.anchorDuration + lastDrainTimeGroup_.lastAnchorPts;
    auto currentMediaTime = latency_ + lastDrainTimeGroup_.nowClockTime;
    auto writeTimeMs = lastDrainTimeGroup_.writeDuration;
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
        MEDIA_LOG_D("Audio drain cost " PUBLIC_LOG_D64, drainTimeDiff);
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
        MEDIA_LOG_D("No need to update time anchor this time.");
        return false;
    }
    uint64_t latency = 0;
    FALSE_LOG_MSG(plugin_->GetLatency(latency) == Status::OK, "failed to get latency");
    syncCenter->UpdateTimeAnchor(nowCt, latency + fixDelay_,
        buffer->pts_ - firstPts_, buffer->pts_, buffer->duration_, this);
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
    int32_t format = 0;
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
            format = AUDIO_SAMPLE_32_BIT;
            break;
        default:
            break;
    }
    FALSE_RETURN_V(format > 0, 0);
    return HST_MSECOND * size / format / sampleRate_ / audioChannelCount_;
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
    MEDIA_LOG_I("AudioSink::SetAudioEffectMode entered. ");
    if (plugin_ == nullptr) {
        return Status::ERROR_NULL_POINTER;
    }
    effectMode_ = effectMode;
    return plugin_->SetAudioEffectMode(effectMode);
}

Status AudioSink::GetAudioEffectMode(int32_t &effectMode)
{
    MEDIA_LOG_I("AudioSink::GetAudioEffectMode entered.");
    if (plugin_ == nullptr) {
        return Status::ERROR_NULL_POINTER;
    }
    return plugin_->GetAudioEffectMode(effectMode);
}

int64_t AudioSink::getPendingAudioPlayoutDurationUs(int64_t nowUs)
{
    int64_t writtenSamples = numFramesWritten_ * samplePerFrame_;
    const int64_t numFramesPlayed = plugin_->GetPlayedOutDurationUs(nowUs);
    int64_t pendingUs = (writtenSamples - numFramesPlayed) * HST_MSECOND/ sampleRate_;
    MEDIA_LOG_D("pendingUs" PUBLIC_LOG_D64, pendingUs);
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
    MEDIA_LOG_I("AudioSink::GetAudioEffectMode ChangeTrack. ");
    std::lock_guard<std::mutex> lock(pluginMutex_);
    Status res = Status::OK;

    if (plugin_) {
        plugin_->Stop();
        plugin_->Deinit();
        plugin_ = nullptr;
    }
    plugin_ = CreatePlugin();
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    if (meta != nullptr) {
        meta->SetData(Tag::APP_PID, appPid_);
        meta->SetData(Tag::APP_UID, appUid_);
    }
    plugin_->SetEventReceiver(receiver);
    plugin_->SetParameter(meta);
    plugin_->Init();
    plugin_->Prepare();
    plugin_->SetMuted(isMuted_);
    meta->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate_);
    meta->GetData(Tag::AUDIO_SAMPLE_PER_FRAME, samplePerFrame_);
    meta->GetData(Tag::AUDIO_CHANNEL_COUNT, audioChannelCount_);
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
        res = plugin_->Start();
    }
    forceUpdateTimeAnchorNextTime_ = true;
    return res;
}

Status AudioSink::SetMuted(bool isMuted)
{
    isMuted_ = isMuted;
    FALSE_RETURN_V(plugin_ != nullptr, Status::ERROR_NULL_POINTER);
    return plugin_->SetMuted(isMuted);
}

Status AudioSink::SetSeekTime(int64_t seekTime)
{
    MEDIA_LOG_I("AudioSink SetSeekTime pts = " PUBLIC_LOG_D64, seekTime);
    seekTimeUs_ = seekTime;
    return Status::OK;
}
int32_t AudioSink::SetMaxAmplitudeCbStatus(bool status)
{
    calMaxAmplitudeCbStatus_ = status;
    MEDIA_LOG_I("audio SetMaxAmplitudeCbStatus  = " PUBLIC_LOG_D32, calMaxAmplitudeCbStatus_);
    return 0;
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
} // namespace MEDIA
} // namespace OHOS
