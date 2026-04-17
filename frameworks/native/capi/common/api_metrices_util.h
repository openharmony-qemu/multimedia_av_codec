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

#ifndef API_METRICES_UTIL_H
#define API_METRICES_UTIL_H

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

class ApiMetricesReporter {
public:
    explicit ApiMetricesReporter(std::string apiName, MetricesType metricesType, int32_t sample = 0);
    ~ApiMetricesReporter();

private:
    MetricesType metricesType_;
    std::string apiName_;
    int32_t sample_;
    int32_t errorCode_{0};
    int64_t beginTime_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // API_METRICES_UTIL_H