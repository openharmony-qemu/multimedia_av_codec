/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef CODEC_CAPABILITY_HELPER_H
#define CODEC_CAPABILITY_HELPER_H

#include <string>
#include <vector>

#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecCapabilityHelper {
public:
    CodecCapabilityHelper() = default;
    ~CodecCapabilityHelper() = default;
    static std::optional<CapabilityData> GetCapabilityByMime(const std::string &mimeType);
private:
    static std::optional<CapabilityData> FindCapsByMime(const std::vector<CapabilityData> &caps,
        const std::string &mimeType);
    static std::optional<CapabilityData> GetVpxAndAV1CapabilityByMime(const std::string &mimeType);
};
}
} // namespace OHOS
#endif // CODEC_CAPABILITY_HELPER_H
