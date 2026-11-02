/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef HISTREAMER_PROBE_FAILURE_NOTIFIER_H
#define HISTREAMER_PROBE_FAILURE_NOTIFIER_H

#include "osal/task/task.h"
#include "plugin/plugin_base.h"
#include "sniffer/sniffer_coordinator.h"

namespace OHOS::Media::Plugins::HttpPlugin {
PluginEvent BuildProbeFailureEvent(const ProbeFailure& failure, const std::string& url);

// One instance per failed route. Its job owns only Context, never a source or downloader.
class ProbeFailureNotifier {
public:
    ProbeFailureNotifier(const ProbeFailure& failure, const std::string& url, std::weak_ptr<Callback> callback);
    ~ProbeFailureNotifier();
    void SetCallback(std::weak_ptr<Callback> callback);
    void Notify();
    void Cancel();
    void WaitForDelivery();

private:
    struct Context;
    static void Deliver(const std::shared_ptr<Context>& context);
    std::shared_ptr<Context> context_;
    std::shared_ptr<Task> task_;
};
} // namespace OHOS::Media::Plugins::HttpPlugin
#endif
