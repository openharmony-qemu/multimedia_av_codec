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

#include <avcodec_trace.h>
#include "hitrace_meter.h"

namespace OHOS {
namespace MediaAVCodec {
AVCodecTrace::AVCodecTrace(const std::string& funcName)
{
    StartTrace(HITRACE_TAG_ZMEDIA, funcName);
}

void AVCodecTrace::TraceBegin(const std::string& funcName, int32_t taskId)
{
    StartAsyncTrace(HITRACE_TAG_ZMEDIA, funcName, taskId);
}

void AVCodecTrace::TraceEnd(const std::string& funcName, int32_t taskId)
{
    FinishAsyncTrace(HITRACE_TAG_ZMEDIA, funcName, taskId);
}

void AVCodecTrace::CounterTrace(const std::string& varName, int32_t val)
{
    CountTrace(HITRACE_TAG_ZMEDIA, varName, val);
}

AVCodecTrace::~AVCodecTrace()
{
    FinishTrace(HITRACE_TAG_ZMEDIA);
}
} // namespace MediaAVCodec
} // namespace OHOS
