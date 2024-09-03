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
 
#ifndef HISTREAMER_HTTP_MEDIA_DOWNLOADER_H
#define HISTREAMER_HTTP_MEDIA_DOWNLOADER_H

#include <string>
#include <memory>
#include "osal/utils/ring_buffer.h"
#include "osal/utils/steady_clock.h"
#include "download/downloader.h"
#include "media_downloader.h"
#include "common/media_source.h"
#include "timer.h"
#include "utils/media_cached_buffer.h"
#include <unistd.h>
#include "common/media_core.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HttpMediaDownloader : public MediaDownloader {
public:
    explicit HttpMediaDownloader(std::string url);
    explicit HttpMediaDownloader(std::string url, uint32_t expectBufferDuration);
    ~HttpMediaDownloader() override;
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override;
    void Close(bool isAsync) override;
    void Pause() override;
    void Resume() override;
    Status Read(unsigned char* buff, ReadDataInfo& readDataInfo) override;
    bool SeekToPos(int64_t offset) override;
    size_t GetContentLength() const override;
    int64_t GetDuration() const override;
    Seekable GetSeekable() const override;
    void SetCallback(Callback* cb) override;
    void SetStatusCallback(StatusCallbackFunc cb) override;
    bool GetStartedStatus() override;
    void SetReadBlockingFlag(bool isReadBlockingAllowed) override;
    void SetDemuxerState(int32_t streamId) override;
    void SetDownloadErrorState() override;
    void SetInterruptState(bool isInterruptNeeded) override;
    void GetDownloadInfo(DownloadInfo& downloadInfo) override;
    std::pair<int32_t, int32_t> GetDownloadInfo() override;
    void GetPlaybackInfo(PlaybackInfo& playbackInfo) override;
    int GetBufferSize();
    RingBuffer& GetBuffer();
    bool GetReadFrame();
    bool GetDownloadErrorState();
    StatusCallbackFunc GetStatusCallbackFunc();
    std::pair<int32_t, int32_t> GetDownloadRateAndSpeed();
    void OnWriteBuffer(uint32_t len);
    void DownloadReport();
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) override;
    void UpdateCachedPercent(BufferingInfoType infoType);

private:
    bool SaveData(uint8_t* data, uint32_t len);
    bool SaveCacheBufferData(uint8_t* data, uint32_t len);
    Status ReadDelegate(unsigned char* buff, ReadDataInfo& readDataInfo);
    bool SaveRingBufferData(uint8_t* data, uint32_t len);
    void OnClientErrorEvent();
    Status CheckIsEosRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);
    Status CheckIsEosCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);
    bool HandleSeekHit(int64_t offest);
    Status HandleDownloadErrorState(unsigned int& realReadLength);
    Status ReadRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);
    Status ReadCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);
    bool SeekRingBuffer(int64_t offset);
    bool SeekCacheBuffer(int64_t offset);
    void InitRingBuffer(uint32_t expectBufferDuration);
    void InitCacheBuffer(uint32_t expectBufferDuration);

    bool HandleBuffering();
    bool StartBuffering(int32_t wantReadLength);
    size_t GetCurrentBufferSize();
    bool HandleBreak();
    void ChangeDownloadPos();
    int32_t GetWaterLineAbove();
    void HandleCachedDuration();
    bool CheckBufferingOneSeconds();
    double CalculateCurrentDownloadSpeed();

private:
    std::shared_ptr<RingBuffer> buffer_;
    std::shared_ptr<CacheMediaChunkBufferImpl> cacheMediaBuffer_;
    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<DownloadRequest> downloadRequest_;
    Mutex mutex_;
    ConditionVariable cvReadWrite_;
    Callback* callback_ {nullptr};
    StatusCallbackFunc statusCallback_ {nullptr};
    bool aboveWaterline_ {false};
    bool startedPlayStatus_ {false};
    uint64_t readTime_ {0};
    bool isTimeOut_ {false};
    bool downloadErrorState_ {false};
    std::atomic<bool> isInterruptNeeded_{false};
    int totalBufferSize_ {0};
    SteadyClock steadyClock_;
    uint64_t totalBits_ {0};
    uint64_t lastBits_ {0};
    uint64_t downloadBits_ {0};
    int64_t openTime_ {0};
    int64_t startDownloadTime_ {0};
    int64_t playDelayTime_ {0};
    int64_t lastCheckTime_ {0};
    double avgDownloadSpeed_ {0};
    bool isDownloadFinish_ {false};
    double avgSpeedSum_ {0};
    uint32_t recordSpeedCount_ {0};
    int64_t lastReportUsageTime_ {0};
    uint64_t dataUsage_ {0};
    bool isFlv_ {false};
    size_t readOffset_ {0};
    size_t writeOffset_ {0};
    std::atomic<bool> canWrite_ {true};
    std::atomic<bool> isNeedClean_ {false};
    std::atomic<bool> isHitSeeking_ {false};
    std::atomic<bool> isNeedDropData_ {false};
    std::atomic<bool> isServerAcceptRange_ {false};
    size_t waterLineAbove_ {0};
    bool isInterrupt_ {false};
    bool isBuffering_ {false};
    bool isFirstFrameArrived_ {false};

    struct RecordData {
        double downloadRate {0};
        uint64_t bufferDuring {0};
    };
    std::shared_ptr<RecordData> recordData_;
    uint64_t currentBitrate_ {1 * 1024 * 1024};         //bps
    uint64_t lastReadCheckTime_ {0};
    uint64_t readTotalBytes_ {0};
    uint64_t readRecordDuringTime_ {0};
    uint64_t downloadDuringTime_ {0}; // 有效下载时长 ms
    uint64_t totalDownloadDuringTime_ {0};
    int32_t currentBitRate_ {0};
    uint64_t lastDurationReacord_ {0};
    int32_t lastCachedSize_ {0};
};
}
}
}
}
#endif