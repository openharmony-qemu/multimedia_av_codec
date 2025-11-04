/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cstdint>
#include <cstddef>
#include <vector>
#include <list>
#include <memory>

#include "dash_mpd_def.h"
#include "mpd_parser_def.h"
#include "sidx_box_parser.h"

using namespace OHOS::Media::Plugins::HttpPlugin;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // 源函数参数是 char*，需做一次可变拷贝
    std::vector<char> buf(size ? size : 1);
    for (size_t i = 0; i < size; ++i) {
        buf[i] = static_cast<char>(data[i]);
    }

    // 构造多种 sidxEndOffset，覆盖正负/极值/数据相关
    int64_t derived = 0;
    if (size >= 8) {
        // 直接按本机端序取 8B 做扰动即可；只是驱动算术路径，不依赖稳定值
        std::memcpy(&derived, buf.data(), 8);
    }

    const int64_t candidates[] = {
        0, -1, 1, (int64_t)0x7fffffffffffffffLL, (int64_t)0x8000000000000000LL, derived
    };

    DashList<std::shared_ptr<SubSegmentIndex>> table;
    for (int i = 0; i < (int)(sizeof(candidates) / sizeof(candidates[0])); ++i) {
        (void)SidxBoxParser::ParseSidxBox(
            buf.data(),
            static_cast<uint32_t>(size),
            candidates[i],
            table
        );
        table.clear(); // 重置容器，反复覆盖 push_back 路径
    }
    return 0;
}