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

#include "xml/xml_element.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
XmlElement::XmlElement(xmlNodePtr xmlNode)
{
    xmlNodePtr_ = xmlNode;
}

std::shared_ptr<XmlElement> XmlElement::GetParent()
{
    if (xmlNodePtr_ != nullptr) {
        xmlNodePtr parent = xmlNodePtr_->parent;
        if (parent != nullptr) {
            return std::make_shared<XmlElement>(parent);
        }
    }
    return nullptr;
}

std::shared_ptr<XmlElement> XmlElement::GetChild()
{
    if (xmlNodePtr_ != nullptr) {
        xmlNodePtr children = xmlNodePtr_->children;
        if (children != nullptr) {
            return std::make_shared<XmlElement>(children);
        }
    }
    return nullptr;
}

std::shared_ptr<XmlElement> XmlElement::GetSiblingPrev()
{
    if (xmlNodePtr_ != nullptr) {
        xmlNodePtr prev = xmlNodePtr_->prev;
        if (prev != nullptr) {
            return std::make_shared<XmlElement>(prev);
        }
    }
    return nullptr;
}

std::shared_ptr<XmlElement> XmlElement::GetSiblingNext()
{
    if (xmlNodePtr_ != nullptr) {
        xmlNodePtr next = xmlNodePtr_->next;
        if (next != nullptr) {
            return std::make_shared<XmlElement>(next);
        }
    }
    return nullptr;
}

std::shared_ptr<XmlElement> XmlElement::GetLast()
{
    if (xmlNodePtr_ != nullptr) {
        xmlNodePtr last = xmlNodePtr_->last;
        if (last != nullptr) {
            return std::make_shared<XmlElement>(last);
        }
    }
    return nullptr;
}

xmlNodePtr XmlElement::GetXmlNode()
{
    return xmlNodePtr_;
}

std::string XmlElement::GetName()
{
    if (xmlNodePtr_ != nullptr) {
        const xmlChar *name = xmlNodePtr_->name;
        if (name != nullptr) {
            std::string nameValue(reinterpret_cast<const char *>(name));
            return nameValue;
        }
    }
    return "";
}

std::string XmlElement::GetText()
{
    if (xmlNodePtr_ != nullptr) {
        xmlChar *content = xmlNodeGetContent(xmlNodePtr_);
        if (content != nullptr) {
            std::string contentValue(reinterpret_cast<char *>(content));
            return contentValue;
        }
    }
    return "";
}

std::string XmlElement::GetAttribute(const std::string &attrName)
{
    if (xmlNodePtr_ != nullptr && !attrName.empty()) {
        if (xmlHasProp(xmlNodePtr_, BAD_CAST attrName.c_str())) {
            xmlChar *attr = xmlGetProp(xmlNodePtr_, BAD_CAST attrName.c_str());
            if (attr != nullptr) {
                std::string attrValue(reinterpret_cast<char *>(attr));
                xmlFree(attr);
                return attrValue;
            }
        }
    }
    return "";
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS