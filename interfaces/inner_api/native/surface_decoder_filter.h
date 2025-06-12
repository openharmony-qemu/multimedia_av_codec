/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#ifndef FILTERS_SURFACE_DECODER_FILTER_H
#define FILTERS_SURFACE_DECODER_FILTER_H

#include <cstring>
#include "filter/filter.h"
#include "surface.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "common/status.h"
#include "common/log.h"
#include "avcodec_common.h"

namespace OHOS {
namespace Media {
class SurfaceDecoderAdapter;
namespace Pipeline {
class SurfaceDecoderFilter : public Filter, public std::enable_shared_from_this<SurfaceDecoderFilter> {
public:
    explicit SurfaceDecoderFilter(const std::string& name, FilterType type);
    ~SurfaceDecoderFilter() override;

    void SetCodecFormat(const std::shared_ptr<Meta> &format);
    void Init(const std::shared_ptr<EventReceiver> &receiver,
        const std::shared_ptr<FilterCallback> &callback) override;
    Status Configure(const std::shared_ptr<Meta> &parameter);
    Status SetOutputSurface(sptr<Surface> surface);
    Status NotifyNextFilterEos(int64_t pts, int64_t frameNum);
    Status DoPrepare() override;
    Status DoStart() override;
    Status DoPause() override;
    Status DoResume() override;
    Status DoStop() override;
    Status DoFlush() override;
    Status DoRelease() override;
    void SetParameter(const std::shared_ptr<Meta>& parameter) override;
    void GetParameter(std::shared_ptr<Meta>& parameter) override;
    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    FilterType GetFilterType();
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);
    void OnUpdatedResult(std::shared_ptr<Meta> &meta);
    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);
    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode);
    sptr<AVBufferQueueProducer> GetInputBufferQueue();

protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    Status ConfigureMediaCodecByMimeType(std::string codecMimeType, bool isHdrVivid);
    std::string name_;
    FilterType filterType_ = FilterType::FILTERTYPE_MAX;

    std::shared_ptr<EventReceiver> eventReceiver_{nullptr};
    std::shared_ptr<FilterCallback> filterCallback_{nullptr};
    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_{nullptr};
    std::shared_ptr<SurfaceDecoderAdapter> mediaCodec_{nullptr};

    std::string codecMimeType_{};
    int32_t colorSpace_;
    bool transcoderIsHdrVivid_{false};
    std::shared_ptr<Meta> configureParameter_{nullptr};

    std::shared_ptr<Filter> nextFilter_{nullptr};

    sptr<Surface> outputSurface_{nullptr};

    std::shared_ptr<Meta> meta_{nullptr};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // FILTERS_SURFACE_DECODER_FILTER_H
