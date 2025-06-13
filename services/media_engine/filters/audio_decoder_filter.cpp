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
#include "avcodec_trace.h"
#include "parameters.h"
#include "filter/filter_factory.h"
#include "common/log.h"
#include "common/media_core.h"
#include "sink/audio_sampleformat.h"
#include "avcodec_info.h"
#include "avcodec_sysevent.h"
#include "avcodec_trace.h"
#include "scoped_timer.h"
#ifdef SUPPORT_DRM
#include "imedia_key_session_service.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "AudioDecoderFilter" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
namespace {
constexpr uint32_t BUFFER_STATUS_INIT_PROCESS_ALWAYS =
    static_cast<uint32_t>(InOutPortBufferStatus::INIT_PROCESS_ALWAYS);
constexpr uint32_t BUFFER_STATUS_INIT_IGNORE_RET = static_cast<uint32_t>(InOutPortBufferStatus::INIT_IGNORE_RET);
constexpr uint32_t BUFFER_STATUS_INIT = static_cast<uint32_t>(InOutPortBufferStatus::INIT);
constexpr uint32_t BUFFER_STATUS_AVAIL_IN_OUT = static_cast<uint32_t>(InOutPortBufferStatus::INPORT_AVAIL) |
    static_cast<uint32_t>(InOutPortBufferStatus::OUTPORT_AVAIL);
constexpr uint32_t BUFFER_STATUS_AVAIL_IN = static_cast<uint32_t>(InOutPortBufferStatus::INPORT_AVAIL);
constexpr uint32_t BUFFER_STATUS_AVAIL_OUT = static_cast<uint32_t>(InOutPortBufferStatus::OUTPORT_AVAIL);
constexpr uint32_t BUFFER_STATUS_AVAIL_NONE = 0;
constexpr uint32_t BUFFER_STATUS_OUT_EOS_START = static_cast<uint32_t>(InOutPortBufferStatus::OUT_EOS_START);
constexpr uint32_t BUFFER_STATUS_AVAIL_OUT_OUT_EOS_START_DONE =
    static_cast<uint32_t>(InOutPortBufferStatus::OUTPORT_AVAIL) |
    static_cast<uint32_t>(InOutPortBufferStatus::OUT_EOS_START) |
    static_cast<uint32_t>(InOutPortBufferStatus::OUT_EOS_DONE);
constexpr uint32_t BUFFER_STATUS_AVAIL_OUT_OUT_EOS_START = static_cast<uint32_t>(InOutPortBufferStatus::OUTPORT_AVAIL) |
    static_cast<uint32_t>(InOutPortBufferStatus::OUT_EOS_START);
constexpr uint32_t BUFFER_STATUS_OUT_EOS_START_DONE = static_cast<uint32_t>(InOutPortBufferStatus::OUT_EOS_START) |
    static_cast<uint32_t>(InOutPortBufferStatus::OUT_EOS_DONE);
}

using namespace MediaAVCodec;
using namespace OHOS::Media::Plugins;
constexpr int32_t SAMPLE_RATE_48K = 48000;
constexpr int32_t SAMPLE_FORMAT_BIT_DEPTH_16 = 16;
constexpr int64_t DECODER_INIT_WARNING_MS = 50;
static AutoRegisterFilter<AudioDecoderFilter> g_registerAudioDecoderFilter("builtin.player.audiodecoder",
    FilterType::FILTERTYPE_ADEC, [](const std::string& name, const FilterType type) {
        bool isAsyncMode = system::GetParameter("debug.media_service.audio.audiodecoder_async", "1") == "1";
        return std::make_shared<AudioDecoderFilter>(name, FilterType::FILTERTYPE_ADEC, isAsyncMode);
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
        MEDIA_LOG_D("AudioDecInputPortConsumerListener OnBufferAvailable");
        if (auto audioDecoderFilter = audioDecoderFilter_.lock()) {
            audioDecoderFilter->HandleInputBuffer(false);
        } else {
            MEDIA_LOG_I("invalid audioDecoderFilter");
        }
    }

private:
    std::weak_ptr<AudioDecoderFilter> audioDecoderFilter_;
};

class AudioDecOutPortProducerListener : public IRemoteStub<IProducerListener> {
public:
    explicit AudioDecOutPortProducerListener(std::shared_ptr<AudioDecoderFilter> audioDecoderFilter)
    {
        audioDecoderFilter_ = audioDecoderFilter;
    }
    virtual ~AudioDecOutPortProducerListener() = default;

    int OnRemoteRequest(uint32_t code, MessageParcel& arguments, MessageParcel& reply, MessageOption& option) override
    {
        return IPCObjectStub::OnRemoteRequest(code, arguments, reply, option);
    }

    void OnBufferAvailable() override
    {
        MEDIA_LOG_D("AudioDecOutPortProducerListener OnBufferAvailable");
        if (auto audioDecoderFilter = audioDecoderFilter_.lock()) {
            audioDecoderFilter->HandleInputBuffer(true);
        } else {
            MEDIA_LOG_I("invalid audioDecoderFilter");
        }
    }

private:
    std::weak_ptr<AudioDecoderFilter> audioDecoderFilter_;
};

AudioDecoderFilter::AudioDecoderFilter(std::string name, FilterType type): Filter(name, type, IS_FILTER_ASYNC)
{
    filterType_ = type;
    MEDIA_LOG_I_SHORT("audio decoder filter create");
}

AudioDecoderFilter::AudioDecoderFilter(
    std::string name, FilterType type, bool isAsyncMode): Filter(name, type, isAsyncMode)
{
    filterType_ = type;
    MEDIA_LOG_I_SHORT("audio decoder filter create, isAsyncMode:" PUBLIC_LOG_D32, isAsyncMode);
}

AudioDecoderFilter::~AudioDecoderFilter()
{
    Filter::StopFilterTask();
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
    state_ = FilterState::READY;
    return ret;
}

Status AudioDecoderFilter::DoStart()
{
    MEDIA_LOG_I("AudioDecoderFilter::Start.");
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");

    std::unique_lock<std::mutex> lock(bufferStatusMutex_);
    bufferStatus_ = BUFFER_STATUS_INIT_PROCESS_ALWAYS;
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
    state_ = ret == Status::OK ? FilterState::RUNNING : FilterState::ERROR;
    return ret;
}

Status AudioDecoderFilter::DoPause()
{
    MEDIA_LOG_I("AudioDecoderFilter::Pause.");
    latestPausedTime_ = latestBufferTime_;

    state_ = FilterState::PAUSED;
    return Status::OK;
}

Status AudioDecoderFilter::DoFreeze()
{
    MEDIA_LOG_E("AudioDecoderFilter::Freeze.");
    FALSE_RETURN_V_MSG(state_ == FilterState::RUNNING, Status::OK, "current state is %{public}d", state_);
    latestPausedTime_ = latestBufferTime_;
    state_ = FilterState::FROZEN;
    return Status::OK;
}

Status AudioDecoderFilter::DoPauseAudioAlign()
{
    return DoPause();
}

Status AudioDecoderFilter::DoResume()
{
    MEDIA_LOG_I("AudioDecoderFilter::Resume.");

    std::unique_lock<std::mutex> lock(bufferStatusMutex_);
    bufferStatus_ = BUFFER_STATUS_INIT_PROCESS_ALWAYS;
    refreshTotalPauseTime_ = true;
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    auto ret = decoder_->Start();
    state_ = ret == Status::OK ? FilterState::RUNNING : FilterState::ERROR;
    return ret;
}

Status AudioDecoderFilter::DoUnFreeze()
{
    MEDIA_LOG_E("AudioDecoderFilter::UnFreeze.");
    FALSE_RETURN_V_MSG(state_ == FilterState::FROZEN, Status::OK, "current state is %{public}d", state_);
    refreshTotalPauseTime_ = true;
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    auto ret = decoder_->Start();
    state_ = ret == Status::OK ? FilterState::RUNNING : FilterState::ERROR;
    return ret;
}

Status AudioDecoderFilter::DoResumeAudioAlign()
{
    return DoResume();
}

Status AudioDecoderFilter::DoStop()
{
    MEDIA_LOG_I("AudioDecoderFilter::Stop.");
    latestBufferTime_ = HST_TIME_NONE;
    latestPausedTime_ = HST_TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    auto ret = decoder_->Stop();
    state_ = ret == Status::OK ? FilterState::STOPPED : FilterState::ERROR;
    return ret;
}

Status AudioDecoderFilter::DoFlush()
{
    MEDIA_LOG_I("AudioDecoderFilter::Flush.");

    std::unique_lock<std::mutex> lock(bufferStatusMutex_);
    bufferStatus_ = BUFFER_STATUS_INIT_PROCESS_ALWAYS;
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
    FALSE_RETURN_V_MSG(meta != nullptr, Status::ERROR_NULL_POINTER, "meta is nullptr");
    std::string mime;
    bool mimeGetRes = meta->GetData(Tag::MIME_TYPE, mime);
    if (!mimeGetRes && eventReceiver_ != nullptr) {
        MEDIA_LOG_I("AudioDecoderFilter cannot get mime");
        eventReceiver_->OnEvent({"audioDecoder", EventType::EVENT_ERROR, MSERR_UNSUPPORT_AUD_DEC_TYPE});
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    UpdateTrackInfoSampleFormat(mime, meta);
    meta_ = meta;
    FALSE_RETURN_V_MSG(decoder_ != nullptr, Status::ERROR_NULL_POINTER, "decoder_ is nullptr");
    Status ret = decoder_->ChangePlugin(mime, false, meta);
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "ChangePlugin failed");

    if (IsAsyncMode()) {
        ret = SetInputBufferQueueConsumerListener();
        FALSE_RETURN_V_MSG(ret == Status::OK, ret, "ChangePlugin SetInputBufferQueueConsumerListener failed");

        ret = SetOutputBufferQueueProducerListener();
        FALSE_RETURN_V_MSG(ret == Status::OK, ret, "ChangePlugin SetOutputBufferQueueProducerListener failed");
    }

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
    Status ret = Status::OK;
    {
        ScopedTimer timer("AudioDecoder Init", DECODER_INIT_WARNING_MS);
        ret = decoder_->Init(true, mime);
    }
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
    MEDIA_LOG_I_SHORT("UpdateTrackInfoSampleFormat mime:" PUBLIC_LOG_S, mime.c_str());
    FALSE_RETURN_NOLOG(mime != CodecMimeType::AUDIO_RAW);
    if (mime != CodecMimeType::AUDIO_APE && mime != CodecMimeType::AUDIO_FLAC) {
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S16LE);
        return;
    }
    int32_t sampleRate = 0;
    bool sampleRateGetRes = meta->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    MEDIA_LOG_I_SHORT("UpdateTrackInfoSampleFormat sampleRate:" PUBLIC_LOG_D32, sampleRate);
    if (!sampleRateGetRes || sampleRate < SAMPLE_RATE_48K) {
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S16LE);
        return;
    }
    Plugins::AudioSampleFormat sampleFormat = Plugins::SAMPLE_U8;
    bool sampleFormatGetRes = meta->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleFormat);
    MEDIA_LOG_I_SHORT("sampleFormat before is: " PUBLIC_LOG_D32, sampleFormat);
    if (sampleFormatGetRes && AudioSampleFormatToBitDepth(sampleFormat) > SAMPLE_FORMAT_BIT_DEPTH_16) {
        MEDIA_LOG_I_SHORT("sampleFormat after is: " PUBLIC_LOG_D32, Plugins::SAMPLE_S32LE);
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S32LE);
        return;
    }

    int32_t sampleDepth = 0;
    bool hasSampleDepthData = meta->GetData(Tag::AUDIO_BITS_PER_CODED_SAMPLE, sampleDepth);
    if (hasSampleDepthData && sampleDepth > SAMPLE_FORMAT_BIT_DEPTH_16) {
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S32LE);
        MEDIA_LOG_I_SHORT("sampleFormat after is: " PUBLIC_LOG_D32, Plugins::SAMPLE_S32LE);
        return;
    }
    bool hasPerRawSampleData = meta->GetData(Tag::AUDIO_BITS_PER_RAW_SAMPLE, sampleDepth);
    if (hasPerRawSampleData && sampleDepth > SAMPLE_FORMAT_BIT_DEPTH_16) {
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S32LE);
        MEDIA_LOG_I_SHORT("sampleFormat after is: " PUBLIC_LOG_D32, Plugins::SAMPLE_S32LE);
        return;
    }
    MEDIA_LOG_I_SHORT("sampleFormat after is: " PUBLIC_LOG_D32, Plugins::SAMPLE_S16LE);
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

    if (IsAsyncMode()) {
        Status ret = SetInputBufferQueueConsumerListener();
        FALSE_RETURN_MSG(ret == Status::OK, "OnLinkedResult SetInputBufferQueueConsumerListener failed");

        ret = SetOutputBufferQueueProducerListener();
        FALSE_RETURN_MSG(ret == Status::OK, "OnLinkedResult SetOutputBufferQueueProducerListener failed");
    }
 
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

