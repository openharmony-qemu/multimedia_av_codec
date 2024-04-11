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

#include "avcodec_log.h"
#ifndef CLIENT_SUPPORT_CODEC
#include "fcodec_loader.h"
#endif
#include "avcodec_errors.h"
#include "audio_codeclist_info.h"
#include "codeclist_builder.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecList_builder"};
}
namespace OHOS {
namespace MediaAVCodec {
#ifndef CLIENT_SUPPORT_CODEC
int32_t VideoCodecList::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    auto ret = FCodecLoader::GetCapabilityList(caps);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Get capability from fcodec successful");
    }
    return ret;
}
#endif
int32_t AudioCodecList::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    auto audioCapabilities = AudioCodeclistInfo::GetInstance().GetAudioCapabilities();
    for (const auto &v : audioCapabilities) {
        caps.emplace_back(v);
    }
    AVCODEC_LOGI("Get capability from audio codec list successful");
    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS