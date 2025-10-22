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

#include <string>
#include "hitrace_meter.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
#define AVCODEC_SYNC_CUSTOM_TRACE(level, fmt, ...) AVCodecTrace trace(level, "", fmt, ##__VA_ARGS__)
#define AVCODEC_SYNC_TRACE AVCODEC_SYNC_CUSTOM_TRACE(HITRACE_LEVEL_INFO, "%s", __FUNCTION__)

#define AVCODEC_SYNC_CUSTOM_TRACE_WITH_TAG(level, customArgs, fmt, ...) \
    AVCodecTrace trace(level, customArgs, "%s" fmt, tag_.load(), ##__VA_ARGS__)
#define AVCODEC_FUNC_TRACE_WITH_TAG        \
    AVCODEC_SYNC_CUSTOM_TRACE_WITH_TAG(HITRACE_LEVEL_INFO, "", "%s", __FUNCTION__)
#define AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT \
    AVCODEC_SYNC_CUSTOM_TRACE_WITH_TAG(HITRACE_LEVEL_INFO, "", "%s:C", __FUNCTION__)
#define AVCODEC_FUNC_TRACE_WITH_TAG_SERVER \
    AVCODEC_SYNC_CUSTOM_TRACE_WITH_TAG(HITRACE_LEVEL_INFO, "", "%s:S", __FUNCTION__)

class AVCodecTrace : public NoCopyable {
public:
    AVCodecTrace(const std::string& funcName, HiTraceOutputLevel level = HITRACE_LEVEL_INFO) : level_(level)
    {
        StartTraceEx(level, HITRACE_TAG_ZMEDIA, funcName.c_str());
    }
    static void TraceBegin(const std::string& funcName, int32_t taskId, HiTraceOutputLevel level = HITRACE_LEVEL_INFO)
    {
        StartAsyncTraceEx(level, HITRACE_TAG_ZMEDIA, funcName.c_str(), taskId, "");
    }
    static void TraceEnd(const std::string& funcName, int32_t taskId, HiTraceOutputLevel level = HITRACE_LEVEL_INFO)
    {
        FinishAsyncTraceEx(level, HITRACE_TAG_ZMEDIA, funcName.c_str(), taskId);
    }
    static void CounterTrace(const std::string& varName, int32_t val, HiTraceOutputLevel level = HITRACE_LEVEL_INFO)
    {
        CountTraceEx(level, HITRACE_TAG_ZMEDIA, varName.c_str(), val);
    }

    template <typename... Args>
    AVCodecTrace(HiTraceOutputLevel level, const char *customArg, const char *fmt, Args&&... args) : level_(level)
    {
        StartTraceArgsEx(level, HITRACE_TAG_ZMEDIA, customArg, fmt, args...);
    }
    template <typename... Args>
    static void TraceBegin(HiTraceOutputLevel level, const char *customArg, int32_t taskId,
                           const char *fmt, Args&&... args)
    {
        StartAsyncTraceArgsEx(level, HITRACE_TAG_ZMEDIA, taskId, "", customArg, fmt, args...);
    }
    template <typename... Args>
    static void TraceEnd(HiTraceOutputLevel level, int32_t taskId, const char *fmt, Args&&... args)
    {
        FinishAsyncTraceArgsEx(level, HITRACE_TAG_ZMEDIA, taskId, fmt, args...);
    }

    ~AVCodecTrace()
    {
        FinishTraceEx(level_, HITRACE_TAG_ZMEDIA);
    }

private:
    HiTraceOutputLevel level_ = HITRACE_LEVEL_INFO;
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