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
 
#include "audio_data_source_filter.h"
#include "common/log.h"
#include "filter/filter_factory.h"
 
namespace OHOS {
namespace Media {
namespace Pipeline {
constexpr uint32_t TIME_OUT_MS = 0;
static AutoRegisterFilter<AudioDataSourceFilter> g_registerAudioDataSourceFilter("builtin.recorder.audiodatasource",
    FilterType::AUDIO_DATA_SOURCE,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<AudioDataSourceFilter>(name, FilterType::AUDIO_DATA_SOURCE);
    });
 
/// End of Stream Buffer Flag
constexpr uint32_t BUFFER_FLAG_EOS = 0x00000001;
class AudioDataSourceFilterLinkCallback : public FilterLinkCallback {
public:
    explicit AudioDataSourceFilterLinkCallback(std::shared_ptr<AudioDataSourceFilter> audioDataSourceFilter)
        : audioDataSourceFilter_(std::move(audioDataSourceFilter))
    {
    }
 
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto dataSourceFilter = audioDataSourceFilter_.lock()) {
            dataSourceFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid dataSourceFilter");
        }
    }
 
    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto dataSourceFilter = audioDataSourceFilter_.lock()) {
            dataSourceFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid dataSourceFilter");
        }
    }
 
    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto dataSourceFilter = audioDataSourceFilter_.lock()) {
            dataSourceFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid dataSourceFilter");
        }
    }
 
private:
    std::weak_ptr<AudioDataSourceFilter> audioDataSourceFilter_;
};
 
AudioDataSourceFilter::AudioDataSourceFilter(std::string name, FilterType type): Filter(name, type)
{
    MEDIA_LOG_I("audio data source filter create");
}
 
AudioDataSourceFilter::~AudioDataSourceFilter()
{
    MEDIA_LOG_I("audio data source filter destroy");
}
 
void AudioDataSourceFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("AudioDataSourceFilter Init");
    receiver_ = receiver;
    callback_ = callback;
    if (!taskPtr_) {
        taskPtr_ = std::make_shared<Task>("DataReader");
        taskPtr_->RegisterJob([this] { ReadLoop(); return 0;});
    }
}
 
Status AudioDataSourceFilter::DoPrepare()
{
    MEDIA_LOG_I("AudioDataSourceFilter DoPrepare");
    if (callback_ == nullptr) {
        MEDIA_LOG_E("callback is nullptr");
        return Status::ERROR_NULL_POINTER;
    }
    callback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
        StreamType::STREAMTYPE_RAW_AUDIO);
    return Status::OK;
}
 
Status AudioDataSourceFilter::DoStart()
{
    MEDIA_LOG_I("AudioDataSourceFilter DoStart");
    eos_ = false;
    if (taskPtr_) {
        taskPtr_->Start();
    }
    return Status::OK;
}
 
Status AudioDataSourceFilter::DoPause()
{
    MEDIA_LOG_I("AudioDataSourceFilter DoPause");
    if (taskPtr_) {
        taskPtr_->Pause();
    }
    return Status::OK;
}
 
Status AudioDataSourceFilter::DoResume()
{
    MEDIA_LOG_I("AudioDataSourceFilter DoResume");
    if (taskPtr_) {
        taskPtr_->Start();
    }
    return Status::OK;
}
 
Status AudioDataSourceFilter::DoStop()
{
    MEDIA_LOG_I("AudioDataSourceFilter DoStop");
    // stop task firstly
    if (taskPtr_) {
        taskPtr_->Stop();
    }
    return Status::OK;
}
 
Status AudioDataSourceFilter::DoFlush()
{
    MEDIA_LOG_I("AudioDataSourceFilter DoFlush");
    return Status::OK;
}
 
Status AudioDataSourceFilter::DoRelease()
{
    MEDIA_LOG_I("AudioDataSourceFilter DoRelease");
    if (taskPtr_) {
        taskPtr_->Stop();
    }
    taskPtr_ = nullptr;
    audioDataSource_ = nullptr;
    return Status::OK;
}
 
void AudioDataSourceFilter::SetParameter(const std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("AudioDataSourceFilter SetParameter");
}
 
