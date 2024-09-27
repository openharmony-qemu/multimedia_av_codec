/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "hevc_decoder_api.h"
#include "hevc_decoder.h"

namespace OHOS::MediaAVCodec::Codec {
extern "C" {
int32_t GetHevcDecoderCapabilityList(std::vector<CapabilityData> &caps)
{
    return HevcDecoder::GetCodecCapability(caps);
}

void CreateHevcDecoderByName(const std::string &name, std::shared_ptr<CodecBase> &codec)
{
    sptr<HevcDecoder> hevcDecoder = new (std::nothrow) HevcDecoder(name);
    hevcDecoder->IncStrongRef(hevcDecoder.GetRefPtr());
    codec = std::shared_ptr<HevcDecoder>(hevcDecoder.GetRefPtr(), [](HevcDecoder *ptr) { (void)ptr; });
}
}
} // namespace OHOS::MediaAVCodec::Codec