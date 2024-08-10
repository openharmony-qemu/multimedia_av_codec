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
const std::string FLV_SEGMENT_BASE = "http://127.0.0.1:46666/h264.flv";

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server;
std::shared_ptr<HttpMediaDownloader> MP4httpMediaDownloader;
std::shared_ptr<HttpMediaDownloader> FLVhttpMediaDownloader;
bool g_flvResult = false;
bool g_mp4Result = false;

void HttpMediaDownloaderUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
    MP4httpMediaDownloader = std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE);
    FLVhttpMediaDownloader = std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    MP4httpMediaDownloader->SetStatusCallback(statusCallback);
    FLVhttpMediaDownloader->SetStatusCallback(statusCallback);
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

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL, TestSize.Level1)
{
    MP4httpMediaDownloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    MP4httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_SEEK, TestSize.Level1)
{
    MP4httpMediaDownloader->GetSeekable();
    bool result = MP4httpMediaDownloader->SeekToPos(100);
    EXPECT_TRUE(result);
    result = MP4httpMediaDownloader->SeekToPos(10000000);
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
    OSAL::SleepFor(1 * 1000);
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL_FLV_DUA, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 5);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL_FLV_MAX_BUFFER, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 30);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL_MP4_DUA, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5 * 1024);
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
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_OPEN_URL_MP4_DOWNLOADINFO, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5 * 1024);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    DownloadInfo downloadInfo;
    httpMediaDownloader->GetDownloadInfo(downloadInfo);
    httpMediaDownloader->recordSpeedCount_ = 10;
    httpMediaDownloader->GetDownloadInfo(downloadInfo);
    httpMediaDownloader->OnClientErrorEvent();
    httpMediaDownloader->SetInterruptState(true);
    httpMediaDownloader->SetInterruptState(false);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_SEEK_FLV, TestSize.Level1)
{
    FLVhttpMediaDownloader->GetSeekable();
    bool result = FLVhttpMediaDownloader->SeekToPos(100);
    FLVhttpMediaDownloader->SetReadBlockingFlag(true);
    EXPECT_TRUE(result);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_MP4, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5);
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
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    httpMediaDownloader->GetWaterLineAbove();
    httpMediaDownloader->SetDownloadErrorState();
    httpMediaDownloader->SetCurrentBitRate(-1);
    httpMediaDownloader->SetCurrentBitRate(1000);
    httpMediaDownloader->GetWaterLineAbove();
    httpMediaDownloader->ChangeDownloadPos();
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_MP4_ERROR, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5);
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
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    httpMediaDownloader->CheckIsEosCacheBuffer(buff, readDataInfo);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_FLV_ERROR, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 30);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->SetCallback(sourceCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    httpMediaDownloader->CheckIsEosRingBuffer(buff, readDataInfo);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_MP4_NULL, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_NULL_SEGMENT_BASE);
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->SetCallback(sourceCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(MP4_NULL_SEGMENT_BASE, httpHeader);
    httpMediaDownloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    httpMediaDownloader->GetWaterLineAbove();
    httpMediaDownloader->SetDownloadErrorState();
    httpMediaDownloader->SetCurrentBitRate(-1);
    httpMediaDownloader->SetCurrentBitRate(1000);
    httpMediaDownloader->GetWaterLineAbove();
    httpMediaDownloader->HandleCachedDuration();
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
}

HWTEST_F(HttpMediaDownloaderUnitTest, GetContentLength, TestSize.Level1)
{
    MP4httpMediaDownloader->GetContentLength();
    FLVhttpMediaDownloader->GetContentLength();
}

HWTEST_F(HttpMediaDownloaderUnitTest, GetStartedStatus, TestSize.Level1)
{
    MP4httpMediaDownloader->startedPlayStatus_ = false;
    FLVhttpMediaDownloader->startedPlayStatus_ = false;
    EXPECT_FALSE(MP4httpMediaDownloader->GetStartedStatus());
    EXPECT_FALSE(FLVhttpMediaDownloader->GetStartedStatus());
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

HWTEST_F(HttpMediaDownloaderUnitTest, HandleBuffering1, TestSize.Level1)
{
    MP4httpMediaDownloader->isBuffering_ = false;
    EXPECT_FALSE(MP4httpMediaDownloader->HandleBuffering());
}

HWTEST_F(HttpMediaDownloaderUnitTest, GET_PLAYBACK_INFO_001, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5);
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
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5);
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
    EXPECT_EQ(playbackInfo.bufferDuration, 1);
}

}