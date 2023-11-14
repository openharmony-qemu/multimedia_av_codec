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

#include "native/demuxer_filter.h"
#include "common/log.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class DemuxerFilterLinkCallback : public FilterLinkCallback {
public:
    DemuxerFilterLinkCallback(std::shared_ptr<DemuxerFilter> demuxerFilter) {
        demuxerFilter_ = demuxerFilter;
    }

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override {
        demuxerFilter_->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override {
        demuxerFilter_->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override {
        demuxerFilter_->OnUpdatedResult(meta);
    }
private:
    std::shared_ptr<DemuxerFilter> demuxerFilter_;
};

DemuxerFilter::~DemuxerFilter()
{
    MEDIA_LOG_I("dtor called");
}

void DemuxerFilter::Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback)
{
    this->receiver_ = receiver;
    this->callback_ = callback;
}

Status DemuxerFilter::Prepare()
{
    MEDIA_LOG_I("Prepare called");
    if (mediaSource_ == nullptr) {
        MEDIA_LOG_E("No valid media source, please call SetDataSource firstly.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::vector<std::shared_ptr<Meta>> trackInfos = demuxer_->GetStreamMetaInfo();
    int32_t trackCount = trackInfos.size();
    if (trackCount <= 0) {
        MEDIA_LOG_E("trackCount is invalid.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    for (size_t index = 0; index < trackCount; index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        if (meta == nullptr) {
            MEDIA_LOG_E("meta is invalid, index: %zu", index);
            return Status::ERROR_INVALID_PARAMETER;
        }

        StreamType streamType;
        if (!meta->GetData(Tag::MEDIA_STREAM_TYPE, streamType)) {
            MEDIA_LOG_E("streamType not found, index: %zu", index);
            return Status::ERROR_INVALID_PARAMETER;
        }
        callback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                              streamType);
    }
    return Status::OK;
}

Status DemuxerFilter::Start()
{
    MEDIA_LOG_I("Start called.");
    return demuxer_->Start();
}

Status DemuxerFilter::Stop()
{
    MEDIA_LOG_I("Stop called.");
    demuxer_->Stop();
    Reset();
    return demuxer_->Stop();
}

Status DemuxerFilter::Pause()
{
    MEDIA_LOG_I("Pause called");
    return Status::OK;
}

Status DemuxerFilter::Flush()
{
    MEDIA_LOG_I("Flush entered");
    return Status::OK;
}

Status DemuxerFilter::Reset()
{
    MEDIA_LOG_I("Reset called");
    demuxer_->Reset();
}

void DemuxerFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter entered");
}

void DemuxerFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter entered");
}

Status DemuxerFilter::SetDataSource(const std::shared_ptr<MediaSource> source)
{
    MEDIA_LOG_I("SetDataSource entered.");
    if (source == nullptr) {
        MEDIA_LOG_E("Invalid source");
        return Status::ERROR_INVALID_PARAMETER;
    }
    mediaSource_ = source;
    demuxer_->SetDataSource(mediaSource_);
    return Status::OK;
}

Status DemuxerFilter::SeekTo(int64_t seekTime, Plugin::SeekMode mode, int64_t& realSeekTime)
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
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    switch (nextFilter->GetFilterType()) {
        case FilterType::FILTERTYPE_ADEC:
            break;
        case FilterType::FILTERTYPE_VDEC:
            break;
        default:
            break;
    }
    std::shared_ptr<FilterLinkCallback> filterLinkCallback = std::make_shared<DemuxerFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, meta, filterLinkCallback);
    nextFilter->Prepare();
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

Status DemuxerFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback)
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