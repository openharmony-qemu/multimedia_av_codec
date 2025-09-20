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

#ifndef AVCODEC_LOG_EX_H
#define AVCODEC_LOG_EX_H

#include "avcodec_log.h"
#include "avcodec_dfx_component.h"

#define AVCODEC_LOG_WITH_TAG(level, fmt, args...)                                                                      \
    do {                                                                                                               \
        (void)HILOG_IMPL(LABEL.type, level, LABEL.domain, LABEL.tag,                                                   \
                         "%{public}s{%{public}s()" CODE_LINE "} " fmt, tag_.load(), __FUNCTION__, ##args);             \
    } while (0)

#define AVCODEC_LOGF_WITH_TAG(fmt, ...) AVCODEC_LOG_WITH_TAG(LOG_FATAL, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGE_WITH_TAG(fmt, ...) AVCODEC_LOG_WITH_TAG(LOG_ERROR, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGW_WITH_TAG(fmt, ...) AVCODEC_LOG_WITH_TAG(LOG_WARN, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGI_WITH_TAG(fmt, ...) AVCODEC_LOG_WITH_TAG(LOG_INFO, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGD_WITH_TAG(fmt, ...) AVCODEC_LOG_WITH_TAG(LOG_DEBUG, fmt, ##__VA_ARGS__)

#define AVCODEC_LOGE_LIMIT_IN_TIME_WITH_TAG(intervalMs, maxCount, fmt, ...)                                            \
    AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGE_WITH_TAG, intervalMs, maxCount, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGW_LIMIT_IN_TIME_WITH_TAG(intervalMs, maxCount, fmt, ...)                                            \
    AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGW_WITH_TAG, intervalMs, maxCount, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGI_LIMIT_IN_TIME_WITH_TAG(intervalMs, maxCount, fmt, ...)                                            \
    AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGI_WITH_TAG, intervalMs, maxCount, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGD_LIMIT_IN_TIME_WITH_TAG(intervalMs, maxCount, fmt, ...)                                            \
    AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGD_WITH_TAG, intervalMs, maxCount, fmt, ##__VA_ARGS__)

#define CHECK_AND_RETURN_RET_LOG_WITH_TAG(cond, ret, fmt, ...)                                                         \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            AVCODEC_LOGE_WITH_TAG(fmt, ##__VA_ARGS__);                                                                 \
            return ret;                                                                                                \
        }                                                                                                              \
    } while (0)

#define CHECK_AND_RETURN_RET_LOGW_WITH_TAG(cond, ret, fmt, ...)                                                        \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            AVCODEC_LOGW_WITH_TAG(fmt, ##__VA_ARGS__);                                                                 \
            return ret;                                                                                                \
        }                                                                                                              \
    } while (0)

#define CHECK_AND_RETURN_RET_LOGD_WITH_TAG(cond, ret, fmt, ...)                                                        \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            AVCODEC_LOGD_WITH_TAG(fmt, ##__VA_ARGS__);                                                                 \
            return ret;                                                                                                \
        }                                                                                                              \
    } while (0)

#define EXPECT_AND_LOGW_WITH_TAG(cond, fmt, ...)                                                                       \
    do {                                                                                                               \
        if ((cond)) {                                                                                                  \
            AVCODEC_LOGW_WITH_TAG(fmt, ##__VA_ARGS__);                                                                 \
        }                                                                                                              \
    } while (0)

#define EXPECT_AND_LOGI_WITH_TAG(cond, fmt, ...)                                                                       \
    do {                                                                                                               \
        if ((cond)) {                                                                                                  \
            AVCODEC_LOGI_WITH_TAG(fmt, ##__VA_ARGS__);                                                                 \
        }                                                                                                              \
    } while (0)

#define EXPECT_AND_LOGD_WITH_TAG(cond, fmt, ...)                                                                       \
    do {                                                                                                               \
        if ((cond)) {                                                                                                  \
            AVCODEC_LOGD_WITH_TAG(fmt, ##__VA_ARGS__);                                                                 \
        }                                                                                                              \
    } while (0)

#define EXPECT_AND_LOGE_WITH_TAG(cond, fmt, ...)                                                                       \
    do {                                                                                                               \
        if ((cond)) {                                                                                                  \
            AVCODEC_LOGE_WITH_TAG(fmt, ##__VA_ARGS__);                                                                 \
        }                                                                                                              \
    } while (0)

#define CHECK_AND_RETURN_LOG_WITH_TAG(cond, fmt, ...)                                                                  \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            AVCODEC_LOGE_WITH_TAG(fmt, ##__VA_ARGS__);                                                                 \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define CHECK_AND_BREAK_LOG_WITH_TAG(cond, fmt, ...)                                                                   \
    if (1) {                                                                                                           \
        if (!(cond)) {                                                                                                 \
            AVCODEC_LOGW_WITH_TAG(fmt, ##__VA_ARGS__);                                                                 \
            break;                                                                                                     \
        }                                                                                                              \
    } else                                                                                                             \
        void(0)

#define CHECK_AND_CONTINUE_LOG_WITH_TAG(cond, fmt, ...)                                                                \
    if (1) {                                                                                                           \
        if (!(cond)) {                                                                                                 \
            AVCODEC_LOGW_WITH_TAG(fmt, ##__VA_ARGS__);                                                                 \
            continue;                                                                                                  \
        }                                                                                                              \
    } else                                                                                                             \
        void(0)

#define CHECK_AND_RETURN_RET_LOG_LIMIT_IN_TIME_WITH_TAG(cond, ret, intervalMs, maxCount, fmt, ...)                     \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            AVCODEC_LOGE_LIMIT_IN_TIME_WITH_TAG(intervalMs, maxCount, fmt, ##__VA_ARGS__);                             \
            return ret;                                                                                                \
        }                                                                                                              \
    } while (0)

#define CHECK_AND_RETURN_RET_LOGW_LIMIT_IN_TIME_WITH_TAG(cond, ret, intervalMs, maxCount, fmt, ...)                    \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            AVCODEC_LOGW_LIMIT_IN_TIME_WITH_TAG(intervalMs, maxCount, fmt, ##__VA_ARGS__);                             \
            return ret;                                                                                                \
        }                                                                                                              \
    } while (0)
#endif // AVCODEC_LOG_EX_H