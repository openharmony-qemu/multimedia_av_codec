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
#include <map>
#include <queue>
#include <shared_mutex>
#include <list>
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
    Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample, uint32_t timeout) override
        { return Status::ERROR_UNKNOWN; }
    Status GetNextSampleSize(uint32_t trackId, int32_t& size) override;
    Status GetNextSampleSize(uint32_t trackId, int32_t& size, uint32_t timeout) override
        { return Status::ERROR_UNKNOWN; }
    Status Pause() override { return Status::ERROR_UNKNOWN; }
    Status GetLastPTSByTrackId(uint32_t trackId, int64_t &lastPTS) override { return Status::ERROR_UNKNOWN; }
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
    Status BoostReadThreadPriority() override { return Status::ERROR_UNKNOWN; }
    Status SetAVReadPacketStopState(bool state) override { return Status::ERROR_UNKNOWN; }
    Status SetReadTimeoutForInitSeek(int64_t timeoutMs) override { return Status::OK; }
    Status ReadSampleZeroCopy(uint32_t trackId, std::shared_ptr<AVBuffer> sample, uint32_t timeout) override
        { return Status::ERROR_UNKNOWN; };
    Status GetCurrentCacheFrameCount(uint32_t trackId, uint32_t& frameCount) override;
    Status SeekToStart() override { return Status::ERROR_UNKNOWN; };
    Status SeekToKeyFrame(
        int32_t trackId, int64_t seekTime, SeekMode mode, int64_t& realSeekTime, uint32_t timeoutMs) override;
    Status SeekToFrameByDts(int32_t trackId, int64_t seekTime,
        SeekMode mode, int64_t& realSeekTime, uint32_t timeoutMs) override { return Status::ERROR_UNKNOWN; };

    // cache pressure control
    Status SetCachePressureCallback(CachePressureCallback cb) override { return Status::ERROR_UNKNOWN; };
    Status SetTrackCacheLimit(uint32_t trackId, uint32_t limitBytes, uint32_t windowMs = 500) override
        { return Status::ERROR_UNKNOWN; };
private:
    enum IndexAndPTSConversionMode : unsigned int {
        GET_FIRST_PTS,
        INDEX_TO_RELATIVEPTS,
        RELATIVEPTS_TO_INDEX,
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
    Status FindBaseTrackAndCalcDelay(int64_t seekTime, SeekMode mode, int32_t &trackId);
    Status FindFragmentSyncSample(uint32_t fragmentIndex, std::shared_ptr<MPEG4AtomParser::Track> track);
    Status FindFragmentAtTime(int64_t reqTime, SeekMode flag, std::shared_ptr<MPEG4AtomParser::Track> track,
        bool &findFragmentSync);
    Status FragmentSeek(int64_t seekTime, SeekMode mode, int32_t trackIndex, bool &findFragmentSync);
    Status DoSeekInternal(int32_t trackIndex, int64_t seekTime, SeekMode mode, int64_t& realSeekTime);
    Status MultiTrackSync(int32_t trackIndex, bool isBaseTrack, int64_t syncSeekTime, SeekMode mode);
    
    Status FindValidSample(std::shared_ptr<MPEG4AtomParser::Track> &track,
        std::shared_ptr<MPEG4SampleHelper::SampleIndexEntry> &sample);
    Status GetUnseekableSampleInfo(uint32_t &trackId, int64_t &pos, int32_t &size);
    Status GetSeekableSampleInfo(uint32_t trackId, int64_t &pos, int32_t &size);
    Status ReadSampleData(std::shared_ptr<Sample> sample, uint64_t pos, int32_t size, uint32_t trackId);
    Status ReadSampleData(std::shared_ptr<MPEG4Sample>& mpeg4Sample, uint64_t pos, int32_t size, uint32_t trackId);
    Status AddSampleToCacheQueue(std::shared_ptr<Sample> sample, const uint32_t trackId);
    Status ConvertSampleToAnnexb(std::shared_ptr<MPEG4Sample> dstMpeg4Sample, const uint32_t trackId);
    Status SetEosSample(std::shared_ptr<AVBuffer> sample);
    Status GetSampleBySeekableStatus(const Seekable seekable, uint32_t trackId,
        std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer);
    Status GetSampleByMimeType(const Seekable seekable, uint32_t trackId,
        std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer);
    uint32_t GetFlags(const std::shared_ptr<Sample> sample, bool memoryNotEnough);
    Status WriteSampleAttr(uint32_t trackId, std::shared_ptr<Sample> sample);
    Status ProcessWebVTTSample(std::shared_ptr<MPEG4Sample> mpeg4Sample);
    Status WriteBuffer(
        uint32_t trackIndex, std::shared_ptr<AVBuffer> outBuffer, std::shared_ptr<MPEG4Sample> mpeg4Sample);
    void CheckResetXPSSendStatus(uint32_t trackIndex, std::shared_ptr<Sample> sample);

    Status CheckCacheDataLimit(uint32_t trackId);
    void ParseHEVCMetadataInfo(const MPEG4AtomParser::Track& track, Meta &format);
    void ConvertCsdToAnnexb(const MPEG4AtomParser::Track& track, Meta& format, uint32_t trackIndex);
    int SelectSeekTrack();

    uint32_t GetSampleCount(uint32_t trackIndex);
    Status GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode mode, uint32_t trackIndex,
        int64_t absolutePTS, uint32_t sampleIndex);
    Status ConvertPTSAndIndexOnlyByStts(IndexAndPTSConversionMode mode, int64_t absolutePTS, uint32_t sampleIndex);
    Status ConvertPTSAndIndexBySttsAndCtts(IndexAndPTSConversionMode mode, int64_t absolutePTS, uint32_t sampleIndex);
};
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_DEMUXER_PLUGIN_H