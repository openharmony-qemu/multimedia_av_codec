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
#include "hls_media_downloader_unit_test.h"
#include "http_server_demo.h"
#include "source_callback.h"

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace std;
using namespace testing::ext;

namespace {
constexpr uint32_t CHUNK_SIZE_UT = 16 * 1024;
constexpr uint64_t MAX_CACHE_BUFFER_SIZE_UT = 19 * 1024 * 1024;
constexpr uint32_t DELAY_AFTER_OPEN = 1 * 1000;
constexpr uint32_t TEST_APP_UID = 100;
const std::map<std::string, std::string> httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"},
};
const std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
int g_eventStatus = 0;
const std::map<PluginEventType, int> POST_ALL_EVENT_STATUS = {
    {PluginEventType::CLIENT_ERROR, 1},
    {PluginEventType::INITIAL_BUFFER_SUCCESS, 2},
    {PluginEventType::SOURCE_DRM_INFO_UPDATE, 3},
    {PluginEventType::VIDEO_SIZE_CHANGE, 4},
    {PluginEventType::SOURCE_BITRATE_START, 5},
    {PluginEventType::CACHED_DURATION, 6},
    {PluginEventType::EVENT_BUFFER_PROGRESS, 7},
    {PluginEventType::HLS_SEEK_READY, 8},
};

const std::map<std::string, int32_t> AUDIO_START_STREAM_ID = {
    {"test_detach_hls/audio_default_auto.m3u8", 2},
    {"test_detach_hls/audio_default.m3u8", 3},
    {"test_detach_hls/audio.m3u8", 4},
    {"test_detach_hls/audio_auto.m3u8", 3},

    {"test_detach_hls/invalidaudio_default_auto.m3u8", 4},
    {"test_detach_hls/invalidaudio_default.m3u8", 4},
    {"test_detach_hls/invalidaudio.m3u8", 4},
    {"test_detach_hls/invalidaudio_auto.m3u8", 4},

    {"test_detach_hls/noaudio_default_auto.m3u8", 2},
    {"test_detach_hls/noaudio_default.m3u8", 3},
    {"test_detach_hls/noaudio.m3u8", 4},
    {"test_detach_hls/noaudio_auto.m3u8", 3},
};

class EventCallback : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event)
    {
        const auto &status = POST_ALL_EVENT_STATUS.at(event.type);
        g_eventStatus = status;
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
};

std::shared_ptr<HlsMediaDownloader> OpenHlsDetachAudioVideo(std::string url = "test_detach_hls/cmaf.m3u8")
{
    auto header = std::map<std::string, std::string>();
    auto downloader = std::make_shared<HlsMediaDownloader>(20, true, header);
    downloader->Init();

    // set callback to media downloader
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);

    std::string testUrl = TEST_URI_PATH + url;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(DELAY_AFTER_OPEN);
    downloader->SetAppUid(TEST_APP_UID);
    
    return downloader;
}

void CloseHlsDetachAudioVideo(std::shared_ptr<HlsMediaDownloader> downloader)
{
    auto sourceCallback = downloader->callback_;
    downloader = nullptr;
    delete sourceCallback;
    sourceCallback = nullptr;
}
}

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;

void HlsMediaDownloaderTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
}

void HlsMediaDownloaderTest::TearDownTestCase(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

void HlsMediaDownloaderTest ::SetUp(void)
{
    header_ = std::map<std::string, std::string>();
    hlsMediaDownloader_ = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    hlsMediaDownloader_->Init();
}

void HlsMediaDownloaderTest ::TearDown(void)
{
    hlsMediaDownloader_ = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_PLAY_INFO_001, TestSize.Level0)
{
    hlsMediaDownloader_->videoSegManager_->isBuffering_ = true;
    EXPECT_FALSE(hlsMediaDownloader_->GetPlayable());

    hlsMediaDownloader_->videoSegManager_ = nullptr;
    EXPECT_FALSE(hlsMediaDownloader_->GetPlayable());
}

HWTEST_F(HlsMediaDownloaderTest, GET_READ_TIMEOUT_001, TestSize.Level0)
{
    hlsMediaDownloader_->videoSegManager_->steadyClock_.Reset();
    EXPECT_FALSE(hlsMediaDownloader_->GetReadTimeOut(true));

    hlsMediaDownloader_->videoSegManager_ = nullptr;
    EXPECT_TRUE(hlsMediaDownloader_->GetReadTimeOut(false));
}

HWTEST_F(HlsMediaDownloaderTest, GET_DOWNLOAD_INFO_001, TestSize.Level1)
{
    hlsMediaDownloader_->videoSegManager_->recordSpeedCount_ = 0;
    DownloadInfo downloadInfo;
    hlsMediaDownloader_->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.avgDownloadRate, 0);

    hlsMediaDownloader_->videoSegManager_ = nullptr;
    hlsMediaDownloader_->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.avgDownloadRate, 0);
}

HWTEST_F(HlsMediaDownloaderTest, GET_DOWNLOAD_INFO_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);

    auto info = downloader->GetDownloadInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_EQ(info.second, 0);

    downloader->videoSegManager_ = nullptr;
    info = downloader->GetDownloadInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_EQ(info.second, 0);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_BUFFER_SIZE_001, TestSize.Level1)
{
    uint64_t actualSize = hlsMediaDownloader_->GetBufferSize();
    EXPECT_EQ(actualSize, 0);

    hlsMediaDownloader_->videoSegManager_ = nullptr;
    actualSize = hlsMediaDownloader_->GetBufferSize();
    EXPECT_EQ(actualSize, 0);
}

HWTEST_F(HlsMediaDownloaderTest, SET_DOWNLOAD_ERROR_STATE_001, TestSize.Level1)
{
    hlsMediaDownloader_->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    hlsMediaDownloader_->audioSegManager_->Init();
    hlsMediaDownloader_->SetDownloadErrorState();
    EXPECT_TRUE(hlsMediaDownloader_->videoSegManager_->downloadErrorState_);
    EXPECT_TRUE(hlsMediaDownloader_->audioSegManager_->downloadErrorState_);

    hlsMediaDownloader_->audioSegManager_ = nullptr;
    hlsMediaDownloader_->SetDownloadErrorState();
    EXPECT_TRUE(hlsMediaDownloader_->videoSegManager_->downloadErrorState_);

    hlsMediaDownloader_->videoSegManager_ = nullptr;
    hlsMediaDownloader_->SetDownloadErrorState();
}

HWTEST_F(HlsMediaDownloaderTest, SET_DEMUXER_STATE_001, TestSize.Level1)
{
    hlsMediaDownloader_->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    hlsMediaDownloader_->audioSegManager_->Init();
    hlsMediaDownloader_->SetDemuxerState(0);
    EXPECT_TRUE(hlsMediaDownloader_->videoSegManager_->isReadFrame_);
    EXPECT_TRUE(hlsMediaDownloader_->videoSegManager_->isFirstFrameArrived_);
    EXPECT_TRUE(hlsMediaDownloader_->audioSegManager_->isReadFrame_);
    EXPECT_TRUE(hlsMediaDownloader_->audioSegManager_->isFirstFrameArrived_);
    
    hlsMediaDownloader_->audioSegManager_ = nullptr;
    hlsMediaDownloader_->SetDemuxerState(0);
    EXPECT_TRUE(hlsMediaDownloader_->videoSegManager_->isReadFrame_);
    EXPECT_TRUE(hlsMediaDownloader_->videoSegManager_->isFirstFrameArrived_);
    
    hlsMediaDownloader_->videoSegManager_ = nullptr;
    hlsMediaDownloader_->SetDemuxerState(0);
}

HWTEST_F(HlsMediaDownloaderTest, TEST_OPEN_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>("", header_);
    downloader->Init();
    downloader->SetAppUid(TEST_APP_UID);
    EXPECT_TRUE(downloader->videoSegManager_);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, TEST_PAUSE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->audioSegManager_->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    EXPECT_TRUE(downloader->videoSegManager_);
    downloader->Pause();
    downloader->Resume();
    downloader->Pause();
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, TEST_PAUSE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    EXPECT_FALSE(downloader->audioSegManager_);
    downloader->Pause();
    downloader->Resume();
    downloader->Pause();
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, TEST_READ_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->audioSegManager_->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(DELAY_AFTER_OPEN);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsMediaDownloaderTest, TEST_READ_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(DELAY_AFTER_OPEN);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsMediaDownloaderTest, TEST_READ_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->bufferingFlag_ = 1;
    auto ret = downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(DELAY_AFTER_OPEN);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_EQ(ret, Status::ERROR_AGAIN);
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsMediaDownloaderTest, TEST_READ_004, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideo();
    downloader->GetSeekable();
    int len = 16 * 1024;
    auto *buff = new unsigned char[len];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = len;
    readDataInfo.isEos_ = false;
    auto ret = downloader->Read(buff, readDataInfo);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_GT(readDataInfo.realReadLength_, 0);
    EXPECT_LE(readDataInfo.realReadLength_, len);
    
    readDataInfo.streamId_ = 3;
    readDataInfo.wantReadLength_ = len;
    readDataInfo.isEos_ = false;
    ret = downloader->Read(buff, readDataInfo);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_GT(readDataInfo.realReadLength_, 0);
    EXPECT_LE(readDataInfo.realReadLength_, len);

    OSAL::SleepFor(DELAY_AFTER_OPEN);
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, TEST_CALLBACK_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->audioSegManager_->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    EXPECT_GE(readDataInfo.realReadLength_, 0);
    OSAL::SleepFor(DELAY_AFTER_OPEN);

    downloader->SetCurrentBitRate(-1, 0);
    downloader->SetCurrentBitRate(10, 0);
    downloader->SetInterruptState(true);
    downloader->SetInterruptState(false);
    downloader->GetContentLength();
    downloader->GetDuration();
    downloader->GetBitRates();
    downloader->SetReadBlockingFlag(true);
    downloader->Close(true);
    downloader = nullptr;
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, TEST_CALLBACK_002, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->audioSegManager_->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    OSAL::SleepFor(DELAY_AFTER_OPEN);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, TEST_CALLBACK_003, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    EXPECT_GE(readDataInfo.realReadLength_, 0);
    OSAL::SleepFor(DELAY_AFTER_OPEN);

    downloader->SetCurrentBitRate(-1, 0);
    downloader->SetCurrentBitRate(10, 0);
    downloader->SetInterruptState(true);
    downloader->SetInterruptState(false);
    downloader->GetContentLength();
    downloader->GetDuration();
    downloader->GetBitRates();
    downloader->SetReadBlockingFlag(true);
    downloader->Close(true);
    downloader = nullptr;
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, TEST_CALLBACK_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    OSAL::SleepFor(DELAY_AFTER_OPEN);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_SEGMENT_OFFSET_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    auto ret = downloader->GetSegmentOffset();
    EXPECT_EQ(ret, 0);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_SEGMENT_OFFSET_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    
    downloader->videoSegManager_ = nullptr;
    auto ret = downloader->GetSegmentOffset();
    EXPECT_EQ(ret, 0);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_HLS_DISCONTINUITY_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    auto ret = downloader->GetHLSDiscontinuity();
    EXPECT_FALSE(ret);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_HLS_DISCONTINUITY_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    auto ret = downloader->GetHLSDiscontinuity();
    EXPECT_FALSE(ret);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_HLS_DISCONTINUITY_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    auto playlistDownloader = downloader->videoSegManager_->playlistDownloader_;
    auto hlsPlaylistDownloader = static_pointer_cast<HlsPlayListDownloader>(playlistDownloader);
    hlsPlaylistDownloader->master_ =
        std::make_shared<M3U8MasterPlaylist>("playList_", "url_", 0, header_, statusCallback);
    hlsPlaylistDownloader->master_->hasDiscontinuity_ = false;
    auto ret = downloader->GetHLSDiscontinuity();
    EXPECT_FALSE(ret);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, STOP_BUFFERING_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->videoSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);
    downloader->audioSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->audioSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    auto ret = downloader->StopBufferring(true);
    EXPECT_EQ(ret, Status::OK);
    
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, STOP_BUFFERING_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);

    downloader->videoSegManager_->cacheMediaBuffer_ = nullptr;
    auto ret = downloader->StopBufferring(false);
    EXPECT_NE(ret, Status::OK);
    
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, STOP_BUFFERING_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);

    downloader->videoSegManager_ = nullptr;
    auto ret = downloader->StopBufferring(true);
    EXPECT_NE(ret, Status::OK);
    
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, STOP_BUFFERING_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->videoSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);

    auto ret = downloader->StopBufferring(true);
    EXPECT_EQ(ret, Status::OK);
    
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, TEST_AUTO_SELECT_BITRATE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->AutoSelectBitrate(1000000);
    
    downloader->videoSegManager_ = nullptr;
    downloader->AutoSelectBitrate(2000000);

    EXPECT_FALSE(downloader->audioSegManager_);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_PLAYBACK_INFO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    PlaybackInfo playbackInfo;
    downloader->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.serverIpAddress, "");
    EXPECT_EQ(playbackInfo.averageDownloadRate, 0);
    EXPECT_EQ(playbackInfo.isDownloading, true);
    EXPECT_EQ(playbackInfo.downloadRate, 0);
    EXPECT_EQ(playbackInfo.bufferDuration, 0);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_PLAYBACK_INFO_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    
    downloader->videoSegManager_ = nullptr;
    PlaybackInfo playbackInfo {};
    downloader->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.serverIpAddress, "");
    EXPECT_EQ(playbackInfo.averageDownloadRate, 0);
    EXPECT_EQ(playbackInfo.isDownloading, false);
    EXPECT_EQ(playbackInfo.downloadRate, 0);
    EXPECT_EQ(playbackInfo.bufferDuration, 0);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SET_INITIAL_BUFFERSIZE_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->callback_ = sourceCallback;
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    PlayInfo playInfo;
    playInfo.url_ = testUrl;
    downloader->videoSegManager_->PutRequestIntoDownloader(playInfo);
    downloader->videoSegManager_->backPlayList_.push_back(playInfo);
    downloader->videoSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->videoSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    EXPECT_FALSE(downloader->SetInitialBufferSize(0, 20000000));
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SET_INITIAL_BUFFERSIZE_002, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->callback_ = sourceCallback;
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    PlayInfo playInfo;
    playInfo.url_ = testUrl;
    downloader->videoSegManager_->PutRequestIntoDownloader(playInfo);
    downloader->videoSegManager_->backPlayList_.push_back(playInfo);
    downloader->videoSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->videoSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    EXPECT_FALSE(downloader->SetInitialBufferSize(0, 50000));
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SET_INITIAL_BUFFERSIZE_003, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_->downloader_ = std::make_shared<Downloader>("hlsMedia");
    auto saveData = [] (uint8_t* data, uint32_t len, bool flag) { return 0; };
    auto statusCallback = [] (DownloadStatus status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    RequestInfo info {};
    downloader->videoSegManager_->downloadRequest_ = std::make_shared<DownloadRequest>(100,
        saveData, statusCallback, info, false);
    EXPECT_TRUE(downloader->SetInitialBufferSize(0, 50000));
}

HWTEST_F(HlsMediaDownloaderTest, SET_INITIAL_BUFFERSIZE_004, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);
    downloader->videoSegManager_->downloader_ = std::make_shared<Downloader>("hlsMedia");
    auto saveData = [] (uint8_t* data, uint32_t len, bool flag) { return 0; };
    auto statusCallback = [] (DownloadStatus status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    RequestInfo info {};
    downloader->videoSegManager_->downloadRequest_ = std::make_shared<DownloadRequest>(100.0,
        saveData, statusCallback, info, false);
    EXPECT_FALSE(downloader->SetInitialBufferSize(0, 50000));
}

HWTEST_F(HlsMediaDownloaderTest, SET_INITIAL_BUFFERSIZE_005, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    EXPECT_FALSE(downloader->SetInitialBufferSize(0, 50000));
}

HWTEST_F(HlsMediaDownloaderTest, SET_PLAY_STRATEGY_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1280;
    playStrategy->height = 720;
    playStrategy->bufferDurationForPlaying = 5;
    downloader->SetPlayStrategy(playStrategy);
    EXPECT_EQ(downloader->videoSegManager_->bufferDurationForPlaying_, 5);
}

HWTEST_F(HlsMediaDownloaderTest, SET_PLAY_STRATEGY_002, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1280;
    playStrategy->height = 720;
    playStrategy->bufferDurationForPlaying = 5;
    downloader->SetPlayStrategy(playStrategy);
    EXPECT_EQ(downloader->videoSegManager_->bufferDurationForPlaying_, 5);
}

HWTEST_F(HlsMediaDownloaderTest, SET_PLAY_STRATEGY_003, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1280;
    playStrategy->height = 720;
    playStrategy->bufferDurationForPlaying = 5;
    downloader->SetPlayStrategy(playStrategy);
    EXPECT_FALSE(downloader->audioSegManager_);
}

HWTEST_F(HlsMediaDownloaderTest, NOTIFY_INIT_SUCCESS_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);
    downloader->NotifyInitSuccess();
    EXPECT_TRUE(downloader->videoSegManager_->isDemuxerInitSuccess_);
}

HWTEST_F(HlsMediaDownloaderTest, NOTIFY_INIT_SUCCESS_002, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->NotifyInitSuccess();
    EXPECT_TRUE(downloader->videoSegManager_->isDemuxerInitSuccess_);
}

HWTEST_F(HlsMediaDownloaderTest, NOTIFY_INIT_SUCCESS_003, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    downloader->NotifyInitSuccess();
    EXPECT_FALSE(downloader->audioSegManager_);
}

HWTEST_F(HlsMediaDownloaderTest, GET_STREAM_INFO_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->callback_ = sourceCallback;
    std::vector<StreamInfo> streams;
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::OK);

    downloader->videoSegManager_ = nullptr;
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::ERROR_UNKNOWN);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, IS_HLS_FMP4_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    EXPECT_EQ(downloader->IsHlsFmp4(), false);
    
    downloader->videoSegManager_ = nullptr;
    EXPECT_EQ(downloader->IsHlsFmp4(), false);
}

HWTEST_F(HlsMediaDownloaderTest, SEEK_TO_TIME_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->videoSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->videoSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);
    downloader->audioSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->audioSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->callback_ = sourceCallback;
    auto ret = downloader->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_TRUE(ret);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SEEK_TO_TIME_002, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->videoSegManager_->cacheMediaBuffer_ = nullptr;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->callback_ = sourceCallback;
    auto ret = downloader->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_FALSE(ret);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SEEK_TO_TIME_003, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->callback_ = sourceCallback;
    auto ret = downloader->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_FALSE(ret);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, ON_MASTER_READY_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->OnMasterReady(false, false);
    EXPECT_FALSE(downloader->audioSegManager_);
    
    downloader->videoSegManager_ = nullptr;
    downloader->OnMasterReady(false, false);
    EXPECT_FALSE(downloader->audioSegManager_);
}

HWTEST_F(HlsMediaDownloaderTest, ON_MASTER_READY_002, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->OnMasterReady(true, true);
    EXPECT_TRUE(downloader->audioSegManager_);
    
    downloader->OnMasterReady(true, true);
    EXPECT_TRUE(downloader->audioSegManager_);
}

HWTEST_F(HlsMediaDownloaderTest, CACHE_BUFFER_FULL_LOOP_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideo();
    downloader->GetSeekable();
    PlayInfo playInfo;
    downloader->videoSegManager_->backPlayList_.push_back(playInfo);
    downloader->videoSegManager_->initCacheSize_ = 100;
    downloader->videoSegManager_->isSeekingFlag = true;
    downloader->videoSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->videoSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    EXPECT_EQ(downloader->videoSegManager_->CacheBufferFullLoop(), true);
    EXPECT_EQ(downloader->videoSegManager_->initCacheSize_, -1);
    downloader->videoSegManager_->isSeekingFlag = false;
    EXPECT_EQ(downloader->videoSegManager_->CacheBufferFullLoop(), false);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, POST_BUFFERING_EVENT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);
    EXPECT_EQ(downloader->bufferingFlag_, 0);
    downloader->PostBufferingEvent(HlsSegmentType::SEG_VIDEO, BufferingInfoType::BUFFERING_START);
    EXPECT_EQ(downloader->bufferingFlag_, 2);
    downloader->PostBufferingEvent(HlsSegmentType::SEG_VIDEO, BufferingInfoType::BUFFERING_END);
    EXPECT_EQ(downloader->bufferingFlag_, 0);
    downloader->PostBufferingEvent(HlsSegmentType::SEG_AUDIO, BufferingInfoType::BUFFERING_START);
    EXPECT_EQ(downloader->bufferingFlag_, 1);
    downloader->PostBufferingEvent(HlsSegmentType::SEG_VIDEO, BufferingInfoType::BUFFERING_START);
    EXPECT_EQ(downloader->bufferingFlag_, 3);
    downloader->PostBufferingEvent(HlsSegmentType::SEG_AUDIO, BufferingInfoType::BUFFERING_END);
    downloader->PostBufferingEvent(HlsSegmentType::SEG_VIDEO, BufferingInfoType::BUFFERING_END);
    EXPECT_EQ(downloader->bufferingFlag_, 0);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_CONTENT_TYPE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);
    auto type = downloader->GetContentType();
    EXPECT_EQ(type, "");

    downloader->videoSegManager_ = nullptr;
    auto typeNullptr = downloader->GetContentType();
    EXPECT_EQ(typeNullptr, "");
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_STARTED_STATUS_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideo();
    auto ret = downloader->GetStartedStatus();
    EXPECT_TRUE(ret);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, GET_STARTED_STATUS_002, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideo("test_detach_hls/cmaf/avc/540p_2000/avc_540p_2000.m3u8");
    auto ret = downloader->GetStartedStatus();
    EXPECT_TRUE(ret);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, GET_STARTED_STATUS_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);
    auto ret = downloader->GetStartedStatus();
    EXPECT_FALSE(ret);

    downloader->videoSegManager_ = nullptr;
    ret = downloader->GetStartedStatus();
    EXPECT_FALSE(ret);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SELECT_BITRATE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);
    auto ret = downloader->SelectBitRate(100000);
    EXPECT_EQ(ret, true);

    downloader->videoSegManager_ = nullptr;
    ret = downloader->SelectBitRate(100000);
    EXPECT_EQ(ret, false);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SET_IS_TRIGGER_AUTO_MODE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->SetCallback(sourceCallback);

    EXPECT_TRUE(downloader->videoSegManager_->isAutoSelectBitrate_);
    downloader->SetIsTriggerAutoMode(false);
    EXPECT_FALSE(downloader->videoSegManager_->isAutoSelectBitrate_);
    downloader->SetIsTriggerAutoMode(true);
    EXPECT_TRUE(downloader->videoSegManager_->isAutoSelectBitrate_);

    downloader->videoSegManager_ = nullptr;
    downloader->SetIsTriggerAutoMode(false);
    downloader->SetAppUid(TEST_APP_UID);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_BUFFERING_TIMEOUT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    
    auto ret = downloader->GetBufferingTimeOut();
    EXPECT_FALSE(ret);
    
    downloader->videoSegManager_ = nullptr;
    ret = downloader->GetBufferingTimeOut();
    EXPECT_TRUE(ret);

    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, WAIT_FOR_BUFFERING_END_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideo();
    
    downloader->WaitForBufferingEnd();
    EXPECT_FALSE(downloader->videoSegManager_->isBuffering_.load());
    
    downloader->SetInitialBufferSize(0, 20000000);
    EXPECT_FALSE(downloader->videoSegManager_->isBuffering_.load());
    
    downloader->videoSegManager_ = nullptr;
    downloader->WaitForBufferingEnd();

    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, SET_IS_REPORTED_ERROR_CODE_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideo();
    EXPECT_FALSE(downloader->videoSegManager_->isReportedErrorCode_);
    EXPECT_FALSE(downloader->audioSegManager_->isReportedErrorCode_);
    
    downloader->SetIsReportedErrorCode();
    EXPECT_TRUE(downloader->videoSegManager_->isReportedErrorCode_);
    EXPECT_TRUE(downloader->audioSegManager_->isReportedErrorCode_);
    
    downloader->videoSegManager_ = nullptr;
    downloader->SetIsReportedErrorCode();
    EXPECT_TRUE(downloader->audioSegManager_->isReportedErrorCode_);

    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, SET_IS_REPORTED_ERROR_CODE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();

    EXPECT_FALSE(downloader->videoSegManager_->isReportedErrorCode_);
    downloader->SetIsReportedErrorCode();
    EXPECT_TRUE(downloader->videoSegManager_->isReportedErrorCode_);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_CACHED_DURATION_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    
    auto duration = downloader->GetCachedDuration();
    EXPECT_EQ(duration, 0);
    
    downloader->videoSegManager_ = nullptr;
    duration = downloader->GetCachedDuration();
    EXPECT_EQ(duration, 0);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_MEMORY_SIZE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    
    auto memorySize = downloader->GetMemorySize();
    EXPECT_EQ(memorySize, 0);
    
    downloader->videoSegManager_ = nullptr;
    memorySize = downloader->GetMemorySize();
    EXPECT_EQ(memorySize, 0);
    
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, IS_HLS_END_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    
    auto isHlsEnd = downloader->IsHlsEnd();
    EXPECT_FALSE(isHlsEnd);
    
    downloader->videoSegManager_ = nullptr;
    isHlsEnd = downloader->IsHlsEnd();
    EXPECT_FALSE(isHlsEnd);
    
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_SEGMENT_MANAGER_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideo();
    
    auto manager = downloader->GetSegmentManager(1);
    EXPECT_TRUE(manager == downloader->videoSegManager_);
    manager = downloader->GetSegmentManager(2);
    EXPECT_TRUE(manager == downloader->videoSegManager_);
    manager = downloader->GetSegmentManager(3);
    EXPECT_TRUE(manager == downloader->audioSegManager_);
    
    downloader->videoSegManager_ = nullptr;
    manager = downloader->GetSegmentManager(1);
    EXPECT_TRUE(manager == nullptr);

    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, SELECT_STREAM_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideo();
    
    auto ret = downloader->SelectStream(1);
    EXPECT_EQ(ret, Status::OK);
    ret = downloader->SelectStream(2);
    EXPECT_EQ(ret, Status::OK);
    ret = downloader->SelectStream(3);
    EXPECT_EQ(ret, Status::OK);
    ret = downloader->SelectStream(100);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
    
    downloader->videoSegManager_ = nullptr;
    ret = downloader->SelectStream(1);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, POST_ALL_EVENT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    Plugins::Callback* eventCallback = new EventCallback();
    downloader->SetCallback(eventCallback);

    g_eventStatus = 0;
    for (const auto &[k, v]: POST_ALL_EVENT_STATUS) {
        HlsSegEvent event {};
        event.type = k;
        downloader->PostAllEvent(event);
        EXPECT_EQ(g_eventStatus, v);
    }
    
    g_eventStatus = 0;
    downloader->SetCallback(nullptr);
    HlsSegEvent event {};
    downloader->PostAllEvent(event);
    EXPECT_EQ(g_eventStatus, 0);

    downloader = nullptr;
    delete eventCallback;
    eventCallback = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, INIT_SOURCE_LOADER_001, TestSize.Level1)
{
    auto sourceLoader = std::make_shared<MediaSourceLoaderCombinations>(nullptr);
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_, sourceLoader);
    downloader->Init();
    
    EXPECT_TRUE(downloader->videoSegManager_->sourceLoader_ != nullptr);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, TEST_AUDIO_START_STREAM_ID, TestSize.Level1)
{
    for (const auto &[k, v]: AUDIO_START_STREAM_ID) {
        std::cout << "TEST_AUDIO_START_STREAM_ID start: " << k << ", start audio id: " << v << std::endl;
        auto downloader = OpenHlsDetachAudioVideo(k);
        ASSERT_TRUE(downloader != nullptr);
        ASSERT_TRUE(downloader->audioSegManager_ != nullptr);
        ASSERT_TRUE(downloader->audioSegManager_->playlistDownloader_ != nullptr);
        auto playlistDownloader =
            std::static_pointer_cast<HlsPlayListDownloader>(downloader->audioSegManager_->playlistDownloader_);
        ASSERT_TRUE(playlistDownloader->currentAudio_ != nullptr);
        auto audioStreamId = playlistDownloader->currentAudio_->streamId_;
        EXPECT_EQ(audioStreamId, v);
        downloader->Close(true);
        CloseHlsDetachAudioVideo(downloader);
        std::cout << "TEST_AUDIO_START_STREAM_ID end: " << k << ", start audio id: " << v << std::endl;
    }
}
}