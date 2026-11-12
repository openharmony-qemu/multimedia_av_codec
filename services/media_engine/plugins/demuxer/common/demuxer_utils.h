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

#ifndef DEMUXER_UTILS_H
#define DEMUXER_UTILS_H

#include <array>
#include <charconv>
#include <cstdint>
#include <string>
#include "meta/meta.h"
#include "meta/meta_key.h"

namespace OHOS {
namespace Media {
namespace Plugins {
enum ConvertValueType {
    INT32 = 0,
    FLOAT = 1,
    BUFFER = 2,
    INT64 = 3,
};

bool StartWith(const char* name, const char* chars);
bool IsHeifFormat(const char* majorBrand);
void SetToFormatIfConvertSuccess(Meta& format, const TagType& tag, std::string valueStr, ConvertValueType valueType);
constexpr uint32_t MAX_LOCATION_FLOAT_COUNT = 3; // 纬度、经度、海拔最多 3 个
void ScanSignedFloatTokens(const std::string& str,
    std::array<std::string, MAX_LOCATION_FLOAT_COUNT>& tokens, uint32_t& count);

template <typename T>
bool ConvertString(const std::string &str, T &result)
{
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
    return ec == std::errc{} && ptr == str.data() + str.size();
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // DEMUXER_UTILS_H
