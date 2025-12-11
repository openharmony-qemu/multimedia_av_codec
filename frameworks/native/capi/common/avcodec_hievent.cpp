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

#include <avcodec_hievent.h>
#include <unistd.h>
#include "avcodec_log.h"
#include "app_event.h"
#include "app_event_processor_mgr.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVcodec_hiappevent"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace HiviewDFX::HiAppEvent;
void WriteCallEvent(AVAppEvent event)
{
    AVCODEC_LOGI("report API invoke event %{public}s", event.apiName.c_str());
    Event eventOutput("api_diagnostic", "api_called_stat", BEHAVIOR);
    eventOutput.AddParam("api_name", event.apiName);
    eventOutput.AddParam("sdk_name", std:string("AVCodecKits"));
    eventOutput.AddParam("total_cost_time", event.sumTime);
    Write(eventOutput);
}
} // namespace MediaAVCodec
} // namespace OHOS
