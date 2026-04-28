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
constexpr int64_t VTTE_SIZE = 8;
constexpr int64_t MAX_SAMPLES_PER_FRAME = 1024;
constexpr size_t MIN_DATA_SIZE = 8;
constexpr int64_t MIN_DATA_OFFSET = 0;
constexpr uint32_t STSZ_HEAD_LENGTH = 12;  // stsz头数据长度
constexpr uint32_t HALF_BYTE_BITS = 4;
constexpr uint32_t BYTE_BITS = 8;
constexpr uint32_t DUL_BYTE_BITS = 16;
    
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

Status MPEG4SampleHelper::FindWithSyncSamples(int64_t inputPTS, bool isBaseTrack, SeekMode flag, uint32_t &syncIndex)
{
    MEDIA_LOG_D("In");
    auto& entry = stssEntry_;
    auto it = std::lower_bound(entry.begin(), entry.end(), inputPTS,
        [](const StssEntry& entry, int64_t pts) { return entry.pts < pts; });
    syncIndex = std::distance(entry.begin(), it);
    if (isBaseTrack && flag == SeekMode::SEEK_PREVIOUS_SYNC && syncIndex == entry.size()) {
        syncIndex--;
    }
    if (syncIndex == entry.size()) {
        return Status::ERROR_UNKNOWN;
    } else if (inputPTS == entry[syncIndex].pts ||
        (inputPTS > entry[syncIndex].pts && flag == SeekMode::SEEK_PREVIOUS_SYNC) ||
        (inputPTS < entry[syncIndex].pts && flag == SeekMode::SEEK_NEXT_SYNC)) {
        return Status::OK;
    }
    return Status::ERROR_UNKNOWN;
}

Status MPEG4SampleHelper::CheckSyncSample(int64_t inputPTS, bool isBaseTrack, SeekMode flag, int64_t &currentIndex)
{
    MEDIA_LOG_D("In");
    const auto& entry = sampleIndexEntry_;
    uint32_t numSamples = entry.size();

    auto findPrevSync = [&entry, inputPTS](int64_t idx) -> int64_t {
        while (idx >= 0 && (entry[idx].flag != Sample::SYNC_FRAME || entry[idx].pts > inputPTS)) {
            --idx;
        }
        return idx;
    };
    auto findNextSync = [&entry, numSamples, inputPTS](int64_t idx) -> int64_t {
        while (idx < static_cast<int64_t>(numSamples) && (entry[idx].flag != Sample::SYNC_FRAME ||
            entry[idx].pts < inputPTS)) {
            ++idx;
        }
        return idx;
    };

    if (flag == SeekMode::SEEK_PREVIOUS_SYNC) {
        currentIndex = findPrevSync(currentIndex - 1);
        if (currentIndex < 0) {
            currentIndex = stssEntry_.empty() ? 0 : stssEntry_[0].index;
            if (inputPTS < entry[currentIndex].pts && isBaseTrack) {
                return Status::ERROR_INVALID_PARAMETER;
            }
        }
        return Status::OK;
    }

    if (flag == SeekMode::SEEK_NEXT_SYNC) {
        currentIndex = findNextSync(currentIndex);
        if (currentIndex == static_cast<int64_t>(numSamples) && isBaseTrack) {
            return Status::ERROR_INVALID_PARAMETER;
        }
        return Status::OK;
    }

    // SEEK_CLOSEST_SYNC
    int64_t prevSync = findPrevSync(currentIndex);
    int64_t nextSync = findNextSync(currentIndex);
    if (prevSync >= 0 && nextSync < static_cast<int64_t>(numSamples)) {
        currentIndex = GetClosestSync(entry[prevSync].pts, entry[nextSync].pts, inputPTS) ? nextSync : prevSync;
    } else if (prevSync < 0) {
        currentIndex = nextSync;
    } else {
        currentIndex = prevSync;
    }
    return Status::OK;
}

Status MPEG4SampleHelper::FindSyncSampleAtTime(int64_t inputPTS, bool isBaseTrack, SeekMode flag, uint32_t &sampleIndex)
{
    MEDIA_LOG_D("In");
    if (inputPTS < 0) {
        sampleIndex = stssEntry_.empty() ? 0 : stssEntry_[0].index;
        return Status::OK;
    }

    int64_t currentIndex = 0;
    const auto& entry = sampleIndexEntry_;
    Status ret = Status::OK;
    if (!stssEntry_.empty()) {
        uint32_t syncIndex = 0;
        ret = FindWithSyncSamples(inputPTS, isBaseTrack, flag, syncIndex);
        currentIndex = syncIndex < stssEntry_.size() ? stssEntry_[syncIndex].index : stssEntry_.back().index;
        if (ret == Status::OK) {
            sampleIndex = currentIndex;
            return ret;
        }
    } else {
        auto it = std::lower_bound(entry.begin(), entry.end(), inputPTS,
            [](const SampleIndexEntry& entry, int64_t pts) { return entry.pts < pts; });
        currentIndex = std::distance(entry.begin(), it);
    }

    if (entry[currentIndex].flag == Sample::SYNC_FRAME && inputPTS == entry[currentIndex].pts) {
        sampleIndex = static_cast<uint32_t>(currentIndex);
        return Status::OK;
    } else if (!isBaseTrack && (inputPTS > lastPTS_ || (!stssEntry_.empty() &&
        inputPTS > stssEntry_.back().pts && flag == SeekMode::SEEK_NEXT_SYNC))) {
        sampleIndex = sampleIndexEntry_.size();
        return Status::OK;
    } else if (mimeType_ == MimeType::TEXT_WEBVTT) {
        if (currentIndex > 0 && inputPTS < entry[currentIndex].pts) {
            sampleIndex = --currentIndex;
        }
        if (inputPTS > totalDuration_) {
            sampleIndex = ++currentIndex;
        }
        return Status::OK;
    }

    ret = CheckSyncSample(inputPTS, isBaseTrack, flag, currentIndex);
    if (ret == Status::OK) {
        sampleIndex = currentIndex;
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
    if (GetU32Value(header)) {
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
        sttsEntry_.emplace_back(SttsEntry{
            .count = GetU32Value(data.get() + i * sizeof(SttsEntry)),
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
    if (GetU32Value(header)) {
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
        cttsEntry_.emplace_back(CttsEntry {
            .count = GetU32Value(data.get() + i * sizeof(CttsEntry)),
            .delta = static_cast<int32_t>(GetU32Value(data.get() + 4 + i * sizeof(CttsEntry)))
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
            .pts = 0,
            .index = sampleIndex - 1
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

Status MPEG4SampleHelper::SetTrackFragmentHeaderParams(int64_t dataOffset, int64_t dataSize, uint32_t flag,
    int64_t moofOffset)
{
    uint32_t tfhdFlag = flag;
    uint32_t size = dataSize;
    auto data = std::make_unique<uint8_t[]>(size);
    Status ret = dataReader_->ReadUintData(dataOffset, data.get(), size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");

    uint32_t tfhdDataOffset = 0;
    if ((tfhdFlag & TrackFragmentHeaderInfo::TFHD_BASE_DATA_OFFSET) && seekable_ == Seekable::SEEKABLE) {
        tfhd_.baseDataOffset = GetU64Value(data.get());
        tfhdDataOffset += sizeof(uint64_t);
    } else {
        tfhd_.baseDataOffset = moofOffset;
    }

    if (tfhdFlag & TrackFragmentHeaderInfo::TFHD_STSD_ID) {
        tfhd_.sampleDescriptionIndex = GetU32Value(data.get() + tfhdDataOffset);
        tfhdDataOffset += sizeof(uint32_t);
    } else {
        tfhd_.sampleDescriptionIndex = trex_.defaultSampleDescriptionIndex;
    }
    
    if (tfhdFlag & TrackFragmentHeaderInfo::TFHD_DEFAULT_DURATION) {
        tfhd_.defaultSampleDuration = GetU32Value(data.get() + tfhdDataOffset);
        tfhdDataOffset += sizeof(uint32_t);
    } else {
        tfhd_.defaultSampleDuration = trex_.defaultSampleDuration;
    }

    if (tfhdFlag & TrackFragmentHeaderInfo::TFHD_DEFAULT_SIZE) {
        tfhd_.defaultSampleSize = GetU32Value(data.get() + tfhdDataOffset);
        tfhdDataOffset += sizeof(uint32_t);
    } else {
        tfhd_.defaultSampleSize = trex_.defaultSampleSize;
    }

    if (tfhdFlag & TrackFragmentHeaderInfo::TFHD_DEFAULT_FLAGS) {
        tfhd_.flags = GetU32Value(data.get() + tfhdDataOffset);
        tfhdDataOffset += sizeof(uint32_t);
    } else {
        tfhd_.flags = trex_.defaultSampleFlags;
    }
    hasTfhd_ = true;
    return Status::OK;
}

Status MPEG4SampleHelper::SetTrackFragmentDecodeTimeParams(int64_t dataOffset, int64_t dataSize)
{
    MEDIA_LOG_D("In");
    int64_t currentOffset = dataOffset;
    if (currentOffset < MIN_DATA_OFFSET || dataSize < MIN_DATA_SIZE) {
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

Status MPEG4SampleHelper::InitSampleInfo(SampleInfo& sampleInfo, uint32_t& bufferOffset, uint16_t trunFlag,
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

void MPEG4SampleHelper::UpdateSampleInfo(SampleInfo& sampleInfo, uint32_t& bufferOffset, uint16_t trunFlag,
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

void MPEG4SampleHelper::UpdateSampleEntryInfo(
    SampleInfo sampleInfo, int64_t& offset, int64_t& currentDts, bool isOverlap)
{
    SampleIndexEntry sample;
    sample.pos = offset;
    sample.size = sampleInfo.size;
    sample.pts = currentDts + sampleInfo.ctsDelta;
    sample.dts = currentDts;
    sample.duration = sampleInfo.duration;
    if (trackType_ == AUDIO_TYPE) {
        sample.flag |= Sample::SampleFlag::SYNC_FRAME;
    } else {
        sample.flag |= (sampleInfo.flag & (0x00010000 | 0x01000000)) ?
            Sample::SampleFlag::NONE : Sample::SampleFlag::SYNC_FRAME;
    }
    if (isOverlap) {
        sample.flag |= Sample::SampleFlag::DISCARD;
    }

    if (sampleIndexEntry_.empty()) {
        lastPTS_ = sample.pts;
        sampleIndexEntry_.emplace_back(std::move(sample));
        sttsEntry_.emplace_back(SttsEntry {
            .count = 1,
            .delta = sampleInfo.duration
        });
    } else {
        uint32_t sampleIndex = 0;
        auto it = std::upper_bound(sampleIndexEntry_.begin(), sampleIndexEntry_.end(), sample,
            [](const SampleIndexEntry& a, const SampleIndexEntry& b) { return a.pos < b.pos; });
        sampleIndex = std::distance(sampleIndexEntry_.begin(), it);
        lastPTS_ = std::max(lastPTS_, sample.pts);
        sampleIndexEntry_.insert(sampleIndexEntry_.begin() + sampleIndex, std::move(sample));

        uint32_t sttsIndex = sampleIndex - numMoovSamples_ + firstMoofSttsIndex_ - 1;
        sttsEntry_.insert(sttsEntry_.begin() + sttsIndex, SttsEntry {
            .count = 1,
            .delta = sampleInfo.duration
        });
    }
}

Status MPEG4SampleHelper::SetTrackFragmentRunParams(int64_t dataOffset, int64_t dataSize, int64_t sidxDts,
    int64_t &moofDts, int64_t &duration)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(hasTfhd_, Status::ERROR_INVALID_DATA, "tfhd does not exist");
    FALSE_RETURN_V_NOLOG(dataOffset >= MIN_DATA_OFFSET && dataSize >= MIN_DATA_SIZE, Status::ERROR_INVALID_PARAMETER);

    uint8_t header[8];
    auto ret = dataReader_->ReadUintData(dataOffset, header, sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");
    FALSE_RETURN_V_MSG_E(header[0] == 0, Status::ERROR_INVALID_DATA, "trun invalid version");
    uint16_t trunFlag = (header[1] << 0x10) + (header[2] << 0x08) + header[3];
    if ((trunFlag & TRUN_FIRST_SAMPLE_FLAGS) && (trunFlag & TRUN_SAMPLE_FLAGS)) {
        return Status::ERROR_INVALID_DATA;
    }

    uint32_t numSamples = GetU32Value(&header[4]);
    auto data = std::make_unique<uint8_t[]>(dataSize - sizeof(header));
    ret = dataReader_->ReadUintData(dataOffset + sizeof(uint64_t), data.get(), dataSize - sizeof(header));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");

    SampleInfo sampleInfo { 0, 0, 0, 0, 0, 0 };
    uint32_t bufferOffset = 0;
    uint32_t firstSampleFlag = 0;
    ret = InitSampleInfo(sampleInfo, bufferOffset, trunFlag, data.get(), firstSampleFlag);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);

    if (sampleInfo.byteLength) {
        if ((dataSize - sizeof(header) - bufferOffset) / sampleInfo.byteLength < numSamples) {
            return Status::ERROR_INVALID_DATA;
        }
    } else if (numSamples > MAX_TRUN_SAMPLES) {
        return Status::ERROR_INVALID_DATA;
    }

    int64_t offset = tfhd_.baseDataOffset + sampleInfo.dataOffset;
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

void MPEG4SampleHelper::GetSampleFlag(uint32_t sampleIndex, SampleIndexEntry &entry, uint32_t &syncIndex)
{
    if (trackType_ == VIDEO_TYPE || trackType_ == AUDIO_TYPE || trackType_ == TIMEDMETA_TYPE) {
        if (!stssEntry_.empty()) {
            if ((syncIndex < stssEntry_.size()) && (sampleIndex == stssEntry_[syncIndex].index)) {
                entry.flag |= Sample::SampleFlag::SYNC_FRAME;
                stssEntry_[syncIndex].pts = entry.pts;
                ++syncIndex;
            } else {
                entry.flag |= Sample::SampleFlag::NONE;
            }
        } else {
            entry.flag |= Sample::SampleFlag::SYNC_FRAME;
        }
    } else if (trackType_ == SUBTITLE_TYPE) {
        if (mimeType_ == MimeType::TEXT_WEBVTT) {
            if (entry.size == VTTE_SIZE) {
                entry.flag |= Sample::SampleFlag::DISCARD;
            } else {
                entry.flag |= Sample::SampleFlag::SYNC_FRAME;
            }
        }
    }
}

Status MPEG4SampleHelper::BuildSampleIndexEntries()
{
    MEDIA_LOG_D("Start");
    FALSE_RETURN_V_MSG_E(PreBuildProcessAndCheck() == Status::OK, Status::ERROR_INVALID_DATA, "PreCheck failed");
    int64_t currentOffset = chunkOffsets_[0];
    uint32_t chunkIndex = 0;
    uint32_t stscIndex = 0;
    uint32_t sampleIndex = 0;
    uint32_t syncIndex = 0;
    int64_t currentDts = -timeOffset_ - dtsShift_;
    uint32_t sampleInChunk = 0;
    uint32_t samplesPerFrame = 1;
    int64_t remainSamples = static_cast<int64_t>(stscEntry_[stscIndex].count);

    for (const auto& sttsEntry : sttsEntry_) {  // 遍历sttsEntry
        uint32_t sampleInSttsEntry = 0;
        while (sampleInSttsEntry < sttsEntry.count) {  // 根据sttsEntry.count进行循环
            int64_t ctsOffset = cttsEntry_.empty() ? 0 : cttsEntry_[sampleIndex].delta;
            sampleIndexEntry_.emplace_back(SampleIndexEntry {
                .dts = currentDts,
                .pts = currentDts + dtsShift_ + ctsOffset,
                .pos = currentOffset,
                .flag = Sample::NONE,
                .size = 0,
                .duration = 0
            });
            FALSE_RETURN_V_MSG_E(GetSampleSize(sampleIndex, samplesPerFrame, sampleIndexEntry_.back(), remainSamples),
                Status::ERROR_INVALID_DATA, "Sample size is too large");
            sampleIndexEntry_.back().duration = static_cast<int64_t>(sttsEntry.delta) * samplesPerFrame;
            GetSampleFlag(sampleIndex, sampleIndexEntry_.back(), syncIndex);

            lastPTS_ = std::max(lastPTS_, sampleIndexEntry_.back().pts);
            currentDts += sttsEntry.delta * samplesPerFrame;
            currentOffset += sampleIndexEntry_.back().size;
            FALSE_RETURN_V_MSG_E(currentOffset >= 0, Status::ERROR_INVALID_DATA, "Sample Offset is negative");
            ++sampleIndex;
            sampleInSttsEntry += samplesPerFrame;
            sampleInChunk += samplesPerFrame;
            if (sampleInChunk >= stscEntry_[stscIndex].count) {
                ++chunkIndex;
                sampleInChunk = 0;
                stscIndex = (chunkIndex == stscEntry_[stscIndex + 1].first && stscIndex < stscEntry_.size() - 1) ?
                    stscIndex + 1 : stscIndex;
                remainSamples = stscEntry_[stscIndex].count;
                currentOffset = chunkOffsets_[chunkIndex];
            }
        }
    }

    totalDuration_ = 0;
    for (const auto& sttsEntry : sttsEntry_) {
        totalDuration_ += static_cast<int64_t>(sttsEntry.count) * static_cast<int64_t>(sttsEntry.delta);
    }
    return Status::OK;
}

Status MPEG4SampleHelper::GetSampleDuration(uint32_t index, uint32_t &duration)
{
    if (sampleIndexEntry_.empty() || sttsEntry_.empty()) {
        return Status::ERROR_INVALID_OPERATION;
    }

    if (firstMoofSampleIndex_ > 0) {
        if (index < firstMoofSampleIndex_) {
            uint32_t sttsIndex = 0;
            while (index > 0 && sttsIndex < sttsEntry_.size()) {
                index -= sttsEntry_[sttsIndex].count;
                ++sttsIndex;
            }
            duration = sttsEntry_[sttsIndex].delta;
        } else {
            uint32_t sttsIndex = index - numMoovSamples_ + firstMoofSttsIndex_ - 1;
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
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
