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
#include "hls_playlist_downloader_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace testing::ext;
using namespace std;
const static std::string TEST_URI_PATH = "http://127.0.0.1:4666/";
const static std::string M3U8_PATH_1 = "test_cbr/test_cbr.m3u8";
const static std::map<std::string, std::string> httpHeader = {
    {"User-Agent", "userAgent"},
    {"Referer", "DEF"},
};

void HlsPlayListDownloaderUnitTest::SetUpTestCase(void) {}

void HlsPlayListDownloaderUnitTest::TearDownTestCase(void) {}

void HlsPlayListDownloaderUnitTest::SetUp(void) {}

void HlsPlayListDownloaderUnitTest::TearDown(void) {}

HWTEST_F(HlsPlayListDownloaderUnitTest, TEST_OPEN, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    std::map<std::string, std::string> tmpHttpHeader;
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_1;
    tmpHttpHeader["Content-Type"] = "application/x-mpegURL";
    downloader.Open(testUrl, tmpHttpHeader);
    EXPECT_EQ(testUrl, downloader.GetUrl());
    EXPECT_EQ(nullptr, downloader.GetMaster());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION, TestSize.Level1)
{
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_1;
    printf("----%s------", testUrl.c_str());
    HlsPlayListDownloader downloader;
    EXPECT_EQ(0, downloader.GetDuration());
    downloader.Open(testUrl, httpHeader);
    EXPECT_GE(downloader.GetDuration(), 0);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_EMPTY, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    downloader.ParseManifest("");
    EXPECT_GE(downloader.GetUrl(), "");
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_001, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    downloader.ParseManifest("http://new.url");
    EXPECT_GE(downloader.GetUrl(), "http://new.url");
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_002, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader.Open(testUrl, httpHeader);
    downloader.ParseManifest(testUrl);
    EXPECT_FALSE(downloader.GetMaster()->isSimple_);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_003, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_1;
    downloader.Open(testUrl, httpHeader);
    downloader.ParseManifest(testUrl);
    EXPECT_FALSE(downloader.GetMaster()->isSimple_);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_004, TestSize.Level1)
{
    HlsPlayListDownloader downloader;

    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    downloader.Open(testUrl, httpHeader);
    downloader.ParseManifest(testUrl);
    EXPECT_FALSE(downloader.GetMaster()->isSimple_);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_005, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    downloader.ParseManifest(testUrl);
    EXPECT_NE(downloader.GetMaster()->uri_, "");
    EXPECT_EQ(downloader.GetMaster()->uri_, testUrl);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_BITRATE_001, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_1;
    std::string testUrl1 = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    downloader.Open(testUrl1, httpHeader);
    downloader.ParseManifest(testUrl1);
    EXPECT_EQ(0, downloader.GetCurBitrate());
    EXPECT_EQ(0, downloader.GetCurrentBitRate());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_BITRATE_002, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    uint64_t bitRate = downloader.GetCurBitrate();
    EXPECT_EQ(0, bitRate);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_VEDIO_WIDTH, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    int width = downloader.GetVedioWidth();
    EXPECT_EQ(0, width);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_VEDIO_HEIGHT, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    int height = downloader.GetVedioHeight();
    EXPECT_EQ(0, height);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE, TestSize.Level1)
{
    HlsPlayListDownloader downloader;
    bool isLive = downloader.IsLive();
    EXPECT_FALSE(isLive);
}
}