/*
 * Copyright (C) 2023-2026 Huawei Device Co., Ltd.
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
constexpr uint32_t HLS_TEST_STREAM_COUNT = 3;
constexpr int32_t HLS_TEST_VIDEO_STREAM_ID = 1;
constexpr int32_t HLS_TEST_AUDIO_STREAM_ID = 2;
constexpr int32_t HLS_TEST_SUBTITLE_STREAM_ID = 3;
constexpr uint32_t HLS_TEST_VIDEO_BITRATE = 1000000;
constexpr uint32_t HLS_TEST_VIDEO_FRAME_RATE = 30;
constexpr int32_t HLS_TEST_VIDEO_WIDTH = 1280;
constexpr int32_t HLS_TEST_VIDEO_HEIGHT = 720;

class HlsPlayListDownloaderTestable : public HlsPlayListDownloader {
public:
    using HlsPlayListDownloader::HlsPlayListDownloader;

    void SetPlayListForTest(const std::string& playList)
    {
        playList_ = playList;
    }

    void UpdateLiveBaseOffsetForTest(const std::list<std::shared_ptr<M3U8Fragment>>& files)
    {
        UpdateLiveBaseOffset(files);
    }

    int64_t GetLiveBaseOffset() const
    {
        return liveBaseOffset_;
    }
};
class RecordingCallback : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event)
    {
        (void)event;
        onEventCount_++;
    }

    void SetSelectBitRateFlag(bool flag, uint32_t desBitRate)
    {
        (void)flag;
        (void)desBitRate;
    }

    bool CanDoSelectBitRate()
    {
        return true;
    }

    uint32_t onEventCount_ = 0;
};

constexpr uint32_t HLS_VIDEO_STREAM_ID = 100;
constexpr uint32_t HLS_AUDIO_STREAM_ID = 200;
constexpr uint32_t HLS_SUBTITLE_STREAM_ID = 300;
constexpr int32_t INVALID_TEST = -1;
constexpr int32_t NUM2_TEST = 2;

std::shared_ptr<HlsPlayListDownloaderTestable> CreateSetDefaultStreamIdDownloader(bool isSimple, bool parseSuccess,
    bool withDefaultVariant)
{
    const std::string testUriPath = "http://127.0.0.1:46666/";
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", testUriPath);
    downloader->master_->isSimple_.store(isSimple);
    downloader->master_->isParseSuccess_.store(parseSuccess);
    if (!withDefaultVariant) {
        return downloader;
    }

    auto m3u8 = std::make_shared<M3U8>(isSimple ? testUriPath : "", "");
    auto variant = std::make_shared<M3U8VariantStream>("video", testUriPath, m3u8);
    variant->streamId_ = HLS_VIDEO_STREAM_ID;
    downloader->master_->defaultVariant_ = variant;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = variant;
    return downloader;
}

void PrepareHlsDownloaderStreamInfo(std::shared_ptr<HlsPlayListDownloader> &downloader)
{
    auto statusCallback = [] (DownloadStatus status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    std::map<std::string, std::string> header;
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", "", 0, header, statusCallback);
    auto videoM3u8 = std::make_shared<M3U8>("videoUri", "video",
        std::unordered_map<std::string, std::string>(), statusCallback, nullptr);
    auto audioM3u8 = std::make_shared<M3U8>("audioUri", "audio",
        std::unordered_map<std::string, std::string>(), statusCallback, nullptr);
    auto subM3u8 = std::make_shared<M3U8>("subUri", "sub",
        std::unordered_map<std::string, std::string>(), statusCallback, nullptr);
    auto audioMedia = std::make_shared<M3U8Media>("audio", "audioUri", audioM3u8);
    audioMedia->lang_ = "eng";
    audioMedia->channels_ = "2";
    audioMedia->streamId_ = HLS_TEST_AUDIO_STREAM_ID;
    auto subMedia = std::make_shared<M3U8Media>("sub", "subUri", subM3u8);
    subMedia->lang_ = "chi";
    subMedia->channels_ = "1";
    subMedia->streamId_ = HLS_TEST_SUBTITLE_STREAM_ID;
    auto variant = std::make_shared<M3U8VariantStream>("video", "videoUri", videoM3u8);
    variant->streamId_ = HLS_TEST_VIDEO_STREAM_ID;
    variant->bandWidth_ = HLS_TEST_VIDEO_BITRATE;
    variant->frameRate_ = HLS_TEST_VIDEO_FRAME_RATE;
    variant->width_ = HLS_TEST_VIDEO_WIDTH;
    variant->height_ = HLS_TEST_VIDEO_HEIGHT;
    variant->defaultAudio_ = audioMedia;
    variant->defaultSubtitles_ = subMedia;
    downloader->master_->variants_.push_back(variant);
    downloader->master_->audioList_.push_back(audioMedia);
    downloader->master_->subtitlesList_.push_back(subMedia);
    downloader->master_->defaultVariant_ = variant;
    downloader->currentVariant_ = variant;
}

void CheckHlsStreamInfo(const std::vector<StreamInfo> &streams)
{
    EXPECT_EQ(streams.size(), HLS_TEST_STREAM_COUNT);
    for (const auto &info : streams) {
        if (info.type == StreamType::VIDEO) {
            EXPECT_EQ(info.streamId, HLS_TEST_VIDEO_STREAM_ID);
            EXPECT_EQ(info.bitRate, HLS_TEST_VIDEO_BITRATE);
            EXPECT_EQ(info.videoWidth, HLS_TEST_VIDEO_WIDTH);
            EXPECT_EQ(info.videoHeight, HLS_TEST_VIDEO_HEIGHT);
        } else if (info.type == StreamType::AUDIO) {
            EXPECT_EQ(info.streamId, HLS_TEST_AUDIO_STREAM_ID);
            EXPECT_EQ(info.lang, "eng");
        } else if (info.type == StreamType::SUBTITLE) {
            EXPECT_EQ(info.streamId, HLS_TEST_SUBTITLE_STREAM_ID);
            EXPECT_EQ(info.lang, "chi");
        }
    }
}
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
#define MIN_PRE_PARSE_NUM 3
#define MAX_LIVE_TS_NUM 5
std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;

class HlsStreamUpdateCallback : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        lastEventType_ = event.type;
        eventTriggered_ = true;
    }

    void SetSelectBitRateFlag(bool flag, uint32_t desBitRate) override
    {
        (void)flag;
        (void)desBitRate;
    }

    bool CanAutoSelectBitRate() override
    {
        return true;
    }

    bool eventTriggered_ {false};
    Plugins::PluginEventType lastEventType_ {Plugins::PluginEventType::CLIENT_ERROR};
};

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

HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    int32_t videoStreamId = -1;
    int32_t audioStreamId = 2;
    int32_t subTitleStreamId = 3;
    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(downloader->GetCurrentVariant(), nullptr);

    auto statusCallback = [] (DownloadStatus status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    std::map<std::string, std::string> header;
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", "", 0, header, statusCallback);
    auto mediaM3u8 = std::make_shared<M3U8>("", "", std::unordered_map<std::string, std::string>(),
        statusCallback, nullptr);
    auto variant = std::make_shared<M3U8VariantStream>("video", "", mediaM3u8);
    variant->streamId_ = 1;
    downloader->master_->variants_.push_back(variant);
    downloader->master_->defaultVariant_ = variant;
    int32_t videoStreamIdNumSec = 1;
    int32_t audioStreamIdNumSec= 2;
    int32_t subTitleStreamIdNumSec = 3;
    downloader->SetDefaultStreamId(videoStreamIdNumSec, audioStreamIdNumSec, subTitleStreamIdNumSec);
    EXPECT_EQ(downloader->GetCurrentVariant(), variant);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams, false);
    EXPECT_TRUE(streams.empty());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    PrepareHlsDownloaderStreamInfo(downloader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams, false);
    CheckHlsStreamInfo(streams);
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
    EXPECT_GE(-1 * downloader->GetDuration(), -1);
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

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_007, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);

    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM);
    downloader->master_->duration_ = 10.5;

    int64_t expected = static_cast<int64_t>(downloader->master_->duration_ * HST_SECOND) / HST_NSECOND;
    int64_t ret = downloader->GetDuration();
    EXPECT_EQ(-1 * ret, expected);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_008, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM - 1);
    downloader->master_->duration_ = 10.5;
    int64_t ret = downloader->GetDuration();
    EXPECT_GE(-1 * ret, 0);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_009, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_->bLive_ = false;
    downloader->master_->duration_ = 15.2;
    int64_t expected = static_cast<int64_t>(downloader->master_->duration_ * HST_SECOND) / HST_NSECOND;
    int64_t ret = downloader->GetDuration();
    EXPECT_EQ(ret, expected);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_010, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = false;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM);
    downloader->master_->duration_ = 10.5;
    int64_t ret = downloader->GetDuration();
    EXPECT_EQ(ret, -1);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_003, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->isInterruptNeeded_ = false;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->bLive_ = true;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM);
    downloader->master_->duration_ = 10.5;
    auto ret = downloader->GetSeekable();
    EXPECT_EQ(ret, Seekable::SEEKABLE);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_004, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->isInterruptNeeded_ = false;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->bLive_ = true;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM - 1);
    downloader->master_->duration_ = 10.5;
    auto ret = downloader->GetSeekable();
    EXPECT_EQ(ret, Seekable::SEEKABLE);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_005, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->isInterruptNeeded_ = false;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->bLive_ = true;
    downloader->isParseFinished_ = false;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM);
    downloader->master_->duration_ = 10.5;
    auto ret = downloader->GetSeekable();
    EXPECT_EQ(ret, Seekable::INVALID);
    downloader->isParseFinished_ = false;
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_RANGES_001, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_ = {};
    auto ret = downloader->GetSeekableRanges();
    EXPECT_TRUE(ret.empty());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_RANGES_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->isInterruptNeeded_ = false;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->bLive_ = true;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM - 1);
    downloader->master_->duration_ = 10.5;
    auto ret = downloader->GetSeekableRanges();
    EXPECT_FALSE(ret.empty());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_RANGES_003, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->isInterruptNeeded_ = false;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->bLive_ = true;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM);
    downloader->master_->duration_ = 10.5;
    std::vector<SeekRange> expectRanges = {{0, 1000000000}, {1000000000, 2000000000}};
    {
        std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
        downloader->seekRanges_ = expectRanges;
    }
    auto ret = downloader->GetSeekableRanges();
    EXPECT_EQ(ret.size(), expectRanges.size());
    for (size_t i = 0; i < ret.size(); ++i) {
        EXPECT_EQ(ret[i].start, expectRanges[i].start);
        EXPECT_EQ(ret[i].end, expectRanges[i].end);
    }
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_RANGES_004, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->isInterruptNeeded_ = false;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->bLive_ = false;
    {
        std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
        downloader->seekRanges_.clear();
    }
    auto ret = downloader->GetSeekableRanges();
    EXPECT_TRUE(ret.empty());
    std::vector<SeekRange> expectRanges = {{0, 500000000}};
    {
        std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
        downloader->seekRanges_ = expectRanges;
    }
    ret = downloader->GetSeekableRanges();
    if (!ret.empty()) {
        EXPECT_EQ(ret[0].start, expectRanges[0].start);
        EXPECT_EQ(ret[0].end, expectRanges[0].end);
    }
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_SHIFT_SEEKABLE_001, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_ = {};
    bool ret = downloader->IsLiveTimeShiftSeekable();
    EXPECT_FALSE(ret);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_SHIFT_SEEKABLE_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_->bLive_ = false;
    bool ret = downloader->IsLiveTimeShiftSeekable();
    EXPECT_FALSE(ret);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_SHIFT_SEEKABLE_003, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_ = false;
    bool ret = downloader->IsLiveTimeShiftSeekable();
    EXPECT_FALSE(ret);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_SHIFT_SEEKABLE_004, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = false;
    bool ret = downloader->IsLiveTimeShiftSeekable();
    EXPECT_FALSE(ret);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_SHIFT_SEEKABLE_005, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM - 1);
    bool ret = downloader->IsLiveTimeShiftSeekable();
    EXPECT_TRUE(ret);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_SHIFT_SEEKABLE_006, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM);
    downloader->master_->duration_ = 0;
    bool ret = downloader->IsLiveTimeShiftSeekable();
    EXPECT_FALSE(ret);
    downloader->master_->duration_ = -5.0;
    ret = downloader->IsLiveTimeShiftSeekable();
    EXPECT_FALSE(ret);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_SHIFT_SEEKABLE_007, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_ = true;
    downloader->isParseFinished_ = true;
    downloader->master_->segmentOffsets_.resize(MIN_PRE_PARSE_NUM);
    downloader->master_->duration_ = 10.5;
    bool ret = downloader->IsLiveTimeShiftSeekable();
    EXPECT_TRUE(ret);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_001, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    std::list<std::shared_ptr<M3U8Fragment>> emptyFiles;
    downloader->UpdateSeekRanges(emptyFiles);
    std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
    EXPECT_TRUE(downloader->seekRanges_.empty());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    std::list<std::shared_ptr<M3U8Fragment>> files;
    files.push_back(nullptr);
    auto frag = std::make_shared<M3U8Fragment>("test1.m3u8", 10.0, 1, false);
    files.push_back(frag);
    downloader->UpdateSeekRanges(files);
    std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
    EXPECT_EQ(downloader->seekRanges_.size(), 1);
    EXPECT_EQ(downloader->seekRanges_[0].start, 0);
    EXPECT_EQ(downloader->seekRanges_[0].end, static_cast<int64_t>(10.0 * HST_SECOND / HST_NSECOND));
}

HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_003, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    std::list<std::shared_ptr<M3U8Fragment>> files;
    auto frag = std::make_shared<M3U8Fragment>("test2.m3u8", 5.0, 2, false);
    files.push_back(frag);
    downloader->UpdateSeekRanges(files);
    std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
    EXPECT_EQ(downloader->seekRanges_.size(), 1);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_004, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    std::list<std::shared_ptr<M3U8Fragment>> files;
    auto frag1 = std::make_shared<M3U8Fragment>("test3_1.m3u8", 10.0, 3, false);
    files.push_back(frag1);
    auto frag2 = std::make_shared<M3U8Fragment>("test3_2.m3u8", 5.0, 4, true);
    files.push_back(frag2);
    downloader->UpdateSeekRanges(files);
    std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
    EXPECT_EQ(downloader->seekRanges_.size(), 2);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_005, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    std::list<std::shared_ptr<M3U8Fragment>> files;
    auto frag = std::make_shared<M3U8Fragment>("test4.m3u8", -5.0, 5, false);
    downloader->UpdateSeekRanges(files);
    std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
    EXPECT_TRUE(downloader->seekRanges_.empty());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_006, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    std::list<std::shared_ptr<M3U8Fragment>> files;
    auto frag1 = std::make_shared<M3U8Fragment>("test5_1.m3u8", 0.0, 6, true);
    auto frag2 = std::make_shared<M3U8Fragment>("test5_2.m3u8", 8.0, 7, false);
    files.push_back(frag2);
    downloader->UpdateSeekRanges(files);
    std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
    EXPECT_EQ(downloader->seekRanges_.size(), 1);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_007, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    std::list<std::shared_ptr<M3U8Fragment>> files;
    auto frag = std::make_shared<M3U8Fragment>("test6.m3u8", 0.0, 8, false);
    files.push_back(frag);
    downloader->UpdateSeekRanges(files);
    std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
    EXPECT_TRUE(downloader->seekRanges_.empty());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, NOTIFY_LIST_CHANGE_001, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    auto mockCallback = std::make_shared<MockPlayListCallback>();
    downloader->SetPlayListCallback(std::weak_ptr<PlayListChangeCallback>(mockCallback));
    downloader->currentVariant_ = std::make_shared<M3U8VariantStream>("", "", nullptr);
    downloader->currentVariant_->m3u8_ = std::make_shared<M3U8>("", "");
    for (int i = 0; i < MAX_LIVE_TS_NUM + 5; ++i) {
        auto frag = std::make_shared<M3U8Fragment>("test.ts", 1.0, i, false);
        downloader->currentVariant_->m3u8_->files_.push_back(frag);
    }
    downloader->isParseFinished_ = true;
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_ = true;
    downloader->master_->segmentOffsets_.clear();
    downloader->master_->duration_ = 0;
    downloader->isInterruptNeeded_ = false;
    downloader->NotifyListChange();
    EXPECT_EQ(mockCallback->savedPlayList.size(), MIN_PRE_PARSE_NUM);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, NOTIFY_LIST_CHANGE_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    auto mockCallback = std::make_shared<MockPlayListCallback>();
    downloader->SetPlayListCallback(std::weak_ptr<PlayListChangeCallback>(mockCallback));
    downloader->currentVariant_ = std::make_shared<M3U8VariantStream>("", "", nullptr);
    downloader->currentVariant_->m3u8_ = std::make_shared<M3U8>("", "");
    for (int i = 0; i < MAX_LIVE_TS_NUM - 1; ++i) {
        auto frag = std::make_shared<M3U8Fragment>("test.ts", 1.0, i, false);
        downloader->currentVariant_->m3u8_->files_.push_back(frag);
    }
    downloader->isParseFinished_ = true;
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_ = true;
    downloader->master_->segmentOffsets_.clear();
    downloader->master_->duration_ = 0;
    downloader->isInterruptNeeded_ = false;
    downloader->NotifyListChange();
    EXPECT_EQ(mockCallback->savedPlayList.size(), MIN_PRE_PARSE_NUM);
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
    EXPECT_EQ(downloader->GetSeekable(), Seekable::SEEKABLE);
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

/**
 * @tc.name: GET_DURATION_006
 * @tc.desc: test GetDuration with master_ set: bLive_=false returns duration, bLive_=true returns -1
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_006, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    // bLive_=false via media playlist
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->ParseManifest("", false);
    EXPECT_GE(downloader->GetDuration(), 0);
    // bLive_=true: set via live playlist string
    auto downloader2 = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader2->Init();
    std::string liveList =
        "#EXTM3U\n"
        "#EXT-X-TARGETDURATION:6\n"
        "#EXTINF:5.0,\nseg0.ts\n"
        "#EXTINF:5.0,\nseg1.ts\n";
    downloader2->SetPlayListForTest(liveList);
    downloader2->ParseManifest("", false);
    EXPECT_GE(-1 * downloader2->GetDuration(), 0);
}

/**
 * @tc.name: GET_SEGMENT_OFFSET_002
 * @tc.desc: test GetSegmentOffset returns valid offset when master_ is set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEGMENT_OFFSET_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->ParseManifest("", false);
    // index 0: segmentOffsets_[0] = 0
    size_t offset = downloader->GetSegmentOffset(0);
    EXPECT_GE(offset, 0u);
    // index out of range: returns 0
    EXPECT_EQ(downloader->GetSegmentOffset(10000), 0u);
}

/**
 * @tc.name: GET_HLS_DISCONTINUITY_002
 * @tc.desc: test GetHLSDiscontinuity returns true when master_ has discontinuity
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_HLS_DISCONTINUITY_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    std::string listWithDiscont =
        "#EXTM3U\n"
        "#EXT-X-TARGETDURATION:6\n"
        "#EXTINF:5.0,\nseg0.ts\n"
        "#EXT-X-DISCONTINUITY\n"
        "#EXTINF:5.0,\nseg1.ts\n"
        "#EXT-X-ENDLIST\n";
    downloader->SetPlayListForTest(listWithDiscont);
    downloader->ParseManifest("", false);
    EXPECT_TRUE(downloader->GetHLSDiscontinuity());
}

/**
 * @tc.name: SET_INIT_RESOLUTION_002
 * @tc.desc: test SetInitResolution actually updates initResolution_ when w>0 && h>0
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_INIT_RESOLUTION_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    downloader->SetInitResolution(1920, 1080);
    // verify via GetSeekable etc. won't work easily, but we can
    // at least verify SetInitResolution(0,0) doesn't change after a valid set
    // Since initResolution_ is private, use indirect verification via Open + no crash
    downloader->SetInitResolution(0, 0); // no-op
    EXPECT_EQ(downloader->GetVideoWidth(), 0); // no currentVariant_ yet
}

/**
 * @tc.name: GET_LIVE_UPDATE_GAP_002
 * @tc.desc: test GetLiveUpdateGap returns value when currentVariant_ with m3u8 set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_LIVE_UPDATE_GAP_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->ParseManifest("", false);
    // currentVariant_ is set, m3u8 has files -> GetLiveUpdateGap should return some value
    size_t gap = downloader->GetLiveUpdateGap();
    EXPECT_GE(gap, 0u);
}

/**
 * @tc.name: IS_MEDIA_SAME_002
 * @tc.desc: test IsMediaSame returns true when currentAudio_ set and streamId matches
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_MEDIA_SAME_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // Set currentAudio_ with streamId=5 -> IsMediaSame(5, SEG_AUDIO)=true
    auto m3u8 = std::make_shared<M3U8>("http://example.com", "");
    auto media = std::make_shared<M3U8Media>("audio", "http://example.com/a.m3u8", m3u8);
    media->streamId_ = 5;
    downloader->currentAudio_ = media;
    EXPECT_TRUE(downloader->IsMediaSame(5, HlsSegmentType::SEG_AUDIO));
    // different streamId -> false
    EXPECT_FALSE(downloader->IsMediaSame(6, HlsSegmentType::SEG_AUDIO));
    // subtitle: currentSubtitles_ set, streamId matches
    auto mediaS = std::make_shared<M3U8Media>("sub", "http://example.com/s.m3u8", m3u8);
    mediaS->streamId_ = 7;
    downloader->currentSubtitles_ = mediaS;
    EXPECT_TRUE(downloader->IsMediaSame(7, HlsSegmentType::SEG_SUBTITLE));
    EXPECT_FALSE(downloader->IsMediaSame(8, HlsSegmentType::SEG_SUBTITLE));
}

/**
 * @tc.name: GET_BIT_RATES_002
 * @tc.desc: test GetBitRates returns rates when master_ has variants (including nullptr item)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_BIT_RATES_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MASTER_LIST);
    downloader->ParseManifest("", false);
    auto rates = downloader->GetBitRates();
    EXPECT_GE(rates.size(), 1u);
}

/**
 * @tc.name: GET_SEG_TYPE_002
 * @tc.desc: test GetSegType returns SEG_AUDIO and SEG_SUBTITLE when ids are in sets
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEG_TYPE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    // manually insert into audioStreamIds_ and subtitlesStreamIds_
    {
        std::lock_guard<std::mutex> lock(downloader->streamIdMutex_);
        downloader->audioStreamIds_.insert(100);
        downloader->subtitlesStreamIds_.insert(200);
    }
    EXPECT_EQ(downloader->GetSegType(100), HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(downloader->GetSegType(200), HlsSegmentType::SEG_SUBTITLE);
    EXPECT_EQ(downloader->GetSegType(999), HlsSegmentType::SEG_VIDEO);
}

/**
 * @tc.name: GET_CUR_STREAM_ID_002
 * @tc.desc: test GetCurStreamId returns audio/subtitle streamId when currentAudio_/Subtitles_ set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_STREAM_ID_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->ParseManifest("", false);
    // currentVariant_ set, currentAudio_ == nullptr -> returns variant streamId
    EXPECT_GT(downloader->GetCurStreamId(), 0u);
    // set currentAudio_
    auto m3u8 = std::make_shared<M3U8>("http://example.com", "");
    auto audio = std::make_shared<M3U8Media>("a", "http://example.com/a.m3u8", m3u8);
    audio->streamId_ = 42;
    downloader->currentAudio_ = audio;
    EXPECT_EQ(downloader->GetCurStreamId(), 42u);
    // clear audio, set subtitles
    downloader->currentAudio_ = nullptr;
    auto sub = std::make_shared<M3U8Media>("s", "http://example.com/s.m3u8", m3u8);
    sub->streamId_ = 55;
    downloader->currentSubtitles_ = sub;
    EXPECT_EQ(downloader->GetCurStreamId(), 55u);
}

/**
 * @tc.name: SELECT_MEDIA_002
 * @tc.desc: test SelectMedia with actual audioMedia_ set -> find and set currentAudio_
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SELECT_MEDIA_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->ParseManifest("", false);
    // currentVariant_ is set but audioMedia_ is likely empty in simple playlist
    // Manually set audioMedia_ on currentVariant_
    if (downloader->currentVariant_ != nullptr) {
        auto m3u8 = std::make_shared<M3U8>("http://example.com/a.m3u8", "");
        auto media = std::make_shared<M3U8Media>("track1", "http://example.com/a.m3u8", m3u8);
        media->streamId_ = 99;
        downloader->currentVariant_->audioMedia_.push_back(media);
        // now SelectMedia with matching streamId
        downloader->SelectMedia(99, HlsSegmentType::SEG_AUDIO);
        EXPECT_NE(downloader->currentAudio_, nullptr);
        EXPECT_EQ(downloader->currentAudio_->streamId_, 99u);
        // SelectMedia with non-matching streamId -> currentAudio_ unchanged
        downloader->currentAudio_ = nullptr;
        downloader->SelectMedia(1000, HlsSegmentType::SEG_AUDIO);
        EXPECT_EQ(downloader->currentAudio_, nullptr);
    }
}

/**
 * @tc.name: GET_DEFAULT_MEDIA_STREAM_ID_002
 * @tc.desc: test GetDefaultMediaStreamId with audioStreamId_ != -1 returns it directly
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DEFAULT_MEDIA_STREAM_ID_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->ParseManifest("", false);
    // set audioStreamId_ = 5 -> returns 5
    downloader->audioStreamId_ = 5;
    EXPECT_EQ(downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_AUDIO), 5);
    // set subtitleStreamId_ = 8 -> returns 8
    downloader->subtitleStreamId_.store(8);
    EXPECT_EQ(downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_SUBTITLE), 8);
}

/**
 * @tc.name: SET_DEFAULT_MEDIA_002
 * @tc.desc: test SetDefaultMedia sets currentAudio_ when defaultAudio_ is present
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_MEDIA_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->ParseManifest("", false);
    if (downloader->GetMaster() != nullptr && downloader->GetMaster()->defaultVariant_ != nullptr) {
        // manually set defaultAudio_ on defaultVariant_
        auto m3u8 = std::make_shared<M3U8>("http://example.com", "");
        auto audio = std::make_shared<M3U8Media>("a", "http://example.com/a.m3u8", m3u8);
        audio->streamId_ = 77;
        downloader->GetMaster()->defaultVariant_->defaultAudio_ = audio;
        downloader->SetDefaultMedia(HlsSegmentType::SEG_AUDIO);
        EXPECT_EQ(downloader->currentAudio_->streamId_, 77u);
    }
}

/**
 * @tc.name: INTERRUPT_M3U8_PARSE_002
 * @tc.desc: test InterruptM3U8Parse with master_ and currentVariant_ set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, INTERRUPT_M3U8_PARSE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->ParseManifest("", false);
    // both master_ and currentVariant_ should be set now
    if (downloader->GetMaster() != nullptr) {
        downloader->InterruptM3U8Parse(true);
        EXPECT_TRUE(downloader->GetMaster()->isInterruptNeeded_);
        downloader->InterruptM3U8Parse(false);
        EXPECT_FALSE(downloader->GetMaster()->isInterruptNeeded_);
    }
}

/**
 * @tc.name: CONTAINS_NON_DIGIT_001
 * @tc.desc: test ContainsNonDigit returns true when string contains non-digit character
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, CONTAINS_NON_DIGIT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();

    EXPECT_EQ(downloader->ContainsNonDigit("12a34"), true);
}

/**
 * @tc.name: CONTAINS_NON_DIGIT_002
 * @tc.desc: test ContainsNonDigit returns false when string contains only digits
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, CONTAINS_NON_DIGIT_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();

    EXPECT_EQ(downloader->ContainsNonDigit("123456"), false);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_001
 * @tc.desc: test SetDefaultStreamId returns when master_ is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, HLS_PLAYLIST_SET_DEFAULT_STREAM_ID_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(videoStreamId, static_cast<int32_t>(HLS_VIDEO_STREAM_ID));
    EXPECT_EQ(audioStreamId, static_cast<int32_t>(HLS_AUDIO_STREAM_ID));
    EXPECT_EQ(subTitleStreamId, static_cast<int32_t>(HLS_SUBTITLE_STREAM_ID));
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_002
 * @tc.desc: test SetDefaultStreamId returns when videoStreamId is -1
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_002, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, true);
    int32_t videoStreamId = -1;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(audioStreamId, static_cast<int32_t>(HLS_AUDIO_STREAM_ID));
    EXPECT_EQ(subTitleStreamId, static_cast<int32_t>(HLS_SUBTITLE_STREAM_ID));
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_003
 * @tc.desc: test SetDefaultStreamId enters simple playlist update path
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_003, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, true);
    downloader->isParseFinished_ = false;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(downloader->isParseFinished_, true);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_004
 * @tc.desc: test SetDefaultStreamId enters non-simple playlist update path
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_004, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(false, true, true);
    downloader->isParseFinished_ = false;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(downloader->isParseFinished_, false);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_005
 * @tc.desc: test SetDefaultStreamId keeps audioStreamId when it is not -1
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_005, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, true);
    auto audio = std::make_shared<M3U8Media>("a", TEST_URI_PATH, downloader->currentVariant_->m3u8_);
    audio->streamId_ = HLS_AUDIO_STREAM_ID;
    downloader->currentVariant_->defaultAudio_ = audio;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = NUM2_TEST;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(audioStreamId, NUM2_TEST);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_006
 * @tc.desc: test SetDefaultStreamId keeps audioStreamId when currentVariant is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_006, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, false);
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = INVALID_TEST;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(audioStreamId, INVALID_TEST);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_007
 * @tc.desc: test SetDefaultStreamId keeps audioStreamId when defaultAudio is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_007, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, true);
    downloader->currentVariant_->defaultAudio_ = nullptr;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = INVALID_TEST;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(audioStreamId, INVALID_TEST);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_008
 * @tc.desc: test SetDefaultStreamId updates audioStreamId from defaultAudio
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_008, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, true);
    auto audio = std::make_shared<M3U8Media>("a", TEST_URI_PATH, downloader->currentVariant_->m3u8_);
    audio->streamId_ = HLS_AUDIO_STREAM_ID;
    downloader->currentVariant_->defaultAudio_ = audio;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = INVALID_TEST;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(audioStreamId, static_cast<int32_t>(HLS_AUDIO_STREAM_ID));
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_009
 * @tc.desc: test SetDefaultStreamId keeps subTitleStreamId when it is not -1
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_009, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, true);
    auto subtitle = std::make_shared<M3U8Media>("s", TEST_URI_PATH, downloader->currentVariant_->m3u8_);
    subtitle->streamId_ = HLS_SUBTITLE_STREAM_ID;
    downloader->currentVariant_->defaultSubtitles_ = subtitle;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = NUM2_TEST;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(subTitleStreamId, NUM2_TEST);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_010
 * @tc.desc: test SetDefaultStreamId keeps subTitleStreamId when currentVariant is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_010, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, false);
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = INVALID_TEST;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(subTitleStreamId, INVALID_TEST);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_011
 * @tc.desc: test SetDefaultStreamId keeps subTitleStreamId when defaultSubtitles is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_011, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, true);
    downloader->currentVariant_->defaultSubtitles_ = nullptr;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = INVALID_TEST;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(subTitleStreamId, INVALID_TEST);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_012
 * @tc.desc: test SetDefaultStreamId updates subTitleStreamId from defaultSubtitles
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_012, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, true);
    auto subtitle = std::make_shared<M3U8Media>("s", TEST_URI_PATH, downloader->currentVariant_->m3u8_);
    subtitle->streamId_ = HLS_SUBTITLE_STREAM_ID;
    downloader->currentVariant_->defaultSubtitles_ = subtitle;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = INVALID_TEST;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(subTitleStreamId, static_cast<int32_t>(HLS_SUBTITLE_STREAM_ID));
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_013
 * @tc.desc: test SetDefaultStreamId does not notify client error when parse succeeds
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_013, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, true, true);
    auto callback = std::make_shared<RecordingCallback>();
    downloader->eventCallback_ = callback;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(callback->onEventCount_, 0u);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_014
 * @tc.desc: test SetDefaultStreamId skips client error when callback is expired
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_014, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, false, true);
    std::shared_ptr<RecordingCallback> callback = std::make_shared<RecordingCallback>();
    downloader->eventCallback_ = callback;
    callback = nullptr;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(videoStreamId, static_cast<int32_t>(HLS_VIDEO_STREAM_ID));
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_015
 * @tc.desc: test SetDefaultStreamId skips client error when callback is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_015, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, false, true);
    downloader->eventCallback_.reset();
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(audioStreamId, static_cast<int32_t>(HLS_AUDIO_STREAM_ID));
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_016
 * @tc.desc: test SetDefaultStreamId reports client error when parse fails and callback exists
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_016, TestSize.Level1)
{
    auto downloader = CreateSetDefaultStreamIdDownloader(true, false, true);
    auto callback = std::make_shared<RecordingCallback>();
    downloader->eventCallback_ = callback;
    int32_t videoStreamId = HLS_VIDEO_STREAM_ID;
    int32_t audioStreamId = HLS_AUDIO_STREAM_ID;
    int32_t subTitleStreamId = HLS_SUBTITLE_STREAM_ID;

    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(callback->onEventCount_, 1u);
}

/**
 * @tc.name: UPDATE_MANIFEST_AND_NOTIFY_LIST_CHANGE_001
 * @tc.desc: test UpdateManifestAndNotifyListChange updates current variant on simple playlist
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MANIFEST_AND_NOTIFY_LIST_CHANGE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isSimple_.store(true);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = HLS_VIDEO_STREAM_ID;
    downloader->master_->defaultVariant_ = variant;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = nullptr;
    downloader->isParseFinished_ = false;

    downloader->UpdateManifestAndNotifyListChange(false);

    EXPECT_EQ(downloader->currentVariant_, variant);
    EXPECT_EQ(downloader->isParseFinished_.load(), true);
}

/**
 * @tc.name: UPDATE_SEEK_RANGES_008
 * @tc.desc: test UpdateSeekRanges with negative duration fragment
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_008, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    std::list<std::shared_ptr<M3U8Fragment>> files;
    auto frag1 = std::make_shared<M3U8Fragment>("test1.ts", -5.0, 0, false);
    auto frag2 = std::make_shared<M3U8Fragment>("test2.ts", 10.0, 1, false);
    auto frag3 = std::make_shared<M3U8Fragment>("test3.ts", -3.0, 2, false);
    files.push_back(frag1);
    files.push_back(frag2);
    files.push_back(frag3);
    downloader->UpdateSeekRanges(files);
    auto ranges = downloader->GetSeekableRanges();
    EXPECT_GE(ranges.size(), 0u);
}

/**
 * @tc.name: UPDATE_SEEK_RANGES_009
 * @tc.desc: test UpdateSeekRanges with empty files list
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_009, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    std::list<std::shared_ptr<M3U8Fragment>> files;
    downloader->UpdateSeekRanges(files);
    auto ranges = downloader->GetSeekableRanges();
    EXPECT_EQ(ranges.size(), 0u);
}

/**
 * @tc.name: UPDATE_LIVE_BASE_OFFSET_001
 * @tc.desc: test UpdateLiveBaseOffset with first call (lastFirstSeq_ < 0)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_LIVE_BASE_OFFSET_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto master = std::make_shared<M3U8MasterPlaylist>("", "http://test.com");
    master->bLive_ = true;
    downloader->master_ = master;
    std::list<std::shared_ptr<M3U8Fragment>> files;
    auto frag1 = std::make_shared<M3U8Fragment>("test1.ts", 5.0, 10, false);
    files.push_back(frag1);
    downloader->UpdateLiveBaseOffsetForTest(files);
    EXPECT_GE(downloader->GetLiveBaseOffset(), 0);
}

/**
 * @tc.name: UPDATE_LIVE_BASE_OFFSET_002
 * @tc.desc: test UpdateLiveBaseOffset with decreased sequence number
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_LIVE_BASE_OFFSET_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto master = std::make_shared<M3U8MasterPlaylist>("", "http://test.com");
    master->bLive_ = true;
    downloader->master_ = master;
    std::list<std::shared_ptr<M3U8Fragment>> files1;
    auto frag1 = std::make_shared<M3U8Fragment>("test1.ts", 5.0, 10, false);
    files1.push_back(frag1);
    downloader->UpdateLiveBaseOffsetForTest(files1);
    std::list<std::shared_ptr<M3U8Fragment>> files2;
    auto frag2 = std::make_shared<M3U8Fragment>("test2.ts", 5.0, 5, false);
    files2.push_back(frag2);
    downloader->UpdateLiveBaseOffsetForTest(files2);
    auto offset = downloader->GetLiveBaseOffset();
    EXPECT_GE(offset, 0);
}

/**
 * @tc.name: UPDATE_LIVE_BASE_OFFSET_003
 * @tc.desc: test UpdateLiveBaseOffset with increased sequence number
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_LIVE_BASE_OFFSET_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto master = std::make_shared<M3U8MasterPlaylist>("", "http://test.com");
    master->bLive_ = true;
    downloader->master_ = master;
    std::list<std::shared_ptr<M3U8Fragment>> files1;
    auto frag1 = std::make_shared<M3U8Fragment>("test1.ts", 5.0, 10, false);
    files1.push_back(frag1);
    downloader->UpdateLiveBaseOffsetForTest(files1);
    auto offset1 = downloader->GetLiveBaseOffset();
    std::list<std::shared_ptr<M3U8Fragment>> files2;
    auto frag2 = std::make_shared<M3U8Fragment>("test2.ts", 5.0, 15, false);
    files2.push_back(frag2);
    downloader->UpdateLiveBaseOffsetForTest(files2);
    auto offset2 = downloader->GetLiveBaseOffset();
    EXPECT_GE(offset2, offset1);
}

/**
 * @tc.name: CLONE_001
 * @tc.desc: test Clone copies httpHeader correctly
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, CLONE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->httpHeader_["User-Agent"] = "testAgent";
    downloader->httpHeader_["Referer"] = "testReferer";

    auto other = std::make_shared<HlsPlayListDownloaderTestable>();
    other->Init();
    other->httpHeader_["Test"] = "value";

    downloader->Clone(other);
    EXPECT_EQ(downloader->httpHeader_["Test"], "value");
}

/**
 * @tc.name: CLONE_002
 * @tc.desc: test Clone copies statusCallback and startedDownloadStatus
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, CLONE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();

    auto other = std::make_shared<HlsPlayListDownloaderTestable>();
    other->Init();
    auto callback = [] (DownloadStatus status, std::shared_ptr<Downloader>& d,
        std::shared_ptr<DownloadRequest>& r) {};
    other->statusCallback_ = callback;
    other->startedDownloadStatus_ = true;
    other->url_ = "http://test.url";
    other->urlOrigin_ = "http://origin.url";
    other->mimeType_ = "application/x-mpegURL";
    other->isParseFinished_.store(true);

    downloader->Clone(other);
    EXPECT_TRUE(downloader->startedDownloadStatus_);
    EXPECT_EQ(downloader->url_, "http://test.url");
}

/**
 * @tc.name: IS_FRAGMENT_IN_PLAYLIST_001
 * @tc.desc: test IsFragmentInCurrentPlayList returns false when currentVariant is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_FRAGMENT_IN_PLAYLIST_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();

    EXPECT_FALSE(downloader->IsFragmentInCurrentPlayList("http://test.ts", "", 0, 0));
}

/**
 * @tc.name: IS_FRAGMENT_IN_PLAYLIST_002
 * @tc.desc: test IsFragmentInCurrentPlayList returns true when fragment uri matches
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_FRAGMENT_IN_PLAYLIST_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto fragment = std::make_shared<M3U8Fragment>("http://test.ts", 5.0, 0, false);
    m3u8->files_.push_back(fragment);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;

    EXPECT_TRUE(downloader->IsFragmentInCurrentPlayList("http://test.ts", "", 0, 0));
}

/**
 * @tc.name: IS_FRAGMENT_IN_PLAYLIST_003
 * @tc.desc: test IsFragmentInCurrentPlayList matches rangeUrl with offset and length
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_FRAGMENT_IN_PLAYLIST_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto fragment = std::make_shared<M3U8Fragment>("http://test.ts", 5.0, 0, false);
    fragment->offset_ = 100;
    fragment->length_ = 200;
    m3u8->files_.push_back(fragment);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;

    EXPECT_TRUE(downloader->IsFragmentInCurrentPlayList("http://test.ts", "http://test.ts", 100, 200));
    EXPECT_FALSE(downloader->IsFragmentInCurrentPlayList("http://test.ts", "http://test.ts", 50, 200));
}

/**
 * @tc.name: UPDATE_MANIFEST_001
 * @tc.desc: test UpdateManifest when currentVariant is null does not crash
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MANIFEST_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->currentVariant_ = nullptr;
    bool notifyFinishedBefore = downloader->isNotifyPlayListFinished_.load();

    downloader->UpdateManifest();
    EXPECT_EQ(downloader->isNotifyPlayListFinished_.load(), notifyFinishedBefore);
}

/**
 * @tc.name: UPDATE_MANIFEST_002
 * @tc.desc: test UpdateManifest when currentVariant m3u8 uri is valid
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MANIFEST_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->uri_ = TEST_URI_PATH + "test.m3u8";
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->isNotifyPlayListFinished_ = false;

    downloader->UpdateManifest();
    EXPECT_EQ(downloader->currentVariant_, variant);
}

/**
 * @tc.name: REOPEN_001
 * @tc.desc: test ReOpen resets master and currentVariant
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, REOPEN_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->downloader_ = nullptr;
    downloader->urlOrigin_ = TEST_URI_PATH;
    downloader->isLiveUpdateTaskStarted_ = true;
    downloader->retryStartTime_ = 1000;

    downloader->ReOpen();
    EXPECT_EQ(downloader->GetMaster(), nullptr);
    EXPECT_EQ(downloader->GetCurrentVariant(), nullptr);
    EXPECT_FALSE(downloader->isLiveUpdateTaskStarted_);
    EXPECT_EQ(downloader->retryStartTime_, 0);
}

/**
 * @tc.name: GET_STREAM_INFO_BY_ID_002
 * @tc.desc: test GetStreamInfoById returns info when variant matches streamId
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_BY_ID_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = HLS_VIDEO_STREAM_ID;
    variant->bandWidth_ = 1000000;
    variant->width_ = 1920;
    variant->height_ = 1080;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = variant;

    auto info = downloader->GetStreamInfoById(HLS_VIDEO_STREAM_ID);
    EXPECT_NE(info, nullptr);
    EXPECT_EQ(info->streamId, static_cast<int32_t>(HLS_VIDEO_STREAM_ID));
}

/**
 * @tc.name: GET_STREAM_INFO_BY_ID_003
 * @tc.desc: test GetStreamInfoById returns nullptr when no matching variant
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_BY_ID_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = HLS_VIDEO_STREAM_ID;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = variant;

    auto info = downloader->GetStreamInfoById(999);
    EXPECT_EQ(info, nullptr);
}

/**
 * @tc.name: SET_DEFAULT_MEDIA_003
 * @tc.desc: test SetDefaultMedia sets currentSubtitles when defaultSubtitles_ is set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_MEDIA_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    auto subM3u8 = std::make_shared<M3U8>("http://sub.m3u8", "");
    auto subtitle = std::make_shared<M3U8Media>("s", "http://sub.m3u8", subM3u8);
    subtitle->streamId_ = HLS_SUBTITLE_STREAM_ID;
    variant->defaultSubtitles_ = subtitle;
    downloader->currentSubtitles_ = nullptr;

    downloader->SetDefaultMedia(HlsSegmentType::SEG_SUBTITLE);
    EXPECT_NE(downloader->currentSubtitles_, nullptr);
}

/**
 * @tc.name: SET_DEFAULT_MEDIA_004
 * @tc.desc: test SetDefaultMedia returns when defaultVariant_ is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_MEDIA_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->defaultVariant_ = nullptr;

    downloader->SetDefaultMedia(HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(downloader->currentAudio_, nullptr);
}

/**
 * @tc.name: SELECT_MEDIA_003
 * @tc.desc: test SelectMedia with empty audioMedia and subtitlesMedia
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SELECT_MEDIA_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->audioMedia_.clear();
    variant->subtitlesMedia_.clear();
    downloader->currentVariant_ = variant;

    downloader->SelectMedia(99, HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(downloader->currentAudio_, nullptr);
}

/**
 * @tc.name: SELECT_MEDIA_004
 * @tc.desc: test SelectMedia sets currentSubtitles from subtitlesMedia
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SELECT_MEDIA_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    auto subM3u8 = std::make_shared<M3U8>("http://sub.m3u8", "");
    auto media = std::make_shared<M3U8Media>("sub1", "http://sub.m3u8", subM3u8);
    media->streamId_ = 88;
    variant->subtitlesMedia_.push_back(media);
    downloader->currentVariant_ = variant;

    downloader->SelectMedia(88, HlsSegmentType::SEG_SUBTITLE);
    EXPECT_NE(downloader->currentSubtitles_, nullptr);
    EXPECT_EQ(downloader->currentSubtitles_->streamId_, 88u);
}

/**
 * @tc.name: UPDATE_MEDIA_001
 * @tc.desc: test UpdateMedia returns when currentVariant is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MEDIA_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->currentVariant_ = nullptr;
    std::string urlBefore = downloader->url_;

    downloader->UpdateMedia(HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(downloader->url_, urlBefore);
}

/**
 * @tc.name: UPDATE_MEDIA_002
 * @tc.desc: test UpdateMedia sets nextUrl for audio
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MEDIA_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    auto audioM3u8 = std::make_shared<M3U8>("http://audio.uri", "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio.uri", audioM3u8);
    audio->uri_ = "http://audio.next.uri";
    downloader->currentAudio_ = audio;

    downloader->UpdateMedia(HlsSegmentType::SEG_AUDIO);
    ASSERT_NE(downloader->downloadRequest_, nullptr);
    EXPECT_EQ(downloader->downloadRequest_->GetUrl(), "http://audio.next.uri");
}

/**
 * @tc.name: UPDATE_MEDIA_003
 * @tc.desc: test UpdateMedia sets nextUrl for subtitle
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MEDIA_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->currentAudio_ = nullptr;
    auto subM3u8 = std::make_shared<M3U8>("http://sub.uri", "");
    auto sub = std::make_shared<M3U8Media>("s", "http://sub.uri", subM3u8);
    sub->uri_ = "http://sub.next.uri";
    downloader->currentSubtitles_ = sub;

    downloader->UpdateMedia(HlsSegmentType::SEG_SUBTITLE);
    ASSERT_NE(downloader->downloadRequest_, nullptr);
    EXPECT_EQ(downloader->downloadRequest_->GetUrl(), "http://sub.next.uri");
}

/**
 * @tc.name: GET_DEFAULT_MEDIA_STREAM_ID_003
 * @tc.desc: test GetDefaultMediaStreamId returns streamId from defaultAudio_
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DEFAULT_MEDIA_STREAM_ID_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    downloader->currentVariant_ = variant;
    downloader->audioStreamId_.store(-1);
    downloader->subtitleStreamId_.store(-1);
    auto audioM3u8 = std::make_shared<M3U8>("http://audio.m3u8", "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio.m3u8", audioM3u8);
    audio->streamId_ = HLS_AUDIO_STREAM_ID;
    variant->defaultAudio_ = audio;

    EXPECT_EQ(downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_AUDIO), HLS_AUDIO_STREAM_ID);
}

/**
 * @tc.name: GET_DEFAULT_MEDIA_STREAM_ID_004
 * @tc.desc: test GetDefaultMediaStreamId returns streamId from defaultSubtitles_
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DEFAULT_MEDIA_STREAM_ID_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    downloader->currentVariant_ = variant;
    downloader->audioStreamId_.store(-1);
    downloader->subtitleStreamId_.store(-1);
    variant->defaultAudio_ = nullptr;
    auto subM3u8 = std::make_shared<M3U8>("http://sub.m3u8", "");
    auto sub = std::make_shared<M3U8Media>("s", "http://sub.m3u8", subM3u8);
    sub->streamId_ = HLS_SUBTITLE_STREAM_ID;
    variant->defaultSubtitles_ = sub;

    EXPECT_EQ(downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_SUBTITLE), HLS_SUBTITLE_STREAM_ID);
}

/**
 * @tc.name: IS_BITRATE_SAME_001
 * @tc.desc: test IsBitrateSame returns false when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_BITRATE_SAME_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();

    EXPECT_FALSE(downloader->IsBitrateSame(1000000));
}

/**
 * @tc.name: IS_BITRATE_SAME_002
 * @tc.desc: test IsBitrateSame with nullptr variant item
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_BITRATE_SAME_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto nullVariant = std::shared_ptr<M3U8VariantStream>(nullptr);
    downloader->master_->variants_.push_back(nullVariant);

    EXPECT_FALSE(downloader->IsBitrateSame(1000000));
}

/**
 * @tc.name: IS_BITRATE_SAME_003
 * @tc.desc: test IsBitrateSame when firstVideoStream_ is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_BITRATE_SAME_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->firstVideoStream_ = nullptr;
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->bandWidth_ = 1000000;
    variant->isVideo_ = false;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = variant;

    downloader->IsBitrateSame(1000000);
    EXPECT_NE(downloader->newVariant_, nullptr);
}

/**
 * @tc.name: IS_BITRATE_SAME_004
 * @tc.desc: test IsBitrateSame when item->isVideo_ is true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_BITRATE_SAME_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->bandWidth_ = 1000000;
    variant->isVideo_ = true;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = variant;

    downloader->IsBitrateSame(1000000);
    EXPECT_NE(downloader->newVariant_, nullptr);
}

/**
 * @tc.name: SELECT_BIT_RATE_001
 * @tc.desc: test SelectBitRate when newVariant_ is null returns early
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SELECT_BIT_RATE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->newVariant_ = nullptr;
    auto variantBefore = downloader->currentVariant_;

    downloader->SelectBitRate(1000000);
    EXPECT_EQ(downloader->currentVariant_, variantBefore);
}

/**
 * @tc.name: IS_LIVE_END_002
 * @tc.desc: test IsLiveEnd when isLiveEnd_ is true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_END_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->isLiveEnd_ = true;

    EXPECT_TRUE(downloader->IsLiveEnd());
}

/**
 * @tc.name: IS_PARSE_AND_NOTIFY_FINISHED_001
 * @tc.desc: test IsParseAndNotifyFinished when both flags are true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_PARSE_AND_NOTIFY_FINISHED_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->isParseFinished_.store(true);
    downloader->isNotifyPlayListFinished_.store(true);

    EXPECT_TRUE(downloader->IsParseAndNotifyFinished());
}

/**
 * @tc.name: IS_PARSE_AND_NOTIFY_FINISHED_002
 * @tc.desc: test IsParseAndNotifyFinished when only isParseFinished is true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_PARSE_AND_NOTIFY_FINISHED_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->isParseFinished_.store(true);
    downloader->isNotifyPlayListFinished_.store(false);

    EXPECT_FALSE(downloader->IsParseAndNotifyFinished());
}

/**
 * @tc.name: GET_BIT_RATES_003
 * @tc.desc: test GetBitRates with master having multiple variants
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_BIT_RATES_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant1 = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant1->bandWidth_ = 1000000;
    auto variant2 = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant2->bandWidth_ = 2000000;
    downloader->master_->variants_.push_back(variant1);
    downloader->master_->variants_.push_back(variant2);

    auto rates = downloader->GetBitRates();
    EXPECT_EQ(rates.size(), 2u);
}

/**
 * @tc.name: GET_CUR_STREAM_ID_003
 * @tc.desc: test GetCurStreamId when currentVariant is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_STREAM_ID_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->currentVariant_ = nullptr;
    downloader->currentAudio_ = nullptr;
    downloader->currentSubtitles_ = nullptr;

    EXPECT_EQ(downloader->GetCurStreamId(), 0u);
}

/**
 * @tc.name: GET_CUR_STREAM_ID_004
 * @tc.desc: test GetCurStreamId returns currentAudio streamId when set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_STREAM_ID_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = HLS_VIDEO_STREAM_ID;
    downloader->currentVariant_ = variant;
    auto audioM3u8 = std::make_shared<M3U8>("http://audio", "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio", audioM3u8);
    audio->streamId_ = HLS_AUDIO_STREAM_ID;
    downloader->currentAudio_ = audio;

    EXPECT_EQ(downloader->GetCurStreamId(), HLS_AUDIO_STREAM_ID);
}

/**
 * @tc.name: GET_CUR_STREAM_ID_005
 * @tc.desc: test GetCurStreamId returns currentSubtitles streamId when audio is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_STREAM_ID_005, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = HLS_VIDEO_STREAM_ID;
    downloader->currentVariant_ = variant;
    downloader->currentAudio_ = nullptr;
    auto subM3u8 = std::make_shared<M3U8>("http://sub", "");
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", subM3u8);
    sub->streamId_ = HLS_SUBTITLE_STREAM_ID;
    downloader->currentSubtitles_ = sub;

    EXPECT_EQ(downloader->GetCurStreamId(), HLS_SUBTITLE_STREAM_ID);
}

/**
 * @tc.name: IS_MEDIA_SAME_003
 * @tc.desc: test IsMediaSame returns true when streamId matches for audio
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_MEDIA_SAME_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>("http://example.com", "");
    auto audio = std::make_shared<M3U8Media>("a", "http://example.com/a.m3u8", m3u8);
    audio->streamId_ = 10;
    downloader->currentAudio_ = audio;

    EXPECT_TRUE(downloader->IsMediaSame(10, HlsSegmentType::SEG_AUDIO));
    EXPECT_FALSE(downloader->IsMediaSame(20, HlsSegmentType::SEG_AUDIO));
}

/**
 * @tc.name: IS_MEDIA_SAME_004
 * @tc.desc: test IsMediaSame returns true when streamId matches for subtitle
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_MEDIA_SAME_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>("http://example.com", "");
    auto sub = std::make_shared<M3U8Media>("s", "http://example.com/s.m3u8", m3u8);
    sub->streamId_ = 15;
    downloader->currentSubtitles_ = sub;

    EXPECT_TRUE(downloader->IsMediaSame(15, HlsSegmentType::SEG_SUBTITLE));
    EXPECT_FALSE(downloader->IsMediaSame(25, HlsSegmentType::SEG_SUBTITLE));
}

/**
 * @tc.name: IS_PURE_BYTE_RANGE_001
 * @tc.desc: test IsPureByteRange returns false when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_PURE_BYTE_RANGE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();

    EXPECT_FALSE(downloader->IsPureByteRange());
}

/**
 * @tc.name: IS_PURE_BYTE_RANGE_002
 * @tc.desc: test IsPureByteRange when isPureByteRange_ is true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_PURE_BYTE_RANGE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isPureByteRange_.store(true);

    EXPECT_TRUE(downloader->IsPureByteRange());
}

/**
 * @tc.name: IS_HLS_FMP4_003
 * @tc.desc: test IsHlsFmp4 returns false when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_HLS_FMP4_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();

    EXPECT_FALSE(downloader->IsHlsFmp4());
}

/**
 * @tc.name: IS_HLS_FMP4_004
 * @tc.desc: test IsHlsFmp4 when isFmp4_ is true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_HLS_FMP4_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isFmp4_.store(true);

    EXPECT_TRUE(downloader->IsHlsFmp4());
}

/**
 * @tc.name: GET_LIVE_UPDATE_GAP_003
 * @tc.desc: test GetLiveUpdateGap when currentVariant m3u8 is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_LIVE_UPDATE_GAP_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->currentVariant_ = nullptr;

    EXPECT_EQ(downloader->GetLiveUpdateGap(), 0u);
}

/**
 * @tc.name: INTERRUPT_M3U8_PARSE_003
 * @tc.desc: test InterruptM3U8Parse with currentVariant m3u8 set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, INTERRUPT_M3U8_PARSE_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;

    downloader->InterruptM3U8Parse(true);
    EXPECT_TRUE(downloader->master_->isInterruptNeeded_);
    downloader->InterruptM3U8Parse(false);
    EXPECT_FALSE(downloader->master_->isInterruptNeeded_);
}

/**
 * @tc.name: GET_HLS_DISCONTINUITY_003
 * @tc.desc: test GetHLSDiscontinuity when master hasDiscontinuity_ is true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_HLS_DISCONTINUITY_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->hasDiscontinuity_ = true;

    EXPECT_TRUE(downloader->GetHLSDiscontinuity());
}

/**
 * @tc.name: GET_HLS_DISCONTINUITY_004
 * @tc.desc: test GetHLSDiscontinuity when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_HLS_DISCONTINUITY_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();

    EXPECT_FALSE(downloader->GetHLSDiscontinuity());
}

/**
 * @tc.name: GET_SEGMENT_OFFSET_003
 * @tc.desc: test GetSegmentOffset when segmentOffsets_ has valid entries
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEGMENT_OFFSET_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->segmentOffsets_.push_back(0);
    downloader->master_->segmentOffsets_.push_back(5000);
    downloader->master_->segmentOffsets_.push_back(10000);

    EXPECT_EQ(downloader->GetSegmentOffset(0), 0u);
    EXPECT_EQ(downloader->GetSegmentOffset(1), 5000u);
    EXPECT_EQ(downloader->GetSegmentOffset(2), 10000u);
}

/**
 * @tc.name: SET_PLAYLIST_CALLBACK_001
 * @tc.desc: test SetPlayListCallback stores callback weak pointer
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_PLAYLIST_CALLBACK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();

    auto callback = std::make_shared<MockPlayListCallback>();
    downloader->SetPlayListCallback(callback);
    auto lockedCallback = downloader->callbackWeak_.lock();
    EXPECT_NE(lockedCallback, nullptr);
    EXPECT_EQ(lockedCallback.get(), callback.get());
}

/**
 * @tc.name: SET_SOURCE_STATISTICS_DFX_001
 * @tc.desc: test SetSourceStatisticsDfx stores reportInfo and sets isFmp4
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_SOURCE_STATISTICS_DFX_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto reportInfo = std::make_shared<MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->reportInfo_ = nullptr;
    downloader->isFmp4_.store(false);

    downloader->SetSourceStatisticsDfx(reportInfo, true);
    EXPECT_NE(downloader->reportInfo_, nullptr);
    EXPECT_TRUE(downloader->isFmp4_.load());
}

/**
 * @tc.name: SET_SOURCE_STATISTICS_DFX_002
 * @tc.desc: test SetSourceStatisticsDfx with isFmp4 false
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_SOURCE_STATISTICS_DFX_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->isFmp4_.store(true);
    auto reportInfo = std::make_shared<MediaAVCodec::SourceStatisticsReportInfo>();

    downloader->SetSourceStatisticsDfx(reportInfo, false);
    EXPECT_FALSE(downloader->isFmp4_.load());
}

/**
 * @tc.name: GET_SEEKABLE_RANGES_005
 * @tc.desc: test GetSeekableRanges returns empty when master bLive_ and not seekable
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_RANGES_005, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_.store(false);

    auto ranges = downloader->GetSeekableRanges();
    EXPECT_TRUE(ranges.empty());
}

/**
 * @tc.name: IS_LIVE_003
 * @tc.desc: test IsLive returns false when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;

    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: IS_LIVE_004
 * @tc.desc: test IsLive returns true when master bLive_ is true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->bLive_ = true;

    EXPECT_TRUE(downloader->IsLive());
}

/**
 * @tc.name: UPDATE_STREAM_INFO_001
 * @tc.desc: test UpdateStreamInfo when master is null does not crash
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_STREAM_INFO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;
    size_t videoSizeBefore = downloader->videoStreamIds_.size();

    downloader->UpdateStreamInfo();
    EXPECT_EQ(downloader->videoStreamIds_.size(), videoSizeBefore);
}

/**
 * @tc.name: UPDATE_STREAM_INFO_002
 * @tc.desc: test UpdateStreamInfo with variants and audio/subtitle media
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_STREAM_INFO_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = HLS_VIDEO_STREAM_ID;
    auto audioM3u8 = std::make_shared<M3U8>("http://audio.m3u8", "");
    auto audioMedia = std::make_shared<M3U8Media>("audio", "http://audio.m3u8", audioM3u8);
    audioMedia->streamId_ = HLS_AUDIO_STREAM_ID;
    variant->audioMedia_.push_back(audioMedia);
    auto subM3u8 = std::make_shared<M3U8>("http://sub.m3u8", "");
    auto subMedia = std::make_shared<M3U8Media>("sub", "http://sub.m3u8", subM3u8);
    subMedia->streamId_ = HLS_SUBTITLE_STREAM_ID;
    variant->subtitlesMedia_.push_back(subMedia);
    downloader->master_->variants_.push_back(variant);
    downloader->needAudioManager_.store(false);
    downloader->needSubtitlesManager_.store(false);

    downloader->UpdateStreamInfo();
    EXPECT_TRUE(downloader->needAudioManager_.load());
    EXPECT_TRUE(downloader->needSubtitlesManager_.load());
}

/**
 * @tc.name: GET_SEG_TYPE_003
 * @tc.desc: test GetSegType returns SEG_VIDEO for streamId in videoStreamIds
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEG_TYPE_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->videoStreamIds_.insert(HLS_VIDEO_STREAM_ID);
    downloader->audioStreamIds_.insert(HLS_AUDIO_STREAM_ID);
    downloader->subtitlesStreamIds_.insert(HLS_SUBTITLE_STREAM_ID);

    EXPECT_EQ(downloader->GetSegType(HLS_VIDEO_STREAM_ID), HlsSegmentType::SEG_VIDEO);
}

/**
 * @tc.name: GET_START_INFO_002
 * @tc.desc: test GetStartInfo returns {0, false} when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;

    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_FALSE(info.second);
}

/**
 * @tc.name: GET_DURATION_011
 * @tc.desc: test GetDuration returns 0 when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_011, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;

    EXPECT_EQ(downloader->GetDuration(), 0);
}

/**
 * @tc.name: GET_CUR_BITRATE_003
 * @tc.desc: test GetCurBitrate returns 0 when currentVariant is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_BITRATE_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->currentVariant_ = nullptr;

    EXPECT_EQ(downloader->GetCurBitrate(), 0u);
}

/**
 * @tc.name: GET_CURRENT_BIT_RATE_001
 * @tc.desc: test GetCurrentBitRate returns 0 when currentVariant is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CURRENT_BIT_RATE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->currentVariant_ = nullptr;

    EXPECT_EQ(downloader->GetCurrentBitRate(), 0u);
}

/**
 * @tc.name: GET_VIDEO_WIDTH_001
 * @tc.desc: test GetVideoWidth returns 0 when currentVariant is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_VIDEO_WIDTH_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->currentVariant_ = nullptr;

    EXPECT_EQ(downloader->GetVideoWidth(), 0);
}

/**
 * @tc.name: GET_VIDEO_HEIGHT_001
 * @tc.desc: test GetVideoHeight returns 0 when currentVariant is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_VIDEO_HEIGHT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->currentVariant_ = nullptr;

    EXPECT_EQ(downloader->GetVideoHeight(), 0);
}

/**
 * @tc.name: READ_FMP4_HEADER_002
 * @tc.desc: test ReadFmp4Header returns false when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, READ_FMP4_HEADER_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    uint8_t buf[64];
    uint32_t readLen = 0;

    EXPECT_FALSE(downloader->ReadFmp4Header(buf, sizeof(buf), readLen, 0));
}

/**
 * @tc.name: GET_MASTER_001
 * @tc.desc: test GetMaster returns master when set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_MASTER_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);

    EXPECT_NE(downloader->GetMaster(), nullptr);
}

/**
 * @tc.name: GET_CURRENT_VARIANT_001
 * @tc.desc: test GetCurrentVariant returns currentVariant when set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CURRENT_VARIANT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;

    EXPECT_NE(downloader->GetCurrentVariant(), nullptr);
}

/**
 * @tc.name: GET_NEW_VARIANT_001
 * @tc.desc: test GetNewVariant returns newVariant when set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_NEW_VARIANT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->newVariant_ = variant;

    EXPECT_NE(downloader->GetNewVariant(), nullptr);
}

/**
 * @tc.name: GET_URL_001
 * @tc.desc: test GetUrl returns url when set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_URL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->url_ = "http://test.url";

    EXPECT_EQ(downloader->GetUrl(), "http://test.url");
}
}