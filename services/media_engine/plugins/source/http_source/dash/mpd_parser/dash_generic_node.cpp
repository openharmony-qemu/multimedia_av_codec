/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "dash_generic_node.h"
#include "i_dash_mpd_node.h"
#include "utils/string_utils.h"
#include <cstdlib>
#include <limits>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

DashGenericNode::DashGenericNode(const std::vector<std::string>& attrs,
                                 const std::vector<std::string>& commonAttrs)
    : attrs_(attrs), vals_(attrs.size()),
      commonAttrs_(commonAttrs), commonVals_(commonAttrs.size())
{
}

DashGenericNode::DashGenericNode(const std::vector<std::string>& attrs,
                                 const std::vector<std::string>& commonAttrs,
                                 const std::vector<std::pair<std::string, std::string>>& childNodes)
    : attrs_(attrs), vals_(attrs.size()),
      commonAttrs_(commonAttrs), commonVals_(commonAttrs.size()),
      childNodeNames_(childNodes.size())
{
    for (size_t i = 0; i < childNodes.size(); ++i) {
        childNodeNames_[i] = childNodes[i].second;
        IDashMpdNode* childNode = IDashMpdNode::CreateNode(childNodes[i].second);
        if (childNode != nullptr) {
            childNodes_.emplace_back(childNode);
        }
    }
}

DashGenericNode::~DashGenericNode()
{
    childNodes_.clear();
}

void DashGenericNode::ParseNode(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement)
{
    if (xmlParser != nullptr) {
        for (size_t i = 0; i < attrs_.size(); ++i) {
            xmlParser->GetAttribute(rootElement, attrs_[i], vals_[i]);
        }
        for (size_t i = 0; i < commonAttrs_.size(); ++i) {
            xmlParser->GetAttribute(rootElement, commonAttrs_[i], commonVals_[i]);
        }
    }
    for (size_t i = 0; i < childNodes_.size(); ++i) {
        if (childNodes_[i] != nullptr) {
            childNodes_[i]->ParseNode(xmlParser, rootElement);
        }
    }
}

void DashGenericNode::GetAttr(const std::string &attrName, std::string &val)
{
    for (size_t i = 0; i < attrs_.size(); ++i) {
        if (attrs_[i] == attrName && !vals_[i].empty()) {
            val = vals_[i];
            return;
        }
    }
    for (size_t i = 0; i < commonAttrs_.size(); ++i) {
        if (commonAttrs_[i] == attrName && !commonVals_[i].empty()) {
            val = commonVals_[i];
            return;
        }
    }
    for (size_t i = 0; i < childNodes_.size(); ++i) {
        if (childNodes_[i] != nullptr) {
            childNodes_[i]->GetAttr(attrName, val);
            if (!val.empty()) {
                return;
            }
        }
    }
    val.clear();
}

void DashGenericNode::GetAttr(const std::string &attrName, int32_t &val)
{
    for (size_t i = 0; i < attrs_.size(); ++i) {
        if (attrs_[i] == attrName && !vals_[i].empty()) {
            int64_t tempVal = 0;
            auto ret = StringUtil::SafeStoInt64(vals_[i], tempVal);
            val = (ret && tempVal >= std::numeric_limits<int32_t>::min() &&
                   tempVal <= std::numeric_limits<int32_t>::max()) ? static_cast<int32_t>(tempVal) : 0;
            return;
        }
    }
    for (size_t i = 0; i < commonAttrs_.size(); ++i) {
        if (commonAttrs_[i] == attrName && !commonVals_[i].empty()) {
            int64_t tempVal = 0;
            auto ret = StringUtil::SafeStoInt64(commonVals_[i], tempVal);
            val = (ret && tempVal >= std::numeric_limits<int32_t>::min() &&
                   tempVal <= std::numeric_limits<int32_t>::max()) ? static_cast<int32_t>(tempVal) : 0;
            return;
        }
    }
    bool found = false;
    int32_t foundVal = 0;
    for (size_t i = 0; i < childNodes_.size(); ++i) {
        if (childNodes_[i] != nullptr) {
            int32_t childVal = 0;
            childNodes_[i]->GetAttr(attrName, childVal);
            std::string strVal;
            childNodes_[i]->GetAttr(attrName, strVal);
            if (!strVal.empty()) {
                found = true;
                foundVal = childVal;
                break;
            }
        }
    }
    if (found) {
        val = foundVal;
    } else {
        val = 0;
    }
}

