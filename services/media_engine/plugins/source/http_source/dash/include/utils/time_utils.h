/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#ifndef DASH_TIME_UTILS_H
#define DASH_TIME_UTILS_H

#include <cstdint>
#include <ctime>
#include <cstring>
#include <iostream>
#include <errno.h>
#include "securec.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

constexpr unsigned int DEFAULT_YEAR = 1900;
constexpr unsigned int BASE_YEAR = 70;
constexpr unsigned int BASE_DAY = 1;

constexpr int64_t D_2_H = 24;
constexpr int64_t H_2_M = 60;
constexpr int64_t M_2_S = 60;

constexpr int64_t S_2_MS = 1000;
constexpr int64_t MS_2_US = 1000;
constexpr int64_t US_2_NS = 1000;

constexpr int64_t H_2_S = H_2_M * M_2_S;
constexpr int64_t D_2_S = D_2_H * H_2_S;

constexpr int64_t M_2_MS = M_2_S * S_2_MS;
constexpr int64_t H_2_MS = H_2_M * M_2_MS;
constexpr int64_t D_2_MS = D_2_H * H_2_MS;

constexpr int64_t S_2_US = S_2_MS * MS_2_US;
constexpr int64_t MS_2_NS = MS_2_US * US_2_NS;
constexpr int64_t S_2_NS = S_2_MS * MS_2_NS;

constexpr int64_t MEDIA_PTS_UNSET = INT64_MIN + 1;

inline time_t String2Time(const std::string szTime)
{
    if (szTime.length() == 0) {
        return 0;
    }

    struct tm tm1;
    errno_t result = memset_s(&tm1, sizeof(tm1), 0, sizeof(tm1));
    if (result != 0) {
        return 0;
    }
    struct tm tmBase;
    result = memset_s(&tmBase, sizeof(tmBase), 0, sizeof(tmBase));
    if (result != 0) {
        return 0;
    }

    if (sscanf_s(szTime.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday, &tm1.tm_hour,
        &tm1.tm_min, &tm1.tm_sec) <= 0) {
        return 0;
    }

    tm1.tm_year -= DEFAULT_YEAR;
    tm1.tm_mon--;
    tm1.tm_isdst = -1;

    tmBase.tm_mday = BASE_DAY;
    tmBase.tm_year = BASE_YEAR;
    tmBase.tm_isdst = -1;
    return mktime(&tm1) - mktime(&tmBase);
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS

#endif // DASH_TIME_UTILS_H
