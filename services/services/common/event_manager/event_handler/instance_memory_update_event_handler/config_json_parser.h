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

#ifndef AVCODEC_CONFIG_JSON_PARSER_H
#define AVCODEC_CONFIG_JSON_PARSER_H

#include <cstdint>
#include <string>
#include <vector>
#include "cJSON.h"

namespace OHOS {
namespace MediaAVCodec {
class ConfigJsonParser {
public:
    ConfigJsonParser();
    ~ConfigJsonParser();

    bool InitJsonFile(const std::string &path);
    const cJSON* GetSubNode(const std::string &key, const cJSON* node) const;
    int GetIntValue(const std::string &key, const cJSON* node) const;
    cJSON* GetRootNode() const;

private:
    cJSON* root_ { nullptr };
};
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_CONFIG_JSON_PARSER_H