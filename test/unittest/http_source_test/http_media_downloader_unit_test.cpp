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

#include "http_media_downloader_unit_test.h"
#include "http_server_demo.h"
#include "source_callback.h"

#define LOCAL true
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace std;
using namespace testing::ext;

const std::string MP4_SEGMENT_BASE = "http://127.0.0.1:46666/dewu.mp4";
const std::string MP4_NULL_SEGMENT_BASE = "http://127.0.0.1:46666/dewuNull.mp4";
const std::string FLV_SEGMENT_BASE = "http://127.0.0.1:46666/aigc_str.flv";
namespace {
    constexpr uint64_t MAX_CACHE_BUFFER_SIZE = 4 * 1024 * 1024;
    constexpr uint64_t MAX_RING_BUFFER_SIZE = 4 * 1024 * 1024;
    constexpr uint64_t MIN_CACHE_BUFFER_SIZE = 1 * 1024 * 1024;
    constexpr uint64_t MIN_RING_BUFFER_SIZE = 1 * 1024 * 1024;
    constexpr uint64_t DURATION_BUFFER_SIZE_RATIO = 512 * 1024;
}
std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server;
std::shared_ptr<HttpMediaDownloader> MP4httpMediaDownloader;
std::shared_ptr<HttpMediaDownloader> FLVhttpMediaDownloader;
std::shared_ptr<SourceCallback> g_mp4SourceCallback;
std::shared_ptr<SourceCallback> g_flvSourceCallback;
bool g_flvResult = false;
bool g_mp4Result = false;

void HttpMediaDownloaderUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
    MP4httpMediaDownloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, MAX_CACHE_BUFFER_SIZE, nullptr);
    MP4httpMediaDownloader->Init();
    FLVhttpMediaDownloader = std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, MAX_CACHE_BUFFER_SIZE, nullptr);
    FLVhttpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    MP4httpMediaDownloader->SetStatusCallback(statusCallback);
    FLVhttpMediaDownloader->SetStatusCallback(statusCallback);
    g_mp4SourceCallback = std::make_shared<SourceCallback>();
    MP4httpMediaDownloader->SetCallback(g_mp4SourceCallback);
    g_flvSourceCallback = std::make_shared<SourceCallback>();
    FLVhttpMediaDownloader->SetCallback(g_flvSourceCallback);
    g_mp4SourceCallback = nullptr;
    g_flvSourceCallback = nullptr;
    std::map<std::string, std::string> httpHeader;
    g_flvResult = FLVhttpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    g_mp4Result = MP4httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
}

void HttpMediaDownloaderUnitTest::TearDownTestCase(void)
{
    MP4httpMediaDownloader->Close(true);
    FLVhttpMediaDownloader->Close(true);
    MP4httpMediaDownloader = nullptr;
    FLVhttpMediaDownloader = nullptr;
    g_server->StopServer();
    g_server = nullptr;
}

void HttpMediaDownloaderUnitTest::SetUp()
{
}

void HttpMediaDownloaderUnitTest::TearDown()
{
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL_INIT, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader4 =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader4->Init();
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader5 =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader5->Init();
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader_4 =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader_4->Init();
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader_5 =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader_5->Init();
    EXPECT_TRUE(httpMediaDownloader4);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_RINGBUFFER, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    int retry = 0;
    while (retry++ < 600 && httpMediaDownloader->GetBufferSize() == 0) {
        OSAL::SleepFor(10);
    }
    unsigned char buff[5 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 5 * 1024 * 1024;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->SetDownloadErrorState();
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_DownloadReport, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    int retry = 0;
    while (retry++ < 600 && httpMediaDownloader->GetBufferSize() == 0) {
        OSAL::SleepFor(10);
    }
    httpMediaDownloader->DownloadReport();
    httpMediaDownloader->GetSeekable();
    unsigned char buff[5 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 5 * 1024 * 1024;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->SetDownloadErrorState();
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_DownloadReport_MP4, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    int retry = 0;
    while (retry++ < 600 && httpMediaDownloader->GetBufferSize() == 0) {
        OSAL::SleepFor(10);
    }
    httpMediaDownloader->DownloadReport();
    httpMediaDownloader->GetSeekable();
    unsigned char buff[5 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 5 * 1024 * 1024;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->SetDownloadErrorState();
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_DownloadReport_MP4_default, TestSize.Level0)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    ReadDataInfo readDataInfo;
    for (int i = 0; i < 800; i++) {
        OSAL::SleepFor(10);
        httpMediaDownloader->DownloadReport();
        unsigned char buff[10 * 1024];
        readDataInfo.streamId_ = 0;
        readDataInfo.wantReadLength_ = 10 * 1024;
        readDataInfo.isEos_ = false;
        httpMediaDownloader->Read(buff, readDataInfo);
        if (i == 3) {
            httpMediaDownloader->SetDemuxerState(0);
        }
    }
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_mp4_read_all, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    ReadDataInfo readDataInfo;
    for (int i = 0; i < 800; i++) {
        OSAL::SleepFor(10);
        unsigned char buff[100 * 1024];
        readDataInfo.streamId_ = 0;
        readDataInfo.wantReadLength_ = 100 * 1024;
        readDataInfo.isEos_ = false;
        httpMediaDownloader->Read(buff, readDataInfo);
        if (i == 3) {
            httpMediaDownloader->SetDemuxerState(0);
        }
    }
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL, TestSize.Level0)
{
    MP4httpMediaDownloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    MP4httpMediaDownloader->Read(buff, readDataInfo);
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_SEEK, TestSize.Level1)
{
    MP4httpMediaDownloader->GetSeekable();
    bool isSeekHit = false;
    bool result = MP4httpMediaDownloader->SeekToPos(100, isSeekHit);
    EXPECT_TRUE(result);
    result = MP4httpMediaDownloader->SeekToPos(10000000, isSeekHit);
    EXPECT_TRUE(result);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL_FLV, TestSize.Level1)
{
    FLVhttpMediaDownloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    FLVhttpMediaDownloader->Read(buff, readDataInfo);
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL_FLV_DUA, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    int retry = 0;
    while (retry++ < 600 && httpMediaDownloader->GetBufferSize() == 0) {
        OSAL::SleepFor(10);
    }
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL_FLV_MAX_BUFFER, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 30, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    int retry = 0;
    while (retry++ < 600 && httpMediaDownloader->GetBufferSize() == 0) {
        OSAL::SleepFor(10);
    }
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL_MP4_DUA, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5 * 1024, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL_MP4_DOWNLOADINFO, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5 * 1024, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    EXPECT_TRUE(httpMediaDownloader);
    DownloadInfo downloadInfo;
    httpMediaDownloader->GetDownloadInfo(downloadInfo);
    httpMediaDownloader->recordSpeedCount_ = 10;
    httpMediaDownloader->GetDownloadInfo(downloadInfo);
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->SetInterruptState(false);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_SEEK_FLV, TestSize.Level1)
{
    FLVhttpMediaDownloader->GetSeekable();
    bool isSeekHit = false;
    bool result = FLVhttpMediaDownloader->SeekToPos(100, isSeekHit);
    FLVhttpMediaDownloader->SetReadBlockingFlag(true);
    EXPECT_TRUE(result);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_MP4_ERROR, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->downloadErrorState_ = true;
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->CheckIsEosCacheBuffer(buff, readDataInfo);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_FLV_ERROR, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 30, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpMediaDownloader->SetCallback(sourceCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->CheckIsEosRingBuffer(buff, readDataInfo);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_FLC_SEEK, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    unsigned char buff[5 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 5 * 1024 * 1024;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->downloadErrorState_ = true;
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->Read(buff, readDataInfo);
    bool isSeekHit = false;
    httpMediaDownloader->SeekToPos(100 * 1024 * 1024, isSeekHit);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GetContentLength, TestSize.Level1)
{
    EXPECT_GE(MP4httpMediaDownloader->GetContentLength(), 0);
    EXPECT_GE(FLVhttpMediaDownloader->GetContentLength(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN, TestSize.Level1)
{
    EXPECT_TRUE(g_flvResult);
    EXPECT_TRUE(g_mp4Result);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_PAUSE_RESUME, TestSize.Level1)
{
    MP4httpMediaDownloader->Pause();
    FLVhttpMediaDownloader->Pause();
    EXPECT_TRUE(MP4httpMediaDownloader->isInterrupt_);
    EXPECT_TRUE(FLVhttpMediaDownloader->isInterrupt_);
    MP4httpMediaDownloader->Resume();
    FLVhttpMediaDownloader->Resume();
    EXPECT_FALSE(MP4httpMediaDownloader->isInterrupt_);
    EXPECT_FALSE(FLVhttpMediaDownloader->isInterrupt_);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_PLAYBACK_INFO_001, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    PlaybackInfo playbackInfo;
    httpMediaDownloader->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.serverIpAddress, "");
    EXPECT_EQ(playbackInfo.averageDownloadRate, 0);
    EXPECT_EQ(playbackInfo.isDownloading, true);
    EXPECT_EQ(playbackInfo.downloadRate, 0);
    EXPECT_EQ(playbackInfo.bufferDuration, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_PLAYBACK_INFO_002, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->totalDownloadDuringTime_ = 1000;
    httpMediaDownloader->totalBits_ = 1000;
    httpMediaDownloader->isDownloadFinish_ = true;
    httpMediaDownloader->recordData_->downloadRate = 1000;
    httpMediaDownloader->recordData_->bufferDuring = 1;
    PlaybackInfo playbackInfo;
    httpMediaDownloader->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.serverIpAddress, "");
    EXPECT_EQ(playbackInfo.averageDownloadRate, 1000);
    EXPECT_EQ(playbackInfo.isDownloading, false);
    EXPECT_EQ(playbackInfo.downloadRate, 1000);
    EXPECT_EQ(playbackInfo.bufferDuration, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, SET_PLAY_STRATEGY_001, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpMediaDownloader->callback_ = sourceCallback;
    httpMediaDownloader->SetPlayStrategy(nullptr);
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->bufferDurationForPlaying = 5;
    httpMediaDownloader->SetPlayStrategy(playStrategy);
}

HWTEST_F(HttpMediaDownloaderUnitTest, SET_PLAY_STRATEGY_002, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpMediaDownloader->callback_ = sourceCallback;

    MediaStreamList mediaStreams;
    std::shared_ptr<PlayMediaStream> mediaStreamA = std::make_shared<PlayMediaStream>();
    mediaStreamA->width = 480;
    mediaStreamA->height = 360;
    mediaStreamA->bitrate = 3200;
    mediaStreams.push_back(mediaStreamA);
    std::shared_ptr<PlayMediaStream> mediaStreamB = std::make_shared<PlayMediaStream>();
    mediaStreamB->width = 640;
    mediaStreamB->height = 480;
    mediaStreamB->bitrate = 4800;
    mediaStreams.push_back(mediaStreamB);
    std::shared_ptr<PlayMediaStream> mediaStreamC = std::make_shared<PlayMediaStream>();
    mediaStreamC->width = 640;
    mediaStreamC->height = 480;
    mediaStreamC->bitrate = 4000;
    mediaStreams.push_back(mediaStreamC);
    std::shared_ptr<PlayMediaStream> mediaStreamD = std::make_shared<PlayMediaStream>();
    mediaStreamD->width = 1280;
    mediaStreamD->height = 720;
    mediaStreamD->bitrate = 8000;
    mediaStreams.push_back(mediaStreamD);
    std::shared_ptr<PlayMediaStream> mediaStreamE = std::make_shared<PlayMediaStream>();
    mediaStreamE->width = 1280;
    mediaStreamE->height = 720;
    mediaStreamE->bitrate = 4000;
    mediaStreams.push_back(mediaStreamE);

    httpMediaDownloader->SetMediaStreams(mediaStreams);
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1280;
    playStrategy->height = 720;
    httpMediaDownloader->SetPlayStrategy(playStrategy);
    EXPECT_EQ(httpMediaDownloader->defaultStream_->width, 1280);
    EXPECT_EQ(httpMediaDownloader->defaultStream_->height, 720);
    EXPECT_EQ(httpMediaDownloader->defaultStream_->bitrate, 4000);
}

HWTEST_F(HttpMediaDownloaderUnitTest, SELECT_BIT_RATE_001, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpMediaDownloader->callback_ = sourceCallback;
    MediaStreamList mediaStreams;
    std::shared_ptr<PlayMediaStream> mediaStreamA = std::make_shared<PlayMediaStream>();
    mediaStreamA->width = 480;
    mediaStreamA->height = 360;
    mediaStreamA->bitrate = 3200;
    mediaStreamA->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamA);
    std::shared_ptr<PlayMediaStream> mediaStreamB = std::make_shared<PlayMediaStream>();
    mediaStreamB->width = 640;
    mediaStreamB->height = 480;
    mediaStreamB->bitrate = 4800;
    mediaStreamB->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamB);
    std::shared_ptr<PlayMediaStream> mediaStreamC = std::make_shared<PlayMediaStream>();
    mediaStreamC->width = 640;
    mediaStreamC->height = 480;
    mediaStreamC->bitrate = 4000;
    mediaStreamC->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamC);
    std::sort(mediaStreams.begin(), mediaStreams.end(),
        [](const std::shared_ptr<PlayMediaStream>& streamA, const std::shared_ptr<PlayMediaStream>& streamB) {
            return (streamA->bitrate < streamB->bitrate) ||
                (streamA->bitrate == streamB->bitrate &&
                    streamA->width * streamA->height < streamB->width * streamB->height);
    });
    httpMediaDownloader->SetMediaStreams(mediaStreams);
    httpMediaDownloader->SelectBitRate(4000);
    EXPECT_EQ(httpMediaDownloader->defaultStream_->width, 640);
    EXPECT_EQ(httpMediaDownloader->defaultStream_->height, 480);
    EXPECT_EQ(httpMediaDownloader->defaultStream_->bitrate, 4000);
}


HWTEST_F(HttpMediaDownloaderUnitTest, GetReadTimeOut, TestSize.Level1)
{
    auto media = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, MAX_CACHE_BUFFER_SIZE, nullptr);
    EXPECT_FALSE(media->GetReadTimeOut(false));
    EXPECT_FALSE(media->GetReadTimeOut(true));
}

HWTEST_F(HttpMediaDownloaderUnitTest, StopBufferring, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->StopBufferring(false);
    EXPECT_GE(httpMediaDownloader->StopBufferring(true), Status::OK);
}

HWTEST_F(HttpMediaDownloaderUnitTest, ClearHasReadBuffer, TestSize.Level1)
{
    MP4httpMediaDownloader->isFirstFrameArrived_ = true;
    MP4httpMediaDownloader->isNeedClearHasRead_ = true;
    EXPECT_FALSE(MP4httpMediaDownloader->ClearHasReadBuffer());
}

HWTEST_F(HttpMediaDownloaderUnitTest, RestartAndClearBuffer, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader1 =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader1->Init();
    auto statusCallback1 = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader1->SetStatusCallback(statusCallback1);
    std::map<std::string, std::string> httpHeader1;
    httpMediaDownloader1->Open(FLV_SEGMENT_BASE, httpHeader1);
    httpMediaDownloader1->RestartAndClearBuffer();
    
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader2 =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 4, nullptr);
    httpMediaDownloader2->Init();
    auto statusCallback2 = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader2->SetStatusCallback(statusCallback2);
    std::map<std::string, std::string> httpHeader2;
    httpMediaDownloader2->Open(MP4_SEGMENT_BASE, httpHeader2);
    httpMediaDownloader2->RestartAndClearBuffer();
    EXPECT_TRUE(httpMediaDownloader1->isRingBuffer_);
}

HWTEST_F(HttpMediaDownloaderUnitTest, ClearCacheBuffer, TestSize.Level1)
{
    FLVhttpMediaDownloader->ClearCacheBuffer();
    MP4httpMediaDownloader->ClearCacheBuffer();
    EXPECT_TRUE(FLVhttpMediaDownloader->isRingBuffer_);
}

HWTEST_F(HttpMediaDownloaderUnitTest, IsFlvLive, TestSize.Level1)
{
    bool ret = FLVhttpMediaDownloader->IsFlvLive();
    EXPECT_FALSE(ret);
}

HWTEST_F(HttpMediaDownloaderUnitTest, IsAutoSelectConditionOk_1, TestSize.Level1)
{
    FLVhttpMediaDownloader->downloadSpeeds_.clear();
 
    EXPECT_FALSE(FLVhttpMediaDownloader->IsAutoSelectConditionOk());

    FLVhttpMediaDownloader->ClearBuffer();
    MP4httpMediaDownloader->ClearBuffer();
}

HWTEST_F(HttpMediaDownloaderUnitTest, IsAutoSelectConditionOk_2, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpMediaDownloader->callback_ = sourceCallback;
    MediaStreamList mediaStreams;
    std::shared_ptr<PlayMediaStream> mediaStreamA = std::make_shared<PlayMediaStream>();
    mediaStreamA->width = 480;
    mediaStreamA->height = 360;
    mediaStreamA->bitrate = 3200;
    mediaStreamA->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamA);
    std::shared_ptr<PlayMediaStream> mediaStreamB = std::make_shared<PlayMediaStream>();
    mediaStreamB->width = 640;
    mediaStreamB->height = 480;
    mediaStreamB->bitrate = 4800;
    mediaStreamB->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamB);
    std::shared_ptr<PlayMediaStream> mediaStreamC = std::make_shared<PlayMediaStream>();
    mediaStreamC->width = 640;
    mediaStreamC->height = 480;
    mediaStreamC->bitrate = 4000;
    mediaStreamC->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamC);
    std::sort(mediaStreams.begin(), mediaStreams.end(),
        [](const std::shared_ptr<PlayMediaStream>& streamA, const std::shared_ptr<PlayMediaStream>& streamB) {
            return (streamA->bitrate < streamB->bitrate) ||
                (streamA->bitrate == streamB->bitrate &&
                    streamA->width * streamA->height < streamB->width * streamB->height);
    });
    httpMediaDownloader->SetMediaStreams(mediaStreams);
    httpMediaDownloader->SelectBitRate(4000);
    
    EXPECT_FALSE(httpMediaDownloader->IsAutoSelectConditionOk());
    httpMediaDownloader->isSelectingBitrate_ = true;
    EXPECT_FALSE(httpMediaDownloader->IsAutoSelectConditionOk());
    httpMediaDownloader->defaultStream_ = nullptr;

    EXPECT_FALSE(httpMediaDownloader->IsAutoSelectConditionOk());
}

HWTEST_F(HttpMediaDownloaderUnitTest, IsAutoSelectConditionOk, TestSize.Level1)
{
    FLVhttpMediaDownloader->isAutoSelectBitrate_.store(false);
    EXPECT_FALSE(FLVhttpMediaDownloader->IsAutoSelectConditionOk());

    FLVhttpMediaDownloader->isAutoSelectBitrate_ = true;
    FLVhttpMediaDownloader->isRingBuffer_ = false;
    EXPECT_FALSE(FLVhttpMediaDownloader->IsAutoSelectConditionOk());

    FLVhttpMediaDownloader->isAutoSelectBitrate_ = true;
    FLVhttpMediaDownloader->isRingBuffer_ = true;
    FLVhttpMediaDownloader->isSelectingBitrate_ = true;
    EXPECT_FALSE(FLVhttpMediaDownloader->IsAutoSelectConditionOk());

    FLVhttpMediaDownloader->isAutoSelectBitrate_ = true;
    FLVhttpMediaDownloader->isRingBuffer_ = true;
    FLVhttpMediaDownloader->isSelectingBitrate_ = false;
    FLVhttpMediaDownloader->downloadSpeeds_.clear();
    EXPECT_FALSE(FLVhttpMediaDownloader->IsAutoSelectConditionOk());
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_RING_BUFFER_SIZE_MAX, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 100, nullptr);
    downloader->Init();
    EXPECT_EQ(downloader->totalBufferSize_, MAX_RING_BUFFER_SIZE);
    EXPECT_TRUE(downloader->ringBuffer_ != nullptr);
    if (downloader->ringBuffer_ != nullptr) {
        EXPECT_EQ(downloader->ringBuffer_->bufferSize_, MAX_RING_BUFFER_SIZE);
    }
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_RING_BUFFER_SIZE_MIN, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 1, nullptr);
    downloader->Init();
    EXPECT_EQ(downloader->totalBufferSize_, MIN_RING_BUFFER_SIZE);
    EXPECT_TRUE(downloader->ringBuffer_ != nullptr);
    if (downloader->ringBuffer_ != nullptr) {
        EXPECT_EQ(downloader->ringBuffer_->bufferSize_, MIN_RING_BUFFER_SIZE);
    }
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_RING_BUFFER_SIZE_NORMAL, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 8, nullptr);
    downloader->Init();
    EXPECT_EQ(downloader->totalBufferSize_, 8 * DURATION_BUFFER_SIZE_RATIO);
    EXPECT_TRUE(downloader->ringBuffer_ != nullptr);
    if (downloader->ringBuffer_ != nullptr) {
        EXPECT_EQ(downloader->ringBuffer_->bufferSize_, 8 * DURATION_BUFFER_SIZE_RATIO);
    }
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_CACHE_BUFFER_SIZE_MAX, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 100, nullptr);
    downloader->Init();
    EXPECT_EQ(downloader->totalBufferSize_, MAX_CACHE_BUFFER_SIZE);
    EXPECT_TRUE(downloader->cacheMediaBuffer_ != nullptr);
    if (downloader->cacheMediaBuffer_ != nullptr) {
        EXPECT_EQ(downloader->cacheMediaBuffer_->totalBuffSize_, MAX_CACHE_BUFFER_SIZE);
    }
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_CACHE_BUFFER_SIZE_MIN, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 1, nullptr);
    downloader->Init();
    EXPECT_EQ(downloader->totalBufferSize_, MIN_CACHE_BUFFER_SIZE);
    EXPECT_TRUE(downloader->cacheMediaBuffer_ != nullptr);
    if (downloader->cacheMediaBuffer_ != nullptr) {
        EXPECT_EQ(downloader->cacheMediaBuffer_->totalBuffSize_, MIN_CACHE_BUFFER_SIZE);
    }
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_CACHE_BUFFER_SIZE_NORMAL, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    downloader->Init();
    EXPECT_EQ(downloader->totalBufferSize_, 8 * DURATION_BUFFER_SIZE_RATIO);
    EXPECT_TRUE(downloader->cacheMediaBuffer_ != nullptr);
    if (downloader->cacheMediaBuffer_ != nullptr) {
        EXPECT_EQ(downloader->cacheMediaBuffer_->totalBuffSize_, 8 * DURATION_BUFFER_SIZE_RATIO);
    }
}

HWTEST_F(HttpMediaDownloaderUnitTest, SET_DEFAULT_STREAM_ID_001, TestSize.Level1)
{
    auto httpMediaDownloader = std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);

    MediaStreamList mediaStreams;
    std::shared_ptr<PlayMediaStream> mediaStreamA = std::make_shared<PlayMediaStream>();
    mediaStreamA->width = 480;
    mediaStreamA->height = 360;
    mediaStreamA->bitrate = 3200;
    mediaStreamA->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamA);
    std::shared_ptr<PlayMediaStream> mediaStreamB = std::make_shared<PlayMediaStream>();
    mediaStreamB->width = 640;
    mediaStreamB->height = 480;
    mediaStreamB->bitrate = 4800;
    mediaStreamB->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamB);

    httpMediaDownloader->SetMediaStreams(mediaStreams);
    EXPECT_EQ(httpMediaDownloader->defaultStream_, mediaStreamA);
    
    int32_t videoStreamId = -1;
    int32_t audioStreamId = 0;
    int32_t subTitleStreamId = 0;
    httpMediaDownloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    EXPECT_EQ(httpMediaDownloader->defaultStream_, mediaStreamA);

    int32_t videoStreamIdNumSec = 1;
    int32_t audioStreamIdNumSec = 0;
    int32_t subTitleStreamIdNumSec = 0;
    httpMediaDownloader->SetDefaultStreamId(videoStreamIdNumSec, audioStreamIdNumSec, subTitleStreamIdNumSec);
    EXPECT_EQ(httpMediaDownloader->defaultStream_, mediaStreamB);
}

HWTEST_F(HttpMediaDownloaderUnitTest, OPEN_START_AND_OPEN_001, TestSize.Level1)
{
    auto httpMediaDownloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    std::map<std::string, std::string> httpHeader1;
    httpHeader1["User-Agent"] = "test-agent-1";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);

    bool ret1 = httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader1);
    EXPECT_TRUE(ret1);

    MediaStreamList mediaStreams;
    std::shared_ptr<PlayMediaStream> mediaStreamA = std::make_shared<PlayMediaStream>();
    mediaStreamA->width = 480;
    mediaStreamA->height = 360;
    mediaStreamA->bitrate = 3200;
    mediaStreamA->url = MP4_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamA);
    httpMediaDownloader->SetMediaStreams(mediaStreams);

    std::map<std::string, std::string> httpHeader2;
    httpHeader2["User-Agent"] = "test-agent-2";
    bool ret2 = httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader2);
    EXPECT_TRUE(ret2);
}

HWTEST_F(HttpMediaDownloaderUnitTest, SELECT_STREAM_001, TestSize.Level1)
{
    auto httpMediaDownloader = std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpMediaDownloader->SetCallback(sourceCallback);

    MediaStreamList mediaStreams;
    std::shared_ptr<PlayMediaStream> mediaStreamA = std::make_shared<PlayMediaStream>();
    mediaStreamA->width = 480;
    mediaStreamA->height = 360;
    mediaStreamA->bitrate = 3200;
    mediaStreamA->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamA);
    std::shared_ptr<PlayMediaStream> mediaStreamB = std::make_shared<PlayMediaStream>();
    mediaStreamB->width = 640;
    mediaStreamB->height = 480;
    mediaStreamB->bitrate = 4800;
    mediaStreamB->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamB);

    httpMediaDownloader->SetMediaStreams(mediaStreams);

    Status retInvalid = httpMediaDownloader->SelectStream(5);
    EXPECT_EQ(retInvalid, Status::OK);

    Status retOk = httpMediaDownloader->SelectStream(1);
    EXPECT_EQ(retOk, Status::OK);
    EXPECT_EQ(httpMediaDownloader->defaultStream_, mediaStreamA);
}

HWTEST_F(HttpMediaDownloaderUnitTest, SET_MEDIA_STREAMS_001, TestSize.Level1)
{
    auto httpMediaDownloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpMediaDownloader->callback_ = sourceCallback;

    MediaStreamList emptyStreams;
    httpMediaDownloader->SetMediaStreams(emptyStreams);
    EXPECT_EQ(httpMediaDownloader->defaultStream_, nullptr);

    MediaStreamList mediaStreams;
    std::shared_ptr<PlayMediaStream> mediaStreamA = std::make_shared<PlayMediaStream>();
    mediaStreamA->width = 480;
    mediaStreamA->height = 360;
    mediaStreamA->bitrate = 3200;
    mediaStreamA->url = MP4_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamA);
    std::shared_ptr<PlayMediaStream> mediaStreamB = std::make_shared<PlayMediaStream>();
    mediaStreamB->width = 640;
    mediaStreamB->height = 480;
    mediaStreamB->bitrate = 4800;
    mediaStreamB->url = MP4_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamB);

    httpMediaDownloader->SetMediaStreams(mediaStreams);
    EXPECT_EQ(httpMediaDownloader->defaultStream_, mediaStreamA);
}

HWTEST_F(HttpMediaDownloaderUnitTest, REPORT_BIT_RATE_001, TestSize.Level1)
{
    auto httpMediaDownloader = std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    auto sourceCallback = std::make_shared<SourceCallback>();
    httpMediaDownloader->callback_ = sourceCallback;

    MediaStreamList mediaStreams;
    std::shared_ptr<PlayMediaStream> mediaStreamA = std::make_shared<PlayMediaStream>();
    mediaStreamA->width = 480;
    mediaStreamA->height = 360;
    mediaStreamA->bitrate = 3200;
    mediaStreamA->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamA);
    std::shared_ptr<PlayMediaStream> mediaStreamB = std::make_shared<PlayMediaStream>();
    mediaStreamB->width = 640;
    mediaStreamB->height = 480;
    mediaStreamB->bitrate = 4800;
    mediaStreamB->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamB);

    httpMediaDownloader->SetMediaStreams(mediaStreams);
    httpMediaDownloader->downloadSpeeds_.push_back(4000);
    httpMediaDownloader->downloadSpeeds_.push_back(6000);

    EXPECT_TRUE(httpMediaDownloader->IsAutoSelectConditionOk());
    httpMediaDownloader->ReportBitRate();
    EXPECT_EQ(httpMediaDownloader->downloadSpeeds_.size(), 2u);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_STREAM_INFO_001, TestSize.Level1)
{
    auto httpMediaDownloader = std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();

    MediaStreamList mediaStreams;
    std::shared_ptr<PlayMediaStream> mediaStreamA = std::make_shared<PlayMediaStream>();
    mediaStreamA->width = 480;
    mediaStreamA->height = 360;
    mediaStreamA->bitrate = 3200;
    mediaStreamA->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamA);
    std::shared_ptr<PlayMediaStream> mediaStreamB = std::make_shared<PlayMediaStream>();
    mediaStreamB->width = 640;
    mediaStreamB->height = 480;
    mediaStreamB->bitrate = 4800;
    mediaStreamB->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamB);
    std::shared_ptr<PlayMediaStream> mediaStreamC = std::make_shared<PlayMediaStream>();
    mediaStreamC->width = 1280;
    mediaStreamC->height = 720;
    mediaStreamC->bitrate = 8000;
    mediaStreamC->url = FLV_SEGMENT_BASE;
    mediaStreams.push_back(mediaStreamC);

    httpMediaDownloader->SetMediaStreams(mediaStreams);

    std::vector<StreamInfo> streams;
    Status ret = httpMediaDownloader->GetStreamInfo(streams, false);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(streams.size(), 3u);
    EXPECT_EQ(streams[0].streamId, 0);

    int32_t videoStreamId = 1;
    int32_t audioStreamId = 0;
    int32_t subTitleStreamId = 0;
    httpMediaDownloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    std::vector<StreamInfo> streams2;
    ret = httpMediaDownloader->GetStreamInfo(streams2, false);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(streams2.size(), 3u);
    EXPECT_EQ(streams2[0].streamId, 1);
    EXPECT_EQ(streams2[1].streamId, 0);
    EXPECT_EQ(streams2[2].streamId, 2);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    downloader->Init();
    downloader->downloadRequest_ = nullptr;
    std::vector<SeekRange> ranges = downloader->GetSeekableRanges();
    EXPECT_TRUE(ranges.empty());
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(0)));
    downloader->downloadRequest_ = mockReq;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    downloader->downloadRequest_ = mockReq;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    downloader->downloadRequest_ = mockReq;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_005, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = false;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_006, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(true));
    downloader->downloadRequest_ = mockReq;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_007, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char *, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockReq, IsChunked(testing::_))
        .WillRepeatedly(testing::Return(false));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = true;
    downloader->mediaDuration_ = 1000000;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_008, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsChunked(testing::_))
        .WillRepeatedly(testing::Return(false));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = false;
    downloader->mediaDuration_ = 1000000;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_009, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = false;
    downloader->sourceLoader_ = nullptr;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_010, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    auto mockSourceLoader = std::make_shared<MockSourceLoader>();
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockSourceLoader, GetenableOfflineCache())
        .WillRepeatedly(testing::Return(true));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = false;
    downloader->sourceLoader_ = mockSourceLoader;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_011, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    auto mockSourceLoader = std::make_shared<MockSourceLoader>();
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockSourceLoader, GetenableOfflineCache())
        .WillRepeatedly(testing::Return(true));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = false;
    downloader->sourceLoader_ = mockSourceLoader;
    downloader->writeOffset_ = 0;
    downloader->readOffset_ = 0;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_012, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    auto mockSourceLoader = std::make_shared<MockSourceLoader>();
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockSourceLoader, GetenableOfflineCache())
        .WillRepeatedly(testing::Return(true));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = false;
    downloader->sourceLoader_ = mockSourceLoader;
    downloader->writeOffset_ = 100;
    downloader->mediaDuration_ = 1000000;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_013, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    auto mockSourceLoader = std::make_shared<MockSourceLoader>();
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockSourceLoader, GetenableOfflineCache())
        .WillRepeatedly(testing::Return(true));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = false;
    downloader->sourceLoader_ = mockSourceLoader;
    downloader->writeOffset_ = 10;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_014, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int {
        return 0;
    };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    auto mockSourceLoader = std::make_shared<MockSourceLoader>();
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockSourceLoader, GetenableOfflineCache())
        .WillRepeatedly(testing::Return(true));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = false;
    downloader->sourceLoader_ = mockSourceLoader;
    downloader->writeOffset_ = 1000;
    downloader->mediaDuration_ = 1000000;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_015, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    downloader->Init();
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int { return 0; };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    auto mockSourceLoader = std::make_shared<MockSourceLoader>();
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockSourceLoader, GetenableOfflineCache())
        .WillRepeatedly(testing::Return(true));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = false;
    downloader->sourceLoader_ = mockSourceLoader;
    downloader->writeOffset_ = 1;
    downloader->readOffset_ = 0;
    downloader->mediaDuration_ = 1000000;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_SEEKABLE_RANGES_016, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    ASSERT_NE(downloader, nullptr);
    downloader->Init();
    DataSaveFunc saveData = [](unsigned char*, unsigned int, bool) -> unsigned int { return 0; };
    StatusCallbackFunc statusCallback = [](
        OHOS::Media::Plugins::HttpPlugin::DownloadStatus,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::Downloader>&,
        std::shared_ptr<OHOS::Media::Plugins::HttpPlugin::DownloadRequest>&
    ) {};
    auto mockReq = std::make_shared<MockDownloadRequest>("test", saveData, statusCallback);
    EXPECT_CALL(*mockReq, GetFileContentLength())
        .WillRepeatedly(testing::Return(static_cast<size_t>(1024)));
    EXPECT_CALL(*mockReq, IsServerAcceptRange())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsChunkedVod())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockReq, IsEos())
        .WillRepeatedly(testing::Return(false));
    downloader->downloadRequest_ = mockReq;
    downloader->isDownloadFinish_ = false;
    downloader->mediaDuration_ = 0;
    auto res = downloader->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_RARE_SPEED_NORMAL, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    EXPECT_TRUE(downloader != nullptr);
    downloader->Init();
    downloader->GetBuffer();
    downloader->GetReadFrame();
    downloader->recordSpeedCount_ = 0;
    downloader->avgDownloadSpeed_ = 0;
    auto result = downloader->GetDownloadRateAndSpeed();

    auto expectResult = std::make_pair(0, 0);
    EXPECT_EQ(result, expectResult);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_RARE_SPEED_MIN, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    EXPECT_TRUE(downloader != nullptr);
    downloader->Init();
    downloader->recordSpeedCount_ = 1;
    downloader->avgDownloadSpeed_ = 0;
    downloader->avgSpeedSum_ = 1;
    auto result = downloader->GetDownloadRateAndSpeed();

    auto expectResult = std::make_pair(1, 0);
    EXPECT_EQ(result, expectResult);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_GETDOWNLOADERINFO_NORMAL, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    EXPECT_TRUE(downloader != nullptr);
    downloader->Init();
    downloader->recordSpeedCount_ = 1;
    downloader->avgDownloadSpeed_ = 0;
    downloader->avgSpeedSum_ = 1;
    auto result = downloader->GetDownloadInfo();

    auto expectResult = std::make_pair(1, 0);
    EXPECT_EQ(result, expectResult);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_GETREADFRAME_NORMAL, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    EXPECT_TRUE(downloader != nullptr);
    downloader->Init();
    downloader->isFirstFrameArrived_ = true;
    auto result = downloader->GetReadFrame();

    EXPECT_EQ(result, true);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_GETSTATUSCALLBACKFUNC_NORMAL, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    EXPECT_TRUE(downloader != nullptr);
    downloader->Init();
    downloader->statusCallback_ = nullptr;
    auto result = downloader->GetStatusCallbackFunc();

    EXPECT_EQ(result, nullptr);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_GETMEMORYSIZE_NORMAL, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    EXPECT_TRUE(downloader != nullptr);
    downloader->Init();
    downloader->totalBufferSize_ = 0;
    auto result = downloader->GetMemorySize();

    EXPECT_EQ(result, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_GETCURURL_NORMAL, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    EXPECT_TRUE(downloader != nullptr);
    downloader->Init();
    std::string url = "http";
    std::shared_ptr<DownloadRequest> downloadRequest = std::make_shared<DownloadRequest>(url, nullptr, nullptr);
    downloader->downloadRequest_ = downloadRequest;
    downloadRequest->haveRedirectRetry_.store(true);
    auto result = downloader->GetCurUrl();

    EXPECT_EQ(result, url);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_INITIAL_STREAM_SELECTION, TestSize.Level1)
{
    // Eight seconds of buffering; compare 360p/720p streams and break resolution ties by lower bitrate.
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    auto invalid = std::make_shared<PlayMediaStream>();
    invalid->width = USHRT_MAX;
    invalid->height = 1;
    auto low = std::make_shared<PlayMediaStream>();
    low->width = 640;
    low->height = 360;
    low->bitrate = 100;
    auto high = std::make_shared<PlayMediaStream>();
    high->width = 1280;
    high->height = 720;
    high->bitrate = 200;
    auto expensive = std::make_shared<PlayMediaStream>(*high);
    expensive->bitrate = 300;
    downloader->playMediaStreams_ = {nullptr, invalid, low, expensive, high};
    downloader->initResolution_ = 1280 * 720;
    downloader->ChooseStreamByResolution();
    EXPECT_EQ(downloader->defaultStream_, high);
    downloader->initResolution_ = 640 * 360;
    downloader->ChooseStreamByResolution();
    EXPECT_EQ(downloader->defaultStream_, low);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_ADDPARAMFORURL_NORMAL, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    EXPECT_TRUE(downloader != nullptr);
    downloader->Init();
    std::string url = "http?";
    std::string key = "1";
    std::string value = "1";
    downloader->AddParamForUrl(url, key, value);

    EXPECT_EQ(url, "http?&1=1");
}

/**
 * @tc.name  : Test UpdateOffsetThresholds max update
 * @tc.number: UpdateOffsetThresholds_001
 * @tc.desc  : Test UpdateOffsetThresholds when readOffset > maxReadOffset
 */
HWTEST_F(HttpMediaDownloaderUnitTest, UpdateOffsetThresholds_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    downloader->Init();
    downloader->maxReadOffset_ = 100;
    downloader->maxOffsetNotUpdateCount_ = 0;
    downloader->minReadOffset_ = 0;
    downloader->minOffsetNotUpdateCount_ = 0;
    downloader->isMinAndMaxOffsetUpdate_.store(true);
    downloader->stateChangeCount_ = 0;
    downloader->UpdateOffsetThresholds(200);
    EXPECT_EQ(downloader->maxReadOffset_, 200);
    EXPECT_EQ(downloader->maxOffsetNotUpdateCount_, 0);
}

/**
 * @tc.name  : Test UpdateOffsetThresholds max not update
 * @tc.number: UpdateOffsetThresholds_002
 * @tc.desc  : Test UpdateOffsetThresholds when readOffset <= maxReadOffset
 */
HWTEST_F(HttpMediaDownloaderUnitTest, UpdateOffsetThresholds_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    downloader->Init();
    downloader->maxReadOffset_ = 200;
    downloader->maxOffsetNotUpdateCount_ = 0;
    downloader->minReadOffset_ = 0;
    downloader->minOffsetNotUpdateCount_ = 0;
    downloader->isMinAndMaxOffsetUpdate_.store(true);
    downloader->stateChangeCount_ = 0;
    downloader->UpdateOffsetThresholds(100);
    EXPECT_EQ(downloader->maxReadOffset_, 200);
    EXPECT_EQ(downloader->maxOffsetNotUpdateCount_, 1);
}

/**
 * @tc.name  : Test UpdateOffsetThresholds large span
 * @tc.number: UpdateOffsetThresholds_003
 * @tc.desc  : Test UpdateOffsetThresholds sets isLargeOffsetSpan when span > threshold
 */
HWTEST_F(HttpMediaDownloaderUnitTest, UpdateOffsetThresholds_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    downloader->Init();
    downloader->maxReadOffset_ = 5000000;
    downloader->maxOffsetNotUpdateCount_ = 0;
    downloader->minReadOffset_ = 0;
    downloader->minOffsetNotUpdateCount_ = 0;
    downloader->isMinAndMaxOffsetUpdate_.store(true);
    downloader->stateChangeCount_ = 0;
    downloader->UpdateOffsetThresholds(5000000);
    EXPECT_TRUE(downloader->isLargeOffsetSpan_.load());
}

/**
 * @tc.name  : Test FindStreamByBitRate
 * @tc.number: FindStreamByBitRate_001
 * @tc.desc  : Test FindStreamByBitRate with single stream
 */
HWTEST_F(HttpMediaDownloaderUnitTest, FindStreamByBitRate_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 8, nullptr);
    downloader->Init();
    auto stream1 = std::make_shared<PlayMediaStream>();
    stream1->bitrate = 500;
    downloader->playMediaStreams_ = {stream1};
    auto result = downloader->FindStreamByBitRate(500);
    EXPECT_EQ(result, stream1);
}
HWTEST_F(HttpMediaDownloaderUnitTest, CONTENT_FLV_FLAG_OVERRIDES_URL_SUFFIX, TestSize.Level1)
{
    auto opaqueFlv = std::make_shared<HttpMediaDownloader>("http://example.test/stream", 1, nullptr, true);
    EXPECT_TRUE(opaqueFlv->isRingBuffer_);
    EXPECT_FALSE(opaqueFlv->isLive_.load());
    auto misleadingUrl = std::make_shared<HttpMediaDownloader>("http://example.test/video.flv", 1, nullptr, false);
    EXPECT_FALSE(misleadingUrl->isRingBuffer_);
    EXPECT_FALSE(misleadingUrl->isLive_.load());
}

HWTEST_F(HttpMediaDownloaderUnitTest, FINITE_FLV_CONTENT_IS_NOT_LIVE, TestSize.Level1)
{
    // FLV version 1, audio/video flags 0x05, big-endian offset equal to the nine-byte header.
    const uint8_t header[] = {'F', 'L', 'V', 1, 5, 0, 0, 0, 9};
    std::string bytes(reinterpret_cast<const char*>(header), sizeof(header));
    bytes.append(512, 'x'); // A small finite body that fits entirely in the 1 KiB read buffer below.
    MediaAVCodec::HttpServerDemo::RegisterContent("/finite_flv", bytes);
    auto media = std::make_shared<HttpMediaDownloader>("http://127.0.0.1:46666/finite_flv", 1, nullptr, true);
    media->Init();
    media->SetStatusCallback([](DownloadStatus, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {});
    ASSERT_TRUE(media->Open("http://127.0.0.1:46666/finite_flv", {}));
    uint8_t buffer[1024] {};
    ReadDataInfo info;
    info.wantReadLength_ = sizeof(buffer);
    EXPECT_EQ(media->Read(buffer, info), Status::OK);
    EXPECT_EQ(std::string(reinterpret_cast<char*>(buffer), info.realReadLength_), bytes);
    EXPECT_FALSE(media->IsFlvLive());
    EXPECT_FALSE(media->isLive_.load());
    auto seekable = media->GetSeekable();
    info.wantReadLength_ = sizeof(buffer);
    EXPECT_EQ(media->Read(buffer, info), Status::END_OF_STREAM);
    media->Close(false);
    MediaAVCodec::HttpServerDemo::ClearContent();
    EXPECT_EQ(seekable, Seekable::SEEKABLE);
}

}
