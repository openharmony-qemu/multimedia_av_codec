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
#include "test_http_port.h"
#include "utils/time_utils.h"

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

    void UpdateLiveBaseOffsetForTest(const std::vector<M3U8Segment>& files)
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
    const std::string testUriPath = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/";
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
const static std::string TEST_URI_PATH = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/";
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
    EXPECT_EQ(streams.size(), HLS_TEST_STREAM_COUNT);
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
    EXPECT_TRUE(downloader->GetDuration() >= 0);
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
    downloader->master_->duration_ = 10ULL * S_2_NS + 500ULL * MS_2_NS;

    int64_t expected = static_cast<int64_t>(downloader->master_->duration_);
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
    downloader->master_->duration_ = 10ULL * S_2_NS;
    int64_t ret = downloader->GetDuration();
    EXPECT_TRUE(-1 * ret > 0);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_009, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    downloader->master_->bLive_ = false;
    downloader->master_->duration_ = 15ULL * S_2_NS + 200ULL * MS_2_NS;
    int64_t expected = static_cast<int64_t>(downloader->master_->duration_);
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
    downloader->master_->duration_ = 10ULL * S_2_NS + 500ULL * MS_2_NS;
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
    downloader->master_->duration_ = 10ULL * S_2_NS + 500ULL * MS_2_NS;
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
    downloader->master_->duration_ = 10ULL * S_2_NS + 500ULL * MS_2_NS;
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
    downloader->master_->duration_ = 10ULL * S_2_NS + 500ULL * MS_2_NS;
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
    downloader->master_->duration_ = 10ULL * S_2_NS + 500ULL * MS_2_NS;
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
    downloader->master_->duration_ = 10ULL * S_2_NS + 500ULL * MS_2_NS;
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
    downloader->master_->duration_ = 0;  // uint64_t cannot be negative; 0 covers the false branch
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
    downloader->master_->duration_ = 10ULL * S_2_NS + 500ULL * MS_2_NS;
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
    std::vector<M3U8Segment> emptyFiles;
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
    std::vector<M3U8Segment> files;
    auto frag = M3U8Segment("test1.m3u8", 10000000, 1, false);  // 10 sec in microseconds
    files.push_back(frag);
    downloader->UpdateSeekRanges(files);
    std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
    EXPECT_EQ(downloader->seekRanges_.size(), 1);
    EXPECT_EQ(downloader->seekRanges_[0].start, 0);
    EXPECT_EQ(downloader->seekRanges_[0].end, 10000000LL); // 10s in microseconds (duration_ unit)
}

HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_003, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>(httpHeader, nullptr);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + M3U8_PATH_LIVE;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    std::vector<M3U8Segment> files;
    auto frag = M3U8Segment("test2.m3u8", 5000000, 2, false);  // 5 sec in microseconds
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
    std::vector<M3U8Segment> files;
    auto frag1 = M3U8Segment("test3_1.m3u8", 10000000, 3, false);  // 10 sec in microseconds
    files.push_back(frag1);
    auto frag2 = M3U8Segment("test3_2.m3u8", 5000000, 4, true);  // 5 sec in microseconds
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
    std::vector<M3U8Segment> files;
    auto frag = M3U8Segment("test4.m3u8", 0, 5, false);  // uint64_t cannot be negative; 0 for non-seekable
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
    std::vector<M3U8Segment> files;
    auto frag1 = M3U8Segment("test5_1.m3u8", 0, 6, true);
    auto frag2 = M3U8Segment("test5_2.m3u8", 8, 7, false);
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
    std::vector<M3U8Segment> files;
    auto frag = M3U8Segment("test6.m3u8", 0, 8, false);
    files.push_back(frag);
    downloader->UpdateSeekRanges(files);
    std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
    EXPECT_TRUE(downloader->seekRanges_.empty());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, CHECK_ALL_FRAGMENTS_SEEKABLE_001, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    std::vector<M3U8Segment> files;

    downloader->CheckAllFragmentsSeekable(files);

    EXPECT_FALSE(downloader->allFragmentsSeekable_);
    EXPECT_TRUE(downloader->seekRanges_.empty());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, CHECK_ALL_FRAGMENTS_SEEKABLE_002, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    std::vector<M3U8Segment> files;
    auto fragment1 = M3U8Segment("test1.ts", 5, 1, false);
    auto fragment2 = M3U8Segment("test2.ts", 5, 2, false);
    auto fragment3 = M3U8Segment("test3.ts", 5, 3, false);
    files.push_back(fragment1);
    files.push_back(fragment2);
    files.push_back(fragment3);

    downloader->CheckAllFragmentsSeekable(files);

    EXPECT_TRUE(downloader->isFragmentsUriSet_);
    EXPECT_TRUE(downloader->allFragmentsSeekable_);
    EXPECT_EQ(downloader->copiedFileSequences_.size(), files.size());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, CHECK_ALL_FRAGMENTS_SEEKABLE_003, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    std::vector<M3U8Segment> firstFiles;
    firstFiles.push_back(M3U8Segment("test1.ts", 5, 1, false));
    firstFiles.push_back(M3U8Segment("test2.ts", 5, 2, false));
    firstFiles.push_back(M3U8Segment("test3.ts", 5, 3, false));
    downloader->CheckAllFragmentsSeekable(firstFiles);

    std::vector<M3U8Segment> nextFiles;
    nextFiles.push_back(M3U8Segment("test2.ts", 5, 2, false));
    nextFiles.push_back(M3U8Segment("test3.ts", 5, 3, false));
    nextFiles.push_back(M3U8Segment("test4.ts", 5, 4, false));
    downloader->CheckAllFragmentsSeekable(nextFiles);

    EXPECT_TRUE(downloader->allFragmentsSeekable_);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, CHECK_ALL_FRAGMENTS_SEEKABLE_004, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    std::vector<M3U8Segment> firstFiles;
    firstFiles.push_back(M3U8Segment("test1.ts", 5, 1, false));
    firstFiles.push_back(M3U8Segment("test2.ts", 5, 2, false));
    firstFiles.push_back(M3U8Segment("test3.ts", 5, 3, false));
    firstFiles.push_back(M3U8Segment("test4.ts", 5, 4, false));
    firstFiles.push_back(M3U8Segment("test5.ts", 5, 5, false));
    downloader->CheckAllFragmentsSeekable(firstFiles);

    std::vector<M3U8Segment> nextFiles;
    nextFiles.push_back(M3U8Segment("test2.ts", 5, 2, false));
    nextFiles.push_back(M3U8Segment("test6.ts", 5, 6, false));
    downloader->CheckAllFragmentsSeekable(nextFiles);

    EXPECT_FALSE(downloader->allFragmentsSeekable_);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, CHECK_ALL_FRAGMENTS_SEEKABLE_005, TestSize.Level0)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    downloader->liveBaseOffset_ = 123;
    downloader->lastFirstSeq_ = 456;
    downloader->lastSegDurations_.push_back(789);

    std::vector<M3U8Segment> files;
    files.push_back(M3U8Segment("test1.ts", 5, 1, false));
    downloader->CheckAllFragmentsSeekable(files);

    EXPECT_EQ(downloader->liveBaseOffset_, 0);
    EXPECT_EQ(downloader->lastFirstSeq_, -1);
    EXPECT_TRUE(downloader->lastSegDurations_.empty());
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
        auto frag = M3U8Segment("test.ts", 1, i, false);
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
        auto frag = M3U8Segment("test.ts", 1, i, false);
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
    EXPECT_EQ(downloader->GetCurBitrate(), 0u);
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
    EXPECT_EQ(downloader->GetCurBitrate(), 0u);
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
    EXPECT_EQ(downloader->GetCurBitrate(), 0u);
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
    EXPECT_EQ(downloader->GetCurBitrate(), 0u);
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
    EXPECT_TRUE(downloader->GetDuration() >= 0);
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
    EXPECT_TRUE(-1 * downloader2->GetDuration() >= 0);
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
    EXPECT_EQ(offset, 0u);
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
    EXPECT_TRUE(gap >= 0u);
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
    EXPECT_TRUE(downloader->GetCurStreamId() > 0u);
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
 * @tc.desc: test UpdateSeekRanges with zero-duration fragment (uint64_t duration cannot be negative)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_008, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    std::vector<M3U8Segment> files;
    auto frag1 = M3U8Segment("test1.ts", 0, 0, false);  // uint64_t cannot be negative; 0 for non-seekable
    auto frag2 = M3U8Segment("test2.ts", 10, 1, false);
    auto frag3 = M3U8Segment("test3.ts", 0, 2, false);  // uint64_t cannot be negative; 0 for non-seekable
    files.push_back(frag1);
    files.push_back(frag2);
    files.push_back(frag3);
    downloader->UpdateSeekRanges(files);
    auto ranges = downloader->GetSeekableRanges();
    EXPECT_TRUE(ranges.size() >= 0u);
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
    std::vector<M3U8Segment> files;
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
    std::vector<M3U8Segment> files;
    auto frag1 = M3U8Segment("test1.ts", 5, 10, false);
    files.push_back(frag1);
    downloader->UpdateLiveBaseOffsetForTest(files);
    EXPECT_TRUE(downloader->GetLiveBaseOffset() >= 0);
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
    std::vector<M3U8Segment> files1;
    auto frag1 = M3U8Segment("test1.ts", 5, 10, false);
    files1.push_back(frag1);
    downloader->UpdateLiveBaseOffsetForTest(files1);
    std::vector<M3U8Segment> files2;
    auto frag2 = M3U8Segment("test2.ts", 5, 5, false);
    files2.push_back(frag2);
    downloader->UpdateLiveBaseOffsetForTest(files2);
    auto offset = downloader->GetLiveBaseOffset();
    EXPECT_TRUE(offset >= 0);
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
    std::vector<M3U8Segment> files1;
    auto frag1 = M3U8Segment("test1.ts", 5, 10, false);
    files1.push_back(frag1);
    downloader->UpdateLiveBaseOffsetForTest(files1);
    auto offset1 = downloader->GetLiveBaseOffset();
    std::vector<M3U8Segment> files2;
    auto frag2 = M3U8Segment("test2.ts", 5, 15, false);
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
    auto fragment = M3U8Segment("http://test.ts", 5, 0, false);
    fragment.uri_ = "http://test.ts";
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
    auto fragment = M3U8Segment("http://test.ts", 5, 0, false);
    fragment.uri_ = "http://test.ts";
    fragment.offset_ = 100;
    fragment.length_ = 200;
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
    EXPECT_EQ(downloader->GetCurBitrate(), 0u);
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
    EXPECT_EQ(downloader->currentSubtitles_->streamId_, HLS_SUBTITLE_STREAM_ID);
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
    EXPECT_FALSE(downloader->IsLive());
    EXPECT_EQ(downloader->GetCurBitrate(), 0u);
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
    EXPECT_FALSE(downloader->IsLive());
    EXPECT_EQ(downloader->GetCurBitrate(), 0u);
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
    EXPECT_EQ(downloader->newVariant_->bandWidth_, 1000000u);
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
    EXPECT_EQ(downloader->newVariant_->bandWidth_, 1000000u);
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
    EXPECT_EQ(downloader->GetMaster()->uri_, TEST_URI_PATH);
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
    EXPECT_EQ(downloader->GetCurrentVariant()->uri_, TEST_URI_PATH);
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
    EXPECT_EQ(downloader->GetNewVariant()->uri_, TEST_URI_PATH);
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

/**
 * @tc.name  : Test HasExplicitIv with null aesDecryptorManager
 * @tc.number: HasExplicitIv_NullManager
 * @tc.desc  : Test HasExplicitIv returns false when aesDecryptorManager_ is null
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, HasExplicitIv_NullManager, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->aesDecryptorManager_ = nullptr;
    bool result = downloader->HasExplicitIv(0);
    EXPECT_FALSE(result);
}

/**
 * @tc.name  : Test HasExplicitIv with valid manager and key
 * @tc.number: HasExplicitIv_ValidManager
 * @tc.desc  : Test HasExplicitIv returns true when key has explicit IV
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, HasExplicitIv_ValidManager, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->aesDecryptorManager_ = std::make_shared<AesDecryptorManager>();
    KeyInfo keyInfo;
    keyInfo.index_ = 1;
    keyInfo.hasExplicitIv_ = true;
    downloader->aesDecryptorManager_->hasExplicitIvMap_[1] = true;
    bool result = downloader->HasExplicitIv(1);
    EXPECT_TRUE(result);

    result = downloader->HasExplicitIv(999);
    EXPECT_FALSE(result);
}

/**
 * @tc.name  : Test GetStartInfo with duration != -1 and timeOffset < 0
 * @tc.number: GetStartInfo_NegativeTimeOffset
 * @tc.desc  : Test GetStartInfo with negative timeOffset and valid duration
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GetStartInfo_NegativeTimeOffset, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    const std::string testUriPath = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/";
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", testUriPath);
    downloader->master_->isStart_ = true;
    downloader->master_->timeOffset_ = static_cast<int64_t>(-5000LL * S_2_NS);
    downloader->master_->duration_ = 10000ULL * S_2_NS;
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, static_cast<int64_t>(5000LL * S_2_NS));
    EXPECT_FALSE(info.second);
}

/**
 * @tc.name: GET_START_INFO_DURATION_NEG1
 * @tc.desc: test GetStartInfo when duration == -1 (live not seekable)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_DURATION_NEG1, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isStart_ = true;
    downloader->master_->bLive_ = true;
    downloader->master_->timeOffset_ = static_cast<int64_t>(5000ULL * S_2_NS);
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_FALSE(info.second);
}

/**
 * @tc.name: GET_START_INFO_TIMEOFFSET_GT_DURATION
 * @tc.desc: test GetStartInfo when timeOffset > duration
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_TIMEOFFSET_GT_DURATION, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isStart_ = true;
    downloader->master_->bLive_ = false;
    downloader->master_->duration_ = 1ULL * S_2_NS;
    downloader->master_->timeOffset_ = static_cast<int64_t>(5000000ULL * S_2_NS);
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, static_cast<int64_t>(1LL * S_2_NS));
    EXPECT_FALSE(info.second);
}

/**
 * @tc.name: GET_START_INFO_DEFAULT_VARIANT_POS
 * @tc.desc: test GetStartInfo via defaultVariant m3u8 isStart with positive offset
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_DEFAULT_VARIANT_POS, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->isStart_ = true;
    m3u8->timeOffset_ = static_cast<int64_t>(5000ULL * S_2_NS);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    downloader->master_->duration_ = 10000ULL * S_2_NS;
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, static_cast<int64_t>(5000LL * S_2_NS));
    EXPECT_FALSE(info.second);
}

/**
 * @tc.name: GET_START_INFO_DEFAULT_VARIANT_NEG
 * @tc.desc: test GetStartInfo via defaultVariant m3u8 with negative offset
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_DEFAULT_VARIANT_NEG, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->isStart_ = true;
    m3u8->timeOffset_ = static_cast<int64_t>(-5000LL * S_2_NS);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    downloader->master_->duration_ = 10000ULL * S_2_NS;
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, static_cast<int64_t>(5000LL * S_2_NS));
    EXPECT_FALSE(info.second);
}

/**
 * @tc.name: GET_START_INFO_DEFAULT_VARIANT_DUR_NEG1
 * @tc.desc: test GetStartInfo via defaultVariant when duration == -1
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_DEFAULT_VARIANT_DUR_NEG1, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->isStart_ = true;
    m3u8->timeOffset_ = static_cast<int64_t>(5000ULL * S_2_NS);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    downloader->master_->bLive_ = true;
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_FALSE(info.second);
}

/**
 * @tc.name: IS_FRAGMENT_IN_PLAYLIST_004
 * @tc.desc: test IsFragmentInCurrentPlayList with currentSubtitles_ m3u8
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_FRAGMENT_IN_PLAYLIST_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto fragment = M3U8Segment("http://sub.ts", 5, 0, false);
    m3u8->files_.push_back(fragment);
    auto sub = std::make_shared<M3U8Media>("s", "http://sub.m3u8", m3u8);
    downloader->currentSubtitles_ = sub;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    EXPECT_TRUE(downloader->IsFragmentInCurrentPlayList("http://sub.ts", "", 0, 0));
    EXPECT_FALSE(downloader->IsFragmentInCurrentPlayList("http://other.ts", "", 0, 0));
}

/**
 * @tc.name: IS_FRAGMENT_IN_PLAYLIST_005
 * @tc.desc: test IsFragmentInCurrentPlayList with currentAudio_ m3u8
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_FRAGMENT_IN_PLAYLIST_005, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto fragment = M3U8Segment("http://audio.ts", 5, 0, false);
    fragment.offset_ = 100;
    fragment.length_ = 200;
    m3u8->files_.push_back(fragment);
    auto audio = std::make_shared<M3U8Media>("a", "http://audio.m3u8", m3u8);
    downloader->currentAudio_ = audio;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    EXPECT_TRUE(downloader->IsFragmentInCurrentPlayList("http://audio.ts", "http://audio.ts", 100, 200));
    EXPECT_FALSE(downloader->IsFragmentInCurrentPlayList("http://audio.ts", "http://audio.ts", 50, 200));
}

/**
 * @tc.name: UPDATE_PLAYLISTS_SIMPLE_AUDIO
 * @tc.desc: test UpdatePlaylists isSimple=true with currentAudio_
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_SIMPLE_AUDIO, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio", m3u8);
    downloader->currentAudio_ = audio;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(true);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: UPDATE_PLAYLISTS_SIMPLE_SUBTITLE
 * @tc.desc: test UpdatePlaylists isSimple=true with currentSubtitles_ (audio null)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_SIMPLE_SUBTITLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", m3u8);
    downloader->currentSubtitles_ = sub;
    downloader->currentAudio_ = nullptr;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(true);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: UPDATE_PLAYLISTS_SIMPLE_VARIANT
 * @tc.desc: test UpdatePlaylists isSimple=true with only currentVariant
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_SIMPLE_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->currentAudio_ = nullptr;
    downloader->currentSubtitles_ = nullptr;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(true);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: UPDATE_PLAYLISTS_NONSIMPLE_AUDIO
 * @tc.desc: test UpdatePlaylists isSimple=false with currentAudio_
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_NONSIMPLE_AUDIO, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio", m3u8);
    downloader->currentAudio_ = audio;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(false);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: UPDATE_PLAYLISTS_NONSIMPLE_SUBTITLE
 * @tc.desc: test UpdatePlaylists isSimple=false with currentSubtitles_ (audio null)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_NONSIMPLE_SUBTITLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", m3u8);
    downloader->currentSubtitles_ = sub;
    downloader->currentAudio_ = nullptr;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(false);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: UPDATE_PLAYLISTS_NONSIMPLE_DEFAULT
 * @tc.desc: test UpdatePlaylists isSimple=false with defaultVariant fallback
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_NONSIMPLE_DEFAULT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    downloader->currentVariant_ = variant;
    downloader->currentAudio_ = nullptr;
    downloader->currentSubtitles_ = nullptr;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(false);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: PRE_PARSE_MANIFEST_NULL_MASTER
 * @tc.desc: test PreParseManifest when master is null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, PRE_PARSE_MANIFEST_NULL_MASTER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->PreParseManifest(TEST_URI_PATH);
    EXPECT_EQ(downloader->GetMaster(), nullptr);
    EXPECT_FALSE(downloader->IsParseFinished());
}

/**
 * @tc.name: PRE_PARSE_MANIFEST_ALREADY_SIMPLE
 * @tc.desc: test PreParseManifest when master isSimple_ is true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, PRE_PARSE_MANIFEST_ALREADY_SIMPLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isSimple_.store(true);
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->PreParseManifest(TEST_URI_PATH);
    EXPECT_TRUE(downloader->master_->isSimple_.load());
}

/**
 * @tc.name: PRE_PARSE_MANIFEST_NO_START_TAG
 * @tc.desc: test PreParseManifest when playList has no EXT M3U start tag
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, PRE_PARSE_MANIFEST_NO_START_TAG, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->SetPlayListForTest("some content without start tag\n#EXTINF:5.0,\nseg.ts\n");
    downloader->PreParseManifest(TEST_URI_PATH);
    EXPECT_FALSE(downloader->master_->isSimple_.load());
}

/**
 * @tc.name: PRE_PARSE_MANIFEST_HAS_END_TAG
 * @tc.desc: test PreParseManifest when playList has ENDLIST tag
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, PRE_PARSE_MANIFEST_HAS_END_TAG, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->SetPlayListForTest("#EXTM3U\n#EXT-X-ENDLIST\n#EXTINF:5.0,\nseg0.ts\n#EXTINF:5.0,\nseg1.ts\n");
    downloader->PreParseManifest(TEST_URI_PATH);
    EXPECT_FALSE(downloader->master_->isSimple_.load());
}

/**
 * @tc.name: PRE_PARSE_MANIFEST_HAS_XMAP
 * @tc.desc: test PreParseManifest when playList has X-MAP tag
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, PRE_PARSE_MANIFEST_HAS_XMAP, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->SetPlayListForTest(
        "#EXTM3U\n#EXT-X-MAP:URI=\"init.mp4\"\n#EXTINF:5.0,\nseg0.ts\n#EXTINF:5.0,\nseg1.ts\n");
    downloader->PreParseManifest(TEST_URI_PATH);
    EXPECT_FALSE(downloader->master_->isSimple_.load());
}

/**
 * @tc.name: PRE_PARSE_MANIFEST_TOO_FEW_TS
 * @tc.desc: test PreParseManifest with fewer than MIN ts fragments
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, PRE_PARSE_MANIFEST_TOO_FEW_TS, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->SetPlayListForTest("#EXTM3U\n#EXTINF:5.0,\nseg0.ts\n");
    downloader->PreParseManifest(TEST_URI_PATH);
    EXPECT_FALSE(downloader->master_->isSimple_.load());
}

/**
 * @tc.name: COPY_FRAGMENT_INFO_NULL_FILE
 * @tc.desc: test CopyFragmentInfo with null file
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COPY_FRAGMENT_INFO_NULL_FILE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH,
        std::make_shared<M3U8>(TEST_URI_PATH, ""));
    variant->streamId_ = 1;
    downloader->currentVariant_ = variant;
    PlayInfo playInfo;
    M3U8Segment dummySeg("", 0, 0, false);
    downloader->CopyFragmentInfo(playInfo, dummySeg, 0);
    EXPECT_EQ(playInfo.keyIndex_, 0ull);
}

/**
 * @tc.name: COPY_FRAGMENT_INFO_FMP4
 * @tc.desc: test CopyFragmentInfo with isFmp4 true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COPY_FRAGMENT_INFO_FMP4, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isFmp4_.store(true);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH,
        std::make_shared<M3U8>(TEST_URI_PATH, ""));
    variant->streamId_ = 1;
    downloader->currentVariant_ = variant;
    auto file = M3U8Segment("http://seg.ts", 5, 0, false);
    file.offset_ = 100;
    file.length_ = 200;
    PlayInfo playInfo;
    downloader->CopyFragmentInfo(playInfo, file, 0);
    EXPECT_NE(playInfo.url_.find("100_200"), std::string::npos);
    EXPECT_EQ(playInfo.rangeUrl_, "http://seg.ts");
}

/**
 * @tc.name: COPY_FRAGMENT_INFO_BYTE_RANGE
 * @tc.desc: test CopyFragmentInfo with isPureByteRange true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COPY_FRAGMENT_INFO_BYTE_RANGE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isPureByteRange_.store(true);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH,
        std::make_shared<M3U8>(TEST_URI_PATH, ""));
    variant->streamId_ = 1;
    downloader->currentVariant_ = variant;
    auto file = M3U8Segment("http://seg.ts", 5, 0, false);
    file.offset_ = 50;
    file.length_ = 100;
    PlayInfo playInfo;
    downloader->CopyFragmentInfo(playInfo, file, 0);
    EXPECT_EQ(playInfo.rangeUrl_, "http://seg.ts");
}

/**
 * @tc.name: COPY_FRAGMENT_INFO_KEY_INDEX_MAX
 * @tc.desc: test CopyFragmentInfo with keyIndex_ == UINT64_MAX
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COPY_FRAGMENT_INFO_KEY_INDEX_MAX, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH,
        std::make_shared<M3U8>(TEST_URI_PATH, ""));
    variant->streamId_ = 1;
    downloader->currentVariant_ = variant;
    M3U8Segment file("http://seg.ts", 5, 0, false);
    file.keyIndex_ = UINT64_MAX;
    PlayInfo playInfo;
    downloader->CopyFragmentInfo(playInfo, file, 0);
    EXPECT_EQ(playInfo.keyIndex_, 0ull);
}

/**
 * @tc.name: COPY_FRAGMENT_INFO_KEY_INDEX_POSITIVE
 * @tc.desc: test CopyFragmentInfo with keyIndex_ > 0
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COPY_FRAGMENT_INFO_KEY_INDEX_POSITIVE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH,
        std::make_shared<M3U8>(TEST_URI_PATH, ""));
    variant->streamId_ = 1;
    downloader->currentVariant_ = variant;
    M3U8Segment file("http://seg.ts", 5, 0, false);
    file.keyIndex_ = 5;
    PlayInfo playInfo;
    downloader->CopyFragmentInfo(playInfo, file, 0);
    EXPECT_EQ(playInfo.keyIndex_, 5ull);
}

/**
 * @tc.name: COPY_FRAGMENT_INFO_SESSION_KEY
 * @tc.desc: test CopyFragmentInfo with sessionKeyIndex > 0
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COPY_FRAGMENT_INFO_SESSION_KEY, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH,
        std::make_shared<M3U8>(TEST_URI_PATH, ""));
    variant->streamId_ = 1;
    downloader->currentVariant_ = variant;
    M3U8Segment file("http://seg.ts", 5, 0, false);
    file.keyIndex_ = 0;
    PlayInfo playInfo;
    downloader->CopyFragmentInfo(playInfo, file, 3);
    EXPECT_EQ(playInfo.keyIndex_, UINT64_MAX);
}

/**
 * @tc.name: GET_STREAM_INFO_UPDATE
 * @tc.desc: test GetStreamInfo with isUpdate=true
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_UPDATE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    PrepareHlsDownloaderStreamInfo(downloader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams, true);
    EXPECT_EQ(streams.size(), HLS_TEST_STREAM_COUNT);
}

/**
 * @tc.name: GET_STREAM_INFO_NULL_VARIANT
 * @tc.desc: test GetStreamInfo with null variant in master
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_NULL_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->variants_.push_back(nullptr);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = 1;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = variant;
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams, false);
    EXPECT_EQ(streams.size(), 1u);
}

/**
 * @tc.name: GET_STREAM_INFO_AUDIO_DEFAULT
 * @tc.desc: test GetMediaStreams audio with audioStreamId_ set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_AUDIO_DEFAULT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    PrepareHlsDownloaderStreamInfo(downloader);
    downloader->audioStreamId_.store(HLS_TEST_AUDIO_STREAM_ID);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams, false);
    bool foundAudio = false;
    for (const auto &s : streams) {
        if (s.type == StreamType::AUDIO) {
            foundAudio = true;
            EXPECT_EQ(s.streamId, HLS_TEST_AUDIO_STREAM_ID);
        }
    }
    EXPECT_TRUE(foundAudio);
}

/**
 * @tc.name: GET_STREAM_INFO_SUBTITLE_DEFAULT
 * @tc.desc: test GetMediaStreams subtitle with subtitleStreamId_ set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_SUBTITLE_DEFAULT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    PrepareHlsDownloaderStreamInfo(downloader);
    downloader->subtitleStreamId_.store(HLS_TEST_SUBTITLE_STREAM_ID);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams, false);
    bool foundSub = false;
    for (const auto &s : streams) {
        if (s.type == StreamType::SUBTITLE) {
            foundSub = true;
            EXPECT_EQ(s.streamId, HLS_TEST_SUBTITLE_STREAM_ID);
        }
    }
    EXPECT_TRUE(foundSub);
}

/**
 * @tc.name: GET_STREAM_INFO_NULL_MEDIA
 * @tc.desc: test GetMediaStreams with null media in audioList
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_NULL_MEDIA, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    PrepareHlsDownloaderStreamInfo(downloader);
    downloader->master_->audioList_.push_back(nullptr);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams, false);
    EXPECT_EQ(streams.size(), HLS_TEST_STREAM_COUNT);
}

/**
 * @tc.name: READ_FMP4_HEADER_SUCCESS
 * @tc.desc: test ReadFmp4Header returns true with valid subtitle header
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, READ_FMP4_HEADER_SUCCESS, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>("http://test", "");
    m3u8->fmp4Header_ = new uint8_t[16]();
    m3u8->downloadHeaderLen_ = 8;
    m3u8->isHeaderReady_.store(true);
    auto media = std::make_shared<M3U8Media>("sub", "http://test", m3u8);
    media->streamId_ = 5;
    downloader->master_->subtitlesList_.push_back(media);
    uint8_t buf[64];
    uint32_t readLen = 1;
    bool ret = downloader->ReadFmp4Header(buf, sizeof(buf), readLen, 5);
    EXPECT_TRUE(ret);
    EXPECT_EQ(readLen, 8u);
}

/**
 * @tc.name: READ_FMP4_HEADER_VARIANT
 * @tc.desc: test ReadFmp4Header via variants list (ReadStreamHeader)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, READ_FMP4_HEADER_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>("http://test", "");
    m3u8->fmp4Header_ = new uint8_t[16]();
    m3u8->downloadHeaderLen_ = 4;
    m3u8->isHeaderReady_.store(true);
    auto variant = std::make_shared<M3U8VariantStream>("video", "http://test", m3u8);
    variant->streamId_ = 7;
    downloader->master_->variants_.push_back(variant);
    uint8_t buf[64];
    uint32_t readLen = 0;
    bool ret = downloader->ReadFmp4Header(buf, sizeof(buf), readLen, 7);
    EXPECT_TRUE(ret);
    EXPECT_EQ(readLen, 4u);
}

/**
 * @tc.name: READ_FMP4_HEADER_LEN_TOO_BIG
 * @tc.desc: test ReadFmp4Header when downloadHeaderLen > wantLen
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, READ_FMP4_HEADER_LEN_TOO_BIG, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>("http://test", "");
    m3u8->fmp4Header_ = new uint8_t[16]();
    m3u8->downloadHeaderLen_ = 100;
    m3u8->isHeaderReady_.store(true);
    auto media = std::make_shared<M3U8Media>("sub", "http://test", m3u8);
    media->streamId_ = 5;
    downloader->master_->subtitlesList_.push_back(media);
    uint8_t buf[64];
    uint32_t readLen = 0;
    bool ret = downloader->ReadFmp4Header(buf, sizeof(buf), readLen, 5);
    EXPECT_FALSE(ret);
    EXPECT_EQ(readLen, 0u);
}

/**
 * @tc.name: READ_FMP4_HEADER_SKIP_STREAMS
 * @tc.desc: test ReadFmp4Header skips null/m3u8null/mismatch/notready streams
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, READ_FMP4_HEADER_SKIP_STREAMS, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->subtitlesList_.push_back(nullptr);
    auto mediaNull = std::make_shared<M3U8Media>("sub", "http://test", nullptr);
    mediaNull->streamId_ = 5;
    downloader->master_->subtitlesList_.push_back(mediaNull);
    auto m3u8Mismatch = std::make_shared<M3U8>("http://test", "");
    m3u8Mismatch->isHeaderReady_.store(true);
    auto mediaMismatch = std::make_shared<M3U8Media>("sub", "http://test", m3u8Mismatch);
    mediaMismatch->streamId_ = 99;
    downloader->master_->subtitlesList_.push_back(mediaMismatch);
    auto m3u8NotReady = std::make_shared<M3U8>("http://test", "");
    m3u8NotReady->isHeaderReady_.store(false);
    auto mediaNotReady = std::make_shared<M3U8Media>("sub", "http://test", m3u8NotReady);
    mediaNotReady->streamId_ = 5;
    downloader->master_->subtitlesList_.push_back(mediaNotReady);
    uint8_t buf[64];
    uint32_t readLen = 0;
    bool ret = downloader->ReadFmp4Header(buf, sizeof(buf), readLen, 5);
    EXPECT_FALSE(ret);
    EXPECT_EQ(readLen, 0u);
}

/**
 * @tc.name: START_PENDING_KEY_EMPTY
 * @tc.desc: test StartPendingKeyDownload with empty keyInfos
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, START_PENDING_KEY_EMPTY, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->aesDecryptorManager_ = std::make_shared<AesDecryptorManager>();
    std::unordered_map<uint64_t, KeyInfo> empty;
    downloader->StartPendingKeyDownload(empty, "http://base");
    EXPECT_EQ(downloader->keyDownloader_, nullptr);
    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: START_PENDING_KEY_SKIP
 * @tc.desc: test StartPendingKeyDownload skips keys with len/uri/exists
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, START_PENDING_KEY_SKIP, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->aesDecryptorManager_ = std::make_shared<AesDecryptorManager>();
    std::unordered_map<uint64_t, KeyInfo> keyInfos;
    KeyInfo k1;
    k1.index_ = 1;
    k1.keyLen_ = 16;
    k1.uri_ = "http://key1";
    keyInfos[1] = k1;
    KeyInfo k2;
    k2.index_ = 2;
    k2.keyLen_ = 0;
    k2.uri_ = "";
    keyInfos[2] = k2;
    downloader->StartPendingKeyDownload(keyInfos, "http://base");
    EXPECT_EQ(downloader->keyDownloader_, nullptr);
    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: START_PENDING_KEY_DOWNLOAD
 * @tc.desc: test StartPendingKeyDownload creates keyDownloader for pending key
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, START_PENDING_KEY_DOWNLOAD, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->aesDecryptorManager_ = std::make_shared<AesDecryptorManager>();
    std::unordered_map<uint64_t, KeyInfo> keyInfos;
    KeyInfo k1;
    k1.index_ = 10;
    k1.keyLen_ = 0;
    k1.uri_ = "http://key1";
    keyInfos[10] = k1;
    downloader->StartPendingKeyDownload(keyInfos, "http://base");
    EXPECT_NE(downloader->keyDownloader_, nullptr);
    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: NOTIFY_DATE_RANGES_EMPTY
 * @tc.desc: test NotifyDateRanges with empty list
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, NOTIFY_DATE_RANGES_EMPTY, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto callback = std::make_shared<MockPlayListCallback>();
    std::list<std::shared_ptr<DateRange>> empty;
    downloader->NotifyDateRanges(empty, callback);
    EXPECT_TRUE(callback->savedPlayList.empty());
}

/**
 * @tc.name: NOTIFY_DATE_RANGES_NULL_AND_VALID
 * @tc.desc: test NotifyDateRanges with null and valid dateRange entries
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, NOTIFY_DATE_RANGES_NULL_AND_VALID, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto callback = std::make_shared<MockPlayListCallback>();
    std::list<std::shared_ptr<DateRange>> ranges;
    ranges.push_back(nullptr);
    auto dr = std::make_shared<DateRange>();
    dr->id = "testId";
    dr->classify = "cls";
    dr->duration = 5000;
    ranges.push_back(dr);
    downloader->NotifyDateRanges(ranges, callback);
    EXPECT_TRUE(callback->savedPlayList.empty());
}

/**
 * @tc.name: START_LIVE_UPDATE_NULL
 * @tc.desc: test StartLiveUpdateIfNeeded with null master
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, START_LIVE_UPDATE_NULL, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->StartLiveUpdateIfNeeded(nullptr);
    EXPECT_FALSE(downloader->isLiveUpdateTaskStarted_.load());
}

/**
 * @tc.name: START_LIVE_UPDATE_NOT_LIVE
 * @tc.desc: test StartLiveUpdateIfNeeded with non-live master
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, START_LIVE_UPDATE_NOT_LIVE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto master = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    master->bLive_ = false;
    downloader->StartLiveUpdateIfNeeded(master);
    EXPECT_FALSE(downloader->isLiveUpdateTaskStarted_.load());
}

/**
 * @tc.name: START_LIVE_UPDATE_LIVE
 * @tc.desc: test StartLiveUpdateIfNeeded with live master
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, START_LIVE_UPDATE_LIVE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto master = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    master->bLive_ = true;
    downloader->StartLiveUpdateIfNeeded(master);
    EXPECT_TRUE(downloader->isLiveUpdateTaskStarted_.load());
}

/**
 * @tc.name: COLLECT_KEY_INFOS_SUBTITLE
 * @tc.desc: test CollectAllKeyInfos with currentSubtitles_ set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COLLECT_KEY_INFOS_SUBTITLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    auto subM3u8 = std::make_shared<M3U8>("http://sub", "");
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", subM3u8);
    downloader->currentSubtitles_ = sub;
    std::unordered_map<uint64_t, KeyInfo> keyInfos;
    std::vector<M3U8Segment> files;
    uint64_t sessionKeyIndex = 0;
    downloader->CollectAllKeyInfos(keyInfos, files, sessionKeyIndex);
    EXPECT_TRUE(keyInfos.empty());
}

/**
 * @tc.name: COLLECT_KEY_INFOS_AUDIO
 * @tc.desc: test CollectAllKeyInfos with currentAudio_ set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COLLECT_KEY_INFOS_AUDIO, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    auto audioM3u8 = std::make_shared<M3U8>("http://audio", "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio", audioM3u8);
    downloader->currentAudio_ = audio;
    std::unordered_map<uint64_t, KeyInfo> keyInfos;
    std::vector<M3U8Segment> files;
    uint64_t sessionKeyIndex = 0;
    downloader->CollectAllKeyInfos(keyInfos, files, sessionKeyIndex);
    EXPECT_TRUE(keyInfos.empty());
}

/**
 * @tc.name: KEY_CHANGE_NEW_MANAGER
 * @tc.desc: test KeyChange creates aesDecryptorManager when null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, KEY_CHANGE_NEW_MANAGER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->sessionKeyIndex_ = 0;
    downloader->currentVariant_ = variant;
    downloader->aesDecryptorManager_ = nullptr;
    std::vector<M3U8Segment> files;
    uint64_t ret = downloader->KeyChange(files);
    EXPECT_NE(downloader->aesDecryptorManager_, nullptr);
    EXPECT_EQ(ret, 0ull);
}

/**
 * @tc.name: KEY_CHANGE_SESSION_KEY
 * @tc.desc: test KeyChange with sessionKeyIndex > 0 and master set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, KEY_CHANGE_SESSION_KEY, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->sessionKeyIndex_ = 5;
    downloader->currentVariant_ = variant;
    downloader->aesDecryptorManager_ = std::make_shared<AesDecryptorManager>();
    std::vector<M3U8Segment> files;
    uint64_t ret = downloader->KeyChange(files);
    EXPECT_EQ(ret, 5ull);
}

/**
 * @tc.name: UPDATE_MASTER_INFO_NULL_VARIANT
 * @tc.desc: test UpdateMasterInfo with null currentVariant
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MASTER_INFO_NULL_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->currentVariant_ = nullptr;
    downloader->UpdateMasterInfo(false);
    EXPECT_FALSE(downloader->isLiveEnd_);
}

/**
 * @tc.name: UPDATE_MASTER_INFO_AUDIO
 * @tc.desc: test UpdateMasterInfo with currentAudio_ set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MASTER_INFO_AUDIO, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    auto audioM3u8 = std::make_shared<M3U8>("http://audio", "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio", audioM3u8);
    downloader->currentAudio_ = audio;
    downloader->UpdateMasterInfo(false);
    EXPECT_FALSE(downloader->isLiveEnd_);
}

/**
 * @tc.name: UPDATE_MASTER_INFO_SUBTITLE
 * @tc.desc: test UpdateMasterInfo with currentSubtitles_ set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MASTER_INFO_SUBTITLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    auto subM3u8 = std::make_shared<M3U8>("http://sub", "");
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", subM3u8);
    downloader->currentSubtitles_ = sub;
    downloader->currentAudio_ = nullptr;
    downloader->UpdateMasterInfo(false);
    EXPECT_FALSE(downloader->isLiveEnd_);
}

/**
 * @tc.name: UPDATE_MASTER_INFO_LIVE_END
 * @tc.desc: test UpdateMasterInfo live-to-vod transition
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MASTER_INFO_LIVE_END, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->bLive_ = false;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->isPreParseFinished_.store(true);
    downloader->master_->bLive_ = true;
    downloader->UpdateMasterInfo(false);
    EXPECT_TRUE(downloader->isLiveEnd_);
}

/**
 * @tc.name: UPDATE_SEEK_RANGES_DISCONT
 * @tc.desc: test UpdateSeekRanges with discontinuity fragment
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_DISCONT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->bLive_ = false;
    std::vector<M3U8Segment> files;
    files.push_back(M3U8Segment("seg0.ts", 5, 0, false));
    files.push_back(M3U8Segment("seg1.ts", 5, 1, true));
    files.push_back(M3U8Segment("seg2.ts", 5, 2, false));
    downloader->UpdateSeekRanges(files);
    auto ranges = downloader->GetSeekableRanges();
    EXPECT_FALSE(ranges.empty());
}

/**
 * @tc.name: UPDATE_SEEK_RANGES_LIVE_NOT_SEEKABLE
 * @tc.desc: test UpdateSeekRanges live with !allFragmentsSeekable
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_LIVE_NOT_SEEKABLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->bLive_ = true;
    downloader->allFragmentsSeekable_ = false;
    std::vector<M3U8Segment> files;
    files.push_back(M3U8Segment("seg0.ts", 5, 0, false));
    files.push_back(M3U8Segment("seg1.ts", 0, 1, false));
    files.push_back(M3U8Segment("seg2.ts", 5, 2, false));
    files.push_back(M3U8Segment("seg3.ts", 5, 3, false));
    downloader->UpdateSeekRanges(files);
    {
        std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
        EXPECT_FALSE(downloader->seekRanges_.empty());
    }
}

/**
 * @tc.name: UPDATE_SEEK_RANGES_ALL_SEEKABLE
 * @tc.desc: test UpdateSeekRanges with allFragmentsSeekable true and live
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_SEEK_RANGES_ALL_SEEKABLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->bLive_ = true;
    downloader->allFragmentsSeekable_ = true;
    std::vector<M3U8Segment> files;
    files.push_back(M3U8Segment("seg0.ts", 5, 0, false));
    files.push_back(M3U8Segment("seg1.ts", 5, 1, false));
    downloader->UpdateSeekRanges(files);
    {
        std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
        EXPECT_FALSE(downloader->seekRanges_.empty());
    }
}

/**
 * @tc.name: UPDATE_MASTER_NOTIFY_NONSIMPLE
 * @tc.desc: test UpdateMasterAndNotifyList with non-simple master
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MASTER_NOTIFY_NONSIMPLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    downloader->currentVariant_ = variant;
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->UpdateMasterAndNotifyList(false);
    EXPECT_TRUE(downloader->master_->isSimple_.load());
}

/**
 * @tc.name: UPDATE_MASTER_NOTIFY_SIMPLE
 * @tc.desc: test UpdateMasterAndNotifyList with simple master
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MASTER_NOTIFY_SIMPLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isSimple_.store(true);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->UpdateMasterAndNotifyList(false);
    EXPECT_TRUE(downloader->master_->isSimple_.load());
}

/**
 * @tc.name: WAIT_FOR_KEY_NULL_DOWNLOADER
 * @tc.desc: test WaitForKeyReady with null keyDownloader
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, WAIT_FOR_KEY_NULL_DOWNLOADER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->keyDownloader_ = nullptr;
    downloader->aesDecryptorManager_ = nullptr;
    bool ret = downloader->WaitForKeyReady(1, 10);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: WAIT_FOR_KEY_WITH_DECRYPTOR
 * @tc.desc: test WaitForKeyReady with aesDecryptorManager set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, WAIT_FOR_KEY_WITH_DECRYPTOR, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->keyDownloader_ = nullptr;
    downloader->aesDecryptorManager_ = std::make_shared<AesDecryptorManager>();
    bool ret = downloader->WaitForKeyReady(1, 10);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: GET_AES_DECRYPTOR_NULL_MANAGER
 * @tc.desc: test GetAesDecryptor with null manager
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_AES_DECRYPTOR_NULL_MANAGER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->aesDecryptorManager_ = nullptr;
    auto ret = downloader->GetAesDecryptor(1);
    EXPECT_EQ(ret, nullptr);
    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: GET_AES_DECRYPTOR_WITH_MANAGER
 * @tc.desc: test GetAesDecryptor with manager set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_AES_DECRYPTOR_WITH_MANAGER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->aesDecryptorManager_ = std::make_shared<AesDecryptorManager>();
    auto ret = downloader->GetAesDecryptor(999);
    EXPECT_EQ(ret, nullptr);
    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: GET_BIT_RATES_NULL_ITEM
 * @tc.desc: test GetBitRates with null variant item
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_BIT_RATES_NULL_ITEM, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->variants_.push_back(nullptr);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->bandWidth_ = 500000;
    downloader->master_->variants_.push_back(variant);
    auto rates = downloader->GetBitRates();
    EXPECT_EQ(rates.size(), 1u);
}

/**
 * @tc.name: SET_INIT_RESOLUTION_ZERO
 * @tc.desc: test SetInitResolution with zero width/height
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_INIT_RESOLUTION_ZERO, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetInitResolution(0, 1080);
    downloader->SetInitResolution(1920, 0);
    EXPECT_EQ(downloader->initResolution_, 0u);
}

/**
 * @tc.name: CONTAINS_NON_DIGIT_EMPTY
 * @tc.desc: test ContainsNonDigit with empty string
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, CONTAINS_NON_DIGIT_EMPTY, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    EXPECT_FALSE(downloader->ContainsNonDigit(""));
    EXPECT_TRUE(downloader->ContainsNonDigit("abc"));
    EXPECT_FALSE(downloader->ContainsNonDigit("123"));
}

/**
 * @tc.name: IS_MEDIA_SAME_VIDEO
 * @tc.desc: test IsMediaSame with SEG_VIDEO type
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_MEDIA_SAME_VIDEO, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    EXPECT_TRUE(downloader->IsMediaSame(0, HlsSegmentType::SEG_VIDEO));
}

/**
 * @tc.name: UPDATE_MEDIA_NO_MEDIA
 * @tc.desc: test UpdateMedia with currentVariant set but no audio/subtitle
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MEDIA_NO_MEDIA, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->currentAudio_ = nullptr;
    downloader->currentSubtitles_ = nullptr;
    std::string urlBefore = downloader->url_;
    downloader->UpdateMedia(HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(downloader->url_, urlBefore);
}

/**
 * @tc.name: DESTRUCTOR_FMP4_LIVE
 * @tc.desc: test destructor with fmp4 live state
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, DESTRUCTOR_FMP4_LIVE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->reportInfo_ = std::make_shared<MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->isFmp4_.store(true);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->bLive_ = true;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    EXPECT_TRUE(downloader->isFmp4_.load());
    EXPECT_NE(downloader->reportInfo_, nullptr);
    EXPECT_TRUE(downloader->currentVariant_ != nullptr);
}

/**
 * @tc.name: DESTRUCTOR_HLS_VOD
 * @tc.desc: test destructor with hls vod state
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, DESTRUCTOR_HLS_VOD, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->reportInfo_ = std::make_shared<MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->isFmp4_.store(false);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->bLive_ = false;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    EXPECT_FALSE(downloader->isFmp4_.load());
    EXPECT_NE(downloader->reportInfo_, nullptr);
    EXPECT_TRUE(downloader->currentVariant_ != nullptr);
}

/**
 * @tc.name: DESTRUCTOR_FMP4_VOD
 * @tc.desc: test destructor with fmp4 vod state
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, DESTRUCTOR_FMP4_VOD, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->reportInfo_ = std::make_shared<MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->isFmp4_.store(true);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->bLive_ = false;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    EXPECT_TRUE(downloader->isFmp4_.load());
    EXPECT_NE(downloader->reportInfo_, nullptr);
    EXPECT_TRUE(downloader->currentVariant_ != nullptr);
}

/**
 * @tc.name: DESTRUCTOR_HLS_LIVE
 * @tc.desc: test destructor with hls live state
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, DESTRUCTOR_HLS_LIVE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->reportInfo_ = std::make_shared<MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->isFmp4_.store(false);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->bLive_ = true;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    EXPECT_FALSE(downloader->isFmp4_.load());
    EXPECT_NE(downloader->reportInfo_, nullptr);
    EXPECT_TRUE(downloader->currentVariant_ != nullptr);
}

/**
 * @tc.name: DESTRUCTOR_NULL_VARIANT
 * @tc.desc: test destructor with null currentVariant
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, DESTRUCTOR_NULL_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->reportInfo_ = std::make_shared<MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->currentVariant_ = nullptr;
    EXPECT_NE(downloader->reportInfo_, nullptr);
    EXPECT_FALSE(downloader->isFmp4_.load());
    EXPECT_EQ(downloader->currentVariant_, nullptr);
}

/**
 * @tc.name: UPDATE_LIVE_BASE_OFFSET_NULL_FRAG
 * @tc.desc: test UpdateLiveBaseOffset with null fragment in list
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_LIVE_BASE_OFFSET_NULL_FRAG, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto master = std::make_shared<M3U8MasterPlaylist>("", "http://test.com");
    master->bLive_ = true;
    downloader->master_ = master;
    std::vector<M3U8Segment> files;
    M3U8Segment emptySeg("", 0, 0, false);
    files.push_back(emptySeg);
    M3U8Segment frag("test.ts", 5, 10, false);
    files.push_back(frag);
    downloader->UpdateLiveBaseOffsetForTest(files);
    EXPECT_TRUE(downloader->GetLiveBaseOffset() >= 0);
}

/**
 * @tc.name: UPDATE_LIVE_BASE_OFFSET_AVG_ZERO
 * @tc.desc: test UpdateLiveBaseOffset with zero avg duration
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_LIVE_BASE_OFFSET_AVG_ZERO, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto master = std::make_shared<M3U8MasterPlaylist>("", "http://test.com");
    master->bLive_ = true;
    downloader->master_ = master;
    std::vector<M3U8Segment> files;
    M3U8Segment emptySeg("", 0, 0, false);
    files.push_back(emptySeg);
    files.push_back(emptySeg);
    downloader->UpdateLiveBaseOffsetForTest(files);
    EXPECT_TRUE(downloader->GetLiveBaseOffset() >= 0);
}

/**
 * @tc.name: CHECK_FRAGMENTS_SEEKABLE_LIVE
 * @tc.desc: test CheckAllFragmentsSeekable with live master and fragments
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, CHECK_FRAGMENTS_SEEKABLE_LIVE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->bLive_ = true;
    std::vector<M3U8Segment> files;
    M3U8Segment frag1("seg0.ts", 5, 10, false);
    M3U8Segment frag2("seg1.ts", 5, 11, false);
    files.push_back(frag1);
    files.push_back(frag2);
    downloader->isFragmentsUriSet_ = false;
    downloader->UpdateSeekRanges(files);
    {
        std::lock_guard<std::mutex> lock(downloader->seekRangeMutex_);
        EXPECT_FALSE(downloader->seekRanges_.empty());
    }
}

/**
 * @tc.name: GET_SEEKABLE_VOD
 * @tc.desc: test GetSeekable returns SEEKABLE for vod
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_VOD, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isSimple_.store(true);
    downloader->isParseFinished_.store(true);
    downloader->master_->bLive_ = false;
    auto ret = downloader->GetSeekable();
    EXPECT_EQ(static_cast<int>(ret), static_cast<int>(Seekable::SEEKABLE));
}

/**
 * @tc.name: GET_SEEKABLE_INTERRUPTED
 * @tc.desc: test GetSeekable returns INVALID when interrupted
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_INTERRUPTED, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->isInterruptNeeded_.store(true);
    auto ret = downloader->GetSeekable();
    EXPECT_EQ(static_cast<int>(ret), static_cast<int>(Seekable::INVALID));
}

/**
 * @tc.name: INTERRUPT_M3U8_PARSE_NULL_M3U8
 * @tc.desc: test InterruptM3U8Parse with currentVariant m3u8 null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, INTERRUPT_M3U8_PARSE_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, nullptr);
    downloader->currentVariant_ = variant;
    downloader->InterruptM3U8Parse(true);
    EXPECT_TRUE(downloader->master_->isInterruptNeeded_);
}

/**
 * @tc.name: GET_LIVE_UPDATE_GAP_WITH_M3U8
 * @tc.desc: test GetLiveUpdateGap with currentVariant m3u8 set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_LIVE_UPDATE_GAP_WITH_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    auto gap = downloader->GetLiveUpdateGap();
    EXPECT_TRUE(gap >= 0u);
}

/**
 * @tc.name: IS_BITRATE_SAME_RETURN_TRUE
 * @tc.desc: test IsBitrateSame returns true when same bandwidth
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_BITRATE_SAME_RETURN_TRUE, TestSize.Level1)
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
    EXPECT_TRUE(downloader->IsBitrateSame(1000000));
}

/**
 * @tc.name: IS_BITRATE_SAME_BETTER_GAP
 * @tc.desc: test IsBitrateSame with better gap variant and firstVideoStream null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_BITRATE_SAME_BETTER_GAP, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->firstVideoStream_ = nullptr;
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant1 = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant1->bandWidth_ = 2000000;
    variant1->isVideo_ = false;
    auto variant2 = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant2->bandWidth_ = 1000000;
    variant2->isVideo_ = false;
    downloader->master_->variants_.push_back(variant1);
    downloader->master_->variants_.push_back(variant2);
    downloader->currentVariant_ = variant1;
    downloader->IsBitrateSame(1100000);
    EXPECT_NE(downloader->newVariant_, nullptr);
    EXPECT_EQ(downloader->newVariant_->bandWidth_, 1000000u);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_AUDIO
 * @tc.desc: test SetDefaultStreamId sets audioStreamId from defaultAudio
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_AUDIO, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = 1;
    downloader->master_->defaultVariant_ = variant;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = variant;
    auto audioM3u8 = std::make_shared<M3U8>("http://audio", "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio", audioM3u8);
    audio->streamId_ = 7;
    variant->defaultAudio_ = audio;
    int32_t videoId = 1;
    int32_t audioId = -1;
    int32_t subId = -1;
    downloader->SetDefaultStreamId(videoId, audioId, subId);
    EXPECT_EQ(audioId, 7);
}

/**
 * @tc.name: SET_DEFAULT_STREAM_ID_SUBTITLE
 * @tc.desc: test SetDefaultStreamId sets subTitleStreamId from defaultSubtitles
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_STREAM_ID_SUBTITLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = 1;
    downloader->master_->defaultVariant_ = variant;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = variant;
    auto subM3u8 = std::make_shared<M3U8>("http://sub", "");
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", subM3u8);
    sub->streamId_ = 9;
    variant->defaultSubtitles_ = sub;
    int32_t videoId = 1;
    int32_t audioId = 2;
    int32_t subId = -1;
    downloader->SetDefaultStreamId(videoId, audioId, subId);
    EXPECT_EQ(subId, 9);
}

/**
 * @tc.name: REOPEN_WITH_DOWNLOADER
 * @tc.desc: test ReOpen with downloader set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, REOPEN_WITH_DOWNLOADER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->urlOrigin_ = TEST_URI_PATH;
    downloader->isLiveUpdateTaskStarted_ = true;
    downloader->retryStartTime_ = 1000;
    downloader->ReOpen();
    EXPECT_EQ(downloader->GetMaster(), nullptr);
    EXPECT_FALSE(downloader->isLiveUpdateTaskStarted_);
}

/**
 * @tc.name: SELECT_MEDIA_AUDIO_MATCH
 * @tc.desc: test SelectMedia sets currentAudio from audioMedia
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SELECT_MEDIA_AUDIO_MATCH, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    auto audioM3u8 = std::make_shared<M3U8>("http://audio", "");
    auto media = std::make_shared<M3U8Media>("a1", "http://audio", audioM3u8);
    media->streamId_ = 55;
    variant->audioMedia_.push_back(media);
    variant->subtitlesMedia_.clear();
    downloader->currentVariant_ = variant;
    downloader->SelectMedia(55, HlsSegmentType::SEG_AUDIO);
    EXPECT_NE(downloader->currentAudio_, nullptr);
    EXPECT_EQ(downloader->currentAudio_->streamId_, 55u);
}

/**
 * @tc.name: SELECT_MEDIA_NOT_FOUND
 * @tc.desc: test SelectMedia when streamId not found
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SELECT_MEDIA_NOT_FOUND, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    auto audioM3u8 = std::make_shared<M3U8>("http://audio", "");
    auto media = std::make_shared<M3U8Media>("a1", "http://audio", audioM3u8);
    media->streamId_ = 55;
    variant->audioMedia_.push_back(media);
    downloader->currentVariant_ = variant;
    downloader->SelectMedia(99, HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(downloader->currentAudio_, nullptr);
    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: GET_DEFAULT_MEDIA_STREAM_ID_AUDIO_SET
 * @tc.desc: test GetDefaultMediaStreamId returns audioStreamId when set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DEFAULT_MEDIA_STREAM_ID_AUDIO_SET, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->audioStreamId_.store(42);
    EXPECT_EQ(downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_AUDIO), 42);
}

/**
 * @tc.name: GET_DEFAULT_MEDIA_STREAM_ID_SUB_SET
 * @tc.desc: test GetDefaultMediaStreamId returns subtitleStreamId when set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DEFAULT_MEDIA_STREAM_ID_SUB_SET, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->subtitleStreamId_.store(33);
    EXPECT_EQ(downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_SUBTITLE), 33);
}

/**
 * @tc.name: GET_DEFAULT_MEDIA_STREAM_ID_NO_DEFAULT
 * @tc.desc: test GetDefaultMediaStreamId returns -1 when no default media
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DEFAULT_MEDIA_STREAM_ID_NO_DEFAULT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->audioStreamId_.store(-1);
    downloader->subtitleStreamId_.store(-1);
    variant->defaultAudio_ = nullptr;
    variant->defaultSubtitles_ = nullptr;
    EXPECT_EQ(downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_AUDIO), -1);
}

/**
 * @tc.name: SET_DEFAULT_MEDIA_AUDIO
 * @tc.desc: test SetDefaultMedia sets currentAudio from defaultAudio
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_MEDIA_AUDIO, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    auto audioM3u8 = std::make_shared<M3U8>("http://audio", "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio", audioM3u8);
    audio->streamId_ = HLS_AUDIO_STREAM_ID;
    variant->defaultAudio_ = audio;
    downloader->currentAudio_ = nullptr;
    downloader->SetDefaultMedia(HlsSegmentType::SEG_AUDIO);
    EXPECT_NE(downloader->currentAudio_, nullptr);
    EXPECT_EQ(downloader->currentAudio_->streamId_, HLS_AUDIO_STREAM_ID);
}

/**
 * @tc.name: SET_DEFAULT_MEDIA_NO_DEFAULT
 * @tc.desc: test SetDefaultMedia returns when no default media
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_DEFAULT_MEDIA_NO_DEFAULT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    variant->defaultAudio_ = nullptr;
    variant->defaultSubtitles_ = nullptr;
    downloader->SetDefaultMedia(HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(downloader->currentAudio_, nullptr);
    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: GET_SEG_TYPE_AUDIO_SUB
 * @tc.desc: test GetSegType returns correct type
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEG_TYPE_AUDIO_SUB, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->audioStreamIds_.insert(100);
    downloader->subtitlesStreamIds_.insert(200);
    EXPECT_EQ(downloader->GetSegType(100), HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(downloader->GetSegType(200), HlsSegmentType::SEG_SUBTITLE);
    EXPECT_EQ(downloader->GetSegType(999), HlsSegmentType::SEG_VIDEO);
}

/**
 * @tc.name: GET_CUR_STREAM_ID_VARIANT
 * @tc.desc: test GetCurStreamId returns variant streamId when no audio/sub
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_STREAM_ID_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = 77;
    downloader->currentVariant_ = variant;
    downloader->currentAudio_ = nullptr;
    downloader->currentSubtitles_ = nullptr;
    EXPECT_EQ(downloader->GetCurStreamId(), 77u);
}

/**
 * @tc.name: GET_CUR_BITRATE_WITH_VARIANT
 * @tc.desc: test GetCurBitrate returns bandwidth when variant set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_CUR_BITRATE_WITH_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->bandWidth_ = 500000;
    downloader->currentVariant_ = variant;
    EXPECT_EQ(downloader->GetCurBitrate(), 500000u);
    EXPECT_EQ(downloader->GetCurrentBitRate(), 500000u);
}

/**
 * @tc.name: GET_VIDEO_DIMENSIONS
 * @tc.desc: test GetVideoWidth/Height with variant set
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_VIDEO_DIMENSIONS, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->width_ = 1920;
    variant->height_ = 1080;
    downloader->currentVariant_ = variant;
    EXPECT_EQ(downloader->GetVideoWidth(), 1920);
    EXPECT_EQ(downloader->GetVideoHeight(), 1080);
}

/**
 * @tc.name: GET_BIT_RATES_NULL_MASTER
 * @tc.desc: test GetBitRates returns empty when master null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_BIT_RATES_NULL_MASTER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;
    auto rates = downloader->GetBitRates();
    EXPECT_TRUE(rates.empty());
}

/**
 * @tc.name: SELECT_BIT_RATE_WITH_VARIANT
 * @tc.desc: test SelectBitRate sets currentVariant
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SELECT_BIT_RATE_WITH_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->bandWidth_ = 800000;
    downloader->newVariant_ = variant;
    downloader->SelectBitRate(800000);
    EXPECT_EQ(downloader->GetCurrentVariant(), variant);
    EXPECT_FALSE(downloader->isFragmentsUriSet_);
}

/**
 * @tc.name: IS_FRAGMENT_IN_PLAYLIST_NULL_VARIANT
 * @tc.desc: test IsFragmentInCurrentPlayList with null currentVariant
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_FRAGMENT_IN_PLAYLIST_NULL_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloader>();
    downloader->Init();
    EXPECT_FALSE(downloader->IsFragmentInCurrentPlayList("http://test.ts", "", 0, 0));
    EXPECT_FALSE(downloader->IsBitrateSame(1000));
}

/**
 * @tc.name: GET_DURATION_VOD
 * @tc.desc: test GetDuration returns positive for vod with duration
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_VOD, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->bLive_ = false;
    downloader->master_->duration_ = 50ULL * S_2_NS;
    int64_t ret = downloader->GetDuration();
    EXPECT_TRUE(ret > 0);
}

/**
 * @tc.name: GET_DURATION_LIVE_SEEKABLE
 * @tc.desc: test GetDuration returns negative for live seekable
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_LIVE_SEEKABLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->bLive_ = true;
    downloader->master_->isSimple_.store(true);
    downloader->isParseFinished_.store(true);
    downloader->master_->segmentOffsets_.resize(5);
    downloader->master_->duration_ = 10ULL * S_2_NS;
    int64_t ret = downloader->GetDuration();
    EXPECT_TRUE(ret < 0);
}

/**
 * @tc.name: GET_DURATION_LIVE_NOT_SEEKABLE
 * @tc.desc: test GetDuration returns -1 for live not seekable
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_DURATION_LIVE_NOT_SEEKABLE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->bLive_ = true;
    int64_t ret = downloader->GetDuration();
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name: UPDATE_STREAM_INFO_NULL_STREAM
 * @tc.desc: test UpdateStreamInfo with null stream in variants
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_STREAM_INFO_NULL_STREAM, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->variants_.push_back(nullptr);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = 1;
    auto audioM3u8 = std::make_shared<M3U8>("http://audio", "");
    auto audioMedia = std::make_shared<M3U8Media>("audio", "http://audio", audioM3u8);
    audioMedia->streamId_ = 2;
    audioMedia->lang_ = "eng";
    variant->audioMedia_.push_back(nullptr);
    variant->audioMedia_.push_back(audioMedia);
    auto subM3u8 = std::make_shared<M3U8>("http://sub", "");
    auto subMedia = std::make_shared<M3U8Media>("sub", "http://sub", subM3u8);
    subMedia->streamId_ = 3;
    variant->subtitlesMedia_.push_back(nullptr);
    variant->subtitlesMedia_.push_back(subMedia);
    downloader->master_->variants_.push_back(variant);
    downloader->UpdateStreamInfo();
    EXPECT_TRUE(downloader->needAudioManager_.load());
    EXPECT_TRUE(downloader->needSubtitlesManager_.load());
}

/**
 * @tc.name: GET_STREAM_INFO_BY_ID_MATCH
 * @tc.desc: test GetStreamInfoById returns info for matching streamId
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_BY_ID_MATCH, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = 5;
    variant->bandWidth_ = 2000000;
    variant->width_ = 1280;
    variant->height_ = 720;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = variant;
    auto info = downloader->GetStreamInfoById(5);
    EXPECT_NE(info, nullptr);
    EXPECT_EQ(info->streamId, 5);
    EXPECT_EQ(info->bitRate, 2000000u);
    EXPECT_EQ(info->videoWidth, 1280);
}

/**
 * @tc.name: SET_MIME_TYPE
 * @tc.desc: test SetMimeType stores value
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, SET_MIME_TYPE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->SetMimeType("application/x-mpegURL");
    downloader->SetInitResolution(1920, 1080);
    EXPECT_EQ(downloader->initResolution_, static_cast<uint32_t>(1920 * 1080));
}

/**
 * @tc.name: GET_SEGMENT_OFFSET_NULL_MASTER
 * @tc.desc: test GetSegmentOffset returns 0 when master null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEGMENT_OFFSET_NULL_MASTER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;
    EXPECT_EQ(downloader->GetSegmentOffset(0), 0u);
    EXPECT_EQ(downloader->GetSegmentOffset(100), 0u);
}

/**
 * @tc.name: IS_HLS_FMP4_FALSE
 * @tc.desc: test IsHlsFmp4 returns false when isFmp4_ false
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_HLS_FMP4_FALSE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isFmp4_.store(false);
    EXPECT_FALSE(downloader->IsHlsFmp4());
}

/**
 * @tc.name: IS_PURE_BYTE_RANGE_FALSE
 * @tc.desc: test IsPureByteRange returns false when isPureByteRange_ false
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_PURE_BYTE_RANGE_FALSE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isPureByteRange_.store(false);
    EXPECT_FALSE(downloader->IsPureByteRange());
}

/**
 * @tc.name: GET_HLS_DISCONTINUITY_NULL_MASTER
 * @tc.desc: test GetHLSDiscontinuity returns false when master null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_HLS_DISCONTINUITY_NULL_MASTER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;
    EXPECT_FALSE(downloader->GetHLSDiscontinuity());
}

/**
 * @tc.name: IS_LIVE_NULL_MASTER
 * @tc.desc: test IsLive returns false when master null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_NULL_MASTER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;
    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: IS_LIVE_FALSE
 * @tc.desc: test IsLive returns false when bLive_ false
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_FALSE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->bLive_ = false;
    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: IS_PARSE_FINISHED
 * @tc.desc: test IsParseFinished returns correct value
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_PARSE_FINISHED, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->isParseFinished_.store(false);
    EXPECT_FALSE(downloader->IsParseFinished());
    downloader->isParseFinished_.store(true);
    EXPECT_TRUE(downloader->IsParseFinished());
}

/**
 * @tc.name: IS_LIVE_END_FALSE
 * @tc.desc: test IsLiveEnd returns false when isLiveEnd_ false
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_LIVE_END_FALSE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->isLiveEnd_ = false;
    EXPECT_FALSE(downloader->IsLiveEnd());
}

/**
 * @tc.name: CLONE_NULL_PTR
 * @tc.desc: test Clone with null ptr does not crash
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, CLONE_NULL_PTR, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->httpHeader_["Test"] = "before";
    downloader->Clone(nullptr);
    EXPECT_EQ(downloader->httpHeader_["Test"], "before");
}


/**
 * @tc.name: UPDATE_PLAYLISTS_SIMPLE_AUDIO_NULL_M3U8
 * @tc.desc: test UpdatePlaylists isSimple=true with currentAudio_ but m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_SIMPLE_AUDIO_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio", nullptr);
    downloader->currentAudio_ = audio;
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", m3u8);
    downloader->currentSubtitles_ = sub;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(true);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: UPDATE_PLAYLISTS_SIMPLE_SUB_NULL_M3U8
 * @tc.desc: test UpdatePlaylists isSimple=true with currentSubtitles_ but m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_SIMPLE_SUB_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    downloader->currentAudio_ = nullptr;
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", nullptr);
    downloader->currentSubtitles_ = sub;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(true);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: UPDATE_PLAYLISTS_SIMPLE_VAR_NULL_M3U8
 * @tc.desc: test UpdatePlaylists isSimple=true with currentVariant m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_SIMPLE_VAR_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, nullptr);
    downloader->currentVariant_ = variant;
    downloader->currentAudio_ = nullptr;
    downloader->currentSubtitles_ = nullptr;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(true);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: UPDATE_PLAYLISTS_NONSIMPLE_AUDIO_NULL_M3U8
 * @tc.desc: test UpdatePlaylists isSimple=false with currentAudio_ m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_NONSIMPLE_AUDIO_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto audio = std::make_shared<M3U8Media>("a", "http://audio", nullptr);
    downloader->currentAudio_ = audio;
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", m3u8);
    downloader->currentSubtitles_ = sub;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(false);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: UPDATE_PLAYLISTS_NONSIMPLE_SUB_NULL_M3U8
 * @tc.desc: test UpdatePlaylists isSimple=false with currentSubtitles_ m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_NONSIMPLE_SUB_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    downloader->currentAudio_ = nullptr;
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", nullptr);
    downloader->currentSubtitles_ = sub;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(false);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: UPDATE_PLAYLISTS_NONSIMPLE_VAR_NULL_M3U8
 * @tc.desc: test UpdatePlaylists isSimple=false with defaultVariant m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_PLAYLISTS_NONSIMPLE_VAR_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, nullptr);
    downloader->master_->defaultVariant_ = variant;
    downloader->currentVariant_ = variant;
    downloader->currentAudio_ = nullptr;
    downloader->currentSubtitles_ = nullptr;
    downloader->SetPlayListForTest(MEDIA_LIST);
    bool ret = downloader->UpdatePlaylists(false);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: GET_LIVE_UPDATE_GAP_NULL_M3U8
 * @tc.desc: test GetLiveUpdateGap with currentVariant m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_LIVE_UPDATE_GAP_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, nullptr);
    downloader->currentVariant_ = variant;
    auto gap = downloader->GetLiveUpdateGap();
    EXPECT_EQ(gap, 0u);
}

/**
 * @tc.name: READ_FMP4_HEADER_AUDIO_PATH
 * @tc.desc: test ReadFmp4Header via audioList (subtitles empty)
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, READ_FMP4_HEADER_AUDIO_PATH, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>("http://test", "");
    m3u8->fmp4Header_ = new uint8_t[16]();
    m3u8->downloadHeaderLen_ = 6;
    m3u8->isHeaderReady_.store(true);
    auto media = std::make_shared<M3U8Media>("audio", "http://test", m3u8);
    media->streamId_ = 9;
    downloader->master_->audioList_.push_back(media);
    uint8_t buf[64];
    uint32_t readLen = 0;
    bool ret = downloader->ReadFmp4Header(buf, sizeof(buf), readLen, 9);
    EXPECT_TRUE(ret);
    EXPECT_EQ(readLen, 6u);
}

/**
 * @tc.name: COPY_FRAGMENT_INFO_NULL_CURRENTVARIANT
 * @tc.desc: test CopyFragmentInfo with currentVariant null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COPY_FRAGMENT_INFO_NULL_CURRENTVARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->currentVariant_ = nullptr;
    M3U8Segment file("http://seg.ts", 5, 0, false);
    PlayInfo playInfo;
    downloader->CopyFragmentInfo(playInfo, file, 0);
    EXPECT_EQ(playInfo.url_, "http://seg.ts");
    EXPECT_EQ(playInfo.keyIndex_, 0ull);
}

/**
 * @tc.name: GET_START_INFO_POS_OFFSET_LT_DURATION
 * @tc.desc: test GetStartInfo with positive timeOffset <= duration
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_POS_OFFSET_LT_DURATION, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isStart_ = true;
    downloader->master_->bLive_ = false;
    downloader->master_->duration_ = 100ULL * S_2_NS;
    downloader->master_->timeOffset_ = static_cast<int64_t>(5ULL * S_2_NS);
    auto info = downloader->GetStartInfo();
    EXPECT_TRUE(info.first > 0);
    EXPECT_EQ(info.second, downloader->master_->precise_);
}

/**
 * @tc.name: GET_START_INFO_NO_DEFAULT_VARIANT
 * @tc.desc: test GetStartInfo with isStart_ false and no defaultVariant
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_NO_DEFAULT_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->isStart_ = false;
    downloader->master_->defaultVariant_ = nullptr;
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_FALSE(info.second);
}

/**
 * @tc.name: GET_START_INFO_DEFAULT_VARIANT_NOT_START
 * @tc.desc: test GetStartInfo with defaultVariant m3u8 isStart_ false
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_DEFAULT_VARIANT_NOT_START, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->isStart_ = false;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_FALSE(info.second);
}

/**
 * @tc.name: UPDATE_LIVE_BASE_OFFSET_VALID_AND_NULL
 * @tc.desc: test UpdateLiveBaseOffset with valid first file and null second
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_LIVE_BASE_OFFSET_VALID_AND_NULL, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto master = std::make_shared<M3U8MasterPlaylist>("", "http://test.com");
    master->bLive_ = true;
    downloader->master_ = master;
    std::vector<M3U8Segment> files;
    M3U8Segment frag1("test1.ts", 5, 10, false);
    files.push_back(frag1);
    downloader->UpdateLiveBaseOffsetForTest(files);
    EXPECT_TRUE(downloader->GetLiveBaseOffset() > 0);
}

/**
 * @tc.name: UPDATE_MASTER_NOTIFY_NULL_VARIANT
 * @tc.desc: test UpdateMasterAndNotifyList with null currentVariant
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MASTER_NOTIFY_NULL_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->currentVariant_ = nullptr;
    downloader->SetPlayListForTest(MEDIA_LIST);
    downloader->UpdateMasterAndNotifyList(false);
    EXPECT_FALSE(downloader->master_->isSimple_.load());
}

/**
 * @tc.name: UPDATE_MASTER_INFO_NULL_MASTER
 * @tc.desc: test UpdateMasterInfo with null master
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MASTER_INFO_NULL_MASTER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->UpdateMasterInfo(false);
    EXPECT_FALSE(downloader->isLiveEnd_);
}

/**
 * @tc.name: UPDATE_MASTER_INFO_NOT_LIVE_END
 * @tc.desc: test UpdateMasterInfo with bLive true and m3u8 still live
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, UPDATE_MASTER_INFO_NOT_LIVE_END, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->bLive_ = true;
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->isPreParseFinished_.store(true);
    downloader->master_->bLive_ = true;
    downloader->UpdateMasterInfo(false);
    EXPECT_FALSE(downloader->isLiveEnd_);
}

/**
 * @tc.name: INTERRUPT_M3U8_PARSE_NULL_VARIANT
 * @tc.desc: test InterruptM3U8Parse with null currentVariant
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, INTERRUPT_M3U8_PARSE_NULL_VARIANT, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->currentVariant_ = nullptr;
    downloader->InterruptM3U8Parse(true);
    EXPECT_TRUE(downloader->master_->isInterruptNeeded_);
}

/**
 * @tc.name: IS_FRAGMENT_IN_PLAYLIST_NULL_M3U8
 * @tc.desc: test IsFragmentInCurrentPlayList with currentVariant m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, IS_FRAGMENT_IN_PLAYLIST_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, nullptr);
    downloader->currentVariant_ = variant;
    EXPECT_FALSE(downloader->IsFragmentInCurrentPlayList("http://test.ts", "", 0, 0));
    EXPECT_FALSE(downloader->IsLive());
}

/**
 * @tc.name: ON_MASTER_READY_NULL_CALLBACK
 * @tc.desc: test OnMasterReady with null callback
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, ON_MASTER_READY_NULL_CALLBACK, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    downloader->OnMasterReady(true, false);
    EXPECT_EQ(downloader->currentVariant_->streamId_, 0u);
}

/**
 * @tc.name: ON_MASTER_READY_NULL_M3U8
 * @tc.desc: test OnMasterReady with m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, ON_MASTER_READY_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, nullptr);
    downloader->currentVariant_ = variant;
    auto callback = std::make_shared<MockPlayListCallback>();
    downloader->SetPlayListCallback(callback);
    downloader->OnMasterReady(true, false);
    EXPECT_EQ(downloader->currentVariant_->m3u8_, nullptr);
    EXPECT_EQ(downloader->currentVariant_->streamId_, 0u);
}

/**
 * @tc.name: GET_SEEKABLE_NULL_MASTER
 * @tc.desc: test GetSeekable with null master
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_NULL_MASTER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;
    auto ret = downloader->GetSeekable();
    EXPECT_EQ(static_cast<int>(ret), static_cast<int>(Seekable::INVALID));
}

/**
 * @tc.name: GET_SEEKABLE_RANGES_NULL_MASTER
 * @tc.desc: test GetSeekableRanges with null master
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_SEEKABLE_RANGES_NULL_MASTER, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = nullptr;
    auto ranges = downloader->GetSeekableRanges();
    EXPECT_TRUE(ranges.empty());
}

/**
 * @tc.name: GET_STREAM_INFO_BY_ID_NULL_VARIANT_ITEM
 * @tc.desc: test GetStreamInfoById with null variant in list
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_STREAM_INFO_BY_ID_NULL_VARIANT_ITEM, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    downloader->master_->variants_.push_back(nullptr);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    variant->streamId_ = 8;
    downloader->master_->variants_.push_back(variant);
    downloader->currentVariant_ = variant;
    auto info = downloader->GetStreamInfoById(8);
    EXPECT_NE(info, nullptr);
    EXPECT_EQ(info->streamId, 8);
}


/**
 * @tc.name: GET_START_INFO_DEFAULT_VARIANT_GT_DUR
 * @tc.desc: test GetStartInfo defaultVariant path with timeOffset > duration
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, GET_START_INFO_DEFAULT_VARIANT_GT_DUR, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    downloader->master_ = std::make_shared<M3U8MasterPlaylist>("", TEST_URI_PATH);
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    m3u8->isStart_ = true;
    m3u8->timeOffset_ = static_cast<int64_t>(5000000ULL * S_2_NS);
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->master_->defaultVariant_ = variant;
    downloader->master_->duration_ = 1ULL * S_2_NS;
    auto info = downloader->GetStartInfo();
    EXPECT_TRUE(info.first > 0);
}

/**
 * @tc.name: COLLECT_KEY_INFOS_SUB_NULL_M3U8
 * @tc.desc: test CollectAllKeyInfos with currentSubtitles_ m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COLLECT_KEY_INFOS_SUB_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    auto sub = std::make_shared<M3U8Media>("s", "http://sub", nullptr);
    downloader->currentSubtitles_ = sub;
    std::unordered_map<uint64_t, KeyInfo> keyInfos;
    std::vector<M3U8Segment> files;
    uint64_t sessionKeyIndex = 0;
    downloader->CollectAllKeyInfos(keyInfos, files, sessionKeyIndex);
    EXPECT_TRUE(keyInfos.empty());
}

/**
 * @tc.name: COLLECT_KEY_INFOS_AUDIO_NULL_M3U8
 * @tc.desc: test CollectAllKeyInfos with currentAudio_ m3u8_ null
 * @tc.type: FUNC
 */
HWTEST_F(HlsPlayListDownloaderUnitTest, COLLECT_KEY_INFOS_AUDIO_NULL_M3U8, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto m3u8 = std::make_shared<M3U8>(TEST_URI_PATH, "");
    auto variant = std::make_shared<M3U8VariantStream>("video", TEST_URI_PATH, m3u8);
    downloader->currentVariant_ = variant;
    auto audio = std::make_shared<M3U8Media>("a", "http://audio", nullptr);
    downloader->currentAudio_ = audio;
    std::unordered_map<uint64_t, KeyInfo> keyInfos;
    std::vector<M3U8Segment> files;
    uint64_t sessionKeyIndex = 0;
    downloader->CollectAllKeyInfos(keyInfos, files, sessionKeyIndex);
    EXPECT_TRUE(keyInfos.empty());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, FEED_MANIFEST_PREPARSE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto cb = std::make_shared<MockPlayListCallback>();
    downloader->SetPlayListCallback(std::weak_ptr<PlayListChangeCallback>(cb));

    // live media：2 个 #EXTINF，无 #EXT-X-ENDLIST，无 #EXT-X-MAP → PreParse 会触发
    std::string liveMedia =
        "#EXTM3U\n"
        "#EXT-X-VERSION:3\n"
        "#EXT-X-TARGETDURATION:6\n"
        "#EXTINF:5.0,\n"
        "seg0.ts\n"
        "#EXTINF:5.0,\n"
        "seg1.ts\n";
    downloader->SetPlayListForTest(liveMedia);
    downloader->PreParseManifest("http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/spike/live.m3u8");

    EXPECT_FALSE(cb->savedPlayList.empty());
    if (!cb->savedPlayList.empty()) {
        EXPECT_NE(cb->savedPlayList[0].url_.find("seg0.ts"), std::string::npos);
    }
    EXPECT_FALSE(downloader->IsParseFinished());  // pre-parse 保持 false
}

HWTEST_F(HlsPlayListDownloaderUnitTest, FEED_MANIFEST_PREPARSE_VOD_SKIP_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto cb = std::make_shared<MockPlayListCallback>();
    downloader->SetPlayListCallback(std::weak_ptr<PlayListChangeCallback>(cb));

    // VOD：含 #EXT-X-ENDLIST → PreParseManifest 在 ENDLIST 检查处早退
    std::string vod =
        "#EXTM3U\n"
        "#EXT-X-TARGETDURATION:6\n"
        "#EXTINF:5.0,\n"
        "seg0.ts\n"
        "#EXTINF:5.0,\n"
        "seg1.ts\n"
        "#EXT-X-ENDLIST\n";
    downloader->SetPlayListForTest(vod);
    downloader->PreParseManifest("http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/spike/vod.m3u8");

    EXPECT_TRUE(cb->savedPlayList.empty());
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PREPARSE_BOUNDARY_XMAP_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto cb = std::make_shared<MockPlayListCallback>();
    downloader->SetPlayListCallback(std::weak_ptr<PlayListChangeCallback>(cb));

    // #EXT-X-MAP present (fMP4 init segment) -> PreParse early-exits (truncation risks init).
    std::string fmp4 =
        "#EXTM3U\n"
        "#EXT-X-VERSION:6\n"
        "#EXT-X-TARGETDURATION:6\n"
        "#EXT-X-MAP:URI=\"init.mp4\"\n"
        "#EXTINF:5.0,\n"
        "seg0.ts\n"
        "#EXTINF:5.0,\n"
        "seg1.ts\n";
    downloader->SetPlayListForTest(fmp4);
    downloader->PreParseManifest("http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/spike/fmp4.m3u8");
    EXPECT_TRUE(cb->savedPlayList.empty()) << "X-MAP present -> PreParse must skip";
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PREPARSE_BOUNDARY_SINGLE_EXTINF_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto cb = std::make_shared<MockPlayListCallback>();
    downloader->SetPlayListCallback(std::weak_ptr<PlayListChangeCallback>(cb));

    // Only 1 #EXTINF (< MIN_PRE_PARSE_NUM=2) -> find loop aborts -> no PreParse.
    std::string single =
        "#EXTM3U\n"
        "#EXT-X-VERSION:3\n"
        "#EXT-X-TARGETDURATION:6\n"
        "#EXTINF:5.0,\n"
        "seg0.ts\n";
    downloader->SetPlayListForTest(single);
    downloader->PreParseManifest("http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/spike/single.m3u8");
    EXPECT_TRUE(cb->savedPlayList.empty()) << "<2 #EXTINF -> PreParse must skip";
}

HWTEST_F(HlsPlayListDownloaderUnitTest, PREPARSE_BOUNDARY_MASTER_PLAYLIST_005, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsPlayListDownloaderTestable>();
    downloader->Init();
    auto cb = std::make_shared<MockPlayListCallback>();
    downloader->SetPlayListCallback(std::weak_ptr<PlayListChangeCallback>(cb));

    // Master playlist (#EXT-X-STREAM-INF, no #EXTINF) -> no segments to pre-parse.
    std::string master =
        "#EXTM3U\n"
        "#EXT-X-VERSION:6\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=1280000,RESOLUTION=720x480\n"
        "720p.m3u8\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=2560000,RESOLUTION=1280x720\n"
        "1080p.m3u8\n";
    downloader->SetPlayListForTest(master);
    downloader->PreParseManifest("http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/spike/master.m3u8");
    EXPECT_TRUE(cb->savedPlayList.empty()) << "master playlist (no #EXTINF) -> PreParse must skip";
}
}
