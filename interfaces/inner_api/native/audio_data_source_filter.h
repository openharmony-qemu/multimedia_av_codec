/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#ifndef FILTERS_AUDIO_DATA_SOURCE_FILTER_H
#define FILTERS_AUDIO_DATA_SOURCE_FILTER_H

#include "audio_capturer.h"
#include "common/status.h"
#include "filter/filter.h"
#include "media_data_source.h"
#include "osal/task/task.h"
#include <condition_variable>
#include <mutex>

namespace OHOS {
namespace Media {
namespace Pipeline {

class AudioDataSourceFilter : public Filter,
                              public IAudioDataSourceListener,
                              public std::enable_shared_from_this<AudioDataSourceFilter> {
public:
    explicit AudioDataSourceFilter(std::string name, FilterType type);
    ~AudioDataSourceFilter() override;
    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) override;
    Status DoPrepare() override;
    Status DoStart() override;
    Status DoPause() override;
    Status DoResume() override;
    Status DoStop() override;
    Status DoFlush() override;
    Status DoRelease() override;
    void SetParameter(const std::shared_ptr<Meta> &meta) override;
    void GetParameter(std::shared_ptr<Meta> &meta) override;
    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status SendEos();
    FilterType GetFilterType();
    void SetAudioDataSource(const std::shared_ptr<IAudioDataSource> &audioSource);
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta);
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback> &callback) override;
    void OnUnlinkedResult(const std::shared_ptr<Meta> &meta);
    void OnUpdatedResult(const std::shared_ptr<Meta> &meta);
    void SetVideoFirstFramePts(int64_t firstFramePts);
    void OnAudioDataReady() override;

private:
    void ProcessLoop();
    AudioDataSourceReadAtActionState DoReadAndPush();
    std::shared_ptr<Task> taskPtr_{nullptr};
    sptr<AVBufferQueueProducer> outputBufferQueue_;
    std::shared_ptr<IAudioDataSource> audioDataSource_{nullptr};

    std::shared_ptr<EventReceiver> receiver_;
    std::shared_ptr<FilterCallback> callback_;

    std::shared_ptr<Filter> nextFilter_;
    std::atomic<bool> eos_{false};

    std::mutex mutex_;
    std::condition_variable dataCond_;
    bool dataReady_ = false;
    std::atomic<bool> stopped_{false};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // FILTERS_AUDIO_CAPTURE_FILTER_H
