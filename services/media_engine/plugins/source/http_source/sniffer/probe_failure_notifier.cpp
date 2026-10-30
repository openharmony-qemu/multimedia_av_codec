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
#define HST_LOG_TAG "ProbeFailureNotifier"
#include "sniffer/probe_failure_notifier.h"
#include <condition_variable>
#include <sstream>
#include <thread>
#include "common/log.h"
#include "common/media_core.h"
#include "utils/string_utils.h"

namespace OHOS::Media::Plugins::HttpPlugin {
namespace {
constexpr int32_t CURL_TIMEOUT = 28;
constexpr char NOTIFICATION_TASK_NAME[] = "ProbeError";
// Bound failed Read/Prepare notification coordination; callback cancellation has its own drain contract.
constexpr auto NOTIFICATION_WAIT = std::chrono::seconds(1);
// Raw Curl/App and HTTP status codes retain DownloadMonitor's existing MediaService mapping.
const std::map<int32_t, MediaServiceErrCode> PROBE_CLIENT_ERRORS = {
    {-6, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
    {-5, MediaServiceErrCode::MSERR_IO_CONNECTION_TIMEOUT},
    {-4, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {-3, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {-2, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
    {-1, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {1, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {2, MediaServiceErrCode::MSERR_DATA_SOURCE_IO_ERROR},
    {4, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {5, MediaServiceErrCode::MSERR_IO_CANNOT_FIND_HOST},
    {6, MediaServiceErrCode::MSERR_IO_CANNOT_FIND_HOST},
    {7, MediaServiceErrCode::MSERR_IO_CANNOT_FIND_HOST},
    {8, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
    {9, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {10, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {11, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {12, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {13, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {14, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {15, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {16, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
    {17, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {18, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
    {30, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {31, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {35, MediaServiceErrCode::MSERR_IO_SSL_CONNECT_FAIL},
    {37, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {38, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {39, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {41, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {45, MediaServiceErrCode::MSERR_IO_CONNECTION_TIMEOUT},
    {47, MediaServiceErrCode::MSERR_IO_CANNOT_FIND_HOST},
    {53, MediaServiceErrCode::MSERR_IO_SSL_CONNECT_FAIL},
    {54, MediaServiceErrCode::MSERR_IO_SSL_CONNECT_FAIL},
    {55, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
    {56, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
    {58, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
    {59, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
    {60, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
    {61, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {64, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {65, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
    {66, MediaServiceErrCode::MSERR_IO_SSL_CONNECT_FAIL},
    {68, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {69, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {71, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {72, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {74, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {77, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
    {78, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
    {79, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {80, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {82, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
    {83, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
    {84, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {85, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {86, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {87, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {90, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
    {91, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
    {95, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {96, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {97, MediaServiceErrCode::MSERR_IO_NETWORK_ABNORMAL},
    {98, MediaServiceErrCode::MSERR_IO_SSL_SERVER_CERT_UNTRUSTED},
    {101, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
};
const std::map<int32_t, MediaServiceErrCode> PROBE_SERVER_ERRORS = {
    {400, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {401, MediaServiceErrCode::MSERR_IO_NO_PERMISSION},
    {403, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {404, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
    {406, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {407, MediaServiceErrCode::MSERR_IO_NO_PERMISSION},
    {408, MediaServiceErrCode::MSERR_IO_CONNECTION_TIMEOUT},
    {409, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {410, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
    {411, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {412, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {413, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {415, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {416, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {417, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {421, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
    {422, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {424, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {425, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {428, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {429, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {431, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {451, MediaServiceErrCode::MSERR_IO_NETWORK_ACCESS_DENIED},
    {500, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
    {501, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {502, MediaServiceErrCode::MSERR_IO_NETWORK_UNAVAILABLE},
    {503, MediaServiceErrCode::MSERR_IO_NETWORK_UNAVAILABLE},
    {504, MediaServiceErrCode::MSERR_IO_CONNECTION_TIMEOUT},
    {505, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {506, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {507, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
    {508, MediaServiceErrCode::MSERR_IO_RESOURE_NOT_FOUND},
    {510, MediaServiceErrCode::MSERR_IO_UNSUPPORTTED_REQUEST},
    {511, MediaServiceErrCode::MSERR_IO_SSL_CLIENT_CERT_NEEDED},
};
}

PluginEvent BuildProbeFailureEvent(const ProbeFailure& failure, const std::string& url)
{
    bool client = failure.clientCode != 0 || failure.serverCode == 0;
    int32_t rawCode = client ? failure.clientCode : failure.serverCode;
    int32_t mediaCode = MSERR_DATA_SOURCE_IO_ERROR;
    const auto& codes = client ? PROBE_CLIENT_ERRORS : PROBE_SERVER_ERRORS;
    auto found = codes.find(rawCode);
    if (found != codes.end()) {
        mediaCode = found->second;
    }
    if (failure.kind == ProbeFailureKind::TIMEOUT) {
        client = true;
        rawCode = CURL_TIMEOUT;
        mediaCode = MSERR_IO_CONNECTION_TIMEOUT;
    }
    std::ostringstream description;
    description << "requestError: true, errorType: " << (client ? "client error" : "server error")
        << ", errorCode: " << rawCode << ", contentType: DEFAULT, maskUrl: "
        << StringUtil::MaskUrl(url.substr(0, url.find_first_of("?#")));
    return {PluginEventType::SERVER_ERROR, {mediaCode}, description.str()};
}

struct ProbeFailureNotifier::Context {
    Context(PluginEvent value, std::weak_ptr<Callback> cb, uint64_t id)
        : event(std::move(value)), callback(std::move(cb)), routeId(id) {}
    const PluginEvent event;
    std::weak_ptr<Callback> callback;
    const uint64_t routeId;
    std::mutex mutex;
    std::condition_variable condition;
    bool cancelled {false};
    bool queued {false};
    bool delivered {false};
    bool inFlight {false};
    std::thread::id callbackThread {};
};

ProbeFailureNotifier::ProbeFailureNotifier(const ProbeFailure& failure, const std::string& url,
    std::weak_ptr<Callback> callback)
    : context_(std::make_shared<Context>(BuildProbeFailureEvent(failure, url), std::move(callback), failure.routeId)),
      task_(std::make_shared<Task>(NOTIFICATION_TASK_NAME, "", TaskType::SINGLETON, TaskPriority::NORMAL, false))
{
}

ProbeFailureNotifier::~ProbeFailureNotifier()
{
    Cancel();
}

void ProbeFailureNotifier::SetCallback(std::weak_ptr<Callback> callback)
{
    std::lock_guard<std::mutex> lock(context_->mutex);
    context_->callback = std::move(callback);
    context_->condition.notify_all();
}

void ProbeFailureNotifier::Notify()
{
    auto context = context_;
    {
        std::lock_guard<std::mutex> lock(context->mutex);
        if (context->cancelled || context->queued || context->delivered || context->callback.expired()) {
            return;
        }
        context->queued = true;
    }
    MEDIA_LOG_I("route=%{public}" PRIu64 " event=probe_error_queued", context->routeId);
    task_->SubmitJobOnce([context] { Deliver(context); });
}

void ProbeFailureNotifier::Deliver(const std::shared_ptr<Context>& context)
{
    std::shared_ptr<Callback> callback;
    {
        std::lock_guard<std::mutex> lock(context->mutex);
        context->queued = false;
        callback = context->callback.lock();
        if (context->cancelled || context->delivered || callback == nullptr) {
            context->condition.notify_all();
            return;
        }
        context->delivered = true;
        context->inFlight = true;
        context->callbackThread = std::this_thread::get_id();
    }
    MEDIA_LOG_I("route=%{public}" PRIu64 " event=probe_error_deliver", context->routeId);
    callback->OnEvent(context->event);
    {
        std::lock_guard<std::mutex> lock(context->mutex);
        context->inFlight = false;
        context->callbackThread = {};
    }
    context->condition.notify_all();
}

void ProbeFailureNotifier::Cancel()
{
    auto context = context_;
    std::unique_lock<std::mutex> lock(context->mutex);
    context->cancelled = true;
    context->condition.notify_all();
    if (context->callbackThread != std::this_thread::get_id()) {
        context->condition.wait(lock, [&] { return !context->inFlight; });
    }
}

void ProbeFailureNotifier::WaitForDelivery()
{
    auto context = context_;
    std::unique_lock<std::mutex> lock(context->mutex);
    context->condition.wait_for(lock, NOTIFICATION_WAIT, [&] {
        return context->cancelled || context->callback.expired() ||
            (context->delivered && (!context->inFlight || context->callbackThread == std::this_thread::get_id()));
    });
}
} // namespace OHOS::Media::Plugins::HttpPlugin
