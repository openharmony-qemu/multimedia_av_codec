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

#ifndef API_METRICS_UTIL_H
#define API_METRICS_UTIL_H

#include <chrono>
#include <cstdint>
#include <string>

namespace OHOS {
namespace MediaAVCodec {
enum MetricesType : int32_t {
    BOOLEAN = 0,
    ENUMERATION,
    CUSTOM_COUNTS,
    TIMES,
    PERCENTAGE,
};

class ApiMetricsReporter {
public:
    ApiMetricsReporter(std::string apiName, MetricesType metricesType, int32_t sample = 0,
        int32_t frequency = 1, bool forceReport = false);
    ~ApiMetricsReporter();

private:
    std::string apiName_;
    MetricesType metricesType_;
    int32_t sample_;
    int32_t frequency_;
    bool forceReport_;
    std::chrono::time_point<std::chrono::steady_clock> beginTime_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // API_METRICS_UTIL_H