/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <cstdarg>

#include "avcodec_trace.h"

#include "hitrace_meter.h"
#include "securec.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr uint64_t AVCODEC_HITRACE_TAG = HITRACE_TAG_ZMEDIA;
constexpr size_t TRACE_NAME_MAX_SIZE = 256;

HiTraceOutputLevel ConvertTraceLevel(AVCodecTraceLevel level)
{
    switch (level) {
        case AVCODEC_TRACE_LEVEL_DEBUG:
            return HITRACE_LEVEL_DEBUG;
        case AVCODEC_TRACE_LEVEL_CRITICAL:
            return HITRACE_LEVEL_CRITICAL;
        case AVCODEC_TRACE_LEVEL_COMMERCIAL:
            return HITRACE_LEVEL_COMMERCIAL;
        case AVCODEC_TRACE_LEVEL_INFO:
        default:
            return HITRACE_LEVEL_INFO;
    }
}

bool FormatTraceName(char *name, size_t size, const char *fmt, va_list args)
{
    if (name == nullptr || size == 0 || fmt == nullptr) {
        return false;
    }
    int32_t ret = vsnprintf_s(name, size, size - 1, fmt, args);
    if (ret < 0) {
        name[0] = '\0';
        return false;
    }
    return true;
}
} // namespace

void AVCodecStartTrace(AVCodecTraceLevel level, const char *name, const char *customArgs)
{
    StartTraceEx(ConvertTraceLevel(level), AVCODEC_HITRACE_TAG, name, customArgs);
}

void AVCodecFinishTrace(AVCodecTraceLevel level)
{
    FinishTraceEx(ConvertTraceLevel(level), AVCODEC_HITRACE_TAG);
}

void AVCodecStartAsyncTrace(AVCodecTraceLevel level, const char *name, int32_t taskId, const char *customArgs)
{
    StartAsyncTraceEx(ConvertTraceLevel(level), AVCODEC_HITRACE_TAG, name, taskId, "", customArgs);
}

void AVCodecFinishAsyncTrace(AVCodecTraceLevel level, const char *name, int32_t taskId)
{
    FinishAsyncTraceEx(ConvertTraceLevel(level), AVCODEC_HITRACE_TAG, name, taskId);
}

void AVCodecCountTrace(AVCodecTraceLevel level, const char *name, int64_t count)
{
    CountTraceEx(ConvertTraceLevel(level), AVCODEC_HITRACE_TAG, name, count);
}

void AVCodecStartTraceArgs(AVCodecTraceLevel level, const char *customArg, const char *fmt, ...)
{
    char name[TRACE_NAME_MAX_SIZE] = {0};
    va_list args;
    va_start(args, fmt);
    bool isFormatted = FormatTraceName(name, sizeof(name), fmt, args);
    va_end(args);
    if (!isFormatted) {
        return;
    }
    AVCodecStartTrace(level, name, customArg);
}

void AVCodecStartAsyncTraceArgs(AVCodecTraceLevel level, const char *customArg, int32_t taskId, const char *fmt, ...)
{
    char name[TRACE_NAME_MAX_SIZE] = {0};
    va_list args;
    va_start(args, fmt);
    bool isFormatted = FormatTraceName(name, sizeof(name), fmt, args);
    va_end(args);
    if (!isFormatted) {
        return;
    }
    AVCodecStartAsyncTrace(level, name, taskId, customArg);
}

void AVCodecFinishAsyncTraceArgs(AVCodecTraceLevel level, int32_t taskId, const char *fmt, ...)
{
    char name[TRACE_NAME_MAX_SIZE] = {0};
    va_list args;
    va_start(args, fmt);
    bool isFormatted = FormatTraceName(name, sizeof(name), fmt, args);
    va_end(args);
    if (!isFormatted) {
        return;
    }
    AVCodecFinishAsyncTrace(level, name, taskId);
}
} // namespace MediaAVCodec
} // namespace OHOS
