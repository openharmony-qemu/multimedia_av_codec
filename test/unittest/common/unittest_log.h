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

#ifndef UNITTEST_LOG_H
#define UNITTEST_LOG_H

#include <cinttypes>
#include <cstdio>
#include "securec.h"

#ifdef PRINT_HILOG
#include "avcodec_log.h"
#define UNITTEST_HILOG(fmt, ...) AVCODEC_LOGI(fmt, ##__VA_ARGS__)
#else
#define UNITTEST_HILOG(fmt, ...)
#endif

namespace OHOS {
#define LOG_MAX_SIZE 400

#ifdef TEST_ID
#define PRINT_TEST_LOG(ch)                                                                                             \
    do {                                                                                                               \
        UNITTEST_HILOG("[%{public}d] %{public}s", TEST_ID, ch);                                                        \
        (void)printf("[%s:%d][%d] %s", __func__, __LINE__, TEST_ID, ch);                                               \
    } while (0)
#else
#define PRINT_TEST_LOG(ch)                                                                                             \
    do {                                                                                                               \
        UNITTEST_HILOG("%{public}s", ch);                                                                              \
        (void)printf("[%s:%d] %s", __func__, __LINE__, ch);                                                            \
    } while (0)
#endif

#define UNITTEST_CHECK_AND_RETURN_RET_LOG(cond, ret, fmt, ...)                                                         \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            char ch[LOG_MAX_SIZE];                                                                                     \
            (void)sprintf_s(ch, LOG_MAX_SIZE, fmt, ##__VA_ARGS__);                                                     \
            PRINT_TEST_LOG(ch);                                                                                        \
            (void)printf("\n");                                                                                        \
            return ret;                                                                                                \
        }                                                                                                              \
    } while (0)

#define UNITTEST_CHECK_AND_RETURN_LOG(cond, fmt, ...)                                                                  \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            char ch[LOG_MAX_SIZE];                                                                                     \
            (void)sprintf_s(ch, LOG_MAX_SIZE, fmt, ##__VA_ARGS__);                                                     \
            PRINT_TEST_LOG(ch);                                                                                        \
            (void)printf("\n");                                                                                        \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define UNITTEST_CHECK_AND_INFO_LOG(cond, fmt, ...)                                                                    \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            char ch[LOG_MAX_SIZE];                                                                                     \
            (void)sprintf_s(ch, LOG_MAX_SIZE, fmt, ##__VA_ARGS__);                                                     \
            PRINT_TEST_LOG(ch);                                                                                        \
            (void)printf("\n");                                                                                        \
        }                                                                                                              \
    } while (0)

#define UNITTEST_CHECK_AND_BREAK_LOG(cond, fmt, ...)                                                                   \
    if (!(cond)) {                                                                                                     \
        char ch[LOG_MAX_SIZE];                                                                                         \
        (void)sprintf_s(ch, LOG_MAX_SIZE, fmt, ##__VA_ARGS__);                                                         \
        PRINT_TEST_LOG(ch);                                                                                            \
        (void)printf("\n");                                                                                            \
        break;                                                                                                         \
    }

#define UNITTEST_CHECK_AND_CONTINUE_LOG(cond, fmt, ...)                                                                \
    if (!(cond)) {                                                                                                     \
        char ch[LOG_MAX_SIZE];                                                                                         \
        (void)sprintf_s(ch, LOG_MAX_SIZE, fmt, ##__VA_ARGS__);                                                         \
        PRINT_TEST_LOG(ch);                                                                                            \
        (void)printf("\n");                                                                                            \
        continue;                                                                                                      \
    }

#define UNITTEST_INFO_LOG(fmt, ...)                                                                                    \
    do {                                                                                                               \
        char ch[LOG_MAX_SIZE];                                                                                         \
        (void)sprintf_s(ch, LOG_MAX_SIZE, fmt, ##__VA_ARGS__);                                                         \
        PRINT_TEST_LOG(ch);                                                                                            \
        (void)printf("\n");                                                                                            \
    } while (0)
} // namespace OHOS

#endif // UNITTEST_LOG_H