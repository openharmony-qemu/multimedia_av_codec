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

#include "xml_parser.h"
#include "gtest/gtest.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class XmlParserUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<XmlParser> xmlParser_;
};

void XmlParserUnitTest::SetUp(void)
{
    xmlParser_ = std::make_shared<XmlParser>();
}

void XmlParserUnitTest::TearDown(void)
{
    if (xmlParser_ != nullptr) {
        xmlParser_->DestroyDoc();
        xmlParser_.reset();
    }
}

/**
 * @tc.name  : Test ParserFromFile API
 * @tc.number: ParserFromFile_001
 * @tc.desc  : Test filePath.empty() == true
 */
HWTEST_F(XmlParserUnitTest, ParserFromFile_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, xmlParser_);
    std::string mockStr;
    ASSERT_EQ(-1, xmlParser_->ParseFromFile(mockStr));
}

/**
 * @tc.name  : Test GetRootElement API
 * @tc.number: GetRootElement_001
 * @tc.desc  : Test xmlDocPtr_ == nullptr
 */
HWTEST_F(XmlParserUnitTest, GetRootElement_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, xmlParser_);
    ASSERT_EQ(nullptr, xmlParser_->xmlDocPtr_);
    ASSERT_EQ(nullptr, xmlParser_->GetRootElement());
}

/**
 * @tc.name  : Test GetRootElement API
 * @tc.number: GetRootElement_002
 * @tc.desc  : Test xmlDocPtr_ != nullptr && rootNode == nullptr
 */
HWTEST_F(XmlParserUnitTest, GetRootElement_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, xmlParser_);
    xmlDoc mockDoc;
    xmlParser_->xmlDocPtr_ = &mockDoc;
    ASSERT_EQ(nullptr, xmlParser_->GetRootElement());
    xmlParser_->xmlDocPtr_ = nullptr;
}

/**
 * @tc.name  : Test GetAttribute API
 * @tc.number: GetAttribute_001
 * @tc.desc  : Test element == nullptr
 */
HWTEST_F(XmlParserUnitTest, GetAttribute_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, xmlParser_);
    std::shared_ptr<XmlElement> element;
    std::string value;
    ASSERT_EQ(-1, xmlParser_->GetAttribute(element, "", value));
}
}
}
}
}