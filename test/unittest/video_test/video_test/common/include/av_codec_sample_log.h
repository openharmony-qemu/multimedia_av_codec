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

#ifndef AVCODEC_SAMPLE_LOG_H
#define AVCODEC_SAMPLE_LOG_H

#include "avcodec_log.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
extern const bool VERBOSE_LOG;

#ifdef AVCODEC_LOG_USE_NO_DICT_LOG
#undef AVCODEC_LOG
#undef AVCODEC_LOGF
#undef AVCODEC_LOGE
#undef AVCODEC_LOGW
#undef AVCODEC_LOGI
#undef AVCODEC_LOGD
#define AVCODEC_LOG(func, fmt, args...)                                                                                \
    do {                                                                                                               \
        (void)func(LABEL, "{%{public}s():%{public}d} " fmt, __FUNCTION__, __LINE__, ##args);                           \
    } while (0)

#define AVCODEC_LOGF(fmt, ...) AVCODEC_LOG(::OHOS::HiviewDFX::HiLog::Fatal, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGE(fmt, ...) AVCODEC_LOG(::OHOS::HiviewDFX::HiLog::Error, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGW(fmt, ...) AVCODEC_LOG(::OHOS::HiviewDFX::HiLog::Warn, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGI(fmt, ...) AVCODEC_LOG(::OHOS::HiviewDFX::HiLog::Info, fmt, ##__VA_ARGS__)
#define AVCODEC_LOGD(fmt, ...) AVCODEC_LOG(::OHOS::HiviewDFX::HiLog::Debug, fmt, ##__VA_ARGS__)
#endif

#define AVCODEC_LOGV(fmt, ...)              \
    if (VERBOSE_LOG) {                      \
        AVCODEC_LOGI(fmt, ##__VA_ARGS__);   \
    }

#define CHECK_AND_CONTINUE(cond)            \
    if (1) {                                \
        if (!(cond)) {                      \
            continue;                       \
        }                                   \
    } else void (0)

#define CHECK_AND_BREAK(cond)               \
    if (1) {                                \
        if (!(cond)) {                      \
            break;                          \
        }                                   \
    } else void (0)

#define CHECK_AND_RETURN(cond)              \
    if (1) {                                \
        if (!(cond)) {                      \
            return;                         \
        }                                   \
    } else void (0)

#define CHECK_AND_RETURN_RET(cond, ret)     \
    if (1) {                                \
        if (!(cond)) {                      \
            return ret;                     \
        }                                   \
    } else void (0)
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_LOG_H