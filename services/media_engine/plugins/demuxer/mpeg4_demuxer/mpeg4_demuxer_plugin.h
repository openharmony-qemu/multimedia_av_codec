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
#include "buffer/avbuffer.h"
#include "plugin/demuxer_plugin.h"
#include "mpeg4_box_parser.h"
#include "multi_stream_parser_manager.h"
#include "reference_parser_manager.h"
#include "common/log.h"
#include "meta/meta.h"
#include "demuxer_data_reader.h"
#include "mpeg4_demuxer_plugin_internal.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {

class MPEG4SampleHelper;
struct ReadSamplePacketAttr;
struct MpegAudioParserStates;

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
    using DumpMode = detail::DumpMode;

    struct ReadSampleFinalizeContext {
        uint32_t trackId;
        std::shared_ptr<MPEG4AtomParser::Track> track;
        std::shared_ptr<MPEG4Sample> sample;
        bool useCache;
        bool isEos;
        Status status;
    };

    enum IndexAndPTSConversionMode : unsigned int {
        GET_FIRST_SAMPLE_PTS,
        INDEX_TO_RELATIVEPTS,
        RELATIVEPTS_TO_INDEX,
    };

    std::shared_ptr<MPEG4AtomParser::Track> firstTrack_ = nullptr;
    std::shared_ptr<MPEG4AtomParser::Track> lastTrack_ = nullptr;
    std::vector<MPEG4AtomParser::FragmentEntry> fragmentEntry_ = {};

    std::mutex mutex_ {};
    std::mutex syncMutex_ {};
    std::shared_mutex sharedMutex_ {};
    // The async read loop never takes sharedMutex_; this mutex protects its view of selection and read progress.
    std::mutex readConfigMutex_ {};
    std::unordered_map<uint32_t, std::shared_ptr<std::mutex>> trackMtx_ {};
    Plugins::DemuxerMode demuxerMode_ {Plugins::DemuxerMode::FULL_PARSE};
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
    MediaInfo mediaInfo_;
    std::shared_ptr<Meta> userformat_ = nullptr;
    uint32_t cachelimitSize_ = 0;
    bool outOfLimit_ = false;
    bool setLimitByUser_ = false;
    bool readToEnd_ = false;
    std::atomic<bool> isInterruptNeeded_{false};

    detail::CachePhase cachePhase_ {detail::CachePhase::BOOTSTRAP};
    uint64_t readConfigEpoch_ {0};
    detail::ReadCursor readCursor_ {};
    bool hasInFlightSample_ {false};
    uint64_t inFlightReadConfigEpoch_ {0};
    detail::SampleReadInfo inFlightSampleInfo_ {};
    int64_t inFlightSampleDtsUs_ {0};

    std::shared_ptr<DataSource> dataSource_ {nullptr};
    std::shared_ptr<DemuxerDataReader> dataReader_ {nullptr};

    std::shared_ptr<MultiStreamParserManager> streamParsers_ {nullptr};
    bool streamParserInited_ {false};
    std::vector<uint32_t> avTrackIds_ {};
    std::map<int32_t, std::shared_ptr<Sample>> firstFrameMap_ {};
    std::unordered_map<uint32_t, std::shared_ptr<MPEG4Sample>> cacheSamples_ {};
    std::unordered_map<uint32_t, std::shared_ptr<MPEG4Sample>> pendingWebVttSamples_ {};
    std::unique_ptr<MpegAudioParserStates> mpegAudioParserStates_ {nullptr};
    std::mutex mpegAudioParserMutex_ {};
    std::unordered_map<uint32_t, std::vector<int64_t>> rawDtsPrefix_ {};

    std::unordered_map<uint32_t, detail::TrackDfxInfo> trackDfxInfoMap_ {};
    DumpMode dumpMode_ {DumpMode::DUMP_NONE};
    static std::atomic<int> readatIndex_;
    int avpacketIndex_ {0};

    static void Dump(const detail::DumpParam &dumpParam);

    int64_t absolutePTSOfIndexZero_ = INT64_MAX;
    std::priority_queue<int64_t> index2RelativePTSHeap_;
    int64_t minRelativePTS_ = INT64_MAX;
    int64_t maxRelativePTS_ = INT64_MIN;
    int64_t minRelativePTSDiffOfLeft_ = INT64_MAX;
    int64_t minRelativePTSDiffOfRight_ = INT64_MAX;
    uint32_t index2RelativePTS_ = 0;

    std::shared_ptr<ReferenceParserManager> referenceParser_ = nullptr;
    bool isInit_ = false;
    std::mutex refPrepareMutex_ {};
    bool refPrepareAttempted_ = false;
    Status refPrepareStatus_ = Status::OK;
    std::vector<uint32_t> refPtsOrder_;
    bool refPtsOrderReady_ = false;
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
    std::shared_ptr<MPEG4Sample> cacheSample_ = nullptr;
    bool isReadSampleDataFromCache_ = false;

    int32_t CountTracks();
    void GetAVTrackIds();
    Status ParseAVFirstFrames();
    Status ParseAVFirstFrameForTrack(uint32_t trackId);
    Status ReadAudioMetadataProbeSample(const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t trackId,
        const std::shared_ptr<MPEG4Sample>& mpeg4Sample, bool& sampleReady);
    bool ProcessVideoFirstFrame(const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t trackId,
        const std::shared_ptr<MPEG4Sample>& mpeg4Sample);
    bool UpdateCodecConfigForSample(const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t sampleIndex);
    bool FirstFrameValid(uint32_t trackIndex);
    bool IsValidBuffer(const std::shared_ptr<AVBuffer>& outBuffer, bool replaceMemory);

    Status SetDrmCencInfo(uint32_t trackId, std::shared_ptr<AVBuffer> sample, int32_t sampleSize,
        int32_t oldSampleSize = -1, uint32_t sampleIndex = UINT32_MAX);
    Status BuildDrmCencBaseInfo(const std::shared_ptr<MPEG4AtomParser::Track>& track, MetaDrmCencInfo& cencInfo);
    Status FillDrmSampleEncryption(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        MetaDrmCencInfo& cencInfo, uint32_t localSampleIndex, int32_t sampleSize, int32_t oldSampleSize);
    Status FillDrmSubsamples(MetaDrmCencInfo& cencInfo,
        const MPEG4AtomParser::Mpeg4SampleEncryptionEntry& entry, int32_t sampleSize, int32_t oldSampleSize);
    bool IsMultiVideoTracks() const;
    bool IsSupportedCodec() const;
    bool GetReferenceParserCapability(int32_t& trackIndex, std::string& mimeType) const;
    Status EnsureReferenceParserPrepared();
    Status EnsureReferencePtsOrder();
    Status GetReferenceSampleTime(uint32_t sampleIndex, int64_t& ptsUs, int64_t& dtsUs) const;
    Status GetGopIdFromSeekPos(int64_t seekMs, int32_t &gopId);
    Status GetSampleIndexBySeekMs(int64_t seekMs, uint32_t &frameId);
    Status RefParserInit();
    Status EnsureParserRefInfoInitialized();
    Status ParserRefInfoLoop(std::shared_ptr<Sample> sample, uint32_t sampleIndex);
    Status PrepareRefParserSample(std::shared_ptr<Sample>& sample, uint32_t sampleIndex);
    Status UpdateParserGopId(int32_t syncSamplesPtsSize);
    Status SelectProGopId();
    int32_t GetSampleSizeWithCodecConfig(const std::shared_ptr<Sample>& sample, uint32_t trackId);

    Status ParseMoof(int64_t offset);
    Status HasCodecParameters();
    Status ConvertElstTimescaleToMs();
    Status GetMediaInfo();
    void GetSingleTrackInfo(MPEG4AtomParser::Track& track, const uint32_t trackIndex);
    void ResetParam();
    void ReleaseReferenceParserState();
    Status PrepareTrackMetadata();
    
    void InitStreamParsers();
    void InitBaseInfoByAtomParser(std::shared_ptr<MPEG4AtomParser> parser);
    void InitDrmInfoCacheByAtomParser(std::shared_ptr<MPEG4AtomParser> parser);
    bool TrackIsSelected(const uint32_t trackId);
    bool TrackIsSelectedLocked(const uint32_t trackId) const;
    bool IsReadConfigCurrentLocked(uint32_t trackId, uint64_t expectedReadConfigEpoch) const;
    Status BuildTrackSamplePacketAttr(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        const MPEG4SampleHelper::SampleIndexView& sampleView, uint32_t sampleIndex, ReadSamplePacketAttr& attr);
    void EnterRuntimeLocked();
    void InvalidateTrackCachesLocked(bool resetCursor, bool resetPhase);
    Status AlignTrackToReadCursorLocked(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        const detail::ReadCursor* cursor = nullptr);
    Status AlignTrackToPositionCursorLocked(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        size_t sampleCount, uint32_t& sampleIndex, const detail::ReadCursor* cursor = nullptr);
    Status AlignVideoTrackToPresentationCursorLocked(
        const std::shared_ptr<MPEG4AtomParser::Track>& track, size_t sampleCount, uint32_t& sampleIndex);
    Status AlignAudioTrackToPresentationCursorLocked(
        const std::shared_ptr<MPEG4AtomParser::Track>& track, size_t sampleCount, uint32_t& sampleIndex);
    Status AlignTrackToDecodeCursorLocked(const std::shared_ptr<MPEG4AtomParser::Track>& track, size_t sampleCount,
        uint32_t& sampleIndex, const detail::ReadCursor* cursor = nullptr);
    Status FindSampleAtOrAfterPts(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        int64_t targetPtsUs, detail::SampleReadInfo& sampleInfo, int64_t& sampleDtsUs);
    Status AdjustSeekCursorForSelectedVideoTracks(uint32_t seekTrackId, int64_t targetPtsUs,
        int64_t& cursorDtsUs, int64_t& cursorPos);
    Status CommitWebVTTSampleForCacheLocked(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        const std::shared_ptr<MPEG4Sample>& sample, uint64_t expectedReadConfigEpoch, uint32_t& cacheBytes);
    Status CacheReadSampleForCurrentTrack(const detail::SampleReadInfo& sampleInfo,
        const std::shared_ptr<MPEG4AtomParser::Track>& track, const std::shared_ptr<MPEG4Sample>& sample,
        uint64_t expectedReadConfigEpoch);
    Status ReadAndCacheNextSample(uint32_t trackId, uint64_t expectedReadConfigEpoch);
    Status ReadAndCacheResolvedSample(const detail::SampleReadInfo& sampleInfo, uint32_t trackId,
        uint64_t expectedReadConfigEpoch);
    Status FinalizeReadSample(const ReadSampleFinalizeContext& context);
    Status PrepareTrackForSelection(const std::shared_ptr<MPEG4AtomParser::Track>& track);
    Status CommitTrackSelection(uint32_t trackId, const std::shared_ptr<MPEG4AtomParser::Track>& track);
    void UpdateReadCursorLocked(const detail::SampleReadInfo& sampleInfo, int64_t dtsUs);
    Status SetInFlightSample(const detail::SampleReadInfo& sampleInfo, uint64_t expectedReadConfigEpoch);
    void ClearInFlightSampleLocked(const detail::SampleReadInfo& sampleInfo, uint64_t expectedReadConfigEpoch);
    Status UpdateReadCursor(const detail::SampleReadInfo& sampleInfo);
    Status UpdateReadCursor(const std::shared_ptr<MPEG4Sample>& sample, bool onlyIfUnset = false);
    Status SetReadCursorFromSeek(uint32_t trackId, uint32_t sampleIndex, SeekMode mode,
        bool usePresentationTime = true);
    std::shared_ptr<MPEG4AtomParser::Track> FindTrackById(int32_t trackId);

    Status DynamicBaseTrackSelect(int64_t seekTime, int64_t &maxPts, int32_t &baseTrackIndex);
    Status UpdateDynamicBaseTrack(const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t trackId,
        int64_t seekTime, detail::DynamicBaseTrackState& state);
    Status FindBaseTrackAndCalcDelay(int64_t seekTime, SeekMode mode, int32_t &trackId);
    Status FindFragmentSyncSample(uint32_t fragmentIndex, std::shared_ptr<MPEG4AtomParser::Track> track);
    Status FindFragmentAtTime(int64_t reqTime, SeekMode flag, std::shared_ptr<MPEG4AtomParser::Track> track,
        bool &findFragmentSync);
    Status FragmentSeek(int64_t seekTime, SeekMode mode, int32_t trackIndex, bool &findFragmentSync);
    Status PrepareSeekTime(const std::shared_ptr<MPEG4AtomParser::Track>& track, int64_t seekTime,
        int64_t& realSeekTime, detail::SeekTimeContext& context);
    Status CalculateRealSeekTime(const std::shared_ptr<MPEG4AtomParser::Track>& track, int64_t seekTime,
        int64_t& realSeekTime);
    Status ValidateElstShiftStartTimeScale(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        const detail::SeekTimeContext& context) const;
    Status ClampSeekTimeToStreamDuration(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        detail::SeekTimeContext& context);
    Status FindSeekSampleIndex(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        detail::SeekSampleIndexContext& context);
    Status DoSeekInternal(int32_t trackIndex, int64_t seekTime, SeekMode mode, int64_t& realSeekTime);
    Status EnsureSelectedTracksDense();
    Status GetVideoSyncSeekTime(const std::shared_ptr<MPEG4AtomParser::Track>& track,
        detail::VideoSyncSeekContext& context);
    Status MultiTrackSync(int32_t trackIndex, bool isBaseTrack, int64_t syncSeekTime, SeekMode mode);
    Status SeekToStartInternal();
    Status CheckSeekToFrameByDtsParam(int32_t trackId, int64_t seekTime, SeekMode mode);
    void ResetCacheForSeek();
    Status PrepareSeekToFrameByDts(int32_t trackId, int64_t seekTime, detail::SeekToFrameByDtsContext& context);
    Status FindValidSample(detail::SampleReadInfo& sampleInfo);
    Status GetUnseekableSampleInfo(detail::SampleReadInfo& sampleInfo);
    Status GetSeekableSampleInfo(uint32_t trackId, detail::SampleReadInfo& sampleInfo);
    Status CheckReadSampleDataStopState(const std::shared_ptr<Sample>& sample);
    Status ReadSampleData(std::shared_ptr<Sample> sample, const detail::SampleReadInfo& sampleInfo);
    Status CheckReadSampleDataState(int32_t size);
    Status ReadSampleDataToBuffer(std::shared_ptr<Sample> sample, uint64_t pos, int32_t size);
    Status UpdateReadSampleDataTimeout(Status ret, std::shared_ptr<Sample> sample);
    void DumpReadAtData(uint8_t* data [[maybe_unused]], uint64_t pos [[maybe_unused]],
        size_t size [[maybe_unused]]);
    void DumpAVPacketData(const std::shared_ptr<Sample>& sample [[maybe_unused]],
        uint32_t trackId [[maybe_unused]], uint64_t pos [[maybe_unused]]);
    void DumpAVBufferData(const uint8_t* data [[maybe_unused]], uint32_t trackId [[maybe_unused]],
        int32_t size [[maybe_unused]], int64_t pts [[maybe_unused]]);
    Status ReadSampleData(std::shared_ptr<MPEG4Sample>& mpeg4Sample, const detail::SampleReadInfo& sampleInfo);
    Status AddSampleToCacheQueue(std::shared_ptr<Sample> sample,
        const uint32_t trackId, uint32_t sampleIndex = UINT32_MAX);
    Status AddMPEG4SampleToCacheQueue(std::shared_ptr<MPEG4Sample> cacheSample,
        const uint32_t trackId, uint32_t sampleIndex = UINT32_MAX);
    void ClearTrackCache(uint32_t trackId);
    bool HasTrackCache(uint32_t trackId);
    bool HasTrackCache(const std::shared_ptr<MPEG4AtomParser::Track>& track) const;
    uint32_t GetTrackCacheDataSize(uint32_t trackId);
    uint32_t GetTrackCacheFrameCount(uint32_t trackId);
    bool ResetTrackCacheInfo(const std::shared_ptr<MPEG4Sample>& mpeg4Sample);
    bool SetTrackCacheInfo(const std::shared_ptr<MPEG4Sample>& mpeg4Sample);
    bool PushTrackCache(uint32_t trackId, const std::shared_ptr<MPEG4Sample>& mpeg4Sample);
    std::shared_ptr<MPEG4Sample> PopTrackCache(uint32_t trackId);
    std::shared_ptr<MPEG4Sample> FrontTrackCache(uint32_t trackId);
    std::shared_ptr<MPEG4Sample> BackTrackCache(uint32_t trackId);
    Status GetTrackCacheLastPTS(uint32_t trackId, int64_t& maxPts);
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
    bool GetAnnexbOutputBufferSize(uint32_t trackId, const std::shared_ptr<Sample>& sample,
        int32_t& outputBufferSize);
    bool CanConvertSampleToAnnexbDirectly(const std::shared_ptr<MPEG4Sample>& mpeg4Sample, uint32_t trackId,
        bool& isAvc);
    Status PrepareAnnexbBufferConversion(const std::shared_ptr<MPEG4Sample>& mpeg4Sample, uint32_t trackId,
        detail::AnnexbBufferConvertContext& context);
    Status HandleAnnexbBufferConversionResult(const std::shared_ptr<Sample>& srcSample,
        detail::AnnexbBufferConvertContext& context, bool convertRet);
    Status CopyOriginalSampleToAnnexbBuffer(const std::shared_ptr<Sample>& srcSample,
        detail::AnnexbBufferConvertContext& context);
    Status TryConvertSampleToAnnexbToBuffer(const std::shared_ptr<MPEG4Sample>& mpeg4Sample, uint32_t trackId,
        detail::AnnexbBufferConvertContext& context);
    Status SetEosSample(std::shared_ptr<AVBuffer> sample);
    Status SetEosSample(std::shared_ptr<AVBuffer> sample, bool replaceMemory);
    Status GetSampleBySeekableStatus(
        uint32_t trackId, std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer);
    Status GetSeekableSample(
        uint32_t trackId, std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer);
    Status GetUnseekableSample(
        uint32_t trackId, std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer);
    Status GetSampleForWebvtt(
        uint32_t trackId, std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer);
    Status GetSeekableSampleDirectly(uint32_t trackId, std::shared_ptr<AVBuffer> outBuffer);
    Status FillBufferDirectly(std::shared_ptr<AVBuffer> sample, uint64_t pos, int32_t size);
    Status WriteBufferAttr(const detail::SampleReadInfo& sampleInfo, std::shared_ptr<AVBuffer> outBuffer,
        int32_t dataSize, bool memoryNotEnough);
    Status ReadAndCacheWebVttSample(const detail::WebVttReadContext& context, bool& continueRead);
    Status UpdateWebVttCachedSampleDuration(const detail::WebVttReadContext& context,
        const std::shared_ptr<MPEG4Sample>& tempSample, uint32_t nextSampleIndex);
    uint32_t GetFlags(uint32_t sampleFlag, bool memoryNotEnough);
    Status WriteSampleAttr(const detail::SampleReadInfo& sampleInfo, std::shared_ptr<Sample> sample);
    Status ProcessWebVTTSample(std::shared_ptr<MPEG4Sample> mpeg4Sample);
    Status WriteBuffer(
        uint32_t trackIndex, std::shared_ptr<AVBuffer> outBuffer, std::shared_ptr<MPEG4Sample> mpeg4Sample);
    void SetOutputBufferTiming(uint32_t trackIndex, const std::shared_ptr<AVBuffer>& outBuffer,
        const std::shared_ptr<Sample>& sample);
    void FinishDirectAnnexbBufferWrite(uint32_t trackIndex, const std::shared_ptr<AVBuffer>& outBuffer,
        const std::shared_ptr<MPEG4Sample>& mpeg4Sample, int32_t oldSize,
        const detail::AnnexbBufferConvertContext& context);
    Status WriteConvertedSampleToBuffer(uint32_t trackIndex, const std::shared_ptr<AVBuffer>& outBuffer,
        const std::shared_ptr<MPEG4Sample>& mpeg4Sample, int32_t oldSize);
    Status ConvertSampleMemoryToAnnexb(uint32_t trackIndex, const std::shared_ptr<MPEG4Sample>& mpeg4Sample,
        std::unique_ptr<uint8_t[]>& directData, detail::AnnexbBufferConvertContext& context);
    Status WriteSampleMemory(
        uint32_t trackIndex, std::shared_ptr<AVBuffer> outBuffer, std::shared_ptr<MPEG4Sample> mpeg4Sample);
    Status SkipPacketAfterPacketConvertError(uint32_t trackId, const std::shared_ptr<MPEG4Sample>& mpeg4Sample,
        bool popSample, bool restoreCacheInfo);
    void CheckResetXPSSendStatus(uint32_t trackIndex, std::shared_ptr<Sample> sample);

    friend class MpegAudioPacketParser;

    Status CheckCacheDataLimit(uint32_t trackId);
    void ParseHEVCMetadataInfo(const MPEG4AtomParser::Track& track, Meta &format);
    void ParseAVCMetadataInfo(const MPEG4AtomParser::Track& track, Meta &format);
    void ConvertCsdToAnnexb(MPEG4AtomParser::Track& track, Meta& format, uint32_t trackIndex,
        bool parseVideoMetadataFromCsd);
    int SelectSeekTrack();
    Status CheckSeekParams(int64_t seekTime, SeekMode mode) const;

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
        uint32_t sampleIndex);
    Status UpdatePTSBySttsAndCtts(int64_t& pts, int64_t dts, int32_t timeScale,
        int64_t sampleOffsetOfCurrentCttsEntry);
    void ProcessIndex2RelativePTS(int64_t pts, uint32_t index);
    void ProcessRelativePTS2Index(int64_t pts, int64_t absolutePTS);

    Status WaitForLoop(uint32_t trackId, uint32_t timeout);
    Status EnsureReadLoopStarted();
    bool ShouldWaitForRead(uint32_t trackId);
    bool ShouldWaitForRead(const std::shared_ptr<MPEG4AtomParser::Track>& track);
    void MPEG4ReadLoop();
    bool NeedWaitForRead(uint32_t trackId);
    void HandleReadWait();
    bool ReadAndCacheSample();
    // Caller holds readConfigMutex_; keep read-config state ahead of the per-track cache lock.
    Status CacheSampleForCurrentReadLocked(uint32_t trackId, uint64_t expectedReadConfigEpoch,
        const std::shared_ptr<MPEG4Sample>& sample);
    Status ReadSampleToCache(uint32_t trackId, uint64_t expectedReadConfigEpoch);
    Status PushEosToCache(uint32_t trackId, uint64_t expectedReadConfigEpoch);
    bool IsWebVTTTrack(const std::shared_ptr<MPEG4AtomParser::Track>& track) const;
    void AdvanceSampleIndex(const std::shared_ptr<MPEG4AtomParser::Track>& track);
    Status ProcessWebVTTSampleForCache(
        const std::shared_ptr<MPEG4AtomParser::Track>& track, std::shared_ptr<MPEG4Sample> mpeg4Sample,
        uint64_t expectedReadConfigEpoch);
    Status FlushPendingWebVTTSample(uint32_t trackId, uint64_t expectedReadConfigEpoch);
    Status FlushPendingWebVTTSamples();
    int64_t GetWebVTTFlushEndPtsUs(
        const std::shared_ptr<MPEG4AtomParser::Track>& track, const std::shared_ptr<Sample>& sample) const;
    void ReleaseReadLoop();
    void UpdateAsyncReadThreadPriority();

    static constexpr uint32_t ReadModeToFlags(detail::ReadMode mode)
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
    std::atomic<detail::ThreadState> threadState_ {detail::ThreadState::NOT_STARTED};
    std::atomic<Status> readLoopStatus_ {Status::OK};
    std::atomic<bool> isPauseReadPacket_ {false};
    std::atomic<bool> threadReady_ {false};
    std::atomic<bool> isWaitingForReadThread_ {false};
    std::atomic<bool> stopReadThread_ {false};
    std::atomic<uint32_t> readModeFlags_ {0};
    std::atomic<bool> isAsyncReadThreadPrioritySet_ {false};
    std::atomic<detail::AVReadPacketStopState> avReadPacketStopState_ {detail::AVReadPacketStopState::UNSET};
    std::atomic<int64_t> readTimeoutMs_ {-1};
    std::atomic<int64_t> readStartTimeMs_ {0};
    CachePressureCallback cachePressureCb_ {nullptr};
    std::unordered_map<uint32_t, uint32_t> trackCacheLimitMap_ {};
    std::unordered_map<uint32_t, uint32_t> trackThrottleWindowMs_ {};
    std::unordered_map<uint32_t, int64_t> trackLastNotifyMs_ {};
    std::mutex cachePressureMutex_ {};

    // DRM info cache
    std::multimap<std::string, std::vector<uint8_t>> cachedDrmInfo_;
    std::mutex cachedDrmInfoMutex_;
    std::atomic<bool> drmInfoCached_ {false};
};
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_DEMUXER_PLUGIN_H
