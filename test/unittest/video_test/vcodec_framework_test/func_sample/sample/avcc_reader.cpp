/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "avcc_reader.h"
#include <algorithm>
#include <functional>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "securec.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "AvccReader"};
constexpr uint8_t AVCC_FRAME_HEAD_LEN = 4;
constexpr uint8_t ANNEXB_FRAME_HEAD[] = {0, 0, 1};
constexpr uint8_t ANNEXB_FRAME_HEAD_LEN = sizeof(ANNEXB_FRAME_HEAD);
constexpr uint32_t MAX_NALU_SIZE = 2 * 1024 * 1024; // 2Mb

static inline int64_t GetTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    // 1000'000: second to micro second; 1000: nano second to micro second
    return (static_cast<int64_t>(now.tv_sec) * 1000'000 + (now.tv_nsec / 1000));
}

enum AvcNalType {
    AVC_UNSPECIFIED = 0,
    AVC_NON_IDR = 1,
    AVC_PARTITION_A = 2,
    AVC_PARTITION_B = 3,
    AVC_PARTITION_C = 4,
    AVC_IDR = 5,
    AVC_SEI = 6,
    AVC_SPS = 7,
    AVC_PPS = 8,
    AVC_AU_DELIMITER = 9,
    AVC_END_OF_SEQUENCE = 10,
    AVC_END_OF_STREAM = 11,
    AVC_FILLER_DATA = 12,
    AVC_SPS_EXT = 13,
    AVC_PREFIX = 14,
    AVC_SUB_SPS = 15,
    AVC_DPS = 16,
};

enum HevcNalType {
    HEVC_TRAIL_N = 0,
    HEVC_TRAIL_R = 1,
    HEVC_TSA_N = 2,
    HEVC_TSA_R = 3,
    HEVC_STSA_N = 4,
    HEVC_STSA_R = 5,
    HEVC_RADL_N = 6,
    HEVC_RADL_R = 7,
    HEVC_RASL_N = 8,
    HEVC_RASL_R = 9,
    HEVC_BLA_W_LP = 16,
    HEVC_BLA_W_RADL = 17,
    HEVC_BLA_N_LP = 18,
    HEVC_IDR_W_RADL = 19,
    HEVC_IDR_N_LP = 20,
    HEVC_CRA_NUT = 21,
    HEVC_VPS_NUT = 32,
    HEVC_SPS_NUT = 33,
    HEVC_PPS_NUT = 34,
    HEVC_AUD_NUT = 35,
    HEVC_EOS_NUT = 36,
    HEVC_EOB_NUT = 37,
    HEVC_FD_NUT = 38,
    HEVC_PREFIX_SEI_NUT = 39,
    HEVC_SUFFIX_SEI_NUT = 40,
};
}

namespace OHOS {
namespace MediaAVCodec {
int32_t AvccReader::Init(const std::shared_ptr<AvccReaderInfo> &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<std::ifstream> inputFile = std::make_unique<std::ifstream>(info->inPath_.data(),
                                                                               std::ios::binary | std::ios::in);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputFile != nullptr && inputFile->is_open(),
                                      AV_ERR_INVALID_VAL, "Open input file failed");

    nalUnitReader_ = std::static_pointer_cast<NalUnitReader>(std::make_shared<AvccNalUnitReader>(inputFile));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(nalUnitReader_, AV_ERR_INVALID_VAL, "Nal unit reader create failed");

    nalDetector_ = info->isH264Stream_ ?
        std::static_pointer_cast<NalDetector>(std::make_shared<AVCNalDetector>()) :
        std::static_pointer_cast<NalDetector>(std::make_shared<HEVCNalDetector>());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(nalDetector_, AV_ERR_INVALID_VAL, "Nal detector create failed");

    return AV_ERR_OK;
}

void AvccReader::FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t naluType,
                                         bool isEosFrame)
{
    attr.size += frameSize;
    attr.pts = GetTimeUs();
    attr.flags |= nalDetector_->IsXPS(naluType) ? AVCODEC_BUFFER_FLAG_CODEC_DATA : 0;
    attr.flags |= nalDetector_->IsIDR(naluType) ? AVCODEC_BUFFER_FLAG_SYNC_FRAME : 0;
    if (isEosFrame) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        std::cout << "Input EOS Frame, frameCount = " << (frameInputCount_ + 1) << std::endl;
    }
}

