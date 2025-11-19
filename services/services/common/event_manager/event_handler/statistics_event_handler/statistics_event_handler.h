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

#ifndef AVCODEC_DFX_EVENTINFO_STATISTICS_H
#define AVCODEC_DFX_EVENTINFO_STATISTICS_H

#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include "meta/meta.h"
#include "event_type.h"

namespace OHOS {
namespace MediaAVCodec {
class StatisticsEventInfoBase {
public:
    StatisticsEventInfoBase() = default;
    virtual ~StatisticsEventInfoBase() = default;

    virtual void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) = 0;
    virtual void OnSubmitEventInfo() {};
    virtual void ResetEventInfo() {};

protected:
    std::unordered_map<StatisticsEventType, std::shared_ptr<StatisticsEventInfoBase>> eventInfoMap_;
};

class StatisticsEventInfo : public StatisticsEventInfoBase {
public:
    using EventHooker = std::function<bool(const Media::Meta &)>;   // return true to erase hooker

    StatisticsEventInfo();

    static StatisticsEventInfo &GetInstance()
    {
        static StatisticsEventInfo instance;
        return instance;
    }

    void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) override;
    void OnSubmitEventInfo() override;
    void ResetEventInfo() override;
    void RegisterEventHooker(StatisticsEventType eventType, EventHooker hooker);

private:
    std::shared_mutex mutex_;
    std::unordered_map<StatisticsEventType, EventHooker> eventHookers_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_DFX_EVENTINFO_STATISTICS_H