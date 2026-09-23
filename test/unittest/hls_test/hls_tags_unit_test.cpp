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
    EXPECT_EQ(attribute->Decimal(), 3ULL);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_QuotedString_0001, TestSize.Level1)
{
    EXPECT_NE(attribute->QuotedString(), "");
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_FloatingPoint_0001, TestSize.Level1)
{
    EXPECT_EQ(attribute->FloatingPoint(), 3.0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_HexSequence_0001, TestSize.Level1)
{
    std::vector<uint8_t> res = attribute->HexSequence();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetByteRange_0001, TestSize.Level0)
{
    std::pair<std::size_t, std::size_t> range = attribute->GetByteRange();
    EXPECT_EQ(range.first, static_cast<std::size_t>(0));
    EXPECT_EQ(range.second, static_cast<std::size_t>(3));
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetResolution_0001, TestSize.Level1)
{
    std::pair<int, int> sol = attribute->GetResolution();
    EXPECT_EQ(sol.first, 3);
    EXPECT_EQ(sol.second, 0);
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
    EXPECT_EQ(titleAttr->QuotedString(), "Segment Title");
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

HWTEST_F(AttributeUnitTest, GetResolutionEmptyString, TestSize.Level1)
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

HWTEST_F(AttributeUnitTest, ParseUriQuery_001, TestSize.Level1)
{
    std::string uri = "http://example.com/path";
    auto params = ParseUriQuery(uri);
    EXPECT_TRUE(params.empty());
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_002, TestSize.Level1)
{
    std::string uri = "http://example.com/path?key=value";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params.size(), 1);
    EXPECT_EQ(params["key"], "value");
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_003, TestSize.Level1)
{
    std::string uri = "http://example.com/path?k1=v1&k2=v2&k3=v3";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params.size(), 3);
    EXPECT_EQ(params["k1"], "v1");
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_004, TestSize.Level1)
{
    std::string uri = "http://example.com/path?param=hello%20world";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params["param"], "hello world");
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_005, TestSize.Level1)
{
    std::string uri = "http://example.com/path?param=hello+world";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params["param"], "hello world");
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_006, TestSize.Level1)
{
    std::string uri = "http://example.com/path?param=hello%2Gworld";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params["param"], "hello%2Gworld");
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_007, TestSize.Level1)
{
    std::string uri = "http://example.com/path=?key=&=value";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params.size(), 1);
    EXPECT_EQ(params["key"], "");
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_008, TestSize.Level1)
{
    std::string uri = "http://example.com/path?k1=v1&&k2=v2";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params.size(), 2);
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_009, TestSize.Level1)
{
    std::string uri = "http://example.com/path?param=100%25";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params["param"], "100%");
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_010, TestSize.Level1)
{
    std::string uri = "";
    auto params = ParseUriQuery(uri);
    EXPECT_TRUE(params.empty());
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_011, TestSize.Level1)
{
    std::string uri = "http://example.com/path?";
    auto params = ParseUriQuery(uri);
    EXPECT_TRUE(params.empty());
}

HWTEST_F(AttributeUnitTest, ParseUriQuery_012, TestSize.Level1)
{
    std::string uri = "http://example.com/path?key=a%4";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params["key"], "a%4");
}
 
HWTEST_F(AttributeUnitTest, ParseUriQuery_013, TestSize.Level1)
{
    std::string uri = "http://example.com/path?key=%4";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params["key"], "%4");
}
 
HWTEST_F(AttributeUnitTest, ParseUriQuery_014, TestSize.Level1)
{
    std::string uri = "http://example.com/path?key=a%";
    auto params = ParseUriQuery(uri);
    EXPECT_EQ(params["key"], "a%");
}

HWTEST_F(AttributeUnitTest, ParseEntries_001, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-DEFINE:NAME=\"test\",VALUE=\"out\"\n"
        "#EXT-X-MEDIA-SEQUENCE:0\n"
        "#EXT-X-KEY:METHOD=AES-128,URI=\"enc.key\",IV=0xd017dc49908d7272fc2def8ffe6fc15e\n"
        "#EXTINF:11.333333,\n"
        "{$test}000.ts";
    auto result = ParseEntries(hlsContent, {}, {});
    auto& tagList = result.first;
    auto tagDefineMap = result.second;

    EXPECT_EQ(tagList.size(), 4);
    EXPECT_EQ(tagDefineMap["test"], "out");
}

HWTEST_F(AttributeUnitTest, ParseEntries_002, TestSize.Level1)
{
    auto result = ParseEntries("", {}, {});
    EXPECT_TRUE(result.first.empty());
}

HWTEST_F(AttributeUnitTest, ParseEntries_003, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-DEFINE:NAME=\"test\"\n"
        "test.ts";
    auto result = ParseEntries(hlsContent, {}, {});
    EXPECT_EQ(result.second.size(), 0);
}

HWTEST_F(AttributeUnitTest, ParseEntries_004, TestSize.Level1)
{
    std::unordered_map<std::string, std::string> masterMap = {{"qual", "ity"}};
    std::string hlsContent = "#EXT-X-DEFINE:IMPORT=\"qual\"\n"
        "test.ts";
    auto result = ParseEntries(hlsContent, masterMap, {});
    EXPECT_EQ(result.second["qual"], "ity");
}

HWTEST_F(AttributeUnitTest, ParseEntries_005, TestSize.Level1)
{
    std::unordered_map<std::string, std::string> uriMap = {{"param", "value"}};
    std::string hlsContent = "#EXT-X-DEFINE:QUERYPARAM=\"param\"\n"
        "test.ts";
    auto result = ParseEntries(hlsContent, {}, uriMap);
    EXPECT_EQ(result.second["param"], "value");
}

HWTEST_F(AttributeUnitTest, SafeStringToInt, TestSize.Level1)
{
    std::string input = "0";
    int output {};
    int base {10};
    auto result = Attribute::SafeStringToInt(input, output, base);
    EXPECT_TRUE(result);
    EXPECT_EQ(output, 0);

    input = "123456";
    result = Attribute::SafeStringToInt(input, output, base);
    EXPECT_TRUE(result);
    EXPECT_EQ(output, 123456);

    input = "#$46(!%)_=+af46";
    result = Attribute::SafeStringToInt(input, output, base);
    EXPECT_FALSE(result);

    input = "123456789123456789123456789123456789123456789";
    result = Attribute::SafeStringToInt(input, output, base);
    EXPECT_FALSE(result);

    input = "2147483648";
    result = Attribute::SafeStringToInt(input, output, base);
    EXPECT_FALSE(result);
    
    input = "";
    result = Attribute::SafeStringToInt(input, output, base);
    EXPECT_FALSE(result);

    input = "-123456789123456789123456789123456789123456789";
    result = Attribute::SafeStringToInt(input, output, base);
    EXPECT_FALSE(result);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetByteRange_002, TestSize.Level1)
{
    Attribute attr("name", "\"");
    auto byteRange = attr.GetByteRange();
    EXPECT_EQ(byteRange.first, 0);
    EXPECT_EQ(byteRange.second, 0);
}

HWTEST_F(AttributeUnitTest, IsSingleValueSection_ValidTag, TestSize.Level0)
{
    EXPECT_TRUE(IsSingleValueSection(HlsTag::URI));
    EXPECT_TRUE(IsSingleValueSection(HlsTag::EXTXTARGETDURATION));
    EXPECT_TRUE(IsSingleValueSection(HlsTag::EXTXPLAYLISTTYPE));
}

HWTEST_F(AttributeUnitTest, IsSingleValueSection_InvalidTag, TestSize.Level0)
{
    EXPECT_FALSE(IsSingleValueSection(HlsTag::EXTXKEY));
    EXPECT_FALSE(IsSingleValueSection(HlsTag::EXTXSTART));
    EXPECT_FALSE(IsSingleValueSection(HlsTag::EXTXDISCONTINUITY));
    EXPECT_FALSE(IsSingleValueSection(HlsTag::EXTINF));
}

HWTEST_F(AttributeUnitTest, IsAttributesSection_ValidTag, TestSize.Level0)
{
    EXPECT_TRUE(IsAttributesSection(HlsTag::EXTXKEY));
    EXPECT_TRUE(IsAttributesSection(HlsTag::EXTXSTART));
    EXPECT_TRUE(IsAttributesSection(HlsTag::EXTXSTREAMINF));
    EXPECT_TRUE(IsAttributesSection(HlsTag::EXTINF));
}

HWTEST_F(AttributeUnitTest, IsAttributesSection_InvalidTag, TestSize.Level0)
{
    EXPECT_FALSE(IsAttributesSection(HlsTag::URI));
    EXPECT_FALSE(IsAttributesSection(HlsTag::EXTXDISCONTINUITY));
}

HWTEST_F(AttributeUnitTest, IsValuesListSection_ValidTag, TestSize.Level0)
{
    EXPECT_TRUE(IsValuesListSection(HlsTag::EXTINF));
}

HWTEST_F(AttributeUnitTest, IsValuesListSection_InvalidTag, TestSize.Level0)
{
    EXPECT_FALSE(IsValuesListSection(HlsTag::EXTXKEY));
    EXPECT_FALSE(IsValuesListSection(HlsTag::URI));
}

HWTEST_F(AttributeUnitTest, HexSequenceNoPrefixLong, TestSize.Level1)
{
    Attribute attr("name", "12345");
    auto result = attr.HexSequence();
    ASSERT_EQ(result.size(), static_cast<size_t>(0));
}

HWTEST_F(AttributeUnitTest, GetByteRangeQuotedWithOffset, TestSize.Level1)
{
    Attribute attr("name", "\"10@100\"");
    auto byteRange = attr.GetByteRange();
    ASSERT_EQ(byteRange.first, static_cast<std::size_t>(100));
    ASSERT_EQ(byteRange.second, static_cast<std::size_t>(10));
}

HWTEST_F(AttributeUnitTest, GetByteRangeQuotedEmpty, TestSize.Level1)
{
    Attribute attr("name", "\"\"");
    auto byteRange = attr.GetByteRange();
    ASSERT_EQ(byteRange.first, static_cast<std::size_t>(0));
    ASSERT_EQ(byteRange.second, static_cast<std::size_t>(0));
}

HWTEST_F(AttributeUnitTest, GetByteRangeQuotedAtEnd, TestSize.Level1)
{
    Attribute attr("name", "\"10@\"");
    auto byteRange = attr.GetByteRange();
    ASSERT_EQ(byteRange.first, static_cast<std::size_t>(0));
    ASSERT_EQ(byteRange.second, static_cast<std::size_t>(10));
}

HWTEST_F(AttributeUnitTest, GetResolutionXAtEnd, TestSize.Level1)
{
    Attribute attr("name", "1920x");
    auto resolution = attr.GetResolution();
    ASSERT_EQ(resolution.first, 1920);
    ASSERT_EQ(resolution.second, 0);
}

HWTEST_F(AttributeUnitTest, QuotedStringSingleQuote, TestSize.Level1)
{
    Attribute attr("name", "\"");
    ASSERT_EQ(attr.QuotedString(), "");
}

HWTEST_F(AttributeUnitTest, QuotedStringBackslashEnd, TestSize.Level1)
{
    Attribute attr("name", "\"hello\\\"");
    ASSERT_EQ(attr.QuotedString(), "hello");
}

HWTEST_F(AttributeUnitTest, ValuesListTagNoComma, TestSize.Level1)
{
    ValuesListTag testTag(HlsTag::EXTINF, "10");
    auto durationAttr = testTag.GetAttributeByName("DURATION");
    auto titleAttr = testTag.GetAttributeByName("TITLE");
    ASSERT_NE(durationAttr, nullptr);
    ASSERT_EQ(durationAttr->QuotedString(), "10");
    ASSERT_EQ(titleAttr, nullptr);
}

HWTEST_F(AttributeUnitTest, ParseEntriesCRLF, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-VERSION:3\r\n#EXT-X-TARGETDURATION:10\r\n";
    auto result = ParseEntries(hlsContent, {}, {});
    ASSERT_EQ(result.first.size(), static_cast<size_t>(2));
}

HWTEST_F(AttributeUnitTest, ParseEntriesMasterPlaylist, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=1280000\n"
        "http://example.com/low.m3u8";
    auto result = ParseEntries(hlsContent, {}, {});
    auto& tagList = result.first;
    ASSERT_EQ(tagList.size(), static_cast<size_t>(1));
    ASSERT_EQ(tagList.front()->GetType(), HlsTag::EXTXSTREAMINF);
    auto streamTag = std::static_pointer_cast<AttributesTag>(tagList.front());
    auto uriAttr = streamTag->GetAttributeByName("URI");
    ASSERT_NE(uriAttr, nullptr);
    ASSERT_EQ(uriAttr->QuotedString(), "http://example.com/low.m3u8");
}

HWTEST_F(AttributeUnitTest, ParseEntriesURIAfterExtinf, TestSize.Level1)
{
    std::string hlsContent = "#EXTINF:10,\nhttp://example.com/segment.ts";
    auto result = ParseEntries(hlsContent, {}, {});
    auto& tagList = result.first;
    ASSERT_EQ(tagList.size(), static_cast<size_t>(2));
    ASSERT_EQ(tagList.back()->GetType(), HlsTag::URI);
}

HWTEST_F(AttributeUnitTest, ParseEntriesInvalidTag, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-UNKNOWN:value\n";
    auto result = ParseEntries(hlsContent, {}, {});
    ASSERT_EQ(result.first.size(), static_cast<size_t>(0));
}

HWTEST_F(AttributeUnitTest, ParseEntriesTagNoColon, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-ENDLIST\n";
    auto result = ParseEntries(hlsContent, {}, {});
    auto& tagList = result.first;
    ASSERT_EQ(tagList.size(), static_cast<size_t>(1));
    ASSERT_EQ(tagList.front()->GetType(), HlsTag::EXTXENDLIST);
}

HWTEST_F(AttributeUnitTest, ParseEntriesNonExtHash, TestSize.Level1)
{
    std::string hlsContent = "#COMMENT\n#EXT-X-VERSION:3\n";
    auto result = ParseEntries(hlsContent, {}, {});
    auto& tagList = result.first;
    ASSERT_EQ(tagList.size(), static_cast<size_t>(1));
    ASSERT_EQ(tagList.front()->GetType(), HlsTag::EXTXVERSION);
}

HWTEST_F(AttributeUnitTest, ParseEntriesConsecutiveNewlines, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-VERSION:3\n\n#EXT-X-TARGETDURATION:10\n";
    auto result = ParseEntries(hlsContent, {}, {});
    auto& tagList = result.first;
    ASSERT_EQ(tagList.size(), static_cast<size_t>(2));
}

HWTEST_F(AttributeUnitTest, CreateTagByNameAllTypes, TestSize.Level1)
{
    auto discontinuityTag = TagFactory::CreateTagByName("ext-x-discontinuity", "");
    auto endlistTag = TagFactory::CreateTagByName("ext-x-endlist", "");
    auto iframesOnlyTag = TagFactory::CreateTagByName("ext-x-i-frames-only", "");
    auto cueOutTag = TagFactory::CreateTagByName("ext-x-cue-out", "");
    auto cueInTag = TagFactory::CreateTagByName("ext-x-cue-in", "");
    auto uriTag = TagFactory::CreateTagByName("", "http://test.com");
    auto versionTag = TagFactory::CreateTagByName("ext-x-version", "3");
    auto byterangeTag = TagFactory::CreateTagByName("ext-x-byterange", "10");
    auto dateTimeTag = TagFactory::CreateTagByName("ext-x-program-date-time", "2024-01-01");
    auto targetDurationTag = TagFactory::CreateTagByName("ext-x-targetduration", "10");
    auto mediaSeqTag = TagFactory::CreateTagByName("ext-x-media-sequence", "0");
    auto discSeqTag = TagFactory::CreateTagByName("ext-x-discontinuity-sequence", "0");
    auto playlistTypeTag = TagFactory::CreateTagByName("ext-x-playlist-type", "VOD");
    auto mapTag = TagFactory::CreateTagByName("ext-x-map", "URI=\"init.mp4\"");
    auto mediaTag = TagFactory::CreateTagByName("ext-x-media", "TYPE=AUDIO");
    auto startTag = TagFactory::CreateTagByName("ext-x-start", "TIME-OFFSET=0");
    auto iFrameStreamInfTag = TagFactory::CreateTagByName("ext-x-i-frame-stream-inf", "URI=\"iframe.m3u8\"");
    auto sessionKeyTag = TagFactory::CreateTagByName("ext-x-session-key", "METHOD=AES-128");
    auto skipTag = TagFactory::CreateTagByName("ext-x-skip", "SKIPPED-SEGMENTS=1");
    ASSERT_NE(discontinuityTag, nullptr);
    ASSERT_EQ(discontinuityTag->GetType(), HlsTag::EXTXDISCONTINUITY);
    ASSERT_NE(endlistTag, nullptr);
    ASSERT_EQ(endlistTag->GetType(), HlsTag::EXTXENDLIST);
    ASSERT_NE(iframesOnlyTag, nullptr);
    ASSERT_EQ(iframesOnlyTag->GetType(), HlsTag::EXTXIFRAMESONLY);
    ASSERT_NE(cueOutTag, nullptr);
    ASSERT_EQ(cueOutTag->GetType(), HlsTag::EXTXCUEOUT);
    ASSERT_NE(cueInTag, nullptr);
    ASSERT_EQ(cueInTag->GetType(), HlsTag::EXTXCUEIN);
    ASSERT_NE(uriTag, nullptr);
    ASSERT_EQ(uriTag->GetType(), HlsTag::URI);
    ASSERT_NE(versionTag, nullptr);
    ASSERT_EQ(versionTag->GetType(), HlsTag::EXTXVERSION);
    ASSERT_NE(byterangeTag, nullptr);
    ASSERT_EQ(byterangeTag->GetType(), HlsTag::EXTXBYTERANGE);
    ASSERT_NE(mapTag, nullptr);
    ASSERT_EQ(mapTag->GetType(), HlsTag::EXTXMAP);
    ASSERT_NE(mediaTag, nullptr);
    ASSERT_EQ(mediaTag->GetType(), HlsTag::EXTXMEDIA);
    ASSERT_NE(startTag, nullptr);
    ASSERT_EQ(startTag->GetType(), HlsTag::EXTXSTART);
    ASSERT_NE(iFrameStreamInfTag, nullptr);
    ASSERT_EQ(iFrameStreamInfTag->GetType(), HlsTag::EXTXIFRAMESTREAMINF);
    ASSERT_NE(sessionKeyTag, nullptr);
    ASSERT_EQ(sessionKeyTag->GetType(), HlsTag::EXTXSESSIONKEY);
    ASSERT_NE(skipTag, nullptr);
    ASSERT_EQ(skipTag->GetType(), HlsTag::EXTXSKIP);
}

HWTEST_F(AttributeUnitTest, ParseAttributesEscapedQuoteInValue, TestSize.Level1)
{
    AttributesTag testTag(HlsTag::EXTXKEY, "URI=\"test\\value\"");
    auto uriAttr = testTag.GetAttributeByName("URI");
    ASSERT_NE(uriAttr, nullptr);
    ASSERT_EQ(uriAttr->QuotedString(), "testvalue");
}

HWTEST_F(AttributeUnitTest, ParseAttributesBackslashOutsideQuotes, TestSize.Level1)
{
    AttributesTag testTag(HlsTag::EXTXKEY, "KEY=val\\ue");
    auto keyAttr = testTag.GetAttributeByName("KEY");
    ASSERT_NE(keyAttr, nullptr);
    ASSERT_EQ(keyAttr->QuotedString(), "val\\ue");
}

HWTEST_F(AttributeUnitTest, ParseAttributesCommaInQuotedValue, TestSize.Level1)
{
    AttributesTag testTag(HlsTag::EXTXKEY, "URI=\"test,value\"");
    auto uriAttr = testTag.GetAttributeByName("URI");
    ASSERT_NE(uriAttr, nullptr);
    ASSERT_EQ(uriAttr->QuotedString(), "test,value");
}

HWTEST_F(AttributeUnitTest, ParseAttributesOutOfRangeChar, TestSize.Level1)
{
    AttributesTag testTag(HlsTag::EXTXKEY, "KEY=value{extra");
    auto keyAttr = testTag.GetAttributeByName("KEY");
    ASSERT_NE(keyAttr, nullptr);
    ASSERT_EQ(keyAttr->QuotedString(), "valueextra");
}

HWTEST_F(AttributeUnitTest, ParseAttributesBackslashAtEnd, TestSize.Level1)
{
    AttributesTag testTag(HlsTag::EXTXKEY, "URI=\"test\\");
    const auto& attrs = testTag.GetAllAttributes();
    ASSERT_EQ(attrs.size(), static_cast<size_t>(1));
    auto uriAttr = testTag.GetAttributeByName("URI");
    ASSERT_NE(uriAttr, nullptr);
    ASSERT_EQ(uriAttr->GetName(), "URI");
}

HWTEST_F(AttributeUnitTest, ParseEntriesDefineShort, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-DEFINE:\ntest.ts";
    auto result = ParseEntries(hlsContent, {}, {});
    ASSERT_EQ(result.second.size(), static_cast<size_t>(0));
    ASSERT_EQ(result.first.size(), static_cast<size_t>(1));
}

HWTEST_F(AttributeUnitTest, ParseEntriesDefineNonAscii, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-DEFINE:NAME=\"test\xC0\",VALUE=\"out\"\ntest.ts";
    auto result = ParseEntries(hlsContent, {}, {});
    ASSERT_EQ(result.second.size(), static_cast<size_t>(0));
    ASSERT_EQ(result.first.size(), static_cast<size_t>(1));
}

HWTEST_F(AttributeUnitTest, ParseEntriesImportNotFound, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-DEFINE:IMPORT=\"notexist\"\ntest.ts";
    auto result = ParseEntries(hlsContent, {}, {});
    ASSERT_EQ(result.second.size(), static_cast<size_t>(0));
    ASSERT_EQ(result.first.size(), static_cast<size_t>(1));
}

HWTEST_F(AttributeUnitTest, ParseEntriesQueryParamNotFound, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-DEFINE:QUERYPARAM=\"notexist\"\ntest.ts";
    auto result = ParseEntries(hlsContent, {}, {});
    ASSERT_EQ(result.second.size(), static_cast<size_t>(0));
    ASSERT_EQ(result.first.size(), static_cast<size_t>(1));
}

HWTEST_F(AttributeUnitTest, ParseEntriesPlaceholderNotFound, TestSize.Level1)
{
    std::string hlsContent = "#EXT-X-DEFINE:NAME=\"test\",VALUE=\"out\"\n{$unknown}000.ts";
    auto result = ParseEntries(hlsContent, {}, {});
    ASSERT_EQ(result.second.size(), static_cast<size_t>(1));
    ASSERT_EQ(result.second["test"], "out");
    ASSERT_EQ(result.first.size(), static_cast<size_t>(0));
}

HWTEST_F(AttributeUnitTest, ParseEntriesPlaceholderNonAscii, TestSize.Level1)
{
    std::string hlsContent = "{$test\xC0}\n#EXT-X-VERSION:3\n";
    auto result = ParseEntries(hlsContent, {}, {});
    auto& tagList = result.first;
    ASSERT_EQ(tagList.size(), static_cast<size_t>(1));
    ASSERT_EQ(tagList.front()->GetType(), HlsTag::EXTXVERSION);
}

HWTEST_F(AttributeUnitTest, ParseUriQueryNonAscii, TestSize.Level1)
{
    std::string uri = "http://example.com/path?key=\xC0value";
    auto params = ParseUriQuery(uri);
    ASSERT_EQ(params.size(), static_cast<size_t>(0));
}

HWTEST_F(AttributeUnitTest, ParseUriQueryDuplicateKey, TestSize.Level1)
{
    std::string uri = "http://example.com/path?key=val1&key=val2";
    auto params = ParseUriQuery(uri);
    ASSERT_EQ(params.size(), static_cast<size_t>(1));
    ASSERT_EQ(params["key"], "val1");
}

HWTEST_F(AttributeUnitTest, ParseUriQueryInvalidPair, TestSize.Level1)
{
    std::string uri = "http://example.com/path?invalidpair&key=val";
    auto params = ParseUriQuery(uri);
    ASSERT_EQ(params.size(), static_cast<size_t>(1));
    ASSERT_EQ(params["key"], "val");
}
}