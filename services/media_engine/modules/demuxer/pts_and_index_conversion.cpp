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

#include <unistd.h>
#include <algorithm>
#include <malloc.h>
#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <limits>
#include "netinet/in.h"
#include "avcodec_trace.h"
#include "securec.h"
#include "common/log.h"
#include "meta/video_types.h"
#include "meta/format.h"
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "TimeAndIndexConversion" };
}

namespace OHOS {
namespace Media {
namespace TimeAndIndex {
const uint32_t BOX_HEAD_SIZE = 8;
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
    auto res = source_->SetSource(source);
    FALSE_RETURN_V_MSG_E(res == Status::OK, res, "Set source failed");
    Status ret = source_->GetSize(mediaDataSize_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Get file size failed");

    streamDemuxer_ = std::make_shared<StreamDemuxer>();
    streamDemuxer_->SetSource(source_);
    streamDemuxer_->Init(uri_);
    streamDemuxer_->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
    dataSource_ = std::make_shared<DataSourceImpl>(streamDemuxer_, 0);

    if (this->dataSource_ != nullptr) {
        MEDIA_LOG_D("TimeAndIndexConversion SetDataSource succeed");
    }
    if (!IsMP4orMOV()) {
        MEDIA_LOG_E("Not a valid MP4 or MOV file");
        return Status::ERROR_UNSUPPORTED_FORMAT;
    } else {
        MEDIA_LOG_D("It is a MP4 or MOV file");
        this->StartParse();
        return Status::OK;
    }
};

std::shared_ptr<Buffer> TimeAndIndexConversion::ReadBufferFromDataSource(size_t bufSize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto buffer = std::make_shared<Buffer>();
    std::vector<uint8_t> buff(bufSize);
    auto bufData = buffer->WrapMemory(buff.data(), bufSize, bufSize);
    auto result = dataSource_->ReadAt(offset_, buffer, bufSize);
    if (result != Status::OK) {
        MEDIA_LOG_E("Buffer read error");
    }
    return buffer;
}

void TimeAndIndexConversion::StartParse()
{
    dataSource_->GetSize(fileSize_);
    MEDIA_LOG_D("fileSize: " PUBLIC_LOG_D64, fileSize_);
    while (static_cast<uint64_t>(offset_) < fileSize_) {
        int bufSize = sizeof(uint32_t) + sizeof(uint32_t);
        auto buffer = ReadBufferFromDataSource(bufSize);
        BoxHeader header;
        ReadBoxHeader(buffer, header);
        if (strncmp(header.type, BOX_TYPE_MOOV, sizeof(header.type)) == 0) {
            offset_ += BOX_HEAD_SIZE;
            ParseMoov(header.size - BOX_HEAD_SIZE);
        } else if (header.size >= BOX_HEAD_SIZE) {
            offset_ += header.size;
        }
    }
}

void TimeAndIndexConversion::ReadBoxHeader(std::shared_ptr<Buffer> buffer, BoxHeader &header)
{
    auto memory = buffer->GetMemory();
    if (memory) {
        const uint8_t* ptr = memory->GetReadOnlyData();
        size_t size = memory->GetSize();
        if (size >= sizeof(header.size) + 4) { // 4 is used to check data
            header.size = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            header.type[0] = ptr[sizeof(header.size)]; // Get the 1st character of the header
            header.type[1] = ptr[sizeof(header.size) + 1]; // Get the 2nd character of the header
            header.type[2] = ptr[sizeof(header.size) + 2]; // Get the 3rd character of the header
            header.type[3] = ptr[sizeof(header.size) + 3]; // Get the 4th character of the header
            header.type[4] = '\0'; // Supplement string tail
        } else {
            MEDIA_LOG_E("Not enough data in buffer to read BoxHeader");
        }
    } else {
        MEDIA_LOG_E("No memory in buffer");
    }
}

bool TimeAndIndexConversion::IsMP4orMOV()
{
    int bufSize = sizeof(uint32_t) + sizeof(uint32_t);
    auto buffer = ReadBufferFromDataSource(bufSize);
    BoxHeader header;
    ReadBoxHeader(buffer, header);
    offset_ = 0; // init offset_
    return strncmp(header.type, BOX_TYPE_FTYP, sizeof(header.type)) == 0;
}

void TimeAndIndexConversion::ParseMoov(uint32_t boxSize)
{
    uint64_t parentSize = offset_ + boxSize;
    while (static_cast<uint64_t>(offset_) < parentSize) {
        int bufSize = sizeof(uint32_t) + sizeof(uint32_t);
        auto buffer = ReadBufferFromDataSource(bufSize);
        BoxHeader header;
        ReadBoxHeader(buffer, header);
        if (strncmp(header.type, BOX_TYPE_TRAK, sizeof(header.type)) == 0) {
            offset_ += BOX_HEAD_SIZE;
            ParseTrak(header.size - BOX_HEAD_SIZE);
        } else if (strncmp(header.type, BOX_TYPE_MVHD, sizeof(header.type)) == 0) {
            offset_ += BOX_HEAD_SIZE;
            ParseMvhd(header.size - BOX_HEAD_SIZE);
        } else if (header.size > BOX_HEAD_SIZE) {
            offset_ += header.size;
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
    uint64_t parentSize = offset_ + boxSize;
    while (static_cast<uint64_t>(offset_) < parentSize) {
        int bufSize = sizeof(uint32_t) + sizeof(uint32_t);
        auto buffer = ReadBufferFromDataSource(bufSize);
        BoxHeader header;
        ReadBoxHeader(buffer, header);
        auto it = boxParsers.find(std::string(header.type));
        if (it != boxParsers.end()) {
            offset_ += BOX_HEAD_SIZE;
            (this->*(it->second))(header.size - BOX_HEAD_SIZE);
        } else {
            offset_ += header.size;
        }
    }
}

void TimeAndIndexConversion::ParseCtts(uint32_t boxSize)
{
    auto buffer = std::make_shared<Buffer>();
    std::vector<uint8_t> buff(boxSize);
    auto bufData = buffer->WrapMemory(buff.data(), boxSize, boxSize);
    auto result = dataSource_->ReadAt(offset_, buffer, boxSize);
    if (result != Status::OK) {
        MEDIA_LOG_E("Buffer read error");
    }
    auto memory = buffer->GetMemory();
    if (!memory) {
        MEDIA_LOG_E("No memory in buffer");
        return;
    }
    const uint8_t* ptr = memory->GetReadOnlyData();
    size_t size = memory->GetSize();
    if (ptr == nullptr) {
        MEDIA_LOG_E("ptr is nullptr");
    }
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
        entries[i].sampleOffset = ntohl(*reinterpret_cast<const uint32_t*>(entryPtr + sizeof(uint32_t)));
        entryPtr += sizeof(CTTSEntry);
    }
    curTrakInfo_.cttsEntries = entries;
    offset_ += boxSize;
}

void TimeAndIndexConversion::ParseStts(uint32_t boxSize)
{
    auto buffer = std::make_shared<Buffer>();
    std::vector<uint8_t> buff(boxSize);
    auto bufData = buffer->WrapMemory(buff.data(), boxSize, boxSize);
    auto result = dataSource_->ReadAt(offset_, buffer, boxSize);
    if (result != Status::OK) {
        MEDIA_LOG_E("Buffer read error");
    }
    auto memory = buffer->GetMemory();
    if (!memory) {
        MEDIA_LOG_E("No memory in buffer");
        return;
    }
    const uint8_t* ptr = memory->GetReadOnlyData();
    if (ptr == nullptr) {
        MEDIA_LOG_E("ptr is nullptr");
    }
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
    offset_ += boxSize;
}

void TimeAndIndexConversion::ParseHdlr(uint32_t boxSize)
{
    auto buffer = std::make_shared<Buffer>();
    std::vector<uint8_t> buff(boxSize);
    auto bufData = buffer->WrapMemory(buff.data(), boxSize, boxSize);
    auto result = dataSource_->ReadAt(offset_, buffer, boxSize);
    if (result != Status::OK) {
        MEDIA_LOG_E("Buffer read error");
    }
    auto memory = buffer->GetMemory();
    if (!memory) {
        MEDIA_LOG_E("No memory in buffer");
        return;
    }
    const uint8_t* ptr = memory->GetReadOnlyData();
    size_t size = memory->GetSize();
    if (ptr == nullptr) {
        MEDIA_LOG_E("ptr is nullptr");
    }
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
        curTrakInfo_.trakType = TrakType::AUDIO;
    } else if (handlerType == "vide") {
        curTrakInfo_.trakType = TrakType::VIDIO;
    }
    offset_ += boxSize;
}

void TimeAndIndexConversion::ParseMvhd(uint32_t boxSize)
{
    auto buffer = std::make_shared<Buffer>();
    std::vector<uint8_t> buff(boxSize);
    auto bufData = buffer->WrapMemory(buff.data(), boxSize, boxSize);
    auto result = dataSource_->ReadAt(offset_, buffer, boxSize);
    if (result != Status::OK) {
        MEDIA_LOG_E("Buffer read error");
    }
    auto memory = buffer->GetMemory();
    if (!memory) {
        MEDIA_LOG_E("No memory in buffer");
        return;
    }
    const uint8_t* ptr = memory->GetReadOnlyData();
    size_t size = memory->GetSize();
    // version + flags + creation_time + modification_time + timescale + duration +
    // rate + volume + reserved + matrix + pre_defined + next_track_id
    if (size < sizeof(uint32_t) * 3 + sizeof(uint64_t) + sizeof(uint32_t) * 6) { // 3 and 6 is used to check data
        MEDIA_LOG_E("Not enough data in buffer to read MVHD header");
        return;
    }
    ptr += sizeof(uint32_t); // skip version and flags
    ptr += sizeof(uint32_t) * 2; // 2 is used to skip creation_time and modification_time
    // read timescale and duration
    uint32_t timescale = *reinterpret_cast<const uint32_t*>(ptr);
    uint32_t duration = *reinterpret_cast<const uint32_t*>(ptr + sizeof(uint32_t));
    timescale = ntohl(timescale);
    duration = ntohl(duration);
    offset_ += boxSize;
}

void TimeAndIndexConversion::ParseMdhd(uint32_t boxSize)
{
    auto buffer = std::make_shared<Buffer>();
    std::vector<uint8_t> buff(boxSize);
    auto bufData = buffer->WrapMemory(buff.data(), boxSize, boxSize);
    auto result = dataSource_->ReadAt(offset_, buffer, boxSize);
    if (result != Status::OK) {
        MEDIA_LOG_E("Buffer read error");
    }
    auto memory = buffer->GetMemory();
    if (!memory) {
        MEDIA_LOG_E("No memory in buffer");
        return;
    }
    const uint8_t* ptr = memory->GetReadOnlyData();
    if (ptr == nullptr) {
        MEDIA_LOG_E("ptr is nullptr");
    }
    size_t size = memory->GetSize();
    if (size < sizeof(uint32_t) * 3) { // 3 is used to check for version, flags, and creation_time
        MEDIA_LOG_E("Not enough data in buffer to read MDHD header");
        return;
    }
    // 读取versionAndFlags，creation_time，modification_time，timeScale，和duration
    uint32_t versionAndFlags = *reinterpret_cast<const uint32_t*>(ptr);
    MEDIA_LOG_D("versionAndFlags: " PUBLIC_LOG_D32, versionAndFlags);
    uint32_t timeScale = *reinterpret_cast<const uint32_t*>(ptr + sizeof(uint32_t) * 3); // 3 is used to check data
    timeScale = ntohl(timeScale);
    MEDIA_LOG_D("timeScale: " PUBLIC_LOG_D32, timeScale);
    curTrakInfo_.timeScale = timeScale;
    offset_ += boxSize;
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

Status TimeAndIndexConversion::GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
    const uint64_t relativePresentationTimeUs, uint32_t &index)
{
    FALSE_RETURN_V_MSG_E(trackIndex < trakInfoVec_.size(), Status::ERROR_INVALID_DATA, "Track is out of range");
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
    if (trackIndex >= trakInfoVec_.size()) {
        MEDIA_LOG_E("Track is out of range");
        return Status::ERROR_INVALID_DATA;
    }
    InitPTSandIndexConvert();
    Status ret = GetPresentationTimeUsFromFfmpegMOV(GET_FIRST_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Get pts failed");

    GetPresentationTimeUsFromFfmpegMOV(INDEX_TO_RELATIVEPTS, trackIndex,
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

    int32_t sttsCurNum = trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleCount;
    int32_t cttsCurNum = 0;

    cttsCurNum = trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries[cttsIndex].sampleCount;
    while (sttsIndex < trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries.size() &&
           cttsIndex < trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries.size() &&
           cttsCurNum >= 0 && sttsCurNum >= 0) {
        if (cttsCurNum == 0) {
            cttsIndex++;
            cttsCurNum = cttsIndex < trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries.size() ?
                         trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries[cttsIndex].sampleCount : 0;
        }
        cttsCurNum--;
        pts = (dts +
                static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].cttsEntries[cttsIndex].sampleOffset)) *
                1000 * 1000 / // 1000 is used for converting pts to us
                static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].timeScale);
        PTSAndIndexConvertSwitchProcess(mode, pts, absolutePTS, index);
        sttsCurNum--;
        dts += static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleDelta);
        if (sttsCurNum == 0) {
            sttsIndex++;
            sttsCurNum = sttsIndex < trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries.size() ?
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

    int32_t sttsCurNum = trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleCount;

    while (sttsIndex < trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries.size() && sttsCurNum >= 0) {
        pts = dts * 1000 * 1000 / // 1000 is for converting pts to us
              static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].timeScale);
        PTSAndIndexConvertSwitchProcess(mode, pts, absolutePTS, index);
        sttsCurNum--;
        dts += static_cast<int64_t>(trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries[sttsIndex].sampleDelta);
        if (sttsCurNum == 0) {
            sttsIndex++;
            sttsCurNum = sttsIndex < trakInfoVec_[curConvertTrakInfoIndex_].sttsEntries.size() ?
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
} // namespace TimeAndIndex
} // namespace Media
} // namespace OHOS