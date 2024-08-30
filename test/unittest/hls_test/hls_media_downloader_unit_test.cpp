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

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace std;
using namespace testing::ext;


const std::map<std::string, std::string> httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"},
};
static const std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
static const std::string M3U8_PATH_1 = "test_hls/testHLSEncode.m3u8";
constexpr int MIN_WITDH = 480;
constexpr int SECOND_WITDH = 720;
constexpr int THIRD_WITDH = 1080;
constexpr int MAX_RECORD_COUNT = 10;
constexpr uint32_t READ_SLEEP_TIME_OUT = 30 * 1000;

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;

void HlsMediaDownloaderUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
}

void HlsMediaDownloaderUnitTest::TearDownTestCase(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

void HlsMediaDownloaderUnitTest ::SetUp(void)
{
    hlsMediaDownloader = new HlsMediaDownloader();
}

void HlsMediaDownloaderUnitTest ::TearDown(void)
{
    delete hlsMediaDownloader;
    hlsMediaDownloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, GetDownloadInfo1, TestSize.Level1)
{
    hlsMediaDownloader->recordSpeedCount_ = 0;
    DownloadInfo downloadInfo;
    hlsMediaDownloader->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.avgDownloadRate, 0);
}

HWTEST_F(HlsMediaDownloaderUnitTest, GetDownloadInfo2, TestSize.Level1)
{
    hlsMediaDownloader->recordSpeedCount_ = 5;
    hlsMediaDownloader->avgSpeedSum_ = 25;
    DownloadInfo downloadInfo;
    hlsMediaDownloader->GetDownloadInfo(downloadInfo);
}

HWTEST_F(HlsMediaDownloaderUnitTest, GetDownloadInfo3, TestSize.Level1)
{
    hlsMediaDownloader->avgDownloadSpeed_ = 10;
    DownloadInfo downloadInfo;
    hlsMediaDownloader->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.avgDownloadSpeed, 10);
}

HWTEST_F(HlsMediaDownloaderUnitTest, GetDownloadInfo4, TestSize.Level1)
{
    hlsMediaDownloader->totalBits_ = 50;
    DownloadInfo downloadInfo;
    hlsMediaDownloader->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.totalDownLoadBits, 50);
}

HWTEST_F(HlsMediaDownloaderUnitTest, GetDownloadInfo5, TestSize.Level1)
{
    hlsMediaDownloader->isTimeOut_ = true;
    DownloadInfo downloadInfo;
    hlsMediaDownloader->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.isTimeOut, true);
}

HWTEST_F(HlsMediaDownloaderUnitTest, GetRingBufferSize, TestSize.Level1)
{
    size_t actualSize = hlsMediaDownloader->GetRingBufferSize();
    EXPECT_EQ(actualSize, 0);
}

HWTEST_F(HlsMediaDownloaderUnitTest, GetTotalBufferSize, TestSize.Level1)
{
    hlsMediaDownloader->totalRingBufferSize_ = 1024;
    EXPECT_EQ(hlsMediaDownloader->GetTotalBufferSize(), 1024);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TransferSizeToBitRate1, TestSize.Level1)
{
    int width = MIN_WITDH;
    uint64_t expectedBitRate = RING_BUFFER_SIZE;
    uint64_t actualBitRate = hlsMediaDownloader->TransferSizeToBitRate(width);
    EXPECT_EQ(expectedBitRate, actualBitRate);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TransferSizeToBitRate2, TestSize.Level1)
{
    int width = SECOND_WITDH - 1;
    uint64_t expectedBitRate = RING_BUFFER_SIZE + RING_BUFFER_SIZE;
    uint64_t actualBitRate = hlsMediaDownloader->TransferSizeToBitRate(width);
    EXPECT_EQ(expectedBitRate, actualBitRate);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TransferSizeToBitRate3, TestSize.Level1)
{
    int width = THIRD_WITDH - 1;
    uint64_t expectedBitRate = RING_BUFFER_SIZE + RING_BUFFER_SIZE + RING_BUFFER_SIZE;
    uint64_t actualBitRate = hlsMediaDownloader->TransferSizeToBitRate(width);
    EXPECT_EQ(expectedBitRate, actualBitRate);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TransferSizeToBitRate4, TestSize.Level1)
{
    int width = THIRD_WITDH + 1;
    uint64_t expectedBitRate = RING_BUFFER_SIZE + RING_BUFFER_SIZE + RING_BUFFER_SIZE + RING_BUFFER_SIZE;
    uint64_t actualBitRate = hlsMediaDownloader->TransferSizeToBitRate(width);
    EXPECT_EQ(expectedBitRate, actualBitRate);
}

HWTEST_F(HlsMediaDownloaderUnitTest, InActiveAutoBufferSize, TestSize.Level1)
{
    hlsMediaDownloader->InActiveAutoBufferSize();
    EXPECT_FALSE(hlsMediaDownloader->autoBufferSize_);
}

HWTEST_F(HlsMediaDownloaderUnitTest, ActiveAutoBufferSize1, TestSize.Level1)
{
    hlsMediaDownloader->ActiveAutoBufferSize();
    EXPECT_TRUE(hlsMediaDownloader->autoBufferSize_);
}

HWTEST_F(HlsMediaDownloaderUnitTest, ActiveAutoBufferSize2, TestSize.Level1)
{
    hlsMediaDownloader->userDefinedBufferDuration_ = true;
    bool oldAutoBufferSize = hlsMediaDownloader->autoBufferSize_;
    hlsMediaDownloader->ActiveAutoBufferSize();
    EXPECT_EQ(oldAutoBufferSize, hlsMediaDownloader->autoBufferSize_);
}

HWTEST_F(HlsMediaDownloaderUnitTest, OnReadRingBuffer1, TestSize.Level1)
{
    uint32_t len = 100;
    hlsMediaDownloader->bufferedDuration_ = 50;
    hlsMediaDownloader->OnReadRingBuffer(len);
    EXPECT_EQ(hlsMediaDownloader->bufferedDuration_, 0);
}

HWTEST_F(HlsMediaDownloaderUnitTest, OnReadRingBuffer2, TestSize.Level1)
{
    uint32_t len = 50;
    hlsMediaDownloader->bufferedDuration_ = 100;
    hlsMediaDownloader->OnReadRingBuffer(len);
    EXPECT_LT(hlsMediaDownloader->bufferedDuration_, 100);
}

HWTEST_F(HlsMediaDownloaderUnitTest, OnReadRingBuffer3, TestSize.Level1)
{
    uint32_t len = 50;
    hlsMediaDownloader->bufferedDuration_ = 0;
    hlsMediaDownloader->lastReadTime_ = 0;
    hlsMediaDownloader->OnReadRingBuffer(len);
    EXPECT_NE(hlsMediaDownloader->bufferLeastRecord_, nullptr);
}

HWTEST_F(HlsMediaDownloaderUnitTest, OnReadRingBuffer4, TestSize.Level1)
{
    uint32_t len = 50;
    hlsMediaDownloader->bufferedDuration_ = 0;
    hlsMediaDownloader->lastReadTime_ = 0;
    for (int i = 0; i < MAX_RECORD_COUNT + 1; i++) {
        hlsMediaDownloader->OnReadRingBuffer(len);
    }
    EXPECT_NE(hlsMediaDownloader->bufferLeastRecord_->next, nullptr);
}

HWTEST_F(HlsMediaDownloaderUnitTest, DownBufferSize1, TestSize.Level1)
{
    hlsMediaDownloader->totalRingBufferSize_ = 10 * 1024 * 1024;
    hlsMediaDownloader->DownBufferSize();
    EXPECT_EQ(hlsMediaDownloader->totalRingBufferSize_, 9 * 1024 * 1024);
}

HWTEST_F(HlsMediaDownloaderUnitTest, DownBufferSize2, TestSize.Level1)
{
    hlsMediaDownloader->totalRingBufferSize_ = RING_BUFFER_SIZE;
    hlsMediaDownloader->DownBufferSize();
    EXPECT_EQ(hlsMediaDownloader->totalRingBufferSize_, RING_BUFFER_SIZE);
}

HWTEST_F(HlsMediaDownloaderUnitTest, RiseBufferSize1, TestSize.Level1)
{
    hlsMediaDownloader->totalRingBufferSize_ = 0;
    hlsMediaDownloader->RiseBufferSize();
    EXPECT_EQ(hlsMediaDownloader->totalRingBufferSize_, 1 * 1024 * 1024);
}

HWTEST_F(HlsMediaDownloaderUnitTest, RiseBufferSize2, TestSize.Level1)
{
    hlsMediaDownloader->totalRingBufferSize_ = MAX_BUFFER_SIZE;
    hlsMediaDownloader->RiseBufferSize();
    EXPECT_EQ(hlsMediaDownloader->totalRingBufferSize_, MAX_BUFFER_SIZE);
}

HWTEST_F(HlsMediaDownloaderUnitTest, CheckPulldownBufferSize, TestSize.Level1)
{
    hlsMediaDownloader->recordData_ = nullptr;
    bool result = hlsMediaDownloader->CheckPulldownBufferSize();
    EXPECT_FALSE(result);
}

HWTEST_F(HlsMediaDownloaderUnitTest, CheckRiseBufferSize, TestSize.Level1)
{
    hlsMediaDownloader->recordData_ = nullptr;
    bool result = hlsMediaDownloader->CheckRiseBufferSize();
    EXPECT_FALSE(result);
}

HWTEST_F(HlsMediaDownloaderUnitTest, SetDownloadErrorState, TestSize.Level1)
{
    hlsMediaDownloader->SetDownloadErrorState();
    EXPECT_TRUE(hlsMediaDownloader->downloadErrorState_);
}

HWTEST_F(HlsMediaDownloaderUnitTest, SetDemuxerState, TestSize.Level1)
{
    hlsMediaDownloader->SetDemuxerState(0);
    EXPECT_TRUE(hlsMediaDownloader->isReadFrame_);
    EXPECT_TRUE(hlsMediaDownloader->isFirstFrameArrived_);
}

HWTEST_F(HlsMediaDownloaderUnitTest, CheckReadTimeOut1, TestSize.Level1)
{
    hlsMediaDownloader->readTime_ = READ_SLEEP_TIME_OUT;
    EXPECT_TRUE(hlsMediaDownloader->CheckReadTimeOut());
}

HWTEST_F(HlsMediaDownloaderUnitTest, CheckReadTimeOut2, TestSize.Level1)
{
    hlsMediaDownloader->downloadErrorState_ = true;
    EXPECT_TRUE(hlsMediaDownloader->CheckReadTimeOut());
}

HWTEST_F(HlsMediaDownloaderUnitTest, CheckReadTimeOut3, TestSize.Level1)
{
    hlsMediaDownloader->isTimeOut_ = true;
    EXPECT_TRUE(hlsMediaDownloader->CheckReadTimeOut());
}

HWTEST_F(HlsMediaDownloaderUnitTest, CheckReadTimeOut4, TestSize.Level1)
{
    hlsMediaDownloader->downloader_ = nullptr;
    EXPECT_FALSE(hlsMediaDownloader->CheckReadTimeOut());
}

HWTEST_F(HlsMediaDownloaderUnitTest, CheckReadTimeOut5, TestSize.Level1)
{
    hlsMediaDownloader->callback_ = nullptr;
    EXPECT_FALSE(hlsMediaDownloader->CheckReadTimeOut());
}

HWTEST_F(HlsMediaDownloaderUnitTest, CheckReadTimeOut6, TestSize.Level1)
{
    hlsMediaDownloader->readTime_ = READ_SLEEP_TIME_OUT - 1;
    hlsMediaDownloader->downloadErrorState_ = false;
    hlsMediaDownloader->isTimeOut_ = false;
    EXPECT_FALSE(hlsMediaDownloader->CheckReadTimeOut());
}

HWTEST_F(HlsMediaDownloaderUnitTest, CheckBreakCondition, TestSize.Level1)
{
    hlsMediaDownloader->downloadErrorState_ = true;
    EXPECT_TRUE(hlsMediaDownloader->CheckBreakCondition());
}

HWTEST_F(HlsMediaDownloaderUnitTest, HandleBuffering, TestSize.Level1)
{
    hlsMediaDownloader->isBuffering_ = false;
    EXPECT_FALSE(hlsMediaDownloader->HandleBuffering());
}

HWTEST_F(HlsMediaDownloaderUnitTest, TestDefaultConstructor, TestSize.Level1)
{
    EXPECT_EQ(hlsMediaDownloader->totalRingBufferSize_, RING_BUFFER_SIZE);
}

HWTEST_F(HlsMediaDownloaderUnitTest, SAVE_HEADER_001, TestSize.Level1)
{
    hlsMediaDownloader->SaveHttpHeader(httpHeader);
    EXPECT_EQ(hlsMediaDownloader->httpHeader_["User-Agent"], "ABC");
    EXPECT_EQ(hlsMediaDownloader->httpHeader_["Referer"], "DEF");
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_OPEN_001, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(1000);
    EXPECT_EQ(downloader->expectDuration_, static_cast<uint64_t>(1000));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_OPEN_002, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    EXPECT_EQ(downloader->expectDuration_, static_cast<uint64_t>(10));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_PAUSE, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->Pause();
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_SEEK_TO_TIME, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    bool result = downloader->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_TRUE(result);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_001, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10);
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

    OSAL::SleepFor(4 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_Encrypted, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    downloader->GetStartedStatus();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(4 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_null, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>("application/m3u8");
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    downloader->GetStartedStatus();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(4 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_MAX_M3U8, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>(10);
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720_5K.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    downloader->GetStartedStatus();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(4 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_SelectBR, TestSize.Level1)
{
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>("test");
    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
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
    downloader->AutoSelectBitrate(1000000);

    OSAL::SleepFor(4 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_WRITE_RINGBUFFER_001, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    downloader->OnWriteRingBuffer(0);
    downloader->OnWriteRingBuffer(0);
    EXPECT_EQ(downloader->bufferedDuration_, 0);
    EXPECT_EQ(downloader->totalBits_, 0);
    EXPECT_EQ(downloader->lastWriteBit_, 0);

    downloader->OnWriteRingBuffer(1);
    downloader->OnWriteRingBuffer(1);
    EXPECT_EQ(downloader->bufferedDuration_, 16);
    EXPECT_EQ(downloader->totalBits_, 16);
    EXPECT_EQ(downloader->lastWriteBit_, 16);

    downloader->OnWriteRingBuffer(1000);
    downloader->OnWriteRingBuffer(1000);
    EXPECT_EQ(downloader->bufferedDuration_, 16016);
    EXPECT_EQ(downloader->totalBits_, 16016);
    EXPECT_EQ(downloader->lastWriteBit_, 16016);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, RISE_BUFFER_001, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    downloader->totalRingBufferSize_ = MAX_BUFFER_SIZE;
    downloader->RiseBufferSize();
    EXPECT_EQ(downloader->totalRingBufferSize_, MAX_BUFFER_SIZE);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, RISE_BUFFER_002, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    downloader->totalRingBufferSize_ = RING_BUFFER_SIZE;
    downloader->RiseBufferSize();
    EXPECT_EQ(downloader->totalRingBufferSize_, 6 * 1024 * 1024);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, DOWN_BUFFER_001, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    downloader->totalRingBufferSize_ = 10 * 1024 * 1024;
    downloader->DownBufferSize();
    EXPECT_EQ(downloader->totalRingBufferSize_, 9 * 1024 * 1024);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, DOWN_BUFFER_002, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    downloader->totalRingBufferSize_ = RING_BUFFER_SIZE;
    downloader->DownBufferSize();
    EXPECT_EQ(downloader->totalRingBufferSize_, RING_BUFFER_SIZE);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, GET_PLAYBACK_INFO_001, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    PlaybackInfo playbackInfo;
    downloader->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.serverIpAddress, "");
    EXPECT_EQ(playbackInfo.averageDownloadRate, 0);
    EXPECT_EQ(playbackInfo.isDownloading, true);
    EXPECT_EQ(playbackInfo.downloadRate, 0);
    EXPECT_EQ(playbackInfo.bufferDuration, 0);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, GET_PLAYBACK_INFO_002, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->totalDownloadDuringTime_ = 1000;
    downloader->totalBits_ = 1000;
    downloader->isDownloadFinish_ = true;
    std::shared_ptr<HlsMediaDownloader::RecordData> recordBuff = std::make_shared<HlsMediaDownloader::RecordData>();
    recordBuff->downloadRate = 1000;
    recordBuff->bufferDuring = 1;
    recordBuff->next = downloader->recordData_;
    downloader->recordData_ = recordBuff;

    PlaybackInfo playbackInfo;
    downloader->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.serverIpAddress, "");
    EXPECT_EQ(playbackInfo.averageDownloadRate, 1000);
    EXPECT_EQ(playbackInfo.isDownloading, false);
    EXPECT_EQ(playbackInfo.downloadRate, 1000);
    EXPECT_EQ(playbackInfo.bufferDuration, 0);
    delete downloader;
    downloader = nullptr;
}

}