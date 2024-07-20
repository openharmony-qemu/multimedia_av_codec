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

#include "xml/xml_parser.h"
#include "common/log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
int32_t XmlParser::ParseFromBuffer(const char *buf, int32_t length)
{
    if (buf == nullptr || length == 0) {
        MEDIA_LOG_E("ParseFromBuffer buffer is empty, length: " PUBLIC_LOG_D32, length);
        return -1;
    }

    xmlInitParser();
    xmlDocPtr doc = xmlParseMemory(buf, length);
    if (doc == nullptr) {
        MEDIA_LOG_E("ParseFromBuffer error.");
        return -1;
    }

    xmlDocPtr_ = doc;
    return 0;
}

int32_t XmlParser::ParseFromString(const std::string &xmlStr)
{
    if (xmlStr.empty()) {
        MEDIA_LOG_E("ParseFromString buffer is empty.");
        return -1;
    }

    const xmlChar *docData = reinterpret_cast<const unsigned char *>(xmlStr.c_str());
    if (docData == nullptr) {
        MEDIA_LOG_E("ParseFromString xmlStr is error.");
        return -1;
    }

    xmlInitParser();
    xmlDocPtr doc = xmlParseDoc(docData);
    if (doc == nullptr) {
        MEDIA_LOG_E("ParseFromString error.");
        return -1;
    }

    xmlDocPtr_ = doc;
    return 0;
}

int32_t XmlParser::ParseFromFile(const std::string &filePath)
{
    if (filePath.empty()) {
        MEDIA_LOG_E("ParseFromString filePath is empty.");
        return -1;
    }

    xmlInitParser();
    xmlDocPtr doc = xmlParseFile(filePath.c_str());
    if (doc == nullptr) {
        MEDIA_LOG_E("ParseFromFile error.");
        return -1;
    }

    xmlDocPtr_ = doc;
    return 0;
}

std::shared_ptr<XmlElement> XmlParser::GetRootElement()
{
    if (xmlDocPtr_ != nullptr) {
        xmlNodePtr rootNode = xmlDocGetRootElement(xmlDocPtr_);
        if (rootNode != nullptr) {
            return std::make_shared<XmlElement>(rootNode);
        }
    }
    return nullptr;
}

int32_t XmlParser::GetAttribute(std::shared_ptr<XmlElement> element, const std::string attrName, std::string &value)
{
    if (element == nullptr) {
        return -1;
    }

    value = element->GetAttribute(attrName);
    return 0;
}

void XmlParser::SkipElementNameSpace(std::string &elementName) const
{
    std::string::size_type colonLastIndex = elementName.find_last_of(":");
    if (colonLastIndex != std::string::npos) {
        elementName = elementName.substr(colonLastIndex + 1);
    }
}

void XmlParser::DestroyDoc()
{
    if (xmlDocPtr_ != nullptr) {
        xmlFreeDoc(xmlDocPtr_);
        xmlCleanupParser();
        xmlMemoryDump();
    }
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS