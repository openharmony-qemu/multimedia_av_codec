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

#include "bitstream_reader.h"
#include <algorithm>
#include <functional>
#include "securec.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "BitstreamReader"};
constexpr uint8_t AVCC_FRAME_HEAD_LEN = 4;
constexpr uint8_t ANNEXB_FRAME_HEAD[] = {0, 0, 1};
constexpr uint8_t ANNEXB_FRAME_HEAD_LEN = sizeof(ANNEXB_FRAME_HEAD);
constexpr uint32_t PREREAD_BUFFER_SIZE = 1 * 1024 * 1024; // 1Mb, must greater than ANNEXB_FRAME_HEAD_LEN
constexpr uint32_t MAX_NALU_SIZE = 4 * 1024 * 1024; // 4Mb

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
namespace Sample {
int32_t BitstreamReader::Init(const std::shared_ptr<SampleInfo> &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sampleInfo_ = info;
    inputFile_ = std::make_unique<std::ifstream>(sampleInfo_->inputFilePath.data(), std::ios::binary | std::ios::in);
    CHECK_AND_RETURN_RET_LOG(inputFile_ && inputFile_->is_open(), AVCODEC_SAMPLE_ERR_ERROR, "Open input file failed");

    nalUnitReader_ = sampleInfo_->dataProducerInfo.bitstreamType == BITSTREAM_TYPE_ANNEXB ?
        std::static_pointer_cast<NalUnitReader>(std::make_shared<AnnexbNalUnitReader>(inputFile_)) :
        std::static_pointer_cast<NalUnitReader>(std::make_shared<AvccNalUnitReader>(inputFile_));
    CHECK_AND_RETURN_RET_LOG(nalUnitReader_, AVCODEC_SAMPLE_ERR_ERROR, "Nal unit reader create failed");

    nalDetector_ = sampleInfo_->codecMime == OH_AVCODEC_MIMETYPE_VIDEO_AVC ?
        std::static_pointer_cast<NalDetector>(std::make_shared<AVCNalDetector>()) :
        std::static_pointer_cast<NalDetector>(std::make_shared<HEVCNalDetector>());
    CHECK_AND_RETURN_RET_LOG(nalDetector_, AVCODEC_SAMPLE_ERR_ERROR, "Nal detector create failed");

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t BitstreamReader::FillBuffer(CodecBufferInfo &bufferInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(inputFile_ != nullptr && inputFile_->is_open(),
        AVCODEC_SAMPLE_ERR_ERROR, "Input file is not open!");

    auto bufferAddr = bufferInfo.bufferAddr;
    CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Got invalid buffer");

    do {
        int32_t frameSize = 0;
        auto ret = nalUnitReader_->ReadNalUnit(bufferAddr, frameSize, bufferInfo.bufferCapacity);
        CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Sample failed");

        auto naluType = nalDetector_->GetNalType(nalDetector_->GetNalTypeAddr(bufferAddr));
        bufferInfo.attr.size += frameSize;
        bufferAddr += frameSize;
        bufferInfo.attr.flags |= nalDetector_->IsXPS(naluType) ? AVCODEC_BUFFER_FLAGS_CODEC_DATA : 0;
        bufferInfo.attr.flags |= nalDetector_->IsIDR(naluType) ? AVCODEC_BUFFER_FLAGS_SYNC_FRAME : 0;
        CHECK_AND_BREAK(
            !nalDetector_->IsXPS(naluType) &&
            !nalDetector_->IsFullVCL(naluType, nalDetector_->GetNalTypeAddr(nalUnitReader_->GetNextNalUnitAddr())) &&
            !IsEOS()
        );
    } while (true);

    return AVCODEC_SAMPLE_ERR_OK;
}

bool BitstreamReader::IsEOS()
{
    return nalUnitReader_ ? nalUnitReader_->IsEOS() : true;
}

uint8_t const * BitstreamReader::NalUnitReader::GetNextNalUnitAddr()
{
    CHECK_AND_RETURN_RET(nalUnit_, nullptr);
    return nalUnit_->data();
}

int32_t BitstreamReader::NalUnitReader::ReadNalUnit(uint8_t *bufferAddr, int32_t &bufferSize, uint32_t bufferCapacity)
{
    CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Got a invalid buffer addr");
    CHECK_AND_RETURN_RET_LOG(nalUnit_, AVCODEC_SAMPLE_ERR_ERROR, "Nal unit buffer is nullptr");

    bufferSize = nalUnit_->size() - readSize_;
    auto ret = memcpy_s(bufferAddr, bufferSize, nalUnit_->data() + readSize_, bufferSize);
    CHECK_AND_RETURN_RET_LOG(ret == EOK, AVCODEC_SAMPLE_ERR_ERROR, "ReadNalUnit failed");

    readSize_ = (bufferSize > bufferCapacity) ? bufferSize : 0;

    if (!IsEOF()) {
        PrereadNalUnit();
    } else {
        nalUnit_->resize(0);
    }
    return AVCODEC_SAMPLE_ERR_OK;
}

BitstreamReader::AnnexbNalUnitReader::AnnexbNalUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    prereadBuffer_ = std::make_unique<uint8_t []>(PREREAD_BUFFER_SIZE + ANNEXB_FRAME_HEAD_LEN);
    PrereadFile();

    nalUnit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadNalUnit();
}

bool BitstreamReader::AnnexbNalUnitReader::IsEOS()
{
    return IsEOF() && nalUnit_->empty();
}

bool BitstreamReader::AnnexbNalUnitReader::IsEOF()
{
    return (pPrereadBuffer_ == prereadBufferSize_) && (inputFile_->peek() == EOF);
}

void BitstreamReader::AnnexbNalUnitReader::PrereadFile()
{
    CHECK_AND_RETURN_LOG(prereadBuffer_, "Preread buffer is nallptr");
    inputFile_->read(reinterpret_cast<char *>(prereadBuffer_.get() + ANNEXB_FRAME_HEAD_LEN), PREREAD_BUFFER_SIZE);
    prereadBufferSize_ = inputFile_->gcount() + ANNEXB_FRAME_HEAD_LEN;
    pPrereadBuffer_ = ANNEXB_FRAME_HEAD_LEN;
}

void BitstreamReader::AnnexbNalUnitReader::PrereadNalUnit()
{
    CHECK_AND_RETURN_LOG(prereadBufferSize_ > 0, "Empty file, nothing to read");
    CHECK_AND_RETURN_LOG(nalUnit_, "Nal unit buffer is nullptr");

    auto pBuffer = nalUnit_->data();
    uint32_t bufferSize = 0;
    nalUnit_->resize(MAX_NALU_SIZE);
    do {
        auto pos = std::search(prereadBuffer_.get() + pPrereadBuffer_ + (bufferSize > 0 ? 0 : ANNEXB_FRAME_HEAD_LEN),
            prereadBuffer_.get() + prereadBufferSize_, std::begin(ANNEXB_FRAME_HEAD), std::end(ANNEXB_FRAME_HEAD));
        uint32_t size = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos);
        auto ret = memcpy_s(pBuffer, size, prereadBuffer_.get() + pPrereadBuffer_, size);
        CHECK_AND_RETURN_LOG(ret == EOK, "Copy buffer failed");
        pPrereadBuffer_ += size;
        bufferSize += size;
        pBuffer += size;

        CHECK_AND_BREAK((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof());

        PrereadFile();
        ret = memcpy_s(prereadBuffer_.get(), ANNEXB_FRAME_HEAD_LEN,
            pBuffer - ANNEXB_FRAME_HEAD_LEN, ANNEXB_FRAME_HEAD_LEN);
        CHECK_AND_RETURN_LOG(ret == EOK, "Copy buffer failed");
        CHECK_AND_CONTINUE(std::search(pBuffer - ANNEXB_FRAME_HEAD_LEN, pBuffer,
            std::begin(ANNEXB_FRAME_HEAD), std::end(ANNEXB_FRAME_HEAD)) != pBuffer);

        bufferSize -= ANNEXB_FRAME_HEAD_LEN;
        pBuffer -= ANNEXB_FRAME_HEAD_LEN;
        pPrereadBuffer_ = 0;
    } while (true);
    nalUnit_->resize(bufferSize);
}

BitstreamReader::AvccNalUnitReader::AvccNalUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    nalUnit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadNalUnit();
}

