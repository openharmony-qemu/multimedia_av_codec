/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "avc_encoder_api.h"
#include "avc_encoder.h"

namespace OHOS::MediaAVCodec::Codec {
extern "C" {
int32_t GetAvcEncoderCapabilityList(std::vector<CapabilityData> &caps)
{
    return AvcEncoder::GetCodecCapability(caps);
}

void CreateAvcEncoderByName(const std::string &name, std::shared_ptr<CodecBase> &codec)
{
    std::vector<CapabilityData> caps;
    int32_t ret = AvcEncoder::GetCodecCapability(caps);
    if (ret != AVCS_ERR_OK) {
        codec = nullptr;
    } else {
        codec = std::make_shared<AvcEncoder>(name);
    }
}
}
} // namespace OHOS::MediaAVCodec::Codec