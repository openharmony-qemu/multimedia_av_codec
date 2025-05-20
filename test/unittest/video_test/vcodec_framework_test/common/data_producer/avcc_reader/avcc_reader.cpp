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
#include <thread>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "securec.h"
#include "unittest_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "AvccReader"};
constexpr uint8_t AVCC_FRAME_HEAD_LEN = 4;
constexpr uint8_t ANNEXB_FRAME_HEAD[] = {0, 0, 1};
constexpr uint8_t ANNEXB_FRAME_HEAD_LEN = sizeof(ANNEXB_FRAME_HEAD);
constexpr uint32_t MAX_NALU_SIZE = 2 * 1024 * 1024; // 2Mb

constexpr uint8_t MPEG2_FRAME_HEAD[] = {0x00, 0x00, 0x01, 0x00};
constexpr uint8_t MPEG2_FRAME_HEAD_LEN = sizeof(MPEG2_FRAME_HEAD);
constexpr uint8_t MPEG2_SEQUENCE_HEAD[] = {0x00, 0x00, 0x01, 0xb3};
constexpr uint8_t MPEG2_SEQUENCE_HEAD_LEN = sizeof(MPEG2_SEQUENCE_HEAD);
constexpr uint8_t MPEG2_TYPE_OFFEST = 1;
constexpr uint8_t MPEG4_FRAME_HEAD[] = {0x00, 0x00, 0x01, 0xb6};
constexpr uint8_t MPEG4_FRAME_HEAD_LEN = sizeof(MPEG4_FRAME_HEAD);
constexpr uint8_t MPEG4_SEQUENCE_HEAD[] = {0x00, 0x00, 0x01, 0xb0};
constexpr uint8_t MPEG4_SEQUENCE_HEAD_LEN = sizeof(MPEG4_SEQUENCE_HEAD);
constexpr uint32_t PREREAD_BUFFER_SIZE = 5 * 1024 * 1024;

constexpr uint8_t H263_HEAD_0[] = {0x00, 0x00, 0x80};
constexpr uint8_t H263_HEAD_1[] = {0x00, 0x00, 0x81};
constexpr uint8_t H263_HEAD_2[] = {0x00, 0x00, 0x82};
constexpr uint8_t H263_HEAD_3[] = {0x00, 0x00, 0x83};
constexpr uint8_t H263_HEAD_LEN = sizeof(H263_HEAD_0);
constexpr uint8_t H263_HEAD_MASK_4_1 = 0x1c;
constexpr uint8_t H263_HEAD_MASK_4_2 = 0x02;
constexpr uint8_t H263_HEAD_MASK_5_1 = 0x80;
constexpr uint8_t H263_HEAD_MASK_5_2 = 0x70;
constexpr uint8_t H263_OFFSET_4 = 4;
constexpr uint8_t H263_OFFSET_5 = 5;
constexpr uint8_t H263_OFFSET_7 = 7;

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
enum Mpeg2Type {
    M2V_UNSPECIFIED = 0,
    M2V_I = 1,
    M2V_P = 2,
    M2V_B = 3,
};
enum Mpeg4Type {
    MPEG4_UNSPECIFIED = 0,
    MPEG4_I = 1,
    MPEG4_P = 2,
    MPEG4_B = 3,
    MPEG4_S = 4,
};

enum H263Type {
    H263_I = 1,
};

}

namespace OHOS {
namespace MediaAVCodec {


int32_t MpegReader::Init(const std::shared_ptr<MpegReaderInfo> &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<std::ifstream> inputFile = std::make_unique<std::ifstream>(info->inPath.data(),
        std::ios::binary | std::ios::in);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputFile != nullptr && inputFile->is_open(),
                                      AV_ERR_INVALID_VAL, "Open input file failed");

    mpegUnitReader_ = info->isMpeg2Stream ?
        std::static_pointer_cast<MpegUnitReader>(std::make_shared<Mpeg2MetaUnitReader>(inputFile)) :
        std::static_pointer_cast<MpegUnitReader>(std::make_shared<Mpeg4MetaUnitReader>(inputFile));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mpegUnitReader_, AV_ERR_INVALID_VAL, "Mpeg unit reader create failed");

    mpegDetector_ = info->isMpeg2Stream ?
        std::static_pointer_cast<MpegDetector>(std::make_shared<Mpeg2Detector>()) :
        std::static_pointer_cast<MpegDetector>(std::make_shared<Mpeg4Detector>());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mpegDetector_, AV_ERR_INVALID_VAL, "Mpeg detector create failed");

    return AV_ERR_OK;
}

void MpegReader::FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t mpegType,
                                bool isEosFrame)
{
    attr.size += frameSize;
    attr.pts = GetTimeUs();
    attr.flags |= mpegDetector_->IsI(mpegType) ? AVCODEC_BUFFER_FLAG_SYNC_FRAME : 0;
    if (isEosFrame) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        std::cout << "Input EOS Frame, frameCount = " << (frameInputCount_ + 1) << std::endl;
    }
}

int32_t MpegReader::FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int32_t frameSize = 0;
    bool isEosFrame = false;
    auto ret = mpegUnitReader_->ReadMpegUnit(bufferAddr, frameSize, isEosFrame);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadMpegUnit failed");
    uint8_t mpegType = mpegDetector_->GetMpegType(mpegDetector_->GetMpegTypeAddr(bufferAddr));
    bufferAddr += frameSize;
    FillBufferAttr(attr, frameSize, mpegType, isEosFrame);
    
    frameInputCount_++;

    return AV_ERR_OK;
}

bool MpegReader::IsEOS()
{
    return mpegUnitReader_ ? mpegUnitReader_->IsEOS() : true;
}

uint8_t const * MpegReader::MpegUnitReader::GetNextMpegUnitAddr()
{
    CHECK_AND_RETURN_RET_LOG(mpegUnit_ != nullptr, nullptr, "mpegUnit_ is nullptr");
    return mpegUnit_->data();
}

void MpegReader::Mpeg2MetaUnitReader::PrereadFile()
{
    CHECK_AND_RETURN_LOG(prereadBuffer_, "Preread buffer is nallptr");
    inputFile_->read(reinterpret_cast<char *>(prereadBuffer_.get() + MPEG2_FRAME_HEAD_LEN), PREREAD_BUFFER_SIZE);
    prereadBufferSize_ = inputFile_->gcount() + MPEG2_FRAME_HEAD_LEN;
    pPrereadBuffer_ = MPEG2_FRAME_HEAD_LEN;
}

int32_t MpegReader::Mpeg2MetaUnitReader::ReadMpegUnit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Got a invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mpegUnit_, AV_ERR_INVALID_VAL, "Mpeg unit buffer is nullptr");
    bufferSize = mpegUnit_->size();
    memcpy_s(bufferAddr, bufferSize, mpegUnit_->data(), bufferSize);

    if (!IsEOF()) {
        isEosFrame = false;
        PrereadMpeg2Unit();
    } else {
        isEosFrame = true;
        mpegUnit_->resize(0);
    }
    return AV_ERR_OK;
}

MpegReader::Mpeg2MetaUnitReader::Mpeg2MetaUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    prereadBuffer_ = std::make_unique<uint8_t []>(PREREAD_BUFFER_SIZE + MPEG2_FRAME_HEAD_LEN);
    PrereadFile();

    mpegUnit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadMpeg2Unit();
}

bool MpegReader::Mpeg2MetaUnitReader::IsEOS()
{
    return IsEOF() && mpegUnit_->empty();
}

bool MpegReader::Mpeg2MetaUnitReader::IsEOF()
{
    return (pPrereadBuffer_ == prereadBufferSize_) && (inputFile_->peek() == EOF);
}

void MpegReader::Mpeg2MetaUnitReader::PrereadMpeg2Unit()
{
    CHECK_AND_RETURN_LOG(prereadBufferSize_ > 0, "Empty file, nothing to read");
    CHECK_AND_RETURN_LOG(mpegUnit_, "Mpeg2 unit buffer is nullptr");
    auto pBuffer = mpegUnit_->data();
    uint32_t bufferSize = 0;
    mpegUnit_->resize(MAX_NALU_SIZE);
    do {
        auto pos1 = std::search(prereadBuffer_.get() + pPrereadBuffer_ + MPEG2_FRAME_HEAD_LEN,
            prereadBuffer_.get() + prereadBufferSize_, std::begin(MPEG2_FRAME_HEAD), std::end(MPEG2_FRAME_HEAD));
        uint32_t size1 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos1);
        auto pos2 = std::search(prereadBuffer_.get() + pPrereadBuffer_, prereadBuffer_.get() +
            pPrereadBuffer_ + size1, std::begin(MPEG2_SEQUENCE_HEAD), std::end(MPEG2_SEQUENCE_HEAD));
        uint32_t size = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos2);
        if (size == 0) {
            auto pos3 = std::search(prereadBuffer_.get() + pPrereadBuffer_ + size1 + MPEG2_FRAME_HEAD_LEN,
                prereadBuffer_.get() + prereadBufferSize_, std::begin(MPEG2_FRAME_HEAD), std::end(MPEG2_FRAME_HEAD));
            uint32_t size2 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos3);
            auto ret = memcpy_s(pBuffer, size2, prereadBuffer_.get() + pPrereadBuffer_, size2);
            CHECK_AND_RETURN_LOG(ret == EOK, "First Copy buffer failed");
            pPrereadBuffer_ += size2;
            bufferSize += size2;
            pBuffer += size2;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        } else if (size1 > size) {
            auto ret = memcpy_s(pBuffer, size, prereadBuffer_.get() + pPrereadBuffer_, size);
            CHECK_AND_RETURN_LOG(ret == EOK, "Last Copy buffer failed");
            pPrereadBuffer_ += size;
            bufferSize += size;
            pBuffer += size;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        } else {
            auto ret = memcpy_s(pBuffer, size1, prereadBuffer_.get() + pPrereadBuffer_, size1);
            CHECK_AND_RETURN_LOG(ret == EOK, "Comom Copy buffer failed");
            pPrereadBuffer_ += size1;
            bufferSize += size1;
            pBuffer += size1;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        }
        PrereadFile();
        auto ret = memcpy_s(prereadBuffer_.get(), MPEG2_FRAME_HEAD_LEN, pBuffer - MPEG2_FRAME_HEAD_LEN,
            MPEG2_FRAME_HEAD_LEN);
        CHECK_AND_RETURN_LOG(ret == EOK, "Buffer End Copy buffer failed");
        bufferSize -= MPEG2_FRAME_HEAD_LEN;
        pBuffer -= MPEG2_FRAME_HEAD_LEN;
        pPrereadBuffer_ = 0;
    } while (pPrereadBuffer_ != prereadBufferSize_);
    mpegUnit_->resize(bufferSize);
}

const uint8_t *MpegReader::Mpeg2Detector::GetMpegTypeAddr(const uint8_t *bufferAddr)
{
    auto pos1 = std::search(bufferAddr, bufferAddr + MPEG2_SEQUENCE_HEAD_LEN + 1,
        std::begin(MPEG2_SEQUENCE_HEAD), std::end(MPEG2_SEQUENCE_HEAD));
    auto size = std::distance(bufferAddr, pos1);
    if (size == 0) {
        return nullptr;
    }
    auto pos = std::search(bufferAddr, bufferAddr + MPEG2_FRAME_HEAD_LEN + 1,
        std::begin(MPEG2_FRAME_HEAD), std::end(MPEG2_FRAME_HEAD));
    return pos + MPEG2_FRAME_HEAD_LEN + MPEG2_TYPE_OFFEST;
}

