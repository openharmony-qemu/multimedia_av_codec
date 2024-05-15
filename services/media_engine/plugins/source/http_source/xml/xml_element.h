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

#ifndef HISTREAMER_XML_ELEMENT_H
#define HISTREAMER_XML_ELEMENT_H

#include <libxml/parser.h>
#include <string>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class XmlElement {
public:
    XmlElement() = delete;
    explicit XmlElement(xmlNodePtr);
    ~XmlElement() = default;

    std::shared_ptr<XmlElement> GetParent();
    std::shared_ptr<XmlElement> GetChild();
    std::shared_ptr<XmlElement> GetSiblingPrev();
    std::shared_ptr<XmlElement> GetSiblingNext();
    std::shared_ptr<XmlElement> GetLast();

    xmlNodePtr GetXmlNode();
    std::string GetName();
    std::string GetText();
    std::string GetAttribute(const std::string &attrName);

private:
    static constexpr const char *const TAG = "XmlElement";
    xmlNodePtr xmlNodePtr_;
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_XML_ELEMENT_H
