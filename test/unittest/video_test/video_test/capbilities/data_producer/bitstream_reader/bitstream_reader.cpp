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

constexpr uint8_t AVC_NAL_IDR = 5;
constexpr uint8_t AVC_NAL_SPS = 7;
constexpr uint8_t AVC_NAL_PPS = 8;
constexpr uint8_t HEVC_NAL_IDR = 19;
constexpr uint8_t HEVC_NAL_VPS = 32;
constexpr uint8_t HEVC_NAL_SPS = 33;
constexpr uint8_t HEVC_NAL_PPS = 34;
constexpr uint8_t HEVC_NAL_FD_NUL = 38;
constexpr uint8_t HEVC_NAL_SEI_PREFIX = 39;
constexpr uint8_t HEVC_NAL_SEI_SUFFIX = 40;
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t BitstreamReader::FillBuffer(CodecBufferInfo &bufferInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(inputFile_ != nullptr && inputFile_->is_open(),
        AVCODEC_SAMPLE_ERR_ERROR, "Input file is not open!");

    auto bufferAddr = static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
        OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer)) :
        OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(bufferInfo.buffer));
    CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Got invalid buffer");
    bufferInfo.attr.size = 0;

    bool keepRead = true;
    do {
        int32_t ret = 0;
        int32_t frameSize = 0;
        int32_t naluType = 0;
        if (sampleInfo_.dataProducerInfo.bitstreamType == BITSTREAM_TYPE_AVCC) {
            ret = ReadAvccSample(bufferAddr, frameSize);
            naluType = GetNaluType(bufferAddr[AVCC_FRAME_HEAD_LEN]);
        } else if (sampleInfo_.dataProducerInfo.bitstreamType == BITSTREAM_TYPE_ANNEXB) {
            ret = ReadAnnexbSample(bufferAddr, frameSize);
            naluType = GetNaluType(bufferAddr);
        }
        CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Sample failed");

        if (IsCodecData(naluType)) {
            bufferInfo.attr.flags |= AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        } else if (IsIDR(naluType)) {
            bufferInfo.attr.flags |= AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
            keepRead = false;
        } else {
            keepRead = false;
        }
        bufferInfo.attr.pts = frameCount_ *
            ((sampleInfo_.frameInterval == 0) ? 1 : sampleInfo_.frameInterval) * 1000; // 1000: 1ms to us
        bufferInfo.attr.size += frameSize;
        bufferAddr += frameSize;
    } while (keepRead);

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

        if ((pPrereadBuffer_ != prereadBufferSize_) || inputFile_->eof()) {
            break;
        }

        PrereadFile();
        ret = memcpy_s(prereadBuffer_.get(), ANNEXB_FRAME_HEAD_LEN,
            pBuffer - ANNEXB_FRAME_HEAD_LEN, ANNEXB_FRAME_HEAD_LEN);
        CHECK_AND_RETURN_RET_LOG(ret == EOK, AVCODEC_SAMPLE_ERR_ERROR, "Copy buffer failed");
        if (std::search(pBuffer - ANNEXB_FRAME_HEAD_LEN, pBuffer,
            std::begin(ANNEXB_FRAME_HEAD), std::end(ANNEXB_FRAME_HEAD)) == pBuffer) {
            bufferSize -= ANNEXB_FRAME_HEAD_LEN;
            pBuffer -= ANNEXB_FRAME_HEAD_LEN;
            pPrereadBuffer_ = 0;
        }
    } while (true);
    if (prereadBuffer_.get()[pPrereadBuffer_ - 1] == 0) {
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
    return sampleInfo_.codecMime == MIME_VIDEO_AVC ? (value & 0x1F) : ((value & 0x7E) >> 1);
}

inline uint8_t BitstreamReader::GetNaluType(const uint8_t *const bufferAddr)
{
    auto pos = std::search(bufferAddr, bufferAddr + ANNEXB_FRAME_HEAD_LEN + 1,
        std::begin(ANNEXB_FRAME_HEAD), std::end(ANNEXB_FRAME_HEAD));
    return GetNaluType(*(pos + ANNEXB_FRAME_HEAD_LEN));
}

bool BitstreamReader::IsCodecData(uint8_t naluType)
{
    bool isH264Stream = sampleInfo_.codecMime == MIME_VIDEO_AVC;
    if ((isH264Stream && ((naluType == AVC_NAL_SPS) || (naluType == AVC_NAL_PPS))) ||
        (!isH264Stream && ((naluType == HEVC_NAL_VPS) || (naluType == HEVC_NAL_SPS) || (naluType == HEVC_NAL_PPS) ||
                           (naluType == HEVC_NAL_FD_NUL) || (naluType == HEVC_NAL_SEI_PREFIX) ||
                           (naluType == HEVC_NAL_SEI_SUFFIX)))) {
        return true;
    }
    return false;
}

bool BitstreamReader::IsIDR(uint8_t naluType)
{
    bool isH264Stream = sampleInfo_.codecMime == MIME_VIDEO_AVC;
    if ((isH264Stream && (naluType == AVC_NAL_IDR)) ||
        (!isH264Stream && (naluType == HEVC_NAL_IDR))) {
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