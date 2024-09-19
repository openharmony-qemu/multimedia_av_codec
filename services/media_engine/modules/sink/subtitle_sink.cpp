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

#include "common/log.h"
#include "syspara/parameters.h"
#include "meta/format.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "SubtitleSink" };
}

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
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isThreadExit_ = true;
    }
    updateCond_.notify_all();
    if (readThread_ != nullptr && readThread_->joinable()) {
        readThread_->join();
        readThread_ = nullptr;
    }

    if (inputBufferQueueProducer_ != nullptr) {
        for (auto &buffer : inputBufferVector_) {
            inputBufferQueueProducer_->DetachBuffer(buffer);
        }
        inputBufferVector_.clear();
        inputBufferQueueProducer_->SetQueueSize(0);
    }
}

void SubtitleSink::NotifySeek()
{
    Flush();
}

void SubtitleSink::GetTargetSubtitleIndex(int64_t currentTime)
{
    int32_t left = 0;
    int32_t right = subtitleInfoVec_.size();
    while (left < right) {
        int32_t mid = (left + right) / 2;
        int64_t startTime = subtitleInfoVec_.at(mid).pts_;
        int64_t endTime = subtitleInfoVec_.at(mid).duration_ + startTime;
        if (startTime > currentTime) {
            right = mid;
            continue;
        } else if (endTime < currentTime) {
            left = mid + 1;
            continue;
        } else {
            left = mid;
            break;
        }
    }
    currentInfoIndex_ = left;
}

Status SubtitleSink::Init(std::shared_ptr<Meta> &meta, const std::shared_ptr<Pipeline::EventReceiver> &receiver)
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

Status SubtitleSink::SetParameter(const std::shared_ptr<Meta> &meta)
{
    return Status::OK;
}

Status SubtitleSink::GetParameter(std::shared_ptr<Meta> &meta)
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
    readThread_ = std::make_unique<std::thread>(&SubtitleSink::RenderLoop, this);
    pthread_setname_np(readThread_->native_handle(), "SubtitleRenderLoop");
    return Status::OK;
}

Status SubtitleSink::Stop()
{
    updateCond_.notify_all();
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
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isEos_ = false;
        state_ = Pipeline::FilterState::RUNNING;
    }
    updateCond_.notify_all();
    return Status::OK;
}

Status SubtitleSink::Flush()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        shouldUpdate_ = true;
        if (subtitleInfoVec_.size() > 0) {
            inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer_);
            subtitleInfoVec_.clear();
        }
    }
    isFlush_.store(true);
    updateCond_.notify_all();
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
    if (inputBufferQueue_ != nullptr && inputBufferQueue_->GetQueueSize() > 0) {
        MEDIA_LOG_I("InputBufferQueue already create");
        return Status::ERROR_INVALID_OPERATION;
    }
    int32_t inputBufferNum = 1;
    int32_t capacity = 1024;
    MemoryType memoryType = MemoryType::SHARED_MEMORY;
#ifndef MEDIA_OHOS
    memoryType = MemoryType::VIRTUAL_MEMORY;
#endif
    MEDIA_LOG_I("PrepareInputBufferQueue");
    if (inputBufferQueue_ == nullptr) {
        inputBufferQueue_ = AVBufferQueue::Create(inputBufferNum, memoryType, INPUT_BUFFER_QUEUE_NAME);
    }
    FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, Status::ERROR_UNKNOWN, "inputBufferQueue_ is nullptr");

    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();

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

void SubtitleSink::DrainOutputBuffer(bool flushed)
{
    Status ret;
    FALSE_RETURN(inputBufferQueueConsumer_ != nullptr);
    FALSE_RETURN(!isEos_.load());
    ret = inputBufferQueueConsumer_->AcquireBuffer(filledOutputBuffer_);
    if (filledOutputBuffer_->flag_ & BUFFER_FLAG_EOS) {
        isEos_ = true;
    }
    if (ret != Status::OK || filledOutputBuffer_ == nullptr || filledOutputBuffer_->memory_ == nullptr) {
        return;
    }
    std::string subtitleText(reinterpret_cast<const char *>(filledOutputBuffer_->memory_->GetAddr()),
                             filledOutputBuffer_->memory_->GetSize());
    SubtitleInfo subtitleInfo{ subtitleText, filledOutputBuffer_->pts_, filledOutputBuffer_->duration_ };
    {
        std::unique_lock<std::mutex> lock(mutex_);
        subtitleInfoVec_.push_back(subtitleInfo);
    }
    updateCond_.notify_all();
}

void SubtitleSink::RenderLoop()
{
    while (SUBTITME_LOOP_RUNNING) {
        std::unique_lock<std::mutex> lock(mutex_);
        updateCond_.wait(lock, [this] {
            return isThreadExit_.load() ||
                   (subtitleInfoVec_.size() > 0 && state_ == Pipeline::FilterState::RUNNING);
        });
        if (isFlush_) {
            MEDIA_LOG_I("SubtitleSink RenderLoop flush");
            isFlush_.store(false);
            continue;
        }
        FALSE_RETURN(!isThreadExit_.load());
        // wait timeout, seek or stop
        SubtitleInfo tempSubtitleInfo = subtitleInfoVec_.back();
        SubtitleInfo subtitleInfo{ tempSubtitleInfo.text_, tempSubtitleInfo.pts_, tempSubtitleInfo.duration_ };
        int64_t waitTime = CalcWaitTime(subtitleInfo);
        updateCond_.wait_for(lock, std::chrono::microseconds(waitTime),
                             [this] { return isThreadExit_.load() || shouldUpdate_; });
        MEDIA_LOG_I("SubtitleSink NotifyRender buffer. pts = " PUBLIC_LOG_D64 " waitTime: " PUBLIC_LOG_D64,
            subtitleInfo.pts_, waitTime);
        if (isFlush_) {
            MEDIA_LOG_I("SubtitleSink RenderLoop flush");
            isFlush_.store(false);
            continue;
        }
        FALSE_RETURN(!isThreadExit_.load());
        auto actionToDo = ActionToDo(subtitleInfo);
        if (actionToDo == SubtitleBufferState::DROP) {
            inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer_);
            subtitleInfoVec_.clear();
            continue;
        } else if (actionToDo == SubtitleBufferState::WAIT) {
            continue;
        } else {}
        NotifyRender(subtitleInfo);
        inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer_);
        subtitleInfoVec_.clear();
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

uint64_t SubtitleSink::CalcWaitTime(SubtitleInfo &subtitleInfo)
{
    int64_t curTime;
    if (shouldUpdate_.load()) {
        shouldUpdate_ = false;
    }
    curTime = GetMediaTime();
    if (subtitleInfo.pts_ < curTime) {
        return 0;
    }
    return (subtitleInfo.pts_ - curTime) / speed_;
}

uint32_t SubtitleSink::ActionToDo(SubtitleInfo &subtitleInfo)
{
    auto curTime = GetMediaTime();
    if (shouldUpdate_ || subtitleInfo.pts_ + subtitleInfo.duration_ < curTime) {
        return SubtitleBufferState::DROP;
    }
    if (subtitleInfo.pts_ > curTime || state_ != Pipeline::FilterState::RUNNING) {
        return SubtitleBufferState::WAIT;
    }
    subtitleInfo.duration_ -= curTime - subtitleInfo.pts_;
    return SubtitleBufferState::SHOW;
}

int64_t SubtitleSink::DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer> &buffer)
{
    (void)buffer;
    return 0;
}

void SubtitleSink::NotifyRender(SubtitleInfo &subtitleInfo)
{
    Format format;
    (void)format.PutStringValue(Tag::SUBTITLE_TEXT, subtitleInfo.text_);
    (void)format.PutIntValue(Tag::SUBTITLE_PTS, Plugins::Us2Ms(subtitleInfo.pts_));
    (void)format.PutIntValue(Tag::SUBTITLE_DURATION, Plugins::Us2Ms(subtitleInfo.duration_));
    Event event{ .srcFilter = "SubtitleSink", .type = EventType::EVENT_SUBTITLE_TEXT_UPDATE, .param = format };
    FALSE_RETURN(playerEventReceiver_ != nullptr);
    playerEventReceiver_->OnEvent(event);
}

void SubtitleSink::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver)
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
    FALSE_RETURN_V_MSG_W(speed > 0, Status::OK, "Invalid speed %{public}f", speed);
    {
        std::unique_lock<std::mutex> lock(mutex_);
        speed_ = speed;
        shouldUpdate_ = true;
    }
    updateCond_.notify_all();
    return Status::OK;
}

int64_t SubtitleSink::GetMediaTime()
{
    auto syncCenter = syncCenter_.lock();
    if (!syncCenter) {
        return 0;
    }
    return syncCenter->GetMediaTimeNow();
}
}  // namespace MEDIA
}  // namespace OHOS