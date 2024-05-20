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

#include "dash_com_attrs_elements.h"
#include "dash_mpd_util.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
DashComAttrsElements::DashComAttrsElements()
{
    for (int32_t index = 0; index < DASH_COMMON_ATTR_NUM; index++) {
        commonAttr_[index].attr_.assign(commonAttrs_[index]);
        commonAttr_[index].val_.assign("");
    }
}

DashComAttrsElements::~DashComAttrsElements() {}

void DashComAttrsElements::ParseAttrs(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement)
{
    if (xmlParser != nullptr) {
        for (uint32_t index = 0; index < DASH_COMMON_ATTR_NUM; index++) {
            xmlParser->GetAttribute(rootElement, commonAttr_[index].attr_, commonAttr_[index].val_);
        }
    }
}

void DashComAttrsElements::GetAttr(const std::string &attrName, std::string &sAttrVal) const
{
    uint32_t index = DashGetAttrIndex(attrName, commonAttrs_, DASH_COMMON_ATTR_NUM);
    if (index < DASH_COMMON_ATTR_NUM) {
        if (commonAttr_[index].val_.length() > 0) {
            sAttrVal = commonAttr_[index].val_;
        } else {
            sAttrVal.assign("");
        }
    }
}

void DashComAttrsElements::GetAttr(const std::string &attrName, uint32_t &uiAttrVal) const
{
    uint32_t index = DashGetAttrIndex(attrName, commonAttrs_, DASH_COMMON_ATTR_NUM);
    if (index < DASH_COMMON_ATTR_NUM) {
        if (commonAttr_[index].val_.length() > 0) {
            uiAttrVal = static_cast<uint32_t>(std::atoll(commonAttr_[index].val_.c_str()));
        } else {
            uiAttrVal = 0;
        }
    }
}

void DashComAttrsElements::GetAttr(const std::string &attrName, int32_t &iAttrVal) const
{
    uint32_t index = DashGetAttrIndex(attrName, commonAttrs_, DASH_COMMON_ATTR_NUM);
    if (index < DASH_COMMON_ATTR_NUM) {
        if (commonAttr_[index].val_.length() > 0) {
            iAttrVal = static_cast<int32_t>(std::atoll(commonAttr_[index].val_.c_str()));
        } else {
            iAttrVal = 0;
        }
    }
}

void DashComAttrsElements::GetAttr(const std::string &attrName, uint64_t &ullAttrVal) const
{
    uint32_t index = DashGetAttrIndex(attrName, commonAttrs_, DASH_COMMON_ATTR_NUM);
    if (index < DASH_COMMON_ATTR_NUM) {
        if (commonAttr_[index].val_.length() > 0) {
            ullAttrVal = static_cast<uint64_t>(std::atoll(commonAttr_[index].val_.c_str()));
        } else {
            ullAttrVal = 0;
        }
    }
}

void DashComAttrsElements::GetAttr(const std::string &attrName, double &dAttrVal) const
{
    uint32_t index = DashGetAttrIndex(attrName, commonAttrs_, DASH_COMMON_ATTR_NUM);
    if (index < DASH_COMMON_ATTR_NUM) {
        if (commonAttr_[index].val_.length() > 0) {
            dAttrVal = atof(commonAttr_[index].val_.c_str());
        } else {
            dAttrVal = 0.0;
        }
    }
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
