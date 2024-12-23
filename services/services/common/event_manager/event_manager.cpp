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

#include "event_manager.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "EventManager"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
EventManager &EventManager::GetInstance()
{
    static EventManager manager;
    return manager;
}

void EventManager::OnInstanceEvent(EventType type, Media::Meta &meta)
{
    CHECK_AND_RETURN_LOG(type > EventType::UNKNOWN && type < EventType::END, "Unknown event type, ignore");
    std::lock_guard<std::mutex> lock(eventMutex_);

    switch (type) {
        case EventType::INSTANCE_INIT:
            OnInstanceInitEvent(meta);
            break;
        case EventType::INSTANCE_RELEASE:
            OnInstanceReleaseEvent(meta);
            break;
        case EventType::INSTANCE_MEMORY_UPDATE:
            OnInstanceMemoryUpdateEvent(meta);
            break;
        case EventType::INSTANCE_FREEZE:
            OnAppFreezeEvent(meta);
            break;
        case EventType::INSTANCE_UNFREEZE:
            OnAppUnfreezeEvent(meta);
            break;
        default:
            AVCODEC_LOGW("Nothing to do with this event: %{public}d", static_cast<int32_t>(type));
            break;
    }
}

void EventManager::OnInstanceInitEvent(Media::Meta &meta)
{
    (void)meta;
}

void EventManager::OnInstanceReleaseEvent(Media::Meta &meta)
{
    (void)meta;
}

void EventManager::OnInstanceMemoryUpdateEvent(Media::Meta &meta)
{
    (void)meta;
}

void EventManager::OnAppFreezeEvent(Media::Meta &meta)
{
    (void)meta;
}

void EventManager::OnAppUnfreezeEvent(Media::Meta &meta)
{
    (void)meta;
}
} // namespace MediaAVCodec
} // namespace OHOS