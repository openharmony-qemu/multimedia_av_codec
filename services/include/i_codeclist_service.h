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

#ifndef I_CODECLIST_SERVICE_H
#define I_CODECLIST_SERVICE_H

#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
class ICodecListService {
public:
    virtual ~ICodecListService() = default;
    virtual std::string FindDecoder(const Media::Format &format) = 0;
    virtual std::string FindEncoder(const Media::Format &format) = 0;
    virtual int32_t GetCapability(CapabilityData &capabilityData, const std::string &mime, const bool isEncoder,
                                  const AVCodecCategory &category) = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // I_CODECLIST_SERVICE_H