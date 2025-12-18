/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef MPEG4_UTILS_H
#define MPEG4_UTILS_H

#include <cstdint>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
enum class RoundingType {
    NEAR_INF = 0, // Round to nearest and halfway cases away from zero.
    UP = 1, // Round toward +infinity.
    DOWN = 2, // Round toward -infinity.
};

/**
 * Convert time from mpeg4 to time in TIME_SCALE_US.
 * @param time mpeg4 time
 * @param timeScale mpeg4 time scale
 * @param type rounding type
 * @return time in TIME_SCALE_US
 */
int64_t ConvertTimeFromMpeg4(int64_t time, int32_t timeScale, RoundingType type = RoundingType::NEAR_INF);

/**
 * Convert time in TIME_SCALE_US to mpeg4 time.
 * @param time time in TIME_SCALE_US
 * @param timeScale mpeg4 time scale
 * @param type rounding type
 * @return time in mpeg4 time scale
 */
int64_t ConvertTimeToMpeg4(int64_t time, int32_t timeScale, RoundingType type = RoundingType::NEAR_INF);

/**
 * Convert time in timeScaleA to time in timeScaleB.
 * @param time time in timeScaleA
 * @param timeScaleA time scale a
 * @param timeScaleB time scale b
 * @param type rounding type
 * @return time in timeScaleB
 */
int64_t ConvertTime(int64_t time, int32_t timeScaleA, int32_t timeScaleB, RoundingType type = RoundingType::NEAR_INF);
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif