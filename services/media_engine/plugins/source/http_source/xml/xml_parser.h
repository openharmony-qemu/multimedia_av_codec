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

#ifndef HISTREAMER_XML_PARSER_H
#define HISTREAMER_XML_PARSER_H

#include "xml_element.h"
#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class XmlParser {
public:
    XmlParser() = default;
    ~XmlParser() = default;

    int32_t ParseFromBuffer(const char *buf, int32_t length);
    int32_t ParseFromString(const std::string &xmlStr);
    int32_t ParseFromFile(const std::string &filePath);
    std::shared_ptr<XmlElement> GetRootElement();
    int32_t GetAttribute(std::shared_ptr<XmlElement> element, const std::string attrName, std::string &value);
    void SkipElementNameSpace(std::string &elementName) const;
    void DestroyDoc();

private:
    static constexpr const char *const TAG = "XmlParser";
    xmlDocPtr xmlDocPtr_;
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_XML_PARSER_H
