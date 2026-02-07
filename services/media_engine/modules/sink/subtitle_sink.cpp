/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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
    int32_t right = static_cast<int32_t>(subtitleInfoVec_.size());
    while (left < right) {
        int32_t mid = (left + right) / 2;
        int64_t startTime = subtitleInfoVec_[mid].pts_;
        int64_t endTime = subtitleInfoVec_[mid].duration_ + startTime;
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
    currentInfoIndex_ = static_cast<uint32_t>(left);
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
    if (inputBufferQueueConsumer_ != nullptr) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            shouldUpdate_ = true;
            while (!subtitleInfoVec_.empty()) {
                inputBufferQueueConsumer_->ReleaseBuffer(subtitleInfoVec_.front().buffer_);
                subtitleInfoVec_.pop_front();
            }
        }
        uint32_t queueSize = inputBufferQueueConsumer_->GetQueueSize();
        std::shared_ptr<AVBuffer> filledOutputBuffer;
        for (uint32_t i = 0; i < queueSize; i++) {
            Status ret = inputBufferQueueConsumer_->AcquireBuffer(filledOutputBuffer);
            if (ret != Status::OK || filledOutputBuffer == nullptr) {
                break;
            }
            inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
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
    int32_t inputBufferNum = 2;
    int32_t capacity = 1024;
    MemoryType memoryType;
#ifndef MEDIA_OHOS
    memoryType = MemoryType::VIRTUAL_MEMORY;
#else
    memoryType = MemoryType::SHARED_MEMORY;
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
    FALSE_RETURN_V_NOLOG(playerEventReceiver_ != nullptr, Status::OK);
    playerEventReceiver_->OnMemoryUsageEvent({"SUBTITLE_BQ",
        DfxEventType::DFX_INFO_MEMORY_USAGE, inputBufferQueue_->GetMemoryUsage()});
    return Status::OK;
}

void SubtitleSink::DrainOutputBuffer(bool flushed)
{
    Status ret;
    FALSE_RETURN(inputBufferQueueConsumer_ != nullptr);
    FALSE_RETURN(!isEos_.load());
    std::shared_ptr<AVBuffer> filledOutputBuffer;
    ret = inputBufferQueueConsumer_->AcquireBuffer(filledOutputBuffer);
    if (ret != Status::OK || filledOutputBuffer == nullptr || filledOutputBuffer->memory_ == nullptr) {
        return;
    }
    if (filledOutputBuffer->flag_ & BUFFER_FLAG_EOS) {
        isEos_ = true;
    }
    std::string subtitleText(reinterpret_cast<const char *>(filledOutputBuffer->memory_->GetAddr()),
                             filledOutputBuffer->memory_->GetSize());
    SubtitleInfo subtitleInfo{ subtitleText, filledOutputBuffer->pts_, filledOutputBuffer->duration_,
            filledOutputBuffer };
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
                   (!subtitleInfoVec_.empty() && state_ == Pipeline::FilterState::RUNNING);
        });
        if (isFlush_) {
            MEDIA_LOG_I("SubtitleSink RenderLoop flush");
            isFlush_.store(false);
            continue;
        }
        FALSE_RETURN(!isThreadExit_.load());
        // wait timeout, seek or stop
        SubtitleInfo subtitleInfo = static_cast<int64_t>(subtitleInfoVec_.front());
        int64_t waitTime = CalcWaitTime(subtitleInfo);
        updateCond_.wait_for(lock, std::chrono::microseconds(waitTime),
                             [this] { return isThreadExit_.load() || shouldUpdate_; });
        if (isFlush_) {
            MEDIA_LOG_I("SubtitleSink RenderLoop flush");
            isFlush_.store(false);
            continue;
        }
        FALSE_RETURN(!isThreadExit_.load());
        auto actionToDo = ActionToDo(subtitleInfo);
        if (actionToDo == SubtitleBufferState::DROP) {
            subtitleInfoVec_.pop_front();
            inputBufferQueueConsumer_->ReleaseBuffer(subtitleInfo.buffer_);
            continue;
        } else if (actionToDo == SubtitleBufferState::WAIT) {
            continue;
        } else {}
        NotifyRender(subtitleInfo);
        subtitleInfoVec_.pop_front();
        inputBufferQueueConsumer_->ReleaseBuffer(subtitleInfo.buffer_);
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
        return -1;
    }
    return (subtitleInfo.pts_ - curTime) / speed_;
}

uint32_t SubtitleSink::ActionToDo(SubtitleInfo &subtitleInfo)
{
    auto curTime = GetMediaTime();
    if (shouldUpdate_ || subtitleInfo.pts_ + subtitleInfo.duration_ < curTime) {
        MEDIA_LOG_D("SubtitleInfo pts " PUBLIC_LOG_D64 " duration " PUBLIC_LOG_D64 " drop",
            subtitleInfo.pts_, subtitleInfo.duration_);
        return SubtitleBufferState::DROP;
    }
    if (subtitleInfo.pts_ > curTime || state_ != Pipeline::FilterState::RUNNING) {
        MEDIA_LOG_D("SubtitleInfo pts " PUBLIC_LOG_D64 " duration " PUBLIC_LOG_D64 " wait",
            subtitleInfo.pts_, subtitleInfo.duration_);
        return SubtitleBufferState::WAIT;
    }
    subtitleInfo.duration_ -= curTime - subtitleInfo.pts_;
    MEDIA_LOG_D("SubtitleInfo pts " PUBLIC_LOG_D64 " duration " PUBLIC_LOG_D64 " show",
        subtitleInfo.pts_, subtitleInfo.duration_);
    return SubtitleBufferState::SHOW;
}

int64_t SubtitleSink::DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer> &buffer, int64_t& actionClock)
{
    (void)buffer;
    (void)actionClock;
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

void SubtitleSink::OnInterrupted(bool isInterruptNeeded)
{
    MEDIA_LOG_I("onInterrupted %{public}d", isInterruptNeeded);
    std::unique_lock<std::mutex> lock(mutex_);
    isInterruptNeeded_ = isInterruptNeeded;
    isThreadExit_ = true;
    updateCond_.notify_all();
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