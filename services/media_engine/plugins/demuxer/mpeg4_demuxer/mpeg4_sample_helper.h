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
#include <limits>
#include <unordered_map>
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

    Status FindSyncSampleByPts(int64_t reqPts, bool isBaseTrack, SeekMode flag, uint32_t &sampleIndex);
    Status SetTimeToSampleParams(int64_t dataOffset, size_t dataSize);
    Status SetCompositionTimeToSampleParams(int64_t dataOffset, size_t dataSize);
    Status SetSyncSampleParams(int64_t dataOffset, size_t dataSize);
    Status SetSampleSizeParams(uint32_t type, int64_t dataOffset, size_t dataSize);
    Status SetChunkOffsetParams(uint32_t type, int64_t dataOffset, size_t dataSize);
    Status SetSampleToChunkParams(int64_t dataOffset, size_t dataSize);
    Status SetSampleDependencyTypeParams(int64_t ctxOffset, int64_t dataOffset, size_t dataSize);
    Status SetTrackExtendsParams(int64_t dataOffset, size_t dataSize);
    Status SetTrackFragmentHeaderParams(int64_t dataOffset, int64_t dataSize, uint32_t flag, int64_t moofOffset);
    Status SetTrackFragmentDecodeTimeParams(int64_t dataOffset, int64_t dataSize);
    Status SetTrackFragmentRunParams(int64_t dataOffset, int64_t dataSize, int64_t sidxDts, int64_t &moofDts,
        int64_t &duration);
    Status BuildSampleIndexEntries();
    void UpdateRawSampleStats();
    Status GetSampleDuration(uint32_t index, uint32_t &delta);
    void SetTimeOffset(int64_t timeOffset);
    void CalculateDtsShift();

    uint32_t CalculateSampleCount();
    uint32_t GetSttsEntryCount();
    uint32_t GetSampleCountOfSttsEntry(uint32_t sttsEntryIndex);
    uint32_t GetSampleDeltaOfSttsEntry(uint32_t sttsEntryIndex);
    uint32_t GetCttsEntryCount();
    uint32_t GetSampleCountOfCttsEntry(uint32_t cttsEntryIndex);
    int32_t GetSampleOffsetOfCttsEntry(uint32_t cttsEntryIndex);
    int32_t GetMinOffsetOfCttsEntry();
    std::vector<uint32_t> GetSyncSamplesIndexFromStssEntry();
    Status GetPtsOfFirstSample(int64_t& startPtsBasedOnTimescale);

    struct EditListEntry {
        int64_t mediaTime = 0;
        int64_t duration = 0;
    };
    struct SampleIndexEntry {
        int64_t pos = 0;
        int64_t dts = 0;
        uint32_t ptsOffset = 0;
        uint32_t sizeAndDiscard = 0;

        static constexpr uint32_t discardMask = 1U << 31;
        static constexpr uint32_t sizeMask = discardMask - 1;

        bool SetPts(int64_t value)
        {
            int64_t offset = 0;
            return !__builtin_sub_overflow(value, dts, &offset) && offset >= 0 &&
                static_cast<uint64_t>(offset) <= std::numeric_limits<uint32_t>::max() &&
                (ptsOffset = static_cast<uint32_t>(offset), true);
        }

        bool GetPts(int64_t& value) const
        {
            return !__builtin_add_overflow(dts, static_cast<int64_t>(ptsOffset), &value);
        }

        int64_t GetPts() const
        {
            int64_t value = 0;
            (void)__builtin_add_overflow(dts, static_cast<int64_t>(ptsOffset), &value);
            return value;
        }

        bool SetSize(uint32_t value)
        {
            if (value > sizeMask) {
                return false;
            }
            sizeAndDiscard = (sizeAndDiscard & discardMask) | value;
            return true;
        }

        int32_t GetSize() const
        {
            return static_cast<int32_t>(sizeAndDiscard & sizeMask);
        }

        void SetDiscard(bool discard)
        {
            sizeAndDiscard = discard ? sizeAndDiscard | discardMask : sizeAndDiscard & sizeMask;
        }

        bool IsDiscard() const
        {
            return (sizeAndDiscard & discardMask) != 0;
        }
    };
    static_assert(sizeof(SampleIndexEntry) == 24, "SampleIndexEntry must remain compact");

    struct SampleIndexView {
        int64_t pos = 0;
        int64_t dts = 0;
        int64_t pts = 0;
        int64_t duration = 0;
        uint32_t flag = 0;
        int32_t size = 0;
    };
    struct CttsEntry {
        uint32_t count;
        int32_t delta;
    };

    struct SdtpEntry {
        uint32_t sdtpEntryCount = 0;
        std::vector<uint8_t> sdtpData;
    };
    
    void SetEditListParams(const std::vector<EditListEntry>& editListEntries, int32_t movieTimeScale,
        int32_t trackTimeScale);
    bool HasEditList() const;
    bool HasAppliedEditList() const;
    int64_t GetRawTotalSize() const;
    int64_t GetRawTotalDuration() const;
    int64_t GetRawSampleCount() const;
    bool HasTrailingZeroSttsDuration() const;
    uint32_t EstimateVideoDelay() const;
    void SetFragmented(bool fragmented);
    int64_t GetBaseDataOffset() const { return tfhd_.baseDataOffset; }
    SdtpEntry GetSdtpEntry();
    bool HasSkipSamplesInfo(uint32_t sampleIndex) const;
    const std::vector<uint8_t>* GetSkipSamplesInfo(uint32_t sampleIndex) const;
    size_t GetSampleCount() const;
    bool IsSampleIndexEmpty() const;
    Status GetSampleIndexView(uint32_t sampleIndex, SampleIndexView& view) const;
    Status GetSampleSize(uint32_t sampleIndex, int32_t& size) const;
    Status GetSampleRange(uint32_t sampleIndex, int64_t& pos, int32_t& size) const;
    Status GetSamplePts(uint32_t sampleIndex, int64_t& pts) const;
    Status GetSampleDts(uint32_t sampleIndex, int64_t& dts) const;
    Status GetSampleDurationTicks(uint32_t sampleIndex, int64_t& duration) const;
    bool IsSyncSample(uint32_t sampleIndex) const;
    bool IsDiscardSample(uint32_t sampleIndex) const;
    Status FindSampleIndexByPosition(int64_t position, uint32_t& sampleIndex) const;
    Status FindFirstSampleAfterPosition(int64_t position, uint32_t& sampleIndex) const;
    Status FindFirstSampleByDts(int64_t dts, uint32_t& sampleIndex) const;

    std::shared_ptr<DemuxerDataReader> dataReader_;
    std::vector<CttsEntry> cttsEntry_;
    Seekable seekable_;
    TrackType trackType_;
    std::string mimeType_;
    int64_t firstMoofSampleIndex_;
    int64_t totalDuration_ = 0;
    int64_t totalTrunDuration_ = 0;
    int64_t lastPTS_ = 0;

