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

#ifndef LOG_H
#define LOG_H

#include <array>
#include <utility>
#include <string_view>
#include <cinttypes>
#include <cstdio>
#include <hilog/log_c.h>

template<size_t N>
constexpr auto CountPercent(const char(&str)[N])
{
    uint32_t cntOfPercentSign = 0;
    std::array<bool, N> isPercentSign{};
    for (size_t i = 0; i < N; i++) {
        if (str[i] == '%') {
            cntOfPercentSign++;
            isPercentSign[i] = true;
        } else {
            isPercentSign[i] = false;
        }
    }
    for (size_t i = 1; i < N; i++) {
        if (isPercentSign[i - 1] && isPercentSign[i]) {
            cntOfPercentSign -= 2; // 2 is length of %%
            isPercentSign[i - 1] = false;
            isPercentSign[i] = false;
        }
    }
    return std::make_pair(cntOfPercentSign, isPercentSign);
}

template<uint32_t cntOfPercentSign, size_t N>
constexpr auto AddPublic(const char(&str)[N], const std::array<bool, N> &isPercentSign)
{
    constexpr std::string_view pub = "{public}";
    std::array<char, N + cntOfPercentSign * pub.size()> newStr{};
    for (size_t i = 0, j = 0; i < N; i++, j++) {
        newStr[j] = str[i];
        if (isPercentSign[i]) {
            for (size_t k = 0; k < pub.size(); k++) {
                newStr[j + 1 + k] = pub[k];
            }
            j += pub.size();
        }
    }
    return newStr;
}

#define RE_FORMAT(level, s, ...) do { \
    constexpr auto pair = CountPercent(s); \
    constexpr auto newStr = AddPublic<pair.first>(s, pair.second); \
    (void)HiLogPrint(LOG_CORE, level, 0xD002B32, "HCODEC", newStr.data(), ##__VA_ARGS__); \
} while (0)

inline constexpr const char* StrLevel(LogLevel level)
{
    switch (level) {
        case LOG_ERROR:
            return "E";
        case LOG_WARN:
            return "W";
        case LOG_INFO:
            return "I";
        case LOG_DEBUG:
            return "D";
        default:
            return "";
    }
}

#define PLOGI(...) RE_FORMAT(LOG_INFO, __VA_ARGS__)

#define LOG(level, s, ...) RE_FORMAT(level, "[%s %d] " s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(...) LOG(LOG_ERROR, __VA_ARGS__)
#define LOGW(...) LOG(LOG_WARN, __VA_ARGS__)
#define LOGI(...) LOG(LOG_INFO, __VA_ARGS__)
#define LOGD(...) LOG(LOG_DEBUG, __VA_ARGS__)

// for test
#define TLOG(level, s, ...) do { \
    RE_FORMAT(level, "[%s %d] " s, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    printf("%s: [%s %d] " s "\n", StrLevel(level), __FUNCTION__, __LINE__, ##__VA_ARGS__); \
} while (0)
#define TLOGE(...) TLOG(LOG_ERROR, __VA_ARGS__)
#define TLOGW(...) TLOG(LOG_WARN, __VA_ARGS__)
#define TLOGI(...) TLOG(LOG_INFO, __VA_ARGS__)
#define TLOGD(...) TLOG(LOG_DEBUG, __VA_ARGS__)

// for HCodec
#define HLOG(level, s, ...) RE_FORMAT(level, "%s[%s][%s %d] " s, \
    compUniqueStr_.c_str(), currState_->GetName().c_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define HLOGE(...) HLOG(LOG_ERROR, __VA_ARGS__)
#define HLOGW(...) HLOG(LOG_WARN, __VA_ARGS__)
#define HLOGI(...) HLOG(LOG_INFO, __VA_ARGS__)
#define HLOGD(...) HLOG(LOG_DEBUG, __VA_ARGS__)

// for HCodec inner state
#define SLOG(level, s, ...) RE_FORMAT(level, "%s[%s][%s %d] " s, \
    codec_->compUniqueStr_.c_str(), stateName_.c_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SLOGE(...) SLOG(LOG_ERROR, __VA_ARGS__)
#define SLOGW(...) SLOG(LOG_WARN, __VA_ARGS__)
#define SLOGI(...) SLOG(LOG_INFO, __VA_ARGS__)
#define SLOGD(...) SLOG(LOG_DEBUG, __VA_ARGS__)

#define IF_TRUE_RETURN_VAL(cond, val)  \
    do {                               \
        if (cond) {                    \
            return val;                \
        }                              \
    } while (0)
#define IF_TRUE_RETURN_VAL_WITH_MSG(cond, val, msg, ...) \
    do {                                        \
        if (cond) {                             \
            LOGE(msg, ##__VA_ARGS__);           \
            return val;                         \
        }                                       \
    } while (0)
#define IF_TRUE_RETURN_VOID(cond)  \
    do {                                \
        if (cond) {                     \
            return;                     \
        }                               \
    } while (0)
#define IF_TRUE_RETURN_VOID_WITH_MSG(cond, msg, ...)     \
    do {                                        \
        if (cond) {                             \
            LOGE(msg, ##__VA_ARGS__);           \
            return;                             \
        }                                       \
    } while (0)

#endif