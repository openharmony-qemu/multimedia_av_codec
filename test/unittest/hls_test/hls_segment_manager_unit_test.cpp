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
#include "hls_segment_manager_unit_test.h"
#include "http_server_demo.h"
#include "source_callback.h"

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace std;
using namespace testing::ext;

constexpr uint32_t RING_BUFFER_SIZE = 1 * 1024 * 1024;
constexpr uint64_t MAX_CACHE_BUFFER_SIZE_UT = 19 * 1024 * 1024;

const std::map<std::string, std::string> httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"},
};
static const std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
static const std::string M3U8_PATH_1 = "test_hls/testHLSEncode.m3u8";
static const std::string M3U8_PATH_X_MAP = "test_hls/testXMap.m3u8";
static const std::string M3U8_PATH_BYTE_RANGE = "test_hls/testByteRange.m3u8";
constexpr int MIN_WITDH = 480;
constexpr int SECOND_WITDH = 720;
constexpr int THIRD_WITDH = 1080;
constexpr int MAX_RECORD_COUNT = 10;
constexpr int BUFFER_SIZE = 10;
constexpr int MAX_TEN = 10 * 1024;

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;

void HlsSegmentManagerUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
}

void HlsSegmentManagerUnitTest::TearDownTestCase(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

void HlsSegmentManagerUnitTest ::SetUp(void)
{
    header_ = std::map<std::string, std::string>();
    hlsSegmentManager_ = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    hlsSegmentManager_->Init();
}

void HlsSegmentManagerUnitTest ::TearDown(void)
{
    hlsSegmentManager_ = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, GetPlayable_1, TestSize.Level0)
{
    EXPECT_FALSE(hlsSegmentManager_->GetPlayable());
    hlsSegmentManager_->isFirstFrameArrived_ = false;
    EXPECT_FALSE(hlsSegmentManager_->GetPlayable());
    hlsSegmentManager_->GetReadTimeOut(false);
}

HWTEST_F(HlsSegmentManagerUnitTest, GetPlayable_2, TestSize.Level0)
{
    hlsSegmentManager_->isFirstFrameArrived_ = true;
    hlsSegmentManager_->wantedReadLength_ = 0;
    EXPECT_FALSE(hlsSegmentManager_->GetPlayable());
}

HWTEST_F(HlsSegmentManagerUnitTest, GetPlayable_3, TestSize.Level1)
{
    hlsSegmentManager_->isFirstFrameArrived_ = true;
    hlsSegmentManager_->wantedReadLength_ = BUFFER_SIZE;
    EXPECT_FALSE(hlsSegmentManager_->GetPlayable());

    hlsSegmentManager_->wantedReadLength_ = MAX_TEN;
    EXPECT_FALSE(hlsSegmentManager_->GetPlayable());
}

HWTEST_F(HlsSegmentManagerUnitTest, GetDownloadInfo1, TestSize.Level1)
{
    hlsSegmentManager_->recordSpeedCount_ = 0;
    DownloadInfo downloadInfo;
    hlsSegmentManager_->GetDownloadInfo(downloadInfo);
    hlsSegmentManager_->GetDownloadInfo();
    EXPECT_EQ(downloadInfo.avgDownloadRate, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, GetDownloadInfo2, TestSize.Level1)
{
    hlsSegmentManager_->recordSpeedCount_ = 5;
    hlsSegmentManager_->avgSpeedSum_ = 25;
    DownloadInfo downloadInfo;
    hlsSegmentManager_->GetDownloadInfo(downloadInfo);
    hlsSegmentManager_->GetDownloadInfo();
    EXPECT_EQ(downloadInfo.avgDownloadRate, 5);
}

HWTEST_F(HlsSegmentManagerUnitTest, GetDownloadInfo3, TestSize.Level1)
{
    hlsSegmentManager_->avgDownloadSpeed_ = 10;
    DownloadInfo downloadInfo;
    hlsSegmentManager_->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.avgDownloadSpeed, 10);
}

HWTEST_F(HlsSegmentManagerUnitTest, GetDownloadInfo4, TestSize.Level1)
{
    hlsSegmentManager_->totalBits_ = 50;
    DownloadInfo downloadInfo;
    hlsSegmentManager_->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.totalDownLoadBits, 50);
}

HWTEST_F(HlsSegmentManagerUnitTest, GetDownloadInfo5, TestSize.Level1)
{
    hlsSegmentManager_->isTimeOut_ = true;
    DownloadInfo downloadInfo;
    hlsSegmentManager_->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.isTimeOut, true);
}

HWTEST_F(HlsSegmentManagerUnitTest, GetBufferSize, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    uint64_t actualSize = downloader->GetBufferSize();
    EXPECT_EQ(actualSize, 0);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, GetTotalBufferSize, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    uint64_t actualSize = downloader->GetTotalBufferSize();
    EXPECT_EQ(actualSize, 10 * 200 * 1024);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, TransferSizeToBitRate1, TestSize.Level1)
{
    int width = MIN_WITDH;
    uint64_t expectedBitRate = RING_BUFFER_SIZE;
    uint64_t actualBitRate = hlsSegmentManager_->TransferSizeToBitRate(width);
    EXPECT_EQ(expectedBitRate, actualBitRate);
}

HWTEST_F(HlsSegmentManagerUnitTest, TransferSizeToBitRate2, TestSize.Level1)
{
    int width = SECOND_WITDH - 1;
    uint64_t expectedBitRate = RING_BUFFER_SIZE + RING_BUFFER_SIZE;
    uint64_t actualBitRate = hlsSegmentManager_->TransferSizeToBitRate(width);
    EXPECT_EQ(expectedBitRate, actualBitRate);
}

