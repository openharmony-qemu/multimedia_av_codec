/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "unittest_utils.h"
#include <regex>

namespace OHOS {
namespace MediaAVCodec {
void DecArgv(int &index, int &argc, char **argv)
{
    if (argc <= 0 || index >= argc) {
        return;
    }
    for (int i = index; i < argc - 1; ++i) {
        (argv)[i] = (argv)[i + 1];
    }
    --argc;
    --index;
}

int32_t GetNum(const char *str, const std::string key)
{
    int32_t resultInt = 0;
    std::string keyVal = std::string(str);
    std::regex regexKey(key + "=(\\d+)");
    std::regex regexDigital("\\d+");
    std::smatch matchVals;
    if (std::regex_search(keyVal, matchVals, regexKey)) {
        std::string startStr = matchVals[1].str();
        resultInt = std::regex_match(startStr, regexDigital) ? std::stoi(startStr) : 0;
    }
    return resultInt;
}
} // namespace MediaAVCodec
} // namespace OHOS