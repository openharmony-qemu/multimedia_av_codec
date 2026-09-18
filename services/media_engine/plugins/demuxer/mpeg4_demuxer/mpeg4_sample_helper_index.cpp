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

#define HST_LOG_TAG "MPEG4SampleHelper"
#include <algorithm>
#include <array>
#include <limits>

#include "common/log.h"
#include "mpeg4_sample.h"
#include "mpeg4_sample_helper.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4SampleHelper"};
constexpr uint32_t MAX_REORDER_DELAY = 16;
constexpr size_t MAX_SIMPLE_EDIT_ENTRY_COUNT = 2;
constexpr size_t SKIP_SAMPLES_INFO_SIZE = 10;
constexpr uint32_t BITS_PER_BYTE = 8;
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
struct MPEG4SampleHelper::SimpleEditProjectionBuildContext {
    uint32_t rawStart = 0;
    uint32_t rawEnd = 0;
    uint32_t firstPlayableRaw = 0;
    int64_t rawDtsAtFirstPlayable = 0;
    int64_t anchorDts = 0;
    int64_t minCorrectedPts = INT64_MAX;
    int64_t maxCorrectedPts = INT64_MIN;
    int64_t maxCorrectedPtsEnd = INT64_MIN;
    int64_t maxOutputDtsEnd = INT64_MIN;
    int64_t rawStartDts = 0;
    int64_t rawStartPts = 0;
    uint32_t skipOutputIndex = 0;
    uint32_t skipSamples = 0;
    bool hasSkipSamples = false;
    bool started = false;
    bool foundKeyframeAfterEdit = false;
};

struct MPEG4SampleHelper::SimpleEditProjectionSampleContext {
    const EditListEntry& edit;
    const SampleIndexView& view;
    SimpleEditProjectionBuildContext& scanCtx;
    int64_t baseDts = 0;
    int64_t editEnd = 0;
    uint32_t rawIndex = 0;
    bool shouldBreak = false;
};

bool MPEG4SampleHelper::IsGroupedRawAudio() const
{
    return mimeType_ == MimeType::AUDIO_G711MU || mimeType_ == MimeType::AUDIO_G711A ||
        mimeType_ == MimeType::AUDIO_RAW;
}

uint32_t MPEG4SampleHelper::CalculateLogicalSampleCount(uint32_t rawSampleCount) const
{
    if (!IsGroupedRawAudio()) {
        return rawSampleCount;
    }
    uint64_t logicalCount = 0;
    uint64_t processedSamples = 0;
    uint32_t stscIndex = 0;
    for (uint32_t chunkIndex = 0; chunkIndex < chunkOffsets_.size() && processedSamples < rawSampleCount;
        ++chunkIndex) {
        if (stscIndex + 1 < stscEntry_.size() && chunkIndex == stscEntry_[stscIndex + 1].first) {
            ++stscIndex;
        }
        uint64_t samplesInChunk = std::min<uint64_t>(stscEntry_[stscIndex].count,
            static_cast<uint64_t>(rawSampleCount) - processedSamples);
        logicalCount += (samplesInChunk + maxGroupedSamplesPerFrame - 1) /
            maxGroupedSamplesPerFrame;
        processedSamples += samplesInChunk;
    }
    return processedSamples == rawSampleCount && logicalCount <= UINT32_MAX ?
        static_cast<uint32_t>(logicalCount) : 0;
}

bool MPEG4SampleHelper::UseSparseIndex() const
{
    return sampleIndexState_.load(std::memory_order_acquire) != SampleIndexState::DENSE && sampleIndexEntry_.empty();
}

size_t MPEG4SampleHelper::GetSampleCount() const
{
    return UseSparseIndex() ? sampleTableSummary_.logicalSampleCount : sampleIndexEntry_.size();
}

bool MPEG4SampleHelper::HasSamples() const
{
    return GetSampleCount() > 0;
}

bool MPEG4SampleHelper::IsSampleIndexEmpty() const
{
    return !HasSamples();
}

Status MPEG4SampleHelper::GetSamplePts(uint32_t sampleIndex, int64_t& pts) const
{
    if (UseSparseIndex()) {
        SampleIndexView view;
        Status ret = GetSparseSampleIndexView(sampleIndex, view);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
        pts = view.pts;
        return Status::OK;
    }
    FALSE_RETURN_V_NOLOG(sampleIndex < sampleIndexEntry_.size(), Status::ERROR_INVALID_PARAMETER);
    return sampleIndexEntry_[sampleIndex].GetPts(pts) ? Status::OK : Status::ERROR_INVALID_DATA;
}

Status MPEG4SampleHelper::GetSampleDts(uint32_t sampleIndex, int64_t& dts) const
{
    if (UseSparseIndex()) {
        SampleIndexView view;
        Status ret = GetSparseSampleIndexView(sampleIndex, view);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
        dts = view.dts;
        return Status::OK;
    }
    FALSE_RETURN_V_NOLOG(sampleIndex < sampleIndexEntry_.size(), Status::ERROR_INVALID_PARAMETER);
    dts = sampleIndexEntry_[sampleIndex].dts;
    return Status::OK;
}

Status MPEG4SampleHelper::GetSampleSize(uint32_t sampleIndex, int32_t& size) const
{
    if (UseSparseIndex()) {
        SampleIndexView view;
        Status ret = GetSparseSampleIndexView(sampleIndex, view);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
        size = view.size;
        return Status::OK;
    }
    FALSE_RETURN_V_NOLOG(sampleIndex < sampleIndexEntry_.size(), Status::ERROR_INVALID_PARAMETER);
    size = sampleIndexEntry_[sampleIndex].GetSize();
    return Status::OK;
}

Status MPEG4SampleHelper::GetSampleRange(uint32_t sampleIndex, int64_t& pos, int32_t& size) const
{
    if (UseSparseIndex()) {
        SampleIndexView view;
        Status ret = GetSparseSampleIndexView(sampleIndex, view);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
        pos = view.pos;
        size = view.size;
        return Status::OK;
    }
    FALSE_RETURN_V_NOLOG(sampleIndex < sampleIndexEntry_.size(), Status::ERROR_INVALID_PARAMETER);
    pos = sampleIndexEntry_[sampleIndex].pos;
    size = sampleIndexEntry_[sampleIndex].GetSize();
    return Status::OK;
}

Status MPEG4SampleHelper::GetSampleIndexView(uint32_t sampleIndex, SampleIndexView& view) const
{
    if (UseSparseIndex()) {
        return GetSparseSampleIndexView(sampleIndex, view);
    }
    FALSE_RETURN_V_NOLOG(sampleIndex < sampleIndexEntry_.size(), Status::ERROR_INVALID_PARAMETER);
    const auto& entry = sampleIndexEntry_[sampleIndex];
    view.pos = entry.pos;
    view.dts = entry.dts;
    view.size = entry.GetSize();
    FALSE_RETURN_V_NOLOG(entry.GetPts(view.pts), Status::ERROR_INVALID_DATA);
    FALSE_RETURN_V_NOLOG(GetSampleDurationTicks(sampleIndex, view.duration) == Status::OK,
        Status::ERROR_INVALID_DATA);
    view.flag = IsSyncSample(sampleIndex) ? Sample::SampleFlag::SYNC_FRAME : Sample::SampleFlag::NONE;
    if (entry.IsDiscard()) {
        view.flag |= Sample::SampleFlag::DISCARD;
    }
    return Status::OK;
}

bool MPEG4SampleHelper::IsDiscardSample(uint32_t sampleIndex) const
{
    if (UseSparseIndex()) {
        SampleIndexView view;
        return GetSampleIndexView(sampleIndex, view) == Status::OK &&
            (view.flag & Sample::SampleFlag::DISCARD) != 0;
    }
    return sampleIndex < sampleIndexEntry_.size() && sampleIndexEntry_[sampleIndex].IsDiscard();
}

bool MPEG4SampleHelper::IsSyncSample(uint32_t sampleIndex) const
{
    if (UseSparseIndex()) {
        SampleIndexView view;
        return GetSampleIndexView(sampleIndex, view) == Status::OK &&
            (view.flag & Sample::SampleFlag::SYNC_FRAME) != 0;
    }
    return sampleIndex < sampleIndexEntry_.size() && IsSyncSample(stssEntry_, allSamplesSync_, sampleIndex);
}

Status MPEG4SampleHelper::FindSampleIndexByPosition(int64_t position, uint32_t& sampleIndex) const
{
    if (UseSparseIndex()) {
        size_t sampleCount = GetSampleCount();
        FALSE_RETURN_V_NOLOG(sampleCount > 0, Status::ERROR_INVALID_DATA);
        uint32_t left = 0;
        uint32_t right = static_cast<uint32_t>(sampleCount);
        while (left < right) {
            uint32_t middle = left + (right - left) / 2;
            SampleIndexView currentSample;
            FALSE_RETURN_V_NOLOG(GetSampleIndexView(middle, currentSample) == Status::OK,
                Status::ERROR_INVALID_DATA);
            if (currentSample.pos <= position) {
                left = middle + 1;
            } else {
                right = middle;
            }
        }
        sampleIndex = left == 0 ? 0 : left - 1;
        return Status::OK;
    }
    auto it = std::upper_bound(sampleIndexEntry_.begin(), sampleIndexEntry_.end(), position,
        [](int64_t pos, const SampleIndexEntry& entry) { return pos < entry.pos; });
    if (it != sampleIndexEntry_.begin()) {
        --it;
    }
    FALSE_RETURN_V_NOLOG(it != sampleIndexEntry_.end(), Status::ERROR_INVALID_DATA);
    return AssignSampleIndex(std::distance(sampleIndexEntry_.begin(), it), sampleIndex) ?
        Status::OK : Status::ERROR_INVALID_DATA;
}

Status MPEG4SampleHelper::FindFirstSampleAfterPosition(int64_t position, uint32_t& sampleIndex) const
{
    if (UseSparseIndex()) {
        size_t sampleCount = GetSampleCount();
        uint32_t left = 0;
        uint32_t right = static_cast<uint32_t>(sampleCount);
        while (left < right) {
            uint32_t middle = left + (right - left) / 2;
            SampleIndexView currentSample;
            FALSE_RETURN_V_NOLOG(GetSampleIndexView(middle, currentSample) == Status::OK,
                Status::ERROR_INVALID_DATA);
            if (currentSample.pos <= position) {
                left = middle + 1;
            } else {
                right = middle;
            }
        }
        sampleIndex = left;
        return Status::OK;
    }
    auto it = std::upper_bound(sampleIndexEntry_.begin(), sampleIndexEntry_.end(), position,
        [](int64_t pos, const SampleIndexEntry& entry) { return pos < entry.pos; });
    return AssignSampleIndex(std::distance(sampleIndexEntry_.begin(), it), sampleIndex) ?
        Status::OK : Status::ERROR_INVALID_DATA;
}

Status MPEG4SampleHelper::FindFirstSampleByDts(int64_t dts, uint32_t& sampleIndex) const
{
    if (UseSparseIndex()) {
        uint32_t left = 0;
        uint32_t right = static_cast<uint32_t>(GetSampleCount());
        while (left < right) {
            uint32_t middle = left + (right - left) / 2;
            int64_t currentDts = 0;
            FALSE_RETURN_V_NOLOG(GetSampleDts(middle, currentDts) == Status::OK, Status::ERROR_INVALID_DATA);
            if (currentDts < dts) {
                left = middle + 1;
            } else {
                right = middle;
            }
        }
        FALSE_RETURN_V_NOLOG(left < GetSampleCount(), Status::END_OF_STREAM);
        sampleIndex = left;
        return Status::OK;
    }
    auto it = std::lower_bound(sampleIndexEntry_.begin(), sampleIndexEntry_.end(), dts,
        [](const SampleIndexEntry& entry, int64_t targetDts) { return entry.dts < targetDts; });
    FALSE_RETURN_V_NOLOG(it != sampleIndexEntry_.end(), Status::END_OF_STREAM);
    return AssignSampleIndex(std::distance(sampleIndexEntry_.begin(), it), sampleIndex) ?
        Status::OK : Status::ERROR_INVALID_DATA;
}

void MPEG4SampleHelper::SetSkipSamplesInfo(uint32_t sampleIndex, std::vector<uint8_t>&& skipSamplesInfo)
{
    if (skipSamplesInfo.empty()) {
        skipSamplesInfoMap_.erase(sampleIndex);
        return;
    }
    skipSamplesInfoMap_[sampleIndex] = std::move(skipSamplesInfo);
}

int64_t MPEG4SampleHelper::GetSyncSamplePts(size_t stssIndex) const
{
    FALSE_RETURN_V_NOLOG(stssIndex < stssEntry_.size(), 0);
    uint32_t sampleIndex = stssEntry_[stssIndex].index;
    if (UseSparseIndex() && simpleEditProjection_.enabled) {
        if (sampleIndex < simpleEditProjection_.rawStart || sampleIndex > simpleEditProjection_.rawEnd) {
            return 0;
        }
        sampleIndex -= simpleEditProjection_.rawStart;
    }
    int64_t pts = 0;
    return GetSamplePts(sampleIndex, pts) == Status::OK ? pts : 0;
}

uint32_t MPEG4SampleHelper::EstimateVideoDelay() const
{
    FALSE_RETURN_V_NOLOG(!sttsEntry_.empty() && !cttsEntry_.empty() && sampleTableSummary_.rawSampleCount > 0, 0);
    // Keep metadata parsing index-free: VIDEO_DELAY only needs DTS plus CTTS order, not full sample offsets.
    std::array<int64_t, MAX_REORDER_DELAY + 1> ptsBuffer;
    ptsBuffer.fill(INT64_MIN);
    uint32_t bufferStart = 0;
    uint32_t videoDelay = 0;
    int64_t currentDts = editListEntries_.empty() ? -timeOffset_ - dtsShift_ : -dtsShift_;
    size_t sttsIndex = 0;
    size_t cttsIndex = 0;
    uint32_t sttsRemaining = sttsEntry_[0].count;
    uint32_t cttsRemaining = cttsEntry_[0].count;
    for (uint32_t i = 0; i < sampleTableSummary_.rawSampleCount; ++i) {
        int64_t samplePts = 0;
        int64_t ptsOffset = 0;
        FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(dtsShift_,
            static_cast<int64_t>(cttsEntry_[cttsIndex].delta), &ptsOffset) &&
            !__builtin_add_overflow(currentDts, ptsOffset, &samplePts), 0);
        uint32_t pos = bufferStart;
        bufferStart = (bufferStart + 1) % ptsBuffer.size();
        ptsBuffer[pos] = samplePts;

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

        FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(currentDts, static_cast<int64_t>(sttsEntry_[sttsIndex].delta),
            &currentDts), 0);
        if (--sttsRemaining == 0 && i + 1 < sampleTableSummary_.rawSampleCount) {
            ++sttsIndex;
            FALSE_RETURN_V_NOLOG(sttsIndex < sttsEntry_.size(), 0);
            sttsRemaining = sttsEntry_[sttsIndex].count;
        }
        if (--cttsRemaining == 0 && i + 1 < sampleTableSummary_.rawSampleCount) {
            ++cttsIndex;
            FALSE_RETURN_V_NOLOG(cttsIndex < cttsEntry_.size(), 0);
            cttsRemaining = cttsEntry_[cttsIndex].count;
        }
    }
    return videoDelay;
}

