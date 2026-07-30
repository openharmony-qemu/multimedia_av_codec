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
#include <limits>

#include "common/log.h"
#include "mpeg4_sample_helper.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4SampleHelper"};
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
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
        logicalCount += (samplesInChunk + MAX_GROUPED_SAMPLES_PER_FRAME - 1) /
            MAX_GROUPED_SAMPLES_PER_FRAME;
        processedSamples += samplesInChunk;
    }
    return processedSamples == rawSampleCount && logicalCount <= UINT32_MAX ?
        static_cast<uint32_t>(logicalCount) : 0;
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
    FALSE_RETURN_V_NOLOG(cursor.sttsIndex < sttsEntry_.size() && cursor.stscIndex < stscEntry_.size() &&
        cursor.chunkIndex < chunkOffsets_.size(), Status::ERROR_INVALID_DATA);
    uint32_t samplesInChunk = stscEntry_[cursor.stscIndex].count;
    FALSE_RETURN_V_NOLOG(cursor.sampleInChunk < samplesInChunk && cursor.sttsRemaining > 0,
        Status::ERROR_INVALID_DATA);
    uint32_t samplesPerFrame = 1;
    if (IsGroupedRawAudio()) {
        samplesPerFrame = std::min({MAX_GROUPED_SAMPLES_PER_FRAME, samplesInChunk - cursor.sampleInChunk,
            cursor.sttsRemaining});
    }
    FALSE_RETURN_V_NOLOG(samplesPerFrame > 0, Status::ERROR_INVALID_DATA);

    int32_t sampleSize = 0;
    if (IsGroupedRawAudio()) {
        int64_t groupedSize = static_cast<int64_t>(sampleSizes_[0]) * samplesPerFrame;
        FALSE_RETURN_V_NOLOG(groupedSize >= 0 && groupedSize <= SampleIndexEntry::sizeMask,
            Status::ERROR_INVALID_DATA);
        sampleSize = static_cast<int32_t>(groupedSize);
    } else {
        size_t sizeIndex = hasDefaultStszSize_ ? 0 : static_cast<size_t>(cursor.processedRawSamples);
        FALSE_RETURN_V_NOLOG(sizeIndex < sampleSizes_.size(), Status::ERROR_INVALID_DATA);
        sampleSize = sampleSizes_[sizeIndex];
    }

    view = SampleIndexView {};
    view.pos = cursor.offset;
    view.dts = cursor.dts;
    view.size = sampleSize;
    int64_t ptsOffset = 0;
    if (!cttsEntry_.empty()) {
        FALSE_RETURN_V_NOLOG(cursor.cttsIndex < cttsEntry_.size() && cursor.cttsRemaining > 0,
            Status::ERROR_INVALID_DATA);
        FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(dtsShift_,
            static_cast<int64_t>(cttsEntry_[cursor.cttsIndex].delta), &ptsOffset) && ptsOffset >= 0 &&
            static_cast<uint64_t>(ptsOffset) <= UINT32_MAX, Status::ERROR_INVALID_DATA);
    }
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(view.dts, ptsOffset, &view.pts), Status::ERROR_INVALID_DATA);
    FALSE_RETURN_V_NOLOG(!__builtin_mul_overflow(static_cast<int64_t>(sttsEntry_[cursor.sttsIndex].delta),
        static_cast<int64_t>(samplesPerFrame), &view.duration), Status::ERROR_INVALID_DATA);
    uint32_t rawSampleIndex = static_cast<uint32_t>(cursor.processedRawSamples);
    view.flag = IsSyncSample(stssEntry_, allSamplesSync_, rawSampleIndex) ?
        Sample::SampleFlag::SYNC_FRAME : Sample::SampleFlag::NONE;

    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(cursor.offset, static_cast<int64_t>(sampleSize), &cursor.offset) &&
        !__builtin_add_overflow(cursor.dts, view.duration, &cursor.dts), Status::ERROR_INVALID_DATA);
    uint32_t remaining = samplesPerFrame;
    while (remaining > 0 && !cttsEntry_.empty()) {
        uint32_t consumed = std::min(remaining, cursor.cttsRemaining);
        remaining -= consumed;
        cursor.cttsRemaining -= consumed;
        if (cursor.cttsRemaining == 0 && remaining > 0) {
            ++cursor.cttsIndex;
            FALSE_RETURN_V_NOLOG(cursor.cttsIndex < cttsEntry_.size(), Status::ERROR_INVALID_DATA);
            cursor.cttsRemaining = cttsEntry_[cursor.cttsIndex].count;
        }
    }
    cursor.sttsRemaining -= samplesPerFrame;
    if (cursor.sttsRemaining == 0) {
        ++cursor.sttsIndex;
        if (cursor.sttsIndex < sttsEntry_.size()) {
            cursor.sttsRemaining = sttsEntry_[cursor.sttsIndex].count;
        }
    }
    cursor.processedRawSamples += samplesPerFrame;
    cursor.sampleInChunk += samplesPerFrame;
    ++cursor.outputIndex;
    if (cursor.sampleInChunk == samplesInChunk &&
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

Status MPEG4SampleHelper::CalculateSampleTableSummary()
{
    uint64_t rawCount = CountSttsSamples();
    FALSE_RETURN_V_NOLOG(rawCount > 0 && rawCount <= MAX_VALIDATED_SAMPLE_COUNT, Status::ERROR_INVALID_DATA);
    sampleTableSummary_ = SampleTableSummary {};
    sampleTableSummary_.maxDtsEnd = INT64_MIN;
    sampleTableSummary_.maxPtsEnd = INT64_MIN;
    sampleTableSummary_.rawSampleCount = static_cast<uint32_t>(rawCount);
    sampleTableSummary_.logicalSampleCount = CalculateLogicalSampleCount(sampleTableSummary_.rawSampleCount);
    FALSE_RETURN_V_NOLOG(sampleTableSummary_.logicalSampleCount > 0, Status::ERROR_INVALID_DATA);

    int64_t maxPts = INT64_MIN;
    if (IsGroupedRawAudio()) {
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
                !__builtin_add_overflow(sampleTableSummary_.rawTotalSize,
                    static_cast<int64_t>(view.size), &totalSize), Status::ERROR_INVALID_DATA);
            sampleTableSummary_.maxDtsEnd = std::max(sampleTableSummary_.maxDtsEnd, dtsEnd);
            sampleTableSummary_.maxPtsEnd = std::max(sampleTableSummary_.maxPtsEnd, ptsEnd);
            sampleTableSummary_.rawTotalSize = totalSize;
            sampleTableSummary_.maxSampleSize = std::max(sampleTableSummary_.maxSampleSize, view.size);
            maxPts = std::max(maxPts, view.pts);
        }
    } else {
        int64_t initialDts = editListEntries_.empty() ? -timeOffset_ - dtsShift_ : -dtsShift_;
        sampleTableSummary_.firstDts = initialDts;
        int64_t firstPtsOffset = cttsEntry_.empty() ? 0 :
            dtsShift_ + static_cast<int64_t>(cttsEntry_.front().delta);
        FALSE_RETURN_V_NOLOG(firstPtsOffset >= 0 && static_cast<uint64_t>(firstPtsOffset) <= UINT32_MAX &&
            !__builtin_add_overflow(initialDts, firstPtsOffset, &sampleTableSummary_.firstPts),
            Status::ERROR_INVALID_DATA);

        if (hasDefaultStszSize_) {
            int64_t defaultSize = sampleSizes_.front();
            FALSE_RETURN_V_NOLOG(defaultSize >= 0 && defaultSize <= SampleIndexEntry::sizeMask &&
                !__builtin_mul_overflow(defaultSize, static_cast<int64_t>(rawCount),
                    &sampleTableSummary_.rawTotalSize), Status::ERROR_INVALID_DATA);
            sampleTableSummary_.maxSampleSize = static_cast<int32_t>(defaultSize);
        } else {
            for (uint32_t i = 0; i < sampleTableSummary_.rawSampleCount; ++i) {
                int64_t totalSize = 0;
                int32_t sampleSize = sampleSizes_[i];
                FALSE_RETURN_V_NOLOG(sampleSize >= 0 &&
                    static_cast<uint32_t>(sampleSize) <= SampleIndexEntry::sizeMask &&
                    !__builtin_add_overflow(sampleTableSummary_.rawTotalSize,
                        static_cast<int64_t>(sampleSize), &totalSize), Status::ERROR_INVALID_DATA);
                sampleTableSummary_.rawTotalSize = totalSize;
                sampleTableSummary_.maxSampleSize = std::max(sampleTableSummary_.maxSampleSize, sampleSize);
            }
        }

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
            int64_t ptsOffset = cttsEntry_.empty() ? 0 :
                dtsShift_ + static_cast<int64_t>(cttsEntry_[cttsIndex].delta);
            int64_t runDtsAdvance = 0;
            int64_t lastSampleDts = 0;
            int64_t lastSamplePts = 0;
            int64_t lastSamplePtsEnd = 0;
            FALSE_RETURN_V_NOLOG(ptsOffset >= 0 && static_cast<uint64_t>(ptsOffset) <= UINT32_MAX &&
                !__builtin_mul_overflow(static_cast<int64_t>(runCount - 1),
                    static_cast<int64_t>(sttsEntry_[sttsIndex].delta), &runDtsAdvance) &&
                !__builtin_add_overflow(currentDts, runDtsAdvance, &lastSampleDts) &&
                !__builtin_add_overflow(lastSampleDts, ptsOffset, &lastSamplePts) &&
                !__builtin_add_overflow(lastSamplePts,
                    static_cast<int64_t>(sttsEntry_[sttsIndex].delta), &lastSamplePtsEnd),
                Status::ERROR_INVALID_DATA);
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
            if (!cttsEntry_.empty() && cttsRemaining == 0 &&
                processedSamples < sampleTableSummary_.rawSampleCount) {
                ++cttsIndex;
                FALSE_RETURN_V_NOLOG(cttsIndex < cttsEntry_.size(), Status::ERROR_INVALID_DATA);
                cttsRemaining = cttsEntry_[cttsIndex].count;
            }
        }
        sampleTableSummary_.maxDtsEnd = currentDts;
    }
    for (const auto& entry : sttsEntry_) {
        int64_t runDuration = 0;
        int64_t totalDuration = 0;
        FALSE_RETURN_V_NOLOG(!__builtin_mul_overflow(static_cast<int64_t>(entry.count),
            static_cast<int64_t>(entry.delta), &runDuration) &&
            !__builtin_add_overflow(sampleTableSummary_.rawTotalDuration, runDuration, &totalDuration),
            Status::ERROR_INVALID_DATA);
        sampleTableSummary_.rawTotalDuration = totalDuration;
    }
    rawSampleCount_ = sampleTableSummary_.rawSampleCount;
    rawTotalSize_ = sampleTableSummary_.rawTotalSize;
    rawTotalDuration_ = sampleTableSummary_.rawTotalDuration;
    totalDuration_ = sampleTableSummary_.rawTotalDuration;
    lastPTS_ = maxPts;
    return Status::OK;
}

Status MPEG4SampleHelper::BuildSimpleEditProjection()
{
    // Sparse ELST is intentionally limited to the common linear projection cases.
    // Complex edit lists still use the existing dense ApplyEditList path for exact compatibility.
    if (editListEntries_.size() == 0 || editListEntries_.size() > 2 ||
        (editListEntries_.size() == 2 && editListEntries_[0].mediaTime != -1) ||
        (editListEntries_.size() == 2 && editListEntries_[1].mediaTime < 0) ||
        IsGroupedRawAudio() || movieTimeScale_ <= 0 || trackTimeScale_ <= 0) {
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    const EditListEntry& edit = editListEntries_.back();
    for (const auto& entry : editListEntries_) {
        if (entry.mediaRateInteger != 1 || entry.mediaRateFraction != 0) {
            return Status::ERROR_UNSUPPORTED_FORMAT;
        }
    }
    int64_t editDuration = 0;
    int64_t emptyDuration = 0;
    FALSE_RETURN_V_NOLOG(ConvertEditDuration(edit, editDuration) == Status::OK && editDuration > 0,
        Status::ERROR_UNSUPPORTED_FORMAT);
    if (editListEntries_.size() == 2) {
        FALSE_RETURN_V_NOLOG(ConvertEditDuration(editListEntries_[0], emptyDuration) == Status::OK,
            Status::ERROR_UNSUPPORTED_FORMAT);
    }
    int64_t editEnd = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(edit.mediaTime, editDuration, &editEnd),
        Status::ERROR_UNSUPPORTED_FORMAT);

    SampleTableCursor cursor;
    FALSE_RETURN_V_NOLOG(CreateInitialCursor(cursor) == Status::OK, Status::ERROR_UNSUPPORTED_FORMAT);
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
        uint32_t rawIndex = static_cast<uint32_t>(cursor.processedRawSamples);
        SampleIndexView view;
        FALSE_RETURN_V_NOLOG(ReadRawSampleAndAdvance(cursor, view) == Status::OK,
            Status::ERROR_UNSUPPORTED_FORMAT);
        if (view.dts > searchDts) {
            break;
        }
        hasAny = true;
        lastAny = rawIndex;
        if (IsSyncSample(stssEntry_, allSamplesSync_, rawIndex) && view.pts <= edit.mediaTime) {
            hasSync = true;
            lastSync = rawIndex;
        }
    }
    uint32_t rawStart = hasSync ? lastSync : (hasAny ? lastAny : 0);

    cursor = SampleTableCursor {};
    FALSE_RETURN_V_NOLOG(CreateInitialCursor(cursor) == Status::OK, Status::ERROR_UNSUPPORTED_FORMAT);
    while (cursor.processedRawSamples < rawStart) {
        SampleIndexView ignored;
        FALSE_RETURN_V_NOLOG(ReadRawSampleAndAdvance(cursor, ignored) == Status::OK,
            Status::ERROR_UNSUPPORTED_FORMAT);
    }

    int64_t baseDts = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(-dtsShift_, emptyDuration, &baseDts),
        Status::ERROR_UNSUPPORTED_FORMAT);
    bool started = false;
    bool foundKeyframeAfterEdit = false;
    uint32_t rawEnd = rawStart;
    uint32_t firstPlayableRaw = rawStart;
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
    while (cursor.processedRawSamples < sampleTableSummary_.rawSampleCount) {
        uint32_t rawIndex = static_cast<uint32_t>(cursor.processedRawSamples);
        SampleIndexView view;
        FALSE_RETURN_V_NOLOG(ReadRawSampleAndAdvance(cursor, view) == Status::OK,
            Status::ERROR_UNSUPPORTED_FORMAT);
        if (rawIndex == rawStart) {
            rawStartDts = view.dts;
            rawStartPts = view.pts;
        }
        int64_t frameEnd = 0;
        FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(view.pts, view.duration, &frameEnd),
            Status::ERROR_UNSUPPORTED_FORMAT);
        bool outside = view.pts < edit.mediaTime || view.pts >= editEnd;
        bool crossedAudioStart = trackType_ == AUDIO_TYPE && view.pts < edit.mediaTime &&
            frameEnd >= edit.mediaTime;
        if (!started && (!outside || crossedAudioStart)) {
            started = true;
            firstPlayableRaw = rawIndex;
            rawDtsAtFirstPlayable = view.dts;
            int64_t skip = crossedAudioStart ? edit.mediaTime - view.pts : 0;
            FALSE_RETURN_V_NOLOG(skip >= 0 && skip <= UINT32_MAX &&
                !__builtin_sub_overflow(baseDts, skip, &anchorDts), Status::ERROR_UNSUPPORTED_FORMAT);
            if (crossedAudioStart && skip > 0) {
                hasSkipSamples = true;
                skipOutputIndex = rawIndex - rawStart;
                skipSamples = static_cast<uint32_t>(skip);
            }
        }
        if (!started) {
            continue;
        }
        int64_t rawPtsOffset = 0;
        int64_t rawDtsDelta = 0;
        int64_t outputDts = 0;
        int64_t correctedPts = 0;
        FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(view.pts, view.dts, &rawPtsOffset) &&
            !__builtin_sub_overflow(view.dts, rawDtsAtFirstPlayable, &rawDtsDelta) &&
            !__builtin_add_overflow(anchorDts, rawDtsDelta, &outputDts) &&
            !__builtin_add_overflow(outputDts, rawPtsOffset, &correctedPts),
            Status::ERROR_UNSUPPORTED_FORMAT);
        maxCorrectedPts = std::max(maxCorrectedPts, correctedPts);
        int64_t correctedPtsEnd = 0;
        FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(correctedPts, view.duration, &correctedPtsEnd),
            Status::ERROR_UNSUPPORTED_FORMAT);
        maxCorrectedPtsEnd = std::max(maxCorrectedPtsEnd, correctedPtsEnd);
        int64_t outputDtsEnd = 0;
        FALSE_RETURN_V_NOLOG(!__builtin_add_overflow(outputDts, view.duration, &outputDtsEnd),
            Status::ERROR_UNSUPPORTED_FORMAT);
        maxOutputDtsEnd = std::max(maxOutputDtsEnd, outputDtsEnd);
        if (!outside) {
            minCorrectedPts = std::min(minCorrectedPts, correctedPts);
        }
        rawEnd = rawIndex;
        if (frameEnd >= editEnd && (IsSyncSample(stssEntry_, allSamplesSync_, rawIndex) || trackType_ == AUDIO_TYPE)) {
            if (trackType_ == VIDEO_TYPE && !cttsEntry_.empty() && !foundKeyframeAfterEdit) {
                foundKeyframeAfterEdit = true;
                continue;
            }
            break;
        }
    }
    FALSE_RETURN_V_NOLOG(started && rawEnd >= rawStart && minCorrectedPts != INT64_MAX,
        Status::ERROR_UNSUPPORTED_FORMAT);
    int64_t correctionCandidate = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(minCorrectedPts, emptyDuration, &correctionCandidate),
        Status::ERROR_UNSUPPORTED_FORMAT);
    int64_t correction = std::max<int64_t>(0, correctionCandidate);
    int64_t dtsAdjustment = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(anchorDts, rawDtsAtFirstPlayable, &dtsAdjustment) &&
        !__builtin_sub_overflow(dtsAdjustment, correction, &dtsAdjustment), Status::ERROR_UNSUPPORTED_FORMAT);
    uint64_t outputCount = static_cast<uint64_t>(rawEnd) - rawStart + 1;
    FALSE_RETURN_V_NOLOG(outputCount <= UINT32_MAX, Status::ERROR_UNSUPPORTED_FORMAT);

    simpleEditProjection_ = SimpleEditProjection {
        true, rawStart, rawEnd, static_cast<uint32_t>(outputCount), firstPlayableRaw,
        dtsAdjustment, edit.mediaTime, editEnd, emptyDuration
    };
    sampleTableSummary_.logicalSampleCount = static_cast<uint32_t>(outputCount);
    int64_t firstPtsOffset = 0;
    FALSE_RETURN_V_NOLOG(!__builtin_sub_overflow(rawStartPts, rawStartDts, &firstPtsOffset) &&
        !__builtin_add_overflow(rawStartDts, dtsAdjustment, &sampleTableSummary_.firstDts) &&
        !__builtin_add_overflow(sampleTableSummary_.firstDts, firstPtsOffset, &sampleTableSummary_.firstPts) &&
        !__builtin_sub_overflow(maxOutputDtsEnd, correction, &sampleTableSummary_.maxDtsEnd) &&
        !__builtin_sub_overflow(maxCorrectedPtsEnd, correction, &sampleTableSummary_.maxPtsEnd) &&
        !__builtin_sub_overflow(maxCorrectedPts, correction, &lastPTS_) &&
        !__builtin_add_overflow(emptyDuration, editDuration, &totalDuration_),
        Status::ERROR_UNSUPPORTED_FORMAT);
    if (hasSkipSamples) {
        constexpr size_t skipSamplesInfoSize = 10;
        std::vector<uint8_t> info(skipSamplesInfoSize, 0);
        for (size_t i = 0; i < sizeof(uint32_t); ++i) {
            info[i] = static_cast<uint8_t>((skipSamples >> (8 * i)) & 0xFF);
        }
        skipSamplesInfoMap_[skipOutputIndex] = std::move(info);
    }
    return Status::OK;
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
    if (isFragmented_ && sttsEntry_.empty()) {
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
    if (sparseStatus_ == Status::OK) {
        if (simpleEditProjection_.enabled) {
            // Start sparse iteration at the projected raw sample while exposing output indices from zero.
            for (uint32_t i = 0; i < simpleEditProjection_.rawStart; ++i) {
                SampleIndexView ignored;
                sparseStatus_ = ReadRawSampleAndAdvance(sparseFrontier_, ignored);
                if (sparseStatus_ != Status::OK) {
                    return sparseStatus_;
                }
            }
            sparseFrontier_.outputIndex = 0;
        }
        sparseCheckpoints_.push_back(SparseCheckpoint {0, sparseFrontier_});
        sampleIndexState_.store(SampleIndexState::SPARSE, std::memory_order_release);
    }
    return sparseStatus_;
}

Status MPEG4SampleHelper::GetSparseSampleIndexView(uint32_t sampleIndex, SampleIndexView& view) const
{
    Status ret = EnsureSparseIndex();
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    FALSE_RETURN_V_NOLOG(sampleIndex < sampleTableSummary_.logicalSampleCount, Status::ERROR_INVALID_PARAMETER);
    std::lock_guard<std::mutex> lock(indexMutex_);
    if (sampleIndex >= sparseFrontier_.outputIndex) {
        while (sparseFrontier_.outputIndex <= sampleIndex) {
            uint32_t rawIndex = static_cast<uint32_t>(sparseFrontier_.processedRawSamples);
            if (sparseFrontier_.outputIndex != 0 &&
                sparseFrontier_.outputIndex % SPARSE_CHECKPOINT_INTERVAL == 0 &&
                sparseCheckpoints_.back().outputIndex != sparseFrontier_.outputIndex) {
                // Checkpoints bound random-access replay cost without storing every sample entry.
                sparseCheckpoints_.push_back(SparseCheckpoint {sparseFrontier_.outputIndex, sparseFrontier_});
            }
            ret = ReadRawSampleAndAdvance(sparseFrontier_, view);
            FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
            ret = ApplySimpleEditProjection(rawIndex, view);
            FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
        }
        return Status::OK;
    }
    auto it = std::upper_bound(sparseCheckpoints_.begin(), sparseCheckpoints_.end(), sampleIndex,
        [](uint32_t index, const SparseCheckpoint& checkpoint) { return index < checkpoint.outputIndex; });
    FALSE_RETURN_V_NOLOG(it != sparseCheckpoints_.begin(), Status::ERROR_INVALID_DATA);
    SampleTableCursor cursor = std::prev(it)->cursor;
    while (cursor.outputIndex <= sampleIndex) {
        uint32_t rawIndex = static_cast<uint32_t>(cursor.processedRawSamples);
        ret = ReadRawSampleAndAdvance(cursor, view);
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
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
