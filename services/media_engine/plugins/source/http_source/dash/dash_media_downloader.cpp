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
#define HST_LOG_TAG "DashMediaDownloader"

#include <algorithm>
#include "dash_media_downloader.h"
#include "securec.h"
#include "plugin/plugin_time.h"
#include "osal/task/task.h"
#include "utils/time_utils.h"
#include "utils/bitrate_process_utils.h"
#include "format.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

constexpr double BUFFER_LOW_LIMIT  = 0.3;
constexpr double BYTE_TO_BIT = 8.0;
constexpr size_t RETRY_TIMES = 15000;
constexpr unsigned int SLEEP_TIME = 1;
constexpr uint32_t AUDIO_BUFFERING_FLAG = 1;
constexpr uint32_t VIDEO_BUFFERING_FLAG = 2;

DashMediaDownloader::DashMediaDownloader(std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
{
    if (sourceLoader != nullptr) {
        MEDIA_LOG_I("DashMediaDownloader app download.");
        sourceLoader_ = sourceLoader;
    }
    mpdDownloader_ = std::make_shared<DashMpdDownloader>(sourceLoader);
    mpdDownloader_->SetMpdCallback(this);
}

DashMediaDownloader::~DashMediaDownloader()
{
    if (mpdDownloader_ != nullptr) {
        mpdDownloader_->Close(false);
    }

    segmentDownloaders_.clear();
}

bool DashMediaDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    mpdDownloader_->Open(url);
    return true;
}

std::string DashMediaDownloader::GetContentType()
{
    FALSE_RETURN_V(mpdDownloader_ != nullptr, "");
    MEDIA_LOG_I("In");
    return mpdDownloader_->GetContentType();
}

void DashMediaDownloader::Close(bool isAsync)
{
    mpdDownloader_->Close(isAsync);
    for (unsigned int index = 0; index < segmentDownloaders_.size(); index++) {
        segmentDownloaders_[index]->Close(isAsync, true);
    }
}

void DashMediaDownloader::Pause()
{
    for (unsigned int index = 0; index < segmentDownloaders_.size(); index++) {
        segmentDownloaders_[index]->Pause();
    }
}

void DashMediaDownloader::Resume()
{
    for (unsigned int index = 0; index < segmentDownloaders_.size(); index++) {
        segmentDownloaders_[index]->Resume();
    }
}

Status DashMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V(buff != nullptr, Status::END_OF_STREAM);
    if (segmentDownloaders_.empty()) {
        MEDIA_LOG_W("dash read, segmentDownloaders size is 0");
        return Status::END_OF_STREAM;
    }

    if (downloadErrorState_) {
        if (callback_ != nullptr) {
            MEDIA_LOG_I("Read Client Error, OnEvent");
            callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
        }
        for (auto &segmentDownloader : segmentDownloaders_) {
            segmentDownloader->Close(false, true);
        }
        return Status::ERROR_AGAIN;
    }

    std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloader(readDataInfo.streamId_);
    if (segmentDownloader == nullptr) {
        MEDIA_LOG_E("GetSegmentDownloader failed when Read, streamId " PUBLIC_LOG_D32, readDataInfo.streamId_);
        return Status::END_OF_STREAM;
    }

    DashReadRet ret = segmentDownloader->Read(buff, readDataInfo, isInterruptNeeded_);
    MEDIA_LOG_D("Read:streamId " PUBLIC_LOG_D32 " readRet:" PUBLIC_LOG_D32, readDataInfo.streamId_, ret);
    if (ret == DASH_READ_END) {
        MEDIA_LOG_I("Read:streamId " PUBLIC_LOG_D32 " segment all finished end", readDataInfo.streamId_);
        readDataInfo.isEos_ = true;
        return Status::END_OF_STREAM;
    } else if (ret == DASH_READ_AGAIN) {
        return Status::ERROR_AGAIN;
    } else if (ret == DASH_READ_FAILED || ret == DASH_READ_INTERRUPT) {
        return Status::END_OF_STREAM;
    }
    return Status::OK;
}

std::shared_ptr<DashSegmentDownloader> DashMediaDownloader::GetSegmentDownloader(int32_t streamId)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = nullptr;
    std::shared_ptr<DashStreamDescription> streamDescription = mpdDownloader_->GetStreamByStreamId(streamId);
    if (streamDescription == nullptr) {
        MEDIA_LOG_E("stream " PUBLIC_LOG_D32 " not exist", streamId);
        return segmentDownloader;
    }
    return GetSegmentDownloaderByType(streamDescription->type_);
}

std::shared_ptr<DashSegmentDownloader> DashMediaDownloader::GetSegmentDownloaderByType(
    MediaAVCodec::MediaType type) const
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = nullptr;
    auto iter = std::find_if(segmentDownloaders_.begin(), segmentDownloaders_.end(),
        [&](const std::shared_ptr<DashSegmentDownloader> &downloader) {
            return downloader->GetStreamType() == type;
        });
    if (iter != segmentDownloaders_.end()) {
        segmentDownloader = *iter;
    }
    return segmentDownloader;
}

void DashMediaDownloader::UpdateDownloadFinished(int streamId)
{
    MEDIA_LOG_I("UpdateDownloadFinished: " PUBLIC_LOG_D32, streamId);
    std::shared_ptr<DashStreamDescription> streamDesc = mpdDownloader_->GetStreamByStreamId(streamId);
    if (streamDesc == nullptr) {
        MEDIA_LOG_E("UpdateDownloadFinished get stream null id: " PUBLIC_LOG_D32, streamId);
        return;
    }
    
    if (streamDesc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        VideoSegmentDownloadFinished(streamId);
        return;
    }

    std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloader(streamId);
    if (segmentDownloader == nullptr) {
        MEDIA_LOG_E("GetSegmentDownloader failed when UpdateDownloadFinished, streamId " PUBLIC_LOG_D32, streamId);
        return;
    }

    std::shared_ptr<DashSegment> seg;
    DashMpdGetRet getRet = mpdDownloader_->GetNextSegmentByStreamId(streamId, seg);
    MEDIA_LOG_I("GetNextSegmentByStreamId " PUBLIC_LOG_D32 ", ret=" PUBLIC_LOG_D32, streamId, getRet);
    if (seg != nullptr) {
        segmentDownloader->Open(seg);
    } else if (getRet == DASH_MPD_GET_FINISH) {
        segmentDownloader->SetAllSegmentFinished();
    }
}

