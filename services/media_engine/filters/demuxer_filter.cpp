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

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "DemuxerFilter" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace MediaAVCodec;
using MediaType = OHOS::Media::Plugins::MediaType;
namespace {
    const std::string MIME_IMAGE = "image";
}
static AutoRegisterFilter<DemuxerFilter> g_registerAudioCaptureFilter(
    "builtin.player.demuxer", FilterType::FILTERTYPE_DEMUXER,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<DemuxerFilter>(name, FilterType::FILTERTYPE_DEMUXER);
    }
);

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
    AutoLock lock(mapMutex_);
    track_id_map_.clear();
}

DemuxerFilter::~DemuxerFilter()
{
    MEDIA_LOG_D_SHORT("~DemuxerFilter enter");
}

void DemuxerFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Init");
    MEDIA_LOG_I_SHORT("DemuxerFilter Init");
    this->receiver_ = receiver;
    this->callback_ = callback;
    MEDIA_LOG_D_SHORT("DemuxerFilter Init for drm callback");

    std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> drmCallback =
        std::make_shared<DemuxerFilterDrmCallback>(shared_from_this());
    demuxer_->SetDrmCallback(drmCallback);
    demuxer_->SetEventReceiver(receiver);
    demuxer_->SetPlayerId(groupId_);
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
    return ret;
}

Status DemuxerFilter::SetSubtitleSource(const std::shared_ptr<MediaSource> source)
{
    return demuxer_->SetSubtitleSource(source);
}

void DemuxerFilter::SetInterruptState(bool isInterruptNeeded)
{
    demuxer_->SetInterruptState(isInterruptNeeded);
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
    demuxer_->DeregisterVideoStreamReadyCallback();
}

Status DemuxerFilter::DoPrepare()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Prepare");
    FALSE_RETURN_V_MSG_E(mediaSource_ != nullptr, Status::ERROR_INVALID_PARAMETER, "No valid media source");
    std::vector<std::shared_ptr<Meta>> trackInfos = demuxer_->GetStreamMetaInfo();
    MEDIA_LOG_I_SHORT("trackCount: %{public}d", trackInfos.size());
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
    for (size_t index = 0; index < trackInfos.size(); index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        FALSE_RETURN_V_MSG_E(meta != nullptr, Status::ERROR_INVALID_PARAMETER, "meta is invalid, index: %zu", index);
        std::string mime;
        if (!meta->GetData(Tag::MIME_TYPE, mime)) {
            MEDIA_LOG_E_SHORT("mimeType not found, index: %zu", index);
            continue;
        }
        MediaType mediaType;
        if (!meta->GetData(Tag::MEDIA_TYPE, mediaType)) {
            MEDIA_LOG_E_SHORT("mediaType not found, index: %zu", index);
            continue;
        }
        if (ShouldTrackSkipped(mediaType, mime, index)) {
            continue;
        }
        StreamType streamType;
        if (!FindStreamType(streamType, mediaType, mime, index)) {
            return Status::ERROR_INVALID_PARAMETER;
        }
        UpdateTrackIdMap(streamType, static_cast<int32_t>(index));
        if (callback_ == nullptr) {
            MEDIA_LOG_W_SHORT("callback is nullptr");
            continue;
        }
        ret = callback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED, streamType);
        if (ret != Status::OK) {
            FaultDemuxerEventInfoWrite(streamType);
        }
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "OnCallback Link Filter Fail.");
        successNodeCount++;
    }
    return ret;
}

void DemuxerFilter::FaultDemuxerEventInfoWrite(StreamType& streamType)
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
}

std::string DemuxerFilter::CollectVideoAndAudioMime()
{
    MEDIA_LOG_I_SHORT("CollectVideoAndAudioInfo entered.");
    std::string mime;
    std::string videoMime = "";
    std::string audioMime = "";
    std::vector<std::shared_ptr<Meta>> metaInfo = demuxer_->GetStreamMetaInfo();
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

Status DemuxerFilter::DoPrepareFrame(bool renderFirstFrame)
{
    MEDIA_LOG_I_SHORT("PrepareFrame.");
    auto ret = demuxer_->PrepareFrame(renderFirstFrame);
    if (ret == Status::OK) {
        isPrepareFramed = true;
    }
    return ret;
}

Status DemuxerFilter::PrepareBeforeStart()
{
    if (isLoopStarted.load()) {
        MEDIA_LOG_I_SHORT("Loop is started. Not need start again.");
        return Status::OK;
    }
    return Filter::Start();
}

Status DemuxerFilter::DoStart()
{
    if (isLoopStarted.load()) {
        MEDIA_LOG_I_SHORT("Loop is started. Resume only.");
        return Filter::Resume();
    }
    MEDIA_LOG_I_SHORT("Loop is not started. PrepareBeforeStart firstly.");
    isLoopStarted = true;
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Start");
    if (isPrepareFramed.load()) {
        return demuxer_->Resume();
    }
    return demuxer_->Start();
}

Status DemuxerFilter::DoStop()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Stop");
    MEDIA_LOG_I_SHORT("Stop in");
    return demuxer_->Stop();
}

Status DemuxerFilter::DoPause()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Pause");
    MEDIA_LOG_I_SHORT("Pause in");
    return demuxer_->Pause();
}

Status DemuxerFilter::PauseForSeek()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::PauseForSeek");
    MEDIA_LOG_I_SHORT("PauseForSeek in");
    // demuxer pause first for auido render immediatly
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
    return demuxer_->Resume();
}

Status DemuxerFilter::DoResumeDragging()
{
    MEDIA_LOG_I("DoResumeDragging in");
    return demuxer_->ResumeDragging();
}

Status DemuxerFilter::ResumeForSeek()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::ResumeForSeek");
    MEDIA_LOG_I_SHORT("ResumeForSeek in size: %{public}d", nextFiltersMap_.size());
    auto it = nextFiltersMap_.find(StreamType::STREAMTYPE_ENCODED_VIDEO);
    if (it != nextFiltersMap_.end() && it->second.size() == 1) {
        auto filter = it->second.back();
        if (filter != nullptr) {
            MEDIA_LOG_I_SHORT("filter resume");
            filter->Resume();
        }
    }
    return demuxer_->Resume();
}

Status DemuxerFilter::DoFlush()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Flush");
    MEDIA_LOG_D_SHORT("Flush entered");
    return demuxer_->Flush();
}

Status DemuxerFilter::Reset()
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Reset");
    MEDIA_LOG_I_SHORT("Reset in");
    {
        AutoLock lock(mapMutex_);
        track_id_map_.clear();
    }
    return demuxer_->Reset();
}

Status DemuxerFilter::StartReferenceParser(int64_t startTimeMs, bool isForward)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::StartReferenceParser");
    MEDIA_LOG_D_SHORT("StartReferenceParser entered");
    return demuxer_->StartReferenceParser(startTimeMs, isForward);
}

Status DemuxerFilter::GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::GetFrameLayerInfo");
    MEDIA_LOG_D_SHORT("GetFrameLayerInfo entered");
    return demuxer_->GetFrameLayerInfo(videoSample, frameLayerInfo);
}

Status DemuxerFilter::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::GetFrameLayerInfo");
    MEDIA_LOG_D_SHORT("GetFrameLayerInfo entered");
    return demuxer_->GetFrameLayerInfo(frameId, frameLayerInfo);
}

Status DemuxerFilter::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::GetGopLayerInfo");
    MEDIA_LOG_D_SHORT("GetGopLayerInfo entered");
    return demuxer_->GetGopLayerInfo(gopId, gopLayerInfo);
}

Status DemuxerFilter::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::GetIFramePos");
    MEDIA_LOG_D_SHORT("GetIFramePos entered");
    return demuxer_->GetIFramePos(IFramePos);
}

Status DemuxerFilter::Dts2FrameId(int64_t dts, uint32_t &frameId, bool offset)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::Dts2FrameId");
    MEDIA_LOG_D_SHORT("Dts2FrameId entered");
    return demuxer_->Dts2FrameId(dts, frameId, offset);
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

std::map<uint32_t, sptr<AVBufferQueueProducer>> DemuxerFilter::GetBufferQueueProducerMap()
{
    return demuxer_->GetBufferQueueProducerMap();
}

Status DemuxerFilter::PauseTaskByTrackId(int32_t trackId)
{
    return demuxer_->PauseTaskByTrackId(trackId);
}

Status DemuxerFilter::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::SeekTo");
    MEDIA_LOG_D_SHORT("SeekTo in");
    return demuxer_->SeekTo(seekTime, mode, realSeekTime);
}

Status DemuxerFilter::StartTask(int32_t trackId)
{
    return demuxer_->StartTask(trackId);
}

Status DemuxerFilter::SelectTrack(int32_t trackId)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerFilter::SelectTrack");
    MEDIA_LOG_I_SHORT("SelectTrack called");
    return demuxer_->SelectTrack(trackId);
}

std::vector<std::shared_ptr<Meta>> DemuxerFilter::GetStreamMetaInfo() const
{
    return demuxer_->GetStreamMetaInfo();
}

std::shared_ptr<Meta> DemuxerFilter::GetGlobalMetaInfo() const
{
    return demuxer_->GetGlobalMetaInfo();
}

Status DemuxerFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    int32_t trackId = -1;
    if (!FindTrackId(outType, trackId)) {
        MEDIA_LOG_E_SHORT("FindTrackId failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::vector<std::shared_ptr<Meta>> trackInfos = demuxer_->GetStreamMetaInfo();
    std::shared_ptr<Meta> meta = trackInfos[trackId];
    for (MapIt iter = meta->begin(); iter != meta->end(); iter++) {
        MEDIA_LOG_D_SHORT("Link " PUBLIC_LOG_S, iter->first.c_str());
    }
    std::string mimeType;
    meta->GetData(Tag::MIME_TYPE, mimeType);
    MEDIA_LOG_I_SHORT("LinkNext mimeType " PUBLIC_LOG_S, mimeType.c_str());

    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    MEDIA_LOG_I_SHORT("LinkNext NextFilter FilterType " PUBLIC_LOG_D32, nextFilter_->GetFilterType());
    meta->SetData(Tag::REGULAR_TRACK_ID, trackId);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback
        = std::make_shared<DemuxerFilterLinkCallback>(shared_from_this());
    return nextFilter->OnLinked(outType, meta, filterLinkCallback);
}

Status DemuxerFilter::GetBitRates(std::vector<uint32_t>& bitRates)
{
    if (mediaSource_ == nullptr) {
        MEDIA_LOG_E_SHORT("GetBitRates failed, mediaSource = nullptr");
    }
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

Status DemuxerFilter::SelectBitRate(uint32_t bitRate)
{
    if (mediaSource_ == nullptr) {
        MEDIA_LOG_E_SHORT("SelectBitRate failed, mediaSource = nullptr");
    }
    return demuxer_->SelectBitRate(bitRate);
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

bool DemuxerFilter::FindStreamType(StreamType &streamType, MediaType mediaType, std::string mime, size_t index)
{
    MEDIA_LOG_I_SHORT("mediaType is %{public}d", static_cast<int32_t>(mediaType));
    if (mediaType == Plugins::MediaType::SUBTITLE) {
        streamType = StreamType::STREAMTYPE_SUBTITLE;
    } else if (mediaType == Plugins::MediaType::AUDIO) {
        if (mime == std::string(MimeType::AUDIO_RAW)) {
            streamType = StreamType::STREAMTYPE_RAW_AUDIO;
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
    if (trackId < 0) {
        return;
    }
    uint32_t trackIdU32 = static_cast<uint32_t>(trackId);
    int32_t decoderFramerateUpperLimit = 0;
    if (meta->GetData(Tag::VIDEO_DECODER_RATE_UPPER_LIMIT, decoderFramerateUpperLimit)) {
        demuxer_->SetDecoderFramerateUpperLimit(decoderFramerateUpperLimit, trackIdU32);
    }
    double framerate;
    if (meta->GetData(Tag::VIDEO_FRAME_RATE, framerate)) {
        demuxer_->SetFrameRate(framerate, trackIdU32);
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

void DemuxerFilter::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D_SHORT("DemuxerFilter::OnDumpInfo called.");
    if (demuxer_ != nullptr) {
        demuxer_->OnDumpInfo(fd);
    }
}

Status DemuxerFilter::DisableMediaTrack(Plugins::MediaType mediaType)
{
    disabledMediaTracks_.emplace(mediaType);
    return demuxer_->DisableMediaTrack(mediaType);
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
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
