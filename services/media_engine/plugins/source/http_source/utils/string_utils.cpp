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
#include <cmath>

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

bool StringUtil::SafeStoDouble(const std::string& str, double& value)
{
    if (str.empty()) {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    double result = strtod(str.c_str(), &end);
    if (end == str.c_str() || end != str.c_str() + str.size() || errno == ERANGE || !std::isfinite(result)) {
        return false;
    }
    value = result;
    return true;
}

std::string StringUtil::MaskUrl(const std::string& url)
{
    if (url.empty()) {
        return url;
    }
    // Split URL into [scheme] + [authority] + [path] + [query/fragment]
    // e.g. "https://" + "user:pass@example.com:8080" + "/a/b/file.txt" + "?q=1"
    size_t schemeEnd = url.find("://"); // length of "://" is 3
    size_t hostStart = (schemeEnd != std::string::npos) ? schemeEnd + 3 : 0;
    size_t pathStart = url.find_first_of("/?#", hostStart);
    // scheme + masked authority
    std::string result = (schemeEnd != std::string::npos) ? url.substr(0, schemeEnd + 3) : "";
    std::string authority = (pathStart != std::string::npos)
        ? url.substr(hostStart, pathStart - hostStart) : url.substr(hostStart);
    // mask userinfo: use find() to locate the first '@' per URL spec (RFC3986)
    size_t atPos = authority.find('@');
    if (atPos != std::string::npos) {
        result += "***@";
        authority = authority.substr(atPos + 1);
    }
    // host + port
    result += authority;
    if (pathStart == std::string::npos || url[pathStart] != '/') {
        return result;
    }
    // path: mask directory with ***, keep filename; ?# params dropped
    size_t queryStart = url.find_first_of("?#", pathStart);
    std::string path = (queryStart != std::string::npos)
        ? url.substr(pathStart, queryStart - pathStart) : url.substr(pathStart);
    size_t lastSlash = path.rfind('/');
    if (path == "/") {
        result += "/";
    } else {
        result += "/***" + path.substr(lastSlash); // keep filename after last /
    }
    return result;
}
}
}
}
}