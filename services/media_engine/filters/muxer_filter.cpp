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

#include "muxer_filter.h"
#include "common/log.h"
#include "filter/filter_factory.h"
#include <sys/timeb.h>

namespace OHOS {
namespace Media {
namespace Pipeline {
constexpr int64_t WAIT_TIME_OUT_NS = 3000000000;
constexpr int32_t NS_TO_US = 1000;
static AutoRegisterFilter<MuxerFilter> g_registerMuxerFilter("builtin.recorder.muxer", FilterType::FILTERTYPE_MUXER,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<MuxerFilter>(name, FilterType::FILTERTYPE_MUXER);
    });

class MuxerBrokerListener : public IBrokerListener {
public:
    MuxerBrokerListener(std::shared_ptr<MuxerFilter> muxerFilter, int32_t trackIndex,
        sptr<AVBufferQueueProducer> inputBufferQueue)
        : muxerFilter_(std::move(muxerFilter)), trackIndex_(trackIndex), inputBufferQueue_(inputBufferQueue)
    {
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }

    void OnBufferFilled(std::shared_ptr<AVBuffer> &avBuffer) override
    {
        muxerFilter_->OnBufferFilled(avBuffer, trackIndex_, inputBufferQueue_);
    }

private:
    std::shared_ptr<MuxerFilter> muxerFilter_;
    int32_t trackIndex_;
    sptr<AVBufferQueueProducer> inputBufferQueue_;
};

MuxerFilter::MuxerFilter(std::string name, FilterType type): Filter(name, type)
{
}

MuxerFilter::~MuxerFilter()
{
}

Status MuxerFilter::SetOutputParameter(int32_t appUid, int32_t appPid, int32_t fd, int32_t format)
{
    mediaMuxer_ = std::make_shared<MediaMuxer>(appUid, appPid);
    return mediaMuxer_->Init(fd, (Plugin::OutputFormat)format);
}

void MuxerFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status MuxerFilter::Prepare()
{
    MEDIA_LOG_I("Prepare");
    return Status::OK;
}

Status MuxerFilter::Start()
{
    MEDIA_LOG_I("Start");
    startCount_++;
    if (startCount_ == preFilterCount_) {
        startCount_ = 0;
        return mediaMuxer_->Start();
    } else {
        return Status::OK;
    }
}

Status MuxerFilter::Pause()
{
    return Status::OK;
}

Status MuxerFilter::Resume()
{
    return Status::OK;
}

Status MuxerFilter::Stop()
{
    MEDIA_LOG_I("Stop");
    stopCount_++;
    if (stopCount_ == preFilterCount_) {
        stopCount_ = 0;
        return mediaMuxer_->Stop();
    } else {
        return Status::OK;
    }
}

Status MuxerFilter::Flush()
{
    return Status::OK;
}

Status MuxerFilter::Release()
{
    return Status::OK;
}

void MuxerFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    mediaMuxer_->SetParameter(parameter);
}

void MuxerFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
}

Status MuxerFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

Status MuxerFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status MuxerFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

FilterType MuxerFilter::GetFilterType()
{
    MEDIA_LOG_I("GetFilterType");
    return FilterType::FILTERTYPE_MUXER;
}

Status MuxerFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    int32_t trackIndex;
    mediaMuxer_->AddTrack(trackIndex, meta);
    sptr<AVBufferQueueProducer> inputBufferQueue = mediaMuxer_->GetInputBufferQueue(trackIndex);
    callback->OnLinkedResult(inputBufferQueue, const_cast<std::shared_ptr<Meta> &>(meta));
    sptr<IBrokerListener> listener = new MuxerBrokerListener(shared_from_this(), trackIndex, inputBufferQueue);
    inputBufferQueue->SetBufferFilledListener(listener);
    preFilterCount_++;
    bufferPtsMap_.insert(std::pair<int32_t, int64_t>(trackIndex, 0));
    return Status::OK;
}

Status MuxerFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}


Status MuxerFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void MuxerFilter::OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer, int32_t trackIndex,
    sptr<AVBufferQueueProducer> inputBufferQueue)
{
    MEDIA_LOG_I("OnBufferFilled");
    int64_t currentBufferPts = inputBuffer->pts_;
    int64_t anotherBufferPts = 0;
    for (auto mapInterator = bufferPtsMap_.begin(); mapInterator != bufferPtsMap_.end(); mapInterator++) {
        if (mapInterator->first != trackIndex) {
            anotherBufferPts = mapInterator->second;
        }
    }
    bufferPtsMap_[trackIndex] = currentBufferPts;
    if (std::abs(currentBufferPts - anotherBufferPts) >= WAIT_TIME_OUT_NS) {
        Stop();
        eventReceiver_->OnEvent({"muxer_filter", EventType::EVENT_ERROR, Status::ERROR_UNKNOWN});
    }
    inputBuffer->pts_ = inputBuffer->pts_ / NS_TO_US;
    inputBufferQueue->ReturnBuffer(inputBuffer, true);
}

} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS