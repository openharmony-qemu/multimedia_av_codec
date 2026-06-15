/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#define HST_LOG_TAG "DashMpdUtil"

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <string>
#include "common/log.h"
#include "dash_mpd_util.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

static const int MATRIX_COEFFICIENTS_BT_2020 = 9;
static const int COLOUR_PRIMARIES_BT_2020 = 9;
static const int TRANSFER_CHARACTERISTICS_BT_2020 = 14;

constexpr int32_t MAX_SUBSTITUTION_FORMAT_WIDTH = 32;
 
static const std::string MPD_SCHEME_CICP_MATRIX_COEFFICIENTS = "cicp:MatrixCoefficients";
static const std::string MPD_SCHEME_CICP_COLOUR_PRIMARIES = "cicp:ColourPrimaries";
static const std::string MPD_SCHEME_CICP_TRANSFER_CHARACTERISTICS = "cicp:TransferCharacteristics";

bool DashUrlIsAbsolute(const std::string &url)
{
    if (url.find("http://") == 0 || url.find("https://") == 0) {
        return true;
    }

    return false;
}

void DashAppendBaseUrl(std::string &srcUrl, DashList<std::string> baseUrlList)
{
    if (baseUrlList.size() > 0) {
        std::string baseUrl = baseUrlList.front();
        DashAppendBaseUrl(srcUrl, baseUrl);
    }
}

void DashAppendBaseUrl(std::string &srcUrl, std::string baseUrl)
{
    if (DashUrlIsAbsolute(baseUrl)) {
        // absolute url use baseUrl
        srcUrl = baseUrl;
    } else {
        if (baseUrl.find('/') == 0) {
            BuildSrcUrl(srcUrl, baseUrl);
        } else {
            // relative path
            srcUrl.append(baseUrl);
        }
    }
}

void BuildSrcUrl(std::string &srcUrl, std::string &baseUrl)
{
    if (DashUrlIsAbsolute(srcUrl)) {
        std::string urlSchem;
        size_t urlSchemIndex = srcUrl.find("://");
        if (urlSchemIndex != std::string::npos) {
            size_t urlSchemLength = strlen("://");
            urlSchem = srcUrl.substr(0, urlSchemIndex + urlSchemLength);
            srcUrl = srcUrl.substr(urlSchemIndex + urlSchemLength);
        }

        size_t urlPathIndex = srcUrl.find('/');
        if (urlPathIndex != std::string::npos) {
            srcUrl = srcUrl.substr(0, urlPathIndex);
        }

        srcUrl = urlSchem.append(srcUrl).append(baseUrl);
    } else {
        srcUrl = baseUrl;
    }
}

static std::string &ReplaceSubStr(std::string &str, uint32_t position, uint32_t length, const std::string &dstSubStr)
{
    if (str.length() >= position + length) {
        return str.replace(position, length, dstSubStr);
    }
    return str;
}

int32_t DashSubstituteTmpltStr(std::string &segTmpltStr, const std::string &segTmpltIdentifier,
                               std::string substitutionStr)
{
    std::string::size_type pos = segTmpltStr.find(segTmpltIdentifier);
    uint32_t identifierLength = segTmpltIdentifier.length();
    while (pos != std::string::npos && (pos + identifierLength) <= segTmpltStr.length()) {
        std::string str = segTmpltStr.substr(pos + identifierLength);
        // contain format %0[width]d
        if (str.length() > 1 && str[0] == '%' && str[1] == '0') {
            std::string::size_type posEnd = str.find("$");
            // not find end '$', drop the representation
            if (posEnd == std::string::npos) {
                return -1;
            }

            GetSubstitutionStr(substitutionStr, str);
            segTmpltStr = ReplaceSubStr(segTmpltStr, static_cast<uint32_t>(pos),
                static_cast<uint32_t>(posEnd + identifierLength + 1), substitutionStr);
        } else if (str[0] == '$') {
            segTmpltStr = ReplaceSubStr(segTmpltStr, static_cast<uint32_t>(pos), identifierLength + 1, substitutionStr);
        } else {
            // error format, should drop the representation
            MEDIA_LOG_E("Error format, should drop the representation");
            return -1;
        }

        pos = segTmpltStr.find(segTmpltIdentifier);
    }

    return 0;
}

void GetSubstitutionStr(std::string &substitutionStr, std::string &str)
{
    size_t strLength = strlen("%0");
    str = str.substr(strLength);
    int32_t formatWidth = atoi(str.c_str());
    if (formatWidth <= 0) {
        MEDIA_LOG_W("Invalid format width " PUBLIC_LOG_D32 ", no padding applied", formatWidth);
        return;
    }

    if (formatWidth > MAX_SUBSTITUTION_FORMAT_WIDTH) {
        MEDIA_LOG_W("Format width " PUBLIC_LOG_D32 " exceeds max " PUBLIC_LOG_D32 ", using max value",
            formatWidth, MAX_SUBSTITUTION_FORMAT_WIDTH);
        formatWidth = MAX_SUBSTITUTION_FORMAT_WIDTH;
    }

    int32_t paddingCount = formatWidth - static_cast<int32_t>(substitutionStr.length());
    if (paddingCount > 0) {
        substitutionStr = std::string(paddingCount, '0') + substitutionStr;
    }
}

