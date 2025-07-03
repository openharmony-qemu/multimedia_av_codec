/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef DASH_MULT_SEG_BASE_NODE_UNITTEST_H
#define DASH_MULT_SEG_BASE_NODE_UNITTEST_H

#include "gtest/gtest.h"
#include "dash_mult_seg_base_node.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

class DashMultSegBaseNodeUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    std::shared_ptr<DashMultSegBaseNode> dashMultSegBaseNode_ { nullptr };
};

class DashMpdNodeImpl : public IDashMpdNode {
public:
    DashMpdNodeImpl() = default;
    virtual ~DashMpdNodeImpl() = default;
    void ParseNode(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement) override {}
    void GetAttr(const std::string &attrName, std::string &val) override
    {
        if (attrName == "test") {
            val = "testVal";
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
#endif // DASH_MULT_SEG_BASE_NODE_UNITTEST_H