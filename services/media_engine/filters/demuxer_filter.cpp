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
#define HST_LOG_TAG "DemuxerFilter"

#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "filter/filter_factory.h"
#include "common/log.h"
#include "osal/task/autolock.h"
#include "common/media_core.h"
#include "demuxer_filter.h"
#include "media_types.h"
#include "avcodec_sysevent.h"
#include "scoped_timer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "DemuxerFilter" };
const char* ENHANCE_FLAG = "com.openharmony.deferredVideoEnhanceFlag";
const char* VIDEO_ID = "com.openharmony.videoId";
}

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace MediaAVCodec;
using MediaType = OHOS::Media::Plugins::MediaType;
using FileType = OHOS::Media::Plugins::FileType;
namespace {
    const std::string MIME_IMAGE = "image";
    const uint32_t DEFAULT_CACHE_LIMIT = 50 * 1024 * 1024; // 50M
    const int64_t DEMUXER_START_WARNING_MS = 20;
}
static AutoRegisterFilter<DemuxerFilter> g_registerAudioCaptureFilter(
    "builtin.player.demuxer", FilterType::FILTERTYPE_DEMUXER,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<DemuxerFilter>(name, FilterType::FILTERTYPE_DEMUXER);
    }
);

static std::map<AudioSampleFormat, AudioSampleFormat> g_sampleFormatBeToLe = {
    {AudioSampleFormat::SAMPLE_S16BE, AudioSampleFormat::SAMPLE_S16LE},
    {AudioSampleFormat::SAMPLE_S24BE, AudioSampleFormat::SAMPLE_S24LE},
    {AudioSampleFormat::SAMPLE_S32BE, AudioSampleFormat::SAMPLE_S32LE},
    {AudioSampleFormat::SAMPLE_F32BE, AudioSampleFormat::SAMPLE_F32LE},
    {AudioSampleFormat::SAMPLE_F64BE, AudioSampleFormat::SAMPLE_F32LE},
};

class DemuxerFilterLinkCallback : public FilterLinkCallback {
public:
    explicit DemuxerFilterLinkCallback(std::shared_ptr<DemuxerFilter> demuxerFilter)
        : demuxerFilter_(demuxerFilter) {}

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        auto demuxerFilter = demuxerFilter_.lock();
        FALSE_RETURN(demuxerFilter != nullptr);
        demuxerFilter->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        auto demuxerFilter = demuxerFilter_.lock();
        FALSE_RETURN(demuxerFilter != nullptr);
        demuxerFilter->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        auto demuxerFilter = demuxerFilter_.lock();
        FALSE_RETURN(demuxerFilter != nullptr);
        demuxerFilter->OnUpdatedResult(meta);
    }
private:
    std::weak_ptr<DemuxerFilter> demuxerFilter_;
};

class DemuxerFilterDrmCallback : public OHOS::MediaAVCodec::AVDemuxerCallback {
public:
    explicit DemuxerFilterDrmCallback(std::shared_ptr<DemuxerFilter> demuxerFilter) : demuxerFilter_(demuxerFilter)
    {
    }

    ~DemuxerFilterDrmCallback()
    {
        MEDIA_LOG_D_SHORT("~DemuxerFilterDrmCallback");
    }

    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo) override
    {
        MEDIA_LOG_I_SHORT("DemuxerFilterDrmCallback OnDrmInfoChanged");
        std::shared_ptr<DemuxerFilter> callback = demuxerFilter_.lock();
        if (callback == nullptr) {
            MEDIA_LOG_E_SHORT("OnDrmInfoChanged demuxerFilter callback is nullptr");
            return;
        }
        callback->OnDrmInfoUpdated(drmInfo);
    }

private:
    std::weak_ptr<DemuxerFilter> demuxerFilter_;
};

DemuxerFilter::DemuxerFilter(std::string name, FilterType type) : Filter(name, type)
{
    demuxer_ = std::make_shared<MediaDemuxer>();
    {
        AutoLock lock(mapMutex_);
        track_id_map_.clear();
    }
    FALSE_RETURN_MSG(demuxer_ != nullptr, "demuxer_ is nullptr");
    demuxer_->SetIsCreatedByFilter(true);
}

DemuxerFilter::~DemuxerFilter()
{
    MEDIA_LOG_D_SHORT("~DemuxerFilter enter");
    if (interruptMonitor_) {
        interruptMonitor_->DeregisterListener(demuxer_);
    }
}

void DemuxerFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    Init(receiver, callback, nullptr);
}

void DemuxerFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback,
    const std::shared_ptr<InterruptMonitor> &monitor)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Init");
    MEDIA_LOG_I_SHORT("DemuxerFilter Init");
    this->receiver_ = receiver;
    this->callback_ = callback;
    this->interruptMonitor_ = monitor;
    MEDIA_LOG_D_SHORT("DemuxerFilter Init for drm callback");

    std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> drmCallback =
        std::make_shared<DemuxerFilterDrmCallback>(shared_from_this());
    demuxer_->SetDrmCallback(drmCallback);
    demuxer_->SetEventReceiver(receiver);
    demuxer_->SetPlayerId(groupId_);
    if (interruptMonitor_) {
        interruptMonitor_->RegisterListener(demuxer_);
    }
}

Status DemuxerFilter::SetTranscoderMode()
{
    FALSE_RETURN_V(demuxer_ != nullptr, Status::ERROR_NULL_POINTER);
    Status status = demuxer_->SetTranscoderMode();
    FALSE_RETURN_V(status == Status::OK, status);
    isTransCoderMode_ = true;
    demuxer_->SetEnableOnlineFdCache(false);
    return status;
}

Status DemuxerFilter::SetSkippingAudioDecAndEnc()
{
    FALSE_RETURN_V(demuxer_ != nullptr, Status::ERROR_NULL_POINTER);
    return demuxer_->SetSkippingAudioDecAndEnc();
}

Status DemuxerFilter::SetDataSource(const std::shared_ptr<MediaSource> source)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::SetDataSource");
    MEDIA_LOG_D_SHORT("SetDataSource entered.");
    if (source == nullptr) {
        MEDIA_LOG_E_SHORT("Invalid source");
        return Status::ERROR_INVALID_PARAMETER;
    }
    mediaSource_ = source;
    Status ret = demuxer_->SetDataSource(mediaSource_);
    demuxer_->SetCacheLimit(DEFAULT_CACHE_LIMIT);
    return ret;
}

Status DemuxerFilter::SetSubtitleSource(const std::shared_ptr<MediaSource> source)
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->SetSubtitleSource(source);
}

Status DemuxerFilter::DoSetPerfRecEnabled(bool isPerfRecEnabled)
{
    isPerfRecEnabled_ = isPerfRecEnabled;
    FALSE_RETURN_V(demuxer_ != nullptr, Status::OK);
    demuxer_->SetPerfRecEnabled(isPerfRecEnabled);
    return Status::OK;
}

void DemuxerFilter::SetBundleName(const std::string& bundleName)
{
    if (demuxer_ != nullptr) {
        MEDIA_LOG_D_SHORT("SetBundleName bundleName: " PUBLIC_LOG_S, bundleName.c_str());
        demuxer_->SetBundleName(bundleName);
    }
}

void DemuxerFilter::SetCallerInfo(uint64_t instanceId, const std::string& appName)
{
    instanceId_ = instanceId;
    bundleName_ = appName;
}

void DemuxerFilter::RegisterVideoStreamReadyCallback(const std::shared_ptr<VideoStreamReadyCallback> &callback)
{
    MEDIA_LOG_I_SHORT("RegisterVideoStreamReadyCallback step into");
    if (callback != nullptr) {
        demuxer_->RegisterVideoStreamReadyCallback(callback);
    }
}

void DemuxerFilter::DeregisterVideoStreamReadyCallback()
{
    MEDIA_LOG_I_SHORT("DeregisterVideoStreamReadyCallback step into");
    FALSE_RETURN_MSG(demuxer_ != nullptr, "demuxer_ is nullptr");
    demuxer_->DeregisterVideoStreamReadyCallback();
}

Status DemuxerFilter::DoPrepare()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Prepare");
    FALSE_RETURN_V_MSG_E(mediaSource_ != nullptr, Status::ERROR_INVALID_PARAMETER, "No valid media source");
    std::vector<std::shared_ptr<Meta>> trackInfos = GetStreamMetaInfo();
    MEDIA_LOG_I_SHORT("trackCount: %{public}zu", trackInfos.size());
    if (trackInfos.size() == 0) {
        MEDIA_LOG_E_SHORT("Doprepare: trackCount is invalid.");
        receiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_DEMUXER_FAILED});
        return Status::ERROR_INVALID_PARAMETER;
    }
    int32_t successNodeCount = 0;
    Status ret = HandleTrackInfos(trackInfos, successNodeCount);
    if (ret != Status::OK) {
        return ret;
    }
    if (successNodeCount == 0) {
        receiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_UNSUPPORT_CONTAINER_TYPE});
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    return Status::OK;
}

Status DemuxerFilter::HandleTrackInfos(const std::vector<std::shared_ptr<Meta>> &trackInfos, int32_t &successNodeCount)
{
    Status ret = Status::OK;
    bool hasVideoFilter = false;
    bool hasInvalidVideo = false;
    bool hasInvalidAudio = false;
    bool hasAudioTrack = false;
    FALSE_RETURN_V_MSG(callback_ != nullptr, Status::OK, "callback is nullptr");
    for (size_t index = 0; index < trackInfos.size(); index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        FALSE_RETURN_V_MSG_E(meta != nullptr, Status::ERROR_INVALID_PARAMETER, "meta is invalid, index: %zu", index);
        std::string mime;
        if (!meta->GetData(Tag::MIME_TYPE, mime)) {
            MEDIA_LOG_E_SHORT("mimeType not found, index: %zu", index);
            continue;
        }
        MEDIA_LOG_D("mimeType: " PUBLIC_LOG_S ", index: " PUBLIC_LOG_U32, mime.c_str(), static_cast<uint32_t>(index));
        MediaType mediaType;
        if (!meta->GetData(Tag::MEDIA_TYPE, mediaType)) {
            MEDIA_LOG_E_SHORT("mediaType not found, index: %zu", index);
            continue;
        }
        FALSE_CONTINUE_NOLOG(!ShouldTrackSkipped(mediaType, mime, index));

        StreamType streamType;
        FALSE_RETURN_V_NOLOG(FindStreamType(streamType, mediaType, mime, index, meta), Status::ERROR_INVALID_PARAMETER);
        bool isTrackInvalid = mime.find("invalid") == 0;
        hasInvalidVideo |= (isTrackInvalid && streamType == StreamType::STREAMTYPE_ENCODED_VIDEO);
        hasInvalidAudio |= (isTrackInvalid && streamType == StreamType::STREAMTYPE_ENCODED_AUDIO);
        FALSE_CONTINUE_NOLOG(!isTrackInvalid);
        UpdateTrackIdMap(streamType, static_cast<int32_t>(index));
        FALSE_CONTINUE_NOLOG(streamType != StreamType::STREAMTYPE_ENCODED_VIDEO || !hasVideoFilter);
        hasVideoFilter |= (streamType == StreamType::STREAMTYPE_ENCODED_VIDEO && isEnableReselectVideoTrack_);
        FALSE_CONTINUE_NOLOG(mediaType != Plugins::MediaType::AUDIO || !hasAudioTrack);
        ret = callback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED, streamType);
        FALSE_RETURN_V_MSG_E(ret == Status::OK || FaultDemuxerEventInfoWrite(streamType) != Status::OK, ret,
            "OnCallback Link Filter Fail.");
        if (mediaType == Plugins::MediaType::AUDIO) {
            hasVideoFilter = true;
        }
        successNodeCount++;
    }
    bool isOnlyInvalidAVTrack = (hasInvalidVideo && !demuxer_->HasVideo())
                                 || (hasInvalidAudio && !demuxer_->HasAudio());
    if (isOnlyInvalidAVTrack) {
        MEDIA_LOG_E("Only has invalid video or invalid audio track");
        receiver_->OnEvent({"demuxer_filter", EventType::EVENT_ERROR, MSERR_DEMUXER_FAILED});
    }
    return ret;
}

Status DemuxerFilter::FaultDemuxerEventInfoWrite(StreamType& streamType)
{
    MEDIA_LOG_I_SHORT("FaultDemuxerEventInfoWrite enter.");
    struct DemuxerFaultInfo demuxerInfo;
    demuxerInfo.appName = bundleName_;
    demuxerInfo.instanceId = std::to_string(instanceId_);
    demuxerInfo.callerType = "player_framework";
    demuxerInfo.sourceType = static_cast<int8_t>(mediaSource_->GetSourceType());
    demuxerInfo.containerFormat = CollectVideoAndAudioMime();
    demuxerInfo.streamType = std::to_string(static_cast<int32_t>(streamType));
    demuxerInfo.errMsg = "OnCallback Link Filter Fail.";
    FaultDemuxerEventWrite(demuxerInfo);
    return Status::OK;
}

std::string DemuxerFilter::CollectVideoAndAudioMime()
{
    MEDIA_LOG_I_SHORT("CollectVideoAndAudioInfo entered.");
    std::string mime;
    std::string videoMime = "";
    std::string audioMime = "";
    std::vector<std::shared_ptr<Meta>> metaInfo = GetStreamMetaInfo();
    for (const auto& trackInfo : metaInfo) {
        if (!(trackInfo->GetData(Tag::MIME_TYPE, mime))) {
            MEDIA_LOG_W_SHORT("Get MIME fail");
            continue;
        }
        if (IsVideoMime(mime)) {
            videoMime += (mime + ",");
        } else if (IsAudioMime(mime)) {
            audioMime += (mime + ",");
        }
    }
    return videoMime + " : " + audioMime;
}

bool DemuxerFilter::IsVideoMime(const std::string& mime)
{
    return mime.find("video/") == 0;
}

bool DemuxerFilter::IsAudioMime(const std::string& mime)
{
    return mime.find("audio/") == 0;
}

void DemuxerFilter::UpdateTrackIdMap(StreamType streamType, int32_t index)
{
    AutoLock lock(mapMutex_);
    auto it = track_id_map_.find(streamType);
    if (it != track_id_map_.end()) {
        it->second.push_back(index);
    } else {
        std::vector<int32_t> vec = {index};
        track_id_map_.insert({streamType, vec});
    }
}

Status DemuxerFilter::PrepareBeforeStart()
{
    if (isLoopStarted.load()) {
        MEDIA_LOG_I_SHORT("Loop is started. Not need start again.");
        return Status::OK;
    }
    SetIsNotPrepareBeforeStart(false);
    return Filter::Start();
}

Status DemuxerFilter::DoStart()
{
    if (isLoopStarted.load()) {
        MEDIA_LOG_I_SHORT("Loop is started. Resume only.");
        return DoResume();
    }
    MEDIA_LOG_I_SHORT("Loop is not started. PrepareBeforeStart firstly.");
    isLoopStarted = true;
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Start");
    ScopedTimer timer("Demuxer Start", DEMUXER_START_WARNING_MS);
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    SetIsInPrePausing(false);
    auto ret = demuxer_->Start();
    state_ = ret == Status::OK ? FilterState::RUNNING : FilterState::ERROR;
    return ret;
}

Status DemuxerFilter::DoStop()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Stop");
    MEDIA_LOG_I_SHORT("Stop in");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    SetIsInPrePausing(true);
    auto ret = demuxer_->Stop();
    SetIsInPrePausing(false);
    state_ = ret == Status::OK ? FilterState::STOPPED : FilterState::ERROR;
    return ret;
}

Status DemuxerFilter::DoPause()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Pause");
    MEDIA_LOG_I_SHORT("Pause in");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    if (state_ == FilterState::FROZEN) {
        MEDIA_LOG_I("current state is frozen");
        state_ = FilterState::PAUSED;
        return Status::OK;
    }
    SetIsInPrePausing(true);
    auto ret = demuxer_->Pause();
    SetIsInPrePausing(false);
    state_ = ret == Status::OK ? FilterState::PAUSED : FilterState::ERROR;
    return ret;
}

Status DemuxerFilter::DoFreeze()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Freeze");
    MEDIA_LOG_I("Freeze in");
    FALSE_RETURN_V_MSG(state_ == FilterState::RUNNING, Status::OK, "current state is %{public}d", state_);
    SetIsInPrePausing(true);
    auto ret = demuxer_->Pause();
    SetIsInPrePausing(false);
    state_ = ret == Status::OK ? FilterState::FROZEN : FilterState::ERROR;
    return ret;
}

Status DemuxerFilter::DoPauseDragging()
{
    MEDIA_LOG_I("DoPauseDragging in");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->PauseDragging();
}

Status DemuxerFilter::DoPauseAudioAlign()
{
    MEDIA_LOG_I("DoPauseAudioAlign in");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->PauseAudioAlign();
}

Status DemuxerFilter::PauseForSeek()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::PauseForSeek");
    MEDIA_LOG_I_SHORT("PauseForSeek in");
    // demuxer pause first for auido render immediatly
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    demuxer_->Pause();
    auto it = nextFiltersMap_.find(StreamType::STREAMTYPE_ENCODED_VIDEO);
    if (it != nextFiltersMap_.end() && it->second.size() == 1) {
        auto filter = it->second.back();
        if (filter != nullptr) {
            MEDIA_LOG_I_SHORT("filter pause");
            return filter->Pause();
        }
    }
    return Status::ERROR_INVALID_OPERATION;
}

Status DemuxerFilter::DoResume()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Resume");
    MEDIA_LOG_I_SHORT("Resume in");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    SetIsInPrePausing(false);
    auto ret = demuxer_->Resume();
    state_ = ret == Status::OK ? FilterState::RUNNING : FilterState::ERROR;
    return ret;
}

Status DemuxerFilter::DoUnFreeze()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::UnFreeze");
    MEDIA_LOG_I("UnFreeze in");
    FALSE_RETURN_V_MSG(state_ == FilterState::FROZEN, Status::OK, "current state is %{public}d", state_);
    SetIsInPrePausing(false);
    auto ret = demuxer_->Resume();
    state_ = ret == Status::OK ? FilterState::RUNNING : FilterState::ERROR;
    return ret;
}

Status DemuxerFilter::DoResumeDragging()
{
    MEDIA_LOG_I("DoResumeDragging in");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->ResumeDragging();
}

Status DemuxerFilter::DoResumeAudioAlign()
{
    MEDIA_LOG_I("DoResumeAudioAlign in");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->ResumeAudioAlign();
}

Status DemuxerFilter::ResumeForSeek()
{
    FALSE_RETURN_V_MSG(isNotPrepareBeforeStart_, Status::OK, "Current is not need resumeForSeek");
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::ResumeForSeek");
    MEDIA_LOG_I_SHORT("ResumeForSeek in size: %{public}zu", nextFiltersMap_.size());
    auto it = nextFiltersMap_.find(StreamType::STREAMTYPE_ENCODED_VIDEO);
    if (it != nextFiltersMap_.end() && it->second.size() == 1) {
        auto filter = it->second.back();
        if (filter != nullptr) {
            MEDIA_LOG_I_SHORT("filter resume");
            filter->Resume();
            filter->WaitAllState(FilterState::RUNNING);
        }
    }
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->Resume();
}

Status DemuxerFilter::DoFlush()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Flush");
    MEDIA_LOG_D_SHORT("Flush entered");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->Flush();
}

Status DemuxerFilter::DoPreroll()
{
    MEDIA_LOG_I("DoPreroll in");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    Status ret = demuxer_->Preroll();
    isLoopStarted.store(true);
    return ret;
}

Status DemuxerFilter::DoWaitPrerollDone(bool render)
{
    (void)render;
    MEDIA_LOG_I("DoWaitPrerollDone in.");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->PausePreroll();
}

Status DemuxerFilter::Reset()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Reset");
    MEDIA_LOG_I_SHORT("Reset in");
    {
        AutoLock lock(mapMutex_);
        track_id_map_.clear();
    }
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->Reset();
}

bool DemuxerFilter::IsRefParserSupported()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::IsRefParserSupported");
    MEDIA_LOG_D("IsRefParserSupported entered");
    return demuxer_->IsRefParserSupported();
}

Status DemuxerFilter::StartReferenceParser(int64_t startTimeMs, bool isForward)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::StartReferenceParser");
    MEDIA_LOG_D_SHORT("StartReferenceParser entered");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->StartReferenceParser(startTimeMs, isForward);
}

Status DemuxerFilter::GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::GetFrameLayerInfo");
    MEDIA_LOG_D_SHORT("GetFrameLayerInfo entered");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->GetFrameLayerInfo(videoSample, frameLayerInfo);
}

Status DemuxerFilter::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::GetFrameLayerInfo");
    MEDIA_LOG_D_SHORT("GetFrameLayerInfo entered");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->GetFrameLayerInfo(frameId, frameLayerInfo);
}

Status DemuxerFilter::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::GetGopLayerInfo");
    MEDIA_LOG_D_SHORT("GetGopLayerInfo entered");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->GetGopLayerInfo(gopId, gopLayerInfo);
}

Status DemuxerFilter::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::GetIFramePos");
    MEDIA_LOG_D_SHORT("GetIFramePos entered");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->GetIFramePos(IFramePos);
}

Status DemuxerFilter::Dts2FrameId(int64_t dts, uint32_t &frameId)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Dts2FrameId");
    MEDIA_LOG_D_SHORT("Dts2FrameId entered");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->Dts2FrameId(dts, frameId);
}

Status DemuxerFilter::SeekMs2FrameId(int64_t seekMs, uint32_t &frameId)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::SeekMs2FrameId");
    MEDIA_LOG_D("SeekMs2FrameId entered");
    return demuxer_->SeekMs2FrameId(seekMs, frameId);
}

Status DemuxerFilter::FrameId2SeekMs(uint32_t frameId, int64_t &seekMs)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::FrameId2SeekMs");
    MEDIA_LOG_D("FrameId2SeekMs entered");
    return demuxer_->FrameId2SeekMs(frameId, seekMs);
}

void DemuxerFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I_SHORT("SetParameter enter");
}

void DemuxerFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I_SHORT("GetParameter enter");
}

void DemuxerFilter::SetDumpFlag(bool isDump)
{
    isDump_ = isDump;
    if (demuxer_ != nullptr) {
        demuxer_->SetDumpInfo(isDump_, instanceId_);
    }
}

std::map<int32_t, sptr<AVBufferQueueProducer>> DemuxerFilter::GetBufferQueueProducerMap()
{
    return demuxer_->GetBufferQueueProducerMap();
}

Status DemuxerFilter::PauseTaskByTrackId(int32_t trackId)
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->PauseTaskByTrackId(trackId);
}

Status DemuxerFilter::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::SeekTo");
    MEDIA_LOG_D_SHORT("SeekTo in");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->SeekTo(seekTime, mode, realSeekTime);
}

Status DemuxerFilter::StartTask(int32_t trackId)
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->StartTask(trackId);
}

Status DemuxerFilter::SelectTrack(int32_t trackId)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::SelectTrack");
    MEDIA_LOG_I_SHORT("SelectTrack called");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->SelectTrack(trackId);
}

std::vector<std::shared_ptr<Meta>> DemuxerFilter::GetStreamMetaInfo() const
{
    auto trackMetas = demuxer_->GetStreamMetaInfo();
    trackMetas.erase(std::remove_if(trackMetas.begin(), trackMetas.end(),
        [](const std::shared_ptr<Meta> &trackMeta) {
            FALSE_RETURN_V_NOLOG(trackMeta, false);
            Plugins::MediaType mediaType;
            bool hasMediaType = trackMeta->GetData(Tag::MEDIA_TYPE, mediaType);
            FALSE_RETURN_V_NOLOG(hasMediaType, false);
            FALSE_RETURN_V_NOLOG(mediaType == Plugins::MediaType::AUXILIARY, false);
            return true;
        }), trackMetas.end());
    return trackMetas;
}

std::shared_ptr<Meta> DemuxerFilter::GetGlobalMetaInfo() const
{
    return demuxer_->GetGlobalMetaInfo();
}

Status DemuxerFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    int32_t trackId = -1;
    FALSE_RETURN_V_MSG_E(nextFilter != nullptr, Status::ERROR_INVALID_OPERATION, "nextFilter nullptr");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_INVALID_OPERATION, "demuxer_ nullptr");
    FALSE_RETURN_V_MSG_E(FindTrackId(outType, trackId), Status::ERROR_INVALID_PARAMETER, "FindTrackId failed");

    std::shared_ptr<Meta> globalInfo = demuxer_->GetGlobalMetaInfo();
    FileType fileType = FileType::UNKNOW;
    if (globalInfo == nullptr || !globalInfo->GetData(Tag::MEDIA_FILE_TYPE, fileType)) {
        MEDIA_LOG_W("Get file type failed");
    }
    std::vector<std::shared_ptr<Meta>> trackInfos = GetStreamMetaInfo();
    std::shared_ptr<Meta> meta = trackInfos[trackId];
    for (MapIt iter = meta->begin(); iter != meta->end(); iter++) {
        MEDIA_LOG_D_SHORT("Link " PUBLIC_LOG_S, iter->first.c_str());
    }
    std::string mimeType;
    meta->GetData(Tag::MIME_TYPE, mimeType);
    MEDIA_LOG_I("LinkNext NextFilter, mimeType " PUBLIC_LOG_S " filterType " PUBLIC_LOG_D32,
        mimeType.c_str(), nextFilter->GetFilterType());

    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);

    meta->SetData(Tag::REGULAR_TRACK_ID, trackId);
    if (fileType == FileType::AVI) {
        MEDIA_LOG_I("File type is AVI " PUBLIC_LOG_D32, static_cast<int32_t>(FileType::AVI));
        meta->SetData(Tag::MEDIA_FILE_TYPE, FileType::AVI);
    }
    std::shared_ptr<Meta> userInfo = demuxer_->GetUserMeta();
    std::string enhanceflag;
    if (userInfo != nullptr && userInfo->GetData(ENHANCE_FLAG, enhanceflag)) {
        MEDIA_LOG_I("Link enhanceflag: %{public}s", enhanceflag.c_str());
        meta->SetData(ENHANCE_FLAG, enhanceflag);
    }
    std::string videoId;
    if (userInfo != nullptr && userInfo->GetData(VIDEO_ID, videoId)) {
        MEDIA_LOG_I("Link videoId: %{public}s", videoId.c_str());
        meta->SetData(VIDEO_ID, videoId);
    }
    std::shared_ptr<FilterLinkCallback> filterLinkCallback
        = std::make_shared<DemuxerFilterLinkCallback>(shared_from_this());
    return nextFilter->OnLinked(outType, meta, filterLinkCallback);
}

Status DemuxerFilter::GetBitRates(std::vector<uint32_t>& bitRates)
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_INVALID_OPERATION,
        "GetBitRates failed, demuxer_ = nullptr.");
    return demuxer_->GetBitRates(bitRates);
}

Status DemuxerFilter::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    if (demuxer_ == nullptr) {
        return Status::ERROR_INVALID_OPERATION;
    }
    return demuxer_->GetDownloadInfo(downloadInfo);
}

Status DemuxerFilter::GetPlaybackInfo(PlaybackInfo& playbackInfo)
{
    if (demuxer_ == nullptr) {
        return Status::ERROR_INVALID_OPERATION;
    }
    return demuxer_->GetPlaybackInfo(playbackInfo);
}

Status DemuxerFilter::SelectBitRate(uint32_t bitRate, bool isAutoSelect)
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_INVALID_OPERATION,
        "SelectBitRate failed, demuxer_ = nullptr.");
    return demuxer_->SelectBitRate(bitRate, isAutoSelect);
}

bool DemuxerFilter::FindTrackId(StreamType outType, int32_t &trackId)
{
    AutoLock lock(mapMutex_);
    auto it = track_id_map_.find(outType);
    if (it != track_id_map_.end()) {
        trackId = it->second.front();
        it->second.erase(it->second.begin());
        if (it->second.empty()) {
            track_id_map_.erase(it);
        }
        return true;
    }
    return false;
}

bool DemuxerFilter::FindStreamType(StreamType &streamType, MediaType mediaType, std::string mime,
    size_t index, std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I_SHORT("mediaType is %{public}d", static_cast<int32_t>(mediaType));
    if (mediaType == Plugins::MediaType::SUBTITLE) {
        streamType = StreamType::STREAMTYPE_SUBTITLE;
    } else if (mediaType == Plugins::MediaType::AUDIO) {
        if (mime == std::string(MimeType::AUDIO_RAW)) {
            if (CheckIsBigendian(meta)) {
                streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
            } else {
                streamType = StreamType::STREAMTYPE_RAW_AUDIO;
            }
        } else {
            streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
        }
    } else if (mediaType == MediaType::VIDEO) {
        streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    } else {
        MEDIA_LOG_E_SHORT("streamType not found, index: %zu", index);
        return false;
    }
    return true;
}

bool DemuxerFilter::CheckIsBigendian(std::shared_ptr<Meta> &meta)
{
    AudioSampleFormat sampleFormat = SAMPLE_U8;
    bool sampleFormatGetRes = meta->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleFormat);
    FALSE_RETURN_V(sampleFormatGetRes, false);
    MEDIA_LOG_I("samplefomart :%{public}d", sampleFormat);
    auto iter = g_sampleFormatBeToLe.find(sampleFormat);
    if (iter != g_sampleFormatBeToLe.end()) {
        meta->SetData(Tag::AUDIO_RAW_SAMPLE_FORMAT, iter->first);
        meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, iter->second);
        return true;
    }
    return false;
}

bool DemuxerFilter::ShouldTrackSkipped(Plugins::MediaType mediaType, std::string mime, size_t index)
{
    if (mime.substr(0, MIME_IMAGE.size()).compare(MIME_IMAGE) == 0) {
        MEDIA_LOG_W_SHORT("is image track, continue");
        return true;
    } else if (!disabledMediaTracks_.empty() && disabledMediaTracks_.find(mediaType) != disabledMediaTracks_.end()) {
        MEDIA_LOG_W_SHORT("mediaType disabled, index: %zu", index);
        return true;
    } else if (mediaType == Plugins::MediaType::TIMEDMETA) {
        return true;
    }
    return false;
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

Status DemuxerFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
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
        MEDIA_LOG_E_SHORT("meta is invalid.");
        return;
    }
    int32_t trackId;
    if (!meta->GetData(Tag::REGULAR_TRACK_ID, trackId)) {
        MEDIA_LOG_E_SHORT("trackId not found");
        return;
    }
    demuxer_->SetOutputBufferQueue(trackId, outputBufferQueue);
    FALSE_RETURN_NOLOG(trackId >= 0);
    int32_t decoderFramerateUpperLimit = 0;
    if (meta->GetData(Tag::VIDEO_DECODER_RATE_UPPER_LIMIT, decoderFramerateUpperLimit)) {
        demuxer_->SetDecoderFramerateUpperLimit(decoderFramerateUpperLimit, trackId);
    }
    double framerate;
    if (meta->GetData(Tag::VIDEO_FRAME_RATE, framerate)) {
        demuxer_->SetFrameRate(framerate, trackId);
    }
}

void DemuxerFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
}

void DemuxerFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
}

bool DemuxerFilter::IsDrmProtected()
{
    MEDIA_LOG_D_SHORT("IsDrmProtected");
    return demuxer_->IsLocalDrmInfosExisted();
}

void DemuxerFilter::OnDrmInfoUpdated(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo)
{
    MEDIA_LOG_I_SHORT("OnDrmInfoUpdated");
    if (this->receiver_ != nullptr) {
        this->receiver_->OnEvent({"demuxer_filter", EventType::EVENT_DRM_INFO_UPDATED, drmInfo});
    } else {
        MEDIA_LOG_E_SHORT("OnDrmInfoUpdated failed receiver is nullptr");
    }
}

bool DemuxerFilter::GetDuration(int64_t& durationMs)
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, false, "demuxer_ is nullptr");
    return demuxer_->GetDuration(durationMs);
}

Status DemuxerFilter::OptimizeDecodeSlow(bool isDecodeOptimizationEnabled)
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_INVALID_OPERATION, "OptimizeDecodeSlow failed.");
    return demuxer_->OptimizeDecodeSlow(isDecodeOptimizationEnabled);
}

Status DemuxerFilter::SetSpeed(float speed)
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_INVALID_OPERATION, "SetSpeed failed.");
    return demuxer_->SetSpeed(speed);
}

Status DemuxerFilter::DisableMediaTrack(Plugins::MediaType mediaType)
{
    disabledMediaTracks_.emplace(mediaType);
    return demuxer_->DisableMediaTrack(mediaType);
}

void DemuxerFilter::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D_SHORT("DemuxerFilter::OnDumpInfo called.");
    if (demuxer_ != nullptr) {
        demuxer_->OnDumpInfo(fd);
    }
}

bool DemuxerFilter::IsRenderNextVideoFrameSupported()
{
    MEDIA_LOG_D_SHORT("DemuxerFilter::OnDumpInfo called.");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, false, "demuxer_ is nullptr");
    return demuxer_->IsRenderNextVideoFrameSupported();
}

Status DemuxerFilter::ResumeDemuxerReadLoop()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::ResumeDemuxerReadLoop");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_INVALID_OPERATION, "ResumeDemuxerReadLoop failed.");
    MEDIA_LOG_I("ResumeDemuxerReadLoop start.");
    return demuxer_->ResumeDemuxerReadLoop();
}

Status DemuxerFilter::PauseDemuxerReadLoop()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::PauseDemuxerReadLoop");
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_INVALID_OPERATION, "PauseDemuxerReadLoop failed.");
    MEDIA_LOG_I("PauseDemuxerReadLoop start.");
    return demuxer_->PauseDemuxerReadLoop();
}

bool DemuxerFilter::IsVideoEos()
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, false, "demuxer_ is nullptr");
    return demuxer_->IsVideoEos();
}

bool DemuxerFilter::HasEosTrack()
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, false, "demuxer_ is nullptr");
    return demuxer_->HasEosTrack();
}

void DemuxerFilter::WaitForBufferingEnd()
{
    FALSE_RETURN_MSG(demuxer_ != nullptr, "demuxer_ is nullptr");
    demuxer_->WaitForBufferingEnd();
}

int32_t DemuxerFilter::GetCurrentVideoTrackId()
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, -1, "demuxer_ is nullptr");
    return demuxer_->GetCurrentVideoTrackId();
}

void DemuxerFilter::SetIsNotPrepareBeforeStart(bool isNotPrepareBeforeStart)
{
    isNotPrepareBeforeStart_ = isNotPrepareBeforeStart;
}

void DemuxerFilter::SetIsEnableReselectVideoTrack(bool isEnable)
{
    isEnableReselectVideoTrack_ = isEnable;
    demuxer_->SetIsEnableReselectVideoTrack(isEnable);
}

void DemuxerFilter::SetApiVersion(int32_t apiVersion)
{
    apiVersion_ = apiVersion;
    demuxer_->SetApiVersion(apiVersion);
}

bool DemuxerFilter::IsLocalFd()
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, false, "demuxer_ is nullptr");
    return demuxer_->IsLocalFd();
}

Status DemuxerFilter::RebootPlugin()
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    return demuxer_->RebootPlugin();
}

uint64_t DemuxerFilter::GetCachedDuration()
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, 0, "demuxer_ is nullptr");
    return demuxer_->GetCachedDuration();
}

void DemuxerFilter::RestartAndClearBuffer()
{
    FALSE_RETURN_MSG(demuxer_ != nullptr, "demuxer_ is nullptr");
    return demuxer_->RestartAndClearBuffer();
}

bool DemuxerFilter::IsFlvLive()
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, false, "demuxer_ is nullptr");
    return demuxer_->IsFlvLive();
}

void DemuxerFilter::SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter)
{
    FALSE_RETURN_MSG(demuxer_ != nullptr, "demuxer_ is nullptr");
    demuxer_->SetSyncCenter(syncCenter);
}

Status DemuxerFilter::StopBufferring(bool isAppBackground)
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_NULL_POINTER, "demuxer_ is nullptr");
    return demuxer_->StopBufferring(isAppBackground);
}

Status DemuxerFilter::SetMediaMuted(OHOS::Media::MediaType mediaType, bool isMuted)
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    if (mediaType == OHOS::Media::MediaType::MEDIA_TYPE_VID) {
        demuxer_->SetMediaMuted(mediaType, isMuted);
        isVideoMuted_ = isMuted;
    }
    return Status::OK;
}

void DemuxerFilter::HandleDecoderErrorFrame(int64_t pts)
{
    FALSE_RETURN_MSG(demuxer_ != nullptr, "demuxer_ is nullptr");
    demuxer_->HandleDecoderErrorFrame(pts);
}

bool DemuxerFilter::IsVideoMuted()
{
    if (demuxer_ != nullptr) {
        return demuxer_->IsVideoMuted();
    }
    return isVideoMuted_;
}

Status DemuxerFilter::NotifyResumeUnMute()
{
    FALSE_RETURN_V_MSG_E(demuxer_ != nullptr, Status::ERROR_UNKNOWN, "demuxer_ is nullptr");
    demuxer_->NotifyResumeUnMute();
    return Status::OK;
}

std::shared_ptr<Meta> DemuxerFilter::GetGlobalInfo()
{
    FALSE_RETURN_V(demuxer_ != nullptr, nullptr);
    return demuxer_->GetUserMeta();
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
