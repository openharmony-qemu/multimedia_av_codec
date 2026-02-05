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

#include "hiappevent_util.h"
#include <unistd.h>
#include <ctime>
#include <map>
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "app_event.h"
#include "app_event_processor_mgr.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVcodec_hiappevent"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace HiviewDFX::HiAppEvent;

int64_t GetElapsedNanoSecondsSinceBoot()
{
    struct timespec times{};
    if (clock_gettime(CLOCK_BOOTTIME, &times) == -1) {
        return -1;
    }
    constexpr int64_t secondToNanosecond = 1 * 1000 * 1000 * 1000; // 1000
    return times.tv_sec * secondToNanosecond + times.tv_nsec;
}

std::atomic<int64_t> AppEventReporter::processorId_ = -1;

AppEventReporter::AppEventReporter(uint32_t reportThd) : reportThd_(reportThd)
{
    if (processorId_.load() == -1) {
        ReportConfig config;
        config.name = "ha_app_event";
        config.configName = "SDK_OCG";
        processorId_ = AppEventProcessorMgr::AddProcessor(config);
    }
    if (processorId_.load() == -200) { // -200：not app process
        AVCODEC_LOGE("failed to AddProcessor, processorId %{public}" PRId64, processorId_.load());
        return;
    }
}

void AppEventReporter::ReportRecord(const std::string &apiName, int errorCode, int64_t costTime)
{
    std::lock_guard<std::mutex> lock(reportMutex_);
    if (processorId_.load() < 0 || reportThd_ == 0) {
        return;
    }

    errorCode_.emplace_back(errorCode);
    maxCostTime_ = std::max(maxCostTime_, costTime);
    minCostTime_ = std::min(minCostTime_, costTime);
    totalCostTime_ += costTime;
    if (errorCode_.size() >= reportThd_) {
        UploadRecordData(apiName);
        errorCode_.assign({});
        minCostTime_ = std::numeric_limits<int64_t>::max();
        maxCostTime_ = std::numeric_limits<int64_t>::min();
        totalCostTime_ = 0;
    }
}

void AppEventReporter::UploadRecordData(const std::string &apiName) const
{
    std::map<std::string, int32_t> errType2Num;
    std::vector<std::string> errType;
    std::vector<int32_t> errNum;

    Event event("api_diagnostic", "api_called_stat_cnt", HiviewDFX::HiAppEvent::BEHAVIOR);
    event.AddParam("api_name", apiName);
    event.AddParam("sdk_name", std::string("AVCodecKit"));
    event.AddParam("call_times", static_cast<int32_t>(errorCode_.size()));
    int32_t successTime = 0;
    for (const auto &errCode : errorCode_) {
        if (errCode == 0) {
            successTime++;
        } else {
            std::string err = std::to_string(errCode);
            errType2Num[err] = !errType2Num[err] ? 1 : errType2Num[err] + 1;
        }
    }
    for (auto it = errType2Num.begin(); it != errType2Num.end(); it++) {
        errType.push_back(it->first);
        errNum.push_back(it->second);
    }
    event.AddParam("error_code_types", errType);
    event.AddParam("error_code_num", errNum);
    event.AddParam("success_times", successTime);
    event.AddParam("max_cost_time", maxCostTime_ / 1000000); // 1000000: ns -> ms
    event.AddParam("min_cost_time", minCostTime_ / 1000000); // 1000000: ns -> ms
    event.AddParam("total_cost_time", totalCostTime_ / 1000000); // 1000000: ns -> ms
    int32_t ret = Write(event);
    AVCODEC_LOGI("%{public}s event write result: %{public}d", apiName.c_str(), ret);
}

ApiInvokeRecorder::ApiInvokeRecorder(std::string apiName, AppEventReporter &reporter) : apiName_(std::move(apiName)),
    beginTime_(GetElapsedNanoSecondsSinceBoot()), reporter_(reporter) {}

ApiInvokeRecorder::~ApiInvokeRecorder()
{
    if (beginTime_ < 0) {
        return;
    }
    const int64_t costTime = GetElapsedNanoSecondsSinceBoot() - beginTime_;
    if (costTime < 0) {
        return;
    }
    reporter_.ReportRecord(apiName_, errorCode_, costTime);
}

void ApiInvokeRecorder::SetErrorCode(int errorCode)
{
    errorCode_ = errorCode;
}
} // namespace MediaAVCodec
} // namespace OHOS