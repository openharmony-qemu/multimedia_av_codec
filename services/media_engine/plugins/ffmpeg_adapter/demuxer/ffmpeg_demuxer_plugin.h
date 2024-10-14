/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include <shared_mutex>
#include <list>
#include "buffer/avbuffer.h"
#include "plugin/demuxer_plugin.h"
#include "block_queue_pool.h"
#include "stream_parser_manager.h"
#include "reference_parser_manager.h"
#include "meta/meta.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavformat/internal.h"
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
    Status GetNextSampleSize(uint32_t trackId, int32_t& size) override;
    Status GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo) override;
    void ResetEosStatus() override;
    Status ParserRefUpdatePos(int64_t timeStampMs, bool isForward = true) override;
    Status ParserRefInfo() override;
    Status GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo) override;
    Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo) override;
    Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo) override;
    Status GetIFramePos(std::vector<uint32_t> &IFramePos) override;
    Status Dts2FrameId(int64_t dts, uint32_t &frameId, bool offset = true) override;
    Status GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
        const uint64_t relativePresentationTimeUs, uint32_t &index) override;
    Status GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
        const uint32_t index, uint64_t &relativePresentationTimeUs) override;
    void SetCacheLimit(uint32_t limitSize) override;

private:
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
    };
    void ConvertCsdToAnnexb(const AVStream& avStream, Meta &format);
    int64_t GetFileDuration(const AVFormatContext& avFormatContext);
    int64_t GetStreamDuration(const AVStream& avStream);

    static int AVReadPacket(void* opaque, uint8_t* buf, int bufSize);
    static int AVWritePacket(void* opaque, uint8_t* buf, int bufSize);
    static int64_t AVSeek(void* opaque, int64_t offset, int whence);
    AVIOContext* AllocAVIOContext(int flags, IOContext *ioContext);
    std::shared_ptr<AVFormatContext> InitAVFormatContext(IOContext *ioContext);
    static int CheckContextIsValid(void* opaque, int &bufSize);
    void NotifyInitializationCompleted();

    void InitParser();
    void InitBitStreamContext(const AVStream& avStream);
    void ConvertAvcToAnnexb(AVPacket& pkt);
    Status PushEOSToAllCache();
    bool TrackIsSelected(const uint32_t trackId);
    Status ReadPacketToCacheQueue(const uint32_t readId);
    Status AddPacketToCacheQueue(AVPacket *pkt);
    Status SetDrmCencInfo(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket);
    void WriteBufferAttr(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket);
    Status ConvertAVPacketToSample(std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket);
    void ConvertPacketToAnnexb(std::shared_ptr<AVBuffer> sample, AVPacket* avpacket,
        std::shared_ptr<SamplePacket> dstSamplePacket);
    Status SetEosSample(std::shared_ptr<AVBuffer> sample);
    Status WriteBuffer(std::shared_ptr<AVBuffer> outBuffer, const uint8_t *writeData, int32_t writeSize);
    void ParseDrmInfo(const MetaDrmInfo *const metaDrmInfo, int32_t drmInfoSize,
        std::multimap<std::string, std::vector<uint8_t>>& drmInfo);
    bool GetNextFrame(const uint8_t *data, const uint32_t size);
    bool NeedCombineFrame(uint32_t trackId);
    AVPacket* CombinePackets(std::shared_ptr<SamplePacket> samplePacket);
    void ConvertHevcToAnnexb(AVPacket& pkt, std::shared_ptr<SamplePacket> samplePacket);
    void ConvertVvcToAnnexb(AVPacket& pkt, std::shared_ptr<SamplePacket> samplePacket);
    Status GetSeiInfo();

    void ParserFirstDts();
    Status ParserRefInit();
    Status ParserRefInfoLoop(AVPacket *pkt, uint32_t curStreamId);
    Status SelectProGopId();
    void ParserBoxInfo();
    bool WebvttPktProcess(AVPacket *pkt);
    bool IsWebvttMP4(const AVStream *avStream);
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
        int64_t pts, int64_t absolutePTS, uint32_t index);
    int64_t absolutePTSIndexZero_ = INT64_MAX;
    std::priority_queue<int64_t> indexToRelativePTSMaxHeap_;
    uint32_t indexToRelativePTSFrameCount_ = 0;
    uint32_t relativePTSToIndexPosition_ = 0;
    int64_t relativePTSToIndexPTSMin_ = INT64_MAX;
    int64_t relativePTSToIndexPTSMax_ = INT64_MIN;
    int64_t relativePTSToIndexRightDiff_ = INT64_MAX;
    int64_t relativePTSToIndexLeftDiff_ = INT64_MAX;
    int64_t relativePTSToIndexTempDiff_ = INT64_MAX;

    std::mutex mutex_ {};
    std::shared_mutex sharedMutex_;
    std::unordered_map<uint32_t, std::shared_ptr<std::mutex>> trackMtx_;
    Seekable seekable_;
    IOContext ioContext_;
    std::vector<uint32_t> selectedTrackIds_;
    BlockQueuePool cacheQueue_;

    std::shared_ptr<AVInputFormat> pluginImpl_ {nullptr};
    std::shared_ptr<AVFormatContext> formatContext_ {nullptr};
    std::shared_ptr<AVBSFContext> avbsfContext_ {nullptr};
    std::shared_ptr<StreamParserManager> streamParser_ {nullptr};
    bool streamParserInited_ {false};

    Status GetVideoFirstKeyFrame(uint32_t trackIndex);
    void ParseHEVCMetadataInfo(const AVStream& avStream, Meta &format);
    AVPacket *firstFrame_ = nullptr;

    std::atomic<bool> parserState_ = true;
    IOContext parserRefIoContext_;
    std::shared_ptr<AVFormatContext> parserRefFormatContext_{nullptr};
    int parserRefVideoStreamIdx_ = -1;
    std::shared_ptr<ReferenceParserManager> referenceParser_{nullptr};
    int32_t parserCurGopId_ = 0;
    int64_t pendingSeekMsTime_ = -1;
    std::list<uint32_t> processingIFrame_;
    std::vector<uint32_t> IFramePos_;
    double fps_{0};
    int64_t firstDts_ = 0;
    bool isSdtpExist_ = false;
    std::mutex syncMutex_;
    bool updatePosIsForward_ = true;
    bool isInit_ = false;
    uint32_t cachelimitSize_ = 0;
    bool outOfLimit_ = false;
    bool setLimitByUser = false;

    // dfx
    struct TrackDfxInfo {
        int frameIndex = 0; // for each track
        int64_t lastPts;
        int64_t lastPos;
        int64_t lastDurantion;
    };
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
};
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // FFMPEG_DEMUXER_PLUGIN_H
