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

#include "string_utils.h"
#include <string>
#include <cstdint>
#include <charconv>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
const char* ProcessPrefix(const char* str, bool& hasError)
{
    hasError = false;
    if (*str == '\0') {
        hasError = true;
        return str;
    }
    const char* p = str;
    while (std::isspace(*p)) {
        ++p;
    }
    if (*p == '\0') {
        hasError = true;
        return p;
    }
    if (*p == '+') {
        const char* next = p + 1;
        if (*next == '+') {
            hasError = true;
            return p;
        }
        if (*next < '0' || *next > '9') {
            hasError = true;
            return p;
        }
        p = next;
    }
    return p;
}

template<typename T>
bool StringToInteger(const std::string& str, T& value)
{
    bool hasError = false;
    const char* strPtr = str.data();
    const char* processed = ProcessPrefix(strPtr, hasError);
    if (hasError) {
        return false;
    }
    char firstChar = *processed;
    if (firstChar == '\0') {
        return false;
    }
    if (firstChar != '-' && (firstChar < '0' || firstChar > '9')) {
        return false;
    }
    size_t startPos = processed - strPtr;
    auto [ptr, ec] = std::from_chars(str.data() + startPos, str.data() + str.size(), value);
    return ec == std::errc();
}
} // namespace

bool StringUtil::SafeStoInt32(const std::string& str, int32_t& value)
{
    return StringToInteger(str, value);
}
bool StringUtil::SafeStoInt64(const std::string& str, int64_t& value)
{
    return StringToInteger(str, value);
}
}
}
}
}