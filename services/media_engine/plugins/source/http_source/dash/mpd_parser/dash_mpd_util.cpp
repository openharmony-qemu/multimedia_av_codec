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
#include "common/log.h"
#include "dash_mpd_util.h"
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
    if (str.length() > position + length) {
        return str.replace(position, length, dstSubStr);
    }
    return str;
}

int32_t DashSubstituteTmpltStr(std::string &segTmpltStr, const std::string &segTmpltIdentifier,
                               std::string substitutionStr)
{
    std::string::size_type pos = segTmpltStr.find(segTmpltIdentifier);
    uint32_t identifierLength = segTmpltIdentifier.length();
    while (pos != std::string::npos && (pos + identifierLength) < segTmpltStr.length()) {
        std::string str = segTmpltStr.substr(pos + identifierLength);
        // contain format %0[width]d
        if (str[0] == '%' && str[1] == '0') {
            std::string::size_type posEnd = str.find("$");
            // not find end '$', drop the representation
            if (posEnd == std::string::npos) {
                return -1;
            }

            GetSubstitutionStr(substitutionStr, str);
            segTmpltStr =
                    ReplaceSubStr(segTmpltStr, (uint32_t) pos, (uint32_t) (posEnd + identifierLength + 1),
                                  substitutionStr);
        } else if (str[0] == '$') {
            segTmpltStr = ReplaceSubStr(segTmpltStr, (uint32_t) pos, identifierLength + 1, substitutionStr);
        } else {
            // error format, should drop the representation
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
    if (static_cast<int32_t>(substitutionStr.length()) < formatWidth) {
        for (int32_t i = formatWidth - static_cast<int32_t>(substitutionStr.length()); i > 0; i--) {
            substitutionStr = "0" + substitutionStr;
        }
    }
}

int32_t DashStrToDuration(const std::string &str, uint32_t &duration)
{
    int64_t time = 0;
    int32_t secondTime = 0;
    char *end = nullptr;
    char *start = const_cast<char *>(str.c_str());
    if (str.length() == 0 || *start != 'P') {
        return -1;
    }

    start++;
    do {
        int32_t coefficient = 0;
        double num = strtod(start, &end);

        if (start != end && end) {
            start = end;
        }

        switch (*start) {
            case 'M':
                if (secondTime) {
                    coefficient = M_2_S;
                }
                break;
            case 'Y':
                break;
            case 'D':
                coefficient = D_2_H * H_2_M * M_2_S;
                break;
            case 'T':
                secondTime = 1;
                break;
            case 'H':
                coefficient = H_2_M * M_2_S;
                break;
            case 'S':
                coefficient = 1;
                break;
            default:
                break;
        }

        time += (int64_t)(num * S_2_MS) * coefficient;
        if (*start) {
            start++;
        }
    } while (*start);

    duration = static_cast<uint32_t>(time);

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
        startRange = atoll(firstRange.c_str());

        if (separatePosition < rangeStr.length() - 1) {
            std::string lastRange = rangeStr.substr(separatePosition + 1, rangeStr.length() - separatePosition - 1);
            endRange = atoll(lastRange.c_str());
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

} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS