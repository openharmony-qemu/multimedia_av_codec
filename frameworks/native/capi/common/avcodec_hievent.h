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

#ifndef AVCODEC_HIEVENT_H
#define AVCODEC_HIEVENT_H

#include <string>
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace HiviewDFX::HiAppEvent;
struct AVAppEvent {
    int64_t sumTime;
    std::string apiName;
    std::string errType;
}
}
}
#endif // AVCODEC_HIEVENT_H