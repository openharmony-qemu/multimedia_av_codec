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
#include <cstdlib>
#include <cstring>
#include <memory>
#include "dash_mpd_parser.h"
#include "mpd_parser_fuzzer.h"

using namespace OHOS::Media::Plugins::HttpPlugin;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 10 || size > 1024 * 1024) {  // 限制输入大小，避免过大内存分配
        return 0;
    }

    // 创建解析器实例
    DashMpdParser parser;

    // 确保输入数据以null结尾，模拟真实xml数据
    std::vector<char> mpdData(size + 1);
    memcpy(mpdData.data(), data, size);
    mpdData[size] = '\0';

    // 执行MPD解析
    parser.ParseMPD(mpdData.data(), static_cast<uint32_t>(size));

    // 获取解析结果
    DashMpdInfo *mpdInfo = nullptr;
    parser.GetMPD(mpdInfo);

    // 清理资源
    parser.Clear();
    return 0;
}