Status AudioDecoderFilter::HandleInputBuffer(bool isTriggeredByOutPort)
{
    ProcessInputBuffer(static_cast<int>(isTriggeredByOutPort ?
        BufferQueueBufferAVailable::BUFFER_AVAILABLE_OUT_PORT : BufferQueueBufferAVailable::BUFFER_AVAILABLE_IN_PORT));
    return Status::OK;
}

bool AudioDecoderFilter::IsNeedProcessInput(bool isOutPort)
{
    MEDIA_LOG_D("AudioDecoderFilter::IsNeedProcessInput bufferStatus:" PUBLIC_LOG_U32X ", isOutPort:" PUBLIC_LOG_D32,
        bufferStatus_, isOutPort);
    FALSE_RETURN_V_MSG_DD((bufferStatus_ != BUFFER_STATUS_AVAIL_IN), isOutPort, "IN avail, need process outport");
    FALSE_RETURN_V_MSG_DD((bufferStatus_ != BUFFER_STATUS_AVAIL_OUT), !isOutPort, "OUT avail, need process inport");
    FALSE_RETURN_V_MSG_DD((bufferStatus_ != BUFFER_STATUS_AVAIL_IN_OUT), true, "IN and OUT avail, need process");
    FALSE_RETURN_V_MSG_DD(
        (bufferStatus_ != BUFFER_STATUS_INIT), !isOutPort, "initial stage:" PUBLIC_LOG_D32, isOutPort);
    FALSE_RETURN_V_MSG_DD((bufferStatus_ != BUFFER_STATUS_OUT_EOS_START), isOutPort, "EOS START, need process outport");
    FALSE_RETURN_V_MSG_DD((bufferStatus_ != BUFFER_STATUS_AVAIL_OUT_OUT_EOS_START_DONE), false,
        "OUT avail, EOS START and DONE, no need process");
    FALSE_RETURN_V_MSG_I((bufferStatus_ != BUFFER_STATUS_AVAIL_OUT_OUT_EOS_START), false,
        "OUT avail and EOS START, should not happen, no need process");
    FALSE_RETURN_V_MSG_I((bufferStatus_ != BUFFER_STATUS_OUT_EOS_START_DONE), false,
        "EOS START and DONE, should not happen, no need process");
    FALSE_RETURN_V_MSG_D((bufferStatus_ != BUFFER_STATUS_INIT_PROCESS_ALWAYS), true, "state change, need process");
    if (bufferStatus_ == BUFFER_STATUS_AVAIL_NONE) {
        bufferStatus_ = isOutPort ? bufferStatus_ : static_cast<uint32_t>(InOutPortBufferStatus::INPORT_AVAIL);
        MEDIA_LOG_D("neither IN nor OUT avail, need process outport");
        return isOutPort;
    }

    MEDIA_LOG_I("AudioDecoderFilter::IsNeedProcessInput, should not happen, bufferStatus:" PUBLIC_LOG_U32X
        ", isOutPort:" PUBLIC_LOG_D32, bufferStatus_, isOutPort);
    return true; // DO ProcessInput by default
}

Status AudioDecoderFilter::DoProcessInputBuffer(int recvArg, bool dropFrame)
{
    bool isOutPort = recvArg == static_cast<int>(BufferQueueBufferAVailable::BUFFER_AVAILABLE_OUT_PORT);
    uint32_t lastBufferStatus = BUFFER_STATUS_INIT_PROCESS_ALWAYS; // DO ProcessInput by default
    MEDIA_TRACE_DEBUG_POSTFIX(std::string("AudioDecoderFilter::DoProcessInputBuffer-In:") +
        std::to_string(isOutPort) + "," + std::to_string(dropFrame) + "," + std::to_string(bufferStatusMutex_), "1");
    {
        std::unique_lock<std::mutex> lock(bufferStatusMutex_, std::try_to_lock);
        if (lock.owns_lock()) {
            FALSE_RETURN_V_MSG_W(!dropFrame, Status::OK, "task created before flush, ignore obsolete process request");
            FALSE_RETURN_V_NOLOG(IsNeedProcessInput(isOutPort), Status::OK);
            lastBufferStatus = bufferStatus_;
        }
    }
    MEDIA_TRACE_DEBUG_POSTFIX(std::string("AudioDecoderFilter::DoProcessInputBuffer-Process:") +
        std::to_string(isOutPort) + "," + std::to_string(dropFrame) + "," + std::to_string(lastBufferStatus), "2");

    uint32_t bufferStatus = BUFFER_STATUS_INIT_IGNORE_RET;
    decoder_->ProcessInputBufferInner(isOutPort, dropFrame, bufferStatus);
    FALSE_RETURN_V_MSG_W((bufferStatus != BUFFER_STATUS_INIT_IGNORE_RET), Status::OK,
        "bufferStatus not updated, should not happen");

    std::unique_lock<std::mutex> lock(bufferStatusMutex_, std::try_to_lock);
    if (lock.owns_lock()) {
        // If bufferStatus_ changed, the return bufferStatus is obsolete, should discard
        if (bufferStatus_ == lastBufferStatus || bufferStatus_ != BUFFER_STATUS_INIT_PROCESS_ALWAYS) {
            MEDIA_LOG_DD("bufferStatus:" PUBLIC_LOG_U32X ", lastBufferStatus:" PUBLIC_LOG_U32X
                ", curBufferStatus:" PUBLIC_LOG_U32X ", isOutPort:" PUBLIC_LOG_D32,
                bufferStatus, lastBufferStatus, bufferStatus_, isOutPort);
            bufferStatus_ = bufferStatus;
        } else {
            MEDIA_LOG_W("bufferStatus_ change, ignore returned bufferStatus:" PUBLIC_LOG_U32X
                ", lastBufferStatus:" PUBLIC_LOG_U32X ", curBufferStatus:" PUBLIC_LOG_U32X
                ", isOutPort:" PUBLIC_LOG_D32, bufferStatus, lastBufferStatus, bufferStatus_, isOutPort);
        }
    } else {
        MEDIA_LOG_W("bufferStatus_ change may occur, ignore returned bufferStatus:" PUBLIC_LOG_U32X
            ", lastBufferStatus:" PUBLIC_LOG_U32X ", isOutPort:" PUBLIC_LOG_D32,
        bufferStatus, lastBufferStatus, isOutPort);
    }
    return Status::OK;
}

Status AudioDecoderFilter::SetInputBufferQueueConsumerListener()
{
    sptr<IConsumerListener> consumerListener =
        OHOS::sptr<AudioDecInputPortConsumerListener>::MakeSptr(shared_from_this());
    FALSE_RETURN_V_MSG(consumerListener != nullptr, Status::ERROR_NULL_POINTER, "consumerListener is nullptr");
    sptr<Media::AVBufferQueueConsumer> inputConsumer = decoder_->GetInputBufferQueueConsumer();
    FALSE_RETURN_V_MSG(inputConsumer != nullptr, Status::ERROR_NULL_POINTER, "inputConsumer is nullptr");
    return inputConsumer->SetBufferAvailableListener(consumerListener);
}

Status AudioDecoderFilter::SetOutputBufferQueueProducerListener()
{
    sptr<IProducerListener> producerListener =
        OHOS::sptr<AudioDecOutPortProducerListener>::MakeSptr(shared_from_this());
    FALSE_RETURN_V_MSG(producerListener != nullptr, Status::ERROR_NULL_POINTER, "producerListener is nullptr");
    sptr<Media::AVBufferQueueProducer> outputProducer = decoder_->GetOutputBufferQueueProducer();
    FALSE_RETURN_V_MSG(outputProducer != nullptr, Status::ERROR_NULL_POINTER, "outputProducer is nullptr");
    return outputProducer->SetBufferAvailableListener(producerListener);
}

void AudioDecoderFilter::OnInterrupted(bool isInterruptNeeded)
{
    FALSE_RETURN_MSG(decoder_ != nullptr, "audioDecoder is nullptr");
    if (isInterruptNeeded) {
        decoder_->Flush();
    }
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

void AudioDecoderFilter::OnOutputFormatChanged(const Format& format)
{
    FALSE_RETURN_NOLOG(meta_ && nextFilter_);
    std::string mime;
    FALSE_RETURN_MSG(meta_->GetData(Tag::MIME_TYPE, mime) && mime != CodecMimeType::AUDIO_VIVID,
        "Audio mimeType %{public}s unsupport format change", mime.c_str());
    int32_t sampleRate = 0;
    int32_t channels = 0;
    int32_t sampleFormat = 0;
    FALSE_RETURN(format.GetIntValue(Tag::AUDIO_SAMPLE_RATE, sampleRate));
    FALSE_RETURN(format.GetIntValue(Tag::AUDIO_CHANNEL_COUNT, channels));
    FALSE_RETURN(format.GetIntValue(Tag::AUDIO_SAMPLE_FORMAT, sampleFormat));
    std::string formatChangeInfo = "AudioFormatChange sampleRate " + std::to_string(sampleRate) + " channels "
                                    + std::to_string(channels) + " smpFmt " + std::to_string(sampleFormat);
    MediaAVCodec::AVCodecTrace trace(formatChangeInfo);
    MEDIA_LOG_I("%{public}s", formatChangeInfo.c_str());
    int32_t preSampleRate = 0;
    int32_t preChannels = 0;
    int32_t preSampleFormat = 0;
    meta_->GetData(Tag::AUDIO_SAMPLE_RATE, preSampleRate);
    meta_->GetData(Tag::AUDIO_OUTPUT_CHANNELS, preChannels);
    meta_->GetData(Tag::AUDIO_SAMPLE_FORMAT, preSampleFormat);
    FALSE_RETURN_NOLOG(preSampleRate != sampleRate || preChannels != channels || preSampleFormat != sampleFormat);

    meta_->SetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    meta_->SetData(Tag::AUDIO_OUTPUT_CHANNELS, channels);
    meta_->SetData(Tag::AUDIO_SAMPLE_FORMAT, sampleFormat);
    nextFilter_->HandleFormatChange(meta_);
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
    auto codecFilter = audioDecoderFilter_.lock();
    FALSE_RETURN_MSG(codecFilter != nullptr, "AudioDecoderFilter is nullptr");
    codecFilter->OnOutputFormatChanged(format);
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
