/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 *
 * Description: header of Type converter from framework to OMX
 */

#include "av1_decoder_api.h"
#include "av1decoder.h"

namespace OHOS::MediaAVCodec::Codec {
extern "C" {
int32_t GetAv1DecoderCapabilityList(std::vector<CapabilityData> &caps)
{
    return Av1Decoder::GetCodecCapability(caps);
}

void CreateAv1DecoderByName(const std::string &name, std::shared_ptr<CodecBase> &codec)
{
    sptr<Av1Decoder> av1Decoder = new (std::nothrow) Av1Decoder(name);
    if (av1Decoder == nullptr || !av1Decoder->IsValid()) {
        codec = nullptr;
        return;
    }
    av1Decoder->IncStrongRef(av1Decoder.GetRefPtr());
    codec = std::shared_ptr<Av1Decoder>(av1Decoder.GetRefPtr(), [](Av1Decoder *ptr) { (void)ptr; });
}
}
} // namespace OHOS::MediaAVCodec::Codec