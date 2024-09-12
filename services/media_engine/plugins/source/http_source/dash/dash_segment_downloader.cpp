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
#include <algorithm>
#include "dash_mpd_util.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
constexpr uint32_t VID_RING_BUFFER_SIZE = 20 * 1024 * 1024;
constexpr uint32_t AUD_RING_BUFFER_SIZE = 2 * 1024 * 1024;
constexpr uint32_t SUBTITLE_RING_BUFFER_SIZE = 1 * 1024 * 1024;
constexpr uint32_t DEFAULT_RING_BUFFER_SIZE = 5 * 1024 * 1024;
constexpr int DEFAULT_WAIT_TIME = 2;
constexpr int32_t HTTP_TIME_OUT_MS = 10 * 1000;
constexpr uint32_t RECORD_TIME_INTERVAL = 1000;
constexpr int32_t RECORD_DOWNLOAD_MIN_BIT = 1000;
constexpr uint32_t SPEED_MULTI_FACT = 1000;
constexpr uint32_t BYTE_TO_BIT = 8;
constexpr int PLAY_WATER_LINE = 5 * 1024;
constexpr int64_t BYTES_TO_BIT = 8;
constexpr int32_t DEFAULT_VIDEO_WATER_LINE = 512 * 1024;
constexpr int32_t DEFAULT_AUDIO_WATER_LINE = 96 * 1024;
constexpr float DEFAULT_MIN_CACHE_TIME = 0.3;
constexpr float DEFAULT_MAX_CACHE_TIME = 10.0;
constexpr uint32_t DURATION_CHANGE_AMOUT_MILLIONSECOND = 500;
constexpr int64_t SECOND_TO_MILLIONSECOND = 1000;
constexpr uint32_t BUFFERING_SLEEP_TIME_MS = 10;
constexpr uint32_t BUFFERING_TIME_OUT_MS = 1000;
constexpr uint32_t UPDATE_CACHE_STEP = 5 * 1024;
constexpr uint32_t BUFFERING_PERCENT_FULL = 100;
constexpr double ZERO_THRESHOLD = 1e-9;

static const std::map<MediaAVCodec::MediaType, uint32_t> BUFFER_SIZE_MAP = {
    {MediaAVCodec::MediaType::MEDIA_TYPE_VID, VID_RING_BUFFER_SIZE},
    {MediaAVCodec::MediaType::MEDIA_TYPE_AUD, AUD_RING_BUFFER_SIZE},
    {MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE, SUBTITLE_RING_BUFFER_SIZE}};

DashSegmentDownloader::DashSegmentDownloader(Callback *callback, int streamId, MediaAVCodec::MediaType streamType,
                                             uint64_t expectDuration)
{
    callback_ = callback;
    streamId_ = streamId;
    streamType_ = streamType;
    expectDuration_ = expectDuration;
    if (expectDuration_ > 0) {
        userDefinedBufferDuration_ = true;
    }
    size_t ringBufferSize = GetRingBufferInitSize(streamType_);
    MEDIA_LOG_I("DashSegmentDownloader streamId:" PUBLIC_LOG_D32 ", ringBufferSize:"
        PUBLIC_LOG_ZU, streamId, ringBufferSize);
    ringBufferCapcity_ = ringBufferSize;
    waterLineAbove_ = PLAY_WATER_LINE;
    buffer_ = std::make_shared<RingBuffer>(ringBufferSize);
    buffer_->Init();

    downloader_ = std::make_shared<Downloader>("dashSegment");

    dataSave_ =  [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };

    downloadRequest_ = nullptr;
    mediaSegment_ = nullptr;
    recordData_ = std::make_shared<RecordData>();
}

DashSegmentDownloader::~DashSegmentDownloader() noexcept
{
    downloadRequest_ = nullptr;
    downloader_ = nullptr;
    mediaSegment_ = nullptr;
    buffer_ = nullptr;
    segmentList_.clear();
}

bool DashSegmentDownloader::Open(const std::shared_ptr<DashSegment>& dashSegment)
{
    std::lock_guard<std::mutex> lock(segmentMutex_);
    steadyClock_.Reset();
    lastCheckTime_ = 0;
    downloadDuringTime_ = 0;
    totalDownloadDuringTime_ = 0;
    downloadBits_ = 0;
    totalBits_ = 0;
    lastBits_ = 0;
    mediaSegment_ = std::make_shared<DashBufferSegment>(dashSegment);
    if (mediaSegment_->byteRange_.length() > 0) {
        DashParseRange(mediaSegment_->byteRange_, mediaSegment_->startRangeValue_, mediaSegment_->endRangeValue_);
    }

    if (mediaSegment_->startRangeValue_ >= 0 && mediaSegment_->endRangeValue_ > mediaSegment_->startRangeValue_) {
        mediaSegment_->contentLength_ = static_cast<size_t>(mediaSegment_->endRangeValue_ -
                                                            mediaSegment_->startRangeValue_ + 1);
    }
    segmentList_.push_back(mediaSegment_);
    MEDIA_LOG_I("Open enter streamId:" PUBLIC_LOG_D32 " ,seqNum:" PUBLIC_LOG_D64 ", range=" PUBLIC_LOG_D64 "-"
        PUBLIC_LOG_D64, mediaSegment_->streamId_, mediaSegment_->numberSeq_,
        mediaSegment_->startRangeValue_, mediaSegment_->endRangeValue_);

    std::shared_ptr<DashInitSegment> initSegment = GetDashInitSegment(streamId_);
    if (initSegment != nullptr && initSegment->writeState_ == INIT_SEGMENT_STATE_UNUSE) {
        MEDIA_LOG_I("Open streamId:" PUBLIC_LOG_D32 ", writeState:"
            PUBLIC_LOG_D32, streamId_, initSegment->writeState_);
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
    downloader_->Stop(isAsync);

    if (downloadRequest_ != nullptr && !downloadRequest_->IsClosed()) {
        downloadRequest_->Close();
    }
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

DashReadRet DashSegmentDownloader::Read(uint8_t *buff, ReadDataInfo &readDataInfo,
                                        const std::atomic<bool> &isInterruptNeeded)
{
    FALSE_RETURN_V_MSG(buffer_ != nullptr, DASH_READ_FAILED, "buffer is null");
    FALSE_RETURN_V_MSG(!isInterruptNeeded.load(), DASH_READ_INTERRUPT, "isInterruptNeeded");
    int32_t streamId = readDataInfo.streamId_;
    uint32_t wantReadLength = readDataInfo.wantReadLength_;
    uint32_t &realReadLength = readDataInfo.realReadLength_;
    int32_t &realStreamId = readDataInfo.nextStreamId_;
    FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, DASH_READ_FAILED, "wantReadLength_ <= 0");

    DashReadRet readRet = DASH_READ_OK;
    if (CheckReadInterrupt(realReadLength, wantReadLength, readRet, isInterruptNeeded)) {
        return readRet;
    }

    std::shared_ptr<DashBufferSegment> currentSegment = GetCurrentSegment();
    int32_t currentStreamId = streamId;
    if (currentSegment != nullptr) {
        currentStreamId = currentSegment->streamId_;
    }
    realStreamId = currentStreamId;
    if (realStreamId != streamId) {
        MEDIA_LOG_I("Read: changed stream streamId:" PUBLIC_LOG_D32 ", realStreamId:"
            PUBLIC_LOG_D32, streamId, realStreamId);
        UpdateInitSegmentState(currentStreamId);
        return readRet;
    }

    if (ReadInitSegment(buff, wantReadLength, realReadLength, currentStreamId)) {
        return DASH_READ_OK;
    }

    uint32_t maxReadLength = GetMaxReadLength(wantReadLength, currentSegment, currentStreamId);
    realReadLength = buffer_->ReadBuffer(buff, maxReadLength, DEFAULT_WAIT_TIME);
    if (realReadLength == 0) {
        MEDIA_LOG_W("After read: streamId:" PUBLIC_LOG_D32 " ,bufferHead:" PUBLIC_LOG_ZU ", bufferTail:" PUBLIC_LOG_ZU
            ", realReadLength:" PUBLIC_LOG_U32, currentStreamId, buffer_->GetHead(), buffer_->GetTail(),
            realReadLength);
        return DASH_READ_AGAIN;
    }

    MEDIA_LOG_D("After read: streamId:" PUBLIC_LOG_D32 " ,bufferHead:" PUBLIC_LOG_ZU ", bufferTail:" PUBLIC_LOG_ZU
            ", realReadLength:" PUBLIC_LOG_U32, currentStreamId, buffer_->GetHead(), buffer_->GetTail(),
            realReadLength);
    ClearReadSegmentList();
    return readRet;
}

uint32_t DashSegmentDownloader::GetMaxReadLength(uint32_t wantReadLength,
                                                 const std::shared_ptr<DashBufferSegment> &currentSegment,
                                                 int32_t currentStreamId) const
{
    uint32_t maxReadLength = wantReadLength;
    if (currentSegment != nullptr) {
        uint32_t availableSize = currentSegment->bufferPosTail_ - buffer_->GetHead();
        if (availableSize > 0) {
            maxReadLength = availableSize;
        }
    }
    maxReadLength = maxReadLength > wantReadLength ? wantReadLength : maxReadLength;
    MEDIA_LOG_D("Read: streamId:" PUBLIC_LOG_D32 " limit, bufferHead:" PUBLIC_LOG_ZU ", bufferTail:" PUBLIC_LOG_ZU
        ", maxReadLength:" PUBLIC_LOG_U32, currentStreamId, buffer_->GetHead(), buffer_->GetTail(), maxReadLength);
    return maxReadLength;
}

bool DashSegmentDownloader::IsSegmentFinished(uint32_t &realReadLength, DashReadRet &readRet)
{
    if (isAllSegmentFinished_.load()) {
        readRet = DASH_READ_SEGMENT_DOWNLOAD_FINISH;
        if (buffer_->GetSize() == 0) {
            readRet = DASH_READ_END;
            realReadLength = 0;
            if (mediaSegment_ != nullptr) {
                MEDIA_LOG_I("Read: streamId:" PUBLIC_LOG_D32 " segment "
                    PUBLIC_LOG_D64 " read Eos", mediaSegment_->streamId_, mediaSegment_->numberSeq_);
            }
            return true;
        }
    }
    return false;
}

bool DashSegmentDownloader::CheckReadInterrupt(uint32_t &realReadLength, uint32_t wantReadLength, DashReadRet &readRet,
                                               const std::atomic<bool> &isInterruptNeeded)
{
    if (IsSegmentFinished(realReadLength, readRet)) {
        return true;
    }

    if (HandleBuffering(isInterruptNeeded)) {
        MEDIA_LOG_E("DashSegmentDownloader read return error again streamId: " PUBLIC_LOG_D32, streamId_);
        readRet = DASH_READ_AGAIN;
        return true;
    }

    bool arrivedBuffering = streamType_ != MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE && isFirstFrameArrived_ &&
        buffer_->GetSize() < static_cast<size_t>(PLAY_WATER_LINE);
    if (arrivedBuffering && !isAllSegmentFinished_.load()) {
        if (HandleCache()) {
            readRet = DASH_READ_AGAIN;
            return true;
        }
    }

    if (isInterruptNeeded.load()) {
        realReadLength = 0;
        readRet = DASH_READ_INTERRUPT;
        MEDIA_LOG_I("DashSegmentDownloader interruptNeeded streamId: " PUBLIC_LOG_D32, streamId_);
        return true;
    }
    return false;
}

bool DashSegmentDownloader::HandleBuffering(const std::atomic<bool> &isInterruptNeeded)
{
    if (!isBuffering_.load()) {
        return false;
    }

    MEDIA_LOG_I("HandleBuffering begin streamId: " PUBLIC_LOG_D32, streamId_);
    int32_t sleepTime = 0;
    while (!isInterruptNeeded.load()) {
        if (!isBuffering_.load()) {
            break;
        }

        if (isAllSegmentFinished_.load()) {
            isBuffering_.store(false);
            break;
        }
        OSAL::SleepFor(BUFFERING_SLEEP_TIME_MS);
        sleepTime += BUFFERING_SLEEP_TIME_MS;
        if (sleepTime > BUFFERING_TIME_OUT_MS) {
            break;
        }
    }
    MEDIA_LOG_I("HandleBuffering end streamId: " PUBLIC_LOG_D32 "isBuffering: "
        PUBLIC_LOG_D32, streamId_, isBuffering_.load());
    return isBuffering_.load();
}

void DashSegmentDownloader::SaveDataHandleBuffering()
{
    if (!isBuffering_.load()) {
        return;
    }
    UpdateCachedPercent(BufferingInfoType::BUFFERING_PERCENT);
    if (buffer_->GetSize() >= waterLineAbove_ || isAllSegmentFinished_.load()) {
        isBuffering_.store(false);
    }

    if (!isBuffering_.load() && callback_ != nullptr) {
        MEDIA_LOG_I("SaveDataHandleBuffering OnEvent streamId: " PUBLIC_LOG_D32 " cacheData buffering end", streamId_);
        UpdateCachedPercent(BufferingInfoType::BUFFERING_END);
        callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
    }
}

bool DashSegmentDownloader::HandleCache()
{
    waterLineAbove_ = static_cast<size_t>(GetWaterLineAbove());
    if (!isBuffering_.load()) {
        if (callback_ != nullptr) {
            MEDIA_LOG_I("HandleCache OnEvent streamId: " PUBLIC_LOG_D32 " start buffering, waterLineAbove:"
                PUBLIC_LOG_U32, streamId_, waterLineAbove_);
            UpdateCachedPercent(BufferingInfoType::BUFFERING_START);
            callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
        }
        isBuffering_.store(true);
        return true;
    }
    return false;
}

int32_t DashSegmentDownloader::GetWaterLineAbove()
{
    int32_t waterLineAbove = streamType_ == MediaAVCodec::MediaType::MEDIA_TYPE_VID ? DEFAULT_VIDEO_WATER_LINE :
        DEFAULT_AUDIO_WATER_LINE;
    if (downloadRequest_ != nullptr && realTimeBitBate_ > 0) {
        MEDIA_LOG_I("GetWaterLineAbove streamId: " PUBLIC_LOG_D32 " realTimeBitBate: "
            PUBLIC_LOG_D64 " downloadBiteRate: " PUBLIC_LOG_U32, streamId_, realTimeBitBate_, downloadBiteRate_);
        if (downloadBiteRate_ == 0) {
            MEDIA_LOG_I("GetWaterLineAbove streamId: " PUBLIC_LOG_D32 " use default waterLineAbove: "
                PUBLIC_LOG_D32, streamId_, waterLineAbove);
            return waterLineAbove;
        }

        if (realTimeBitBate_ > static_cast<int64_t>(downloadBiteRate_)) {
            waterLineAbove = static_cast<int32_t>(DEFAULT_MAX_CACHE_TIME * realTimeBitBate_ / BYTES_TO_BIT);
        } else {
            waterLineAbove = static_cast<int32_t>(DEFAULT_MIN_CACHE_TIME * realTimeBitBate_ / BYTES_TO_BIT);
        }
        int32_t maxWaterLineAbove = static_cast<int32_t>(ringBufferCapcity_ / 2);
        waterLineAbove = waterLineAbove > maxWaterLineAbove ? maxWaterLineAbove : waterLineAbove;
        int32_t minWaterLineAbove = 2 * PLAY_WATER_LINE;
        waterLineAbove = waterLineAbove < minWaterLineAbove ? minWaterLineAbove : waterLineAbove;
    }
    MEDIA_LOG_I("GetWaterLineAbove streamId: " PUBLIC_LOG_D32 " waterLineAbove: "
        PUBLIC_LOG_D32, streamId_, waterLineAbove);
    return waterLineAbove;
}

void DashSegmentDownloader::CalculateBitRate(size_t fragmentSize, double duration)
{
    if (fragmentSize == 0 || duration == 0) {
        return;
    }

    realTimeBitBate_ = static_cast<int64_t>(fragmentSize * BYTES_TO_BIT * SECOND_TO_MILLIONSECOND) / duration;
    MEDIA_LOG_I("CalculateBitRate streamId: " PUBLIC_LOG_D32 " realTimeBitBate: "
        PUBLIC_LOG_D64, streamId_, realTimeBitBate_);
}

void DashSegmentDownloader::HandleCachedDuration()
{
    if (realTimeBitBate_ <= 0) {
        return;
    }

    uint64_t cachedDuration = static_cast<uint64_t>((static_cast<int64_t>(buffer_->GetSize()) *
        BYTES_TO_BIT * SECOND_TO_MILLIONSECOND) / realTimeBitBate_);
    if ((cachedDuration > lastDurationRecord_ &&
         cachedDuration - lastDurationRecord_ > DURATION_CHANGE_AMOUT_MILLIONSECOND) ||
        (lastDurationRecord_ > cachedDuration &&
         lastDurationRecord_ - cachedDuration > DURATION_CHANGE_AMOUT_MILLIONSECOND)) {
        if (callback_ != nullptr) {
            MEDIA_LOG_D("HandleCachedDuration OnEvent streamId: " PUBLIC_LOG_D32 " cachedDuration: "
                PUBLIC_LOG_U64, streamId_, cachedDuration);
            callback_->OnEvent({PluginEventType::CACHED_DURATION, {cachedDuration}, "buffering_duration"});
        }
        lastDurationRecord_ = cachedDuration;
    }
}

void DashSegmentDownloader::UpdateCachedPercent(BufferingInfoType infoType)
{
    if (waterLineAbove_ == 0 || callback_ == nullptr) {
        MEDIA_LOG_I("OnEvent streamId: " PUBLIC_LOG_D32 " UpdateCachedPercent error", streamId_);
        return;
    }
    if (infoType == BufferingInfoType::BUFFERING_START) {
        callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {0}, "buffer percent"});
        lastCachedSize_ = 0;
        return;
    }
    if (infoType == BufferingInfoType::BUFFERING_END) {
        callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {BUFFERING_PERCENT_FULL}, "buffer percent"});
        lastCachedSize_ = 0;
        return;
    }
    if (infoType != BufferingInfoType::BUFFERING_PERCENT) {
        return;
    }

    uint32_t bufferSize = static_cast<uint32_t>(buffer_->GetSize());
    if (bufferSize < lastCachedSize_) {
        return;
    }
    uint32_t deltaSize = bufferSize - lastCachedSize_;
    if (deltaSize >= UPDATE_CACHE_STEP) {
        uint32_t percent = (bufferSize >= waterLineAbove_) ? BUFFERING_PERCENT_FULL : bufferSize *
            BUFFERING_PERCENT_FULL / waterLineAbove_;
        callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {percent}, "buffer percent"});
        lastCachedSize_ = bufferSize;
    }
}

std::shared_ptr<DashBufferSegment> DashSegmentDownloader::GetCurrentSegment()
{
    std::shared_ptr<DashBufferSegment> currentSegment;
    {
        std::lock_guard<std::mutex> lock(segmentMutex_);
        auto it = std::find_if(segmentList_.begin(), segmentList_.end(),
            [this](const std::shared_ptr<DashBufferSegment> &item) -> bool {
                return buffer_->GetHead() >= item->bufferPosHead_ && buffer_->GetHead() <= item->bufferPosTail_;
            });
        if (it != segmentList_.end()) {
            currentSegment = *it;
        }
    }
    return currentSegment;
}

void DashSegmentDownloader::UpdateInitSegmentState(int32_t currentStreamId)
{
    for (auto it = initSegments_.begin(); it != initSegments_.end(); ++it) {
        if ((*it)->streamId_ == currentStreamId) {
            MEDIA_LOG_I("UpdateInitSegmentState: init streamId:" PUBLIC_LOG_D32 ", contentLen:"
                PUBLIC_LOG_ZU ", readIndex:" PUBLIC_LOG_D32 ", flag:" PUBLIC_LOG_D32 ", readState:"
                PUBLIC_LOG_D32, currentStreamId, (*it)->content_.length(), (*it)->readIndex_,
                (*it)->isDownloadFinish_, (*it)->readState_);
        }
        (*it)->readIndex_ = 0;
        (*it)->readState_ = INIT_SEGMENT_STATE_UNUSE;
    }
}

bool DashSegmentDownloader::ReadInitSegment(uint8_t *buff, uint32_t wantReadLength, uint32_t &realReadLength,
                                            int32_t currentStreamId)
{
    std::shared_ptr<DashInitSegment> initSegment = GetDashInitSegment(currentStreamId);
    if (initSegment != nullptr && initSegment->readState_ != INIT_SEGMENT_STATE_USED) {
        unsigned int contentLen = initSegment->content_.length();
        MEDIA_LOG_I("Read: init streamId:" PUBLIC_LOG_D32 ", contentLen:" PUBLIC_LOG_U32 ", readIndex:"
            PUBLIC_LOG_D32 ", flag:" PUBLIC_LOG_D32 ", readState:"
            PUBLIC_LOG_D32, currentStreamId, contentLen, initSegment->readIndex_,
        initSegment->isDownloadFinish_, initSegment->readState_);
        if (initSegment->readIndex_ == contentLen && initSegment->isDownloadFinish_) {
            // init segment read finish
            initSegment->readState_ = INIT_SEGMENT_STATE_USED;
            initSegment->readIndex_ = 0;
            return true;
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

        MEDIA_LOG_I("after Read: init streamId:" PUBLIC_LOG_D32 ", contentLen:" PUBLIC_LOG_U32 ", readIndex_:"
            PUBLIC_LOG_D32 ", flag:" PUBLIC_LOG_D32, currentStreamId, contentLen, initSegment->readIndex_,
            initSegment->isDownloadFinish_);
        return true;
    }
    return false;
}

void DashSegmentDownloader::ClearReadSegmentList()
{
    std::lock_guard<std::mutex> lock(segmentMutex_);
    for (auto it = segmentList_.begin(); it != segmentList_.end(); ++it) {
        if (buffer_->GetHead() != 0 && (*it)->isEos_ && buffer_->GetHead() >= (*it)->bufferPosTail_) {
            MEDIA_LOG_D("Read:streamId:" PUBLIC_LOG_D32 ", erase numberSeq:"
                PUBLIC_LOG_D64, (*it)->streamId_, (*it)->numberSeq_);
            it = segmentList_.erase(it);
        } else {
            break;
        }
    }
}

void DashSegmentDownloader::SetStatusCallback(StatusCallbackFunc statusCallbackFunc)
{
    statusCallback_ = statusCallbackFunc;
}

void DashSegmentDownloader::SetDownloadDoneCallback(SegmentDownloadDoneCbFunc doneCbFunc)
{
    downloadDoneCbFunc_ = doneCbFunc;
}

size_t DashSegmentDownloader::GetRingBufferInitSize(MediaAVCodec::MediaType streamType) const
{
    size_t ringBufferFixSize = DEFAULT_RING_BUFFER_SIZE;
    auto ringBufferSizeItem = BUFFER_SIZE_MAP.find(streamType);
    if (ringBufferSizeItem != BUFFER_SIZE_MAP.end()) {
        ringBufferFixSize = ringBufferSizeItem->second;
    }

    if (streamType == MediaAVCodec::MediaType::MEDIA_TYPE_VID && userDefinedBufferDuration_) {
        size_t ringBufferSize = expectDuration_ * currentBitrate_;
        if (ringBufferSize < DEFAULT_RING_BUFFER_SIZE) {
            MEDIA_LOG_I("Setting buffer size: " PUBLIC_LOG_ZU ", already lower than the min buffer size: "
                PUBLIC_LOG_U32 ", setting buffer size: "
                PUBLIC_LOG_U32, ringBufferSize, DEFAULT_RING_BUFFER_SIZE, DEFAULT_RING_BUFFER_SIZE);
            ringBufferSize = DEFAULT_RING_BUFFER_SIZE;
        } else if (ringBufferSize > ringBufferFixSize) {
            MEDIA_LOG_I("Setting buffer size: " PUBLIC_LOG_ZU ", already exceed the max buffer size: "
                PUBLIC_LOG_ZU ", setting buffer size: "
                PUBLIC_LOG_ZU, ringBufferSize, ringBufferFixSize, ringBufferFixSize);
            ringBufferSize = ringBufferFixSize;
        }
        return ringBufferSize;
    } else {
        return ringBufferFixSize;
    }
}

void DashSegmentDownloader::SetInitSegment(std::shared_ptr<DashInitSegment> initSegment, bool needUpdateState)
{
    if (initSegment == nullptr) {
        return;
    }

    int streamId = initSegment->streamId_;
    std::shared_ptr<DashInitSegment> dashInitSegment = GetDashInitSegment(streamId);
    if (dashInitSegment == nullptr) {
        initSegments_.push_back(initSegment);
        dashInitSegment = initSegment;
        needUpdateState = true;
    }

    if (!dashInitSegment->isDownloadFinish_) {
        dashInitSegment->writeState_ = INIT_SEGMENT_STATE_UNUSE;
    }

    // seek or first time to set stream init segment should update to UNUSE
    // read will update state to UNUSE when stream id is changed
    if (needUpdateState) {
        dashInitSegment->readState_ = INIT_SEGMENT_STATE_UNUSE;
    }
    MEDIA_LOG_I("SetInitSegment:streamId:" PUBLIC_LOG_D32 ", isDownloadFinish_="
        PUBLIC_LOG_D32 ", readIndex=" PUBLIC_LOG_U32 ", readState_=" PUBLIC_LOG_D32 ", update="
        PUBLIC_LOG_D32 ", writeState_=" PUBLIC_LOG_D32, streamId, dashInitSegment->isDownloadFinish_,
        dashInitSegment->readIndex_, dashInitSegment->readState_, needUpdateState, dashInitSegment->writeState_);
}

void DashSegmentDownloader::UpdateStreamId(int streamId)
{
    streamId_ = streamId;
}

void DashSegmentDownloader::SetCurrentBitRate(int32_t bitRate)
{
    if (bitRate <= 0) {
        realTimeBitBate_ = -1;
    } else {
        realTimeBitBate_ = static_cast<int64_t>(bitRate);
    }
}

void DashSegmentDownloader::SetDemuxerState()
{
    isFirstFrameArrived_ = true;
}

void DashSegmentDownloader::SetAllSegmentFinished()
{
    MEDIA_LOG_I("SetAllSegmentFinished streamId: " PUBLIC_LOG_D32 " download complete", streamId_);
    isAllSegmentFinished_.store(true);
}

int DashSegmentDownloader::GetStreamId() const
{
    return streamId_;
}

MediaAVCodec::MediaType DashSegmentDownloader::GetStreamType() const
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

bool DashSegmentDownloader::GetStartedStatus() const
{
    return startedPlayStatus_;
}

bool DashSegmentDownloader::IsSegmentFinish() const
{
    if (mediaSegment_ != nullptr && mediaSegment_->isEos_) {
        return true;
    }

    return false;
}

bool DashSegmentDownloader::CleanAllSegmentBuffer(bool isCleanAll, int64_t& remainLastNumberSeq)
{
    if (isCleanAll) {
        MEDIA_LOG_I("CleanAllSegmentBuffer clean all");
        isCleaningBuffer_.store(true);
        Close(true, true);
        std::lock_guard<std::mutex> lock(segmentMutex_);
        isAllSegmentFinished_.store(false);
        for (const auto &it: segmentList_) {
            if (it == nullptr || buffer_->GetHead() > it->bufferPosTail_) {
                continue;
            }

            remainLastNumberSeq = it->numberSeq_;
            break;
        }

        downloader_ = std::make_shared<Downloader>("dashSegment");
        buffer_->Clear();
        segmentList_.clear();
        buffer_->SetActive(true);
        return true;
    }

    return false;
}

bool DashSegmentDownloader::CleanSegmentBuffer(bool isCleanAll, int64_t& remainLastNumberSeq)
{
    if (CleanAllSegmentBuffer(isCleanAll, remainLastNumberSeq)) {
        return true;
    }

    size_t clearTail = 0;
    {
        std::lock_guard<std::mutex> lock(segmentMutex_);
        remainLastNumberSeq = -1;
        uint32_t remainDuration = 0;
        for (const auto &it: segmentList_) {
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

        MEDIA_LOG_I("CleanSegmentBuffer:streamId:" PUBLIC_LOG_D32 ", remain numberSeq:"
            PUBLIC_LOG_D64, streamId_, remainLastNumberSeq);
    }

    if (clearTail > 0) {
        isCleaningBuffer_.store(true);
        Close(true, false);
        std::lock_guard<std::mutex> lock(segmentMutex_);
        isAllSegmentFinished_.store(false);
        segmentList_.remove_if([&remainLastNumberSeq](std::shared_ptr<DashBufferSegment> bufferSegment) {
            return (bufferSegment->numberSeq_ > remainLastNumberSeq);
        });

        downloader_ = std::make_shared<Downloader>("dashSegment");
        MEDIA_LOG_I("CleanSegmentBuffer bufferHead:" PUBLIC_LOG_ZU " ,bufferTail:" PUBLIC_LOG_ZU " ,clearTail:"
            PUBLIC_LOG_ZU, buffer_->GetHead(), buffer_->GetTail(), clearTail);
        buffer_->SetTail(clearTail);
        buffer_->SetActive(true);
        return true;
    }
    return false;
}

void DashSegmentDownloader::CleanByTimeInternal(int64_t& remainLastNumberSeq, size_t& clearTail, bool& isEnd)
{
    // residue segment duration
    uint32_t remainDuration = 0;
    uint32_t segmentKeepDuration = 1000; // ms
    uint32_t segmentKeepDelta = 100; // ms
    for (const auto &it: segmentList_) {
        if (it == nullptr ||
            buffer_->GetHead() > it->bufferPosTail_ ||
            it->bufferPosHead_ >= it->bufferPosTail_) {
            continue;
        }

        remainLastNumberSeq = it->numberSeq_;
        isEnd = it->isEos_;
        if (it->contentLength_ == 0 ||
            it->duration_ == 0) {
            MEDIA_LOG_I("CleanByTimeInternal: contentLength is:" PUBLIC_LOG_ZU ", duration is:"
                PUBLIC_LOG_U32, it->contentLength_, it->duration_);
            // can not caculate segment content length, just keep one segment
            clearTail = it->bufferPosTail_;
            break;
        }

        remainDuration = (it->bufferPosTail_ - buffer_->GetHead()) * it->duration_ / it->contentLength_;
        if (remainDuration < segmentKeepDuration + segmentKeepDelta &&
            remainDuration + segmentKeepDelta >= segmentKeepDuration) {
            // find clear buffer position with segment tail position
            clearTail = it->bufferPosTail_;
        } else if (remainDuration < segmentKeepDuration) {
            // get next segment buffer
            clearTail = it->bufferPosTail_;
            segmentKeepDuration -= remainDuration;
            continue;
        } else {
            // find clear buffer position
            uint32_t segmentSize = (segmentKeepDuration * it->contentLength_) / it->duration_;
            if (clearTail > 0) {
                // find clear buffer position in multi segments
                clearTail += segmentSize;
            } else {
                // find clear buffer position in one segment
                clearTail = buffer_->GetHead() + segmentSize;
            }
            it->bufferPosTail_ = clearTail;
            it->isEos_ = true;
            isEnd = false;
        }

        break;
    }
}

bool DashSegmentDownloader::CleanBufferByTime(int64_t& remainLastNumberSeq, bool& isEnd)
{
    Close(true, false);
    std::lock_guard<std::mutex> lock(segmentMutex_);
    remainLastNumberSeq = -1;
    size_t clearTail = 0;
    CleanByTimeInternal(remainLastNumberSeq, clearTail, isEnd);

    if (remainLastNumberSeq == -1 && mediaSegment_ != nullptr) {
        isEnd = false;
    }

    MEDIA_LOG_I("CleanBufferByTime:streamId:" PUBLIC_LOG_D32 ", remain numberSeq:"
        PUBLIC_LOG_D64 ", isEnd:" PUBLIC_LOG_D32 ", size:" PUBLIC_LOG_ZU, streamId_,
        remainLastNumberSeq, isEnd, segmentList_.size());

    if (clearTail > 0) {
        isCleaningBuffer_.store(true);
        segmentList_.remove_if([&remainLastNumberSeq](std::shared_ptr<DashBufferSegment> bufferSegment) {
            return (bufferSegment->numberSeq_ > remainLastNumberSeq);
        });

        downloader_ = std::make_shared<Downloader>("dashSegment");
        MEDIA_LOG_I("CleanBufferByTime bufferHead:" PUBLIC_LOG_ZU " ,bufferTail:" PUBLIC_LOG_ZU " ,clearTail:"
            PUBLIC_LOG_ZU " ,seq:" PUBLIC_LOG_D64 ",size:" PUBLIC_LOG_ZU, buffer_->GetHead(), buffer_->GetTail(),
            clearTail, remainLastNumberSeq, segmentList_.size());
        buffer_->SetTail(clearTail);
        buffer_->SetActive(true);
        return true;
    }
    return false;
}

bool DashSegmentDownloader::SeekToTime(const std::shared_ptr<DashSegment> &segment)
{
    std::lock_guard<std::mutex> lock(segmentMutex_);
    std::shared_ptr<DashBufferSegment> desSegment;
    auto it = std::find_if(segmentList_.begin(), segmentList_.end(),
        [&segment](const std::shared_ptr<DashBufferSegment> &item) -> bool {
            return (item->numberSeq_ - item->startNumberSeq_) == (segment->numberSeq_ - segment->startNumberSeq_);
        });
    if (it != segmentList_.end()) {
        desSegment = *it;
    }

    if (desSegment != nullptr && desSegment->bufferPosTail_ > 0) {
        return buffer_->SetHead(desSegment->bufferPosHead_);
    }
    return false;
}

bool DashSegmentDownloader::SaveData(uint8_t* data, uint32_t len)
{
    MEDIA_LOG_D("SaveData:streamId:" PUBLIC_LOG_D32 ", len:" PUBLIC_LOG_D32, streamId_, len);
    startedPlayStatus_ = true;
    if (streamType_ == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        OnWriteRingBuffer(len);
    }
    std::shared_ptr<DashInitSegment> initSegment = GetDashInitSegment(streamId_);
    if (initSegment != nullptr && initSegment->writeState_ == INIT_SEGMENT_STATE_USING) {
        MEDIA_LOG_I("SaveData:streamId:" PUBLIC_LOG_D32 ", writeState:"
            PUBLIC_LOG_D32, streamId_, initSegment->writeState_);
        initSegment->content_.append(reinterpret_cast<const char*>(data), len);
        return true;
    }

    size_t bufferTail = buffer_->GetTail();
    bool writeRet = buffer_->WriteBuffer(data, len);
    HandleCachedDuration();
    SaveDataHandleBuffering();
    if (!writeRet) {
        MEDIA_LOG_E("SaveData:error streamId:" PUBLIC_LOG_D32 ", len:" PUBLIC_LOG_D32, streamId_, len);
        return writeRet;
    }

    std::lock_guard<std::mutex> lock(segmentMutex_);
    for (const auto &mediaSegment: segmentList_) {
        if (mediaSegment == nullptr || mediaSegment->isEos_) {
            continue;
        }

        if (mediaSegment->bufferPosTail_ == 0) {
            mediaSegment->bufferPosHead_ = bufferTail;
        }
        mediaSegment->bufferPosTail_ = buffer_->GetTail();
        UpdateBufferSegment(mediaSegment, len);
        break;
    }
    return writeRet;
}

void DashSegmentDownloader::UpdateBufferSegment(const std::shared_ptr<DashBufferSegment> &mediaSegment, uint32_t len)
{
    if (mediaSegment->contentLength_ == 0 && downloadRequest_ != nullptr) {
        mediaSegment->contentLength_ = downloadRequest_->GetFileContentLength();
    }

    // last packet len is 0 of chunk
    if (len == 0 || (mediaSegment->contentLength_ > 0 &&
                     mediaSegment->bufferPosTail_ >= (mediaSegment->bufferPosHead_ + mediaSegment->contentLength_))) {
        mediaSegment->isEos_ = true;
        if (mediaSegment->isLast_) {
            MEDIA_LOG_I("AllSegmentFinish streamId: " PUBLIC_LOG_D32 " download complete", streamId_);
            isAllSegmentFinished_.store(true);
        }
        if (mediaSegment->contentLength_ == 0) {
            mediaSegment->contentLength_ = mediaSegment->bufferPosTail_ - mediaSegment->bufferPosHead_;
        }
        MEDIA_LOG_I("SaveData eos:streamId:" PUBLIC_LOG_D32 ", segmentNum:" PUBLIC_LOG_D64 ", contentLength:"
            PUBLIC_LOG_ZU ", bufferPosHead:" PUBLIC_LOG_ZU  " ,bufferPosEnd:" PUBLIC_LOG_ZU,
            streamId_, mediaSegment->numberSeq_, mediaSegment->contentLength_, mediaSegment->bufferPosHead_,
            mediaSegment->bufferPosTail_);
    }
}

void DashSegmentDownloader::OnWriteRingBuffer(uint32_t len)
{
    uint32_t writeBits = len * BYTE_TO_BIT;
    totalBits_ += writeBits;
    uint64_t now = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    if (now > lastCheckTime_ && now - lastCheckTime_ > RECORD_TIME_INTERVAL) {
        uint64_t curDownloadBits = totalBits_ - lastBits_;
        if (curDownloadBits >= RECORD_DOWNLOAD_MIN_BIT) {
            downloadDuringTime_ = now - lastCheckTime_;
            downloadBits_ += curDownloadBits;
            totalDownloadDuringTime_ += downloadDuringTime_;
            double downloadRate = 0;
            double tmpNumerator = static_cast<double>(curDownloadBits);
            double tmpDenominator = static_cast<double>(downloadDuringTime_) / SECOND_TO_MILLIONSECOND;
            if (tmpDenominator > ZERO_THRESHOLD) {
                downloadRate = tmpNumerator / tmpDenominator;
            }
            size_t remainingBuffer = 0;
            if (buffer_ != nullptr) {
                remainingBuffer = buffer_->GetSize();
            }
            MEDIA_LOG_D("Current download speed : " PUBLIC_LOG_D32 " Kbit/s,Current buffer size : " PUBLIC_LOG_U64
                " KByte", static_cast<int32_t>(downloadRate / 1024), static_cast<uint64_t>(remainingBuffer / 1024));
            // Remaining playable time: s
            uint64_t bufferDuration = 0;
            if (realTimeBitBate_ > 0) {
                bufferDuration =
                    remainingBuffer * static_cast<uint64_t>(BYTES_TO_BIT) / static_cast<uint64_t>(realTimeBitBate_);
            } else {
                bufferDuration = static_cast<uint64_t>(remainingBuffer * BYTES_TO_BIT) / currentBitrate_;
            }
            if (recordData_ != nullptr) {
                recordData_->downloadRate = downloadRate;
                recordData_->bufferDuring = bufferDuration;
            }
        }
        lastBits_ = totalBits_;
        lastCheckTime_ = now;
    }
}

uint64_t DashSegmentDownloader::GetDownloadSpeed() const
{
    return static_cast<uint64_t>(downloadSpeed_);
}

void DashSegmentDownloader::GetIp(std::string& ip)
{
    if (downloader_) {
        downloader_->GetIp(ip);
    }
}

bool DashSegmentDownloader::GetDownloadFinishState()
{
    std::shared_ptr<DashInitSegment> finishState = GetDashInitSegment(streamId_);
    if (finishState) {
        return finishState->isDownloadFinish_;
    } else {
        return true;
    }
}

std::pair<int64_t, int64_t> DashSegmentDownloader::GetDownloadRecordData()
{
    std::pair<int64_t, int64_t> recordData;
    if (recordData_ != nullptr) {
        recordData.first = static_cast<int64_t>(recordData_->downloadRate);
        recordData.second = static_cast<int64_t>(recordData_->bufferDuring);
    } else {
        recordData.first = 0;
        recordData.second = 0;
    }
    return recordData;
}

uint32_t DashSegmentDownloader::GetRingBufferSize() const
{
    if (buffer_ != nullptr) {
        return buffer_->GetSize();
    }
    return 0;
}

uint32_t DashSegmentDownloader::GetRingBufferCapacity() const
{
    return ringBufferCapcity_;
}

void DashSegmentDownloader::PutRequestIntoDownloader(unsigned int duration, int64_t startPos, int64_t endPos,
                                                     const std::string &url)
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
    MediaSouce mediaSouce;
    mediaSouce.url = url;
    mediaSouce.timeoutMs = HTTP_TIME_OUT_MS;
    downloadRequest_ = std::make_shared<DownloadRequest>(duration, dataSave_,
                                                         realStatusCallback, mediaSouce, requestWholeFile);
    downloadRequest_->SetDownloadDoneCb(downloadDoneCallback);
    if (!requestWholeFile && (endPos > startPos)) {
        downloadRequest_->SetRangePos(startPos, endPos);
    }
    MEDIA_LOG_I("PutRequestIntoDownloader:range=" PUBLIC_LOG_D64 "-" PUBLIC_LOG_D64, startPos, endPos);

    isCleaningBuffer_.store(false);
    if (downloader_ != nullptr) {
        downloader_->Download(downloadRequest_, -1); // -1
        downloader_->Start();
    }
}

void DashSegmentDownloader::UpdateDownloadFinished(const std::string& url, const std::string& location)
{
    MEDIA_LOG_I("UpdateDownloadFinished:streamId:" PUBLIC_LOG_D32, streamId_);
    std::shared_ptr<DashInitSegment> initSegment = GetDashInitSegment(streamId_);
    if (totalDownloadDuringTime_ > 0) {
        double tmpNumerator = static_cast<double>(downloadBits_);
        double tmpDenominator = static_cast<double>(totalDownloadDuringTime_);
        double downloadRate = tmpNumerator / tmpDenominator;
        downloadSpeed_ = downloadRate * SPEED_MULTI_FACT;
    } else {
        downloadSpeed_ = 0;
    }

    if (initSegment != nullptr && initSegment->writeState_ == INIT_SEGMENT_STATE_USING) {
        MEDIA_LOG_I("UpdateDownloadFinished:streamId:" PUBLIC_LOG_D32 ", writeState:"
            PUBLIC_LOG_D32, streamId_, initSegment->writeState_);
        initSegment->writeState_ = INIT_SEGMENT_STATE_USED;
        initSegment->isDownloadFinish_ = true;
        if (mediaSegment_ != nullptr) {
            PutRequestIntoDownloader(mediaSegment_->duration_, mediaSegment_->startRangeValue_,
                                     mediaSegment_->endRangeValue_, mediaSegment_->url_);
            return;
        }
    }

    if (mediaSegment_ != nullptr) {
        if (mediaSegment_->contentLength_ == 0 && downloadRequest_ != nullptr) {
            mediaSegment_->contentLength_ = downloadRequest_->GetFileContentLength();
        }
        if (downloadRequest_ != nullptr) {
            size_t fragmentSize = mediaSegment_->contentLength_;
            double duration = downloadRequest_->GetDuration();
            CalculateBitRate(fragmentSize, duration);
            downloadBiteRate_ = downloadRequest_->GetBitRate();
        }
        mediaSegment_->isEos_ = true;
        if (mediaSegment_->isLast_) {
            MEDIA_LOG_I("AllSegmentFinish streamId: " PUBLIC_LOG_D32 " download complete", streamId_);
            isAllSegmentFinished_.store(true);
        }
        MEDIA_LOG_I("UpdateDownloadFinished: segmentNum:" PUBLIC_LOG_D64 ", contentLength:" PUBLIC_LOG_ZU
            ", isCleaningBuffer:" PUBLIC_LOG_D32 " isLast: " PUBLIC_LOG_D32, mediaSegment_->numberSeq_,
            mediaSegment_->contentLength_, isCleaningBuffer_.load(), mediaSegment_->isLast_);
    }

    SaveDataHandleBuffering();
    if (downloadDoneCbFunc_ && !isCleaningBuffer_.load()) {
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
    auto it = std::find_if(initSegments_.begin(), initSegments_.end(),
        [&streamId](const std::shared_ptr<DashInitSegment> &initSegment) -> bool {
            return initSegment != nullptr && initSegment->streamId_ == streamId;
        });
    if (it != initSegments_.end()) {
        segment = *it;
    }
    return segment;
}

void DashSegmentDownloader::SetInterruptState(bool isInterruptNeeded)
{
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
}

}
}
}
}