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
 */

#include <sstream>
#include <string>
#include "hcodec_utils.h"
#include "qos.h"

namespace OHOS::MediaAVCodec {
std::string StringifyMeta(std::shared_ptr<Media::Meta> &meta)
{
    std::stringstream dumpStream;
    for (auto iter = meta->begin(); iter != meta->end(); ++iter) {
        switch (meta->GetValueType(iter->first)) {
            case OHOS::Media::AnyValueType::INT32_T:
                dumpStream << iter->first << " = " << std::to_string(Media::AnyCast<int32_t>(iter->second)) << " | ";
                break;
            case OHOS::Media::AnyValueType::UINT32_T:
                dumpStream << iter->first << " = " << std::to_string(Media::AnyCast<uint32_t>(iter->second)) << " | ";
                break;
            case OHOS::Media::AnyValueType::BOOL:
                dumpStream << iter->first << " = " << std::to_string(Media::AnyCast<bool>(iter->second)) << " | ";
                break;
            case OHOS::Media::AnyValueType::DOUBLE:
                dumpStream << iter->first << " = " << std::to_string(Media::AnyCast<double>(iter->second)) << " | ";
                break;
            case OHOS::Media::AnyValueType::INT64_T:
                dumpStream << iter->first << " = " << std::to_string(Media::AnyCast<int64_t>(iter->second)) << " | ";
                break;
            case OHOS::Media::AnyValueType::FLOAT:
                dumpStream << iter->first << " = " << std::to_string(Media::AnyCast<float>(iter->second)) << " | ";
                break;
            default:
                dumpStream << iter->first << " = " << "unknown type | ";
                break;
        }
    }
    return dumpStream.str();
}

void SetThreadInteractiveQos(bool enable)
{
    // thread_local bool inInteractiveQos_ = false;
    // if (enable && !inInteractiveQos_) {
    //     inInteractiveQos_ = true;
    //     OHOS::QOS::SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    // } else if (!enable && inInteractiveQos_) {
    //     inInteractiveQos_ = false;
    //     OHOS::QOS::ResetThreadQos();
    // }
}
}
