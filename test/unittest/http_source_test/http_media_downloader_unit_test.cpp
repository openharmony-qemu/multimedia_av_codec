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
    unsigned char buff[5 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 5 * 1024 * 1024;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->SetDownloadErrorState();
    OSAL::SleepFor(1 * 1000);
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
    OSAL::SleepFor(6 * 1000);
    httpMediaDownloader->DownloadReport();
    httpMediaDownloader->GetSeekable();
    unsigned char buff[5 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 5 * 1024 * 1024;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->SetDownloadErrorState();
    OSAL::SleepFor(1 * 1000);
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
    OSAL::SleepFor(6 * 1000);
    httpMediaDownloader->DownloadReport();
    httpMediaDownloader->GetSeekable();
    unsigned char buff[5 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 5 * 1024 * 1024;
    readDataInfo.isEos_ = true;
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->SetDownloadErrorState();
    OSAL::SleepFor(1 * 1000);
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
    OSAL::SleepFor(1 * 1000);
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
    OSAL::SleepFor(1 * 1000);
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
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 30, nullptr);
    httpMediaDownloader->Init();
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
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
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
    httpMediaDownloader->OnClientErrorEvent();
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

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_MP4, TestSize.Level1)
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
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    httpMediaDownloader->UpdateWaterLineAbove();
    httpMediaDownloader->SetDownloadErrorState();
    httpMediaDownloader->SetCurrentBitRate(-1, 0);
    httpMediaDownloader->SetCurrentBitRate(1000, 0);
    httpMediaDownloader->UpdateWaterLineAbove();
    httpMediaDownloader->ChangeDownloadPos(false);
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
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
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
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
    httpMediaDownloader->OnClientErrorEvent();
    httpMediaDownloader->Close(true);
    httpMediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, TEST_MP4_NULL, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_NULL_SEGMENT_BASE, MAX_CACHE_BUFFER_SIZE, nullptr);
    httpMediaDownloader->Init();
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
    httpMediaDownloader->UpdateWaterLineAbove();
    httpMediaDownloader->SetDownloadErrorState();
    httpMediaDownloader->SetCurrentBitRate(-1, 0);
    httpMediaDownloader->SetCurrentBitRate(1000, 0);
    httpMediaDownloader->UpdateWaterLineAbove();
    httpMediaDownloader->HandleCachedDuration();
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
    httpMediaDownloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1000);
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

HWTEST_F(HttpMediaDownloaderUnitTest, HANDLE_WATER_LINE_001, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->callback_ = sourceCallback;
    httpMediaDownloader->canWrite_ = false;
    httpMediaDownloader->waterLineAbove_ = 0;
    httpMediaDownloader->readOffset_ = 0;
    httpMediaDownloader->initCacheSize_ = 5000;
    httpMediaDownloader->HandleWaterline();
    EXPECT_EQ(httpMediaDownloader->initCacheSize_, -1);
    EXPECT_EQ(httpMediaDownloader->isBuffering_, false);
}

HWTEST_F(HttpMediaDownloaderUnitTest, CACHE_BUFFER_FULL_LOOP_001, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->callback_ = sourceCallback;
    httpMediaDownloader->isHitSeeking_ = true;
    httpMediaDownloader->initCacheSize_ = 1;
    EXPECT_EQ(httpMediaDownloader->CacheBufferFullLoop(), true);
    httpMediaDownloader->isHitSeeking_ = false;
    EXPECT_EQ(httpMediaDownloader->CacheBufferFullLoop(), false);
}

HWTEST_F(HttpMediaDownloaderUnitTest, SET_INITIAL_BUFFER_SIZE_001, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    httpMediaDownloader->Open(MP4_SEGMENT_BASE, httpHeader);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->callback_ = sourceCallback;
    httpMediaDownloader->isBuffering_ = false;
    EXPECT_EQ(httpMediaDownloader->SetInitialBufferSize(0, 0), false);
    EXPECT_EQ(httpMediaDownloader->SetInitialBufferSize(0, 20000000), true);
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
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->callback_ = sourceCallback;
    httpMediaDownloader->SetPlayStrategy(nullptr);
    EXPECT_EQ(httpMediaDownloader->waterlineForPlaying_, 0);
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->bufferDurationForPlaying = 5;
    httpMediaDownloader->SetPlayStrategy(playStrategy);
    EXPECT_NE(httpMediaDownloader->waterlineForPlaying_, 0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, IS_NEED_BUFFER_FOR_PLAYING_001, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->callback_ = sourceCallback;
    EXPECT_EQ(httpMediaDownloader->IsNeedBufferForPlaying(), false);
    httpMediaDownloader->bufferDurationForPlaying_ = 5;
    httpMediaDownloader->isDemuxerInitSuccess_ = true;
    httpMediaDownloader->isBuffering_ = true;
    httpMediaDownloader->bufferingTime_ = static_cast<size_t>(httpMediaDownloader->
                        steadyClock_.ElapsedMilliseconds()) - 100 * 1000;
    EXPECT_EQ(httpMediaDownloader->IsNeedBufferForPlaying(), false);
    httpMediaDownloader->bufferingTime_ = 0;
    httpMediaDownloader->waterlineForPlaying_ = 0;
    EXPECT_EQ(httpMediaDownloader->IsNeedBufferForPlaying(), false);
}

HWTEST_F(HttpMediaDownloaderUnitTest, NOTIFY_INIT_SUCCESS_001, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(MP4_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpMediaDownloader->callback_ = sourceCallback;
    httpMediaDownloader->isBuffering_ = false;
    httpMediaDownloader->NotifyInitSuccess();
    EXPECT_EQ(httpMediaDownloader->isBuffering_, false);
    httpMediaDownloader->bufferDurationForPlaying_ = 5;
    httpMediaDownloader->currentBitRate_ = 1000000;
    httpMediaDownloader->NotifyInitSuccess();
    EXPECT_EQ(httpMediaDownloader->isBuffering_, true);
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
    Plugins::Callback* sourceCallback = new SourceCallback();
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
    Plugins::Callback* sourceCallback = new SourceCallback();
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

HWTEST_F(HttpMediaDownloaderUnitTest, CHECK_AUTO_SELECT_BITRATE_001, TestSize.Level1)
{
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(FLV_SEGMENT_BASE, 5, nullptr);
    httpMediaDownloader->Init();
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    httpMediaDownloader->Open(FLV_SEGMENT_BASE, httpHeader);
    Plugins::Callback* sourceCallback = new SourceCallback();
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
    httpMediaDownloader->downloadSpeeds_.push_back(5100);
    EXPECT_EQ(httpMediaDownloader->CheckAutoSelectBitrate(), true);
    httpMediaDownloader->downloadSpeeds_.push_back(3200);
    EXPECT_EQ(httpMediaDownloader->CheckAutoSelectBitrate(), false);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GetReadTimeOut, TestSize.Level1)
{
    bool ret = MP4httpMediaDownloader->GetReadTimeOut(false);
    ret = MP4httpMediaDownloader->GetReadTimeOut(true);
    EXPECT_FALSE(ret);
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

HWTEST_F(HttpMediaDownloaderUnitTest, GetPlayable, TestSize.Level1)
{
    MP4httpMediaDownloader->isBuffering_ = true;
    EXPECT_FALSE(MP4httpMediaDownloader->GetPlayable());

    MP4httpMediaDownloader->isBuffering_ = false;
    MP4httpMediaDownloader->isFirstFrameArrived_ = true;
    MP4httpMediaDownloader->GetPlayable();
    
    MP4httpMediaDownloader->isBuffering_ = false;
    MP4httpMediaDownloader->isFirstFrameArrived_ = false;
    MP4httpMediaDownloader->GetPlayable();
    MP4httpMediaDownloader->SetAppUid(1);
}

HWTEST_F(HttpMediaDownloaderUnitTest, GetCacheDuration, TestSize.Level1)
{
    float num = MP4httpMediaDownloader->GetCacheDuration(1);
    num = MP4httpMediaDownloader->GetCacheDuration(0);
    EXPECT_EQ(num, 1.0);
}

HWTEST_F(HttpMediaDownloaderUnitTest, IsAutoSelectConditionOk_1, TestSize.Level1)
{
    FLVhttpMediaDownloader->downloadSpeeds_.clear();
 
    EXPECT_FALSE(FLVhttpMediaDownloader->IsAutoSelectConditionOk());
    EXPECT_FALSE(FLVhttpMediaDownloader->CheckAutoSelectBitrate());

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
    Plugins::Callback* sourceCallback = new SourceCallback();
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
}