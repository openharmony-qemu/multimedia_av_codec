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
#include "avcodec_log_tag.h"
#include <algorithm>
#include "instance_info.h"
#include "meta/meta.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace OHOS::Media;
std::string CreateVideoLogTag(const Meta &callerInfo)
{
    int32_t instanceId = 0;
    std::string codecName = "";
    std::string type = "";
    (void)callerInfo.GetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId);
    if (instanceId == 0) {
        return "";
    }
    (void)callerInfo.GetData(Tag::MEDIA_CODEC_NAME, codecName);
    std::transform(codecName.begin(), codecName.end(), codecName.begin(), ::tolower);
    if (codecName.find("omx") != std::string::npos) {
        type = "hw.";
    } else {
        type = "sw.";
    }
    if (codecName.find("decode") != std::string::npos) {
        type += "vdec";
    } else if (codecName.find("encode") != std::string::npos) {
        type += "venc";
    } else {
        return "";
    }
    return std::string("[") + std::to_string(instanceId) + "][" + type + "]";
}

const char *AVCodecLogTag::GetLogTag()
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return tag_.c_str();
}

void AVCodecLogTag::SetLogTag(const std::string &tagStr)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    tag_ = tagStr;
}
} // namespace MediaAVCodec
} // namespace OHOS
