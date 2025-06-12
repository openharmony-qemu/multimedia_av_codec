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
#include "utils/write_bitrate_caculator.h"
#include "osal/task/mutex.h"
#include "osal/task/condition_variable.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HttpMediaDownloader : public MediaDownloader {
public:
    explicit HttpMediaDownloader(std::string url, uint32_t expectBufferDuration,
                                 std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader = nullptr);
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
    RingBuffer& GetBuffer();
    bool GetReadFrame();
    bool GetDownloadErrorState();
    StatusCallbackFunc GetStatusCallbackFunc();
    std::pair<int32_t, int32_t> GetDownloadRateAndSpeed();
    void OnWriteBuffer(uint32_t len);
    void DownloadReport();
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) override;
    void UpdateCachedPercent(BufferingInfoType infoType);
    size_t GetBufferSize() const override;
    bool GetPlayable() override;
    bool GetBufferingTimeOut() override;
    bool GetReadTimeOut(bool isDelay) override;
    void SetAppUid(int32_t appUid) override;
    Status StopBufferring(bool isAppBackground) override;
    void WaitForBufferingEnd() override;
    void SetIsReportedErrorCode() override;
    bool IsNotRetry(const std::shared_ptr<DownloadRequest>& request) override
    {
        if (isRingBuffer_ && isSelectingBitrate_.load()) {
            return false;
        }
        std::string location = "";
        request->GetLocation(location);
        if (isRingBuffer_ && !location.empty()) {
            return false;
        }
        return isRingBuffer_ && request->GetFileContentLengthNoWait() == 0 && !isAllowResume_.load();
    }
    bool SetInitialBufferSize(int32_t offset, int32_t size) override;
    void SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy) override;
    void NotifyInitSuccess() override;
    void SetStartPts(int64_t startPts) override;
    void SetExtraCache(uint64_t cacheDuration) override;
    bool SelectBitRate(uint32_t bitRate) override;
    bool AutoSelectBitRate(uint32_t bitRate) override;
    void SetMediaStreams(const MediaStreamList& mediaStreams) override;
    uint64_t GetCachedDuration() override;
    void RestartAndClearBuffer() override;
    bool IsFlvLive() override;
    uint64_t GetMemorySize() override;
    std::string GetContentType() override;
    void SetIsTriggerAutoMode(bool isAuto) override;
    void ClearBuffer() override;

private:
    uint32_t SaveData(uint8_t* data, uint32_t len, bool notBlock);
    uint32_t SaveCacheBufferData(uint8_t* data, uint32_t len, bool notBlock);
    Status ReadDelegate(unsigned char* buff, ReadDataInfo& readDataInfo);
    uint32_t SaveRingBufferData(uint8_t* data, uint32_t len, bool notBlock);
    void OnClientErrorEvent();
    Status CheckIsEosRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);
    Status CheckIsEosCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);
    bool HandleSeekHit(int64_t offest);
    Status ReadRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);
    Status ReadCacheBufferLoop(unsigned char* buff, ReadDataInfo& readDataInfo);
    Status ReadCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);
    bool SeekRingBuffer(int64_t offset);
    bool SeekCacheBuffer(int64_t offset);
    void InitRingBuffer(uint32_t expectBufferDuration);
    void InitCacheBuffer(uint32_t expectBufferDuration);

    Status HandleRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);
    Status HandleCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);

    bool HandleBuffering();
    bool StartBuffering(unsigned int& wantReadLength);
    size_t GetCurrentBufferSize() const;
    bool HandleBreak();
    bool ChangeDownloadPos(bool isSeekHit);
    void UpdateWaterLineAbove();
    void HandleCachedDuration();
    bool CheckBufferingOneSeconds();
    double CalculateCurrentDownloadSpeed();
    float GetCacheDuration(float ratio);
    void HandleDownloadWaterLine();
    void UpdateMinAndMaxReadOffset();
    bool IsStartDurationOfFlvMultiStream();
    bool StartBufferingCheck(unsigned int& wantReadLength);
    bool ClearHasReadBuffer();
    void ClearCacheBuffer();
    void CheckDownloadPos(unsigned int wantReadLength);
    void HandleWaterline();
    bool CacheBufferFullLoop();
    bool IsNeedBufferForPlaying();
    uint32_t SaveCacheBufferDataNotblock(uint8_t* data, uint32_t len);
    void AddParamForUrl(std::string& url, const std::string& key, const std::string& value);
    void ChooseStreamByResolution();
    bool IsNearToInitResolution(const std::shared_ptr<PlayMediaStream> &choosedStream,
        const std::shared_ptr<PlayMediaStream> &currentStream);
    uint32_t GetResolutionDelta(uint32_t width, uint32_t height);
    bool CheckAutoSelectBitrate();
    bool IsAutoSelectConditionOk();
    void WaitCacheBufferInit();

private:
    std::shared_ptr<RingBuffer> ringBuffer_;
    std::shared_ptr<CacheMediaChunkBufferImpl> cacheMediaBuffer_;
    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<DownloadRequest> downloadRequest_;
    Mutex mutex_;
    ConditionVariable cvReadWrite_;
    Callback* callback_ {nullptr};
    StatusCallbackFunc statusCallback_ {nullptr};
    bool aboveWaterline_ {false};
    bool startedPlayStatus_ {false};
    bool isTimeOut_ {false};
    bool downloadErrorState_ {false};
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
    bool isRingBuffer_ {false};
    size_t readOffset_ {0};
    size_t writeOffset_ {0};
    std::atomic<bool> canWrite_ {true};
    std::atomic<bool> isNeedClean_ {false};
    std::atomic<bool> isHitSeeking_ {false};
    std::atomic<bool> isNeedDropData_ {false};
    std::atomic<bool> isServerAcceptRange_ {false};
    std::atomic<bool> isInterrupt_ {false};
    std::atomic<bool> isInterruptNeeded_{false};

    size_t waterLineAbove_ {0};
    std::atomic<bool> isBuffering_ {false};
    bool isFirstFrameArrived_ {false};

    struct RecordData {
        double downloadRate {0};
        uint64_t bufferDuring {0};
    };
    std::shared_ptr<RecordData> recordData_ {};
    uint64_t readBitrate_ {1 * 1024 * 1024};         //bps
    uint64_t lastReadCheckTime_ {0};
    uint64_t readTotalBytes_ {0};
    uint64_t readRecordDuringTime_ {0};
    uint64_t downloadDuringTime_ {0}; // 有效下载时长 ms
    uint64_t totalDownloadDuringTime_ {0};
    int32_t currentBitRate_ {0};
    uint64_t lastDurationReacord_ {0};
    int32_t lastCachedSize_ {0};
    std::atomic<bool> isBufferingStart_ {false};
    std::shared_ptr<WriteBitrateCaculator> writeBitrateCaculator_;

    volatile size_t wantedReadLength_ {0};
    volatile size_t bufferingTime_ {0};
    volatile size_t readTime_ {0};

    uint64_t minReadOffset_ {0};
    uint64_t maxReadOffset_ {0};
    int32_t minOffsetNotUpdateCount_ {0};
    int32_t maxOffsetNotUpdateCount_ {0};
    std::atomic<bool> isMinAndMaxOffsetUpdate_ {false};

    std::atomic<bool> isLargeOffsetSpan_ {false};
    int32_t stateChangeCount_ {0};
    FairMutex bufferingEndMutex_ {};
    ConditionVariable bufferingEndCond_;
    bool isSeekWait_ {false};
    bool isReportedErrorCode_ {false};
    bool isNeedClearHasRead_ {false};
    std::atomic<int32_t> expectOffset_ {-1};
    std::atomic<int32_t> initCacheSize_ {-1};
    Mutex initCacheMutex_ {};
    double bufferDurationForPlaying_ {0};
    uint64_t waterlineForPlaying_ {0};
    std::atomic<bool> isDemuxerInitSuccess_ {false};
	
    size_t timeoutInterval_ = 0;
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader_;
    int64_t flvStartPts_ {0};
    uint64_t extraCache_ {0};
    MediaStreamList playMediaStreams_;
    std::atomic<bool> isSelectingBitrate_ {false};
    std::shared_ptr<PlayMediaStream> defaultStream_ {nullptr};
    uint32_t initResolution_ {0};
    std::atomic<bool> isTimeoutErrorNotified_ {false};
    std::atomic<bool> isNeedResume_ {false};
    size_t totalConsumeSize_ {0};
    FairMutex savedataMutex_ {};
    uint64_t cachedDuration_ {0};
    std::atomic<bool> isAllowResume_ {false};
    bool isCacheBufferInited_ {false};
    ConditionVariable sleepCond_;
    FairMutex sleepMutex_;
    std::atomic<bool> isAutoSelectBitrate_ {true};
    std::deque<uint32_t> downloadSpeeds_;
    uint32_t videoBitrate_ {0};
};
}
}
}
}
#endif