private:
    struct DurationOverride {
        uint32_t sampleIndex = 0;
        int64_t duration = 0;
    };
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
        int64_t baseDataOffset;
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
    struct TrunSampleInitContext {
        uint32_t trunFlag = 0;
        const uint8_t* data = nullptr;
        size_t dataSize = 0;
        uint32_t bufferOffset = 0;
        uint32_t firstSampleFlag = 0;
    };
    struct SampleIndexBuildContext {
        int64_t currentOffset = 0;
        uint32_t chunkIndex = 0;
        uint32_t stscIndex = 0;
        uint32_t sampleIndex = 0;
        int64_t currentDts = 0;
        uint32_t sampleInChunk = 0;
        uint32_t samplesPerFrame = 1;
        int64_t remainSamples = 0;
        uint64_t totalSttsSamples = 0;
        uint64_t processedSttsSamples = 0;
        uint32_t cttsIndex = 0;
        uint32_t cttsSamplesRemaining = 0;
        int32_t cttsDelta = 0;
        int64_t previousDuration = 0;
        bool hasPreviousDuration = false;
    };
    using SkipSamplesInfoMap = std::unordered_map<uint32_t, std::vector<uint8_t>>;
    struct EditListBuildContext {
        const std::vector<SampleIndexEntry>& oldEntries;
        const std::vector<DurationOverride>& oldDurationOverrides;
        const std::vector<StssEntry>& oldSyncSamples;
        bool oldAllSamplesSync;
        std::vector<SampleIndexEntry>& newEntries;
        std::vector<int64_t>& newDurations;
        std::vector<StssEntry>& newSyncSamples;
        SkipSamplesInfoMap& newSkipSamplesInfo;
        int64_t& editListDtsEntryEnd;
        int64_t& emptyEditsDuration;
        int64_t& minCorrectedPts;
        bool& foundNonEmptyEdit;
        int32_t& firstNonZeroAudioEdit;
        int64_t startDts;
    };
    struct EditListProcessContext {
        const EditListEntry* edit = nullptr;
        int64_t editDuration = 0;
        size_t startIndex = 0;
        int64_t dtsCounter = 0;
        int64_t editEndTime = 0;
        bool skipEntry = false;
        bool editListStartEncountered = false;
        bool foundKeyframeAfterEdit = false;
        bool continueAfterEdit = false;
        bool shouldBreak = false;
        bool newEntryIsSync = false;
        SampleIndexEntry newEntry;
        int64_t frameDuration = 0;
        int64_t frameEndTime = 0;
        std::vector<uint8_t> skipSamplesInfo;
        std::vector<int64_t> leadingDurations;
    };
    struct EditListRestoreGuard {
        std::vector<SampleIndexEntry>& entries;
        std::vector<DurationOverride>& overrides;
        std::vector<StssEntry>& syncSamples;
        bool& allSamplesSync;
        std::vector<SampleIndexEntry>& oldEntries;
        std::vector<DurationOverride>& oldOverrides;
        std::vector<StssEntry>& oldSyncSamples;
        bool oldAllSamplesSync;
        bool active = true;

        ~EditListRestoreGuard();
    };
    struct SampleEntryCreateContext {
        const SampleInfo& sampleInfo;
        int64_t offset = 0;
        int64_t currentDts = 0;
        bool isOverlap = false;
    };
    struct TfhdParseContext {
        uint32_t flag = 0;
        const uint8_t* data = nullptr;
        size_t dataSize = 0;
        size_t dataOffset = 0;
    };
    struct TfhdOptionalUint32Field {
        uint32_t flag = 0;
        uint32_t defaultValue = 0;
        uint32_t* output = nullptr;
    };

    std::vector<SttsEntry> sttsEntry_;
    std::vector<SampleIndexEntry> sampleIndexEntry_;
    std::vector<StscEntry> stscEntry_;
    std::vector<StssEntry> stssEntry_;
    std::vector<int32_t> sampleSizes_;
    std::vector<int64_t> chunkOffsets_;
    uint32_t sampleSizeCount_ = 0;
    SdtpEntry sdtpEntry_;
    TrackExtendsInfo trex_;
    TrackFragmentHeaderInfo tfhd_;
    int64_t tfdtDts_ = -1;
    int64_t baseFragmentDts_ = -1;
    int64_t firstMoofSttsIndex_ = 0;
    uint32_t numMoovSamples_ = 0;
    bool hasTfhd_ = false;
    bool hasDefaultStszSize_ = false;
    bool editListApplied_ = false;
    int64_t dtsShift_ = 0;
    int64_t timeOffset_ = 0;
    int64_t rawTotalSize_ = 0;
    int64_t rawTotalDuration_ = 0;
    int64_t rawSampleCount_ = 0;
    int32_t movieTimeScale_ = 0;
    int32_t trackTimeScale_ = 0;
    bool isFragmented_ = false;
    std::vector<EditListEntry> editListEntries_;
    SkipSamplesInfoMap skipSamplesInfoMap_;

    void ProcessHalfByteSampleSizes(const uint8_t* data, uint32_t& entryIndex, uint32_t numSampleSizes);
    Status ValidateCttsEntries(uint32_t sttsNum);
    Status ApplyEditList();
    Status FindWithSyncSamplesByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag, uint32_t& syncIndex);
    int64_t FindIndexWithoutSyncSamples(int64_t inputPTS) const;
    bool AssignSampleIndex(int64_t currentIndex, uint32_t& sampleIndex) const;
    bool AssignSampleIndexFromSize(size_t currentIndex, uint32_t& sampleIndex) const;
    bool ResolveDirectSyncSampleByPts(int64_t inputPts, int64_t currentIndex, uint32_t& sampleIndex) const;
    bool ResolveNonBaseTrackEosByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag, uint32_t& sampleIndex) const;
    bool IsAtOrBeyondLastSampleEnd(int64_t inputPts) const;
    bool ResolveTextWebVttSample(int64_t inputPTS, int64_t& currentIndex, uint32_t& sampleIndex) const;
    bool IsAudioSampleIndexInRange(int64_t currentIndex) const;
    bool IsInputCoveredByPreviousSample(int64_t inputPts, int64_t currentIndex) const;
    bool ShouldStepBackForBaseAudioCoverage(int64_t inputPts, SeekMode flag, int64_t currentIndex) const;
    bool ShouldStepBackForFirstNonBaseAudioSample(int64_t inputPts, bool isBaseTrack, int64_t currentIndex) const;
    bool ShouldStepBackForSyncPrerollSample(int64_t inputPts, bool isBaseTrack, bool fromSyncSamples,
        int64_t currentIndex) const;
    bool ShouldStepBackForShiftedAudioCoverage(int64_t inputPts, int64_t currentIndex) const;
    bool ShouldStepBackForNoSyncPrerollSample(int64_t inputPts, SeekMode flag, bool fromSyncSamples,
        int64_t currentIndex) const;
    bool ShouldStepBackForExactPtsPrerollSample(int64_t inputPts, bool isBaseTrack, SeekMode flag,
        bool fromSyncSamples, int64_t currentIndex) const;
    void AdjustAudioSeekSampleIndexByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag,
        bool fromSyncSamples, int64_t& currentIndex) const;
    Status ResolveEarlySeekSampleByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag,
        uint32_t& sampleIndex, bool& resolved) const;
    Status FindIndexWithSyncSamplesByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag,
        int64_t& currentIndex);
    int64_t FindPrevSyncByPts(int64_t inputPts, int64_t currentIndex) const;
    int64_t FindNextSyncByPts(int64_t inputPts, int64_t currentIndex) const;
    void SelectClosestSyncByPts(int64_t inputPts, int64_t& currentIndex) const;
    Status CheckSyncSampleByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag, int64_t &currentIndex);
    bool IsSyncSample(const std::vector<StssEntry>& syncSamples, bool allSamplesSync, uint32_t sampleIndex) const;
    void SetSkipSamplesInfo(uint32_t sampleIndex, std::vector<uint8_t>&& skipSamplesInfo);
    int64_t GetSyncSamplePts(size_t stssIndex) const;
    int64_t GetFrameDuration(const std::vector<SampleIndexEntry>& entries,
        const std::vector<DurationOverride>& durationOverrides, size_t index, int64_t editDuration) const;
    int64_t FindPrevClosestIndex(const std::vector<SampleIndexEntry>& entries,
        const std::vector<StssEntry>& syncSamples, bool allSamplesSync, int64_t timestampPts, bool anyFrame) const;
    void FixLeadingDiscardTimestamps(std::vector<SampleIndexEntry>& entries, size_t endIndex, int64_t endDts,
        const std::vector<int64_t>& leadingDurations) const;
    Status ConvertEditDuration(const EditListEntry& edit, int64_t& editDuration) const;
    Status PrepareEditListEntry(const EditListEntry& edit, EditListBuildContext& buildCtx,
        EditListProcessContext& processCtx);
    Status BuildEditListSampleEntry(const SampleIndexEntry& oldEntry, size_t index, EditListBuildContext& buildCtx,
        EditListProcessContext& processCtx);
    Status AppendEditListSamples(EditListBuildContext& buildCtx, EditListProcessContext& processCtx);
    Status AppendEditListSample(const SampleIndexEntry& oldEntry, size_t index, EditListBuildContext& buildCtx,
        EditListProcessContext& processCtx);
    void FinalizeAppliedEditList(EditListBuildContext& buildCtx);
    Status SetFieldSize(uint32_t atomType, int64_t &currentOffset, uint32_t& fieldSize);
    Status ProcessSampleSizeData(uint32_t atomType, int64_t currentOffset, uint32_t size,
        uint32_t fieldSize, uint32_t numSampleSizes);
    bool HasTfhdRemainingData(const TfhdParseContext& ctx, size_t fieldSize) const;
    Status ParseTfhdBaseDataOffset(TfhdParseContext& ctx, int64_t moofOffset);
    Status ParseTfhdOptionalUint32(TfhdParseContext& ctx, const TfhdOptionalUint32Field& field);
    Status PreBuildProcessAndCheck();
    uint64_t CountSttsSamples() const;
    SampleIndexBuildContext CreateSampleIndexBuildContext() const;
    Status AppendSampleIndexEntry(const SttsEntry& sttsEntry, uint32_t& sampleInSttsEntry,
        SampleIndexBuildContext& buildCtx);
    Status AdvanceChunkIfNeeded(SampleIndexBuildContext& buildCtx);
    bool GetSampleSize(uint32_t sampleIndex, uint32_t& samplesPerFrame, SampleIndexEntry& entry,
        int64_t& remainSamples);
    Status UpdateSampleDurationAndDts(uint32_t sttsDelta, uint32_t samplesPerFrame, int64_t& duration,
        int64_t& currentDts) const;
    void InitMoofParam();
    Status ReadTrunRunData(int64_t dataOffset, int64_t dataSize, uint32_t& trunFlag, uint32_t& numSamples,
        std::unique_ptr<uint8_t[]>& data);
    bool IsValidTrunSampleData(int64_t dataSize, uint32_t bufferOffset, uint32_t byteLength,
        uint32_t numSamples) const;
    Status InitSampleInfo(SampleInfo& sampleInfo, TrunSampleInitContext& initContext);
    void UpdateSampleInfo(SampleInfo& sampleInfo, uint32_t& bufferOffset, uint32_t trunFlag,
        uint8_t* data, uint32_t firstSampleFlag);
    Status CreateSampleEntry(const SampleEntryCreateContext& context, SampleIndexEntry& sample, bool& isSync) const;
    bool InsertSampleEntry(SampleIndexEntry&& sample, uint32_t duration, bool isSync);
    Status UpdateSampleEntryInfo(SampleInfo sampleInfo, int64_t& offset, int64_t& currentDts,
        bool isOverlap = false);
    Status SetEntryPts(SampleIndexEntry& entry, int64_t pts) const;
    Status SetEntrySize(SampleIndexEntry& entry, uint32_t size) const;
    void SetEntryDiscard(SampleIndexEntry& entry, bool discard) const;
    Status AddDurationOverride(uint32_t sampleIndex, int64_t duration);
    Status AddDurationOverride(std::vector<DurationOverride>& overrides, uint32_t sampleIndex, int64_t duration) const;
    const DurationOverride* FindDurationOverride(const std::vector<DurationOverride>& overrides,
        uint32_t sampleIndex) const;
    Status GetSampleDurationTicks(const std::vector<SampleIndexEntry>& entries,
        const std::vector<DurationOverride>& overrides, uint32_t sampleIndex, int64_t& duration) const;
    void ShiftDurationOverridesForInsert(uint32_t sampleIndex);
    void ConvertAllSyncSamplesToSparse(uint32_t sampleIndex);
    void ShiftSyncSampleIndices(uint32_t sampleIndex);
    void UpdateSyncSamplesForInsert(uint32_t sampleIndex, bool isSync);
    Status ConsumeCttsSamples(SampleIndexBuildContext& buildCtx, uint32_t sampleCount, int64_t& ptsOffset);

    std::vector<DurationOverride> durationOverrides_;
    bool allSamplesSync_ = false;
};
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_SAMPLE_HELPER_H
