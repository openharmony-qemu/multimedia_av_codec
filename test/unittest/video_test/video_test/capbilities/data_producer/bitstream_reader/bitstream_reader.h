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

#ifndef AVCODEC_SAMPLE_DATA_PRODUCER_BITSTREAM_READER_H
#define AVCODEC_SAMPLE_DATA_PRODUCER_BITSTREAM_READER_H
#include "data_producer_base.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class BitstreamReader : public DataProducerBase {
private:
    int32_t FillBuffer(CodecBufferInfo &bufferInfo) override;
    int32_t ReadAvccSample(uint8_t *bufferAddr, int32_t &bufferSize);
    int32_t ReadAnnexbSample(uint8_t *bufferAddr, int32_t &bufferSize);
    void PrereadFile();
    int32_t ToAnnexb(uint8_t *bufferAddr);
    uint8_t GetNaluType(uint8_t value);
    uint8_t GetNaluType(const uint8_t *const bufferAddr);
    bool IsXPS(uint8_t naluType);
    bool IsIDR(uint8_t naluType);
    bool IsVCL(uint8_t nalType);
    bool IsEOS() override;

    std::unique_ptr<uint8_t []> prereadBuffer_ = nullptr;
    uint32_t prereadBufferSize_ = 0;
    uint32_t pPrereadBuffer_ = 0;
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_DATA_PRODUCER_BITSTREAM_READER_H