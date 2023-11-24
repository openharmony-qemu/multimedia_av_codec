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

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<MuxerFilter> g_registerMuxerFilter("builtin.recorder.muxer", FilterType::FILTERTYPE_MUXER, 
    [](const std::string& name, const FilterType type) {return std::make_shared<MuxerFilter>(name, FilterType::FILTERTYPE_MUXER); });
MuxerFilter::MuxerFilter(std::string name, FilterType type): Filter(name, type) { }

MuxerFilter::~MuxerFilter()
{
}

Status MuxerFilter::SetOutputParameter(int32_t appUid, int32_t appPid, int32_t fd, int32_t format) {
    mediaMuxer_ = std::make_shared<MediaMuxer>(appUid, appPid);
    return mediaMuxer_->Init(fd, (Plugin::OutputFormat)format);
}

void MuxerFilter::Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) {
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status MuxerFilter::Prepare() {
    return Status::OK;
}

Status MuxerFilter::Start() {
    return mediaMuxer_->Start();
}

Status MuxerFilter::Pause() {
    return Status::OK;
}

Status MuxerFilter::Resume() {
    return Status::OK;
}

Status MuxerFilter::Stop() {
    return mediaMuxer_->Stop();
}

Status MuxerFilter::Flush() {
    return Status::OK;
}

Status MuxerFilter::Release() {
    return Status::OK;
}

void MuxerFilter::SetParameter(const std::shared_ptr<Meta> &parameter) {
    MEDIA_LOG_I("SetParameter enter.");
    mediaMuxer_->SetParameter(parameter);
}

void MuxerFilter::GetParameter(std::shared_ptr<Meta> &parameter) {
}

Status MuxerFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

Status MuxerFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

Status MuxerFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

FilterType MuxerFilter::GetFilterType() {
    return FilterType::FILTERTYPE_MUXER;
}

Status MuxerFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback) {
    MEDIA_LOG_I("OnLinked enter.");
    int32_t trackIndex = 0;
    mediaMuxer_->AddTrack(trackIndex, meta);
    sptr<AVBufferQueueProducer> inputBufferQueue = mediaMuxer_->GetInputBufferQueue(trackIndex);
    callback->OnLinkedResult(inputBufferQueue, const_cast<std::shared_ptr<Meta> &>(meta));
    return Status::OK;
}

Status MuxerFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
                              const std::shared_ptr<FilterLinkCallback> &callback) {
    return Status::OK;
}


Status MuxerFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback> &callback) {
    return Status::OK;
}

} //namespace Pipeline
} //namespace MEDIA
} //namespace OHOS