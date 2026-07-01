/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "api_metrics_util.h"

#include <mutex>
#include <unordered_map>
#include <utility>

#ifdef HISTOGRAMMANAGER_ENABLE
#include "histogram_plugin_macros.h"
#endif

namespace {
std::mutex g_apiMetricsMutex;
std::unordered_map<std::string, int32_t> g_apiMetricsCounters;

std::string BuildMetricsKey(const std::string &apiName, OHOS::MediaAVCodec::MetricesType metricesType)
{
    return apiName + "#" + std::to_string(static_cast<int32_t>(metricesType));
}

bool ShouldReportByFrequency(const std::string &apiName, OHOS::MediaAVCodec::MetricesType metricesType,
    int32_t frequency, bool forceReport)
{
    if (forceReport) {
        std::lock_guard<std::mutex> lock(g_apiMetricsMutex);
        auto &count = g_apiMetricsCounters[BuildMetricsKey(apiName, metricesType)];
        count = 0;
        return true;
    }

    if (frequency <= 1) {
        return true;
    }

    std::lock_guard<std::mutex> lock(g_apiMetricsMutex);
    auto &count = g_apiMetricsCounters[BuildMetricsKey(apiName, metricesType)];

    ++count;
    if (count >= frequency) {
        count = 0;
        return true;
    }
    return false;
}
}

namespace OHOS {
namespace MediaAVCodec {
ApiMetricsReporter::ApiMetricsReporter(std::string apiName, MetricesType metricesType, int32_t sample,
    int32_t frequency, bool forceReport)
    : apiName_(std::move(apiName)), metricesType_(metricesType), sample_(sample), frequency_(frequency),
    forceReport_(forceReport), beginTime_(std::chrono::steady_clock::now())
{
    (void)apiName_;
    (void)metricesType_;
    (void)sample_;
    (void)frequency_;
    (void)forceReport_;
    (void)beginTime_;
}

ApiMetricsReporter::~ApiMetricsReporter()
{
#ifdef HISTOGRAMMANAGER_ENABLE
    switch (metricesType_) {
        case ENUMERATION:
            if (sample_ == 0) {
                break;
            }
            if (sample_ == 201) { // drm err 201->21
                sample_ = 21;
            } else if (sample_ == 301) { // csc err 301->31
                sample_ = 31;
            }
            HISTOGRAM_ENUMERATION(apiName_.c_str(), sample_, 50); // 50：boundary
            break;
        case TIMES:
            if (!ShouldReportByFrequency(apiName_, metricesType_, frequency_, forceReport_)) {
                break;
            }
            HISTOGRAM_TIMES(apiName_.c_str(), static_cast<int32_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - beginTime_).count()));
            break;
        default:
            break;
    }
#endif
}
} // namespace MediaAVCodec
} // namespace OHOS
