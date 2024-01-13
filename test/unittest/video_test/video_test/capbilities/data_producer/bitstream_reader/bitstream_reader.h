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

#include "data_producer_base.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class BitstreamReader : public DataProducerBase {
public:
    BitstreamReader(BitstreamType &type);
    int32_t ReadSample(CodecBufferInfo &bufferInfo) override;

private:
    BitstreamReader() {}
    int32_t ToAnnexb(uint8_t *bufferAddr);
    bool IsCodecData(const uint8_t *const bufferAddr);

    BitstreamType bitstreamType_;
};
} // Sample
} // MediaAVCodec
} // OHOS