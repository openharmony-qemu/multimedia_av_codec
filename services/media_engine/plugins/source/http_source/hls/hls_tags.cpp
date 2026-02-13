/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include <cstring>
#include <sstream>
#include <stack>
#include <utility>
#include "hls_tags.h"
#include <regex>
#include <utility>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr size_t EXT_X_DEFINE_LEN = 14;
constexpr size_t MIN_MATCH_GROUPS = 3;
}
struct {
    const char* name;
    HlsTag type;
} const g_exttagmapping[] = {
    {"ext-x-byterange",              HlsTag::EXTXBYTERANGE},
    {"ext-x-discontinuity",          HlsTag::EXTXDISCONTINUITY},
    {"ext-x-key",                    HlsTag::EXTXKEY},
    {"ext-x-map",                    HlsTag::EXTXMAP},
    {"ext-x-version",                HlsTag::EXTXVERSION},
    {"ext-x-program-date-time",      HlsTag::EXTXPROGRAMDATETIME},
    {"ext-x-targetduration",         HlsTag::EXTXTARGETDURATION},
    {"ext-x-media-sequence",         HlsTag::EXTXMEDIASEQUENCE},
    {"ext-x-discontinuity-sequence", HlsTag::EXTXDISCONTINUITYSEQUENCE},
    {"ext-x-endlist",                HlsTag::EXTXENDLIST},
    {"ext-x-playlist-type",          HlsTag::EXTXPLAYLISTTYPE},
    {"ext-x-i-frames-only",          HlsTag::EXTXIFRAMESONLY},
    {"ext-x-media",                  HlsTag::EXTXMEDIA},
    {"ext-x-start",                  HlsTag::EXTXSTART},
    {"ext-x-stream-inf",             HlsTag::EXTXSTREAMINF},
    {"ext-x-i-frame-stream-inf",     HlsTag::EXTXIFRAMESTREAMINF},
    {"ext-x-session-key",            HlsTag::EXTXSESSIONKEY},
    {"ext-x-skip",                   HlsTag::EXTXSKIP},
    {"extinf",                       HlsTag::EXTINF},
    {"",                             HlsTag::URI}
};

Attribute::Attribute(std::string name, std::string value)
    : name_(std::move(name)), value_(std::move(value))
{
}

uint64_t Attribute::Decimal() const
{
    std::istringstream is(value_);
    is.imbue(std::locale("C"));
    uint64_t ret;
    is >> ret;
    return ret;
}

double Attribute::FloatingPoint() const
{
    std::istringstream is(value_);
    is.imbue(std::locale("C"));
    double ret;
    is >> ret;
    return ret;
}

std::vector<uint8_t> Attribute::HexSequence() const
{
    std::vector<uint8_t> ret;
    if (value_.length() > 2 && (value_.substr(0, 2) == "0X" || value_.substr(0, 2) == "0x")) { // 2
        for (size_t i = 2; i <= (value_.length() - 2); i += 2) { // 2
            unsigned val;
            std::stringstream ss(value_.substr(i, 2));  // 2
            ss.imbue(std::locale("C"));
            ss >> std::hex >> val;
            ret.push_back(val);
        }
    }
    return ret;
}

bool Attribute::SafeStringToInt(const std::string& str, int& result, int base)
{
    if (str.empty()) {
        return false;
    }
    char* endptr;
    errno = 0;
    long num = std::strtol(str.c_str(), &endptr, base);

    if (errno == ERANGE) {
        return false;
    }
    if (*endptr != '\0') {
        return false;
    }

    if (num < INT_MIN || num > INT_MAX) {
        return false;
    }
    result = static_cast<int>(num);
    return true;
}

std::pair<std::size_t, std::size_t> Attribute::GetByteRange() const
{
    std::size_t length = 0;
    std::size_t offset = 0;
    std::string newValue;
    if (value_.find("\"") == std::string::npos) {
        newValue = value_;
    } else {
        newValue = value_.substr(1, value_.size() - 2); // 2
    }
    std::istringstream is(newValue);
    is.imbue(std::locale("C"));
    if (!is.eof()) {
        is >> length;
        if (!is.eof()) {
            char c = static_cast<char>(is.get());
            if (c == '@' && !is.eof()) {
                is >> offset;
            }
        }
    }
    return std::make_pair(offset, length);
}

std::pair<int, int> Attribute::GetResolution() const
{
    int w = 0;
    int h = 0;
    std::istringstream is(value_);
    is.imbue(std::locale("C"));
    if (!is.eof()) {
        is >> w;
        if (!is.eof()) {
            char c = static_cast<char>(is.get());
            if (c == 'x' && !is.eof()) {
                is >> h;
            }
        }
    }
    return std::make_pair(w, h);
}
std::string Attribute::GetName() const
{
    return name_;
}

Attribute Attribute::UnescapeQuotes() const
{
    return {name_, QuotedString()};
}

std::string Attribute::QuotedString() const
{
    if (!value_.empty() && value_.at(0) != '"') {
        return value_;
    }
    if (value_.length() < 2) { // 2
        return "";
    }
    std::istringstream is(value_.substr(1, value_.length() - 2)); // 2
    std::ostringstream os;
    char c;
    while (is.get(c)) {
        if (c == '\\') {
            if (!is.get(c)) {
                break;
            }
        }
        os << c;
    }
    return os.str();
}

HlsTag Tag::GetType() const
{
    return type_;
}

SingleValueTag::SingleValueTag(HlsTag type, const std::string& v)
    : Tag(type), attr_("", v)
{
}

const Attribute& SingleValueTag::GetValue() const
{
    return attr_;
}

AttributesTag::AttributesTag(HlsTag type, const std::string& v) : Tag(type)
{
    AttributesTag::ParseAttributes(v);
}

std::shared_ptr<Attribute> AttributesTag::GetAttributeByName(const char* name) const
{
    auto iter = std::find_if(attributes.begin(), attributes.end(), [&](const std::shared_ptr<Attribute>& attribute) {
        return attribute != nullptr && attribute->GetName() == name;
    });
    if (iter != attributes.end()) {
        return *iter;
    }
    return nullptr;
}

void AttributesTag::AddAttribute(std::shared_ptr<Attribute>& attr)
{
    attributes.push_back(attr);
}

void AttributesTag::ParseAttributes(const std::string& field)
{
    std::istringstream iss(field);
    std::ostringstream oss;
    while (!iss.eof()) {
        std::string attrName = ParseAttributeName(iss, oss);
        oss.str("");
        std::string attrValue = ParseAttributeValue(iss, oss);
        oss.str("");
        if (!attrName.empty())  {
            auto attribute = std::make_shared<Attribute>(attrName, attrValue);
            attributes.push_back(attribute);
        }
    }
}

std::string AttributesTag::ParseAttributeValue(std::istringstream &iss, std::ostringstream &oss)
{
    bool bQuoted = false;
    while (!iss.eof()) {
        char c = static_cast<char>(iss.peek());
        if (c == '\\' && bQuoted) {
            iss.get();
        } else if (c == ',' && !bQuoted) {
            iss.get();
            break;
        } else if (c == '"') {
            bQuoted = !bQuoted;
            if (!bQuoted) {
                oss.put(static_cast<char>(iss.get()));
                break;
            }
        } else if (!bQuoted && (c < '-' || c > 'z'))  { /* out of range */
            iss.get();
            continue;
        }
        if (!iss.eof()) {
            oss.put(static_cast<char>(iss.get()));
        }
    }
    return oss.str();
}

std::string AttributesTag::ParseAttributeName(std::istringstream& iss, std::ostringstream& oss)
{
    while (!iss.eof()) {
        char c = static_cast<char>(iss.peek());
        if ((c >= 'A' && c <= 'Z') || c == '-') {
            oss.put(static_cast<char>(iss.get()));
        } else if (c == '=') {
            iss.get();
            break;
        } else { /* out of range */
            iss.get();
        }
    }
    return oss.str();
}

ValuesListTag::ValuesListTag(HlsTag type, const std::string& v) : AttributesTag(type, v)
{
    ValuesListTag::ParseAttributes(v);
}

void ValuesListTag::ParseAttributes(const std::string& field)
{
    auto pos = field.find(',');
    std::shared_ptr<Attribute> attr;
    if (pos != std::string::npos) {
        attr = std::make_shared<Attribute>("DURATION", field.substr(0, pos));
        if (attr) {
            AddAttribute(attr);
        }
        attr = std::make_shared<Attribute>("TITLE", field.substr(pos));
        if (attr) {
            AddAttribute(attr);
        }
    } else { /* broken EXTINF without mandatory comma */
        attr = std::make_shared<Attribute>("DURATION", field);
        if (attr) {
            AddAttribute(attr);
        }
    }
}

std::shared_ptr<Tag> TagFactory::CreateTagByName(const std::string& name, const std::string& value)
{
    auto size = sizeof(g_exttagmapping) / sizeof(g_exttagmapping[0]);
    for (unsigned int i = 0; i < size; i++) {
        if (name != g_exttagmapping[i].name) {
            continue;
        }
        switch (g_exttagmapping[i].type) {
            case HlsTag::EXTXDISCONTINUITY:
            case HlsTag::EXTXENDLIST:
            case HlsTag::EXTXIFRAMESONLY:
                return std::make_shared<Tag>(g_exttagmapping[i].type);
            case HlsTag::URI:
            case HlsTag::EXTXVERSION:
            case HlsTag::EXTXBYTERANGE:
            case HlsTag::EXTXPROGRAMDATETIME:
            case HlsTag::EXTXTARGETDURATION:
            case HlsTag::EXTXMEDIASEQUENCE:
            case HlsTag::EXTXDISCONTINUITYSEQUENCE:
            case HlsTag::EXTXPLAYLISTTYPE:
                return std::make_shared<SingleValueTag>(g_exttagmapping[i].type, value);
            case HlsTag::EXTINF:
                return std::make_shared<ValuesListTag>(g_exttagmapping[i].type, value);
            case HlsTag::EXTXKEY:
            case HlsTag::EXTXSESSIONKEY:
            case HlsTag::EXTXMAP:
            case HlsTag::EXTXMEDIA:
            case HlsTag::EXTXSTART:
            case HlsTag::EXTXSTREAMINF:
            case HlsTag::EXTXIFRAMESTREAMINF:
            case HlsTag::EXTXSKIP:
                return std::make_shared<AttributesTag>(g_exttagmapping[i].type, value);
            default:
                return nullptr;
        }
    }
    return nullptr;
}

static std::vector<std::string> Split(const std::string& s, const char* delim)
{
    std::vector<std::string> ret;
    if (delim == nullptr) {
        ret.push_back(s);
        return ret;
    }
    size_t delimLen = std::strlen(delim);
    if (delimLen == 0) {
        ret.push_back(s);
        return ret;
    }
    std::string::size_type last = 0;
    auto index = s.find(delim, last);
    while (index != std::string::npos && last < s.size()) {
        if (index > last) {
            ret.push_back(s.substr(last, index - last));
        }
        last = index + delimLen;
        index = s.find(delim, last);
    }
    if (last < s.size()) {
        ret.push_back(s.substr(last));
    } else if (last == s.size()) {
        ret.push_back("");
    }
    return ret;
}

static void ParseTag(std::list<std::shared_ptr<Tag>>& entriesList, std::shared_ptr<Tag>& lastTag, std::string& line)
{
    if (line.find("#EXT") != std::string::npos) { // tag
        line = line.substr(1);
        std::string key;
        std::string value;
        auto keyValue = Split(line, ":");
        key = keyValue[0];
        if (keyValue.size() >= 2) { // 2
            value = line.substr(key.length() + 1);
        }
        if (!key.empty()) {
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            auto tag = TagFactory::CreateTagByName(key, value);
            if (tag) {
                entriesList.push_back(tag);
            }
            lastTag = tag;
        }
    }
}

static void ParseURI(std::list<std::shared_ptr<Tag>>& entriesList,
                     std::shared_ptr<Tag>& lastTag, const std::string& line)
{
    if (lastTag && lastTag->GetType() == HlsTag::EXTXSTREAMINF) {
        auto streaminftag = std::static_pointer_cast<AttributesTag>(lastTag);
        /* master playlist uri, merge as attribute */
        auto uriAttr = std::make_shared<Attribute>("URI", line);
        if (uriAttr) {
            streaminftag->AddAttribute(uriAttr);
        }
    } else {  /* playlist tag, will take modifiers */
        auto tag = TagFactory::CreateTagByName("", line);
        if (tag) {
            entriesList.push_back(tag);
        }
    }
    lastTag = nullptr;
}

static bool ContainsNonAscii(const std::string& str)
{
    return std::any_of(str.begin(), str.end(),
        [](unsigned char c) {
            return c > 127;  // ASCII: 0-127
        });
}

static std::pair<std::string, std::string> ParseDefine(const std::string& s,
    const std::unordered_map<std::string, std::string>& tagMasterMap,
    const std::unordered_map<std::string, std::string>& tagUriMap)
{
    if (s.length() <= EXT_X_DEFINE_LEN) {
        return std::make_pair("", "");
    }
    if (ContainsNonAscii(s)) {
        return std::make_pair("", "");
    }
    std::string content = s.substr(EXT_X_DEFINE_LEN);
    std::regex combinedRegex(
        R"(NAME\s*=\s*\"([a-zA-Z0-9-_]+)\"\s*,\s*VALUE\s*=\s*\"([^\"]*)\")"
        "|"
        R"(IMPORT\s*=\s*\"([a-zA-Z0-9-_]+)\")"
        "|"
        R"(QUERYPARAM\s*=\s*\"([a-zA-Z0-9-_]+)\")"
    );
    std::smatch match;
    if (!std::regex_match(content, match, combinedRegex)) {
        return std::make_pair("", "");
    }
    if (match[1].matched && match[2].matched) {
        return std::make_pair(match[1].str(), match[2].str());
    }
    if (match[3].matched) {
        std::string importName = match[3].str();
        auto it = tagMasterMap.find(importName);
        if (it != tagMasterMap.end()) {
            return std::make_pair(importName, it->second);
        }
    }
    if (match[4].matched) {
        std::string paramName = match[4].str();
        auto it = tagUriMap.find(paramName);
        if (it != tagUriMap.end()) {
            return std::make_pair(paramName, it->second);
        }
    }
    return std::make_pair("", "");
}

static std::string ReplacePlaceholders(const std::string& s,
    const std::unordered_map<std::string, std::string>& tagDefineMap)
{
    if (s.find("{$") == std::string::npos) {
        return s;
    }
    if (ContainsNonAscii(s)) {
        return "";
    }
    const std::regex pattern(R"(\{\$([a-zA-Z0-9_-]+)\})");
    std::smatch match;
    if (!std::regex_search(s, match, pattern)) {
        return s;
    }
    if (tagDefineMap.empty()) {
        return "";
    }
    std::string result;
    std::string::const_iterator searchStart = s.cbegin();
    while (std::regex_search(searchStart, s.cend(), match, pattern)) {
        result.append(searchStart, match[0].first);
        std::string placeholderName = match[1].str();
        auto it = tagDefineMap.find(placeholderName);
        if (it == tagDefineMap.end()) {
            return "";
        }
        result.append(it->second);
        searchStart = match[0].second;
    }
    result.append(searchStart, s.cend());
    return result;
}

static bool IsHexValid(const std::string& hex)
{
    if (hex.empty()) {
        return true;
    }
    auto it = std::find_if(hex.begin(), hex.end(),
        [](char c) {return !std::isxdigit(static_cast<unsigned char>(c));});
    return it == hex.end();
}

static void UriInsert(std::string& result, std::string& hex, int base)
{
    int resultTmp = 0;
    bool ret = Attribute::SafeStringToInt(hex, resultTmp, base);
    if (ret) {
        result += static_cast<char>(resultTmp);
    }
}

static std::string UriDecode(const std::string& uri)
{
    std::string result;
    result.reserve(uri.size());
    uint64_t i = 0;
    while (i < uri.size()) {
        if (uri[i] == '%' && i + 2 <= uri.size()) { // 2:“%20”
            std::string hex = uri.substr(i + 1, 2); // 2
            if (IsHexValid(hex)) {
                UriInsert(result, hex, 16);
                i += 3; // 3:“%20”
            } else {
                result += uri.substr(i, 3); // 3
                i += 3; // 3
            }
        } else if (uri[i] == '+') {
            result += ' ';
            i++;
        } else {
            result += uri[i];
            i++;
        }
    }
    return result;
}

static std::unordered_map<std::string, std::string> ExtractPairs(const std::string& decodedQuery)
{
    std::unordered_map<std::string, std::string> params;
    if (decodedQuery.empty()) {
        return params;
    }
    std::size_t start = 0;
    std::regex paramRegex("^([^&=]+)=([^&]*)$");
    while (start < decodedQuery.length()) {
        std::size_t end = decodedQuery.find('&', start);
        std::string paramPair = (end == std::string::npos) ?
            decodedQuery.substr(start) : decodedQuery.substr(start, end - start);
        start = (end == std::string::npos) ? decodedQuery.length() : end + 1;
        if (ContainsNonAscii(paramPair)) {
            continue;
        }
        std::smatch match;
        if (!std::regex_match(paramPair, match, paramRegex) || match.size() < MIN_MATCH_GROUPS) {
            continue;
        }
        const std::string& key = match[1].str();
        if (params.find(key) == params.end()) {
            params[key] = match[2].str();
        }
    }
    return params;
}

std::unordered_map<std::string, std::string> ParseUriQuery(const std::string& uri)
{
    std::unordered_map<std::string, std::string> params;
    uint64_t queryStart = uri.find('?');
    if (queryStart == std::string::npos) {
        return params;
    }
    std::string queryString = uri.substr(queryStart + 1);
    if (ContainsNonAscii(queryString)) {
        return params;
    }
    if (queryString.find('%') != std::string::npos || queryString.find('+') != std::string::npos) {
        queryString = UriDecode(queryString);
    }
    params = ExtractPairs(queryString);
    return params;
}

std::pair<std::list<std::shared_ptr<Tag>>, std::unordered_map<std::string, std::string>>ParseEntries(
    const std::string& s, const std::unordered_map<std::string, std::string>& tagMasterMap,
    const std::unordered_map<std::string, std::string>& tagUriMap)
{
    std::unordered_map<std::string, std::string> tagDefineMap;
    std::list<std::shared_ptr<Tag>> list;
    std::shared_ptr<Tag> lastTag = nullptr;
    auto lines = Split(s, "\r\n");
    if (lines.size() == 1) {  // 1
        lines = Split(s, "\n");
    } else {
        std::vector<std::string> newLines;
        for (const auto& line : lines) {
            std::vector<std::string> msplits = Split(line, "\n");
            newLines.insert(newLines.end(), msplits.begin(), msplits.end());
        }
        lines = newLines;
    }
    for (auto line : lines) {
        if (line.find("#EXT-X-DEFINE:") != std::string::npos) {
            auto ret = ParseDefine(line, tagMasterMap, tagUriMap);
            if (!ret.first.empty()) {
                tagDefineMap.insert(ret);
            }
        } else {
            auto newLine = ReplacePlaceholders(line, tagDefineMap);
            if (!newLine.empty() && newLine[0] == '#') {
                ParseTag(list, lastTag, newLine);
            } else if (!newLine.empty()) {
                /* URI */
                ParseURI(list, lastTag, newLine);
            } else {
                // drop
                lastTag = nullptr;
            }
        }
    }
    return {list, tagDefineMap};
}
}
}
}
}