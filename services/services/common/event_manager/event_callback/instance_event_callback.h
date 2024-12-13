/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_EVENT_CALLBACK_H
#define AVCODEC_EVENT_CALLBACK_H

#include "meta.h"

namespace OHOS {
namespace MediaAVCodec {
enum class EventType {
    UNKNOWN,
    INIT,
    MEMORY,
    FREEZE,
    UNFREEZE,
    END,
};

class InstanceEventCallback {
public:
    virtual ~InstanceEventCallback() {}
    virtual void OnInstanceEvent(EventType type, Media::Meta &meta) = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_EVENT_CALLBACK_H
