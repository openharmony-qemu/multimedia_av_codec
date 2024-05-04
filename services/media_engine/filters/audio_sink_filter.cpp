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

#define HST_LOG_TAG "AudioSinkFilter"
#include "audio_sink_filter.h"
#include "common/log.h"
#include "osal/utils/util.h"
#include "osal/utils/dump_buffer.h"
#include "filter/filter_factory.h"
#include "media_core.h"
#include "parameters.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace Plugins;

static AutoRegisterFilter<AudioSinkFilter> g_registerAudioSinkFilter("builtin.player.audiosink",
    FilterType::FILTERTYPE_ASINK, [](const std::string& name, const FilterType type) {
        return std::make_shared<AudioSinkFilter>(name, FilterType::FILTERTYPE_ASINK);
    });

static const bool IS_FILTER_ASYNC = system::GetParameter("persist.media_service.async_filter", "1") == "1";

AudioSinkFilter::AVBufferAvailableListener::AVBufferAvailableListener(std::shared_ptr<AudioSinkFilter> audioSinkFilter)
{
    audioSinkFilter_ = audioSinkFilter;
}

void AudioSinkFilter::AVBufferAvailableListener::OnBufferAvailable()
{
    if (auto sink = audioSinkFilter_.lock()) {
        sink->ProcessInputBuffer();
    } else {
        MEDIA_LOG_I("invalid audioSink");
    }
}

AudioSinkFilter::AudioSinkFilter(const std::string& name, FilterType filterType)
    : Filter(name, FilterType::FILTERTYPE_ASINK, IS_FILTER_ASYNC)
{
    filterType_ = filterType;
    audioSink_ = std::make_shared<AudioSink>();
    MEDIA_LOG_I("audio sink ctor called");
}

AudioSinkFilter::~AudioSinkFilter()
{
    MEDIA_LOG_I("dtor called");
}

void AudioSinkFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
                           const std::shared_ptr<FilterCallback> &callback)
{
    Filter::Init(receiver, callback);
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    MEDIA_LOG_I("audio sink Init called");
}

Status AudioSinkFilter::DoInitAfterLink()
{
    audioSink_->SetParameter(globalMeta_);
    Status ret = audioSink_->Init(trackMeta_, eventReceiver_);
    audioSink_->SetEventReceiver(eventReceiver_);
    return ret;
}

Status AudioSinkFilter::DoPrepare()
{
    audioSink_->Prepare();
    inputBufferQueueConsumer_ = audioSink_->GetBufferQueueConsumer();
    sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    if (onLinkedResultCallback_ != nullptr) {
        onLinkedResultCallback_->OnLinkedResult(audioSink_->GetBufferQueueProducer(), trackMeta_);
    }
    state_ = FilterState::READY;
    return Status::OK;
}

Status AudioSinkFilter::DoStart()
{
    MEDIA_LOG_I("start called");
    if (state_ == FilterState::RUNNING) {
        return Status::OK;
    }
    if (state_ != FilterState::READY && state_ != FilterState::PAUSED) {
        MEDIA_LOG_W("sink is not ready when start, state: " PUBLIC_LOG_D32, state_);
        return Status::ERROR_INVALID_OPERATION;
    }
    forceUpdateTimeAnchorNextTime_ = true;
    auto err = audioSink_->Start();
    if (err != Status::OK) {
        eventReceiver_->OnEvent({"audio_sink_filter", EventType::EVENT_ERROR, MSERR_AUD_RENDER_FAILED});
        return err;
    }
    state_ = FilterState::RUNNING;
    frameCnt_ = 0;
    return err;
}

Status AudioSinkFilter::DoPause()
{
    MEDIA_LOG_I("audio sink filter pause start");
    if (state_ == FilterState::PAUSED) {
        return Status::OK;
    }
    // only worked when state is working
    if (state_ != FilterState::READY && state_ != FilterState::RUNNING) {
        MEDIA_LOG_W("audio sink cannot pause when not working");
        return Status::ERROR_INVALID_OPERATION;
    }
    state_ = FilterState::PAUSED;
    auto err = audioSink_->Pause();
    MEDIA_LOG_D("audio sink filter pause end");
    return err;
}

Status AudioSinkFilter::DoResume()
{
    MEDIA_LOG_I("audio sink filter resume");
    if (state_ == FilterState::RUNNING) {
        return Status::OK;
    }
    // only worked when state is paused
    if (state_ == FilterState::PAUSED || state_ == FilterState::RUNNING) {
        forceUpdateTimeAnchorNextTime_ = true;
        state_ = FilterState::RUNNING;
        if (frameCnt_ > 0) {
            frameCnt_ = 0;
        }
        return audioSink_->Resume();
    }
    return Status::OK;
}

Status AudioSinkFilter::DoFlush()
{
    // only worked when state is working
    if (state_ != FilterState::PAUSED && state_ != FilterState::STOPPED) {
        MEDIA_LOG_W("audio sink cannot flush when not paused or stopped");
        return Status::ERROR_INVALID_OPERATION;
    }
    MEDIA_LOG_I("audio sink flush start");
    if (audioSink_ != nullptr) {
        audioSink_->Flush();
    }
    MEDIA_LOG_I("audio sink flush end");
    return Status::OK;
}

Status AudioSinkFilter::DoStop()
{
    if (state_ == FilterState::STOPPED) {
        return Status::OK;
    }
    MEDIA_LOG_I("audio sink stop start");
    if (audioSink_ != nullptr) {
        audioSink_->Stop();
    }
    state_ = FilterState::STOPPED;
    MEDIA_LOG_I("audio sink stop finish");
    return Status::OK;
}

Status AudioSinkFilter::DoRelease()
{
    return audioSink_->Release();
}

Status AudioSinkFilter::DoProcessInputBuffer(int recvArg, bool dropFrame)
{
    audioSink_->DrainOutputBuffer();
    return Status::OK;
}

int32_t AudioSinkFilter::SetVolumeWithRamp(float targetVolume, int32_t duration)
{
    MEDIA_LOG_I("start Flush");
    return audioSink_->SetVolumeWithRamp(targetVolume, duration);
}

void AudioSinkFilter::SetParameter(const std::shared_ptr<Meta>& meta)
{
    globalMeta_ = meta;
    audioSink_->SetParameter(meta);
}

void AudioSinkFilter::GetParameter(std::shared_ptr<Meta>& meta)
{
    audioSink_->GetParameter(meta);
}

Status AudioSinkFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta>& meta,
    const std::shared_ptr<FilterLinkCallback>& callback)
{
    Plugins::AudioRenderInfo audioRenderInfo;
    int32_t interruptMode;
    if (globalMeta_ != nullptr && meta != nullptr) {
        if (globalMeta_->GetData(OHOS::Media::Tag::AUDIO_RENDER_INFO, audioRenderInfo)) {
            meta->SetData(Tag::AUDIO_RENDER_INFO, audioRenderInfo);
        }
        if (globalMeta_->GetData(OHOS::Media::Tag::AUDIO_INTERRUPT_MODE, interruptMode)) {
            meta->SetData(Tag::AUDIO_INTERRUPT_MODE, interruptMode);
        }
    }
    trackMeta_ = meta;
    onLinkedResultCallback_ = callback;
    return Filter::OnLinked(inType, meta, callback);
}

Status AudioSinkFilter::SetVolume(float volume)
{
    FALSE_RETURN_V(audioSink_ != nullptr, Status::ERROR_INVALID_STATE);
    volume_ = volume;
    if (volume_ < 0) {
        MEDIA_LOG_D("No need to set volume because upper layer hasn't set it.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    auto err = audioSink_->SetVolume(volume);
    MEDIA_LOG_I("set volume " PUBLIC_LOG ".3f", volume);
    return err;
}

void AudioSinkFilter::SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter)
{
    FALSE_RETURN(audioSink_ != nullptr);
    audioSink_->SetSyncCenter(syncCenter);
}

Status AudioSinkFilter::SetSpeed(float speed)
{
    MEDIA_LOG_I("AudioSinkFilter::SetSpeed in, speed is " PUBLIC_LOG ".3f", speed);
    FALSE_RETURN_V(audioSink_ != nullptr, Status::ERROR_INVALID_STATE);
    if (speed < 0) {
        MEDIA_LOG_E("AudioSinkFilter::SetSpeed speed is less than 0.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    Status res = audioSink_->SetSpeed(speed);
    MEDIA_LOG_I("AudioSinkFilter::SetSpeed out");
    return res;
}

Status AudioSinkFilter::SetAudioEffectMode(int32_t effectMode)
{
    MEDIA_LOG_I("AudioSinkFilter::SetAudioEffectMode in");
    FALSE_RETURN_V(audioSink_ != nullptr, Status::ERROR_INVALID_STATE);

    Status res = audioSink_->SetAudioEffectMode(effectMode);
    return res;
}

Status AudioSinkFilter::GetAudioEffectMode(int32_t &effectMode)
{
    MEDIA_LOG_I("AudioSinkFilter::GetAudioEffectMode in");
    FALSE_RETURN_V(audioSink_ != nullptr, Status::ERROR_INVALID_STATE);
    Status res = audioSink_->GetAudioEffectMode(effectMode);
    return res;
}

Status AudioSinkFilter::SetIsTransitent(bool isTransitent)
{
    MEDIA_LOG_I("AudioSinkFilter::SetIsTransitent in");
    FALSE_RETURN_V(audioSink_ != nullptr, Status::ERROR_INVALID_STATE);
    return audioSink_->SetIsTransitent(isTransitent);
}

Status AudioSinkFilter::ChangeTrack(std::shared_ptr<Meta>& meta)
{
    MEDIA_LOG_I("AudioSinkFilter::ChangeTrack in");
    FALSE_RETURN_V(audioSink_ != nullptr, Status::ERROR_INVALID_STATE);
    return audioSink_->ChangeTrack(meta, eventReceiver_);
}

Status AudioSinkFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta>& meta,
    const std::shared_ptr<FilterLinkCallback>& callback)
{
    return Filter::OnUpdated(inType, meta, callback);
}

Status AudioSinkFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    return Filter::OnUnLinked(inType, callback);
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
