/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#include "sei_parser_filter.h"

#include "common/log.h"
#include "filter/filter_factory.h"
#include "media_core.h"
#include "osal/utils/util.h"
#include "osal/utils/dump_buffer.h"
#include "parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "SeiParserFilter" };
constexpr float VIDEO_CAPACITY_RATE = 1.5F;
constexpr int32_t DEFAULT_BUFFER_CAPACITY = 1024 * 1024;
}

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace Plugins;

static AutoRegisterFilter<SeiParserFilter> g_registerSeiParserFilter(
    "builtin.player.seiParser", FilterType::FILTERTYPE_SEI, [](const std::string &name, const FilterType type) {
        return std::make_shared<SeiParserFilter>(name, FilterType::FILTERTYPE_SEI);
    });

SeiParserFilter::AVBufferAvailableListener::AVBufferAvailableListener(std::shared_ptr<SeiParserFilter> seiParserFilter)
{
    seiParserFilter_ = seiParserFilter;
}

void SeiParserFilter::AVBufferAvailableListener::OnBufferAvailable()
{
    auto seiParserFilter = seiParserFilter_.lock();
    FALSE_RETURN_MSG(seiParserFilter != nullptr, "invalid seiParserFilter");
    seiParserFilter->ProcessInputBuffer();
}

SeiParserFilter::SeiParserFilter(const std::string &name, FilterType filterType)
    : Filter(name, FilterType::FILTERTYPE_SEI, false)
{
    filterType_ = filterType;
}

SeiParserFilter::~SeiParserFilter()
{
    MEDIA_LOG_I("SeiParserFilter dtor called");
}

void SeiParserFilter::Init(
    const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("SeiParserFilter Init enter.");
    Filter::Init(receiver, callback);
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status SeiParserFilter::DoInitAfterLink()
{
    MEDIA_LOG_I("DoInit enter the codecMimeType_ is %{public}s", codecMimeType_.c_str());
    return Status::OK;
}

Status SeiParserFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare enter.");
    PrepareState();
    inputBufferQueueConsumer_ = GetBufferQueueConsumer();
    FALSE_RETURN_V_MSG(inputBufferQueueConsumer_ != nullptr, Status::ERROR_NULL_POINTER,
                       "inputBufferQueueConsumer_ is nullptr");
    sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    if (onLinkedResultCallback_ != nullptr) {
        onLinkedResultCallback_->OnLinkedResult(GetBufferQueueProducer(), trackMeta_);
    }

    FALSE_RETURN_V_MSG(seiMessageCbStatus_, Status::OK, "disenable parse sei info");
    return Status::OK;
}

Status SeiParserFilter::PrepareState()
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

Status SeiParserFilter::PrepareInputBufferQueue()
{
    if (inputBufferQueue_ != nullptr && inputBufferQueue_->GetQueueSize() > 0) {
        MEDIA_LOG_I("InputBufferQueue already create");
        return Status::ERROR_INVALID_OPERATION;
    }
    FALSE_RETURN_V(trackMeta_ != nullptr, Status::ERROR_UNKNOWN);
    int32_t videoHeight = 0;
    int32_t videoWidth = 0;
    auto metaRes = trackMeta_->Get<Tag::VIDEO_HEIGHT>(videoHeight) && trackMeta_->Get<Tag::VIDEO_WIDTH>(videoWidth);
    int32_t capacity = metaRes ? videoWidth * videoHeight * VIDEO_CAPACITY_RATE : DEFAULT_BUFFER_CAPACITY;
    if (capacity <= 0 || capacity > INT32_MAX) {
        capacity = DEFAULT_BUFFER_CAPACITY;
    }
    MemoryType memoryType = MemoryType::VIRTUAL_MEMORY;

    MEDIA_LOG_I("PrepareInputBufferQueue");
    int32_t inputBufferNum = 1;
    if (inputBufferQueue_ == nullptr) {
        inputBufferQueue_ = AVBufferQueue::Create(inputBufferNum, memoryType, INPUT_BUFFER_QUEUE_NAME);
    }
    FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, Status::ERROR_UNKNOWN, "inputBufferQueue_ is nullptr");
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    FALSE_RETURN_V_MSG_E(
        inputBufferQueueProducer_ != nullptr, Status::ERROR_UNKNOWN, "inputBufferQueueProducer_ is nullptr");
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();

    for (int i = 0; i < inputBufferNum; i++) {
        std::shared_ptr<AVAllocator> avAllocator;
        MEDIA_LOG_D("CreateVirtualAllocator,i=%{public}d capacity=%{public}d", i, capacity);
        avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
        std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
        FALSE_RETURN_V_MSG_E(inputBuffer != nullptr, Status::ERROR_UNKNOWN, "inputBuffer is nullptr");
        inputBufferQueueProducer_->AttachBuffer(inputBuffer, false);
        MEDIA_LOG_I(
            "Attach intput buffer. index: %{public}d, bufferId: %{public}" PRIu64, i, inputBuffer->GetUniqueId());
    }
    FALSE_RETURN_V_NOLOG(eventReceiver_ != nullptr, Status::OK);
    eventReceiver_->OnMemoryUsageEvent({"SEI_BQ",
        DfxEventType::DFX_INFO_MEMORY_USAGE, inputBufferQueue_->GetMemoryUsage()});
    return Status::OK;
}

sptr<AVBufferQueueProducer> SeiParserFilter::GetBufferQueueProducer()
{
    if (state_ != Pipeline::FilterState::READY) {
        return nullptr;
    }
    return inputBufferQueueProducer_;
}

sptr<AVBufferQueueConsumer> SeiParserFilter::GetBufferQueueConsumer()
{
    if (state_ != Pipeline::FilterState::READY) {
        return nullptr;
    }
    return inputBufferQueueConsumer_;
}

Status SeiParserFilter::DoProcessInputBuffer(int recvArg, bool dropFrame)
{
    DrainOutputBuffer(dropFrame);
    return Status::OK;
}

Status SeiParserFilter::OnLinked(
    StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback)
{
    FALSE_RETURN_V_MSG(meta != nullptr && meta->GetData(Tag::MIME_TYPE, codecMimeType_),
        Status::ERROR_INVALID_PARAMETER, "get mime failed.");
    trackMeta_ = meta;
    onLinkedResultCallback_ = callback;
    return Filter::OnLinked(inType, meta, callback);
}

Status SeiParserFilter::OnUpdated(
    StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback)
{
    return Filter::OnUpdated(inType, meta, callback);
}

Status SeiParserFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback> &callback)
{
    return Filter::OnUnLinked(inType, callback);
}

void SeiParserFilter::DrainOutputBuffer(bool flushed)
{
    FALSE_RETURN(inputBufferQueueConsumer_ != nullptr);
    inputBufferQueueConsumer_->AcquireBuffer(filledOutputBuffer_);
    inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer_);
}

Status SeiParserFilter::SetSeiMessageCbStatus(bool status, const std::vector<int32_t> &payloadTypes)
{
    MEDIA_LOG_I("seiMessageCbStatus_  = " PUBLIC_LOG_D32, seiMessageCbStatus_);
    seiMessageCbStatus_ = status;
    FALSE_RETURN_V_MSG(
        inputBufferQueueProducer_ != nullptr, Status::ERROR_NO_MEMORY, "get producer failed");
    if (producerListener_ == nullptr) {
        producerListener_ =
            new SeiParserListener(codecMimeType_, inputBufferQueueProducer_, eventReceiver_, true);
        FALSE_RETURN_V_MSG(
            producerListener_ != nullptr, Status::ERROR_NO_MEMORY, "sei listener create failed");
        if (syncCenter_ != nullptr) {
            producerListener_->SetSyncCenter(syncCenter_);
        } else {
            MEDIA_LOG_W("syncCenter_ is nullptr");
        }
    }
    producerListener_->SetSeiMessageCbStatus(status, payloadTypes);
    return Status::OK;
}

void SeiParserFilter::SetSyncCenter(std::shared_ptr<IMediaSyncCenter> syncCenter)
{
    syncCenter_ = syncCenter;
    FALSE_RETURN(producerListener_ != nullptr);
    producerListener_->SetSyncCenter(syncCenter);
}

void SeiParserFilter::OnInterrupted(bool isInterruptNeeded)
{
    FALSE_RETURN_MSG(producerListener_ != nullptr, "no sei parser listener now");
    producerListener_->OnInterrupted(isInterruptNeeded);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS