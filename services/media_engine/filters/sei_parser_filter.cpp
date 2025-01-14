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
    if (auto seiParserFilter = seiParserFilter_.lock()) {
        seiParserFilter->ProcessInputBuffer();
    } else {
        MEDIA_LOG_I("invalid seiParserFilter");
    }
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
    sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    if (onLinkedResultCallback_ != nullptr) {
        onLinkedResultCallback_->OnLinkedResult(GetBufferQueueProducer(), trackMeta_);
    }

    FALSE_RETURN_V_MSG(seiMessageCbStatus_, Status::OK, "disenable parse sei info");
    SetSeiMessageListener();
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
    int32_t inputBufferNum = 1;
    int32_t capacity = 1024 * 1024;
    MemoryType memoryType = MemoryType::VIRTUAL_MEMORY;

    MEDIA_LOG_I("PrepareInputBufferQueue");
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
    MEDIA_LOG_I("SeiParserFilter OnLinked enter.");
    trackMeta_ = meta;
    FALSE_RETURN_V_MSG(
        meta->GetData(Tag::MIME_TYPE, codecMimeType_), Status::ERROR_INVALID_PARAMETER, "get mime failed.");
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

int32_t SeiParserFilter::SetSeiMessageCbStatus(bool status, const std::vector<int32_t> &payloadTypes)
{
    MEDIA_LOG_I(" SeiParserFilter seiMessageCbStatus_  = " PUBLIC_LOG_D32, seiMessageCbStatus_);
    payloadTypes_ = payloadTypes;
    if (status) {
        SetSeiMessageListener();
    } else if (!status && seiMessageCbStatus_) {
        RemoveSeiMessageListener();
    }
    seiMessageCbStatus_ = status;
    return 0;
}

void SeiParserFilter::SetSeiMessageListener()
{
    sptr<Media::AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueueProducer_;
    FALSE_RETURN_MSG(inputBufferQueueProducer != nullptr, "get producer failed");
    if (producerListener_ == nullptr) {
        producerListener_ =
            new SeiParserListener(codecMimeType_, inputBufferQueueProducer, eventReceiver_);
        FALSE_RETURN_MSG(producerListener_ != nullptr, "sei listener create failed");
    }
    producerListener_->SetPayloadTypeVec(payloadTypes_);
    FALSE_RETURN_MSG(seiMessageCbStatus_, "Has set listener before, need not set again");
    sptr<IBrokerListener> tmpListener = producerListener_;
    inputBufferQueueProducer->SetBufferFilledListener(tmpListener);
}

void SeiParserFilter::RemoveSeiMessageListener()
{
    sptr<Media::AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueueProducer_;
    FALSE_RETURN_MSG(inputBufferQueueProducer != nullptr, "get producer failed");
    FALSE_RETURN_MSG(producerListener_ != nullptr, "no sei parser listener now");
    sptr<IBrokerListener> tmpListener = producerListener_;
    inputBufferQueueProducer->RemoveBufferFilledListener(tmpListener);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
