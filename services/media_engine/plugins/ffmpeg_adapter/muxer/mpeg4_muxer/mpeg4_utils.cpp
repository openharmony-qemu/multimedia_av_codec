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

#include "mpeg4_utils.h"
#include "common/log.h"

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "Mpeg4Utils" };
}
#endif // !_WIN32

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
/* time scale microsecond */
constexpr int32_t TIME_SCALE_US = 1000000;


int64_t ConvertTimeFromMpeg4(int64_t time, int32_t timeScale,
    RoundingType type)
{
    return ConvertTime(time, timeScale, TIME_SCALE_US, type);
}

int64_t ConvertTimeToMpeg4(int64_t time, int32_t timeScale,
    RoundingType type)
{
    return ConvertTime(time, TIME_SCALE_US, timeScale, type);
}

int64_t ConvertTime(int64_t time, int32_t timeScaleA, int32_t timeScaleB,
    RoundingType type)
{
    FALSE_RETURN_V_MSG_E(timeScaleA > 0, INT64_MIN, "time scale a %{public}d must be > 0", timeScaleA);
    FALSE_RETURN_V_MSG_E(timeScaleB > 0, INT64_MIN, "time scale b %{public}d must be > 0", timeScaleB);

    if (time < 0 && type == RoundingType::UP) {
        return -ConvertTime(-time, timeScaleA, timeScaleB, RoundingType::DOWN);
    } else if (time < 0 && type == RoundingType::DOWN) {
        return -ConvertTime(-time, timeScaleA, timeScaleB, RoundingType::UP);
    } else if (time < 0) {
        return -ConvertTime(-time, timeScaleA, timeScaleB, type);
    }

    int64_t timeC = 0;
    if (type == RoundingType::NEAR_INF) {
        timeC = timeScaleA / 2;  // 2
    } else if (type == RoundingType::UP) {
        timeC = timeScaleA - 1;
    }

    int64_t timeD = time / timeScaleA;
    int64_t timeE = (time % timeScaleA * timeScaleB + timeC) / timeScaleA;
    FALSE_RETURN_V_MSG_E(timeD < INT32_MAX || timeD <= (INT64_MAX - timeE) / timeScaleB, INT64_MIN,
        "time after conversion must be <= INT64_MAX, time " PUBLIC_LOG_D64 " * " PUBLIC_LOG_D32 " / " PUBLIC_LOG_D32,
        time, timeScaleB, timeScaleA);
    return timeD * timeScaleB + timeE;
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS