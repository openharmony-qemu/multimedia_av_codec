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
#include "avcodec_log.h"
#ifdef HISTOGRAMMANAGER_ENABLE
#include "histogram_plugin_macros.h"
#endif
namespace OHOS {
namespace MediaAVCodec {
ApiMetricsReporter::ApiMetricsReporter(std::string apiName, MetricesType metricesType, int32_t sample) :
    apiName_(std::move(apiName)),
    metricesType_(metricesType),
    sample_(sample),
    beginTime_(std::chrono::steady_clock::now()) {}

ApiMetricsReporter::~ApiMetricsReporter()
{
#ifdef HISTOGRAMMANAGER_ENABLE
    switch (metricesType_) {
        case ENUMERATION:
            if (sample_ == 201) { // drm err 201->21
                sample_ = 21;
            } else if (sample_ == 301) { // csc err 301->31
                sample_ = 31;
            }
            HISTOGRAM_ENUMERATION(apiName_.c_str(), sample_, 50); // 50：boundary
            break;
        case TIMES:
            HISTOGRAM_TIMES(apiName_.c_str(), std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - beginTime_).count());
            break;
        default:
            break;
    }
#endif
}
} // namespace MediaAVCodec
} // namespace OHOS