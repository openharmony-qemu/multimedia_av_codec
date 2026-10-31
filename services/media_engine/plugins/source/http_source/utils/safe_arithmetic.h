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

#ifndef HISTREAMER_SAFE_ARITHMETIC_H
#define HISTREAMER_SAFE_ARITHMETIC_H

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

// SafeAdd: returns true on success, false on overflow.
// result = a + b; if overflow occurs, result is unchanged and returns false.
template <typename T>
inline typename std::enable_if<std::is_integral<T>::value, bool>::type SafeAdd(T a, T b, T& result)
{
    return !__builtin_add_overflow(a, b, &result);
}

// SafeSub: returns true on success, false on overflow.
// result = a - b; if overflow occurs, result is unchanged and returns false.
template <typename T>
inline typename std::enable_if<std::is_integral<T>::value, bool>::type SafeSub(T a, T b, T& result)
{
    return !__builtin_sub_overflow(a, b, &result);
}

// SafeMul: returns true on success, false on overflow.
// result = a * b; if overflow occurs, result is unchanged and returns false.
template <typename T>
inline typename std::enable_if<std::is_integral<T>::value, bool>::type SafeMul(T a, T b, T& result)
{
    return !__builtin_mul_overflow(a, b, &result);
}

// SafeIntCast: safe integer type conversion.
// Returns true if src fits in Dst, false otherwise.
// On success, dst = static_cast<Dst>(src); on failure dst is unchanged.
// Note: must dispatch by sign-combination to avoid implicit promotions in the
// boundary comparison (e.g. uint64_t src vs int64_t::min() negative would
// promote the negative to a huge unsigned value and falsely report overflow).
namespace {
template <typename Src, typename Dst>
inline bool SafeIntCastSameSign(Src src, Dst& dst)
{
    if (src < std::numeric_limits<Dst>::min() || src > std::numeric_limits<Dst>::max()) {
        return false;
    }
    dst = static_cast<Dst>(src);
    return true;
}

template <typename Src, typename Dst>
inline bool SafeIntCastSignedToUnsigned(Src src, Dst& dst)
{
    if (src < 0 || static_cast<std::make_unsigned_t<Src>>(src) > std::numeric_limits<Dst>::max()) {
        return false;
    }
    dst = static_cast<Dst>(src);
    return true;
}

template <typename Src, typename Dst>
inline bool SafeIntCastUnsignedToSigned(Src src, Dst& dst)
{
    if (src > static_cast<std::make_unsigned_t<Dst>>(std::numeric_limits<Dst>::max())) {
        return false;
    }
    dst = static_cast<Dst>(src);
    return true;
}
} // namespace

template <typename Src, typename Dst>
inline bool SafeIntCast(Src src, Dst& dst)
{
    if constexpr (std::is_signed_v<Src> == std::is_signed_v<Dst>) {
        return SafeIntCastSameSign<Src, Dst>(src, dst);
    } else if constexpr (std::is_signed_v<Src>) {
        return SafeIntCastSignedToUnsigned<Src, Dst>(src, dst);
    } else {
        return SafeIntCastUnsignedToSigned<Src, Dst>(src, dst);
    }
}

// SafeDoubleToUint64: converts a double to uint64_t safely.
// Returns true if value is finite and within [0, UINT64_MAX], false otherwise.
// On success, dst = static_cast<uint64_t>(value); on failure dst is unchanged.
inline bool SafeDoubleToUint64(double value, uint64_t& dst)
{
    if (!std::isfinite(value) || value < 0.0 || value > static_cast<double>(std::numeric_limits<uint64_t>::max())) {
        return false;
    }
    dst = static_cast<uint64_t>(value);
    return true;
}

// SafeDoubleToInt64: converts a double to int64_t safely.
// Returns true if value is finite and within [INT64_MIN, INT64_MAX], false otherwise.
// On success, dst = static_cast<int64_t>(value); on failure dst is unchanged.
inline bool SafeDoubleToInt64(double value, int64_t& dst)
{
    if (!std::isfinite(value) || value < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
        value > static_cast<double>(std::numeric_limits<int64_t>::max())) {
        return false;
    }
    dst = static_cast<int64_t>(value);
    return true;
}

} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_SAFE_ARITHMETIC_H
