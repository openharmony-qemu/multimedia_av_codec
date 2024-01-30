/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "hls_tags_unit_test.h"
#include "gtest/gtest.h"
#include <cstring>
#include <sstream>
#include <stack>
#include <utility>

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
string g_name = "#EXT-X-VERSION";
string g_value = "3";
string g_key = "METHOD";
string g_value = "AES-128";

void AttributeUnitTest::SetUpTestCase()
{
    attribute_ = std::make_shared<Attribute>(g_name, g_value);
    tag_ = TagFactory::CreateTagByName(g_key, g_value);
    singleValueTag_ = std::make_shared<SingleValueTag>(tag_, g_value);
    attributeTag_ = std::make_shared<AttributesTag>(tag_, g_value);
    cout << "start" << endl;
}

void AttributeUnitTest::TearDownTestCase() {}

Attribute AttributeUnitTest::UnescapeQuotesTest()
{
    Attribute attribute = attribute_->UnescapeQuotes();
    return attribute;
}


HWTEST_F(AttributeUnitTest, HLS_TAGS_CreatAttribute_0001, TestSize.Level1)
{
    EXPECT_NE(attribute_, nullptr);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetAttributeName_0001, TestSize.Level1)
{
    EXPECT_EQ(attribute_->GetName(), g_name);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_UnescapeQuotes_0001, TestSize.Level1)
{
    attribute_ = AttributeUnitTest::UnescapeQuotesTest();
    EXPECT_NE(attribute_, nullptr);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_Decimal_0001, TestSize.Level1)
{
    EXPECT_NE(attribute_->Decimal(), 0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_QuotedString_0001, TestSize.Level1)
{
    EXPECT_NE(attribute_->QuotedString(), "");
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_FloatingPoint_0001, TestSize.Level1)
{
    EXPECT_NE(attribute_->FloatingPoint(), "");
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_HexSequence_0001, TestSize.Level1)
{
    std::vector<uint8_t> res = attribute_->HexSequence();
    EXPECT_GT(res.size(), 0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetByteRange_0001, TestSize.Level1)
{
    std::pair<std::size_t, std::size_t> range = attribute_->GetByteRange();
    EXPECT_GE(range.first, 0);
    EXPECT_GT(range.second, 0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetResolution_0001, TestSize.Level1)
{
    std::pair<int, int> sol = attribute_->GetResolution();
    EXPECT_GE(sol.first, 0);
    EXPECT_GT(sol.second, 0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_CreateTagByName_0001, TestSize.Level1)
{   
    String nameExists = "METHOD";
    String nameNoExists = "123";
    attributeTag_->GetAttributeByName();
    EXPECT_EQ(attributeTag_->GetAttributeByName(nameNoExists), nullptr);
    EXPECT_NE(attributeTag_->GetAttributeByName(nameExists), nullptr);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetType_0001, TestSize.Level1)
{
    EXPECT_NE(tag_->GetType(), "");
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetValue_0001, TestSize.Level1)
{
    std::shared_ptr<Attribute> attr = singleValueTag_->GetValue();
    EXPECT_NE(attr, nullptr);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetAttributeByName_0001, TestSize.Level1)
{
    std::shared_ptr<Attribute> attr = attributeTag_->GetAttributeByName();
    EXPECT_NE(attr, nullptr);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_AddAttribute_0001, TestSize.Level1)
{
    attributeTag_->AddAttribute(attribute_);
    EXPECT_QE(attributeTag_->attributes.size(), 1);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_ParseAttributeName_0001, TestSize.Level1)
{
    std::string name = attributeTag_->ParseAttributeName();
    EXPECT_NE(tag_, "");
}
}
