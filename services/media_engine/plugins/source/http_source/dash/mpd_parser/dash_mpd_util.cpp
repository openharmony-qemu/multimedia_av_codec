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

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <string>
#include "common/log.h"
#include "dash_mpd_util.h"
#include "utils/safe_arithmetic.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

constexpr int32_t MATRIX_COEFFICIENTS_BT_2020 = 9;
constexpr int32_t COLOUR_PRIMARIES_BT_2020 = 9;
constexpr int32_t TRANSFER_CHARACTERISTICS_BT_2020 = 14;

constexpr int32_t MAX_SUBSTITUTION_FORMAT_WIDTH = 32;
// 模板替换防护：限制替换后字符串最大长度与循环次数，防止恶意 MPD 触发内存/CPU 耗尽
constexpr size_t MAX_SUBSTITUTED_TMPLT_LEN = 10 * 1024 * 1024; // 10MB
constexpr uint32_t MAX_SUBSTITUTION_LOOP_COUNT = 100000;
 
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
    // 自引用检测：substitutionStr 含标识符会导致替换后字符串仍含标识符，形成无限递归替换
    if (substitutionStr.find(segTmpltIdentifier) != std::string::npos) {
        MEDIA_LOG_E("Substitution string contains identifier, drop the representation");
        return -1;
    }

    std::string::size_type pos = segTmpltStr.find(segTmpltIdentifier);
    uint32_t identifierLength = segTmpltIdentifier.length();
    uint32_t loopCount = 0;
    while (pos != std::string::npos && (pos + identifierLength) <= segTmpltStr.length()) {
        // 循环次数上限：防止模板中塞入海量标识符长时间占用 CPU/内存
        if (++loopCount > MAX_SUBSTITUTION_LOOP_COUNT) {
            MEDIA_LOG_E("Substitution loop count exceeds max " PUBLIC_LOG_U32, MAX_SUBSTITUTION_LOOP_COUNT);
            return -1;
        }
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

        // 结果长度上限：防止"长 id × 多标识符"导致的字符串膨胀
        if (segTmpltStr.length() > MAX_SUBSTITUTED_TMPLT_LEN) {
            MEDIA_LOG_E("Substituted segTmpltStr length " PUBLIC_LOG_ZU " exceeds max " PUBLIC_LOG_ZU,
                segTmpltStr.length(), MAX_SUBSTITUTED_TMPLT_LEN);
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
    int32_t formatWidth = 0;
    int64_t tmpWidth = 0;
    StringUtil::SafeStoInt64(str, tmpWidth);
    if (!SafeIntCast(tmpWidth, formatWidth)) {
        formatWidth = 0;
    }
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
        case 'W':
            return D_2_S * 7; // 7 days per week
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

void ClampDurationToUint32(uint64_t time, uint32_t &duration)
{
    if (time > std::numeric_limits<uint32_t>::max()) {
        duration = std::numeric_limits<uint32_t>::max();
    } else {
        duration = static_cast<uint32_t>(time);
    }
}

int32_t DashStrToDuration(const std::string &str, uint64_t &duration)
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
            if (!std::isfinite(num)) {
                num = 0.0;
            }
        }
        if (parsedPos > 0) {
            remain = remain.substr(parsedPos);
        }
        if (remain.empty()) {
            break;
        }
        int32_t coefficient = GetDurationCoefficient(remain[0], secondTime);
        if (coefficient > 0 && num > 0) {
            double scaled = num * static_cast<double>(S_2_NS) * static_cast<double>(coefficient);
            int64_t scaledInt = 0;
            if (!SafeDoubleToInt64(scaled, scaledInt)) {
                MEDIA_LOG_W("DashStrToDuration scaled value overflow, reject");
                return -1;
            }
            int64_t newTime = 0;
            if (!SafeAdd(time, scaledInt, newTime)) {
                MEDIA_LOG_W("DashStrToDuration accumulation overflow, reject");
                return -1;
            }
            time = newTime;
        }

        remain = remain.substr(1);
    }

    if (time < 0) {
        MEDIA_LOG_W("DashStrToDuration invalid duration, time=" PUBLIC_LOG_D64, time);
        return -1;
    }
    duration = static_cast<uint64_t>(time);
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
        if (!StringUtil::SafeStoInt64(firstRange, tmpStart) || tmpStart < 0) {
            MEDIA_LOG_W("invalid startRange in " PUBLIC_LOG_S, rangeStr.c_str());
            return;
        }

        int64_t tmpEnd = 0;
        if (separatePosition < rangeStr.length() - 1) {
            std::string lastRange = rangeStr.substr(separatePosition + 1, rangeStr.length() - separatePosition - 1);
            if (!StringUtil::SafeStoInt64(lastRange, tmpEnd) || tmpEnd < 0) {
                MEDIA_LOG_W("invalid endRange in " PUBLIC_LOG_S, rangeStr.c_str());
                return;
            }
        }

        if (tmpEnd < tmpStart) {
            MEDIA_LOG_W("endRange < startRange in " PUBLIC_LOG_S, rangeStr.c_str());
            return;
        }

        startRange = tmpStart;
        endRange = tmpEnd;
        MEDIA_LOG_D("startRange=" PUBLIC_LOG_D64 ", endRange=" PUBLIC_LOG_D64 ", range="
            PUBLIC_LOG_S, startRange, endRange, rangeStr.c_str());
    }
}

static void ParseCicpValue(DashDescriptor* descriptor, int32_t &matrixCoefficients,
    int32_t &colourPrimaries, int32_t &transferCharacteristics)
{
    if (descriptor == nullptr || descriptor->value_.length() == 0) {
        return;
    }
    int64_t tmp = 0;
    if (descriptor->schemeIdUrl_.find(MPD_SCHEME_CICP_MATRIX_COEFFICIENTS) != std::string::npos) {
        if (StringUtil::SafeStoInt64(descriptor->value_, tmp) && tmp >= 0 &&
            tmp <= std::numeric_limits<int32_t>::max()) {
            matrixCoefficients = static_cast<int32_t>(tmp);
        }
    } else if (descriptor->schemeIdUrl_.find(MPD_SCHEME_CICP_COLOUR_PRIMARIES) != std::string::npos) {
        if (StringUtil::SafeStoInt64(descriptor->value_, tmp) && tmp >= 0 &&
            tmp <= std::numeric_limits<int32_t>::max()) {
            colourPrimaries = static_cast<int32_t>(tmp);
        }
    } else if (descriptor->schemeIdUrl_.find(MPD_SCHEME_CICP_TRANSFER_CHARACTERISTICS) != std::string::npos) {
        if (StringUtil::SafeStoInt64(descriptor->value_, tmp) && tmp >= 0 &&
            tmp <= std::numeric_limits<int32_t>::max()) {
            transferCharacteristics = static_cast<int32_t>(tmp);
        }
    }
}

bool DashStreamIsHdr(DashList<DashDescriptor*> essentialPropertyList)
{
    int32_t matrixCoefficients = 1;
    int32_t colourPrimaries = 1;
    int32_t transferCharacteristics = 1;
 
    for (auto descriptor : essentialPropertyList) {
        ParseCicpValue(descriptor, matrixCoefficients, colourPrimaries, transferCharacteristics);
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
    if (!std::isfinite(result) || result < 0.0) {
        return false;
    }
    return true;
}

} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS