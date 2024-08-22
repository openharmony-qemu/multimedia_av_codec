/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_SUBTITLE_SINK_FILTER_H
#define HISTREAMER_SUBTITLE_SINK_FILTER_H

#include <atomic>

#include "media_sync_manager.h"
#include "subtitle_sink.h"
#include "plugin/plugin_info.h"
#include "filter/filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class SubtitleSinkFilter : public Filter, public std::enable_shared_from_this<SubtitleSinkFilter> {
public:
    explicit SubtitleSinkFilter(const std::string& name, FilterType filterType = FilterType::FILTERTYPE_SSINK);
    ~SubtitleSinkFilter() override;

    void Init(const std::shared_ptr<EventReceiver>& receiver, const std::shared_ptr<FilterCallback>& callback) override;

    Status DoInitAfterLink() override;

    Status DoPrepare() override;

    Status DoStart() override;

    Status DoPause() override;

    Status DoResume() override;

    Status DoFlush() override;

    Status DoStop() override;

    Status DoRelease() override;

    Status DoProcessInputBuffer(int recvArg, bool dropFrame) override;

    void SetParameter(const std::shared_ptr<Meta>& meta) override;

    void GetParameter(std::shared_ptr<Meta>& meta) override;

    Status OnLinked(StreamType inType, const std::shared_ptr<Meta>& meta,
        const std::shared_ptr<FilterLinkCallback>& callback) override;

    void SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter);

    Status SetIsTransitent(bool isTransitent);

    void NotifySeek();

    Status SetSpeed(float speed);
protected:
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta>& meta,
        const std::shared_ptr<FilterLinkCallback>& callback) override;

    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

    class AVBufferAvailableListener : public IConsumerListener {
    public:
	    AVBufferAvailableListener(std::shared_ptr<SubtitleSinkFilter> subtitleSinkFilter);

        void OnBufferAvailable() override;
    private:
	    std::weak_ptr<SubtitleSinkFilter> subtitleSinkFilter_;
    };

private:
    std::shared_ptr<SubtitleSink> subtitleSink_;
    std::string name_;
    FilterType filterType_ = FilterType::FILTERTYPE_SSINK;
    FilterState state_ = FilterState::CREATED;
    std::shared_ptr<Meta> trackMeta_;
    std::shared_ptr<Meta> globalMeta_;

    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;

    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    int64_t frameCnt_ {0};

    float volume_ {-1.0f}; // default invalid value
    std::atomic<bool> isThreadExit_ = true;
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_SUBTITLE_SINK_FILTER_H
