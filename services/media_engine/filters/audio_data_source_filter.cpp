/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SCREENCAPTURE, "AudioDataSourceFilter" };
static constexpr uint8_t LOG_LIMIT_HUNDRED = 100;
}

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
            MEDIA_LOG_I("OnLinkedResult invalid dataSourceFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto dataSourceFilter = audioDataSourceFilter_.lock()) {
            dataSourceFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("OnUnlinkedResult invalid dataSourceFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto dataSourceFilter = audioDataSourceFilter_.lock()) {
            dataSourceFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("OnUpdatedResult invalid dataSourceFilter");
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
        taskPtr_->RegisterJob([this] {
            ProcessLoop();
            return 0;
        });
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
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_.store(false);
        dataReady_ = false;
    }
    if (taskPtr_) {
        taskPtr_->Start();
    }
    return Status::OK;
}

Status AudioDataSourceFilter::DoPause()
{
    MEDIA_LOG_I("AudioDataSourceFilter DoPause");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_.store(true);
        dataCond_.notify_all();
    }
    if (taskPtr_) {
        taskPtr_->Pause();
    }
    return Status::OK;
}

Status AudioDataSourceFilter::DoResume()
{
    MEDIA_LOG_I("AudioDataSourceFilter DoResume");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_.store(false);
        dataReady_ = false;
    }
    if (taskPtr_) {
        taskPtr_->Start();
    }
    return Status::OK;
}

Status AudioDataSourceFilter::DoStop()
{
    MEDIA_LOG_I("AudioDataSourceFilter DoStop");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_.store(true);
        dataReady_ = false;
        dataCond_.notify_all();
    }
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
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_.store(true);
        dataCond_.notify_all();
    }
    if (taskPtr_) {
        taskPtr_->Stop();
    }
    taskPtr_ = nullptr;
    if (audioDataSource_) {
        audioDataSource_->SetListener(nullptr);
    }
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

void AudioDataSourceFilter::SetAudioDataSource(const std::shared_ptr<IAudioDataSource> &audioSource)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (audioDataSource_) {
        audioDataSource_->SetListener(nullptr);
    }
    audioDataSource_ = audioSource;
    if (audioDataSource_) {
        audioDataSource_->SetListener(shared_from_this());
    }
}

void AudioDataSourceFilter::SetVideoFirstFramePts(int64_t firstFramePts)
{
    if (audioDataSource_) {
        MEDIA_LOG_I("set firstVideoFramePts: " PUBLIC_LOG_D64, firstFramePts);
        audioDataSource_->SetVideoFirstFramePts(firstFramePts);
    }
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
            MEDIA_LOG_E("RequestBuffer fail, ret" PUBLIC_LOG_D32, ret);
            return ret;
        }
        if (buffer == nullptr) {
            MEDIA_LOG_E("RequestBuffer returned OK but buffer is nullptr");
            return Status::ERROR_NULL_POINTER;
        }
        buffer->flag_ |= BUFFER_FLAG_EOS;
        outputBufferQueue_->PushBuffer(buffer, false);
    }
    eos_ = true;
    return ret;
}

void AudioDataSourceFilter::OnAudioDataReady()
{
    std::lock_guard<std::mutex> lock(mutex_);
    dataReady_ = true;
    dataCond_.notify_one();
}

void AudioDataSourceFilter::ProcessLoop()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        dataCond_.wait(lock, [this] { return dataReady_ || stopped_.load(); });
        if (stopped_.load()) {
            return;
        }
        dataReady_ = false;
    }
    while (!stopped_.load() && !eos_.load()) {
        if (DoReadAndPush() != AudioDataSourceReadAtActionState::OK) {
            break;
        }
    }
}

AudioDataSourceReadAtActionState AudioDataSourceFilter::DoReadAndPush()
{
    std::shared_ptr<IAudioDataSource> dataSource;
    sptr<AVBufferQueueProducer> bufferQueue;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        dataSource = audioDataSource_;
        bufferQueue = outputBufferQueue_;
    }
    if (dataSource == nullptr) {
        return AudioDataSourceReadAtActionState::INVALID;
    }
    int64_t bufferSize = 0;
    if (dataSource->GetSize(bufferSize) != 0) {
        return AudioDataSourceReadAtActionState::RETRY_SKIP;
    }
    if (bufferQueue == nullptr) {
        MEDIA_LOG_I("AudioDataSourceFilter outputBufferQueue_ is nullptr");
        return AudioDataSourceReadAtActionState::INVALID;
    }
    std::shared_ptr<AVBuffer> buffer;
    AVBufferConfig avBufferConfig;
    avBufferConfig.size = bufferSize;
    avBufferConfig.memoryFlag = MemoryFlag::MEMORY_READ_WRITE;
    Status status = bufferQueue->RequestBuffer(buffer, avBufferConfig, TIME_OUT_MS);
    if (status != Status::OK) {
        MEDIA_LOGE_LIMIT(LOG_LIMIT_HUNDRED, "AudioDataSourceFilter RequestBuffer fail");
        dataSource->ReadAt(nullptr, bufferSize);
        return AudioDataSourceReadAtActionState::RETRY_SKIP;
    }
    AudioDataSourceReadAtActionState readAtRet = dataSource->ReadAt(buffer, bufferSize);
    if (readAtRet != AudioDataSourceReadAtActionState::OK) {
        if (readAtRet != AudioDataSourceReadAtActionState::SKIP_WITHOUT_LOG) {
            MEDIA_LOGE_LIMIT(LOG_LIMIT_HUNDRED, "AudioDataSourceFilter ReadAt fail ret: %{public}d",
                static_cast<int32_t>(readAtRet));
        }
        bufferQueue->PushBuffer(buffer, false);
        return readAtRet;
    }
    if (buffer == nullptr || buffer->memory_ == nullptr) {
        MEDIA_LOG_E("buffer->memory_ is nullptr");
        bufferQueue->PushBuffer(buffer, false);
        return AudioDataSourceReadAtActionState::RETRY_SKIP;
    }
    buffer->memory_->SetSize(bufferSize);
    status = bufferQueue->PushBuffer(buffer, true);
    if (status != Status::OK) {
        MEDIA_LOG_E("AudioDataSourceFilter PushBuffer fail");
    }
    return AudioDataSourceReadAtActionState::OK;
}

void AudioDataSourceFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("AudioDataSourceFilter OnLinkedResult");
    std::lock_guard<std::mutex> lock(mutex_);
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
