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

#ifndef HISTREAMER_DASH_SEGMENT_DOWNLOADER_H
#define HISTREAMER_DASH_SEGMENT_DOWNLOADER_H

#include <memory>
#include <list>
#include <mutex>
#include <atomic>
#include "dash_common.h"
#include "download/downloader.h"
#include "media_downloader.h"
#include "common/media_core.h"
#include "osal/utils/ring_buffer.h"
#include "osal/utils/steady_clock.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

enum DashReadRet {
    DASH_READ_FAILED = 0,
    DASH_READ_OK = 1,
    DASH_READ_SEGMENT_DOWNLOAD_FINISH = 2,
    DASH_READ_END = 3, // segment download finish and buffer read finish
    DASH_READ_AGAIN = 4,
    DASH_READ_INTERRUPT = 5
};

struct DashBufferSegment {
    DashBufferSegment()
    {
        streamId_ = -1;
        duration_ = 0;
        bandwidth_ = 0;
        startNumberSeq_ = 1;
        numberSeq_ = 1;
        startRangeValue_ = 0;
        endRangeValue_ = 0;
        bufferPosHead_ = 0;
        bufferPosTail_ = 0;
        contentLength_ = 0;
        isEos_ = false;
        isLast_ = false;
    }

    DashBufferSegment(const DashBufferSegment& srcSegment)
    {
        streamId_ = srcSegment.streamId_;
        duration_ = srcSegment.duration_;
        bandwidth_ = srcSegment.bandwidth_;
        startNumberSeq_ = srcSegment.startNumberSeq_;
        numberSeq_ = srcSegment.numberSeq_;
        startRangeValue_ = srcSegment.startRangeValue_;
        endRangeValue_ = srcSegment.endRangeValue_;
        url_ = srcSegment.url_;
        byteRange_ = srcSegment.byteRange_;
        bufferPosHead_ = srcSegment.bufferPosHead_;
        bufferPosTail_ = srcSegment.bufferPosTail_;
        contentLength_ = srcSegment.contentLength_;
        isEos_ = srcSegment.isEos_;
        isLast_ = srcSegment.isLast_;
    }

    DashBufferSegment& operator=(const DashBufferSegment& srcSegment)
    {
        if (this != &srcSegment) {
            streamId_ = srcSegment.streamId_;
            duration_ = srcSegment.duration_;
            bandwidth_ = srcSegment.bandwidth_;
            startNumberSeq_ = srcSegment.startNumberSeq_;
            numberSeq_ = srcSegment.numberSeq_;
            startRangeValue_ = srcSegment.startRangeValue_;
            endRangeValue_ = srcSegment.endRangeValue_;
            url_ = srcSegment.url_;
            byteRange_ = srcSegment.byteRange_;
            bufferPosHead_ = srcSegment.bufferPosHead_;
            bufferPosTail_ = srcSegment.bufferPosTail_;
            contentLength_ = srcSegment.contentLength_;
            isEos_ = srcSegment.isEos_;
            isLast_ = srcSegment.isLast_;
        }
        return *this;
    }

    explicit DashBufferSegment(const std::shared_ptr<DashSegment> &dashSegment)
    {
        streamId_ = dashSegment->streamId_;
        duration_ = dashSegment->duration_;
        bandwidth_ = dashSegment->bandwidth_;
        startNumberSeq_ = dashSegment->startNumberSeq_;
        numberSeq_ = dashSegment->numberSeq_;
        startRangeValue_ = dashSegment->startRangeValue_;
        endRangeValue_ = dashSegment->endRangeValue_;
        url_ = dashSegment->url_;
        byteRange_ = dashSegment->byteRange_;
        bufferPosHead_ = 0;
        bufferPosTail_ = 0;
        contentLength_ = 0;
        isEos_ = false;
        isLast_ = dashSegment->isLast_;
    }

    int32_t streamId_;
    unsigned int duration_;
    unsigned int bandwidth_;
    int64_t startNumberSeq_;
    int64_t numberSeq_;
    int64_t startRangeValue_;
    int64_t endRangeValue_;
    std::string url_;
    std::string byteRange_;
    size_t bufferPosHead_;
    size_t bufferPosTail_;
    size_t contentLength_;
    bool isEos_;
    bool isLast_;
};

using SegmentDownloadDoneCbFunc = std::function<void(int streamId)>;

class DashSegmentDownloader {
public:
    DashSegmentDownloader(Callback *callback, int streamId, MediaAVCodec::MediaType streamType,
                          uint64_t expectDuration);
    virtual ~DashSegmentDownloader();

