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

#include "pts_and_index_conversion.h"

#include <algorithm>
#include <string>
#include "netinet/in.h"
#include "avcodec_trace.h"
#include "securec.h"
#include "common/log.h"
#include "meta/video_types.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "TimeAndIndexConversion" };
}

namespace OHOS {
namespace Media {
const uint32_t BOX_HEAD_SIZE = 8;
const uint32_t PTS_AND_INDEX_CONVERSION_MAX_FRAMES = 36000;
const uint32_t BOX_HEAD_LARGE_SIZE = 16;
constexpr size_t UINT32_BYTES = sizeof(uint32_t);
constexpr size_t UINT32_BITS = sizeof(uint32_t) * 8;
TimeAndIndexConversion::TimeAndIndexConversion()
    : source_(std::make_shared<Source>())
{
};

TimeAndIndexConversion::~TimeAndIndexConversion()
{
};

Status TimeAndIndexConversion::SetDataSource(const std::shared_ptr<MediaSource>& source)
{
    MediaAVCodec::AVCODEC_SYNC_TRACE;
    MEDIA_LOG_I("In");
    FALSE_RETURN_V_MSG_E(source_ != nullptr, Status::ERROR_NULL_POINTER, "The source_ is nullptr");
    auto res = source_->SetSource(source);
    FALSE_RETURN_V_MSG_E(res == Status::OK, res, "Set source failed");
    Status ret = source_->GetSize(mediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Get file size failed");

    if (!IsMP4orMOV()) {
        MEDIA_LOG_E("Not a valid MP4 or MOV file");
        return Status::ERROR_UNSUPPORTED_FORMAT;
    } else {
        MEDIA_LOG_D("It is a MP4 or MOV file");
        StartParse();
        return Status::OK;
    }
};

Status TimeAndIndexConversion::GetFirstVideoTrackIndex(uint32_t &trackIndex)
{
    for (auto trakInfo : trakInfoVec_) {
        if (trakInfo.trakType == TrakType::TRAK_VIDIO) {
            trackIndex = trakInfo.trakId;
            return Status::OK;
        }
    }
    return Status::ERROR_INVALID_DATA;
}

void TimeAndIndexConversion::ReadBufferFromDataSource(size_t bufSize, std::shared_ptr<Buffer> &buffer)
{
    FALSE_RETURN_MSG(buffer != nullptr, "Buffer is nullptr");
    auto result = source_->SeekTo(offset_);
    if (result != Status::OK) {
        MEDIA_LOG_E("Seek to " PUBLIC_LOG_U64 " fail", offset_);
        buffer = nullptr;
        return;
    }
    result = source_->Read(0, buffer, offset_, bufSize);
    if (result != Status::OK) {
        MEDIA_LOG_E("Buffer read error");
        buffer = nullptr;
        return;
    }
}

void TimeAndIndexConversion::StartParse()
{
    source_->GetSize(fileSize_);
    MEDIA_LOG_I("fileSize: " PUBLIC_LOG_D64, fileSize_);
    while (offset_ < fileSize_) {
        int bufSize = sizeof(uint32_t) + sizeof(uint32_t);
        auto buffer = std::make_shared<Buffer>();
        FALSE_RETURN_MSG(buffer != nullptr, "StartParse failed due to read buffer error");
        std::vector<uint8_t> buff(bufSize);
        auto bufData = buffer->WrapMemory(buff.data(), bufSize, bufSize);
        ReadBufferFromDataSource(bufSize, buffer);
        FALSE_RETURN_MSG(buffer != nullptr, "StartParse failed due to read buffer error");
        BoxHeader header{0};
        ReadBoxHeader(buffer, header);
        FALSE_RETURN_MSG(header.size >= 0, "StartParse failed due to error box size");
        uint64_t boxSize = static_cast<uint64_t>(header.size);
        uint32_t headerSize = BOX_HEAD_SIZE;
        if (boxSize == 1 || boxSize == 0) { // 0 and 1 are used to verify whether there is a large size
            uint64_t largeSize = 0;
            ReadLargeSize(buffer, largeSize);
            boxSize = largeSize;
            headerSize = BOX_HEAD_LARGE_SIZE;
        }
        FALSE_RETURN_MSG(boxSize >= headerSize, "StartParse failed due to error box size");
        if (strncmp(header.type, BOX_TYPE_MOOV, sizeof(header.type)) == 0) {
            offset_ += headerSize;
            ParseMoov(boxSize - headerSize);
        } else {
            offset_ += boxSize;
        }
    }
}

void TimeAndIndexConversion::ReadLargeSize(std::shared_ptr<Buffer> buffer, uint64_t &largeSize)
{
    offset_ += BOX_HEAD_SIZE;
    FALSE_RETURN_MSG(buffer != nullptr, "ReadLargeSize failed due to read buffer error");
    int bufSize = sizeof(uint64_t); // The type of largeSize is uint64_t
    ReadBufferFromDataSource(bufSize, buffer);
    FALSE_RETURN_MSG(buffer != nullptr, "ReadLargeSize failed due to read buffer error");
    auto memory = buffer->GetMemory();
    FALSE_RETURN_MSG(memory != nullptr, "No memory in buffer");
    const uint8_t* ptr = memory->GetReadOnlyData();
    FALSE_RETURN_MSG(ptr != nullptr, "ReadLargeSize failed due to nullptr");
    size_t size = memory->GetSize();
    FALSE_RETURN_MSG(size >= sizeof(uint64_t), "Not enough data in buffer to read large size");
    uint32_t high = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
    uint32_t low  = ntohl(*reinterpret_cast<const uint32_t*>(ptr + UINT32_BYTES));
    largeSize = (static_cast<uint64_t>(high) << UINT32_BITS) | low;
    offset_ -= BOX_HEAD_SIZE;
}

void TimeAndIndexConversion::ReadBoxHeader(std::shared_ptr<Buffer> buffer, BoxHeader &header)
{
    auto memory = buffer->GetMemory();
    FALSE_RETURN_MSG(memory != nullptr, "No memory in buffer");
    const uint8_t* ptr = memory->GetReadOnlyData();
    FALSE_RETURN_MSG(ptr != nullptr, "ReadBoxHeader failed due to nullptr");
    size_t size = memory->GetSize();
    FALSE_RETURN_MSG(size >= sizeof(header.size) + 4,  // 4 is used to check data
        "Not enough data in buffer to read BoxHeader");
    header.size = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
    header.type[0] = ptr[sizeof(header.size)]; // Get the 1st character of the header
    header.type[1] = ptr[sizeof(header.size) + 1]; // Get the 2nd character of the header
    header.type[2] = ptr[sizeof(header.size) + 2]; // Get the 3rd character of the header
    header.type[3] = ptr[sizeof(header.size) + 3]; // Get the 4th character of the header
    header.type[4] = '\0'; // Supplement string tail
}

bool TimeAndIndexConversion::IsMP4orMOV()
{
    int bufSize = sizeof(uint32_t) + sizeof(uint32_t);
    auto buffer = std::make_shared<Buffer>();
    FALSE_RETURN_V_MSG_E(buffer != nullptr, false, "IsMP4orMOV failed due to read buffer error");
    std::vector<uint8_t> buff(bufSize);
    auto bufData = buffer->WrapMemory(buff.data(), bufSize, bufSize);
    ReadBufferFromDataSource(bufSize, buffer);
    FALSE_RETURN_V_MSG_E(buffer != nullptr, false, "IsMP4orMOV failed due to read buffer error");
    BoxHeader header{0};
    ReadBoxHeader(buffer, header);
    offset_ = 0; // init offset_
    return strncmp(header.type, BOX_TYPE_FTYP, sizeof(header.type)) == 0;
}

void TimeAndIndexConversion::ParseMoov(uint32_t boxSize)
{
    uint64_t parentSize = offset_ + static_cast<uint64_t>(boxSize);
    while (offset_ < parentSize) {
        int bufSize = sizeof(uint32_t) + sizeof(uint32_t);
        auto buffer = std::make_shared<Buffer>();
        FALSE_RETURN_MSG(buffer != nullptr, "ParseMoov failed due to read buffer error");
        std::vector<uint8_t> buff(bufSize);
        auto bufData = buffer->WrapMemory(buff.data(), bufSize, bufSize);
        ReadBufferFromDataSource(bufSize, buffer);
        FALSE_RETURN_MSG(buffer != nullptr, "ParseMoov failed due to read buffer error");
        BoxHeader header{0};
        ReadBoxHeader(buffer, header);
        FALSE_RETURN_MSG(header.size >= 0, "ParseMoov failed due to error box size");
        uint64_t childBoxSize = static_cast<uint64_t>(header.size);
        uint32_t headerSize = BOX_HEAD_SIZE;
        if (childBoxSize == 1 || childBoxSize == 0) { // 0 and 1 are used to verify whether there is a large size
            uint64_t largeSize = 0;
            ReadLargeSize(buffer, largeSize);
            childBoxSize = largeSize;
            headerSize = BOX_HEAD_LARGE_SIZE;
        }
        FALSE_RETURN_MSG(childBoxSize >= headerSize, "ParseMoov failed due to error box size");
        if (strncmp(header.type, BOX_TYPE_TRAK, sizeof(header.type)) == 0) {
            offset_ += headerSize;
            ParseTrak(childBoxSize - headerSize);
        } else {
            offset_ += childBoxSize;
        }
    }
}

void TimeAndIndexConversion::ParseTrak(uint32_t boxSize)
{
    MEDIA_LOG_D("curTrakInfoIndex_: " PUBLIC_LOG_D32, curTrakInfoIndex_);
    curTrakInfo_.trakId = curTrakInfoIndex_;
    ParseBox(boxSize - BOX_HEAD_SIZE);
    trakInfoVec_.push_back(curTrakInfo_);
    curTrakInfo_.cttsEntries.clear();
    curTrakInfo_.sttsEntries.clear();
    curTrakInfoIndex_++;
}

void TimeAndIndexConversion::ParseBox(uint32_t boxSize)
{
    uint64_t parentSize = offset_ + static_cast<uint64_t>(boxSize);
    while (offset_ < parentSize) {
        int bufSize = sizeof(uint32_t) + sizeof(uint32_t);
        auto buffer = std::make_shared<Buffer>();
        FALSE_RETURN_MSG(buffer != nullptr, "ParseBox failed due to read buffer error");
        std::vector<uint8_t> buff(bufSize);
        auto bufData = buffer->WrapMemory(buff.data(), bufSize, bufSize);
        ReadBufferFromDataSource(bufSize, buffer);
        FALSE_RETURN_MSG(buffer != nullptr, "ParseBox failed due to read buffer error");
        BoxHeader header{0};
        ReadBoxHeader(buffer, header);
        FALSE_RETURN_MSG(header.size >= 0, "ParseBox failed due to error box size");
        uint64_t childBoxSize = static_cast<uint64_t>(header.size);
        uint32_t headerSize = BOX_HEAD_SIZE;
        if (childBoxSize == 1 || childBoxSize == 0) { // 0 and 1 are used to verify whether there is a large size
            uint64_t largeSize = 0;
            ReadLargeSize(buffer, largeSize);
            childBoxSize = largeSize;
            headerSize = BOX_HEAD_LARGE_SIZE;
        }
        FALSE_RETURN_MSG(childBoxSize >= headerSize, "ParseBox failed due to error box size");
        auto it = boxParsers.find(std::string(header.type));
        if (it != boxParsers.end()) {
            offset_ += headerSize;
            (this->*(it->second))(childBoxSize - headerSize);
        } else {
            offset_ += childBoxSize;
        }
    }
}

void TimeAndIndexConversion::ParseCtts(uint32_t boxSize)
{
    auto buffer = std::make_shared<Buffer>();
    std::vector<uint8_t> buff(boxSize);
    auto bufData = buffer->WrapMemory(buff.data(), boxSize, boxSize);
    auto result = source_->Read(0, buffer, offset_, boxSize);
    FALSE_RETURN_MSG(result == Status::OK, "ParseCtts failed due to read buffer error");
    auto memory = buffer->GetMemory();
    FALSE_RETURN_MSG(memory != nullptr, "ParseCtts failed due to no memory in buffer");
    const uint8_t* ptr = memory->GetReadOnlyData();
    FALSE_RETURN_MSG(ptr != nullptr, "ParseCtts failed due to nullptr");
    size_t size = memory->GetSize();
    if (size < sizeof(uint32_t) * 2) { // 2 is used to check data
        MEDIA_LOG_E("Not enough data in buffer to read CTTS header");
        return;
    }
    // read versionAndFlags and entryCount
    uint32_t versionAndFlags = *reinterpret_cast<const uint32_t*>(ptr);
    MEDIA_LOG_D("versionAndFlags: " PUBLIC_LOG_D32, versionAndFlags);
    uint32_t entryCount = *reinterpret_cast<const uint32_t*>(ptr + sizeof(uint32_t));
    entryCount = ntohl(entryCount);
    // Check if the remaining data is sufficient
    if (size < sizeof(uint32_t) * 2 + entryCount * sizeof(CTTSEntry)) { // 2 is used to check data
        return;
    }
    std::vector<CTTSEntry> entries(entryCount);
    const uint8_t* entryPtr = ptr + sizeof(uint32_t) * 2; // 2 is used to skip versionAndFlags and entryCount
    for (uint32_t i = 0; i < entryCount; ++i) {
        entries[i].sampleCount = ntohl(*reinterpret_cast<const uint32_t*>(entryPtr));
        entries[i].sampleOffset = static_cast<int32_t>(ntohl(*reinterpret_cast<const uint32_t*>(entryPtr +
                                  sizeof(uint32_t))));
        entryPtr += sizeof(CTTSEntry);
    }
    curTrakInfo_.cttsEntries = entries;
    offset_ += static_cast<uint64_t>(boxSize);
}

void TimeAndIndexConversion::ParseStts(uint32_t boxSize)
{
    auto buffer = std::make_shared<Buffer>();
    std::vector<uint8_t> buff(boxSize);
    auto bufData = buffer->WrapMemory(buff.data(), boxSize, boxSize);
    auto result = source_->Read(0, buffer, offset_, boxSize);
    FALSE_RETURN_MSG(result == Status::OK, "ParseStts failed due to read buffer error");
    auto memory = buffer->GetMemory();
    FALSE_RETURN_MSG(memory != nullptr, "ParseStts failed due to no memory in buffer");
    const uint8_t* ptr = memory->GetReadOnlyData();
    FALSE_RETURN_MSG(ptr != nullptr, "ParseStts failed due to nullptr");
    size_t size = memory->GetSize();
    if (size < sizeof(uint32_t) * 2) { // 2 is used to check data
        MEDIA_LOG_E("Not enough data in buffer to read STTS header");
        return;
    }
    // read versionAndFlags and entryCount
    uint32_t versionAndFlags = *reinterpret_cast<const uint32_t*>(ptr);
    MEDIA_LOG_D("versionAndFlags: " PUBLIC_LOG_D32, versionAndFlags);
    uint32_t entryCount = *reinterpret_cast<const uint32_t*>(ptr + sizeof(uint32_t));
    entryCount = ntohl(entryCount);
    // Check if the remaining data is sufficient
    if (size < sizeof(uint32_t) * 2 + entryCount * sizeof(STTSEntry)) { // 2 is used to check data
        return;
    }
    std::vector<STTSEntry> entries(entryCount);
    const uint8_t* entryPtr = ptr + sizeof(uint32_t) * 2; // 2 is used to skip versionAndFlags and entryCount
    for (uint32_t i = 0; i < entryCount; ++i) {
        entries[i].sampleCount = ntohl(*reinterpret_cast<const uint32_t*>(entryPtr));
        entries[i].sampleDelta = ntohl(*reinterpret_cast<const uint32_t*>(entryPtr + sizeof(uint32_t)));
        entryPtr += sizeof(STTSEntry);
    }
    curTrakInfo_.sttsEntries = entries;
    offset_ += static_cast<uint64_t>(boxSize);
}

void TimeAndIndexConversion::ParseHdlr(uint32_t boxSize)
{
    auto buffer = std::make_shared<Buffer>();
    std::vector<uint8_t> buff(boxSize);
    auto bufData = buffer->WrapMemory(buff.data(), boxSize, boxSize);
    auto result = source_->Read(0, buffer, offset_, boxSize);
    FALSE_RETURN_MSG(result == Status::OK, "ParseHdlr failed due to read buffer error");
    auto memory = buffer->GetMemory();
    FALSE_RETURN_MSG(memory != nullptr, "ParseHdlr failed due to no memory in buffer");
    const uint8_t* ptr = memory->GetReadOnlyData();
    FALSE_RETURN_MSG(ptr != nullptr, "ParseHdlr failed due to nullptr");
    size_t size = memory->GetSize();
    if (size < sizeof(uint32_t) * 3) { // 3 is versionAndFlags + entryCount + handlerType
        MEDIA_LOG_E("Not enough data in buffer to read HDLR header");
        return;
    }
    // skip versionAndFlags
    ptr += sizeof(uint32_t);
    // skip reDefined
    ptr += sizeof(uint32_t);
    // read handlerType
    std::string handlerType = "";
    handlerType.append(std::string(1, static_cast<char>(ptr[0]))); // Get the 1st character of the handlerType
    handlerType.append(std::string(1, static_cast<char>(ptr[1]))); // Get the 2nd character of the handlerType
    handlerType.append(std::string(1, static_cast<char>(ptr[2]))); // Get the 3rd character of the handlerType
    handlerType.append(std::string(1, static_cast<char>(ptr[3]))); // Get the 4th character of the handlerType
    if (handlerType == "soun") {
        curTrakInfo_.trakType = TrakType::TRAK_AUDIO;
    } else if (handlerType == "vide") {
        curTrakInfo_.trakType = TrakType::TRAK_VIDIO;
    } else {
        curTrakInfo_.trakType = TrakType::TRAK_OTHER;
    }
    offset_ += static_cast<uint64_t>(boxSize);
}

void TimeAndIndexConversion::ParseMdhd(uint32_t boxSize)
{
    auto buffer = std::make_shared<Buffer>();
    std::vector<uint8_t> buff(boxSize);
    auto bufData = buffer->WrapMemory(buff.data(), boxSize, boxSize);
    auto result = source_->Read(0, buffer, offset_, boxSize);
    FALSE_RETURN_MSG(result == Status::OK, "ParseMdhd failed due to read buffer error");
    auto memory = buffer->GetMemory();
    FALSE_RETURN_MSG(memory != nullptr, "ParseMdhd failed due to no memory in buffer");
    const uint8_t* ptr = memory->GetReadOnlyData();
    FALSE_RETURN_MSG(ptr != nullptr, "ParseMdhd failed due to nullptr");
    size_t size = memory->GetSize();

    // mdhd(full box): version(1)+flags(3) + creation/modification + timescale + duration
    // version 0: creation/modification/timescale/duration are uint32
    // version 1: creation/modification/duration are uint64, timescale is uint32
    FALSE_RETURN_MSG(size >= sizeof(uint32_t), "Not enough data in buffer to read MDHD version");
    uint8_t version = ptr[0];
    size_t timeScaleOffset = 0;
    size_t minBytesForTimeScale = 0;
    if (version == 1) {
        timeScaleOffset = sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t);
        minBytesForTimeScale = timeScaleOffset + sizeof(uint32_t);
    } else {
        timeScaleOffset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
        minBytesForTimeScale = timeScaleOffset + sizeof(uint32_t);
    }
    if (size < minBytesForTimeScale) {
        MEDIA_LOG_E("Not enough data in buffer to read MDHD timeScale, size: " PUBLIC_LOG_ZU ", need: "
            PUBLIC_LOG_ZU, size, minBytesForTimeScale);
        return;
    }

    // 读取versionAndFlags/ timeScale
    uint32_t versionAndFlags = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
    MEDIA_LOG_D("versionAndFlags: " PUBLIC_LOG_D32, versionAndFlags);
    uint32_t timeScale = *reinterpret_cast<const uint32_t*>(ptr + timeScaleOffset);
    timeScale = ntohl(timeScale);
    MEDIA_LOG_D("timeScale: " PUBLIC_LOG_D32, timeScale);
    curTrakInfo_.timeScale = timeScale;
    offset_ += static_cast<uint64_t>(boxSize);
}

void TimeAndIndexConversion::InitPTSandIndexConvert()
{
    indexToRelativePTSFrameCount_ = 0; // init IndexToRelativePTSFrameCount_
    relativePTSToIndexPosition_ = 0; // init RelativePTSToIndexPosition_
    indexToRelativePTSMaxHeap_ = std::priority_queue<int64_t>(); // init IndexToRelativePTSMaxHeap_
    relativePTSToIndexPTSMin_ = INT64_MAX;
    relativePTSToIndexPTSMax_ = INT64_MIN;
    relativePTSToIndexRightDiff_ = INT64_MAX;
    relativePTSToIndexLeftDiff_ = INT64_MAX;
    relativePTSToIndexTempDiff_ = INT64_MAX;
}

bool TimeAndIndexConversion::IsWithinPTSAndIndexConversionMaxFrames(uint32_t trackIndex)
{
    uint32_t frames = 0;
    for (auto sttsEntry : trakInfoVec_[trackIndex].sttsEntries) {
        FALSE_RETURN_V_MSG_E(frames <= UINT32_MAX - sttsEntry.sampleCount, false, "Frame count exceeds limit");
        frames += sttsEntry.sampleCount;
        FALSE_RETURN_V_MSG_E(frames <= PTS_AND_INDEX_CONVERSION_MAX_FRAMES, false, "Frame count exceeds limit");
    }
    return true;
}

Status TimeAndIndexConversion::GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
    const uint64_t relativePresentationTimeUs, uint32_t &index)
{
    FALSE_RETURN_V_MSG_E(trackIndex < trakInfoVec_.size(), Status::ERROR_INVALID_DATA, "Track is out of range");
    bool frameCheck = IsWithinPTSAndIndexConversionMaxFrames(trackIndex);
    FALSE_RETURN_V_MSG_E(frameCheck, Status::ERROR_INVALID_DATA, "Frame count exceeds limit");
    InitPTSandIndexConvert();
    Status ret = GetPresentationTimeUsFromFfmpegMOV(GET_FIRST_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Get pts failed");

    int64_t absolutePTS = static_cast<int64_t>(relativePresentationTimeUs) + absolutePTSIndexZero_;

    ret = GetPresentationTimeUsFromFfmpegMOV(RELATIVEPTS_TO_INDEX, trackIndex,
        absolutePTS, index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Get pts failed");

    if (absolutePTS < relativePTSToIndexPTSMin_ || absolutePTS > relativePTSToIndexPTSMax_) {
        MEDIA_LOG_E("Pts is out of range");
        return Status::ERROR_INVALID_DATA;
    }

    if (relativePTSToIndexLeftDiff_ == 0 || relativePTSToIndexRightDiff_ == 0) {
        index = relativePTSToIndexPosition_;
    } else {
        index = relativePTSToIndexLeftDiff_ < relativePTSToIndexRightDiff_ ?
        relativePTSToIndexPosition_ - 1 : relativePTSToIndexPosition_;
    }
    return Status::OK;
}

Status TimeAndIndexConversion::GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
    const uint32_t index, uint64_t &relativePresentationTimeUs)
{
    FALSE_RETURN_V_MSG_E(trackIndex < trakInfoVec_.size(), Status::ERROR_INVALID_DATA, "Track is out of range");
    FALSE_RETURN_V_MSG_E(index < UINT32_MAX, Status::ERROR_INVALID_DATA, "Index is out of range");
    bool frameCheck = IsWithinPTSAndIndexConversionMaxFrames(trackIndex);
    FALSE_RETURN_V_MSG_E(frameCheck, Status::ERROR_INVALID_DATA, "Frame count exceeds limit");
    InitPTSandIndexConvert();
    Status ret = GetPresentationTimeUsFromFfmpegMOV(GET_FIRST_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Get pts failed");

    ret = GetPresentationTimeUsFromFfmpegMOV(INDEX_TO_RELATIVEPTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Get pts failed");

    if (index + 1 > indexToRelativePTSFrameCount_) {
        MEDIA_LOG_E("Index is out of range");
        return Status::ERROR_INVALID_DATA;
    }

    int64_t relativepts = indexToRelativePTSMaxHeap_.top() - absolutePTSIndexZero_;
    FALSE_RETURN_V_MSG_E(relativepts >= 0, Status::ERROR_INVALID_DATA, "Existence of calculation results less than 0");
    relativePresentationTimeUs = static_cast<uint64_t>(relativepts);

    return Status::OK;
}

Status TimeAndIndexConversion::PTSAndIndexConvertSttsAndCttsProcess(IndexAndPTSConvertMode mode,
    int64_t absolutePTS, uint32_t index)
{
    uint32_t sttsIndex = 0;
    uint32_t cttsIndex = 0;
    int64_t pts = 0; // init pts
    int64_t dts = 0; // init dts
    uint32_t sttsCount = trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries.size();
    uint32_t cttsCount = trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries.size();
    uint32_t sttsCurNum = trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleCount;
    uint32_t cttsCurNum = trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries[cttsIndex].sampleCount;
    while (sttsIndex < sttsCount && cttsIndex < cttsCount) {
        if (cttsCurNum == 0) {
            cttsIndex++;
            if (cttsIndex >= cttsCount) {
                break;
            }
            cttsCurNum = trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries[cttsIndex].sampleCount;
        }
        if (cttsCurNum == 0) {
            break;
        }
        cttsCurNum--;
        if ((INT64_MAX / 1000 / 1000) < // 1000 is used for converting pts to us
            ((dts + static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries[cttsIndex].sampleOffset)) /
            static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].timeScale))) {
                MEDIA_LOG_E("pts overflow");
                return Status::ERROR_INVALID_DATA;
        }
        double timeScaleRate = 1000 * 1000 / // 1000 is used for converting pts to us
                                static_cast<double>(trakInfoVec_[curConvertTrakInfoIndex_].timeScale);
        double ptsTemp = static_cast<double>(dts) +
            static_cast<double>(trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries[cttsIndex].sampleOffset);
        pts = static_cast<int64_t>(ptsTemp * timeScaleRate);
        PTSAndIndexConvertSwitchProcess(mode, pts, absolutePTS, index);
        if (sttsCurNum == 0) {
            break;
        }
        sttsCurNum--;
        dts += static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleDelta);
        if (sttsCurNum == 0) {
            sttsIndex++;
            sttsCurNum = sttsIndex < sttsCount ?
                         trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleCount : 0;
        }
    }
    return Status::OK;
}