static int32_t GetDurationCoefficient(char unit, int32_t &secondTime)
{
    switch (unit) {
        case 'M':
            return secondTime ? M_2_S : 0;
        case 'D':
            return D_2_H * H_2_M * M_2_S;
        case 'T':
            secondTime = 1;
            return 0;
        case 'H':
            return H_2_M * M_2_S;
        case 'S':
            return 1;
        default:
            return 0;
    }
}

static void ClampDurationToUint32(int64_t time, uint32_t &duration)
{
    if (time < 0) {
        duration = 0;
    } else if (time > static_cast<int64_t>(std::numeric_limits<uint32_t>::max())) {
        duration = std::numeric_limits<uint32_t>::max();
    } else {
        duration = static_cast<uint32_t>(time);
    }
}

int32_t DashStrToDuration(const std::string &str, uint32_t &duration)
{
    int64_t time = 0;
    int32_t secondTime = 0;
    if (str.length() == 0 || str[0] != 'P') {
        return -1;
    }

    std::string remain = str.substr(1);
    while (!remain.empty()) {
        size_t parsedPos = 0;
        double num = 0.0;
        if (!remain.empty()) {
            char* end = nullptr;
            num = strtod(remain.c_str(), &end);
            parsedPos = static_cast<size_t>(end - remain.c_str());
        }
        if (parsedPos > 0) {
            remain = remain.substr(parsedPos);
        }
        if (remain.empty()) {
            break;
        }
        int32_t coefficient = GetDurationCoefficient(remain[0], secondTime);
        if (coefficient > 0 && num > 0) {
            double scaled = num * static_cast<double>(S_2_MS) * static_cast<double>(coefficient);
            if (scaled > static_cast<double>(std::numeric_limits<int64_t>::max() - time)) {
                duration = std::numeric_limits<uint32_t>::max();
                return 0;
            }
            time += static_cast<int64_t>(scaled);
        }

        remain = remain.substr(1);
    }

    ClampDurationToUint32(time, duration);
    return 0;
}

uint32_t DashGetAttrIndex(const std::string &attrName, const char *const *nodeAttrs, uint32_t attrNums)
{
    uint32_t i;
    for (i = 0; i < attrNums; i++) {
        if (attrName == nodeAttrs[i]) {
            break;
        }
    }

    return i;
}

void DashParseRange(const std::string &rangeStr, int64_t &startRange, int64_t &endRange)
{
    if (rangeStr.length() > 0) {
        std::string::size_type separatePosition = rangeStr.find_first_of('-');
        if (separatePosition == std::string::npos) {
            MEDIA_LOG_W("range is error, no - in " PUBLIC_LOG_S, rangeStr.c_str());
            return;
        }

        if (separatePosition == 0) {
            MEDIA_LOG_W("not support, the - is begin at string " PUBLIC_LOG_S, rangeStr.c_str());
            return;
        }

        std::string firstRange = rangeStr.substr(0, separatePosition);
        int64_t tmpStart = 0;
        if (StringUtil::SafeStoInt64(firstRange, tmpStart) && tmpStart >= 0) {
            startRange = tmpStart;
        }

        if (separatePosition < rangeStr.length() - 1) {
            std::string lastRange = rangeStr.substr(separatePosition + 1, rangeStr.length() - separatePosition - 1);
            int64_t tmpEnd = 0;
            if (StringUtil::SafeStoInt64(lastRange, tmpEnd) && tmpEnd >= 0) {
                endRange = tmpEnd;
            }
        }
        MEDIA_LOG_D("startRange=" PUBLIC_LOG_D64 ", endRange=" PUBLIC_LOG_D64 ", range="
            PUBLIC_LOG_S, startRange, endRange, rangeStr.c_str());
    }
}

bool DashStreamIsHdr(DashList<DashDescriptor*> essentialPropertyList)
{
    int matrixCoefficients = 1;
    int colourPrimaries = 1;
    int transferCharacteristics = 1;
 
    for (auto descriptor : essentialPropertyList) {
        if (descriptor != nullptr && descriptor->value_.length() > 0) {
            if (descriptor->schemeIdUrl_.find(MPD_SCHEME_CICP_MATRIX_COEFFICIENTS) != std::string::npos) {
                matrixCoefficients = atoi(descriptor->value_.c_str());
            } else if (descriptor->schemeIdUrl_.find(MPD_SCHEME_CICP_COLOUR_PRIMARIES) != std::string::npos) {
                colourPrimaries = atoi(descriptor->value_.c_str());
            } else if (descriptor->schemeIdUrl_.find(MPD_SCHEME_CICP_TRANSFER_CHARACTERISTICS) != std::string::npos) {
                transferCharacteristics = atoi(descriptor->value_.c_str());
            }
        }
    }
 
    if (matrixCoefficients >= MATRIX_COEFFICIENTS_BT_2020 &&
        colourPrimaries >= COLOUR_PRIMARIES_BT_2020 &&
        transferCharacteristics >= TRANSFER_CHARACTERISTICS_BT_2020) {
        return true;
    }
 
    return false;
}

bool DashStrToNonNegativeDouble(const std::string &str, double &result)
{
    if (str.empty()) {
        return false;
    }
    size_t pos = 0;
    char* end = nullptr;
    result = strtod(str.c_str(), &end);
    pos = static_cast<size_t>(end - str.c_str());
    if (pos == 0) {
        MEDIA_LOG_E("DashStrToNonNegativeDouble parse failed");
        return false;
    }
    if (pos < str.length()) {
        return false;
    }
    if (result < 0.0) {
        return false;
    }
    return true;
}

} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS