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
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "BitstreamReader"};
constexpr uint8_t AVCC_FRAME_HEAD_LEN = 4;

constexpr uint8_t AVC_NAL_SPS = 7;
constexpr uint8_t AVC_NAL_PPS = 8;
constexpr uint8_t HEVC_NAL_VPS = 32;
constexpr uint8_t HEVC_NAL_SPS = 33;
constexpr uint8_t HEVC_NAL_PPS = 34;
// constexpr uint8_t HEVC_NAL_FD_NUL = 38;
// constexpr uint8_t HEVC_NAL_SEI_PREFIX = 39;
// constexpr uint8_t HEVC_NAL_SEI_SUFFIX = 40;
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
BitstreamReader::BitstreamReader(BitstreamType &type)
{
    bitstreamType_ = type;
}

int32_t BitstreamReader::ReadSample(CodecBufferInfo &bufferInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);

    int32_t ret = 0;
    if (sampleInfo_.dataProducerInfo.bitstreamType == BITSTREAM_TYPE_AVCC) {
        ret = ReadAvccSample(bufferInfo);
    } else if (sampleInfo_.dataProducerInfo.bitstreamType == BITSTREAM_TYPE_ANNEXB) {
        ret = ReadAnnexbSample(bufferInfo);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Sample failed");

    frameCount_++;
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t BitstreamReader::ReadAvccSample(CodecBufferInfo &bufferInfo)
{
    CHECK_AND_RETURN_RET_LOG(inputFile_ != nullptr && inputFile_->is_open(),
        AVCODEC_SAMPLE_ERR_ERROR, "Input file is not open!");

    if (frameCount_ > sampleInfo_.maxFrames) {
        bufferInfo.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        return AVCODEC_SAMPLE_ERR_OK;
    }

    char ch[AVCC_FRAME_HEAD_LEN] = {};
    (void)inputFile_->read(ch, AVCC_FRAME_HEAD_LEN);
    // 0 1 2 3: avcc frame head byte offset; 8 16 24: avcc frame head bit offset
    uint32_t bufferSize = static_cast<uint32_t>(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << 8) |
        ((ch[1] & 0xFF) << 16) | ((ch[0] & 0xFF) << 24));

    auto bufferAddr = static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
                      OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer)) :
                      OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(bufferInfo.buffer));
    (void)inputFile_->read(reinterpret_cast<char *>(bufferAddr + AVCC_FRAME_HEAD_LEN), bufferSize);
    if (ANNEXB_INPUT_ONLY) {
        ToAnnexb(bufferAddr);
    }

    bufferInfo.attr.flags = IsCodecData(bufferAddr) ? AVCODEC_BUFFER_FLAGS_CODEC_DATA : AVCODEC_BUFFER_FLAGS_NONE;
    if (inputFile_->eof() && !Repeat()) {
        bufferInfo.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    }
    bufferInfo.attr.size = bufferSize + AVCC_FRAME_HEAD_LEN;
    bufferInfo.attr.pts = frameCount_ *
        ((sampleInfo_.frameInterval == 0) ? 1 : sampleInfo_.frameInterval) * 1000; // 1000: 1ms to us

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t BitstreamReader::ReadAnnexbSample(CodecBufferInfo &bufferInfo)
{
    (void)bufferInfo;
    return AVCODEC_SAMPLE_ERR_OK;
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

bool BitstreamReader::IsCodecData(const uint8_t *const bufferAddr)
{
    bool isH264Stream = sampleInfo_.codecMime == MIME_VIDEO_AVC;

    // 0x1F: avc nulu type mask; 0x7E: hevc nalu type mask
    uint8_t naluType = isH264Stream ?
        (bufferAddr[AVCC_FRAME_HEAD_LEN] & 0x1F) : ((bufferAddr[AVCC_FRAME_HEAD_LEN] & 0x7E) >> 1);

    if ((isH264Stream && ((naluType == AVC_NAL_SPS) || (naluType == AVC_NAL_PPS))) ||
        (!isH264Stream && ((naluType == HEVC_NAL_VPS) || (naluType == HEVC_NAL_SPS) || (naluType == HEVC_NAL_PPS)))) {
        return true;
    }
    return false;
}
} // Sample
} // MediaAVCodec
} // OHOS