Status TimeAndIndexConversion::PTSAndIndexConvertOnlySttsProcess(IndexAndPTSConvertMode mode,
    int64_t absolutePTS, uint32_t index)
{
    uint32_t sttsIndex = 0;
    int64_t pts = 0; // init pts
    int64_t dts = 0; // init dts
    uint32_t sttsCount = trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries.size();
    uint32_t sttsCurNum = trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleCount;

    while (sttsIndex < sttsCount) {
        if ((INT64_MAX / 1000 / 1000) < // 1000 is used for converting pts to us
            (dts / static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].timeScale))) {
                MEDIA_LOG_E("pts overflow");
                return Status::ERROR_INVALID_DATA;
        }
        double timeScaleRate = 1000 * 1000 / // 1000 is used for converting pts to us
                                static_cast<double>(trakInfoVec_[curConvertTrakInfoIndex_].timeScale);
        double ptsTemp = static_cast<double>(dts);
        pts = static_cast<int64_t>(ptsTemp * timeScaleRate);
        PTSAndIndexConvertSwitchProcess(mode, pts, absolutePTS, index);
        if (sttsCurNum == 0) {
            break;
        }
        sttsCurNum--;
        if ((INT64_MAX - dts) <
            (static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleDelta))) {
            MEDIA_LOG_E("dts overflow");
            return Status::ERROR_INVALID_DATA;
        }
        dts += static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleDelta);
        if (sttsCurNum == 0) {
            sttsIndex++;
            sttsCurNum = sttsIndex < sttsCount ?
                         trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleCount : 0;
        }
    }
    return Status::OK;
}

Status TimeAndIndexConversion::GetPresentationTimeUsFromFfmpegMOV(IndexAndPTSConvertMode mode,
    uint32_t trackIndex, int64_t absolutePTS, uint32_t index)
{
    curConvertTrakInfoIndex_ = trackIndex;
    FALSE_RETURN_V_MSG_E(trakInfoVec_[curConvertTrakInfoIndex_].timeScale != 0,
                         Status::ERROR_INVALID_DATA, "timeScale_ is zero");
    FALSE_RETURN_V_MSG_E(trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries.size(),
        Status::ERROR_INVALID_DATA, "PTS is empty");
    return !trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries.size() ?
        PTSAndIndexConvertOnlySttsProcess(mode, absolutePTS, index) :
        PTSAndIndexConvertSttsAndCttsProcess(mode, absolutePTS, index);
}

void TimeAndIndexConversion::PTSAndIndexConvertSwitchProcess(IndexAndPTSConvertMode mode,
    int64_t pts, int64_t absolutePTS, uint32_t index)
{
    switch (mode) {
        case GET_FIRST_PTS:
            absolutePTSIndexZero_ = pts < absolutePTSIndexZero_ ? pts : absolutePTSIndexZero_;
            break;
        case INDEX_TO_RELATIVEPTS:
            IndexToRelativePTSProcess(pts, index);
            break;
        case RELATIVEPTS_TO_INDEX:
            RelativePTSToIndexProcess(pts, absolutePTS);
            break;
        default:
            MEDIA_LOG_E("Wrong mode");
            break;
    }
}

void TimeAndIndexConversion::IndexToRelativePTSProcess(int64_t pts, uint32_t index)
{
    if (indexToRelativePTSMaxHeap_.size() < index + 1) {
        indexToRelativePTSMaxHeap_.push(pts);
    } else {
        if (pts < indexToRelativePTSMaxHeap_.top()) {
            indexToRelativePTSMaxHeap_.pop();
            indexToRelativePTSMaxHeap_.push(pts);
        }
    }
    indexToRelativePTSFrameCount_++;
}

void TimeAndIndexConversion::RelativePTSToIndexProcess(int64_t pts, int64_t absolutePTS)
{
    if (relativePTSToIndexPTSMin_ > pts) {
        relativePTSToIndexPTSMin_ = pts;
    }
    if (relativePTSToIndexPTSMax_ < pts) {
        relativePTSToIndexPTSMax_ = pts;
    }
    relativePTSToIndexTempDiff_ = abs(pts - absolutePTS);
    if (pts < absolutePTS && relativePTSToIndexTempDiff_ < relativePTSToIndexLeftDiff_) {
        relativePTSToIndexLeftDiff_ = relativePTSToIndexTempDiff_;
    }
    if (pts >= absolutePTS && relativePTSToIndexTempDiff_ < relativePTSToIndexRightDiff_) {
        relativePTSToIndexRightDiff_ = relativePTSToIndexTempDiff_;
    }
    if (pts < absolutePTS) {
        relativePTSToIndexPosition_++;
    }
}
} // namespace Media
} // namespace OHOS