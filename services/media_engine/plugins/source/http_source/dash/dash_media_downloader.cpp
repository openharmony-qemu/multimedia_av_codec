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
#define HST_LOG_TAG "DashMediaDownloader"

#include <algorithm>
#include "dash_media_downloader.h"
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

DashMediaDownloader::DashMediaDownloader(std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
{
    if (sourceLoader != nullptr) {
        MEDIA_LOG_I("DashMediaDownloader app download.");
        sourceLoader_ = sourceLoader;
    }
    mpdDownloader_ = std::make_shared<DashMpdDownloader>(sourceLoader);
}

DashMediaDownloader::~DashMediaDownloader()
{
    if (mpdDownloader_ != nullptr) {
        mpdDownloader_->Close(false);
    }

    segmentDownloaders_.Clear();
}

void DashMediaDownloader::Init()
{
    MEDIA_LOG_D("%{public}" PRIu64 " Init", avcodecUniqueId_);
    downloadMetricsInfo_ = std::make_shared<DownloadMetricsInfo>();
    if (mpdDownloader_ != nullptr && downloadMetricsInfo_ != nullptr) {
        mpdDownloader_->SetDownloadCallback(downloadMetricsInfo_);
    }
    mpdDownloader_->SetMpdCallback(shared_from_this());
    mpdDownloader_->Init();
}

bool DashMediaDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    mpdDownloader_->Open(url);
    if (reportInfo_ != nullptr) {
        std::vector<StreamInfo> streams;
        GetStreamInfo(streams);
        reportInfo_->videoStreamCnt_ = streams.size();
    }
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
    segmentDownloaders_.Foreach([isAsync](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            return;
        }
        segDownloader->Close(isAsync, true);
    });
}

void DashMediaDownloader::Pause()
{
    segmentDownloaders_.Foreach([](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            return;
        }
        segDownloader->Pause();
    });
}

void DashMediaDownloader::Resume()
{
    segmentDownloaders_.Foreach([](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            return;
        }
        segDownloader->Resume();
    });
}

Status DashMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V(buff != nullptr, Status::END_OF_STREAM);
    if (segmentDownloaders_.Empty()) {
        MEDIA_LOG_W("dash read, segmentDownloaders size is 0");
        return Status::END_OF_STREAM;
    }

    if (downloadErrorState_) {
        return HandleDownloadError();
    }

    std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloader(readDataInfo.streamId_);
    if (segmentDownloader == nullptr) {
        MEDIA_LOG_E("GetSegmentDownloader failed when Read, streamId " PUBLIC_LOG_D32, readDataInfo.streamId_);
        return Status::END_OF_STREAM;
    }

    DashReadRet ret = segmentDownloader->Read(buff, readDataInfo, isInterruptNeeded_.load());
    MEDIA_LOG_D("Read:streamId " PUBLIC_LOG_D32 " readRet:" PUBLIC_LOG_D32, readDataInfo.streamId_, ret);
    if (ret == DASH_READ_END) {
        MEDIA_LOG_I("Read:streamId " PUBLIC_LOG_D32 " segment all finished end, currentPeriodIndex: " PUBLIC_LOG_U32,
            readDataInfo.streamId_, currentPeriodIndex_.load());
        if (ApplyPendingPeriodSwitch(segmentDownloader)) {
            return Status::OK;
        }
        auto mpdInfo = mpdDownloader_->GetMpdInfo();
        if (mpdInfo != nullptr) {
            size_t totalPeriods = mpdInfo->periodInfoList_.size();
            unsigned int nextPeriodIndex = currentPeriodIndex_.load() + 1;
            if (totalPeriods > 1 && nextPeriodIndex < totalPeriods) {
                MEDIA_LOG_I("Read EOS: switch from period " PUBLIC_LOG_U32 " to period " PUBLIC_LOG_U32,
                    currentPeriodIndex_.load(), nextPeriodIndex);
                SwitchStreamToPeriod(nextPeriodIndex, segmentDownloader);
                return Status::OK;
            }
        }
        readDataInfo.isEos_ = true;
        return Status::END_OF_STREAM;
    } else if (ret == DASH_READ_AGAIN) {
        return Status::ERROR_AGAIN;
    } else if (ret == DASH_READ_FAILED || ret == DASH_READ_INTERRUPT) {
        return Status::END_OF_STREAM;
    }
    return Status::OK;
}

Status DashMediaDownloader::HandleDownloadError()
{
    auto callback = callback_.lock();
    if (callback) {
        MEDIA_LOG_I("Read Client Error, OnEvent");
        callback->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT},
            "errorMessage: Dash read error due to request failed"});
    }
    segmentDownloaders_.Foreach([](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            return;
        }
        segDownloader->Close(false, true);
    });
    return Status::ERROR_AGAIN;
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
    auto result = segmentDownloaders_.FindIf([type](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        return segDownloader != nullptr && segDownloader->GetStreamType() == type;
    });
    if (result == std::nullopt) {
        return nullptr;
    }
    return *result;
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
    while (!status && times < RETRY_TIMES && !isInterruptNeeded_.load()) {
        segmentDownloaders_.FindIf([&status](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
            if (segDownloader == nullptr) {
                return false;
            }
            status = segDownloader->GetStartedStatus();
            return !status;
        });
        OSAL::SleepFor(SLEEP_TIME);
        times++;
    }

    if (times >= RETRY_TIMES || isInterruptNeeded_.load()) {
        MEDIA_LOG_I("DashMediaDownloader GetSeekable INVALID");
        return Seekable::INVALID;
    }

    MEDIA_LOG_I("DashMediaDownloader GetSeekable end");
    return value;
}

std::vector<SeekRange> DashMediaDownloader::GetSeekableRanges() const
{
    MEDIA_LOG_I("DashMediaDownloader GetSeekableRanges begin");
    FALSE_RETURN_V_MSG_E(mpdDownloader_ != nullptr, {}, "mpdDownloader_ is nullptr, GetSeekableRanges fail");
    return mpdDownloader_->GetSeekableRanges();
}

void DashMediaDownloader::SetCallback(const std::shared_ptr<Callback>& cb)
{
    callback_ = cb;
    if (mpdDownloader_ != nullptr) {
        mpdDownloader_->SetCallback(cb);
    }
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

            segmentDownloaders_.Foreach([](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
                if (segDownloader == nullptr) {
                    return;
                }
                MEDIA_LOG_I("Dash clean streamId: " PUBLIC_LOG_D32, segDownloader->GetStreamId());
                int64_t remainLastNumberSeq = -1;
                segDownloader->CleanSegmentBuffer(true, remainLastNumberSeq);
            });
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
    auto callback = callback_.lock();
    if (callback) {
        callback->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT},
            "errorMessage: Dash download pause due to request failed"});
    }
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
    }
}

Status DashMediaDownloader::GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate)
{
    if (!isUpdate) {
        GetSeekable();
    }
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

    // Report EventStream metadata from current Period
    ReportEventStreamMetadata();
}

void DashMediaDownloader::OpenInitSegment(
    const std::shared_ptr<DashStreamDescription> &streamDesc, const std::shared_ptr<DashSegment> &seg)
{
    auto callback = callback_.lock();
    std::shared_ptr<DashSegmentDownloader> downloader = std::make_shared<DashSegmentDownloader>(
        callback, streamDesc->streamId_, streamDesc->type_, expectDuration_, sourceLoader_);
    if (downloadMetricsInfo_ != nullptr) {
        downloader->SetDownloadCallback(downloadMetricsInfo_);
    }
    downloader->Init();
    downloader->SetSourceStatisticsDfx(reportInfo_);
    downloader->SetStatusCallback(statusCallback_);
    auto weakDownloader = weak_from_this();
    auto doneCallback = [weakDownloader] (int streamId) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "doneCb, dash media downloader already destructed.");
        shareDownloader->UpdateDownloadFinished(streamId);
    };
    downloader->SetDownloadDoneCallback(doneCallback);
    segmentDownloaders_.PushBack(downloader);
    std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(
        streamDesc->streamId_);
    if (initSeg != nullptr) {
        downloader->SetInitSegment(initSeg);
    }
    downloader->Open(seg);
    MEDIA_LOG_I("dash first get segment in streamId " PUBLIC_LOG_D32 ", type "
        PUBLIC_LOG_D32, streamDesc->streamId_, streamDesc->type_);
}

void DashMediaDownloader::SetSourceStatisticsDfx(
    std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr)
{
    reportInfo_ = rpInfoPtr;
    FALSE_RETURN_MSG(mpdDownloader_ != nullptr, "mpdDownloader_ is nullptr");
    mpdDownloader_->SetSourceStatisticsDfx(reportInfo_);
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
            auto callback = callback_.lock();
            if (callback) {
                switchFlag = callback->CanAutoSelectBitRate();
            }
            std::shared_ptr<DashSegmentDownloader> segmentDownloader = GetSegmentDownloaderByType(
                MediaAVCodec::MediaType::MEDIA_TYPE_VID);
            if (segmentDownloader != nullptr && !segmentDownloader->IsAllSegmentFinished()) {
                uint32_t downloadSpeed = static_cast<uint32_t>(segmentDownloader->GetDownloadSpeed());
                if (callback != nullptr) {
                    callback->OnEvent({PluginEventType::NETWORK_BITRATE_CHANGED, {downloadSpeed},
                        "network bitrate change"});
                    callback->OnEvent({PluginEventType::LOADING_RATE_CHANGED, {downloadSpeed},
                        "loading rate change"});
                }
            }
            if (segmentDownloader != nullptr && !segmentDownloader->IsAllSegmentFinished() &&
                switchFlag && isAutoSelectBitrate_) {
                bool flag = CheckAutoSelectBitrate(streamId);
                if (callback) {
                    callback->SetSelectBitRateFlag(flag, bitrateParam_.bitrate_);
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
    auto result = segmentDownloaders_.FindIf([](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        return segDownloader != nullptr && segDownloader->GetStreamType() == MediaAVCodec::MEDIA_TYPE_VID;
    });
    if (result == std::nullopt) {
        MEDIA_LOG_W("Dash not start, can not SelectBitRate.");
        return;
    }
    segmentDownloader = *result;

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
    auto callback = callback_.lock();
    if (callback) {
        MEDIA_LOG_D("Onevent dash seek ready");
        callback->OnEvent({PluginEventType::DASH_SEEK_READY, seekReadyInfo, "dash_seek_ready"});
    }
}

int64_t DashMediaDownloader::GetVideoSeekTime(int64_t seekTimeMs)
{
    auto videoSegmentDownloader = GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    FALSE_RETURN_V_NOLOG(videoSegmentDownloader != nullptr, seekTimeMs);

    std::shared_ptr<DashSegment> segment;
    return mpdDownloader_->SeekToTs(videoSegmentDownloader->GetStreamId(), seekTimeMs, segment);
}

void DashMediaDownloader::SeekToSegment(const std::shared_ptr<DashSegmentDownloader> &segDownloader,
    int64_t seekTimeMs, bool isSwitching)
{
    if (segDownloader == nullptr) {
        return;
    }
    std::shared_ptr<DashSegment> segment;
    int32_t streamId = static_cast<int32_t>(segDownloader->GetStreamId());
    mpdDownloader_->SeekToTs(segDownloader->GetStreamId(), seekTimeMs, segment);
    if (segment == nullptr) {
        MEDIA_LOG_I("Dash SeekToTs end streamId " PUBLIC_LOG_D32 ", type " PUBLIC_LOG_D32,
            segDownloader->GetStreamId(), segDownloader->GetStreamType());
        int64_t remainLastNumberSeq = -1;
        segDownloader->CleanSegmentBuffer(true, remainLastNumberSeq);
        segDownloader->SetAllSegmentFinished();
        this->HandleSeekReady(static_cast<int32_t>(segDownloader->GetStreamType()), streamId, seekTimeMs, 1);
        return;
    }

    if (segDownloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_VID &&
        segDownloader->GetBufferSize() == 0 && seekTimeMs == 0) {
        segDownloader->NotifyInitSuccess();
    }
    MEDIA_LOG_D("Dash SeekToTs segment " PUBLIC_LOG_D64 ", duration:"
        PUBLIC_LOG_U32, segment->numberSeq_, segment->duration_);
    std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(
        segDownloader->GetStreamId());
    if (!isSwitching && segDownloader->SeekToTime(segment, streamId)) {
        MEDIA_LOG_I("Dash SeekToTs of buffered streamId " PUBLIC_LOG_D32 ", type " PUBLIC_LOG_D32,
            segDownloader->GetStreamId(), segDownloader->GetStreamType());
        segDownloader->SetInitSegment(initSeg, true);
    } else {
        int64_t remainLastNumberSeq = -1;
        segDownloader->CleanSegmentBuffer(true, remainLastNumberSeq);
        mpdDownloader_->SetCurrentNumberSeqByStreamId(segDownloader->GetStreamId(), segment->numberSeq_);
        segDownloader->SetInitSegment(initSeg, true);
        segDownloader->Open(segment);
    }
    this->HandleSeekReady(static_cast<int32_t>(segDownloader->GetStreamType()), streamId, seekTimeMs, 0);
}

void DashMediaDownloader::SeekInternal(int64_t seekTimeMs)
{
    bool isSwitching = IsSeekingInSwitch();

    {
        std::lock_guard<std::mutex> lock(pendingSwitchMutex_);
        pendingVideoSwitch_ = {};
        pendingAudioSwitch_ = {};
    }

    seekTimeMs = GetVideoSeekTime(seekTimeMs);
    unsigned int tragetPeriodIndex = mpdDownloader_->GetPeriodIndexBySeekTime(seekTimeMs);
    if (tragetPeriodIndex != currentPeriodIndex_.load()) {
        MEDIA_LOG_I("SeekInternal: seek from period " PUBLIC_LOG_U32 " to period " PUBLIC_LOG_U32,
            currentPeriodIndex_.load(), tragetPeriodIndex);
        SwitchToPeriod(tragetPeriodIndex);
    }
    segmentDownloaders_.ForeachSnapshot([this, seekTimeMs, isSwitching](
        const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        SeekToSegment(segDownloader, seekTimeMs, isSwitching);
    });
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
    auto callback = callback_.lock();
    if (callback) {
        callback->OnEvent({PluginEventType::SOURCE_DRM_INFO_UPDATE, {drmInfos}, "drm_info_update"});
    }
}

void DashMediaDownloader::OnTimedMetaDataChanged(const std::shared_ptr<MediaAVCodec::AVTimedMetaData> &metadata)
{
    FALSE_RETURN_MSG(metadata != nullptr, "OnTimedMetaDataChanged: metadata is nullptr, ignore");
    auto callback = callback_.lock();
    FALSE_RETURN_MSG(callback != nullptr, "OnTimedMetaDataChanged: callback is nullptr, ignore");
    callback->OnEvent({PluginEventType::SOURCE_TIMED_METADATA, {metadata}, "timed_meta_data"});
}

void DashMediaDownloader::ReportEventStreamMetadata()
{
    FALSE_RETURN_MSG(mpdDownloader_ != nullptr, "ReportEventStreamMetadata: mpdDownloader is nullptr, ignore");
    DashList<DashEventStreamInfo *> eventStreamList = mpdDownloader_->GetCurrentPeriodEventStreams();
    FALSE_RETURN_MSG(eventStreamList.size() > 0, "ReportEventStreamMetadata: no event streams in current period");
    for (auto *eventStream : eventStreamList) {
        if (eventStream == nullptr) {
            continue;
        }
        for (auto *event : eventStream->eventList_) {
            if (event == nullptr) {
                continue;
            }
            auto metaData = std::make_shared<MediaAVCodec::AVTimedMetaData>();
            metaData->id = event->id_;
            metaData->classify = event->classify_;
            metaData->start = static_cast<int64_t>(event->start_);
            metaData->duration = static_cast<int64_t>(event->duration_);
            metaData->contents = event->contents_;

            MEDIA_LOG_I("ReportEventStreamMetadata: id=" PUBLIC_LOG_S ", schemeIdUri=" PUBLIC_LOG_S
                ", start=" PUBLIC_LOG_D64 ", duration=" PUBLIC_LOG_D64,
                metaData->id.c_str(), metaData->classify.c_str(), metaData->start, metaData->duration);

            OnTimedMetaDataChanged(metaData);
        }
    }
}

void DashMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_.store(isInterruptNeeded);
    mpdDownloader_->SetInterruptState(isInterruptNeeded);
    segmentDownloaders_.Foreach([isInterruptNeeded](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            return;
        }
        segDownloader->SetInterruptState(isInterruptNeeded);
    });
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
    if (segmentDownloaders_.Empty()) {
        return;
    }
    auto firstSegDownloader = segmentDownloaders_.Front();
    if (firstSegDownloader) {
        (*firstSegDownloader)->GetIp(playbackInfo.serverIpAddress);
    }
    bool DownloadFinishStateTmp = true;
    playbackInfo.averageDownloadRate = 0;
    segmentDownloaders_.Foreach([&](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            return;
        }
        if (playbackInfo.averageDownloadRate < static_cast<int64_t>(segDownloader->GetDownloadSpeed())) {
            playbackInfo.averageDownloadRate = static_cast<int64_t>(segDownloader->GetDownloadSpeed());
            std::pair<int64_t, int64_t> recordData = segDownloader->GetDownloadRecordData();
            playbackInfo.downloadRate = recordData.first;
            playbackInfo.bufferDuration = recordData.second;
        }
        DownloadFinishStateTmp = (DownloadFinishStateTmp && segDownloader->GetDownloadFinishState());
    });
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
    auto vid = GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    auto aud = GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
    if (vid && aud) {
        return vid->GetBufferSize() > 0 && aud->GetBufferSize() > 0;
    } else if (vid) {
        return vid->GetBufferSize() > 0;
    } else if (aud) {
        return aud->GetBufferSize() > 0;
    }
    return false;
}

void DashMediaDownloader::SetAppUid(int32_t appUid)
{
    segmentDownloaders_.Foreach([appUid](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            return;
        }
        segDownloader->SetAppUid(appUid);
    });
}

void DashMediaDownloader::NotifyInitSuccess()
{
    segmentDownloaders_.Foreach([](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            return;
        }
        segDownloader->NotifyInitSuccess();
    });
}

uint64_t DashMediaDownloader::GetMemorySize()
{
    uint64_t memorySize = 0;
    segmentDownloaders_.Foreach([&](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            return;
        }
        auto streamType = segDownloader->GetStreamType();
        memorySize += static_cast<uint64_t>(segDownloader->GetRingBufferInitSize(streamType));
    });
    return memorySize;
}

Status DashMediaDownloader::StopBufferring(bool isAppBackground)
{
    MEDIA_LOG_I("DashMediaDownloader:StopBufferring enter");
    auto result = segmentDownloaders_.FindIf([&](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            MEDIA_LOG_E("segment nullptr");
            return true;
        }
        segDownloader->StopBufferring(isAppBackground);
        return false;
    });
    if (result != std::nullopt) {
        return Status::ERROR_NULL_POINTER;
    }
    return Status::OK;
}

void DashMediaDownloader::SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId)
{
    if (mpdDownloader_ != nullptr) {
        mpdDownloader_->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    }
}

void DashMediaDownloader::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    if (downloadMetricsInfo_ != nullptr) {
        downloadInfo.totalDownLoadBytes = downloadMetricsInfo_->GetTotalTotalDownLoadBytes();
        downloadInfo.totalLoadingTime = downloadMetricsInfo_->GetTotalTotalDownloadTime();
        downloadInfo.loadingCount = downloadMetricsInfo_->GetTotalDownloadCount();
        downloadInfo.firstDownloadTime = downloadMetricsInfo_->GetTotalFirstDownloadTime();
        downloadInfo.firstFrameDecapsulationTime = downloadMetricsInfo_->GetTotalFirstDownloadTimestamp();
    }
}

bool DashMediaDownloader::NeedRestartDecoder(
    const std::shared_ptr<DashStreamDescription>& oldStream,
    const std::shared_ptr<DashStreamDescription>& newStream)
{
    if (oldStream == nullptr || newStream == nullptr) {
        return false;
    }
    if (oldStream->codecs_ != newStream->codecs_) {
        MEDIA_LOG_I("NeedRestartDecoder: codecs changed from" PUBLIC_LOG_S " to " PUBLIC_LOG_S,
            oldStream->codecs_.c_str(), newStream->codecs_.c_str());
        return true;
    }
    if (oldStream->videoType_ != newStream->videoType_) {
        MEDIA_LOG_I("NeedRestartDecoder: videoType changed from" PUBLIC_LOG_D32 " to " PUBLIC_LOG_D32,
            oldStream->videoType_, newStream->videoType_);
        return true;
    }
    return false;
}

bool DashMediaDownloader::GetStreamsForPeriodSwitch(uint32_t newPeriodIndex,
    std::shared_ptr<DashStreamDescription> &oldVideoStream,
    std::shared_ptr<DashStreamDescription> &newVideoStream,
    std::shared_ptr<DashStreamDescription> &newAudioStream)
{
    oldVideoStream = mpdDownloader_->GetStreamByStreamId(mpdDownloader_->GetInUseVideoStreamId());
    if (oldVideoStream == nullptr) {
        MEDIA_LOG_E("SwitchToPeriod: old stream not found");
        return false;
    }
    newVideoStream = mpdDownloader_->GetStreamByPeriodIndex(newPeriodIndex,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    newAudioStream = mpdDownloader_->GetStreamByPeriodIndex(newPeriodIndex,
        MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
    if (newVideoStream == nullptr) {
        MEDIA_LOG_E("SwitchToPeriod: new stream not found, periodIndex: " PUBLIC_LOG_U32, newPeriodIndex);
        return false;
    }
    return true;
}

void DashMediaDownloader::SwitchVideoStream(const std::shared_ptr<DashStreamDescription> &newVideoStream,
    const std::shared_ptr<DashSegmentDownloader> &segDownloader)
{
    if (newVideoStream == nullptr || segDownloader == nullptr) {
        MEDIA_LOG_E("newVideoStream or segDownloader is null");
        return;
    }
    int64_t remainLastNumberSeq = -1;
    segDownloader->CleanSegmentBuffer(true, remainLastNumberSeq);

    std::lock_guard<std::mutex> lock(switchMutex_);
    trackParam_.isEnd_ = false;
    trackParam_.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    trackParam_.streamId_ = newVideoStream->streamId_;
    trackParam_.position_ = remainLastNumberSeq;

    DashMpdGetRet ret = mpdDownloader_->GetNextTrackStream(trackParam_);
    if (ret == DASH_MPD_GET_ERROR) {
        MEDIA_LOG_E("Dash SwitchToPeriod video switch failed");
        return;
    }
    if (ret == DASH_MPD_GET_UNDONE) {
        trackParam_.waitSidxFinish_ = true;
        MEDIA_LOG_I("Dash SwitchToPeriod video wait sidx finsh");
        return;
    }

    GetSegmentToDownload(trackParam_.streamId_, true);
    ResetTrackParam();
    MEDIA_LOG_I("Dash SwitchToPeriod video switch done, new streamId: " PUBLIC_LOG_D32,
        newVideoStream->streamId_);
}

void DashMediaDownloader::SwitchAudioStream(const std::shared_ptr<DashStreamDescription> &newAudioStream,
    const std::shared_ptr<DashSegmentDownloader> &segDownloader)
{
    if (newAudioStream == nullptr || segDownloader == nullptr) {
        MEDIA_LOG_E("newAudioStream or segDownloader is null");
        return;
    }
    int64_t remainLastNumberSeq = -1;
    segDownloader->CleanSegmentBuffer(true, remainLastNumberSeq);

    std::lock_guard<std::mutex> lock(switchMutex_);
    trackParam_.isEnd_ = false;
    trackParam_.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    trackParam_.streamId_ = newAudioStream->streamId_;
    trackParam_.position_ = remainLastNumberSeq;

    DashMpdGetRet ret = mpdDownloader_->GetNextTrackStream(trackParam_);
    if (ret == DASH_MPD_GET_ERROR) {
        MEDIA_LOG_E("Dash SwitchToPeriod audio switch failed");
        return;
    }
    if (ret == DASH_MPD_GET_UNDONE) {
        trackParam_.waitSidxFinish_ = true;
        MEDIA_LOG_I("Dash SwitchToPeriod audio wait sidx finsh");
        return;
    }

    GetSegmentToDownload(trackParam_.streamId_, true);
    ResetTrackParam();
    MEDIA_LOG_I("Dash SwitchToPeriod audio switch done, new streamId: " PUBLIC_LOG_D32,
        newAudioStream->streamId_);
}

bool DashMediaDownloader::ApplyPendingPeriodSwitch(const std::shared_ptr<DashSegmentDownloader> &segDownloader)
{
    if (segDownloader == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(pendingSwitchMutex_);

    PendingPeriodSwitch* pendingSwitch = nullptr;
    if (segDownloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        pendingSwitch = &pendingVideoSwitch_;
    } else if (segDownloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
        pendingSwitch = &pendingAudioSwitch_;
    } else {
        return false;
    }

    if (!pendingSwitch->pending) {
        return false;
    }

    MEDIA_LOG_I("ApplyPendingPeriodSwitch: applying deferred switch for stream type "
        PUBLIC_LOG_D32 " to period " PUBLIC_LOG_U32,
        static_cast<int32_t>(segDownloader->GetStreamType()), pendingSwitch->targetPeriodIndex);

    if (segDownloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        SwitchVideoStream(pendingSwitch->newStream, segDownloader);
    } else {
        SwitchAudioStream(pendingSwitch->newStream, segDownloader);
    }

    pendingSwitch->pending = false;
    pendingSwitch->newStream = nullptr;
    return true;
}

void DashMediaDownloader::SwitchToPeriod(uint32_t newPeriodIndex)
{
    std::shared_ptr<DashStreamDescription> oldVideoStream;
    std::shared_ptr<DashStreamDescription> newVideoStream;
    std::shared_ptr<DashStreamDescription> newAudioStream;

    if (!GetStreamsForPeriodSwitch(newPeriodIndex, oldVideoStream, newVideoStream, newAudioStream)) {
        return;
    }

    MEDIA_LOG_I("SwitchToPeriod: switch from period: " PUBLIC_LOG_U32 " to period: " PUBLIC_LOG_U32,
        currentPeriodIndex_.load(), newPeriodIndex);
    
    bool needRestart = NeedRestartDecoder(oldVideoStream, newVideoStream);

    segmentDownloaders_.Foreach([this, newVideoStream,
        newAudioStream](const std::shared_ptr<DashSegmentDownloader> &segDownloader) {
        if (segDownloader == nullptr) {
            return;
        }

        if (segDownloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
            SwitchVideoStream(newVideoStream, segDownloader);
        } else if (segDownloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_AUD &&
                    newAudioStream != nullptr) {
            SwitchAudioStream(newAudioStream, segDownloader);
        }
    });

    currentPeriodIndex_.store(newPeriodIndex);
    mpdDownloader_->SetCurrentPeriodIndex(newPeriodIndex);

    if (needRestart) {
        MEDIA_LOG_I("SwitchToPeriod: codec changed, need restart decoder");
    }
}

void DashMediaDownloader::SwitchOtherStreamToPeriod(uint32_t newPeriodIndex,
    MediaAVCodec::MediaType otherType,
    const std::shared_ptr<DashStreamDescription> &newVideoStream,
    const std::shared_ptr<DashStreamDescription> &newAudioStream)
{
    auto otherDownloader = GetSegmentDownloaderByType(otherType);
    if (otherDownloader == nullptr) {
        return;
    }

    if (otherType == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        if (otherDownloader->IsAllSegmentFinished() && otherDownloader->GetBufferSize() == 0) {
            SwitchVideoStream(newVideoStream, otherDownloader);
        } else {
            MEDIA_LOG_I("SwitchOtherStreamToPeriod: defer video switch, buffer size: "
                PUBLIC_LOG_U64, otherDownloader->GetBufferSize());
            std::lock_guard<std::mutex> lock(pendingSwitchMutex_);
            pendingVideoSwitch_.targetPeriodIndex = newPeriodIndex;
            pendingVideoSwitch_.newStream = newVideoStream;
            pendingVideoSwitch_.pending = true;
        }
    } else if (newAudioStream != nullptr) {
        if (otherDownloader->IsAllSegmentFinished() && otherDownloader->GetBufferSize() == 0) {
            SwitchAudioStream(newAudioStream, otherDownloader);
        } else {
            MEDIA_LOG_I("SwitchOtherStreamToPeriod: defer audio switch, buffer size: "
                PUBLIC_LOG_U64, otherDownloader->GetBufferSize());
            std::lock_guard<std::mutex> lock(pendingSwitchMutex_);
            pendingAudioSwitch_.targetPeriodIndex = newPeriodIndex;
            pendingAudioSwitch_.newStream = newAudioStream;
            pendingAudioSwitch_.pending = true;
        }
    }
}

void DashMediaDownloader::SwitchStreamToPeriod(uint32_t newPeriodIndex,
    const std::shared_ptr<DashSegmentDownloader> &triggerDownloader)
{
    if (triggerDownloader == nullptr) {
        return;
    }

    std::shared_ptr<DashStreamDescription> oldVideoStream;
    std::shared_ptr<DashStreamDescription> newVideoStream;
    std::shared_ptr<DashStreamDescription> newAudioStream;

    if (!GetStreamsForPeriodSwitch(newPeriodIndex, oldVideoStream, newVideoStream, newAudioStream)) {
        return;
    }

    MEDIA_LOG_I("SwitchStreamToPeriod: switch from period: " PUBLIC_LOG_U32 " to period: " PUBLIC_LOG_U32
        ", trigger stream type: " PUBLIC_LOG_D32, currentPeriodIndex_.load(), newPeriodIndex,
        static_cast<int32_t>(triggerDownloader->GetStreamType()));
    
    bool needRestart = NeedRestartDecoder(oldVideoStream, newVideoStream);

    if (triggerDownloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        SwitchVideoStream(newVideoStream, triggerDownloader);
        SwitchOtherStreamToPeriod(newPeriodIndex, MediaAVCodec::MediaType::MEDIA_TYPE_AUD,
            newVideoStream, newAudioStream);
    } else if (triggerDownloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
        SwitchAudioStream(newAudioStream, triggerDownloader);
        SwitchOtherStreamToPeriod(newPeriodIndex, MediaAVCodec::MediaType::MEDIA_TYPE_VID,
            newVideoStream, newAudioStream);
    }

    currentPeriodIndex_.store(newPeriodIndex);
    mpdDownloader_->SetCurrentPeriodIndex(newPeriodIndex);

    if (needRestart) {
        MEDIA_LOG_I("SwitchStreamToPeriod: codec changed, need restart decoder");
    }
}
}
}
}
}