void DashGenericNode::GetAttr(const std::string &attrName, uint32_t &val)
{
    for (size_t i = 0; i < attrs_.size(); ++i) {
        if (attrs_[i] == attrName && !vals_[i].empty()) {
            int64_t tempVal = 0;
            auto ret = StringUtil::SafeStoInt64(vals_[i], tempVal);
            val = (ret && tempVal >= 0 && tempVal <= std::numeric_limits<uint32_t>::max()) ?
                  static_cast<uint32_t>(tempVal) : 0;
            return;
        }
    }
    for (size_t i = 0; i < commonAttrs_.size(); ++i) {
        if (commonAttrs_[i] == attrName && !commonVals_[i].empty()) {
            int64_t tempVal = 0;
            auto ret = StringUtil::SafeStoInt64(commonVals_[i], tempVal);
            val = (ret && tempVal >= 0 && tempVal <= std::numeric_limits<uint32_t>::max()) ?
                  static_cast<uint32_t>(tempVal) : 0;
            return;
        }
    }
    bool found = false;
    uint32_t foundVal = 0;
    for (size_t i = 0; i < childNodes_.size(); ++i) {
        if (childNodes_[i] != nullptr) {
            std::string strVal;
            childNodes_[i]->GetAttr(attrName, strVal);
            if (!strVal.empty()) {
                uint32_t childVal = 0;
                childNodes_[i]->GetAttr(attrName, childVal);
                found = true;
                foundVal = childVal;
                break;
            }
        }
    }
    if (found) {
        val = foundVal;
    } else {
        val = 0;
    }
}

void DashGenericNode::GetAttr(const std::string &attrName, uint64_t &val)
{
    for (size_t i = 0; i < attrs_.size(); ++i) {
        if (attrs_[i] == attrName && !vals_[i].empty()) {
            int64_t tempVal = 0;
            auto ret = StringUtil::SafeStoInt64(vals_[i], tempVal);
            val = (ret && tempVal >= 0) ? static_cast<uint64_t>(tempVal) : 0;
            return;
        }
    }
    for (size_t i = 0; i < commonAttrs_.size(); ++i) {
        if (commonAttrs_[i] == attrName && !commonVals_[i].empty()) {
            int64_t tempVal = 0;
            auto ret = StringUtil::SafeStoInt64(commonVals_[i], tempVal);
            val = (ret && tempVal >= 0) ? static_cast<uint64_t>(tempVal) : 0;
            return;
        }
    }
    bool found = false;
    uint64_t foundVal = 0;
    for (size_t i = 0; i < childNodes_.size(); ++i) {
        if (childNodes_[i] != nullptr) {
            std::string strVal;
            childNodes_[i]->GetAttr(attrName, strVal);
            if (!strVal.empty()) {
                uint64_t childVal = 0;
                childNodes_[i]->GetAttr(attrName, childVal);
                found = true;
                foundVal = childVal;
                break;
            }
        }
    }
    if (found) {
        val = foundVal;
    } else {
        val = 0;
    }
}

void DashGenericNode::GetAttr(const std::string &attrName, double &val)
{
    for (size_t i = 0; i < attrs_.size(); ++i) {
        if (attrs_[i] == attrName && !vals_[i].empty()) {
            val = std::atof(vals_[i].c_str());
            return;
        }
    }
    for (size_t i = 0; i < commonAttrs_.size(); ++i) {
        if (commonAttrs_[i] == attrName && !commonVals_[i].empty()) {
            val = std::atof(commonVals_[i].c_str());
            return;
        }
    }
    bool found = false;
    double foundVal = 0.0;
    for (size_t i = 0; i < childNodes_.size(); ++i) {
        if (childNodes_[i] != nullptr) {
            std::string strVal;
            childNodes_[i]->GetAttr(attrName, strVal);
            if (!strVal.empty()) {
                double childVal = 0.0;
                childNodes_[i]->GetAttr(attrName, childVal);
                found = true;
                foundVal = childVal;
                break;
            }
        }
    }
    if (found) {
        val = foundVal;
    } else {
        val = 0.0;
    }
}

} // namespace HttpPlugin
} // namespace Plugin
} // namespace Media
} // namespace OHOS
