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
#include <sys/timeb.h>
#include "common/log.h"
#include "filter/filter_factory.h"
#include "muxer/media_muxer.h"

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
        if (inputBufferQueue_ != nullptr) {
            if (auto muxerFilter = muxerFilter_.lock()) {
                muxerFilter->OnBufferFilled(avBuffer, trackIndex_, inputBufferQueue_.promote());
            } else {
                MEDIA_LOG_I("invalid muxerFilter");
            }
        }
    }

private:
    std::weak_ptr<MuxerFilter> muxerFilter_;
    int32_t trackIndex_;
    wptr<AVBufferQueueProducer> inputBufferQueue_;
};

MuxerFilter::MuxerFilter(std::string name, FilterType type): Filter(name, type)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "muxer filter create", logTag_.c_str());
}

MuxerFilter::~MuxerFilter()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "muxer filter destroy", logTag_.c_str());
}

Status MuxerFilter::SetOutputParameter(int32_t appUid, int32_t appPid, int32_t fd, int32_t format)
{
    mediaMuxer_ = std::make_shared<MediaMuxer>(appUid, appPid);
    return mediaMuxer_->Init(fd, (Plugins::OutputFormat)format);
}

void MuxerFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Init", logTag_.c_str());
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

void MuxerFilter::SetLogTag(std::string logTag)
{
    logTag_ = std::move(logTag);
}

Status MuxerFilter::Prepare()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Prepare", logTag_.c_str());
    return Status::OK;
}

Status MuxerFilter::Start()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Start", logTag_.c_str());
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
    MEDIA_LOG_I(PUBLIC_LOG_S "Stop", logTag_.c_str());
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
    MEDIA_LOG_I(PUBLIC_LOG_S "SetParameter", logTag_.c_str());
    mediaMuxer_->SetParameter(parameter);
}

void MuxerFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "GetParameter", logTag_.c_str());
}

Status MuxerFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

Status MuxerFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "UpdateNext", logTag_.c_str());
    return Status::OK;
}

Status MuxerFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "UnLinkNext", logTag_.c_str());
    return Status::OK;
}

FilterType MuxerFilter::GetFilterType()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "GetFilterType", logTag_.c_str());
    return FilterType::FILTERTYPE_MUXER;
}

Status MuxerFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnLinked", logTag_.c_str());
    int32_t trackIndex;
    auto ret = mediaMuxer_->AddTrack(trackIndex, meta);
    if (ret != Status::OK) {
        eventReceiver_->OnEvent({"muxer_filter", EventType::EVENT_ERROR, ret});
        return ret;
    }
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
    MEDIA_LOG_I(PUBLIC_LOG_S "OnUpdated", logTag_.c_str());
    return Status::OK;
}


Status MuxerFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnUnLinked", logTag_.c_str());
    return Status::OK;
}

void MuxerFilter::OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer, int32_t trackIndex,
    sptr<AVBufferQueueProducer> inputBufferQueue)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnBufferFilled", logTag_.c_str());
    int64_t currentBufferPts = inputBuffer->pts_;
    int64_t anotherBufferPts = 0;
    for (auto mapInterator = bufferPtsMap_.begin(); mapInterator != bufferPtsMap_.end(); mapInterator++) {
        if (mapInterator->first != trackIndex) {
            anotherBufferPts = mapInterator->second;
        }
    }
    bufferPtsMap_[trackIndex] = currentBufferPts;
    if (preFilterCount_ != 1 && std::abs(currentBufferPts - anotherBufferPts) >= WAIT_TIME_OUT_NS) {
        MEDIA_LOG_I(PUBLIC_LOG_S "OnBufferFilled pts time interval is greater than 3 seconds", logTag_.c_str());
    }
    inputBuffer->pts_ = inputBuffer->pts_ / NS_TO_US;
    MEDIA_LOG_I(PUBLIC_LOG_S "OnBufferFilled buffer->pts" PUBLIC_LOG_D64, logTag_.c_str(), inputBuffer->pts_);
    inputBufferQueue->ReturnBuffer(inputBuffer, true);
}

} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS