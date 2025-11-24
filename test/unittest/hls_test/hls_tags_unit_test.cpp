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
#include <cstring>
#include <sstream>
#include <stack>
#include <utility>
#include "http_server_demo.h"

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {

using namespace testing::ext;
using namespace std;
std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
void AttributeUnitTest::SetUpTestCase(void) {}

void AttributeUnitTest::TearDownTestCase(void) {}

void AttributeUnitTest::SetUp(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
}

void AttributeUnitTest::TearDown(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_Decimal_0001, TestSize.Level1)
{
    EXPECT_NE(attribute->Decimal(), 0.0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_QuotedString_0001, TestSize.Level1)
{
    EXPECT_NE(attribute->QuotedString(), "");
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_FloatingPoint_0001, TestSize.Level1)
{
    EXPECT_NE(attribute->FloatingPoint(), 0.0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_HexSequence_0001, TestSize.Level1)
{
    std::vector<uint8_t> res = attribute->HexSequence();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetByteRange_0001, TestSize.Level0)
{
    std::pair<std::size_t, std::size_t> range = attribute->GetByteRange();
    EXPECT_GE(range.first, 0);
    EXPECT_GE(range.second, 0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetResolution_0001, TestSize.Level1)
{
    std::pair<int, int> sol = attribute->GetResolution();
    EXPECT_GE(sol.first, 0);
    EXPECT_GE(sol.second, 0);
}

HWTEST_F(AttributeUnitTest, GetName, TestSize.Level0)
{
    Attribute attr("name", "value");
    EXPECT_EQ(attr.GetName(), "name");
}

HWTEST_F(AttributeUnitTest, Decimal, TestSize.Level1)
{
    Attribute attr("name", "123");
    EXPECT_EQ(attr.Decimal(), 123);
}

HWTEST_F(AttributeUnitTest, DecimalWithLocale, TestSize.Level1)
{
    Attribute attr("name", "1,234"); // "1,234" with comma as thousand separator
    EXPECT_EQ(attr.Decimal(), 1);    // Comma will be treated as delimiter and value will be truncated
}

HWTEST_F(AttributeUnitTest, FloatingPoint, TestSize.Level1)
{
    Attribute attr("name", "3.14");
    EXPECT_DOUBLE_EQ(attr.FloatingPoint(), 3.14);
}

HWTEST_F(AttributeUnitTest, HexSequence, TestSize.Level1)
{
    Attribute attr("name", "0x48656C6C6F");
    std::vector<uint8_t> expected { 0x48, 0x65, 0x6C, 0x6C, 0x6F };
    EXPECT_EQ(attr.HexSequence(), expected);
}

HWTEST_F(AttributeUnitTest, HexSequenceInvalidInput, TestSize.Level1)
{
    Attribute attr("name", "0x1G2H3I"); // Invalid hex characters
    std::vector<uint8_t> expected {'\x1', '\x2', '\x3'};
    EXPECT_EQ(attr.HexSequence(), expected);
}

HWTEST_F(AttributeUnitTest, GetByteRange, TestSize.Level1)
{
    Attribute attr("name", "10@100");
    auto byteRange = attr.GetByteRange();
    EXPECT_EQ(byteRange.first, 100);
    EXPECT_EQ(byteRange.second, 10);
}

HWTEST_F(AttributeUnitTest, GetByteRangeNoOffset, TestSize.Level1)
{
    Attribute attr("name", "10"); // Byte range without offset
    auto byteRange = attr.GetByteRange();
    EXPECT_EQ(byteRange.first, 0); // Offset should default to 0
    EXPECT_EQ(byteRange.second, 10);
}

HWTEST_F(AttributeUnitTest, GetResolution, TestSize.Level1)
{
    Attribute attr("name", "1920x1080");
    auto resolution = attr.GetResolution();
    EXPECT_EQ(resolution.first, 1920);
    EXPECT_EQ(resolution.second, 1080);
}

HWTEST_F(AttributeUnitTest, QuotedString, TestSize.Level1)
{
    Attribute attr("name", "\"quoted\"");
    EXPECT_EQ(attr.QuotedString(), "quoted");
}

HWTEST_F(AttributeUnitTest, QuotedStringEmpty, TestSize.Level1)
{
    Attribute attr("name", "\"\"");
    EXPECT_EQ(attr.QuotedString(), "");
}

HWTEST_F(AttributeUnitTest, QuotedStringNoQuotes, TestSize.Level1)
{
    Attribute attr("name", "quoted"); // No quotes
    EXPECT_EQ(attr.QuotedString(), "quoted");
}

HWTEST_F(AttributeUnitTest, UnescapeQuotes, TestSize.Level1)
{
    Attribute attr("name", "\"quo\\\"ted\"");
    auto unescapedAttr = attr.UnescapeQuotes();
    EXPECT_EQ(unescapedAttr.QuotedString(), "quo\"ted");
}

HWTEST_F(AttributeUnitTest, GetType, TestSize.Level1)
{
    Tag testTag(HlsTag::EXTXDISCONTINUITY);
    EXPECT_EQ(testTag.GetType(), HlsTag::EXTXDISCONTINUITY);
}

HWTEST_F(AttributeUnitTest, GetValue, TestSize.Level1)
{
    SingleValueTag testTag(HlsTag::EXTXVERSION, "3");
    const Attribute &attr = testTag.GetValue();
    EXPECT_EQ(attr.GetName(), "");
    EXPECT_EQ(attr.QuotedString(), "3");
}

HWTEST_F(AttributeUnitTest, GetAttributeByName, TestSize.Level1)
{
    AttributesTag testTag(HlsTag::EXTXKEY, "METHOD=AES-128,URI=\"https://test.com/key\",IV=0x1234567890ABCDEF");
    auto methodAttr = testTag.GetAttributeByName("METHOD");
    auto uriAttr = testTag.GetAttributeByName("URI");
    auto ivAttr = testTag.GetAttributeByName("IV");
    auto nonExistentAttr = testTag.GetAttributeByName("NON_EXISTENT");
    EXPECT_NE(methodAttr, nullptr);
    EXPECT_EQ(methodAttr->QuotedString(), "AES-128");
    EXPECT_NE(uriAttr, nullptr);
    EXPECT_EQ(uriAttr->QuotedString(), "https://test.com/key");
    EXPECT_NE(ivAttr, nullptr);
    EXPECT_EQ(ivAttr->QuotedString(), "0x1234567890ABCDEF");
    EXPECT_EQ(nonExistentAttr, nullptr);
}

HWTEST_F(AttributeUnitTest, ParseAttributes, TestSize.Level1)
{
    ValuesListTag testTag(HlsTag::EXTINF, "10,Segment Title");
    auto durationAttr = testTag.GetAttributeByName("DURATION");
    auto titleAttr = testTag.GetAttributeByName("TITLE");
    EXPECT_NE(durationAttr, nullptr);
    EXPECT_EQ(durationAttr->QuotedString(), "10");
    EXPECT_NE(titleAttr, nullptr);
    EXPECT_EQ(titleAttr->QuotedString(), ",Segment Title");
}

HWTEST_F(AttributeUnitTest, CreateTagByName, TestSize.Level1)
{
    auto extinfTag = TagFactory::CreateTagByName("extinf", "10,Segment Title");
    auto extxkeyTag = TagFactory::CreateTagByName("ext-x-key",
        "METHOD=AES-128,URI=\"https://test.com/key\",IV=0x1234567890ABCDEF");
    auto invalidTag = TagFactory::CreateTagByName("INVALID_TAG", "");
    EXPECT_NE(extinfTag, nullptr);
    EXPECT_EQ(extinfTag->GetType(), HlsTag::EXTINF);
    EXPECT_NE(extxkeyTag, nullptr);
    EXPECT_EQ(extxkeyTag->GetType(), HlsTag::EXTXKEY);
    EXPECT_EQ(invalidTag, nullptr);
}

HWTEST_F(AttributeUnitTest, DecimalEmptyString, TestSize.Level1)
{
    Attribute attr("name", "");
    EXPECT_EQ(attr.Decimal(), 0);
}

HWTEST_F(AttributeUnitTest, DecimalWhiteSpaceOnly, TestSize.Level1)
{
    Attribute attr("name", "   ");
    EXPECT_EQ(attr.Decimal(), 0);
}

HWTEST_F(AttributeUnitTest, DecimalInvalidStart, TestSize.Level1)
{
    Attribute attr("name", "abc123");
    EXPECT_EQ(attr.Decimal(), 0);
}

HWTEST_F(AttributeUnitTest, DecimalWithDecimalPoint, TestSize.Level1)
{
    Attribute attr("name", "123.45");
    EXPECT_EQ(attr.Decimal(), 123);
}

HWTEST_F(AttributeUnitTest, DecimalMaxValue, TestSize.Level1)
{
    // UINT64_MAX = 18446744073709551615
    Attribute attr("name", "18446744073709551615");
    EXPECT_EQ(attr.Decimal(), 18446744073709551615ULL);
}

HWTEST_F(AttributeUnitTest, FloatingPointInteger, TestSize.Level1)
{
    Attribute attr("name", "123");
    EXPECT_EQ(attr.FloatingPoint(), 123.0);
}

HWTEST_F(AttributeUnitTest, FloatingPointDecimal, TestSize.Level1)
{
    Attribute attr("name", "123.45");
    EXPECT_EQ(attr.FloatingPoint(), 123.45);
}

HWTEST_F(AttributeUnitTest, FloatingPointScientific, TestSize.Level1)
{
    Attribute attr("name", "1.23e4");
    EXPECT_EQ(attr.FloatingPoint(), 12300.0);

    Attribute attr2("name", "1.23E-5");
    EXPECT_EQ(attr2.FloatingPoint(), 1.23e-5);
}

HWTEST_F(AttributeUnitTest, FloatingPointEmptyString, TestSize.Level1)
{
    Attribute attr("name", "");
    EXPECT_EQ(attr.FloatingPoint(), 0.0);
}

HWTEST_F(AttributeUnitTest, FloatingPointWhiteSpaceOnly, TestSize.Level1)
{
    Attribute attr("name", "   ");
    EXPECT_EQ(attr.FloatingPoint(), 0.0);
}

HWTEST_F(AttributeUnitTest, FloatingPointInvalidStart, TestSize.Level1)
{
    Attribute attr("name", "abc123");
    EXPECT_EQ(attr.FloatingPoint(), 0.0);
}

HWTEST_F(AttributeUnitTest, HexSequenceNormalEven, TestSize.Level1)
{
    Attribute attr("name", "0x1A2B3C");
    auto result = attr.HexSequence();
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 0x1A);
    EXPECT_EQ(result[1], 0x2B);
    EXPECT_EQ(result[2], 0x3C);
}

HWTEST_F(AttributeUnitTest, HexSequenceCaseInsensitive, TestSize.Level1)
{
    Attribute attr("name", "0X1a2B3c");
    auto result = attr.HexSequence();
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 0x1A);
    EXPECT_EQ(result[1], 0x2B);
    EXPECT_EQ(result[2], 0x3C);
}

HWTEST_F(AttributeUnitTest, HexSequenceSingleByte, TestSize.Level1)
{
    Attribute attr("name", "0x1A");
    auto result = attr.HexSequence();
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 0x1A);
}

HWTEST_F(AttributeUnitTest, HexSequenceOddLength, TestSize.Level1)
{
    Attribute attr("name", "0x1A2B3");
    auto result = attr.HexSequence();
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], 0x1A);
    EXPECT_EQ(result[1], 0x2B);
}

HWTEST_F(AttributeUnitTest, HexSequenceEmpty, TestSize.Level1)
{
    Attribute attr("name", "");
    auto result = attr.HexSequence();
    EXPECT_TRUE(result.empty());
}

HWTEST_F(AttributeUnitTest, HexSequenceOnlyPrefix, TestSize.Level1)
{
    Attribute attr("name", "0x");
    auto result = attr.HexSequence();
    EXPECT_TRUE(result.empty());
}

HWTEST_F(AttributeUnitTest, GetByteRangeNormalWithOffset, TestSize.Level1)
{
    Attribute attr("name", "123@456");
    auto result = attr.GetByteRange();
    EXPECT_EQ(result.first, 456);
    EXPECT_EQ(result.second, 123);
}

HWTEST_F(AttributeUnitTest, GetByteRangeOnlyLength, TestSize.Level1)
{
    Attribute attr("name", "123");
    auto result = attr.GetByteRange();
    EXPECT_EQ(result.first, 0);
    EXPECT_EQ(result.second, 123);
}

HWTEST_F(AttributeUnitTest, GetByteRangeInvalidOffset, TestSize.Level1)
{
    Attribute attr("name", "123@abc");
    auto result = attr.GetByteRange();
    EXPECT_EQ(result.first, 0);
    EXPECT_EQ(result.second, 123);
}

HWTEST_F(AttributeUnitTest, GetByteRangeInvalidLength, TestSize.Level1)
{
    Attribute attr("name", "abc@123");
    auto result = attr.GetByteRange();
    EXPECT_EQ(result.first, 0);
    EXPECT_EQ(result.second, 0);
}

HWTEST_F(AttributeUnitTest, GetByteRangeMultipleAt, TestSize.Level1)
{
    Attribute attr("name", "123@456@789");
    auto result = attr.GetByteRange();
    EXPECT_EQ(result.first, 456);
    EXPECT_EQ(result.second, 123);
}

HWTEST_F(AttributeUnitTest, GetByteRangeOnlyAt, TestSize.Level1)
{
    Attribute attr("name", "@123");
    auto result = attr.GetByteRange();
    EXPECT_EQ(result.first, 0);
    EXPECT_EQ(result.second, 0);
}

HWTEST_F(AttributeUnitTest, GetResolutionNormal, TestSize.Level1)
{
    Attribute attr("name", "1920x1080");
    auto result = attr.GetResolution();
    EXPECT_EQ(result.first, 1920);
    EXPECT_EQ(result.second, 1080);
}

HWTEST_F(AttributeUnitTest, GetResolutionOnlyWidth, TestSize.Level1)
{
    Attribute attr("name", "1920");
    auto result = attr.GetResolution();
    EXPECT_EQ(result.first, 1920);
    EXPECT_EQ(result.second, 0);
}

HWTEST_F(AttributeUnitTest, GetResolutionInvalidCharacters, TestSize.Level1)
{
    Attribute attr("name", "abcx123");
    auto result = attr.GetResolution();
    EXPECT_EQ(result.first, 0);
    EXPECT_EQ(result.second, 0);
}

HWTEST_F(AttributeUnitTest, GetResolutionEmptyString TestSize.Level1)
{
    Attribute attr("name", "");
    auto result = attr.GetResolution();
    EXPECT_EQ(result.first, 0);
    EXPECT_EQ(result.second, 0);
}

HWTEST_F(AttributeUnitTest, GetResolutionSpaceAroundX, TestSize.Level1)
{
    Attribute attr("name", " 1920 x 1080");
    auto result = attr.GetResolution();
    EXPECT_EQ(result.first, 1920);
    EXPECT_EQ(result.second, 0);
}

HWTEST_F(AttributeUnitTest, QuotedStringNormal, TestSize.Level1)
{
    Attribute attr("name", "\"hello\"");
    EXPECT_EQ(attr.QuotedString(), "hello");
}

HWTEST_F(AttributeUnitTest, QuotedStringWithDoubleQuote, TestSize.Level1)
{
    Attribute attr("name", "\"hello\\\"world\"");
    EXPECT_EQ(attr.QuotedString(), "hello\"world");
}

HWTEST_F(AttributeUnitTest, QuotedStringEmpty_001, TestSize.Level1)
{
    Attribute attr("name", "\"\"");
    EXPECT_EQ(attr.QuotedString(), "");
}

HWTEST_F(AttributeUnitTest, QuotedStringNoQuotes_001, TestSize.Level1)
{
    Attribute attr("name", "hello");
    EXPECT_EQ(attr.QuotedString(), "hello");
}
}