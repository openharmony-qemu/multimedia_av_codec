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

#ifndef MEDIA_PIPELINE_DEMUXER_FILTER_H
#define MEDIA_PIPELINE_DEMUXER_FILTER_H

#include <string>
#include "filter/filter.h"
#include "media_demuxer.h"
#include "meta/meta.h"
#include "meta/media_types.h"
#include "osal/task/mutex.h"
namespace OHOS {
namespace Media {
namespace Pipeline {
class DemuxerFilter : public Filter, public std::enable_shared_from_this<DemuxerFilter> {
public:
    explicit DemuxerFilter(std::string name, FilterType type);
    ~DemuxerFilter() override;

    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) override;
    Status Prepare() override;
    Status Start() override;
    Status Stop() override;
    Status Pause() override;
    Status Resume() override;
    Status Flush() override;
    Status Reset();

    void SetParameter(const std::shared_ptr<Meta> &parameter) override;
    void GetParameter(std::shared_ptr<Meta> &parameter) override;

    Status SetDataSource(const std::shared_ptr<MediaSource> source);
    Status SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime);

    std::vector<std::shared_ptr<Meta>> GetStreamMetaInfo() const;
    std::shared_ptr<Meta> GetGlobalMetaInfo() const;

    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status GetBitRates(std::vector<uint32_t>& bitRates);
    Status SelectBitRate(uint32_t bitRate);

    FilterType GetFilterType();

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);
    void OnUpdatedResult(std::shared_ptr<Meta> &meta);
    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);

    // drm callback
    void OnDrmInfoUpdated(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo);
    bool GetDuration(int64_t& durationMs);
protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    struct MediaMetaData {
        std::vector<std::shared_ptr<Meta>> trackMetas;
        std::shared_ptr<Meta> globalMeta;
    };

    bool FindTrackId(StreamType outType, int32_t &trackId);
    bool FindStreamType(StreamType &streamType, Plugins::MediaType mediaType, std::string mime);
    std::string uri_;

    std::shared_ptr<Filter> nextFilter_;
    MediaMetaData mediaMetaData_;
    std::shared_ptr<MediaDemuxer> demuxer_;
    std::shared_ptr<MediaSource> mediaSource_;
    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;

    std::map<StreamType, std::vector<int32_t>> track_id_map_;
    Mutex mapMutex_ {};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // MEDIA_PIPELINE_DEMUXER_FILTER_H