void DashMediaDownloader::PostBufferingEvent(int streamId, BufferingInfoType type)
{
    if (callback_ == nullptr || mpdDownloader_ == nullptr) {
        MEDIA_LOG_I("PostBufferingEvent streamId: " PUBLIC_LOG_D32 " type: " PUBLIC_LOG_D32, streamId, type);
        return;
    }

    std::shared_ptr<DashStreamDescription> streamDesc = mpdDownloader_->GetStreamByStreamId(streamId);
    if (streamDesc == nullptr || streamDesc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
        return;
    }

    if (type == BufferingInfoType::BUFFERING_PERCENT) {
        uint32_t percent = 0;
        for (auto &downloader : segmentDownloaders_) {
            if (downloader == nullptr || downloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
                continue;
            }

            uint32_t segPercent = downloader->GetCachedPercent();
            percent = (percent == 0 || segPercent < percent) ? segPercent : percent;
        }
        if (percent > 0 && percent < BUFFERING_PERCENT_FULL && lastBufferingPercent_ != percent) {
            MEDIA_LOG_I("PostBufferingEvent buffering percent " PUBLIC_LOG_U32, percent);
            callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {percent}, "buffer percent"});
            lastBufferingPercent_ = percent;
        }
        return;
    }

    // ensure the order of the buffering_start and buffering_end, must use lock, this lock can not use in other scene
    // OnEvent can not block and lock other resource
    std::lock_guard<std::mutex> bufferingLock(bufferingMutex_);
    // audio or video buffering start will post event, audio and video buffering end will post event
    uint32_t flag =
        streamDesc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_VID ? VIDEO_BUFFERING_FLAG : AUDIO_BUFFERING_FLAG;
    if (type == BufferingInfoType::BUFFERING_START) {
        if (bufferingFlag_ == 0) {
            MEDIA_LOG_I("PostBufferingEvent buffering start");
            callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
            callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {0}, "buffer percent"});
        }
        bufferingFlag_ |= flag;
        return;
    } else if (type == BufferingInfoType::BUFFERING_END) {
        uint32_t lastBufferingFlag = bufferingFlag_;
        if ((bufferingFlag_ & flag) > 0) {
            bufferingFlag_ ^= flag;
        }
        if (lastBufferingFlag > 0 && bufferingFlag_ == 0) {
            MEDIA_LOG_I("PostBufferingEvent buffering end");
            callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {BUFFERING_PERCENT_FULL}, "buffer percent"});
            callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
        }
        return;
    }
}

bool DashMediaDownloader::SeekToTime(int64_t seekTime, SeekMode mode)
{
    MEDIA_LOG_I("DashMediaDownloader SeekToTime: " PUBLIC_LOG_D64, seekTime);
    SeekToTs(seekTime);
    return true;
}

size_t DashMediaDownloader::GetContentLength() const
{
    return 0;
}

int64_t DashMediaDownloader::GetDuration() const
{
    MEDIA_LOG_I("DashMediaDownloader GetDuration " PUBLIC_LOG_D64, mpdDownloader_->GetDuration());
    return mpdDownloader_->GetDuration();
}

Seekable DashMediaDownloader::GetSeekable() const
{
    MEDIA_LOG_I("DashMediaDownloader GetSeekable begin");
    Seekable value = mpdDownloader_->GetSeekable();
    if (value == Seekable::INVALID) {
        return value;
    }

    size_t times = 0;
    bool status = false;
    while (!status && times < RETRY_TIMES && !isInterruptNeeded_) {
        for (auto downloader : segmentDownloaders_) {
            status = downloader->GetStartedStatus();
            if (!status) {
                break;
            }
        }
        OSAL::SleepFor(SLEEP_TIME);
        times++;
    }

    if (times >= RETRY_TIMES || isInterruptNeeded_) {
        MEDIA_LOG_I("DashMediaDownloader GetSeekable INVALID");
        return Seekable::INVALID;
    }

    MEDIA_LOG_I("DashMediaDownloader GetSeekable end");
    return value;
}

void DashMediaDownloader::SetCallback(Callback* cb)
{
    callback_ = cb;
}

bool DashMediaDownloader::GetStartedStatus()
{
    return true;
}

void DashMediaDownloader::SetStatusCallback(StatusCallbackFunc cb)
{
    statusCallback_ = cb;
    mpdDownloader_->SetStatusCallback(cb);
}

std::vector<uint32_t> DashMediaDownloader::GetBitRates()
{
    return mpdDownloader_->GetBitRates();
}

bool DashMediaDownloader::SelectBitRate(uint32_t bitrate)
{
    std::lock_guard<std::mutex> sidxLock(parseSidxMutex_);
    MEDIA_LOG_I("Dash SelectBitRate bitrate:" PUBLIC_LOG_U32, bitrate);
    {
        isAutoSelectBitrate_ = false;

        std::lock_guard<std::mutex> lock(switchMutex_);
        // The bit rate is being switched. Wait until the sidx download and parsing are complete.
        if (bitrateParam_.waitSidxFinish_ ||
            trackParam_.waitSidxFinish_) {
            // Save the target stream information and update the downloaded stream information
            // when the callback indicating that the sidx parsing is complete is received.
            MEDIA_LOG_I("wait last switch bitrate or track:" PUBLIC_LOG_U32 " sidx parse finish, switch type:"
                PUBLIC_LOG_D32, bitrateParam_.bitrate_, (int) bitrateParam_.type_);
            preparedAction_.preparedBitrateParam_.bitrate_ = bitrate;
            preparedAction_.preparedBitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_SMOOTH;
            return true;
        }
        
        bitrateParam_.bitrate_ = bitrate;
        bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_SMOOTH;
        bitrateParam_.waitSidxFinish_ = true;
    }

    int64_t remainLastNumberSeq = -1;
    bool bufferCleanFlag = true;
    CleanVideoSegmentBuffer(bufferCleanFlag, remainLastNumberSeq);

    return SelectBitrateInternal(bufferCleanFlag, remainLastNumberSeq);
}

Status DashMediaDownloader::SelectStream(int32_t streamId)
{
    MEDIA_LOG_I("Dash SelectStream streamId:" PUBLIC_LOG_D32, streamId);
    std::shared_ptr<DashStreamDescription> streamDesc = mpdDownloader_->GetStreamByStreamId(streamId);
    if (streamDesc == nullptr) {
        MEDIA_LOG_W("Dash SelectStream can not find streamId");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (streamDesc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
        return SelectAudio(streamDesc);
    } else if (streamDesc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
        return SelectSubtitle(streamDesc);
    } else {
        SelectBitRate(streamDesc->bandwidth_);
        return Status::OK;
    }
}

void DashMediaDownloader::SeekToTs(int64_t seekTime)
{
    int64_t seekTimeMs;
    std::lock_guard<std::mutex> lock(parseSidxMutex_);
    {
        if (seekTime < 0 || seekTime > mpdDownloader_->GetDuration()) {
            return;
        }
        seekTimeMs = seekTime / MS_2_NS;
        if (preparedAction_.seekPosition_ != -1 ||
            bitrateParam_.waitSidxFinish_ ||
            trackParam_.waitSidxFinish_) {
            preparedAction_.seekPosition_ = seekTimeMs;
            MEDIA_LOG_I("SeekToTs:" PUBLIC_LOG_D64 ", wait sidx finish, bitrate:" PUBLIC_LOG_U32 ", type:"
                PUBLIC_LOG_D32, preparedAction_.seekPosition_, bitrateParam_.bitrate_, (int) bitrateParam_.type_);
            
            for (auto &segmentDownloader : segmentDownloaders_) {
                MEDIA_LOG_I("Dash clean streamId: " PUBLIC_LOG_D32, segmentDownloader->GetStreamId());
                int64_t remainLastNumberSeq = -1;
                segmentDownloader->CleanSegmentBuffer(true, remainLastNumberSeq);
            }

            return;
        }
    }
    
    SeekInternal(seekTimeMs);
}

void DashMediaDownloader::SetIsTriggerAutoMode(bool isAuto)
{
    isAutoSelectBitrate_ = isAuto;
}

void DashMediaDownloader::SetDownloadErrorState()
{
    MEDIA_LOG_I("Dash SetDownloadErrorState");
    downloadErrorState_ = true;
}

void DashMediaDownloader::SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy)
{
    if (playStrategy != nullptr) {
        mpdDownloader_->SetHdrStart(playStrategy->preferHDR);
        mpdDownloader_->SetInitResolution(playStrategy->width, playStrategy->height);
        mpdDownloader_->SetDefaultLang(playStrategy->audioLanguage, MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
        mpdDownloader_->SetDefaultLang(playStrategy->subtitleLanguage, MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE);
        expectDuration_ = static_cast<uint64_t>(playStrategy->duration);
        bufferDurationForPlaying_ = playStrategy->bufferDurationForPlaying;
    }
}

Status DashMediaDownloader::GetStreamInfo(std::vector<StreamInfo>& streams)
{
    GetSeekable();
    return mpdDownloader_->GetStreamInfo(streams);
}

void DashMediaDownloader::OnMpdInfoUpdate(DashMpdEvent mpdEvent)
{
    switch (mpdEvent) {
        case DASH_MPD_EVENT_STREAM_INIT:
            ReceiveMpdStreamInitEvent();
            break;
        case DASH_MPD_EVENT_PARSE_OK:
            ReceiveMpdParseOkEvent();
            break;
        default:
            break;
    }
}

void DashMediaDownloader::ReceiveMpdStreamInitEvent()
{
    MEDIA_LOG_I("Dash ReceiveMpdStreamInitEvent");
    std::vector<StreamInfo> streams;
    mpdDownloader_->GetStreamInfo(streams);
    std::shared_ptr<DashStreamDescription> streamDesc = nullptr;
    for (unsigned int index = 0; index < streams.size(); index++) {
        streamDesc = mpdDownloader_->GetStreamByStreamId(streams[index].streamId);
        if (streamDesc != nullptr && streamDesc->inUse_) {
            std::shared_ptr<DashSegment> seg = nullptr;
            if (breakpoint_ > 0) {
                mpdDownloader_->GetBreakPointSegment(streamDesc->streamId_, breakpoint_, seg);
            } else {
                mpdDownloader_->GetNextSegmentByStreamId(streamDesc->streamId_, seg);
            }
            
            if (seg == nullptr) {
                MEDIA_LOG_W("Dash get segment null in streamId " PUBLIC_LOG_D32, streamDesc->streamId_);
                continue;
            }

            OpenInitSegment(streamDesc, seg);
        }
    }
}

void DashMediaDownloader::OpenInitSegment(
    const std::shared_ptr<DashStreamDescription> &streamDesc, const std::shared_ptr<DashSegment> &seg)
{
    std::shared_ptr<DashSegmentDownloader> downloader = std::make_shared<DashSegmentDownloader>(
        callback_, streamDesc->streamId_, streamDesc->type_, expectDuration_, sourceLoader_);
    if (statusCallback_ != nullptr) {
        downloader->SetStatusCallback(statusCallback_);
    }
    auto doneCallback = [this] (int streamId) {
        UpdateDownloadFinished(streamId);
    };
    downloader->SetDownloadDoneCallback(doneCallback);
    auto bufferingCallback = [this] (int streamId, BufferingInfoType type) {
        PostBufferingEvent(streamId, type);
    };
    downloader->SetSegmentBufferingCallback(bufferingCallback);
    segmentDownloaders_.push_back(downloader);
    std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(
        streamDesc->streamId_);
    if (initSeg != nullptr) {
        downloader->SetInitSegment(initSeg);
    }
    downloader->SetDurationForPlaying(bufferDurationForPlaying_);
    downloader->Open(seg);
    MEDIA_LOG_I("dash first get segment in streamId " PUBLIC_LOG_D32 ", type "
        PUBLIC_LOG_D32, streamDesc->streamId_, streamDesc->type_);
}

void DashMediaDownloader::ReceiveMpdParseOkEvent()
{
    MEDIA_LOG_I("Dash ReceiveMpdParseOkEvent");
    int streamId = -1;
    {
        std::lock_guard<std::mutex> lock(parseSidxMutex_);
        if (bitrateParam_.waitSidxFinish_ ||
            trackParam_.waitSidxFinish_) {
            UpdateSegmentIndexAfterSidxParseOk();

            if (DoPreparedAction(streamId)) {
                MEDIA_LOG_I("Dash DoPreparedAction, no need download segment");
                return;
            }
        } else {
            MEDIA_LOG_I("switch type: " PUBLIC_LOG_D32 " or waitSidxFinish: "
                PUBLIC_LOG_D32 " is error ", bitrateParam_.waitSidxFinish_, bitrateParam_.type_);
            return;
        }
    }
    
    GetSegmentToDownload(streamId, true);
}

void DashMediaDownloader::VideoSegmentDownloadFinished(int streamId)
{
    MEDIA_LOG_I("VideoSegmentDownloadFinished streamId:" PUBLIC_LOG_D32 ", type:"
        PUBLIC_LOG_U32, streamId, bitrateParam_.type_);
    int downloadStreamId = streamId;
    {
        std::lock_guard<std::mutex> lock(switchMutex_);
        if (bitrateParam_.type_ != DASH_MPD_SWITCH_TYPE_NONE) {
            // no need to auto switch
            if (bitrateParam_.waitSegmentFinish_) {
                bitrateParam_.waitSegmentFinish_  = false;
            } else {
                MEDIA_LOG_I("old segment download finish, should get next segment in select bitrate");
                return;
            }

            if (bitrateParam_.waitSidxFinish_) {
                MEDIA_LOG_I("wait sidx download finish, should not get next segment");
                return;
            }
            
            downloadStreamId = bitrateParam_.streamId_;
            ResetBitrateParam();
        } else {
            // auto switch
            bool switchFlag = true;
            if (callback_ != nullptr) {
                switchFlag = callback_->CanAutoSelectBitRate();
            }
            std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloaderByType(
                MediaAVCodec::MediaType::MEDIA_TYPE_VID);
            if (segmentDownloader != nullptr && !segmentDownloader->IsAllSegmentFinished() &&
                switchFlag && isAutoSelectBitrate_) {
                bool flag = CheckAutoSelectBitrate(streamId);
                if (callback_ != nullptr) {
                    callback_->SetSelectBitRateFlag(flag, bitrateParam_.bitrate_);
                }
                if (flag) {
                    // switch success
                    return;
                }
            }
        }
    }

    GetSegmentToDownload(downloadStreamId, downloadStreamId != streamId);
}

void DashMediaDownloader::GetSegmentToDownload(int downloadStreamId, bool streamSwitchFlag)
{
    MEDIA_LOG_I("GetSegmentToDownload streamId: " PUBLIC_LOG_D32 ", streamSwitchFlag: "
        PUBLIC_LOG_D32, downloadStreamId, streamSwitchFlag);
    // segment list is ok and no segment is downloading in segmentDownloader, so get next segment to download
    std::shared_ptr<DashSegment> segment = nullptr;
    DashMpdGetRet ret = mpdDownloader_->GetNextSegmentByStreamId(downloadStreamId, segment);
    if (ret == DASH_MPD_GET_ERROR) {
        return;
    }

    std::shared_ptr<DashStreamDescription> stream = mpdDownloader_->GetStreamByStreamId(downloadStreamId);
    if (stream == nullptr) {
        MEDIA_LOG_E("GetSegmentToDownload streamId: " PUBLIC_LOG_D32 " get stream is null", downloadStreamId);
        return;
    }

    std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloaderByType(stream->type_);
    if (segmentDownloader == nullptr) {
        return;
    }

    if (streamSwitchFlag) {
        MEDIA_LOG_I("switch stream update streamId from " PUBLIC_LOG_D32 " to "
            PUBLIC_LOG_D32, segmentDownloader->GetStreamId(), downloadStreamId);
        segmentDownloader->UpdateStreamId(downloadStreamId);
        
        std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(downloadStreamId);
        if (initSeg != nullptr) {
            segmentDownloader->SetInitSegment(initSeg);
        }
    }
    
    if (segment != nullptr) {
        segmentDownloader->Open(segment);
    } else if (ret == DASH_MPD_GET_FINISH) {
        segmentDownloader->SetAllSegmentFinished();
    }
}

void DashMediaDownloader::CleanVideoSegmentBuffer(bool &bufferCleanFlag, int64_t &remainLastNumberSeq)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader;
    auto iter = std::find_if(segmentDownloaders_.begin(), segmentDownloaders_.end(),
        [&](const std::shared_ptr<DashSegmentDownloader> &downloader) {
            return downloader->GetStreamType() == MediaAVCodec::MEDIA_TYPE_VID;
        });
    if (iter != segmentDownloaders_.end()) {
        segmentDownloader = *iter;
    }

    if (segmentDownloader == nullptr) {
        MEDIA_LOG_W("Dash not start, can not SelectBitRate.");
        return;
    }

    remainLastNumberSeq = -1;
    bufferCleanFlag = true;
    // 1. clean segment buffer, get switch segment sequence. segment is in downloading or no segment buffer
    if (!segmentDownloader->CleanSegmentBuffer(false, remainLastNumberSeq) &&
        !segmentDownloader->IsSegmentFinish()) {
        MEDIA_LOG_I("Dash SelectBitRate no need clean buffer, wait current segment download finish");
        bufferCleanFlag = false;
    }
}

bool DashMediaDownloader::SelectBitrateInternal(bool bufferCleanFlag, int64_t remainLastNumberSeq)
{
    std::lock_guard<std::mutex> lock(switchMutex_);
    MEDIA_LOG_I("Dash SelectBitrateInternal remainLastNumberSeq:" PUBLIC_LOG_D64, remainLastNumberSeq);

    // 2. switch to destination bitrate
    bitrateParam_.position_ = remainLastNumberSeq; // update by segment sequence, -1 means no segment downloading

    if (!bufferCleanFlag) {
        bitrateParam_.waitSegmentFinish_ = true;
    } else {
        bitrateParam_.waitSegmentFinish_ = false;
    }

    DashMpdGetRet ret = mpdDownloader_->GetNextVideoStream(bitrateParam_, bitrateParam_.streamId_);
    if (ret == DASH_MPD_GET_ERROR) {
        MEDIA_LOG_W("Dash SelectBitRate Stream:" PUBLIC_LOG_D32 " GetNextVideoStream failed.", bitrateParam_.streamId_);
        return false;
    }

    if (ret == DASH_MPD_GET_UNDONE) {
        bitrateParam_.waitSidxFinish_ = true;
        MEDIA_LOG_I("Dash SelectBitRate wait sidx finish");
        return true;
    } else {
        bitrateParam_.waitSidxFinish_ = false;
    }

    if (bitrateParam_.waitSegmentFinish_) {
        std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloaderByType(
            MediaAVCodec::MediaType::MEDIA_TYPE_VID);
        if (segmentDownloader == nullptr) {
            MEDIA_LOG_W("SelectBitrateInternal can not get segmentDownloader.");
            return false;
        }

        if (!segmentDownloader->IsSegmentFinish()) {
            return true;
        }

        // old segment download finish, should get next segment
        bitrateParam_.waitSegmentFinish_ = false;
    }

    // 3. get dest segment and download
    bitrateParam_.bitrate_ = 0;
    bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_NONE;
    GetSegmentToDownload(bitrateParam_.streamId_, true);
    return true;
}

bool DashMediaDownloader::CheckAutoSelectBitrate(int streamId)
{
    MEDIA_LOG_I("AutoSelectBitrate streamId: " PUBLIC_LOG_D32, streamId);
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloaderByType(
        MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    if (segmentDownloader == nullptr) {
        MEDIA_LOG_W("AutoSelectBitrate can not get segmentDownloader.");
        return false;
    }
    uint32_t desBitrate = GetNextBitrate(segmentDownloader);
    if (desBitrate == 0) {
        return false;
    }
    return AutoSelectBitrateInternal(desBitrate);
}

uint32_t DashMediaDownloader::GetNextBitrate(std::shared_ptr<DashSegmentDownloader> segmentDownloader)
{
    std::shared_ptr<DashStreamDescription> stream = mpdDownloader_->GetUsingStreamByType(
        MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    if (stream == nullptr) {
        return 0;
    }

    if (stream->videoType_ != DASH_VIDEO_TYPE_SDR) {
        MEDIA_LOG_I("hdr stream no need to switch auto");
        return 0;
    }

    std::vector<uint32_t> bitRates =  mpdDownloader_->GetBitRatesByHdr(stream->videoType_ != DASH_VIDEO_TYPE_SDR);
    if (bitRates.size() == 0) {
        return 0;
    }
    uint32_t curBitrate = stream->bandwidth_;
    uint64_t downloadSpeed = static_cast<uint64_t>(segmentDownloader->GetDownloadSpeed());
    if (downloadSpeed == 0) {
        return 0;
    }
    uint32_t desBitrate = GetDesBitrate(bitRates, downloadSpeed);
    if (curBitrate == desBitrate) {
        return 0;
    }
    uint32_t bufferLowSize =
        static_cast<uint32_t>(static_cast<double>(curBitrate) / BYTE_TO_BIT * BUFFER_LOW_LIMIT);
    // switch to high bitrate,if buffersize less than lowsize, do not switch
    if (curBitrate < desBitrate && segmentDownloader->GetBufferSize()  < bufferLowSize) {
        MEDIA_LOG_I("AutoSelectBitrate curBitrate " PUBLIC_LOG_D32 ", desBitRate " PUBLIC_LOG_D32
            ", bufferLowSize " PUBLIC_LOG_D32, curBitrate, desBitrate, bufferLowSize);
        return 0;
    }
    // high size: buffersize * 0.8
    uint32_t bufferHighSize = segmentDownloader->GetRingBufferCapacity() * BUFFER_LIMIT_FACT;
    // switch to low bitrate, if buffersize more than highsize, do not switch
    if (curBitrate > desBitrate && segmentDownloader->GetBufferSize() > bufferHighSize) {
        MEDIA_LOG_I("AutoSelectBitrate curBitrate " PUBLIC_LOG_D32 ", desBitRate " PUBLIC_LOG_D32
            ", bufferHighSize " PUBLIC_LOG_D32, curBitrate, desBitrate, bufferHighSize);
        return 0;
    }
    return desBitrate;
}

bool DashMediaDownloader::AutoSelectBitrateInternal(uint32_t bitrate)
{
    bitrateParam_.position_ = -1;
    bitrateParam_.bitrate_ = bitrate;
    bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_AUTO;
    if (mpdDownloader_ == nullptr) {
        return false;
    }
    DashMpdGetRet ret = mpdDownloader_->GetNextVideoStream(bitrateParam_, bitrateParam_.streamId_);
    if (ret == DASH_MPD_GET_ERROR) {
        return false;
    }
    if (ret == DASH_MPD_GET_UNDONE) {
        bitrateParam_.waitSidxFinish_ = true;
        return true;
    }
    bitrateParam_.bitrate_ = bitrate;
    bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_NONE;
    GetSegmentToDownload(bitrateParam_.streamId_, true);
    return true;
}

bool DashMediaDownloader::IsSeekingInSwitch()
{
    std::lock_guard<std::mutex> lock(switchMutex_);
    bool isSwitching = false;
    if (bitrateParam_.type_ != DASH_MPD_SWITCH_TYPE_NONE) {
        MEDIA_LOG_I("IsSeekingInSwitch streamId:" PUBLIC_LOG_D32 ", switching bitrate:"
            PUBLIC_LOG_U32 ", type:" PUBLIC_LOG_D32, bitrateParam_.streamId_, bitrateParam_.bitrate_,
            (int) bitrateParam_.type_);
        int streamId = bitrateParam_.streamId_;
        std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloaderByType(
            MediaAVCodec::MediaType::MEDIA_TYPE_VID);
        if (segmentDownloader != nullptr && segmentDownloader->GetStreamId() != streamId) {
            segmentDownloader->UpdateStreamId(streamId);
        }

        ResetBitrateParam();
        isSwitching = true;
    }

    if (trackParam_.waitSidxFinish_) {
        MEDIA_LOG_I("IsSeekingInSwitch track streamId:" PUBLIC_LOG_D32 ", waitSidxFinish_:"
            PUBLIC_LOG_D32, trackParam_.streamId_, trackParam_.waitSidxFinish_);
        int streamId = trackParam_.streamId_;
        std::shared_ptr<DashStreamDescription> streamDesc = mpdDownloader_->GetStreamByStreamId(streamId);
        if (streamDesc != nullptr) {
            std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloaderByType(streamDesc->type_);
            if (segmentDownloader != nullptr && segmentDownloader->GetStreamId() != streamId) {
                segmentDownloader->UpdateStreamId(streamId);
            }
            ResetTrackParam();
            isSwitching = true;
        }
    }

    return isSwitching;
}

void DashMediaDownloader::HandleSeekReady(int32_t streamType, int32_t streamId, int64_t seekTimeMs, int32_t isEos)
{
    Format seekReadyInfo {};
    seekReadyInfo.PutIntValue("currentStreamType", streamType);
    seekReadyInfo.PutIntValue("currentStreamId", streamId);
    seekReadyInfo.PutLongValue("seekTime", seekTimeMs);
    seekReadyInfo.PutIntValue("isEOS", isEos);
    MEDIA_LOG_D("StreamType: " PUBLIC_LOG_D32 " StreamId: " PUBLIC_LOG_D32
        " seekTime: " PUBLIC_LOG_D64 " isEOS: " PUBLIC_LOG_D32, streamType, streamId, seekTimeMs, isEos);
    if (callback_ != nullptr) {
        MEDIA_LOG_D("Onevent dash seek ready");
        callback_->OnEvent({PluginEventType::DASH_SEEK_READY, seekReadyInfo, "dash_seek_ready"});
    }
}

int64_t DashMediaDownloader::GetVideoSeekTime(int64_t seekTimeMs)
{
    auto videoSegmentDownloader = GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    FALSE_RETURN_V_NOLOG(videoSegmentDownloader != nullptr, seekTimeMs);

    std::shared_ptr<DashSegment> segment;
    return mpdDownloader_->SeekToTs(videoSegmentDownloader->GetStreamId(), seekTimeMs, segment);
}

void DashMediaDownloader::SeekInternal(int64_t seekTimeMs)
{
    bool isSwitching = IsSeekingInSwitch();

    seekTimeMs = GetVideoSeekTime(seekTimeMs);
    for (auto &segmentDownloader : segmentDownloaders_) {
        std::shared_ptr<DashSegment> segment;
        int32_t streamId = static_cast<int32_t>(segmentDownloader->GetStreamId());
        mpdDownloader_->SeekToTs(segmentDownloader->GetStreamId(), seekTimeMs, segment);
        if (segment == nullptr) {
            MEDIA_LOG_I("Dash SeekToTs end streamId " PUBLIC_LOG_D32 ", type " PUBLIC_LOG_D32,
                segmentDownloader->GetStreamId(), segmentDownloader->GetStreamType());
            int64_t remainLastNumberSeq = -1;
            segmentDownloader->CleanSegmentBuffer(true, remainLastNumberSeq);
            segmentDownloader->SetAllSegmentFinished();
            HandleSeekReady(static_cast<int32_t>(segmentDownloader->GetStreamType()), streamId, seekTimeMs, 1);
            continue;
        }

        if (segmentDownloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_VID &&
            segmentDownloader->GetBufferSize() == 0 && seekTimeMs == 0) {
            segmentDownloader->NotifyInitSuccess();
        }
        MEDIA_LOG_D("Dash SeekToTs segment " PUBLIC_LOG_D64 ", duration:"
            PUBLIC_LOG_U32, segment->numberSeq_, segment->duration_);
        std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(
            segmentDownloader->GetStreamId());
        if (!isSwitching && segmentDownloader->SeekToTime(segment, streamId)) {
            MEDIA_LOG_I("Dash SeekToTs of buffered streamId " PUBLIC_LOG_D32 ", type " PUBLIC_LOG_D32,
                segmentDownloader->GetStreamId(), segmentDownloader->GetStreamType());
            segmentDownloader->SetInitSegment(initSeg, true);
        } else {
            int64_t remainLastNumberSeq = -1;
            segmentDownloader->CleanSegmentBuffer(true, remainLastNumberSeq);
            mpdDownloader_->SetCurrentNumberSeqByStreamId(segmentDownloader->GetStreamId(), segment->numberSeq_);
            segmentDownloader->SetInitSegment(initSeg, true);
            segmentDownloader->Open(segment);
        }
        HandleSeekReady(static_cast<int32_t>(segmentDownloader->GetStreamType()), streamId, seekTimeMs, 0);
    }
}

Status DashMediaDownloader::SelectAudio(const std::shared_ptr<DashStreamDescription> &streamDesc)
{
    std::lock_guard<std::mutex> lock(parseSidxMutex_);
    // stream track is being switched. Wait until the sidx download and parsing are complete.
    if (bitrateParam_.waitSidxFinish_ ||
        trackParam_.waitSidxFinish_) {
        MEDIA_LOG_I("last bitrate switch: " PUBLIC_LOG_D32 " last track switch:"
            PUBLIC_LOG_D32, bitrateParam_.waitSidxFinish_, trackParam_.waitSidxFinish_);
        preparedAction_.preparedAudioParam_.streamId_ = streamDesc->streamId_;
        return Status::OK;
    }
    return SelectAudioInternal(streamDesc);
}

Status DashMediaDownloader::SelectAudioInternal(const std::shared_ptr<DashStreamDescription> &streamDesc)
{
    MEDIA_LOG_I("SelectAudioInternal Stream:" PUBLIC_LOG_D32, streamDesc->streamId_);
    std::shared_ptr<DashSegmentDownloader> downloader = GetSegmentDownloaderByType(streamDesc->type_);
    if (downloader == nullptr) {
        return Status::ERROR_UNKNOWN;
    }

    int64_t remainLastNumberSeq = -1;
    bool isEnd = false;
    
    // 1. clean segment buffer keep 1000 ms, get switch segment sequence and is segment receive finish flag.
    downloader->CleanBufferByTime(remainLastNumberSeq, isEnd);

    std::lock_guard<std::mutex> lock(switchMutex_);
    // 2. switch to destination stream
    trackParam_.isEnd_ = isEnd;
    trackParam_.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    trackParam_.streamId_ = streamDesc->streamId_;
    trackParam_.position_ = remainLastNumberSeq; // update by segment sequence, -1 means no segment downloading

    DashMpdGetRet ret = mpdDownloader_->GetNextTrackStream(trackParam_);
    if (ret == DASH_MPD_GET_ERROR) {
        MEDIA_LOG_W("Dash SelectAudio Stream:" PUBLIC_LOG_D32 " failed.", trackParam_.streamId_);
        return Status::ERROR_UNKNOWN;
    }
    
    if (ret == DASH_MPD_GET_UNDONE) {
        trackParam_.waitSidxFinish_ = true;
        MEDIA_LOG_I("Dash SelectAudio wait sidx finish");
        return Status::OK;
    }
    
    // 3. get dest segment and download
    GetSegmentToDownload(trackParam_.streamId_, true);
    ResetTrackParam();
    return Status::OK;
}

Status DashMediaDownloader::SelectSubtitle(const std::shared_ptr<DashStreamDescription> &streamDesc)
{
    std::lock_guard<std::mutex> lock(parseSidxMutex_);
    // stream track is being switched. Wait until the sidx download and parsing are complete.
    if (bitrateParam_.waitSidxFinish_ ||
        trackParam_.waitSidxFinish_) {
        MEDIA_LOG_I("last bitrate switch: " PUBLIC_LOG_D32 " last track switch:"
            PUBLIC_LOG_D32, bitrateParam_.waitSidxFinish_, trackParam_.waitSidxFinish_);
        preparedAction_.preparedSubtitleParam_.streamId_ = streamDesc->streamId_;
        return Status::OK;
    }
    return SelectSubtitleInternal(streamDesc);
}

Status DashMediaDownloader::SelectSubtitleInternal(const std::shared_ptr<DashStreamDescription> &streamDesc)
{
    MEDIA_LOG_I("SelectSubtitleInternal Stream:" PUBLIC_LOG_D32, streamDesc->streamId_);
    std::shared_ptr<DashSegmentDownloader> downloader = GetSegmentDownloaderByType(streamDesc->type_);
    if (downloader == nullptr) {
        return Status::ERROR_UNKNOWN;
    }

    int64_t remainLastNumberSeq = -1;
    
    // 1. clean all segment buffer, get switch segment sequence and is segment receive finish flag.
    downloader->CleanSegmentBuffer(true, remainLastNumberSeq);

    std::lock_guard<std::mutex> lock(switchMutex_);
    // 2. switch to destination stream
    trackParam_.isEnd_ = false;
    trackParam_.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE;
    trackParam_.streamId_ = streamDesc->streamId_;
    trackParam_.position_ = remainLastNumberSeq; // update by segment sequence, -1 means no segment downloading

    DashMpdGetRet ret = mpdDownloader_->GetNextTrackStream(trackParam_);
    if (ret == DASH_MPD_GET_ERROR) {
        MEDIA_LOG_W("Dash SelectSubtitle Stream:" PUBLIC_LOG_D32 " failed.", trackParam_.streamId_);
        return Status::ERROR_UNKNOWN;
    }
    
    if (ret == DASH_MPD_GET_UNDONE) {
        trackParam_.waitSidxFinish_ = true;
        MEDIA_LOG_I("Dash SelectSubtitle wait sidx finish");
        return Status::OK;
    }
    
    // 3. get dest segment and download
    GetSegmentToDownload(trackParam_.streamId_, true);
    ResetTrackParam();
    return Status::OK;
}

bool DashMediaDownloader::DoPreparedSwitchBitrate(bool switchBitrateOk, bool &needDownload, int &streamId)
{
    bool needSwitchBitrate = false;
    {
        std::lock_guard<std::mutex> lock(switchMutex_);
        if (switchBitrateOk) {
            if (!bitrateParam_.waitSegmentFinish_) {
                ResetBitrateParam();
            } else {
                bitrateParam_.waitSidxFinish_ = false;
                // wait segment download finish, no need to download video segment
                MEDIA_LOG_I("SwitchBitrate sidx ok and need wait segment finish");
                needDownload = false;
            }
        } else {
            ResetTrackParam();
        }

        if (preparedAction_.preparedBitrateParam_.type_ != DASH_MPD_SWITCH_TYPE_NONE) {
            ResetBitrateParam();
            needSwitchBitrate = true;
            bitrateParam_.bitrate_ = preparedAction_.preparedBitrateParam_.bitrate_;
            bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_SMOOTH;
            // keep wait sidx finish flag, avoid to reset bitrateParam after video segment download finish
            bitrateParam_.waitSidxFinish_ = true;
            preparedAction_.preparedBitrateParam_.bitrate_ = 0;
            preparedAction_.preparedBitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_NONE;
        }
    }
    
    if (needSwitchBitrate) {
        int64_t remainLastNumberSeq = -1;
        bool bufferCleanFlag = true;
        CleanVideoSegmentBuffer(bufferCleanFlag, remainLastNumberSeq);
        MEDIA_LOG_I("PreparedSwitchBitrate: " PUBLIC_LOG_U32, bitrateParam_.bitrate_);
        return SelectBitrateInternal(bufferCleanFlag, remainLastNumberSeq);
    }

    return false;
}

bool DashMediaDownloader::DoPreparedSwitchAudio(int &streamId)
{
    if (preparedAction_.preparedAudioParam_.streamId_ != -1) {
        ResetTrackParam();
        std::shared_ptr<DashStreamDescription> streamDesc =
            mpdDownloader_->GetStreamByStreamId(preparedAction_.preparedAudioParam_.streamId_);
        MEDIA_LOG_I("PreparedSwitchAudio id:" PUBLIC_LOG_D32, preparedAction_.preparedAudioParam_.streamId_);
        preparedAction_.preparedAudioParam_.streamId_ = -1;
        if (streamDesc != nullptr && SelectAudioInternal(streamDesc) == Status::OK) {
            return true;
        }
    }

    return false;
}

bool DashMediaDownloader::DoPreparedSwitchSubtitle(int &streamId)
{
    if (preparedAction_.preparedSubtitleParam_.streamId_ != -1) {
        ResetTrackParam();
        std::shared_ptr<DashStreamDescription> streamDesc =
            mpdDownloader_->GetStreamByStreamId(preparedAction_.preparedSubtitleParam_.streamId_);
        MEDIA_LOG_I("PreparedSwitchSubtitle id:" PUBLIC_LOG_D32, preparedAction_.preparedSubtitleParam_.streamId_);
        preparedAction_.preparedSubtitleParam_.streamId_ = -1;

        if (streamDesc != nullptr && SelectSubtitleInternal(streamDesc) == Status::OK) {
            return true;
        }
    }

    return false;
}

bool DashMediaDownloader::DoPreparedSwitchAction(bool switchBitrateOk,
    bool switchAudioOk, bool switchSubtitleOk, int &streamId)
{
    bool segmentNeedDownload = true;
    // first should check switch bitrate
    if (DoPreparedSwitchBitrate(switchBitrateOk, segmentNeedDownload, streamId)) {
        // previous action is switch bitrate, no need to get segment when do prepare switch bitrate
        segmentNeedDownload = !switchBitrateOk;
        if (bitrateParam_.type_ != DASH_MPD_SWITCH_TYPE_NONE && bitrateParam_.waitSidxFinish_) {
            MEDIA_LOG_I("DoPreparedAction switch bitrate wait sidx finish:" PUBLIC_LOG_U32, bitrateParam_.bitrate_);
            return switchBitrateOk;
        }
    }

    if (DoPreparedSwitchAudio(streamId)) {
        // previous action is switch audio, no need to get segment when do prepare switch audio
        segmentNeedDownload = switchAudioOk ? false : segmentNeedDownload;
        if (trackParam_.waitSidxFinish_) {
            MEDIA_LOG_I("DoPreparedAction switch audio wait sidx finish:" PUBLIC_LOG_D32, trackParam_.streamId_);
            return switchAudioOk ? true : !segmentNeedDownload;
        }
    }

    if (DoPreparedSwitchSubtitle(streamId)) {
        // previous action is switch subtitle, no need to get segment when do prepare switch subtitle
        segmentNeedDownload = switchSubtitleOk ? false : segmentNeedDownload;
        if (trackParam_.waitSidxFinish_) {
            MEDIA_LOG_I("DoPreparedAction switch subtitle wait sidx finish:" PUBLIC_LOG_D32, trackParam_.streamId_);
            return switchSubtitleOk ? true : !segmentNeedDownload;
        }
    }

    int64_t seekPosition = -1;
    if (preparedAction_.seekPosition_ != -1) {
        seekPosition = preparedAction_.seekPosition_;
        preparedAction_.seekPosition_ = -1;
    }
    
    if (seekPosition > -1) {
        SeekInternal(seekPosition); // seek after switch ok
        return true;
    }

    return !segmentNeedDownload;
}

bool DashMediaDownloader::DoPreparedAction(int &streamId)
{
    bool switchBitrateOk = bitrateParam_.waitSidxFinish_;
    bool switchAudioOk = (trackParam_.waitSidxFinish_ && trackParam_.type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
    bool switchSubtitleOk =
        (trackParam_.waitSidxFinish_ && trackParam_.type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE);

    streamId = switchBitrateOk ? bitrateParam_.streamId_ : trackParam_.streamId_;
    return DoPreparedSwitchAction(switchBitrateOk, switchAudioOk, switchSubtitleOk, streamId);
}

void DashMediaDownloader::UpdateSegmentIndexAfterSidxParseOk()
{
    std::lock_guard<std::mutex> lock(switchMutex_);
    if (bitrateParam_.waitSidxFinish_ && bitrateParam_.nextSegTime_ > 0) {
        mpdDownloader_->UpdateCurrentNumberSeqByTime(mpdDownloader_->GetStreamByStreamId(bitrateParam_.streamId_),
            bitrateParam_.nextSegTime_);
        bitrateParam_.nextSegTime_ = 0;
    } else if (trackParam_.waitSidxFinish_ && trackParam_.nextSegTime_ > 0) {
        mpdDownloader_->UpdateCurrentNumberSeqByTime(mpdDownloader_->GetStreamByStreamId(trackParam_.streamId_),
            trackParam_.nextSegTime_);
        trackParam_.nextSegTime_ = 0;
    }
}

void DashMediaDownloader::ResetBitrateParam()
{
    bitrateParam_.bitrate_ = 0;
    bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_NONE;
    bitrateParam_.streamId_ = -1;
    bitrateParam_.position_ = -1;
    bitrateParam_.nextSegTime_ = 0;
    bitrateParam_.waitSegmentFinish_ = false;
    bitrateParam_.waitSidxFinish_ = false;
}

void DashMediaDownloader::ResetTrackParam()
{
    trackParam_.waitSidxFinish_ = false;
    trackParam_.isEnd_ = false;
    trackParam_.streamId_ = -1;
    trackParam_.position_ = -1;
    trackParam_.nextSegTime_ = 0;
}

void DashMediaDownloader::OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos)
{
    if (callback_ != nullptr) {
        callback_->OnEvent({PluginEventType::SOURCE_DRM_INFO_UPDATE, {drmInfos}, "drm_info_update"});
    }
}

void DashMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    mpdDownloader_->SetInterruptState(isInterruptNeeded);
    for (unsigned int index = 0; index < segmentDownloaders_.size(); index++) {
        segmentDownloaders_[index]->SetInterruptState(isInterruptNeeded);
    }
}

Status DashMediaDownloader::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    MEDIA_LOG_I("SetCurrentBitRate stream: " PUBLIC_LOG_D32 " biteRate: " PUBLIC_LOG_D32, streamID, bitRate);
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloader(streamID);
    if (segmentDownloader != nullptr) {
        segmentDownloader->SetCurrentBitRate(bitRate);
    }
    return Status::OK;
}

