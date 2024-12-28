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

#include "config_json_parser.h"
#include <fstream>
#include <cstdio>
#include "cJSON.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "ConfigJsonParser"};
}

namespace OHOS {
namespace MediaAVCodec {
ConfigJsonParser::ConfigJsonParser() {}

ConfigJsonParser::~ConfigJsonParser()
{
    if (root_ != nullptr) {
        cJSON_Delete(root_);
        root_ = nullptr;
    }
}

bool ConfigJsonParser::InitJsonFile(const std::string &path)
{
    if (root_ != nullptr) {
        return false;
    }
    std::ifstream fin(path.data());
    CHECK_AND_RETURN_RET_LOG(fin.is_open(), false, "open path %{public}s failed", path.c_str());

    std::string strText;
    std::string configJson;
    while (getline(fin, strText)) {
        configJson += strText;
    }
    AVCODEC_LOGD("open path, get string %{public}s", configJson.c_str());
    root_ = cJSON_Parse(configJson.c_str());
    CHECK_AND_RETURN_RET_LOG(root_ != nullptr, false, "root_ is null, stop parser config");
    return true;
}

const cJSON* ConfigJsonParser::GetSubNode(const std::string &key, const cJSON* node) const
{
    return cJSON_GetObjectItem(node, key.c_str());
}

int ConfigJsonParser::GetIntValue(const std::string &key, const cJSON* node) const
{
    auto value = GetSubNode(key, node);
    return cJSON_IsNumber(value) ? value->valueint : 0;
}

cJSON* ConfigJsonParser::GetRootNode() const
{
    return root_;
}
} // MediaAVCodec
} // OHOS