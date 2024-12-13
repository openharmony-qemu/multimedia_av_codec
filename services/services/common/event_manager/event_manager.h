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

#ifndef AVCODEC_EVENT_MANAGER_H
#define AVCODEC_EVENT_MANAGER_H

#include <mutex>
#include <memory>
#include "instance_init_event_callback.h"
#include "instance_memory_event_callback.h"
#include "rss_event_callback.h"

namespace OHOS {
namespace MediaAVCodec {
class EventManager : std::enable_shared_from_this<EventManager>,
    private InstanceInitEventCallback, InstanceMemoryEventCallback, RssEventCallback {
public:
    static EventManager &GetInstance();
    void OnInstanceEvent(EventType type, Media::Meta &meta) override;

private:
    std::mutex eventMutex_;

/* For extented event callback */
private:
    void OnInstanceInitEvent(Media::Meta &meta) override;
    void OnInstanceMemoryEvent(Media::Meta &meta) override;
    void OnAppFreezeEvent(Media::Meta &meta) override;
    void OnAppUnfreezeEvent(Media::Meta &meta) override;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_EVENT_MANAGER_H