void DashMediaDownloader::SetDemuxerState(int32_t streamId)
{
    MEDIA_LOG_I("SetDemuxerState streamId: " PUBLIC_LOG_D32, streamId);
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloader(streamId);
    if (segmentDownloader != nullptr) {
        segmentDownloader->SetDemuxerState();
    }
}

void DashMediaDownloader::GetPlaybackInfo(PlaybackInfo& playbackInfo)
{
    if (segmentDownloaders_.empty()) {
        return ;
    }
    if (segmentDownloaders_[0] != nullptr) {
        segmentDownloaders_[0]->GetIp(playbackInfo.serverIpAddress);
    }
    bool DownloadFinishStateTmp = true;
    playbackInfo.averageDownloadRate = 0;
    for (size_t i = 0; i < segmentDownloaders_.size(); i++) {
        if (playbackInfo.averageDownloadRate < static_cast<int64_t>(segmentDownloaders_[i]->GetDownloadSpeed())) {
            playbackInfo.averageDownloadRate = static_cast<int64_t>(segmentDownloaders_[i]->GetDownloadSpeed());
            std::pair<int64_t, int64_t> recordData = segmentDownloaders_[i]->GetDownloadRecordData();
            playbackInfo.downloadRate = recordData.first;
            playbackInfo.bufferDuration = recordData.second;
        }
        DownloadFinishStateTmp = (DownloadFinishStateTmp && segmentDownloaders_[i]->GetDownloadFinishState());
    }
    playbackInfo.isDownloading = DownloadFinishStateTmp ? false : true;
}

uint64_t DashMediaDownloader::GetBufferSize() const
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader =
        GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    if (segmentDownloader == nullptr) {
        MEDIA_LOG_W("GetBufferSize can not get segmentDownloader.");
        return 0;
    }
    return segmentDownloader->GetBufferSize();
}

bool DashMediaDownloader::GetPlayable()
{
    std::shared_ptr<DashSegmentDownloader> vidSegmentDownloader =
        GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    std::shared_ptr<DashSegmentDownloader> audSegmentDownloader =
        GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
    if (vidSegmentDownloader != nullptr && audSegmentDownloader != nullptr) {
        return !vidSegmentDownloader->GetBufferringStatus() && !audSegmentDownloader->GetBufferringStatus();
    } else if (vidSegmentDownloader != nullptr) {
        return !vidSegmentDownloader->GetBufferringStatus();
    } else if (audSegmentDownloader != nullptr) {
        return !audSegmentDownloader->GetBufferringStatus();
    } else {
        MEDIA_LOG_E("GetPlayable error.");
        return false;
    }
}

void DashMediaDownloader::SetAppUid(int32_t appUid)
{
    for (size_t i = 0; i < segmentDownloaders_.size(); i++) {
        segmentDownloaders_[i]->SetAppUid(appUid);
    }
}

