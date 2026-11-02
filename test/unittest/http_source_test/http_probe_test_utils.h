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
#ifndef HTTP_PROBE_TEST_UTILS_H
#define HTTP_PROBE_TEST_UTILS_H
#include <sstream>
#include "http_source_plugin.h"
#include "sniffer_test_support.h"

namespace OHOS::Media::Plugins::HttpPlugin::SnifferTest {
inline std::string PatternBytes(size_t length)
{
    std::string bytes(length, '\0');
    uint32_t state = 0x12345678; // xorshift32 seed; shifts 13/17/5 define the deterministic test payload.
    for (auto& byte : bytes) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        byte = static_cast<char>(state & UINT8_MAX);
    }
    return bytes;
}

inline Status ReadHttpToEos(const std::shared_ptr<HttpSourcePlugin>& plugin, std::string& collected)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(WAIT_MS);
    Status status = Status::ERROR_AGAIN;
    while (std::chrono::steady_clock::now() < deadline) {
        std::shared_ptr<Buffer> buffer;
        status = plugin->Read(buffer, collected.size(), READ_SIZE);
        if (buffer != nullptr && buffer->GetMemory() != nullptr) {
            auto memory = buffer->GetMemory();
            if (memory->GetSize() > 0) {
                collected.append(reinterpret_cast<const char*>(memory->GetReadOnlyData()), memory->GetSize());
            }
        }
        if (status != Status::OK && status != Status::ERROR_AGAIN) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_MS));
    }
    return status;
}

inline std::vector<std::string> RequestRanges(const std::vector<std::string>& requests)
{
    std::vector<std::string> ranges;
    for (const auto& request : requests) {
        std::istringstream lines(request);
        std::string line;
        std::string range;
        while (std::getline(lines, line)) {
            if (line.find("Range:") == 0 || line.find("range:") == 0) {
                auto begin = line.find("bytes=");
                if (begin != std::string::npos) {
                    range = line.substr(begin, line.find('\r', begin) - begin);
                }
                break;
            }
        }
        ranges.push_back(range); // Empty means this request carried no Range header.
    }
    return ranges;
}
} // namespace OHOS::Media::Plugins::HttpPlugin::SnifferTest
#endif
