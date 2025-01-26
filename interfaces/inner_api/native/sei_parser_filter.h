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

#ifndef SEI_PARSER_FILTER_H
#define SEI_PARSER_FILTER_H

#include <atomic>

#include "plugin/plugin_info.h"
#include "filter/filter.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_define.h"
#include "sink/media_synchronous_sink.h"
#include "meta/format.h"
#include "sei_parser_helper.h"
#include "media_sync_manager.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class SeiParserFilter : public Filter, public InterruptListener, public std::enable_shared_from_this<SeiParserFilter> {
public:
    explicit SeiParserFilter(const std::string &name, FilterType filterType = FilterType::FILTERTYPE_SEI);
    ~SeiParserFilter() override;

    sptr<AVBufferQueueProducer> GetBufferQueueProducer();

    sptr<AVBufferQueueConsumer> GetBufferQueueConsumer();

    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) override;

    Status DoInitAfterLink() override;

    Status DoPrepare() override;

    void OnInterrupted(bool isInterruptNeeded) override;

    Status PrepareState();

    Status DoProcessInputBuffer(int recvArg, bool dropFrame) override;

    Status ParseSei(std::shared_ptr<AVBuffer> buffer);

    Status SetSeiMessageCbStatus(bool status, const std::vector<int32_t> &payloadTypes);

    void DrainOutputBuffer(bool flushed);

    void SetParseSeiEnabled(bool needParseSeiInfo);

    void SetSyncCenter(std::shared_ptr<IMediaSyncCenter> syncCenter);

protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback> &callback) override;

    class AVBufferAvailableListener : public IConsumerListener {
    public:
        AVBufferAvailableListener(std::shared_ptr<SeiParserFilter> SeiParserFilter);
        void OnBufferAvailable() override;

    private:
        std::weak_ptr<SeiParserFilter> seiParserFilter_;
    };

private:
    Status PrepareInputBufferQueue();

    const std::string INPUT_BUFFER_QUEUE_NAME = "SeiParserFilterInputBufferQueue";
    std::string name_;
    std::string codecMimeType_;
    FilterType filterType_ = FilterType::FILTERTYPE_SEI;
    FilterState state_ = FilterState::CREATED;
    std::shared_ptr<Meta> trackMeta_;
    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;
    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    std::shared_ptr<AVBuffer> filledOutputBuffer_;
    bool seiMessageCbStatus_{ false };
    std::vector<int32_t> payloadTypes_{};
    sptr<SeiParserListener> producerListener_ {};
    std::shared_ptr<IMediaSyncCenter> syncCenter_;
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // SEI_PARSER_FILTER_H
