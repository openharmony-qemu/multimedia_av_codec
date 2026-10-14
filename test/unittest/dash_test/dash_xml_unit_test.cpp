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
    int32_t ret = xmlParser_->ParseFromString("<test>123</test>");
    ASSERT_EQ(ret, 0);
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

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Failed_004, TestSize.Level1)
{
    std::string xml = "<test>123</test>";
    double ret = xmlParser_->ParseFromBuffer(xml.c_str(), 0);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Success_001, TestSize.Level1)
{
    std::string xml = "<test>123</test>";
    int32_t ret = xmlParser_->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Success_002, TestSize.Level1)
{
    std::string xml = "<root id=\"1\"><test1>123</test1><test2></test2></root>";
    int32_t ret = xmlParser_->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
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

HWTEST_F(DashXmlUnitTest, Test_ParseFromFile_Failed_001, TestSize.Level1)
{
    std::string xml = "";
    double ret = xmlParser_->ParseFromFile(xml);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromFile_Failed_002, TestSize.Level1)
{
    std::string xml = "/test/index.mpd";
    double ret = xmlParser_->ParseFromFile(xml);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_GetSiblingNext_Positive_001, TestSize.Level1)
{
    std::string xml = "<root><child1>text1</child1><child2>text2</child2></root>";
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    int32_t ret = parser->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_NE(root, nullptr);
    std::shared_ptr<XmlElement> child = root->GetChild();
    ASSERT_NE(child, nullptr);
    ASSERT_EQ(child->GetName(), "child1");
    std::shared_ptr<XmlElement> next = child->GetSiblingNext();
    ASSERT_NE(next, nullptr);
    ASSERT_EQ(next->GetName(), "child2");
    parser->DestroyDoc();
}

HWTEST_F(DashXmlUnitTest, Test_GetText_Positive_001, TestSize.Level1)
{
    std::string xml = "<root>hello world</root>";
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    int32_t ret = parser->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_NE(root, nullptr);
    std::string text = root->GetText();
    ASSERT_EQ(text, "hello world");
    parser->DestroyDoc();
}

HWTEST_F(DashXmlUnitTest, Test_GetAttribute_Xmlns_001, TestSize.Level1)
{
    std::string xml = "<root xmlns=\"http://example.com\"><child/></root>";
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    int32_t ret = parser->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_NE(root, nullptr);
    std::string xmlnsAttr = root->GetAttribute("xmlns");
    ASSERT_EQ(xmlnsAttr, "http://example.com");
    parser->DestroyDoc();
}

HWTEST_F(DashXmlUnitTest, Test_GetAttribute_XmlnsPrefix_001, TestSize.Level1)
{
    std::string xml = "<root xmlns:ns=\"http://test.com\"><child/></root>";
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    int32_t ret = parser->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_NE(root, nullptr);
    std::string attr = root->GetAttribute("xmlns:ns");
    ASSERT_EQ(attr, "");
    parser->DestroyDoc();
}

HWTEST_F(DashXmlUnitTest, Test_GetAttribute_EmptyAttrName_001, TestSize.Level1)
{
    std::string xml = "<root id=\"1\"/>";
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    int32_t ret = parser->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_NE(root, nullptr);
    std::string attr = root->GetAttribute("");
    ASSERT_EQ(attr, "");
    parser->DestroyDoc();
}

HWTEST_F(DashXmlUnitTest, Test_GetAttribute_NoProp_NotXmlns_001, TestSize.Level1)
{
    std::string xml = "<root id=\"1\"/>";
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    int32_t ret = parser->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_NE(root, nullptr);
    std::string attr = root->GetAttribute("notExist");
    ASSERT_EQ(attr, "");
    parser->DestroyDoc();
}

HWTEST_F(DashXmlUnitTest, Test_GetRootElement_NullDoc_001, TestSize.Level1)
{
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_EQ(root, nullptr);
    std::string value;
    int32_t attrRet = parser->GetAttribute(nullptr, "test", value);
    ASSERT_EQ(attrRet, -1);
    std::string name = "ns:test";
    parser->SkipElementNameSpace(name);
    ASSERT_EQ(name, "test");
}

HWTEST_F(DashXmlUnitTest, Test_XmlParserGetAttribute_NullElement_001, TestSize.Level1)
{
    std::shared_ptr<XmlElement> nullElement = nullptr;
    std::string value;
    int32_t ret = xmlParser_->GetAttribute(nullElement, "test", value);
    ASSERT_EQ(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_XmlParserGetAttribute_ValidElement_001, TestSize.Level1)
{
    std::string xml = "<root id=\"42\"/>";
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    int32_t ret = parser->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_NE(root, nullptr);
    std::string value;
    int32_t attrRet = parser->GetAttribute(root, "id", value);
    ASSERT_EQ(attrRet, 0);
    ASSERT_EQ(value, "42");
    parser->DestroyDoc();
}

HWTEST_F(DashXmlUnitTest, Test_SkipElementNameSpace_WithColon_001, TestSize.Level1)
{
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    std::string name = "ns:element";
    parser->SkipElementNameSpace(name);
    ASSERT_EQ(name, "element");
}

HWTEST_F(DashXmlUnitTest, Test_SkipElementNameSpace_NoColon_001, TestSize.Level1)
{
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    std::string name = "element";
    parser->SkipElementNameSpace(name);
    ASSERT_EQ(name, "element");
}

HWTEST_F(DashXmlUnitTest, Test_GetLast_Children_001, TestSize.Level1)
{
    std::string xml = "<root><a>1</a><b>2</b><c>3</c></root>";
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    int32_t ret = parser->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_NE(root, nullptr);
    std::shared_ptr<XmlElement> last = root->GetLast();
    ASSERT_NE(last, nullptr);
    ASSERT_EQ(last->GetName(), "c");
    std::shared_ptr<XmlElement> prev = last->GetSiblingPrev();
    ASSERT_NE(prev, nullptr);
    ASSERT_EQ(prev->GetName(), "b");
    parser->DestroyDoc();
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromString_Success_GetRoot_001, TestSize.Level1)
{
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    int32_t ret = parser->ParseFromString("<root attr=\"val\">content</root>");
    ASSERT_EQ(ret, 0);
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->GetName(), "root");
    ASSERT_EQ(root->GetAttribute("attr"), "val");
    ASSERT_EQ(root->GetText(), "content");
    parser->DestroyDoc();
}

HWTEST_F(DashXmlUnitTest, Test_GetXmlNode_001, TestSize.Level1)
{
    std::string xml = "<root/>";
    std::shared_ptr<XmlParser> parser = std::make_shared<XmlParser>();
    int32_t ret = parser->ParseFromBuffer(xml.c_str(), xml.length());
    ASSERT_EQ(ret, 0);
    std::shared_ptr<XmlElement> root = parser->GetRootElement();
    ASSERT_NE(root, nullptr);
    xmlNodePtr node = root->GetXmlNode();
    ASSERT_NE(node, nullptr);
    std::string name = reinterpret_cast<const char*>(node->name);
    ASSERT_EQ(name, "root");
    parser->DestroyDoc();
}

}
}
}
}
