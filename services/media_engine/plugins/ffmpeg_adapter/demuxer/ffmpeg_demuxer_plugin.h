/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef FFMPEG_DEMUXER_PLUGIN_H
#define FFMPEG_DEMUXER_PLUGIN_H

#include <atomic>
#include <vector>
#include <thread>
#include <map>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <list>
#include "buffer/avbuffer.h"
#include "plugin/demuxer_plugin.h"
#include "avpacket_memory.h"
#include "avpacket_wrapper.h"
#include "block_queue_pool.h"
#include "multi_stream_parser_manager.h"
#include "reference_parser_manager.h"
#include "meta/meta.h"
#include "qos.h"
#include "time_range_manager.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/dict.h"
#include "libavutil/opt.h"
#include "libavutil/parseutils.h"
#include "libavcodec/bsf.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
extern const std::vector<AVCodecID> g_streamContainedXPS;
class FFmpegDemuxerPlugin : public DemuxerPlugin {
public:
    explicit FFmpegDemuxerPlugin(std::string name);
    ~FFmpegDemuxerPlugin() override;
    Status Reset() override;
    Status Start() override;
    Status Stop() override;
    Status Flush() override;
    Status SetDataSource(const std::shared_ptr<DataSource>& source) override;
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
    void ResetEosStatus() override;
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
    Status SeekToStart() override;
    Status SeekToKeyFrame(int32_t trackId, int64_t seekTime,
        SeekMode mode, int64_t& realSeekTime, uint32_t timeoutMs) override;
private:
    enum ThreadState : unsigned int {
        NOT_STARTED,
        WAITING,
        READING,
    };
   
    enum InvokerType : unsigned int {
        INVOKER_NONE = 0,
        INIT,
        FLUSH,
        READ,
        SEEK,
        DESTORY,
    };

    enum DumpMode : unsigned long {
        DUMP_NONE = 0,
        DUMP_READAT_INPUT = 0b001,
        DUMP_AVPACKET_OUTPUT = 0b010,
        DUMP_AVBUFFER_OUTPUT = 0b100,
    };
    enum IndexAndPTSConvertMode : unsigned int {
        GET_FIRST_PTS,
        INDEX_TO_RELATIVEPTS,
        RELATIVEPTS_TO_INDEX,
        GET_ALL_FRAME_PTS,
    };
    enum AVReadPacketStopState : unsigned int {
        UNSET = 0,
        TRUE,
        FALSE,
    };
    struct IOContext {
        std::shared_ptr<DataSource> dataSource {nullptr};
        int64_t offset {0};
        uint64_t fileSize {0};
        bool eos {false};
        std::atomic<bool> retry {false};
        uint32_t initDownloadDataSize {0};
        std::atomic<bool> initCompleted {false};
        DumpMode dumpMode {DUMP_NONE};
        bool isLimit {false};
        bool isLimitType {false};
        int32_t sizeLimit {0};
        int32_t readSizeCnt {0};
        std::atomic<bool> initErrorAgain {false};
        std::mutex invokerTypeMutex;
        std::atomic<InvokerType> invokerType {INVOKER_NONE};
        std::atomic<bool> readCbReady {false};
        std::atomic<AVReadPacketStopState> avReadPacketStopState {UNSET};
    };

    // Snapshot of the AVStream fields needed at runtime, built in SetDataSource so
    // other threads can read stream metadata without touching FFmpeg structs directly.
    struct AVStreamSnapshot {
        bool valid {false};
        AVCodecID codecId {AV_CODEC_ID_NONE};
        AVMediaType codecType {AVMEDIA_TYPE_UNKNOWN};
        AVRational timeBase {0, 1};
        int32_t extradataSize {0};
        int64_t startTime {AV_NOPTS_VALUE};
        bool needCombineFrame {false};
        bool isVideo {false};
        bool isAudio {false};
    };

    bool SelectedVideo();
    bool NeedDropAfterSeek(uint32_t trackId, int64_t pts);
    std::atomic<int64_t> seekTime_ = AV_NOPTS_VALUE;
    std::atomic<SeekMode> seekMode_ = SeekMode::SEEK_NEXT_SYNC;
    void ConvertCsdToAnnexb(const AVStream& avStream, Meta &format);
    int64_t GetFileDuration(const AVFormatContext& avFormatContext);
    int64_t GetStreamDuration(const AVStream& avStream);

    bool FrameReady(Status ret);
    int SelectSeekTrack() const;
    Status CheckSeekParams(int64_t seekTime, SeekMode mode) const;
    void SyncSeekThread();
    Status DoSeekInternal(int trackIndex, int64_t seekTime, int64_t ffTime, SeekMode mode, int64_t& realSeekTime);
    bool IsUseFirstFrameDts(int trackIndex, int64_t seekTime);
    void GetStreamInitialParams();
    void SetStreamInitialParams(uint32_t trackId, Meta &format);

    static int AVReadPacket(void* opaque, uint8_t* buf, int bufSize);
    static int HandleReadOK(IOContext* ioContext, int dataSize);
    static int HandleReadAgain(IOContext* ioContext, int dataSize, int& tryCount);
    static int HandleReadEOS(IOContext* ioContext);
    static int HandleReadError(int result);
    static void UpdateInitDownloadData(IOContext* ioContext, int dataSize);
    static int AVWritePacket(void* opaque, uint8_t* buf, int bufSize);
    static int64_t AVSeek(void* opaque, int64_t offset, int whence);
    AVIOContext* AllocAVIOContext(int flags, IOContext *ioContext);
    std::shared_ptr<AVFormatContext> InitAVFormatContext(IOContext *ioContext);
    static int CheckContextIsValid(void* opaque, int &bufSize);
    void NotifyInitializationCompleted();

    void InitParser();
    void InitBitStreamContext(const AVStream& avStream);
    Status ConvertAvcToAnnexb(AVPacket& pkt);
    Status PushEOSToAllCache();
    bool TrackIsSelected(const uint32_t trackId);
    Status ReadPacketToCacheQueue(const uint32_t readId);
    Status AddPacketToCacheQueue(Plugins::AVPacketWrapperPtr pkt);
    Status SetDrmCencInfo(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket);
    void WriteBufferAttr(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket);
    Status BufferIsValid(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket);
    Status ConvertAVPacketToSample(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket);
    Status ConvertAVPacketToSampleMemory(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket);
    Status PreparePacketForConversion(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket,
        Plugins::AVPacketWrapperPtr& tempPktWrapper, bool& combined);
    Status PrepareBufferMetadata(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket,
        Plugins::AVPacketWrapperPtr tempPktWrapper, uint32_t& copySize);
    Status WritePacketDataAndUpdateInfo(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket,
        Plugins::AVPacketWrapperPtr tempPktWrapper, uint32_t copySize, bool combined);
    Status ConvertToAnnexbAndUpdateSample(std::shared_ptr<AVBuffer> sample,
        std::shared_ptr<SamplePacket> samplePacket, Plugins::AVPacketWrapperPtr& tempPktWrapper, bool combined);
    Status ConvertPacketToAnnexb(std::shared_ptr<AVBuffer> sample, Plugins::AVPacketWrapperPtr avpacketWrapper,
        std::shared_ptr<SamplePacket> dstSamplePacket);
    Status ConvertPacketToAnnexb(Plugins::AVPacketWrapperPtr srcWrapper, std::shared_ptr<SamplePacket> samplePacket,
        Plugins::AVPacketWrapperPtr& outWrapper);
    Status SetEosSample(std::shared_ptr<AVBuffer> sample);
    Status WriteBuffer(std::shared_ptr<AVBuffer> outBuffer, const uint8_t *writeData, uint32_t writeSize);
    void ParseDrmInfo(const MetaDrmInfo *const metaDrmInfo, size_t drmInfoSize,
        std::multimap<std::string, std::vector<uint8_t>>& drmInfo);
    void UpdateCachedDrmInfoFromStream(AVStream* avStream);
    bool NeedCombineFrame(uint32_t trackId);
    Plugins::AVPacketWrapperPtr CombinePackets(std::shared_ptr<SamplePacket> samplePacket);
    Status ConvertHevcToAnnexb(AVPacket& pkt, std::shared_ptr<SamplePacket> samplePacket);
    Status ConvertVvcToAnnexb(AVPacket& pkt, std::shared_ptr<SamplePacket> samplePacket);
    
    struct ConvertToAnnexbParams {
        std::shared_ptr<SamplePacket> samplePacket {nullptr};
        uint8_t *outBuffer {nullptr};
        int32_t outBufferSize {0};
        int32_t &outDataSize;
        uint8_t *sideData {nullptr};
        size_t sideDataSize {0};

        // 构造函数，用于初始化引用成员
        explicit ConvertToAnnexbParams(int32_t &outSizeRef) : outDataSize(outSizeRef) {}
    };
    Status ConvertHevcToAnnexb(AVPacket& pkt, const ConvertToAnnexbParams &params);
    Status ConvertVvcToAnnexb(AVPacket& pkt, const ConvertToAnnexbParams &params);
    Status ConvertPacketToAnnexbWithParser(AVPacket* srcAVPacket, std::shared_ptr<SamplePacket> samplePacket,
        Plugins::AVPacketWrapperPtr& outWrapper, const AVStreamSnapshot* snapshot);
    bool HasCodecParameters();
    Status GetMediaInfo();
    void ResetParam();
    void UpdateStreamSnapshots();
    const AVStreamSnapshot* GetStreamSnapshot(uint32_t trackId) const;

    bool WebvttPktProcess(AVPacket *pkt);
    bool IsWebvttMP4(const AVStream *avStream);
    bool IsLessMaxReferenceParserFrames(uint32_t trackIndex);
    void WebvttMP4EOSProcess(const AVPacket *pkt);
    Status CheckCacheDataLimit(uint32_t trackId);

    Status GetPresentationTimeUsFromFfmpegMOV(IndexAndPTSConvertMode mode,
        uint32_t trackIndex, int64_t absolutePTS, uint32_t index);
    Status PTSAndIndexConvertSttsAndCttsProcess(IndexAndPTSConvertMode mode,
        const AVStream* avStream, int64_t absolutePTS, uint32_t index);
    Status PTSAndIndexConvertOnlySttsProcess(IndexAndPTSConvertMode mode,
        const AVStream* avStream, int64_t absolutePTS, uint32_t index);
    void InitPTSandIndexConvert();
    void IndexToRelativePTSProcess(int64_t pts, uint32_t index);
    void RelativePTSToIndexProcess(int64_t pts, int64_t absolutePTS);
    void PTSAndIndexConvertSwitchProcess(IndexAndPTSConvertMode mode,
        int64_t pts, int64_t absolutePTS, uint32_t index, int64_t dts);
    void ResetContext();
    int64_t absolutePTSIndexZero_ = INT64_MAX;
    std::priority_queue<int64_t> indexToRelativePTSMaxHeap_;
    uint32_t indexToRelativePTSFrameCount_ = 0;
    uint32_t relativePTSToIndexPosition_ = 0;
    int64_t relativePTSToIndexPTSMin_ = INT64_MAX;
    int64_t relativePTSToIndexPTSMax_ = INT64_MIN;
    int64_t relativePTSToIndexRightDiff_ = INT64_MAX;
    int64_t relativePTSToIndexLeftDiff_ = INT64_MAX;
    int64_t relativePTSToIndexTempDiff_ = INT64_MAX;
    Status InitIoContext();
    void InitIoContextInDemuxer(const std::shared_ptr<DataSource>& source);
    Status ParserRefInit();
    Status ParserRefInfoLoop(AVPacket *pkt, uint32_t curStreamId);
    Status SelectProGopId();
    void ParserBoxInfo();
    AVStream *GetVideoStream();

    std::mutex mutex_ {};
    std::shared_mutex sharedMutex_;
    std::unordered_map<uint32_t, std::shared_ptr<std::mutex>> trackMtx_;
    Seekable seekable_;
    IOContext ioContext_;
    std::vector<uint32_t> selectedTrackIds_;
    BlockQueuePool cacheQueue_;
    MediaInfo mediaInfo_;
    FileType fileType_ = FileType::UNKNOW;

    std::shared_ptr<AVInputFormat> pluginImpl_ {nullptr};
    std::shared_ptr<AVFormatContext> formatContext_ {nullptr};
    std::map<uint32_t, std::shared_ptr<AVBSFContext>> avbsfContexts_ {};
    std::vector<AVStreamSnapshot> streamSnapshots_;
    
    void UpdateReferenceIds();
    std::map<int32_t, std::vector<int32_t>> referenceIdsMap_ {};

    Status ParseVideoFirstFrames();
    bool AllVideoFirstFramesReady();
    bool AllSupportTrackFramesReady();
    Status SetVideoFirstFrame(Plugins::AVPacketWrapperPtr pkt, bool isConvert = true);
    bool VideoFirstFrameValid(uint32_t trackIndex);
    std::map<int32_t, Plugins::AVPacketWrapperPtr> videoFirstFrameMap_ {};
    std::unordered_map<int32_t, int64_t> seekCalibMap_ {};
    bool TrackIsChecked(const uint32_t trackId);
    std::vector<uint32_t> checkedTrackIds_ {};
    void ClearUnselectTrackCache();
    bool needClear_ = true;

    std::shared_ptr<MultiStreamParserManager> streamParsers_ {nullptr};

    void ParseHEVCMetadataInfo(const AVStream& avStream, Meta &format);
    void ProcessHevcFirstFrame(uint32_t trackId, AVStream* avStream, bool parserReady, bool firstFrameReady,
                               Meta &meta);
    void BuildTrackMeta(uint32_t trackId, AVStream* avStream);
    std::atomic<bool> parserState_ = true;
    IOContext parserRefIoContext_;
    std::shared_ptr<AVFormatContext> parserRefCtx_{nullptr};
    int parserRefIdx_ = -1;
    std::shared_ptr<ReferenceParserManager> referenceParser_{nullptr};
    int32_t parserCurGopId_ = 0;
    int64_t pendingSeekMsTime_ = -1;
    int64_t parserRefStartTime_ = -1;
    std::list<uint32_t> processingIFrame_;
    std::vector<uint32_t> IFramePos_;
    int64_t minPts_ = 0;
    int64_t startPts_ = 0;
    uint32_t ptsCnt_ = 0;
    bool isSdtpExist_ = false;
    std::mutex syncMutex_;
    bool updatePosIsForward_ = true;
    bool isInit_ = false;
    uint32_t cachelimitSize_ = 0;
    bool outOfLimit_ = false;
    bool setLimitByUser = false;
    std::atomic<bool> isInterruptNeeded_{false};

    // dfx
    struct TrackDfxInfo {
        int frameIndex = 0; // for each track
        int64_t lastPts;
        int64_t lastPos;
        int64_t lastDuration;
        bool dumpFirstInfo = false;
    };
    enum Stage : int32_t {
        FIRST_READ = 0,
        FILE_END   = 1,
    };
    void UpdateLastPacketInfo(int32_t trackId, int64_t pts, int64_t pos, int64_t duration);
    void DumpPacketInfo(int32_t trackId, Stage stage);
    struct DumpParam {
        DumpMode mode;
        uint8_t* buf;
        int trackId;
        int64_t offset;
        int size;
        int index;
        int64_t pts;
        int64_t pos;
    };
    std::unordered_map<int, TrackDfxInfo> trackDfxInfoMap_;
    DumpMode dumpMode_ {DUMP_NONE};
    static std::atomic<int> readatIndex_;
    int avpacketIndex_ {0};

    static void Dump(const DumpParam &dumpParam);

    std::map<int64_t, int64_t> pts2DtsMap_;
    std::unordered_map<int32_t, int64_t> iFramePtsMap_;
    Status GetGopIdFromSeekPos(int64_t seekMs, int32_t &gopId);
    Status ParserRefCheckVideoValid(const AVStream *videoStream);
    bool IsMultiVideoTrack();
    int AVReadFrameLimit(AVPacket *pkt);
    Status SetAVReadFrameLimit();
    void SetAVReadFrameLimitDefault();
    bool Mp4CheckKeyFrame(AVStream* stream);

    Status WaitForLoop(const uint32_t trackId, const uint32_t timeout);
    bool ShouldWaitForRead(uint32_t trackId);
    void FFmpegReadLoop();
    bool NeedWaitForRead();
    void HandleReadWait();
    bool EnsurePacketAllocated(Plugins::AVPacketWrapperPtr& pktWrapper);
    bool ReadAndProcessFrame(Plugins::AVPacketWrapperPtr& pktWrapper);
    void HandleAVPacketEndOfStream(Plugins::AVPacketWrapperPtr& pktWrapper);
    void HandleAVPacketReadError(Plugins::AVPacketWrapperPtr& pktWrapper, int ffmpegRet);
    bool ReadOnePacketAndProcessWebVTT(Plugins::AVPacketWrapperPtr& pktWrapper);
    void ReleaseFFmpegReadLoop();
    std::unique_ptr<std::thread> readThread_ {nullptr};
    std::condition_variable readLoopCv_;
    static std::condition_variable readCbCv_;
    std::condition_variable readCacheCv_;
    static std::mutex readPacketMutex_;
    std::mutex getNextSampleMutex_;
    std::mutex readSampleMutex_;
    std::mutex fFmpegReadLoopMutex_;
    uint32_t trackId_ = 0;
    ThreadState threadState_ {ThreadState::NOT_STARTED};
    std::atomic<Status> readLoopStatus_ = {Status::OK};
    std::atomic<bool> isPauseReadPacket_ = false;
    std::unordered_map<int, int> readModeMap_; // 0 mean sync read, 1 mean async read
    std::mutex seekWaitMutex_;
    std::condition_variable seekWaitCv_;
    std::atomic<bool> threadReady_ {false};
    std::atomic<bool> isWaitingForReadThread_ {false};
    std::unordered_map<uint32_t, Meta> streamInitialParam_;

    std::atomic<bool> isAsyncReadThreadPrioritySet_ = false;
    void UpdateAsyncReadThreadPriority();

    // DRM info cache to avoid race condition in async mode
    std::multimap<std::string, std::vector<uint8_t>> cachedDrmInfo_;
    std::mutex cachedDrmInfoMutex_;
    std::atomic<bool> drmInfoCached_ {false};

    struct MinTsPacketInfo {
        bool isInit = false;
        bool isUpd = false;
        int32_t streamIndex = -1;
        int64_t minPts = AV_NOPTS_VALUE;
        int64_t minDts = AV_NOPTS_VALUE;
    };
    MinTsPacketInfo minTsPktInfo_ {};
    Status GetFileFirstPacket();
    void InitMinTsPacketInfo(AVPacket *pkt);
    void UpdMinTsPacketInfo(AVPacket *pkt);
    bool IsSkipGetMinTsPktInfo();
    Status SeekToStartInternal();

    int AVSeekFrameLock(int idx, int64_t timestamp, int flags);
    TimeRangeManager timeRangeManager_;
    Status ReadUntilKeyFrame(Plugins::AVPacketWrapperPtr pkt, int trackIndex,
        TimeoutGuard &timeoutGuard, TimeRange &readRange);
    Status SeekToKeyFrameCheckParam(int64_t seekTime, SeekMode mode,
        int32_t &trackIndex, int64_t &ffTime, AVStream* &avStream);
    void ResetAfterSeek(int64_t seekTime, SeekMode mode);

    std::unordered_map<int32_t, int32_t> mp4FirstKeyFrameIdx_; // key: track index, value: first key frame index
};

typedef struct DtsFinder {
    explicit DtsFinder(int64_t dts) : dts_(dts) { }
    bool operator ()(const std::map<int64_t, int64_t>::value_type &item)
    {
        return dts_ == item.second || dts_ == item.second - 1 || dts_ == item.second + 1;
    }
    int64_t dts_;
} DtsFinder;
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // FFMPEG_DEMUXER_PLUGIN_H
