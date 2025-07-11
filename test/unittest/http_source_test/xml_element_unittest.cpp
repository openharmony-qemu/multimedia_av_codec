/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "xml_element.h"
#include "gtest/gtest.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class XmlElementUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<XmlElement> xmlElement_;
};

void XmlElementUnitTest::SetUp(void)
{
    xmlElement_ = std::make_shared<XmlElement>(nullptr);
}

void XmlElementUnitTest::TearDown(void)
{
    xmlElement_.reset();
}

/**
 * @tc.name  : Test GetName API
 * @tc.number: GetName_001
 * @tc.desc  : Test name == nullptr
 */
HWTEST_F(XmlElementUnitTest, GetName_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, xmlElement_);
    xmlNode node;
    node.name = nullptr;
    xmlElement_->xmlNodePtr_ = &node;
    ASSERT_NE(nullptr, xmlElement_->xmlNodePtr_);
    ASSERT_EQ("", xmlElement_->GetName());
}

/**
 * @tc.name  : Test GetText API
 * @tc.number: GetText_001
 * @tc.desc  : Test context == nullptr
 */
HWTEST_F(XmlElementUnitTest, GetText_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, xmlElement_);
    xmlNode node;
    node.name = nullptr;
    xmlElement_->xmlNodePtr_ = &node;
    ASSERT_EQ("", xmlElement_->GetText());
}
}
}
}
}