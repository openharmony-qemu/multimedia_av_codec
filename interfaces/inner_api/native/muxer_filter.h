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

#ifndef FILTERS_MUXER_FILTER_H
#define FILTERS_MUXER_FILTER_H

#include <cstring>
#include <map>
#include <atomic>

#include "filter/filter.h"
#include "common/status.h"
#include "meta/media_types.h"

namespace OHOS {
namespace Media {
class MediaMuxer;
namespace Pipeline {
using Plugins::OutputFormat;
class MuxerFilter : public Filter, public std::enable_shared_from_this<MuxerFilter> {
public:
    explicit MuxerFilter(std::string name, FilterType type);
    ~MuxerFilter() override;
    Status SetOutputParameter(int32_t appUid, int32_t appPid, int32_t fd, int32_t format);
    Status SetTransCoderMode();
    void SetClipEndTimeUs(int64_t endTimeUs);
    void SetClipStartTimeUs(int64_t startTimeUs);
    int64_t GetCurrentPtsMs();
    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) override;
    Status DoPrepare() override;
    Status DoStart() override;
    Status DoPause() override;
    Status DoResume() override;
    Status DoStop() override;
    Status DoFlush() override;
    Status DoRelease() override;
    void SetParameter(const std::shared_ptr<Meta> &parameter) override;
    Status SetUserMeta(const std::shared_ptr<Meta> &userMeta);
    void GetParameter(std::shared_ptr<Meta> &parameter) override;
    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    FilterType GetFilterType();
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;
    void OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer, int32_t trackIndex,
        StreamType streamType, sptr<AVBufferQueueProducer> inputBufferQueue);
    void OnTransCoderBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer, int32_t trackIndex,
        StreamType streamType, sptr<AVBufferQueueProducer> inputBufferQueue);
    void SetFaultEvent(const std::string &errMsg);
    void SetFaultEvent(const std::string &errMsg, int32_t ret);
    const std::string &GetContainerFormat(Plugins::OutputFormat format);
    void SetCallingInfo(int32_t appUid, int32_t appPid, const std::string &bundleName, uint64_t instanceId);
    void SetMaxDuration(int32_t maxDuration);
    void EventCompleteStopAsync();

private:
    void HandleTransCoderComplete();
    void ConfigureTrackMeta(const std::shared_ptr<Meta> &meta, const std::string &mimeType);
    Status AddPendingMetaTracks();
    // Returns true if the buffer was consumed (discarded) and caller should return early.
    bool FilterPreClipBuffers(std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType,
        sptr<AVBufferQueueProducer> inputBufferQueue);
    // Returns true if the buffer was consumed (track already EOS) and caller should return early.
    bool HandleClipEnd(std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType,
        sptr<AVBufferQueueProducer> inputBufferQueue);
    void AdjustPtsForClip(std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType);
    void HandleTransCoderEos(std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType, bool &isCompleted);
    void LogFirstAudioAfterClip(const std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType);
    bool IsTrackEos(StreamType streamType);
    void UpdateTrackEosState(const std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType);
    void WriteTransCoderBuffer(std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType,
        sptr<AVBufferQueueProducer> inputBufferQueue);

    std::string name_;

    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;

    std::shared_ptr<MediaMuxer> mediaMuxer_;

    int32_t preFilterCount_{0};
    std::atomic<int32_t> stopCount_{0};
    std::atomic<int32_t> eosCount_{0};
    std::map<int32_t, int64_t> bufferPtsMap_;
    std::map<std::string, int32_t> trackIndexMap_;
    std::string videoCodecMimeType_;
    std::string audioCodecMimeType_;
    std::string metaDataCodecMimeType_;
    std::string bundleName_;
    uint64_t instanceId_{0};
    int32_t appUid_ {0};
    int32_t appPid_ {0};
    Plugins::OutputFormat outputFormat_{Plugins::OutputFormat::DEFAULT};

    std::atomic<int64_t> lastVideoPts_{0};
    std::atomic<int64_t> lastAudioPts_{0};
    std::atomic<bool> videoIsEos{false};
    std::atomic<bool> audioIsEos{false};
    std::atomic<bool> isTransCoderMode{false};

    int64_t clipEndTimeUs_ = -1;
    int64_t clipStartTimeUs_ = -1;
    std::atomic<int32_t> audioClipPassedCount_{0};
    std::atomic<int32_t> audioClipFilteredCount_{0};
 
    std::mutex stopMutex_;
    std::condition_variable stopCondition_;
    std::mutex eosMutex_;
    std::mutex bufferPtsMutex_;

    bool isStarted{false};

    int32_t maxDuration_ {INT32_MAX};
    std::atomic<bool> isReachMaxDuration_{false};

    bool isAdts_{true};
    bool mediaProfile_{0}; // default value: AAC_LC

    struct PendingMetaTrack {
        std::shared_ptr<Meta> meta;
        std::shared_ptr<FilterLinkCallback> callback;
        StreamType streamType;
    };
    std::vector<PendingMetaTrack> pendingMetaTracks_;
};
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
#endif // FILTERS_MUXER_FILTER_H
