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
#include "http_server_demo.h"

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace testing::ext;
using namespace std;
const static std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
const static std::string M3U8_PATH_MASTER = "test_cbr/test_cbr.m3u8";
const static std::string M3U8_PATH_MEDIA = "test_cbr/720_1M/video_720.m3u8";
const static std::string M3U8_PATH_LIVE = "test_hls/testHlsLive.m3u8";
const static std::string M3U8_PATH_XMAP = "test_hls/testXMap.m3u8";
const static std::string M3U8_PATH_ENCODE = "test_hls/testHLSEncode_session_key.m3u8";
const static std::map<std::string, std::string> httpHeader = {
    {"User-Agent", "userAgent"},
    {"Referer", "DEF"},
};
const static std::string MEDIA_LIST =
R"(#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:9
#EXT-X-MEDIA-SEQUENCE:0
#EXTINF:5.000000,
video_0.ts
#EXTINF:6.400000,
video_1.ts
#EXTINF:5.000000,
video_2.ts
#EXTINF:5.520000,
video_3.ts
#EXTINF:3.640000,
video_4.ts
#EXTINF:5.600000,
video_5.ts
#EXTINF:5.600000,
video_6.ts
#EXTINF:7.840000,
video_7.ts
#EXTINF:2.760000,
video_8.ts
#EXTINF:5.000000,
video_9.ts
#EXT-X-ENDLIST
)";
const static std::string MASTER_LIST =
R"(#EXTM3U
#EXT-X-STREAM-INF:PROGRAM-ID=1, BANDWIDTH=1000000
720_1M/video_720.m3u8
#EXT-X-STREAM-INF:PROGRAM-ID=1, BANDWIDTH=2000000
720_2M/video_720.m3u8
#EXT-X-STREAM-INF:PROGRAM-ID=1, BANDWIDTH=3000000
1080_3M/video_1080.m3u8
)";
std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
void HlsPlayListDownloaderUnitTest::SetUpTestCase(void) {}

void HlsPlayListDownloaderUnitTest::TearDownTestCase(void) {}

void HlsPlayListDownloaderUnitTest::SetUp(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
}

void HlsPlayListDownloaderUnitTest::TearDown(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

HWTEST_F(HlsPlayListDownloaderUnitTest, TEST_OPEN, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    std::map<std::string, std::string> tmpHttpHeader;
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    tmpHttpHeader["Content-Type"] = "application/x-mpegURL";
    downloader->Open(testUrl, tmpHttpHeader);
    EXPECT_EQ(testUrl, downloader->GetUrl());
    EXPECT_EQ(nullptr, downloader->GetMaster());
    EXPECT_EQ(nullptr, downloader->GetCurrentVariant());
    EXPECT_EQ(nullptr, downloader->GetNewVariant());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION, TestSize.Level1)
{
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    printf("----%s------", testUrl.c_str());
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    EXPECT_EQ(0, downloader->GetDuration());
    downloader->Open(testUrl, httpHeader);
    EXPECT_GE(downloader->GetDuration(), 0);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_EMPTY, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    downloader->ParseManifest("", false);
    EXPECT_GE(downloader->GetUrl(), "");
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    downloader->ParseManifest("http://new.url", false);
    EXPECT_GE(downloader->GetUrl(), "http://new.url");
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    downloader->ParseManifest(testUrl, false);
    EXPECT_FALSE(downloader->GetMaster()->isSimple_);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_MASTER_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_TRUE(downloader->GetMaster()->isSimple_);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_MASTER_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_EQ(downloader->GetMaster()->playList_, MASTER_LIST);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_MASTER_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_EQ(downloader->GetMaster()->uri_, testUrl);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_MEDIA_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MEDIA;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_TRUE(downloader->GetMaster()->isSimple_);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_MEDIA_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MEDIA;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_EQ(downloader->GetMaster()->playList_, MEDIA_LIST);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_MEDIA_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MEDIA;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_EQ(downloader->GetMaster()->uri_, testUrl);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_BITRATE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    downloader->ParseManifest(testUrl, false);
    EXPECT_EQ(0, downloader->GetCurBitrate());
    EXPECT_EQ(0, downloader->GetCurrentBitRate());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_BITRATE_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    uint64_t bitRate = downloader->GetCurBitrate();
    EXPECT_EQ(0, bitRate);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_VEDIO_WIDTH, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    int width = downloader->GetVideoWidth();
    EXPECT_EQ(0, width);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_VEDIO_HEIGHT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    int height = downloader->GetVideoHeight();
    EXPECT_EQ(0, height);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    bool isLive = downloader->IsLive();
    EXPECT_FALSE(isLive);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GetLiveUpdateGap001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    size_t time = downloader->GetLiveUpdateGap();
    EXPECT_EQ(0, time);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_001, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MEDIA;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_GE(downloader->GetDuration(), 1);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_GE(downloader->GetDuration(), -1);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_003, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_EQ(downloader->GetCurBitrate(), 3000000);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_004, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_TRUE(downloader->IsBitrateSame(3000000));
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_005, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->IsBitrateSame(1000000);
    downloader->SelectBitRate(1000000);
    EXPECT_EQ(downloader->GetCurBitrate(), 1000000);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_001, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_FALSE(downloader->IsLive());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_TRUE(downloader->IsLive());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_HLS_FMP4_001, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_FALSE(downloader->IsHlsFmp4());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_HLS_FMP4_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_XMAP;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_TRUE(downloader->IsHlsFmp4());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_001, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_EQ(downloader->GetSeekable(), Seekable::SEEKABLE);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_EQ(downloader->GetSeekable(), Seekable::UNSEEKABLE);
}
}