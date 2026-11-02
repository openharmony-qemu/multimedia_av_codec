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
#ifndef HISTREAMER_FD_SOURCE_H
#define HISTREAMER_FD_SOURCE_H

#include <algorithm>
#include <cstdint>
#include <limits>
#include <regex>
#include <string>
#include <sys/stat.h>
#include "utils/string_utils.h"
#include "safe_arithmetic.h"

namespace OHOS::Media::Plugins::HttpPlugin {
struct FdSource {
    int32_t fd {-1};
    int64_t offset {0};
    int64_t size {0};
    int64_t fileSize {0};

    static bool Parse(const std::string& uri, FdSource& source)
    {
        constexpr char FD_URI_PATTERN[] = "^fd://([0-9]+)(?:\\?offset=(.*)&size=(.*))?$";
        constexpr size_t FD_GROUP = 1;
        constexpr size_t OFFSET_GROUP = 2;
        constexpr size_t SIZE_GROUP = 3;
        std::smatch match;
        if (!std::regex_match(uri, match, std::regex(FD_URI_PATTERN))) {
            return false;
        }
        int64_t number = 0;
        if (!StringUtil::SafeStoInt64(match[FD_GROUP].str(), number) || number < 0 ||
            number > std::numeric_limits<int32_t>::max()) {
            return false;
        }
        source.fd = static_cast<int32_t>(number);
        struct stat info {};
        if (fstat(source.fd, &info) != 0 || !S_ISREG(info.st_mode) || info.st_size < 0) {
            return false;
        }
        source.fileSize = info.st_size;
        source.offset = 0;
        source.size = source.fileSize;
        if (match[OFFSET_GROUP].matched) {
            int64_t offset = 0;
            int64_t size = 0;
            if (!StringUtil::SafeStoInt64(match[OFFSET_GROUP].str(), offset)) {
                offset = 0;
            }
            if (!StringUtil::SafeStoInt64(match[SIZE_GROUP].str(), size)) {
                size = 0;
            }
            source.offset = offset < 0 ? source.fileSize : std::min(offset, source.fileSize);
            int64_t remaining = 0;
            if (!SafeSub(source.fileSize, source.offset, remaining) || remaining < 0) {
                remaining = 0;
            }
            source.size = std::min(std::max<int64_t>(size, 0), remaining);
        }
        return true;
    }
};
} // namespace OHOS::Media::Plugins::HttpPlugin
#endif
