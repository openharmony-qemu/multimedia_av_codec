/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
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

#include "codec_capability_helper.h"

#include "avcodec_log.h"
#include "fcodec_loader.h"
#include "hevc_decoder_loader.h"
#include <optional>
#ifdef SUPPORT_CODEC_VP8
#include "vp8_decoder_loader.h"
#endif
#ifdef SUPPORT_CODEC_VP9
#include "vp9_decoder_loader.h"
#endif
#ifdef SUPPORT_CODEC_AV1
#include "av1_decoder_loader.h"
#endif
#include "avc_encoder_loader.h"
#include "avcodec_errors.h"
#include "codeclist_builder.h"

namespace OHOS {
namespace MediaAVCodec {
std::optional<CapabilityData> CodecCapabilityHelper::FindCapsByMime(const std::vector<CapabilityData> &caps,
    const std::string &mimeType)
{
    for (const auto &cap : caps) {
        const std::string &longer = (cap.mimeType.size() >= mimeType.size()) ? cap.mimeType : mimeType;
        const std::string &shorter = (cap.mimeType.size() >= mimeType.size()) ? mimeType : cap.mimeType;
        if (longer.find(shorter) != std::string::npos) {
            return cap;
        }
    }
    return std::nullopt;
}

std::optional<CapabilityData> CodecCapabilityHelper::GetVpxAndAV1CapabilityByMime(const std::string &mimeType)
{
    std::optional<CapabilityData> result;
    std::vector<CapabilityData> caps;
#ifdef SUPPORT_CODEC_VP8
    caps.clear();
    ret = Vp8DecoderLoader::GetCapabilityList(caps);
    if (ret == AVCS_ERR_OK) {
        result = FindCapsByMime(caps, mimeType);
    }
    if (result != std::nullopt) {
        return result;
    }
#endif
#ifdef SUPPORT_CODEC_VP9
    caps.clear();
    ret = Vp9DecoderLoader::GetCapabilityList(caps);
    if (ret == AVCS_ERR_OK) {
        result = FindCapsByMime(caps, mimeType);
    }
    if (result != std::nullopt) {
        return result;
    }
#endif
#ifdef SUPPORT_CODEC_AV1
    caps.clear();
    ret = Av1DecoderLoader::GetCapabilityList(caps);
    if (ret == AVCS_ERR_OK) {
        result = FindCapsByMime(caps, mimeType);
    }
    if (result != std::nullopt) {
        return result;
    }
#endif
    return std::nullopt;
}

std::optional<CapabilityData> CodecCapabilityHelper::GetCapabilityByMime(const std::string &mimeType)
{
    std::optional<CapabilityData> result;
    std::vector<CapabilityData> caps;
    int32_t ret = FCodecLoader::GetCapabilityList(caps);
    if (ret == AVCS_ERR_OK) {
        result = FindCapsByMime(caps, mimeType);
    }
    if (result != std::nullopt) {
        return result;
    }
    caps.clear();
    ret = HevcDecoderLoader::GetCapabilityList(caps);
    if (ret == AVCS_ERR_OK) {
        result = FindCapsByMime(caps, mimeType);
    }
    if (result != std::nullopt) {
        return result;
    }
    result = GetVpxAndAV1CapabilityByMime(mimeType);
    if (result != std::nullopt) {
        return result;
    }
    caps.clear();
    ret = AvcEncoderLoader::GetCapabilityList(caps);
    if (ret == AVCS_ERR_OK) {
        result = FindCapsByMime(caps, mimeType);
    }
    return result;
}
} // namespace MediaAVCodec
} // namespace OHOS

