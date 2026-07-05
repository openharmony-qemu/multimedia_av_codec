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

#include <cstring>
#include <string_view>

#include "http_source_plugin_stream_protocol_sniffer.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

size_t StreamProtocolSniffer::GetMinSniffBufferSize()
{
    return MIN_SNIFF_BUFFER_SIZE;
}

StreamProtocolType StreamProtocolSniffer::SniffStreamProtocolFromData(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < MIN_SNIFF_BUFFER_SIZE) {
        return StreamProtocolType::HTTP;
    }

    if (memcmp(data, HLS_SIGNATURE, HLS_SIGNATURE_LEN) == 0) {
        return StreamProtocolType::HLS;
    }

    if (memcmp(data, DASH_XML_SIGNATURE, DASH_XML_SIGNATURE_LEN) == 0 &&
        std::string_view(reinterpret_cast<const char*>(data), size).find(DASH_MPD_SIGNATURE) !=
            std::string_view::npos) {
        return StreamProtocolType::DASH;
    }

    return StreamProtocolType::HTTP;
}

} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS

extern "C" {
size_t GetMinSniffBufferSize()
{
    return OHOS::Media::Plugins::HttpPlugin::StreamProtocolSniffer::GetMinSniffBufferSize();
}

OHOS::Media::Plugins::HttpPlugin::StreamProtocolType SniffStreamProtocolFromData(const uint8_t* data, size_t size)
{
    return OHOS::Media::Plugins::HttpPlugin::StreamProtocolSniffer::SniffStreamProtocolFromData(data, size);
}
}