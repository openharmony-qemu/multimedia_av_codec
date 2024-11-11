/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef PTS_AND_INDEX_CONVERSION_H
#define PTS_AND_INDEX_CONVERSION_H

#include "securec.h"
#include <atomic>
#include <vector>
#include <thread>
#include <map>
#include <queue>
#include <shared_mutex>
#include <list>
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "common/media_source.h"
#include "stream_demuxer.h"
#include "base_stream_demuxer.h"
#include "demuxer_plugin_manager.h"

#define BOX_TYPE_FTYP "ftyp"
#define BOX_TYPE_MOOV "moov"
#define BOX_TYPE_MVHD "mvhd"
#define BOX_TYPE_TRAK "trak"
#define BOX_TYPE_MDIA "mdia"
#define BOX_TYPE_MINF "minf"
#define BOX_TYPE_STBL "stbl"
#define BOX_TYPE_STTS "stts"
#define BOX_TYPE_CTTS "ctts"
#define BOX_TYPE_HDLR "hdlr"
#define BOX_TYPE_MDHD "mdhd"

namespace OHOS {
namespace Media {
using StreamDemuxer = OHOS::Media::StreamDemuxer;
using DataSourceImpl = OHOS::Media::DataSourceImpl;
using MediaSource = OHOS::Media::Plugins::MediaSource;
using DataSource = OHOS::Media::Plugins::DataSource;
using Buffer = OHOS::Media::Plugins::Buffer;
class Source;

namespace TimeAndIndex {
class TimeAndIndexConversion {
public:
    explicit TimeAndIndexConversion();
    ~TimeAndIndexConversion() ;
    Status SetDataSource(const std::shared_ptr<MediaSource>& source);
    Status GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
        const uint64_t relativePresentationTimeUs, uint32_t &index);
    Status GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
        const uint32_t index, uint64_t &relativePresentationTimeUs);
private:
    enum IndexAndPTSConvertMode : unsigned int {
        GET_FIRST_PTS,
        INDEX_TO_RELATIVEPTS,
        RELATIVEPTS_TO_INDEX,
    };

    enum TrakType : unsigned int {
        AUDIO,
        VIDIO,
    };

    struct BoxHeader {
        uint32_t size;
        char type[5];
    };

    struct STTSEntry {
        uint32_t sampleCount;
        uint32_t sampleDelta;
    };

    struct CTTSEntry {
        uint32_t sampleCount;
        uint32_t sampleOffset;
    };

    struct TrakInfo {
        uint32_t trakId;
        TrakType trakType;
        int32_t timeScale;
        std::vector<STTSEntry> sttsEntries;
        std::vector<CTTSEntry> cttsEntries;
    };

    std::mutex mutex_ {};
    std::shared_ptr<Source> source_ {nullptr};
    uint64_t mediaDataSize_;
    int offset_ = 0;
    uint64_t fileSize_ = 0;

    TrakInfo curTrakInfo_;
    uint32_t curTrakInfoIndex_ = 0;
    std::vector<TrakInfo> trakInfoVec_;

    std::map<std::string, void(TimeAndIndexConversion::*)(uint32_t)> boxParsers = {
        {BOX_TYPE_STTS, &TimeAndIndexConversion::ParseStts},
        {BOX_TYPE_CTTS, &TimeAndIndexConversion::ParseCtts},
        {BOX_TYPE_HDLR, &TimeAndIndexConversion::ParseHdlr},
        {BOX_TYPE_MDHD, &TimeAndIndexConversion::ParseMdhd},
        {BOX_TYPE_STBL, &TimeAndIndexConversion::ParseBox},
        {BOX_TYPE_MINF, &TimeAndIndexConversion::ParseBox},
        {BOX_TYPE_MDIA, &TimeAndIndexConversion::ParseBox},
    };

    void StartParse();
    std::shared_ptr<Buffer> ReadBufferFromDataSource(size_t bufSize);
    void ReadBoxHeader(std::shared_ptr<Buffer> buffer, BoxHeader &header);
    bool IsMP4orMOV();
    void ParseMoov(uint32_t boxSize);
    void ParseMvhd(uint32_t boxSize);
    void ParseTrak(uint32_t boxSize);
    void ParseBox(uint32_t boxSize);
    void ParseCtts(uint32_t boxSize);
    void ParseStts(uint32_t boxSize);
    void ParseHdlr(uint32_t boxSize);
    void ParseMdhd(uint32_t boxSize);

    Status GetPresentationTimeUsFromFfmpegMOV(IndexAndPTSConvertMode mode,
        uint32_t trackIndex, int64_t absolutePTS, uint32_t index);
    Status PTSAndIndexConvertSttsAndCttsProcess(IndexAndPTSConvertMode mode,
        int64_t absolutePTS, uint32_t index);
    Status PTSAndIndexConvertOnlySttsProcess(IndexAndPTSConvertMode mode,
        int64_t absolutePTS, uint32_t index);
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
    uint32_t curConvertTrakInfoIndex_ = 0;
};
} // namespace TimeAndIndex
} // namespace Media
} // namespace OHOS
#endif // PTS_AND_INDEX_CONVERSION_H