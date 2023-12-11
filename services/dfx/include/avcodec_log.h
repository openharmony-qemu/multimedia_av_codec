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
#ifndef AVCODEC_LOG_H
#define AVCODEC_LOG_H

#include <hilog/log.h>
#include <cinttypes>

namespace OHOS {
namespace MediaAVCodec {
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002BAC

#define AVCODEC_LOG_FREQ_LIMIT(frequency)                    \
    if (1) {                                                 \
        thread_local uint64_t currentTimes = 0;              \
        if (currentTimes++ % ((uint64_t)(frequency)) != 0) { \
            break;                                           \
        }                                                    \
    }

#define AVCODEC_LOG(func, fmt, args...)                  \
    do {                                                 \
        (void)func(LABEL, "{%{public}s():%{public}d} " fmt, __FUNCTION__, __LINE__, ##args);   \
    } while (0)

#define AVCODEC_LOGF(fmt, ...) AVCODEC_LOG(::OHOS::HiviewDFX::HiLog::Fatal, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGE(fmt, ...) AVCODEC_LOG(::OHOS::HiviewDFX::HiLog::Error, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGW(fmt, ...) AVCODEC_LOG(::OHOS::HiviewDFX::HiLog::Warn,  fmt, ##__VA_ARGS__)
#define AVCODEC_LOGI(fmt, ...) AVCODEC_LOG(::OHOS::HiviewDFX::HiLog::Info,  fmt, ##__VA_ARGS__)
#define AVCODEC_LOGD(fmt, ...) AVCODEC_LOG(::OHOS::HiviewDFX::HiLog::Debug, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGD_LIMIT(frequency, fmt, ...)         \
    do {                                                \
        AVCODEC_LOG_FREQ_LIMIT(frequency);              \
        AVCODEC_LOGD(fmt, ##__VA_ARGS__);               \
    } while (0)

#define CHECK_AND_RETURN_RET_LOG(cond, ret, fmt, ...)   \
    do {                                                \
        if (!(cond)) {                                  \
            AVCODEC_LOGE(fmt, ##__VA_ARGS__);           \
            return ret;                                 \
        }                                               \
    } while (0)
       
#define CHECK_AND_RETURN_LOG(cond, fmt, ...)            \
    do {                                                \
        if (!(cond)) {                                  \
            AVCODEC_LOGE(fmt, ##__VA_ARGS__);           \
            return;                                     \
        }                                               \
    } while (0)

#define CHECK_AND_BREAK_LOG(cond, fmt, ...)             \
    if (1) {                                            \
        if (!(cond)) {                                  \
            AVCODEC_LOGW(fmt, ##__VA_ARGS__);           \
            break;                                      \
        }                                               \
    } else void (0)

#define CHECK_AND_CONTINUE_LOG(cond, fmt, ...)          \
    if (1) {                                            \
        if (!(cond)) {                                  \
            AVCODEC_LOGW(fmt, ##__VA_ARGS__);           \
            continue;                                   \
        }                                               \
    } else void (0)

#define POINTER_MASK 0x00FFFFFF
#define FAKE_POINTER(addr) (POINTER_MASK & reinterpret_cast<uintptr_t>(addr))
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_LOG_H