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
    
    //mpdDownloader_->Close();
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

bool DashMediaDownloader::Read(int32_t streamId, unsigned char* buff, unsigned int wantReadLength,
                               unsigned int& realReadLength, int32_t& realStreamId, bool& isEos)
{
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

    std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloader(streamId);
    if (segmentDownloader == nullptr) {
        MEDIA_LOG_E("GetSegmentDownloader failed when Read, streamId " PUBLIC_LOG_D32, streamId);
        return false;
    }

    DashReadRet ret = segmentDownloader->Read(streamId, buff, wantReadLength, realReadLength, realStreamId);
    if (ret == DASH_READ_END && mpdDownloader_->IsAllSegmentFinishedByStreamId(streamId)) {
        MEDIA_LOG_I("Read:streamId " PUBLIC_LOG_D32 " segment all finished end", streamId);
        isEos = true;
    } else if (ret == DASH_READ_TIMEOUT) {
        if (callback_ != nullptr) {
            MEDIA_LOG_I("Read time out, OnEvent");
            callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
        }
        return false;
    }
    return true;
}

std::shared_ptr<DashSegmentDownloader> DashMediaDownloader::GetSegmentDownloader(int32_t streamId) {
    std::shared_ptr<DashSegmentDownloader> segmentDownloader;
    std::shared_ptr<DashStreamDescription> streamDescription = mpdDownloader_->GetStreamByStreamId(streamId);
    if (streamDescription == nullptr) {
        MEDIA_LOG_E("stream " PUBLIC_LOG_D32 " not exist", streamId);
        return segmentDownloader;
    }

    for (auto &downloader : segmentDownloaders_) {
        if ((streamDescription->type_ == MediaAVCodec::MEDIA_TYPE_VID ||
             streamDescription->type_ == MediaAVCodec::MEDIA_TYPE_AUD) &&
            (streamDescription->type_ == downloader->GetStreamType())) {
            segmentDownloader = downloader;
            break;
        } else if (streamDescription->type_ == MediaAVCodec::MEDIA_TYPE_SUBTITLE &&
                   downloader->GetStreamId() == streamId) {
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
    MEDIA_LOG_I("SelectBitRate bitrate " PUBLIC_LOG_U32, bitrate);
    if (mpdDownloader_->IsBitrateSame(bitrate)) {
        MEDIA_LOG_W("SelectBitRate is same bitrate.");
        return true;
    }

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

void DashMediaDownloader::SeekToTs(int64_t seekTime)
{
    MEDIA_LOG_I("SeekToTs seekTime:" PUBLIC_LOG_D64, seekTime);
    if (seekTime < 0) {
        return;
    }

    int64_t seekTimeMs = seekTime / MS_2_NS;
    for (auto &segmentDownloader : segmentDownloaders_) {
        std::shared_ptr<DashSegment> segment;
        mpdDownloader_->SeekToTs(segmentDownloader->GetStreamId(), seekTimeMs, segment);
        if (segment == nullptr) {
            continue;
        }

        MEDIA_LOG_I("Dash SeekToTs segment " PUBLIC_LOG_D64 ", duration:" PUBLIC_LOG_U32, segment->numberSeq_, segment->duration_);
        std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(segmentDownloader->GetStreamId());
        if (segmentDownloader->SeekToTime(segment)) {
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

void DashMediaDownloader::SetIsTriggerAutoMode(bool isAuto)
{
    isAutoSelectBitrate_ = isAuto;
}

void DashMediaDownloader::SetDownloadErrorState()
{
    MEDIA_LOG_I("SetDownloadErrorState");
    downloadErrorState_ = true;
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

            std::shared_ptr<DashSegmentDownloader> downloader = std::make_shared<DashSegmentDownloader>(
                    streamDesc->streamId_, streamDesc->type_);
            if (downloader != nullptr) {
                if (statusCallback_ != nullptr) {
                    downloader->SetStatusCallback(statusCallback_);
                }
                auto doneCallback = [this] (int streamId) {
                    UpdateDownloadFinished(streamId);
                };
                downloader->SetDownloadDoneCallback(doneCallback);
                segmentDownloaders_.push_back(downloader);
                std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(streamDesc->streamId_);
                if (initSeg != nullptr) {
                    downloader->SetInitSegment(initSeg);
                }
                downloader->Open(seg);
                MEDIA_LOG_I("dash first get segment in streamId " PUBLIC_LOG_D32 ", type " PUBLIC_LOG_D32 ", url:" PUBLIC_LOG_S, streamDesc->streamId_, streamDesc->type_, seg->url_.c_str());
            }
        }
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
            // ToDo 是否下载分片，要根据前面切换码流执行的过程判断，如果缓存是出于等待分片下载的场景，此时不能直接下载
            if (bitrateParam_.waitSegmentFinish_) {
                MEDIA_LOG_I("wait segment download finish, should not get next segment");
                return;
            }
            
            // reset bitrateParam_
            bitrateParam_.bitrate_ = 0;
            bitrateParam_.type_ = DASH_MPD_SWITCH_TYPE_NONE;
            streamId = bitrateParam_.streamId_;
            bitrateParam_.streamId_ = -1;
        } else {
            MEDIA_LOG_I("switch type: " PUBLIC_LOG_D32 " or waitSidxFinish: " PUBLIC_LOG_D32 " is error ", bitrateParam_.waitSidxFinish_, bitrateParam_.type_);
            return;
        }
    }
    
    GetSegmentToDownload(streamId, true);
}

void DashMediaDownloader::VideoSegmentDownloadFinished(int streamId)
{
    MEDIA_LOG_I("VideoSegmentDownloadFinished streamId:" PUBLIC_LOG_D32 ", type:" PUBLIC_LOG_U32, streamId, bitrateParam_.type_);
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
    MEDIA_LOG_I("GetSegmentToDownload streamId: " PUBLIC_LOG_D32 ", streamSwitchFlag: " PUBLIC_LOG_D32, downloadStreamId, streamSwitchFlag);
    // segment list is ok and no segment is downloading in segmentDownloader, so get next segment to download
    std::shared_ptr<DashSegment> segment = nullptr;
    DashMpdGetRet ret = mpdDownloader_->GetNextSegmentByStreamId(downloadStreamId, segment);
    if (ret == DASH_MPD_GET_ERROR) {
        return;
    }

    std::shared_ptr<DashSegmentDownloader> segmentDownloader;
    for (auto &downloader : segmentDownloaders_) {
        if (downloader->GetStreamType() == MediaAVCodec::MEDIA_TYPE_VID) {
            segmentDownloader = downloader;
            break;
        }
    }
    
    if (streamSwitchFlag) {
        MEDIA_LOG_I("switch bitrate update streamId from " PUBLIC_LOG_D32 " to " PUBLIC_LOG_D32, segmentDownloader->GetStreamId(), downloadStreamId);
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