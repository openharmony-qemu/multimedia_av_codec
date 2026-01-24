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

#include "vpx_decoder_api.h"
#include "vpxDecoder.h"

namespace OHOS::MediaAVCodec::Codec {
extern "C" {
int32_t GetVpxDecoderCapabilityList(std::vector<CapabilityData> &caps)
{
    return VpxDecoder::GetCodecCapability(caps);
}

void CreateVpxDecoderByName(const std::string &name, std::shared_ptr<CodecBase> &codec)
{
    sptr<VpxDecoder> vpxDecoder = new (std::nothrow) VpxDecoder(name);
    if (vpxDecoder == nullptr || !vpxDecoder->IsValid()) {
        codec = nullptr;
        return;
    }
    vpxDecoder->IncStrongRef(vpxDecoder.GetRefPtr());
    codec = std::shared_ptr<VpxDecoder>(vpxDecoder.GetRefPtr(), [](VpxDecoder *ptr) { (void)ptr; });
}
}
} // namespace OHOS::MediaAVCodec::Codec