void AudioDataSourceFilter::GetParameter(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("AudioDataSourceFilter GetParameter");
}
 
Status AudioDataSourceFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("AudioDataSourceFilter LinkNext");
    auto meta = std::make_shared<Meta>();
    GetParameter(meta);
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<AudioDataSourceFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, meta, filterLinkCallback);
    return Status::OK;
}
 
FilterType AudioDataSourceFilter::GetFilterType()
{
    MEDIA_LOG_I("AudioDataSourceFilter GetFilterType");
    return FilterType::AUDIO_CAPTURE;
}
 
void AudioDataSourceFilter::SetAudioDataSource(const std::shared_ptr<IAudioDataSource>& audioSource)
{
    audioDataSource_ = audioSource;
}

Status AudioDataSourceFilter::SendEos()
{
    MEDIA_LOG_I("AudioDataSourceFilter SendEos");
    Status ret = Status::OK;
    if (outputBufferQueue_) {
        std::shared_ptr<AVBuffer> buffer;
        AVBufferConfig avBufferConfig;
        ret = outputBufferQueue_->RequestBuffer(buffer, avBufferConfig, TIME_OUT_MS);
        if (ret != Status::OK) {
            MEDIA_LOG_I("RequestBuffer fail, ret" PUBLIC_LOG_D32, ret);
            return ret;
        }
        buffer->flag_ |= BUFFER_FLAG_EOS;
        outputBufferQueue_->PushBuffer(buffer, false);
    }
    eos_ = true;
    return ret;
}
 
void AudioDataSourceFilter::ReadLoop()
{
    MEDIA_LOG_D("AudioDataSourceFilter ReadLoop");
    if (eos_.load()) {
        return;
    }
    int64_t bufferSize = 0;
    if (audioDataSource_->GetSize(bufferSize) != 0) {
        MEDIA_LOG_E("Get audioCaptureModule buffer size fail");
        return;
    }
    MEDIA_LOG_I("AudioDataSourceFilter GetSize : %{public}zu", bufferSize);
    std::shared_ptr<AVBuffer> buffer;
    AVBufferConfig avBufferConfig;
    avBufferConfig.size = bufferSize;
    avBufferConfig.memoryFlag = MemoryFlag::MEMORY_READ_WRITE;
    if (outputBufferQueue_ == nullptr) {
        MEDIA_LOG_I("AudioDataSourceFilter outputBufferQueue_ is nullptr");
    }
    Status status = outputBufferQueue_->RequestBuffer(buffer, avBufferConfig, TIME_OUT_MS);
    if (status != Status::OK) {
        MEDIA_LOG_E("RequestBuffer fail");
        return;
    }
    if (audioDataSource_->ReadAt(buffer, bufferSize) != 0) {
        MEDIA_LOG_E("RequestBuffer fail");
        outputBufferQueue_->PushBuffer(buffer, false);
        return;
    }
    buffer->memory_->SetSize(bufferSize);
    status = outputBufferQueue_->PushBuffer(buffer, true);
    if (status != Status::OK) {
        MEDIA_LOG_E("PushBuffer fail");
    }
}
 
void AudioDataSourceFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("AudioDataSourceFilter OnLinkedResult");
    outputBufferQueue_ = queue;
}
 
Status AudioDataSourceFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("AudioDataSourceFilter UpdateNext");
    return Status::OK;
}
 
Status AudioDataSourceFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("AudioDataSourceFilter UnLinkNext");
    return Status::OK;
}
 
Status AudioDataSourceFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("AudioDataSourceFilter OnLinked");
    return Status::OK;
}
 
Status AudioDataSourceFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("AudioDataSourceFilter OnUpdated");
    return Status::OK;
}
 
Status AudioDataSourceFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("AudioDataSourceFilter OnUnLinked");
    return Status::OK;
}
 
void AudioDataSourceFilter::OnUnlinkedResult(const std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("AudioDataSourceFilter OnUnlinkedResult");
    (void) meta;
}
 
void AudioDataSourceFilter::OnUpdatedResult(const std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("AudioDataSourceFilter OnUpdatedResult");
    (void) meta;
}
 
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
