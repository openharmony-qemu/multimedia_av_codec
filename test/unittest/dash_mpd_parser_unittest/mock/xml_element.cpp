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

#include "xml_element.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

std::shared_ptr<XmlElement> XmlElement::GetParent()
{
    return nullptr;
}

std::shared_ptr<XmlElement> XmlElement::GetChild()
{
    return shared_from_this();
}

std::shared_ptr<XmlElement> XmlElement::GetSiblingPrev()
{
    return nullptr;
}

std::shared_ptr<XmlElement> XmlElement::GetSiblingNext()
{
    return nullptr;
}

std::shared_ptr<XmlElement> XmlElement::GetLast()
{
    return nullptr;
}

xmlNodePtr XmlElement::GetXmlNode()
{
    return nullptr;
}

std::string XmlElement::GetName()
{
    return name_;
}

std::string XmlElement::GetText()
{
    if (name_ == "pssh") {
        return "pssh";
    }
    if (name_ == "BaseURL") {
        return "BaseURL";
    }
    return "null";
}

std::string XmlElement::GetAttribute(const std::string &attrName)
{
    return "";
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS