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

#define HST_LOG_TAG "SubtitleSinkFilter"
#include "subtitle_sink_filter.h"
#include "common/log.h"
#include "osal/utils/util.h"
#include "osal/utils/dump_buffer.h"
#include "filter/filter_factory.h"
#include "media_core.h"
#include "parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "SubtitleSinkFilter" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace Plugins;

static AutoRegisterFilter<SubtitleSinkFilter> g_registerSubtitleSinkFilter("builtin.player.subtitlesink",
    FilterType::FILTERTYPE_SSINK, [](const std::string& name, const FilterType type) {
        return std::make_shared<SubtitleSinkFilter>(name, FilterType::FILTERTYPE_SSINK);
    });

SubtitleSinkFilter::AVBufferAvailableListener::AVBufferAvailableListener(
    std::shared_ptr<SubtitleSinkFilter> subtitleSinkFilter)
{
    subtitleSinkFilter_ = subtitleSinkFilter;
}

void SubtitleSinkFilter::AVBufferAvailableListener::OnBufferAvailable()
{
    if (auto sink = subtitleSinkFilter_.lock()) {
        sink->ProcessInputBuffer();
    } else {
        MEDIA_LOG_I("invalid subtitleSink");
    }
}

SubtitleSinkFilter::SubtitleSinkFilter(const std::string& name, FilterType filterType)
    : Filter(name, FilterType::FILTERTYPE_SSINK, false)
{
    filterType_ = filterType;
    subtitleSink_ = std::make_shared<SubtitleSink>();
    MEDIA_LOG_I("subtitle sink ctor called");
}

SubtitleSinkFilter::~SubtitleSinkFilter()
{
    MEDIA_LOG_I("dtor called");
}

void SubtitleSinkFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    Filter::Init(receiver, callback);
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    MEDIA_LOG_I("subtitle sink Init called");
}

Status SubtitleSinkFilter::DoInitAfterLink()
{
    subtitleSink_->SetParameter(globalMeta_);
    Status ret = subtitleSink_->Init(trackMeta_, eventReceiver_);
    subtitleSink_->SetEventReceiver(eventReceiver_);
    return ret;
}

Status SubtitleSinkFilter::DoPrepare()
{
    subtitleSink_->Prepare();
    inputBufferQueueConsumer_ = subtitleSink_->GetBufferQueueConsumer();
    sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    if (onLinkedResultCallback_ != nullptr) {
        onLinkedResultCallback_->OnLinkedResult(subtitleSink_->GetBufferQueueProducer(), trackMeta_);
    }
    state_ = FilterState::READY;
    return Status::OK;
}

Status SubtitleSinkFilter::DoStart()
{
    MEDIA_LOG_I("start called");
    if (state_ == FilterState::RUNNING) {
        return Status::OK;
    }
    if (state_ != FilterState::READY && state_ != FilterState::PAUSED) {
        MEDIA_LOG_W("sink is not ready when start, state: " PUBLIC_LOG_D32, state_);
        return Status::ERROR_INVALID_OPERATION;
    }
    auto err = subtitleSink_->Start();
    if (err != Status::OK) {
        MEDIA_LOG_E("subtitleSink_ start failed");
        return err;
    }
    state_ = FilterState::RUNNING;
    frameCnt_ = 0;
    return err;
}

Status SubtitleSinkFilter::DoPause()
{
    MEDIA_LOG_I("subtitle sink filter pause start");
    if (state_ == FilterState::PAUSED || state_ == FilterState::STOPPED) {
        return Status::OK;
    }
    // only worked when state is working
    if (state_ != FilterState::READY && state_ != FilterState::RUNNING) {
        MEDIA_LOG_W("subtitle sink cannot pause when not working");
        return Status::ERROR_INVALID_OPERATION;
    }
    state_ = FilterState::PAUSED;
    auto err = subtitleSink_->Pause();
    MEDIA_LOG_D("subtitle sink filter pause end");
    return err;
}

Status SubtitleSinkFilter::DoResume()
{
    MEDIA_LOG_I("subtitle sink filter resume");
    // only worked when state is paused
    if (state_ == FilterState::PAUSED) {
        state_ = FilterState::RUNNING;
        if (frameCnt_ > 0) {
            frameCnt_ = 0;
        }
        return subtitleSink_->Resume();
    }
    return Status::OK;
}

Status SubtitleSinkFilter::DoFlush()
{
    MEDIA_LOG_I("subtitle sink flush start");
    if (subtitleSink_ != nullptr) {
        subtitleSink_->Flush();
    }
    MEDIA_LOG_I("subtitle sink flush end");
    return Status::OK;
}

Status SubtitleSinkFilter::DoStop()
{
    MEDIA_LOG_I("subtitle sink stop start");
    if (subtitleSink_ != nullptr) {
        subtitleSink_->Stop();
    }
    state_ = FilterState::STOPPED;
    MEDIA_LOG_I("subtitle sink stop finish");
    return Status::OK;
}

Status SubtitleSinkFilter::DoRelease()
{
    return subtitleSink_->Release();
}

Status SubtitleSinkFilter::DoProcessInputBuffer(int recvArg, bool dropFrame)
{
    subtitleSink_->DrainOutputBuffer(dropFrame);
    return Status::OK;
}

void SubtitleSinkFilter::SetParameter(const std::shared_ptr<Meta>& meta)
{
    globalMeta_ = meta;
    subtitleSink_->SetParameter(meta);
}

void SubtitleSinkFilter::GetParameter(std::shared_ptr<Meta>& meta)
{
    subtitleSink_->GetParameter(meta);
}

Status SubtitleSinkFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta>& meta,
    const std::shared_ptr<FilterLinkCallback>& callback)
{
    trackMeta_ = meta;
    onLinkedResultCallback_ = callback;
    return Filter::OnLinked(inType, meta, callback);
}

void SubtitleSinkFilter::SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter)
{
    FALSE_RETURN(subtitleSink_ != nullptr);
    subtitleSink_->SetSyncCenter(syncCenter);
}

Status SubtitleSinkFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta>& meta,
    const std::shared_ptr<FilterLinkCallback>& callback)
{
    return Filter::OnUpdated(inType, meta, callback);
}

Status SubtitleSinkFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    return Filter::OnUnLinked(inType, callback);
}

void SubtitleSinkFilter::NotifySeek()
{
    subtitleSink_->NotifySeek();
}

Status SubtitleSinkFilter::SetSpeed(float speed)
{
    return subtitleSink_->SetSpeed(speed);
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
