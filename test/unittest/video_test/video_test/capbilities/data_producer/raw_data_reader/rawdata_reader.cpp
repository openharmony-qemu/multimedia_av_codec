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
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t RawdataReader::FillBuffer(CodecBufferInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(inputFile_ != nullptr && inputFile_->is_open(),
        AVCODEC_SAMPLE_ERR_ERROR, "Input file is not open!");

    uint8_t *bufferAddr = nullptr;
    if (info.bufferAddr != nullptr) {
        bufferAddr = info.bufferAddr;
    } else {
        bufferAddr = static_cast<uint8_t>(sampleInfo_->codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
            OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(info.buffer)) :
            OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(info.buffer));
        CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Invalid buffer address");
    }
    int32_t bufferSize = GetBufferSize();
    inputFile_->read(reinterpret_cast<char *>(bufferAddr), bufferSize);
    info.attr.size = inputFile_->gcount();

    return AVCODEC_SAMPLE_ERR_OK;
}

inline int32_t RawdataReader::GetBufferSize()
{
    int32_t size = sampleInfo_->pixelFormat == AV_PIXEL_FORMAT_RGBA ?
        sampleInfo_->videoWidth * sampleInfo_->videoHeight * 3 :          // RGBA buffer size
        sampleInfo_->videoWidth * sampleInfo_->videoHeight * 3 / 2;       // YUV420 buffer size
    return sampleInfo_->profile == HEVC_PROFILE_MAIN_10 ? size * 2 : size;
}

bool RawdataReader::IsEOS()
{
    return inputFile_->peek() == EOF;
}
} // Sample
} // MediaAVCodec
} // OHOS