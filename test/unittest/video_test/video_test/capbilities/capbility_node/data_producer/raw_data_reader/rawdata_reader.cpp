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

#include "rawdata_reader.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "RawdataReader"};
constexpr uint8_t RATIO_10BIT     = 2;
constexpr uint8_t RATIO_8BIT      = 1;
constexpr uint8_t RATIO_CHROMA_UV = 2;
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t RawdataReader::FillBuffer(CodecBufferInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(inputFile_ != nullptr && inputFile_->is_open(),
        AVCODEC_SAMPLE_ERR_ERROR, "Input file is not open!");

    uint8_t *bufferAddr = info.bufferAddr;
    CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Invalid buffer address");

    int32_t format = sampleInfo_->pixelFormat;
    switch (format) {
        case AV_PIXEL_FORMAT_RGBA1010102:
            ReadInputBufferWithStrideRGBA(bufferAddr);
            break;
        case AV_PIXEL_FORMAT_RGBA:
            ReadInputBufferWithStrideRGBA(bufferAddr);
            break;
        case AV_PIXEL_FORMAT_YUVI420:
            ReadInputBufferWithStrideYUVI420(bufferAddr);
            break;
        default:
            ReadInputBufferWithStrideYUV420(bufferAddr);
            break;
    }
    info.attr.size = GetBufferSize();

    return AVCODEC_SAMPLE_ERR_OK;
}

void RawdataReader::ReadInputBufferWithStrideRGBA(uint8_t *bufferAddr)
{
    int32_t width = sampleInfo_->videoWidth;
    for (uint32_t row = 0; row < sampleInfo_->videoSliceHeight; row++) {
        inputFile_->read(reinterpret_cast<char *>(bufferAddr), width * 4); // 4: RGBA 4 channels
        bufferAddr += sampleInfo_->videoStrideWidth;
    }
}

void RawdataReader::ReadInputBufferWithStrideYUVI420(uint8_t *bufferAddr)
{
    int32_t width = sampleInfo_->videoWidth *
        ((sampleInfo_->codecMime == OH_AVCODEC_MIMETYPE_VIDEO_HEVC && sampleInfo_->profile == HEVC_PROFILE_MAIN_10) ?
         RATIO_10BIT : RATIO_8BIT);
    // Y
    for (uint32_t row = 0; row < sampleInfo_->videoSliceHeight; row++) {
        inputFile_->read(reinterpret_cast<char *>(bufferAddr), width);
        bufferAddr += sampleInfo_->videoStrideWidth;
    }
    // U/V
    for (uint32_t row = 0; row < sampleInfo_->videoSliceHeight; row++) {
        inputFile_->read(reinterpret_cast<char *>(bufferAddr), width / RATIO_CHROMA_UV);
        bufferAddr += sampleInfo_->videoStrideWidth / RATIO_CHROMA_UV;
    }
}

void RawdataReader::ReadInputBufferWithStrideYUV420(uint8_t *bufferAddr)
{
    int32_t width = sampleInfo_->videoWidth *
        ((sampleInfo_->codecMime == OH_AVCODEC_MIMETYPE_VIDEO_HEVC && sampleInfo_->profile == HEVC_PROFILE_MAIN_10) ?
         RATIO_10BIT : RATIO_8BIT);
    // Y
    for (uint32_t row = 0; row < sampleInfo_->videoSliceHeight; row++) {
        inputFile_->read(reinterpret_cast<char *>(bufferAddr), width);
        bufferAddr += sampleInfo_->videoStrideWidth;
    }
    // UV
    for (uint32_t row = 0; row < sampleInfo_->videoSliceHeight / RATIO_CHROMA_UV; row++) {
        inputFile_->read(reinterpret_cast<char *>(bufferAddr), width);
        bufferAddr += sampleInfo_->videoStrideWidth;
    }
}

inline int32_t RawdataReader::GetBufferSize()
{
    int32_t size = sampleInfo_->pixelFormat == AV_PIXEL_FORMAT_RGBA ?
        sampleInfo_->videoWidth * sampleInfo_->videoHeight * 4 :          // RGBA8888 buffer size
        sampleInfo_->videoWidth * sampleInfo_->videoHeight * 3 / 2;       // YUV420 buffer size
    size *= (sampleInfo_->codecMime == OH_AVCODEC_MIMETYPE_VIDEO_HEVC && sampleInfo_->profile == HEVC_PROFILE_MAIN_10) ?
        RATIO_10BIT : RATIO_8BIT;
    return size;
}

bool RawdataReader::IsEOS()
{
    return inputFile_->peek() == EOF;
}
} // Sample
} // MediaAVCodec
} // OHOS