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
#include <algorithm>
#include <limits>
#include "meta/mime_type.h"
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
constexpr uint32_t MAX_SAMPLE_TABLE_SAMPLES = static_cast<uint32_t>(MAX_TRUN_SAMPLES);
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
constexpr uint32_t HALF_BYTE_NUMBER = 2;

Status ConvertTimeScaleRoundNearest(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result)
{
    int64_t scaled = 0;
    if (result == nullptr || timeScale <= 0) {
        return Status::ERROR_INVALID_DATA;
    }
    if (__builtin_mul_overflow(multiplicand, multiplier, &scaled)) {
        return Status::ERROR_INVALID_DATA;
    }
    int64_t quotient = scaled / timeScale;
    int64_t remainder = scaled % timeScale;
    int64_t halfTimeScale = timeScale / 2;
    int64_t roundThreshold = timeScale - halfTimeScale;
    if (remainder > 0 && remainder >= roundThreshold) {
        FALSE_RETURN_V_NOLOG(quotient != std::numeric_limits<int64_t>::max(), Status::ERROR_INVALID_DATA);
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
      cttsEntry_(),
      seekable_(Seekable::UNSEEKABLE),
      trackType_(INVALID_TYPE),
      mimeType_(""),
      firstMoofSampleIndex_(-1),
      sttsEntry_(),
      sampleIndexEntry_(),
      stscEntry_(),
      stssEntry_(),
      sampleSizes_(),
      chunkOffsets_(),
      trex_(),
      tfhd_(),
      dtsShift_(0),
      timeOffset_(0),
      skipSamplesInfoMap_()
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

// LCOV_EXCL_START
bool MPEG4SampleHelper::HasEditList() const
{
    return !editListEntries_.empty();
}
// LCOV_EXCL_STOP

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

bool MPEG4SampleHelper::HasSkipSamplesInfo(uint32_t sampleIndex) const
{
    return skipSamplesInfoMap_.find(sampleIndex) != skipSamplesInfoMap_.end();
}

const std::vector<uint8_t>* MPEG4SampleHelper::GetSkipSamplesInfo(uint32_t sampleIndex) const
{
    auto iter = skipSamplesInfoMap_.find(sampleIndex);
    return iter == skipSamplesInfoMap_.end() ? nullptr : &iter->second;
}

Status MPEG4SampleHelper::FindWithSyncSamplesByPts(
    int64_t inputPts, bool isBaseTrack, SeekMode flag, uint32_t &syncIndex)
{
    MEDIA_LOG_D("In");
    int64_t previousIndex = -1;
    int64_t nextIndex = -1;
    for (size_t i = 0; i < stssEntry_.size(); ++i) {
        const int64_t syncPts = GetSyncSamplePts(i);
        if (syncPts <= inputPts && (previousIndex < 0 ||
            syncPts > GetSyncSamplePts(static_cast<size_t>(previousIndex)))) {
            previousIndex = static_cast<int64_t>(i);
        }
        if (syncPts >= inputPts && (nextIndex < 0 ||
            syncPts < GetSyncSamplePts(static_cast<size_t>(nextIndex)))) {
            nextIndex = static_cast<int64_t>(i);
        }
    }

    int64_t selectedIndex = -1;
    if (flag == SeekMode::SEEK_PREVIOUS_SYNC) {
        selectedIndex = previousIndex;
    } else if (flag == SeekMode::SEEK_NEXT_SYNC) {
        selectedIndex = nextIndex;
    } else if (previousIndex >= 0 && nextIndex >= 0) {
        selectedIndex = GetClosestSync(GetSyncSamplePts(static_cast<size_t>(previousIndex)),
            GetSyncSamplePts(static_cast<size_t>(nextIndex)), inputPts) ? nextIndex : previousIndex;
    } else {
        selectedIndex = (previousIndex >= 0) ? previousIndex : nextIndex;
    }

    if (selectedIndex < 0) {
        if (isBaseTrack && flag == SeekMode::SEEK_PREVIOUS_SYNC && !stssEntry_.empty() &&
            inputPts > GetSyncSamplePts(stssEntry_.size() - 1)) {
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
            if (inputPts < entry[currentIndex].GetPts() && isBaseTrack) {
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
    while (currentIndex >= 0 && (!IsSyncSample(static_cast<uint32_t>(currentIndex)) ||
        entry[currentIndex].GetPts() > inputPts)) {
        --currentIndex;
    }
    return currentIndex;
}

int64_t MPEG4SampleHelper::FindNextSyncByPts(int64_t inputPts, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    while (currentIndex < static_cast<int64_t>(entry.size()) &&
        (!IsSyncSample(static_cast<uint32_t>(currentIndex)) || entry[currentIndex].GetPts() < inputPts)) {
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
        currentIndex = GetClosestSync(entry[prevSync].GetPts(), entry[nextSync].GetPts(), inputPts) ?
            nextSync : prevSync;
    } else if (prevSync < 0) {
        currentIndex = nextSync;
    } else {
        currentIndex = prevSync;
    }
}

int64_t MPEG4SampleHelper::FindIndexWithoutSyncSamples(int64_t inputPTS) const
{
    auto it = std::lower_bound(sampleIndexEntry_.begin(), sampleIndexEntry_.end(), inputPTS,
        [](const SampleIndexEntry& entry, int64_t pts) { return entry.GetPts() < pts; });
    return std::distance(sampleIndexEntry_.begin(), it);
}

bool MPEG4SampleHelper::AssignSampleIndex(int64_t currentIndex, uint32_t& sampleIndex) const
{
    FALSE_RETURN_V_NOLOG(currentIndex >= 0 &&
        static_cast<uint64_t>(currentIndex) <= static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()), false);
    sampleIndex = static_cast<uint32_t>(currentIndex);
    return true;
}

bool MPEG4SampleHelper::AssignSampleIndexFromSize(size_t currentIndex, uint32_t& sampleIndex) const
{
    FALSE_RETURN_V_NOLOG(currentIndex <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), false);
    sampleIndex = static_cast<uint32_t>(currentIndex);
    return true;
}

bool MPEG4SampleHelper::ResolveDirectSyncSampleByPts(int64_t inputPts, int64_t currentIndex,
    uint32_t& sampleIndex) const
{
    const auto& entry = sampleIndexEntry_;
    if (trackType_ == AUDIO_TYPE && currentIndex >= 0 &&
        static_cast<size_t>(currentIndex) < entry.size()) {
        int64_t duration = 0;
        int64_t sampleEnd = 0;
        if (GetSampleDurationTicks(static_cast<uint32_t>(currentIndex), duration) == Status::OK && duration >= 0 &&
            !__builtin_add_overflow(entry[currentIndex].GetPts(), duration, &sampleEnd) &&
            inputPts >= entry[currentIndex].GetPts() && inputPts <= sampleEnd) {
            return AssignSampleIndex(currentIndex, sampleIndex);
        }
    }
    if (!IsSyncSample(static_cast<uint32_t>(currentIndex)) || inputPts != entry[currentIndex].GetPts()) {
        return false;
    }
    return AssignSampleIndex(currentIndex, sampleIndex);
}

bool MPEG4SampleHelper::ResolveNonBaseTrackEosByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag,
    uint32_t& sampleIndex) const
{
    bool beyondLastSync =
        !stssEntry_.empty() && inputPts > GetSyncSamplePts(stssEntry_.size() - 1) &&
        flag == SeekMode::SEEK_NEXT_SYNC;
    bool atOrBeyondLastSampleEnd = IsAtOrBeyondLastSampleEnd(inputPts);
    if (!isBaseTrack && trackType_ == AUDIO_TYPE && flag == SeekMode::SEEK_PREVIOUS_SYNC &&
        atOrBeyondLastSampleEnd) {
        return AssignSampleIndexFromSize(sampleIndexEntry_.size() - 1, sampleIndex);
    }
    if (!isBaseTrack && trackType_ == AUDIO_TYPE && flag == SeekMode::SEEK_NEXT_SYNC &&
        atOrBeyondLastSampleEnd) {
        return AssignSampleIndexFromSize(sampleIndexEntry_.size(), sampleIndex);
    }
    if (isBaseTrack || (!atOrBeyondLastSampleEnd && !beyondLastSync)) {
        return false;
    }
    return AssignSampleIndexFromSize(sampleIndexEntry_.size(), sampleIndex);
}

bool MPEG4SampleHelper::IsAtOrBeyondLastSampleEnd(int64_t inputPts) const
{
    if (sampleIndexState_.load(std::memory_order_acquire) != SampleIndexState::DENSE && sampleIndexEntry_.empty()) {
        size_t sampleCount = GetSampleCount();
        FALSE_RETURN_V_NOLOG(sampleCount > 0, true);
        int64_t lastPts = 0;
        int64_t duration = 0;
        if (GetSamplePts(static_cast<uint32_t>(sampleCount - 1), lastPts) != Status::OK ||
            GetSampleDurationTicks(static_cast<uint32_t>(sampleCount - 1), duration) != Status::OK || duration < 0) {
            return inputPts >= lastPTS_;
        }
        int64_t sampleEnd = 0;
        return __builtin_add_overflow(lastPts, duration, &sampleEnd) ? inputPts >= lastPts : inputPts >= sampleEnd;
    }
    FALSE_RETURN_V_NOLOG(!sampleIndexEntry_.empty(), true);
    int64_t duration = 0;
    int64_t sampleEnd = 0;
    uint32_t lastIndex = static_cast<uint32_t>(sampleIndexEntry_.size() - 1);
    if (GetSampleDurationTicks(lastIndex, duration) != Status::OK || duration < 0 ||
        __builtin_add_overflow(lastPTS_, duration, &sampleEnd)) {
        return inputPts >= lastPTS_;
    }
    return inputPts >= sampleEnd;
}

bool MPEG4SampleHelper::ResolveTextWebVttSample(int64_t inputPTS, int64_t& currentIndex,
    uint32_t& sampleIndex) const
{
    if (mimeType_ != MimeType::TEXT_WEBVTT) {
        return false;
    }
    const auto& entry = sampleIndexEntry_;
    FALSE_RETURN_V_NOLOG(AssignSampleIndex(currentIndex, sampleIndex), false);
    if (currentIndex > 0 && inputPTS < entry[currentIndex].GetPts()) {
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
    int64_t duration = 0;
    return IsAudioSampleIndexInRange(currentIndex) &&
        GetSampleDurationTicks(static_cast<uint32_t>(currentIndex - 1), duration) == Status::OK &&
        inputPts < entry[currentIndex - 1].GetPts() + duration;
}

bool MPEG4SampleHelper::ShouldStepBackForBaseAudioCoverage(int64_t inputPts, SeekMode flag,
    int64_t currentIndex) const
{
    if (!IsAudioSampleIndexInRange(currentIndex)) {
        return false;
    }
    const auto& entry = sampleIndexEntry_;
    bool allowCoverageBackStep = flag == SeekMode::SEEK_CLOSEST_SYNC ||
        (flag == SeekMode::SEEK_NEXT_SYNC && static_cast<size_t>(currentIndex) == entry.size() - 1);
    return entry[currentIndex].GetPts() > inputPts && IsInputCoveredByPreviousSample(inputPts, currentIndex) &&
        allowCoverageBackStep;
}

bool MPEG4SampleHelper::ShouldStepBackForFirstNonBaseAudioSample(int64_t inputPts, bool isBaseTrack,
    int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    int64_t duration = 0;
    return !isBaseTrack && currentIndex == 1 && !entry.empty() &&
        GetSampleDurationTicks(0, duration) == Status::OK && inputPts <= entry[0].GetPts() + duration;
}

bool MPEG4SampleHelper::ShouldStepBackForSyncPrerollSample(int64_t inputPts, bool isBaseTrack,
    bool fromSyncSamples, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    return fromSyncSamples && !isBaseTrack && IsAudioSampleIndexInRange(currentIndex) &&
        entry[currentIndex].GetPts() >= inputPts && HasSkipSamplesInfo(static_cast<uint32_t>(currentIndex - 1));
}

bool MPEG4SampleHelper::ShouldStepBackForShiftedAudioCoverage(int64_t inputPts, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    return trackTimeScale_ > 0 && !entry.empty() && entry.front().GetPts() >= trackTimeScale_ &&
        IsAudioSampleIndexInRange(currentIndex) && inputPts < entry[currentIndex].GetPts() &&
        IsInputCoveredByPreviousSample(inputPts, currentIndex);
}

bool MPEG4SampleHelper::ShouldStepBackForNoSyncPrerollSample(int64_t inputPts, SeekMode flag, bool fromSyncSamples,
    int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    bool currentIndexIsLast = IsAudioSampleIndexInRange(currentIndex) &&
        static_cast<size_t>(currentIndex) == entry.size() - 1;
    bool shouldKeepPreroll = flag == SeekMode::SEEK_PREVIOUS_SYNC || flag == SeekMode::SEEK_CLOSEST_SYNC ||
        (flag == SeekMode::SEEK_NEXT_SYNC && currentIndexIsLast);
    return inputPts > 0 && !fromSyncSamples && shouldKeepPreroll &&
        IsAudioSampleIndexInRange(currentIndex) && entry[currentIndex].GetPts() >= inputPts &&
        entry[currentIndex - 1].GetPts() < inputPts && HasSkipSamplesInfo(static_cast<uint32_t>(currentIndex - 1));
}

bool MPEG4SampleHelper::ShouldStepBackForExactPtsPrerollSample(int64_t inputPts, bool isBaseTrack, SeekMode flag,
    bool fromSyncSamples, int64_t currentIndex) const
{
    const auto& entry = sampleIndexEntry_;
    return fromSyncSamples && !isBaseTrack && flag == SeekMode::SEEK_PREVIOUS_SYNC &&
        IsAudioSampleIndexInRange(currentIndex) && entry[currentIndex].GetPts() == inputPts &&
        entry[currentIndex - 1].GetPts() < inputPts && HasSkipSamplesInfo(static_cast<uint32_t>(currentIndex - 1));
}

void MPEG4SampleHelper::AdjustAudioSeekSampleIndexByPts(int64_t inputPts, bool isBaseTrack, SeekMode flag,
    bool fromSyncSamples, int64_t& currentIndex) const
{
    if (trackType_ != AUDIO_TYPE) {
        return;
    }
    // Only one back step is allowed; later checks must not re-evaluate a modified index.
    // Step back when the selected audio sample starts after inputPts but the previous sample still covers it.
    if (isBaseTrack && ShouldStepBackForBaseAudioCoverage(inputPts, flag, currentIndex)) {
        --currentIndex;
        return;
    }
    // Keep the first audio sample if it covers the sync point for a non-base track.
    if (ShouldStepBackForFirstNonBaseAudioSample(inputPts, isBaseTrack, currentIndex)) {
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
    if (ShouldStepBackForNoSyncPrerollSample(inputPts, flag, fromSyncSamples, currentIndex)) {
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
    FALSE_RETURN_V_NOLOG(!entry.empty(), Status::ERROR_INVALID_DATA);
    if (inputPts < 0) {
        sampleIndex = stssEntry_.empty() ? 0 : stssEntry_[0].index;
        return Status::OK;
    }
    if (trackType_ == AUDIO_TYPE && isBaseTrack && inputPts == 0 && entry.front().GetPts() < 0 &&
        !HasSkipSamplesInfo(0)) {
        sampleIndex = 0;
        return Status::OK;
    }
    if (trackType_ == AUDIO_TYPE && isBaseTrack && inputPts > lastPTS_) {
        return AssignSampleIndex(entry.size() - 1, sampleIndex) ? Status::OK : Status::ERROR_INVALID_DATA;
    }
    FALSE_RETURN_V_NOLOG(isBaseTrack || trackType_ != AUDIO_TYPE ||
        (flag != SeekMode::SEEK_NEXT_SYNC && flag != SeekMode::SEEK_CLOSEST_SYNC) ||
        !IsAtOrBeyondLastSampleEnd(inputPts),
        AssignSampleIndexFromSize(entry.size(), sampleIndex) ? Status::OK : Status::ERROR_INVALID_DATA);
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
    FALSE_RETURN_V_NOLOG(!entry.empty(), Status::ERROR_INVALID_DATA);
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
    uint32_t flags = GetU32Value(header, sizeof(header)) & 0x00FFFFFF;
    if ((version != 0 && version != 1) || flags != 0) {
        return Status::ERROR_INVALID_DATA;
    }

    uint32_t numSttsEntries = GetU32Value(&header[4]);
    FALSE_RETURN_V_MSG_E(numSttsEntries <= MAX_TRUN_SAMPLES, Status::ERROR_INVALID_DATA, "Invalid stts entry count");
    currentOffset += sizeof(uint64_t);

    if ((dataSize - sizeof(header)) / sizeof(SttsEntry) < numSttsEntries) {
        return Status::ERROR_UNKNOWN;
    }

    size_t size = 0;
    FALSE_RETURN_V_MSG_E(!__builtin_mul_overflow(numSttsEntries, sizeof(SttsEntry), &size),
                         Status::ERROR_INVALID_DATA, "numSttsEntries * sizeof(SttsEntry) overflow");
    auto data = std::make_unique<uint8_t[]>(size);
    ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    currentOffset += static_cast<int64_t>(size);
    sttsEntry_.reserve(numSttsEntries);

    for (uint32_t i = 0; i < numSttsEntries; ++i) {
        uint32_t sampleCount = GetU32Value(data.get() + i * sizeof(SttsEntry));
        FALSE_RETURN_V_MSG_E(sampleCount > 0 && sampleCount <= MAX_SAMPLE_TABLE_SAMPLES,
            Status::ERROR_INVALID_DATA, "Invalid stts sample count");
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
    FALSE_RETURN_V_NOLOG(currentOffset >= MIN_DATA_OFFSET && dataSize >= MIN_DATA_SIZE,
        Status::ERROR_INVALID_PARAMETER);

    uint8_t header[8];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    uint8_t version = header[0];
    uint32_t flags = GetU32Value(header, sizeof(header)) & 0x00FFFFFF;
    if ((version != 0 && version != 1) || flags != 0) {
        return Status::ERROR_INVALID_DATA;
    }

    uint32_t numCttsEntries = GetU32Value(&header[4]);
    FALSE_RETURN_V_MSG_E(numCttsEntries <= MAX_TRUN_SAMPLES, Status::ERROR_INVALID_DATA, "Invalid ctts entry count");
    currentOffset += sizeof(header);

    if ((dataSize - sizeof(header)) / sizeof(CttsEntry) < numCttsEntries) {
        return Status::ERROR_UNKNOWN;
    }

    size_t size = numCttsEntries * sizeof(CttsEntry);
    auto data = std::make_unique<uint8_t[]>(size);
    ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");
    currentOffset += static_cast<int64_t>(size);
    cttsEntry_.reserve(numCttsEntries);

    for (uint32_t i = 0; i < numCttsEntries; ++i) {
        uint32_t sampleCount = GetU32Value(data.get() + i * sizeof(CttsEntry));
        FALSE_RETURN_V_MSG_E(sampleCount > 0 && sampleCount <= MAX_SAMPLE_TABLE_SAMPLES,
            Status::ERROR_INVALID_DATA, "Invalid ctts sample count");
        uint32_t rawDelta = GetU32Value(data.get() + 4 + i * sizeof(CttsEntry));
        cttsEntry_.emplace_back(CttsEntry {
            .count = sampleCount,
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
    FALSE_RETURN_V_NOLOG(currentOffset >= MIN_DATA_OFFSET && dataSize >= MIN_DATA_SIZE,
        Status::ERROR_INVALID_PARAMETER);

    uint8_t header[8];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    if (GetU32Value(header, sizeof(header))) {
        return Status::ERROR_INVALID_DATA;
    }

    uint32_t numSyncs = GetU32Value(&header[4]);
    FALSE_RETURN_V_MSG_E(numSyncs <= MAX_TRUN_SAMPLES, Status::ERROR_INVALID_DATA, "Invalid sync sample count");
    currentOffset += sizeof(header);

    FALSE_RETURN_V_NOLOG((dataSize - sizeof(header)) / sizeof(uint32_t) >= numSyncs, Status::ERROR_UNKNOWN);
    
    size_t size = numSyncs * sizeof(uint32_t);
    auto data = std::make_unique<uint8_t[]>(size);
    ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    currentOffset += static_cast<int64_t>(size);
    stssEntry_.reserve(numSyncs);

    for (uint32_t i = 0; i < numSyncs; ++i) {
        uint32_t sampleIndex = GetU32Value(data.get() + i * sizeof(uint32_t));
        FALSE_RETURN_V_MSG_E(sampleIndex > 0, Status::ERROR_INVALID_DATA, "Invalid sample index");
        stssEntry_.emplace_back(StssEntry{
            .index = sampleIndex - 1,
        });
    }

    return Status::OK;
}

Status MPEG4SampleHelper::SetFieldSize(uint32_t atomType, int64_t &currentOffset, uint32_t &fieldSize)
{
    if (atomType == static_cast<uint32_t>(FourccType("stsz"))) {
        fieldSize = DEFAULT_FIELD_SIZE;
        uint8_t sizeBuffer[4];
        Status ret = dataReader_->ReadUintData(currentOffset, sizeBuffer, sizeof(sizeBuffer));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sizeBuffer failed");
        uint32_t defaultSampleSize = GetU32Value(sizeBuffer, sizeof(sizeBuffer));
        currentOffset += sizeof(uint32_t);
        if (defaultSampleSize) {
            FALSE_RETURN_V_MSG_E(defaultSampleSize <= SampleIndexEntry::sizeMask,
                Status::ERROR_INVALID_DATA, "Invalid default sample size");
            sampleSizes_.emplace_back(static_cast<int32_t>(defaultSampleSize));
            hasDefaultStszSize_ = true;
        }
    } else {
        uint8_t sizeBuffer[4];
        Status ret = dataReader_->ReadUintData(currentOffset, sizeBuffer, sizeof(sizeBuffer));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sizeBuffer failed");
        fieldSize = GetU32Value(sizeBuffer);
        currentOffset += sizeof(uint32_t);

        FALSE_RETURN_V_NOLOG(fieldSize == HALF_BYTE_BITS || fieldSize == BYTE_BITS || fieldSize == DUL_BYTE_BITS,
            Status::ERROR_INVALID_DATA);
    }
    return Status::OK;
}

Status MPEG4SampleHelper::ProcessSampleSizeData(uint32_t atomType, int64_t currentOffset, uint32_t size,
    uint32_t fieldSize, uint32_t numSampleSizes)
{
    auto data = std::make_unique<uint8_t[]>(size);
    Status ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sampleSize failed");
    currentOffset += static_cast<int64_t>(size);
    sampleSizes_.reserve(numSampleSizes);

    if (atomType == static_cast<uint32_t>(FourccType("stsz"))) {
        for (uint32_t i = 0; i < numSampleSizes; ++i) {
            uint32_t sampleSize = GetU32Value(data.get() + i * sizeof(uint32_t));
            FALSE_RETURN_V_MSG_E(sampleSize <= SampleIndexEntry::sizeMask,
                Status::ERROR_INVALID_DATA, "Invalid sample size");
            sampleSizes_.emplace_back(static_cast<int32_t>(sampleSize));
        }
    } else {
        // LCOV_EXCL_START
        uint32_t i = 0;
        while (i < numSampleSizes) {
            uint32_t sampleSize = 0;
            if (fieldSize == HALF_BYTE_BITS) {
                FALSE_BREAK_LOGD((i / HALF_BYTE_NUMBER) < size, "index > data size");
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
        // LCOV_EXCL_STOP
    }
    return ret;
}

void MPEG4SampleHelper::ProcessHalfByteSampleSizes(const uint8_t* data, uint32_t& entryIndex, uint32_t numSampleSizes)
{
    uint32_t index = entryIndex / 2;
    uint8_t sampleSize = data[index];
    sampleSizes_.emplace_back(sampleSize >> 0x4);
    if (entryIndex + 1 < numSampleSizes) {
        sampleSizes_.emplace_back(sampleSize - (sampleSize >> 0x4));
        ++entryIndex;
    }
}

Status MPEG4SampleHelper::SetSampleSizeParams(uint32_t atomType, int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(sampleSizes_.empty(), Status::ERROR_INVALID_OPERATION, "sampleSizes exist");
    FALSE_RETURN_V_NOLOG(atomType == static_cast<uint32_t>(FourccType("stsz")) ||
                         atomType == static_cast<uint32_t>(FourccType("stz2")), Status::ERROR_INVALID_PARAMETER);

    int64_t currentOffset = dataOffset;
    FALSE_RETURN_V_NOLOG(currentOffset >= MIN_DATA_OFFSET && dataSize >= MIN_DATA_SIZE,
        Status::ERROR_INVALID_PARAMETER);

    uint8_t header[4];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    FALSE_RETURN_V_NOLOG(!GetU32Value(header, sizeof(header)), Status::ERROR_INVALID_DATA);
    currentOffset += sizeof(uint32_t);

    uint32_t fieldSize;
    ret = SetFieldSize(atomType, currentOffset, fieldSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set fieldSize failed");

    FALSE_RETURN_V_NOLOG(dataSize >= STSZ_HEAD_LENGTH, Status::ERROR_INVALID_PARAMETER);
    uint8_t entryBuffer[4];
    ret = dataReader_->ReadUintData(currentOffset, entryBuffer, sizeof(entryBuffer));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read entryBuffer failed");
    uint32_t numSampleSizes = GetU32Value(entryBuffer);
    sampleSizeCount_ = numSampleSizes;
    currentOffset += sizeof(uint32_t);
    if (!numSampleSizes) {
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(numSampleSizes <= MAX_TRUN_SAMPLES, Status::ERROR_INVALID_DATA, "Invalid sample size count");
    if (hasDefaultStszSize_) {
        return Status::OK;
    }

    constexpr uint32_t maxUint32 = std::numeric_limits<uint32_t>::max();
    uint32_t unit = atomType == static_cast<uint32_t>(FourccType("stsz")) ? sizeof(uint32_t) : fieldSize;
    FALSE_RETURN_V_NOLOG(unit > 0 && numSampleSizes <= maxUint32 / unit, Status::ERROR_INVALID_PARAMETER);
    uint32_t size = atomType == static_cast<uint32_t>(FourccType("stsz"))
        ? numSampleSizes * sizeof(uint32_t)
        : (numSampleSizes * fieldSize + BYTE_BITS - 1) / BYTE_BITS;
    FALSE_RETURN_V_NOLOG(dataSize >= STSZ_HEAD_LENGTH && dataSize - STSZ_HEAD_LENGTH >= size, Status::ERROR_UNKNOWN);
    return ProcessSampleSizeData(atomType, currentOffset, size, fieldSize, numSampleSizes);
}

Status MPEG4SampleHelper::SetChunkOffsetParams(uint32_t atomType, int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(chunkOffsets_.empty(), Status::ERROR_INVALID_OPERATION, "chunkOffsets exist");
    FALSE_RETURN_V_NOLOG(atomType == static_cast<uint32_t>(FourccType("stco")) ||
                         atomType == static_cast<uint32_t>(FourccType("co64")), Status::ERROR_INVALID_PARAMETER);

    int64_t currentOffset = dataOffset;
    FALSE_RETURN_V_NOLOG(currentOffset >= MIN_DATA_OFFSET && dataSize >= MIN_DATA_SIZE,
        Status::ERROR_INVALID_PARAMETER);

    uint8_t header[4];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    FALSE_RETURN_V_NOLOG(!GetU32Value(header, sizeof(header)), Status::ERROR_INVALID_DATA);
    currentOffset += sizeof(uint32_t);

    uint8_t entryBuffer[4];
    ret = dataReader_->ReadUintData(currentOffset, entryBuffer, sizeof(entryBuffer));
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    uint32_t numChunkOffsets = GetU32Value(entryBuffer, sizeof(entryBuffer));
    FALSE_RETURN_V_MSG_E(numChunkOffsets <= MAX_TRUN_SAMPLES, Status::ERROR_INVALID_DATA, "Invalid chunk offset count");
    currentOffset += sizeof(uint32_t);
    uint32_t unit = atomType == static_cast<uint32_t>(FourccType("stco")) ? sizeof(uint32_t) : sizeof(uint64_t);
    FALSE_RETURN_V_NOLOG((dataSize - sizeof(uint64_t)) / unit >= numChunkOffsets, Status::ERROR_UNKNOWN);

    size_t size = 0;
    FALSE_RETURN_V_MSG_E(!__builtin_mul_overflow(numChunkOffsets, unit, &size),
                         Status::ERROR_INVALID_DATA, "Size overflow");
    auto data = std::make_unique<uint8_t[]>(size);
    ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    chunkOffsets_.reserve(numChunkOffsets);

    for (uint32_t i = 0; i < numChunkOffsets; ++i) {
        uint64_t offset = atomType == static_cast<uint32_t>(FourccType("stco"))
                              ? static_cast<uint64_t>(GetU32Value(data.get() + i * sizeof(uint32_t)))
                              : GetU64Value(data.get() + i * sizeof(uint64_t));
        FALSE_RETURN_V_MSG_E(offset <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()),
                             Status::ERROR_INVALID_DATA, "Invalid chunk offset");
        chunkOffsets_.emplace_back(static_cast<int64_t>(offset));
    }
    return Status::OK;
}

Status MPEG4SampleHelper::SetSampleToChunkParams(int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(stscEntry_.empty(), Status::ERROR_INVALID_OPERATION, "stscEntry exist");
    
    int64_t currentOffset = dataOffset;
    FALSE_RETURN_V_NOLOG(currentOffset >= MIN_DATA_OFFSET && dataSize >= MIN_DATA_SIZE,
        Status::ERROR_INVALID_PARAMETER);

    uint8_t header[8];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    FALSE_RETURN_V_NOLOG(!GetU32Value(header, sizeof(header)), Status::ERROR_INVALID_DATA);

    uint32_t numStscEntries = GetU32Value(&header[4]);
    FALSE_RETURN_V_MSG_E(numStscEntries <= MAX_TRUN_SAMPLES, Status::ERROR_INVALID_DATA, "Invalid stsc entry count");
    currentOffset += sizeof(header);

    FALSE_RETURN_V_NOLOG((dataSize - sizeof(header)) / sizeof(StscEntry) >= numStscEntries, Status::ERROR_UNKNOWN);

    size_t size = numStscEntries * sizeof(StscEntry);
    auto data = std::make_unique<uint8_t[]>(size);
    ret = dataReader_->ReadUintData(currentOffset, data.get(), size);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    currentOffset += static_cast<int64_t>(size);
    stscEntry_.reserve(numStscEntries);

    for (uint32_t i = 0; i < numStscEntries; ++i) {
        uint32_t tempFirst = GetU32Value(data.get() + i * sizeof(StscEntry));
        FALSE_RETURN_V_NOLOG(tempFirst > 0, Status::ERROR_INVALID_DATA);
        uint32_t sampleCount = GetU32Value(data.get() + sizeof(uint32_t) + i * sizeof(StscEntry));
        FALSE_RETURN_V_MSG_E(sampleCount > 0 && sampleCount <= MAX_SAMPLE_TABLE_SAMPLES,
            Status::ERROR_INVALID_DATA, "Invalid stsc sample count");

        stscEntry_.emplace_back(StscEntry{
            .first = tempFirst - 1,
            .count = sampleCount,
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
    FALSE_RETURN_V_MSG_W(ctxOffset >= currentOffset, Status::OK, "ctxOffset is invalid");
    size_t readableSize = std::min(dataSize, static_cast<size_t>(ctxOffset - currentOffset));
    FALSE_RETURN_V_MSG_W(readableSize >= versionAndFlagsOffset, Status::OK, "sdtp readable size is invalid");

    size_t entryCount = readableSize - versionAndFlagsOffset; // Each entry size is 1 Byte.
    FALSE_RETURN_V_NOLOG(entryCount != 0, Status::OK);
    FALSE_RETURN_V_MSG_E(entryCount <= std::numeric_limits<uint32_t>::max(), Status::ERROR_INVALID_DATA,
        "sdtp entry count is too large");
    sdtpEntry_.sdtpData.resize(entryCount);
    Status ret = dataReader_->ReadFully(currentOffset + versionAndFlagsOffset, sdtpEntry_.sdtpData.data(), entryCount);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sdtp data failed");
    sdtpEntry_.sdtpEntryCount = static_cast<uint32_t>(entryCount);
    return Status::OK;
}

// LCOV_EXCL_START
Status MPEG4SampleHelper::SetTrackExtendsParams(int64_t dataOffset, size_t dataSize)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(trex_.trackId == 0, Status::ERROR_INVALID_OPERATION, "trex exist");

    int64_t currentOffset = dataOffset;
    const size_t totalTrexSize = 24; // trex size is 24 bytes
    FALSE_RETURN_V_NOLOG(currentOffset >= MIN_DATA_OFFSET && dataSize >= totalTrexSize,
        Status::ERROR_INVALID_PARAMETER);

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
    FALSE_RETURN_V_NOLOG(currentOffset >= MIN_DATA_OFFSET && dataSize >= MIN_DATA_SIZE_INT,
        Status::ERROR_INVALID_PARAMETER);
    
    uint8_t header[4];
    auto ret = dataReader_->ReadUintData(currentOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    uint32_t version = header[0];
    currentOffset += sizeof(uint32_t);

    FALSE_RETURN_V_NOLOG((!version || dataSize - sizeof(header) >= sizeof(uint64_t)) &&
        (version || dataSize - sizeof(header) >= sizeof(uint32_t)), Status::ERROR_INVALID_DATA);

    if (version) {
        uint8_t data[8];
        ret = dataReader_->ReadUintData(currentOffset, data, sizeof(data));
        tfdtDts_ = static_cast<int64_t>(GetU64Value(data, sizeof(data)));
    } else {
        uint8_t data[4];
        ret = dataReader_->ReadUintData(currentOffset, data, sizeof(data));
        tfdtDts_ = GetU32Value(data, sizeof(data));
    }
    return ret;
}
// LCOV_EXCL_STOP

Status MPEG4SampleHelper::InitSampleInfo(SampleInfo& sampleInfo, TrunSampleInitContext& initContext)
{
    auto readUint32 = [&initContext](uint32_t& value) -> Status {
        size_t offset = static_cast<size_t>(initContext.bufferOffset);
        FALSE_RETURN_V_MSG_E(initContext.data != nullptr && offset <= initContext.dataSize &&
            initContext.dataSize - offset >= sizeof(uint32_t),
            Status::ERROR_INVALID_DATA, "trun optional field exceeds payload");
        value = GetU32Value(initContext.data + offset, initContext.dataSize - offset);
        initContext.bufferOffset += sizeof(uint32_t);
        return Status::OK;
    };
    if (initContext.trunFlag & TRUN_DATA_OFFSET) {
        Status ret = readUint32(sampleInfo.dataOffset);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    }
    if (initContext.trunFlag & TRUN_FIRST_SAMPLE_FLAGS) {
        Status ret = readUint32(initContext.firstSampleFlag);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    }
    if (initContext.trunFlag & TRUN_SAMPLE_DURATION) {
        sampleInfo.byteLength += sizeof(uint32_t);
    } else if (tfhd_.defaultSampleDuration) {
        sampleInfo.duration = tfhd_.defaultSampleDuration;
    } else {
        return Status::ERROR_INVALID_DATA;
    }
    if (initContext.trunFlag & TRUN_SAMPLE_SIZE) {
        sampleInfo.byteLength += sizeof(uint32_t);
    } else if (tfhd_.defaultSampleSize) {
        sampleInfo.size = tfhd_.defaultSampleSize;
    } else {
        return Status::ERROR_INVALID_DATA;
    }
    if (initContext.trunFlag & TRUN_SAMPLE_FLAGS) {
        sampleInfo.byteLength += sizeof(uint32_t);
    } else if (tfhd_.flags) {
        sampleInfo.flag = tfhd_.flags;
    } else {
        return Status::ERROR_INVALID_DATA;
    }
    if (initContext.trunFlag & TRUN_SAMPLE_CTS) {
        sampleInfo.byteLength += sizeof(uint32_t);
    }
    return Status::OK;
}

// LCOV_EXCL_START
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
// LCOV_EXCL_STOP

void MPEG4SampleHelper::InitMoofParam()
{
    if (firstMoofSampleIndex_ == -1) {
        firstMoofSampleIndex_ = static_cast<int64_t>(sampleIndexEntry_.size());
        firstMoofSttsIndex_ = static_cast<int64_t>(sttsEntry_.size());
        numMoovSamples_ = sampleIndexEntry_.size();
    }
}

// LCOV_EXCL_START
Status MPEG4SampleHelper::SetEntryPts(SampleIndexEntry& entry, int64_t pts) const
{
    return entry.SetPts(pts) ? Status::OK : Status::ERROR_INVALID_DATA;
}

Status MPEG4SampleHelper::SetEntrySize(SampleIndexEntry& entry, uint32_t size) const
{
    return entry.SetSize(size) ? Status::OK : Status::ERROR_INVALID_DATA;
}

void MPEG4SampleHelper::SetEntryDiscard(SampleIndexEntry& entry, bool discard) const
{
    entry.SetDiscard(discard);
}

const MPEG4SampleHelper::DurationOverride* MPEG4SampleHelper::FindDurationOverride(
    const std::vector<DurationOverride>& overrides, uint32_t sampleIndex) const
{
    auto it = std::lower_bound(overrides.begin(), overrides.end(), sampleIndex,
        [](const DurationOverride& item, uint32_t index) { return item.sampleIndex < index; });
    return it != overrides.end() && it->sampleIndex == sampleIndex ? &(*it) : nullptr;
}

Status MPEG4SampleHelper::AddDurationOverride(std::vector<DurationOverride>& overrides,
    uint32_t sampleIndex, int64_t duration) const
{
    FALSE_RETURN_V_NOLOG(duration >= 0, Status::ERROR_INVALID_DATA);
    auto it = std::lower_bound(overrides.begin(), overrides.end(), sampleIndex,
        [](const DurationOverride& item, uint32_t index) { return item.sampleIndex < index; });
    if (it != overrides.end() && it->sampleIndex == sampleIndex) {
        it->duration = duration;
    } else {
        overrides.insert(it, DurationOverride {sampleIndex, duration});
    }
    return Status::OK;
}

Status MPEG4SampleHelper::AddDurationOverride(uint32_t sampleIndex, int64_t duration)
{
    return AddDurationOverride(durationOverrides_, sampleIndex, duration);
}

Status MPEG4SampleHelper::GetSampleDurationTicks(const std::vector<SampleIndexEntry>& entries,
    const std::vector<DurationOverride>& overrides, uint32_t sampleIndex, int64_t& duration) const
{
    FALSE_RETURN_V_NOLOG(sampleIndex < entries.size(), Status::ERROR_INVALID_PARAMETER);
    const DurationOverride* item = FindDurationOverride(overrides, sampleIndex);
    if (item != nullptr) {
        duration = item->duration;
        return Status::OK;
    }
    FALSE_RETURN_V_NOLOG(sampleIndex + 1 < entries.size(), Status::ERROR_INVALID_DATA);
    FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(entries[sampleIndex + 1].dts, entries[sampleIndex].dts, &duration) &&
        duration > 0, Status::ERROR_INVALID_DATA);
    return Status::OK;
}

Status MPEG4SampleHelper::GetSampleDurationTicks(uint32_t sampleIndex, int64_t& duration) const
{
    if (sampleIndexState_.load(std::memory_order_acquire) != SampleIndexState::DENSE && sampleIndexEntry_.empty()) {
        SampleIndexView view;
        Status ret = GetSparseSampleIndexView(sampleIndex, view);
        if (ret != Status::OK) {
            return ret;
        }
        duration = view.duration;
        return Status::OK;
    }
    return GetSampleDurationTicks(sampleIndexEntry_, durationOverrides_, sampleIndex, duration);
}

void MPEG4SampleHelper::ShiftDurationOverridesForInsert(uint32_t sampleIndex)
{
    for (auto& item : durationOverrides_) {
        if (item.sampleIndex >= sampleIndex) {
            ++item.sampleIndex;
        }
    }
}

void MPEG4SampleHelper::ConvertAllSyncSamplesToSparse(uint32_t sampleIndex)
{
    allSamplesSync_ = false;
    stssEntry_.reserve(sampleIndexEntry_.size() - 1);
    for (uint32_t i = 0; i < sampleIndexEntry_.size(); ++i) {
        if (i != sampleIndex) {
            stssEntry_.emplace_back(StssEntry {.index = i});
        }
    }
}

void MPEG4SampleHelper::ShiftSyncSampleIndices(uint32_t sampleIndex)
{
    for (auto& entry : stssEntry_) {
        if (entry.index >= sampleIndex) {
            ++entry.index;
        }
    }
}

void MPEG4SampleHelper::UpdateSyncSamplesForInsert(uint32_t sampleIndex, bool isSync)
{
    if (allSamplesSync_) {
        if (!isSync) {
            ConvertAllSyncSamplesToSparse(sampleIndex);
        }
        return;
    }
    ShiftSyncSampleIndices(sampleIndex);
    if (!isSync) {
        return;
    }
    auto it = std::lower_bound(stssEntry_.begin(), stssEntry_.end(), sampleIndex,
        [](const StssEntry& entry, uint32_t index) { return entry.index < index; });
    stssEntry_.insert(it, StssEntry {.index = sampleIndex});
}

Status MPEG4SampleHelper::CreateSampleEntry(
    const SampleEntryCreateContext& context, SampleIndexEntry& sample, bool& isSync) const
{
    const auto& sampleInfo = context.sampleInfo;
    FALSE_RETURN_V_NOLOG(context.offset >= 0, Status::ERROR_INVALID_DATA);
    sample = SampleIndexEntry {};
    sample.pos = context.offset;
    sample.dts = context.currentDts;
    FALSE_RETURN_V_NOLOG(SetEntrySize(sample, sampleInfo.size) == Status::OK, Status::ERROR_INVALID_DATA);
    int64_t pts = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(context.currentDts,
        static_cast<int64_t>(sampleInfo.ctsDelta), &pts), Status::ERROR_INVALID_DATA);
    FALSE_RETURN_V_NOLOG(SetEntryPts(sample, pts) == Status::OK, Status::ERROR_INVALID_DATA);
    isSync = trackType_ == AUDIO_TYPE || (sampleInfo.flag & (0x00010000 | 0x01000000)) == 0;
    SetEntryDiscard(sample, context.isOverlap);
    return Status::OK;
}

bool MPEG4SampleHelper::InsertSampleEntry(SampleIndexEntry&& sample, uint32_t duration, bool isSync)
{
    uint32_t sampleIndex = 0;
    auto it = std::upper_bound(sampleIndexEntry_.begin(), sampleIndexEntry_.end(), sample,
        [](const SampleIndexEntry& a, const SampleIndexEntry& b) { return a.pos < b.pos; });
    FALSE_RETURN_V_MSG_E(AssignSampleIndex(std::distance(sampleIndexEntry_.begin(), it), sampleIndex), false,
        "Invalid sample index");

    const int64_t sttsIndex = static_cast<int64_t>(sampleIndex) - static_cast<int64_t>(numMoovSamples_) +
        firstMoofSttsIndex_ - 1;
    FALSE_RETURN_V_MSG_E(sttsIndex >= 0 && static_cast<uint64_t>(sttsIndex) <= static_cast<uint64_t>(sttsEntry_.size()),
        false, "Invalid stts index");

    int64_t samplePts = 0;
    FALSE_RETURN_V_NOLOG(sample.GetPts(samplePts), false);
    int64_t previousDuration = 0;
    bool hasPrevious = sampleIndex > 0 && GetSampleDurationTicks(sampleIndex - 1, previousDuration) == Status::OK;
    lastPTS_ = std::max(lastPTS_, samplePts);
    int64_t newRawTotalSize = 0;
    FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(rawTotalSize_, static_cast<int64_t>(sample.GetSize()),
        &newRawTotalSize), false, "rawTotalSize_ overflow");
    rawTotalSize_ = newRawTotalSize;
    ShiftDurationOverridesForInsert(sampleIndex);
    sampleIndexEntry_.insert(sampleIndexEntry_.begin() + sampleIndex, std::move(sample));
    UpdateSyncSamplesForInsert(sampleIndex, isSync);
    if (hasPrevious) {
        auto durationIt = std::lower_bound(durationOverrides_.begin(), durationOverrides_.end(), sampleIndex - 1,
            [](const DurationOverride& item, uint32_t index) { return item.sampleIndex < index; });
        if (durationIt != durationOverrides_.end() && durationIt->sampleIndex == sampleIndex - 1) {
            durationOverrides_.erase(durationIt);
        }
        int64_t derived = sampleIndexEntry_[sampleIndex].dts - sampleIndexEntry_[sampleIndex - 1].dts;
        if (derived != previousDuration) {
            FALSE_RETURN_V_NOLOG(AddDurationOverride(sampleIndex - 1, previousDuration) == Status::OK, false);
        }
    }
    FALSE_RETURN_V_NOLOG(AddDurationOverride(sampleIndex, duration) == Status::OK, false);
    const int64_t insertedDts = sampleIndexEntry_[sampleIndex].dts;
    for (size_t i = static_cast<size_t>(sampleIndex) + 1; i < sampleIndexEntry_.size(); ++i) {
        if (insertedDts < sampleIndexEntry_[i].dts) {
            break;
        }
        SetEntryDiscard(sampleIndexEntry_[i], true);
    }

    sttsEntry_.insert(sttsEntry_.begin() + static_cast<size_t>(sttsIndex), SttsEntry {
        .count = 1,
        .delta = duration
    });
    return true;
}

Status MPEG4SampleHelper::UpdateSampleEntryInfo(
    SampleInfo sampleInfo, int64_t& offset, int64_t& currentDts, bool isOverlap)
{
    SampleIndexEntry sample;
    bool isSync = false;
    SampleEntryCreateContext context {sampleInfo, offset, currentDts, isOverlap};
    FALSE_RETURN_V_NOLOG(CreateSampleEntry(context, sample, isSync) == Status::OK, Status::ERROR_INVALID_DATA);
    ++rawSampleCount_;
    if (sampleIndexEntry_.empty()) {
        FALSE_RETURN_V_NOLOG(sample.GetPts(lastPTS_), Status::ERROR_INVALID_DATA);
        rawTotalSize_ += sample.GetSize();
        sampleIndexEntry_.emplace_back(std::move(sample));
        allSamplesSync_ = isSync;
        if (!isSync) {
            stssEntry_.clear();
        }
        FALSE_RETURN_V_NOLOG(AddDurationOverride(0, sampleInfo.duration) == Status::OK,
            Status::ERROR_INVALID_DATA);
        sttsEntry_.emplace_back(SttsEntry {
            .count = 1,
            .delta = sampleInfo.duration
        });
        return Status::OK;
    }
    return InsertSampleEntry(std::move(sample), sampleInfo.duration, isSync) ? Status::OK : Status::ERROR_INVALID_DATA;
}
// LCOV_EXCL_STOP

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
    FALSE_RETURN_V_NOLOG(!(trunFlag & TRUN_FIRST_SAMPLE_FLAGS) || !(trunFlag & TRUN_SAMPLE_FLAGS),
        Status::ERROR_INVALID_DATA);

    numSamples = GetU32Value(&header[trunSampleCountOffset]);
    FALSE_RETURN_V_MSG_E(dataSize > sizeof(header) &&
        static_cast<uint64_t>(dataSize - sizeof(header)) <= static_cast<uint64_t>(std::numeric_limits<size_t>::max()),
        Status::ERROR_INVALID_BUFFER_SIZE, "Invalid trun payload size");
    size_t payloadSize = static_cast<size_t>(dataSize - sizeof(header));
    data = std::make_unique<uint8_t[]>(payloadSize);
    ret = dataReader_->ReadUintData(dataOffset + sizeof(uint64_t), data.get(), payloadSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");
    return Status::OK;
}

bool MPEG4SampleHelper::IsValidTrunSampleData(int64_t dataSize, uint32_t bufferOffset, uint32_t byteLength,
    uint32_t numSamples) const
{
    if (byteLength != 0) {
        constexpr int64_t trunHeaderSize = sizeof(uint64_t);
        FALSE_RETURN_V_NOLOG(dataSize >= trunHeaderSize, false);
        int64_t dataBytes = dataSize - trunHeaderSize;
        int64_t safeBufferOffset = static_cast<int64_t>(bufferOffset);
        FALSE_RETURN_V_NOLOG(dataBytes >= safeBufferOffset, false);
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
    size_t payloadSize = static_cast<size_t>(dataSize - sizeof(uint64_t));
    TrunSampleInitContext initContext = {trunFlag, data.get(), payloadSize, 0, 0};
    ret = InitSampleInfo(sampleInfo, initContext);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);

    FALSE_RETURN_V_NOLOG(IsValidTrunSampleData(dataSize, initContext.bufferOffset,
        sampleInfo.byteLength, numSamples), Status::ERROR_INVALID_DATA);

    FALSE_RETURN_V_MSG_E(tfhd_.baseDataOffset <= std::numeric_limits<int64_t>::max() -
        static_cast<int64_t>(sampleInfo.dataOffset), Status::ERROR_INVALID_DATA, "Invalid trun data offset");
    int64_t offset = tfhd_.baseDataOffset + static_cast<int64_t>(sampleInfo.dataOffset);
    int64_t currentDts = (tfdtDts_ != -1) ? tfdtDts_ : (sidxDts != -1) ? sidxDts : baseFragmentDts_;
    moofDts = currentDts;
    InitMoofParam();
    int64_t prevDts = (sampleIndexEntry_.size() > 0) ? sampleIndexEntry_.back().dts : INT64_MIN;
    for (uint32_t i = 0; i < numSamples; ++i) {
        sampleInfo.index = i;
        UpdateSampleInfo(sampleInfo, initContext.bufferOffset, trunFlag, data.get(), initContext.firstSampleFlag);
        bool isOverlap = (prevDts != INT64_MIN && prevDts >= currentDts);
        FALSE_RETURN_V_MSG_E(UpdateSampleEntryInfo(sampleInfo, offset, currentDts, isOverlap) == Status::OK,
            Status::ERROR_INVALID_DATA, "Update sample entry failed");
        FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(duration, static_cast<int64_t>(sampleInfo.duration), &duration),
            Status::ERROR_INVALID_DATA, "Fragment duration overflow");
        prevDts = currentDts;
        FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(currentDts, static_cast<int64_t>(sampleInfo.duration),
            &currentDts), Status::ERROR_INVALID_DATA, "Fragment dts overflow");
        FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(offset, static_cast<int64_t>(sampleInfo.size), &offset),
            Status::ERROR_INVALID_DATA, "Fragment offset overflow");
        FALSE_RETURN_V_MSG_E(duration >= 0 && offset >= 0, Status::ERROR_INVALID_DATA,
            "Duration DTS or Offset is negative");
    }
    totalTrunDuration_ = currentDts;
    baseFragmentDts_ = currentDts;
    totalDuration_ = (totalDuration_ < baseFragmentDts_) ? baseFragmentDts_ : totalDuration_;
    rawTotalDuration_ = std::max(rawTotalDuration_, totalDuration_);
    return Status::OK;
}

Status MPEG4SampleHelper::ValidateCttsEntries(uint32_t sttsNum)
{
    uint32_t cttsNum = 0;
    for (const auto& entry : cttsEntry_) {
        uint32_t temp;
        if (entry.count == 0 || entry.count > MAX_SAMPLE_TABLE_SAMPLES ||
            __builtin_add_overflow(cttsNum, entry.count, &temp) || temp > MAX_SAMPLE_TABLE_SAMPLES) {
            MEDIA_LOG_E("CTTS count overflow: current=" PUBLIC_LOG_U32 ", add="
                PUBLIC_LOG_U32, cttsNum, entry.count);
            return Status::ERROR_INVALID_DATA;
        }
        cttsNum = temp;
    }
    FALSE_RETURN_V_MSG_E(sttsNum == cttsNum, Status::ERROR_INVALID_DATA, "ctts did not match stts");
    return Status::OK;
}

Status MPEG4SampleHelper::PreBuildProcessAndCheck()
{
    if (isFragmented_) {
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(!sttsEntry_.empty(), Status::ERROR_INVALID_DATA, "stts corrupted");
    FALSE_RETURN_V_MSG_E(!sampleSizes_.empty(), Status::ERROR_INVALID_DATA, "stsz corrupted");
    FALSE_RETURN_V_MSG_E(!chunkOffsets_.empty(), Status::ERROR_INVALID_DATA, "stco corrupted");
    FALSE_RETURN_V_MSG_E(!stscEntry_.empty(), Status::ERROR_INVALID_DATA, "stsc corrupted");

    uint32_t sttsNum = 0;
    for (const auto& entry : sttsEntry_) {
        uint32_t temp = 0;
        if (entry.count == 0 || entry.count > MAX_SAMPLE_TABLE_SAMPLES ||
            __builtin_add_overflow(sttsNum, entry.count, &temp) || temp > MAX_SAMPLE_TABLE_SAMPLES) {
            MEDIA_LOG_E("STTS count overflow: current=" PUBLIC_LOG_U32 ", add="
                PUBLIC_LOG_U32, sttsNum, entry.count);
            return Status::ERROR_INVALID_DATA;
        }
        sttsNum = temp;
    }
    FALSE_RETURN_V_MSG_E(sttsNum > 0, Status::ERROR_INVALID_DATA, "Invalid stts sample count");

    if (!hasDefaultStszSize_) {
        FALSE_RETURN_V_MSG_E(sampleSizes_.size() == sampleSizeCount_, Status::ERROR_INVALID_DATA,
            "Invalid stsz sample size count");
        FALSE_RETURN_V_MSG_E(sttsNum <= sampleSizes_.size(), Status::ERROR_INVALID_DATA,
            "stsz sample size count is less than stts");
    } else if (sampleSizeCount_ != sttsNum) {
        MEDIA_LOG_W("stsz sample count " PUBLIC_LOG_U32 " does not match stts sample count " PUBLIC_LOG_U32,
            sampleSizeCount_, sttsNum);
    }
    for (uint32_t i = 0; i < stscEntry_.size(); ++i) {
        const auto& entry = stscEntry_[i];
        FALSE_RETURN_V_MSG_E(entry.count > 0 && entry.count <= MAX_SAMPLE_TABLE_SAMPLES, Status::ERROR_INVALID_DATA,
            "Invalid stsc sample count");
        FALSE_RETURN_V_MSG_E(entry.first < chunkOffsets_.size(), Status::ERROR_INVALID_DATA,
            "Invalid stsc first chunk");
        if (i > 0) {
            FALSE_RETURN_V_MSG_E(entry.first > stscEntry_[i - 1].first, Status::ERROR_INVALID_DATA,
                "Invalid stsc first chunk order");
        }
    }

    if (!cttsEntry_.empty()) {
        Status ret = ValidateCttsEntries(sttsNum);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ValidateCttsEntries failed");
        CalculateDtsShift();
    }

    allSamplesSync_ = stssEntry_.empty() &&
        (trackType_ == VIDEO_TYPE || trackType_ == AUDIO_TYPE || trackType_ == TIMEDMETA_TYPE ||
        trackType_ == AUXILIARY_TYPE || (trackType_ == SUBTITLE_TYPE && mimeType_ == MimeType::TEXT_WEBVTT));
    return Status::OK;
}


bool MPEG4SampleHelper::GetSampleSize(uint32_t sampleIndex, uint32_t& samplesPerFrame, SampleIndexEntry& entry,
    int64_t& remainSamples)
{
    if (mimeType_ == MimeType::AUDIO_G711MU || mimeType_ == MimeType::AUDIO_G711A ||
        mimeType_ == MimeType::AUDIO_RAW) {
        int32_t sampleSize = sampleSizes_[0];
        FALSE_RETURN_V_NOLOG(sampleSize <= INT32_MAX / MAX_SAMPLES_PER_FRAME, false);
        samplesPerFrame = std::min(MAX_SAMPLES_PER_FRAME, remainSamples);
        remainSamples -= static_cast<int64_t>(samplesPerFrame);
        FALSE_RETURN_V_NOLOG(SetEntrySize(entry, samplesPerFrame * static_cast<uint32_t>(sampleSize)) == Status::OK,
            false);
    } else {
        int32_t sampleSize = hasDefaultStszSize_ ? sampleSizes_[0] : sampleSizes_[sampleIndex];
        FALSE_RETURN_V_NOLOG(sampleSize >= 0 && SetEntrySize(entry, static_cast<uint32_t>(sampleSize)) == Status::OK,
            false);
    }
    return true;
}

Status MPEG4SampleHelper::UpdateSampleDurationAndDts(uint32_t sttsDelta, uint32_t samplesPerFrame,
    int64_t& duration, int64_t& currentDts) const
{
    FALSE_RETURN_V_MSG_E(samplesPerFrame > 0, Status::ERROR_INVALID_DATA, "Invalid samples per frame");
    int64_t delta = static_cast<int64_t>(sttsDelta);
    int64_t frameCount = static_cast<int64_t>(samplesPerFrame);
    FALSE_RETURN_V_MSG_E(delta <= std::numeric_limits<int64_t>::max() / frameCount,
        Status::ERROR_INVALID_DATA, "Sample duration is too large");
    duration = delta * frameCount;
    FALSE_RETURN_V_MSG_E(currentDts <= std::numeric_limits<int64_t>::max() - duration,
        Status::ERROR_INVALID_DATA, "Sample dts is too large");
    currentDts += duration;
    return Status::OK;
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

bool MPEG4SampleHelper::IsSyncSample(const std::vector<StssEntry>& syncSamples, bool allSamplesSync,
    uint32_t sampleIndex) const
{
    if (allSamplesSync) {
        return true;
    }
    auto it = std::lower_bound(syncSamples.begin(), syncSamples.end(), sampleIndex,
        [](const StssEntry& entry, uint32_t index) { return entry.index < index; });
    return it != syncSamples.end() && it->index == sampleIndex;
}

int64_t MPEG4SampleHelper::GetFrameDuration(const std::vector<SampleIndexEntry>& entries,
    const std::vector<DurationOverride>& durationOverrides, size_t index, int64_t editDuration) const
{
    int64_t duration = 0;
    return GetSampleDurationTicks(entries, durationOverrides, static_cast<uint32_t>(index), duration) == Status::OK ?
        duration : editDuration;
}

int64_t MPEG4SampleHelper::FindPrevClosestIndex(const std::vector<SampleIndexEntry>& entries,
    const std::vector<StssEntry>& syncSamples, bool allSamplesSync, int64_t timestampPts, bool anyFrame) const
{
    if (dtsShift_ > 0) {
        timestampPts -= dtsShift_;
    }
    int64_t found = -1;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].dts > timestampPts) {
            break;
        }
        if (anyFrame || IsSyncSample(syncSamples, allSamplesSync, static_cast<uint32_t>(i))) {
            found = static_cast<int64_t>(i);
        }
    }
    while (!cttsEntry_.empty() && found >= 0) {
        const auto& entry = entries[found];
        int64_t rawPts = 0;
        if (!__builtin_sub_overflow(entry.GetPts(), dtsShift_, &rawPts) && rawPts <= timestampPts &&
            IsSyncSample(syncSamples, allSamplesSync, static_cast<uint32_t>(found))) {
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
        entry.dts = endDts;
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
    int64_t startIndex = FindPrevClosestIndex(buildCtx.oldEntries, buildCtx.oldSyncSamples,
        buildCtx.oldAllSamplesSync, searchTimestamp, false);
    if (startIndex < 0) {
        MEDIA_LOG_W("Missing key frame while searching edit timestamp: " PUBLIC_LOG_D64, searchTimestamp);
        startIndex = FindPrevClosestIndex(buildCtx.oldEntries, buildCtx.oldSyncSamples,
            buildCtx.oldAllSamplesSync, searchTimestamp, true);
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
    processCtx.frameDuration = GetFrameDuration(buildCtx.oldEntries, buildCtx.oldDurationOverrides,
        index, processCtx.editDuration);
    int64_t currCtts = static_cast<int64_t>(oldEntry.ptsOffset) - dtsShift_;
    int64_t currCts = 0;
    int64_t shiftedDts = 0;
    if (__builtin_add_overflow(oldEntry.dts, dtsShift_, &shiftedDts) ||
        __builtin_add_overflow(shiftedDts, currCtts, &currCts)) {
        return Status::ERROR_INVALID_DATA;
    }
    if (__builtin_add_overflow(currCts, processCtx.frameDuration, &processCtx.frameEndTime)) {
        return Status::ERROR_INVALID_DATA;
    }

    processCtx.newEntry = oldEntry;
    processCtx.newEntry.dts = processCtx.dtsCounter;
    processCtx.skipSamplesInfo.clear();
    bool outsideEdit = currCts < edit.mediaTime || currCts >= processCtx.editEndTime;
    bool crossedAudioStart = trackType_ == AUDIO_TYPE &&
        currCts < edit.mediaTime && processCtx.frameEndTime >= edit.mediaTime && buildCtx.firstNonZeroAudioEdit > 0;
    if (outsideEdit && crossedAudioStart) {
        int64_t packetSkipSamples = edit.mediaTime - currCts;
        processCtx.skipSamplesInfo = MakeSkipSamplesInfo(packetSkipSamples);
        SetEntryDiscard(processCtx.newEntry, true);
        processCtx.dtsCounter -= packetSkipSamples;
        processCtx.newEntry.dts = processCtx.dtsCounter;
    } else if (outsideEdit) {
        SetEntryDiscard(processCtx.newEntry, true);
        if (!processCtx.editListStartEncountered) {
            processCtx.leadingDurations.emplace_back(processCtx.frameDuration);
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
    processCtx.newEntryIsSync = IsSyncSample(buildCtx.oldSyncSamples, buildCtx.oldAllSamplesSync,
        static_cast<uint32_t>(index));
    return Status::OK;
}

Status MPEG4SampleHelper::AppendEditListSample(const SampleIndexEntry& oldEntry, size_t index,
    EditListBuildContext& buildCtx, EditListProcessContext& processCtx)
{
    Status ret = BuildEditListSampleEntry(oldEntry, index, buildCtx, processCtx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Build edit list sample entry failed");
    buildCtx.newEntries.emplace_back(std::move(processCtx.newEntry));
    uint32_t newSampleIndex = static_cast<uint32_t>(buildCtx.newEntries.size() - 1);
    buildCtx.newDurations.emplace_back(processCtx.frameDuration);
    if (!buildCtx.oldAllSamplesSync && processCtx.newEntryIsSync) {
        buildCtx.newSyncSamples.emplace_back(StssEntry {.index = newSampleIndex});
    }
    if (!processCtx.skipSamplesInfo.empty()) {
        buildCtx.newSkipSamplesInfo[newSampleIndex] = std::move(processCtx.skipSamplesInfo);
    }
    if (processCtx.editListStartEncountered) {
        int64_t newDtsCounter = 0;
        FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(processCtx.dtsCounter, processCtx.frameDuration,
            &newDtsCounter), Status::ERROR_INVALID_DATA, "DtsCounter + duration overflow");
        processCtx.dtsCounter = newDtsCounter;
    }
    if (processCtx.frameEndTime >= processCtx.editEndTime &&
        (processCtx.newEntryIsSync || trackType_ == AUDIO_TYPE)) {
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
            }
        }
    }

    sampleIndexEntry_ = std::move(buildCtx.newEntries);
    durationOverrides_.clear();
    for (uint32_t i = 0; i < sampleIndexEntry_.size(); ++i) {
        int64_t derivedDuration = 0;
        bool canDerive = i + 1 < sampleIndexEntry_.size() &&
            !__builtin_sub_overflow(sampleIndexEntry_[i + 1].dts, sampleIndexEntry_[i].dts, &derivedDuration) &&
            derivedDuration > 0 && derivedDuration == buildCtx.newDurations[i];
        if (!canDerive) {
            (void)AddDurationOverride(i, buildCtx.newDurations[i]);
        }
    }
    allSamplesSync_ = buildCtx.oldAllSamplesSync;
    stssEntry_ = std::move(buildCtx.newSyncSamples);
    skipSamplesInfoMap_.clear();
    for (auto& skipSamplesInfo : buildCtx.newSkipSamplesInfo) {
        SetSkipSamplesInfo(skipSamplesInfo.first, std::move(skipSamplesInfo.second));
    }
    totalDuration_ = buildCtx.editListDtsEntryEnd - buildCtx.startDts;
    lastPTS_ = 0;
    for (const auto& entry : sampleIndexEntry_) {
        lastPTS_ = std::max(lastPTS_, entry.GetPts());
    }
    editListApplied_ = true;
}

MPEG4SampleHelper::EditListRestoreGuard::~EditListRestoreGuard()
{
    if (!active) {
        return;
    }
    entries = std::move(oldEntries);
    overrides = std::move(oldOverrides);
    syncSamples = std::move(oldSyncSamples);
    allSamplesSync = oldAllSamplesSync;
}

Status MPEG4SampleHelper::ApplyEditList()
{
    if (editListEntries_.empty() || sampleIndexEntry_.empty()) {
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(movieTimeScale_ > 0 && trackTimeScale_ > 0, Status::ERROR_INVALID_DATA,
        "Invalid edit list time scale");

    auto oldEntries = std::move(sampleIndexEntry_);
    auto oldDurationOverrides = std::move(durationOverrides_);
    auto oldSyncSamples = std::move(stssEntry_);
    bool oldAllSamplesSync = allSamplesSync_;
    std::vector<SampleIndexEntry> newEntries;
    newEntries.reserve(oldEntries.size());
    std::vector<int64_t> newDurations;
    newDurations.reserve(oldEntries.size());
    std::vector<StssEntry> newSyncSamples;
    newSyncSamples.reserve(oldSyncSamples.size());
    SkipSamplesInfoMap newSkipSamplesInfo;

    EditListRestoreGuard restoreGuard {sampleIndexEntry_, durationOverrides_, stssEntry_, allSamplesSync_, oldEntries,
        oldDurationOverrides, oldSyncSamples, oldAllSamplesSync};

    int64_t editListDtsEntryEnd = dtsShift_ > 0 ? -dtsShift_ : 0;
    int64_t startDts = editListDtsEntryEnd;
    int64_t emptyEditsDuration = 0;
    int64_t minCorrectedPts = -1;
    bool foundNonEmptyEdit = false;
    int32_t firstNonZeroAudioEdit = -1;
    EditListBuildContext buildCtx {
        oldEntries, oldDurationOverrides, oldSyncSamples, oldAllSamplesSync, newEntries, newDurations,
        newSyncSamples, newSkipSamplesInfo, editListDtsEntryEnd, emptyEditsDuration, minCorrectedPts,
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

    FALSE_RETURN_V_NOLOG(!newEntries.empty(), Status::OK);
    FinalizeAppliedEditList(buildCtx);
    restoreGuard.active = false;
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
    if (!cttsEntry_.empty()) {
        buildCtx.cttsSamplesRemaining = cttsEntry_[0].count;
        buildCtx.cttsDelta = cttsEntry_[0].delta;
    }
    return buildCtx;
}

Status MPEG4SampleHelper::ConsumeCttsSamples(SampleIndexBuildContext& buildCtx, uint32_t sampleCount,
    int64_t& ptsOffset)
{
    if (buildCtx.cttsSamplesRemaining == 0) {
        ++buildCtx.cttsIndex;
        FALSE_RETURN_V_NOLOG(buildCtx.cttsIndex < cttsEntry_.size(), Status::ERROR_INVALID_DATA);
        buildCtx.cttsSamplesRemaining = cttsEntry_[buildCtx.cttsIndex].count;
        buildCtx.cttsDelta = cttsEntry_[buildCtx.cttsIndex].delta;
    }
    int64_t offset = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(dtsShift_, static_cast<int64_t>(buildCtx.cttsDelta), &offset) &&
        offset >= 0 && static_cast<uint64_t>(offset) <= std::numeric_limits<uint32_t>::max(),
        Status::ERROR_INVALID_DATA);
    ptsOffset = offset;
    uint32_t remaining = sampleCount;
    while (remaining > 0 && !cttsEntry_.empty()) {
        if (buildCtx.cttsSamplesRemaining == 0) {
            ++buildCtx.cttsIndex;
            FALSE_RETURN_V_NOLOG(buildCtx.cttsIndex < cttsEntry_.size(), Status::ERROR_INVALID_DATA);
            buildCtx.cttsSamplesRemaining = cttsEntry_[buildCtx.cttsIndex].count;
            buildCtx.cttsDelta = cttsEntry_[buildCtx.cttsIndex].delta;
        }
        uint32_t consumed = std::min(remaining, buildCtx.cttsSamplesRemaining);
        remaining -= consumed;
        buildCtx.cttsSamplesRemaining -= consumed;
    }
    return remaining == 0 ? Status::OK : Status::ERROR_INVALID_DATA;
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
    if (buildCtx.hasPreviousDuration) {
        int64_t derivedDuration = buildCtx.currentDts - sampleIndexEntry_.back().dts;
        if (derivedDuration != buildCtx.previousDuration) {
            FALSE_RETURN_V_NOLOG(AddDurationOverride(buildCtx.sampleIndex - 1, buildCtx.previousDuration) == Status::OK,
                Status::ERROR_INVALID_DATA);
        }
    }
    sampleIndexEntry_.emplace_back(SampleIndexEntry {});
    sampleIndexEntry_.back().pos = buildCtx.currentOffset;
    sampleIndexEntry_.back().dts = buildCtx.currentDts;
    FALSE_RETURN_V_MSG_E(GetSampleSize(buildCtx.sampleIndex, buildCtx.samplesPerFrame, sampleIndexEntry_.back(),
        buildCtx.remainSamples), Status::ERROR_INVALID_DATA, "Sample size is too large");
    int64_t ptsOffset = 0;
    if (!cttsEntry_.empty()) {
        FALSE_RETURN_V_NOLOG(ConsumeCttsSamples(buildCtx, buildCtx.samplesPerFrame, ptsOffset) == Status::OK,
            Status::ERROR_INVALID_DATA);
    }
    FALSE_RETURN_V_NOLOG(sampleIndexEntry_.back().SetPts(sampleIndexEntry_.back().dts + ptsOffset),
        Status::ERROR_INVALID_DATA);
    int64_t duration = 0;
    Status ret = UpdateSampleDurationAndDts(sttsEntry.delta, buildCtx.samplesPerFrame, duration, buildCtx.currentDts);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Update sample duration failed");
    lastPTS_ = std::max(lastPTS_, sampleIndexEntry_.back().GetPts());
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(buildCtx.currentOffset,
        static_cast<int64_t>(sampleIndexEntry_.back().GetSize()), &buildCtx.currentOffset),
        Status::ERROR_INVALID_DATA);
    FALSE_RETURN_V_MSG_E(buildCtx.currentOffset >= 0, Status::ERROR_INVALID_DATA, "Sample Offset is negative");
    buildCtx.previousDuration = duration;
    buildCtx.hasPreviousDuration = true;
    ++buildCtx.sampleIndex;
    sampleInSttsEntry += buildCtx.samplesPerFrame;
    buildCtx.sampleInChunk += buildCtx.samplesPerFrame;
    buildCtx.processedSttsSamples += buildCtx.samplesPerFrame;
    return AdvanceChunkIfNeeded(buildCtx);
}

Status MPEG4SampleHelper::BuildSampleIndexEntries()
{
    std::lock_guard<std::mutex> lock(indexMutex_);
    if (sampleIndexState_.load(std::memory_order_relaxed) == SampleIndexState::DENSE) {
        return Status::OK;
    }
    if (denseAttempted_) {
        return denseStatus_;
    }
    denseAttempted_ = true;
    sampleIndexEntry_.clear();
    durationOverrides_.clear();
    if (isFragmented_ && sttsEntry_.empty()) {
        denseStatus_ = Status::OK;
        sampleIndexState_.store(SampleIndexState::DENSE, std::memory_order_release);
        return denseStatus_;
    }
    denseStatus_ = BuildDenseSampleIndexEntries();
    if (denseStatus_ == Status::OK) {
        sampleTableSummary_.logicalSampleCount = static_cast<uint32_t>(sampleIndexEntry_.size());
        sampleIndexState_.store(SampleIndexState::DENSE, std::memory_order_release);
    } else {
        sampleIndexEntry_.clear();
        durationOverrides_.clear();
    }
    return denseStatus_;
}

Status MPEG4SampleHelper::EnsureDenseIndex()
{
    return BuildSampleIndexEntries();
}

Status MPEG4SampleHelper::BuildDenseSampleIndexEntries()
{
    FALSE_RETURN_V_MSG_E(PreBuildProcessAndCheck() == Status::OK, Status::ERROR_INVALID_DATA, "PreCheck failed");
    uint64_t expectedSampleCount = CountSttsSamples();
    FALSE_RETURN_V_MSG_E(expectedSampleCount > 0 && expectedSampleCount <= MAX_SAMPLE_TABLE_SAMPLES,
        Status::ERROR_INVALID_DATA, "Invalid sample index entry count");
    SampleIndexBuildContext buildCtx = CreateSampleIndexBuildContext();
    for (const auto& sttsEntry : sttsEntry_) {  // 遍历sttsEntry
        FALSE_RETURN_V_MSG_E(sttsEntry.count > 0 && sttsEntry.count <= expectedSampleCount,
            Status::ERROR_INVALID_DATA, "Invalid stts sample count");
        uint32_t sampleInSttsEntry = 0;
        while (sampleInSttsEntry < sttsEntry.count) {  // 根据sttsEntry.count进行循环
            Status ret = AppendSampleIndexEntry(sttsEntry, sampleInSttsEntry, buildCtx);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Append sample index entry failed");
        }
    }

    if (buildCtx.hasPreviousDuration && !sampleIndexEntry_.empty()) {
        FALSE_RETURN_V_NOLOG(AddDurationOverride(static_cast<uint32_t>(sampleIndexEntry_.size() - 1),
            buildCtx.previousDuration) == Status::OK, Status::ERROR_INVALID_DATA);
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
        rawTotalSize_ += sample.GetSize();
    }
    rawTotalDuration_ = totalDuration_;
}

Status MPEG4SampleHelper::GetSampleDuration(uint32_t index, uint32_t &duration)
{
    FALSE_RETURN_V_NOLOG(!sampleIndexEntry_.empty() && !sttsEntry_.empty(), Status::ERROR_INVALID_OPERATION);

    // LCOV_EXCL_START
    if (firstMoofSampleIndex_ > 0) {
        if (index < firstMoofSampleIndex_) {
            int64_t temp = index;
            uint32_t sttsIndex = 0;
            while (sttsIndex < sttsEntry_.size() - 1 &&
                temp >= static_cast<int64_t>(sttsEntry_[sttsIndex].count)) {
                temp -= static_cast<int64_t>(sttsEntry_[sttsIndex].count);
                ++sttsIndex;
            }
            duration = sttsEntry_[sttsIndex].delta;
        } else {
            FALSE_RETURN_V_MSG_E(index >= numMoovSamples_ && firstMoofSttsIndex_ > 0,
                Status::ERROR_INVALID_PARAMETER, "Invalid sample index");
            uint64_t sttsIndex = static_cast<uint64_t>(index) - numMoovSamples_ +
                                 static_cast<uint64_t>(firstMoofSttsIndex_) - 1;
            FALSE_RETURN_V_MSG_E(sttsIndex < sttsEntry_.size(), Status::ERROR_INVALID_PARAMETER,
                "sttsIndex out of range");
            duration = sttsEntry_[sttsIndex].delta;
        }
        return Status::OK;
    }

    int64_t temp = index;
    uint32_t sttsIndex = 0;
    while (sttsIndex < sttsEntry_.size() - 1 && temp >= static_cast<int64_t>(sttsEntry_[sttsIndex].count)) {
        temp -= static_cast<int64_t>(sttsEntry_[sttsIndex].count);
        ++sttsIndex;
    }
    // LCOV_EXCL_STOP
    FALSE_RETURN_V_NOLOG(sttsIndex < sttsEntry_.size(), Status::ERROR_INVALID_PARAMETER);
    duration = sttsEntry_[sttsIndex].delta;
    return Status::OK;
}

uint32_t MPEG4SampleHelper::CalculateSampleCount()
{
    uint32_t sampleCount = 0;
    for (const auto& entry : sttsEntry_) {
        uint32_t temp = 0;
        FALSE_RETURN_V_MSG_E(entry.count > 0 && entry.count <= MAX_SAMPLE_TABLE_SAMPLES &&
            !__builtin_add_overflow(sampleCount, entry.count, &temp) && temp <= MAX_SAMPLE_TABLE_SAMPLES,
            0, "sampleCount overflow");
        sampleCount = temp;
    }
    MEDIA_LOG_I("sampleCount is " PUBLIC_LOG_U32, sampleCount);
    return sampleCount;
}

} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
