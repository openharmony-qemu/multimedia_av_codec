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
int32_t BitstreamReader::FillBuffer(CodecBufferInfo &bufferInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(inputFile_ != nullptr && inputFile_->is_open(),
        AVCODEC_SAMPLE_ERR_ERROR, "Input file is not open!");

    auto bufferAddr = static_cast<uint8_t>(sampleInfo_->codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
        OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer)) :
        OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(bufferInfo.buffer));
    CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Got invalid buffer");

    do {
        int32_t ret = 0;
        int32_t frameSize = 0;
        int32_t naluType = 0;
        if (sampleInfo_->dataProducerInfo.bitstreamType == BITSTREAM_TYPE_AVCC) {
            ret = ReadAvccSample(bufferAddr, frameSize);
            naluType = GetNaluType(bufferAddr[AVCC_FRAME_HEAD_LEN]);
        } else if (sampleInfo_->dataProducerInfo.bitstreamType == BITSTREAM_TYPE_ANNEXB) {
            ret = ReadAnnexbSample(bufferAddr, frameSize);
            naluType = GetNaluType(bufferAddr);
        }
        CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Sample failed");

        bufferInfo.attr.size += frameSize;
        bufferAddr += frameSize;
        bufferInfo.attr.flags |= IsXPS(naluType) ? AVCODEC_BUFFER_FLAGS_CODEC_DATA : 0;
        bufferInfo.attr.flags |= IsIDR(naluType) ? AVCODEC_BUFFER_FLAGS_SYNC_FRAME : 0;
        CHECK_AND_BREAK(!IsVCL(naluType));
    } while (true);

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t BitstreamReader::ReadAvccSample(uint8_t *bufferAddr, int32_t &bufferSize)
{
    uint8_t len[AVCC_FRAME_HEAD_LEN] = {};
    (void)inputFile_->read(reinterpret_cast<char *>(len), AVCC_FRAME_HEAD_LEN);
    // 0 1 2 3: avcc frame head byte offset; 8 16 24: avcc frame head bit offset
    bufferSize = static_cast<uint32_t>((len[3]) | (len[2] << 8) | (len[1] << 16) | (len[0] << 24));

    (void)inputFile_->read(reinterpret_cast<char *>(bufferAddr + AVCC_FRAME_HEAD_LEN), bufferSize);
    if (ANNEXB_INPUT_ONLY) {
        ToAnnexb(bufferAddr);
    }

    bufferSize += AVCC_FRAME_HEAD_LEN;
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t BitstreamReader::ReadAnnexbSample(uint8_t *bufferAddr, int32_t &bufferSize)
{
    if (pPrereadBuffer_ >= prereadBufferSize_) {
        PrereadFile();
        CHECK_AND_RETURN_RET_LOG(prereadBufferSize_ > 0, AVCODEC_SAMPLE_ERR_ERROR, "Empty file, nothing to read");
    }

    auto pBuffer = bufferAddr;
    do {
        auto pos = std::search(prereadBuffer_.get() + pPrereadBuffer_ + (bufferSize > 0 ? 0 : ANNEXB_FRAME_HEAD_LEN),
            prereadBuffer_.get() + prereadBufferSize_, std::begin(ANNEXB_FRAME_HEAD), std::end(ANNEXB_FRAME_HEAD));
        uint32_t size = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos);
        auto ret = memcpy_s(pBuffer, size, prereadBuffer_.get() + pPrereadBuffer_, size);
        CHECK_AND_RETURN_RET_LOG(ret == EOK, AVCODEC_SAMPLE_ERR_ERROR, "Copy buffer failed");
        pPrereadBuffer_ += size;
        bufferSize += size;
        pBuffer += size;

        CHECK_AND_BREAK((pPrereadBuffer_ == prereadBufferSize_) && !inputFile_->eof());

        PrereadFile();
        ret = memcpy_s(prereadBuffer_.get(), ANNEXB_FRAME_HEAD_LEN,
            pBuffer - ANNEXB_FRAME_HEAD_LEN, ANNEXB_FRAME_HEAD_LEN);
        CHECK_AND_RETURN_RET_LOG(ret == EOK, AVCODEC_SAMPLE_ERR_ERROR, "Copy buffer failed");
        CHECK_AND_CONTINUE(std::search(pBuffer - ANNEXB_FRAME_HEAD_LEN, pBuffer,
            std::begin(ANNEXB_FRAME_HEAD), std::end(ANNEXB_FRAME_HEAD)) != pBuffer);

        bufferSize -= ANNEXB_FRAME_HEAD_LEN;
        pBuffer -= ANNEXB_FRAME_HEAD_LEN;
        pPrereadBuffer_ = 0;
    } while (true);
    if (prereadBuffer_.get()[pPrereadBuffer_ - 1] == 0 && !IsEOS()) {
        bufferSize--;
        pPrereadBuffer_--;
    }
    return AVCODEC_SAMPLE_ERR_OK;
}

void BitstreamReader::PrereadFile()
{
    if (prereadBuffer_ == nullptr) {
        prereadBuffer_ = std::make_unique<uint8_t []>(PREREAD_BUFFER_SIZE + ANNEXB_FRAME_HEAD_LEN);
    }
    inputFile_->read(reinterpret_cast<char *>(
        prereadBuffer_.get() + ANNEXB_FRAME_HEAD_LEN), PREREAD_BUFFER_SIZE);
    prereadBufferSize_ = inputFile_->gcount() + ANNEXB_FRAME_HEAD_LEN;
    pPrereadBuffer_ = ANNEXB_FRAME_HEAD_LEN;
}

int32_t BitstreamReader::ToAnnexb(uint8_t *bufferAddr)
{
    CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Buffer address is null");

    bufferAddr[0] = 0;
    bufferAddr[1] = 0;
    bufferAddr[2] = 0;      // 2: annexB frame head offset 2
    bufferAddr[3] = 1;      // 3: annexB frame head offset 3
    return AVCODEC_SAMPLE_ERR_OK;
}

inline uint8_t BitstreamReader::GetNaluType(uint8_t value)
{
    return sampleInfo_->codecMime == OH_AVCODEC_MIMETYPE_VIDEO_AVC ? (value & 0x1F) : ((value & 0x7E) >> 1);
}

inline uint8_t BitstreamReader::GetNaluType(const uint8_t *const bufferAddr)
{
    auto pos = std::search(bufferAddr, bufferAddr + ANNEXB_FRAME_HEAD_LEN + 1,
        std::begin(ANNEXB_FRAME_HEAD), std::end(ANNEXB_FRAME_HEAD));
    return GetNaluType(*(pos + ANNEXB_FRAME_HEAD_LEN));
}

bool BitstreamReader::IsXPS(uint8_t naluType)
{
    bool isH264Stream = sampleInfo_->codecMime == OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    if ((isH264Stream && ((naluType == AVC_SPS) || (naluType == AVC_PPS))) ||
        (!isH264Stream && ((naluType >= HEVC_VPS_NUT) && (naluType <= HEVC_PPS_NUT)))) {
        return true;
    }
    return false;
}

bool BitstreamReader::IsIDR(uint8_t naluType)
{
    bool isH264Stream = sampleInfo_->codecMime == OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    if ((isH264Stream && (naluType == AVC_IDR)) ||
        (!isH264Stream && ((naluType >= HEVC_IDR_W_RADL) && (naluType <= HEVC_CRA_NUT)))) {
        return true;
    }
    return false;
}

bool BitstreamReader::IsVCL(uint8_t nalType)
{
    bool isH264Stream = sampleInfo_->codecMime == OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    if ((isH264Stream && (nalType >= AVC_NON_IDR && nalType <= AVC_IDR)) ||
        (!isH264Stream && (nalType >= HEVC_TRAIL_N && nalType <= HEVC_CRA_NUT))) {
        return true;
    }
    return false;
}

bool BitstreamReader::IsEOS()
{
    return (pPrereadBuffer_ == prereadBufferSize_) && (inputFile_->peek() == EOF);
}
} // Sample
} // MediaAVCodec
} // OHOS