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

#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "filter/filter_factory.h"
#include "common/log.h"
#include "osal/task/autolock.h"
#include "demuxer_filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
namespace {
    const std::string MIME_IMAGE = "image";
}
static AutoRegisterFilter<DemuxerFilter> g_registerAudioCaptureFilter(
    "builtin.player.demuxer", FilterType::FILTERTYPE_DEMUXER,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<DemuxerFilter>(name, FilterType::FILTERTYPE_DEMUXER);
    }
);

class DemuxerFilterLinkCallback : public FilterLinkCallback {
public:
    explicit DemuxerFilterLinkCallback(std::shared_ptr<DemuxerFilter> demuxerFilter)
        : demuxerFilter_(demuxerFilter) {}

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        auto demuxerFilter = demuxerFilter_.lock();
        FALSE_RETURN(demuxerFilter != nullptr);
        demuxerFilter->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        auto demuxerFilter = demuxerFilter_.lock();
        FALSE_RETURN(demuxerFilter != nullptr);
        demuxerFilter->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        auto demuxerFilter = demuxerFilter_.lock();
        FALSE_RETURN(demuxerFilter != nullptr);
        demuxerFilter->OnUpdatedResult(meta);
    }
private:
    std::weak_ptr<DemuxerFilter> demuxerFilter_;
};

class DemuxerFilterDrmCallback : public OHOS::MediaAVCodec::AVDemuxerCallback {
public:
    explicit DemuxerFilterDrmCallback(std::shared_ptr<DemuxerFilter> demuxerFilter) : demuxerFilter_(demuxerFilter)
    {
    }

    ~DemuxerFilterDrmCallback()
    {
        MEDIA_LOG_I("~DemuxerFilterDrmCallback");
    }

    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo) override
    {
        MEDIA_LOG_I("DemuxerFilterDrmCallback OnDrmInfoChanged");
        std::shared_ptr<DemuxerFilter> callback = demuxerFilter_.lock();
        if (callback == nullptr) {
            MEDIA_LOG_E("OnDrmInfoChanged demuxerFilter callback is nullptr");
            return;
        }
        callback->OnDrmInfoUpdated(drmInfo);
    }

private:
    std::weak_ptr<DemuxerFilter> demuxerFilter_;
};

DemuxerFilter::DemuxerFilter(std::string name, FilterType type) : Filter(name, type)
{
    demuxer_ = std::make_shared<MediaDemuxer>();
    AutoLock lock(mapMutex_);
    track_id_map_.clear();
}

DemuxerFilter::~DemuxerFilter()
{
    MEDIA_LOG_I("~DemuxerFilter enter");
}

void DemuxerFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Init");
    MEDIA_LOG_I("DemuxerFilter Init");
    this->receiver_ = receiver;
    this->callback_ = callback;
    MEDIA_LOG_I("DemuxerFilter Init for drm callback");

    std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> drmCallback =
        std::make_shared<DemuxerFilterDrmCallback>(shared_from_this());
    demuxer_->SetDrmCallback(drmCallback);
    demuxer_->SetEventReceiver(receiver);
}

Status DemuxerFilter::SetDataSource(const std::shared_ptr<MediaSource> source)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::SetDataSource");
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
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Prepare");
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
        if (mime.substr(0, MIME_IMAGE.size()).compare(MIME_IMAGE) == 0) {
            MEDIA_LOG_W("is image track, continue");
            continue;
        }
        MediaType mediaType;
        if (!meta->GetData(Tag::MEDIA_TYPE, mediaType)) {
            MEDIA_LOG_E("mediaType not found, index: %zu", index);
            continue;
        }

        StreamType streamType;
        MEDIA_LOG_I("streamType is %{public}d", static_cast<int32_t>(mediaType));
        if (!FindStreamType(streamType, mediaType, mime)) {
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
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Start");
    MEDIA_LOG_I("Start called.");
    Filter::Start();
    return demuxer_->Start();
}

Status DemuxerFilter::Stop()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Stop");
    MEDIA_LOG_I("Stop called.");
    Filter::Stop();
    return demuxer_->Stop();
}

Status DemuxerFilter::Pause()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Pause");
    MEDIA_LOG_I("Pause called");
    // demuxer pause first for auido render immediatly
    demuxer_->Pause();
    return Filter::Pause();
}

Status DemuxerFilter::Resume()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Resume");
    MEDIA_LOG_I("Resume called");
    Filter::Resume();
    return demuxer_->Resume();
}

Status DemuxerFilter::Flush()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Flush");
    MEDIA_LOG_I("Flush entered");
    Filter::Flush();
    return demuxer_->Flush();
}

Status DemuxerFilter::Reset()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Reset");
    MEDIA_LOG_I("Reset called");
    {
        AutoLock lock(mapMutex_);
        track_id_map_.clear();
    }
    return demuxer_->Reset();
}

void DemuxerFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter enter");
}

void DemuxerFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter enter");
}

Status DemuxerFilter::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::SeekTo");
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

Status DemuxerFilter::GetBitRates(std::vector<uint32_t>& bitRates)
{
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

bool DemuxerFilter::FindStreamType(StreamType &streamType, MediaType mediaType, std::string mime)
{
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
        return false;
    }
    return true;
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

void DemuxerFilter::OnDrmInfoUpdated(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo)
{
    MEDIA_LOG_I("OnDrmInfoUpdated");
    if (this->receiver_ != nullptr) {
        this->receiver_->OnEvent({"demuxer_filter", EventType::EVENT_DRM_INFO_UPDATED, drmInfo});
    } else {
        MEDIA_LOG_E("OnDrmInfoUpdated failed receiver is nullptr");
    }
}

bool DemuxerFilter::GetDuration(int64_t& durationMs)
{
    return demuxer_->GetDuration(durationMs);
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS