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

#include "audio_encoder_filter.h"
#include "filter/filter_factory.h"
#include "media_codec/media_codec.h"
#include "avcodec_sysevent.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace OHOS::MediaAVCodec;
static AutoRegisterFilter<AudioEncoderFilter> g_registerAudioEncoderFilter("builtin.recorder.audioencoder",
    FilterType::FILTERTYPE_AENC,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<AudioEncoderFilter>(name, FilterType::FILTERTYPE_AENC);
    });

class AudioEncoderFilterLinkCallback : public FilterLinkCallback {
public:
    explicit AudioEncoderFilterLinkCallback(std::shared_ptr<AudioEncoderFilter> audioEncoderFilter)
        : audioEncoderFilter_(std::move(audioEncoderFilter))
    {
    }

    ~AudioEncoderFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto encoderFilter = audioEncoderFilter_.lock()) {
            encoderFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid encoderFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto encoderFilter = audioEncoderFilter_.lock()) {
            encoderFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid encoderFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto encoderFilter = audioEncoderFilter_.lock()) {
            encoderFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid encoderFilter");
        }
    }
private:
    std::weak_ptr<AudioEncoderFilter> audioEncoderFilter_;
};

AudioEncoderFilter::AudioEncoderFilter(std::string name, FilterType type): Filter(name, type)
{
    filterType_ = type;
    MEDIA_LOG_I("audio encoder filter create");
}

AudioEncoderFilter::~AudioEncoderFilter()
{
    MEDIA_LOG_I("audio encoder filter destroy");
}

Status AudioEncoderFilter::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    MEDIA_LOG_I("SetCodecFormat");
    FALSE_RETURN_V(format->Get<Tag::MIME_TYPE>(codecMimeType_), Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

void AudioEncoderFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    mediaCodec_ = std::make_shared<MediaCodec>();
    mediaCodec_->Init(codecMimeType_, true);
}

Status AudioEncoderFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    configureParameter_ = parameter;
    int32_t ret = mediaCodec_->Configure(parameter);
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::Configure error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

sptr<Surface> AudioEncoderFilter::GetInputSurface()
{
    MEDIA_LOG_I("GetInputSurface");
    return mediaCodec_->GetInputSurface();
}

Status AudioEncoderFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare");
    switch (filterType_) {
        case FilterType::FILTERTYPE_AENC:
            filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                StreamType::STREAMTYPE_ENCODED_AUDIO);
            break;
        default:
            break;
    }
    return Status::OK;
}

Status AudioEncoderFilter::DoStart()
{
    MEDIA_LOG_I("Start");
    int32_t ret = mediaCodec_->Start();
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::DoStart error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    return Status::OK;
}

Status AudioEncoderFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    return Status::OK;
}

Status AudioEncoderFilter::DoStop()
{
    MEDIA_LOG_I("Stop");
    int32_t ret = mediaCodec_->Stop();
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::DoStop error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::DoFlush()
{
    MEDIA_LOG_I("Flush");
    int32_t ret = mediaCodec_->Flush();
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::DoFlush error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::DoRelease()
{
    MEDIA_LOG_I("Release");
    int32_t ret = mediaCodec_->Release();
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::DoRelease error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::NotifyEos()
{
    MEDIA_LOG_I("NotifyEos");
    int32_t ret = mediaCodec_->NotifyEos();
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::NotifyEos error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

void AudioEncoderFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    mediaCodec_->SetParameter(parameter);
}

void AudioEncoderFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
}

Status AudioEncoderFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("LinkNext");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<AudioEncoderFilterLinkCallback>(shared_from_this());
    auto ret = nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    if (ret != Status::OK) {
        SetFaultEvent("AudioEncoderFilter::LinkNext error", (int32_t)ret);
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "OnLinked failed");
    return Status::OK;
}

Status AudioEncoderFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status AudioEncoderFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

FilterType AudioEncoderFilter::GetFilterType()
{
    MEDIA_LOG_I("GetFilterType");
    return filterType_;
}

Status AudioEncoderFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    onLinkedResultCallback_ = callback;
    return Status::OK;
}

Status AudioEncoderFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}

Status AudioEncoderFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void AudioEncoderFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnLinkedResult");
    mediaCodec_->SetOutputBufferQueue(outputBufferQueue);
    mediaCodec_->Prepare();
    onLinkedResultCallback_->OnLinkedResult(mediaCodec_->GetInputBufferQueue(), meta);
}

void AudioEncoderFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUpdatedResult");
    (void) meta;
}

void AudioEncoderFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUnlinkedResult");
    (void) meta;
}

void AudioEncoderFilter::SetFaultEvent(const std::string &errMsg, int32_t ret)
{
    SetFaultEvent(errMsg + ", ret = " + std::to_string(ret));
}

void AudioEncoderFilter::SetFaultEvent(const std::string &errMsg)
{
    AudioCodecFaultInfo audioCodecFaultInfo;
    audioCodecFaultInfo.appName = bundleName_;
    audioCodecFaultInfo.instanceId = std::to_string(instanceId_);
    audioCodecFaultInfo.callerType ="player_framework";
    audioCodecFaultInfo.audioCodec = codecMimeType_;
    audioCodecFaultInfo.errMsg = errMsg;
    FaultAudioCodecEventWrite(audioCodecFaultInfo);
}

void AudioEncoderFilter::SetCallingInfo(int32_t appUid, int32_t appPid,
    const std::string &bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS