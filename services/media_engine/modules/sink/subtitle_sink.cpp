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

#include "subtitle_sink.h"
#include "syspara/parameters.h"
#include "meta/format.h"

namespace OHOS {
namespace Media {
namespace {
constexpr bool SUBTITME_LOOP_RUNNING = true;
}

SubtitleSink::SubtitleSink()
{
    MEDIA_LOG_I("SubtitleSink ctor");
    syncerPriority_ = IMediaSynchronizer::SUBTITLE_SINK;
}

SubtitleSink::~SubtitleSink()
{
    MEDIA_LOG_I("SubtitleSink dtor");
    isThreadExit_ = true;
    condBufferAvailable_.notify_all();
    if (readThread_ != nullptr && readThread_->joinable()) {
        readThread_->join();
        readThread_ = nullptr;
    }
}

void SubtitleSink::SetSeekTime(int64_t timeUs)
{
    isSeek_ = true;
    seekTimeUs_ = timeUs;
    GetTargetSubtitleIndex();
    condBufferAvailable_.notify_all();
}

void SubtitleSink::GetTargetSubtitleIndex()
{
    auto bufferCount = subtitleInfoVec_.size();
    FALSE_RETURN(bufferCount > 0);
    int32_t left = 0;
    int32_t right = bufferCount - 1;
    while (left < right) {
        int32_t mid = (left + right) / 2;
        if (mid == left) {
            break;
        }
        if (subtitleInfoVec_.at(mid).pts_ > seekTimeUs_) {
            right = mid;
            continue;
        }
        left = mid;
    }
    currentInfoIndex_ = left;
}

Status SubtitleSink::Init(std::shared_ptr<Meta>& meta, const std::shared_ptr<Pipeline::EventReceiver>& receiver)
{
    state_ = Pipeline::FilterState::INITIALIZED;
    if (meta != nullptr) {
        meta->SetData(Tag::APP_PID, appPid_);
        meta->SetData(Tag::APP_UID, appUid_);
    }
    return Status::OK;
}

sptr<AVBufferQueueProducer> SubtitleSink::GetBufferQueueProducer()
{
    if (state_ != Pipeline::FilterState::READY) {
        return nullptr;
    }
    return inputBufferQueueProducer_;
}

sptr<AVBufferQueueConsumer> SubtitleSink::GetBufferQueueConsumer()
{
    if (state_ != Pipeline::FilterState::READY) {
        return nullptr;
    }
    return inputBufferQueueConsumer_;
}

Status SubtitleSink::SetParameter(const std::shared_ptr<Meta>& meta)
{
    return Status::OK;
}

Status SubtitleSink::GetParameter(std::shared_ptr<Meta>& meta)
{
    return Status::OK;
}

Status SubtitleSink::Prepare()
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

Status SubtitleSink::Start()
{
    isEos_ = false;
    state_ = Pipeline::FilterState::RUNNING;
    if (isPaused_.load()) {
        return Resume();
    }
    isThreadExit_ = false;
    isPaused_ = false;
    readThread_ = std::make_unique<std::thread>(&SubtitleSink::RenderLoop, this);
    pthread_setname_np(readThread_->native_handle(), "SubtitleRenderLoop");
    return Status::OK;
}

Status SubtitleSink::Stop()
{
    condBufferAvailable_.notify_all();
    state_ = Pipeline::FilterState::INITIALIZED;
    return Status::OK;
}

Status SubtitleSink::Pause()
{
    state_ = Pipeline::FilterState::PAUSED;
    return Status::OK;
}

Status SubtitleSink::Resume()
{
    state_ = Pipeline::FilterState::RUNNING;
    condBufferAvailable_.notify_all();
    return Status::OK;
}

Status SubtitleSink::Flush()
{
    return Status::OK;
}

Status SubtitleSink::Release()
{
    return Status::OK;
}

Status SubtitleSink::SetIsTransitent(bool isTransitent)
{
    isTransitent_ = isTransitent;
    return Status::OK;
}

Status SubtitleSink::PrepareInputBufferQueue()
{
    if (inputBufferQueue_ != nullptr && inputBufferQueue_-> GetQueueSize() > 0) {
        MEDIA_LOG_I("InputBufferQueue already create");
        return Status::ERROR_INVALID_OPERATION;
    }
    int inputBufferSize = 8;
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

void SubtitleSink::DrainOutputBuffer(bool flushed)
{
    Status ret;
    if (inputBufferQueueConsumer_ == nullptr) {
        return;
    }
    ret = inputBufferQueueConsumer_->AcquireBuffer(filledOutputBuffer_);
    if (ret != Status::OK || filledOutputBuffer_ == nullptr || filledOutputBuffer_->memory_ == nullptr) {
        return;
    }
    std::string subtitleText(reinterpret_cast<const char *>(filledOutputBuffer_->memory_->GetAddr()),
        filledOutputBuffer_->memory_->GetSize());
    SubtitleInfo subtitleInfo {
        subtitleText,
        filledOutputBuffer_->pts_,
        filledOutputBuffer_->duration_
    };
    subtitleInfoVec_.push_back(subtitleInfo);
    inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer_);
}

void SubtitleSink::RenderLoop()
{
    while (SUBTITME_LOOP_RUNNING) {
        if (currentInfoIndex_ >= subtitleInfoVec_.size()) {
            break;
        }
        SubtitleInfo subtitleInfo = subtitleInfoVec_.at(currentInfoIndex_);
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condBufferAvailable_.wait(lock, [this] { return !isPaused_.load() || isThreadExit_.load(); });
            if (isThreadExit_.load()) {
                break;
            }
        }
        int64_t waitTime = CalcWaitTime(subtitleInfo);
        if (speed_ != 0.0) {
            waitTime /= speed_;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        condBufferAvailable_.wait_for(lock, std::chrono::microseconds(waitTime), [this] {
            return isThreadExit_.load();
        });
        if (isThreadExit_) {
            return;
        }
        auto actionToDo = ActionToDo(subtitleInfo);
        if (state_ == Pipeline::FilterState::PAUSED || actionToDo == SubtitleBufferState::WAIT) {
            continue;
        }
        if (actionToDo == SubtitleBufferState::SHOW) {
            DoSyncWrite(subtitleInfo);
        }
        if (++currentInfoIndex_ >= subtitleInfoVec_.size()) {
            currentInfoIndex_ = subtitleInfoVec_.size() - 1;
        }
    }
}

void SubtitleSink::ResetSyncInfo()
{
    auto syncCenter = syncCenter_.lock();
    if (syncCenter) {
        syncCenter->Reset();
    }
    lastReportedClockTime_ = HST_TIME_NONE;
}

int64_t SubtitleSink::CalcWaitTime(SubtitleInfo subtitleInfo)
{
    auto syncCenter = syncCenter_.lock();
    if (!syncCenter) {
        return -1;
    }
    auto curTime = syncCenter->GetMediaTimeNow();
    return subtitleInfo.pts_ - curTime;
}

uint32_t SubtitleSink::ActionToDo(SubtitleInfo subtitleInfo)
{
    if (isSeek_) {
        isSeek_ = false;
        return SubtitleBufferState::DROP;
    }
    auto syncCenter = syncCenter_.lock();
    if (!syncCenter) {
        return SubtitleBufferState::DROP;
    }
    auto curTime = syncCenter->GetMediaTimeNow();
    if (subtitleInfo.pts_ > curTime) {
        return SubtitleBufferState::WAIT;
    }
    if (subtitleInfo.pts_ + subtitleInfo.duration_ < curTime) {
        return SubtitleBufferState::DROP;
    }
    return SubtitleBufferState::SHOW;
}

int64_t SubtitleSink::DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    (void)buffer;
    return 0;
}

int64_t SubtitleSink::DoSyncWrite(SubtitleInfo subtitleInfo)
{
    auto syncCenter = syncCenter_.lock();
    if (!syncCenter) {
        return -1;
    }
    auto curTime = syncCenter->GetMediaTimeNow();
    Format format;
    (void)format.PutStringValue(Tag::SUBTITLE_TEXT, subtitleInfo.text_);
    (void)format.PutIntValue(Tag::SUBTITLE_PTS, subtitleInfo.pts_);
    (void)format.PutIntValue(Tag::SUBTITLE_DURATION, subtitleInfo.duration_);
    Event event {
        .srcFilter = "SubtitleSink",
        .type = EventType::EVENT_SUBTITLE_TEXT_UPDATE,
        .param = format
    };
    FALSE_RETURN_V(playerEventReceiver_ != nullptr, 0);
    playerEventReceiver_->OnEvent(event);
    return subtitleInfo.pts_ - curTime;
}

void SubtitleSink::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver)
{
    FALSE_RETURN(receiver != nullptr);
    playerEventReceiver_ = receiver;
}

void SubtitleSink::SetSyncCenter(std::shared_ptr<Pipeline::MediaSyncManager> syncCenter)
{
    syncCenter_ = syncCenter;
    MediaSynchronousSink::Init();
}

Status SubtitleSink::SetSpeed(float speed)
{
    speed_ = speed;
    return Status::OK;
}
} // namespace MEDIA
} // namespace OHOS
