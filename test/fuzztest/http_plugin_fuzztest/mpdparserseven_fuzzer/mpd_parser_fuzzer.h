/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef MPD_PARSER_FUZZER_H
#define MPD_PARSER_FUZZER_H

#include "mock/xml_element.h"
#include "dash_mpd_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

class DashMpdNodeImpl : public IDashMpdNode {
public:
    DashMpdNodeImpl() = default;
    virtual ~DashMpdNodeImpl() = default;
    void ParseNode(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement) override {}
    void GetAttr(const std::string &attrName, std::string &val) override
    {
        if (attrName == "bitstreamSwitching") {
            val = "true";
        } else if (attrName == "codingDependency") {
            val = "true";
        } else if (attrName == "scanType") {
            val = "interlaced";
        } else if (attrName == "type") {
            val = "dynamic";
        } else {
            val = "";
        }
    }
    void GetAttr(const std::string &attrName, int32_t &val) override {}
    void GetAttr(const std::string &attrName, uint32_t &val) override {}
    void GetAttr(const std::string &attrName, uint64_t &val) override {}
    void GetAttr(const std::string &attrName, double &val) override {}
    {
        if (attrName == "maxPlayoutRate") {
            val = 0;
        }
    }
};

class DashMpdNodeImpl2 : public IDashMpdNode {
public:
    DashMpdNodeImpl2() = default;
    virtual ~DashMpdNodeImpl2() = default;
    void ParseNode(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement) override {}
    void GetAttr(const std::string &attrName, std::string &val) override
    {
        if (attrName == "scanType") {
            val = "unknown";
        } else {
            val = "";
        }
    }
    void GetAttr(const std::string &attrName, int32_t &val) override {}
    void GetAttr(const std::string &attrName, uint32_t &val) override {}
    void GetAttr(const std::string &attrName, uint64_t &val) override {}
    void GetAttr(const std::string &attrName, double &val) override {}
};

} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // MPD_PARSER_FUZZER_H