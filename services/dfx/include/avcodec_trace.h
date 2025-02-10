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
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
#define AVCODEC_SYNC_TRACE AVCodecTrace trace(std::string(__FUNCTION__))

class __attribute__((visibility("default"))) AVCodecTrace : public NoCopyable {
public:
    explicit AVCodecTrace(const std::string& funcName);
    static void TraceBegin(const std::string& funcName, int32_t taskId);
    static void TraceEnd(const std::string& funcName, int32_t taskId);
    static void CounterTrace(const std::string& varName, int32_t val);
    ~AVCodecTrace();
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_TRACE_H