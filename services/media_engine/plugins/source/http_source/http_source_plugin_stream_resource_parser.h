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

#ifndef HTTP_SOURCE_PLUGIN_STREAM_RESOURCE_PARSER_H
#define HTTP_SOURCE_PLUGIN_STREAM_RESOURCE_PARSER_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "http_source_plugin_stream_protocol_sniffer.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

struct StreamResourceInfo {
    std::string url;
    bool isSubPlaylist;
    StreamProtocolType protocol;
};

class __attribute__((visibility("default"))) StreamResourceParser {
public:
    static StreamResourceParser* Parse(
        const uint8_t* data, size_t size, StreamProtocolType protocol, const std::string& rootUrl);

    bool HasNext() const;
    StreamResourceInfo Next();

    std::vector<StreamResourceInfo> NextBatch(size_t batchSize);
    virtual std::vector<StreamResourceInfo> GetAll();

    size_t GetTotalCount() const;
    size_t GetProcessedCount() const;

    StreamResourceParser() = default;
    virtual ~StreamResourceParser() = default;

private:

    void ParseHls(const uint8_t* data, size_t size);
    void ParseDash(const uint8_t* data, size_t size);
    void ParseHttp(const uint8_t* data, size_t size);

    std::vector<StreamResourceInfo> resources_;
    size_t currentIndex_ = 0;
    StreamProtocolType protocol_ = StreamProtocolType::HTTP;
    std::string rootUrl_;
};

} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS

extern "C" {
OHOS::Media::Plugins::HttpPlugin::StreamResourceParser* __attribute__((visibility("default")))GetStreamResourceParser(
    const uint8_t* data, size_t size, OHOS::Media::Plugins::HttpPlugin::StreamProtocolType protocol,
    const std::string& rootUrl);
}
#endif