bool BitstreamReader::AvccNalUnitReader::IsEOS()
{
    return IsEOF() && nalUnit_->empty();
}

bool BitstreamReader::AvccNalUnitReader::IsEOF()
{
    return inputFile_->peek() == EOF;
}

void BitstreamReader::AvccNalUnitReader::PrereadNalUnit()
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

int32_t BitstreamReader::AvccNalUnitReader::ToAnnexb(uint8_t *bufferAddr)
{
    CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Buffer address is null");

    bufferAddr[0] = 0;
    bufferAddr[1] = 0;
    bufferAddr[2] = 0;      // 2: annexB frame head offset 2
    bufferAddr[3] = 1;      // 3: annexB frame head offset 3
    return AVCODEC_SAMPLE_ERR_OK;
}

const uint8_t *BitstreamReader::NalDetector::GetNalTypeAddr(const uint8_t *bufferAddr)
{
    auto pos = std::search(bufferAddr, bufferAddr + ANNEXB_FRAME_HEAD_LEN + 1,
        std::begin(ANNEXB_FRAME_HEAD), std::end(ANNEXB_FRAME_HEAD));
    return pos + ANNEXB_FRAME_HEAD_LEN;
}

bool BitstreamReader::NalDetector::IsFullVCL(uint8_t nalType, const uint8_t *nextNalTypeAddr)
{
    auto nextNalType = GetNalType(nextNalTypeAddr);
    return (IsVCL(nalType) && (
        (!IsVCL(nextNalType)) ||
        ((IsVCL(nextNalType)) && IsFirstSlice(nextNalTypeAddr))
    ));
}

uint8_t BitstreamReader::AVCNalDetector::GetNalType(const uint8_t *bufferAddr)
{
    return (*bufferAddr) & 0x1F; // AVC Nal offset: value & 0x1F
}

bool BitstreamReader::AVCNalDetector::IsXPS(uint8_t nalType)
{
    return (nalType == AVC_SPS) || (nalType == AVC_PPS) ? true : false;
}

bool BitstreamReader::AVCNalDetector::IsIDR(uint8_t nalType)
{
    return (nalType == AVC_IDR) ? true : false;
}

uint8_t BitstreamReader::HEVCNalDetector::GetNalType(const uint8_t *bufferAddr)
{
    return (((*bufferAddr) & 0x7E) >> 1);    // HEVC Nal offset: (value & 0x7E) >> 1
}

bool BitstreamReader::AVCNalDetector::IsVCL(uint8_t nalType)
{
    return (nalType >= AVC_NON_IDR && nalType <= AVC_IDR) ? true : false;
}

bool BitstreamReader::AVCNalDetector::IsFirstSlice(const uint8_t *nalTypeAddr)
{
    return (*(nalTypeAddr + 1) & 0x80) == 0x80;   // *(nalTypeAddr + 1) & 0x80: AVC first_mb_in_slice
}

bool BitstreamReader::HEVCNalDetector::IsXPS(uint8_t nalType)
{
    return (nalType >= HEVC_VPS_NUT) && (nalType <= HEVC_PPS_NUT) ? true : false;
}

bool BitstreamReader::HEVCNalDetector::IsIDR(uint8_t nalType)
{
    return (nalType >= HEVC_IDR_W_RADL) && (nalType <= HEVC_CRA_NUT) ? true : false;
}

bool BitstreamReader::HEVCNalDetector::IsVCL(uint8_t nalType)
{
    return (nalType >= HEVC_TRAIL_N && nalType <= HEVC_CRA_NUT) ? true : false;
}

bool BitstreamReader::HEVCNalDetector::IsFirstSlice(const uint8_t *nalTypeAddr)
{
    return *(nalTypeAddr + 2) & 0x80;   // *(nalTypeAddr + 2) & 0x80: HEVC first_slice_segment_in_pic_flag
}
} // Sample
} // MediaAVCodec
} // OHOS