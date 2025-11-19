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

#include "event_manager.h"
#include "avcodec_log.h"
#include "avcodec_server_manager.h"
#include "meta/meta_key.h"

#include "event_info_extented_key.h"
#include "instance_memory_update_event_handler.h"
#include "instance_operation_event_handler.h"
#include "statistics_event_handler.h"

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

    switch (type & EventType::MASk) {
        case EventType::INSTANCE_INIT:
            OnInstanceInitEvent(meta);
            break;
        case EventType::INSTANCE_RELEASE:
            OnInstanceReleaseEvent(meta);
            break;
        case EventType::INSTANCE_MEMORY_UPDATE:
            OnInstanceMemoryUpdateEvent(meta);
            break;
        case EventType::INSTANCE_MEMORY_RESET:
            OnInstanceMemoryResetEvent(meta);
            break;
        case EventType::INSTANCE_ENCODE_BEGIN:
            OnInstanceEncodeBeginEvent(meta);
            break;
        case EventType::INSTANCE_ENCODE_END:
            OnInstanceEncodeEndEvent(meta);
            break;
        case EventType::STATISTICS_EVENT:
            OnStatisticsEvent(static_cast<StatisticsEventType>(type), meta);
            break;
        case EventType::STATISTICS_EVENT_SUBMIT:
            OnStatisticsEventSubmit();
            break;
        default:
            AVCODEC_LOGW("Nothing to do with this event: %{public}d", static_cast<int32_t>(type));
            break;
    }
}

void EventManager::OnInstanceEvent(StatisticsEventType type, Media::Meta &meta)
{
    OnStatisticsEvent(type, meta);
}

void EventManager::OnInstanceInitEvent(Media::Meta &meta)
{
    auto instanceId = GetInstanceIdFromMeta(meta);
    auto instanceInfoOpt = AVCodecServerManager::GetInstance().GetInstanceInfoByInstanceId(instanceId);
    CHECK_AND_RETURN_LOG(instanceInfoOpt != std::nullopt, "Can not find this instance, id: %{public}d", instanceId);
    auto instanceInfo = instanceInfoOpt.value();

    meta.GetData(Media::Tag::AV_CODEC_CALLER_PID,                  instanceInfo.caller.pid);
    meta.GetData(Media::Tag::AV_CODEC_CALLER_UID,                  instanceInfo.caller.uid);
    meta.GetData(Media::Tag::AV_CODEC_CALLER_PROCESS_NAME,         instanceInfo.caller.processName);
    meta.GetData(Media::Tag::AV_CODEC_FORWARD_CALLER_PID,          instanceInfo.forwardCaller.pid);
    meta.GetData(Media::Tag::AV_CODEC_FORWARD_CALLER_UID,          instanceInfo.forwardCaller.uid);
    meta.GetData(Media::Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, instanceInfo.forwardCaller.processName);
    meta.GetData(EventInfoExtentedKey::CODEC_TYPE.data(),          instanceInfo.codecType);
    meta.GetData(Media::Tag::MEDIA_CODEC_NAME,                     instanceInfo.codecName);
    AVCodecServerManager::GetInstance().SetInstanceInfoByInstanceId(instanceId, instanceInfo);
}

void EventManager::OnInstanceReleaseEvent(Media::Meta &meta)
{
    InstanceMemoryUpdateEventHandler::GetInstance().OnInstanceRelease(meta);
}

void EventManager::OnInstanceMemoryUpdateEvent(Media::Meta &meta)
{
    InstanceMemoryUpdateEventHandler::GetInstance().OnInstanceMemoryUpdate(meta);
}

void EventManager::OnInstanceMemoryResetEvent(Media::Meta &meta)
{
    InstanceMemoryUpdateEventHandler::GetInstance().OnInstanceMemoryReset(meta);
}

void EventManager::OnInstanceEncodeBeginEvent(Media::Meta &meta)
{
    InstanceOperationEventHandler::GetInstance().OnInstanceEncodeBegin(meta);
}

void EventManager::OnInstanceEncodeEndEvent(Media::Meta &meta)
{
    InstanceOperationEventHandler::GetInstance().OnInstanceEncodeEnd(meta);
}

void EventManager::OnStatisticsEvent(StatisticsEventType type, Media::Meta &meta)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(type, meta);
}

void EventManager::OnStatisticsEventSubmit()
{
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}
} // namespace MediaAVCodec
} // namespace OHOS