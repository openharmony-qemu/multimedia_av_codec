/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef MPEG4_DEMUXER_PLUGIN_H
#define MPEG4_DEMUXER_PLUGIN_H

#include <atomic>
#include <vector>
#include <thread>
#include <condition_variable>
#include <map>
#include <queue>
#include <shared_mutex>
#include <list>
#include <unordered_map>
#include <arpa/inet.h>
#include "buffer/avbuffer.h"
#include "plugin/demuxer_plugin.h"
#include "mpeg4_box_parser.h"
#include "block_queue_pool.h"
#include "multi_stream_parser_manager.h"
#include "reference_parser_manager.h"
#include "common/log.h"
#include "meta/format.h"
#include "meta/meta.h"
#include "demuxer_data_reader.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {

class MPEG4SampleHelper;

class MPEG4DemuxerPlugin : public DemuxerPlugin {
public:
    using CachePressureCallback = DemuxerPlugin::CachePressureCallback;
    
    explicit MPEG4DemuxerPlugin(std::string name);
    ~MPEG4DemuxerPlugin() override;
    Status Reset() override;
    Status Start() override { return Status::OK; };
    Status Stop() override { return Status::OK; };
    Status Flush() override;
    Status SetDataSource(const std::shared_ptr<DataSource>& source,
        const std::shared_ptr<Media::Meta>& configs = nullptr) override;
    Status GetMediaInfo(MediaInfo& mediaInfo) override;
    Status GetUserMeta(std::shared_ptr<Meta> meta) override;
    Status SelectTrack(uint32_t trackId) override;
    Status UnselectTrack(uint32_t trackId) override;
    Status SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode, int64_t& realSeekTime) override;
    Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample) override;
    Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample, uint32_t timeout) override;
    Status GetNextSampleSize(uint32_t trackId, int32_t& size) override;
    Status GetNextSampleSize(uint32_t trackId, int32_t& size, uint32_t timeout) override;
    Status Pause() override;
    Status GetLastPTSByTrackId(uint32_t trackId, int64_t &lastPTS) override;
    Status GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo) override;
    void ResetEosStatus() override {}

    bool IsRefParserSupported() override;
    Status ParserRefUpdatePos(int64_t timeStampMs, bool isForward = true) override;
    Status ParserRefInfo() override;
    Status GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo) override;
    Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo) override;
    Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo) override;
    Status GetIFramePos(std::vector<uint32_t> &IFramePos) override;
    Status Dts2FrameId(int64_t dts, uint32_t &frameId) override;
    Status SeekMs2FrameId(int64_t seekMs, uint32_t &frameId) override;
    Status FrameId2SeekMs(uint32_t frameId, int64_t &seekMs) override;

    bool IsMultiVideoTracks();
    bool IsSupportedCodec();
    Status GetGopIdFromSeekPos(int64_t seekMs, int32_t &gopId);
    Status GetSampleIndexBySeekMs(int64_t seekMs, uint32_t &frameId);
    Status RefParserInit();
    Status ParserRefInfoLoop(std::shared_ptr<Sample> sample, uint32_t sampleIndex,
        const std::vector<SampleIndexEntry>& sampleIndexEntries_);
    Status UpdateParserGopId(int32_t syncSamplesPtsSize);
    Status SelectProGopId();

    Status GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
        const uint64_t relativePresentationTimeUs, uint32_t &index) override;
    Status GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
        const uint32_t index, uint64_t &relativePresentationTimeUs) override;
    void SetCacheLimit(uint32_t limitSize) override;
    Status GetCurrentCacheSize(uint32_t trackId, uint32_t& size) override;
    bool GetProbeSize(int32_t &offset, int32_t &size) override;
    void SetInterruptState(bool isInterruptNeeded) override;
    Status SetDataSourceWithProbSize(const std::shared_ptr<DataSource>& source,
        const int32_t probSize) override;
    Status BoostReadThreadPriority() override;
    Status SetAVReadPacketStopState(bool state) override;
    Status SetReadTimeoutForInitSeek(int64_t timeoutMs) override;
    Status ReadSampleZeroCopy(uint32_t trackId, std::shared_ptr<AVBuffer> sample, uint32_t timeout) override;
    Status GetCurrentCacheFrameCount(uint32_t trackId, uint32_t& frameCount) override;
    Status SeekToStart() override;
    Status SeekToKeyFrame(
        int32_t trackId, int64_t seekTime, SeekMode mode, int64_t& realSeekTime, uint32_t timeoutMs) override;
    Status SeekToFrameByDts(int32_t trackId, int64_t seekTime,
        SeekMode mode, int64_t& realSeekTime, uint32_t timeoutMs) override;

    // cache pressure control
    Status SetCachePressureCallback(CachePressureCallback cb) override;
    Status SetTrackCacheLimit(uint32_t trackId, uint32_t limitBytes, uint32_t windowMs = 500) override;
private:
    enum IndexAndPTSConversionMode : unsigned int {
        GET_FIRST_SAMPLE_PTS,
        INDEX_TO_RELATIVEPTS,
        RELATIVEPTS_TO_INDEX,
        PTS_TO_DTS_MAPPING,
    };

    std::shared_ptr<MPEG4AtomParser::Track> firstTrack_ = nullptr;
    std::shared_ptr<MPEG4AtomParser::Track> lastTrack_ = nullptr;
    std::vector<MPEG4AtomParser::FragmentEntry> fragmentEntry_ = {};

    std::mutex mutex_ {};
    std::mutex syncMutex_ {};
    std::shared_mutex sharedMutex_ {};
    std::unordered_map<uint32_t, std::shared_ptr<std::mutex>> trackMtx_ {};
    std::atomic<bool> inited_ = false;
    bool isIsom_ = false;
    bool hasSidxBox_ = false;
    bool hasMoofBox_ = false;
    int32_t movieTimeScale_ = 1;
    int64_t minElstInitEmptyEdit_ = INT64_MAX;
    uint64_t relativeEndPts_ = 0;
    uint32_t currentFragment_ = 0;
    Seekable seekable_;
    std::vector<uint32_t> selectedTrackIds_ = {};
    Mpeg4BlockQueuePool cacheQueue_;
    MediaInfo mediaInfo_;
    std::shared_ptr<Meta> userformat_ = nullptr;
    uint32_t cachelimitSize_ = 0;
    bool outOfLimit_ = false;
    bool setLimitByUser_ = false;
    bool readToEnd_ = false;
    std::atomic<bool> isInterruptNeeded_{false};

    std::shared_ptr<DataSource> dataSource_ {nullptr};
    std::shared_ptr<DemuxerDataReader> dataReader_ {nullptr};

    std::shared_ptr<MultiStreamParserManager> streamParsers_ {nullptr};
    bool streamParserInited_ {false};
    std::vector<uint32_t> avTrackIds_ {};
    std::map<int32_t, std::shared_ptr<Sample>> firstFrameMap_ {};
    std::unordered_map<uint32_t, std::shared_ptr<MPEG4Sample>> cacheSamples_ {};
    std::unordered_map<uint32_t, std::shared_ptr<Sample>> pendingWebVttSamples_ {};
    std::unordered_map<uint32_t, std::vector<int64_t>> rawDtsPrefix_ {};

    int64_t absolutePTSOfIndexZero_ = INT64_MAX;
    std::priority_queue<int64_t> index2RelativePTSHeap_;
    int64_t minRelativePTS_ = INT64_MAX;
    int64_t maxRelativePTS_ = INT64_MIN;
    int64_t minRelativePTSDiffOfLeft_ = INT64_MAX;
    int64_t minRelativePTSDiffOfRight_ = INT64_MAX;
    uint32_t index2RelativePTS_ = 0;

    std::shared_ptr<ReferenceParserManager> referenceParser_ = nullptr;
    bool isInit_ = false;
    int64_t ptsOfIndexZero_ = 0;
    std::map<int64_t, int64_t> pts2Dts_;  // relative pts(us)->relative dts(us)
    std::map<int64_t, uint32_t> dts2SyncSampleIndex_;
    std::unordered_map<uint32_t, int64_t> sampleIndex2Pts_;  // Sample index->relative pts(us)
    int32_t refParserTrackIdx_ = -1;
    std::shared_ptr<MPEG4AtomParser::Track> trackOfRefParser_ = nullptr;
    std::vector<uint32_t> syncSamplesIndex_;  // Sync sample index or Gop id (start from 0)
    int64_t refParserStartTime_ = -1;
    int64_t refParserPendingSeekMsTime_ = -1;
    int32_t refParserCurGopId_ = 0;  // Gop Id(start from 0)
    bool refParserIsForward_ = true;
    std::atomic<bool> refParserState_ = true;
    std::list<uint32_t> processingSyncSamplesIndex_;
    std::unordered_map<int32_t, int64_t> gopId2DecodedSamplePts_;  // gopId->absolute dts(us)
    std::string mimeTypeOfVideoStream_ = MimeType::INVALID_TYPE;
    int64_t startPts_ = 0;
    bool updatePosIsForward_ = true;
    bool isSdtpExist_ = false;
    bool isReadSampleDataFromCache_ = false;

    int32_t CountTracks();
    void GetAVTrackIds();
    Status ParseAVFirstFrames();
    bool FirstFrameValid(uint32_t trackIndex);

    Status ParseMoof(int64_t offset);
    Status HasCodecParameters();
    Status ConvertElstTimescaleToMs();
    Status GetMediaInfo();
    void GetSingleTrackInfo(const MPEG4AtomParser::Track& track, const uint32_t trackIndex);
    void ResetParam();
    void InitParser();
    bool TrackIsSelected(const uint32_t trackId);
    std::shared_ptr<MPEG4AtomParser::Track> FindTrackById(int32_t trackId);

    Status DynamicBaseTrackSelect(int64_t seekTime, int64_t &maxPts, int32_t &baseTrackIndex);
    struct DynamicBaseTrackState {
        int64_t& minPts;
        int32_t& baseTrackTimeBase;
        bool& baseTrackIsVideo;
        int32_t& baseTrackIndex;
    };
    Status UpdateDynamicBaseTrack(const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t trackId,
        int64_t seekTime, DynamicBaseTrackState& state);
    Status FindBaseTrackAndCalcDelay(int64_t seekTime, SeekMode mode, int32_t &trackId);
    Status FindFragmentSyncSample(uint32_t fragmentIndex, std::shared_ptr<MPEG4AtomParser::Track> track);
    Status FindFragmentAtTime(int64_t reqTime, SeekMode flag, std::shared_ptr<MPEG4AtomParser::Track> track,
        bool &findFragmentSync);
    Status FragmentSeek(int64_t seekTime, SeekMode mode, int32_t trackIndex, bool &findFragmentSync);
    struct SeekTimeContext {
        bool editListApplied = false;
        int32_t baseTrackTimeScale = 0;
        int64_t relativeSeekTime = 0;
        int64_t trackSeekTime = 0;
        int64_t syncSeekTime = 0;
    };
    Status PrepareSeekTime(const std::shared_ptr<MPEG4AtomParser::Track>& track, int64_t seekTime,
        int64_t& realSeekTime, SeekTimeContext& context);
    Status CalculateRealSeekTime(const std::shared_ptr<MPEG4AtomParser::Track>& track, int64_t seekTime,
        int64_t& realSeekTime);
    Status ValidateElstShiftStartTimeScale(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        const SeekTimeContext& context) const;
    Status ClampSeekTimeToStreamDuration(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        SeekTimeContext& context);
    struct SeekSampleIndexContext {
        int64_t seekTime = 0;
        int64_t trackSeekTime = 0;
        SeekMode mode = SeekMode::SEEK_PREVIOUS_SYNC;
        uint32_t sampleIndex = 0;
        bool isBaseTrack = true;
    };
    Status FindSeekSampleIndex(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        SeekSampleIndexContext& context);
    Status DoSeekInternal(int32_t trackIndex, int64_t seekTime, SeekMode mode, int64_t& realSeekTime);
    struct VideoSyncSeekContext {
        uint32_t sampleIndex;
        bool isBaseTrack;
        bool editListApplied;
        int32_t baseTrackTimeScale;
        int64_t& syncSeekTime;
    };
    Status GetVideoSyncSeekTime(const std::shared_ptr<MPEG4AtomParser::Track>& track, VideoSyncSeekContext& context);
    Status MultiTrackSync(int32_t trackIndex, bool isBaseTrack, int64_t syncSeekTime, SeekMode mode);
    Status SeekToStartInternal();
    Status CheckSeekToFrameByDtsParam(int32_t trackId, int64_t seekTime, SeekMode mode);
    void ResetCacheForSeek();
    struct SeekToFrameByDtsContext {
        std::shared_ptr<MPEG4AtomParser::Track> track;
        uint32_t sampleIndex = 0;
        int64_t actualDts = 0;
        int32_t trackTimeScale = 0;
    };
    Status PrepareSeekToFrameByDts(int32_t trackId, int64_t seekTime, SeekToFrameByDtsContext& context);
    Status FindValidSample(std::shared_ptr<MPEG4AtomParser::Track> &track,
        std::shared_ptr<MPEG4SampleHelper::SampleIndexEntry> &sample);
    Status GetUnseekableSampleInfo(uint32_t &trackId, int64_t &pos, int32_t &size);
    Status GetSeekableSampleInfo(uint32_t trackId, int64_t &pos, int32_t &size);
    Status ReadSampleData(std::shared_ptr<Sample> sample, uint64_t pos, int32_t size, uint32_t trackId);
    Status CheckReadSampleDataState(int32_t size);
    Status ReadSampleDataToBuffer(std::shared_ptr<Sample> sample, uint64_t pos, int32_t size);
    Status UpdateReadSampleDataTimeout(Status ret, std::shared_ptr<Sample> sample);
    Status ReadSampleData(std::shared_ptr<MPEG4Sample>& mpeg4Sample, uint64_t pos, int32_t size, uint32_t trackId);
    Status AddSampleToCacheQueue(std::shared_ptr<Sample> sample, const uint32_t trackId);
    void MaybeNotifyCachePressure(uint32_t trackId, uint32_t cacheBytes);
    int64_t NowMs() const;
    Status FindFirstSampleIndexByDts(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        int64_t targetDts, uint32_t& sampleIndex) const;
    Status SyncSelectedTracksByDts(int32_t baseTrackId, int64_t baseDts);
    Status PrefetchSeekSampleWithTimeout(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        uint32_t sampleIndex, int64_t startMs, uint32_t timeoutMs);
    Status PrefetchSeekSample(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        uint32_t sampleIndex);
    Status ConvertSampleToAnnexb(std::shared_ptr<MPEG4Sample> dstMpeg4Sample, const uint32_t trackId);
    Status SetEosSample(std::shared_ptr<AVBuffer> sample);
    Status SetEosSample(std::shared_ptr<AVBuffer> sample, bool replaceMemory);
    Status GetSampleBySeekableStatus(const Seekable seekable, uint32_t trackId,
        std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer);
    Status GetSampleByMimeType(const Seekable seekable, uint32_t trackId,
        std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer);
    struct WebVttReadContext {
        Seekable seekable;
        uint32_t trackId = 0;
        std::shared_ptr<MPEG4AtomParser::Track> track;
        std::shared_ptr<AVBuffer> outBuffer;
    };
    Status ReadAndCacheWebVttSample(const WebVttReadContext& context, bool& continueRead);
    Status UpdateWebVttCachedSampleDuration(const WebVttReadContext& context,
        const std::shared_ptr<MPEG4Sample>& tempSample, uint32_t nextSampleIndex);
    uint32_t GetFlags(const std::shared_ptr<Sample> sample, bool memoryNotEnough);
    Status WriteSampleAttr(uint32_t trackId, std::shared_ptr<Sample> sample);
    Status ProcessWebVTTSample(std::shared_ptr<MPEG4Sample> mpeg4Sample);
    Status WriteBuffer(
        uint32_t trackIndex, std::shared_ptr<AVBuffer> outBuffer, std::shared_ptr<MPEG4Sample> mpeg4Sample);
    Status WriteSampleMemory(
        uint32_t trackIndex, std::shared_ptr<AVBuffer> outBuffer, std::shared_ptr<MPEG4Sample> mpeg4Sample);
    void CheckResetXPSSendStatus(uint32_t trackIndex, std::shared_ptr<Sample> sample);

    Status CheckCacheDataLimit(uint32_t trackId);
    void ParseHEVCMetadataInfo(const MPEG4AtomParser::Track& track, Meta &format);
    void ParseAVCMetadataInfo(const MPEG4AtomParser::Track& track, Meta &format);
    void ConvertCsdToAnnexb(const MPEG4AtomParser::Track& track, Meta& format, uint32_t trackIndex);
    int SelectSeekTrack();

    // 辅助轨相关函数
    bool IsVideoType(const MPEG4AtomParser::Track& track);
    bool IsAudioType(const MPEG4AtomParser::Track& track);
    void UpdateReferenceIds();
    std::map<int32_t, std::vector<int32_t>> referenceIdsMap_ {};
    
    void ResetParametersOfPTSAndIndexConversion();
    uint32_t GetSampleCount(uint32_t trackIndex);
    Status GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode mode, uint32_t trackIndex,
        int64_t absolutePTS, uint32_t sampleIndex);
    Status ConvertPTSAndIndexOnlyByStts(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        IndexAndPTSConversionMode mode, int64_t absolutePTS, uint32_t sampleIndex);
    Status ConvertPTSAndIndexBySttsAndCtts(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        IndexAndPTSConversionMode mode, int64_t absolutePTS, uint32_t sampleIndex);
    void ProcessConversionByMode(IndexAndPTSConversionMode mode, int64_t pts, int64_t absolutePTS,
        uint32_t sampleIndex, int64_t dts);
    Status UpdatePTSBySttsAndCtts(int64_t& pts, int64_t dts, double timeScaleRate,
        int64_t sampleOffsetOfCurrentCttsEntry);
    void ProcessIndex2RelativePTS(int64_t pts, uint32_t index);
    void ProcessRelativePTS2Index(int64_t pts, int64_t absolutePTS);
    void ProcessPts2DtsMap(int64_t pts, int64_t dts);

    Status WaitForLoop(uint32_t trackId, uint32_t timeout);
    Status EnsureReadLoopStarted();
    bool ShouldWaitForRead(uint32_t trackId);
    void MPEG4ReadLoop();
    bool NeedWaitForRead(uint32_t trackId);
    void HandleReadWait();
    bool ReadAndCacheSample();
    Status ReadSampleToCache(uint32_t trackId);
    Status PushEosToCache(uint32_t trackId);
    bool IsWebVTTTrack(const std::shared_ptr<MPEG4AtomParser::Track>& track) const;
    void AdvanceSampleIndex(const std::shared_ptr<MPEG4AtomParser::Track>& track);
    Status ProcessWebVTTSampleForCache(
        const std::shared_ptr<MPEG4AtomParser::Track>& track, std::shared_ptr<MPEG4Sample> mpeg4Sample);
    Status FlushPendingWebVTTSample(uint32_t trackId);
    Status FlushPendingWebVTTSamples();
    int64_t GetWebVTTFlushEndPtsUs(
        const std::shared_ptr<MPEG4AtomParser::Track>& track, const std::shared_ptr<Sample>& sample) const;
    void ReleaseReadLoop();
    void UpdateAsyncReadThreadPriority();

    enum class ThreadState : uint32_t {
        NOT_STARTED,
        WAITING,
        READING,
    };

    enum class ReadMode : uint32_t {
        NONE = 0,
        SYNC = 1U << 0,
        ASYNC = 1U << 1,
    };

    enum class AVReadPacketStopState : uint32_t {
        UNSET = 0,
        TRUE,
        FALSE,
    };

    static constexpr uint32_t ReadModeToFlags(ReadMode mode)
    {
        return static_cast<uint32_t>(mode);
    }

    std::unique_ptr<std::thread> readThread_ {nullptr};
    std::condition_variable readLoopCv_;
    std::condition_variable readCacheCv_;
    std::mutex readSampleMutex_ {};
    std::mutex readThreadMutex_ {};
    std::mutex seekWaitMutex_ {};
    std::condition_variable seekWaitCv_;
    std::atomic<uint32_t> trackId_ {0};
    std::atomic<ThreadState> threadState_ {ThreadState::NOT_STARTED};
    std::atomic<Status> readLoopStatus_ {Status::OK};
    std::atomic<bool> isPauseReadPacket_ {false};
    std::atomic<bool> threadReady_ {false};
    std::atomic<bool> isWaitingForReadThread_ {false};
    std::atomic<bool> stopReadThread_ {false};
    std::atomic<uint32_t> readModeFlags_ {0};
    std::atomic<bool> isAsyncReadThreadPrioritySet_ {false};
    std::atomic<AVReadPacketStopState> avReadPacketStopState_ {AVReadPacketStopState::UNSET};
    std::atomic<int64_t> readTimeoutMs_ {-1};
    std::atomic<int64_t> readStartTimeMs_ {0};
    CachePressureCallback cachePressureCb_ {nullptr};
    std::unordered_map<uint32_t, uint32_t> trackCacheLimitMap_ {};
    std::unordered_map<uint32_t, uint32_t> trackThrottleWindowMs_ {};
    std::unordered_map<uint32_t, int64_t> trackLastNotifyMs_ {};
    std::mutex cachePressureMutex_ {};
};
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_DEMUXER_PLUGIN_H
