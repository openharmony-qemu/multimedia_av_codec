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

#ifndef HISTREAMER_BASE64_UTILS_H
#define HISTREAMER_BASE64_UTILS_H

#include <cstdint>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class Base64Utils {
public:
    static bool Base64Decode(const uint8_t *src, uint32_t srcSize, uint8_t *dest, uint32_t *destSize);

private:
    Base64Utils() = delete;
    ~Base64Utils() = delete;
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS

#endif //HISTREAMER_BASE64_UTILS_H
