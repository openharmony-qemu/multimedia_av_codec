/*
* Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

namespace OHOS {
namespace Media {

const int32_t DEFAULT_BUFFER_QUEUE_SIZE = 8;
const int32_t APE_BUFFER_QUEUE_SIZE = 32;

int64_t GetAudioLatencyFixDelay()
{
    constexpr uint64_t defaultValue = 120 * HST_USECOND;
    static uint64_t fixDelay = OHOS::system::GetUintParameter("debug.media_service.audio_sync_fix_delay", defaultValue);
    MEDIA_LOG_I("audio_sync_fix_delay, pid:%{public}d, fixdelay: " PUBLIC_LOG_U64, getprocpid(), fixDelay);
    return (int64_t)fixDelay;
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
    meta->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate_);
    meta->GetData(Tag::AUDIO_SAMPLE_PER_FRAME, samplePerFrame_);

    std::string mime;
    bool mimeGetRes = meta->Get<Tag::MIME_TYPE>(mime);
    if (mimeGetRes && mime == "audio/x-ape") {
        isApe_ = true;
        MEDIA_LOG_I("AudioSink::Init is ape");
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
    Status ret = plugin_->Stop();
    if (ret != Status::OK) {
        return ret;
    }
    state_ = Pipeline::FilterState::INITIALIZED;
    return ret;
}

Status AudioSink::Pause()
{
    Status ret = Status::OK;
    if (isTransitent_ || isEos_) {
        ret = plugin_->PauseTransitent();
    } else {
        ret = plugin_->Pause();
    }
    if (ret != Status::OK) {
        return ret;
    }
    state_ = Pipeline::FilterState::PAUSED;
    return ret;
}

Status AudioSink::Resume()
{
    Status ret = plugin_->Resume();
    if (ret != Status::OK) {
        MEDIA_LOG_I("AudioSink resume error " PUBLIC_LOG_D32, ret);
        return ret;
    }
    state_ = Pipeline::FilterState::RUNNING;
    return ret;
}

Status AudioSink::Flush()
{
    return plugin_->Flush();
}

Status AudioSink::Release()
{
    return plugin_->Deinit();
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
    Plugins::PluginType pluginType = Plugins::PluginType::AUDIO_SINK;
    auto names = Plugins::PluginManager::Instance().ListPlugins(pluginType);
    std::string pluginName = "";
    for (auto& name : names) {
        auto info = Plugins::PluginManager::Instance().GetPluginInfo(pluginType, name);
        pluginName = name;
        break;
    }
    MEDIA_LOG_I("pluginName %{public}s", pluginName.c_str());
    if (!pluginName.empty()) {
        auto plugin = Plugins::PluginManager::Instance().CreatePlugin(pluginName, pluginType);
        return std::reinterpret_pointer_cast<Plugins::AudioSinkPlugin>(plugin);
    } else {
        MEDIA_LOG_E("No plugins matching output format");
    }
    return nullptr;
}

void AudioSink::DrainOutputBuffer()
{
    std::lock_guard<std::mutex> lock(pluginMutex_);
    Status ret;
    std::shared_ptr<AVBuffer> filledOutputBuffer = nullptr;
    if (plugin_ == nullptr || inputBufferQueueConsumer_ == nullptr) {
        return;
    }
    ret = inputBufferQueueConsumer_->AcquireBuffer(filledOutputBuffer);
    if (ret != Status::OK || filledOutputBuffer == nullptr) {
        return;
    }
    if (state_ != Pipeline::FilterState::RUNNING) {
        inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
        return;
    }
    if (filledOutputBuffer->flag_ & BUFFER_FLAG_EOS) {
        isEos_ = true;
        Event event {
            .srcFilter = "AudioSink",
            .type = EventType::EVENT_COMPLETE,
        };
        FALSE_RETURN(playerEventReceiver_ != nullptr);
        playerEventReceiver_->OnEvent(event);
        inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
        plugin_->Drain();
        plugin_->PauseTransitent();
        return;
    }

    DoSyncWrite(filledOutputBuffer);
    plugin_->Write(filledOutputBuffer);
    MEDIA_LOG_D("audio DrainOutputBuffer pts = " PUBLIC_LOG_D64, filledOutputBuffer->pts_);
    numFramesWritten_++;
    inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
}

void AudioSink::ResetSyncInfo()
{
    auto syncCenter = syncCenter_.lock();
    if (syncCenter) {
        syncCenter->Reset();
    }
    lastReportedClockTime_ = HST_TIME_NONE;
    forceUpdateTimeAnchorNextTime_ = false;
    firstPts_ = HST_TIME_NONE;
}

int64_t AudioSink::DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    bool render = true; // audio sink always report time anchor and do not drop
    int64_t nowCt = 0;

    if (firstPts_ == HST_TIME_NONE) {
        firstPts_ = buffer->pts_;
        MEDIA_LOG_I("audio DoSyncWrite set firstPts = " PUBLIC_LOG_D64, firstPts_);
    }

    auto syncCenter = syncCenter_.lock();
    if (syncCenter) {
        nowCt = syncCenter->GetClockTimeNow();
    }
    if (lastReportedClockTime_ == HST_TIME_NONE || forceUpdateTimeAnchorNextTime_) {
        uint64_t latency = 0;
        if (plugin_->GetLatency(latency) != Status::OK) {
            MEDIA_LOG_W("failed to get latency");
        }
        if (syncCenter) {
            render = syncCenter->UpdateTimeAnchor(nowCt, latency + fixDelay_,
                buffer->pts_ - firstPts_, buffer->pts_, buffer->duration_, this);
            MEDIA_LOG_D("AudioSink fixDelay_: " PUBLIC_LOG_D64
                " us, latency: " PUBLIC_LOG_D64
                " us, pts-f: " PUBLIC_LOG_D64
                " us, pts: " PUBLIC_LOG_D64
                " us, nowCt: " PUBLIC_LOG_D64 " us",
                fixDelay_, latency, buffer->pts_ - firstPts_, buffer->pts_, nowCt);
        }
        lastReportedClockTime_ = nowCt;
        forceUpdateTimeAnchorNextTime_ = true;
    }
    latestBufferPts_ = buffer->pts_ - firstPts_;
    latestBufferDuration_ = buffer->duration_;
    return render ? 0 : -1;
}

Status AudioSink::SetSpeed(float speed)
{
    if (plugin_ == nullptr) {
        return Status::ERROR_NULL_POINTER;
    }
    if (speed < 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    speed_ = speed;
    return plugin_->SetSpeed(speed);
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
    MEDIA_LOG_I("AudioSink::GetAudioEffectMode entered. ");
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
    render = syncCenter->UpdateTimeAnchor(nowUs, pendingTimeUs, mediaTimeUs, mediaTimeUs, mediaTimeUs, this);
    return render;
}

int64_t AudioSink::getPendingAudioPlayoutDurationUs(int64_t nowUs)
{
    int64_t writtenSamples = numFramesWritten_ * samplePerFrame_;
    const int64_t numFramesPlayed = plugin_->GetPlayedOutDurationUs(nowUs);
    int64_t pendingUs = (writtenSamples - numFramesPlayed) * HST_MSECOND / sampleRate_;
    MEDIA_LOG_D("pendingUs: " PUBLIC_LOG_D64, pendingUs);
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
    return (int64_t)(numFrames * HST_MSECOND / sampleRate);
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
    meta->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate_);
    meta->GetData(Tag::AUDIO_SAMPLE_PER_FRAME, samplePerFrame_);
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

    return res;
}

} // namespace MEDIA
} // namespace OHOS
