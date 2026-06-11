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

#define HST_LOG_TAG "MPEG4SampleHelper"
#include <array>
#include <algorithm>
#include <limits>
#include <regex>
#include <iconv.h>
#include <sstream>
#include "meta/media_types.h"
#include "meta/mime_type.h"
#include "meta/video_types.h"
#include "meta/audio_types.h"
#include "common/log.h"
#include "mpeg4_sample_helper.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4SampleHelper" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
constexpr uint32_t DEFAULT_FIELD_SIZE = 32;
constexpr int64_t MAX_TRUN_SAMPLES = 1000000; //需要确定讨论
constexpr int64_t NEXT_FRAME = 1;
constexpr int64_t PREVIOUS_FRAME = 0;
constexpr int64_t MAX_SAMPLES_PER_FRAME = 1024;
constexpr size_t MIN_DATA_SIZE = 8;
constexpr int64_t MIN_DATA_SIZE_INT = 8;
constexpr int64_t MIN_DATA_OFFSET = 0;
constexpr uint32_t STSZ_HEAD_LENGTH = 12;  // stsz头数据长度
constexpr uint32_t HALF_BYTE_BITS = 4;
constexpr uint32_t BYTE_BITS = 8;
constexpr uint32_t DUL_BYTE_BITS = 16;
constexpr uint32_t MAX_REORDER_DELAY = 16;

Status ConvertTimeScaleRoundNearest(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result)
{
    int64_t scaled = 0;
    if (result == nullptr || timeScale == 0 || __builtin_mul_overflow(multiplicand, multiplier, &scaled)) {
        return Status::ERROR_INVALID_DATA;
    }
    int64_t quotient = scaled / timeScale;
    int64_t remainder = scaled % timeScale;
    int64_t absTimeScale = std::abs(timeScale);
    int64_t halfTimeScale = absTimeScale / 2;
    int64_t roundThreshold = absTimeScale - halfTimeScale;
    if (remainder > 0 && remainder >= roundThreshold) {
        if (quotient == std::numeric_limits<int64_t>::max()) {
            return Status::ERROR_INVALID_DATA;
        }
        ++quotient;
    } else if (remainder < 0 && -remainder >= roundThreshold) {
        if (quotient == std::numeric_limits<int64_t>::min()) {
            return Status::ERROR_INVALID_DATA;
        }
        --quotient;
    }
    *result = quotient;
    return Status::OK;
}
    
int64_t GetClosestSync(int64_t prevTime, int64_t nextTime, int64_t reqTime)
{
    int64_t prevDiff = std::abs(reqTime - prevTime);
    int64_t nextDiff = std::abs(nextTime - reqTime);
    
    return (prevDiff <= nextDiff) ? PREVIOUS_FRAME : NEXT_FRAME;
}

MPEG4SampleHelper::MPEG4SampleHelper()
    : dataReader_(nullptr),
      sampleIndexEntry_(),
      cttsEntry_(),
      seekable_(Seekable::UNSEEKABLE),
      trackType_(INVALID_TYPE),
      mimeType_(""),
      firstMoofSampleIndex_(-1),
      sttsEntry_(),
      stscEntry_(),
      stssEntry_(),
      sampleSizes_(),
      chunkOffsets_(),
      trex_(),
      tfhd_(),
      dtsShift_(0),
      timeOffset_(0)
{
}

MPEG4SampleHelper::~MPEG4SampleHelper()
{
}

void MPEG4SampleHelper::SetEditListParams(const std::vector<EditListEntry>& editListEntries, int32_t movieTimeScale,
    int32_t trackTimeScale)
{
    editListEntries_ = editListEntries;
    movieTimeScale_ = movieTimeScale;
    trackTimeScale_ = trackTimeScale;
}

bool MPEG4SampleHelper::HasEditList() const
{
    return !editListEntries_.empty();
}

bool MPEG4SampleHelper::HasAppliedEditList() const
{
    return editListApplied_;
}

int64_t MPEG4SampleHelper::GetRawTotalSize() const
{
    return rawTotalSize_;
}

int64_t MPEG4SampleHelper::GetRawTotalDuration() const
{
    return rawTotalDuration_;
}

int64_t MPEG4SampleHelper::GetRawSampleCount() const
{
    return rawSampleCount_;
}

bool MPEG4SampleHelper::HasTrailingZeroSttsDuration() const
{
    return !sttsEntry_.empty() && sttsEntry_.back().delta == 0;
}

uint32_t MPEG4SampleHelper::EstimateVideoDelay() const
{
    if (sampleIndexEntry_.empty()) {
        return 0;
    }
    bool hasReorderedPts = std::any_of(sampleIndexEntry_.begin(), sampleIndexEntry_.end(),
        [](const SampleIndexEntry& sample) {
            return sample.pts != sample.dts;
        });
    if (!hasReorderedPts) {
        return 0;
    }

    std::array<int64_t, MAX_REORDER_DELAY + 1> ptsBuffer;
    ptsBuffer.fill(INT64_MIN);
    uint32_t bufferStart = 0;
    uint32_t videoDelay = 0;
    for (const auto& sample : sampleIndexEntry_) {
        uint32_t pos = bufferStart;
        bufferStart = (bufferStart + 1) % ptsBuffer.size();
        ptsBuffer[pos] = sample.pts;

        uint32_t swaps = 0;
        while (pos != bufferStart) {
            int32_t previous = static_cast<int32_t>(pos) - 1;
            if (previous < 0) {
                previous = MAX_REORDER_DELAY;
            }
            if (ptsBuffer[pos] >= ptsBuffer[previous]) {
                break;
            }
            std::swap(ptsBuffer[pos], ptsBuffer[previous]);
            ++swaps;
            pos = static_cast<uint32_t>(previous);
        }
        videoDelay = std::max(videoDelay, swaps);
    }
    return videoDelay;
}

Status MPEG4SampleHelper::FindWithSyncSamplesByPts(
    int64_t inputPts, bool isBaseTrack, SeekMode flag, uint32_t &syncIndex)
{
    MEDIA_LOG_D("In");
    int64_t previousIndex = -1;
    int64_t nextIndex = -1;
    for (size_t i = 0; i < stssEntry_.size(); ++i) {
        const int64_t syncPts = stssEntry_[i].syncPts;
        if (syncPts <= inputPts && (previousIndex < 0 || syncPts > stssEntry_[previousIndex].syncPts)) {
            previousIndex = static_cast<int64_t>(i);
        }
        if (syncPts >= inputPts && (nextIndex < 0 || syncPts < stssEntry_[nextIndex].syncPts)) {
            nextIndex = static_cast<int64_t>(i);
        }
    }

    int64_t selectedIndex = -1;
    if (flag == SeekMode::SEEK_PREVIOUS_SYNC) {
        selectedIndex = previousIndex;
    } else if (flag == SeekMode::SEEK_NEXT_SYNC) {
        selectedIndex = nextIndex;
    } else if (previousIndex >= 0 && nextIndex >= 0) {
        selectedIndex = GetClosestSync(stssEntry_[previousIndex].syncPts, stssEntry_[nextIndex].syncPts, inputPts) ?
            nextIndex : previousIndex;
    } else {
        selectedIndex = (previousIndex >= 0) ? previousIndex : nextIndex;
    }

    if (selectedIndex < 0) {
        if (isBaseTrack && flag == SeekMode::SEEK_PREVIOUS_SYNC && !stssEntry_.empty() &&
            inputPts > stssEntry_.back().syncPts) {
            syncIndex = static_cast<uint32_t>(stssEntry_.size() - 1);
            return Status::OK;
        }
        return Status::ERROR_UNKNOWN;
    }
    syncIndex = static_cast<uint32_t>(selectedIndex);
    return Status::OK;
}

Status MPEG4SampleHelper::CheckSyncSampleByPts(
    int64_t inputPts, bool isBaseTrack, SeekMode flag, int64_t &currentIndex)
{
    MEDIA_LOG_D("In");
    const auto& entry = sampleIndexEntry_;
    if (flag == SeekMode::SEEK_PREVIOUS_SYNC) {
        currentIndex = FindPrevSyncByPts(inputPts, currentIndex - 1);
        if (currentIndex < 0) {
            currentIndex = stssEntry_.empty() ? 0 : stssEntry_[0].index;
            if (inputPts < entry[currentIndex].pts && isBaseTrack) {
                return Status::ERROR_INVALID_PARAMETER;
            }
        }
        return Status::OK;
    }

    if (flag == SeekMode::SEEK_NEXT_SYNC) {
        currentIndex = FindNextSyncByPts(inputPts, currentIndex);
        if (currentIndex == static_cast<int64_t>(entry.size()) && isBaseTrack) {
            return Status::ERROR_INVALID_PARAMETER;
        }
        return Status::OK;
    }

    SelectClosestSyncByPts(inputPts, currentIndex);
    return Status::OK;
}

int64_t MPEG4SampleHelper::FindPrevSyncByPts(int64_t inputPts, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    while (currentIndex >= 0 && (((entry[currentIndex].flag & Sample::SYNC_FRAME) == 0) ||
        entry[currentIndex].pts > inputPts)) {
        --currentIndex;
    }
    return currentIndex;
}

int64_t MPEG4SampleHelper::FindNextSyncByPts(int64_t inputPts, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    while (currentIndex < static_cast<int64_t>(entry.size()) &&
        (((entry[currentIndex].flag & Sample::SYNC_FRAME) == 0) || entry[currentIndex].pts < inputPts)) {
        ++currentIndex;
    }
    return currentIndex;
}

void MPEG4SampleHelper::SelectClosestSyncByPts(int64_t inputPts, int64_t& currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    int64_t prevSync = FindPrevSyncByPts(inputPts, currentIndex);
    int64_t nextSync = FindNextSyncByPts(inputPts, currentIndex);
    if (prevSync >= 0 && nextSync < static_cast<int64_t>(entry.size())) {
        currentIndex = GetClosestSync(entry[prevSync].pts, entry[nextSync].pts, inputPts) ? nextSync : prevSync;
    } else if (prevSync < 0) {
        currentIndex = nextSync;
    } else {
        currentIndex = prevSync;
    }
}

int64_t MPEG4SampleHelper::FindIndexWithoutSyncSamples(int64_t inputPTS) const
{
    auto it = std::lower_bound(sampleIndexEntry_.begin(), sampleIndexEntry_.end(), inputPTS,
        [](const SampleIndexEntry& entry, int64_t pts) { return entry.pts < pts; });
    return std::distance(sampleIndexEntry_.begin(), it);
}

bool MPEG4SampleHelper::AssignSampleIndex(int64_t currentIndex, uint32_t& sampleIndex) const
{
    if (currentIndex < 0 ||
        static_cast<uint64_t>(currentIndex) > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())) {
        return false;
    }
    sampleIndex = static_cast<uint32_t>(currentIndex);
    return true;
}

bool MPEG4SampleHelper::AssignSampleIndexFromSize(size_t currentIndex, uint32_t& sampleIndex) const
{
    if (currentIndex > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
        return false;
    }
    sampleIndex = static_cast<uint32_t>(currentIndex);
    return true;
}

bool MPEG4SampleHelper::ResolveDirectSyncSampleByPts(int64_t inputPts, int64_t currentIndex,
    uint32_t& sampleIndex) const
{
    const auto& entry = sampleIndexEntry_;
    if ((entry[currentIndex].flag & Sample::SYNC_FRAME) == 0 || inputPts != entry[currentIndex].pts) {
        return false;
    }
    return AssignSampleIndex(currentIndex, sampleIndex);
}

bool MPEG4SampleHelper::ResolveNonBaseTrackEosByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag,
    uint32_t& sampleIndex) const
{
    bool beyondLastSync =
        !stssEntry_.empty() && inputPts > stssEntry_.back().syncPts && flag == SeekMode::SEEK_NEXT_SYNC;
    if (!isBaseTrack && trackType_ == AUDIO_TYPE && flag == SeekMode::SEEK_NEXT_SYNC && inputPts >= lastPTS_) {
        return AssignSampleIndexFromSize(sampleIndexEntry_.size(), sampleIndex);
    }
    if (isBaseTrack || (inputPts <= lastPTS_ && !beyondLastSync)) {
        return false;
    }
    return AssignSampleIndexFromSize(sampleIndexEntry_.size(), sampleIndex);
}

bool MPEG4SampleHelper::ResolveTextWebVttSample(int64_t inputPTS, int64_t& currentIndex,
    uint32_t& sampleIndex) const
{
    if (mimeType_ != MimeType::TEXT_WEBVTT) {
        return false;
    }
    const auto& entry = sampleIndexEntry_;
    if (!AssignSampleIndex(currentIndex, sampleIndex)) {
        return false;
    }
    if (currentIndex > 0 && inputPTS < entry[currentIndex].pts) {
        --currentIndex;
        return AssignSampleIndex(currentIndex, sampleIndex);
    }
    if (inputPTS > totalDuration_) {
        ++currentIndex;
        return AssignSampleIndex(currentIndex, sampleIndex);
    }
    return true;
}

bool MPEG4SampleHelper::IsAudioSampleIndexInRange(int64_t currentIndex) const
{
    return trackType_ == AUDIO_TYPE && currentIndex > 0 &&
        static_cast<size_t>(currentIndex) < sampleIndexEntry_.size();
}

bool MPEG4SampleHelper::IsInputCoveredByPreviousSample(int64_t inputPts, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    return IsAudioSampleIndexInRange(currentIndex) &&
        inputPts < entry[currentIndex - 1].pts + entry[currentIndex - 1].duration;
}

bool MPEG4SampleHelper::ShouldStepBackForBaseAudioCoverage(int64_t inputPts, SeekMode flag, bool fromSyncSamples,
    int64_t currentIndex) const
{
    if (!fromSyncSamples || !IsAudioSampleIndexInRange(currentIndex)) {
        return false;
    }
    const auto& entry = sampleIndexEntry_;
    bool allowCoverageBackStep = flag == SeekMode::SEEK_CLOSEST_SYNC ||
        (flag == SeekMode::SEEK_NEXT_SYNC && static_cast<size_t>(currentIndex) == entry.size() - 1);
    return entry[currentIndex].pts > inputPts && IsInputCoveredByPreviousSample(inputPts, currentIndex) &&
        allowCoverageBackStep;
}

bool MPEG4SampleHelper::ShouldStepBackForFirstNonBaseAudioSample(int64_t inputPts, bool isBaseTrack,
    bool fromSyncSamples, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    return fromSyncSamples && !isBaseTrack && currentIndex == 1 && !entry.empty() &&
        inputPts <= entry[0].pts + entry[0].duration;
}

bool MPEG4SampleHelper::ShouldStepBackForSyncPrerollSample(int64_t inputPts, bool isBaseTrack,
    bool fromSyncSamples, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    return fromSyncSamples && !isBaseTrack && IsAudioSampleIndexInRange(currentIndex) &&
        entry[currentIndex].pts >= inputPts && !entry[currentIndex - 1].skipSamplesInfo.empty();
}

bool MPEG4SampleHelper::ShouldStepBackForShiftedAudioCoverage(int64_t inputPts, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    return trackTimeScale_ > 0 && !entry.empty() && entry.front().pts >= trackTimeScale_ &&
        IsAudioSampleIndexInRange(currentIndex) && inputPts < entry[currentIndex].pts &&
        IsInputCoveredByPreviousSample(inputPts, currentIndex);
}

bool MPEG4SampleHelper::ShouldStepBackForNoSyncPrerollSample(int64_t inputPts, bool isBaseTrack, SeekMode flag,
    bool fromSyncSamples, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    return !fromSyncSamples && !isBaseTrack && flag == SeekMode::SEEK_PREVIOUS_SYNC &&
        IsAudioSampleIndexInRange(currentIndex) && entry[currentIndex].pts >= inputPts &&
        entry[currentIndex - 1].pts < inputPts && !entry[currentIndex - 1].skipSamplesInfo.empty();
}

bool MPEG4SampleHelper::ShouldStepBackForExactPtsPrerollSample(int64_t inputPts, bool isBaseTrack, SeekMode flag,
    bool fromSyncSamples, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    return fromSyncSamples && !isBaseTrack && flag == SeekMode::SEEK_PREVIOUS_SYNC &&
        IsAudioSampleIndexInRange(currentIndex) && entry[currentIndex].pts == inputPts &&
        entry[currentIndex - 1].pts < inputPts && !entry[currentIndex - 1].skipSamplesInfo.empty();
}

void MPEG4SampleHelper::AdjustAudioSeekSampleIndexByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag,
    bool fromSyncSamples, int64_t& currentIndex) const
{
    if (trackType_ != AUDIO_TYPE) {
        return;
    }
    // Only one back step is allowed; later checks must not re-evaluate a modified index.
    // Step back when the selected audio sample starts after inputPts but the previous sample still covers it.
    if (isBaseTrack && ShouldStepBackForBaseAudioCoverage(inputPts, flag, fromSyncSamples, currentIndex)) {
        --currentIndex;
        return;
    }
    // Keep the first audio sample if it covers the sync point for a non-base track.
    if (ShouldStepBackForFirstNonBaseAudioSample(inputPts, isBaseTrack, fromSyncSamples, currentIndex)) {
        --currentIndex;
        return;
    }
    // Keep AAC priming/skip-sample metadata by seeking to the preroll sample before the target point.
    if (ShouldStepBackForSyncPrerollSample(inputPts, isBaseTrack, fromSyncSamples, currentIndex)) {
        --currentIndex;
        return;
    }
    // Empty edits can shift audio PTS forward; use sample coverage instead of skipping the covered sample.
    if (ShouldStepBackForShiftedAudioCoverage(inputPts, currentIndex)) {
        --currentIndex;
        return;
    }
    // Apply the same preroll rule on tracks without sync-sample entries.
    if (ShouldStepBackForNoSyncPrerollSample(inputPts, isBaseTrack, flag, fromSyncSamples, currentIndex)) {
        --currentIndex;
        return;
    }
    // Exact PTS matches may still need the previous AAC preroll sample for correct decode trimming.
    if (ShouldStepBackForExactPtsPrerollSample(inputPts, isBaseTrack, flag, fromSyncSamples, currentIndex)) {
        --currentIndex;
    }
}

Status MPEG4SampleHelper::ResolveEarlySeekSampleByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag,
    uint32_t& sampleIndex, bool& resolved) const
{
    resolved = true;
    const auto& entry = sampleIndexEntry_;
    if (entry.empty()) {
        return Status::ERROR_INVALID_DATA;
    }
    if (inputPts < 0) {
        sampleIndex = stssEntry_.empty() ? 0 : stssEntry_[0].index;
        return Status::OK;
    }
    if (trackType_ == AUDIO_TYPE && isBaseTrack && inputPts == 0 && entry.front().pts < 0 &&
        entry.front().skipSamplesInfo.empty()) {
        sampleIndex = 0;
        return Status::OK;
    }
    if (trackType_ == AUDIO_TYPE && isBaseTrack && inputPts > lastPTS_) {
        return AssignSampleIndex(entry.size() - 1, sampleIndex) ? Status::OK : Status::ERROR_INVALID_DATA;
    }
    if (!isBaseTrack && trackType_ == AUDIO_TYPE &&
        (flag == SeekMode::SEEK_NEXT_SYNC || flag == SeekMode::SEEK_CLOSEST_SYNC) && inputPts >= lastPTS_) {
        return AssignSampleIndexFromSize(entry.size(), sampleIndex) ? Status::OK : Status::ERROR_INVALID_DATA;
    }
    resolved = false;
    return Status::OK;
}

Status MPEG4SampleHelper::FindIndexWithSyncSamplesByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag,
    int64_t& currentIndex)
{
    uint32_t syncIndex = 0;
    Status ret = FindWithSyncSamplesByPts(inputPts, isBaseTrack, flag, syncIndex);
    currentIndex = syncIndex < stssEntry_.size() ? stssEntry_[syncIndex].index : stssEntry_.back().index;
    if (ret == Status::OK) {
        AdjustAudioSeekSampleIndexByPts(inputPts, isBaseTrack, flag, true, currentIndex);
    }
    return ret;
}

Status MPEG4SampleHelper::FindSyncSampleByPts(
    int64_t inputPts, bool isBaseTrack, SeekMode flag, uint32_t &sampleIndex)
{
    MEDIA_LOG_D("In");
    const auto& entry = sampleIndexEntry_;
    if (entry.empty()) {
        return Status::ERROR_INVALID_DATA;
    }
    bool resolved = false;
    Status ret = ResolveEarlySeekSampleByPts(inputPts, isBaseTrack, flag, sampleIndex, resolved);
    if (resolved) {
        return ret;
    }

    int64_t currentIndex = 0;
    if (!stssEntry_.empty()) {
        ret = FindIndexWithSyncSamplesByPts(inputPts, isBaseTrack, flag, currentIndex);
        if (ret == Status::OK) {
            FALSE_RETURN_V_MSG_E(AssignSampleIndex(currentIndex, sampleIndex), Status::ERROR_INVALID_DATA,
                "Invalid sample index");
            return Status::OK;
        }
    } else {
        currentIndex = FindIndexWithoutSyncSamples(inputPts);
        AdjustAudioSeekSampleIndexByPts(inputPts, isBaseTrack, flag, false, currentIndex);
    }

    if (currentIndex >= static_cast<int64_t>(entry.size())) {
        currentIndex = static_cast<int64_t>(entry.size()) - 1;
    }
    if (ResolveDirectSyncSampleByPts(inputPts, currentIndex, sampleIndex) ||
        ResolveNonBaseTrackEosByPts(inputPts, isBaseTrack, flag, sampleIndex) ||
        ResolveTextWebVttSample(inputPts, currentIndex, sampleIndex)) {
        return Status::OK;
    }

    ret = CheckSyncSampleByPts(inputPts, isBaseTrack, flag, currentIndex);
    if (ret == Status::OK) {
        FALSE_RETURN_V_MSG_E(AssignSampleIndex(currentIndex, sampleIndex), Status::ERROR_INVALID_DATA,
            "Invalid sample index");
        return Status::OK;
    } else {
        return ret;
    }
}

Status MPEG4SampleHelper::SetTimeToSampleParams(int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(sttsEntry_.empty(), Status::ERROR_INVALID_OPERATION, "sttsEntry exist");
    FALSE_RETURN_V_MSG_E(dataOffset >= 0 && dataSize >= MIN_DATA_SIZE, Status::ERROR_INVALID_PARAMETER, "Invalid data");

    int64_t currentOffset = dataOffset;
    uint8_t header[8];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    uint8_t version = header[0];
    uint32_t flags = GetU32Value(header) & 0x00FFFFFF;
    if ((version != 0 && version != 1) || flags != 0) {
        return Status::ERROR_INVALID_DATA;
    }

    uint32_t numSttsEntries = GetU32Value(&header[4]);
    currentOffset += sizeof(uint64_t);

    if ((dataSize - sizeof(header)) / sizeof(SttsEntry) < numSttsEntries) {
        return Status::ERROR_UNKNOWN;
    }

    uint32_t size = numSttsEntries * sizeof(SttsEntry);
    auto data = std::make_unique<uint8_t[]>(size);
    ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    if (ret != Status::OK) {
        return ret;
    }
    currentOffset += static_cast<int64_t>(size);
    sttsEntry_.reserve(numSttsEntries);

    for (uint32_t i = 0; i < numSttsEntries; ++i) {
        uint32_t sampleCount = GetU32Value(data.get() + i * sizeof(SttsEntry));
        FALSE_RETURN_V_MSG_E(sampleCount > 0, Status::ERROR_INVALID_DATA, "Invalid stts sample count");
        sttsEntry_.emplace_back(SttsEntry{
            .count = sampleCount,
            .delta = GetU32Value(data.get() + 4 + i * sizeof(SttsEntry))
        });
    }

    return Status::OK;
}

Status MPEG4SampleHelper::SetCompositionTimeToSampleParams(int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(cttsEntry_.empty(), Status::ERROR_INVALID_OPERATION, "cttsEntry exist");

    int64_t currentOffset = dataOffset;
    if (currentOffset < MIN_DATA_OFFSET || dataSize < MIN_DATA_SIZE) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    uint8_t header[8];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    uint8_t version = header[0];
    uint32_t flags = GetU32Value(header) & 0x00FFFFFF;
    if ((version != 0 && version != 1) || flags != 0) {
        return Status::ERROR_INVALID_DATA;
    }

    uint32_t numCttsEntries = GetU32Value(&header[4]);
    currentOffset += sizeof(header);

    if ((dataSize - sizeof(header)) / sizeof(CttsEntry) < numCttsEntries) {
        return Status::ERROR_UNKNOWN;
    }

    uint32_t size = numCttsEntries * sizeof(CttsEntry);
    auto data = std::make_unique<uint8_t[]>(size);
    ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");
    currentOffset += static_cast<int64_t>(size);
    cttsEntry_.reserve(numCttsEntries);

    for (uint32_t i = 0; i < numCttsEntries; ++i) {
        uint32_t rawDelta = GetU32Value(data.get() + 4 + i * sizeof(CttsEntry));
        cttsEntry_.emplace_back(CttsEntry {
            .count = GetU32Value(data.get() + i * sizeof(CttsEntry)),
            .delta = (version == 1) ? static_cast<int32_t>(rawDelta) : static_cast<int32_t>(rawDelta)
        });
    }

    return Status::OK;
}

Status MPEG4SampleHelper::SetSyncSampleParams(int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(stssEntry_.empty(), Status::ERROR_INVALID_OPERATION, "stssEntry exist");

    int64_t currentOffset = dataOffset;
    if (currentOffset < MIN_DATA_OFFSET || dataSize < MIN_DATA_SIZE) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    uint8_t header[8];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    if (GetU32Value(header)) {
        return Status::ERROR_INVALID_DATA;
    }

    uint32_t numSyncs = GetU32Value(&header[4]);
    currentOffset += sizeof(header);

    if ((dataSize - sizeof(header)) / sizeof(uint32_t) < numSyncs) {
        return Status::ERROR_UNKNOWN;
    }
    
    uint32_t size = numSyncs * sizeof(uint32_t);
    auto data = std::make_unique<uint8_t[]>(size);
    ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    if (ret != Status::OK) {
        return ret;
    }
    currentOffset += static_cast<int64_t>(size);
    stssEntry_.reserve(numSyncs);

    for (uint32_t i = 0; i < numSyncs; ++i) {
        uint32_t sampleIndex = GetU32Value(data.get() + i * sizeof(uint32_t));
        FALSE_RETURN_V_MSG_E(sampleIndex > 0, Status::ERROR_INVALID_DATA, "Invalid sample index");
        stssEntry_.emplace_back(StssEntry{
            .index = sampleIndex - 1,
            .syncPts = 0,
            .syncDts = 0
        });
    }

    return Status::OK;
}

Status MPEG4SampleHelper::SetFieldSize(uint32_t atomType, int64_t &currentOffset, uint32_t &fieldSize)
{
    if (atomType == FourccType("stsz")) {
        fieldSize = DEFAULT_FIELD_SIZE;
        uint8_t sizeBuffer[4];
        Status ret = dataReader_->ReadUintData(currentOffset, sizeBuffer, sizeof(sizeBuffer));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sizeBuffer failed");
        uint32_t defaultSampleSize = GetU32Value(sizeBuffer);
        currentOffset += sizeof(uint32_t);
        if (defaultSampleSize) {
            sampleSizes_.emplace_back(defaultSampleSize);
            hasDefaultStszSize_ = true;
        }
    } else {
        uint8_t sizeBuffer[4];
        Status ret = dataReader_->ReadUintData(currentOffset, sizeBuffer, sizeof(sizeBuffer));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sizeBuffer failed");
        fieldSize = GetU32Value(sizeBuffer);
        currentOffset += sizeof(uint32_t);

        if (fieldSize != HALF_BYTE_BITS && fieldSize != BYTE_BITS && fieldSize != DUL_BYTE_BITS) {
            return Status::ERROR_INVALID_DATA;
        }
    }
    return Status::OK;
}

Status MPEG4SampleHelper::ProcessSampleSizeData(uint32_t atomType, int64_t currentOffset, uint32_t size,
    uint32_t fieldSize, uint32_t numSampleSizes)
{
    auto data = std::make_unique<uint8_t[]>(size);
    Status ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sampleSize failed");
    currentOffset += size;
    sampleSizes_.reserve(numSampleSizes);

    if (atomType == FourccType("stsz")) {
        for (uint32_t i = 0; i < numSampleSizes; ++i) {
            uint32_t sampleSize = GetU32Value(data.get() + i * sizeof(uint32_t));
            sampleSizes_.emplace_back(static_cast<int32_t>(sampleSize));
        }
    } else {
        uint32_t i = 0;
        while (i < numSampleSizes) {
            uint32_t sampleSize = 0;
            if (fieldSize == HALF_BYTE_BITS) {
                ProcessHalfByteSampleSizes(data.get(), i, numSampleSizes);
            } else if (fieldSize == BYTE_BITS) {
                sampleSize = data[i];
                sampleSizes_.emplace_back(sampleSize);
            } else {
                sampleSize = GetU16Value(data.get() + i * sizeof(uint16_t));
                sampleSizes_.emplace_back(sampleSize);
            }
            ++i;
        }
    }
    return ret;
}

void MPEG4SampleHelper::ProcessHalfByteSampleSizes(const uint8_t* data, uint32_t& entryIndex, uint32_t numSampleSizes)
{
    uint32_t index = entryIndex / 2;
    uint8_t sampleSize = data[index];
    sampleSizes_.emplace_back(sampleSize >> 0x4);
    if (entryIndex < numSampleSizes) {
        sampleSizes_.emplace_back(sampleSize - (sampleSize >> 0x4));
        ++entryIndex;
    }
}

Status MPEG4SampleHelper::SetSampleSizeParams(uint32_t atomType, int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(sampleSizes_.empty(), Status::ERROR_INVALID_OPERATION, "sampleSizes exist");
    if (atomType != FourccType("stsz") && atomType != FourccType("stz2")) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    int64_t currentOffset = dataOffset;
    if (currentOffset < MIN_DATA_OFFSET || dataSize < MIN_DATA_SIZE) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    uint8_t header[4];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    if (GetU32Value(header)) {
        return Status::ERROR_INVALID_DATA;
    }
    currentOffset += sizeof(uint32_t);

    uint32_t fieldSize;
    ret = SetFieldSize(atomType, currentOffset, fieldSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set fieldSize failed");
    if (hasDefaultStszSize_) {
        return Status::OK;
    }

    uint8_t entryBuffer[4];
    ret = dataReader_->ReadUintData(currentOffset, entryBuffer, sizeof(entryBuffer));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read entryBuffer failed");
    uint32_t numSampleSizes = GetU32Value(entryBuffer);
    currentOffset += sizeof(uint32_t);
    if (!numSampleSizes) {
        return Status::OK;
    }

    if ((atomType == FourccType("stsz") && (dataSize - STSZ_HEAD_LENGTH) / sizeof(uint32_t) < numSampleSizes) ||
        (atomType == FourccType("stz2") &&
        (dataSize - STSZ_HEAD_LENGTH) < (numSampleSizes * fieldSize + sizeof(uint32_t)) / BYTE_BITS)) {
        return Status::ERROR_UNKNOWN;
    }

    uint32_t size = atomType == FourccType("stsz") ? numSampleSizes * sizeof(uint32_t) :
        (numSampleSizes * fieldSize + sizeof(uint32_t)) / 8;
    ret = ProcessSampleSizeData(atomType, currentOffset, size, fieldSize, numSampleSizes);
    return ret;
}

Status MPEG4SampleHelper::SetChunkOffsetParams(uint32_t atomType, int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(chunkOffsets_.empty(), Status::ERROR_INVALID_OPERATION, "chunkOffsets exist");
    if (atomType != FourccType("stco") && atomType != FourccType("co64")) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    int64_t currentOffset = dataOffset;
    if (currentOffset < MIN_DATA_OFFSET || dataSize < MIN_DATA_SIZE) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    uint8_t header[4];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    if (GetU32Value(header)) {
        return Status::ERROR_INVALID_DATA;
    }
    currentOffset += sizeof(uint32_t);

    uint8_t entryBuffer[4];
    ret = dataReader_->ReadUintData(currentOffset, entryBuffer, sizeof(entryBuffer));
    if (ret != Status::OK) {
        return ret;
    }
    uint32_t numChunkOffsets = GetU32Value(entryBuffer);
    currentOffset += sizeof(uint32_t);
    if ((atomType == FourccType("co64") && (dataSize - sizeof(uint64_t)) / sizeof(uint64_t) < numChunkOffsets) ||
        (atomType == FourccType("stco") && (dataSize - sizeof(uint64_t)) / sizeof(uint32_t) < numChunkOffsets)) {
        return Status::ERROR_UNKNOWN;
    }

    uint32_t size = atomType == FourccType("stco") ? numChunkOffsets * sizeof(uint32_t) :
        numChunkOffsets * sizeof(uint64_t);
    auto data = std::make_unique<uint8_t[]>(size);
    ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    if (ret != Status::OK) {
        return ret;
    }
    currentOffset += static_cast<int64_t>(size);
    chunkOffsets_.reserve(numChunkOffsets);

    if (atomType == FourccType("stco")) {
        for (uint32_t i = 0; i < numChunkOffsets; ++i) {
            uint32_t offset = GetU32Value(data.get() + i * sizeof(uint32_t));
            chunkOffsets_.emplace_back(static_cast<int64_t>(offset));
        }
    } else {
        for (uint32_t i = 0; i < numChunkOffsets; ++i) {
            uint64_t offset = GetU64Value(data.get() + i * sizeof(uint64_t));
            chunkOffsets_.emplace_back(static_cast<int64_t>(offset));
        }
    }

    return Status::OK;
}

Status MPEG4SampleHelper::SetSampleToChunkParams(int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(stscEntry_.empty(), Status::ERROR_INVALID_OPERATION, "stscEntry exist");
    
    int64_t currentOffset = dataOffset;
    if (currentOffset < MIN_DATA_OFFSET || dataSize < MIN_DATA_SIZE) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    uint8_t header[8];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    if (GetU32Value(header)) {
        return Status::ERROR_INVALID_DATA;
    }

    uint32_t numStscEntries = GetU32Value(&header[4]);
    currentOffset += sizeof(header);

    if ((dataSize - sizeof(header)) / sizeof(StscEntry) < numStscEntries) {
        return Status::ERROR_UNKNOWN;
    }

    uint32_t size = numStscEntries * sizeof(StscEntry);
    auto data = std::make_unique<uint8_t[]>(size);
    ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    if (ret != Status::OK) {
        return ret;
    }
    currentOffset += static_cast<int64_t>(size);
    stscEntry_.reserve(numStscEntries);

    for (uint32_t i = 0; i < numStscEntries; ++i) {
        uint32_t tempFirst = GetU32Value(data.get() + i * sizeof(StscEntry));
        if (tempFirst <= 0) {
            return Status::ERROR_INVALID_DATA;
        }

        stscEntry_.emplace_back(StscEntry{
            .first = tempFirst - 1,
            .count = GetU32Value(data.get() + sizeof(uint32_t) + i * sizeof(StscEntry)),
            .id = GetU32Value(data.get() + sizeof(uint64_t) + i * sizeof(StscEntry))
        });
    }

    return Status::OK;
}

Status MPEG4SampleHelper::SetSampleDependencyTypeParams(int64_t ctxOffset, int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    sdtpEntry_.sdtpEntryCount = 0;
    sdtpEntry_.sdtpData.clear();

    const uint32_t versionAndFlagsOffset = 4;  // version + flags = 4Bytes
    FALSE_RETURN_V_MSG_E(dataSize > versionAndFlagsOffset, Status::ERROR_INVALID_DATA, "dataSize is invalid");

    // After removing type, size..., the starting offset of the actual data in the atom
    int64_t currentOffset = dataOffset;
    FALSE_RETURN_V_MSG_E(currentOffset >= MIN_DATA_OFFSET, Status::ERROR_INVALID_PARAMETER, "currentOffset is invalid");
    for (uint32_t i = 0; i < dataSize && currentOffset < ctxOffset; ++i) {  // Each entry size is 1 Byte
        if (i >= 0 && i < versionAndFlagsOffset) {  // skip version and flags
            currentOffset++;
            continue;
        }
        uint8_t sdtpEntry;
        Status ret = dataReader_->ReadUintData(currentOffset, &sdtpEntry, sizeof(sdtpEntry));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
        sdtpEntry_.sdtpData.emplace_back(sdtpEntry);
        currentOffset++;
        sdtpEntry_.sdtpEntryCount++;
    }
    return Status::OK;
}

Status MPEG4SampleHelper::SetTrackExtendsParams(int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(trex_.trackId == 0, Status::ERROR_INVALID_OPERATION, "trex exist");

    int64_t currentOffset = dataOffset;
    const size_t totalTrexSize = 24; // trex size is 24 bytes
    if (currentOffset < MIN_DATA_OFFSET || dataSize < totalTrexSize) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    uint8_t data[totalTrexSize];
    auto ret = dataReader_->ReadUintData(currentOffset, data, sizeof(data));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    currentOffset += sizeof(data);

    const uint32_t trackIdOffset = 4;          // trackId字段偏移
    const uint32_t sampleDescIndexOffset = 8;  // defaultSampleDescriptionIndex字段偏移
    const uint32_t sampleDurationOffset = 12;  // defaultSampleDuration字段偏移
    const uint32_t sampleSizeOffset = 16;      // defaultSampleSize字段偏移
    const uint32_t sampleFlagsOffset = 20;     // defaultSampleFlags字段偏移

    trex_.trackId = GetU32Value(data + trackIdOffset);
    trex_.defaultSampleDescriptionIndex = GetU32Value(data + sampleDescIndexOffset);
    trex_.defaultSampleDuration = GetU32Value(data + sampleDurationOffset);
    trex_.defaultSampleSize = GetU32Value(data + sampleSizeOffset);
    trex_.defaultSampleFlags = GetU32Value(data + sampleFlagsOffset);
    return Status::OK;
}

bool MPEG4SampleHelper::HasTfhdRemainingData(const TfhdParseContext& ctx, size_t fieldSize) const
{
    return ctx.dataOffset <= ctx.dataSize && fieldSize <= ctx.dataSize - ctx.dataOffset;
}

Status MPEG4SampleHelper::ParseTfhdBaseDataOffset(TfhdParseContext& ctx, int64_t moofOffset)
{
    if ((ctx.flag & TrackFragmentHeaderInfo::TFHD_BASE_DATA_OFFSET) && seekable_ == Seekable::SEEKABLE) {
        FALSE_RETURN_V_MSG_E(HasTfhdRemainingData(ctx, sizeof(uint64_t)),
            Status::ERROR_INVALID_DATA, "Invalid tfhd base data offset");
        uint64_t baseDataOffset = GetU64Value(ctx.data + ctx.dataOffset);
        FALSE_RETURN_V_MSG_E(baseDataOffset <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()),
            Status::ERROR_INVALID_DATA, "Invalid tfhd base data offset");
        tfhd_.baseDataOffset = static_cast<int64_t>(baseDataOffset);
        ctx.dataOffset += sizeof(uint64_t);
    } else {
        FALSE_RETURN_V_MSG_E(moofOffset >= 0, Status::ERROR_INVALID_DATA, "Invalid moof offset");
        tfhd_.baseDataOffset = moofOffset;
    }
    return Status::OK;
}

Status MPEG4SampleHelper::ParseTfhdOptionalUint32(TfhdParseContext& ctx, const TfhdOptionalUint32Field& field)
{
    FALSE_RETURN_V_MSG_E(field.output != nullptr, Status::ERROR_INVALID_PARAMETER, "Invalid tfhd output");
    if (ctx.flag & field.flag) {
        FALSE_RETURN_V_MSG_E(HasTfhdRemainingData(ctx, sizeof(uint32_t)),
            Status::ERROR_INVALID_DATA, "Invalid tfhd optional field");
        *field.output = GetU32Value(ctx.data + ctx.dataOffset);
        ctx.dataOffset += sizeof(uint32_t);
    } else {
        *field.output = field.defaultValue;
    }
    return Status::OK;
}

Status MPEG4SampleHelper::SetTrackFragmentHeaderParams(int64_t dataOffset, int64_t dataSize, uint32_t flag,
    int64_t moofOffset)
{
    FALSE_RETURN_V_MSG_E(dataSize >= 0 &&
        static_cast<uint64_t>(dataSize) <= static_cast<uint64_t>(std::numeric_limits<size_t>::max()),
        Status::ERROR_INVALID_PARAMETER, "Invalid tfhd data size");
    size_t size = static_cast<size_t>(dataSize);
    auto data = std::make_unique<uint8_t[]>(size);
    Status ret = dataReader_->ReadUintData(dataOffset, data.get(), size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");

    TfhdParseContext ctx = {flag, data.get(), size, 0};
    ret = ParseTfhdBaseDataOffset(ctx, moofOffset);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse tfhd base data offset failed");
    ret = ParseTfhdOptionalUint32(ctx,
        {TrackFragmentHeaderInfo::TFHD_STSD_ID, trex_.defaultSampleDescriptionIndex, &tfhd_.sampleDescriptionIndex});
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse tfhd sample description index failed");
    ret = ParseTfhdOptionalUint32(ctx,
        {TrackFragmentHeaderInfo::TFHD_DEFAULT_DURATION, trex_.defaultSampleDuration, &tfhd_.defaultSampleDuration});
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse tfhd default sample duration failed");
    ret = ParseTfhdOptionalUint32(ctx,
        {TrackFragmentHeaderInfo::TFHD_DEFAULT_SIZE, trex_.defaultSampleSize, &tfhd_.defaultSampleSize});
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse tfhd default sample size failed");
    ret = ParseTfhdOptionalUint32(ctx,
        {TrackFragmentHeaderInfo::TFHD_DEFAULT_FLAGS, trex_.defaultSampleFlags, &tfhd_.flags});
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse tfhd default sample flags failed");
    hasTfhd_ = true;
    return Status::OK;
}

Status MPEG4SampleHelper::SetTrackFragmentDecodeTimeParams(int64_t dataOffset, int64_t dataSize)
{
    MEDIA_LOG_D("In");
    int64_t currentOffset = dataOffset;
    if (currentOffset < MIN_DATA_OFFSET || dataSize < MIN_DATA_SIZE_INT) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    
    uint8_t header[4];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    uint32_t version = header[0];
    currentOffset += sizeof(uint32_t);

    if ((version && dataSize - sizeof(header) < sizeof(uint64_t)) ||
        (!version && dataSize - sizeof(header) < sizeof(uint32_t))) {
        return Status::ERROR_INVALID_DATA;
    }

    if (version) {
        uint8_t data[8];
        ret = dataReader_->ReadUintData(currentOffset, data, sizeof(data));
        tfdtDts_ = static_cast<int64_t>(GetU64Value(data));
    } else {
        uint8_t data[4];
        ret = dataReader_->ReadUintData(currentOffset, data, sizeof(data));
        tfdtDts_ = GetU32Value(data);
    }
    return ret;
}

Status MPEG4SampleHelper::InitSampleInfo(SampleInfo& sampleInfo, uint32_t& bufferOffset, uint32_t trunFlag,
    uint8_t* data, uint32_t& firstSampleFlag)
{
    if (trunFlag & TRUN_DATA_OFFSET) {
        sampleInfo.dataOffset = GetU32Value(data + bufferOffset);
        bufferOffset += sizeof(uint32_t);
    }
    if (trunFlag & TRUN_FIRST_SAMPLE_FLAGS) {
        firstSampleFlag = GetU32Value(data + bufferOffset);
        bufferOffset += sizeof(uint32_t);
    }
    if (trunFlag & TRUN_SAMPLE_DURATION) {
        sampleInfo.byteLength += sizeof(uint32_t);
    } else if (tfhd_.defaultSampleDuration) {
        sampleInfo.duration = tfhd_.defaultSampleDuration;
    } else {
        return Status::ERROR_INVALID_DATA;
    }
    if (trunFlag & TRUN_SAMPLE_SIZE) {
        sampleInfo.byteLength += sizeof(uint32_t);
    } else if (tfhd_.defaultSampleSize) {
        sampleInfo.size = tfhd_.defaultSampleSize;
    } else {
        return Status::ERROR_INVALID_DATA;
    }
    if (trunFlag & TRUN_SAMPLE_FLAGS) {
        sampleInfo.byteLength += sizeof(uint32_t);
    } else if (tfhd_.flags) {
        sampleInfo.flag = tfhd_.flags;
    } else {
        return Status::ERROR_INVALID_DATA;
    }
    if (trunFlag & TRUN_SAMPLE_CTS) {
        sampleInfo.byteLength += sizeof(uint32_t);
    }
    return Status::OK;
}

void MPEG4SampleHelper::UpdateSampleInfo(SampleInfo& sampleInfo, uint32_t& bufferOffset, uint32_t trunFlag,
    uint8_t* data, uint32_t firstSampleFlag)
{
    if (trunFlag & TRUN_SAMPLE_DURATION) {
        sampleInfo.duration = GetU32Value(data + bufferOffset);
        bufferOffset += sizeof(uint32_t);
    }
    if (trunFlag & TRUN_SAMPLE_SIZE) {
        sampleInfo.size = GetU32Value(data + bufferOffset);
        bufferOffset += sizeof(uint32_t);
    }
    if (sampleInfo.index == 0 && (trunFlag & TRUN_FIRST_SAMPLE_FLAGS)) {
        sampleInfo.flag = firstSampleFlag;
    } else if (trunFlag & TRUN_SAMPLE_FLAGS) {
        sampleInfo.flag = GetU32Value(data + bufferOffset);
        bufferOffset += sizeof(uint32_t);
    } else {
        sampleInfo.flag = tfhd_.flags;
    }
    if (trunFlag & TRUN_SAMPLE_CTS) {
        sampleInfo.ctsDelta = GetU32Value(data + bufferOffset);
        bufferOffset += sizeof(uint32_t);
    }
}

void MPEG4SampleHelper::InitMoofParam()
{
    if (firstMoofSampleIndex_ == -1) {
        firstMoofSampleIndex_ = sampleIndexEntry_.size();
        firstMoofSttsIndex_ = sttsEntry_.size();
        numMoovSamples_ = sampleIndexEntry_.size();
    }
}

MPEG4SampleHelper::SampleIndexEntry MPEG4SampleHelper::CreateSampleEntry(
    const SampleInfo& sampleInfo, int64_t offset, int64_t currentDts, bool isOverlap) const
{
    SampleIndexEntry sample;
    sample.pos = offset;
    sample.size = sampleInfo.size;
    sample.pts = currentDts + sampleInfo.ctsDelta;
    sample.dts = currentDts;
    sample.duration = sampleInfo.duration;
    sample.flag = Sample::SampleFlag::NONE;
    if (trackType_ == AUDIO_TYPE) {
        sample.flag |= Sample::SampleFlag::SYNC_FRAME;
    } else {
        sample.flag |= (sampleInfo.flag & (0x00010000 | 0x01000000)) ?
            Sample::SampleFlag::NONE : Sample::SampleFlag::SYNC_FRAME;
    }
    if (isOverlap) {
        sample.flag |= Sample::SampleFlag::DISCARD;
    }
    return sample;
}

bool MPEG4SampleHelper::InsertSampleEntry(SampleIndexEntry&& sample, uint32_t duration)
{
    uint32_t sampleIndex = 0;
    auto it = std::upper_bound(sampleIndexEntry_.begin(), sampleIndexEntry_.end(), sample,
        [](const SampleIndexEntry& a, const SampleIndexEntry& b) { return a.pos < b.pos; });
    if (!AssignSampleIndex(std::distance(sampleIndexEntry_.begin(), it), sampleIndex)) {
        MEDIA_LOG_E("Invalid sample index");
        return false;
    }

    const int64_t sttsIndex = static_cast<int64_t>(sampleIndex) - static_cast<int64_t>(numMoovSamples_) +
        firstMoofSttsIndex_ - 1;
    if (sttsIndex < 0 || static_cast<uint64_t>(sttsIndex) > static_cast<uint64_t>(sttsEntry_.size())) {
        MEDIA_LOG_E("Invalid stts index");
        return false;
    }

    lastPTS_ = std::max(lastPTS_, sample.pts);
    rawTotalSize_ += sample.size;
    sampleIndexEntry_.insert(sampleIndexEntry_.begin() + sampleIndex, std::move(sample));
    const int64_t insertedDts = sampleIndexEntry_[sampleIndex].dts;
    for (size_t i = static_cast<size_t>(sampleIndex) + 1; i < sampleIndexEntry_.size(); ++i) {
        if (insertedDts < sampleIndexEntry_[i].dts) {
            break;
        }
        sampleIndexEntry_[i].flag |= Sample::SampleFlag::DISCARD;
    }

    sttsEntry_.insert(sttsEntry_.begin() + static_cast<size_t>(sttsIndex), SttsEntry {
        .count = 1,
        .delta = duration
    });
    return true;
}

void MPEG4SampleHelper::UpdateSampleEntryInfo(
    SampleInfo sampleInfo, int64_t& offset, int64_t& currentDts, bool isOverlap)
{
    SampleIndexEntry sample = CreateSampleEntry(sampleInfo, offset, currentDts, isOverlap);
    ++rawSampleCount_;
    if (sampleIndexEntry_.empty()) {
        lastPTS_ = sample.pts;
        rawTotalSize_ += sample.size;
        sampleIndexEntry_.emplace_back(std::move(sample));
        sttsEntry_.emplace_back(SttsEntry {
            .count = 1,
            .delta = sampleInfo.duration
        });
        return;
    }
    if (!InsertSampleEntry(std::move(sample), sampleInfo.duration)) {
        return;
    }
}

Status MPEG4SampleHelper::ReadTrunRunData(int64_t dataOffset, int64_t dataSize, uint32_t& trunFlag,
    uint32_t& numSamples, std::unique_ptr<uint8_t[]>& data)
{
    constexpr uint32_t trunVersionIndex = 0;
    constexpr uint32_t trunFlagsFirstIndex = 1;
    constexpr uint32_t trunFlagsSecondIndex = 2;
    constexpr uint32_t trunFlagsThirdIndex = 3;
    constexpr uint32_t trunSampleCountOffset = 4;
    constexpr uint32_t trunFlagFirstShift = 0x10;
    constexpr uint32_t trunFlagSecondShift = 0x08;
    uint8_t header[8];
    auto ret = dataReader_->ReadUintData(dataOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    FALSE_RETURN_V_MSG_E(header[trunVersionIndex] == 0, Status::ERROR_INVALID_DATA, "trun invalid version");
    trunFlag = (static_cast<uint32_t>(header[trunFlagsFirstIndex]) << trunFlagFirstShift) |
        (static_cast<uint32_t>(header[trunFlagsSecondIndex]) << trunFlagSecondShift) |
        static_cast<uint32_t>(header[trunFlagsThirdIndex]);
    if ((trunFlag & TRUN_FIRST_SAMPLE_FLAGS) && (trunFlag & TRUN_SAMPLE_FLAGS)) {
        return Status::ERROR_INVALID_DATA;
    }

    numSamples = GetU32Value(&header[trunSampleCountOffset]);
    data = std::make_unique<uint8_t[]>(dataSize - sizeof(header));
    ret = dataReader_->ReadUintData(dataOffset + sizeof(uint64_t), data.get(), dataSize - sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");
    return Status::OK;
}

bool MPEG4SampleHelper::IsValidTrunSampleData(int64_t dataSize, uint32_t bufferOffset, uint32_t byteLength,
    uint32_t numSamples) const
{
    if (byteLength != 0) {
        constexpr int64_t trunHeaderSize = sizeof(uint64_t);
        if (dataSize < trunHeaderSize) {
            return false;
        }
        int64_t dataBytes = dataSize - trunHeaderSize;
        int64_t safeBufferOffset = static_cast<int64_t>(bufferOffset);
        if (dataBytes < safeBufferOffset) {
            return false;
        }
        return (dataBytes - safeBufferOffset) / static_cast<int64_t>(byteLength) >= static_cast<int64_t>(numSamples);
    }
    return static_cast<int64_t>(numSamples) <= MAX_TRUN_SAMPLES;
}

Status MPEG4SampleHelper::SetTrackFragmentRunParams(int64_t dataOffset, int64_t dataSize, int64_t sidxDts,
    int64_t &moofDts, int64_t &duration)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(hasTfhd_, Status::ERROR_INVALID_DATA, "tfhd does not exist");
    FALSE_RETURN_V_NOLOG(dataOffset >= MIN_DATA_OFFSET && dataSize >= MIN_DATA_SIZE_INT,
        Status::ERROR_INVALID_PARAMETER);

    uint32_t trunFlag = 0;
    uint32_t numSamples = 0;
    std::unique_ptr<uint8_t[]> data;
    Status ret = ReadTrunRunData(dataOffset, dataSize, trunFlag, numSamples, data);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read trun run data failed");

    SampleInfo sampleInfo;
    uint32_t bufferOffset = 0;
    uint32_t firstSampleFlag = 0;
    ret = InitSampleInfo(sampleInfo, bufferOffset, trunFlag, data.get(), firstSampleFlag);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);

    if (!IsValidTrunSampleData(dataSize, bufferOffset, sampleInfo.byteLength, numSamples)) {
        return Status::ERROR_INVALID_DATA;
    }

    FALSE_RETURN_V_MSG_E(tfhd_.baseDataOffset <= std::numeric_limits<int64_t>::max() -
        static_cast<int64_t>(sampleInfo.dataOffset), Status::ERROR_INVALID_DATA, "Invalid trun data offset");
    int64_t offset = tfhd_.baseDataOffset + static_cast<int64_t>(sampleInfo.dataOffset);
    int64_t currentDts = (tfdtDts_ != -1) ? tfdtDts_ : (sidxDts != -1) ? sidxDts : baseFragmentDts_;
    moofDts = currentDts;
    InitMoofParam();
    int64_t prevDts = (sampleIndexEntry_.size() > 0) ? sampleIndexEntry_.back().dts : INT64_MIN;
    for (uint32_t i = 0; i < numSamples; ++i) {
        sampleInfo.index = i;
        UpdateSampleInfo(sampleInfo, bufferOffset, trunFlag, data.get(), firstSampleFlag);
        bool isOverlap = (prevDts != INT64_MIN && prevDts >= currentDts);
        UpdateSampleEntryInfo(sampleInfo, offset, currentDts, isOverlap);
        duration += sampleInfo.duration;
        prevDts = currentDts;
        currentDts += sampleInfo.duration;
        offset += sampleInfo.size;
        FALSE_RETURN_V_MSG_E(duration >= 0 && offset >= 0, Status::ERROR_INVALID_DATA,
            "Duration DTS or Offset is negative");
    }
    totalTrunDuration_ = currentDts;
    baseFragmentDts_ = currentDts;
    totalDuration_ = (totalDuration_ < baseFragmentDts_) ? baseFragmentDts_ : totalDuration_;
    rawTotalDuration_ = std::max(rawTotalDuration_, totalDuration_);
    return Status::OK;
}

Status MPEG4SampleHelper::ExpandCttsEntry(uint32_t sttsNum)
{
    uint32_t cttsNum = 0;
    for (const auto& entry : cttsEntry_) {
        uint32_t temp;
        if (__builtin_add_overflow(cttsNum, entry.count, &temp)) {
            MEDIA_LOG_E("CTTS count overflow: current=" PUBLIC_LOG_U32 ", add="
                PUBLIC_LOG_U32, cttsNum, entry.count);
            return Status::ERROR_INVALID_DATA;
        }
        cttsNum = temp;
    }
    FALSE_RETURN_V_MSG_E(sttsNum == cttsNum, Status::ERROR_INVALID_DATA, "ctts did not match stts");
    std::vector<CttsEntry> tempCttsEntry;
    tempCttsEntry.reserve(cttsNum);
    for (const auto& entry : cttsEntry_) {
        for (uint32_t j = 0; j < entry.count; ++j) {
            tempCttsEntry.emplace_back(CttsEntry {
                .count = 1,
                .delta = entry.delta
            });
        }
    }
    cttsEntry_ = std::move(tempCttsEntry);
    return Status::OK;
}

Status MPEG4SampleHelper::PreBuildProcessAndCheck()
{
    FALSE_RETURN_V_MSG_E(!sttsEntry_.empty(), Status::ERROR_INVALID_DATA, "stts corrupted");
    FALSE_RETURN_V_MSG_E(!sampleSizes_.empty(), Status::ERROR_INVALID_DATA, "stsz corrupted");
    FALSE_RETURN_V_MSG_E(!chunkOffsets_.empty(), Status::ERROR_INVALID_DATA, "stco corrupted");
    FALSE_RETURN_V_MSG_E(!stscEntry_.empty(), Status::ERROR_INVALID_DATA, "stsc corrupted");

    uint32_t sttsNum = 0;
    for (const auto& entry : sttsEntry_) {
        sttsNum += entry.count;
    }

    if (!cttsEntry_.empty()) {
        // cttsEntry展开
        Status ret = ExpandCttsEntry(sttsNum);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ExpandCttsEntry failed");
        CalculateDtsShift();
    }

    if (!hasDefaultStszSize_) {
        FALSE_RETURN_V_MSG_E(sttsNum == sampleSizes_.size(), Status::ERROR_INVALID_DATA, "stsz did not match stts");
    }

    return Status::OK;
}

bool MPEG4SampleHelper::GetSampleSize(uint32_t sampleIndex, uint32_t &samplesPerFrame,
    SampleIndexEntry &entry, int64_t &remainSamples)
{
    if (mimeType_ == MimeType::AUDIO_G711MU || mimeType_ == MimeType::AUDIO_G711A || mimeType_ == MimeType::AUDIO_RAW) {
        int32_t sampleSize = sampleSizes_[0];
        if (sampleSize > INT32_MAX / MAX_SAMPLES_PER_FRAME) {
            return false;
        }
        samplesPerFrame = std::min(MAX_SAMPLES_PER_FRAME, remainSamples);
        remainSamples -= samplesPerFrame;
        entry.size = samplesPerFrame * sampleSize;
    } else {
        entry.size = hasDefaultStszSize_ ? sampleSizes_[0] : sampleSizes_[sampleIndex];
    }
    return true;
}

Status MPEG4SampleHelper::UpdateSampleDurationAndDts(uint32_t sttsDelta, uint32_t samplesPerFrame,
    SampleIndexEntry& entry, int64_t& currentDts) const
{
    FALSE_RETURN_V_MSG_E(samplesPerFrame > 0, Status::ERROR_INVALID_DATA, "Invalid samples per frame");
    int64_t delta = static_cast<int64_t>(sttsDelta);
    int64_t frameCount = static_cast<int64_t>(samplesPerFrame);
    FALSE_RETURN_V_MSG_E(delta <= std::numeric_limits<int64_t>::max() / frameCount,
        Status::ERROR_INVALID_DATA, "Sample duration is too large");
    int64_t duration = delta * frameCount;
    FALSE_RETURN_V_MSG_E(currentDts <= std::numeric_limits<int64_t>::max() - duration,
        Status::ERROR_INVALID_DATA, "Sample dts is too large");
    entry.duration = duration;
    currentDts += duration;
    return Status::OK;
}

void MPEG4SampleHelper::GetSampleFlag(uint32_t sampleIndex, SampleIndexEntry &entry, uint32_t &syncIndex)
{
    if (trackType_ == VIDEO_TYPE || trackType_ == AUDIO_TYPE || trackType_ == TIMEDMETA_TYPE ||
        trackType_ == AUXILIARY_TYPE) {
        if (!stssEntry_.empty()) {
            if ((syncIndex < stssEntry_.size()) && (sampleIndex == stssEntry_[syncIndex].index)) {
                entry.flag |= Sample::SampleFlag::SYNC_FRAME;
                stssEntry_[syncIndex].syncPts = entry.pts;
                stssEntry_[syncIndex].syncDts = entry.dts;
                ++syncIndex;
            } else {
                entry.flag |= Sample::SampleFlag::NONE;
            }
        } else {
            entry.flag |= Sample::SampleFlag::SYNC_FRAME;
        }
    } else if (trackType_ == SUBTITLE_TYPE) {
        if (mimeType_ == MimeType::TEXT_WEBVTT) {
            entry.flag |= Sample::SampleFlag::SYNC_FRAME;
        }
    }
}

namespace {
std::vector<uint8_t> MakeSkipSamplesInfo(int64_t skipSamples)
{
    constexpr size_t skipSamplesInfoSize = 10;
    std::vector<uint8_t> info(skipSamplesInfoSize, 0);
    uint32_t skip = skipSamples > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(std::max<int64_t>(skipSamples, 0));
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        info[i] = static_cast<uint8_t>((skip >> (BYTE_BITS * i)) & 0xFF);
    }
    return info;
}
}

Status MPEG4SampleHelper::RebuildSyncSamples()
{
    stssEntry_.clear();
    for (uint32_t i = 0; i < sampleIndexEntry_.size(); ++i) {
        if (sampleIndexEntry_[i].flag & Sample::SampleFlag::SYNC_FRAME) {
            stssEntry_.emplace_back(StssEntry {
                .index = i,
                .syncPts = sampleIndexEntry_[i].pts,
                .syncDts = sampleIndexEntry_[i].dts
            });
        }
    }
    return Status::OK;
}

bool MPEG4SampleHelper::IsSyncSample(const SampleIndexEntry& entry) const
{
    return (entry.flag & Sample::SampleFlag::SYNC_FRAME) != 0;
}

int64_t MPEG4SampleHelper::GetCttsDelta(size_t index) const
{
    return index < cttsEntry_.size() ? cttsEntry_[index].delta : 0;
}

int64_t MPEG4SampleHelper::GetFrameDuration(const std::vector<SampleIndexEntry>& entries, size_t index,
    int64_t editDuration) const
{
    if (index + 1 < entries.size()) {
        int64_t duration = entries[index + 1].dts - entries[index].dts;
        if (duration > 0) {
            return duration;
        }
    }
    return entries[index].duration > 0 ? entries[index].duration : editDuration;
}

int64_t MPEG4SampleHelper::FindPrevClosestIndex(const std::vector<SampleIndexEntry>& entries,
    int64_t timestampPts, bool anyFrame) const
{
    if (dtsShift_ > 0) {
        timestampPts -= dtsShift_;
    }
    int64_t found = -1;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].dts > timestampPts) {
            break;
        }
        if (anyFrame || IsSyncSample(entries[i])) {
            found = static_cast<int64_t>(i);
        }
    }
    while (!cttsEntry_.empty() && found >= 0) {
        const auto& entry = entries[found];
        if ((entry.dts + GetCttsDelta(found)) <= timestampPts && IsSyncSample(entry)) {
            break;
        }
        --found;
    }
    return found;
}

void MPEG4SampleHelper::FixLeadingDiscardTimestamps(std::vector<SampleIndexEntry>& entries, size_t endIndex,
    int64_t endDts, const std::vector<int64_t>& leadingDurations) const
{
    for (size_t i = 0; i < leadingDurations.size(); ++i) {
        endDts -= leadingDurations[leadingDurations.size() - 1 - i];
        SampleIndexEntry& entry = entries[endIndex - 1 - i];
        int64_t ptsDelta = entry.pts - entry.dts;
        entry.dts = endDts;
        entry.pts = endDts + ptsDelta;
    }
}

Status MPEG4SampleHelper::ConvertEditDuration(const EditListEntry& edit, int64_t& editDuration) const
{
    FALSE_RETURN_V_MSG_E(ConvertTimeScaleRoundNearest(edit.duration, trackTimeScale_, movieTimeScale_, &editDuration)
        == Status::OK, Status::ERROR_INVALID_DATA, "Convert edit duration failed");
    return editDuration < 0 ? Status::ERROR_INVALID_DATA : Status::OK;
}

Status MPEG4SampleHelper::PrepareEditListEntry(const EditListEntry& edit, EditListBuildContext& buildCtx,
    EditListProcessContext& processCtx)
{
    processCtx = EditListProcessContext {};
    processCtx.edit = &edit;
    FALSE_RETURN_V_MSG_E(ConvertEditDuration(edit, processCtx.editDuration) == Status::OK,
        Status::ERROR_INVALID_DATA, "Convert edit duration failed");
    processCtx.dtsCounter = buildCtx.editListDtsEntryEnd;
    if (__builtin_add_overflow(buildCtx.editListDtsEntryEnd, processCtx.editDuration,
        &buildCtx.editListDtsEntryEnd)) {
        return Status::ERROR_INVALID_DATA;
    }
    if (!buildCtx.foundNonEmptyEdit && edit.mediaTime == -1) {
        buildCtx.emptyEditsDuration += processCtx.editDuration;
        processCtx.skipEntry = true;
        return Status::OK;
    }
    buildCtx.foundNonEmptyEdit = true;
    if (trackType_ == AUDIO_TYPE) {
        buildCtx.firstNonZeroAudioEdit = buildCtx.firstNonZeroAudioEdit < 0 ? 1 : 0;
    }

    int64_t searchTimestamp = edit.mediaTime;
    if (trackType_ == AUDIO_TYPE) {
        searchTimestamp = std::max(searchTimestamp - trackTimeScale_, buildCtx.oldEntries.front().dts);
    }
    int64_t startIndex = FindPrevClosestIndex(buildCtx.oldEntries, searchTimestamp, false);
    if (startIndex < 0) {
        MEDIA_LOG_W("Missing key frame while searching edit timestamp: " PUBLIC_LOG_D64, searchTimestamp);
        startIndex = FindPrevClosestIndex(buildCtx.oldEntries, searchTimestamp, true);
    }
    processCtx.startIndex = static_cast<size_t>(std::max<int64_t>(startIndex, 0));
    FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(edit.mediaTime, processCtx.editDuration,
        &processCtx.editEndTime), Status::ERROR_INVALID_DATA, "Edit end time overflow");
    return Status::OK;
}

Status MPEG4SampleHelper::BuildEditListSampleEntry(const SampleIndexEntry& oldEntry, size_t index,
    EditListBuildContext& buildCtx, EditListProcessContext& processCtx)
{
    const auto& edit = *processCtx.edit;
    int64_t frameDuration = GetFrameDuration(buildCtx.oldEntries, index, processCtx.editDuration);
    int64_t currCtts = GetCttsDelta(index);
    int64_t currCts = 0;
    int64_t shiftedDts = 0;
    if (__builtin_add_overflow(oldEntry.dts, dtsShift_, &shiftedDts) ||
        __builtin_add_overflow(shiftedDts, currCtts, &currCts)) {
        return Status::ERROR_INVALID_DATA;
    }
    if (__builtin_add_overflow(currCts, frameDuration, &processCtx.frameEndTime)) {
        return Status::ERROR_INVALID_DATA;
    }

    processCtx.newEntry = oldEntry;
    processCtx.newEntry.dts = processCtx.dtsCounter;
    processCtx.newEntry.pts = processCtx.dtsCounter + dtsShift_ + currCtts;
    processCtx.newEntry.duration = frameDuration;
    processCtx.newEntry.skipSamplesInfo.clear();
    bool outsideEdit = currCts < edit.mediaTime || currCts >= processCtx.editEndTime;
    bool crossedAudioStart = trackType_ == AUDIO_TYPE && mimeType_ != MimeType::AUDIO_VORBIS &&
        currCts < edit.mediaTime && processCtx.frameEndTime >= edit.mediaTime && buildCtx.firstNonZeroAudioEdit > 0;
    if (outsideEdit && crossedAudioStart) {
        int64_t packetSkipSamples = edit.mediaTime - currCts;
        processCtx.newEntry.skipSamplesInfo = MakeSkipSamplesInfo(packetSkipSamples);
        processCtx.newEntry.flag |= Sample::SampleFlag::DISCARD;
        processCtx.dtsCounter -= packetSkipSamples;
        processCtx.newEntry.dts = processCtx.dtsCounter;
        processCtx.newEntry.pts = processCtx.dtsCounter + dtsShift_ + currCtts;
    } else if (outsideEdit) {
        processCtx.newEntry.flag |= Sample::SampleFlag::DISCARD;
        if (!processCtx.editListStartEncountered) {
            processCtx.leadingDurations.emplace_back(frameDuration);
        }
    } else {
        int64_t correctedPts = processCtx.dtsCounter + currCtts + dtsShift_;
        buildCtx.minCorrectedPts = buildCtx.minCorrectedPts < 0 ? correctedPts :
            std::min(buildCtx.minCorrectedPts, correctedPts);
    }

    if (!outsideEdit || crossedAudioStart) {
        processCtx.editListStartEncountered = true;
        FixLeadingDiscardTimestamps(buildCtx.newEntries, buildCtx.newEntries.size(), processCtx.dtsCounter,
            processCtx.leadingDurations);
        processCtx.leadingDurations.clear();
    }
    return Status::OK;
}

Status MPEG4SampleHelper::AppendEditListSample(const SampleIndexEntry& oldEntry, size_t index,
    EditListBuildContext& buildCtx, EditListProcessContext& processCtx)
{
    Status ret = BuildEditListSampleEntry(oldEntry, index, buildCtx, processCtx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Build edit list sample entry failed");
    buildCtx.newEntries.emplace_back(std::move(processCtx.newEntry));
    if (processCtx.editListStartEncountered) {
        processCtx.dtsCounter += buildCtx.newEntries.back().duration;
    }
    if (processCtx.frameEndTime >= processCtx.editEndTime && (IsSyncSample(oldEntry) || trackType_ == AUDIO_TYPE)) {
        if (!cttsEntry_.empty() && trackType_ != AUDIO_TYPE && !processCtx.foundKeyframeAfterEdit) {
            processCtx.foundKeyframeAfterEdit = true;
            processCtx.continueAfterEdit = true;
        } else {
            processCtx.shouldBreak = true;
        }
    }
    return Status::OK;
}

Status MPEG4SampleHelper::AppendEditListSamples(EditListBuildContext& buildCtx,
    EditListProcessContext& processCtx)
{
    for (size_t i = processCtx.startIndex; i < buildCtx.oldEntries.size(); ++i) {
        const auto& oldEntry = buildCtx.oldEntries[i];
        Status ret = AppendEditListSample(oldEntry, i, buildCtx, processCtx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Append edit list sample failed");
        if (processCtx.continueAfterEdit) {
            processCtx.continueAfterEdit = false;
            continue;
        }
        if (processCtx.shouldBreak) {
            break;
        }
    }
    return Status::OK;
}

void MPEG4SampleHelper::FinalizeAppliedEditList(EditListBuildContext& buildCtx)
{
    if (buildCtx.minCorrectedPts >= 0) {
        buildCtx.minCorrectedPts -= buildCtx.emptyEditsDuration;
        if (trackType_ == VIDEO_TYPE && buildCtx.minCorrectedPts > 0) {
            for (auto& entry : buildCtx.newEntries) {
                entry.dts -= buildCtx.minCorrectedPts;
                entry.pts -= buildCtx.minCorrectedPts;
            }
        }
    }

    sampleIndexEntry_ = std::move(buildCtx.newEntries);
    totalDuration_ = buildCtx.editListDtsEntryEnd - buildCtx.startDts;
    lastPTS_ = 0;
    for (const auto& entry : sampleIndexEntry_) {
        lastPTS_ = std::max(lastPTS_, entry.pts);
    }
    RebuildSyncSamples();
    editListApplied_ = true;
}

Status MPEG4SampleHelper::ApplyEditList()
{
    if (editListEntries_.empty() || sampleIndexEntry_.empty()) {
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(movieTimeScale_ > 0 && trackTimeScale_ > 0, Status::ERROR_INVALID_DATA,
        "Invalid edit list time scale");

    const auto oldEntries = sampleIndexEntry_;
    std::vector<SampleIndexEntry> newEntries;
    newEntries.reserve(oldEntries.size());

    int64_t editListDtsEntryEnd = dtsShift_ > 0 ? -dtsShift_ : 0;
    int64_t startDts = editListDtsEntryEnd;
    int64_t emptyEditsDuration = 0;
    int64_t minCorrectedPts = -1;
    bool foundNonEmptyEdit = false;
    int32_t firstNonZeroAudioEdit = -1;
    EditListBuildContext buildCtx {
        oldEntries, newEntries, editListDtsEntryEnd, emptyEditsDuration, minCorrectedPts,
        foundNonEmptyEdit, firstNonZeroAudioEdit, startDts
    };

    for (const auto& edit : editListEntries_) {
        EditListProcessContext processCtx;
        Status ret = PrepareEditListEntry(edit, buildCtx, processCtx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Prepare edit list entry failed");
        if (processCtx.skipEntry) {
            continue;
        }
        ret = AppendEditListSamples(buildCtx, processCtx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Append edit list samples failed");
    }

    if (newEntries.empty()) {
        return Status::OK;
    }
    FinalizeAppliedEditList(buildCtx);
    return Status::OK;
}

uint64_t MPEG4SampleHelper::CountSttsSamples() const
{
    uint64_t totalSttsSamples = 0;
    for (const auto& sttsEntry : sttsEntry_) {
        totalSttsSamples += sttsEntry.count;
    }
    return totalSttsSamples;
}

MPEG4SampleHelper::SampleIndexBuildContext MPEG4SampleHelper::CreateSampleIndexBuildContext() const
{
    SampleIndexBuildContext buildCtx;
    buildCtx.currentOffset = chunkOffsets_[0];
    buildCtx.currentDts = editListEntries_.empty() ? -timeOffset_ - dtsShift_ : -dtsShift_;
    buildCtx.remainSamples = static_cast<int64_t>(stscEntry_[buildCtx.stscIndex].count);
    buildCtx.totalSttsSamples = CountSttsSamples();
    return buildCtx;
}

Status MPEG4SampleHelper::AdvanceChunkIfNeeded(SampleIndexBuildContext& buildCtx)
{
    if (buildCtx.sampleInChunk < stscEntry_[buildCtx.stscIndex].count) {
        return Status::OK;
    }
    ++buildCtx.chunkIndex;
    buildCtx.sampleInChunk = 0;
    if (buildCtx.processedSttsSamples >= buildCtx.totalSttsSamples) {
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(buildCtx.chunkIndex < chunkOffsets_.size(), Status::ERROR_INVALID_DATA,
        "Chunk index is out of range");
    buildCtx.stscIndex = (buildCtx.stscIndex < stscEntry_.size() - 1 &&
        buildCtx.chunkIndex == stscEntry_[buildCtx.stscIndex + 1].first) ? buildCtx.stscIndex + 1 :
        buildCtx.stscIndex;
    buildCtx.remainSamples = stscEntry_[buildCtx.stscIndex].count;
    buildCtx.currentOffset = chunkOffsets_[buildCtx.chunkIndex];
    return Status::OK;
}

Status MPEG4SampleHelper::AppendSampleIndexEntry(const SttsEntry& sttsEntry, uint32_t& sampleInSttsEntry,
    SampleIndexBuildContext& buildCtx)
{
    int64_t ctsOffset = cttsEntry_.empty() ? 0 : cttsEntry_[buildCtx.sampleIndex].delta;
    sampleIndexEntry_.emplace_back(SampleIndexEntry {
        .dts = buildCtx.currentDts,
        .pts = buildCtx.currentDts + dtsShift_ + ctsOffset,
        .pos = buildCtx.currentOffset,
        .flag = Sample::NONE,
        .size = 0,
        .duration = 0
    });
    FALSE_RETURN_V_MSG_E(GetSampleSize(buildCtx.sampleIndex, buildCtx.samplesPerFrame,
        sampleIndexEntry_.back(), buildCtx.remainSamples), Status::ERROR_INVALID_DATA, "Sample size is too large");
    Status ret = UpdateSampleDurationAndDts(sttsEntry.delta, buildCtx.samplesPerFrame, sampleIndexEntry_.back(),
        buildCtx.currentDts);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Update sample duration failed");
    GetSampleFlag(buildCtx.sampleIndex, sampleIndexEntry_.back(), buildCtx.syncIndex);
    lastPTS_ = std::max(lastPTS_, sampleIndexEntry_.back().pts);
    buildCtx.currentOffset += sampleIndexEntry_.back().size;
    FALSE_RETURN_V_MSG_E(buildCtx.currentOffset >= 0, Status::ERROR_INVALID_DATA, "Sample Offset is negative");
    ++buildCtx.sampleIndex;
    sampleInSttsEntry += buildCtx.samplesPerFrame;
    buildCtx.sampleInChunk += buildCtx.samplesPerFrame;
    buildCtx.processedSttsSamples += buildCtx.samplesPerFrame;
    return AdvanceChunkIfNeeded(buildCtx);
}

Status MPEG4SampleHelper::BuildSampleIndexEntries()
{
    FALSE_RETURN_V_MSG_E(PreBuildProcessAndCheck() == Status::OK, Status::ERROR_INVALID_DATA, "PreCheck failed");
    SampleIndexBuildContext buildCtx = CreateSampleIndexBuildContext();
    for (const auto& sttsEntry : sttsEntry_) {  // 遍历sttsEntry
        uint32_t sampleInSttsEntry = 0;
        while (sampleInSttsEntry < sttsEntry.count) {  // 根据sttsEntry.count进行循环
            Status ret = AppendSampleIndexEntry(sttsEntry, sampleInSttsEntry, buildCtx);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Append sample index entry failed");
        }
    }

    UpdateRawSampleStats();
    Status ret = ApplyEditList();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Apply edit list failed");
    return Status::OK;
}

void MPEG4SampleHelper::UpdateRawSampleStats()
{
    totalDuration_ = 0;
    rawSampleCount_ = 0;
    rawTotalSize_ = 0;
    for (const auto& sttsEntry : sttsEntry_) {
        totalDuration_ += static_cast<int64_t>(sttsEntry.count) * static_cast<int64_t>(sttsEntry.delta);
        rawSampleCount_ += sttsEntry.count;
    }
    for (const auto& sample : sampleIndexEntry_) {
        rawTotalSize_ += sample.size;
    }
    rawTotalDuration_ = totalDuration_;
}

Status MPEG4SampleHelper::GetSampleDuration(uint32_t index, uint32_t &duration)
{
    if (sampleIndexEntry_.empty() || sttsEntry_.empty()) {
        return Status::ERROR_INVALID_OPERATION;
    }

    if (firstMoofSampleIndex_ > 0) {
        if (index < firstMoofSampleIndex_) {
            int64_t temp = index;
            uint32_t sttsIndex = 0;
            while (sttsIndex < sttsEntry_.size() - 1 &&
                temp >= static_cast<int64_t>(sttsEntry_[sttsIndex].count)) {
                temp -= sttsEntry_[sttsIndex].count;
                ++sttsIndex;
            }
            duration = sttsEntry_[sttsIndex].delta;
        } else {
            FALSE_RETURN_V_MSG_E(index >= numMoovSamples_ && firstMoofSttsIndex_ > 0,
                Status::ERROR_INVALID_PARAMETER, "Invalid sample index");
            uint64_t sttsIndex = static_cast<uint64_t>(index) - numMoovSamples_ + firstMoofSttsIndex_ - 1;
            FALSE_RETURN_V_MSG_E(sttsIndex < sttsEntry_.size(), Status::ERROR_INVALID_PARAMETER,
                "sttsIndex out of range");
            duration = sttsEntry_[sttsIndex].delta;
        }
        return Status::OK;
    }

    int64_t temp = index;
    uint32_t sttsIndex = 0;
    while (sttsIndex < sttsEntry_.size() - 1 && temp >= static_cast<int64_t>(sttsEntry_[sttsIndex].count)) {
        temp -= sttsEntry_[sttsIndex].count;
        ++sttsIndex;
    }
    if (sttsIndex >= sttsEntry_.size()) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    duration = sttsEntry_[sttsIndex].delta;
    return Status::OK;
}

void MPEG4SampleHelper::SetTimeOffset(int64_t timeOffset)
{
    timeOffset_ = timeOffset;
}

void MPEG4SampleHelper::CalculateDtsShift()
{
    dtsShift_ = 0;
    for (const auto& entry : cttsEntry_) {
        if (entry.delta < 0) {
            dtsShift_ = std::max(dtsShift_, static_cast<int64_t>(-entry.delta));
        }
    }
}

uint32_t MPEG4SampleHelper::CalculateSampleCount()
{
    uint32_t sampleCount = 0;
    for (const auto& entry : sttsEntry_) {
        FALSE_RETURN_V_MSG_E(UINT32_MAX - entry.count >= sampleCount, 0, "sampleCount overflow");
        sampleCount += entry.count;
    }
    MEDIA_LOG_I("sampleCount is " PUBLIC_LOG_U32, sampleCount);
    return sampleCount;
}

uint32_t MPEG4SampleHelper::GetSttsEntryCount()
{
    return sttsEntry_.size();
}

uint32_t MPEG4SampleHelper::GetSampleCountOfSttsEntry(uint32_t sttsEntryIndex)
{
    FALSE_RETURN_V_MSG_E(sttsEntryIndex >= 0 && sttsEntryIndex < sttsEntry_.size(), 0,
        "sttsEntryIndex(%{public}d) is invalid", sttsEntryIndex);
    return sttsEntry_[sttsEntryIndex].count;
}

uint32_t MPEG4SampleHelper::GetSampleDeltaOfSttsEntry(uint32_t sttsEntryIndex)
{
    FALSE_RETURN_V_MSG_E(sttsEntryIndex >= 0 && sttsEntryIndex < sttsEntry_.size(), 0,
        "sttsEntryIndex(%{public}d) is invalid", sttsEntryIndex);
    return sttsEntry_[sttsEntryIndex].delta;
}

uint32_t MPEG4SampleHelper::GetCttsEntryCount()
{
    return cttsEntry_.size();
}

uint32_t MPEG4SampleHelper::GetSampleCountOfCttsEntry(uint32_t cttsEntryIndex)
{
    FALSE_RETURN_V_MSG_E(cttsEntryIndex >= 0 && cttsEntryIndex < cttsEntry_.size(), 0,
        "cttsEntryIndex(%{public}d) is invalid", cttsEntryIndex);
    return cttsEntry_[cttsEntryIndex].count;
}

int32_t MPEG4SampleHelper::GetSampleOffsetOfCttsEntry(uint32_t cttsEntryIndex)
{
    FALSE_RETURN_V_MSG_E(cttsEntryIndex >= 0 && cttsEntryIndex < cttsEntry_.size(), 0,
        "cttsEntryIndex(%{public}d) is invalid", cttsEntryIndex);
    return cttsEntry_[cttsEntryIndex].delta;
}

int32_t MPEG4SampleHelper::GetMinOffsetOfCttsEntry()
{
    int32_t minSampleOffsetOfCttsEntry = INT32_MAX;
    for (uint32_t i = 0; i < cttsEntry_.size(); ++i) {
        minSampleOffsetOfCttsEntry = minSampleOffsetOfCttsEntry < cttsEntry_[i].delta ? minSampleOffsetOfCttsEntry :
            cttsEntry_[i].delta;
    }
    return std::min(0, minSampleOffsetOfCttsEntry);
}

std::vector<uint32_t> MPEG4SampleHelper::GetSyncSamplesIndexFromStssEntry()
{
    std::vector<uint32_t> syncSamplesIndex;
    for (uint32_t i = 0; i < stssEntry_.size(); ++i) {
        syncSamplesIndex.emplace_back(stssEntry_[i].index);
    }
    return syncSamplesIndex;
}

MPEG4SampleHelper::SdtpEntry MPEG4SampleHelper::GetSdtpEntry()
{
    return sdtpEntry_;
}

Status MPEG4SampleHelper::GetPtsOfFirstSample(int64_t& startPtsBasedOnTimescale)
{
    FALSE_RETURN_V_MSG_E(!sampleIndexEntry_.empty(), Status::ERROR_INVALID_DATA, "sampleIndexEntry_ is empty");
    startPtsBasedOnTimescale = sampleIndexEntry_[0].pts;  // pts based on track time scale
    return Status::OK;
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