    bool Open(const std::shared_ptr<DashSegment> &dashSegment);
    void Close(bool isAsync, bool isClean);
    void Pause();
    void Resume();
    DashReadRet Read(uint8_t *buff, ReadDataInfo &readDataInfo, const std::atomic<bool> &isInterruptNeeded);
    void SetStatusCallback(StatusCallbackFunc statusCallbackFunc);
    void SetDownloadDoneCallback(SegmentDownloadDoneCbFunc doneCbFunc);
    bool CleanSegmentBuffer(bool isCleanAll, int64_t& remainLastNumberSeq);
    bool CleanBufferByTime(int64_t& remainLastNumberSeq, bool& isEnd);
    bool SeekToTime(const std::shared_ptr<DashSegment>& segment);
    void SetInitSegment(std::shared_ptr<DashInitSegment> initSegment, bool needUpdateState = false);
    void UpdateStreamId(int streamId);
    void SetCurrentBitRate(int32_t bitRate);
    void SetDemuxerState();
    void SetAllSegmentFinished();
    int GetStreamId() const;
    MediaAVCodec::MediaType GetStreamType() const;
    size_t GetContentLength();
    bool GetStartedStatus() const;
    bool IsSegmentFinish() const;
    uint64_t GetDownloadSpeed() const;
    uint32_t GetRingBufferSize() const;
    uint32_t GetRingBufferCapacity() const;
    void GetIp(std::string& ip);
    bool GetDownloadFinishState();
    std::pair<int64_t, int64_t> GetDownloadRecordData();
    void SetInterruptState(bool isInterruptNeeded);

private:
    bool SaveData(uint8_t* data, uint32_t len);
    void PutRequestIntoDownloader(unsigned int duration, int64_t startPos, int64_t endPos, const std::string &url);
    void UpdateDownloadFinished(const std::string& url, const std::string& location);
    uint32_t GetSegmentRemainDuration(const std::shared_ptr<DashBufferSegment>& currentSegment);
    std::shared_ptr<DashInitSegment> GetDashInitSegment(int32_t streamId);
    bool CleanAllSegmentBuffer(bool isCleanAll, int64_t& remainLastNumberSeq);
    void CleanByTimeInternal(int64_t& remainLastNumberSeq, size_t& clearTail, bool& isEnd);
    void ClearReadSegmentList();
    void UpdateInitSegmentState(int32_t currentStreamId);
    bool ReadInitSegment(uint8_t *buff, uint32_t wantReadLength, uint32_t &realReadLength,
                         int32_t currentStreamId);
    std::shared_ptr<DashBufferSegment> GetCurrentSegment();
    bool IsSegmentFinished(uint32_t &realReadLength, DashReadRet &readRet);
    bool CheckReadInterrupt(uint32_t &realReadLength, uint32_t wantReadLength, DashReadRet &readRet,
                            const std::atomic<bool> &isInterruptNeeded);
    uint32_t GetMaxReadLength(uint32_t wantReadLength, const std::shared_ptr<DashBufferSegment> &currentSegment,
                              int32_t currentStreamId) const;
    size_t GetRingBufferInitSize(MediaAVCodec::MediaType streamType) const;
    void OnWriteRingBuffer(uint32_t len);
    bool HandleBuffering(const std::atomic<bool> &isInterruptNeeded);
    void SaveDataHandleBuffering();
    bool HandleCache();
    void HandleCachedDuration();
    int32_t GetWaterLineAbove();
    void CalculateBitRate(size_t fragmentSize, double duration);
    void UpdateCachedPercent(BufferingInfoType infoType);
    void UpdateBufferSegment(const std::shared_ptr<DashBufferSegment> &mediaSegment, uint32_t len);

private:
    static constexpr uint32_t MIN_RETENTION_DURATION_MS = 5 * 1000;
    std::shared_ptr<RingBuffer> buffer_;
    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<DownloadRequest> downloadRequest_;
    std::shared_ptr<DashBufferSegment> mediaSegment_;
    std::list<std::shared_ptr<DashBufferSegment>> segmentList_;
    std::vector<std::shared_ptr<DashInitSegment>> initSegments_;
    std::mutex segmentMutex_;
    DataSaveFunc dataSave_;
    StatusCallbackFunc statusCallback_;
    SegmentDownloadDoneCbFunc downloadDoneCbFunc_;
    bool startedPlayStatus_{false};
    int streamId_{0};
    MediaAVCodec::MediaType streamType_;
    std::atomic<bool> isCleaningBuffer_{false};

    // support ringbuffer size of duration
    uint64_t currentBitrate_{1 * 1024 * 1024};
    bool userDefinedBufferDuration_{false};
    uint64_t expectDuration_{0};

    uint32_t ringBufferCapcity_ {0};
    uint64_t lastCheckTime_ {0};
    uint64_t totalBits_ {0};
    uint64_t lastBits_ {0};
    double downloadSpeed_ {0};
    uint64_t downloadDuringTime_ {0};
    uint64_t downloadBits_ {0};
    uint64_t totalDownloadDuringTime_ {0};
    struct RecordData {
        double downloadRate {0};
        uint64_t bufferDuring {0};
    };
    std::shared_ptr<RecordData> recordData_;
    SteadyClock steadyClock_;

    // play water line
    Callback* callback_{nullptr};
    uint32_t waterLineAbove_{0};
    std::atomic<bool> isBuffering_{false};
    uint32_t downloadBiteRate_{0};
    int64_t realTimeBitBate_{0};
    uint64_t lastDurationRecord_{0};
    uint32_t lastCachedSize_{0};
    bool isFirstFrameArrived_{false};
    std::atomic<bool> isAllSegmentFinished_{false};
};
}
}
}
}
#endif