bool DashMediaDownloader::GetBufferingTimeOut()
{
    return false;
}

void DashMediaDownloader::NotifyInitSuccess()
{
    for (size_t i = 0; i < segmentDownloaders_.size(); i++) {
        if (segmentDownloaders_[i] != nullptr) {
            segmentDownloaders_[i]->NotifyInitSuccess();
        }
    }
}

uint64_t DashMediaDownloader::GetMemorySize()
{
    uint64_t memorySize = 0;
    for (size_t i = 0; i < segmentDownloaders_.size(); i++) {
        if (segmentDownloaders_[i] != nullptr) {
            auto streamType = segmentDownloaders_[i]->GetStreamType();
            memorySize += static_cast<uint64_t>(segmentDownloaders_[i]->GetRingBufferInitSize(streamType));
        }
    }
    return memorySize;
}

Status DashMediaDownloader::StopBufferring(bool isAppBackground)
{
    MEDIA_LOG_I("DashMediaDownloader:StopBufferring enter");
    for (size_t index = 0; index < segmentDownloaders_.size(); index++) {
        FALSE_RETURN_V(segmentDownloaders_[index] != nullptr, Status::ERROR_NULL_POINTER);
        segmentDownloaders_[index]->StopBufferring(isAppBackground);
    }
    return Status::OK;
}
}
}
}
}
