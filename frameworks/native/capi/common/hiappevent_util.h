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

#ifndef HIAPPEVENT_UTIL_H
#define HIAPPEVENT_UTIL_H

#include <string>
#include <mutex>
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class AppEventReporter {
public:
    explicit AppEventReporter(uint32_t reportThd = 20); // report num 20
    void ReportRecord(const std::string &apiName, int errorCode, int64_t costTime);
private:
    void UploadRecordData(const std::string &apiName) const;

    std::mutex reportMutex_;
    const uint32_t reportThd_;
    int64_t totalCostTime_{0};
    int64_t maxCostTime_{std::numeric_limits<int64_t>::min()};
    int64_t minCostTime_{std::numeric_limits<int64_t>::max()};;
    std::vector<int32_t> errorCode_{};

    static std::atomic<int64_t> processorId_;
};

class ApiInvokeRecorder {
public:
    explicit ApiInvokeRecorder(std::string apiName, AppEventReporter &reporter);
    ~ApiInvokeRecorder();
    void SetErrorCode(int errorCode);

private:
    std::string apiName_;
    int32_t errorCode_{0};
    int64_t beginTime_;
    AppEventReporter &reporter_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // HIAPPEVENT_UTIL_H