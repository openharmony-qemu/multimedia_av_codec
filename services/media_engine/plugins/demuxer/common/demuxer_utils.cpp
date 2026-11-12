/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "DemuxerUtils"

#include <cstdint>
#include <cstring>
#include "avcodec_log.h"
#include "common/log.h"
#include "demuxer_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "DemuxerUtils"};
}

namespace OHOS {
namespace Media {
namespace Plugins {

const int32_t INVALID_VAL = -1;
const int32_t POS_1 = 1;
const int32_t VALUE_2 = 2;
const int32_t VALUE_4 = 4;
const int32_t VALUE_10 = 10;

const char* g_heifBrands[] = {"heic", "avif", "heif", "mif1", "msf1", "miaf", "heix", "heim", "heis"};

bool StartWith(const char* name, const char* chars)
{
    if (name == nullptr || chars == nullptr) {
        return false;
    }
    return !strncmp(name, chars, strlen(chars));
}

bool IsHeifFormat(const char* majorBrand)
{
    if (majorBrand == nullptr) {
        return false;
    }
    for (const auto& brand : g_heifBrands) {
        if (StartWith(majorBrand, brand)) {
            return true;
        }
    }
    return false;
}

void ScanSignedFloatTokens(const std::string& str,
    std::array<std::string, MAX_LOCATION_FLOAT_COUNT>& tokens, uint32_t& count)
{
    count = 0;
    size_t i = 0;
    while (i < str.size() && count < tokens.size()) {
        if (str[i] != '+' && str[i] != '-') {
            ++i;
            continue;
        }
        size_t intStart = i + 1;
        size_t j = intStart;
        while (j < str.size() && str[j] >= '0' && str[j] <= '9') {
            ++j;
        }
        if (j == intStart || j >= str.size() || str[j] != '.') {
            ++i;
            continue;
        }
        size_t fracStart = j + 1;
        size_t k = fracStart;
        while (k < str.size() && str[k] >= '0' && str[k] <= '9') {
            ++k;
        }
        if (k == fracStart) {
            ++i;
            continue;
        }
        tokens[count++] = str.substr(i, k - i);
        i = k;
    }
}

void SetLocationMeta(Meta& format, const std::array<std::string, MAX_LOCATION_FLOAT_COUNT>& tokens,
    uint32_t count)
{
    constexpr uint32_t latitudeIndex = 0;
    constexpr uint32_t longitudeIndex = 1;
    constexpr uint32_t altitudeIndex = 2;
    constexpr uint32_t minLocationCount = 2;
    // Callers guarantee count >= minLocationCount before calling; count is capped by
    // tokens.size(), so tokens[altitudeIndex] is only read when count exceeds the base pair.
    SetToFormatIfConvertSuccess(format, Tag::MEDIA_LATITUDE, tokens[latitudeIndex], ConvertValueType::FLOAT);
    SetToFormatIfConvertSuccess(format, Tag::MEDIA_LONGITUDE, tokens[longitudeIndex], ConvertValueType::FLOAT);
    if (count > minLocationCount) {
        SetToFormatIfConvertSuccess(format, Tag::MEDIA_ALTITUDE, tokens[altitudeIndex], ConvertValueType::FLOAT);
    }
}

bool StringConverterFloat(const std::string &str, float &result)
{
    char *end = nullptr;
    errno = 0;
    result = std::strtof(str.c_str(), &end);
    return end != str.c_str() && *end == '\0' && errno == 0;
}

static int8_t HexCharToValue(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + VALUE_10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + VALUE_10;
    }
    return INVALID_VAL;
}

static bool ConvertHexStrToBuffer(const std::string hexStr, std::vector<uint8_t> &buffer)
{
    if (hexStr.empty() || hexStr.size() % VALUE_2 != 0) {
        return false;
    }
    size_t len = hexStr.size() / VALUE_2;
    buffer.resize(len);
    for (size_t i = 0; i < len; i++) {
        int8_t high = HexCharToValue(hexStr[i * VALUE_2]);
        int8_t low = HexCharToValue(hexStr[i * VALUE_2 + POS_1]);
        FALSE_RETURN_V_MSG_E(high >= 0 && low >= 0, false,
            "invalid hexChar" PUBLIC_LOG_U8 " " PUBLIC_LOG_U8, hexStr[i * VALUE_2], hexStr[i * VALUE_2 + POS_1]);
        buffer[i] = (static_cast<uint8_t>(high) << VALUE_4) | static_cast<uint8_t>(low);
    }
    return true;
}

void SetToFormatIfConvertSuccess(Meta& format, const TagType& tag, std::string valueStr, ConvertValueType valueType)
{
    bool convertSuccess = false;
    switch (valueType) {
        case ConvertValueType::INT32: {
            int32_t int32Value = -1;
            if (ConvertString(valueStr, int32Value)) {
                format.SetData<int32_t>(tag, int32Value);
                convertSuccess = true;
            }
            break;
        }
        case ConvertValueType::FLOAT: {
            float floatValue = -1;
            if (StringConverterFloat(valueStr, floatValue)) {
                format.SetData<float>(tag, floatValue);
                convertSuccess = true;
            }
            break;
        }
        case ConvertValueType::BUFFER: {
            std::vector<uint8_t> buffer;
            if (ConvertHexStrToBuffer(valueStr, buffer)) {
                format.SetData(tag, buffer);
                convertSuccess = true;
            }
            break;
        }
        case ConvertValueType::INT64: {
            int64_t int64Value = -1;
            if (ConvertString(valueStr, int64Value)) {
                format.SetData<int64_t>(tag, int64Value);
                convertSuccess = true;
            }
            break;
        }
        default:
            MEDIA_LOG_W("Unsupport value type " PUBLIC_LOG_D32, static_cast<int32_t>(valueType));
            break;
    }
    if (!convertSuccess) {
        MEDIA_LOG_W("Parse failed key[" PUBLIC_LOG_S "] value[" PUBLIC_LOG_S "] type[" PUBLIC_LOG_D32 "]",
            std::string(tag).c_str(), valueStr.c_str(), static_cast<int32_t>(valueType));
    }
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS
