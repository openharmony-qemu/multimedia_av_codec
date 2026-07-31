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
constexpr const char* SCHEME_SEPARATOR = "://";
constexpr size_t SCHEME_SEPARATOR_LEN = 3;
constexpr char PATH_SEPARATOR = '/';
constexpr char QUERY_SEPARATOR = '?';
constexpr char FRAGMENT_SEPARATOR = '#';
constexpr char USERINFO_SEPARATOR = '@';
constexpr const char* PATH_MASK = "***";
constexpr size_t CHAR_COUNT_ONE = 1;

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

size_t FindPathEnd(const std::string& url, size_t pathPos)
{
    size_t pathEnd = url.size();
    size_t qPos = url.find(QUERY_SEPARATOR, pathPos + CHAR_COUNT_ONE);
    size_t fPos = url.find(FRAGMENT_SEPARATOR, pathPos + CHAR_COUNT_ONE);
    if (qPos != std::string::npos) {
        pathEnd = std::min(pathEnd, qPos);
    }
    if (fPos != std::string::npos) {
        pathEnd = std::min(pathEnd, fPos);
    }
    return pathEnd;
}

size_t FindLastSlashInPath(const std::string& url, size_t pathPos, size_t pathEnd)
{
    for (size_t i = pathEnd; i > pathPos + CHAR_COUNT_ONE; --i) {
        if (url[i - CHAR_COUNT_ONE] == PATH_SEPARATOR) {
            return i - CHAR_COUNT_ONE;
        }
    }
    return pathPos;
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
        return {};
    }
    size_t schemeEnd = url.find(SCHEME_SEPARATOR);
    if (schemeEnd == std::string::npos) {
        return url;
    }
    size_t hostStart = schemeEnd + SCHEME_SEPARATOR_LEN;
    size_t pathPos = url.find(PATH_SEPARATOR, hostStart);
    size_t hostEnd = (pathPos == std::string::npos) ? url.size() : pathPos;
    size_t atPos = url.rfind(USERINFO_SEPARATOR, hostEnd - CHAR_COUNT_ONE);
    if (atPos != std::string::npos && atPos >= hostStart) {
        hostStart = atPos + CHAR_COUNT_ONE;
    }
    // Build prefix: scheme://hostPort
    std::string result;
    result.reserve(url.size());
    result.append(url, 0, schemeEnd)
        .append(SCHEME_SEPARATOR)
        .append(url, hostStart, hostEnd - hostStart);
    // No path or path ends with '/' — just return prefix
    if (pathPos == std::string::npos) {
        return result;
    }
    size_t pathEnd = FindPathEnd(url, pathPos);
    if (pathEnd <= pathPos + CHAR_COUNT_ONE || url[pathEnd - CHAR_COUNT_ONE] == PATH_SEPARATOR) {
        return result;
    }
    // Path has a last segment: prefix/ + [***/] + lastSegment
    size_t lastSlash = FindLastSlashInPath(url, pathPos, pathEnd);
    result += PATH_SEPARATOR;
    if (lastSlash > pathPos) {
        result.append(PATH_MASK).append(CHAR_COUNT_ONE, PATH_SEPARATOR);
    }
    result.append(url, lastSlash + CHAR_COUNT_ONE, pathEnd - lastSlash - CHAR_COUNT_ONE);
    return result;
}
}
}
}
}