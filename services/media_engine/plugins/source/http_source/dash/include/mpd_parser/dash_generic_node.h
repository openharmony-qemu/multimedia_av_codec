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

#ifndef DASH_GENERIC_NODE_H
#define DASH_GENERIC_NODE_H

#include "i_dash_mpd_node.h"
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

class IDashMpdNode;

class DashGenericNode : public IDashMpdNode {
public:
    DashGenericNode(const std::vector<std::string>& attrs,
                    const std::vector<std::string>& commonAttrs = {});
    DashGenericNode(const std::vector<std::string>& attrs,
                    const std::vector<std::string>& commonAttrs,
                    const std::vector<std::pair<std::string, std::string>>& childNodes);
    ~DashGenericNode() override;

    void ParseNode(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement) override;
    void GetAttr(const std::string &attrName, std::string &val) override;
    void GetAttr(const std::string &attrName, int32_t &val) override;
    void GetAttr(const std::string &attrName, uint32_t &val) override;
    void GetAttr(const std::string &attrName, uint64_t &val) override;
    void GetAttr(const std::string &attrName, double &val) override;

private:
    std::vector<std::string> attrs_;
    std::vector<std::string> vals_;
    std::vector<std::string> commonAttrs_;
    std::vector<std::string> commonVals_;
    std::vector<std::unique_ptr<IDashMpdNode>> childNodes_;
    std::vector<std::string> childNodeNames_;
};

} // namespace HttpPlugin
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // DASH_GENERIC_NODE_H
