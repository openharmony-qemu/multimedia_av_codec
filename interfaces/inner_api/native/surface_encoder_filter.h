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

#ifndef FILTERS_SURFACE_ENCODER_FILTER_H
#define FILTERS_SURFACE_ENCODER_FILTER_H

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

namespace OHOS {
namespace Media {
class SurfaceEncoderAdapter;
namespace Pipeline {
class SurfaceEncoderFilter : public Filter, public std::enable_shared_from_this<SurfaceEncoderFilter> {
public:
    explicit SurfaceEncoderFilter(std::string name, FilterType type);
    ~SurfaceEncoderFilter() override;
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
    Status Reset();
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
    void SetCallingInfo(int32_t appUid, int32_t appPid, const std::string &bundleName, uint64_t instanceId);

protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    std::string name_;
    FilterType filterType_;

    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;

    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;

    std::shared_ptr<SurfaceEncoderAdapter> mediaCodec_;

    std::string codecMimeType_;
    std::shared_ptr<Meta> configureParameter_;

    std::shared_ptr<Filter> nextFilter_;

    std::atomic<bool> isUpdateCodecNeeded_ = false;
    sptr<Surface> surface_{nullptr};
    std::string bundleName_;
    uint64_t instanceId_{0};
    int32_t appUid_ {0};
    int32_t appPid_ {0};
};
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
#endif // FILTERS_SURFACE_ENCODER_FILTER_H
