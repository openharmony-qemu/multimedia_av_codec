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

#ifndef DOUBLE_UTILS_H
#define DOUBLE_UTILS_H

#include <cmath>

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
namespace DoubleUtils {

constexpr double DEFAULT_EPSILON = 1e-6;

inline bool IsEqual(double a, double b, double epsilon = DEFAULT_EPSILON)
{
    return std::abs(a - b) <= epsilon;
}

inline bool IsNotEqual(double a, double b, double epsilon = DEFAULT_EPSILON)
{
    return std::abs(a - b) > epsilon;
}

inline bool IsNotEqual(const std::optional<double> &a, const std::optional<double> &b, double epsilon = DEFAULT_EPSILON)
{
    if (a.has_value() != b.has_value()) {
        return true;
    }
    if (!a.has_value()) {
        return false;
    }
    return IsNotEqual(a.value(), b.value(), epsilon);
}

inline bool IsGreaterOrEqual(double a, double b, double epsilon = DEFAULT_EPSILON)
{
    return a >= (b - epsilon);
}

inline bool IsLessOrEqual(double a, double b, double epsilon = DEFAULT_EPSILON)
{
    return a <= (b + epsilon);
}

inline bool IsGreater(double a, double b, double epsilon = DEFAULT_EPSILON)
{
    return a > (b + epsilon);
}

inline bool IsLess(double a, double b, double epsilon = DEFAULT_EPSILON)
{
    return a < (b - epsilon);
}
} // namespace DoubleUtils
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // DOUBLE_UTILS_H