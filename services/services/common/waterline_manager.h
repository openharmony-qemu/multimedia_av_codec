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

#include "client/memory_collector_client.h"
#include "config_json_parser.h"
#include <map>

namespace OHOS {
namespace MediaAVCodec {
using ConfigMap = std::map <std::string, uint32_t>;

class DeviceDetector {
public:
    static void TryToGetDeviceType();
    static std::string deviceType_;
};

class WaterLineManager {
public:
    WaterLineManager();
    ~WaterLineManager() = default;
    uint32_t GetWaterLine();
    void ReportHiview(int32_t appId);

private:
    void DoParseConfig(std::string filePath);

private:
    ConfigMap configParam_;
    ConfigJsonParser parser_;
    std::shared_ptr<HiviewDFX::UCollectClient::MemoryCollector> collector_;
}; // WaterLineManager
} // MediaAVCodec
} // OHOS
