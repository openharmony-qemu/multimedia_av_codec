/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MPEG4_SAMPLE_HELPER_H
#define MPEG4_SAMPLE_HELPER_H

#include <cstdint>
#include <vector>
#include "common/status.h"
#include "block_queue_pool.h"
#include "demuxer_data_reader.h"
#include "mpeg4_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
enum TrunFlag : uint32_t {
    TRUN_DATA_OFFSET        = 0x01,
    TRUN_FIRST_SAMPLE_FLAGS = 0x04,
    TRUN_SAMPLE_DURATION    = 0x100,
    TRUN_SAMPLE_SIZE        = 0x200,
    TRUN_SAMPLE_FLAGS       = 0x400,
    TRUN_SAMPLE_CTS         = 0x800
};

class MPEG4SampleHelper {
public:
    MPEG4SampleHelper();
    ~MPEG4SampleHelper();

    Status FindSyncSampleAtTime(int64_t reqTime, bool isBaseTrack, SeekMode flag, uint32_t &sampleIndex);
    Status SetTimeToSampleParams(int64_t dataOffset, size_t dataSize);
    Status SetCompositionTimeToSampleParams(int64_t dataOffset, size_t dataSize);
    Status SetSyncSampleParams(int64_t dataOffset, size_t dataSize);
    Status SetSampleSizeParams(uint32_t type, int64_t dataOffset, size_t dataSize);
    Status SetChunkOffsetParams(uint32_t type, int64_t dataOffset, size_t dataSize);
    Status SetSampleToChunkParams(int64_t dataOffset, size_t dataSize);
    Status SetTrackExtendsParams(int64_t dataOffset, size_t dataSize);
    Status SetTrackFragmentHeaderParams(int64_t dataOffset, int64_t dataSize, uint32_t flag, int64_t moofOffset);
    Status SetTrackFragmentDecodeTimeParams(int64_t dataOffset, int64_t dataSize);
    Status SetTrackFragmentRunParams(int64_t dataOffset, int64_t dataSize, int64_t sidxDts, int64_t &moofDts,
        int64_t &duration);
    Status BuildSampleIndexEntries();
    Status GetSampleDuration(uint32_t index, uint32_t &delta);
    void SetTimeOffset(int64_t timeOffset);
    void CalculateDtsShift();
    
    uint32_t CalculateSampleCount();
    uint32_t GetSttsEntryCount();
    uint32_t GetcttsEntryCount();
    uint32_t GetSampleCount

    struct SampleIndexEntry {
        int64_t dts;
        int64_t pts;
        int64_t pos;
        uint32_t flag;
        int32_t size;
        int64_t duration;
    };
    struct CttsEntry {
        uint32_t count;
        int32_t delta;
    };

    std::shared_ptr<DemuxerDataReader> dataReader_;
    std::vector<SampleIndexEntry> sampleIndexEntry_;
    std::vector<CttsEntry> cttsEntry_;
    Seekable seekable_;
    TrackType trackType_;
    std::string mimeType_;
    int64_t firstMoofSampleIndex_;
    int64_t totalDuration_ = 0;
    int64_t totalTrunDuration_ = 0;
    int64_t lastPTS_ = 0;

private:
    struct TrackExtendsInfo {
        uint32_t trackId;
        uint32_t defaultSampleDescriptionIndex;
        uint32_t defaultSampleDuration;
        uint32_t defaultSampleSize;
        uint32_t defaultSampleFlags;
    };
    struct SttsEntry {
        uint32_t count;
        uint32_t delta;
    };
    struct StscEntry {
        uint32_t first;
        uint32_t count;
        uint32_t id;
    };
    struct StssEntry {
        uint32_t index;
        int64_t pts;
    };
    struct TrackFragmentHeaderInfo {
        enum Flags {
            TFHD_BASE_DATA_OFFSET       = 0x01,
            TFHD_STSD_ID                = 0x02,
            TFHD_DEFAULT_DURATION       = 0x08,
            TFHD_DEFAULT_SIZE           = 0x10,
            TFHD_DEFAULT_FLAGS          = 0x20,
            TFHD_DURATION_IS_EMPTY      = 0x010000,
            TFHD_DEFAULT_BASE_IS_MOOF   = 0x020000
        };
        uint32_t flags;
        uint64_t baseDataOffset;
        uint32_t sampleDescriptionIndex;
        uint32_t defaultSampleDuration;
        uint32_t defaultSampleSize;
    };
    struct SampleInfo {
        uint32_t duration = 0;
        uint32_t size = 0;
        uint32_t flag = 0;
        uint32_t ctsDelta = 0;
        uint32_t byteLength = 0;
        uint32_t dataOffset = 0;
        uint32_t index = 0;
    };

    std::vector<SttsEntry> sttsEntry_;
    std::vector<StscEntry> stscEntry_;
    std::vector<StssEntry> stssEntry_;
    std::vector<int32_t> sampleSizes_;
    std::vector<int64_t> chunkOffsets_;
    TrackExtendsInfo trex_;
    TrackFragmentHeaderInfo tfhd_;
    int64_t tfdtDts_ = -1;
    int64_t baseFragmentDts_ = -1;
    int64_t firstMoofSttsIndex_ = 0;
    uint32_t numMoovSamples_ = 0;
    bool hasTfhd_ = false;
    bool hasDefaultStszSize_ = false;
    int64_t dtsShift_ = 0;
    int64_t timeOffset_ = 0;

    void ProcessHalfByteSampleSizes(const uint8_t* data, uint32_t& entryIndex, uint32_t numSampleSizes);
    Status ExpandCttsEntry(uint32_t sttsNum);
    Status FindWithSyncSamples(int64_t inputPTS, bool isBaseTrack, SeekMode flag, uint32_t& syncIndex);
    Status CheckSyncSample(int64_t inputPTS, bool isBaseTrack, SeekMode flag, int64_t &currentIndex);
    Status SetFieldSize(uint32_t atomType, int64_t &currentOffset, uint32_t& fieldSize);
    Status ProcessSampleSizeData(uint32_t atomType, int64_t currentOffset, uint32_t size,
        uint32_t fieldSize, uint32_t numSampleSizes);
    Status PreBuildProcessAndCheck();
    bool GetSampleSize(uint32_t sampleIndex, uint32_t& samplesPerFrame, SampleIndexEntry& entry,
        int64_t& remainSamples);
    void GetSampleFlag(uint32_t sampleIndex, SampleIndexEntry& entry, uint32_t& syncIndex);
    void InitMoofParam();
    Status InitSampleInfo(SampleInfo& sampleInfo, uint32_t& bufferOffset, uint16_t trunFlag,
        uint8_t* data, uint32_t& firstSampleFlag);
    void UpdateSampleInfo(SampleInfo& sampleInfo, uint32_t& bufferOffset, uint16_t trunFlag,
        uint8_t* data, uint32_t firstSampleFlag);
    void UpdateSampleEntryInfo(SampleInfo sampleInfo, int64_t& offset, int64_t& currentDts, bool isOverlap = false);
};
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_SAMPLE_HELPER_H