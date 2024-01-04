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

#define HST_LOG_TAG "DemuxerFilter"

#include "filter/filter_factory.h"
#include "common/log.h"
#include "meta/media_types.h"
#include "osal/task/autolock.h"
#include "demuxer_filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<DemuxerFilter> g_registerAudioCaptureFilter(
    "builtin.player.demuxer", FilterType::FILTERTYPE_DEMUXER,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<DemuxerFilter>(name, FilterType::FILTERTYPE_DEMUXER);
    }
);

class DemuxerFilterLinkCallback : public FilterLinkCallback {
public:
    explicit DemuxerFilterLinkCallback(std::shared_ptr<DemuxerFilter> demuxerFilter)
    {
        demuxerFilter_ = demuxerFilter;
    }

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        demuxerFilter_->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        demuxerFilter_->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        demuxerFilter_->OnUpdatedResult(meta);
    }
private:
    std::shared_ptr<DemuxerFilter> demuxerFilter_;
};

DemuxerFilter::DemuxerFilter(std::string name, FilterType type) : Filter(name, type)
{
    demuxer_ = std::make_shared<MediaDemuxer>();
    AutoLock lock(mapMutex_);
    track_id_map_.clear();
}

DemuxerFilter::~DemuxerFilter()
{
    MEDIA_LOG_I("~DemuxerFilter called");
}

void DemuxerFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    this->receiver_ = receiver;
    this->callback_ = callback;
}

Status DemuxerFilter::SetDataSource(const std::shared_ptr<MediaSource> source)
{
    MEDIA_LOG_I("SetDataSource entered.");
    if (source == nullptr) {
        MEDIA_LOG_E("Invalid source");
        return Status::ERROR_INVALID_PARAMETER;
    }
    mediaSource_ = source;
    return demuxer_->SetDataSource(mediaSource_);
}

Status DemuxerFilter::Prepare()
{
    if (mediaSource_ == nullptr) {
        MEDIA_LOG_E("No valid media source, please call SetDataSource firstly.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::vector<std::shared_ptr<Meta>> trackInfos = demuxer_->GetStreamMetaInfo();
    size_t trackCount = trackInfos.size();
    FALSE_RETURN_V_MSG_E(trackInfos.size() != 0, Status::ERROR_INVALID_PARAMETER,
        "trackCount is invalid.");
    
    MEDIA_LOG_I("trackCount: %{public}d", trackCount);
    for (size_t index = 0; index < trackCount; index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        if (meta == nullptr) {
            MEDIA_LOG_E("meta is invalid, index: %zu", index);
            return Status::ERROR_INVALID_PARAMETER;
        }
        std::string mime;
        meta->GetData(Tag::MIME_TYPE, mime);
        if (mime.substr(0, 5).compare("image") == 0) {
            MEDIA_LOG_W("is image track, continue");
            continue;
        }
        MediaType mediaType;
        if (!meta->GetData(Tag::MEDIA_TYPE, mediaType)) {
            MEDIA_LOG_E("mediaType not found, index: %zu", index);
            return Status::ERROR_INVALID_PARAMETER;
        }

        StreamType streamType;
        MEDIA_LOG_I("streamType is %{public}d", static_cast<int32_t>(mediaType));
        if (mediaType == MediaType::AUDIO) {
            if (mime == std::string(MimeType::AUDIO_RAW)) {
                streamType = StreamType::STREAMTYPE_RAW_AUDIO;
            } else {
                streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
            }
        } else if (mediaType == MediaType::VIDEO) {
            streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
        } else {
            MEDIA_LOG_E("streamType not found, index: %zu", index);
            return Status::ERROR_INVALID_PARAMETER;
        }

        {
            AutoLock lock(mapMutex_);
            auto it = track_id_map_.find(streamType);
            if (it != track_id_map_.end()) {
                it->second.push_back(index);
            } else {
                std::vector<int32_t> vec = {index};
                track_id_map_.insert({streamType, vec});
            }
        }
        if (callback_ == nullptr) {
            MEDIA_LOG_W("callback is nullptr");
            continue;
        }
        callback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED, streamType);
    }
    return Filter::Prepare();
}

Status DemuxerFilter::Start()
{
    MEDIA_LOG_I("Start called.");
    Filter::Start();
    return demuxer_->Start();
}

Status DemuxerFilter::Stop()
{
    MEDIA_LOG_I("Stop called.");
    Filter::Stop();
    return demuxer_->Stop();
}

Status DemuxerFilter::Pause()
{
    MEDIA_LOG_I("Pause called");
    return demuxer_->Stop();
}

Status DemuxerFilter::Resume()
{
    MEDIA_LOG_I("Resume called");
    return demuxer_->Start();
}

Status DemuxerFilter::Flush()
{
    MEDIA_LOG_I("Flush entered");
    return Filter::Flush();
}

Status DemuxerFilter::Reset()
{
    MEDIA_LOG_I("Reset called");
    {
        AutoLock lock(mapMutex_);
        track_id_map_.clear();
    }
    return demuxer_->Reset();
}

void DemuxerFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter entered");
}

void DemuxerFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter entered");
}

Status DemuxerFilter::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MEDIA_LOG_I("SeekTo called");
    return demuxer_->SeekTo(seekTime, mode, realSeekTime);
}

std::vector<std::shared_ptr<Meta>> DemuxerFilter::GetStreamMetaInfo() const
{
    return demuxer_->GetStreamMetaInfo();
}

std::shared_ptr<Meta> DemuxerFilter::GetGlobalMetaInfo() const
{
    return demuxer_->GetGlobalMetaInfo();
}

Status DemuxerFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    int32_t trackId = -1;
    if (!FindTrackId(outType, trackId)) {
        MEDIA_LOG_E("FindTrackId failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::vector<std::shared_ptr<Meta>> trackInfos = demuxer_->GetStreamMetaInfo();
    std::shared_ptr<Meta> meta = trackInfos[trackId];
    for (MapIt iter = meta->begin(); iter != meta->end(); iter++) {
        MEDIA_LOG_I("LinkNext iter->first " PUBLIC_LOG_S, iter->first.c_str());
    }
    std::string mimeType;
    meta->GetData(Tag::MIME_TYPE, mimeType);
    MEDIA_LOG_I("LinkNext mimeType " PUBLIC_LOG_S, mimeType.c_str());

    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    MEDIA_LOG_I("LinkNext NextFilter FilterType " PUBLIC_LOG_D32, nextFilter_->GetFilterType());
    meta->SetData(Tag::REGULAR_TRACK_ID, trackId);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback
        = std::make_shared<DemuxerFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, meta, filterLinkCallback);
    return Status::OK;
}

Status DemuxerFilter::GetBitRates(std::vector<uint32_t>& bitRates) {
    if (mediaSource_ == nullptr) {
        MEDIA_LOG_E("GetBitRates failed, mediaSource = nullptr");
    }
    return demuxer_->GetBitRates(bitRates);
}

Status DemuxerFilter::SelectBitRate(uint32_t bitRate) 
{
    if (mediaSource_ == nullptr) {
        MEDIA_LOG_E("SelectBitRate failed, mediaSource = nullptr");
    }
    return demuxer_->SelectBitRate(bitRate);
}

bool DemuxerFilter::FindTrackId(StreamType outType, int32_t &trackId)
{
    AutoLock lock(mapMutex_);
    auto it = track_id_map_.find(outType);
    if (it != track_id_map_.end()) {
        trackId = it->second.front();
        it->second.erase(it->second.begin());
        if (it->second.empty()) {
            track_id_map_.erase(it);
        }
        return true;
    }
    return false;
}

Status DemuxerFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

Status DemuxerFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

FilterType DemuxerFilter::GetFilterType()
{
    return filterType_;
}

Status DemuxerFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    onLinkedResultCallback_ = callback;
    return Status::OK;
}

Status DemuxerFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    return Status::OK;
}

Status DemuxerFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    return Status::OK;
}

void DemuxerFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta)
{
    if (meta == nullptr) {
        MEDIA_LOG_E("meta is invalid.");
        return;
    }
    int32_t trackId;
    if (!meta->GetData(Tag::REGULAR_TRACK_ID, trackId)) {
        MEDIA_LOG_E("trackId not found");
        return;
    }
    demuxer_->SetOutputBufferQueue(trackId, outputBufferQueue);
}

void DemuxerFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
}

void DemuxerFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS