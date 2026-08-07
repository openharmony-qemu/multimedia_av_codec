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

#ifndef I_DASH_MPD_NODE_H
#define I_DASH_MPD_NODE_H

#include <cstdint>
#include <memory>
#include "xml_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class IDashMpdNode {
public:
    IDashMpdNode() = default;
    virtual ~IDashMpdNode() = default;

    static IDashMpdNode *CreateNode(const std::string &nodeName);
    static void DestroyNode(IDashMpdNode *node);

    virtual void ParseNode(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement) = 0;
    virtual void GetAttr(const std::string &attrName, std::string &val) = 0;
    virtual void GetAttr(const std::string &attrName, int32_t &val) = 0;
    virtual void GetAttr(const std::string &attrName, uint32_t &val) = 0;
    virtual void GetAttr(const std::string &attrName, uint64_t &val) = 0;
    virtual void GetAttr(const std::string &attrName, double &val) = 0;
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // I_DASH_MPD_NODE_H
