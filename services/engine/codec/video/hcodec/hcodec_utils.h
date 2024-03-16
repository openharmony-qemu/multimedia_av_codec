/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HCODEC_UTILS_H
#define HCODEC_UTILS_H

#include <vector>
#include <algorithm>

namespace OHOS::MediaAVCodec {
inline constexpr int TIME_RATIO_S_TO_MS = 1000;
inline constexpr double US_TO_MS = 1000.0;
inline constexpr double US_TO_S = 1000000.0;

inline uint32_t GetYuv420Size(uint32_t w, uint32_t h)
{
    return w * h * 3 / 2;  // 3: nom of ratio, 2: denom of ratio
}

template <typename T>
void AppendToVector(std::vector<uint8_t>& vec, const T& param)
{
    size_t beforeSize = vec.size();
    size_t afterSize = beforeSize + sizeof(T);
    vec.resize(afterSize);

    const uint8_t* p = reinterpret_cast<const uint8_t*>(&param);
    std::copy(p, p + sizeof(T), vec.begin() + beforeSize);
}
}
#endif // HCODEC_UTILS_H
