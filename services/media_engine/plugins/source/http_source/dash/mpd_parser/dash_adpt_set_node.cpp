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

#include <cstdint>
#include <limits>
#include "dash_adpt_set_node.h"
#include "dash_mpd_util.h"
#include "utils/string_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

DashAdptSetNode::DashAdptSetNode()
{
    for (uint32_t index = 0; index < DASH_ADAPTATION_SET_ATTR_NUM; index++) {
        adptSetAttr_[index].attr_.assign(adptSetAttrs_[index]);
        adptSetAttr_[index].val_.assign("");
    }
}

DashAdptSetNode::~DashAdptSetNode() {}

void DashAdptSetNode::ParseNode(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement)
{
    if (xmlParser != nullptr) {
        comAttrsElements_.ParseAttrs(xmlParser, rootElement);

        for (uint32_t index = 0; index < DASH_ADAPTATION_SET_ATTR_NUM; index++) {
            xmlParser->GetAttribute(rootElement, adptSetAttr_[index].attr_, adptSetAttr_[index].val_);
        }
    }
}

void DashAdptSetNode::GetAttr(const std::string &attrName, std::string &sAttrVal)
{
    uint32_t index = DashGetAttrIndex(attrName, adptSetAttrs_, DASH_ADAPTATION_SET_ATTR_NUM);
    if (index < DASH_ADAPTATION_SET_ATTR_NUM) {
        if (adptSetAttr_[index].val_.length() > 0) {
            sAttrVal = adptSetAttr_[index].val_;
        } else {
            sAttrVal.assign("");
        }
    } else {
        comAttrsElements_.GetAttr(attrName, sAttrVal);
    }
}

void DashAdptSetNode::GetAttr(const std::string &attrName, uint32_t &uiAttrVal)
{
    uint32_t index = DashGetAttrIndex(attrName, adptSetAttrs_, DASH_ADAPTATION_SET_ATTR_NUM);
    if (index < DASH_ADAPTATION_SET_ATTR_NUM) {
        if (adptSetAttr_[index].val_.length() > 0) {
            int64_t tempUiAttrVal = 0;
            auto ret = StringUtil::SafeStoInt64(adptSetAttr_[index].val_, tempUiAttrVal);
            uiAttrVal = (ret && tempUiAttrVal >= 0 && tempUiAttrVal <= std::numeric_limits<uint32_t>::max()) ?
                static_cast<uint32_t>(tempUiAttrVal) : 0;
        } else {
            uiAttrVal = 0;
        }
    } else {
        comAttrsElements_.GetAttr(attrName, uiAttrVal);
    }
}

void DashAdptSetNode::GetAttr(const std::string &attrName, int32_t &iAttrVal)
{
    uint32_t index = DashGetAttrIndex(attrName, adptSetAttrs_, DASH_ADAPTATION_SET_ATTR_NUM);
    if (index < DASH_ADAPTATION_SET_ATTR_NUM) {
        if (adptSetAttr_[index].val_.length() > 0) {
            int64_t tempiAttrVal = 0;
            auto ret = StringUtil::SafeStoInt64(adptSetAttr_[index].val_, tempiAttrVal);
            iAttrVal = (ret && tempiAttrVal >= std::numeric_limits<int32_t>::min() &&
                tempiAttrVal <= std::numeric_limits<int32_t>::max()) ? static_cast<int32_t>(tempiAttrVal) : 0;
        } else {
            iAttrVal = 0;
        }
    } else {
        comAttrsElements_.GetAttr(attrName, iAttrVal);
    }
}

void DashAdptSetNode::GetAttr(const std::string &attrName, uint64_t &ullAttrVal)
{
    uint32_t index = DashGetAttrIndex(attrName, adptSetAttrs_, DASH_ADAPTATION_SET_ATTR_NUM);
    if (index < DASH_ADAPTATION_SET_ATTR_NUM) {
        if (adptSetAttr_[index].val_.length() > 0) {
            int64_t tempUllAttrVal = 0;
            auto ret = StringUtil::SafeStoInt64(adptSetAttr_[index].val_, tempUllAttrVal);
            ullAttrVal = (ret && tempUllAttrVal >= 0) ?
                static_cast<uint64_t>(tempUllAttrVal) : 0;
        } else {
            ullAttrVal = 0;
        }
    } else {
        comAttrsElements_.GetAttr(attrName, ullAttrVal);
    }
}

void DashAdptSetNode::GetAttr(const std::string &attrName, double &dAttrVal)
{
    uint32_t index = DashGetAttrIndex(attrName, adptSetAttrs_, DASH_ADAPTATION_SET_ATTR_NUM);
    if (index < DASH_ADAPTATION_SET_ATTR_NUM) {
        if (adptSetAttr_[index].val_.length() > 0) {
            dAttrVal = atof(adptSetAttr_[index].val_.c_str());
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