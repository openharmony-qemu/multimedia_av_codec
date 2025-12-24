/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "avcodec_dfx_component.h"
#include <algorithm>
#include "instance_info.h"
#include "event_info_extented_key.h"
#include "meta/meta.h"

using namespace OHOS::Media;
namespace OHOS {
namespace MediaAVCodec {
std::string CreateVideoLogTag(const Meta &callerInfo)
{
    int32_t instanceId = 0;
    std::string codecName = "";
    std::string type = "";
    bool ret = callerInfo.GetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId) &&
               callerInfo.GetData(Tag::MEDIA_CODEC_NAME, codecName);
    if (!ret || instanceId == INVALID_INSTANCE_ID) {
        return "";
    }
    std::transform(codecName.begin(), codecName.end(), codecName.begin(), ::tolower);
    type += codecName.find("omx") != std::string::npos ? "h." : "s.";
    if (codecName.find("decode") != std::string::npos) {
        type += "vdec";
    } else if (codecName.find("encode") != std::string::npos) {
        type += "venc";
    } else {
        return "";
    }
    return std::string("[") + std::to_string(instanceId) + "][" + type + "]";
}

AVCodecDfxComponent::AVCodecDfxComponent()
{
    tag_.store("");
}

AVCodecDfxComponent::~AVCodecDfxComponent() {}

void AVCodecDfxComponent::SetTag(const std::string &str)
{
    if (str == "") {
        return;
    }
    tagContent_ = str;
    tag_.store(tagContent_.c_str());
}

const std::string &AVCodecDfxComponent::GetTag()
{
    return tagContent_;
}
} // namespace MediaAVCodec
} // namespace OHOS
