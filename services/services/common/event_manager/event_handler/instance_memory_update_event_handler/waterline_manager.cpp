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

#include "waterline_manager.h"
#include <cstdio>
#include "parameters.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "WaterLineManager"};
const std::string KERNAL_CONFIG_PATH = "/system/etc/hiview/kernel_leak_config.json";
constexpr uint32_t DEFAULT_WATER_LINE = 9999;
constexpr const char* const KEY_DEVICE_TYPE = "cosnt.product.devicetype";
const std::string PHONE = "phone";
const std::string TABLET = "tablet";
const std::string PC = "pc";
const std::string WATCH = "watch";
const std::string UNKNOWN = "unknown";
}

namespace OHOS {
namespace MediaAVCodec {
std::string DeviceDetector::deviceType_ {"unknown"};

void DeviceDetector::TryToGetDeviceType()
{
    if (deviceType_ != UNKNOWN) {
        return;
    }
    deviceType_ = OHOS::system::GetParameter(KEY_DEVICE_TYPE, UNKNOWN);
}

WaterLineManager::WaterLineManager()
{
    collector_ = HiviewDFX::UCollectClient::MemoryCollector::Create();
    DeviceDetector::TryToGetDeviceType();
    DoParseConfig(KERNAL_CONFIG_PATH);
}

void WaterLineManager::DoParseConfig(std::string filePath)
{
    if (!parser_.InitJsonFile(filePath)) {
        return;
    }
    auto rootNode = parser_.GetSubNode("av_codec_config", parser_.GetRootNode());
    configParam_[PHONE] = static_cast<int32_t>(parser_.GetIntValue(PHONE, rootNode));
    configParam_[TABLET] = static_cast<int32_t>(parser_.GetIntValue(TABLET, rootNode));
    configParam_[PC] = static_cast<int32_t>(parser_.GetIntValue(PC, rootNode));
    configParam_[WATCH] = static_cast<int32_t>(parser_.GetIntValue(WATCH, rootNode));
    configParam_[UNKNOWN] = static_cast<int32_t>(parser_.GetIntValue(UNKNOWN, rootNode));
}

uint32_t WaterLineManager::GetWaterLine()
{
    std::string deviceType =  DeviceDetector::deviceType_;
    if (configParam_.find(deviceType) != configParam_.end()){
        return configParam_.find(deviceType)->second;
    }
    return DEFAULT_WATER_LINE;
}

void WaterLineManager::ReportHiview(int32_t appId)
{
    CHECK_AND_RETURN_LOG(collector_ != nullptr, "collector_ is null");

    std::vector<HiviewDFX::UCollectClient::MemoryCaller> memList;
    HiviewDFX::UCollectClient::MemoryCaller memoryCaller = {
        .pid = appId,
        .resourceType = "AVCodec",
        .limitValue = GetWaterLine(),
    };
    memList.emplace_back(memoryCaller);
    // collector_->SetSplitMemoryValue(memList);
}
} // MediaAVCodec
} // OHOS