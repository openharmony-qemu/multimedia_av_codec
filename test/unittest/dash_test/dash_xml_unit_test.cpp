/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "dash_xml_unit_test.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing::ext;

void DashXmlUnitTest::SetUpTestCase(void) {}

void DashXmlUnitTest::TearDownTestCase(void)
{
    if (xmlParser_ != nullptr) {
        xmlParser_->DestroyDoc();
    }
}

void DashXmlUnitTest::SetUp(void) {}

void DashXmlUnitTest::TearDown(void) {}

HWTEST_F(DashXmlUnitTest, Test_ParseFromString_Failed_001, TestSize.Level1)
{
    double ret = xmlParser_->ParseFromString("");
    EXPECT_GE(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromString_Failed_002, TestSize.Level1)
{
    double ret = xmlParser_->ParseFromString("<test><11111>");
    EXPECT_GE(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromString_Success_001, TestSize.Level1)
{
    double ret = xmlParser_->ParseFromString("<test>123</test>");
    EXPECT_GE(ret, 0);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Failed_001, TestSize.Level1)
{
    double ret = xmlParser_->ParseFromBuffer(nullptr, 1);
    EXPECT_GE(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Failed_002, TestSize.Level1)
{
    std::string xml = "";
    double ret = xmlParser_->ParseFromBuffer(xml.c_str(), xml.length());
    EXPECT_GE(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Failed_003, TestSize.Level1)
{
    std::string xml = "<test><11111>";
    double ret = xmlParser_->ParseFromBuffer(xml.c_str(), xml.length());
    EXPECT_GE(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Success_001, TestSize.Level1)
{
    std::string xml = "<test>123</test>";
    double ret = xmlParser_->ParseFromBuffer(xml.c_str(), xml.length());
    EXPECT_GE(ret, 0);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Success_002, TestSize.Level1)
{
    std::string xml = "<root id=\"1\"><test1>123</test1><test2></test2></root>";
    double ret = xmlParser_->ParseFromBuffer(xml.c_str(), xml.length());
    EXPECT_GE(ret, 0);
    std::shared_ptr<XmlElement> root = xmlParser_->GetRootElement();
    root->GetXmlNode();
    std::string attr = root->GetAttribute("test");
    EXPECT_TRUE(attr.empty());
    std::string id = root->GetAttribute("id");
    EXPECT_EQ(id, "1");
    std::shared_ptr<XmlElement> child = root->GetChild();
    std::shared_ptr<XmlElement> parent = child->GetParent();
    std::shared_ptr<XmlElement> last = root->GetLast();
    std::shared_ptr<XmlElement> prev = last->GetSiblingPrev();
    EXPECT_NE(parent, nullptr);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Success_003, TestSize.Level1)
{
    std::shared_ptr<XmlElement> root = std::make_shared<XmlElement>(nullptr);
    std::string name = root->GetName();
    EXPECT_TRUE(name.empty());
    std::string text = root->GetText();
    EXPECT_TRUE(text.empty());
    std::string attr = root->GetAttribute("test");
    EXPECT_TRUE(attr.empty());
    std::shared_ptr<XmlElement> parent = root->GetParent();
    EXPECT_EQ(parent, nullptr);
    std::shared_ptr<XmlElement> child = root->GetChild();
    EXPECT_EQ(child, nullptr);
    std::shared_ptr<XmlElement> prev = root->GetSiblingPrev();
    EXPECT_EQ(prev, nullptr);
    std::shared_ptr<XmlElement> next = root->GetSiblingNext();
    EXPECT_EQ(next, nullptr);
    std::shared_ptr<XmlElement> last = root->GetLast();
    EXPECT_EQ(last, nullptr);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Success_004, TestSize.Level1)
{
    xmlNodePtr node = xmlNewNode(nullptr, BAD_CAST "root");
    std::shared_ptr<XmlElement> root = std::make_shared<XmlElement>(node);
    std::string name = root->GetName();
    EXPECT_FALSE(name.empty());
    std::string text = root->GetText();
    EXPECT_TRUE(text.empty());
    std::string attr = root->GetAttribute("test");
    EXPECT_TRUE(attr.empty());
    std::shared_ptr<XmlElement> parent = root->GetParent();
    EXPECT_EQ(parent, nullptr);
    std::shared_ptr<XmlElement> child = root->GetChild();
    EXPECT_EQ(child, nullptr);
    std::shared_ptr<XmlElement> prev = root->GetSiblingPrev();
    EXPECT_EQ(prev, nullptr);
    std::shared_ptr<XmlElement> next = root->GetSiblingNext();
    EXPECT_EQ(next, nullptr);
    std::shared_ptr<XmlElement> last = root->GetLast();
    EXPECT_EQ(last, nullptr);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromFile_Success_001, TestSize.Level1)
{
    std::string xml = "/data/test/media/test_dash/segment_base/index.mpd";
    double ret = xmlParser_->ParseFromFile(xml);
    EXPECT_GE(ret, -1);
}
}
}
}
}