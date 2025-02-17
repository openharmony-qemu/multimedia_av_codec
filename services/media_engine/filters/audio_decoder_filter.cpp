/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#define MEDIA_PIPELINE

#include "audio_decoder_filter.h"
#include "parameters.h"
#include "filter/filter_factory.h"
#include "common/log.h"
#include "common/media_core.h"
#include "sink/audio_sampleformat.h"
#include "avcodec_info.h"
#include "avcodec_sysevent.h"
#ifdef SUPPORT_DRM
#include "i_keysession_service.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "AudioDecoderFilter" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace MediaAVCodec;
using namespace OHOS::Media::Plugins;
constexpr int32_t SAMPLE_RATE_48K = 48000;
constexpr int32_t SAMPLE_FORMAT_BIT_DEPTH_16 = 16;
static AutoRegisterFilter<AudioDecoderFilter> g_registerAudioDecoderFilter("builtin.player.audiodecoder",
    FilterType::FILTERTYPE_ADEC, [](const std::string& name, const FilterType type) {
        return std::make_shared<AudioDecoderFilter>(name, FilterType::FILTERTYPE_ADEC);
    });

static const bool IS_FILTER_ASYNC = system::GetParameter("persist.media_service.async_filter", "1") == "1";

class AudioDecoderFilterLinkCallback : public FilterLinkCallback {
public:
    explicit AudioDecoderFilterLinkCallback(std::shared_ptr<AudioDecoderFilter> codecFilter)
        : codecFilter_(codecFilter) {}

    ~AudioDecoderFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto codecFilter = codecFilter_.lock()) {
            codecFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid codecFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto codecFilter = codecFilter_.lock()) {
            codecFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid codecFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto codecFilter = codecFilter_.lock()) {
            codecFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid codecFilter");
        }
    }
private:
    std::weak_ptr<AudioDecoderFilter> codecFilter_;
};

class AudioDecInputPortConsumerListener : public OHOS::Media::IConsumerListener {
public:
    explicit AudioDecInputPortConsumerListener(std::shared_ptr<AudioDecoderFilter> audioDecoderFilter)
    {
        audioDecoderFilter_ = audioDecoderFilter;
    }
    ~AudioDecInputPortConsumerListener() = default;

    void OnBufferAvailable() override
    {
        if (auto audioDecoderFilter = audioDecoderFilter_.lock()) {
            audioDecoderFilter->HandleInputBuffer();
        } else {
            MEDIA_LOG_I("invalid audioDecoderFilter");
        }
    }

private:
    std::weak_ptr<AudioDecoderFilter> audioDecoderFilter_;
};

AudioDecoderFilter::AudioDecoderFilter(std::string name, FilterType type): Filter(name, type)
{
    filterType_ = type;
    MEDIA_LOG_I_SHORT("audio decoder filter create");
}

AudioDecoderFilter::~AudioDecoderFilter()
{
    {
        std::lock_guard<std::mutex> lock(releaseMutex_);
        if (isReleased_.load()) {
            MEDIA_LOG_W_SHORT("AudioDecoderFilter has beed released.");
        } else {
            isReleased_.store(true);
            decoder_->Release();
        }
    }
    MEDIA_LOG_I_SHORT("audio decoder filter destroy");
}

void AudioDecoderFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("AudioDecoderFilter::Init.");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    decoder_ = std::make_shared<AudioDecoderAdapter>();
}

Status AudioDecoderFilter::DoPrepare()
{
    MEDIA_LOG_I("AudioDecoderFilter::Prepare.");
    Status ret = Status::OK;
    switch (filterType_) {
        case FilterType::FILTERTYPE_AENC:
            MEDIA_LOG_I("AudioDecoderFilter::FILTERTYPE_AENC.");
            ret = filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                StreamType::STREAMTYPE_ENCODED_AUDIO);
            break;
        case FilterType::FILTERTYPE_ADEC:
            ret = filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                StreamType::STREAMTYPE_RAW_AUDIO);
            break;
        default:
            break;
    }
    return ret;
}

Status AudioDecoderFilter::DoStart()
{
    MEDIA_LOG_E("AudioDecoderFilter::Start.");
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    auto ret = decoder_->Start();
    if (ret != Status::OK) {
        std::string mime;
        meta_->GetData(Tag::MIME_TYPE, mime);
        std::string instanceId = std::to_string(instanceId_);
        struct AudioCodecFaultInfo audioCodecFaultInfo;
        audioCodecFaultInfo.appName = appName_;
        audioCodecFaultInfo.instanceId = instanceId;
        audioCodecFaultInfo.callerType = "player_framework";
        audioCodecFaultInfo.audioCodec = mime;
        audioCodecFaultInfo.errMsg = "AudioDecoder start failed";
        FaultAudioCodecEventWrite(audioCodecFaultInfo);
    }
    return ret;
}

Status AudioDecoderFilter::DoPause()
{
    MEDIA_LOG_E("AudioDecoderFilter::Pause.");
    latestPausedTime_ = latestBufferTime_;
    return Status::OK;
}

Status AudioDecoderFilter::DoPauseAudioAlign()
{
    return DoPause();
}

Status AudioDecoderFilter::DoResume()
{
    MEDIA_LOG_E("AudioDecoderFilter::Resume.");
    refreshTotalPauseTime_ = true;
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    return decoder_->Start();
}

Status AudioDecoderFilter::DoResumeAudioAlign()
{
    return DoResume();
}

Status AudioDecoderFilter::DoStop()
{
    MEDIA_LOG_E("AudioDecoderFilter::Stop.");
    latestBufferTime_ = HST_TIME_NONE;
    latestPausedTime_ = HST_TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    return decoder_->Stop();
}

Status AudioDecoderFilter::DoFlush()
{
    MEDIA_LOG_E("AudioDecoderFilter::Flush.");
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    return decoder_->Flush();
}

Status AudioDecoderFilter::DoRelease()
{
    std::lock_guard<std::mutex> lock(releaseMutex_);
    MEDIA_LOG_I_SHORT("AudioDecoderFilter::Release.");
    if (isReleased_.load()) {
        MEDIA_LOG_W_SHORT("AudioDecoderFilter has beed released.");
        return Status::OK;
    }
    isReleased_.store(true);
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    return decoder_->Release();
}

void AudioDecoderFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("AudioDecoderFilter::SetParameter");
    FALSE_RETURN_MSG(decoder_ != nullptr, "decoder_ is nullptr");
    decoder_->SetParameter(parameter);
}

void AudioDecoderFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    FALSE_RETURN_MSG(decoder_ != nullptr, "decoder_ is nullptr");
    decoder_->GetOutputFormat(parameter);
}

Status AudioDecoderFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_E("AudioDecoderFilter::LinkNext.");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<AudioDecoderFilterLinkCallback>(shared_from_this());
    return nextFilter->OnLinked(outType, meta_, filterLinkCallback);
}

Status AudioDecoderFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

Status AudioDecoderFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

Status AudioDecoderFilter::ChangePlugin(std::shared_ptr<Meta> meta)
{
    MEDIA_LOG_I("AudioDecoderFilter::ChangePlugin.");
    std::string mime;
    meta_ = meta;
    bool mimeGetRes = meta_->GetData(Tag::MIME_TYPE, mime);
    if (!mimeGetRes && eventReceiver_ != nullptr) {
        MEDIA_LOG_I("AudioDecoderFilter cannot get mime");
        eventReceiver_->OnEvent({"audioDecoder", EventType::EVENT_ERROR, MSERR_UNSUPPORT_AUD_DEC_TYPE});
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S16LE);
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    Status ret = decoder_->ChangePlugin(mime, false, meta);
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "decoder_ ChangePlugin failed");

    return Status::OK;
}

FilterType AudioDecoderFilter::GetFilterType()
{
    return filterType_;
}

Status AudioDecoderFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("AudioDecoderFilter::OnLinked.");
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    onLinkedResultCallback_ = callback;
    std::string mime;
    bool mimeGetRes = meta->GetData(Tag::MIME_TYPE, mime);
    if (!mimeGetRes && eventReceiver_ != nullptr) {
        MEDIA_LOG_I("AudioDecoderFilter cannot get mime");
        eventReceiver_->OnEvent({"audioDecoder", EventType::EVENT_ERROR, MSERR_UNSUPPORT_AUD_DEC_TYPE});
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    UpdateTrackInfoSampleFormat(mime, meta);
    meta_ = meta;
    SetParameter(meta);
    auto ret = decoder_->Init(true, mime);
    FALSE_RETURN_V(ret == Status::OK, Status::ERROR_INVALID_PARAMETER);

    std::shared_ptr<AudioDecoderCallback> mediaCodecCallback
        = std::make_shared<AudioDecoderCallback>(shared_from_this());
    decoder_->SetCodecCallback(mediaCodecCallback);

    ret = decoder_->Configure(meta);
    if (ret != Status::OK && ret != Status::ERROR_INVALID_STATE) {
        MEDIA_LOG_I_SHORT("AudioDecoderFilter unsupport format");
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent({"audioDecoder", EventType::EVENT_ERROR, MSERR_UNSUPPORT_AUD_DEC_TYPE});
        }
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    decoder_->SetDumpInfo(isDump_, instanceId_);
    if (isDrmProtected_) {
        MEDIA_LOG_D("AudioDecoderFilter::isDrmProtected_ true.");
#ifdef SUPPORT_DRM
        decoder_->SetAudioDecryptionConfig(keySessionServiceProxy_, svpFlag_);
#endif
    }
    return Status::OK;
}

void AudioDecoderFilter::UpdateTrackInfoSampleFormat(const std::string& mime, const std::shared_ptr<Meta> &meta)
{
    if (mime != CodecMimeType::AUDIO_APE && mime != CodecMimeType::AUDIO_FLAC) {
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S16LE);
        return;
    }
    int32_t sampleRate = 0;
    bool sampleRateGetRes = meta->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    if (!sampleRateGetRes || sampleRate < SAMPLE_RATE_48K) {
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S16LE);
        return;
    }
    Plugins::AudioSampleFormat sampleFormat = Plugins::SAMPLE_U8;
    bool sampleFormatGetRes = meta->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleFormat);
    MEDIA_LOG_I_SHORT("Audio decoder set sampleFormat before is: " PUBLIC_LOG_D32, sampleFormat);
    if (sampleFormatGetRes && AudioSampleFormatToBitDepth(sampleFormat) > SAMPLE_FORMAT_BIT_DEPTH_16) {
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S32LE);
        return;
    }

    int32_t sampleDepth = 0;
    bool hasSampleDepthData = meta->GetData(Tag::AUDIO_BITS_PER_CODED_SAMPLE, sampleDepth);
    if (hasSampleDepthData && sampleDepth > SAMPLE_FORMAT_BIT_DEPTH_16) {
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S32LE);
        return;
    }
    bool hasPerRawSampleData = meta->GetData(Tag::AUDIO_BITS_PER_RAW_SAMPLE, sampleDepth);
    if (hasPerRawSampleData && sampleDepth > SAMPLE_FORMAT_BIT_DEPTH_16) {
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S32LE);
        return;
    }
    meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S16LE);
}

Status AudioDecoderFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    return Status::OK;
}

Status AudioDecoderFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    return Status::OK;
}

Status AudioDecoderFilter::SetDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
    bool svp)
{
    MEDIA_LOG_I("AudioDecoderFilter SetDecryptionConfig enter.");
    if (keySessionProxy == nullptr) {
        MEDIA_LOG_E("SetDecryptionConfig keySessionProxy is nullptr.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    isDrmProtected_ = true;
    (void)svp;
    // audio svp: false
    svpFlag_ = false;
#ifdef SUPPORT_DRM
    keySessionServiceProxy_ = keySessionProxy;
#endif
    return Status::OK;
}

void AudioDecoderFilter::SetDumpFlag(bool isDump)
{
    isDump_ = isDump;
}

void AudioDecoderFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I_SHORT("AudioDecoderFilter::OnLinkedResult");
    FALSE_RETURN_MSG(decoder_ != nullptr, "decoder_ is nullptr");
    decoder_->SetOutputBufferQueue(outputBufferQueue);
    decoder_->Prepare();

    sptr<AVBufferQueueProducer> inputProducer = decoder_->GetInputBufferQueue();
    FALSE_RETURN(inputProducer != nullptr && onLinkedResultCallback_ != nullptr);
    onLinkedResultCallback_->OnLinkedResult(inputProducer, meta_);
}

void AudioDecoderFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    meta_ = meta;
}

void AudioDecoderFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    meta_ = meta;
}

Status AudioDecoderFilter::HandleInputBuffer()
{
    ProcessInputBuffer();
    return Status::OK;
}

Status AudioDecoderFilter::DoProcessInputBuffer(int recvArg, bool dropFrame)
{
    decoder_->ProcessInputBuffer();
    return Status::OK;
}

void AudioDecoderFilter::OnInterrupted(bool isInterruptNeeded)
{
    FALSE_RETURN_MSG(decoder_ != nullptr, "audioDecoder is nullptr");
    if (isInterruptNeeded) {
        decoder_->Flush();
    }
}

Status AudioDecoderFilter::SetInputBufferQueueConsumerListener()
{
    sptr<IConsumerListener> consumerListener = new (std::nothrow) AudioDecInputPortConsumerListener(shared_from_this());
    FALSE_RETURN_V_MSG(consumerListener != nullptr, Status::ERROR_NULL_POINTER, "listener is nullptr");
    sptr<Media::AVBufferQueueConsumer> inputConsumer = decoder_->GetInputBufferQueueConsumer();
    FALSE_RETURN_V_MSG(inputConsumer != nullptr, Status::ERROR_NULL_POINTER, "inputConsumer is nullptr");
    return inputConsumer->SetBufferAvailableListener(consumerListener);
}

void AudioDecoderFilter::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D("AudioDecoderFilter::OnDumpInfo called.");
    if (decoder_ != nullptr) {
        decoder_->OnDumpInfo(fd);
    }
}

void AudioDecoderFilter::SetCallerInfo(uint64_t instanceId, const std::string& appName)
{
    instanceId_ = instanceId;
    appName_ = appName;
}

void AudioDecoderFilter::OnError(CodecErrorType errorType, int32_t errorCode)
{
    MEDIA_LOG_E("Audio Decoder error happened. ErrorType: %{public}d, errorCode: %{public}d",
        static_cast<int32_t>(errorType), errorCode);
    if (eventReceiver_ == nullptr) {
        MEDIA_LOG_E("Audio Decoder OnError failed due to eventReceiver is nullptr.");
        return;
    }
    switch (errorType) {
        case CodecErrorType::CODEC_DRM_DECRYTION_FAILED:
            eventReceiver_->OnEvent({"audioDecoder", EventType::EVENT_ERROR, MSERR_DRM_VERIFICATION_FAILED});
            break;
        default:
            break;
    }
}

AudioDecoderCallback::AudioDecoderCallback(std::shared_ptr<AudioDecoderFilter> audioDecoderFilter)
    : audioDecoderFilter_(audioDecoderFilter)
{
    MEDIA_LOG_I("AudioDecoderCallback");
}

AudioDecoderCallback::~AudioDecoderCallback()
{
    MEDIA_LOG_I("~AudioDecoderCallback");
}

void AudioDecoderCallback::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
    if (auto codecFilter = audioDecoderFilter_.lock()) {
        switch (errorType) {
            case AVCodecErrorType::AVCODEC_ERROR_DECRYTION_FAILED:
                codecFilter->OnError(CodecErrorType::CODEC_DRM_DECRYTION_FAILED, errorCode);
                break;
            default:
                codecFilter->OnError(CodecErrorType::CODEC_ERROR_INTERNAL, errorCode);
                break;
        }
    } else {
        MEDIA_LOG_I("OnError failed due to the codecFilter is invalid");
    }
}

void AudioDecoderCallback::OnOutputFormatChanged(const Format &format)
{
    (void)format;
}

void AudioDecoderCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    (void)index;
    (void)buffer;
}

void AudioDecoderCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    (void)index;
    (void)buffer;
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
