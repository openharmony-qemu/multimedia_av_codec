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

#ifndef DASH_COM_ATTRS_ELEMENTS_H
#define DASH_COM_ATTRS_ELEMENTS_H

#include <cstdint>
#include <memory>
#include "mpd_parser_def.h"
#include "xml_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class DashComAttrsElements {
public:
    DashComAttrsElements();
    virtual ~DashComAttrsElements();

    void ParseAttrs(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement);
    void GetAttr(const std::string &attrName, std::string &sAttrVal) const;
    void GetAttr(const std::string &attrName, uint32_t &uiAttrVal) const;
    void GetAttr(const std::string &attrName, int32_t &iAttrVal) const;
    void GetAttr(const std::string &attrName, uint64_t &ullAttrVal) const;
    void GetAttr(const std::string &attrName, double &dAttrVal) const;

private:
    static constexpr int32_t DASH_COMMON_ATTR_NUM = 14;
    static constexpr const char *commonAttrs_[DASH_COMMON_ATTR_NUM] = {
        "profiles",          "width",          "height",           "sar",     "frameRate",
        "audioSamplingRate", "mimeType",       "segmentProfiles",  "codecs",  "maximumSAPPeriod",
        "startWithSAP",      "maxPlayoutRate", "codingDependency", "scanType"};

    NodeAttr commonAttr_[DASH_COMMON_ATTR_NUM];
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // DASH_COM_ATTRS_ELEMENTS_H
