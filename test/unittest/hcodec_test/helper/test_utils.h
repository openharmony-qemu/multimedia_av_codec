/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef COST_RECORDER_H
#define COST_RECORDER_H

#include <chrono>
#include <string>
#include <map>

namespace OHOS::MediaAVCodec {
std::string GetCodecName(bool isEncoder, const std::string& mime);

struct CostRecorder {
    static CostRecorder& Instance();
    void Clear();
    void Update(std::chrono::steady_clock::time_point begin, const std::string& apiName);
    void Print() const;

private:
    struct Total {
        long long totalCost = 0;
        uint32_t totalCnt = 0;
    };
    std::map<std::string, Total> records_;
};
}
#endif