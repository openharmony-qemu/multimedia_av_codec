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
#include <string>
#include <memory>
#include "dash_mpd_parser.h"

using namespace OHOS::Media::Plugins::HttpPlugin;

class FuzzTimeParser : public DashMpdParser {
public:
    // 暴露String2Time方法用于fuzz测试
    time_t FuzzString2Time(const std::string& timeStr) {
        return String2Time(timeStr);
    }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0 || size > 256) { // 时间字符串通常不会很长
        return 0;
    }

    FuzzTimeParser parser;

    // 创建各种可能的时间格式字符串进行测试
    std::vector<std::string> timeFormats;

    // 原始输入
    std::string rawInput(reinterpret_cast<const char*>(data), size);
    timeFormats.push_back(rawInput);

    // 确保null终止的字符串
    std::string nullTerminated(reinterpret_cast<const char*>(data),
        strnlen(reinterpret_cast<const char*>(data), size));
    timeFormats.push_back(nullTerminated);

    // 生成一些变异的时间格式
    if (size >= 19) { // 最小IOS时间格式长度
        std::string isoFormat = "2024-01-01T12:00:00";
        // 用输入数据替换部分字符
        for (size_t i = 0; i < std::min(size, isoFormat.length()); i++) {
            isoFormat[i] = static_cast<char>(data[i]);
        }
        timeFormats.push_back(isoFormat);
    }

    // 测试所有格式
    for (const auto& timeStr : timeFormats) {
        try {
            time_t result = parser.FuzzString2Time(timeStr);
            (void)result; // 避免未使用变量警告
        } catch (...) {
            // 忽略异常
        }
    }

    return 0;
}