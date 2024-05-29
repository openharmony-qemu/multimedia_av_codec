/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#ifndef BITRATE_UTILS_H
#define BITRATE_UTILS_H

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

constexpr double BUFFER_LIMIT_FACT = 0.8;

uint32_t GetDesBitrate(std::vector<uint32_t> bitRates, uint64_t downloadSpeed)
{
    uint32_t desBitrate = bitRates[0];
    for (size_t i = 0; i<bitRates.size(); ++i) {
        if (bitRates[i] < downloadSpeed * BUFFER_LIMIT_FACT) {
            desBitrate = bitRates[i];
        } else {
            break;
        }
    }
    return desBitrate;
}
}
}
}
}

#endif