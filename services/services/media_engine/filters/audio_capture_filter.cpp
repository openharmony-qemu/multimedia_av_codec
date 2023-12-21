/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd.
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

#include "common/log.h"
#include "audio_capture_filter.h"
#include "filter/filter_factory.h"

#define TIME_OUT_MS 100

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<AudioCaptureFilter> g_registerAudioCaptureFilter("builtin.recorder.audiocapture",
    FilterType::FILTERTYPE_ACAPTURE,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<AudioCaptureFilter>(name, FilterType::FILTERTYPE_ACAPTURE); });

/// End of Stream Buffer Flag
constexpr uint32_t BUFFER_FLAG_EOS = 0x00000001;
class AudioCaptureFilterLinkCallback : public FilterLinkCallback {
public:
    explicit AudioCaptureFilterLinkCallback(std::shared_ptr<AudioCaptureFilter> audioCaptureFilter)
        : audioCaptureFilter_(audioCaptureFilter)
    {
    }

    void OnLinkedResult(const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta) override {
        audioCaptureFilter_->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta>& meta) override {
        audioCaptureFilter_->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta>& meta) override {
        audioCaptureFilter_->OnUpdatedResult(meta);
    }

private:
    std::shared_ptr<AudioCaptureFilter> audioCaptureFilter_;
};

AudioCaptureFilter::AudioCaptureFilter(std::string name, FilterType type): Filter(name, type) {}

AudioCaptureFilter::~AudioCaptureFilter() {
    if (taskPtr_) {
        taskPtr_->Stop();
    }
    if (audioCaptureModule_) {
        audioCaptureModule_->Deinit();
    }
}

void AudioCaptureFilter::Init(const std::shared_ptr<EventReceiver>& receiver,
    const std::shared_ptr<FilterCallback>& callback) {
    MEDIA_LOG_I("Init");
    receiver_ = receiver;
    callback_ = callback;
    state_ = FilterState::INITIALIZED;
    audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    Status err = audioCaptureModule_->Init();
    if (err != Status::OK ) {
        MEDIA_LOG_E("Init module fail");
    }
}

Status AudioCaptureFilter::PrepareAudioCapture() {
    MEDIA_LOG_I("PrepareAudioCapture");
    FALSE_RETURN_V_MSG_W(state_ == FilterState::INITIALIZED, Status::ERROR_INVALID_OPERATION,
                         "filter is not in init state");
    state_ = FilterState::PREPARING;
    if (!taskPtr_) {
        taskPtr_ = std::make_shared<Task>("DataReader");
        taskPtr_->RegisterJob([this] { ReadLoop(); });
    }

    Status err = audioCaptureModule_->Prepare();
    return err;
}

Status AudioCaptureFilter::Prepare() {
    MEDIA_LOG_I("Prepare");
    callback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
        StreamType::STREAMTYPE_RAW_AUDIO);
    return Status::OK;
}

Status AudioCaptureFilter::Start() {
    MEDIA_LOG_I("Start");
    nextFilter_->Start();
    eos_ = false;
    auto res = Status::OK;
    // start audioCaptureModule firstly
    if (audioCaptureModule_) {
        res = audioCaptureModule_->Start();
    } else {
        res = Status::ERROR_INVALID_OPERATION;
    }
    FALSE_RETURN_V_MSG_E(res == Status::OK, res, "start audioCaptureModule failed");
    // start task secondly
    if (taskPtr_) {
        taskPtr_->Start();
    }
    return res;
}

Status AudioCaptureFilter::Pause() {
    MEDIA_LOG_I("Pause");
    state_ = FilterState::PAUSED;
    if (taskPtr_) {
        taskPtr_->Pause();
    }
    Status ret = Status::OK;
    if (audioCaptureModule_) {
        ret = audioCaptureModule_->Stop();
    }
    return ret;
}

Status AudioCaptureFilter::Resume() {
    MEDIA_LOG_I("Resume");
    state_ = FilterState::RUNNING;
    if (taskPtr_) {
        taskPtr_->Start();
    }
    return audioCaptureModule_ ? audioCaptureModule_->Start() : Status::ERROR_INVALID_OPERATION;
}

Status AudioCaptureFilter::Stop() {
    MEDIA_LOG_I("Stop");
    state_ = FilterState::INITIALIZED;
    // stop task firstly
    if (taskPtr_) {
        taskPtr_->Stop();
    }
    // stop audioCaptureModule secondly
    Status ret = Status::OK;
    if (audioCaptureModule_) {
        ret = audioCaptureModule_->Stop();
    }
    nextFilter_->Stop();
    return ret;
}

Status AudioCaptureFilter::Flush() {
    MEDIA_LOG_I("Flush");
    return Status::OK;
}

Status AudioCaptureFilter::Release() {
    MEDIA_LOG_I("Release");
    return Status::OK;
}

void AudioCaptureFilter::SetParameter(const std::shared_ptr<Meta>& meta) {
    MEDIA_LOG_I("SetParameter");
    audioCaptureModule_->SetParameter(meta);
}

void AudioCaptureFilter::GetParameter(std::shared_ptr<Meta>& meta) {
    MEDIA_LOG_I("GetParameter");
    audioCaptureModule_->GetParameter(meta);
}

Status AudioCaptureFilter::LinkNext(const std::shared_ptr<Filter>& nextFilter, StreamType outType) {
    MEDIA_LOG_I("LinkNext");
    auto meta = std::make_shared<Meta>();
    GetParameter(meta);
    nextFilter_ = nextFilter;
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<AudioCaptureFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, meta, filterLinkCallback);
    nextFilter->Prepare();
    return Status::OK;
}

FilterType AudioCaptureFilter::GetFilterType() {
    return FilterType::FILTERTYPE_ACAPTURE;
}

Status AudioCaptureFilter::SendEos() {
    MEDIA_LOG_I("SendEos");
    auto buffer = AVBuffer::CreateAVBuffer();
    AVBufferConfig avBufferConfig; 
    outputBufferQueue_->RequestBuffer(buffer, avBufferConfig, TIME_OUT_MS); 
    buffer->flag_ |= BUFFER_FLAG_EOS;
    outputBufferQueue_->PushBuffer(buffer, true);
    eos_ = true;
    return Status::OK;
}

void AudioCaptureFilter::ReadLoop() {
    MEDIA_LOG_I("ReadLoop");
    if (eos_.load()) {
        return;
    }
    uint64_t bufferSize = 0;
    auto ret = audioCaptureModule_->GetSize(bufferSize);
    if (ret != Status::OK || bufferSize <= 0) {
        MEDIA_LOG_E("Get audioCaptureModule buffer size fail");
        return;
    }
    auto buffer = AVBuffer::CreateAVBuffer();
    AVBufferConfig avBufferConfig; 
    avBufferConfig.size = bufferSize;
    avBufferConfig.memoryFlag = MemoryFlag::MEMORY_READ_WRITE;
    outputBufferQueue_->RequestBuffer(buffer, avBufferConfig, TIME_OUT_MS);
    ret = audioCaptureModule_->Read(buffer, bufferSize);
    if (ret == Status::ERROR_AGAIN) {
        MEDIA_LOG_E("audioCaptureModule read return again");
        return;
    }
    if (ret != Status::OK) {
        MEDIA_LOG_E("RequestBuffer fail");
        SendEos();
        return;
    }
    buffer->memory_->SetSize(bufferSize);

    Status status = outputBufferQueue_->PushBuffer(buffer, true);
    if (status != Status::OK) {
        MEDIA_LOG_E("PushBuffer fail");
    }
}

void AudioCaptureFilter::OnLinkedResult(const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta) {
    MEDIA_LOG_I("OnLinkedResult");
    outputBufferQueue_ = queue;
    PrepareAudioCapture();
}

Status AudioCaptureFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

Status AudioCaptureFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

Status AudioCaptureFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
                                            const std::shared_ptr<FilterLinkCallback> &callback) {
    return Status::OK;
}

Status AudioCaptureFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
                                             const std::shared_ptr<FilterLinkCallback> &callback) {
    return Status::OK;
}

Status AudioCaptureFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback> &callback) {
    return Status::OK;
}

void AudioCaptureFilter::OnUnlinkedResult(const std::shared_ptr<Meta> &meta) {
    (void) meta;
}

void AudioCaptureFilter::OnUpdatedResult(const std::shared_ptr<Meta> &meta) {
    (void) meta;
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS