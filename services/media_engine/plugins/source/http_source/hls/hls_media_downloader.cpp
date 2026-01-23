/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#define HST_LOG_TAG "HlsMediaDownloader"

#include <algorithm>
#include <arpa/inet.h>
#include <netdb.h>
#include <regex>
#include <utility>
#include "hls_media_downloader.h"
#include "media_downloader.h"
#include "common/media_core.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

namespace {
    constexpr uint32_t AUDIO_BUFFERING_FLAG = 1;
    constexpr uint32_t VIDEO_BUFFERING_FLAG = 2;
    constexpr uint32_t SUBTITLES_BUFFERING_FLAG = 4;
}

//   hls manifest, m3u8 --- content get from m3u8 url, we get play list from the content
//   fragment --- one item in play list, download media data according to the fragment address.
HlsMediaDownloader::HlsMediaDownloader(int expectBufferDuration, bool userDefinedDuration,
    const std::map<std::string, std::string>& httpHeader,
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
{
    videoSegManager_ = std::make_shared<HlsSegmentManager>(
        expectBufferDuration, userDefinedDuration, httpHeader, HlsSegmentType::SEG_VIDEO, sourceLoader);
    downloadMetricsInfo_ = std::make_shared<DownloadMetricsInfo>();
    if (videoSegManager_ != nullptr && downloadMetricsInfo_ != nullptr) {
        videoSegManager_->SetDownloadCallback(downloadMetricsInfo_);
    }
    videoSegManager_->Init();
}

HlsMediaDownloader::HlsMediaDownloader(std::string mimeType,
    const std::map<std::string, std::string>& httpHeader)
{
    videoSegManager_ = std::make_shared<HlsSegmentManager>(mimeType, HlsSegmentType::SEG_VIDEO, httpHeader);
    downloadMetricsInfo_ = std::make_shared<DownloadMetricsInfo>();
    if (videoSegManager_ != nullptr && downloadMetricsInfo_ != nullptr) {
        videoSegManager_->SetDownloadCallback(downloadMetricsInfo_);
    }
    videoSegManager_->Init();
}

void HlsMediaDownloader::Init()
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "Init no video segment manager found!");
    auto weakDownloader = weak_from_this();
    auto callback = [weakDownloader] (bool needAudioManager, bool needSubtitlesManager) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "masterReadyCb, Hls Media Downloader already destoryed.");
        shareDownloader->OnMasterReady(needAudioManager, needSubtitlesManager);
    };
    videoSegManager_->SetMasterReadyCallback(callback);
    auto bufferingCallback = [weakDownloader] (HlsSegmentType mediaType, BufferingInfoType type) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "bufferingEventCb, Hls Media Downloader already destoryed.");
        shareDownloader->PostBufferingEvent(mediaType, type);
    };
    videoSegManager_->SetSegmentBufferingCallback(bufferingCallback);
    auto segEventCallback = [weakDownloader] (HlsSegEvent event) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "allEventCb, Hls Media Downloader already destoryed.");
        shareDownloader->PostAllEvent(event);
    };
    videoSegManager_->SetSegmentAllCallback(segEventCallback);
}

void HlsMediaDownloader::OnMasterReady(bool needAudioManager, bool needSubtitlesManager)
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "OnMasterReady no video segment manager found!");
    MEDIA_LOG_I("HlsMediaDownloader OnMasterReady Audio: %{public}d, Subtitles: %{public}d", needAudioManager,
        needSubtitlesManager);
    if (needAudioManager && !audioSegManager_) {
        MEDIA_LOG_I("HlsMediaDownloader Audio OnMasterReady: 0x%{public}06" PRIXPTR, FAKE_POINTER(this));
        audioSegManager_ = std::make_shared<HlsSegmentManager>(videoSegManager_, HlsSegmentType::SEG_AUDIO);
        audioSegManager_->Init();
        audioSegManager_->Clone(videoSegManager_);
        auto audioDefaultStreamId = videoSegManager_->GetDefaultMediaStreamId(HlsSegmentType::SEG_AUDIO);
        audioSegManager_->StartMediaDownload(audioDefaultStreamId, HlsSegmentType::SEG_AUDIO);
    }
    if (needSubtitlesManager && !subtitlesSegManager_) {
        MEDIA_LOG_I("HlsMediaDownloader Subtitles OnMasterReady: 0x%{public}06" PRIXPTR, FAKE_POINTER(this));
        subtitlesSegManager_ = std::make_shared<HlsSegmentManager>(videoSegManager_, HlsSegmentType::SEG_SUBTITLE);
        subtitlesSegManager_->Init();
        subtitlesSegManager_->Clone(videoSegManager_);
        auto subtitlesDefaultStreamId = videoSegManager_->GetDefaultMediaStreamId(HlsSegmentType::SEG_SUBTITLE);
        subtitlesSegManager_->StartMediaDownload(subtitlesDefaultStreamId, HlsSegmentType::SEG_SUBTITLE);
    }
}

HlsMediaDownloader::~HlsMediaDownloader()
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " ~HlsMediaDownloader dtor in", FAKE_POINTER(this));
    videoSegManager_ = nullptr;
    audioSegManager_ = nullptr;
    subtitlesSegManager_ = nullptr;
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " ~HlsMediaDownloader dtor out", FAKE_POINTER(this));
}

std::string HlsMediaDownloader::GetContentType()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, "", "GetContentType no video segment manager found!");
    return videoSegManager_->GetContentType();
}

bool HlsMediaDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, false, "hls media downloader open failed, no video seg manager!");
    videoSegManager_->Open(url, httpHeader);
    return true;
}

void HlsMediaDownloader::Close(bool isAsync)
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " HLS Close enter", FAKE_POINTER(this));
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "hls media downloader open failed, no video seg manager!");
    videoSegManager_->Close(isAsync);

    if (audioSegManager_ != nullptr) {
        audioSegManager_->Close(isAsync);
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->Close(isAsync);
    }
}

void HlsMediaDownloader::Pause()
{
    MEDIA_LOG_I("HLS Pause enter");
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "hls media downloader pause failed, no video seg manager!");
    videoSegManager_->Pause();

    if (audioSegManager_ != nullptr) {
        audioSegManager_->Pause();
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->Pause();
    }
}

void HlsMediaDownloader::Resume()
{
    MEDIA_LOG_I("HLS Resume enter");
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "hls media downloader resume failed, no video seg manager!");
    videoSegManager_->Resume();

    if (audioSegManager_ != nullptr) {
        audioSegManager_->Resume();
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->Resume();
    }
}

Status HlsMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    auto segManager = GetSegmentManager(readDataInfo.streamId_);
    FALSE_RETURN_V_MSG(segManager != nullptr, Status::ERROR_AGAIN, "Read no segment manager found!");
    return segManager->Read(buff, readDataInfo);
}

bool HlsMediaDownloader::SeekToTime(int64_t seekTime, SeekMode mode)
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, false, "SeekToTime no video segment manager found!");
    auto ret = videoSegManager_->SeekToTime(seekTime, mode);
    FALSE_RETURN_V_MSG(ret != false, ret, "video segment manager seek failed!");
    if (audioSegManager_ != nullptr) {
        ret = audioSegManager_->SeekToTime(seekTime, mode);
    }
    FALSE_RETURN_V_MSG(ret != false, ret, "audio segment manager seek failed!");
    if (subtitlesSegManager_ != nullptr) {
        ret = subtitlesSegManager_->SeekToTime(seekTime, mode);
    }
    return ret;
}

size_t HlsMediaDownloader::GetContentLength() const
{
    return 0;
}

int64_t HlsMediaDownloader::GetDuration() const
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, 0, "GetDuration no video segment manager found!");
    return videoSegManager_->GetDuration();
}

std::pair<int64_t, bool> HlsMediaDownloader::GetStartInfo() const
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, std::make_pair(0, false),
        "GetStartInfo no video segment manager found!");
    return videoSegManager_->GetStartInfo();
}

Seekable HlsMediaDownloader::GetSeekable() const
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, Seekable::INVALID, "GetSeekable no video segment manager found!");
    return videoSegManager_->GetSeekable();
}

void HlsMediaDownloader::SetCallback(Callback* cb)
{
    callback_ = cb;
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "SetCallback no video segment manager found!");
    videoSegManager_->SetCallback(cb);
    FALSE_RETURN(audioSegManager_ != nullptr);
    audioSegManager_->SetCallback(cb);
    FALSE_RETURN(subtitlesSegManager_ != nullptr);
    subtitlesSegManager_->SetCallback(cb);
}

bool HlsMediaDownloader::GetStartedStatus()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, false, "GetStartedStatus no video segment manager found!");
    auto isVideoStarted = videoSegManager_->GetStartedStatus();
    if (audioSegManager_ != nullptr) {
        isVideoStarted = isVideoStarted && audioSegManager_->GetStartedStatus();
    }
    if (subtitlesSegManager_ != nullptr) {
        isVideoStarted = isVideoStarted && subtitlesSegManager_->GetStartedStatus();
    }
    return isVideoStarted;
}

void HlsMediaDownloader::SetStatusCallback(StatusCallbackFunc cb)
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "SetStatusCallback no video segment manager found!");
    videoSegManager_->SetStatusCallback(cb);
    if (audioSegManager_ != nullptr) {
        audioSegManager_->SetStatusCallback(cb);
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->SetStatusCallback(cb);
    }
}

std::vector<uint32_t> HlsMediaDownloader::GetBitRates()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, std::vector<uint32_t>(),
        "GetBitRates no video segment manager found!");
    return videoSegManager_->GetBitRates();
}

bool HlsMediaDownloader::SelectBitRate(uint32_t bitRate)
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, false, "SelectBitRate no video segment manager found!");
    return videoSegManager_->SelectBitRate(bitRate);
}

void HlsMediaDownloader::SetReadBlockingFlag(bool isReadBlockingAllowed)
{
    MEDIA_LOG_D("SetReadBlockingFlag entered");
}

void HlsMediaDownloader::SetIsTriggerAutoMode(bool isAuto)
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "SetIsTriggerAutoMode no video segment manager found!");
    videoSegManager_->SetIsTriggerAutoMode(isAuto);
}

void HlsMediaDownloader::SetDemuxerState(int32_t streamId)
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "SetDemuxerState no video segment manager found!");
    videoSegManager_->SetDemuxerState(streamId);
    if (audioSegManager_ != nullptr) {
        audioSegManager_->SetDemuxerState(streamId);
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->SetDemuxerState(streamId);
    }
}

void HlsMediaDownloader::SetDownloadErrorState()
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "SetDownloadErrorState no video segment manager found!");
    videoSegManager_->SetDownloadErrorState();
    if (audioSegManager_ != nullptr) {
        audioSegManager_->SetDownloadErrorState();
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->SetDownloadErrorState();
    }
}

void HlsMediaDownloader::AutoSelectBitrate(uint32_t bitRate)
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "AutoSelectBitrate no video segment manager found!");
    videoSegManager_->AutoSelectBitrate(bitRate);
}

void HlsMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "SetInterruptState no video segment manager found!");
    videoSegManager_->SetInterruptState(isInterruptNeeded);
    if (audioSegManager_ != nullptr) {
        audioSegManager_->SetInterruptState(isInterruptNeeded);
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->SetInterruptState(isInterruptNeeded);
    }
}

void HlsMediaDownloader::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "GetDownloadInfo no video segment manager found!");
    videoSegManager_->GetDownloadInfo(downloadInfo);
    if (downloadMetricsInfo_ != nullptr) {
        downloadInfo.totalDownLoadBytes = downloadMetricsInfo_->GetTotalTotalDownLoadBytes();
        downloadInfo.totalLoadingTime = downloadMetricsInfo_->GetTotalTotalDownloadTime();
        downloadInfo.loadingCount = downloadMetricsInfo_->GetTotalDownloadCount();
        downloadInfo.firstDownloadTime = downloadMetricsInfo_->GetTotalFirstDownloadTime();
        downloadInfo.firstFrameDecapsulationTime = downloadMetricsInfo_->GetTotalFirstDownloadTimestamp();
    }
}

void HlsMediaDownloader::GetPlaybackInfo(PlaybackInfo& playbackInfo)
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "GetPlaybackInfo no video segment manager found!");
    return videoSegManager_->GetPlaybackInfo(playbackInfo);
}

std::pair<int32_t, int32_t> HlsMediaDownloader::GetDownloadInfo()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, std::make_pair(0, 0),
        "GetDownloadInfo pair no video segment manager found!");
    return videoSegManager_->GetDownloadInfo();
}

Status HlsMediaDownloader::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, Status::ERROR_UNKNOWN,
        "SetCurrentBitRate no video segment manager found!");
    auto ret = videoSegManager_->SetCurrentBitRate(bitRate, streamID);
    return ret;
}

void HlsMediaDownloader::SetAppUid(int32_t appUid)
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "SetAppUid no video segment manager found!");
    videoSegManager_->SetAppUid(appUid);
    if (audioSegManager_ != nullptr) {
        audioSegManager_->SetAppUid(appUid);
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->SetAppUid(appUid);
    }
}

uint64_t HlsMediaDownloader::GetBufferSize() const
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, 0, "GetBufferSize no video segment manager found!");
    return videoSegManager_->GetBufferSize();
}

bool HlsMediaDownloader::GetPlayable()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, false, "GetPlayable no video segment manager found!");
    return videoSegManager_->GetPlayable();
}

bool HlsMediaDownloader::GetBufferingTimeOut()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, true, "GetBufferingTimeOut no video segment manager found!");
    return videoSegManager_->GetBufferingTimeOut();
}

bool HlsMediaDownloader::GetReadTimeOut(bool isDelay)
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, true, "GetReadTimeOut no video segment manager found!");
    return videoSegManager_->GetReadTimeOut(isDelay);
}

size_t HlsMediaDownloader::GetSegmentOffset()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, 0, "GetSegmentOffset no video segment manager found!");
    return videoSegManager_->GetSegmentOffset();
}

bool HlsMediaDownloader::GetHLSDiscontinuity()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, false, "GetHLSDiscontinuity no video segment manager found!");
    auto ret = videoSegManager_->GetHLSDiscontinuity();
    FALSE_RETURN_V_MSG(ret != true, ret, "video segment manager GetHLSDiscontinuity true!");
    if (audioSegManager_ != nullptr) {
        ret = audioSegManager_->GetHLSDiscontinuity();
    }
    FALSE_RETURN_V_MSG(ret != true, ret, "audio segment manager GetHLSDiscontinuity true!");
    if (subtitlesSegManager_ != nullptr) {
        ret = subtitlesSegManager_->GetHLSDiscontinuity();
    }
    return ret;
}

Status HlsMediaDownloader::StopBufferring(bool isAppBackground)
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, Status::ERROR_UNKNOWN,
        "StopBufferring no video segment manager found!");
    auto ret = videoSegManager_->StopBufferring(isAppBackground);
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "video segment manager StopBufferring failed!");
    if (audioSegManager_ != nullptr) {
        ret = audioSegManager_->StopBufferring(isAppBackground);
    }
    FALSE_RETURN_V_MSG(ret == Status::OK, ret, "audio segment manager StopBufferring failed!");
    if (subtitlesSegManager_ != nullptr) {
        ret = subtitlesSegManager_->StopBufferring(isAppBackground);
    }
    return ret;
}

void HlsMediaDownloader::WaitForBufferingEnd()
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "WaitForBufferingEnd no video segment manager found!");
    return videoSegManager_->WaitForBufferingEnd();
}

void HlsMediaDownloader::SetIsReportedErrorCode()
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "SetIsReportedErrorCode no video segment manager found!");
    videoSegManager_->SetIsReportedErrorCode();
    if (audioSegManager_ != nullptr) {
        audioSegManager_->SetIsReportedErrorCode();
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->SetIsReportedErrorCode();
    }
}

bool HlsMediaDownloader::SetInitialBufferSize(int32_t offset, int32_t size)
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, false, "SetInitialBufferSize no video segment manager found!");
    auto ret = videoSegManager_->SetInitialBufferSize(offset, size);
    FALSE_RETURN_V_MSG(ret != false, ret, "video segment manager SetInitialBufferSize failed!");
    if (audioSegManager_ != nullptr) {
        ret = audioSegManager_->SetInitialBufferSize(offset, size);
    }
    FALSE_RETURN_V_MSG(ret != false, ret, "audio segment manager SetInitialBufferSize failed!");
    if (subtitlesSegManager_ != nullptr) {
        ret = subtitlesSegManager_->SetInitialBufferSize(offset, size);
    }
    return ret;
}

void HlsMediaDownloader::SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy)
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "SetPlayStrategy no video segment manager found!");
    videoSegManager_->SetPlayStrategy(playStrategy);
    if (audioSegManager_ != nullptr) {
        audioSegManager_->SetPlayStrategy(playStrategy);
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->SetPlayStrategy(playStrategy);
    }
}

void HlsMediaDownloader::NotifyInitSuccess()
{
    FALSE_RETURN_MSG(videoSegManager_ != nullptr, "NotifyInitSuccess no video segment manager found!");
    videoSegManager_->NotifyInitSuccess();
    if (audioSegManager_ != nullptr) {
        audioSegManager_->NotifyInitSuccess();
    }
    if (subtitlesSegManager_ != nullptr) {
        subtitlesSegManager_->NotifyInitSuccess();
    }
}

uint64_t HlsMediaDownloader::GetCachedDuration()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, 0, "GetCachedDuration no video segment manager found!");
    return videoSegManager_->GetCachedDuration();
}

Status HlsMediaDownloader::GetStreamInfo(std::vector<StreamInfo>& streams)
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, Status::ERROR_UNKNOWN,
        "GetStreamInfo no video segment manager found!");
    return videoSegManager_->GetStreamInfo(streams);
}

bool HlsMediaDownloader::IsHlsFmp4()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, false, "IsHlsFmp4 no video segment manager found!");
    return videoSegManager_->IsHlsFmp4();
}

uint64_t HlsMediaDownloader::GetMemorySize()
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, 0, "GetMemorySize no video segment manager found!");
    return videoSegManager_->GetMemorySize();
}

bool HlsMediaDownloader::IsHlsEnd(int32_t streamId)
{
    auto segManager = GetSegmentManager(streamId);
    FALSE_RETURN_V_MSG(segManager != nullptr, false, "IsHlsEnd no segment manager found!");
    return segManager->IsHlsEnd();
}

std::shared_ptr<HlsSegmentManager> HlsMediaDownloader::GetSegmentManager(uint32_t streamId)
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, nullptr, "GetSegmentManager no video segment manager found!");
    auto segType = videoSegManager_->GetSegType(streamId);
    switch (segType) {
        case HlsSegmentType::SEG_AUDIO:
            return audioSegManager_;
        case HlsSegmentType::SEG_SUBTITLE:
            return subtitlesSegManager_;
        case HlsSegmentType::SEG_VIDEO:
        default:
            return videoSegManager_;
    }
}

Status HlsMediaDownloader::SelectStream(int32_t streamId)
{
    FALSE_RETURN_V_MSG(videoSegManager_ != nullptr, Status::ERROR_UNKNOWN,
        "SelectStream no video segment manager found!");
    MEDIA_LOG_I("HLS SelectStream streamId:" PUBLIC_LOG_D32, streamId);
    auto segType = videoSegManager_->GetSegType(streamId);
    if (segType == HlsSegmentType::SEG_VIDEO) {
        std::shared_ptr<StreamInfo> streamInfo = videoSegManager_->GetStreamInfoById(streamId);
        if (streamInfo == nullptr) {
            MEDIA_LOG_W("HLS SelectStream can not find streamId");
            return Status::ERROR_INVALID_PARAMETER;
        }
        SelectBitRate(streamInfo->bitRate);
        if (audioSegManager_ != nullptr) {
            auto defaultAudioStreamId = videoSegManager_->GetDefaultMediaStreamId(HlsSegmentType::SEG_AUDIO);
            audioSegManager_->SelectMedia(defaultAudioStreamId, HlsSegmentType::SEG_AUDIO);
        }
        if (subtitlesSegManager_ != nullptr) {
            auto defaultSubtitlesStreamId = videoSegManager_->GetDefaultMediaStreamId(HlsSegmentType::SEG_SUBTITLE);
            subtitlesSegManager_->SelectMedia(defaultSubtitlesStreamId, HlsSegmentType::SEG_SUBTITLE);
        }
        return Status::OK;
    } else if (segType == HlsSegmentType::SEG_AUDIO) {
        FALSE_RETURN_V_MSG(audioSegManager_ != nullptr, Status::ERROR_UNKNOWN,
            "SelectStream no audio segment manager found!");
        audioSegManager_->SelectMedia(streamId, HlsSegmentType::SEG_AUDIO);
        return Status::OK;
    } else if (segType == HlsSegmentType::SEG_SUBTITLE) {
        FALSE_RETURN_V_MSG(subtitlesSegManager_ != nullptr, Status::ERROR_UNKNOWN,
            "SelectStream no subtitles segment manager found!");
        subtitlesSegManager_->SelectMedia(streamId, HlsSegmentType::SEG_SUBTITLE);
        return Status::OK;
    } else {
        return Status::ERROR_INVALID_PARAMETER;
    }
}

void HlsMediaDownloader::PostAllEvent(HlsSegEvent event)
{
    FALSE_RETURN_MSG(callback_, "PostAllEvent no callback, %{public}d", event.type);
    MEDIA_LOG_I("PostAllEvent: %{public}d, msg: %{public}s", event.type, event.str.c_str());
    switch (event.type) {
        case PluginEventType::CLIENT_ERROR:
            callback_->OnEvent({event.type, event.networkError, event.str});
            break;
        case PluginEventType::INITIAL_BUFFER_SUCCESS:
            callback_->OnEvent({event.type, event.bufferType, event.str});
            break;
        case PluginEventType::SOURCE_DRM_INFO_UPDATE:
            callback_->OnEvent({event.type, event.drmInfos, event.str});
            break;
        case PluginEventType::VIDEO_SIZE_CHANGE:
            callback_->OnEvent({event.type, event.videoSize, event.str});
            break;
        case PluginEventType::SOURCE_BITRATE_START:
            callback_->OnEvent({event.type, event.bitRate, event.str});
            break;
        case PluginEventType::CACHED_DURATION:
            callback_->OnEvent({event.type, event.cachedDuration, event.str});
            break;
        case PluginEventType::EVENT_BUFFER_PROGRESS:
            if (event.segType == HlsSegmentType::SEG_VIDEO) {
                callback_->OnEvent({event.type, event.percent, event.str});
            }
            break;
        case PluginEventType::HLS_SEEK_READY:
            callback_->OnEvent({event.type, event.seekReadyInfo, event.str});
            break;
        default:
            break;
    }
}

void HlsMediaDownloader::PostBufferingEvent(HlsSegmentType mediaType, BufferingInfoType type)
{
    FALSE_RETURN_MSG(callback_, "PostBufferingEvent no callback");
    std::lock_guard<std::mutex> bufferingLock(bufferingMutex_);
    uint32_t flag;
    switch (mediaType) {
        case HlsSegmentType::SEG_VIDEO:
            flag = VIDEO_BUFFERING_FLAG;
            break;
        case HlsSegmentType::SEG_AUDIO:
            flag = AUDIO_BUFFERING_FLAG;
            break;
        case HlsSegmentType::SEG_SUBTITLE:
            flag = SUBTITLES_BUFFERING_FLAG;
            break;
        default:
            MEDIA_LOG_D("PostBufferingEvent unknown media type");
            return;
    }
    MEDIA_LOG_I("PostBufferingEvent flag: %{public}u, start: %{public}d, end: %{public}d, type: %{public}d",
        bufferingFlag_, type == BufferingInfoType::BUFFERING_START, type == BufferingInfoType::BUFFERING_END, type);
    if (type == BufferingInfoType::BUFFERING_START) {
        if (bufferingFlag_ == 0) {
            MEDIA_LOG_I("PostBufferingEvent buffering start");
            callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
        }
        bufferingFlag_ |= flag;
    } else if (type == BufferingInfoType::BUFFERING_END) {
        uint32_t lastBufferingFlag = bufferingFlag_;
        if ((bufferingFlag_ & flag) > 0) {
            bufferingFlag_ ^= flag;
        }
        if (lastBufferingFlag > 0 && bufferingFlag_ == 0) {
            MEDIA_LOG_I("PostBufferingEvent buffering end");
            callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
        }
    } else {
        MEDIA_LOG_D("PostBufferingEvent unknown type");
    }
}
}
}
}
}