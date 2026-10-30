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

namespace {
constexpr std::string_view XML_INSTRUCTION = "<?";
constexpr std::string_view XML_INSTRUCTION_END = "?>";
constexpr std::string_view XML_COMMENT = "<!--";
constexpr std::string_view XML_COMMENT_END = "-->";
constexpr std::string_view XML_DOCTYPE = "<!DOCTYPE";
constexpr std::string_view XML_DECLARATION = "<!";
constexpr std::string_view XML_NAME_DELIMITERS = " \t\r\n/>";
constexpr std::string_view DASH_ROOT_NAME = "MPD";
constexpr std::string_view FLV_SIGNATURE = "FLV";
constexpr size_t FLV_HEADER_SIZE = 9;
constexpr size_t FLV_VERSION_INDEX = 3;
constexpr size_t FLV_FLAGS_INDEX = 4;
constexpr size_t FLV_OFFSET_INDEX = 5;
constexpr uint8_t FLV_VERSION = 1;
constexpr uint8_t FLV_AUDIO_VIDEO_FLAGS = 0x05;
constexpr uint32_t BITS_PER_BYTE = 8;
constexpr size_t FLV_OFFSET_BYTES = 4;

bool IsSpace(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

size_t SkipDoctype(std::string_view text, size_t pos)
{
    int depth = 0;
    char quote = 0;
    for (pos += XML_DOCTYPE.size(); pos < text.size(); ++pos) {
        char ch = text[pos];
        if (quote != 0) {
            if (ch == quote) {
                quote = 0;
            }
        } else if (ch == '\'' || ch == '"') {
            quote = ch;
        } else if (ch == '[') {
            ++depth;
        } else if (ch == ']') {
            --depth;
        } else if (ch == '>' && depth == 0) {
            return pos + 1;
        }
    }
    return std::string_view::npos;
}

// Skip an XML declaration/comment/doctype without allocating in the body callback.
size_t SkipXmlPrefix(std::string_view text, size_t pos)
{
    while (pos < text.size()) {
        if (IsSpace(text[pos])) {
            ++pos;
            continue;
        }
        auto rest = text.substr(pos);
        if (rest.substr(0, XML_INSTRUCTION.size()) == XML_INSTRUCTION) {
            auto end = text.find(XML_INSTRUCTION_END, pos + XML_INSTRUCTION.size());
            if (end == std::string_view::npos) {
                return end;
            }
            pos = end + XML_INSTRUCTION_END.size();
        } else if (rest.substr(0, XML_COMMENT.size()) == XML_COMMENT) {
            auto end = text.find(XML_COMMENT_END, pos + XML_COMMENT.size());
            if (end == std::string_view::npos) {
                return end;
            }
            pos = end + XML_COMMENT_END.size();
        } else if (rest.substr(0, XML_DOCTYPE.size()) == XML_DOCTYPE) {
            pos = SkipDoctype(text, pos);
            if (pos == std::string_view::npos) {
                return pos;
            }
        } else {
            break;
        }
    }
    return pos;
}

bool IsFlvHeader(const uint8_t* data)
{
    uint32_t offset = 0;
    for (size_t i = 0; i < FLV_OFFSET_BYTES; ++i) {
        offset = (offset << BITS_PER_BYTE) | data[FLV_OFFSET_INDEX + i];
    }
    return data[FLV_VERSION_INDEX] == FLV_VERSION &&
        (data[FLV_FLAGS_INDEX] & ~FLV_AUDIO_VIDEO_FLAGS) == 0 && offset >= FLV_HEADER_SIZE;
}

ProtocolProbeResult ProbeXml(std::string_view text, bool isEos)
{
    ProtocolProbeResult result;
    constexpr std::string_view bom = "\xEF\xBB\xBF";
    if (text.size() < bom.size() && bom.substr(0, text.size()) == text) {
        return result;
    }
    size_t pos = text.substr(0, bom.size()) == bom ? bom.size() : 0;
    pos = SkipXmlPrefix(text, pos);
    if (pos == std::string_view::npos || pos == text.size()) {
        return result;
    }
    auto root = text.substr(pos);
    if (root[0] == '<') {
        // A complete root name starts after '<', including an optional namespace prefix.
        auto end = root.find_first_of(XML_NAME_DELIMITERS, 1);
        if (end == std::string_view::npos) {
            return result;
        }
        auto name = root.substr(1, end - 1);
        auto colon = name.find(':');
        if (colon != std::string_view::npos) {
            name.remove_prefix(colon + 1);
        }
        if (name == DASH_ROOT_NAME) {
            return {StreamProtocolType::DASH, false, false};
        }
        // Partial processing instructions, comments and declarations need more bytes.
        if (root.size() < XML_DOCTYPE.size() &&
            (root.substr(0, XML_DECLARATION.size()) == XML_DECLARATION ||
            root.substr(0, XML_INSTRUCTION.size()) == XML_INSTRUCTION)) {
            return result;
        }
    }
    result.needMore = !isEos && text.size() < MIN_SNIFF_BUFFER_SIZE;
    return result;
}
} // namespace

ProtocolProbeResult StreamProtocolSniffer::Probe(const uint8_t* data, size_t size, bool isEos)
{
    ProtocolProbeResult result;
    if (data == nullptr || size == 0) {
        return result;
    }
    std::string_view text(reinterpret_cast<const char*>(data), size);
    constexpr std::string_view hls = "#EXTM3U";
    if (text.substr(0, hls.size()) == hls) {
        return {StreamProtocolType::HLS, false, false};
    }
    if (size < hls.size() && hls.substr(0, size) == text) {
        return result;
    }
    if (text.substr(0, FLV_SIGNATURE.size()) == FLV_SIGNATURE) {
        if (size < FLV_HEADER_SIZE) {
            return result;
        }
        if (IsFlvHeader(data)) {
            return {StreamProtocolType::HTTP, true, false};
        }
    }
    return ProbeXml(text, isEos);
}

StreamProtocolType StreamProtocolSniffer::SniffStreamProtocolFromData(const uint8_t* data, size_t size)
{
    return Probe(data, size, true).type;
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
