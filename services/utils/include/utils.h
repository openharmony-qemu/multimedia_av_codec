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

#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>

namespace OHOS {
namespace MediaAVCodec {
inline void SleepFor(unsigned ms)
{
    constexpr int factor = 1000;
    usleep(ms * factor);
}

template <typename T>
using MakeUnsigned = typename std::make_unsigned<T>::type;

template <typename T, typename U>
constexpr T AlignUp(T num, U alignment)
{
    return (alignment > 0) ? (static_cast<uint64_t>((num + static_cast<MakeUnsigned<T>>(alignment) - 1)) &
                              static_cast<uint64_t>((~(static_cast<MakeUnsigned<T>>(alignment) - 1))))
                           : num;
}

template <typename T, typename U>
constexpr T AlignDown(T num, U alignment)
{
    return (alignment > 0) ? (static_cast<uint64_t>(num) &
                              static_cast<uint64_t>((~(static_cast<MakeUnsigned<T>>(alignment) - 1))))
                           : num;
}

template<typename T, typename U>
inline std::shared_ptr<T> ReinterpretPointerCast(const std::shared_ptr<U>& ptr) noexcept
{
    return std::shared_ptr<T>(ptr, reinterpret_cast<T*>(ptr.get()));
}
} // namespace MediaAVCodec
} // namespace OHOS
#endif