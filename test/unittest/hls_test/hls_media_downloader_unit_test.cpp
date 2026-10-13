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
const std::string M3U8_PATH_1 = "test_hls/testHLSEncode.m3u8";
const std::string M3U8_PATH_2 = "test_hls/testHLSEncode_session_key.m3u8";
const std::string M3U8_PATH_3 = "test_hls/testHLSEncode_session_key_01.m3u8";
const std::string M3U8_PATH_4 = "test_hls/testHLSEncode_01.m3u8";
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
    {PluginEventType::SOURCE_TIMED_METADATA, 9},
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
class HlsPlayListDownloaderMockForDefaultStream : public HlsPlayListDownloader {
public:
    using HlsPlayListDownloader::HlsPlayListDownloader;

    void SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId) override
    {
        called_ = true;
        videoStreamId_ = videoStreamId;
        audioStreamId_ = audioStreamId;
        subTitleStreamId_ = subTitleStreamId;
    }

    bool called_ {false};
    int32_t videoStreamId_ {0};
    int32_t audioStreamId_ {0};
    int32_t subTitleStreamId_ {0};
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
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);

    std::string testUrl = TEST_URI_PATH + url;
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(DELAY_AFTER_OPEN);
    downloader->SetAppUid(TEST_APP_UID);
    
    return downloader;
}

std::shared_ptr<HlsMediaDownloader> OpenHlsDetachAudioVideoWithMasterReady(
    std::string url = "test_detach_hls/cmaf.m3u8")
{
    auto header = std::map<std::string, std::string>();
    auto downloader = std::make_shared<HlsMediaDownloader>(20, true, header);
    downloader->Init();

    // set callback to media downloader
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);

    std::string testUrl = TEST_URI_PATH + url;
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    OSAL::SleepFor(DELAY_AFTER_OPEN);
    int32_t defaultStreamId = 1;
    downloader->SetDefaultStreamId(defaultStreamId, defaultStreamId, defaultStreamId);
    downloader->GetSeekable();
    if (downloader->audioSegManager_ != nullptr) {
        downloader->audioSegManager_->GetSeekable();
    }
    if (downloader->subtitlesSegManager_ != nullptr) {
        downloader->subtitlesSegManager_->GetSeekable();
    }
    downloader->SetAppUid(TEST_APP_UID);
    
    return downloader;
}

void CloseHlsDetachAudioVideo(std::shared_ptr<HlsMediaDownloader> downloader)
{
    downloader = nullptr;
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

HWTEST_F(HlsMediaDownloaderTest, GET_STREAM_INFO_002, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::vector<StreamInfo> streams;
    EXPECT_EQ(downloader->GetStreamInfo(streams, true), Status::OK);
    downloader->videoSegManager_ = nullptr;
    EXPECT_EQ(downloader->GetStreamInfo(streams, true), Status::ERROR_UNKNOWN);
}

HWTEST_F(HlsMediaDownloaderTest, SET_DEFAULT_STREAM_ID_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->videoSegManager_ = nullptr;
    int32_t videoStreamId = 1;
    int32_t audioStreamId = 2;
    int32_t subTitleStreamId = 3;
    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    downloader->videoSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->videoSegManager_->Init();
    auto mockPlaylist = std::make_shared<HlsPlayListDownloaderMockForDefaultStream>();
    downloader->videoSegManager_->playlistDownloader_ = mockPlaylist;
    int32_t videoStreamIdNumSec = 4;
    int32_t audioStreamIdNumSec= 5;
    int32_t subTitleStreamIdNumSec = 6;
    downloader->SetDefaultStreamId(videoStreamIdNumSec, audioStreamIdNumSec, subTitleStreamIdNumSec);
    EXPECT_TRUE(mockPlaylist->called_);
    EXPECT_EQ(mockPlaylist->videoStreamId_, 4);
    EXPECT_EQ(mockPlaylist->audioStreamId_, 5);
    EXPECT_EQ(mockPlaylist->subTitleStreamId_, 6);
}

HWTEST_F(HlsMediaDownloaderTest, GET_PLAY_INFO_001, TestSize.Level0)
{
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
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);

    auto info = downloader->GetDownloadInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_EQ(info.second, 0);

    downloader->videoSegManager_ = nullptr;
    info = downloader->GetDownloadInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_EQ(info.second, 0);
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

HWTEST_F(HlsMediaDownloaderTest, SET_DOWNLOAD_ERROR_STATE_002, TestSize.Level1)
{
    hlsMediaDownloader_->subtitlesSegManager_ =
        std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    hlsMediaDownloader_->subtitlesSegManager_->Init();
    hlsMediaDownloader_->SetDownloadErrorState();
    EXPECT_TRUE(hlsMediaDownloader_->videoSegManager_->downloadErrorState_);
    EXPECT_TRUE(hlsMediaDownloader_->subtitlesSegManager_->downloadErrorState_);

    hlsMediaDownloader_->subtitlesSegManager_ = nullptr;
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

HWTEST_F(HlsMediaDownloaderTest, SET_DEMUXER_STATE_002, TestSize.Level1)
{
    hlsMediaDownloader_->subtitlesSegManager_ =
        std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    hlsMediaDownloader_->subtitlesSegManager_->Init();
    hlsMediaDownloader_->SetDemuxerState(0);
    EXPECT_TRUE(hlsMediaDownloader_->videoSegManager_->isReadFrame_);
    EXPECT_TRUE(hlsMediaDownloader_->videoSegManager_->isFirstFrameArrived_);
    EXPECT_TRUE(hlsMediaDownloader_->subtitlesSegManager_->isReadFrame_);
    EXPECT_TRUE(hlsMediaDownloader_->subtitlesSegManager_->isFirstFrameArrived_);
    
    hlsMediaDownloader_->subtitlesSegManager_ = nullptr;
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

HWTEST_F(HlsMediaDownloaderTest, TEST_PAUSE_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->subtitlesSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->subtitlesSegManager_->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    EXPECT_TRUE(downloader->videoSegManager_);
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
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
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
    auto ret = downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(DELAY_AFTER_OPEN);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_EQ(ret, Status::ERROR_AGAIN);
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsMediaDownloaderTest, TEST_READ_004, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    downloader->GetSeekable();
    int len = 16 * 1024;
    auto *buff = new unsigned char[len];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = len;
    readDataInfo.isEos_ = false;
    auto ret = downloader->Read(buff, readDataInfo);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_LE(readDataInfo.realReadLength_, len);
    
    readDataInfo.streamId_ = 3;
    readDataInfo.wantReadLength_ = len;
    readDataInfo.isEos_ = false;
    ret = downloader->Read(buff, readDataInfo);
    EXPECT_NE(ret, Status::OK);
    EXPECT_LE(readDataInfo.realReadLength_, len);

    OSAL::SleepFor(DELAY_AFTER_OPEN);
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, TEST_READ_005, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->subtitlesSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->subtitlesSegManager_->Init();
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
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
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
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
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
    auto sourceCallback = std::make_shared<SourceCallback>();
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
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
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
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
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
    auto sourceCallback = std::make_shared<SourceCallback>();
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
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsMediaDownloaderTest, TEST_CALLBACK_005, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->subtitlesSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->subtitlesSegManager_->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
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
}

HWTEST_F(HlsMediaDownloaderTest, TEST_CALLBACK_006, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->subtitlesSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->subtitlesSegManager_->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
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
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
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

HWTEST_F(HlsMediaDownloaderTest, GET_HLS_DISCONTINUITY_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->subtitlesSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_SUBTITLE);
    downloader->subtitlesSegManager_->Init();
    downloader->subtitlesSegManager_->Clone(downloader->videoSegManager_);
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

HWTEST_F(HlsMediaDownloaderTest, STOP_BUFFERING_005, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->videoSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    downloader->subtitlesSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_SUBTITLE);
    downloader->subtitlesSegManager_->Init();
    downloader->subtitlesSegManager_->Clone(downloader->videoSegManager_);
    downloader->subtitlesSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->subtitlesSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    auto ret = downloader->StopBufferring(true);
    EXPECT_EQ(ret, Status::OK);
    
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, STOP_BUFFERING_006, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->subtitlesSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_SUBTITLE);
    downloader->subtitlesSegManager_->Init();
    downloader->subtitlesSegManager_->Clone(downloader->videoSegManager_);

    downloader->videoSegManager_->cacheMediaBuffer_ = nullptr;
    auto ret = downloader->StopBufferring(false);
    EXPECT_NE(ret, Status::OK);
    
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, STOP_BUFFERING_007, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->subtitlesSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_SUBTITLE);
    downloader->subtitlesSegManager_->Init();
    downloader->subtitlesSegManager_->Clone(downloader->videoSegManager_);

    downloader->videoSegManager_ = nullptr;
    auto ret = downloader->StopBufferring(true);
    EXPECT_NE(ret, Status::OK);
    
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
    EXPECT_FALSE(downloader->audioSegManager_->isReportedErrorCode_);
}

HWTEST_F(HlsMediaDownloaderTest, GET_STREAM_INFO_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::vector<StreamInfo> streams;
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::OK);

    downloader->videoSegManager_ = nullptr;
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::ERROR_UNKNOWN);
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
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    auto ret = downloader->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_TRUE(ret);
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
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    auto ret = downloader->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_FALSE(ret);
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
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    auto ret = downloader->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_FALSE(ret);
}

HWTEST_F(HlsMediaDownloaderTest, SEEK_TO_TIME_004, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->videoSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->videoSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    downloader->subtitlesSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_SUBTITLE);
    downloader->subtitlesSegManager_->Init();
    downloader->subtitlesSegManager_->Clone(downloader->videoSegManager_);
    downloader->subtitlesSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->subtitlesSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    auto ret = downloader->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_TRUE(ret);
}

HWTEST_F(HlsMediaDownloaderTest, SEEK_TO_TIME_BY_STREAMID_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    auto ret = downloader->MediaSeekTimeByStreamId(0, SeekMode::SEEK_PREVIOUS_SYNC, 3);
    EXPECT_TRUE(ret);
    ret = downloader->MediaSeekTimeByStreamId(0, SeekMode::SEEK_PREVIOUS_SYNC, 4);
    EXPECT_TRUE(ret);
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, SEEK_TO_TIME_BY_STREAMID_002, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideo();
    downloader->subtitlesSegManager_ = nullptr;
    auto ret = downloader->MediaSeekTimeByStreamId(0, SeekMode::SEEK_PREVIOUS_SYNC, 4);
    EXPECT_FALSE(ret);
    downloader->audioSegManager_ = nullptr;
    ret = downloader->MediaSeekTimeByStreamId(0, SeekMode::SEEK_PREVIOUS_SYNC, 3);
    EXPECT_FALSE(ret);
    downloader->videoSegManager_ = nullptr;
    ret = downloader->MediaSeekTimeByStreamId(0, SeekMode::SEEK_PREVIOUS_SYNC, 1);
    EXPECT_FALSE(ret);
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
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

HWTEST_F(HlsMediaDownloaderTest, ON_MASTER_READY_003, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->OnMasterReady(false, false);
    EXPECT_FALSE(downloader->subtitlesSegManager_);
    
    downloader->videoSegManager_ = nullptr;
    downloader->OnMasterReady(false, false);
    EXPECT_FALSE(downloader->subtitlesSegManager_);
}

HWTEST_F(HlsMediaDownloaderTest, ON_MASTER_READY_004, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->OnMasterReady(true, true);
    EXPECT_TRUE(downloader->subtitlesSegManager_);
    
    downloader->OnMasterReady(true, true);
    EXPECT_TRUE(downloader->subtitlesSegManager_);
}

HWTEST_F(HlsMediaDownloaderTest, UPDATE_STREAM_STATISTICS_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto reportInfo = std::make_shared<OHOS::MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->SetSourceStatisticsDfx(reportInfo);
    ASSERT_NE(downloader->reportInfo_, nullptr);

    downloader->OnMasterReady(true, true);
    EXPECT_GT(downloader->reportInfo_->videoStreamCnt_, 0u);
    EXPECT_GT(downloader->reportInfo_->audioStreamCnt_, 0u);
    EXPECT_GT(downloader->reportInfo_->subtitleCnt_, 0u);

    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, UPDATE_STREAM_STATISTICS_002, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto reportInfo = std::make_shared<OHOS::MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->SetSourceStatisticsDfx(reportInfo);

    downloader->OnMasterReady(false, false);
    EXPECT_EQ(downloader->reportInfo_->videoStreamCnt_, 0u);
    EXPECT_EQ(downloader->reportInfo_->audioStreamCnt_, 0u);
    EXPECT_EQ(downloader->reportInfo_->subtitleCnt_, 0u);

    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, UPDATE_STREAM_STATISTICS_003, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->OnMasterReady(true, true);
    EXPECT_NO_FATAL_FAILURE(downloader->OnMasterReady(true, true));

    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, UPDATE_STREAM_STATISTICS_004, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto reportInfo = std::make_shared<OHOS::MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->SetSourceStatisticsDfx(reportInfo);

    downloader->videoSegManager_ = nullptr;
    downloader->OnMasterReady(true, true);
    EXPECT_EQ(downloader->reportInfo_->videoStreamCnt_, 0u);
    EXPECT_EQ(downloader->reportInfo_->audioStreamCnt_, 0u);
    EXPECT_EQ(downloader->reportInfo_->subtitleCnt_, 0u);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, CACHE_BUFFER_FULL_LOOP_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideo();
    downloader->GetSeekable();
    PlayInfo playInfo;
    downloader->videoSegManager_->backPlayList_.push_back(playInfo);
    downloader->videoSegManager_->isSeekingFlag = true;
    downloader->videoSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->videoSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    EXPECT_EQ(downloader->videoSegManager_->CacheBufferFullLoop(), true);
    downloader->videoSegManager_->isSeekingFlag = false;
    EXPECT_EQ(downloader->videoSegManager_->CacheBufferFullLoop(), false);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, GET_CONTENT_TYPE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    auto type = downloader->GetContentType();
    EXPECT_EQ(type, "");

    downloader->videoSegManager_ = nullptr;
    auto typeNullptr = downloader->GetContentType();
    EXPECT_EQ(typeNullptr, "");
}

HWTEST_F(HlsMediaDownloaderTest, SELECT_BITRATE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    auto ret = downloader->SelectBitRate(100000);
    EXPECT_EQ(ret, true);

    downloader->videoSegManager_ = nullptr;
    ret = downloader->SelectBitRate(100000);
    EXPECT_EQ(ret, false);
}

HWTEST_F(HlsMediaDownloaderTest, SET_IS_TRIGGER_AUTO_MODE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);

    EXPECT_FALSE(downloader->videoSegManager_->isAutoSelectBitrate_);
    downloader->SetIsTriggerAutoMode(false);
    EXPECT_FALSE(downloader->videoSegManager_->isAutoSelectBitrate_);
    downloader->SetIsTriggerAutoMode(true);
    EXPECT_TRUE(downloader->videoSegManager_->isAutoSelectBitrate_);

    downloader->videoSegManager_ = nullptr;
    downloader->SetIsTriggerAutoMode(false);
    downloader->SetAppUid(TEST_APP_UID);
}

HWTEST_F(HlsMediaDownloaderTest, SET_IS_REPORTED_ERROR_CODE_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
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

HWTEST_F(HlsMediaDownloaderTest, SET_IS_REPORTED_ERROR_CODE_003, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    EXPECT_FALSE(downloader->videoSegManager_->isReportedErrorCode_);
    EXPECT_FALSE(downloader->subtitlesSegManager_->isReportedErrorCode_);
    
    downloader->SetIsReportedErrorCode();
    EXPECT_TRUE(downloader->videoSegManager_->isReportedErrorCode_);
    EXPECT_TRUE(downloader->subtitlesSegManager_->isReportedErrorCode_);
    
    downloader->videoSegManager_ = nullptr;
    downloader->SetIsReportedErrorCode();
    EXPECT_TRUE(downloader->subtitlesSegManager_->isReportedErrorCode_);

    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
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
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    
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

HWTEST_F(HlsMediaDownloaderTest, GET_SEGMENT_MANAGER_002, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    
    auto manager = downloader->GetSegmentManager(1);
    EXPECT_TRUE(manager == downloader->videoSegManager_);
    manager = downloader->GetSegmentManager(2);
    EXPECT_TRUE(manager == downloader->videoSegManager_);
    manager = downloader->GetSegmentManager(4);
    EXPECT_TRUE(manager == downloader->subtitlesSegManager_);
    
    downloader->videoSegManager_ = nullptr;
    manager = downloader->GetSegmentManager(1);
    EXPECT_TRUE(manager == nullptr);

    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, SELECT_STREAM_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    
    auto ret = downloader->SelectStream(1);
    EXPECT_EQ(ret, Status::OK);
    ret = downloader->SelectStream(2);
    EXPECT_EQ(ret, Status::OK);
    ret = downloader->SelectStream(3);
    EXPECT_EQ(ret, Status::OK);
    ret = downloader->SelectStream(4);
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
    auto eventCallback = std::make_shared<EventCallback>();
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
}

HWTEST_F(HlsMediaDownloaderTest, INIT_SOURCE_LOADER_001, TestSize.Level1)
{
    auto sourceLoader = std::make_shared<MediaSourceLoaderCombinations>(nullptr);
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_, sourceLoader);
    downloader->Init();
    
    EXPECT_TRUE(downloader->videoSegManager_->sourceLoader_ != nullptr);
    EXPECT_EQ(downloader->GetSegmentOffset(), 0);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, TEST_AUDIO_START_STREAM_ID, TestSize.Level1)
{
    for (const auto &[k, v]: AUDIO_START_STREAM_ID) {
        std::cout << "TEST_AUDIO_START_STREAM_ID start: " << k << ", start audio id: " << v << std::endl;
        auto downloader = OpenHlsDetachAudioVideoWithMasterReady(k);
        ASSERT_TRUE(downloader != nullptr);
        ASSERT_TRUE(downloader->videoSegManager_ != nullptr);
        ASSERT_TRUE(downloader->videoSegManager_->playlistDownloader_ != nullptr);
        downloader->videoSegManager_->playlistDownloader_->SetDefaultMedia(HlsSegmentType::SEG_AUDIO);
        auto playlistDownloader =
            std::static_pointer_cast<HlsPlayListDownloader>(downloader->videoSegManager_->playlistDownloader_);
        ASSERT_TRUE(playlistDownloader->currentAudio_ != nullptr);
        auto audioStreamId = playlistDownloader->currentAudio_->streamId_;
        EXPECT_EQ(audioStreamId, v);
        downloader->Close(true);
        CloseHlsDetachAudioVideo(downloader);
        std::cout << "TEST_AUDIO_START_STREAM_ID end: " << k << ", start audio id: " << v << std::endl;
    }
}

HWTEST_F(HlsMediaDownloaderTest, LIVE_ENDLIST_MASTER_PLAYLIST, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady("test_detach_hls/live_endlist.m3u8");

    EXPECT_EQ(downloader->GetSeekable(), Seekable::SEEKABLE);
    EXPECT_EQ(downloader->GetDuration(), 60060000000);

    CloseHlsDetachAudioVideo(std::move(downloader));
}

HWTEST_F(HlsMediaDownloaderTest, LIVE_ENDLIST_MEDIA_PLAYLIST, TestSize.Level1)
{
    std::map<std::string, int64_t> mediaPlaylistArr = {
        {"test_detach_hls/cmaf/avc/540p_2000/avc_540p_2000_live_endlist.m3u8", 60060000000},
        {"test_detach_hls/cmaf/aac/lc_192/aac_lc_192_live_endlist.m3u8", 60097999999},
        {"test_detach_hls/subtitles/eng/prog_index_live_endlist.m3u8", 66066000000}
    };
    for (const auto &[playlist, duration]: mediaPlaylistArr) {
        auto downloader = OpenHlsDetachAudioVideoWithMasterReady(playlist);

        EXPECT_EQ(downloader->GetSeekable(), Seekable::SEEKABLE);
        EXPECT_EQ(downloader->GetDuration(), duration);

        CloseHlsDetachAudioVideo(std::move(downloader));
    }
}

HWTEST_F(HlsMediaDownloaderTest, OPEN_HLS_ENCODE_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady(M3U8_PATH_1);
    auto playlistDownloader =
        std::static_pointer_cast<HlsPlayListDownloader>(downloader->videoSegManager_->playlistDownloader_);
    EXPECT_TRUE(playlistDownloader->GetAesDecryptor(1) != nullptr);
    EXPECT_FALSE(downloader->GetHLSDiscontinuity());
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, OPEN_HLS_ENCODE_002, TestSize.Level1)
{
    GTEST_SKIP() << "temporarily skipped: crashes with SIGSEGV, blocks .gcda generation";
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady(M3U8_PATH_2);
    int32_t videoStreamId = 0;
    int32_t audioStreamId = 0;
    int32_t subtitleStreamId = 0;
    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subtitleStreamId);
    downloader->GetSeekable();
    auto playlistDownloader =
        std::static_pointer_cast<HlsPlayListDownloader>(downloader->videoSegManager_->playlistDownloader_);
    EXPECT_TRUE(playlistDownloader->GetAesDecryptor(1) != nullptr);
    EXPECT_TRUE(playlistDownloader->GetAesDecryptor(UINT64_MAX) != nullptr);
    EXPECT_FALSE(downloader->GetHLSDiscontinuity());
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, OPEN_HLS_ENCODE_003, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady(M3U8_PATH_3);
    auto playlistDownloader =
        std::static_pointer_cast<HlsPlayListDownloader>(downloader->videoSegManager_->playlistDownloader_);
    EXPECT_TRUE(playlistDownloader->GetAesDecryptor(UINT64_MAX) != nullptr);
    EXPECT_FALSE(downloader->GetHLSDiscontinuity());
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, OPEN_HLS_ENCODE_004, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady(M3U8_PATH_4);
    auto playlistDownloader =
        std::static_pointer_cast<HlsPlayListDownloader>(downloader->videoSegManager_->playlistDownloader_);
    EXPECT_TRUE(playlistDownloader->GetAesDecryptor(1) != nullptr);
    EXPECT_FALSE(downloader->GetHLSDiscontinuity());
    downloader = nullptr;
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_VideoSegManagerNull
 * @tc.desc  : Test GetMaskUrlByReadIndex returns empty when videoSegManager_ is nullptr
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_VideoSegManagerNull, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    downloader->videoSegManager_ = nullptr;
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(1), "");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_EmptyUrl
 * @tc.desc  : Test GetMaskUrlByReadIndex returns empty when segment url is empty
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_EmptyUrl, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    ASSERT_TRUE(downloader->videoSegManager_ != nullptr);
    downloader->videoSegManager_->InfoIndexMap_.writeMap.clear();
    downloader->videoSegManager_->curUrl_.clear();
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(1), "");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_NoScheme
 * @tc.desc  : Test MaskUrl branch: url without scheme separator returned unchanged
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_NoScheme, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    ASSERT_TRUE(downloader->videoSegManager_ != nullptr);
    downloader->videoSegManager_->InfoIndexMap_.writeMap.clear();
    downloader->videoSegManager_->curUrl_ = "no-scheme";
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(1), "no-scheme");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_HostOnly
 * @tc.desc  : Test MaskUrl branch: url with no path returns scheme://host
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_HostOnly, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    ASSERT_TRUE(downloader->videoSegManager_ != nullptr);
    downloader->videoSegManager_->InfoIndexMap_.writeMap.clear();
    downloader->videoSegManager_->curUrl_ = "https://host";
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(1), "https://host");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_RootPath
 * @tc.desc  : Test MaskUrl branch: root path "/" stripped to scheme://host
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_RootPath, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    ASSERT_TRUE(downloader->videoSegManager_ != nullptr);
    downloader->videoSegManager_->InfoIndexMap_.writeMap.clear();
    downloader->videoSegManager_->curUrl_ = "https://host/";
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(1), "https://host");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_DirectoryPath
 * @tc.desc  : Test MaskUrl branch: directory path ending with "/" stripped
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_DirectoryPath, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    ASSERT_TRUE(downloader->videoSegManager_ != nullptr);
    downloader->videoSegManager_->InfoIndexMap_.writeMap.clear();
    downloader->videoSegManager_->curUrl_ = "https://host/a/b/";
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(1), "https://host");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_SingleSegment
 * @tc.desc  : Test MaskUrl branch: single-segment file path needs no mask
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_SingleSegment, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    ASSERT_TRUE(downloader->videoSegManager_ != nullptr);
    downloader->videoSegManager_->InfoIndexMap_.writeMap.clear();
    downloader->videoSegManager_->curUrl_ = "https://host/file.m3u8";
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(1), "https://host/file.m3u8");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_UserInfoStripped
 * @tc.desc  : Test MaskUrl branch: userinfo (user:pass@) is stripped
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_UserInfoStripped, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    ASSERT_TRUE(downloader->videoSegManager_ != nullptr);
    downloader->videoSegManager_->InfoIndexMap_.writeMap.clear();
    downloader->videoSegManager_->curUrl_ = "http://user:pass@host/dir/file";
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(1), "http://host/***/file");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_SegVideo
 * @tc.desc  : Test GetMaskUrlByReadIndex masks video segment url (SEG_VIDEO branch)
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_SegVideo, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    ASSERT_TRUE(downloader->videoSegManager_ != nullptr);
    downloader->videoSegManager_->InfoIndexMap_.writeMap.clear();
    downloader->videoSegManager_->curUrl_ = "https://host/dir/video.ts";
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(1), "https://host/***/video.ts");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_SegAudio
 * @tc.desc  : Test GetMaskUrlByReadIndex masks audio segment url (SEG_AUDIO branch)
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_SegAudio, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    ASSERT_TRUE(downloader->audioSegManager_ != nullptr);
    downloader->audioSegManager_->InfoIndexMap_.writeMap.clear();
    downloader->audioSegManager_->curUrl_ = "https://host/dir/audio.aac";
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(3), "https://host/***/audio.aac");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_SegSubtitle
 * @tc.desc  : Test GetMaskUrlByReadIndex masks subtitle segment url (SEG_SUBTITLE branch)
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_SegSubtitle, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    ASSERT_TRUE(downloader->subtitlesSegManager_ != nullptr);
    downloader->subtitlesSegManager_->InfoIndexMap_.writeMap.clear();
    downloader->subtitlesSegManager_->curUrl_ = "https://host/dir/sub.vtt";
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(4), "https://host/***/sub.vtt");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

/**
 * @tc.name  : Test GetMaskUrlByReadIndex API
 * @tc.number: GetMaskUrlByReadIndex_SegAudioManagerNull
 * @tc.desc  : Test GetMaskUrlByReadIndex returns empty when SEG_AUDIO maps to nullptr audioSegManager_
 */
HWTEST_F(HlsMediaDownloaderTest, GetMaskUrlByReadIndex_SegAudioManagerNull, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    downloader->audioSegManager_ = nullptr;
    EXPECT_EQ(downloader->GetMaskUrlByReadIndex(3), "");
    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, INIT_NULLPTR_VIDEO_MANAGER_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->videoSegManager_ = nullptr;
    downloader->Init();
    EXPECT_EQ(downloader->GetContentType(), "");
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, OPEN_WITH_REPORT_INFO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto reportInfo = std::make_shared<OHOS::MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->SetSourceStatisticsDfx(reportInfo);
    downloader->audioSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->audioSegManager_->Init();
    downloader->subtitlesSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->subtitlesSegManager_->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    auto ret = downloader->Open(testUrl, httpHeader);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(reportInfo->audioStreamCnt_, 0);
    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, OPEN_WITH_REPORT_INFO_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto reportInfo = std::make_shared<OHOS::MediaAVCodec::SourceStatisticsReportInfo>();
    downloader->SetSourceStatisticsDfx(reportInfo);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    auto ret = downloader->Open(testUrl, httpHeader);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(reportInfo->audioStreamCnt_, 0);
    EXPECT_EQ(reportInfo->subtitleCnt_, 0);
    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_DOWNLOAD_INFO_METRICS_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->downloadMetricsInfo_ = nullptr;
    DownloadInfo downloadInfo;
    downloader->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.totalDownLoadBytes, 0);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SET_PLAY_STRATEGY_WITH_MANAGERS_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->audioSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->audioSegManager_->Init();
    downloader->subtitlesSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->subtitlesSegManager_->Init();
    auto playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1280;
    playStrategy->height = 720;
    downloader->SetPlayStrategy(playStrategy);
    EXPECT_EQ(downloader->GetSegmentOffset(), 0);

    downloader->audioSegManager_ = nullptr;
    downloader->SetPlayStrategy(playStrategy);
    EXPECT_EQ(downloader->GetSegmentOffset(), 0);

    downloader->subtitlesSegManager_ = nullptr;
    downloader->SetPlayStrategy(playStrategy);
    EXPECT_EQ(downloader->GetSegmentOffset(), 0);

    downloader->videoSegManager_ = nullptr;
    downloader->SetPlayStrategy(playStrategy);
    EXPECT_EQ(downloader->GetSegmentOffset(), 0);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, NOTIFY_INIT_SUCCESS_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->subtitlesSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->subtitlesSegManager_->Init();
    downloader->NotifyInitSuccess();
    EXPECT_FALSE(downloader->subtitlesSegManager_->isReportedErrorCode_);

    downloader->videoSegManager_ = nullptr;
    downloader->NotifyInitSuccess();
    EXPECT_FALSE(downloader->subtitlesSegManager_->isReportedErrorCode_);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SET_INITIAL_BUFFER_SIZE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto ret = downloader->SetInitialBufferSize(0, 1024);
    EXPECT_FALSE(ret);

    downloader->videoSegManager_ = nullptr;
    ret = downloader->SetInitialBufferSize(0, 1024);
    EXPECT_FALSE(ret);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SELECT_STREAM_NO_AUDIO_SUBTITLE_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    downloader->audioSegManager_ = nullptr;
    auto ret = downloader->SelectStream(1);
    EXPECT_EQ(ret, Status::OK);

    downloader->subtitlesSegManager_ = nullptr;
    ret = downloader->SelectStream(1);
    EXPECT_EQ(ret, Status::OK);

    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, SELECT_STREAM_AUDIO_NULL_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    downloader->audioSegManager_ = nullptr;
    auto ret = downloader->SelectStream(3);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, SELECT_STREAM_SUBTITLE_NULL_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    downloader->subtitlesSegManager_ = nullptr;
    auto ret = downloader->SelectStream(4);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, SELECT_STREAM_NO_CALLBACK_001, TestSize.Level1)
{
    auto downloader = OpenHlsDetachAudioVideoWithMasterReady();
    downloader->SetCallback(nullptr);
    auto ret = downloader->SelectStream(1);
    EXPECT_EQ(ret, Status::OK);

    downloader->Close(true);
    CloseHlsDetachAudioVideo(downloader);
}

HWTEST_F(HlsMediaDownloaderTest, POST_ALL_EVENT_TIMED_METADATA_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto eventCallback = std::make_shared<EventCallback>();
    downloader->SetCallback(eventCallback);

    g_eventStatus = 0;
    HlsSegEvent event {};
    event.type = PluginEventType::SOURCE_TIMED_METADATA;
    downloader->PostAllEvent(event);
    EXPECT_EQ(g_eventStatus, 9);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, POST_ALL_EVENT_DEFAULT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto eventCallback = std::make_shared<EventCallback>();
    downloader->SetCallback(eventCallback);

    g_eventStatus = 0;
    HlsSegEvent event {};
    event.type = PluginEventType::SERVER_ERROR;
    downloader->PostAllEvent(event);
    EXPECT_EQ(g_eventStatus, 0);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, POST_ALL_EVENT_BUFFER_PROGRESS_NON_VIDEO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto eventCallback = std::make_shared<EventCallback>();
    downloader->SetCallback(eventCallback);

    g_eventStatus = 0;
    HlsSegEvent event {};
    event.type = PluginEventType::EVENT_BUFFER_PROGRESS;
    event.segType = HlsSegmentType::SEG_AUDIO;
    downloader->PostAllEvent(event);
    EXPECT_EQ(g_eventStatus, 0);

    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, WAIT_FOR_BUFFERING_END_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->WaitForBufferingEnd();
    EXPECT_EQ(downloader->GetSegmentOffset(), 0);

    downloader->videoSegManager_ = nullptr;
    downloader->WaitForBufferingEnd();
    EXPECT_EQ(downloader->GetSegmentOffset(), 0);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SET_CALLBACK_WITH_SUBTITLE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->audioSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->audioSegManager_->Init();
    downloader->subtitlesSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->subtitlesSegManager_->Init();
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    EXPECT_EQ(downloader->GetSegmentOffset(), 0);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_START_INFO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, 0);

    downloader->videoSegManager_ = nullptr;
    info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, 0);
    EXPECT_EQ(info.second, false);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_SEEKABLE_RANGES_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    auto ranges = downloader->GetSeekableRanges();
    EXPECT_EQ(ranges.size(), static_cast<size_t>(0));

    downloader->videoSegManager_ = nullptr;
    ranges = downloader->GetSeekableRanges();
    EXPECT_EQ(ranges.size(), static_cast<size_t>(0));
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_HLS_DISCONTINUITY_TRUE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& dl,
        std::shared_ptr<DownloadRequest>& req) {};
    auto playlistDownloader = downloader->videoSegManager_->playlistDownloader_;
    auto hlsPlaylistDownloader = static_pointer_cast<HlsPlayListDownloader>(playlistDownloader);
    hlsPlaylistDownloader->master_ =
        std::make_shared<M3U8MasterPlaylist>("playList_", "url_", 0, header_, statusCallback);
    hlsPlaylistDownloader->master_->hasDiscontinuity_ = true;
    auto ret = downloader->GetHLSDiscontinuity();
    EXPECT_TRUE(ret);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, GET_HLS_DISCONTINUITY_TRUE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& dl,
        std::shared_ptr<DownloadRequest>& req) {};

    auto videoPlaylist = downloader->videoSegManager_->playlistDownloader_;
    auto videoHlsPlaylist = static_pointer_cast<HlsPlayListDownloader>(videoPlaylist);
    videoHlsPlaylist->master_ =
        std::make_shared<M3U8MasterPlaylist>("p_", "u_", 0, header_, statusCallback);
    videoHlsPlaylist->master_->hasDiscontinuity_ = false;

    auto audioPlaylist = downloader->audioSegManager_->playlistDownloader_;
    auto audioHlsPlaylist = static_pointer_cast<HlsPlayListDownloader>(audioPlaylist);
    audioHlsPlaylist->master_ =
        std::make_shared<M3U8MasterPlaylist>("p_", "u_", 0, header_, statusCallback);
    audioHlsPlaylist->master_->hasDiscontinuity_ = true;

    auto ret = downloader->GetHLSDiscontinuity();
    EXPECT_TRUE(ret);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, STOP_BUFFERING_AUDIO_FAIL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(10, true, header_);
    downloader->Init();
    downloader->videoSegManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->videoSegManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE_UT);
    downloader->audioSegManager_ =
        std::make_shared<HlsSegmentManager>(downloader->videoSegManager_, HlsSegmentType::SEG_AUDIO);
    downloader->audioSegManager_->Init();
    downloader->audioSegManager_->Clone(downloader->videoSegManager_);
    downloader->audioSegManager_->cacheMediaBuffer_ = nullptr;
    auto ret = downloader->StopBufferring(true);
    EXPECT_NE(ret, Status::OK);
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderTest, SET_APP_UID_WITH_SUBTITLE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsMediaDownloader>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->subtitlesSegManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->subtitlesSegManager_->Init();
    downloader->SetAppUid(TEST_APP_UID);
    EXPECT_FALSE(downloader->subtitlesSegManager_->isReportedErrorCode_);
    downloader = nullptr;
}
}