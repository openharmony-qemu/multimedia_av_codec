/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "m3u8_unit_test.h"

#define LOCAL true

namespace OHOS::Media::Plugins::HttpPlugin {
using namespace testing::ext;
using namespace std;

void M3u8UnitTest::SetUpTestCase(void) {}

void M3u8UnitTest::TearDownTestCase(void) {}

void M3u8UnitTest::SetUp(void) {}

void M3u8UnitTest::TearDown(void) {}

HWTEST_F(M3u8UnitTest, Init_Tag_Updaters_Map_001, TestSize.Level1)
{
    double duration = testM3u8->GetDuration();
    bool isLive = testM3u8->IsLive();
    EXPECT_GE(duration, 0.0);
    EXPECT_EQ(isLive, false);
}

HWTEST_F(M3u8UnitTest, is_live_001, TestSize.Level1)
{
    EXPECT_NE(testM3u8->GetDuration(), 0.0);
}

HWTEST_F(M3u8UnitTest, parse_key_001, TestSize.Level1)
{
    testM3u8->ParseKey(std::make_shared<AttributesTag>(HlsTag::EXTXKEY, tagAttribute));
    testM3u8->DownloadKey();
}

HWTEST_F(M3u8UnitTest, base_64_decode_001, TestSize.Level1)
{
    EXPECT_EQ(testM3u8->Base64Decode((uint8_t *)0x20000550, (uint32_t)16, (uint8_t *)0x20000550, (uint32_t *)16), true);
    EXPECT_EQ(testM3u8->Base64Decode((uint8_t *)0x20000550, (uint32_t)10, (uint8_t *)0x20000550, (uint32_t *)10), true);
    EXPECT_EQ(testM3u8->Base64Decode(nullptr, (uint32_t)10, (uint8_t *)0x20000550, (uint32_t *)10), true);
}

bool StrHasPrefix(const std::string &str, const std::string &prefix);
std::string UriJoin(std::string &baseUrl, const std::string &uri);

// StrHasPrefix 函数的单元测试
HWTEST_F(M3u8UnitTest, STR_HAS_PREFIX_TEST_001, TestSize.Level1)
{
    std::string str = "hello world";
    std::string prefix = "hello";
    EXPECT_TRUE(StrHasPrefix(str, prefix));
}

HWTEST_F(M3u8UnitTest, STR_HAS_PREFIX_TEST_002, TestSize.Level1)
{
    std::string str = "hello world";
    std::string prefix = "world";
    EXPECT_FALSE(StrHasPrefix(str, prefix));
}

// UriJoin 函数的单元测试
HWTEST_F(M3u8UnitTest, URI_JOIN_TEST_001, TestSize.Level1)
{
    std::string baseUrl = "http://example.com";
    std::string uri = "https://test.com";
    EXPECT_EQ(UriJoin(baseUrl, uri), uri);
}

HWTEST_F(M3u8UnitTest, URI_JOIN_TEST_002, TestSize.Level1)
{
    std::string baseUrl = "http://example.com";
    std::string uri = "//example.org";
    EXPECT_EQ(UriJoin(baseUrl, uri), "http://example.org");
}

HWTEST_F(M3u8UnitTest, URI_JOIN_TEST_003, TestSize.Level1)
{
    std::string baseUrl = "http://example.com/path/";
    std::string uri = "page.html";
    EXPECT_EQ(UriJoin(baseUrl, uri), "http://example.com/path/page.html");
}
}