Status MPEG4SampleHelper::CreateInitialCursor(SampleTableCursor& cursor) const
{
    FALSE_RETURN_V_NOLOG(!sttsEntry_.empty() && !stscEntry_.empty() && !chunkOffsets_.empty() &&
        !sampleSizes_.empty(), Status::ERROR_INVALID_DATA);
    // The sparse cursor rebuilds one logical sample from raw MP4 tables without materializing sampleIndexEntry_.
    cursor = SampleTableCursor {};
    cursor.offset = chunkOffsets_[0];
    cursor.dts = editListEntries_.empty() ? -timeOffset_ - dtsShift_ : -dtsShift_;
    cursor.sttsRemaining = sttsEntry_[0].count;
    if (!cttsEntry_.empty()) {
        cursor.cttsRemaining = cttsEntry_[0].count;
    }
    return Status::OK;
}

Status MPEG4SampleHelper::ReadRawSampleAndAdvance(SampleTableCursor& cursor, SampleIndexView& view) const
{
    RawSampleStep step;
    Status ret = PrepareRawSampleStep(cursor, step);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    ret = FillRawSampleView(cursor, step, view);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    return AdvanceRawSampleCursor(cursor, step);
}

Status MPEG4SampleHelper::PrepareRawSampleStep(const SampleTableCursor& cursor, RawSampleStep& step) const
{
    FALSE_RETURN_V_NOLOG(cursor.sttsIndex < sttsEntry_.size() && cursor.stscIndex < stscEntry_.size() &&
        cursor.chunkIndex < chunkOffsets_.size(), Status::ERROR_INVALID_DATA);
    step.samplesInChunk = stscEntry_[cursor.stscIndex].count;
    FALSE_RETURN_V_NOLOG(cursor.sampleInChunk < step.samplesInChunk && cursor.sttsRemaining > 0,
        Status::ERROR_INVALID_DATA);
    step.samplesPerFrame = 1;
    if (IsGroupedRawAudio()) {
        step.samplesPerFrame = std::min({maxGroupedSamplesPerFrame, step.samplesInChunk - cursor.sampleInChunk,
            cursor.sttsRemaining});
    }
    FALSE_RETURN_V_NOLOG(step.samplesPerFrame > 0, Status::ERROR_INVALID_DATA);
    Status ret = ResolveRawSampleSize(cursor, step.samplesPerFrame, step.sampleSize);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    FALSE_RETURN_V_NOLOG(!__builtin_mul_overflow(static_cast<int64_t>(sttsEntry_[cursor.sttsIndex].delta),
        static_cast<int64_t>(step.samplesPerFrame), &step.duration), Status::ERROR_INVALID_DATA);
    return Status::OK;
}

Status MPEG4SampleHelper::ResolveRawSampleSize(
    const SampleTableCursor& cursor, uint32_t samplesPerFrame, int32_t& sampleSize) const
{
    if (IsGroupedRawAudio()) {
        int64_t groupedSize = static_cast<int64_t>(sampleSizes_.front()) * static_cast<int64_t>(samplesPerFrame);
        FALSE_RETURN_V_NOLOG(groupedSize >= 0 &&
            groupedSize <= static_cast<int64_t>(SampleIndexEntry::sizeMask),
            Status::ERROR_INVALID_DATA);
        sampleSize = static_cast<int32_t>(groupedSize);
    } else {
        size_t sizeIndex = hasDefaultStszSize_ ? size_t {0} : static_cast<size_t>(cursor.processedRawSamples);
        FALSE_RETURN_V_NOLOG(sizeIndex < sampleSizes_.size(), Status::ERROR_INVALID_DATA);
        sampleSize = sampleSizes_[sizeIndex];
    }
    return Status::OK;
}

Status MPEG4SampleHelper::FillRawSampleView(
    const SampleTableCursor& cursor, const RawSampleStep& step, SampleIndexView& view) const
{
    view = SampleIndexView {};
    view.pos = cursor.offset;
    view.dts = cursor.dts;
    view.size = step.sampleSize;
    int64_t ptsOffset = 0;
    if (!cttsEntry_.empty()) {
        FALSE_RETURN_V_NOLOG(cursor.cttsIndex < cttsEntry_.size() && cursor.cttsRemaining > 0,
            Status::ERROR_INVALID_DATA);
        FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(dtsShift_,
            static_cast<int64_t>(cttsEntry_[cursor.cttsIndex].delta), &ptsOffset) && ptsOffset >= 0 &&
            static_cast<uint64_t>(ptsOffset) <= UINT32_MAX, Status::ERROR_INVALID_DATA);
    }
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(view.dts, ptsOffset, &view.pts), Status::ERROR_INVALID_DATA);
    view.duration = step.duration;
    uint32_t rawSampleIndex = static_cast<uint32_t>(cursor.processedRawSamples);
    bool isSync = IsSyncSample(stssEntry_, allSamplesSync_, rawSampleIndex);
    view.flag = isSync ? Sample::SampleFlag::SYNC_FRAME : Sample::SampleFlag::NONE;
    return Status::OK;
}

Status MPEG4SampleHelper::AdvanceRawSampleCursor(SampleTableCursor& cursor, const RawSampleStep& step) const
{
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(cursor.offset, static_cast<int64_t>(step.sampleSize),
        &cursor.offset) && !__builtin_add_overflow(cursor.dts, step.duration, &cursor.dts),
        Status::ERROR_INVALID_DATA);
    Status ret = ConsumeRawSampleCtts(cursor, step.samplesPerFrame);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    cursor.sttsRemaining -= step.samplesPerFrame;
    if (cursor.sttsRemaining == 0) {
        ++cursor.sttsIndex;
        if (cursor.sttsIndex < sttsEntry_.size()) {
            cursor.sttsRemaining = sttsEntry_[cursor.sttsIndex].count;
        }
    }
    uint64_t nextRawSample = cursor.processedRawSamples + step.samplesPerFrame;
    while (cursor.stssIndex < stssEntry_.size() && stssEntry_[cursor.stssIndex].index < nextRawSample) {
        ++cursor.stssIndex;
    }
    cursor.processedRawSamples = nextRawSample;
    cursor.sampleInChunk += step.samplesPerFrame;
    ++cursor.outputIndex;
    if (cursor.sampleInChunk == step.samplesInChunk &&
        cursor.processedRawSamples < sampleTableSummary_.rawSampleCount) {
        ++cursor.chunkIndex;
        FALSE_RETURN_V_NOLOG(cursor.chunkIndex < chunkOffsets_.size(), Status::ERROR_INVALID_DATA);
        cursor.sampleInChunk = 0;
        if (cursor.stscIndex + 1 < stscEntry_.size() &&
            cursor.chunkIndex == stscEntry_[cursor.stscIndex + 1].first) {
            ++cursor.stscIndex;
        }
        cursor.offset = chunkOffsets_[cursor.chunkIndex];
    }
    return Status::OK;
}

Status MPEG4SampleHelper::ConsumeRawSampleCtts(SampleTableCursor& cursor, uint32_t samplesPerFrame) const
{
    uint32_t remaining = samplesPerFrame;
    while (remaining > 0 && !cttsEntry_.empty()) {
        uint32_t consumed = std::min(remaining, cursor.cttsRemaining);
        remaining -= consumed;
        cursor.cttsRemaining -= consumed;
        if (cursor.cttsRemaining == 0) {
            bool hasMoreRawSamples = cursor.processedRawSamples < sampleTableSummary_.rawSampleCount &&
                samplesPerFrame < sampleTableSummary_.rawSampleCount - cursor.processedRawSamples;
            if (remaining > 0 || hasMoreRawSamples) {
                ++cursor.cttsIndex;
                FALSE_RETURN_V_NOLOG(cursor.cttsIndex < cttsEntry_.size(), Status::ERROR_INVALID_DATA);
                cursor.cttsRemaining = cttsEntry_[cursor.cttsIndex].count;
            }
        }
    }
    return Status::OK;
}

Status MPEG4SampleHelper::CalculateSampleTableSummary()
{
    uint64_t rawCount = CountSttsSamples();
    FALSE_RETURN_V_NOLOG(rawCount > 0, Status::ERROR_INVALID_DATA);
    sampleTableSummary_ = SampleTableSummary {};
    sampleTableSummary_.maxDtsEnd = INT64_MIN;
    sampleTableSummary_.maxPtsEnd = INT64_MIN;
    sampleTableSummary_.rawSampleCount = static_cast<uint32_t>(rawCount);
    sampleTableSummary_.logicalSampleCount = CalculateLogicalSampleCount(sampleTableSummary_.rawSampleCount);
    FALSE_RETURN_V_NOLOG(sampleTableSummary_.logicalSampleCount > 0, Status::ERROR_INVALID_DATA);

    int64_t maxPts = INT64_MIN;
    if (IsGroupedRawAudio()) {
        FALSE_RETURN_V_NOLOG(CalculateGroupedSampleTableSummary(maxPts) == Status::OK, Status::ERROR_INVALID_DATA);
    } else {
        int64_t initialDts = editListEntries_.empty() ? -timeOffset_ - dtsShift_ : -dtsShift_;
        sampleTableSummary_.firstDts = initialDts;
        int64_t firstPtsOffset = cttsEntry_.empty() ? 0 :
            dtsShift_ + static_cast<int64_t>(cttsEntry_.front().delta);
        FALSE_RETURN_V_NOLOG(firstPtsOffset >= 0 && static_cast<uint64_t>(firstPtsOffset) <= UINT32_MAX &&
            !__builtin_add_overflow(initialDts, firstPtsOffset, &sampleTableSummary_.firstPts),
            Status::ERROR_INVALID_DATA);
        FALSE_RETURN_V_NOLOG(CalculateFlatSampleSizeSummary(rawCount) == Status::OK &&
            CalculateFlatSampleTimeSummary(initialDts, maxPts) == Status::OK, Status::ERROR_INVALID_DATA);
    }
    FALSE_RETURN_V_NOLOG(CalculateRawSampleDurationSummary() == Status::OK, Status::ERROR_INVALID_DATA);
    rawSampleCount_ = sampleTableSummary_.rawSampleCount;
    rawTotalSize_ = sampleTableSummary_.rawTotalSize;
    rawTotalDuration_ = sampleTableSummary_.rawTotalDuration;
    totalDuration_ = sampleTableSummary_.rawTotalDuration;
    lastPTS_ = maxPts;
    return Status::OK;
}

Status MPEG4SampleHelper::CalculateGroupedSampleTableSummary(int64_t& maxPts)
{
    SampleTableCursor cursor;
    FALSE_RETURN_V_NOLOG(CreateInitialCursor(cursor) == Status::OK, Status::ERROR_INVALID_DATA);
    for (uint32_t i = 0; i < sampleTableSummary_.logicalSampleCount; ++i) {
        SampleIndexView view;
        FALSE_RETURN_V_NOLOG(ReadRawSampleAndAdvance(cursor, view) == Status::OK, Status::ERROR_INVALID_DATA);
        if (i == 0) {
            sampleTableSummary_.firstDts = view.dts;
            sampleTableSummary_.firstPts = view.pts;
        }
        int64_t dtsEnd = 0;
        int64_t ptsEnd = 0;
        int64_t totalSize = 0;
        FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(view.dts, view.duration, &dtsEnd) &&
            !__builtin_add_overflow(view.pts, view.duration, &ptsEnd) &&
            !__builtin_add_overflow(sampleTableSummary_.rawTotalSize, static_cast<int64_t>(view.size), &totalSize),
            Status::ERROR_INVALID_DATA);
        sampleTableSummary_.maxDtsEnd = std::max(sampleTableSummary_.maxDtsEnd, dtsEnd);
        sampleTableSummary_.maxPtsEnd = std::max(sampleTableSummary_.maxPtsEnd, ptsEnd);
        sampleTableSummary_.rawTotalSize = totalSize;
        sampleTableSummary_.maxSampleSize = std::max(sampleTableSummary_.maxSampleSize, view.size);
        maxPts = std::max(maxPts, view.pts);
    }
    return Status::OK;
}

Status MPEG4SampleHelper::CalculateFlatSampleSizeSummary(uint64_t rawCount)
{
    if (hasDefaultStszSize_) {
        int64_t defaultSize = sampleSizes_.front();
        FALSE_RETURN_V_NOLOG(defaultSize >= 0 && defaultSize <= SampleIndexEntry::sizeMask &&
            !__builtin_mul_overflow(defaultSize, static_cast<int64_t>(rawCount), &sampleTableSummary_.rawTotalSize),
            Status::ERROR_INVALID_DATA);
        sampleTableSummary_.maxSampleSize = static_cast<int32_t>(defaultSize);
        return Status::OK;
    }
    for (uint32_t i = 0; i < sampleTableSummary_.rawSampleCount; ++i) {
        int64_t totalSize = 0;
        int32_t sampleSize = sampleSizes_[i];
        FALSE_RETURN_V_NOLOG(sampleSize >= 0 && static_cast<uint32_t>(sampleSize) <= SampleIndexEntry::sizeMask &&
            !__builtin_add_overflow(sampleTableSummary_.rawTotalSize, static_cast<int64_t>(sampleSize), &totalSize),
            Status::ERROR_INVALID_DATA);
        sampleTableSummary_.rawTotalSize = totalSize;
        sampleTableSummary_.maxSampleSize = std::max(sampleTableSummary_.maxSampleSize, sampleSize);
    }
    return Status::OK;
}

Status MPEG4SampleHelper::CalculateFlatSampleTimeSummary(int64_t initialDts, int64_t& maxPts)
{
    size_t sttsIndex = 0;
    size_t cttsIndex = 0;
    uint32_t sttsRemaining = sttsEntry_[0].count;
    uint32_t cttsRemaining = cttsEntry_.empty() ? sampleTableSummary_.rawSampleCount : cttsEntry_[0].count;
    int64_t currentDts = initialDts;
    uint32_t processedSamples = 0;
    // Intersect STTS and CTTS runs to derive metadata, avoiding dense per-sample entries at SetDataSource.
    while (processedSamples < sampleTableSummary_.rawSampleCount) {
        uint32_t runCount = std::min(sttsRemaining, cttsRemaining);
        FALSE_RETURN_V_NOLOG(runCount > 0, Status::ERROR_INVALID_DATA);
        int64_t ptsOffset = cttsEntry_.empty() ? 0 : dtsShift_ + static_cast<int64_t>(cttsEntry_[cttsIndex].delta);
        int64_t runDtsAdvance = 0;
        int64_t lastSampleDts = 0;
        int64_t lastSamplePts = 0;
        int64_t lastSamplePtsEnd = 0;
        FALSE_RETURN_V_NOLOG(ptsOffset >= 0 && static_cast<uint64_t>(ptsOffset) <= UINT32_MAX &&
            !__builtin_mul_overflow(static_cast<int64_t>(runCount - 1),
                static_cast<int64_t>(sttsEntry_[sttsIndex].delta), &runDtsAdvance) &&
            !__builtin_add_overflow(currentDts, runDtsAdvance, &lastSampleDts) &&
            !__builtin_add_overflow(lastSampleDts, ptsOffset, &lastSamplePts) &&
        !__builtin_add_overflow(lastSamplePts, static_cast<int64_t>(sttsEntry_[sttsIndex].delta),
            &lastSamplePtsEnd), Status::ERROR_INVALID_DATA);
        maxPts = std::max(maxPts, lastSamplePts);
        sampleTableSummary_.maxPtsEnd = std::max(sampleTableSummary_.maxPtsEnd, lastSamplePtsEnd);
        int64_t dtsAdvance = 0;
        FALSE_RETURN_V_NOLOG(!__builtin_mul_overflow(static_cast<int64_t>(runCount),
            static_cast<int64_t>(sttsEntry_[sttsIndex].delta), &dtsAdvance) &&
            !__builtin_add_overflow(currentDts, dtsAdvance, &currentDts), Status::ERROR_INVALID_DATA);
        processedSamples += runCount;
        sttsRemaining -= runCount;
        cttsRemaining -= runCount;
        if (sttsRemaining == 0 && processedSamples < sampleTableSummary_.rawSampleCount) {
            ++sttsIndex;
            FALSE_RETURN_V_NOLOG(sttsIndex < sttsEntry_.size(), Status::ERROR_INVALID_DATA);
            sttsRemaining = sttsEntry_[sttsIndex].count;
        }
        if (!cttsEntry_.empty() && cttsRemaining == 0 && processedSamples < sampleTableSummary_.rawSampleCount) {
            ++cttsIndex;
            FALSE_RETURN_V_NOLOG(cttsIndex < cttsEntry_.size(), Status::ERROR_INVALID_DATA);
            cttsRemaining = cttsEntry_[cttsIndex].count;
        }
    }
    sampleTableSummary_.maxDtsEnd = currentDts;
    return Status::OK;
}

Status MPEG4SampleHelper::CalculateRawSampleDurationSummary()
{
    for (const auto& entry : sttsEntry_) {
        int64_t runDuration = 0;
        int64_t totalDuration = 0;
        FALSE_RETURN_V_NOLOG(!__builtin_mul_overflow(static_cast<int64_t>(entry.count),
            static_cast<int64_t>(entry.delta), &runDuration) &&
            !__builtin_add_overflow(sampleTableSummary_.rawTotalDuration, runDuration, &totalDuration),
            Status::ERROR_INVALID_DATA);
        sampleTableSummary_.rawTotalDuration = totalDuration;
    }
    return Status::OK;
}

Status MPEG4SampleHelper::BuildSimpleEditProjection()
{
    hasSimpleEditStartCursor_ = false;
    if (!IsSimpleEditProjectionSupported()) {
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    const EditListEntry& edit = editListEntries_.back();
    int64_t editDuration = 0;
    int64_t emptyDuration = 0;
    int64_t editEnd = 0;
    FALSE_RETURN_V_NOLOG(PrepareSimpleEditProjection(edit, editDuration, emptyDuration, editEnd) == Status::OK,
        Status::ERROR_UNSUPPORTED_FORMAT);

    SimpleEditProjectionBuildContext scanCtx;
    SampleTableCursor startCursor;
    FALSE_RETURN_V_NOLOG(FindSimpleEditRawStart(edit, scanCtx.rawStart, startCursor) == Status::OK &&
        ScanSimpleEditProjectionSamples(edit, emptyDuration, editEnd, startCursor, scanCtx) == Status::OK,
        Status::ERROR_UNSUPPORTED_FORMAT);
    Status ret = CommitSimpleEditProjection(edit, editDuration, emptyDuration, editEnd, scanCtx);
    if (ret == Status::OK) {
        simpleEditStartCursor_ = startCursor;
        hasSimpleEditStartCursor_ = true;
    }
    return ret;
}

bool MPEG4SampleHelper::IsSimpleEditProjectionSupported() const
{
    // Sparse ELST is intentionally limited to the common linear projection cases.
    // Complex edit lists still use the existing dense ApplyEditList path for exact compatibility.
    if (editListEntries_.empty() || editListEntries_.size() > MAX_SIMPLE_EDIT_ENTRY_COUNT ||
        IsGroupedRawAudio() || movieTimeScale_ <= 0 || trackTimeScale_ <= 0) {
        return false;
    }
    bool hasLeadingEmptyEdit = editListEntries_.size() == MAX_SIMPLE_EDIT_ENTRY_COUNT &&
        editListEntries_[0].mediaTime == -1 && editListEntries_[1].mediaTime >= 0;
    bool hasSingleMediaEdit = editListEntries_.size() == 1 && editListEntries_[0].mediaTime >= 0;
    if (!hasLeadingEmptyEdit && !hasSingleMediaEdit) {
        return false;
    }
    for (const auto& entry : editListEntries_) {
        if (entry.mediaRateInteger != 1 || entry.mediaRateFraction != 0) {
            return false;
        }
    }
    return true;
}

Status MPEG4SampleHelper::PrepareSimpleEditProjection(
    const EditListEntry& edit, int64_t& editDuration, int64_t& emptyDuration, int64_t& editEnd) const
{
    FALSE_RETURN_V_NOLOG(ConvertEditDuration(edit, editDuration) == Status::OK && editDuration > 0,
        Status::ERROR_UNSUPPORTED_FORMAT);
    if (editListEntries_.size() == MAX_SIMPLE_EDIT_ENTRY_COUNT) {
        FALSE_RETURN_V_NOLOG(ConvertEditDuration(editListEntries_[0], emptyDuration) == Status::OK,
            Status::ERROR_UNSUPPORTED_FORMAT);
    }
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(edit.mediaTime, editDuration, &editEnd),
        Status::ERROR_UNSUPPORTED_FORMAT);
    return Status::OK;
}

Status MPEG4SampleHelper::FindSimpleEditRawStart(const EditListEntry& edit, uint32_t& rawStart,
    SampleTableCursor& startCursor) const
{
    SampleTableCursor cursor;
    FALSE_RETURN_V_NOLOG(CreateInitialCursor(cursor) == Status::OK, Status::ERROR_UNSUPPORTED_FORMAT);
    SampleTableCursor lastAnyCursor = cursor;
    SampleTableCursor lastSyncCursor = cursor;
    int64_t searchTimestamp = edit.mediaTime;
    if (trackType_ == AUDIO_TYPE) {
        int64_t audioSearch = 0;
        FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(searchTimestamp, trackTimeScale_, &audioSearch),
            Status::ERROR_UNSUPPORTED_FORMAT);
        searchTimestamp = std::max(audioSearch, cursor.dts);
    }
    int64_t searchDts = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(searchTimestamp, dtsShift_, &searchDts),
        Status::ERROR_UNSUPPORTED_FORMAT);
    uint32_t lastAny = 0;
    uint32_t lastSync = 0;
    bool hasAny = false;
    bool hasSync = false;
    while (cursor.processedRawSamples < sampleTableSummary_.rawSampleCount) {
        SampleTableCursor currentCursor = cursor;
        uint32_t rawIndex = static_cast<uint32_t>(cursor.processedRawSamples);
        SampleIndexView view;
        FALSE_RETURN_V_NOLOG(ReadRawSampleAndAdvance(cursor, view) == Status::OK,
            Status::ERROR_UNSUPPORTED_FORMAT);
        if (view.dts > searchDts) {
            break;
        }
        hasAny = true;
        lastAny = rawIndex;
        lastAnyCursor = currentCursor;
        if (IsSyncSample(stssEntry_, allSamplesSync_, rawIndex) && view.pts <= edit.mediaTime) {
            hasSync = true;
            lastSync = rawIndex;
            lastSyncCursor = currentCursor;
        }
    }
    rawStart = hasSync ? lastSync : (hasAny ? lastAny : 0);
    startCursor = hasSync ? lastSyncCursor : lastAnyCursor;
    return Status::OK;
}

Status MPEG4SampleHelper::ScanSimpleEditProjectionSamples(const EditListEntry& edit, int64_t emptyDuration,
    int64_t editEnd, const SampleTableCursor& startCursor, SimpleEditProjectionBuildContext& scanCtx) const
{
    SampleTableCursor cursor = startCursor;
    FALSE_RETURN_V_NOLOG(cursor.processedRawSamples == scanCtx.rawStart, Status::ERROR_UNSUPPORTED_FORMAT);
    int64_t baseDts = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(-dtsShift_, emptyDuration, &baseDts),
        Status::ERROR_UNSUPPORTED_FORMAT);
    scanCtx.rawEnd = scanCtx.rawStart;
    scanCtx.firstPlayableRaw = scanCtx.rawStart;
    while (cursor.processedRawSamples < sampleTableSummary_.rawSampleCount) {
        uint32_t rawIndex = static_cast<uint32_t>(cursor.processedRawSamples);
        SampleIndexView view;
        FALSE_RETURN_V_NOLOG(ReadRawSampleAndAdvance(cursor, view) == Status::OK,
            Status::ERROR_UNSUPPORTED_FORMAT);
        SimpleEditProjectionSampleContext sampleCtx {edit, view, scanCtx, baseDts, editEnd, rawIndex};
        Status ret = ProcessSimpleEditProjectionSample(sampleCtx);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
        if (sampleCtx.shouldBreak) {
            break;
        }
    }
    return Status::OK;
}

Status MPEG4SampleHelper::ProcessSimpleEditProjectionSample(SimpleEditProjectionSampleContext& sampleCtx) const
{
    sampleCtx.shouldBreak = false;
    if (sampleCtx.rawIndex == sampleCtx.scanCtx.rawStart) {
        sampleCtx.scanCtx.rawStartDts = sampleCtx.view.dts;
        sampleCtx.scanCtx.rawStartPts = sampleCtx.view.pts;
    }
    int64_t frameEnd = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(sampleCtx.view.pts, sampleCtx.view.duration, &frameEnd),
        Status::ERROR_UNSUPPORTED_FORMAT);
    bool outside = sampleCtx.view.pts < sampleCtx.edit.mediaTime || sampleCtx.view.pts >= sampleCtx.editEnd;
    bool crossedAudioStart = trackType_ == AUDIO_TYPE && sampleCtx.view.pts < sampleCtx.edit.mediaTime &&
        frameEnd >= sampleCtx.edit.mediaTime;
    if (!sampleCtx.scanCtx.started && (!outside || crossedAudioStart)) {
        Status ret = StartSimpleEditProjectionSample(sampleCtx, crossedAudioStart);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    }
    if (!sampleCtx.scanCtx.started) {
        return Status::OK;
    }
    Status ret = UpdateSimpleEditProjectionTimes(sampleCtx.view, outside, sampleCtx.scanCtx);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    sampleCtx.shouldBreak = ShouldStopSimpleEditProjectionScan(sampleCtx, frameEnd);
    return Status::OK;
}

Status MPEG4SampleHelper::StartSimpleEditProjectionSample(
    SimpleEditProjectionSampleContext& sampleCtx, bool crossedAudioStart) const
{
    auto& scanCtx = sampleCtx.scanCtx;
    scanCtx.started = true;
    scanCtx.firstPlayableRaw = sampleCtx.rawIndex;
    scanCtx.rawDtsAtFirstPlayable = sampleCtx.view.dts;
    int64_t skip = crossedAudioStart ? sampleCtx.edit.mediaTime - sampleCtx.view.pts : 0;
    FALSE_RETURN_V_NOLOG(skip >= 0 && skip <= UINT32_MAX &&
        !__builtin_sub_overflow(sampleCtx.baseDts, skip, &scanCtx.anchorDts), Status::ERROR_UNSUPPORTED_FORMAT);
    if (crossedAudioStart && skip > 0) {
        scanCtx.hasSkipSamples = true;
        scanCtx.skipOutputIndex = sampleCtx.rawIndex - scanCtx.rawStart;
        scanCtx.skipSamples = static_cast<uint32_t>(skip);
    }
    return Status::OK;
}

Status MPEG4SampleHelper::UpdateSimpleEditProjectionTimes(
    const SampleIndexView& view, bool outside, SimpleEditProjectionBuildContext& scanCtx) const
{
    int64_t rawPtsOffset = 0;
    int64_t rawDtsDelta = 0;
    int64_t outputDts = 0;
    int64_t correctedPts = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(view.pts, view.dts, &rawPtsOffset) &&
        !__builtin_sub_overflow(view.dts, scanCtx.rawDtsAtFirstPlayable, &rawDtsDelta) &&
        !__builtin_add_overflow(scanCtx.anchorDts, rawDtsDelta, &outputDts) &&
        !__builtin_add_overflow(outputDts, rawPtsOffset, &correctedPts), Status::ERROR_UNSUPPORTED_FORMAT);
    int64_t correctedPtsEnd = 0;
    int64_t outputDtsEnd = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(correctedPts, view.duration, &correctedPtsEnd) &&
        !__builtin_add_overflow(outputDts, view.duration, &outputDtsEnd), Status::ERROR_UNSUPPORTED_FORMAT);
    scanCtx.maxCorrectedPts = std::max(scanCtx.maxCorrectedPts, correctedPts);
    scanCtx.maxCorrectedPtsEnd = std::max(scanCtx.maxCorrectedPtsEnd, correctedPtsEnd);
    scanCtx.maxOutputDtsEnd = std::max(scanCtx.maxOutputDtsEnd, outputDtsEnd);
    if (!outside) {
        scanCtx.minCorrectedPts = std::min(scanCtx.minCorrectedPts, correctedPts);
    }
    return Status::OK;
}

bool MPEG4SampleHelper::ShouldStopSimpleEditProjectionScan(
    SimpleEditProjectionSampleContext& sampleCtx, int64_t frameEnd) const
{
    auto& scanCtx = sampleCtx.scanCtx;
    scanCtx.rawEnd = sampleCtx.rawIndex;
    if (frameEnd < sampleCtx.editEnd ||
        (!IsSyncSample(stssEntry_, allSamplesSync_, sampleCtx.rawIndex) && trackType_ != AUDIO_TYPE)) {
        return false;
    }
    if (trackType_ == VIDEO_TYPE && !cttsEntry_.empty() && !scanCtx.foundKeyframeAfterEdit) {
        scanCtx.foundKeyframeAfterEdit = true;
        return false;
    }
    return true;
}

Status MPEG4SampleHelper::CommitSimpleEditProjection(const EditListEntry& edit, int64_t editDuration,
    int64_t emptyDuration, int64_t editEnd, const SimpleEditProjectionBuildContext& scanCtx)
{
    FALSE_RETURN_V_NOLOG(scanCtx.started && scanCtx.rawEnd >= scanCtx.rawStart &&
        scanCtx.minCorrectedPts != INT64_MAX, Status::ERROR_UNSUPPORTED_FORMAT);
    int64_t correctionCandidate = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(scanCtx.minCorrectedPts, emptyDuration, &correctionCandidate),
        Status::ERROR_UNSUPPORTED_FORMAT);
    // Keep the sparse projection consistent with ApplyEditList(): only video DTS is shifted by min corrected PTS.
    int64_t correction = trackType_ == VIDEO_TYPE ? std::max<int64_t>(0, correctionCandidate) : 0;
    int64_t dtsAdjustment = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(scanCtx.anchorDts, scanCtx.rawDtsAtFirstPlayable, &dtsAdjustment) &&
        !__builtin_sub_overflow(dtsAdjustment, correction, &dtsAdjustment), Status::ERROR_UNSUPPORTED_FORMAT);
    uint64_t outputCount = static_cast<uint64_t>(scanCtx.rawEnd) - scanCtx.rawStart + 1;
    FALSE_RETURN_V_NOLOG(outputCount <= UINT32_MAX, Status::ERROR_UNSUPPORTED_FORMAT);
    simpleEditProjection_ = SimpleEditProjection {
        true, scanCtx.rawStart, scanCtx.rawEnd, static_cast<uint32_t>(outputCount), scanCtx.firstPlayableRaw,
        dtsAdjustment, edit.mediaTime, editEnd, emptyDuration
    };
    sampleTableSummary_.logicalSampleCount = static_cast<uint32_t>(outputCount);
    int64_t firstPtsOffset = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(scanCtx.rawStartPts, scanCtx.rawStartDts, &firstPtsOffset) &&
        !__builtin_add_overflow(scanCtx.rawStartDts, dtsAdjustment, &sampleTableSummary_.firstDts) &&
        !__builtin_add_overflow(sampleTableSummary_.firstDts, firstPtsOffset, &sampleTableSummary_.firstPts) &&
        !__builtin_sub_overflow(scanCtx.maxOutputDtsEnd, correction, &sampleTableSummary_.maxDtsEnd) &&
        !__builtin_sub_overflow(scanCtx.maxCorrectedPtsEnd, correction, &sampleTableSummary_.maxPtsEnd) &&
        !__builtin_sub_overflow(scanCtx.maxCorrectedPts, correction, &lastPTS_) &&
        !__builtin_add_overflow(emptyDuration, editDuration, &totalDuration_),
        Status::ERROR_UNSUPPORTED_FORMAT);
    if (scanCtx.hasSkipSamples) {
        SaveSimpleEditSkipSamples(scanCtx.skipOutputIndex, scanCtx.skipSamples);
    }
    return Status::OK;
}

void MPEG4SampleHelper::SaveSimpleEditSkipSamples(uint32_t skipOutputIndex, uint32_t skipSamples)
{
    std::vector<uint8_t> info(SKIP_SAMPLES_INFO_SIZE, 0);
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        info[i] = static_cast<uint8_t>((skipSamples >> (BITS_PER_BYTE * i)) & 0xFF);
    }
    skipSamplesInfoMap_[skipOutputIndex] = std::move(info);
}

Status MPEG4SampleHelper::ApplySimpleEditProjection(uint32_t rawIndex, SampleIndexView& view) const
{
    if (!simpleEditProjection_.enabled) {
        return Status::OK;
    }
    FALSE_RETURN_V_NOLOG(rawIndex >= simpleEditProjection_.rawStart && rawIndex <= simpleEditProjection_.rawEnd,
        Status::ERROR_INVALID_DATA);
    int64_t rawPtsOffset = 0;
    int64_t rawPts = view.pts;
    int64_t projectedDts = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(view.pts, view.dts, &rawPtsOffset) &&
        !__builtin_add_overflow(view.dts, simpleEditProjection_.dtsAdjustment, &projectedDts) &&
        !__builtin_add_overflow(projectedDts, rawPtsOffset, &view.pts), Status::ERROR_INVALID_DATA);
    view.dts = projectedDts;
    bool outside = rawPts < simpleEditProjection_.editMediaTime || rawPts >= simpleEditProjection_.editEndTime;
    if (rawIndex < simpleEditProjection_.firstPlayableRaw || outside) {
        view.flag |= Sample::SampleFlag::DISCARD;
    }
    return Status::OK;
}

Status MPEG4SampleHelper::FinalizeSampleTables()
{
    std::lock_guard<std::mutex> lock(indexMutex_);
    if (sampleTablesFinalized_) {
        return sampleTableStatus_;
    }
    sampleTablesFinalized_ = true;
    // Latch raw-table validation before lazy access so invalid media cannot bypass pre-build checks.
    requiresDenseIndex_ = isFragmented_;
    sampleTableStatus_ = PreBuildProcessAndCheck();
    if (sampleTableStatus_ != Status::OK) {
        return sampleTableStatus_;
    }
    if (sttsEntry_.empty()) {
        return Status::OK;
    }
    sampleTableStatus_ = CalculateSampleTableSummary();
    if (sampleTableStatus_ == Status::OK && !editListEntries_.empty()) {
        Status projectionRet = BuildSimpleEditProjection();
        if (projectionRet != Status::OK) {
            requiresDenseIndex_ = true;
        }
    }
    return sampleTableStatus_;
}

Status MPEG4SampleHelper::EnsureSparseIndex() const
{
    if (sampleIndexState_.load(std::memory_order_acquire) != SampleIndexState::UNBUILT) {
        return Status::OK;
    }
    std::lock_guard<std::mutex> lock(indexMutex_);
    if (sampleIndexState_.load(std::memory_order_relaxed) != SampleIndexState::UNBUILT) {
        return Status::OK;
    }
    return BuildSparseIndexLocked();
}

Status MPEG4SampleHelper::BuildSparseIndexLocked() const
{
    // Cache the first sparse result; repeated readers must observe the same failure instead of retrying.
    if (sparseAttempted_) {
        return sparseStatus_;
    }
    sparseAttempted_ = true;
    if (!sampleTablesFinalized_ || sampleTableStatus_ != Status::OK || requiresDenseIndex_) {
        sparseStatus_ = sampleTableStatus_ != Status::OK ? sampleTableStatus_ : Status::ERROR_INVALID_OPERATION;
        return sparseStatus_;
    }
    sparseStatus_ = CreateInitialCursor(sparseFrontier_);
    if (sparseStatus_ != Status::OK) {
        return sparseStatus_;
    }
    sparseStatus_ = SkipSparseProjectionPrefix();
    FALSE_RETURN_V_NOLOG(sparseStatus_ == Status::OK, sparseStatus_);
    ClearSparseViewCacheLocked();
    size_t checkpointCount = (static_cast<size_t>(sampleTableSummary_.logicalSampleCount) +
        sparseCheckpointInterval - 1) / sparseCheckpointInterval;
    sparseCheckpoints_.reserve(checkpointCount);
    sparseCheckpoints_.push_back(SparseCheckpoint {0, sparseFrontier_});
    sampleIndexState_.store(SampleIndexState::SPARSE, std::memory_order_release);
    return sparseStatus_;
}

bool MPEG4SampleHelper::FindSparseViewCacheLocked(uint32_t sampleIndex, SampleIndexView& view) const
{
    const auto& cache = sparseViewCache_[sampleIndex & (sparseViewCacheSize - 1)];
    if (cache.sampleIndex != sampleIndex) {
        return false;
    }
    view = cache.view;
    return true;
}

void MPEG4SampleHelper::SaveSparseViewCacheLocked(uint32_t sampleIndex, const SampleIndexView& view) const
{
    auto& cache = sparseViewCache_[sampleIndex & (sparseViewCacheSize - 1)];
    cache.sampleIndex = sampleIndex;
    cache.view = view;
}

void MPEG4SampleHelper::ClearSparseViewCacheLocked() const
{
    for (auto& cache : sparseViewCache_) {
        cache = SparseViewCacheEntry {};
    }
}

void MPEG4SampleHelper::ReleaseSparseStorageLocked() const
{
    std::vector<SparseCheckpoint>().swap(sparseCheckpoints_);
    sparseFrontier_ = SampleTableCursor {};
    ClearSparseViewCacheLocked();
}

Status MPEG4SampleHelper::SkipSparseProjectionPrefix() const
{
    if (!simpleEditProjection_.enabled) {
        return Status::OK;
    }
    if (hasSimpleEditStartCursor_) {
        sparseFrontier_ = simpleEditStartCursor_;
        sparseFrontier_.outputIndex = 0;
        return Status::OK;
    }
    // Start sparse iteration at the projected raw sample while exposing output indices from zero.
    for (uint32_t i = 0; i < simpleEditProjection_.rawStart; ++i) {
        SampleIndexView ignored;
        Status ret = ReadRawSampleAndAdvance(sparseFrontier_, ignored);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    }
    sparseFrontier_.outputIndex = 0;
    return Status::OK;
}

Status MPEG4SampleHelper::GetSparseSampleIndexView(uint32_t sampleIndex, SampleIndexView& view) const
{
    Status ret = EnsureSparseIndex();
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    FALSE_RETURN_V_NOLOG(sampleIndex < sampleTableSummary_.logicalSampleCount, Status::ERROR_INVALID_PARAMETER);
    std::lock_guard<std::mutex> lock(indexMutex_);
    if (FindSparseViewCacheLocked(sampleIndex, view)) {
        return Status::OK;
    }
    if (sampleIndex >= sparseFrontier_.outputIndex) {
        ret = AdvanceSparseFrontierToSample(sampleIndex, view);
    } else {
        ret = ReadSparseSampleFromCheckpoint(sampleIndex, view);
    }
    if (ret == Status::OK) {
        SaveSparseViewCacheLocked(sampleIndex, view);
    }
    return ret;
}

Status MPEG4SampleHelper::AdvanceSparseFrontierToSample(uint32_t sampleIndex, SampleIndexView& view) const
{
    while (sparseFrontier_.outputIndex <= sampleIndex) {
        uint32_t rawIndex = static_cast<uint32_t>(sparseFrontier_.processedRawSamples);
        if (sparseFrontier_.outputIndex != 0 &&
            sparseFrontier_.outputIndex % sparseCheckpointInterval == 0 &&
            sparseCheckpoints_.back().outputIndex != sparseFrontier_.outputIndex) {
            // Checkpoints bound random-access replay cost without storing every sample entry.
            sparseCheckpoints_.push_back(SparseCheckpoint {sparseFrontier_.outputIndex, sparseFrontier_});
        }
        Status ret = ReadRawSampleAndAdvance(sparseFrontier_, view);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
        ret = ApplySimpleEditProjection(rawIndex, view);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    }
    return Status::OK;
}

Status MPEG4SampleHelper::ReadSparseSampleFromCheckpoint(uint32_t sampleIndex, SampleIndexView& view) const
{
    size_t checkpointIndex = sampleIndex / sparseCheckpointInterval;
    FALSE_RETURN_V_NOLOG(checkpointIndex < sparseCheckpoints_.size(), Status::ERROR_INVALID_DATA);
    const auto& checkpoint = sparseCheckpoints_[checkpointIndex];
    FALSE_RETURN_V_NOLOG(checkpoint.outputIndex <= sampleIndex, Status::ERROR_INVALID_DATA);
    SampleTableCursor cursor = checkpoint.cursor;
    while (cursor.outputIndex <= sampleIndex) {
        uint32_t rawIndex = static_cast<uint32_t>(cursor.processedRawSamples);
        Status ret = ReadRawSampleAndAdvance(cursor, view);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
        ret = ApplySimpleEditProjection(rawIndex, view);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    }
    return Status::OK;
}

MPEG4SampleHelper::SampleIndexState MPEG4SampleHelper::GetSampleIndexState() const
{
    return sampleIndexState_.load(std::memory_order_acquire);
}

bool MPEG4SampleHelper::ShouldBuildDenseIndex() const
{
    return requiresDenseIndex_;
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
    if (emptyStss_ && GetSampleCount() > 0) {
        syncSamplesIndex.emplace_back(0);
        return syncSamplesIndex;
    }
    for (uint32_t i = 0; i < stssEntry_.size(); ++i) {
        uint32_t index = stssEntry_[i].index;
        if (sampleIndexState_.load(std::memory_order_acquire) != SampleIndexState::DENSE &&
            simpleEditProjection_.enabled) {
            if (index < simpleEditProjection_.rawStart || index > simpleEditProjection_.rawEnd) {
                continue;
            }
            index -= simpleEditProjection_.rawStart;
        }
        syncSamplesIndex.emplace_back(index);
    }
    return syncSamplesIndex;
}

MPEG4SampleHelper::SdtpEntry MPEG4SampleHelper::GetSdtpEntry()
{
    return sdtpEntry_;
}

Status MPEG4SampleHelper::GetPtsOfFirstSample(int64_t& startPtsBasedOnTimescale)
{
    return GetSamplePts(0, startPtsBasedOnTimescale);
}

void MPEG4SampleHelper::SetFragmented(bool fragmented)
{
    MEDIA_LOG_I("SetFragmented in");
    isFragmented_ = fragmented;
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
    if (duration == 0) {
        FALSE_RETURN_V_NOLOG(AddDurationOverride(buildCtx.sampleIndex, 0) == Status::OK,
            Status::ERROR_INVALID_DATA);
    }
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
        ReleaseSparseStorageLocked();
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
    FALSE_RETURN_V_MSG_E(expectedSampleCount > 0, Status::ERROR_INVALID_DATA, "Invalid sample index entry count");
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

    // AAC can carry one final zero-byte stsz entry without a matching stts entry. FFmpeg exposes it as an
    // untimed packet at the end of the decode timeline; keep the source tables unchanged and add only its view.
    if (mimeType_ == MimeType::AUDIO_AAC && !hasDefaultStszSize_ &&
        sampleSizes_.size() == expectedSampleCount + 1 && sampleSizes_[expectedSampleCount] == 0) {
        SampleIndexEntry trailingSample;
        trailingSample.pos = buildCtx.currentOffset;
        trailingSample.dts = buildCtx.currentDts;
        FALSE_RETURN_V_NOLOG(trailingSample.SetPts(trailingSample.dts) && trailingSample.SetSize(0) &&
            AddDurationOverride(static_cast<uint32_t>(sampleIndexEntry_.size()), 0) == Status::OK,
            Status::ERROR_INVALID_DATA);
        sampleIndexEntry_.emplace_back(std::move(trailingSample));
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
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
