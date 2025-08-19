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

#ifndef HCODEC_DFX_H
#define HCODEC_DFX_H
#include "hitrace_meter.h"
namespace OHOS::MediaAVCodec {

#define SCOPED_TRACE() \
    HITRACE_METER_FMT(HITRACE_TAG_ZMEDIA, "[hcodec][%s]%s", compUniqueStr_.c_str(), __func__)
#define SCOPED_TRACE_FMT(fmt, ...) \
    HITRACE_METER_FMT(HITRACE_TAG_ZMEDIA, "[hcodec][%s]%s " fmt, compUniqueStr_.c_str(), __func__, ##__VA_ARGS__)

struct FuncTracker {
    explicit FuncTracker(std::string value);
    ~FuncTracker();
private:
    std::string value_;
};
#define FUNC_TRACKER() FuncTracker tracker("[" + compUniqueStr_ + " " + __func__ + "]")

}
#endif