bool MpegReader::Mpeg2Detector::IsI(uint8_t mpegType)
{
    return (mpegType == M2V_I) ? true : false;
}
uint8_t MpegReader::Mpeg2Detector::GetMpegType(const uint8_t *bufferAddr)
{
    if (bufferAddr == nullptr) {
        return 1;
    }
    uint8_t flagi =  ((*bufferAddr) & 0x38) == 0x08;
    uint8_t flagp =  ((*bufferAddr) & 0x38) == 0x08;
    uint8_t flagb =  ((*bufferAddr) & 0x38) == 0x18;
    if (flagi) {
        return 1;
    }
    if (flagp) {
        return 0;
    }
    if (flagb) {
        return 0;
    }
    return 0;
}

void MpegReader::Mpeg4MetaUnitReader::PrereadFile()
{
    CHECK_AND_RETURN_LOG(prereadBuffer_, "Preread buffer is nallptr");
    inputFile_->read(reinterpret_cast<char *>(prereadBuffer_.get() + MPEG4_FRAME_HEAD_LEN),
        PREREAD_BUFFER_SIZE);
    prereadBufferSize_ = inputFile_->gcount() + MPEG4_FRAME_HEAD_LEN;
    pPrereadBuffer_ = MPEG2_FRAME_HEAD_LEN;
}

int32_t MpegReader::Mpeg4MetaUnitReader::ReadMpegUnit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Got a invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mpegUnit_, AV_ERR_INVALID_VAL, "Mpeg unit buffer is nullptr");
    bufferSize = mpegUnit_->size();
    memcpy_s(bufferAddr, bufferSize, mpegUnit_->data(), bufferSize);

    if (!IsEOF()) {
        isEosFrame = false;
        PrereadMpeg4Unit();
    } else {
        isEosFrame = true;
        mpegUnit_->resize(0);
    }
    return AV_ERR_OK;
}

MpegReader::Mpeg4MetaUnitReader::Mpeg4MetaUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    prereadBuffer_ = std::make_unique<uint8_t []>(PREREAD_BUFFER_SIZE + MPEG4_FRAME_HEAD_LEN);
    PrereadFile();

    mpegUnit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadMpeg4Unit();
}

bool MpegReader::Mpeg4MetaUnitReader::IsEOS()
{
    return IsEOF() && mpegUnit_->empty();
}

bool MpegReader::Mpeg4MetaUnitReader::IsEOF()
{
    return (pPrereadBuffer_ == prereadBufferSize_) && (inputFile_->peek() == EOF);
}

void MpegReader::Mpeg4MetaUnitReader::PrereadMpeg4Unit()
{
    CHECK_AND_RETURN_LOG(prereadBufferSize_ > 0, "Empty file, nothing to read");
    CHECK_AND_RETURN_LOG(mpegUnit_, "Mpeg4 unit buffer is nullptr");
    auto pBuffer = mpegUnit_->data();
    uint32_t bufferSize = 0;
    mpegUnit_->resize(MAX_NALU_SIZE);
    do {
        auto pos1 = std::search(prereadBuffer_.get() + pPrereadBuffer_ + MPEG4_FRAME_HEAD_LEN,
            prereadBuffer_.get() + prereadBufferSize_, std::begin(MPEG4_FRAME_HEAD), std::end(MPEG4_FRAME_HEAD));
        uint32_t size1 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos1);
        auto pos2 = std::search(prereadBuffer_.get() + pPrereadBuffer_, prereadBuffer_.get() +
            pPrereadBuffer_ + size1, std::begin(MPEG4_SEQUENCE_HEAD), std::end(MPEG4_SEQUENCE_HEAD));
        uint32_t size = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos2);
        if (size == 0) {
            auto pos3 = std::search(prereadBuffer_.get() + pPrereadBuffer_ + size1 + MPEG4_FRAME_HEAD_LEN,
                prereadBuffer_.get() + prereadBufferSize_, std::begin(MPEG4_FRAME_HEAD), std::end(MPEG4_FRAME_HEAD));
            uint32_t size2 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos3);
            auto ret = memcpy_s(pBuffer, size2, prereadBuffer_.get() + pPrereadBuffer_, size2);
            CHECK_AND_RETURN_LOG(ret == EOK, "First Copy buffer failed");
            pPrereadBuffer_ += size2;
            bufferSize += size2;
            pBuffer += size2;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        } else if (size1 > size) {
            auto ret = memcpy_s(pBuffer, size, prereadBuffer_.get() + pPrereadBuffer_, size);
            CHECK_AND_RETURN_LOG(ret == EOK, "Last Copy buffer failed");
            pPrereadBuffer_ += size;
            bufferSize += size;
            pBuffer += size;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        } else {
            auto ret = memcpy_s(pBuffer, size1, prereadBuffer_.get() + pPrereadBuffer_, size1);
            CHECK_AND_RETURN_LOG(ret == EOK, "Copy buffer failed");
            pPrereadBuffer_ += size1;
            bufferSize += size1;
            pBuffer += size1;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        }
        PrereadFile();
        auto ret = memcpy_s(prereadBuffer_.get(), MPEG4_FRAME_HEAD_LEN, pBuffer - MPEG4_FRAME_HEAD_LEN,
            MPEG4_FRAME_HEAD_LEN);
        CHECK_AND_RETURN_LOG(ret == EOK, "Copy buffer failed");
        bufferSize -= MPEG2_FRAME_HEAD_LEN;
        pBuffer -= MPEG2_FRAME_HEAD_LEN;
        pPrereadBuffer_ = 0;
    } while (pPrereadBuffer_ != prereadBufferSize_);
    mpegUnit_->resize(bufferSize);
}

const uint8_t *MpegReader::Mpeg4Detector::GetMpegTypeAddr(const uint8_t *bufferAddr)
{
    auto pos1 = std::search(bufferAddr, bufferAddr + MPEG4_SEQUENCE_HEAD_LEN + 1,
        std::begin(MPEG4_SEQUENCE_HEAD), std::end(MPEG4_SEQUENCE_HEAD));
    auto size = std::distance(bufferAddr, pos1);
    if (size == 0) {
        return nullptr;
    }
    auto pos = std::search(bufferAddr, bufferAddr + MPEG4_FRAME_HEAD_LEN + 1,
        std::begin(MPEG4_FRAME_HEAD), std::end(MPEG4_FRAME_HEAD));
    return pos + MPEG4_FRAME_HEAD_LEN;
}

bool MpegReader::Mpeg4Detector::IsI(uint8_t mpegType)
{
    return (mpegType == MPEG4_I) ? true : false;
}

uint8_t MpegReader::Mpeg4Detector::GetMpegType(const uint8_t *bufferAddr)
{
    if (bufferAddr == nullptr) {
        return 1;
    }
    uint8_t flagi =  ((*bufferAddr) & 0xc0) == 0x00;
    uint8_t flagp =  ((*bufferAddr) & 0xc0) == 0x40;
    uint8_t flagb =  ((*bufferAddr) & 0xc0) == 0x80;
    uint8_t flags =  ((*bufferAddr) & 0xc0) == 0xC0;
    if (flagi) {
        return 1;
    }
    if (flagp) {
        return 0;
    }
    if (flagb) {
        return 0;
    }
    if (flags) {
        return 0;
    }
    return 0;
}


int32_t H263Reader::Init(const std::shared_ptr<H263ReaderInfo> &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<std::ifstream> inputFile = std::make_unique<std::ifstream>(info->inPath.c_str(),
                                                std::ios::binary | std::ios::in);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputFile != nullptr && inputFile->is_open(),
                                      AV_ERR_INVALID_VAL, "Open input file failed");
    h263UnitReader_= std::static_pointer_cast<H263UnitReader>(std::make_shared<H263MetaUnitReader>(inputFile));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(h263UnitReader_, AV_ERR_INVALID_VAL, "h263 unit reader create failed");
    h263Detector_= std::static_pointer_cast<H263Detector>(std::make_shared<H263Detector>());
    UNITTEST_CHECK_AND_RETURN_RET_LOG(h263Detector_, AV_ERR_INVALID_VAL, "h263 detector create failed");
    return AV_ERR_OK;
}

void H263Reader::FillBufferAttr(OH_AVCodecBufferAttr &attr, int32_t frameSize, uint8_t h263Type,
                                bool isEosFrame)
{
    attr.size += frameSize;
    attr.pts = GetTimeUs();
    attr.flags |= h263Detector_->IsI(h263Type) ? AVCODEC_BUFFER_FLAG_SYNC_FRAME : 0;
    if (isEosFrame) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        std::cout << "Input EOS Frame, frameCount = " << (frameInputCount_ + 1) << std::endl;
    }
}

int32_t H263Reader::FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int32_t frameSize = 0;
    bool isEosFrame = false;
    auto ret = h263UnitReader_->ReadH263Unit(bufferAddr, frameSize, isEosFrame);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadH263Unit failed");
    uint8_t h263Type = h263Detector_->GetH263Type(h263Detector_->GetH263TypeAddr(bufferAddr));
    bufferAddr += frameSize;
    FillBufferAttr(attr, frameSize, h263Type, isEosFrame);
    frameInputCount_++;
    return AV_ERR_OK;
}

bool H263Reader::IsEOS()
{
    return h263UnitReader_ ? h263UnitReader_->IsEOS() : true;
}

uint8_t const * H263Reader::H263UnitReader::GetNextH263UnitAddr()
{
    CHECK_AND_RETURN_RET_LOG(h263Unit_ != nullptr, nullptr, "h263Unit_ is nullptr");
    return h263Unit_->data();
}


int32_t H263Reader::H263MetaUnitReader::ReadH263Unit(uint8_t *bufferAddr, int32_t &bufferSize, bool &isEosFrame)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL, "Got a invalid buffer addr");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(h263Unit_, AV_ERR_INVALID_VAL, "h263 unit buffer is nullptr");
    bufferSize = h263Unit_->size();
    memcpy_s(bufferAddr, bufferSize, h263Unit_->data(), bufferSize);

    if (!IsEOF()) {
        isEosFrame = false;
        PrereadH263Unit();
    } else {
        isEosFrame = true;
        h263Unit_->resize(0);
    }
    return AV_ERR_OK;
}

H263Reader::H263MetaUnitReader::H263MetaUnitReader(std::shared_ptr<std::ifstream> inputFile)
{
    inputFile_ = inputFile;
    prereadBuffer_ = std::make_unique<uint8_t []>(PREREAD_BUFFER_SIZE); // + MPEG2_FRAME_HEAD_LEN);
    PrereadFile();

    h263Unit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);
    PrereadH263Unit();
}

bool H263Reader::H263MetaUnitReader::IsEOS()
{
    return IsEOF() && h263Unit_->empty();
}

bool H263Reader::H263MetaUnitReader::IsEOF()
{
    return (pPrereadBuffer_ == prereadBufferSize_) && (inputFile_->peek() == EOF);
}

uint8_t* H263Reader::H263MetaUnitReader::GetDelimiterPos(uint8_t* addrstart, uint8_t* addrend)
{
    uint8_t* posMin = std::search(addrstart, addrend, std::begin(H263_HEAD_0), std::end(H263_HEAD_0));
    auto pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_1), std::end(H263_HEAD_1));
    if (pos1<posMin)
        posMin=pos1;
    pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_2), std::end(H263_HEAD_2));
    if (pos1<posMin)
        posMin=pos1;
    pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_3), std::end(H263_HEAD_3));
    if (pos1<posMin)
        posMin=pos1;
    return posMin;
}

void H263Reader::H263MetaUnitReader::PrereadH263Unit()
{
    CHECK_AND_RETURN_LOG(prereadBufferSize_ > 0, "Empty file, nothing to read");
    CHECK_AND_RETURN_LOG(h263Unit_, "h263 unit buffer is nullptr");
    auto pBuffer = h263Unit_->data();
    uint32_t bufferSize = 0;
    h263Unit_->resize(MAX_NALU_SIZE);
    do {
        uint8_t* pos1 = GetDelimiterPos(prereadBuffer_.get() + pPrereadBuffer_,
                                        prereadBuffer_.get() + prereadBufferSize_);
        uint32_t size1 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos1);
        auto pos2 = GetDelimiterPos(prereadBuffer_.get() + pPrereadBuffer_,
                                    prereadBuffer_.get()+ pPrereadBuffer_ + size1);
        uint32_t size = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos2);
        if (size == 0) {
            auto pos3 = GetDelimiterPos(prereadBuffer_.get() + pPrereadBuffer_ + size1 + H263_HEAD_LEN,
                                        prereadBuffer_.get() + prereadBufferSize_);
            uint32_t size2 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos3);
            auto ret = memcpy_s(pBuffer, size2, prereadBuffer_.get() + pPrereadBuffer_, size2);
            CHECK_AND_RETURN_LOG(ret == EOK, "First Copy buffer failed");
            pPrereadBuffer_ += size2;
            bufferSize += size2;
            pBuffer += size2;
            UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
        } else {
            if (size1 > size) {
                auto ret = memcpy_s(pBuffer, size, prereadBuffer_.get() + pPrereadBuffer_, size);
                CHECK_AND_RETURN_LOG(ret == EOK, "Last Copy buffer failed");
                pPrereadBuffer_ += size;
                bufferSize += size;
                pBuffer += size;
                UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
            } else {
                auto ret = memcpy_s(pBuffer, size1, prereadBuffer_.get() + pPrereadBuffer_, size1);
                CHECK_AND_RETURN_LOG(ret == EOK, "Comom Copy buffer failed");
                pPrereadBuffer_ += size1;
                bufferSize += size1;
                pBuffer += size1;
                UNITTEST_CHECK_AND_BREAK_LOG((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof(), "");
            }
        }
        PrereadFile();
        auto ret = memcpy_s(prereadBuffer_.get(), H263_HEAD_LEN, pBuffer - H263_HEAD_LEN, H263_HEAD_LEN);
        CHECK_AND_RETURN_LOG(ret == EOK, "Buffer End Copy buffer failed");
        bufferSize -= H263_HEAD_LEN;
        pBuffer -= H263_HEAD_LEN;
        pPrereadBuffer_ = 0;
    }
    while (pPrereadBuffer_ != prereadBufferSize_);
    h263Unit_->resize(bufferSize);
}

uint8_t* H263Reader::H263Detector::GetDelimiterPos(uint8_t* addrstart, uint8_t* addrend)
{
    uint8_t* posMin = std::search(addrstart, addrend, std::begin(H263_HEAD_0), std::end(H263_HEAD_0));
    auto pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_1), std::end(H263_HEAD_1));
    if (pos1 < posMin)
        posMin = pos1;
    pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_2), std::end(H263_HEAD_2));
    if (pos1 < posMin)
        posMin = pos1;
    pos1 = std::search(addrstart, addrend, std::begin(H263_HEAD_3), std::end(H263_HEAD_3));
    if (pos1 < posMin)
        posMin = pos1;
    return posMin;
}

const uint8_t* H263Reader::H263Detector::GetH263TypeAddr(const uint8_t *bufferAddr)
{
    auto pos1 = GetDelimiterPos(const_cast<uint8_t*>(bufferAddr),
                                const_cast<uint8_t*>(bufferAddr) + H263_HEAD_LEN + 1 /*prereadBufferSize_*/);
    auto size = std::distance(const_cast<uint8_t*>(bufferAddr), pos1);
    if (size == 0) {
        return nullptr;
    }
    auto pos = GetDelimiterPos(const_cast<uint8_t*>(bufferAddr), const_cast<uint8_t*>(bufferAddr) + size);
    return pos;
}

bool H263Reader::H263Detector::IsI(uint8_t h263Type)
{
    return (h263Type == H263_I) ? true : false;
}

uint8_t H263Reader::H263Detector::GetH263Type(const uint8_t *bufferAddr)
{
    if (bufferAddr == nullptr) {
        return 1;
    }
    if ((bufferAddr[H263_OFFSET_4] & H263_HEAD_MASK_4_1) != H263_HEAD_MASK_4_1) {
        return (bufferAddr[H263_OFFSET_4] & H263_HEAD_MASK_4_2) == 0 ? 1 : 0;
    }
    if ((bufferAddr[H263_OFFSET_5] & H263_HEAD_MASK_5_1) == 0) {
        return (bufferAddr[H263_OFFSET_5] & H263_HEAD_MASK_5_2) == 0 ? 1 : 0;
    }
    return (bufferAddr[H263_OFFSET_7] & H263_HEAD_MASK_4_1) == 0 ? 1 : 0;
}

void H263Reader::H263MetaUnitReader::PrereadFile()
{
    CHECK_AND_RETURN_LOG(prereadBuffer_, "Preread buffer is nallptr");
    inputFile_->read((char *)prereadBuffer_.get(), PREREAD_BUFFER_SIZE);
    prereadBufferSize_ = inputFile_->gcount(); //+ MPEG4_FRAME_HEAD_LEN
    pPrereadBuffer_ = 0; //MPEG2_FRAME_HEAD_LEN;
}


int32_t AvccReader::Init(const std::shared_ptr<AvccReaderInfo> &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<std::ifstream> inputFile = std::make_unique<std::ifstream>(info->inPath.data(),
                                                                               std::ios::binary | std::ios::in);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputFile != nullptr && inputFile->is_open(),
                                      AV_ERR_INVALID_VAL, "Open input file failed");

    nalUnitReader_ = std::static_pointer_cast<NalUnitReader>(std::make_shared<AvccNalUnitReader>(inputFile));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(nalUnitReader_, AV_ERR_INVALID_VAL, "Nal unit reader create failed");

    nalDetector_ = info->isAvcStream ?
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

bool AvccReader::CheckFillBuffer(uint8_t naluType)
{
    if (nalDetector_->IsXPS(naluType)) {
        return false;
    } else if (nalDetector_->IsFullVCL(naluType,
                                       nalDetector_->GetNalTypeAddr(nalUnitReader_->GetNextNalUnitAddr()))) {
        return false;
    } else if (IsEOS()) {
        return false;
    } else {
        return true;
    }
}

int32_t AvccReader::FillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        int32_t frameSize = 0;
        bool isEosFrame = false;
        auto ret = nalUnitReader_->ReadNalUnit(bufferAddr, frameSize, isEosFrame);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadNalUnit failed");
        uint8_t naluType = nalDetector_->GetNalType(nalDetector_->GetNalTypeAddr(bufferAddr));
        bufferAddr += frameSize;
        FillBufferAttr(attr, frameSize, naluType, isEosFrame);
        UNITTEST_CHECK_AND_BREAK_LOG(CheckFillBuffer(naluType), "FillBuffer stop running");
    } while (true);
    frameInputCount_++;

    return AV_ERR_OK;
}

int32_t AvccReader::KeepFillBuffer(uint8_t *bufferAddr, OH_AVCodecBufferAttr &attr)
{
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        int32_t frameSize = 0;
        bool isEosFrame = false;
        auto ret = nalUnitReader_->ReadNalUnit(bufferAddr, frameSize, isEosFrame);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_INVALID_VAL, "ReadNalUnit failed");
        uint8_t naluType = nalDetector_->GetNalType(nalDetector_->GetNalTypeAddr(bufferAddr));
        bufferAddr += frameSize;
        if (isEosFrame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // 3000ms
        }
        FillBufferAttr(attr, frameSize, naluType, isEosFrame);
        UNITTEST_CHECK_AND_BREAK_LOG(CheckFillBuffer(naluType), "FillBuffer stop running");
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