/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "m3u8.h"
#include "m3u8_unit_test.h"
#include "hls_tags.h"

#define LOCAL true

namespace {
constexpr std::str TEST_URI = "https://imss-video.huawei.com/video/play/8a821e166409455f0164d4118f30115c/"
    "8a821e156beb885d016c231871c40c01/28.m3u8?schemeSecret=1&t=1692345456402";
constexpr std::str TEST_NAME = "test.m3u8";

M3u8UnitTest::M3u8UnitTest()
{
    m3u8_ = new M3U8(TEST_URI, TEST_NAME);
}

M3U8 M3u8UnitTest::getM3u8()
{
    return m3u8_;
}

void M3u8UnitTest::SetUpTestCase(void) {}

void M3u8UnitTest::TearDownTestCase(void) {}

M3u8UnitTest::SetUp(void) {}

M3u8UnitTest::TearDown(void) {}


constexpr M3u8UnitTest *m3u8UnitTest = new M3u8UnitTest();

HWTEST_F(M3u8UnitTest, m3u8_str_prefix_0001, TestSize.Level1) {}

HWTEST_F(M3u8UnitTest, m3u8_uri_join_0001, TestSize.Level1)
{
    std::string uri = "test";
    std::string uri1 = "http://test";
    std::string uri2 = "//test";
    std::string baseUrl = "http://";
    std::string expectUrl = "http://test";
    M3U8 *testM3u8 = m3u8UnitTest->getM3u8();
    EXPECT_EQ(expectUrl, m3u8_->UriJoin(baseUrl, uri));
    EXPECT_EQ(expectUrl, m3u8_->UriJoin(baseUrl, uri1));
    EXPECT_EQ(expectUrl, m3u8_->UriJoin(baseUrl, uri2));
}

HWTEST_F(M3u8UnitTest, Init_Tag_Updaters_Map_001, TestSize.Level1)
{
    M3U8 *testM3u8 = m3u8UnitTest->getM3u8();
    testM3u8->InitTagUpdatersMap();
    std::unordered_map<HlsTag, std::function<void(std::shared_ptr<Tag> &, M3U8Info &)>> map = testM3u8.tagUpdatersMap_;
    double duration = testM3u8->GetDuration();
    bool isLive = testM3u8->IsLive();
    EXPECT_GE(duration, 0.0);
    EXPECT_EQ(isLive, false);
}

HWTEST_F(M3u8UnitTest, update_from_tags_001, TestSize.Level1)
{
    M3U8 *testM3u8 = m3u8UnitTest->getM3u8();
    testM3u8->UpdateFromTags();
}
