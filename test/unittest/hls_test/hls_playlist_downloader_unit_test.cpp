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
namespace {
class HlsPlayListDownloaderTestable : public HlsPlayListDownloader {
public:
    using HlsPlayListDownloader::HlsPlayListDownloader;

    void SetPlayListForTest(const std::string& playList)
    {
        playList_ = playList;
    }
};
}
const static std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
const static std::string M3U8_PATH_MASTER = "test_cbr/test_cbr.m3u8";
const static std::string M3U8_PATH_MEDIA = "test_cbr/720_1M/video_720.m3u8";
const static std::string M3U8_PATH_LIVE = "test_hls/testHlsLive.m3u8";
const static std::string M3U8_PATH_XMAP = "test_hls/testXMap.m3u8";
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

void HlsPlayListDownloaderUnitTest::SetDefaultStreamId(std::shared_ptr<HlsPlayListDownloader> downloader)
{
    int32_t videoStreamId = 0;
    int32_t audioStreamId = 0;
    int32_t subTitleStreamId = 0;
    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
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
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MASTER_LIST);
    downloader->ParseManifest("", false);
    EXPECT_GE(downloader->GetUrl(), "");
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MASTER_LIST);
    downloader->ParseManifest("http://new.url", false);
    EXPECT_GE(downloader->GetUrl(), "http://new.url");
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PARSE_MANIFEST_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MASTER_LIST);
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
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MASTER_LIST);
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    downloader->ParseManifest(testUrl, false);
    SetDefaultStreamId(downloader);
    EXPECT_EQ(3000000, downloader->GetCurBitrate());
    EXPECT_EQ(3000000, downloader->GetCurrentBitRate());
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
    SetDefaultStreamId(downloader);
    EXPECT_EQ(downloader->GetCurBitrate(), 3000000);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_004, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    SetDefaultStreamId(downloader);
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
    SetDefaultStreamId(downloader);
    EXPECT_EQ(downloader->GetSeekable(), Seekable::SEEKABLE);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    SetDefaultStreamId(downloader);
    EXPECT_EQ(downloader->GetSeekable(), Seekable::UNSEEKABLE);
}

/**
 * @tc.name: SET_MIME_TYPE_001
 * @tc.desc: test SetMimeType does not crash and Open can be called after SetMimeType
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_MIME_TYPE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // set mime type then open, expect no crash and url set
    downloader->SetMimeType("application/x-mpegURL");
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    EXPECT_EQ(downloader->GetUrl(), testUrl);
    downloader->SetMimeType("");
    EXPECT_EQ(downloader->GetUrl(), testUrl);
}

/**
 * @tc.name: GET_SEGMENT_OFFSET_001
 * @tc.desc: test GetSegmentOffset returns 0 when master is null or index out of range
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEGMENT_OFFSET_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // master_ == nullptr -> 0
    EXPECT_EQ(downloader->GetSegmentOffset(0), 0u);
    EXPECT_EQ(downloader->GetSegmentOffset(100), 0u);
}

/**
 * @tc.name: GET_HLS_DISCONTINUITY_001
 * @tc.desc: test GetHLSDiscontinuity returns false when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_HLS_DISCONTINUITY_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // master_ == nullptr -> false
    EXPECT_FALSE(downloader->GetHLSDiscontinuity());
}

/**
 * @tc.name: SET_INIT_RESOLUTION_001
 * @tc.desc: test SetInitResolution with valid and zero values does not crash
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_INIT_RESOLUTION_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // zero width/height -> no update, no crash
    downloader->SetInitResolution(0, 0);
    // valid resolution -> no crash
    downloader->SetInitResolution(1920, 1080);
    // verify via GetVideoWidth/Height before any master is set
    EXPECT_EQ(downloader->GetVideoWidth(), 0);
    EXPECT_EQ(downloader->GetVideoHeight(), 0);
}

/**
 * @tc.name: INTERRUPT_M3U8_PARSE_001
 * @tc.desc: test InterruptM3U8Parse with null master does not crash
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, INTERRUPT_M3U8_PARSE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // master_ == nullptr -> no crash
    downloader->InterruptM3U8Parse(true);
    downloader->InterruptM3U8Parse(false);
    EXPECT_EQ(downloader->GetMaster(), nullptr);
}

/**
 * @tc.name: READ_FMP4_HEADER_001
 * @tc.desc: test ReadFmp4Header returns false when master or buffer is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, READ_FMP4_HEADER_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    uint8_t buf[64];
    uint32_t readLen = 0;
    // master_ == nullptr -> false
    EXPECT_FALSE(downloader->ReadFmp4Header(buf, sizeof(buf), readLen, 0));
    // buffer == nullptr -> false (need master_ set)
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_MASTER;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    EXPECT_FALSE(downloader->ReadFmp4Header(nullptr, sizeof(buf), readLen, 0));
}

/**
 * @tc.name: IS_PARSE_FINISHED_001
 * @tc.desc: test IsParseFinished and IsParseAndNotifyFinished initial states
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_PARSE_FINISHED_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    EXPECT_FALSE(downloader->IsParseFinished());
    EXPECT_FALSE(downloader->IsParseAndNotifyFinished());
}

/**
 * @tc.name: IS_LIVE_END_001
 * @tc.desc: test IsLiveEnd returns false initially
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_END_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    EXPECT_FALSE(downloader->IsLiveEnd());
}

/**
 * @tc.name: IS_MEDIA_SAME_001
 * @tc.desc: test IsMediaSame returns true when no current audio/subtitle set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_MEDIA_SAME_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // currentAudio_ == nullptr -> true
    EXPECT_TRUE(downloader->IsMediaSame(1, HlsSegmentType::SEG_AUDIO));
    // currentSubtitles_ == nullptr -> true
    EXPECT_TRUE(downloader->IsMediaSame(2, HlsSegmentType::SEG_SUBTITLE));
}

/**
 * @tc.name: GET_BIT_RATES_001
 * @tc.desc: test GetBitRates returns empty when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_BIT_RATES_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    auto rates = downloader->GetBitRates();
    EXPECT_TRUE(rates.empty());
}

/**
 * @tc.name: GET_SEG_TYPE_001
 * @tc.desc: test GetSegType returns SEG_VIDEO for unknown streamId
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEG_TYPE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // not in any set -> SEG_VIDEO
    EXPECT_EQ(downloader->GetSegType(999), HlsSegmentType::SEG_VIDEO);
}

/**
 * @tc.name: GET_CUR_STREAM_ID_001
 * @tc.desc: test GetCurStreamId returns 0 when currentVariant_ is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_STREAM_ID_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // currentVariant_ == nullptr -> 0
    EXPECT_EQ(downloader->GetCurStreamId(), 0u);
}

/**
 * @tc.name: GET_START_INFO_001
 * @tc.desc: test GetStartInfo returns {0, false} initially
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_FALSE(info.second);
}

/**
 * @tc.name: SELECT_MEDIA_001
 * @tc.desc: test SelectMedia with null currentVariant_ does not crash
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SELECT_MEDIA_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // currentVariant_ == nullptr -> early return, no crash
    downloader->SelectMedia(1, HlsSegmentType::SEG_AUDIO);
    downloader->SelectMedia(1, HlsSegmentType::SEG_SUBTITLE);
    EXPECT_EQ(downloader->GetCurrentVariant(), nullptr);
}

/**
 * @tc.name: SET_DEFAULT_MEDIA_001
 * @tc.desc: test SetDefaultMedia with null master does not crash
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_MEDIA_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // master_ == nullptr -> early return, no crash
    downloader->SetDefaultMedia(HlsSegmentType::SEG_AUDIO);
    downloader->SetDefaultMedia(HlsSegmentType::SEG_SUBTITLE);
    EXPECT_EQ(downloader->GetMaster(), nullptr);
}

/**
 * @tc.name: GET_DEFAULT_MEDIA_STREAM_ID_001
 * @tc.desc: test GetDefaultMediaStreamId returns -1 when no variant/master
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DEFAULT_MEDIA_STREAM_ID_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    EXPECT_EQ(downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_AUDIO), -1);
    EXPECT_EQ(downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_SUBTITLE), -1);
}

/**
 * @tc.name: GET_STREAM_INFO_BY_ID_001
 * @tc.desc: test GetStreamInfoById returns nullptr when no currentVariant_
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_BY_ID_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    EXPECT_EQ(downloader->GetStreamInfoById(1), nullptr);
}
}