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

#ifndef FILTERS_ENCODER_FILTER_H
#define FILTERS_ENCODER_FILTER_H

#include <cstring>
#include "codec_server.h"
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
class SurfaceEncoderFilter : public Filter, public std::enable_shared_from_this<SurfaceEncoderFilter> {
public:
    explicit SurfaceEncoderFilter(std::string name, FilterType type);
    ~SurfaceEncoderFilter() override;

    Status SetCodecFormat(const std::shared_ptr<Meta> &format);

    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) override;

    Status Configure(const std::shared_ptr<Meta> &parameter);

    sptr<Surface> GetInputSurface();

    Status Prepare() override;

    Status Start() override;

    Status Pause() override;

    Status Resume() override;

    Status Stop() override;

    Status Flush() override;

    Status Release() override;

    Status NotifyEOS();

    void SetParameter(const std::shared_ptr<Meta> &parameter) override;

    void GetParameter(std::shared_ptr<Meta> &parameter) override;

    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;

    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;

    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;

    FilterType GetFilterType();

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);

    void OnUpdatedResult(std::shared_ptr<Meta> &meta);

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);

    void OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer);

protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    Status DoStop();

    std::string name_;
    FilterType filterType_;

    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;

    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;

    std::shared_ptr<MediaAVCodec::ICodecService> mediaCodec_;

    std::string codecMimeType_;
    std::shared_ptr<Meta> configureParameter_;

    std::shared_ptr<Filter> nextFilter_;

    sptr<AVBufferQueueProducer> outputBufferQueueProducer_;

    int64_t stopTime_{TIME_NONE};
    bool isStop_ {false};

    bool refreshTotalPauseTime_{false};
    int64_t latestBufferTime_{TIME_NONE};
    int64_t latestPausedTime_{TIME_NONE};
    int64_t totalPausedTime_{0};
};
} //namespace Pipeline
} //namespace MEDIA
} //namespace OHOS
#endif // FILTERS_CODEC_FILTER_H