HWTEST_F(HlsSegmentManagerUnitTest, TransferSizeToBitRate3, TestSize.Level1)
{
    int width = THIRD_WITDH - 1;
    uint64_t expectedBitRate = RING_BUFFER_SIZE + RING_BUFFER_SIZE + RING_BUFFER_SIZE;
    uint64_t actualBitRate = hlsSegmentManager_->TransferSizeToBitRate(width);
    EXPECT_EQ(expectedBitRate, actualBitRate);
}

HWTEST_F(HlsSegmentManagerUnitTest, TransferSizeToBitRate4, TestSize.Level1)
{
    int width = THIRD_WITDH + 1;
    uint64_t expectedBitRate = RING_BUFFER_SIZE + RING_BUFFER_SIZE + RING_BUFFER_SIZE + RING_BUFFER_SIZE;
    uint64_t actualBitRate = hlsSegmentManager_->TransferSizeToBitRate(width);
    EXPECT_EQ(expectedBitRate, actualBitRate);
}

HWTEST_F(HlsSegmentManagerUnitTest, InActiveAutoBufferSize, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    EXPECT_TRUE(downloader->autoBufferSize_);
    downloader->InActiveAutoBufferSize();
    EXPECT_FALSE(downloader->autoBufferSize_);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, ActiveAutoBufferSize1, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->InActiveAutoBufferSize();
    EXPECT_FALSE(downloader->autoBufferSize_);
    downloader->ActiveAutoBufferSize();
    EXPECT_FALSE(downloader->autoBufferSize_);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, ActiveAutoBufferSize2, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->InActiveAutoBufferSize();
    EXPECT_FALSE(downloader->autoBufferSize_);
    downloader->ActiveAutoBufferSize();
    EXPECT_TRUE(downloader->autoBufferSize_);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, OnReadRingBuffer1, TestSize.Level1)
{
    uint32_t len = 100;
    hlsSegmentManager_->bufferedDuration_ = 50;
    hlsSegmentManager_->OnReadBuffer(len);
    EXPECT_EQ(hlsSegmentManager_->bufferedDuration_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, OnReadRingBuffer2, TestSize.Level1)
{
    uint32_t len = 50;
    hlsSegmentManager_->bufferedDuration_ = 100;
    hlsSegmentManager_->OnReadBuffer(len);
    EXPECT_EQ(hlsSegmentManager_->bufferedDuration_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, OnReadRingBuffer3, TestSize.Level1)
{
    uint32_t len = 50;
    hlsSegmentManager_->bufferedDuration_ = 0;
    hlsSegmentManager_->OnReadBuffer(len);
    EXPECT_EQ(hlsSegmentManager_->bufferedDuration_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, OnReadRingBuffer4, TestSize.Level1)
{
    uint32_t len = 50;
    hlsSegmentManager_->bufferedDuration_ = 0;
    for (int i = 0; i < MAX_RECORD_COUNT + 1; i++) {
        hlsSegmentManager_->OnReadBuffer(len);
    }
    EXPECT_EQ(hlsSegmentManager_->bufferedDuration_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, DownBufferSize1, TestSize.Level1)
{
    hlsSegmentManager_->totalBufferSize_ = 10 * 1024 * 1024;
    hlsSegmentManager_->DownBufferSize();
    EXPECT_EQ(hlsSegmentManager_->totalBufferSize_, 10 * 1024 * 1024 - 200 * 1024);
}

HWTEST_F(HlsSegmentManagerUnitTest, DownBufferSize2, TestSize.Level1)
{
    hlsSegmentManager_->totalBufferSize_ = RING_BUFFER_SIZE;
    hlsSegmentManager_->DownBufferSize();
    EXPECT_EQ(hlsSegmentManager_->totalBufferSize_, RING_BUFFER_SIZE);
}

HWTEST_F(HlsSegmentManagerUnitTest, RiseBufferSize1, TestSize.Level1)
{
    hlsSegmentManager_->totalBufferSize_ = 0;
    hlsSegmentManager_->RiseBufferSize();
    EXPECT_EQ(hlsSegmentManager_->totalBufferSize_, 200 * 1024);
}

HWTEST_F(HlsSegmentManagerUnitTest, RiseBufferSize2, TestSize.Level1)
{
    uint32_t maxCachebufferSize = 4 * 1024 * 1024
    hlsSegmentManager_->totalBufferSize_ = maxCachebufferSize;
    hlsSegmentManager_->RiseBufferSize();
    EXPECT_EQ(hlsSegmentManager_->totalBufferSize_, maxCachebufferSize);
}

HWTEST_F(HlsSegmentManagerUnitTest, CheckPulldownBufferSize, TestSize.Level1)
{
    hlsSegmentManager_->recordData_ = nullptr;
    bool result = hlsSegmentManager_->CheckPulldownBufferSize();
    EXPECT_FALSE(result);
}

HWTEST_F(HlsSegmentManagerUnitTest, CheckRiseBufferSize, TestSize.Level1)
{
    hlsSegmentManager_->recordData_ = nullptr;
    bool result = hlsSegmentManager_->CheckRiseBufferSize();
    EXPECT_FALSE(result);
}

HWTEST_F(HlsSegmentManagerUnitTest, SetDownloadErrorState, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    EXPECT_FALSE(downloader->downloadErrorState_);
    downloader->SetDownloadErrorState();
    EXPECT_TRUE(downloader->downloadErrorState_);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, SetDemuxerState, TestSize.Level1)
{
    hlsSegmentManager_->SetDemuxerState(0);
    EXPECT_TRUE(hlsSegmentManager_->isReadFrame_);
    EXPECT_TRUE(hlsSegmentManager_->isFirstFrameArrived_);
}

HWTEST_F(HlsSegmentManagerUnitTest, CheckBreakCondition, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    EXPECT_FALSE(downloader->CheckBreakCondition());
    downloader->SetDownloadErrorState();
    EXPECT_TRUE(downloader->CheckBreakCondition());
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, TestDefaultConstructor, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    EXPECT_EQ(downloader->totalBufferSize_, MAX_CACHE_BUFFER_SIZE_UT);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, SAVE_HEADER_001, TestSize.Level1)
{
    hlsSegmentManager_->SaveHttpHeader(httpHeader);
    EXPECT_EQ(hlsSegmentManager_->httpHeader_["User-Agent"], "ABC");
    EXPECT_EQ(hlsSegmentManager_->httpHeader_["Referer"], "DEF");
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_OPEN_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(1000, true, header_);
    downloader->Init();
    EXPECT_EQ(downloader->expectDuration_, static_cast<uint64_t>(19));
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_OPEN_002, TestSize.Level1)
{
    bool userDefinedDuration = true;
    const int expectBufDuration = 10; // 10s
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(expectBufDuration,
        userDefinedDuration, header_);
    downloader->Init();
    EXPECT_EQ(downloader->expectDuration_, static_cast<uint64_t>(expectBufDuration));
    downloader = nullptr;

    userDefinedDuration = false;
    downloader = std::make_shared<HlsSegmentManager>(expectBufDuration,
        userDefinedDuration, header_);
    downloader->Init();
    EXPECT_EQ(downloader->expectDuration_, 0);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_PAUSE, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    EXPECT_TRUE(downloader);
    downloader->isInterrupt_ = false;
    downloader->Pause();
    downloader->Resume();
    downloader->Pause();
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_SEEK_TO_TIME, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
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

HWTEST_F(HlsSegmentManagerUnitTest, HandleFfmpegReadback_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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

    downloader->readTsIndex_ = 0;
    downloader->HandleFfmpegReadback(1000);
    downloader->Read(buff, readDataInfo);

    downloader->readTsIndex_ = 0;
    downloader->ffmpegOffset_ = 10000;
    downloader->HandleFfmpegReadback(1000);

    downloader->readTsIndex_ = 1;
    downloader->ffmpegOffset_ = 10000;
    downloader->HandleFfmpegReadback(1000);

    downloader->readTsIndex_ = 10000;
    downloader->ffmpegOffset_ = 10000;
    downloader->HandleFfmpegReadback(1000);

    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, CheckPlaylist_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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

    downloader->isStopped = true;
    downloader->CheckPlaylist(buff, readDataInfo);
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(1 * 1000);
    downloader->isStopped = true;
    downloader->CheckPlaylist(buff, readDataInfo);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, SeekToTsForRead_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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

    OSAL::SleepFor(1 * 1000);
    downloader->SeekToTsForRead(100);
    downloader->SeekToTsForRead(0);
    downloader->SetIsTriggerAutoMode(false);

    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, ReportVideoSizeChange_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->ReportVideoSizeChange();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->ReportVideoSizeChange();
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(1 * 1000);
    downloader->ReportVideoSizeChange();
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, DownloadRecordHistory_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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

    OSAL::SleepFor(1 * 1000);
    downloader->DownloadRecordHistory(2000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, CheckRiseBufferSize_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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

    OSAL::SleepFor(1 * 1000);
    std::shared_ptr<HlsSegmentManager::RecordData> recordBuff =
        std::make_shared<HlsSegmentManager::RecordData>();
    recordBuff->bufferDuring = 10;
    downloader->recordData_ = recordBuff;
    downloader->CheckRiseBufferSize();
    downloader->CheckPulldownBufferSize();

    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, GetDownloadRateAndSpeed, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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

    downloader->recordSpeedCount_ = 0;
    downloader->GetDownloadRateAndSpeed();
    downloader->recordSpeedCount_ = 1;
    downloader->GetDownloadRateAndSpeed();
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_READ_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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

    OSAL::SleepFor(1 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_CALLBACK, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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
    EXPECT_GE(readDataInfo.realReadLength_, 0);
    OSAL::SleepFor(1 * 1000);

    downloader->SetCurrentBitRate(-1, 0);
    downloader->SetCurrentBitRate(10, 0);
    downloader->HandleCachedDuration();
    downloader->SetInterruptState(true);
    downloader->SetInterruptState(false);
    downloader->GetContentLength();
    downloader->GetDuration();
    downloader->GetBitRates();
    downloader->SetReadBlockingFlag(true);
    downloader->SetReadBlockingFlag(false);
    downloader->ReportBitrateStart(100);
    downloader->CalculateBitRate(0, 0);
    downloader->CalculateBitRate(1, 0);
    std::multimap<std::string, std::vector<uint8_t>> drmInfos;
    downloader->OnDrmInfoChanged(drmInfos);
    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_CALLBACK1, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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
    OSAL::SleepFor(1 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_DownloadReport, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/1080_3M/video_1080.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    ReadDataInfo readDataInfo;
    for (int i = 0; i < 80; i++) {
        OSAL::SleepFor(100);
        downloader->DownloadReport();

        unsigned char buff[100 * 1024];
        readDataInfo.streamId_ = 0;
        readDataInfo.wantReadLength_ = 100 * 1024;
        readDataInfo.isEos_ = false;
        downloader->Read(buff, readDataInfo);
    }
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_DownloadReport_5M, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/1080_3M/video_1080.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(6 * 1000);
    downloader->DownloadReport();
    downloader->GetSeekable();
    unsigned char buff[5 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 5 * 1024 * 1024;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_DownloadReport_5M_default, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/1080_3M/video_1080.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    ReadDataInfo readDataInfo;
    for (int i = 0; i < 80; i++) {
        OSAL::SleepFor(10);
        downloader->DownloadReport();

        unsigned char buff[10 * 1024];
        readDataInfo.streamId_ = 0;
        readDataInfo.wantReadLength_ = 10 * 1024;
        readDataInfo.isEos_ = false;
        downloader->Read(buff, readDataInfo);
        if (i == 3) {
            downloader->SetDemuxerState(0);
        }
    }
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_read_all, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    ReadDataInfo readDataInfo;
    for (int i = 0; i < 80; i++) {
        OSAL::SleepFor(10);
        unsigned char buff[10 * 1024];
        readDataInfo.streamId_ = 0;
        readDataInfo.wantReadLength_ = 10 * 1024;
        readDataInfo.isEos_ = false;
        downloader->Read(buff, readDataInfo);
    }
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_Read_Live, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720_live.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[5 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 5 * 1024 * 1024;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    OSAL::SleepFor(1 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_READ_Encrypted, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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

    OSAL::SleepFor(2 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_READ_Encrypted_session_key, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode_session_key.m3u8";
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

    OSAL::SleepFor(2 * 1000);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_OPEN_URL, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = 
        std::make_shared<HlsSegmentManager>("application/m3u8", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    std::string testUrl = "fd://1?offset=0&size=1024";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    EXPECT_TRUE(downloader);
    downloader->Close(true);
    testUrl = "fd://-1?offset=0&size=1024";
    downloader->Open(testUrl, httpHeader);
    downloader->Close(true);
    testUrl = "fd://1?offset=2048&size=1024";
    downloader->Open(testUrl, httpHeader);
    downloader->Close(true);
    testUrl = "fd://1?offset=512&size=1024";
    downloader->Open(testUrl, httpHeader);
    downloader->Close(true);
    downloader = nullptr;
}

int64_t GetFileSize(const string &fileName)
{
    int64_t fileSize = 0;
    if (!fileName.empty()) {
        struct stat fileStatus {};
        if (stat(fileName.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_READ_null, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader =
        std::make_shared<HlsSegmentManager>("application/m3u8", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    std::string path = "/data/test/media/test_cbr/720_1M/video_720_native.m3u8";
    int32_t fd = open(path.c_str(), O_RDONLY);
    int64_t size = GetFileSize(path);
    std::string testUrl = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(size);
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

HWTEST_F(HlsSegmentManagerUnitTest, GetSegmentOffset, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader =
        std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    downloader->GetSegmentOffset();
    downloader->GetHLSDiscontinuity();
    downloader->WaitForBufferingEnd();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720_5K.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    downloader->GetStartedStatus();
    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    downloader->GetSegmentOffset();
    downloader->GetHLSDiscontinuity();
    downloader->WaitForBufferingEnd();
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, StopBufferring, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = 
        std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    downloader->StopBufferring(true);
    downloader->StopBufferring(false);
    downloader->StopBufferring(true);
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720_5K.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    downloader->GetStartedStatus();
    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    downloader->StopBufferring(true);
    downloader->StopBufferring(false);
    downloader->StopBufferring(true);
    downloader->StopBufferring(false);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, TEST_READ_MAX_M3U8, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
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

HWTEST_F(HlsSegmentManagerUnitTest, TEST_READ_SelectBR, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader =
        std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
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

HWTEST_F(HlsSegmentManagerUnitTest, TEST_WRITE_RINGBUFFER_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->OnWriteCacheBuffer(0);
    downloader->OnWriteCacheBuffer(0);
    EXPECT_EQ(downloader->bufferedDuration_, 0);
    EXPECT_EQ(downloader->totalBits_, 0);
    EXPECT_EQ(downloader->lastWriteBit_, 0);

    downloader->OnWriteCacheBuffer(1);
    downloader->OnWriteCacheBuffer(1);
    EXPECT_EQ(downloader->bufferedDuration_, 16);
    EXPECT_EQ(downloader->totalBits_, 16);
    EXPECT_EQ(downloader->lastWriteBit_, 16);

    downloader->OnWriteCacheBuffer(1000);
    downloader->OnWriteCacheBuffer(1000);
    EXPECT_EQ(downloader->bufferedDuration_, 16016);
    EXPECT_EQ(downloader->totalBits_, 16016);
    EXPECT_EQ(downloader->lastWriteBit_, 16016);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, RISE_BUFFER_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->totalBufferSize_ = MAX_CACHE_BUFFER_SIZE_UT;
    downloader->RiseBufferSize();
    EXPECT_EQ(downloader->totalBufferSize_, MAX_CACHE_BUFFER_SIZE_UT);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, RISE_BUFFER_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->totalBufferSize_ = RING_BUFFER_SIZE;
    downloader->RiseBufferSize();
    EXPECT_EQ(downloader->totalBufferSize_, RING_BUFFER_SIZE + 200 * 1024);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, DOWN_BUFFER_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->totalBufferSize_ = 10 * 1024 * 1024;
    downloader->DownBufferSize();
    EXPECT_EQ(downloader->totalBufferSize_, 10 * 1024 * 1024 - 200 * 1024);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, DOWN_BUFFER_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->totalBufferSize_ = RING_BUFFER_SIZE;
    downloader->DownBufferSize();
    EXPECT_EQ(downloader->totalBufferSize_, RING_BUFFER_SIZE);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_PLAYBACK_INFO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
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

HWTEST_F(HlsSegmentManagerUnitTest, GET_PLAYBACK_INFO_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->totalDownloadDuringTime_ = 1000;
    downloader->totalBits_ = 1000;
    downloader->isDownloadFinish_ = true;
    std::shared_ptr<HlsSegmentManager::RecordData> recordBuff = std::make_shared<HlsSegmentManager::RecordData>();
    recordBuff->downloadRate = 1000;
    recordBuff->bufferDuring = 1;
    downloader->recordData_ = recordBuff;

    PlaybackInfo playbackInfo;
    downloader->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.serverIpAddress, "");
    EXPECT_EQ(playbackInfo.averageDownloadRate, 1000);
    EXPECT_EQ(playbackInfo.isDownloading, false);
    EXPECT_EQ(playbackInfo.downloadRate, 1000);
    EXPECT_EQ(playbackInfo.bufferDuration, 0);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, SET_INITIAL_BUFFERSIZE_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    PlayInfo playInfo;
    playInfo.url_ = testUrl;
    downloader->PutRequestIntoDownloader(playInfo);
    downloader->backPlayList_.push_back(playInfo);
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    EXPECT_EQ(downloader->SetInitialBufferSize(0, 50000), true);
    downloader->cacheMediaBuffer_ = nullptr;
    EXPECT_EQ(downloader->SetInitialBufferSize(0, 20000000), true);
}

HWTEST_F(HlsSegmentManagerUnitTest, SET_PLAY_STRATEGY_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->SetPlayStrategy(nullptr);
    EXPECT_EQ(downloader->waterlineForPlaying_, 0);
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1280;
    playStrategy->height = 720;
    playStrategy->bufferDurationForPlaying = 5;
    downloader->SetPlayStrategy(playStrategy);
    EXPECT_NE(downloader->waterlineForPlaying_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, NOTIFY_INIT_SUCCESS_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->bufferDurationForPlaying_ = 0;
    downloader->NotifyInitSuccess();
    EXPECT_EQ(downloader->waterlineForPlaying_, 0);
    downloader->bufferDurationForPlaying_ = 5;
    downloader->NotifyInitSuccess();
    EXPECT_EQ(downloader->waterlineForPlaying_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, IS_CACHED_INIT_SIZE_READY_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->SetCallback(sourceCallback);
    EXPECT_EQ(downloader->IsCachedInitSizeReady(-1), false);
    PlayInfo playInfo;
    downloader->backPlayList_.push_back(playInfo);
    EXPECT_EQ(downloader->backPlayList_.size(), 1);
    EXPECT_EQ(downloader->IsCachedInitSizeReady(10), true);
    downloader->tsStorageInfo_[0].second = true;
    downloader->tsStorageInfo_[1].second = true;
    downloader->backPlayList_.push_back(playInfo);
    EXPECT_EQ(downloader->IsCachedInitSizeReady(10), true);
    downloader->cacheMediaBuffer_ = nullptr;
    EXPECT_EQ(downloader->IsCachedInitSizeReady(10), false);
}

HWTEST_F(HlsSegmentManagerUnitTest, CACHE_BUFFER_FULL_LOOP_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    PlayInfo playInfo;
    downloader->backPlayList_.push_back(playInfo);
    downloader->initCacheSize_ = 100;
    downloader->isSeekingFlag = true;
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    auto bufferingCallback = [](HlsSegmentType segType, BufferingInfoType bufferingType) {};
    downloader->SetSegmentBufferingCallback(bufferingCallback);
    EXPECT_EQ(downloader->CacheBufferFullLoop(), true);
    downloader->isSeekingFlag = false;
    EXPECT_EQ(downloader->CacheBufferFullLoop(), false);
}

HWTEST_F(HlsSegmentManagerUnitTest, read, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    readDataInfo.isEos_ = true;

    downloader->canWrite_ = false;
    downloader->Read(buff, readDataInfo);

    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, CheckPlaylist, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(1 * 1000);
    downloader->isStopped = true;
    downloader->readOffset_ = 1;
    downloader->readTsIndex_ = BUFFER_SIZE + BUFFER_SIZE;
    downloader->isStopped = true;
    downloader->CheckPlaylist(buff, readDataInfo);

    downloader->isStopped = false;
    downloader->CheckPlaylist(buff, readDataInfo);

    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, CheckPlaylist_1, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    readDataInfo.isEos_ = true;

    downloader->readTsIndex_ = BUFFER_SIZE + BUFFER_SIZE;
    downloader->isStopped = false;
    downloader->CheckPlaylist(buff, readDataInfo);

    downloader->isStopped = true;
    downloader->CheckPlaylist(buff, readDataInfo);

    downloader->Read(buff, readDataInfo);

    downloader->isStopped = true;
    downloader->readOffset_ = 1;

    downloader->readTsIndex_ = BUFFER_SIZE + BUFFER_SIZE;
    downloader->isStopped = true;
    downloader->CheckPlaylist(buff, readDataInfo);

    downloader->isStopped = false;
    downloader->CheckPlaylist(buff, readDataInfo);

    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, ReadDelegate, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    readDataInfo.isEos_ = true;
    downloader->readTsIndex_ = BUFFER_SIZE - 1;

    downloader->ReadDelegate(buff, readDataInfo);
    downloader->Read(buff, readDataInfo);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, SaveCacheBufferDataNotblock, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    uint8_t * data = new uint8_t[10];
    uint32_t len = BUFFER_SIZE;
    uint32_t res = downloader->SaveCacheBufferDataNotblock(data, len);
    delete[] data;
    EXPECT_GE(res, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, SaveCacheBufferDataNotblock_1, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    uint8_t * data = new uint8_t[BUFFER_SIZE];
    uint32_t len = BUFFER_SIZE;
    downloader->isNeedResume_.store(true);
    uint32_t res = downloader->SaveCacheBufferDataNotblock(data, len);
    delete[] data;
    EXPECT_GE(res, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, SaveCacheBufferDataNotblock_3, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    unsigned char buff[BUFFER_SIZE];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = BUFFER_SIZE;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);
    uint8_t * data = new uint8_t[BUFFER_SIZE];
    uint32_t len = BUFFER_SIZE;
    downloader->isNeedResume_.store(true);
    uint32_t res = downloader->SaveCacheBufferDataNotblock(data, len);
    delete[] data;
    EXPECT_GE(res, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_STREAM_INFO_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::vector<StreamInfo> streams;
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::OK);
    downloader->isInterruptNeeded_ = true;
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::OK);
    downloader->isInterruptNeeded_ = false;
    std::string testUrl = TEST_URI_PATH + "test_hls/testXMap.m3u8";
    downloader->Open(testUrl, httpHeader);
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::OK);
    downloader->playlistDownloader_ = nullptr;
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::OK);
    std::shared_ptr<HlsPlayListDownloader> playlist = std::make_shared<HlsPlayListDownloader>();
    downloader->playlistDownloader_ = playlist;
    playlist->currentVariant_ = std::make_shared<M3U8VariantStream>("example_name", "example_uri", nullptr);
    playlist->master_ = std::make_shared<M3U8MasterPlaylist>("example_playlist", "example_uri", 0,
    std::map<std::string, std::string>(),
    [](DownloadStatus status, std::shared_ptr<Downloader>& dl, std::shared_ptr<DownloadRequest>& req) {});
    playlist->master_->isFmp4_ = false;
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::OK);
    playlist->currentVariant_ = nullptr;
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::OK);
    playlist->master_ = nullptr;
    EXPECT_EQ(downloader->GetStreamInfo(streams), Status::OK);
    downloader->playlistDownloader_ = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, IS_HLS_FMP4_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    EXPECT_EQ(downloader->IsHlsFmp4(), false);
    downloader->playlistDownloader_ = nullptr;
    EXPECT_EQ(downloader->IsHlsFmp4(), false);
}

HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_SEEK_READY_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->HandleSeekReady(1, 0);
    EXPECT_EQ(downloader->IsHlsFmp4(), false);
    std::string testUrl = TEST_URI_PATH + "test_hls/testXMap.m3u8";
    downloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams);
    downloader->HandleSeekReady(1, 0);
    EXPECT_EQ(downloader->IsHlsFmp4(), true);
    downloader->callback_.reset();
    downloader->HandleSeekReady(1, 0);
    EXPECT_EQ(downloader->IsHlsFmp4(), true);
    downloader->playlistDownloader_ = nullptr;
    downloader->HandleSeekReady(1, 0);
    EXPECT_EQ(downloader->IsHlsFmp4(), false);
}

HWTEST_F(HlsSegmentManagerUnitTest, REMOVE_FMP4_PADDING_DATA_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::string testUrl = TEST_URI_PATH + "test_hls/testXMap.m3u8";
    downloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams);
    uint8_t buffer[16] = {
    0x0d, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C
    };
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 1;
    readDataInfo.wantReadLength_ = 100;
    readDataInfo.realReadLength_ = 16;
    downloader->RemoveFmp4PaddingData(buffer, readDataInfo);
    EXPECT_EQ(readDataInfo.realReadLength_, 16);
    downloader->aesDecryptor_->keyLen_ = 1;
    downloader->RemoveFmp4PaddingData(buffer, readDataInfo);
    EXPECT_NE(readDataInfo.realReadLength_, 16);
    readDataInfo.realReadLength_ = 0;
    downloader->RemoveFmp4PaddingData(buffer, readDataInfo);
    EXPECT_EQ(readDataInfo.realReadLength_, 0);
    readDataInfo.realReadLength_ = 5;
    downloader->RemoveFmp4PaddingData(buffer, readDataInfo);
    EXPECT_EQ(readDataInfo.realReadLength_, 5);
}

HWTEST_F(HlsSegmentManagerUnitTest, READ_HEADER_DATA_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    unsigned char * buffer = new unsigned char[1 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 1;
    readDataInfo.wantReadLength_ = 1 * 1024 * 1024;
    readDataInfo.realReadLength_ = 16;
    EXPECT_EQ(downloader->IsHlsFmp4(), false);
    EXPECT_EQ(downloader->ReadHeaderData(buffer, readDataInfo), true);
    std::string testUrl = TEST_URI_PATH + "test_hls/testXMap.m3u8";
    downloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams);
    EXPECT_EQ(downloader->ReadHeaderData(buffer, readDataInfo), true);
    readDataInfo.streamId_ = 2;
    EXPECT_EQ(downloader->ReadHeaderData(buffer, readDataInfo), true);
    downloader->ReadCacheBuffer(buffer, readDataInfo);
    readDataInfo.streamId_ = 0;
    EXPECT_EQ(downloader->ReadHeaderData(buffer, readDataInfo), false);
    readDataInfo.streamId_ = 1;
    downloader->isNeedReadHeader_ = false;
    EXPECT_EQ(downloader->ReadHeaderData(buffer, readDataInfo), false);
    downloader->playlistDownloader_  = nullptr;
    EXPECT_EQ(downloader->ReadHeaderData(buffer, readDataInfo), false);
    delete[] buffer;
}

HWTEST_F(HlsSegmentManagerUnitTest, IS_PURE_BYTE_RANGE_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    EXPECT_EQ(downloader->IsPureByteRange(), false);
    std::string testUrl = TEST_URI_PATH + "test_hls/testByteRange.m3u8";
    downloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams);
    EXPECT_EQ(downloader->IsPureByteRange(), true);
    downloader->playlistDownloader_  = nullptr;
    EXPECT_EQ(downloader->IsPureByteRange(), false);
}

HWTEST_F(HlsSegmentManagerUnitTest, PUT_REQUEST_INTO_DOWNLOADER_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    PlayInfo playInfo;
    playInfo.url_ = TEST_URI_PATH + "test_hls/testByteRange.m3u8";
    playInfo.length_ = 1000;
    playInfo.offset_ = 0;
    playInfo.rangeUrl_ = TEST_URI_PATH + "test_hls/testByteRange.m3u8";
    downloader->fragmentDownloadStart[playInfo.url_] = true;
    downloader->writeTsIndex_ = 1;
    downloader->PutRequestIntoDownloader(playInfo);
    EXPECT_EQ(downloader->downloadRequest_, nullptr);
    downloader->writeTsIndex_ = 0;
    downloader->PutRequestIntoDownloader(playInfo);
    EXPECT_EQ(downloader->downloadRequest_, nullptr);
    downloader->fragmentDownloadStart[playInfo.url_] = false;
    downloader->PutRequestIntoDownloader(playInfo);
    EXPECT_NE(downloader->downloadRequest_, nullptr);
}

HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_FFMPEG_READ_BACK_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        false, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->curStreamId_ = 1;
    downloader->isNeedResetOffset_ = true;
    downloader->HandleFfmpegReadback(100);
    EXPECT_EQ(downloader->ffmpegOffset_, 100);
    downloader->curStreamId_ = 0;
    downloader->HandleFfmpegReadback(100);
    EXPECT_EQ(downloader->ffmpegOffset_, 100);
    downloader->curStreamId_ = 1;
    downloader->isNeedResetOffset_ = false;
    downloader->HandleFfmpegReadback(100);
    EXPECT_EQ(downloader->ffmpegOffset_, 100);
}

HWTEST_F(HlsSegmentManagerUnitTest, READ_DELEGATE_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::string testUrl = TEST_URI_PATH + "test_hls/testByteRange.m3u8";
    downloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams);
    OSAL::SleepFor(3 * 1000);
    downloader->cacheMediaBuffer_->Clear();
    downloader->writeTsIndex_ = 0;
    
    EXPECT_NE(downloader->GetDuration(), 0);
    EXPECT_EQ(downloader->CheckReadStatus(), true);
    EXPECT_EQ(downloader->GetBufferSize(), 0);
    unsigned char * buffer = new unsigned char[1 * 1024 * 1024];
    ReadDataInfo readDataInfo;
    downloader->isStopped = false;
    EXPECT_EQ(downloader->CheckPlaylist(buffer, readDataInfo), Status::ERROR_UNKNOWN);
    downloader->downloadErrorState_ = true;
    EXPECT_EQ(downloader->CheckBreakCondition(), true);
    readDataInfo.wantReadLength_ = 0;
    EXPECT_EQ(downloader->ReadDelegate(buffer, readDataInfo), Status::END_OF_STREAM);
    readDataInfo.wantReadLength_ = 4096;
    EXPECT_EQ(downloader->ReadDelegate(buffer, readDataInfo), Status::OK);
    EXPECT_EQ(downloader->IsHlsEnd(), false);
    EXPECT_EQ(downloader->writeTsIndex_, 0);

    downloader->seekTime_ = 100000000000;
    EXPECT_EQ(downloader->backPlayList_.size(), 2);
    downloader->readTsIndex_ = 1;
    downloader->tsStorageInfo_[downloader->readTsIndex_] = std::make_pair(0, true);
    EXPECT_EQ(downloader->ReadDelegate(buffer, readDataInfo), Status::END_OF_STREAM);
    EXPECT_EQ(downloader->IsHlsEnd(), true);

    downloader->isInterruptNeeded_ = true;
    EXPECT_EQ(downloader->ReadDelegate(buffer, readDataInfo), Status::END_OF_STREAM);
    downloader->cacheMediaBuffer_  = nullptr;
    EXPECT_EQ(downloader->ReadDelegate(buffer, readDataInfo), Status::END_OF_STREAM);
}

HWTEST_F(HlsSegmentManagerUnitTest, PREPARE_TO_SEEK_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->PrepareToSeek();
    EXPECT_EQ(downloader->playlistDownloader_->IsParseAndNotifyFinished(), false);
    downloader->isInterruptNeeded_ = true;
    downloader->PrepareToSeek();
    EXPECT_EQ(downloader->playlistDownloader_->IsParseAndNotifyFinished(), false);
    downloader->isInterruptNeeded_ = false;
    std::string testUrl = TEST_URI_PATH + "test_hls/testXMap.m3u8";
    downloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams);
    downloader->PrepareToSeek();
    EXPECT_EQ(downloader->playlistDownloader_->IsParseAndNotifyFinished(), true);
    EXPECT_EQ(downloader->IsHlsFmp4(), true);
}

HWTEST_F(HlsSegmentManagerUnitTest, SEEK_TO_TIME_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_EQ(downloader->GetBufferSize(), 0);

    std::string testUrl = TEST_URI_PATH + "test_hls/testXMap.m3u8";
    downloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams);
    downloader->PrepareToSeek();
    downloader->SeekToTime(10000, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_EQ(downloader->isNeedReadHeader_, true);
    downloader->SeekToTime(50000000000, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_EQ(downloader->readTsIndex_, 0);
    downloader->backPlayList_.clear();
    downloader->SeekToTime(50000000000, SeekMode::SEEK_NEXT_SYNC);
    EXPECT_EQ(downloader->readTsIndex_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, ON_PLAYLIST_CHANGED_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        false, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::string testUrl = TEST_URI_PATH + "test_hls/testXMap.m3u8";
    downloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams);
    downloader->PrepareToSeek();

    std::vector<PlayInfo> playList;
    PlayInfo palyInfo;
    playList.push_back(palyInfo);
    downloader->isSelectingBitrate_ = true;
    downloader->writeTsIndex_ = 10;
    downloader->OnPlayListChanged(playList);
    EXPECT_EQ(downloader->isSelectingBitrate_, true);

    downloader->isInterruptNeeded_ = true;
    downloader->OnPlayListChanged(playList);
    EXPECT_EQ(downloader->isInterruptNeeded_, true);
}

HWTEST_F(HlsSegmentManagerUnitTest, UPDATE_MASTER_PLAYLIST_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::string testUrl = TEST_URI_PATH + "test_hls/testMutiStream.m3u8";
    downloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams);
    EXPECT_EQ(streams.size(), 2);
}

HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_DOWNLOADER_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    uint8_t data[1] = {1};
    EXPECT_EQ(downloader->playlistDownloader_->SaveData(data, 0, true), 0);
    EXPECT_EQ(downloader->playlistDownloader_->SaveData(nullptr, 0, true), 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_DOWNLOADER_002, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::shared_ptr<Downloader> downloader1 = std::make_shared<Downloader>("hlsPlayList");
    downloader1->Init();
    std::shared_ptr<DownloadRequest> request = std::make_shared<DownloadRequest>("", nullptr, nullptr,  false);
    downloader->playlistDownloader_->OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, downloader1, request);
    request->clientError_ = 52;
    downloader->playlistDownloader_->OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, downloader1, request);
    request->serverError_ = 403;
    downloader->playlistDownloader_->OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, downloader1, request);
    request->serverError_ = 0;
    downloader->playlistDownloader_->OnDownloadStatus(DownloadStatus::PARTTAL_DOWNLOAD, downloader1, request);
    EXPECT_EQ(request->clientError_, 52);
}

HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_DOWNLOADER_003, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    downloader->playlistDownloader_->downloader_ = nullptr;
    downloader->playlistDownloader_->GetHttpHeader();
    EXPECT_EQ(downloader->playlistDownloader_->IsLive(), false);

    downloader->playlistDownloader_->StopBufferring(true);
    downloader->playlistDownloader_->Cancel();
    downloader->playlistDownloader_->Start();
    downloader->playlistDownloader_->Stop();
    downloader->playlistDownloader_->Pause();
    downloader->playlistDownloader_->SetInterruptState(true);
    downloader->playlistDownloader_->SetAppUid(1);
    EXPECT_EQ(downloader->playlistDownloader_->isAppBackground_, false);
}

HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_DOWNLOADER_004, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHlsLive.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    downloader->GetSeekable();
    downloader->playlistDownloader_->Init();
    downloader->playlistDownloader_->Resume();
    downloader->playlistDownloader_->Pause(false);
    downloader->playlistDownloader_->Pause(true);
    EXPECT_EQ(downloader->playlistDownloader_->IsLive(), true);
    EXPECT_NE(downloader->playlistDownloader_->updateTask_, nullptr);
}

HWTEST_F(HlsSegmentManagerUnitTest, STOP_BUFFERING_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720_small.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    OSAL::SleepFor(2 * 1000);
    downloader->StopBufferring(true);
    EXPECT_EQ(downloader->isInterrupt_, true);
    downloader->StopBufferring(false);
    EXPECT_EQ(downloader->isInterrupt_, false);
}

HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_DOWNLOADER_005, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        false, header_);
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                            std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->SetCallback(sourceCallback);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHlsLive.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    downloader->playlistDownloader_->Init();
    downloader->playlistDownloader_->Resume();
    downloader->playlistDownloader_->Pause(false);
    downloader->playlistDownloader_->Pause(true);
    EXPECT_EQ(downloader->playlistDownloader_->IsLive(), true);
    EXPECT_NE(downloader->playlistDownloader_->updateTask_, nullptr);

    downloader->playlistDownloader_->ReOpen();
    downloader->GetSeekable();
    EXPECT_EQ(downloader->playlistDownloader_->IsLive(), true);
    EXPECT_NE(downloader->playlistDownloader_->updateTask_, nullptr);
    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, SAVE_DATA_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    EXPECT_EQ(downloader->SaveData(nullptr, 0, true), 0);
    downloader = nullptr;
}
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_ENCRYPT_DATA_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    uint8_t data[1] = {1};
    EXPECT_EQ(downloader->SaveEncryptData(data, 1, true), 1);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, SEEK_TO_TIME_002, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    bool result = downloader->SeekToTime(1000000000, SeekMode::SEEK_NEXT_SYNC);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_TRUE(result);
}

HWTEST_F(HlsSegmentManagerUnitTest, SEEK_TO_TIME_003, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720.m3u8";
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    downloader->GetSeekable();
    bool result = downloader->SeekToTime(1, SeekMode::SEEK_PREVIOUS_SYNC);
    downloader->Close(true);
    downloader = nullptr;
    EXPECT_TRUE(result);
}
}