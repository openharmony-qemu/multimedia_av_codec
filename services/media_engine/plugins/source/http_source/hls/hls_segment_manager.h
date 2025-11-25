/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
 
#ifndef HISTREAMER_HLS_SEG_MANAGER_H
#define HISTREAMER_HLS_SEG_MANAGER_H

#include <mutex>
#include <thread>
#include <unistd.h>
#include <utility>
#include "playlist_downloader.h"
#include "av_common.h"
#include "download/downloader.h"
#include "media_downloader.h"
#include "openssl/aes.h"
#include "osal/task/task.h"
#include "osal/utils/ring_buffer.h"
#include "osal/utils/steady_clock.h"
#include "common/media_source.h"
#include "common/media_core.h"
#include "utils/media_cached_buffer.h"
#include "utils/write_bitrate_caculator.h"
#include "osal/task/mutex.h"
#include "osal/task/condition_variable.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

struct HlsSegEvent {
    HlsSegmentType segType {HlsSegmentType::SEG_VIDEO};
    PluginEventType type;
    NetworkClientErrorCode networkError;
    BufferingInfoType bufferType;
    std::multimap<std::string, std::vector<uint8_t>> drmInfos;
    std::pair<int32_t, int32_t> videoSize;
    uint32_t bitRate;
    uint64_t cachedDuration;
    int percent;
    Format seekReadyInfo {};
    std::string str;
};

using HlsSegmentBufferingCbFunc = std::function<void(HlsSegmentType, BufferingInfoType)>;
using HlsSegmentEventCbFunc = std::function<void(HlsSegEvent)>;
constexpr uint32_t CHUNK_SIZE = 16 * 1024;
class HlsMediaDownloader;
class HlsSegmentManager : public PlayListChangeCallback, public std::enable_shared_from_this<HlsSegmentManager> {
public:
    explicit HlsSegmentManager(int expectBufferDuration, bool userDefinedDuration,
        const std::map<std::string, std::string>& httpHeader = std::map<std::string, std::string>(),
        HlsSegmentType type = HlsSegmentType::SEG_VIDEO,
        std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader = nullptr);
    explicit HlsSegmentManager(std::string mimeType, HlsSegmentType type,
        const std::map<std::string, std::string>& httpHeader = std::map<std::string, std::string>());
    explicit HlsSegmentManager(const std::shared_ptr<HlsSegmentManager> &other, HlsSegmentType type);
    ~HlsSegmentManager() override;
    void Init();
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader);
    void Close(bool isAsync);
    void Pause();
    void Resume();
    Status Read(unsigned char* buff, ReadDataInfo& readDataInfo);
    bool SeekToTime(int64_t seekTime, SeekMode mode);
    void Clone(const std::shared_ptr<HlsSegmentManager> &other);
    void SetMasterReadyCallback(std::function<void(bool, bool)> cb);
    HlsSegmentType GetSegType(uint32_t streamId);

    size_t GetContentLength() const;
    int64_t GetDuration() const;
    Seekable GetSeekable() const;
    void SetCallback(Callback* cb);
    void OnPlayListChanged(const std::vector<PlayInfo>& playList) override;
    void OnMasterReady(bool needAudioManager, bool needSubTitleManager) override;
    void SetStatusCallback(StatusCallbackFunc cb);
    bool GetStartedStatus();
    std::vector<uint32_t> GetBitRates();
    bool SelectBitRate(uint32_t bitRate);
    void OnSourceKeyChange(const uint8_t *key, size_t keyLen, const uint8_t *iv) override;
    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos) override;
    void SetIsTriggerAutoMode(bool isAuto);
    void SetReadBlockingFlag(bool isReadBlockingAllowed);
    void SeekToTs(uint64_t seekTime, SeekMode mode);
    void PutRequestIntoDownloader(const PlayInfo& playInfo);
    int64_t RequestNewTs(uint64_t seekTime, SeekMode mode, double totalDuration,
        double hstTime, const PlayInfo& item);
    void UpdateDownloadFinished(const std::string &url, const std::string& location);
    void AutoSelectBitrate(uint32_t bitRate);
    size_t GetTotalBufferSize();
    void SetInterruptState(bool isInterruptNeeded);
    void GetPlaybackInfo(PlaybackInfo& playbackInfo);
    void ReportBitrateStart(uint32_t bitRate);
    std::pair<int32_t, int32_t> GetDownloadRateAndSpeed();
    void GetDownloadInfo(DownloadInfo& downloadInfo);
    std::pair<int32_t, int32_t> GetDownloadInfo();
    void ReportVideoSizeChange();
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID);
    uint64_t GetBufferSize() const;
    bool GetPlayable();
    bool GetBufferingTimeOut();
    bool GetReadTimeOut(bool isDelay);
    void SetAppUid(int32_t appUid);
    size_t GetSegmentOffset();
    bool GetHLSDiscontinuity();
    Status StopBufferring(bool isAppBackground);
    void WaitForBufferingEnd();
    void SetIsReportedErrorCode();
    void SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy);
    bool SetInitialBufferSize(int32_t offset, int32_t size);
    void NotifyInitSuccess();
    uint64_t GetCachedDuration();
    Status GetStreamInfo(std::vector<StreamInfo>& streams);
    bool IsHlsFmp4();
    uint64_t GetMemorySize();
    std::string GetContentType();
    bool IsHlsEnd();
    bool SelectAudio(int32_t streamId);
    bool StartAudioDownload(int32_t streamId);
    std::shared_ptr<StreamInfo> GetStreamInfoById(int32_t streamId);
    int32_t GetDefaultAudioStreamId();
    void SetDemuxerState(int32_t streamId);
    void SetDownloadErrorState();
    void SetSegmentBufferingCallback(HlsSegmentBufferingCbFunc bufferingCbFunc);
    void SetSegmentAllCallback(HlsSegmentEventCbFunc segEventCallback);

public:
    static constexpr size_t VIDEO_MIN_BUFFER_SIZE = 5 * 1024 * 1024;
    static constexpr size_t VIDEO_MAX_CACHE_BUFFER_SIZE = 19 * 1024 * 1024;
    static constexpr uint64_t DECRYPT_UNIT_LEN = 16;
    static const std::map<HlsSegmentType, size_t> MIN_BUFFER_SIZE;
    static const std::map<HlsSegmentType, size_t> MAX_CACHE_BUFFER_SIZE;

private:
    void InitCacheWithDuration();
    void SaveHttpHeader(const std::map<std::string, std::string>& httpHeader);
    uint32_t SaveData(uint8_t *data, uint32_t len, bool notBlock);
    Status ReadDelegate(unsigned char* buff, ReadDataInfo& readDataInfo);
    void ReadCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo);
    uint32_t SaveEncryptData(uint8_t* data, uint32_t len, bool notBlock);
    void InitMediaDownloader();
    void DownloadRecordHistory(int64_t nowTime);
    void OnWriteCacheBuffer(uint32_t len);
    void OnReadBuffer(uint32_t len);
    double GetAveDownSpeed();
    uint64_t GetMinBuffer();
    void DownloadReport();
    void ReportDownloadSpeed();
    bool CheckRiseBufferSize();
    bool CheckPulldownBufferSize();

    void RiseBufferSize();
    void DownBufferSize();
    void ActiveAutoBufferSize();
    void InActiveAutoBufferSize();
    uint64_t TransferSizeToBitRate(int width);
    bool HandleBuffering();
    bool HandleCache();
    bool CheckReadStatus();
    Status CheckPlaylist(unsigned char* buff, ReadDataInfo& readDataInfo);
    bool CheckBreakCondition();
    uint32_t GetDecrptyRealLen(uint8_t* writeDataPoint, uint32_t waitLen, uint32_t writeLen);
    void ResetPlaylistCapacity(size_t size);
    void PlaylistBackup(const PlayInfo& fragment);
    void HandleCachedDuration();
    void UpdateWaterLineAbove();
    void CalculateBitRate(size_t fragmentSize, double duration);
    double CalculateCurrentDownloadSpeed();
    void UpdateCachedPercent(BufferingInfoType infoType);
    bool CheckBufferingOneSeconds();
    float GetCacheDuration(float ratio);
    void HandleFfmpegReadback(uint64_t ffmpegOffset);
    void SeekToTsForRead(uint32_t currentTsIndex);
    int64_t RequestNewTsForRead(const PlayInfo& item);
    void PushPlayInfo(PlayInfo playInfo);
    void PrepareToSeek();
    bool CheckDataIntegrity();
    uint32_t SaveCacheBufferData(uint8_t* data, uint32_t len, bool notBlock);
    uint32_t SaveCacheBufferDataNotblock(uint8_t* data, uint32_t len);
    bool ClearChunksOfFragment();
    uint64_t GetCrossTsBuffersize();
    bool IsCachedInitSizeReady(int32_t wantInitSize);
    void HandleWaterLine();
    bool CacheBufferFullLoop();
    bool IsNeedBufferForPlaying();
    bool CheckLoopTimeout(int64_t loopStartTime);
    void HandleSaveDataLoopContinue();
    bool ReadHeaderData(unsigned char* buff, ReadDataInfo& readDataInfo);
    void HandleSeekReady(int32_t streamId, int32_t isEos);
    void RemoveFmp4PaddingData(unsigned char* buff, ReadDataInfo& readDataInfo);
    uint64_t GetTotalTsBuffersize();
    bool IsPureByteRange();
    void SetDownloaderRequestCb(StatusCallbackFunc& statusCallback, DownloadDoneCbFunc& downloadDoneCallback);
    void SetType(HlsSegmentType type);
    bool HandleTimeout();
    void HandleBufferingState();
    void HandleBufferingEnd();
    bool CheckTsEndOrEos(ReadDataInfo& readDataInfo);

private:
    HlsSegmentType type_ = HlsSegmentType::SEG_VIDEO;
    size_t minBufferSize_ = VIDEO_MIN_BUFFER_SIZE;
    size_t maxCacheBufferSize_ = VIDEO_MAX_CACHE_BUFFER_SIZE;
    size_t totalBufferSize_ {0};
    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<DownloadRequest> downloadRequest_;
    Callback* callback_ {nullptr};
    DataSaveFunc dataSave_;
    StatusCallbackFunc statusCallback_;
    std::function<void(bool, bool)> masterReadyCallback_;
    bool startedPlayStatus_ {false};

    std::shared_ptr<PlayListDownloader> playlistDownloader_;

    std::shared_ptr<BlockingQueue<PlayInfo>> playList_;
    std::map<std::string, bool> fragmentDownloadStart;
    std::map<std::string, bool> fragmentPushed;
    std::deque<PlayInfo> backPlayList_;
    bool isSelectingBitrate_ {false};
    bool isDownloadStarted_ {false};
    uint8_t afterAlignRemainedBuffer_[DECRYPT_UNIT_LEN] {0};
    uint64_t afterAlignRemainedLength_ = 0;
    uint64_t totalLen_ = 0;
    std::string curUrl_;
    uint8_t key_[DECRYPT_UNIT_LEN] = {0};
    size_t keyLen_ {0};
    uint8_t iv_[DECRYPT_UNIT_LEN] = {0};
    uint8_t initIv_[DECRYPT_UNIT_LEN] = {0};
    AES_KEY aesKey_;
    uint8_t decryptCache_[VIDEO_MIN_BUFFER_SIZE] {0};
    uint8_t decryptBuffer_[VIDEO_MIN_BUFFER_SIZE] {0};
    uint32_t writeTsIndex_ = 0;
    bool isAutoSelectBitrate_ {true};
    uint64_t seekTime_ = 0;

    bool isReadFrame_ {false};
    bool isTimeOut_ {false};
    bool downloadErrorState_ {false};
    uint64_t bufferedDuration_ {0};
    uint64_t readBitrate_ {1 * 1024 * 1024}; // bps
    bool userDefinedBufferDuration_ {false};
    uint64_t expectDuration_ {0};
    bool autoBufferSize_ {true};
    uint64_t lastCheckTime_ {0};
    uint32_t recordCount_ {0};
    uint64_t lastRecordTime_ {0};
    std::atomic<bool> isInterruptNeeded_{false};

    uint64_t lastWriteTime_ {0};
    uint64_t lastWriteBit_ {0};
    SteadyClock steadyClock_;

    uint64_t totalBits_ {0};        // 总下载量

    uint64_t lastBits_ {0};         // 上一统计周期的总下载量

    uint64_t downloadDuringTime_ {0};    // 累计有效下载时长 ms

    uint64_t downloadBits_ {0};          // 累计有效时间内下载数据量 bit
    uint32_t changeBitRateCount_ {0};  // 设置码率次数
    int32_t seekFailedCount_ {0};   // seek失败次数
    int64_t openTime_ {0};
    int64_t playDelayTime_ {0};
    int64_t startDownloadTime_ {0};
    int32_t avgDownloadSpeed_ {0};
    bool isDownloadFinish_ {false};
    double avgSpeedSum_ {0};
    uint32_t recordSpeedCount_ {0};

    int64_t lastReportUsageTime_ {0};
    uint64_t dataUsage_ {0};

    struct RecordData {
        double downloadRate {0};
        uint64_t bufferDuring {0};
    };
    std::shared_ptr<RecordData> recordData_ {nullptr};
    std::map<std::string, std::string> httpHeader_ {};
    std::atomic<bool> isStopped = false;
    std::string mimeType_;
    size_t waterLineAbove_ {0};
    bool isInterrupt_ {false};
    std::atomic<bool> isBuffering_ {false};
    bool isFirstFrameArrived_ {false};
    std::atomic<bool> isSeekingFlag {false};
    Mutex switchMutex_ {};
    bool isLastDecryptWriteError_ {false};
    uint32_t lastRealLen_ {0};

    uint64_t lastReadCheckTime_ = 0;
    uint64_t readTotalBytes_ = 0;
    uint64_t readRecordDuringTime_ = 0;
    uint64_t totalDownloadDuringTime_ {0};
    int32_t currentBitRate_ {0};
    int32_t fragmentBitRate_ {0};
    uint64_t lastDurationReacord_ {0};
    uint64_t lastCachedSize_ {0};
    std::atomic<bool> isBufferingStart_ {false};
    std::shared_ptr<CacheMediaChunkBufferImpl> cacheMediaBuffer_;
    uint64_t readOffset_ {0};
    uint64_t writeOffset_ {0};
    std::map<uint32_t, std::pair<uint32_t, bool>> tsStorageInfo_ {};
    std::atomic<uint32_t> readTsIndex_ {0};
    std::atomic<bool> canWrite_ {true};
    uint64_t ffmpegOffset_ = 0;
    volatile size_t wantedReadLength_ {0};
    volatile size_t bufferingTime_ {0};
    volatile size_t readTime_ {0};
    FairMutex tsStorageInfoMutex_ {};
    std::shared_ptr<WriteBitrateCaculator> writeBitrateCaculator_ {nullptr};

    FairMutex bufferingEndMutex_ {};
    ConditionVariable bufferingEndCond_;
    bool isReportedErrorCode_ {false};
    std::atomic<int32_t> expectOffset_ {-1};
    std::atomic<int32_t> initCacheSize_ {-1};
    Mutex initCacheMutex_ {};
    double bufferDurationForPlaying_ {0};
    uint64_t waterlineForPlaying_ {0};
    std::atomic<bool> isDemuxerInitSuccess_ {false};
    std::atomic<bool> isTimeoutErrorNotified_ {false};
	
    size_t timeoutInterval_ {0};
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader_ {nullptr};
    std::atomic<bool> isNeedResume_ {false};
    uint64_t cachedDuration_ {0};
    SteadyClock loopInterruptClock_;

    std::map<uint32_t, uint32_t> tsStreamIdInfo_ {};
    uint32_t curStreamId_ {0};
    std::atomic<bool> isNeedReadHeader_ {false};
    std::atomic<bool> isNeedResetOffset_ {false};
    uint64_t memorySize_ {0};

    int64_t seekStartTimePos_ {0};
    std::atomic<bool> isTsEnd_ {false};
    std::atomic<bool> notNeedReadBack_ {false};
    HlsSegmentEventCbFunc segEventCb_;
    HlsSegmentBufferingCbFunc bufferingCbFunc_;
    std::string masterString_;
    bool isEos_ {false};
};
}
}
}
}
#endif