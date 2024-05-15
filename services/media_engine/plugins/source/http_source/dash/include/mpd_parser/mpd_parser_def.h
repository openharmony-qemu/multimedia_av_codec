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

#ifndef MPD_PARSER_DEF_H
#define MPD_PARSER_DEF_H

#include <list>
#include <string>
#include <vector>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
template <typename T, typename Alloc = std::allocator<T>>
using DashVector = std::vector<T, Alloc>;

template <typename T, typename Alloc = std::allocator<T>>
using DashList = std::list<T, Alloc>;

// xml base return value definition
enum class XmlBaseRtnValue { XML_BASE_ERR = -1, XML_BASE_OK = 0 };

constexpr const char *MPD_LABEL_PSSH = "pssh";
constexpr const char *MPD_LABEL_DEFAULT_KID = "default_KID";

struct NodeAttr {
    std::string attr_;
    std::string val_;
};

/**
 * synchronize client time with server time
 */
struct TimeBase {
    time_t server_;
    time_t client_;
};

class SubSegmentIndex {
public:
    SubSegmentIndex() = default;
    ~SubSegmentIndex() = default;

public:
    bool startsSap_{false};
    int32_t sapType_{0};
    int32_t referenceType_{0};
    int32_t referencedSize_{0};
    uint32_t duration_{0};
    uint32_t timeScale_{1};
    uint32_t sapDeltaTime_{0};
    int64_t startPos_{0};
    int64_t endPos_{0};
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // MPD_PARSER_DEF_H
