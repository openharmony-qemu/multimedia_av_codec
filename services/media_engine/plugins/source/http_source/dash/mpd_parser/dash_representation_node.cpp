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

#include "dash_representation_node.h"
#include "dash_mpd_util.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
DashRepresentationNode::DashRepresentationNode()
{
    for (uint32_t index = 0; index < DASH_REPRESENTATION_ATTR_NUM; index++) {
        representationAttr_[index].attr_.assign(representationAttrs_[index]);
        representationAttr_[index].val_.assign("");
    }
}

DashRepresentationNode::~DashRepresentationNode() {}

void DashRepresentationNode::ParseNode(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement)
{
    if (xmlParser != nullptr) {
        comAttrsElements_.ParseAttrs(xmlParser, rootElement);

        for (uint32_t index = 0; index < DASH_REPRESENTATION_ATTR_NUM; index++) {
            xmlParser->GetAttribute(rootElement, representationAttr_[index].attr_, representationAttr_[index].val_);
        }
    }
}

void DashRepresentationNode::GetAttr(const std::string &attrName, std::string &sAttrVal)
{
    uint32_t index = DashGetAttrIndex(attrName, representationAttrs_, DASH_REPRESENTATION_ATTR_NUM);
    if (index < DASH_REPRESENTATION_ATTR_NUM) {
        if (representationAttr_[index].val_.length() > 0) {
            sAttrVal = representationAttr_[index].val_;
        } else {
            sAttrVal.assign("");
        }
    } else {
        comAttrsElements_.GetAttr(attrName, sAttrVal);
    }
}

void DashRepresentationNode::GetAttr(const std::string &attrName, uint32_t &uiAttrVal)
{
    uint32_t index = DashGetAttrIndex(attrName, representationAttrs_, DASH_REPRESENTATION_ATTR_NUM);
    if (index < DASH_REPRESENTATION_ATTR_NUM) {
        if (representationAttr_[index].val_.length() > 0) {
            uiAttrVal = static_cast<uint32_t>(std::atoll(representationAttr_[index].val_.c_str()));
        } else {
            uiAttrVal = 0;
        }
    } else {
        comAttrsElements_.GetAttr(attrName, uiAttrVal);
    }
}

void DashRepresentationNode::GetAttr(const std::string &attrName, int32_t &iAttrVal)
{
    uint32_t index = DashGetAttrIndex(attrName, representationAttrs_, DASH_REPRESENTATION_ATTR_NUM);
    if (index < DASH_REPRESENTATION_ATTR_NUM) {
        if (representationAttr_[index].val_.length() > 0) {
            iAttrVal = static_cast<int32_t>(std::atoll(representationAttr_[index].val_.c_str()));
        } else {
            iAttrVal = 0;
        }
    } else {
        comAttrsElements_.GetAttr(attrName, iAttrVal);
    }
}

void DashRepresentationNode::GetAttr(const std::string &attrName, uint64_t &ullAttrVal)
{
    uint32_t index = DashGetAttrIndex(attrName, representationAttrs_, DASH_REPRESENTATION_ATTR_NUM);
    if (index < DASH_REPRESENTATION_ATTR_NUM) {
        if (representationAttr_[index].val_.length() > 0) {
            ullAttrVal = static_cast<uint64_t>(std::atoll(representationAttr_[index].val_.c_str()));
        } else {
            ullAttrVal = 0;
        }
    } else {
        comAttrsElements_.GetAttr(attrName, ullAttrVal);
    }
}

void DashRepresentationNode::GetAttr(const std::string &attrName, double &dAttrVal)
{
    uint32_t index = DashGetAttrIndex(attrName, representationAttrs_, DASH_REPRESENTATION_ATTR_NUM);
    if (index < DASH_REPRESENTATION_ATTR_NUM) {
        if (representationAttr_[index].val_.length() > 0) {
            dAttrVal = atof(representationAttr_[index].val_.c_str());
        } else {
            dAttrVal = 0.0;
        }
    } else {
        comAttrsElements_.GetAttr(attrName, dAttrVal);
    }
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS