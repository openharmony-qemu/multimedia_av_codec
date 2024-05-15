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
#include "dash_common.h"
#include "download/downloader.h"
#include "osal/utils/ring_buffer.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

enum DashReadRet {
    DASH_READ_FAILED = 0,
    DASH_READ_OK,
    DASH_READ_SEGMENT_DOWNLOAD_FINISH,
    DASH_READ_END, // segment download finish and buffer read finish
    DASH_READ_TIMEOUT
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
    }

    DashBufferSegment(const std::shared_ptr<DashSegment> &dashSegment)
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
};

using SegmentDownloadDoneCbFunc = std::function<void(int streamId)>;

class DashSegmentDownloader {
public:
    DashSegmentDownloader(int streamId, MediaAVCodec::MediaType streamType);
    virtual ~DashSegmentDownloader();

    bool Open(const std::shared_ptr<DashSegment> &dashSegment);
    void Close(bool isAsync, bool isClean);
    void Pause();
    void Resume();
    void CloseRequest();
    DashReadRet Read(int32_t streamId, uint8_t* buff, uint32_t wantReadLength, uint32_t& realReadLength, int32_t& realStreamId);
    void SetStatusCallback(StatusCallbackFunc statusCallbackFunc);
    void SetDownloadDoneCallback(SegmentDownloadDoneCbFunc doneCbFunc);
    bool CleanSegmentBuffer(bool isCleanAll, int64_t& remainLastNumberSeq);
    bool SeekToTime(const std::shared_ptr<DashSegment>& segment);
    int64_t GetDownloadLength();
    void SetInitSegment(std::shared_ptr<DashInitSegment> initSegment);
    void UpdateStreamId(int streamId);
    int GetStreamId();
    MediaAVCodec::MediaType GetStreamType();
    size_t GetContentLength();
    bool GetStartedStatus();
    bool IsSegmentFinish();

private:
    bool SaveData(uint8_t* data, uint32_t len);
    void PutRequestIntoDownloader(unsigned int duration, int64_t startPos, int64_t endPos, std::string url);
    void UpdateDownloadFinished(const std::string& url, const std::string& location);
    uint32_t GetSegmentRemainDuration(const std::shared_ptr<DashBufferSegment>& currentSegment);
    std::shared_ptr<DashInitSegment> GetDashInitSegment(int32_t streamId);

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
    int64_t downloadLength_{0};
    int streamId_{0};
    MediaAVCodec::MediaType streamType_;
    uint64_t readTime_ {0};
    bool isCleaningBuffer_{false};
};
}
}
}
}
#endif