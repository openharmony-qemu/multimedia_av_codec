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
#ifndef HTTP_SOURCE_PLUGIN_STREAM_PROTOCOL_SNIFFER_H
#define HTTP_SOURCE_PLUGIN_STREAM_PROTOCOL_SNIFFER_H

#include <cstddef>
#include <cstdint>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

constexpr size_t MIN_SNIFF_BUFFER_SIZE = 256;

enum class StreamProtocolType : int32_t {
    HTTP = 0,
    HLS = 1,
    DASH = 2,
};

class __attribute__((visibility("default"))) StreamProtocolSniffer {
public:
    static size_t GetMinSniffBufferSize();
    static StreamProtocolType SniffStreamProtocolFromData(const uint8_t* data, size_t size);

private:
    static constexpr const char* HLS_SIGNATURE = "#EXTM3U";
    static constexpr size_t HLS_SIGNATURE_LEN = 7;
    static constexpr const char* DASH_XML_SIGNATURE = "<?xml";
    static constexpr size_t DASH_XML_SIGNATURE_LEN = 5;
    static constexpr const char* DASH_MPD_SIGNATURE = "MPD";
};

} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS

extern "C" {
size_t __attribute__((visibility("default")))GetMinSniffBufferSize();

OHOS::Media::Plugins::HttpPlugin::StreamProtocolType __attribute__((visibility("default")))SniffStreamProtocolFromData(
    const uint8_t* data, size_t size);
}

#endif