int32_t AvccReader::FillBuffer(std::shared_ptr<VDecSignal> &signal_, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto buffer = signal_->inMemoryQueue_.front();
    auto bufferAddr = buffer->GetAddr();

    do {
        int32_t frameSize = 0;
        bool isEosFrame = false;
        auto ret = nalUnitReader_->ReadNalUnit(bufferAddr, frameSize, isEosFrame);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadNalUnit failed");
        uint8_t naluType = nalDetector_->GetNalType(nalDetector_->GetNalTypeAddr(bufferAddr));
        bufferAddr += frameSize;
        FillBufferAttr(attr, frameSize, naluType, isEosFrame);
        UNITTEST_CHECK_AND_BREAK_LOG(!nalDetector_->IsFullVCL(
            naluType, nalDetector_->GetNalTypeAddr(nalUnitReader_->GetNextNalUnitAddr())) && !IsEOS(),
            "FillBuffer stop running");
    } while (true);
    frameInputCount_++;

    return AV_ERR_OK;
}

int32_t AvccReader::FillBufferExt(std::shared_ptr<VDecSignal> &signal_, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto buffer = signal_->inBufferQueue_.front();
    auto bufferAddr = buffer->GetAddr();

    do {
        int32_t frameSize = 0;
        bool isEosFrame = false;
        auto ret = nalUnitReader_->ReadNalUnit(bufferAddr, frameSize, isEosFrame);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadNalUnit failed");
        auto naluType = nalDetector_->GetNalType(nalDetector_->GetNalTypeAddr(bufferAddr));
        bufferAddr += frameSize;
        FillBufferAttr(attr, frameSize, naluType, isEosFrame);
        buffer->SetBufferAttr(attr);
        UNITTEST_CHECK_AND_BREAK_LOG(!nalDetector_->IsFullVCL(
            naluType, nalDetector_->GetNalTypeAddr(nalUnitReader_->GetNextNalUnitAddr())) && !IsEOS(),
            "FillBuffer stop running");
    } while (true);
    frameInputCount_++;

    return AV_ERR_OK;
}

bool AvccReader::IsEOS()
{
    return nalUnitReader_ ? nalUnitReader_->IsEOS() : true;
}

uint8_t const * AvccReader::NalUnitReader::GetNextNalUnitAddr()
{
    CHECK_AND_RETURN_RET_LOG(nalUnit_ != nullptr, nullptr, "nalUnit_ is nullptr");
    return nalUnit_->data();
}

int32_t AvccReader::NalUnitReader::ReadNalUnit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Got a invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(nalUnit_, AV_ERR_INVALID_VAL, "Nal unit buffer is nullptr");
    bufferSize = nalUnit_->size();
    memcpy_s(bufferAddr, bufferSize, nalUnit_->data(), bufferSize);

    if (!IsEOF()) {
        isEosFrame = false;
        PrereadNalUnit();
    } else {
        isEosFrame = true;
        nalUnit_->resize(0);
    }
    return AV_ERR_OK;
}

AvccReader::AvccNalUnitReader::AvccNalUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    nalUnit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadNalUnit();
}

bool AvccReader::AvccNalUnitReader::IsEOS()
{
    return IsEOF() && nalUnit_->empty();
}

bool AvccReader::AvccNalUnitReader::IsEOF()
{
    return inputFile_->peek() == EOF;
}

void AvccReader::AvccNalUnitReader::PrereadNalUnit()
{
    uint8_t len[AVCC_FRAME_HEAD_LEN] = {};
    (void)inputFile_->read(reinterpret_cast<char *>(len), AVCC_FRAME_HEAD_LEN);
    nalUnit_->resize(MAX_NALU_SIZE);
    // 0 1 2 3: avcc frame head byte offset; 8 16 24: avcc frame head bit offset
    uint32_t bufferSize = static_cast<uint32_t>((len[3]) | (len[2] << 8) | (len[1] << 16) | (len[0] << 24));
    uint8_t *bufferAddr = nalUnit_->data();

    (void)inputFile_->read(reinterpret_cast<char *>(bufferAddr + AVCC_FRAME_HEAD_LEN), bufferSize);
    ToAnnexb(bufferAddr);

    bufferSize += AVCC_FRAME_HEAD_LEN;
    nalUnit_->resize(bufferSize);
}

int32_t AvccReader::AvccNalUnitReader::ToAnnexb(uint8_t *bufferAddr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Buffer address is null");

    bufferAddr[0] = 0;
    bufferAddr[1] = 0;
    bufferAddr[2] = 0;      // 2: annexB frame head offset 2
    bufferAddr[3] = 1;      // 3: annexB frame head offset 3
    return AV_ERR_OK;
}

const uint8_t *AvccReader::NalDetector::GetNalTypeAddr(const uint8_t *bufferAddr)
{
    auto pos = std::search(bufferAddr, bufferAddr + ANNEXB_FRAME_HEAD_LEN + 1,
        std::begin(ANNEXB_FRAME_HEAD), std::end(ANNEXB_FRAME_HEAD));
    return pos + ANNEXB_FRAME_HEAD_LEN;
}

bool AvccReader::NalDetector::IsFullVCL(uint8_t nalType, const uint8_t *nextNalTypeAddr)
{
    auto nextNaluType = GetNalType(nextNalTypeAddr);
    return (IsVCL(nalType) && (
        (!IsVCL(nextNaluType)) ||
        ((IsVCL(nextNaluType)) && IsFirstSlice(nextNalTypeAddr)) // 0x80: first_slice_segment_in_pic_flag
    ));
}

uint8_t AvccReader::AVCNalDetector::GetNalType(const uint8_t *bufferAddr)
{
    return (*bufferAddr) & 0x1F; // AVC Nal offset: value & 0x1F
}

bool AvccReader::AVCNalDetector::IsXPS(uint8_t nalType)
{
    return (nalType == AVC_SPS) || (nalType == AVC_PPS) ? true : false;
}

bool AvccReader::AVCNalDetector::IsIDR(uint8_t nalType)
{
    return (nalType == AVC_IDR) ? true : false;
}

uint8_t AvccReader::HEVCNalDetector::GetNalType(const uint8_t *bufferAddr)
{
    return (((*bufferAddr) & 0x7E) >> 1);    // HEVC Nal offset: (value & 0x7E) >> 1
}

bool AvccReader::AVCNalDetector::IsVCL(uint8_t nalType)
{
    return (nalType >= AVC_NON_IDR && nalType <= AVC_IDR) ? true : false;
}

bool AvccReader::AVCNalDetector::IsFirstSlice(const uint8_t *nalTypeAddr)
{
    return (*(nalTypeAddr + 1) & 0x80) == 0x80; // *(nalTypeAddr + 1) & 0x80: AVC first_mb_in_slice
}

bool AvccReader::HEVCNalDetector::IsXPS(uint8_t nalType)
{
    return (nalType >= HEVC_VPS_NUT) && (nalType <= HEVC_PPS_NUT) ? true : false;
}

bool AvccReader::HEVCNalDetector::IsIDR(uint8_t nalType)
{
    return (nalType >= HEVC_IDR_W_RADL) && (nalType <= HEVC_CRA_NUT) ? true : false;
}

bool AvccReader::HEVCNalDetector::IsVCL(uint8_t nalType)
{
    return (nalType >= HEVC_TRAIL_N && nalType <= HEVC_CRA_NUT) ? true : false;
}

bool AvccReader::HEVCNalDetector::IsFirstSlice(const uint8_t *nalTypeAddr)
{
    return *(nalTypeAddr + 2) & 0x80; // *(nalTypeAddr + 2) & 0x80: HEVC first_slice_segment_in_pic_flag
}
} // MediaAVCodec
} // OHOS