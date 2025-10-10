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

#ifndef AVCODEC_EVENT_MANAGER_H
#define AVCODEC_EVENT_MANAGER_H

#include <mutex>
#include <memory>
#include <string>
#include "meta.h"

namespace OHOS {
namespace MediaAVCodec {
enum class EventType {
    UNKNOWN,
    INSTANCE_INIT,
    INSTANCE_RELEASE,
    INSTANCE_MEMORY_UPDATE,
    INSTANCE_MEMORY_RESET,
    INSTANCE_ENCODE_BEGIN,
    INSTANCE_ENCODE_END,
    END,
};

class EventManager {
public:
    static EventManager &GetInstance();
    void OnInstanceEvent(EventType type, Media::Meta &meta);

private:
    EventManager() {}

/* For extented event callback */
private:
    void OnInstanceInitEvent(Media::Meta &meta);
    void OnInstanceReleaseEvent(Media::Meta &meta);
    void OnInstanceMemoryUpdateEvent(Media::Meta &meta);
    void OnInstanceMemoryResetEvent(Media::Meta &meta);
    void OnInstanceEncodeBeginEvent(Media::Meta &meta);
    void OnInstanceEncodeEndEvent(Media::Meta &meta);
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_EVENT_MANAGER_H
