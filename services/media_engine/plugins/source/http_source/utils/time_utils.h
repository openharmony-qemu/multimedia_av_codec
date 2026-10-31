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

#ifndef HTTP_SOURCE_TIME_UTILS_H
#define HTTP_SOURCE_TIME_UTILS_H

#include <cstdint>
#include <ctime>
#include <cstring>
#include <iostream>
#include <limits>
#include <cerrno>
#include "securec.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

constexpr int DEFAULT_YEAR = 1900;
constexpr int BASE_YEAR = 70;
constexpr int BASE_DAY = 1;

constexpr uint64_t D_2_H = 24;
constexpr uint64_t H_2_M = 60;
constexpr uint64_t M_2_S = 60;

constexpr uint64_t S_2_MS = 1000;
constexpr uint64_t MS_2_US = 1000;
constexpr uint64_t US_2_NS = 1000;

constexpr uint64_t H_2_S = H_2_M * M_2_S;
constexpr uint64_t D_2_S = D_2_H * H_2_S;

constexpr uint64_t M_2_MS = M_2_S * S_2_MS;
constexpr uint64_t H_2_MS = H_2_M * M_2_MS;
constexpr uint64_t D_2_MS = D_2_H * H_2_MS;

constexpr uint64_t S_2_US = S_2_MS * MS_2_US;
constexpr uint64_t MS_2_NS = MS_2_US * US_2_NS;
constexpr uint64_t S_2_NS = S_2_MS * MS_2_NS;
constexpr uint64_t D_2_NS = D_2_S * S_2_NS;

// 分片时长下限，单位纳秒
constexpr uint64_t MIN_SEGMENT_DURATION_NS = 10 * MS_2_NS;     // 10 ms

class String2Time {
    std::string str_;
public:
    explicit String2Time(const std::string& s): str_(s) {}

    time_t operator()() const
    {
        if (str_.length() == 0) {
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

        if (sscanf_s(str_.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday, &tm1.tm_hour,
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
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS

#endif // HTTP_SOURCE_TIME_UTILS_H
