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

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
bool StringUtil::SafeStoInt(const std::string& str, int& value) {
    return StringToInteger(str, value);
}
bool StringUtil::SafeStoLong(const std::string& str, long& value) {
    return StringToInteger(str, value);
}
bool StringUtil::SafeStoLongLong(const std::string& str, long long& value) {
    return StringToInteger(str, value);
}
bool StringUtil::SafeStoInt32(const std::string& str, int32_t& value) {
    return StringToInteger(str, value);
}
bool StringUtil::SafeStoUInt32(const std::string& str, uint32_t& value) {
    return StringToInteger(str, value);
}
bool StringUtil::SafeStoInt64(const std::string& str, int64_t& value) {
    return StringToInteger(str, value);
}
bool StringUtil::SafeStoUInt64(const std::string& str, uint64_t& value) {
    return StringToInteger(str, value);
}
bool StringUtil::SafeStoSizeT(const std::string& str, size_t& value) {
    return StringToInteger(str, value);
}
}
}
}
}