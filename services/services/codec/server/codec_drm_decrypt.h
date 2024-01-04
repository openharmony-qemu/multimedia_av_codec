/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef CODEC_DRM_DECRYPT_H
#define CODEC_DRM_DECRYPT_H

#include "buffer/avbuffer.h"
#include "meta/meta.h"

namespace OHOS {
namespace MediaAVCodec {

using namespace Media;
using MetaDrmSubSample = Plugins::MetaDrmSubSample;
using MetaDrmCencInfo = Plugins::MetaDrmCencInfo;
using MetaDrmCencAlgorithm = Plugins::MetaDrmCencAlgorithm;

class CodecDrmDecrypt {
public:
    void DrmCencDecrypt(uint32_t svp, std::shared_ptr<AVBuffer> inBuf, std::shared_ptr<AVBuffer> outBuf,
        uint32_t &dataSize);
    void SetCodecName(std::string codecName);
private:
    void GetCodingType();
    void DrmGetSkipClearBytes(uint32_t &skipBytes);
    int32_t DrmGetNalTypeAndIndex(uint8_t *data, uint32_t dataSize, uint8_t &nalType, uint32_t &posIndex);
    void DrmGetSyncHeaderIndex(uint8_t *data, uint32_t dataSize, uint32_t &posIndex);
    uint8_t DrmGetFinalNalTypeAndIndex(uint8_t *data, uint32_t dataSize, uint32_t &posStartIndex,
        uint32_t &posEndIndex);
    void DrmRemoveAmbiguityBytes(uint8_t *data, uint32_t &posEndIndex, uint32_t offset, uint32_t &dataSize);
    void DrmModifyCencInfo(uint8_t *data, uint32_t &dataSize, MetaDrmCencInfo *cencInfo);
private:
    std::string codecName_;
    int32_t codingType_;
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_DRM_DECRYPT_H