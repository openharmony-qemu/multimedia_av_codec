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

#include <hilog/log_cpp.h>
#include <cinttypes>

inline constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD002B32, "HCODEC"};

#define LOG_FMT "[%{public}s %{public}d] "
#define LOGE(x, ...) OHOS::HiviewDFX::HiLog::Error(LABEL, LOG_FMT x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(x, ...) OHOS::HiviewDFX::HiLog::Warn(LABEL, LOG_FMT x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGI(x, ...) OHOS::HiviewDFX::HiLog::Info(LABEL, LOG_FMT x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(x, ...) OHOS::HiviewDFX::HiLog::Debug(LABEL, LOG_FMT x, __FUNCTION__, __LINE__, ##__VA_ARGS__)

// for HCodec
#define HLOG_FMT "%{public}s[%{public}s][%{public}s %{public}d] "
#define HLOGE(x, ...) OHOS::HiviewDFX::HiLog::Error(LABEL, HLOG_FMT x, compUniqueStr_.c_str(), \
    currState_->GetName().c_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define HLOGW(x, ...) OHOS::HiviewDFX::HiLog::Warn(LABEL, HLOG_FMT x, compUniqueStr_.c_str(), \
    currState_->GetName().c_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define HLOGI(x, ...) OHOS::HiviewDFX::HiLog::Info(LABEL, HLOG_FMT x, compUniqueStr_.c_str(), \
    currState_->GetName().c_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define HLOGD(x, ...) \
    do {    \
        if (debugMode_) {   \
            OHOS::HiviewDFX::HiLog::Debug(LABEL, HLOG_FMT x, compUniqueStr_.c_str(), \
            currState_->GetName().c_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }   \
    } while (0)

// for HCodec inner state
#define SLOGE(x, ...) OHOS::HiviewDFX::HiLog::Error(LABEL, HLOG_FMT x, \
    codec_->compUniqueStr_.c_str(), stateName_.c_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SLOGW(x, ...) OHOS::HiviewDFX::HiLog::Warn(LABEL, HLOG_FMT x, \
    codec_->compUniqueStr_.c_str(), stateName_.c_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SLOGI(x, ...) OHOS::HiviewDFX::HiLog::Info(LABEL, HLOG_FMT x, \
    codec_->compUniqueStr_.c_str(), stateName_.c_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SLOGD(x, ...) \
    do {    \
        if (codec_->debugMode_) {   \
            OHOS::HiviewDFX::HiLog::Debug(LABEL, HLOG_FMT x, \
            codec_->compUniqueStr_.c_str(), stateName_.c_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }   \
    } while (0)

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