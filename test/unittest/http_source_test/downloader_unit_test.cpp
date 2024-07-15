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

#include "download/downloader.h"
#include "download/http_curl_client.h"
#include "gtest/gtest.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class DownloaderUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<Downloader> downloader;
};

void DownloaderUnitTest::SetUpTestCase(void)
{
}

void DownloaderUnitTest::TearDownTestCase(void)
{
}

void DownloaderUnitTest::SetUp(void)
{
    downloader = std::make_shared<Downloader>("test");
}

void DownloaderUnitTest::TearDown(void)
{
    downloader.reset();
}

HWTEST_F(DownloaderUnitTest, Downloader_Construct_nullptr, TestSize.Level1)
{
    EXPECT_NE(downloader->client_, nullptr);
}

HWTEST_F(DownloaderUnitTest, ToString_EmptyList, TestSize.Level1)
{
    list<string> lists;
    string result = ToString(lists);
    EXPECT_EQ(result, "");
}

HWTEST_F(DownloaderUnitTest, ToString_NoEmptyList, TestSize.Level1)
{
    list<string> lists = {"Hello", "World"};
    string result = ToString(lists);
    EXPECT_EQ(result, "Hello,World");
}

HWTEST_F(DownloaderUnitTest, InsertCharBefore_Test1, TestSize.Level1)
{
    string input = "hello world";
    char from = 'x';
    char preChar = 'y';
    char nextChar = 'z';
    string expected = "hello world";
    string actual = InsertCharBefore(input, from, preChar, nextChar);
    EXPECT_EQ(expected, actual);
}

HWTEST_F(DownloaderUnitTest, InsertCharBefore_Test2, TestSize.Level1)
{
    string input = "hello world";
    char from = 'l';
    char preChar = 'y';
    char nextChar = 'o';
    string expected = "heyllo woryld";
    string actual = InsertCharBefore(input, from, preChar, nextChar);
    EXPECT_EQ(expected, actual);
}

HWTEST_F(DownloaderUnitTest, InsertCharBefore_Test3, TestSize.Level1)
{
    string input = "hello world";
    char from = 'l';
    char preChar = 'y';
    char nextChar = 'l';
    string expected = "hello woryld";
    string actual = InsertCharBefore(input, from, preChar, nextChar);
    EXPECT_EQ(expected, actual);
}

HWTEST_F(DownloaderUnitTest, Trim_EmptyString, TestSize.Level1)
{
    string input = "";
    string expected = "";
    EXPECT_EQ(Trim(input), expected);
}

HWTEST_F(DownloaderUnitTest, Trim_LeadingSpaces, TestSize.Level1)
{
    string input = "   Hello";
    string expected = "Hello";
    EXPECT_EQ(Trim(input), expected);
}

HWTEST_F(DownloaderUnitTest, Trim_TrailingSpaces, TestSize.Level1)
{
    string input = "Hello   ";
    string expected = "Hello";
    EXPECT_EQ(Trim(input), expected);
}

HWTEST_F(DownloaderUnitTest, Trim_LeadingAndTrailingSpaces, TestSize.Level1)
{
    string input = "  Hello   ";
    string expected = "Hello";
    EXPECT_EQ(Trim(input), expected);
}

HWTEST_F(DownloaderUnitTest, Trim_NoSpaces, TestSize.Level1)
{
    string input = "Hello";
    string expected = "Hello";
    EXPECT_EQ(Trim(input), expected);
}

HWTEST_F(DownloaderUnitTest, IsRegexValid_Test1, TestSize.Level1)
{
    string regex = "";
    bool result = IsRegexValid(regex);
    EXPECT_EQ(result, false);
}

HWTEST_F(DownloaderUnitTest, IsRegexValid_Test2, TestSize.Level1)
{
    string regex = "!@#$%^&*()";
    bool result = IsRegexValid(regex);
    EXPECT_EQ(result, false);
}

HWTEST_F(DownloaderUnitTest, IsRegexValid_Test3, TestSize.Level1)
{
    string regex = "abc123";
    bool result = IsRegexValid(regex);
    EXPECT_EQ(result, true);
}

HWTEST_F(DownloaderUnitTest, ReplaceCharacters_01, TestSize.Level1)
{
    string input = "";
    string output = ReplaceCharacters(input);
    EXPECT_EQ(output, "");
}

HWTEST_F(DownloaderUnitTest, ReplaceCharacters_02, TestSize.Level1)
{
    string input = "abc";
    string output = ReplaceCharacters(input);
    EXPECT_EQ(output, "abc");
}

HWTEST_F(DownloaderUnitTest, ReplaceCharacters_03, TestSize.Level1)
{
    string input = "a.b.c";
    string output = ReplaceCharacters(input);
    EXPECT_EQ(output, "a\\.b\\.c");
}

HWTEST_F(DownloaderUnitTest, ReplaceCharacters_04, TestSize.Level1)
{
    string input = "a\\b\\c";
    string output = ReplaceCharacters(input);
    EXPECT_EQ(output, "a\\b\\c");
}

HWTEST_F(DownloaderUnitTest, ReplaceCharacters_05, TestSize.Level1)
{
    string input = "a\\b.c";
    string output = ReplaceCharacters(input);
    EXPECT_EQ(output, "a\\b\\.c");
}

HWTEST_F(DownloaderUnitTest, IsMatch_01, TestSize.Level1)
{
    string str = "test";
    string patternStr = "";
    bool result = IsMatch(str, patternStr);
    EXPECT_FALSE(result);
}

HWTEST_F(DownloaderUnitTest, IsMatch_02, TestSize.Level1)
{
    string str = "test";
    string patternStr = "*";
    bool result = IsMatch(str, patternStr);
    EXPECT_TRUE(result);
}

HWTEST_F(DownloaderUnitTest, IsMatch_03, TestSize.Level1)
{
    string str = "test";
    string patternStr = "test";
    bool result = IsMatch(str, patternStr);
    EXPECT_TRUE(result);
}

HWTEST_F(DownloaderUnitTest, IsMatch_04, TestSize.Level1)
{
    string str = "test";
    string patternStr = "t.*";
    bool result = IsMatch(str, patternStr);
    EXPECT_FALSE(result);
}

HWTEST_F(DownloaderUnitTest, IsMatch_05, TestSize.Level1)
{
    string str = "test";
    string patternStr = "e.*";
    bool result = IsMatch(str, patternStr);
    EXPECT_FALSE(result);
}

HWTEST_F(DownloaderUnitTest, IsExcluded_01, TestSize.Level1)
{
    string str = "test";
    string exclusions = "";
    string split = ",";
    EXPECT_FALSE(IsExcluded(str, exclusions, split));
}

HWTEST_F(DownloaderUnitTest, IsExcluded_02, TestSize.Level1)
{
    string str = "test";
    string exclusions = "test,example";
    string split = ",";
    EXPECT_TRUE(IsExcluded(str, exclusions, split));
}

HWTEST_F(DownloaderUnitTest, IsExcluded_03, TestSize.Level1)
{
    string str = "test";
    string exclusions = "example,sample";
    string split = ",";
    EXPECT_FALSE(IsExcluded(str, exclusions, split));
}

HWTEST_F(DownloaderUnitTest, IsExcluded_04, TestSize.Level1)
{
    string str = "test";
    string exclusions = "example";
    string split = ",";
    EXPECT_FALSE(IsExcluded(str, exclusions, split));
}

}
}
}
}