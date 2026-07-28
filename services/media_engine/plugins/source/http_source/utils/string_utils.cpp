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
#include <cctype>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr const char* SCHEME_AUTHORITY_SEP = "://";
constexpr size_t SCHEME_AUTHORITY_SEP_LEN = 3;
constexpr const char* NETWORK_PATH_PREFIX = "//";
constexpr size_t NETWORK_PATH_PREFIX_LEN = 2;
constexpr size_t COLON_LEN = 1;
constexpr size_t SLASH_LEN = 1;
constexpr const char* USERINFO_MASK = "***";
constexpr const char* MASKED_PATH_PREFIX_SLASH = "/***/";
constexpr const char* MASKED_PATH_PREFIX = "***/";
constexpr size_t RESERVE_PADDING = 8;

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

// RFC 3986: scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
bool IsValidScheme(const std::string& url, size_t end)
{
    if (end == 0 || !std::isalpha(static_cast<unsigned char>(url[0]))) {
        return false;
    }
    for (size_t i = 1; i < end; ++i) {
        unsigned char c = static_cast<unsigned char>(url[i]);
        if (!std::isalnum(c) && c != '+' && c != '-' && c != '.') {
            return false;
        }
    }
    return true;
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
    size_t hierEnd = url.find_first_of("?#");
    if (hierEnd == std::string::npos) {
        hierEnd = url.size();
    }
    std::string result;
    result.reserve(hierEnd + RESERVE_PADDING);

    size_t colon = url.find(':');
    bool hasScheme = (colon != std::string::npos && colon < hierEnd && IsValidScheme(url, colon));
    bool hasAuthority = false;
    size_t pathBegin = 0;
    if (hasScheme &&
        colon + SCHEME_AUTHORITY_SEP_LEN <= hierEnd &&
        url.compare(colon, SCHEME_AUTHORITY_SEP_LEN, SCHEME_AUTHORITY_SEP) == 0) {
        pathBegin = colon + SCHEME_AUTHORITY_SEP_LEN;
        hasAuthority = true;
    } else if (hasScheme) {
        pathBegin = colon + COLON_LEN;
    } else if (hierEnd >= NETWORK_PATH_PREFIX_LEN &&
        url.compare(0, NETWORK_PATH_PREFIX_LEN, NETWORK_PATH_PREFIX) == 0) {
        pathBegin = NETWORK_PATH_PREFIX_LEN;
        hasAuthority = true;
    }
    result.append(url, 0, pathBegin);

    if (hasAuthority) {
        size_t authEnd = url.find_first_of("/?#", pathBegin);
        if (authEnd == std::string::npos || authEnd > hierEnd) {
            authEnd = hierEnd;
        }
        size_t atPos = url.find('@', pathBegin);
        if (atPos != std::string::npos && atPos < authEnd) {
            result += USERINFO_MASK;
            result.append(url, atPos, authEnd - atPos);
        } else {
            result.append(url, pathBegin, authEnd - pathBegin);
        }
        pathBegin = authEnd;
    }

    size_t pathLen = hierEnd - pathBegin;
    if (pathLen == 0) {
        return result;
    }
    size_t lastSlash = url.rfind('/', hierEnd);
    if (lastSlash == std::string::npos || lastSlash == pathBegin) {
        result.append(url, pathBegin, pathLen);
    } else {
        result += (url[pathBegin] == '/') ? MASKED_PATH_PREFIX_SLASH : MASKED_PATH_PREFIX;
        size_t segStart = lastSlash + SLASH_LEN;
        result.append(url, segStart, hierEnd - segStart);
    }
    return result;
}
}
}
}
}