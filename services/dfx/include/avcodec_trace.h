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

#ifndef AVCODEC_TRACE_H
#define AVCODEC_TRACE_H

#include <cstdint>
#include <string>
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
enum AVCodecTraceLevel : int32_t {
    AVCODEC_TRACE_LEVEL_DEBUG = 0,
    AVCODEC_TRACE_LEVEL_INFO = 1,
    AVCODEC_TRACE_LEVEL_CRITICAL = 2,
    AVCODEC_TRACE_LEVEL_COMMERCIAL = 3,
};

__attribute__((visibility("default"))) void AVCodecStartTrace(AVCodecTraceLevel level, const char *name,
                                                              const char *customArgs = "");
__attribute__((visibility("default"))) void AVCodecFinishTrace(AVCodecTraceLevel level);
__attribute__((visibility("default"))) void AVCodecStartAsyncTrace(AVCodecTraceLevel level, const char *name,
                                                                   int32_t taskId, const char *customArgs = "");
__attribute__((visibility("default"))) void AVCodecFinishAsyncTrace(AVCodecTraceLevel level, const char *name,
                                                                    int32_t taskId);
__attribute__((visibility("default"))) void AVCodecCountTrace(AVCodecTraceLevel level, const char *name,
                                                              int64_t count);
__attribute__((visibility("default"))) void AVCodecStartTraceArgs(AVCodecTraceLevel level, const char *customArg,
                                                                  const char *fmt, ...);
__attribute__((visibility("default"))) void AVCodecStartAsyncTraceArgs(AVCodecTraceLevel level, const char *customArg,
                                                                       int32_t taskId, const char *fmt, ...);
__attribute__((visibility("default"))) void AVCodecFinishAsyncTraceArgs(AVCodecTraceLevel level, int32_t taskId,
                                                                        const char *fmt, ...);

#define AVCODEC_SYNC_CUSTOM_TRACE(level, fmt, ...) AVCodecTrace trace(level, "", fmt, ##__VA_ARGS__)
#define AVCODEC_SYNC_TRACE AVCODEC_SYNC_CUSTOM_TRACE(::OHOS::MediaAVCodec::AVCODEC_TRACE_LEVEL_INFO, "%s", __FUNCTION__)

#define AVCODEC_SYNC_CUSTOM_TRACE_WITH_TAG(level, customArgs, fmt, ...) \
    AVCodecTrace trace(level, customArgs, "%s" fmt, tag_.load(), ##__VA_ARGS__)
#define AVCODEC_FUNC_TRACE_WITH_TAG        \
    AVCODEC_SYNC_CUSTOM_TRACE_WITH_TAG(::OHOS::MediaAVCodec::AVCODEC_TRACE_LEVEL_INFO, "", "%s", __FUNCTION__)
#define AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT \
    AVCODEC_SYNC_CUSTOM_TRACE_WITH_TAG(::OHOS::MediaAVCodec::AVCODEC_TRACE_LEVEL_INFO, "", "%s:C", __FUNCTION__)
#define AVCODEC_FUNC_TRACE_WITH_TAG_SERVER \
    AVCODEC_SYNC_CUSTOM_TRACE_WITH_TAG(::OHOS::MediaAVCodec::AVCODEC_TRACE_LEVEL_INFO, "", "%s:S", __FUNCTION__)

class AVCodecTrace : public NoCopyable {
public:
    AVCodecTrace(const std::string& funcName, AVCodecTraceLevel level = AVCODEC_TRACE_LEVEL_INFO) : level_(level)
    {
        AVCodecStartTrace(level, funcName.c_str());
    }
    static void TraceBegin(const std::string& funcName, int32_t taskId,
                           AVCodecTraceLevel level = AVCODEC_TRACE_LEVEL_INFO)
    {
        AVCodecStartAsyncTrace(level, funcName.c_str(), taskId);
    }
    static void TraceEnd(const std::string& funcName, int32_t taskId,
                         AVCodecTraceLevel level = AVCODEC_TRACE_LEVEL_INFO)
    {
        AVCodecFinishAsyncTrace(level, funcName.c_str(), taskId);
    }
    static void CounterTrace(const std::string& varName, int32_t val,
                             AVCodecTraceLevel level = AVCODEC_TRACE_LEVEL_INFO)
    {
        AVCodecCountTrace(level, varName.c_str(), val);
    }

    template <typename... Args>
    AVCodecTrace(AVCodecTraceLevel level, const char *customArg, const char *fmt, Args&&... args) : level_(level)
    {
        AVCodecStartTraceArgs(level, customArg, fmt, args...);
    }
    template <typename... Args>
    static void TraceBegin(AVCodecTraceLevel level, const char *customArg, int32_t taskId,
                           const char *fmt, Args&&... args)
    {
        AVCodecStartAsyncTraceArgs(level, customArg, taskId, fmt, args...);
    }
    template <typename... Args>
    static void TraceEnd(AVCodecTraceLevel level, int32_t taskId, const char *fmt, Args&&... args)
    {
        AVCodecFinishAsyncTraceArgs(level, taskId, fmt, args...);
    }

    ~AVCodecTrace()
    {
        AVCodecFinishTrace(level_);
    }

private:
    AVCodecTraceLevel level_ = AVCODEC_TRACE_LEVEL_INFO;
};

#ifdef MEDIA_TRACE_DEBUG_ENABLE
#define MEDIA_TRACE_DEBUG(name) MediaAVCodec::AVCodecTrace trace(name)
#define MEDIA_TRACE_DEBUG_POSTFIX(name, postfix) MediaAVCodec::AVCodecTrace trace##postfix(name)
#else
#define MEDIA_TRACE_DEBUG(name) ((void)0)
#define MEDIA_TRACE_DEBUG_POSTFIX(name, postfix) ((void)0)
#endif
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_TRACE_H
