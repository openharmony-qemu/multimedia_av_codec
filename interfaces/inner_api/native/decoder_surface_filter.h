/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd.
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

#ifndef MEDIA_PIPELINE_VIDEO_SINK_FILTER_H
#define MEDIA_PIPELINE_VIDEO_SINK_FILTER_H

#ifdef VIDEO_SUPPORT

#include <atomic>
#if !defined(OHOS_LITE) && defined(VIDEO_SUPPORT)
#include "refbase.h"
#include "surface/surface.h"
#endif
#include "codec_server.h"
#include "osal/task/condition_variable.h"
#include "osal/task/mutex.h"
#include "osal/task/task.h"
#include "module/video_sink.h"
#include "module/media_synchronous_sink.h"
#include "common/status.h"
#include "meta/meta.h"
#include "filter/filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class DecoderSurfaceFilter : public Filter {
public:
    explicit DecoderSurfaceFilter(const std::string& name, FilterType type);
    ~DecoderSurfaceFilter() override;

    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) override;

    Status Configure(const std::shared_ptr<Meta> &parameter);

    Status Prepare() override;

    Status Start() override;

    Status Pause() override;

    Status Resume() override;

    Status Stop() override;

    Status Flush() override;

    Status Release() override;

    void SetParameter(std::shared_ptr<Meta>& meta) override;

    void GetParameter(std::shared_ptr<Meta>& meta) override;

    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;

    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;

    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);

    void OnUpdatedResult(std::shared_ptr<Meta> &meta);

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);

    FilterType GetFilterType();

    void DrainInputBuffer(uint32_t index, std::shared_ptr<AVBuffer> &inputBuffer);

    void DrainOutputBuffer(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer);

    void UpdateAvailableInputBufferCount();

#if !defined(OHOS_LITE) && defined(VIDEO_SUPPORT)
    Status SetVideoSurface(sptr<Surface> surface);
#endif
    sptr<AVBufferQueueProducer> GetInputBufferQueue();

protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    std::string name_;
    FilterType filterType_;
    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;
    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;
    std::shared_ptr<MediaAVCodec::ICodecService> mediaCodec_;
    std::string codecMimeType_;
    std::shared_ptr<Meta> configureParameter_;
    std::shared_ptr<Meta> meta_;
    OSAL::Mutex inputCntMutex_ {};
    OSAL::ConditionVariable cond_ {};

    std::shared_ptr<Filter> nextFilter_;
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;

    int64_t refreshTime_ {0};
    bool isFirstFrame_ {true};
    uint32_t frameRate_ {0};
    bool forceRenderNextFrame_ {false};

    std::atomic<uint64_t> availableInputBufferCnt_ {0};
    std::atomic<uint64_t> renderFrameCnt_ {0};
    std::atomic<uint64_t> discardFrameCnt_ {0};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif
#endif // MEDIA_PIPELINE_VIDEO_SINK_FILTER_H