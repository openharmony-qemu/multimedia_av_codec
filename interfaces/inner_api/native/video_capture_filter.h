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

#ifndef FILTERS_VIDEO_CAPTURE_FILTER_H
#define FILTERS_VIDEO_CAPTURE_FILTER_H

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

#define TIME_NONE ((int64_t) -1)

namespace OHOS {
namespace Media {
namespace Pipeline {
class VideoCaptureFilter : public Filter, public std::enable_shared_from_this<VideoCaptureFilter> {
public:
    explicit VideoCaptureFilter(std::string name, FilterType type);
    ~VideoCaptureFilter() override;
    Status SetCodecFormat(const std::shared_ptr<Meta> &format);
    void Init(const std::shared_ptr<EventReceiver> &receiver,
        const std::shared_ptr<FilterCallback> &callback) override;
    Status Configure(const std::shared_ptr<Meta> &parameter);
    Status SetInputSurface(sptr<Surface> surface);
    sptr<Surface> GetInputSurface();
    Status DoPrepare() override;
    Status DoStart() override;
    Status DoPause() override;
    Status DoResume() override;
    Status DoStop() override;
    Status DoFlush() override;
    Status DoRelease() override;
    Status NotifyEos();
    void SetParameter(const std::shared_ptr<Meta> &parameter) override;
    void GetParameter(std::shared_ptr<Meta> &parameter) override;
    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    FilterType GetFilterType();
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);
    void OnUpdatedResult(std::shared_ptr<Meta> &meta);
    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);
    void OnBufferAvailable();

protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    void UpdateBufferConfig(std::shared_ptr<AVBuffer> buffer, int64_t timestamp);
    static constexpr uint32_t ENCODE_USAGE = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE |
                                             BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_VIDEO_ENCODER;
    std::string name_;
    FilterType filterType_;

    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;

    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;
    std::shared_ptr<Meta> configureParameter_;
    std::shared_ptr<Filter> nextFilter_;

    sptr<AVBufferQueueProducer> outputBufferQueueProducer_;

    sptr<Surface> inputSurface_;

    bool isStop_{false};
    bool refreshTotalPauseTime_{false};
    int64_t startBufferTime_{TIME_NONE};
    int64_t latestBufferTime_{TIME_NONE};
    int64_t latestPausedTime_{TIME_NONE};
    int64_t totalPausedTime_{0};
};
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
#endif // FILTERS_VIDEO_CAPTURE_FILTER_H
