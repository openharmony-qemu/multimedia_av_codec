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
#define HST_LOG_TAG "DashSegmentDownloader"

#include "dash_segment_downloader.h"
#include <map>
#include "dash_mpd_util.h"
#include "common/log.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
constexpr uint32_t VID_RING_BUFFER_SIZE = 20 * 1024 * 1024;
constexpr uint32_t AUD_RING_BUFFER_SIZE = 5 * 1024 * 1024;
constexpr uint32_t DEFAULT_RING_BUFFER_SIZE = 2 * 1024 * 1024;
constexpr int32_t TIME_OUT = 3 * 1000;
static const std::map<MediaAVCodec::MediaType, uint32_t> BUFFER_SIZE_MAP = {
        {MediaAVCodec::MediaType::MEDIA_TYPE_VID,      VID_RING_BUFFER_SIZE},
        {MediaAVCodec::MediaType::MEDIA_TYPE_AUD,      AUD_RING_BUFFER_SIZE},
        {MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE, DEFAULT_RING_BUFFER_SIZE}
};

DashSegmentDownloader::DashSegmentDownloader(int streamId, MediaAVCodec::MediaType streamType)
{
    streamId_ = streamId;
    streamType_ = streamType;
    uint32_t ringBufferSize = DEFAULT_RING_BUFFER_SIZE;
    auto ringBufferSizeItem = BUFFER_SIZE_MAP.find(streamType);
    if (ringBufferSizeItem != BUFFER_SIZE_MAP.end()) {
        ringBufferSize = ringBufferSizeItem->second;
    }
    MEDIA_LOG_I("DashSegmentDownloader ringBufferSize:" PUBLIC_LOG_U32, ringBufferSize);
    buffer_ = std::make_shared<RingBuffer>(ringBufferSize);
    buffer_->Init();

    downloader_ = std::make_shared<Downloader>("dashSegment");

    dataSave_ =  [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };

    downloadRequest_ = nullptr;
    mediaSegment_ = nullptr;
}

DashSegmentDownloader::~DashSegmentDownloader() noexcept
{
    downloadRequest_ = nullptr;
    downloader_ = nullptr;
    mediaSegment_ = nullptr;
    buffer_ = nullptr;
    segmentList_.clear();
}

bool DashSegmentDownloader::Open(const std::shared_ptr<DashSegment>& seg)
{
    std::lock_guard<std::mutex> lock(segmentMutex_);
    mediaSegment_ = std::make_shared<DashBufferSegment>(seg);
    if (mediaSegment_->startRangeValue_ >= 0 && mediaSegment_->endRangeValue_ > 0) {
        mediaSegment_->contentLength_ = mediaSegment_->endRangeValue_ - mediaSegment_->startRangeValue_ + 1;
    }
    segmentList_.push_back(mediaSegment_);
    MEDIA_LOG_I("Open enter streamId:"
    PUBLIC_LOG_D32
    " ,seqNum:" PUBLIC_LOG_D64 ", range=" PUBLIC_LOG_D64 "-" PUBLIC_LOG_D64 " url:"
    PUBLIC_LOG_S,
            mediaSegment_->streamId_, mediaSegment_->numberSeq_,
            mediaSegment_->startRangeValue_, mediaSegment_->endRangeValue_, mediaSegment_->url_.c_str());

    std::shared_ptr<DashInitSegment> initSegment = GetDashInitSegment(streamId_);
    if (initSegment != nullptr && initSegment->writeState_ == INIT_SEGMENT_STATE_UNUSE) {
        MEDIA_LOG_I("Open streamId:" PUBLIC_LOG_D32 ", writeState:" PUBLIC_LOG_D32, streamId_, initSegment->writeState_);
        initSegment->writeState_ = INIT_SEGMENT_STATE_USING;
        if (!initSegment->isDownloadFinish_) {
            int64_t startPos = initSegment->rangeBegin_;
            int64_t endPos = initSegment->rangeEnd_;
            PutRequestIntoDownloader(0, startPos, endPos, initSegment->url_);
        } else {
            initSegment->writeState_ = INIT_SEGMENT_STATE_USED;
            PutRequestIntoDownloader(mediaSegment_->duration_, mediaSegment_->startRangeValue_,
                                     mediaSegment_->endRangeValue_, mediaSegment_->url_);
        }
    } else {
        PutRequestIntoDownloader(mediaSegment_->duration_, mediaSegment_->startRangeValue_,
                                 mediaSegment_->endRangeValue_, mediaSegment_->url_);
    }
    
    return true;
}

void DashSegmentDownloader::Close(bool isAsync, bool isClean)
{
    MEDIA_LOG_I("Close enter");
    buffer_->SetActive(false, isClean);
    downloader_->Cancel();
    downloader_->Stop(isAsync);
}

void DashSegmentDownloader::Pause()
{
    MEDIA_LOG_I("Pause enter");
    buffer_->SetActive(false);
    downloader_->Pause();
}

void DashSegmentDownloader::Resume()
{
    MEDIA_LOG_I("Resume enter");
    buffer_->SetActive(true);
    downloader_->Resume();
}

void DashSegmentDownloader::CloseRequest()
{
    MEDIA_LOG_I("CloseRequest enter");
    if (downloadRequest_ != nullptr && !downloadRequest_->IsClosed()) {
        downloadRequest_->Close();
    }
}

DashReadRet DashSegmentDownloader::Read(int32_t streamId, uint8_t* buff, uint32_t wantReadLength, uint32_t& realReadLength, int32_t& realStreamId)
{
    if (buff == nullptr) {
        return DASH_READ_FAILED;
    }

    //MEDIA_LOG_I("Read: wantReadLength " PUBLIC_LOG_D32 ", streamId: "  PUBLIC_LOG_D32, wantReadLength, streamId);
    DashReadRet ret = DASH_READ_OK;
    if ((downloadRequest_ != nullptr) && downloadRequest_->IsEos()) {
        ret = DASH_READ_SEGMENT_DOWNLOAD_FINISH;
        if (buffer_->GetSize() == 0) {
            ret = DASH_READ_END;
            realReadLength = 0;
            if (mediaSegment_ != nullptr) {
                MEDIA_LOG_I("Read: streamId"
                PUBLIC_LOG_D32
                " ,segment "
                PUBLIC_LOG_D64
                " ,read Eos.", mediaSegment_->streamId_, mediaSegment_->numberSeq_);
            }
            return ret;
        }
    }

    readTime_ = 0;
    while (buffer_->GetSize() == 0) {
        if (readTime_ >= TIME_OUT) {
            Close(true, true);
            CloseRequest();
            return DASH_READ_TIMEOUT;
        }
        OSAL::SleepFor(5);  // 5
        readTime_ += 5;
    }

    std::shared_ptr<DashBufferSegment> currentSegment;
    {
        std::lock_guard<std::mutex> lock(segmentMutex_);
        for (auto &it : segmentList_) {
            MEDIA_LOG_D(
                    "Read: streamId:"
            PUBLIC_LOG_D32
            ", bufferHead:"
            PUBLIC_LOG_ZU
            ", bufferTail:"
            PUBLIC_LOG_ZU
            ", segmentHead:"
            PUBLIC_LOG_ZU
            ", segmentTail:"
            PUBLIC_LOG_ZU,
                    it->streamId_, buffer_->GetHead(), buffer_->GetTail(), it->bufferPosHead_, it->bufferPosTail_);
            if (buffer_->GetHead() >= it->bufferPosHead_ && buffer_->GetHead() <= it->bufferPosTail_) {
                currentSegment = it;
                break;
            }
        }
    }

    int32_t currentStreamId = streamId;
    if (currentSegment != nullptr) {
        currentStreamId = currentSegment->streamId_;
    }

    realStreamId = currentStreamId;
    if (realStreamId != streamId) {
        MEDIA_LOG_I("Read: changed stream streamId:"
        PUBLIC_LOG_D32
        ", realStreamId:"
        PUBLIC_LOG_D32, streamId, realStreamId);
        return ret;
    }

    std::shared_ptr<DashInitSegment> initSegment = GetDashInitSegment(currentStreamId);
    if (initSegment != nullptr && initSegment->readState_ != INIT_SEGMENT_STATE_USED) {
        unsigned int contentLen = initSegment->content_.length();
        MEDIA_LOG_I("Read: streamId:"
        PUBLIC_LOG_D32
        ", contentLen:"
        PUBLIC_LOG_U32
        ", readIndex:"
        PUBLIC_LOG_D32
        ", flag:"
        PUBLIC_LOG_D32
        ", readState:"
        PUBLIC_LOG_D32, currentStreamId, contentLen, initSegment->readIndex_, initSegment->isDownloadFinish_, initSegment->readState_);
        if (initSegment->readIndex_ == contentLen && initSegment->isDownloadFinish_) {
            // init segment read finish
            initSegment->readState_ = INIT_SEGMENT_STATE_USED;
            initSegment->readIndex_ = 0;
            return DASH_READ_OK;
        }

        unsigned int unReadSize = contentLen - initSegment->readIndex_;
        if (unReadSize > 0) {
            realReadLength = unReadSize > wantReadLength ? wantReadLength : unReadSize;
            std::string readStr = initSegment->content_.substr(initSegment->readIndex_);
            memcpy_s(buff, wantReadLength, readStr.c_str(), realReadLength);
            initSegment->readIndex_ += realReadLength;
            if (initSegment->readIndex_ == contentLen && initSegment->isDownloadFinish_) {
                // init segment read finish
                initSegment->readState_ = INIT_SEGMENT_STATE_USED;
                initSegment->readIndex_ = 0;
            }
        }

        MEDIA_LOG_I("after Read: streamId:" PUBLIC_LOG_D32 ", contentLen:" PUBLIC_LOG_U32 ", readIndex_:"
        PUBLIC_LOG_D32 ", flag:"
        PUBLIC_LOG_D32, currentStreamId, contentLen, initSegment->readIndex_, initSegment->isDownloadFinish_);
        return DASH_READ_OK;
    }

    uint32_t maxReadLength = wantReadLength;
    if (currentSegment != nullptr) {
        uint32_t availableSize = currentSegment->bufferPosTail_ - buffer_->GetHead();
        if (availableSize > 0) {
            maxReadLength = availableSize;
        }
        MEDIA_LOG_I(
                "Read: streamId:"
        PUBLIC_LOG_D32
        " limit, bufferHead:"
        PUBLIC_LOG_ZU
        ", bufferTail:"
        PUBLIC_LOG_ZU
        ", maxReadLength:"
        PUBLIC_LOG_U32,
                currentStreamId, buffer_->GetHead(), buffer_->GetTail(), maxReadLength);
    }

    realReadLength = buffer_->ReadBuffer(buff, maxReadLength > wantReadLength ? wantReadLength : maxReadLength, 2); // wait 2 times
    if (realReadLength <= 0) {
        MEDIA_LOG_W(
                "after Read: streamId:"
        PUBLIC_LOG_D32
        " ,bufferHead:"
        PUBLIC_LOG_ZU
        ", bufferTail:"
        PUBLIC_LOG_ZU
        ", realReadLength:"
        PUBLIC_LOG_U32,
                currentStreamId, buffer_->GetHead(), buffer_->GetTail(), realReadLength);
        return ret;
    }

    std::lock_guard<std::mutex> lock(segmentMutex_);
    for (auto it = segmentList_.begin(); it != segmentList_.end(); ++it) {
        if (buffer_->GetHead() != 0 && (*it)->isEos_ && buffer_->GetHead() >= (*it)->bufferPosTail_) {
            MEDIA_LOG_D("Read:streamId:"
            PUBLIC_LOG_D32
            ", erase numberSeq:"
            PUBLIC_LOG_D64, (*it)->streamId_, (*it)->numberSeq_);
            it = segmentList_.erase(it);
        } else {
            break;
        }
    }
    return ret;
}

void DashSegmentDownloader::SetStatusCallback(StatusCallbackFunc statusCallbackFunc)
{
    statusCallback_ = statusCallbackFunc;
}

void DashSegmentDownloader::SetDownloadDoneCallback(SegmentDownloadDoneCbFunc doneCbFunc)
{
    downloadDoneCbFunc_ = doneCbFunc;
}

int64_t DashSegmentDownloader::GetDownloadLength()
{
    return downloadLength_;
}

void DashSegmentDownloader::SetInitSegment(std::shared_ptr<DashInitSegment> initSegment)
{
    if (initSegment == nullptr) {
        return;
    }

    int streamId = initSegment->streamId_;
    std::shared_ptr<DashInitSegment> dashInitSegment = GetDashInitSegment(streamId);
    if (dashInitSegment == nullptr) {
        initSegments_.push_back(initSegment);
        dashInitSegment = initSegment;
    }

    if (!dashInitSegment->isDownloadFinish_) {
        dashInitSegment->writeState_ = INIT_SEGMENT_STATE_UNUSE;
    }

    dashInitSegment->readState_ = INIT_SEGMENT_STATE_UNUSE;
    MEDIA_LOG_I("SetInitSegment:streamId:"
    PUBLIC_LOG_D32
    ", isDownloadFinish_="
    PUBLIC_LOG_D32
    ", readState_="
    PUBLIC_LOG_D32
    ", writeState_="
    PUBLIC_LOG_D32, streamId, dashInitSegment->isDownloadFinish_, dashInitSegment->readState_, dashInitSegment->writeState_);
}

void DashSegmentDownloader::UpdateStreamId(int streamId)
{
    streamId_ = streamId;
}

int DashSegmentDownloader::GetStreamId()
{
    return streamId_;
}

MediaAVCodec::MediaType DashSegmentDownloader::GetStreamType()
{
    return streamType_;
}

size_t DashSegmentDownloader::GetContentLength()
{
    if (downloadRequest_ == nullptr || downloadRequest_->IsClosed()) {
        return 0; // 0
    }
    return downloadRequest_->GetFileContentLength();
}

bool DashSegmentDownloader::GetStartedStatus()
{
    return startedPlayStatus_;
}

bool DashSegmentDownloader::IsSegmentFinish()
{
    if (mediaSegment_ != nullptr && mediaSegment_->isEos_) {
        return true;
    }

    return false;
}

bool DashSegmentDownloader::CleanSegmentBuffer(bool isCleanAll, int64_t& remainLastNumberSeq)
{
    std::lock_guard<std::mutex> lock(segmentMutex_);
    if (isCleanAll) {
        MEDIA_LOG_I("CleanSegmentBuffer clean all");
        isCleaningBuffer_ = true;
        Close(true, true);
        CloseRequest();
        downloader_ = std::make_shared<Downloader>("dashSegment");
        buffer_->Clear();
        segmentList_.clear();
        buffer_->SetActive(true);
        return true;
    }

    remainLastNumberSeq = -1;
    size_t clearTail = 0;
    uint32_t remainDuration = 0;
    for (auto &it: segmentList_) {
        if (it == nullptr || buffer_->GetHead() > it->bufferPosTail_) {
            continue;
        }

        remainLastNumberSeq = it->numberSeq_;
        if (!it->isEos_) {
            break;
        }

        remainDuration += GetSegmentRemainDuration(it);
        if (remainDuration >= MIN_RETENTION_DURATION_MS) {
            clearTail = it->bufferPosTail_;
            break;
        }
    }

    if (remainLastNumberSeq == -1 && mediaSegment_ != nullptr) {
        remainLastNumberSeq = mediaSegment_->numberSeq_;
    }

    MEDIA_LOG_I("CleanSegmentBuffer:streamId:"
    PUBLIC_LOG_D32
    ", remain numberSeq:"
    PUBLIC_LOG_D64, streamId_, remainLastNumberSeq);

    if (clearTail > 0) {
        isCleaningBuffer_ = true;
        Close(true, false);
        CloseRequest();
        segmentList_.remove_if([&remainLastNumberSeq](std::shared_ptr<DashBufferSegment> bufferSegment) {
            return (bufferSegment->numberSeq_ > remainLastNumberSeq);
        });

        downloader_ = std::make_shared<Downloader>("dashSegment");
        MEDIA_LOG_I("CleanSegmentBuffer bufferHead:"
        PUBLIC_LOG_ZU
        " ,bufferTail:"
        PUBLIC_LOG_ZU
        " ,clearTail:"
        PUBLIC_LOG_ZU, buffer_->GetHead(), buffer_->GetTail(), clearTail);
        buffer_->SetTail(clearTail);
        buffer_->SetActive(true);
        return true;
    }
    return false;
}

bool DashSegmentDownloader::SeekToTime(const std::shared_ptr<DashSegment> &segment) {
    std::lock_guard<std::mutex> lock(segmentMutex_);
    std::shared_ptr<DashBufferSegment> desSegment;
    for (auto &it: segmentList_) {
        if ((it->numberSeq_ - it->startNumberSeq_) == (segment->numberSeq_ - segment->startNumberSeq_)) {
            desSegment = it;
            break;
        }
    }

    if (desSegment != nullptr && desSegment->bufferPosHead_ >= 0 && desSegment->bufferPosTail_ > 0) {
        return buffer_->SetHead(desSegment->bufferPosHead_);
    }
    return false;
}

bool DashSegmentDownloader::SaveData(uint8_t* data, uint32_t len)
{
    MEDIA_LOG_I("SaveData:streamId:" PUBLIC_LOG_D32 ", len:" PUBLIC_LOG_D32, streamId_, len);
    startedPlayStatus_ = true;
    std::shared_ptr<DashInitSegment> initSegment = GetDashInitSegment(streamId_);
    if (initSegment != nullptr && initSegment->writeState_ == INIT_SEGMENT_STATE_USING) {
        MEDIA_LOG_I("SaveData:streamId:"
        PUBLIC_LOG_D32
        ", writeState:"
        PUBLIC_LOG_D32, streamId_, initSegment->writeState_);
        initSegment->content_.append(reinterpret_cast<const char*>(data), len);
        return true;
    }

    size_t bufferTail = buffer_->GetTail();
    bool writeRet = buffer_->WriteBuffer(data, len);
    if (writeRet) {
        std::lock_guard<std::mutex> lock(segmentMutex_);
        for (auto &mediaSegment: segmentList_) {
            if (mediaSegment == nullptr || mediaSegment->isEos_) {
                continue;
            }

            if (mediaSegment->bufferPosTail_ == 0) {
                mediaSegment->bufferPosHead_ = bufferTail;
            }
            mediaSegment->bufferPosTail_ = buffer_->GetTail();

            if (mediaSegment->contentLength_ == 0) {
                mediaSegment->contentLength_ = downloadRequest_->GetFileContentLength();
            }

            // last packet len is 0 of chunk
            if (len == 0 || (mediaSegment->contentLength_ > 0 &&
                             (mediaSegment->bufferPosTail_ - mediaSegment->bufferPosHead_) >=
                             mediaSegment->contentLength_)) {
                mediaSegment->isEos_ = true;
                MEDIA_LOG_I("SaveData eos:streamId:" PUBLIC_LOG_D32 ", segmentNum:" PUBLIC_LOG_D64 ", contentLength:"
                PUBLIC_LOG_D64 ", bufferPosHead:" PUBLIC_LOG_D64  " ,bufferPosEnd:" PUBLIC_LOG_D64,
                streamId_, mediaSegment->numberSeq_, mediaSegment->contentLength_, mediaSegment->bufferPosHead_,
                mediaSegment->bufferPosTail_);
            }
            break;
        }
        downloadLength_ += len;
    }
    return writeRet;
}

void DashSegmentDownloader::PutRequestIntoDownloader(unsigned int duration, int64_t startPos, int64_t endPos, std::string url)
{
    auto realStatusCallback = [this](DownloadStatus &&status, std::shared_ptr<Downloader> &downloader,
                                     std::shared_ptr<DownloadRequest> &request) {
        statusCallback_(status, downloader_, std::forward<decltype(request)>(request));
    };
    auto downloadDoneCallback = [this](const std::string &url, const std::string &location) {
        UpdateDownloadFinished(url, location);
    };

    bool requestWholeFile = true;
    if (startPos >= 0 && endPos > 0) {
        requestWholeFile = false;
    }
    downloadRequest_ = std::make_shared<DownloadRequest>(url, duration, dataSave_,
                                                         realStatusCallback, requestWholeFile);
    downloadRequest_->SetDownloadDoneCb(downloadDoneCallback);
    if (!requestWholeFile && (endPos > startPos)) {
        downloadRequest_->SetRangePos(startPos, endPos);
    }
    MEDIA_LOG_I("PutRequestIntoDownloader:range="
    PUBLIC_LOG_D64
    "-"
    PUBLIC_LOG_D64
    " url:"
    PUBLIC_LOG_S, startPos, endPos,
            url.c_str());

    isCleaningBuffer_ = false;
    downloader_->Download(downloadRequest_, -1); // -1
    downloader_->Start();
    downloadLength_ = 0;
}

void DashSegmentDownloader::UpdateDownloadFinished(const std::string& url, const std::string& location)
{
    MEDIA_LOG_I("UpdateDownloadFinished:streamId:" PUBLIC_LOG_D32 ", url=" PUBLIC_LOG_S, streamId_, url.c_str());
    std::shared_ptr<DashInitSegment> initSegment = GetDashInitSegment(streamId_);
    if (initSegment != nullptr && initSegment->writeState_ == INIT_SEGMENT_STATE_USING) {
        MEDIA_LOG_I("UpdateDownloadFinished:streamId:"
        PUBLIC_LOG_D32
        ", writeState:"
        PUBLIC_LOG_D32, streamId_, initSegment->writeState_);
        initSegment->writeState_ = INIT_SEGMENT_STATE_USED;
        initSegment->isDownloadFinish_ = true;
        if (mediaSegment_ != nullptr) {
            PutRequestIntoDownloader(mediaSegment_->duration_, mediaSegment_->startRangeValue_,
                                     mediaSegment_->endRangeValue_, mediaSegment_->url_);
            return;
        }
    }

    if (mediaSegment_->contentLength_ == 0) {
        mediaSegment_->contentLength_ = downloadRequest_->GetFileContentLength();
    }

    if (!mediaSegment_->isEos_) {
        mediaSegment_->isEos_ = true;
    }
    MEDIA_LOG_I("UpdateDownloadFinished: segmentNum:" PUBLIC_LOG_D64 ", contentLength:"
    PUBLIC_LOG_D64
    ", isCleaningBuffer:" PUBLIC_LOG_D32, mediaSegment_->numberSeq_, mediaSegment_->contentLength_, isCleaningBuffer_);
    if (downloadDoneCbFunc_ && !isCleaningBuffer_) {
        downloadDoneCbFunc_(streamId_);
    }
}

uint32_t DashSegmentDownloader::GetSegmentRemainDuration(const std::shared_ptr<DashBufferSegment>& currentSegment)
{
    if (buffer_->GetHead() > currentSegment->bufferPosHead_) {
        return (((currentSegment->bufferPosTail_ - buffer_->GetHead()) * currentSegment->duration_) /
                (currentSegment->bufferPosTail_ - currentSegment->bufferPosHead_));
    } else {
        return currentSegment->duration_;
    }
}

std::shared_ptr<DashInitSegment> DashSegmentDownloader::GetDashInitSegment(int32_t streamId)
{
    std::shared_ptr<DashInitSegment> segment = nullptr;
    for (auto &initSegment : initSegments_) {
        if (initSegment != nullptr && initSegment->streamId_ == streamId) {
            segment = initSegment;
            break;
        }
    }
    return segment;
}

}
}
}
}