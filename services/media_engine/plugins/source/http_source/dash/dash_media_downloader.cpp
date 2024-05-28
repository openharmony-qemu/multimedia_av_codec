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
#include "openssl/aes.h"
#include "osal/task/task.h"
#include "utils/time_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

DashMediaDownloader::DashMediaDownloader() noexcept
{
    mpdDownloader_ = std::make_shared<DashMpdDownloader>();
    mpdDownloader_->SetMpdCallback(this);
}

DashMediaDownloader::~DashMediaDownloader()
{
    Close(false);
    segmentDownloaders_.clear();
    mpdDownloader_ = nullptr;
}

bool DashMediaDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    mpdDownloader_->Open(url);
    return true;
}

void DashMediaDownloader::Close(bool isAsync)
{
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

bool DashMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V(buff != nullptr, false);
    if (segmentDownloaders_.empty()) {
        MEDIA_LOG_W("dash read, segmentDownloaders size is 0");
        return false;
    }

    if (downloadErrorState_) {
        if (callback_ != nullptr) {
            MEDIA_LOG_I("Read Client Error, OnEvent");
            callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
        }
        for (auto &segmentDownloader : segmentDownloaders_) {
            segmentDownloader->Close(false, true);
        }
        return false;
    }

    std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloader(readDataInfo.streamId_);
    if (segmentDownloader == nullptr) {
        MEDIA_LOG_E("GetSegmentDownloader failed when Read, streamId " PUBLIC_LOG_D32, readDataInfo.streamId_);
        return false;
    }

    DashReadRet ret = segmentDownloader->Read(readDataInfo.streamId_, buff, readDataInfo.wantReadLength_,
                                              readDataInfo.realReadLength_, readDataInfo.nextStreamId_);
    if (ret == DASH_READ_END && mpdDownloader_->IsAllSegmentFinishedByStreamId(readDataInfo.streamId_)) {
        MEDIA_LOG_I("Read:streamId " PUBLIC_LOG_D32 " segment all finished end", readDataInfo.streamId_);
        readDataInfo.isEos_ = true;
    } else if (ret == DASH_READ_TIMEOUT) {
        if (callback_ != nullptr) {
            MEDIA_LOG_I("Read time out, OnEvent");
            callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
        }
        return false;
    }
    return true;
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

std::shared_ptr<DashSegmentDownloader> DashMediaDownloader::GetSegmentDownloaderByType(MediaAVCodec::MediaType type)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = nullptr;
    for (auto &downloader : segmentDownloaders_) {
        if (downloader->GetStreamType() == type) {
            segmentDownloader = downloader;
            break;
        }
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
    }
}

bool DashMediaDownloader::SeekToTime(int64_t seekTime, SeekMode mode)
{
    MEDIA_LOG_I("SeekToTime: " PUBLIC_LOG_D64, seekTime);
    SeekToTs(seekTime);
    return true;
}

size_t DashMediaDownloader::GetContentLength() const
{
    return 0;
}

int64_t DashMediaDownloader::GetDuration() const
{
    MEDIA_LOG_I("GetDuration " PUBLIC_LOG_D64, mpdDownloader_->GetDuration());
    return mpdDownloader_->GetDuration();
}

Seekable DashMediaDownloader::GetSeekable() const
{
    MEDIA_LOG_I("GetSeekable begin");
    Seekable value = mpdDownloader_->GetSeekable();
    bool status = false;
    while (!status) {
        for (auto downloader : segmentDownloaders_) {
            status = downloader->GetStartedStatus();
            if (!status) {
                break;
            }
        }
        OSAL::SleepFor(1);
    }
    MEDIA_LOG_I("GetSeekable end");
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
    std::lock_guard<std::mutex> lock(switchMutex_);
    MEDIA_LOG_I("SelectBitRate bitrate " PUBLIC_LOG_U32, bitrate);
    if (mpdDownloader_->IsBitrateSame(bitrate)) {
        MEDIA_LOG_W("SelectBitRate is same bitrate.");
        return true;
    }

    // The bit rate is being switched. Wait until the sidx download and parsing are complete.
    if (bitrateParam_.waitSidxFinish_) {
        // Save the target stream information and update the downloaded stream information
        // when the callback indicating that the sidx parsing is complete is received.
        MEDIA_LOG_I("wait last switch bitrate:"
        PUBLIC_LOG_U32
        " sidx parse finish, switch type:"
        PUBLIC_LOG_D32, bitrateParam_.bitrate_, (int) bitrateParam_.type_);
        preparedAction_.preparedBitrateParam_.bitrate_ = bitrate;
        preparedAction_.preparedBitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_SMOOTH;
        return true;
    }
    return SelectBitrateInternal(bitrate);
}

void DashMediaDownloader::SeekToTs(int64_t seekTime)
{
    std::lock_guard<std::mutex> lock(switchMutex_);
    MEDIA_LOG_I("SeekToTs seekTime:" PUBLIC_LOG_D64, seekTime);
    if (seekTime < 0 || seekTime >= mpdDownloader_->GetDuration()) {
        return;
    }

    int64_t seekTimeMs = seekTime / MS_2_NS;
    if (preparedAction_.seekPosition_ != -1 ||
        (bitrateParam_.waitSidxFinish_ && bitrateParam_.type_ != DASH_MPD_SWITCH_TYPE_NONE)) {
        preparedAction_.seekPosition_ = seekTimeMs;
        MEDIA_LOG_I("SeekToTs:"
        PUBLIC_LOG_D64
        ", wait sidx finish,bitrate:"
        PUBLIC_LOG_U32
        ", type:"
        PUBLIC_LOG_D32, preparedAction_.seekPosition_, bitrateParam_.bitrate_, (int) bitrateParam_.type_);
        return;
    }

    SeekInternal(seekTimeMs);
}

void DashMediaDownloader::SetIsTriggerAutoMode(bool isAuto)
{
    isAutoSelectBitrate_ = isAuto;
}

void DashMediaDownloader::SetDownloadErrorState()
{
    MEDIA_LOG_I("SetDownloadErrorState");
    downloadErrorState_ = true;
}

void DashMediaDownloader::SetPlayStrategy(PlayStrategy* playStrategy)
{
    if (playStrategy != nullptr) {
        mpdDownloader_->SetHdrStart(playStrategy->preferHDR);
        expectDuration_ = static_cast<uint64_t>(playStrategy->duration);
    }
}

Status DashMediaDownloader::GetStreamInfo(std::vector<StreamInfo>& streams)
{
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
    MEDIA_LOG_I("ReceiveMpdStreamInitEvent");
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
                MEDIA_LOG_W("get segment null in streamId " PUBLIC_LOG_D32, streamDesc->streamId_);
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
            streamDesc->streamId_, streamDesc->type_, expectDuration_);
    if (downloader != nullptr) {
        if (statusCallback_ != nullptr) {
            downloader->SetStatusCallback(statusCallback_);
        }
        auto doneCallback = [this] (int streamId) {
            UpdateDownloadFinished(streamId);
        };
        downloader->SetDownloadDoneCallback(doneCallback);
        segmentDownloaders_.push_back(downloader);
        std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(
            streamDesc->streamId_);
        if (initSeg != nullptr) {
            downloader->SetInitSegment(initSeg);
        }
        downloader->Open(seg);
        MEDIA_LOG_I("dash first get segment in streamId "
        PUBLIC_LOG_D32
        ", type "
        PUBLIC_LOG_D32
        ", url:"
        PUBLIC_LOG_S, streamDesc->streamId_, streamDesc->type_, seg->url_.c_str());
    }
}

void DashMediaDownloader::ReceiveMpdParseOkEvent()
{
    MEDIA_LOG_I("ReceiveMpdParseOkEvent");
    int streamId = -1;
    {
        std::lock_guard<std::mutex> lock(switchMutex_);
        if (bitrateParam_.waitSidxFinish_ && bitrateParam_.type_ != DASH_MPD_SWITCH_TYPE_NONE) {
            bitrateParam_.waitSidxFinish_ = false;
            
            if (DoPreparedAction()) {
                // The prepared action is being processed.
                // The segment corresponding to the last switchover does not need to be downloaded.
                MEDIA_LOG_I("DoPreparedAction, just return");
                return;
            }

            // Whether to download segments depends on the process of switching streams.
            // If the cache is waiting for segment download, the segments cannot be directly downloaded.
            if (bitrateParam_.waitSegmentFinish_) {
                MEDIA_LOG_I("wait segment download finish, should not get next segment");
                return;
            }
            
            streamId = bitrateParam_.streamId_;
            ResetBitrateParam();
        } else {
            MEDIA_LOG_I("switch type: "
            PUBLIC_LOG_D32
            " or waitSidxFinish: "
            PUBLIC_LOG_D32
            " is error ", bitrateParam_.waitSidxFinish_, bitrateParam_.type_);
            return;
        }
    }
    
    GetSegmentToDownload(streamId, true);
}

void DashMediaDownloader::VideoSegmentDownloadFinished(int streamId)
{
    MEDIA_LOG_I("VideoSegmentDownloadFinished streamId:"
    PUBLIC_LOG_D32
    ", type:"
    PUBLIC_LOG_U32, streamId, bitrateParam_.type_);
    int downloadStreamId = streamId;
    {
        std::lock_guard<std::mutex> lock(switchMutex_);
        if (bitrateParam_.type_ != DASH_MPD_SWITCH_TYPE_NONE) {
            // no need to auto switch
            if (bitrateParam_.waitSegmentFinish_) {
                bitrateParam_.waitSegmentFinish_  = false;
            }
            if (bitrateParam_.waitSidxFinish_) {
                MEDIA_LOG_I("wait sidx download finish, should not get next segment");
                return;
            }
            // reset bitrateParam_
            bitrateParam_.bitrate_ = 0;
            bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_NONE;
            downloadStreamId = bitrateParam_.streamId_;
            bitrateParam_.streamId_ = -1;
        } else {
            // auto switch
        }
    }

    GetSegmentToDownload(downloadStreamId, downloadStreamId != streamId);
}

void DashMediaDownloader::GetSegmentToDownload(int downloadStreamId, bool streamSwitchFlag)
{
    MEDIA_LOG_I("GetSegmentToDownload streamId: "
    PUBLIC_LOG_D32
    ", streamSwitchFlag: "
    PUBLIC_LOG_D32, downloadStreamId, streamSwitchFlag);
    // segment list is ok and no segment is downloading in segmentDownloader, so get next segment to download
    std::shared_ptr<DashSegment> segment = nullptr;
    DashMpdGetRet ret = mpdDownloader_->GetNextSegmentByStreamId(downloadStreamId, segment);
    if (ret == DASH_MPD_GET_ERROR) {
        return;
    }

    std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloaderByType(
        MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    if (segmentDownloader == nullptr) {
        return;
    }

    if (streamSwitchFlag) {
        MEDIA_LOG_I("switch bitrate update streamId from "
        PUBLIC_LOG_D32
        " to "
        PUBLIC_LOG_D32, segmentDownloader->GetStreamId(), downloadStreamId);
        segmentDownloader->UpdateStreamId(downloadStreamId);
        
        std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(downloadStreamId);
        if (initSeg != nullptr) {
            segmentDownloader->SetInitSegment(initSeg);
        }
    }
    
    if (segment != nullptr) {
        segmentDownloader->Open(segment);
    }
}

bool DashMediaDownloader::SelectBitrateInternal(uint32_t bitrate)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader;
    for (auto &downloader : segmentDownloaders_) {
        if (downloader->GetStreamType() == MediaAVCodec::MEDIA_TYPE_VID) {
            segmentDownloader = downloader;
            break;
        }
    }

    if (segmentDownloader == nullptr) {
        MEDIA_LOG_W("Dash not start, can not SelectBitRate.");
        return false;
    }

    int64_t remainLastNumberSeq = -1;
    bool bufferCleanFlag = true;
    // 1. clean segment buffer, get switch segment sequence. segment is in downloading or no segment buffer
    if (!segmentDownloader->CleanSegmentBuffer(false, remainLastNumberSeq)) {
        MEDIA_LOG_I("Dash SelectBitRate no need clean buffer, wait current segment download finish");
        bufferCleanFlag = false;
    }

    // 2. switch to destination bitrate
    bitrateParam_.position_ = remainLastNumberSeq; // update by segment sequence, -1 means no segment downloading
    bitrateParam_.bitrate_ = bitrate;
    bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_SMOOTH;
    if (!bufferCleanFlag && !segmentDownloader->IsSegmentFinish()) {
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
    }

    if (bitrateParam_.waitSegmentFinish_) {
        return true;
    }
    
    // 3. get dest segment and download
    bitrateParam_.bitrate_ = 0;
    bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_NONE;
    GetSegmentToDownload(bitrateParam_.streamId_, true);
    return true;
}

void DashMediaDownloader::SeekInternal(int64_t seekTimeMs)
{
    bool isSwitching = false;
    if (bitrateParam_.waitSegmentFinish_ && bitrateParam_.type_ != DASH_MPD_SWITCH_TYPE_NONE) {
        MEDIA_LOG_I("SeekInternal streamId:"
        PUBLIC_LOG_D32
        ", do not wait segment finish, bitrate:"
        PUBLIC_LOG_U32
        ", type:"
        PUBLIC_LOG_D32, bitrateParam_.streamId_, bitrateParam_.bitrate_, (int) bitrateParam_.type_);
        int streamId = bitrateParam_.streamId_;
        std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloaderByType(
            MediaAVCodec::MediaType::MEDIA_TYPE_VID);
        if (segmentDownloader != nullptr && segmentDownloader->GetStreamId() != streamId) {
            segmentDownloader->UpdateStreamId(streamId);
        }

        ResetBitrateParam();
        isSwitching = true;
    }

    for (auto &segmentDownloader : segmentDownloaders_) {
        std::shared_ptr<DashSegment> segment;
        mpdDownloader_->SeekToTs(segmentDownloader->GetStreamId(), seekTimeMs, segment);
        if (segment == nullptr) {
            continue;
        }

        MEDIA_LOG_D("Dash SeekToTs segment "
        PUBLIC_LOG_D64
        ", duration:"
        PUBLIC_LOG_U32, segment->numberSeq_, segment->duration_);
        std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(
            segmentDownloader->GetStreamId());
        if (!isSwitching && segmentDownloader->SeekToTime(segment)) {
            MEDIA_LOG_I("Dash SeekToTs of buffered streamId " PUBLIC_LOG_D32 ", type " PUBLIC_LOG_D32,
                        segmentDownloader->GetStreamId(), segmentDownloader->GetStreamType());
            segmentDownloader->SetInitSegment(initSeg);
            continue;
        } else {
            int64_t remainLastNumberSeq = -1;
            segmentDownloader->CleanSegmentBuffer(true, remainLastNumberSeq);
            mpdDownloader_->SetCurrentNumberSeqByStreamId(segmentDownloader->GetStreamId(), segment->numberSeq_);
            segmentDownloader->SetInitSegment(initSeg);
            segmentDownloader->Open(segment);
        }
    }
}

bool DashMediaDownloader::DoPreparedAction()
{
    bool doPrepared = false;
    if (preparedAction_.preparedBitrateParam_.type_ != DASH_MPD_SWITCH_TYPE_NONE) {
        doPrepared = true;
        ResetBitrateParam();
        SelectBitrateInternal(preparedAction_.preparedBitrateParam_.bitrate_);
        preparedAction_.preparedBitrateParam_.bitrate_ = 0;
        preparedAction_.preparedBitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_NONE;
        if (bitrateParam_.type_ != DASH_MPD_SWITCH_TYPE_NONE && bitrateParam_.waitSidxFinish_) {
            MEDIA_LOG_I("DoPreparedAction switch bitrate wait sidx finish:" PUBLIC_LOG_U32, bitrateParam_.bitrate_);
            return doPrepared;
        }
    }

    if (preparedAction_.seekPosition_ != -1) {
        doPrepared = true;
        // seek after switch ok
        int64_t seekPosition = preparedAction_.seekPosition_;
        preparedAction_.seekPosition_ = -1;
        SeekInternal(seekPosition);
    }

    return doPrepared;
}

void DashMediaDownloader::ResetBitrateParam()
{
    bitrateParam_.bitrate_ = 0;
    bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_NONE;
    bitrateParam_.streamId_ = -1;
    bitrateParam_.waitSegmentFinish_ = false;
    bitrateParam_.waitSidxFinish_ = false;
}

void DashMediaDownloader::OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos)
{
    if (callback_ != nullptr) {
        callback_->OnEvent({PluginEventType::SOURCE_DRM_INFO_UPDATE, {drmInfos}, "drm_info_update"});
    }
}

void DashMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    return;
}
}
}
}
}