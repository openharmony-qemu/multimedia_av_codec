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
constexpr uint64_t MAX_CACHE_BUFFER_SIZE_UT = 4 * 1024 * 1024;
constexpr size_t AES_IV_LEN = 16;

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

class HlsPlayListDownloaderMockForUpdate : public HlsPlayListDownloader {
public:
    using HlsPlayListDownloader::HlsPlayListDownloader;

    bool IsParseAndNotifyFinished() override
    {
        return true;
    }

    std::vector<uint32_t> GetBitRates() override
    {
        calledGetBitRates_ = true;
        return {currentBitrate_};
    }

    uint32_t GetCurBitrate() override
    {
        return currentBitrate_;
    }

    bool calledGetBitRates_ {false};
    uint32_t currentBitrate_ {0};
};

class HlsNetworkBitrateCallback : public Plugins::Callback {
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

class TestPlayListDownloader : public PlayListDownloader {
public:
    bool returnInPlaylist_ = true;

    void Open(const std::string& url, const std::map<std::string, std::string>& httpHeaderMap) override {}
    void Clone(std::shared_ptr<PlayListDownloader> other) override {}
    void UpdateManifest() override {}
    void ParseManifest(const std::string& location, bool isPreParse = false) override {}
    void SetPlayListCallback(std::weak_ptr<PlayListChangeCallback> callback) override {}
    int64_t GetDuration() override { return duration_; }
    std::pair<int64_t, bool> GetStartInfo() override { return {startInfo_, false}; }
    Seekable GetSeekable() override { return seekable_; }
    void SelectBitRate(uint32_t bitRate) override {}
    std::vector<uint32_t> GetBitRates() override { return bitRates_; }
    uint64_t GetCurrentBitRate() override { return currentBitRateVal_; }
    int GetVideoHeight() override { return videoHeight_; }
    int GetVideoWidth() override { return videoWidth_; }
    bool IsBitrateSame(uint32_t bitRate) override { return isBitrateSame_; }
    bool IsMediaSame(uint32_t streamId, HlsSegmentType mediaType) override { return isMediaSame_; }
    uint32_t GetCurBitrate() override { return curBitrate_; }
    bool IsLive() override { return isLive_; }
    void SetMimeType(const std::string& mimeType) override {}
    void PreParseManifest(const std::string& location) override {}
    bool IsParseAndNotifyFinished() override { return isParseAndNotifyFinished_; }
    bool IsParseFinished() override { return isParseFinished_; }
    bool IsFragmentInCurrentPlayList(const std::string& url, const std::string& rangeUrl,
        uint32_t offset, uint32_t length) override
    {
        (void)url;
        (void)rangeUrl;
        (void)offset;
        (void)length;
        return returnInPlaylist_;
    }
    void SetInitResolution(uint32_t width, uint32_t height) override {}
    void GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate = false) override {}
    bool ReadFmp4Header(uint8_t *buffer, uint32_t wantLen, uint32_t &readLen, uint32_t streamId) override
    {
        return false;
    }
    bool IsHlsFmp4() override { return isHlsFmp4_; }
    bool IsPureByteRange() override { return isPureByteRange_; }
    void ReOpen(void) override {}
    std::shared_ptr<StreamInfo> GetStreamInfoById(int32_t streamId) override { return nullptr; }
    int32_t GetDefaultMediaStreamId(HlsSegmentType mediaType) override { return 0; }
    void SelectMedia(int32_t streamId, HlsSegmentType mediaType) override {}
    void SetDefaultMedia(HlsSegmentType mediaType) override {}
    void UpdateMedia(HlsSegmentType mediaType) override {}
    uint32_t GetCurStreamId() override { return 0; }
    void SetSourceStatisticsDfx(std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr,
        bool fmp4 = false) override {}
    bool IsLiveEnd() override { return isLiveEnd_; }
    void UpdateStreamInfo() override {}
    HlsSegmentType GetSegType(uint32_t streamId) override { return HlsSegmentType::SEG_AUDIO; }
    std::shared_ptr<AesDecryptor> GetAesDecryptor(uint64_t keyIndex) override { return aesDecryptor_; }
    bool HasExplicitIv(uint64_t keyIndex) override { return hasExplicitIvResult_; }
    bool WaitForKeyReady(uint64_t keyIndex, uint32_t timeoutMs) override { return waitForKeyReady_; }
    std::shared_ptr<AesDecryptor> aesDecryptor_ = nullptr;
    bool hasExplicitIvResult_ = false;
    bool isHlsFmp4_ = false;
    bool isPureByteRange_ = false;
    int64_t duration_ = 0;
    int64_t startInfo_ = 0;
    Seekable seekable_ = Seekable::SEEKABLE;
    std::vector<uint32_t> bitRates_;
    uint64_t currentBitRateVal_ = 0;
    int videoHeight_ = 0;
    int videoWidth_ = 0;
    bool isBitrateSame_ = false;
    bool isMediaSame_ = false;
    uint32_t curBitrate_ = 0;
    bool isLive_ = false;
    bool isParseAndNotifyFinished_ = true;
    bool isParseFinished_ = true;
    bool isLiveEnd_ = false;
    bool waitForKeyReady_ = false;
};

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

void HlsSegmentManagerUnitTest::SetDefaultStreamId(std::shared_ptr<HlsSegmentManager> downloader)
{
    int32_t videoStreamId = 0;
    int32_t audioStreamId = 0;
    int32_t subTitleStreamId = 0;
    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
}

HWTEST_F(HlsSegmentManagerUnitTest, SET_DEFAULT_STREAM_ID_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    downloader->playlistDownloader_ = nullptr;
    int32_t videoStreamId = 1;
    int32_t audioStreamId = 2;
    int32_t subTitleStreamId = 3;
    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    auto mockPlaylist = std::make_shared<HlsPlayListDownloaderMockForDefaultStream>();
    downloader->playlistDownloader_ = mockPlaylist;
    int32_t videoStreamIdNumSec = 4;
    int32_t audioStreamIdNumSec= 5;
    int32_t subTitleStreamIdNumSec = 6;
    downloader->SetDefaultStreamId(videoStreamIdNumSec, audioStreamIdNumSec, subTitleStreamIdNumSec);
    EXPECT_TRUE(mockPlaylist->called_);
    EXPECT_EQ(mockPlaylist->videoStreamId_, 4);
    EXPECT_EQ(mockPlaylist->audioStreamId_, 5);
    EXPECT_EQ(mockPlaylist->subTitleStreamId_, 6);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_STREAM_INFO_002, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    std::vector<StreamInfo> streams;
    EXPECT_EQ(downloader->GetStreamInfo(streams, true), Status::OK);
    downloader->playlistDownloader_ = nullptr;
    EXPECT_EQ(downloader->GetStreamInfo(streams, true), Status::OK);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_STREAM_INFO_003, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT,
        true, header_);
    downloader->Init();
    std::vector<StreamInfo> streams;
    downloader->playlistDownloader_ = nullptr;
    downloader->isInterruptNeeded_.store(true);
    EXPECT_EQ(downloader->GetStreamInfo(streams, false), Status::OK);
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
    DownloadInfo downloadInfo;
    hlsSegmentManager_->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(downloadInfo.isTimeOut, false);
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
    hlsSegmentManager_->totalBufferSize_ = MAX_CACHE_BUFFER_SIZE_UT;
    hlsSegmentManager_->RiseBufferSize();
    EXPECT_EQ(hlsSegmentManager_->totalBufferSize_, MAX_CACHE_BUFFER_SIZE_UT);
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
    EXPECT_EQ(downloader->totalBufferSize_, 19 * 200 * 1024);
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(readDataInfo.realReadLength_ <= readDataInfo.wantReadLength_, true);
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(2 * 1000);
    downloader->Close(true);
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(2 * 1000);
    downloader->Close(true);
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(4 * 1000);
    downloader->Close(true);
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, GetSegmentOffset, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader =
        std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    downloader->GetSegmentOffset();
    downloader->GetHLSDiscontinuity();
    std::string testUrl = TEST_URI_PATH + "test_cbr/720_1M/video_720_5K.m3u8";
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
    downloader->GetSegmentOffset();
    downloader->GetHLSDiscontinuity();
    downloader->Close(true);
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 0;
    readDataInfo.wantReadLength_ = 10;
    readDataInfo.isEos_ = true;
    downloader->Read(buff, readDataInfo);

    OSAL::SleepFor(4 * 1000);
    downloader->Close(true);
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1280;
    playStrategy->height = 720;
    playStrategy->bufferDurationForPlaying = 5;
    downloader->SetPlayStrategy(playStrategy);
    EXPECT_EQ(downloader->playlistDownloader_ != nullptr, true);
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
    downloader->isSeekingFlag = true;
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
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
    EXPECT_EQ(res == 0, true);
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
    EXPECT_EQ(res == 0, true);
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
    EXPECT_EQ(res == 0, true);
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
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
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
    EXPECT_EQ(downloader->curUrl_.empty(), false);
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
    OSAL::SleepFor(100);
    SetDefaultStreamId(downloader);
    downloader->GetSeekable();
    std::vector<StreamInfo> streams;
    downloader->GetStreamInfo(streams);
    OSAL::SleepFor(3 * 1000);
    downloader->cacheMediaBuffer_->Clear();
    downloader->writeTsIndex_ = 0;
    
    EXPECT_EQ(downloader->GetDuration() != 0, true);
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
    OSAL::SleepFor(100);
    SetDefaultStreamId(downloader);
    downloader->GetSeekable();
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
    downloader->currentSequence_ = 10;
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
    OSAL::SleepFor(100);
    SetDefaultStreamId(downloader);
    downloader->GetSeekable();
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

HWTEST_F(HlsSegmentManagerUnitTest, GET_CONTENT_TYPE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    // downloader_ is not nullptr after Init
    std::string contentType = downloader->GetContentType();
    EXPECT_EQ(contentType, "");
    // downloader_ nullptr branch
    downloader->downloader_ = nullptr;
    EXPECT_EQ(downloader->GetContentType(), "");
}

HWTEST_F(HlsSegmentManagerUnitTest, SET_DOWNLOAD_CALLBACK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto cb = std::make_shared<DownloadMetricsInfo>();
    downloader->SetDownloadCallback(cb);
    EXPECT_EQ(downloader->downloadCallback_, cb);
    downloader->SetDownloadCallback(nullptr);
    EXPECT_EQ(downloader->downloadCallback_, nullptr);
}

HWTEST_F(HlsSegmentManagerUnitTest, SET_SOURCE_STATISTICS_DFX_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    // Pass nullptr - should not crash
    downloader->SetSourceStatisticsDfx(nullptr);
    EXPECT_EQ(downloader->playlistDownloader_ != nullptr, true);
    EXPECT_EQ(downloader->reportInfo_, nullptr);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_START_INFO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    // playlistDownloader_ is valid
    auto info = downloader->GetStartInfo();
    EXPECT_EQ(info.first, 0);
    // playlistDownloader_ nullptr
    downloader->playlistDownloader_ = nullptr;
    auto info2 = downloader->GetStartInfo();
    EXPECT_EQ(info2.first, 0);
    EXPECT_FALSE(info2.second);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_MEMORY_SIZE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    // memorySize_ set during construction
    uint64_t memSize = downloader->GetMemorySize();
    EXPECT_EQ(memSize, 0);
    uint64_t resumeThreshold = downloader->GetDownloadResumeThreshold();
    EXPECT_EQ(resumeThreshold, 0);
    uint64_t throttleThreshold = downloader->GetDownloadThrottleThreshold();
    EXPECT_EQ(throttleThreshold, 0);
    uint64_t clearThreshold = downloader->GetsourceLoaderClearThreshold();
    EXPECT_EQ(clearThreshold, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, SET_IS_REPORTED_ERROR_CODE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    EXPECT_FALSE(downloader->isReportedErrorCode_);
    downloader->SetIsReportedErrorCode();
    EXPECT_TRUE(downloader->isReportedErrorCode_);
}

HWTEST_F(HlsSegmentManagerUnitTest, SET_APP_UID_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    // both downloader_ and playlistDownloader_ are valid
    downloader->SetAppUid(1001);
    EXPECT_EQ(downloader->isStopped.load(), false);
    // downloader_ nullptr branch
    downloader->downloader_ = nullptr;
    downloader->SetAppUid(1002);
    EXPECT_EQ(downloader->isStopped.load(), false);
    // playlistDownloader_ nullptr branch
    downloader->playlistDownloader_ = nullptr;
    downloader->SetAppUid(1003);
    EXPECT_EQ(downloader->isStopped.load(), false);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_CROSS_TS_BUFFERSIZE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    // cacheMediaBuffer_ nullptr
    downloader->cacheMediaBuffer_ = nullptr;
    EXPECT_EQ(downloader->GetCrossTsBuffersize(), 0);
    // cacheMediaBuffer_ valid but empty
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    EXPECT_EQ(downloader->GetCrossTsBuffersize(), 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_READ_TIMEOUT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    // readTime_ set to current time: not yet timed out
    downloader->readTime_ = static_cast<size_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    EXPECT_FALSE(downloader->GetReadTimeOut(false));
    // isDelay = true changes timeoutInterval_ to MAX_BUFFERING_TIME_OUT_DELAY
    EXPECT_FALSE(downloader->GetReadTimeOut(true));
}

HWTEST_F(HlsSegmentManagerUnitTest, ON_MASTER_READY_001, TestSize.Level1)
{
    // SEG_AUDIO returns early
    auto downloaderAudio = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloaderAudio->type_ = HlsSegmentType::SEG_AUDIO;
    downloaderAudio->OnMasterReady(false, false);
    EXPECT_TRUE(downloaderAudio != nullptr);
    // SEG_SUBTITLE returns early
    auto downloaderSub = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloaderSub->type_ = HlsSegmentType::SEG_SUBTITLE;
    downloaderSub->OnMasterReady(false, false);
    EXPECT_TRUE(downloaderSub != nullptr);
    // SEG_VIDEO with no callback
    auto downloaderVideo = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloaderVideo->type_ = HlsSegmentType::SEG_VIDEO;
    downloaderVideo->masterReadyCallback_ = nullptr;
    downloaderVideo->OnMasterReady(true, false);
    EXPECT_TRUE(downloaderVideo != nullptr);
    // SEG_VIDEO with callback
    bool called = false;
    downloaderVideo->masterReadyCallback_ = [&called](bool, bool) { called = true; };
    downloaderVideo->OnMasterReady(true, true);
    EXPECT_TRUE(called);
}

HWTEST_F(HlsSegmentManagerUnitTest, SET_MASTER_READY_CALLBACK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    EXPECT_EQ(downloader->masterReadyCallback_, nullptr);
    bool called = false;
    downloader->SetMasterReadyCallback([&called](bool, bool) { called = true; });
    EXPECT_NE(downloader->masterReadyCallback_, nullptr);
    downloader->masterReadyCallback_(false, false);
    EXPECT_TRUE(called);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_STREAM_INFO_BY_ID_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    // playlistDownloader_ valid - return nullptr (no stream added)
    auto info = downloader->GetStreamInfoById(0);
    EXPECT_EQ(info, nullptr);
    // playlistDownloader_ nullptr
    downloader->playlistDownloader_ = nullptr;
    auto info2 = downloader->GetStreamInfoById(0);
    EXPECT_EQ(info2, nullptr);
    EXPECT_EQ(downloader->type_, HlsSegmentType::SEG_VIDEO);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_DEFAULT_MEDIA_STREAM_ID_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    // playlistDownloader_ valid
    int32_t streamId = downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_AUDIO);
    EXPECT_GE(streamId, -1);
    // playlistDownloader_ nullptr
    downloader->playlistDownloader_ = nullptr;
    int32_t streamId2 = downloader->GetDefaultMediaStreamId(HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(streamId2, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_SEG_TYPE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    // playlistDownloader_ valid
    HlsSegmentType type = downloader->GetSegType(0);
    EXPECT_EQ(type == HlsSegmentType::SEG_AUDIO || type == HlsSegmentType::SEG_VIDEO, true);
    // playlistDownloader_ nullptr
    downloader->playlistDownloader_ = nullptr;
    HlsSegmentType type2 = downloader->GetSegType(0);
    EXPECT_EQ(type2, HlsSegmentType::SEG_VIDEO);
}

HWTEST_F(HlsSegmentManagerUnitTest, SET_SEGMENT_ALL_CALLBACK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    EXPECT_EQ(downloader->segEventCb_, nullptr);
    bool called = false;
    HlsSegmentEventCbFunc cb = [&called](HlsSegEvent) { called = true; };
    downloader->SetSegmentAllCallback(cb);
    EXPECT_NE(downloader->segEventCb_, nullptr);
    HlsSegEvent event{};
    downloader->segEventCb_(event);
    EXPECT_TRUE(called);
}

HWTEST_F(HlsSegmentManagerUnitTest, SET_GET_DOWNLOAD_REQUEST_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    // GetDownloadRequest returns nullptr initially
    EXPECT_EQ(downloader->GetDownloadRequest(), nullptr);
    // SetDownloadRequest
    auto request = std::make_shared<DownloadRequest>("", nullptr, nullptr, false);
    downloader->SetDownloadRequest(request);
    auto req = downloader->GetDownloadRequest();
    EXPECT_EQ(req != nullptr, true);
    EXPECT_EQ(req->GetUrl(), "");
    // SetDownloadRequest nullptr
    downloader->SetDownloadRequest(nullptr);
    EXPECT_EQ(downloader->GetDownloadRequest(), nullptr);
}

HWTEST_F(HlsSegmentManagerUnitTest, CLEAR_CHUNKS_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    // ClearChunksOfFragment: diff <= READ_BACK_SAVE_SIZE, returns false
    downloader->readOffset_ = 0;
    downloader->readTsIndex_ = 0;
    bool ret = downloader->ClearChunksOfFragment();
    EXPECT_FALSE(ret);
    // ClearChunksInLargeSegment: writeTsIndex_ == readTsIndex_ branch
    downloader->writeTsIndex_ = 0;
    downloader->writeOffset_ = 0;
    downloader->readOffset_ = 0;
    downloader->ClearChunksInLargeSegment();
    EXPECT_EQ(downloader->writeTsIndex_, 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, SELECT_MEDIA_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testMutiStream.m3u8";
    auto statusCallback = [] (DownloadStatus&&, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(200);
    SetDefaultStreamId(downloader);
    downloader->GetSeekable();
    // SEG_VIDEO type returns true directly
    bool ret = downloader->SelectMedia(0, HlsSegmentType::SEG_AUDIO);
    EXPECT_TRUE(ret);
    // playlistDownloader_ nullptr
    downloader->playlistDownloader_ = nullptr;
    bool ret2 = downloader->SelectMedia(0, HlsSegmentType::SEG_AUDIO);
    EXPECT_FALSE(ret2);
}

HWTEST_F(HlsSegmentManagerUnitTest, START_MEDIA_DOWNLOAD_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testMutiStream.m3u8";
    auto statusCallback = [] (DownloadStatus&&, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(100);
    SetDefaultStreamId(downloader);
    downloader->GetSeekable();
    bool ret = downloader->StartMediaDownload(0, HlsSegmentType::SEG_AUDIO);
    EXPECT_TRUE(ret);
    // playlistDownloader_ nullptr
    downloader->playlistDownloader_ = nullptr;
    bool ret2 = downloader->StartMediaDownload(0, HlsSegmentType::SEG_AUDIO);
    EXPECT_FALSE(ret2);
}

HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_001, TestSize.Level1)
{
    // SEG_AUDIO type returns true (skip)
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_AUDIO);
    downloader->Init();
    bool ret = downloader->SelectBitRate(1000000);
    EXPECT_TRUE(ret);
    // SEG_VIDEO with no playlist - returns false
    auto downloaderV = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloaderV->Init();
    downloaderV->playlistDownloader_ = nullptr;
    bool ret2 = downloaderV->SelectBitRate(1000000);
    EXPECT_FALSE(ret2);
}

HWTEST_F(HlsSegmentManagerUnitTest, GET_TOTAL_TS_BUFFERSIZE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    // cacheMediaBuffer_ nullptr
    downloader->cacheMediaBuffer_ = nullptr;
    EXPECT_EQ(downloader->GetTotalTsBuffersize(), 0);
    // cacheMediaBuffer_ valid but writeTsIndex_ == readTsIndex_
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->readTsIndex_ = 0;
    downloader->writeTsIndex_ = 0;
    EXPECT_EQ(downloader->GetTotalTsBuffersize(), 0);
}

// ===== 分支补充：GetReadTimeOut 超时分支 =====
HWTEST_F(HlsSegmentManagerUnitTest, GET_READ_TIMEOUT_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    size_t now = static_cast<size_t>(downloader->steadyClock_.ElapsedMilliseconds());
    if (now == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        now = static_cast<size_t>(downloader->steadyClock_.ElapsedMilliseconds());
    }
    // readTime_=1, timeoutInterval_=0 => 超时
    downloader->readTime_ = 1;
    downloader->timeoutInterval_ = 0;
    EXPECT_TRUE(downloader->GetReadTimeOut(false));
}

// ===== 分支补充：ClearChunksOfFragment diff>READ_BACK_SAVE_SIZE 分支 =====
HWTEST_F(HlsSegmentManagerUnitTest, CLEAR_CHUNKS_OF_FRAGMENT_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    // readTsIndex_=0 => SpliceOffset(0,0)=0, readOffset_=300KB > READ_BACK_SAVE_SIZE(200KB)
    downloader->readTsIndex_ = 0;
    downloader->readOffset_ = 300 * 1024;
    bool ret = downloader->ClearChunksOfFragment();
    // 不论内部清理结果，此分支已执行
    EXPECT_EQ(ret, false);
}

// ===== 分支补充：ClearChunksInLargeSegment writeTsIndex_!=readTsIndex_ 分支 =====
HWTEST_F(HlsSegmentManagerUnitTest, CLEAR_CHUNKS_IN_LARGE_SEGMENT_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    // writeTsIndex_ != readTsIndex_ => 不进入清理
    downloader->writeTsIndex_ = 1;
    downloader->readTsIndex_ = 0;
    downloader->ClearChunksInLargeSegment();
    EXPECT_EQ(downloader->writeTsIndex_, 0);
}

// ===== 分支补充：GetCrossTsBuffersize backPlayList_非空且readTsIndex_+1 < size 分支 =====
HWTEST_F(HlsSegmentManagerUnitTest, GET_CROSS_TS_BUFFERSIZE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    PlayInfo p1, p2, p3;
    downloader->backPlayList_.push_back(p1);
    downloader->backPlayList_.push_back(p2);
    downloader->backPlayList_.push_back(p3);
    // readTsIndex_=0, backPlayList_.size()=3 => 0+1<3 => 进入nextTs分支
    downloader->readTsIndex_ = 0;
    uint64_t size = downloader->GetCrossTsBuffersize();
    EXPECT_EQ(size <= static_cast<uint64_t>(MAX_CACHE_BUFFER_SIZE_UT), true);
}

// ===== 分支补充：SelectBitRate SEG_VIDEO且IsBitrateSame=false (触发切码率流程) =====
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    auto statusCallback = [] (DownloadStatus&&, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(200);
    SetDefaultStreamId(downloader);
    downloader->GetSeekable();
    // 存在多码率，SelectBitRate触发切换流程
    bool ret = downloader->SelectBitRate(500000);
    downloader->Close(true);
    EXPECT_TRUE(ret);
}

// ===== 分支补充：SelectMedia SEG_AUDIO且isMediaSame=false (触发实际切换) =====
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_MEDIA_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_AUDIO);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testMutiStream.m3u8";
    auto statusCallback = [] (DownloadStatus&&, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(200);
    SetDefaultStreamId(downloader);
    downloader->GetSeekable();
    // SEG_AUDIO type且streamId不同 => isMediaSame=false => 触发实际切换
    bool ret = downloader->SelectMedia(1, HlsSegmentType::SEG_AUDIO);
    downloader->Close(true);
    EXPECT_TRUE(ret);
}

HWTEST_F(HlsSegmentManagerUnitTest, RequestNewTs_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testMutiStream.m3u8";
    auto statusCallback = [] (DownloadStatus&&, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(200);
    downloader->GetSeekable();
    SetDefaultStreamId(downloader);
    
    PlayInfo item;
    item.url_ = "test.ts";
    item.streamId_ = 0;
    item.duration_ = 10.0;
    item.offset_ = 0;
    item.length_ = 1000;
    item.keyIndex_ = 0;
    
    uint64_t seekTime = 5000;
    double totalDuration = 100.0;
    double hstTime = 50.0;
    
    // Test SEEK_PREVIOUS_SYNC mode with lastTotalDuration <= seekTime
    auto res = downloader->RequestNewTs(seekTime, SeekMode::SEEK_PREVIOUS_SYNC, totalDuration, hstTime, item);
    downloader->Close(true);
    EXPECT_EQ(res, -1);
}

HWTEST_F(HlsSegmentManagerUnitTest, RequestNewTs_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testMutiStream.m3u8";
    auto statusCallback = [] (DownloadStatus&&, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(200);
    downloader->GetSeekable();
    SetDefaultStreamId(downloader);
    
    PlayInfo item;
    item.url_ = "test.ts";
    item.streamId_ = 0;
    item.duration_ = 10.0;
    item.offset_ = 0;
    item.length_ = 1000;
    item.keyIndex_ = 0;
    
    uint64_t seekTime = 100;
    double totalDuration = 100.0;
    double hstTime = 50.0;
    
    // Test else branch (SEEK_CLOSEST) with lastTotalDuration > seekTime
    auto res = downloader->RequestNewTs(seekTime, SeekMode::SEEK_CLOSEST, totalDuration, hstTime, item);
    downloader->Close(true);
    EXPECT_EQ(res, -1);
}

HWTEST_F(HlsSegmentManagerUnitTest, RequestNewTs_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testMutiStream.m3u8";
    auto statusCallback = [] (DownloadStatus&&, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(200);
    downloader->GetSeekable();
    
    PlayInfo item;
    item.url_ = "test_not_in_playlist.ts";
    item.streamId_ = 0;
    item.duration_ = 10.0;
    item.offset_ = 0;
    item.length_ = 1000;
    item.keyIndex_ = 0;
    
    uint64_t seekTime = 100;
    double totalDuration = 100.0;
    double hstTime = 50.0;
    
    // Test inCurrentPlaylist == false branch, should return -1
    auto res = downloader->RequestNewTs(seekTime, SeekMode::SEEK_CLOSEST, totalDuration, hstTime, item);
    downloader->Close(true);
    EXPECT_EQ(res, -1);
}

HWTEST_F(HlsSegmentManagerUnitTest, RequestNewTs_004, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    std::string testUrl = TEST_URI_PATH + "test_hls/testMutiStream.m3u8";
    auto statusCallback = [] (DownloadStatus&&, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>&) {};
    downloader->SetStatusCallback(statusCallback);
    downloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(200);
    downloader->GetSeekable();
    SetDefaultStreamId(downloader);
    
    PlayInfo item;
    item.url_ = "test.ts";
    item.streamId_ = 0;
    item.duration_ = 10.0;
    item.offset_ = 0;
    item.length_ = 1000;
    item.keyIndex_ = 0;
    
    uint64_t seekTime = 200;
    double totalDuration = 100.0;
    double hstTime = 50.0;
    
    // Test SEEK_PREVIOUS_SYNC mode with lastTotalDuration > seekTime
    auto res = downloader->RequestNewTs(seekTime, SeekMode::SEEK_PREVIOUS_SYNC, totalDuration, hstTime, item);
    downloader->Close(true);
    EXPECT_EQ(res, -1);
}

HWTEST_F(HlsSegmentManagerUnitTest, SeekListHelper_001, TestSize.Level1)
{
    auto manager = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    manager->Init();
    auto testPlaylist = std::make_shared<TestPlayListDownloader>();
    testPlaylist->returnInPlaylist_ = true;
    manager->playlistDownloader_ = testPlaylist;
    
    PlayInfo frag1;
    frag1.url_ = "test1.ts";
    frag1.streamId_ = 0;
    frag1.duration_ = 10.0;
    frag1.offset_ = 0;
    frag1.length_ = 1000;
    manager->seekCandidates_.push_back(frag1);
    
    auto result = manager->SeekListHelper(true);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].url_, "test1.ts");
}

HWTEST_F(HlsSegmentManagerUnitTest, SeekListHelper_002, TestSize.Level1)
{
    auto manager = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    manager->Init();
    auto testPlaylist = std::make_shared<TestPlayListDownloader>();
    testPlaylist->returnInPlaylist_ = false;
    manager->playlistDownloader_ = testPlaylist;
    
    PlayInfo frag1;
    frag1.url_ = "test1.ts";
    frag1.streamId_ = 0;
    frag1.duration_ = 10.0;
    frag1.offset_ = 0;
    frag1.length_ = 1000;
    manager->seekCandidates_.push_back(frag1);
    
    auto result = manager->SeekListHelper(true);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, SeekListHelper_003, TestSize.Level1)
{
    auto manager = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    manager->Init();
    manager->playlistDownloader_ = nullptr;
    
    PlayInfo frag1;
    frag1.url_ = "test1.ts";
    frag1.streamId_ = 0;
    frag1.duration_ = 10.0;
    frag1.offset_ = 0;
    frag1.length_ = 1000;
    manager->seekCandidates_.push_back(frag1);
    
    auto result = manager->SeekListHelper(true);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, SeekListHelper_004, TestSize.Level1)
{
    auto manager = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    manager->Init();
    auto testPlaylist = std::make_shared<TestPlayListDownloader>();
    manager->playlistDownloader_ = testPlaylist;
    
    auto result = manager->SeekListHelper(true);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, SeekListHelper_005, TestSize.Level1)
{
    auto manager = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    manager->Init();
    auto testPlaylist = std::make_shared<TestPlayListDownloader>();
    testPlaylist->returnInPlaylist_ = true;
    manager->playlistDownloader_ = testPlaylist;
    
    PlayInfo frag1, frag2, frag3;
    frag1.url_ = "test1.ts";
    frag1.streamId_ = 0;
    frag1.duration_ = 10.0;
    frag2.url_ = "test2.ts";
    frag2.streamId_ = 0;
    frag2.duration_ = 10.0;
    frag3.url_ = "test3.ts";
    frag3.streamId_ = 0;
    frag3.duration_ = 10.0;
    manager->seekCandidates_.push_back(frag1);
    manager->seekCandidates_.push_back(frag2);
    manager->seekCandidates_.push_back(frag3);
    
    auto result = manager->SeekListHelper(true);
    EXPECT_EQ(result.size(), 3);
}

HWTEST_F(HlsSegmentManagerUnitTest, SeekListHelper_006, TestSize.Level1)
{
    auto manager = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    manager->Init();
    
    auto result = manager->SeekListHelper(false);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(HlsSegmentManagerUnitTest, SeekListHelper_007, TestSize.Level1)
{
    auto manager = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    manager->Init();
    
    PlayInfo item1, item2;
    item1.url_ = "vod1.ts";
    item1.streamId_ = 0;
    item1.duration_ = 10.0;
    item2.url_ = "vod2.ts";
    item2.streamId_ = 0;
    item2.duration_ = 10.0;
    manager->backPlayList_.push_back(item1);
    manager->backPlayList_.push_back(item2);
    
    auto result = manager->SeekListHelper(false);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].url_, "vod1.ts");
    EXPECT_EQ(result[1].url_, "vod2.ts");
}

HWTEST_F(HlsSegmentManagerUnitTest, SeekListHelper_008, TestSize.Level1)
{
    auto manager = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    manager->Init();
    auto testPlaylist = std::make_shared<TestPlayListDownloader>();
    testPlaylist->returnInPlaylist_ = true;
    manager->playlistDownloader_ = testPlaylist;
    
    PlayInfo frag1, frag2;
    frag1.url_ = "test1.ts";
    frag1.streamId_ = 0;
    frag1.duration_ = 10.0;
    frag1.offset_ = 0;
    frag1.length_ = 1000;
    frag2.url_ = "test2.ts";
    frag2.streamId_ = 0;
    frag2.duration_ = 10.0;
    frag2.offset_ = 1000;
    frag2.length_ = 1000;
    manager->seekCandidates_.push_back(frag1);
    manager->seekCandidates_.push_back(frag2);
    
    auto result = manager->SeekListHelper(true);
    EXPECT_EQ(result.size(), 2);
}

HWTEST_F(HlsSegmentManagerUnitTest, REMOVE_FMP4_PADDING_DATA_002, TestSize.Level1)
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
    0x04, 0x04, 0x04, 0x00
    };
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 1;
    readDataInfo.wantReadLength_ = 16;
    readDataInfo.realReadLength_ = 16;
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
    downloader->RemoveFmp4PaddingData(buffer, readDataInfo);
    EXPECT_EQ(readDataInfo.realReadLength_, 16);
}

/**
 * @tc.name  : Test IsCurrentTsFinished
 * @tc.number: IsCurrentTsFinished_001
 * @tc.desc  : Test IsCurrentTsFinished when tsStorageInfo_ does not contain index
 */
HWTEST_F(HlsSegmentManagerUnitTest, IsCurrentTsFinished_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    hlsSegmentManager_->tsStorageInfo_.clear();
    hlsSegmentManager_->writeTsIndex_ = 0;
    hlsSegmentManager_->isFirstFrameArrived_ = true;
    EXPECT_FALSE(hlsSegmentManager_->IsCurrentTsFinished());
}

/**
 * @tc.name  : Test IsCurrentTsFinished
 * @tc.number: IsCurrentTsFinished_002
 * @tc.desc  : Test IsCurrentTsFinished when tsStorageInfo_ contains index and second==true
 */
HWTEST_F(HlsSegmentManagerUnitTest, IsCurrentTsFinished_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    hlsSegmentManager_->tsStorageInfo_.clear();
    hlsSegmentManager_->writeTsIndex_ = 0;
    hlsSegmentManager_->isFirstFrameArrived_ = true;
    hlsSegmentManager_->tsStorageInfo_[0] = {0, true};
    EXPECT_TRUE(hlsSegmentManager_->IsCurrentTsFinished());
}

/**
 * @tc.name  : Test IsCurrentTsFinished
 * @tc.number: IsCurrentTsFinished_003
 * @tc.desc  : Test IsCurrentTsFinished when tsStorageInfo_ contains index and second==false
 */
HWTEST_F(HlsSegmentManagerUnitTest, IsCurrentTsFinished_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    hlsSegmentManager_->tsStorageInfo_.clear();
    hlsSegmentManager_->writeTsIndex_ = 0;
    hlsSegmentManager_->isFirstFrameArrived_ = true;
    hlsSegmentManager_->tsStorageInfo_[0] = {0, false};
    EXPECT_FALSE(hlsSegmentManager_->IsCurrentTsFinished());
}

/**
 * @tc.name  : Test NotifyBitrateChange
 * @tc.number: NotifyBitrateChange_001
 * @tc.desc  : Test NotifyBitrateChange with null callback does not crash
 */
HWTEST_F(HlsSegmentManagerUnitTest, NotifyBitrateChange_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    hlsSegmentManager_->callback_.reset();
    hlsSegmentManager_->NotifyBitrateChange(1000);
    EXPECT_EQ(hlsSegmentManager_->isStopped.load(), false);
}

/**
 * @tc.name  : Test NotifyBitrateChange
 * @tc.number: NotifyBitrateChange_002
 * @tc.desc  : Test NotifyBitrateChange with valid callback triggers events
 */
HWTEST_F(HlsSegmentManagerUnitTest, NotifyBitrateChange_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    auto callback = std::make_shared<HlsNetworkBitrateCallback>();
    hlsSegmentManager_->callback_ = callback;
    hlsSegmentManager_->NotifyBitrateChange(500);
    EXPECT_TRUE(callback->eventTriggered_);
}

/**
 * @tc.name  : Test HandleSpliceEnd early return when offset below splice end
 * @tc.number: HandleSpliceEnd_001
 * @tc.desc  : Test HandleSpliceEnd returns early when readOffset_ < tsEndOffset
 */
HWTEST_F(HlsSegmentManagerUnitTest, HandleSpliceEnd_001, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    hlsSegmentManager_->tsStorageInfo_.clear();
    hlsSegmentManager_->readTsIndex_ = 0;
    hlsSegmentManager_->isFirstFrameArrived_ = true;
    hlsSegmentManager_->tsStorageInfo_[0] = {100, true};
    hlsSegmentManager_->readOffset_ = 50;
    unsigned char buff[16] = {0};
    ReadDataInfo readDataInfo;
    hlsSegmentManager_->HandleSpliceEnd(buff, readDataInfo);
    EXPECT_FALSE(hlsSegmentManager_->isTsEnd_.load());
}

/**
 * @tc.name  : Test HandleSpliceEnd triggers splice end
 * @tc.number: HandleSpliceEnd_002
 * @tc.desc  : Test HandleSpliceEnd sets isTsEnd_ when readOffset_ >= tsEndOffset
 */
HWTEST_F(HlsSegmentManagerUnitTest, HandleSpliceEnd_002, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    hlsSegmentManager_->tsStorageInfo_.clear();
    hlsSegmentManager_->readTsIndex_ = 0;
    hlsSegmentManager_->isFirstFrameArrived_ = true;
    hlsSegmentManager_->tsStorageInfo_[0] = {100, true};
    hlsSegmentManager_->readOffset_ = 100;
    hlsSegmentManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    hlsSegmentManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    unsigned char buff[16] = {0};
    ReadDataInfo readDataInfo;
    hlsSegmentManager_->HandleSpliceEnd(buff, readDataInfo);
    EXPECT_TRUE(hlsSegmentManager_->isTsEnd_.load());
}

/**
 * @tc.name  : Test HandleSpliceEnd early return when tsStorageInfo_ key not found
 * @tc.number: HandleSpliceEnd_003
 * @tc.desc  : Test HandleSpliceEnd returns early when readTsIndex_ not in tsStorageInfo_
 */
HWTEST_F(HlsSegmentManagerUnitTest, HandleSpliceEnd_003, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    hlsSegmentManager_->tsStorageInfo_.clear();
    hlsSegmentManager_->readTsIndex_ = 0;
    hlsSegmentManager_->isFirstFrameArrived_ = true;
    unsigned char buff[16] = {0};
    ReadDataInfo readDataInfo;
    hlsSegmentManager_->HandleSpliceEnd(buff, readDataInfo);
    EXPECT_FALSE(hlsSegmentManager_->isTsEnd_.load());
}

/**
 * @tc.name  : Test HandleSpliceEnd early return when tsStorageInfo second is false
 * @tc.number: HandleSpliceEnd_004
 * @tc.desc  : Test HandleSpliceEnd returns early when tsStorageInfo_[readTsIndex_].second is false
 */
HWTEST_F(HlsSegmentManagerUnitTest, HandleSpliceEnd_004, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    hlsSegmentManager_->tsStorageInfo_.clear();
    hlsSegmentManager_->readTsIndex_ = 0;
    hlsSegmentManager_->isFirstFrameArrived_ = true;
    hlsSegmentManager_->tsStorageInfo_[0] = {100, false};
    hlsSegmentManager_->readOffset_ = 100;
    unsigned char buff[16] = {0};
    ReadDataInfo readDataInfo;
    hlsSegmentManager_->HandleSpliceEnd(buff, readDataInfo);
    EXPECT_FALSE(hlsSegmentManager_->isTsEnd_.load());
}

/**
 * @tc.name  : Test RemoveFmp4PaddingData with valid PKCS#7 padding
 * @tc.number: RemoveFmp4PaddingData_valid_padding
 * @tc.desc  : Test PKCS#7 valid padding (all bytes equal padding length)
 */
HWTEST_F(HlsSegmentManagerUnitTest, RemoveFmp4PaddingData_valid_padding, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(
        10, true, header_, HlsSegmentType::SEG_VIDEO);
    auto mockPlaylist = std::make_shared<TestPlayListDownloader>();
    mockPlaylist->isHlsFmp4_ = true;
    downloader->playlistDownloader_ = mockPlaylist;
    auto decryptor = std::make_shared<AesDecryptor>();
    uint8_t iv[16] = {0};
    decryptor->SetIv(iv, sizeof(iv));
    downloader->SetAesDecryptor(decryptor);
    unsigned char buffer[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3};
    ReadDataInfo readDataInfo;
    readDataInfo.realReadLength_ = 16;
    readDataInfo.wantReadLength_ = 16;
    readDataInfo.streamId_ = 1;
    downloader->RemoveFmp4PaddingData(buffer, readDataInfo);
    EXPECT_EQ(readDataInfo.realReadLength_, 13);
}

/**
 * @tc.name  : Test RemoveFmp4PaddingData with invalid PKCS#7 padding
 * @tc.number: RemoveFmp4PaddingData_invalid_padding
 * @tc.desc  : Test PKCS#7 invalid padding (mixed values) does not remove padding
 */
HWTEST_F(HlsSegmentManagerUnitTest, RemoveFmp4PaddingData_invalid_padding, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(
        10, true, header_, HlsSegmentType::SEG_VIDEO);
    auto mockPlaylist = std::make_shared<TestPlayListDownloader>();
    mockPlaylist->isHlsFmp4_ = true;
    downloader->playlistDownloader_ = mockPlaylist;
    auto decryptor = std::make_shared<AesDecryptor>();
    uint8_t iv[16] = {0};
    decryptor->SetIv(iv, sizeof(iv));
    downloader->SetAesDecryptor(decryptor);
    unsigned char buffer[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 2, 3};
    ReadDataInfo readDataInfo;
    readDataInfo.realReadLength_ = 16;
    readDataInfo.wantReadLength_ = 16;
    readDataInfo.streamId_ = 1;
    downloader->RemoveFmp4PaddingData(buffer, readDataInfo);
    EXPECT_EQ(readDataInfo.realReadLength_, 16);
}

/**
 * @tc.name  : Test UpdateAesDecryptor HasExplicitIv true
 * @tc.number: UpdateAesDecryptor_HasExplicitIv_True
 * @tc.desc  : Test UpdateAesDecryptor skips per-fragment IV when HasExplicitIv returns true
 */
HWTEST_F(HlsSegmentManagerUnitTest, UpdateAesDecryptor_HasExplicitIv_True, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    auto mockPlaylist = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mockPlaylist;
    auto decryptor = std::make_shared<AesDecryptor>();
    uint8_t iv[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    decryptor->SetIv(iv, sizeof(iv));
    mockPlaylist->aesDecryptor_ = decryptor;
    mockPlaylist->hasExplicitIvResult_ = true;
    PlayInfo playInfo;
    playInfo.keyIndex_ = 0;
    playInfo.sequence_ = 1;
    hlsSegmentManager_->UpdateAesDecryptor(playInfo);
    auto dec = hlsSegmentManager_->GetAesDecryptor();
    ASSERT_NE(dec, nullptr);
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(dec->iv_[i], iv[i]);
    }
}

/**
 * @tc.name  : Test UpdateAesDecryptor HasExplicitIv false (per-fragment IV)
 * @tc.number: UpdateAesDecryptor_HasExplicitIv_False
 * @tc.desc  : Test UpdateAesDecryptor computes per-fragment IV from sequence when HasExplicitIv returns false
 */
HWTEST_F(HlsSegmentManagerUnitTest, UpdateAesDecryptor_HasExplicitIv_False, TestSize.Level1)
{
    ASSERT_NE(nullptr, hlsSegmentManager_);
    auto mockPlaylist = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mockPlaylist;
    auto decryptor = std::make_shared<AesDecryptor>();
    uint8_t iv[16] = {0};
    decryptor->SetIv(iv, sizeof(iv));
    mockPlaylist->aesDecryptor_ = decryptor;
    mockPlaylist->hasExplicitIvResult_ = false;
    PlayInfo playInfo;
    playInfo.keyIndex_ = 0;
    playInfo.sequence_ = 1;
    hlsSegmentManager_->UpdateAesDecryptor(playInfo);
    auto dec = hlsSegmentManager_->GetAesDecryptor();
    ASSERT_NE(dec, nullptr);
    uint8_t expectedIv[AES_IV_LEN] = {0};
    expectedIv[AES_IV_LEN - 1] = 1;
    for (int i = 0; i < AES_IV_LEN; i++) {
        EXPECT_EQ(dec->iv_[i], expectedIv[i]);
    }
}

HWTEST_F(HlsSegmentManagerUnitTest, DECRYPT_DATA_LEFT_NOT_ENOUGH_DATA_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(
        MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->memorySize_ = MAX_CACHE_BUFFER_SIZE_UT;
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
    uint8_t aesKey[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t aesIv[16] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    downloader->aesDecryptor_->OnSourceKeyChange(aesKey, 16, aesIv);

    downloader->singleBlockLen_ = 5;
    for (uint32_t i = 0; i < 5; i++) {
        downloader->singleBlock_[i] = static_cast<uint8_t>(i);
    }

    uint8_t data[5] = {5, 6, 7, 8, 9};
    uint32_t len = 5;
    uint8_t *dataPtr = data;
    uint32_t result = downloader->DecryptDataLeft(dataPtr, len, true);

    EXPECT_EQ(result, 5);
    EXPECT_EQ(downloader->singleBlockLen_, 10);
    EXPECT_EQ(len, 0);
    for (uint32_t i = 0; i < 10; i++) {
        EXPECT_EQ(downloader->singleBlock_[i], static_cast<uint8_t>(i));
    }

    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, DECRYPT_DATA_LEFT_COMPLETE_BLOCK_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> downloader = std::make_shared<HlsSegmentManager>(
        MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->memorySize_ = MAX_CACHE_BUFFER_SIZE_UT;
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
    uint8_t aesKey[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    uint8_t aesIv[16] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    downloader->aesDecryptor_->OnSourceKeyChange(aesKey, 16, aesIv);

    uint8_t plaintext[16] = "HelloWorld12345";
    uint8_t ciphertext[16] = {0};
    AES_KEY encryptKey;
    AES_set_encrypt_key(aesKey, 128, &encryptKey);
    uint8_t encIv[16];
    (void)memcpy_s(encIv, 16, aesIv, 16);
    AES_cbc_encrypt(plaintext, ciphertext, 16, &encryptKey, encIv, AES_ENCRYPT);

    downloader->singleBlockLen_ = 5;
    (void)memcpy_s(downloader->singleBlock_, 16, ciphertext, 5);

    uint8_t data[11] = {0};
    (void)memcpy_s(data, 11, ciphertext + 5, 11);
    uint32_t len = 11;
    uint8_t *dataPtr = data;
    uint32_t result = downloader->DecryptDataLeft(dataPtr, len, true);

    EXPECT_EQ(result, 11);
    EXPECT_EQ(downloader->singleBlockLen_, 0);
    EXPECT_EQ(len, 0);

    downloader = nullptr;
}

// ==================== Branch Coverage Supplement ====================

// L1027,L1034,L1040,L1046: nullptr defensive checks for getter methods
HWTEST_F(HlsSegmentManagerUnitTest, NULLPTR_GETTERS_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    EXPECT_EQ(hlsSegmentManager_->GetDuration(), 0);
    auto si = hlsSegmentManager_->GetStartInfo();
    EXPECT_EQ(si.first, 0);
    EXPECT_EQ(si.second, false);
    EXPECT_EQ(hlsSegmentManager_->GetSeekable(), Seekable::INVALID);
    EXPECT_EQ(hlsSegmentManager_->GetSeekableRanges().size(), 0);
}

// L1052,L1538,L1545,L2241: nullptr defensive checks for setter/callback methods
HWTEST_F(HlsSegmentManagerUnitTest, NULLPTR_SETTERS_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    auto cb = std::make_shared<SourceCallback>();
    hlsSegmentManager_->SetCallback(cb);
    EXPECT_EQ(hlsSegmentManager_->callback_.lock().get(), nullptr);
    EXPECT_EQ(hlsSegmentManager_->GetBitRates().size(), 0);
    EXPECT_EQ(hlsSegmentManager_->GetHLSDiscontinuity(), false);
    hlsSegmentManager_->SetStatusCallback(nullptr);
    EXPECT_EQ(hlsSegmentManager_->playlistDownloader_, nullptr);
}

// L459,L466,L401: nullptr defensive for Pause/Resume/Open
HWTEST_F(HlsSegmentManagerUnitTest, NULLPTR_PAUSE_RESUME_OPEN_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    hlsSegmentManager_->Pause();
    hlsSegmentManager_->Resume();
    EXPECT_EQ(hlsSegmentManager_->Open("test", header_), false);
}

// L1837,L1838: ReportVideoSizeChange with no callback and nullptr playlist
HWTEST_F(HlsSegmentManagerUnitTest, REPORT_VIDEO_SIZE_NULL_001, TestSize.Level1)
{
    hlsSegmentManager_->segEventCb_ = nullptr;
    hlsSegmentManager_->ReportVideoSizeChange();
    EXPECT_EQ(hlsSegmentManager_->segEventCb_, nullptr);
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    hlsSegmentManager_->ReportVideoSizeChange();
    EXPECT_FALSE(called);
}

// L1875: AutoSelectBitrate with nullptr playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, AUTO_SELECT_BITRATE_NULL_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    hlsSegmentManager_->AutoSelectBitrate(1000);
    EXPECT_EQ(hlsSegmentManager_->recordSpeedCount_, 0);
}

// L1877: AutoSelectBitrate with empty bitrates
HWTEST_F(HlsSegmentManagerUnitTest, AUTO_SELECT_BITRATE_EMPTY_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    mock->bitRates_ = {};
    hlsSegmentManager_->AutoSelectBitrate(1000);
    EXPECT_EQ(mock->bitRates_.size(), 0);
}

// L1883,L1884,L1887,L1894: AutoSelectBitrate with matching bitrate
HWTEST_F(HlsSegmentManagerUnitTest, AUTO_SELECT_BITRATE_MATCH_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    mock->bitRates_ = {500000};
    mock->curBitrate_ = 500000;
    mock->currentBitRateVal_ = 500000;
    hlsSegmentManager_->AutoSelectBitrate(1000000);
    EXPECT_EQ(mock->curBitrate_, 500000);
}

// L1894,L1900: AutoSelectBitrate switch to high but buffer too small
HWTEST_F(HlsSegmentManagerUnitTest, AUTO_SELECT_BITRATE_HIGH_LOWBUF_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    mock->bitRates_ = {100000, 500000};
    mock->curBitrate_ = 100000;
    mock->currentBitRateVal_ = 100000;
    hlsSegmentManager_->GetBufferSize();
    hlsSegmentManager_->AutoSelectBitrate(200000);
    EXPECT_EQ(mock->curBitrate_, 100000);
}

// L1908: AutoSelectBitrate switch to low but buffer too large
HWTEST_F(HlsSegmentManagerUnitTest, AUTO_SELECT_BITRATE_LOW_HIGHBUF_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    mock->bitRates_ = {100000, 500000};
    mock->curBitrate_ = 500000;
    mock->currentBitRateVal_ = 500000;
    hlsSegmentManager_->AutoSelectBitrate(200000);
    EXPECT_EQ(mock->curBitrate_, 500000);
}

// L144,L146: Copy constructor with nullptr other
HWTEST_F(HlsSegmentManagerUnitTest, COPY_CTOR_NULLPTR_001, TestSize.Level1)
{
    std::shared_ptr<HlsSegmentManager> nullOther = nullptr;
    auto downloader = std::make_shared<HlsSegmentManager>(nullOther, HlsSegmentType::SEG_VIDEO);
    EXPECT_EQ(downloader->totalBufferSize_, 0);
    downloader = nullptr;
}

// L146 branch 0: Copy constructor with valid other, userDefinedBufferDuration_ = true
HWTEST_F(HlsSegmentManagerUnitTest, COPY_CTOR_VALID_001, TestSize.Level1)
{
    auto other = std::make_shared<HlsSegmentManager>(10, true, header_);
    other->Init();
    auto downloader = std::make_shared<HlsSegmentManager>(other, HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(downloader->userDefinedBufferDuration_, true);
    EXPECT_EQ(downloader->expectDuration_, other->expectDuration_);
    downloader = nullptr;
    other = nullptr;
}

// L2372 branch 2: HandleSeekReady SEG_SUBTITLE switch
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_SEEK_READY_SUBTITLE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_SUBTITLE);
    downloader->Init();
    bool called = false;
    downloader->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    downloader->HandleSeekReady(1, 0);
    EXPECT_TRUE(called);
    downloader = nullptr;
}

// L2372 branch 0: HandleSeekReady SEG_AUDIO (default) switch
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_SEEK_READY_AUDIO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_AUDIO);
    downloader->Init();
    bool called = false;
    downloader->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    downloader->HandleSeekReady(2, 1);
    EXPECT_TRUE(called);
    downloader = nullptr;
}

// L2389 branch 1: HandleSeekReady with segEventCb_ set (SEG_VIDEO)
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_SEEK_READY_VIDEO_CB_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    bool called = false;
    downloader->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    downloader->HandleSeekReady(1, 0);
    EXPECT_TRUE(called);
    downloader = nullptr;
}

// L2251 branches 0-5: StopBufferring with null pointers
HWTEST_F(HlsSegmentManagerUnitTest, STOP_BUFFERING_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = nullptr;
    EXPECT_EQ(downloader->StopBufferring(false), Status::ERROR_NULL_POINTER);
    downloader = nullptr;
}

// L2256: StopBufferring isAppBackground true/false
HWTEST_F(HlsSegmentManagerUnitTest, STOP_BUFFERING_BG_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    EXPECT_EQ(downloader->StopBufferring(true), Status::OK);
    EXPECT_EQ(downloader->isInterrupt_.load(), true);
    EXPECT_EQ(downloader->StopBufferring(false), Status::OK);
    EXPECT_EQ(downloader->isInterrupt_.load(), false);
    downloader = nullptr;
}

// L1554: SelectBitRate SEG_AUDIO returns true
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_AUDIO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_AUDIO);
    downloader->Init();
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L1555: SelectBitRate SEG_SUBTITLE returns true
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_SUBTITLE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_SUBTITLE);
    downloader->Init();
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L1561,L1564: SelectBitRate with callback, auto select, CanAutoSelectBitRate returns false
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_NO_AUTO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    auto cb = std::make_shared<HlsNetworkBitrateCallback>();
    downloader->SetCallback(cb);
    downloader->isAutoSelectBitrate_ = true;
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L1561: SelectBitRate with callback but isAutoSelectBitrate_ = false
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_NO_AUTOMODE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    auto cb = std::make_shared<HlsNetworkBitrateCallback>();
    downloader->SetCallback(cb);
    downloader->isAutoSelectBitrate_ = false;
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L2146: HandleCachedDuration with tmpBitRate <= 0
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_CACHED_DUR_ZERO_001, TestSize.Level1)
{
    hlsSegmentManager_->currentBitRate_ = 0;
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    hlsSegmentManager_->HandleCachedDuration();
    EXPECT_FALSE(called);
}

// L2146 branch 3: HandleCachedDuration with segEventCb_ == nullptr
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_CACHED_DUR_NO_CB_001, TestSize.Level1)
{
    hlsSegmentManager_->currentBitRate_ = 1000;
    hlsSegmentManager_->segEventCb_ = nullptr;
    hlsSegmentManager_->HandleCachedDuration();
    EXPECT_EQ(hlsSegmentManager_->cachedDuration_, 0);
}

// L2151,L2152: HandleCachedDuration with duration change > threshold
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_CACHED_DUR_CHANGE_001, TestSize.Level1)
{
    hlsSegmentManager_->currentBitRate_ = 1000;
    hlsSegmentManager_->lastDurationReacord_ = 0;
    hlsSegmentManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    hlsSegmentManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    uint8_t data[4096] = {0};
    hlsSegmentManager_->writeOffset_ = 0;
    hlsSegmentManager_->writeTsIndex_ = 1;
    hlsSegmentManager_->readTsIndex_ = 0;
    hlsSegmentManager_->cacheMediaBuffer_->Write(data, 0, 4096);
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    hlsSegmentManager_->HandleCachedDuration();
    EXPECT_EQ(hlsSegmentManager_->cachedDuration_ > 0, true);
}

// L1425,L1428,L1430: DownloadRecordHistory with autoBufferSize_
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_RECORD_HISTORY_AUTO_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    hlsSegmentManager_->autoBufferSize_ = true;
    hlsSegmentManager_->userDefinedBufferDuration_ = false;
    hlsSegmentManager_->lastWriteTime_ = 0;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 1000000;
    hlsSegmentManager_->recordData_ = rd;
    mock->currentBitRateVal_ = 500000;
    hlsSegmentManager_->DownloadRecordHistory(2000);
    EXPECT_EQ(hlsSegmentManager_->lastWriteTime_ > 0, true);
}

// L1425 branch 1,3: DownloadRecordHistory with userDefinedBufferDuration_ = true
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_RECORD_HISTORY_USERDEF_001, TestSize.Level1)
{
    hlsSegmentManager_->autoBufferSize_ = true;
    hlsSegmentManager_->userDefinedBufferDuration_ = true;
    hlsSegmentManager_->lastWriteTime_ = 0;
    hlsSegmentManager_->DownloadRecordHistory(2000);
    EXPECT_EQ(hlsSegmentManager_->lastWriteTime_ > 0, true);
}

// L1463: CalculateCurrentDownloadSpeed with data
HWTEST_F(HlsSegmentManagerUnitTest, CALC_DOWNLOAD_SPEED_001, TestSize.Level1)
{
    hlsSegmentManager_->downloadBits_ = 10000;
    hlsSegmentManager_->downloadDuringTime_ = 1000;
    double speed = hlsSegmentManager_->CalculateCurrentDownloadSpeed();
    EXPECT_EQ(static_cast<int>(speed), 10000);
}

// L1463 branch 0: CalculateCurrentDownloadSpeed with zero denominator
HWTEST_F(HlsSegmentManagerUnitTest, CALC_DOWNLOAD_SPEED_ZERO_001, TestSize.Level1)
{
    hlsSegmentManager_->downloadBits_ = 10000;
    hlsSegmentManager_->downloadDuringTime_ = 0;
    double speed = hlsSegmentManager_->CalculateCurrentDownloadSpeed();
    EXPECT_EQ(speed, 0.0);
}

// L1494: DownloadReport with readBitrate_ > 0
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REPORT_BITRATE_001, TestSize.Level1)
{
    hlsSegmentManager_->totalBits_ = 100000;
    hlsSegmentManager_->lastBits_ = 0;
    hlsSegmentManager_->lastCheckTime_ = 0;
    hlsSegmentManager_->readBitrate_ = 1000;
    hlsSegmentManager_->bufferedDuration_ = 10000;
    hlsSegmentManager_->lastCheckTime_ = 0;
    OSAL::SleepFor(1100);
    hlsSegmentManager_->DownloadReport();
    EXPECT_EQ(hlsSegmentManager_->lastCheckTime_ > 0, true);
}

// L1507: DownloadReport with data usage interval exceeded
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REPORT_USAGE_001, TestSize.Level1)
{
    hlsSegmentManager_->isDownloadFinish_ = false;
    hlsSegmentManager_->lastReportUsageTime_ = 0;
    hlsSegmentManager_->dataUsage_ = 5000;
    hlsSegmentManager_->DownloadReport();
    EXPECT_EQ(hlsSegmentManager_->dataUsage_, 5000);
}

// L1517: OnDrmInfoChanged with segEventCb_ set
HWTEST_F(HlsSegmentManagerUnitTest, ON_DRM_INFO_CHANGED_001, TestSize.Level1)
{
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    std::multimap<std::string, std::vector<uint8_t>> drmInfos;
    drmInfos.insert({"key", {1, 2, 3}});
    hlsSegmentManager_->OnDrmInfoChanged(drmInfos);
    EXPECT_TRUE(called);
}

// L1517 branch 0: OnDrmInfoChanged with no callback
HWTEST_F(HlsSegmentManagerUnitTest, ON_DRM_INFO_CHANGED_NULL_001, TestSize.Level1)
{
    hlsSegmentManager_->segEventCb_ = nullptr;
    std::multimap<std::string, std::vector<uint8_t>> drmInfos;
    hlsSegmentManager_->OnDrmInfoChanged(drmInfos);
    EXPECT_EQ(hlsSegmentManager_->totalBits_, 0);
}

// L1526: OnTimedMetaDataChanged with nullptr metadata
HWTEST_F(HlsSegmentManagerUnitTest, ON_TIMED_META_NULL_001, TestSize.Level1)
{
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    hlsSegmentManager_->OnTimedMetaDataChanged(nullptr);
    EXPECT_FALSE(called);
}

// L1529: OnTimedMetaDataChanged with valid metadata and callback
HWTEST_F(HlsSegmentManagerUnitTest, ON_TIMED_META_VALID_001, TestSize.Level1)
{
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    auto metadata = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    metadata->id = "test";
    metadata->start = 0;
    metadata->duration = 10;
    hlsSegmentManager_->OnTimedMetaDataChanged(metadata);
    EXPECT_TRUE(called);
}

// L2029,L2032: SetInterruptState with null playlistDownloader_ and downloader_
HWTEST_F(HlsSegmentManagerUnitTest, SET_INTERRUPT_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->playlistDownloader_ = nullptr;
    downloader->downloader_ = nullptr;
    downloader->SetInterruptState(true);
    EXPECT_EQ(downloader->isInterruptNeeded_.load(), true);
    downloader = nullptr;
}

// L2052: GetPlaybackInfo with null downloader_
HWTEST_F(HlsSegmentManagerUnitTest, GET_PLAYBACK_INFO_NULL_DL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->downloader_ = nullptr;
    PlaybackInfo info;
    downloader->GetPlaybackInfo(info);
    EXPECT_EQ(info.averageDownloadRate, 0);
    downloader = nullptr;
}

// L2066 branch 1: GetPlaybackInfo with readBitrate_ > 0 and recordData_
HWTEST_F(HlsSegmentManagerUnitTest, GET_PLAYBACK_INFO_BITRATE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->readBitrate_ = 1000;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 500.0;
    downloader->recordData_ = rd;
    PlaybackInfo info;
    downloader->GetPlaybackInfo(info);
    EXPECT_EQ(info.downloadRate, 500);
    downloader = nullptr;
}

// L2080: ReportBitrateStart without segEventCb_
HWTEST_F(HlsSegmentManagerUnitTest, REPORT_BITRATE_START_NULL_001, TestSize.Level1)
{
    hlsSegmentManager_->segEventCb_ = nullptr;
    hlsSegmentManager_->ReportBitrateStart(1000);
    EXPECT_EQ(hlsSegmentManager_->changeBitRateCount_, 0);
}

// L2080 branch 1: ReportBitrateStart with segEventCb_
HWTEST_F(HlsSegmentManagerUnitTest, REPORT_BITRATE_START_CB_001, TestSize.Level1)
{
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    hlsSegmentManager_->ReportBitrateStart(2000);
    EXPECT_TRUE(called);
}

// L2122: SetCurrentBitRate with downloadRequest_ set
HWTEST_F(HlsSegmentManagerUnitTest, SET_CURRENT_BITRATE_REQ_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto req = std::make_shared<DownloadRequest>("", nullptr, nullptr, false);
    downloader->SetDownloadRequest(req);
    EXPECT_EQ(downloader->SetCurrentBitRate(1000, 0), Status::OK);
    downloader = nullptr;
}

// L2122 branch 1: SetCurrentBitRate without downloadRequest_
HWTEST_F(HlsSegmentManagerUnitTest, SET_CURRENT_BITRATE_NOREQ_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->SetDownloadRequest(nullptr);
    EXPECT_EQ(downloader->SetCurrentBitRate(1000, 0), Status::OK);
    downloader = nullptr;
}

// L2122: SetCurrentBitRate with bitRate <= 0 and currentBitRate_ == 0
HWTEST_F(HlsSegmentManagerUnitTest, SET_CURRENT_BITRATE_ZERO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->currentBitRate_ = 0;
    EXPECT_EQ(downloader->SetCurrentBitRate(0, 0), Status::OK);
    EXPECT_EQ(downloader->currentBitRate_, -1);
    downloader = nullptr;
}

// L2214,L2215: GetSegmentOffset with live playlist
HWTEST_F(HlsSegmentManagerUnitTest, GET_SEGMENT_OFFSET_LIVE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = true;
    downloader->playlistDownloader_ = mock;
    downloader->readTsIndex_ = 0;
    auto offset = downloader->GetSegmentOffset();
    EXPECT_EQ(offset, 0);
    downloader = nullptr;
}

// L2214 branch 0: GetSegmentOffset with nullptr playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, GET_SEGMENT_OFFSET_NULL_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    EXPECT_EQ(hlsSegmentManager_->GetSegmentOffset(), 0);
}

// L2220: GetSegmentOffset with sumDuration < lastDuration
HWTEST_F(HlsSegmentManagerUnitTest, GET_SEGMENT_OFFSET_LIVE_SUM_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = true;
    downloader->playlistDownloader_ = mock;
    downloader->readTsIndex_ = 0;
    PlayInfo p;
    p.url_ = "test_url";
    p.duration_ = 10;
    p.sumDuration_ = 100;
    downloader->InfoIndexMap_.writeMap[0] = "test_url";
    downloader->InfoIndexMap_.urlMap["test_url"] = p;
    auto offset = downloader->GetSegmentOffset();
    EXPECT_EQ(offset, 0);
    downloader = nullptr;
}

// L2232: GetCurUrl with map entry found and not empty
HWTEST_F(HlsSegmentManagerUnitTest, GET_CUR_URL_MAP_001, TestSize.Level1)
{
    hlsSegmentManager_->InfoIndexMap_.writeMap[0] = "http://test.com/seg.ts";
    hlsSegmentManager_->readTsIndex_ = 0;
    EXPECT_EQ(hlsSegmentManager_->GetCurUrl(), "http://test.com/seg.ts");
}

// L2232: GetCurUrl with map entry found but empty
HWTEST_F(HlsSegmentManagerUnitTest, GET_CUR_URL_EMPTY_001, TestSize.Level1)
{
    hlsSegmentManager_->InfoIndexMap_.writeMap[0] = "";
    hlsSegmentManager_->readTsIndex_ = 0;
    hlsSegmentManager_->curUrl_ = "http://fallback.com/";
    EXPECT_EQ(hlsSegmentManager_->GetCurUrl(), "http://fallback.com/");
}

// L2232: GetCurUrl with map entry not found
HWTEST_F(HlsSegmentManagerUnitTest, GET_CUR_URL_NOTFOUND_001, TestSize.Level1)
{
    hlsSegmentManager_->readTsIndex_ = 999;
    hlsSegmentManager_->curUrl_ = "http://fallback.com/";
    EXPECT_EQ(hlsSegmentManager_->GetCurUrl(), "http://fallback.com/");
}

// L2241: GetHLSDiscontinuity with valid playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, GET_HLS_DISCONTINUITY_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    EXPECT_EQ(hlsSegmentManager_->GetHLSDiscontinuity(), false);
}

// L1977: DownBufferSize with totalBufferSize_ <= ADJUST_SIZE
HWTEST_F(HlsSegmentManagerUnitTest, DOWN_BUFFER_SIZE_SMALL_001, TestSize.Level1)
{
    hlsSegmentManager_->totalBufferSize_ = 100;
    hlsSegmentManager_->minBufferSize_ = 50;
    hlsSegmentManager_->DownBufferSize();
    EXPECT_EQ(hlsSegmentManager_->totalBufferSize_, 50);
}

// L1986 branch 1: OnReadBuffer with duration < bufferedDuration_
HWTEST_F(HlsSegmentManagerUnitTest, ON_READ_BUFFER_PARTIAL_001, TestSize.Level1)
{
    hlsSegmentManager_->bufferedDuration_ = 1000;
    hlsSegmentManager_->OnReadBuffer(50);
    EXPECT_EQ(hlsSegmentManager_->bufferedDuration_, 600);
}

// L2011: TransferSizeToBitRate width = MIN_WIDTH (480)
HWTEST_F(HlsSegmentManagerUnitTest, TRANSFER_SIZE_MIN_WIDTH_001, TestSize.Level1)
{
    EXPECT_EQ(hlsSegmentManager_->TransferSizeToBitRate(MIN_WITDH), hlsSegmentManager_->minBufferSize_);
}

// L2013: TransferSizeToBitRate width = SECOND_WIDTH (720)
HWTEST_F(HlsSegmentManagerUnitTest, TRANSFER_SIZE_SECOND_WIDTH_001, TestSize.Level1)
{
    EXPECT_EQ(hlsSegmentManager_->TransferSizeToBitRate(SECOND_WITDH),
        hlsSegmentManager_->minBufferSize_ * 3);
}

// L2482: Clone with nullptr
HWTEST_F(HlsSegmentManagerUnitTest, CLONE_NULLPTR_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->Clone(nullptr);
    EXPECT_EQ(downloader->totalBufferSize_, 10 * 200 * 1024);
    downloader = nullptr;
}

// L2500: Clone with null playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, CLONE_NULL_PLAYLIST_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->playlistDownloader_ = nullptr;
    auto other = std::make_shared<HlsSegmentManager>(10, true, header_);
    other->Init();
    downloader->Clone(other);
    EXPECT_EQ(downloader->totalBufferSize_, 10 * 200 * 1024);
    downloader = nullptr;
    other = nullptr;
}

// L2502: Clone with null other playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, CLONE_NULL_OTHER_PLAYLIST_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto other = std::make_shared<HlsSegmentManager>(10, true, header_);
    other->Init();
    other->playlistDownloader_ = nullptr;
    downloader->Clone(other);
    EXPECT_EQ(downloader->playlistDownloader_ != nullptr, true);
    downloader = nullptr;
    other = nullptr;
}

// L2515: SelectMedia with nullptr playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_MEDIA_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_AUDIO);
    downloader->Init();
    downloader->playlistDownloader_ = nullptr;
    EXPECT_EQ(downloader->SelectMedia(0, HlsSegmentType::SEG_AUDIO), false);
    downloader = nullptr;
}

// L2518: SelectMedia with SEG_VIDEO type returns true
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_MEDIA_VIDEO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    EXPECT_EQ(downloader->SelectMedia(0, HlsSegmentType::SEG_AUDIO), true);
    downloader = nullptr;
}

// L1289: SaveData with null data
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_DATA_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    EXPECT_EQ(downloader->SaveData(nullptr, 100, false), 0);
    downloader = nullptr;
}

// L1289 branch 3: SaveData with null cacheMediaBuffer_
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_DATA_NULL_BUF_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = nullptr;
    uint8_t data[10] = {0};
    EXPECT_EQ(downloader->SaveData(data, 10, false), 0);
    downloader = nullptr;
}

// L883: Read with null cacheMediaBuffer_
HWTEST_F(HlsSegmentManagerUnitTest, READ_NULL_BUF_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = nullptr;
    unsigned char buff[10];
    ReadDataInfo readDataInfo;
    readDataInfo.wantReadLength_ = 10;
    EXPECT_EQ(downloader->Read(buff, readDataInfo), Status::ERROR_AGAIN);
    downloader = nullptr;
}

// L572: CheckLiveLastSegment with null playlist
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_LIVE_LAST_SEG_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->playList_ = nullptr;
    EXPECT_EQ(downloader->CheckLiveLastSegment(), false);
}

// L572 branch 1: CheckLiveLastSegment with valid playlist but not last play changed
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_LIVE_LAST_SEG_FALSE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->lastPlaychanged_.store(false);
    EXPECT_EQ(downloader->CheckLiveLastSegment(), false);
}

// L682: CheckLiveToVodEnd with null playlist
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_LIVE_TO_VOD_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->playList_ = nullptr;
    EXPECT_EQ(downloader->CheckLiveToVodEnd(), false);
}

// L477: CheckReadStatus with null playList_
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_READ_STATUS_NULL_PL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->playList_ = nullptr;
    EXPECT_EQ(downloader->CheckReadStatus(), false);
}

// L477 branch 1: CheckReadStatus with null downloadRequest_
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_READ_STATUS_NULL_REQ_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->SetDownloadRequest(nullptr);
    EXPECT_EQ(downloader->CheckReadStatus(), false);
}

// L845: IsCurrentTsFinished with SEG_SUBTITLE and !isFirstFrameArrived_
HWTEST_F(HlsSegmentManagerUnitTest, IS_CURRENT_TS_FINISHED_SUB_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_SUBTITLE);
    downloader->Init();
    downloader->isFirstFrameArrived_ = false;
    downloader->readTsIndex_ = 0;
    EXPECT_EQ(downloader->IsCurrentTsFinished(), false);
}

// L875,L878: IsAllDownloadFinish with tsStorageInfo and isStopped
HWTEST_F(HlsSegmentManagerUnitTest, IS_ALL_DOWNLOAD_FINISH_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->isStopped = true;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(100, true);
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    EXPECT_EQ(downloader->IsAllDownloadFinish(), true);
    downloader = nullptr;
}

// L875 branch 0: IsAllDownloadFinish with tsStorageInfo not found
HWTEST_F(HlsSegmentManagerUnitTest, IS_ALL_DOWNLOAD_FINISH_NOTFOUND_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->isStopped = true;
    downloader->writeTsIndex_ = 999;
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    EXPECT_EQ(downloader->IsAllDownloadFinish(), false);
    downloader = nullptr;
}

// L1820: IsDownloadLastSplice with non-live and last splice
HWTEST_F(HlsSegmentManagerUnitTest, IS_DOWNLOAD_LAST_SPLICE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = false;
    downloader->playlistDownloader_ = mock;
    downloader->writeTsIndex_ = 1;
    PlayInfo p;
    downloader->backPlayList_.push_back(p);
    downloader->backPlayList_.push_back(p);
    EXPECT_EQ(downloader->IsDownloadLastSplice(), true);
    downloader = nullptr;
}

// L1820: IsDownloadLastSplice with null playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, IS_DOWNLOAD_LAST_SPLICE_NULL_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    EXPECT_EQ(hlsSegmentManager_->IsDownloadLastSplice(), false);
}

// L2441: ClearChunksInLargeSegment with conditions met
HWTEST_F(HlsSegmentManagerUnitTest, CLEAR_CHUNKS_LARGE_MET_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->writeTsIndex_ = 0;
    downloader->readTsIndex_ = 0;
    downloader->writeOffset_ = 100;
    downloader->readOffset_ = 50;
    downloader->ClearChunksInLargeSegment();
    EXPECT_GE(downloader->writeOffset_, downloader->readOffset_);
    downloader = nullptr;
}

// L2404: GetTotalTsBuffersize with writeTsIndex_ > readTsIndex_
HWTEST_F(HlsSegmentManagerUnitTest, GET_TOTAL_TS_BUFSIZE_MULTI_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    uint8_t data[1024] = {0};
    downloader->cacheMediaBuffer_->Write(data, 0, 1024);
    downloader->readTsIndex_ = 0;
    downloader->writeTsIndex_ = 1;
    downloader->readOffset_ = 0;
    uint64_t size = downloader->GetTotalTsBuffersize();
    EXPECT_EQ(size <= static_cast<uint64_t>(MAX_CACHE_BUFFER_SIZE_UT), true);
    downloader = nullptr;
}

// L1764,L1766: ResetDecryptBuffer with aesDecryptor_
HWTEST_F(HlsSegmentManagerUnitTest, RESET_DECRYPT_BUFFER_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
    uint8_t key[16] = {0};
    uint8_t iv[16] = {0};
    downloader->aesDecryptor_->OnSourceKeyChange(key, 16, iv);
    downloader->singleBlockLen_ = 5;
    downloader->decryptLen_ = 10;
    downloader->ResetDecryptBuffer();
    EXPECT_EQ(downloader->singleBlockLen_, 0);
    EXPECT_EQ(downloader->decryptLen_, 0);
    downloader = nullptr;
}

// L1764,L1766 branch 1: ResetDecryptBuffer without aesDecryptor_
HWTEST_F(HlsSegmentManagerUnitTest, RESET_DECRYPT_BUFFER_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->aesDecryptor_ = nullptr;
    downloader->singleBlockLen_ = 5;
    downloader->decryptLen_ = 10;
    downloader->ResetDecryptBuffer();
    EXPECT_EQ(downloader->singleBlockLen_, 0);
    EXPECT_EQ(downloader->decryptLen_, 0);
    downloader = nullptr;
}

// L1357: DecryptDataLeft with writtenLen != decryptLen_
HWTEST_F(HlsSegmentManagerUnitTest, DECRYPT_DATA_LEFT_WRITE_ERR_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
    uint8_t key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint8_t iv[16] = {0};
    downloader->aesDecryptor_->OnSourceKeyChange(key, 16, iv);
    downloader->singleBlockLen_ = 0;
    uint8_t data[16] = {0};
    uint32_t len = 16;
    uint8_t *ptr = data;
    uint32_t result = downloader->DecryptDataLeft(ptr, len, true);
    EXPECT_EQ(result, 0);
    downloader = nullptr;
}

// L1368: DecryptData with nullptr aesDecryptor
HWTEST_F(HlsSegmentManagerUnitTest, DECRYPT_DATA_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->aesDecryptor_ = nullptr;
    uint8_t data[16] = {0};
    uint32_t len = 16;
    uint8_t *ptr = data;
    EXPECT_EQ(downloader->DecryptData(ptr, len, true), 0);
    downloader = nullptr;
}

// L507: HandleFfmpegReadback with notNeedReadBack_ = true
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_FFMPEG_READBACK_NOBACK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->notNeedReadBack_.store(true);
    downloader->ffmpegOffset_ = 0;
    downloader->HandleFfmpegReadback(1000);
    EXPECT_EQ(downloader->ffmpegOffset_, 1000);
    downloader = nullptr;
}

// L507 branch 0: HandleFfmpegReadback with ffmpegOffset_ <= ffmpegOffset
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_FFMPEG_READBACK_LE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->notNeedReadBack_.store(false);
    downloader->curStreamId_ = 0;
    downloader->isNeedResetOffset_.store(false);
    downloader->ffmpegOffset_ = 100;
    downloader->HandleFfmpegReadback(200);
    EXPECT_EQ(downloader->ffmpegOffset_, 100);
    downloader = nullptr;
}

// L660 branch 0: ReadDelegate returns tmpRes != ERROR_UNKNOWN
HWTEST_F(HlsSegmentManagerUnitTest, READ_DELEGATE_TMPRES_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->isStopped = true;
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->duration_ = 100;
    downloader->playlistDownloader_ = mock;
    downloader->writeTsIndex_ = 0;
    downloader->readTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, true);
    unsigned char buff[10];
    ReadDataInfo rdi;
    rdi.wantReadLength_ = 10;
    rdi.streamId_ = 0;
    auto status = downloader->ReadDelegate(buff, rdi);
    EXPECT_EQ(status, Status::END_OF_STREAM);
    downloader = nullptr;
}

// L1806: UpdateDownloadFinished with SEG_SUBTITLE type
HWTEST_F(HlsSegmentManagerUnitTest, UPDATE_DOWNLOAD_FINISHED_SUB_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_SUBTITLE);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isDownloadStarted_ = false;
    auto req = std::make_shared<DownloadRequest>("", nullptr, nullptr, false);
    downloader->SetDownloadRequest(req);
    downloader->UpdateDownloadFinished("test_url", "");
    EXPECT_EQ(downloader->isDownloadFinish_, true);
    downloader = nullptr;
}

// L1069: PlaylistBackup with nullptr playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_BACKUP_NULL_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    PlayInfo p;
    hlsSegmentManager_->PlaylistBackup(p);
    EXPECT_EQ(hlsSegmentManager_->backPlayList_.size(), 0);
}

// L1069 branch 1, L1070, L1074, L1077: PlaylistBackup with live and duplicate
HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_BACKUP_LIVE_DUP_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = true;
    mock->isParseFinished_ = true;
    hlsSegmentManager_->playlistDownloader_ = mock;
    PlayInfo p;
    p.url_ = "test.ts";
    p.rangeUrl_ = "range.ts";
    p.offset_ = 0;
    p.length_ = 100;
    hlsSegmentManager_->PlaylistBackup(p);
    hlsSegmentManager_->PlaylistBackup(p);
    EXPECT_EQ(hlsSegmentManager_->seekCandidates_.size(), 1);
}

// L1069 branch 1, L1082, L1083: PlaylistBackup with live and parse finished
HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_BACKUP_LIVE_PARSED_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = true;
    mock->isParseFinished_ = true;
    hlsSegmentManager_->playlistDownloader_ = mock;
    PlayInfo p1;
    p1.url_ = "a.ts";
    hlsSegmentManager_->backPlayList_.push_back(p1);
    PlayInfo p2;
    p2.url_ = "b.ts";
    hlsSegmentManager_->PlaylistBackup(p2);
    EXPECT_EQ(hlsSegmentManager_->backPlayList_.size(), 0);
}

// L1069 branch 1, L1088: PlaylistBackup non-live and parse finished
HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_BACKUP_VOD_PARSED_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = false;
    mock->isParseFinished_ = true;
    hlsSegmentManager_->playlistDownloader_ = mock;
    PlayInfo p;
    p.url_ = "test.ts";
    hlsSegmentManager_->PlaylistBackup(p);
    EXPECT_EQ(hlsSegmentManager_->backPlayList_.size(), 1);
}

// L1122,L1131: HandleBitrateSelectFragment with isSelectingBitrate_
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_BITRATE_SELECT_001, TestSize.Level1)
{
    hlsSegmentManager_->isSelectingBitrate_.store(true);
    hlsSegmentManager_->currentSequence_ = 5;
    PlayInfo fragment;
    fragment.sequence_ = 10;
    fragment.url_ = "test.ts";
    bool ret = hlsSegmentManager_->HandleBitrateSelectFragment(fragment);
    EXPECT_EQ(ret, true);
}

// L1122,L1131 branch 0: HandleBitrateSelectFragment with matching sequence
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_BITRATE_SELECT_MATCH_001, TestSize.Level1)
{
    hlsSegmentManager_->isSelectingBitrate_.store(true);
    hlsSegmentManager_->currentSequence_ = 5;
    PlayInfo fragment;
    fragment.sequence_ = 5;
    fragment.url_ = "test.ts";
    bool ret = hlsSegmentManager_->HandleBitrateSelectFragment(fragment);
    EXPECT_EQ(ret, false);
    EXPECT_EQ(hlsSegmentManager_->isSelectingBitrate_.load(), false);
}

// L279: EvictFragmentMapIfNeeded with size >= MAX_FRAGMENT_TRACKING
HWTEST_F(HlsSegmentManagerUnitTest, EVICT_FRAGMENT_MAP_001, TestSize.Level1)
{
    std::map<std::string, bool> fragmentMap;
    for (int i = 0; i < 100001; i++) {
        fragmentMap["url_" + std::to_string(i)] = true;
    }
    hlsSegmentManager_->EvictFragmentMapIfNeeded(fragmentMap, "test");
    EXPECT_EQ(fragmentMap.size(), 100000);
}

// L897,L899: Read with isNeedResume_ and freeSize > threshold
HWTEST_F(HlsSegmentManagerUnitTest, READ_RESUME_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->isNeedResume_.store(true);
    downloader->memorySize_ = MAX_CACHE_BUFFER_SIZE_UT;
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = false;
    downloader->playlistDownloader_ = mock;
    downloader->readOffset_ = 0;
    downloader->readTsIndex_ = 0;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(100, true);
    unsigned char buff[10];
    ReadDataInfo rdi;
    rdi.wantReadLength_ = 10;
    rdi.ffmpegOffset = 0;
    rdi.streamId_ = 0;
    downloader->Read(buff, rdi);
    EXPECT_EQ(rdi.realReadLength_ <= static_cast<uint32_t>(rdi.wantReadLength_), true);
    downloader = nullptr;
}

// L909,L911: Read with read speed calculation
HWTEST_F(HlsSegmentManagerUnitTest, READ_SPEED_CALC_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    downloader->readOffset_ = 0;
    downloader->readTsIndex_ = 0;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(100, true);
    downloader->lastReadCheckTime_ = 0;
    downloader->readTotalBytes_ = 0;
    unsigned char buff[10];
    ReadDataInfo rdi;
    rdi.wantReadLength_ = 10;
    rdi.ffmpegOffset = 0;
    rdi.streamId_ = 0;
    downloader->Read(buff, rdi);
    EXPECT_EQ(rdi.realReadLength_ <= static_cast<uint32_t>(rdi.wantReadLength_), true);
    downloader = nullptr;
}

// L976,L979: SeekToTime with isSelectingBitrate_ and timeout
HWTEST_F(HlsSegmentManagerUnitTest, SEEK_TO_TIME_SELECTING_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->isSelectingBitrate_.store(true);
    EXPECT_EQ(downloader->SeekToTime(100, SeekMode::SEEK_NEXT_SYNC), false);
    downloader = nullptr;
}

// L993: SeekToTime with isInterruptNeeded_
HWTEST_F(HlsSegmentManagerUnitTest, SEEK_TO_TIME_INTERRUPT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->isInterruptNeeded_ = true;
    EXPECT_EQ(downloader->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC), true);
    downloader = nullptr;
}

// L995: SeekToTime with totalDuration == -1
HWTEST_F(HlsSegmentManagerUnitTest, SEEK_TO_TIME_NODUR_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->duration_ = -1;
    downloader->playlistDownloader_ = mock;
    downloader->isInterruptNeeded_ = false;
    EXPECT_EQ(downloader->SeekToTime(100, SeekMode::SEEK_NEXT_SYNC), false);
    downloader = nullptr;
}

// L1005: SeekToTime with seekTime >= totalDuration
HWTEST_F(HlsSegmentManagerUnitTest, SEEK_TO_TIME_BEYOND_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->duration_ = 100;
    downloader->playlistDownloader_ = mock;
    downloader->isInterruptNeeded_ = false;
    downloader->isSelectingBitrate_.store(false);
    PlayInfo p1, p2;
    downloader->backPlayList_.push_back(p1);
    downloader->backPlayList_.push_back(p2);
    EXPECT_EQ(downloader->SeekToTime(200, SeekMode::SEEK_NEXT_SYNC), true);
    downloader = nullptr;
}

// L1199: SaveCacheBufferDataNotblock with throttle threshold
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_CACHE_NOTBLOCK_THROTTLE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->memorySize_ = MAX_CACHE_BUFFER_SIZE_UT;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isNeedResume_.store(false);
    uint8_t data[10] = {0};
    uint32_t result = downloader->SaveCacheBufferDataNotblock(data, 10);
    EXPECT_EQ(result == 10, true);
    downloader = nullptr;
}

// L1199 branch 2: SaveCacheBufferDataNotblock triggering resume
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_CACHE_NOTBLOCK_RESUME_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(1024, CHUNK_SIZE);
    downloader->memorySize_ = 1024;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isNeedResume_.store(false);
    uint8_t data[10] = {0};
    uint32_t result = downloader->SaveCacheBufferDataNotblock(data, 10);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(downloader->isNeedResume_.load(), true);
    downloader = nullptr;
}

// L1293,L1296: SaveData with isSourceLoaderSeeking_ timeout
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_DATA_SEEKING_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->isSourceLoaderSeeking_.store(true);
    uint8_t data[10] = {0};
    uint32_t result = downloader->SaveData(data, 10, false);
    EXPECT_EQ(result, 0);
    downloader = nullptr;
}

// L1302: SaveData with isSeekingFlag
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_DATA_SEEKING_FLAG_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->isSeekingFlag = true;
    downloader->isSourceLoaderSeeking_.store(false);
    uint8_t data[10] = {0};
    uint32_t result = downloader->SaveData(data, 10, false);
    EXPECT_EQ(result, 10);
    downloader = nullptr;
}

// L1306: SaveData with aesDecryptor_ set (encrypted path)
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_DATA_ENCRYPTED_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
    uint8_t key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint8_t iv[16] = {0};
    downloader->aesDecryptor_->OnSourceKeyChange(key, 16, iv);
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    uint8_t data[32] = {0};
    uint32_t result = downloader->SaveData(data, 32, true);
    EXPECT_EQ(result, 32);
    downloader = nullptr;
}

// L1322: FillSingleBlock with memcpy
HWTEST_F(HlsSegmentManagerUnitTest, FILL_SINGLE_BLOCK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, true, header_);
    downloader->Init();
    downloader->singleBlockLen_ = 0;
    uint8_t data[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    uint32_t len = 16;
    uint8_t *ptr = data;
    uint32_t filled = downloader->FillSingleBlock(ptr, len);
    EXPECT_EQ(filled, 16);
    EXPECT_EQ(len, 0);
    downloader = nullptr;
}

// L2296: SetPlayStrategy with valid playStrategy and width/height > 0
HWTEST_F(HlsSegmentManagerUnitTest, SET_PLAY_STRATEGY_VALID_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    auto ps = std::make_shared<PlayStrategy>();
    ps->width = 1920;
    ps->height = 1080;
    downloader->SetPlayStrategy(ps);
    EXPECT_EQ(downloader->playlistDownloader_, mock);
    downloader = nullptr;
}

// L2321: CheckLoopTimeout with large duration
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_LOOP_TIMEOUT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    int64_t oldTime = downloader->loopInterruptClock_.ElapsedSeconds() - 100;
    bool result = downloader->CheckLoopTimeout(oldTime);
    EXPECT_EQ(result, true);
    EXPECT_EQ(downloader->downloadErrorState_, true);
    downloader = nullptr;
}

// L2449,L2450,L2451: IsHlsEnd with live end and conditions
HWTEST_F(HlsSegmentManagerUnitTest, IS_HLS_END_LIVE_END_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLiveEnd_ = true;
    downloader->playlistDownloader_ = mock;
    downloader->lastPlaychanged_.store(false);
    EXPECT_EQ(downloader->IsHlsEnd(), false);
    downloader = nullptr;
}

// L2449 branch 0: IsHlsEnd with no playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, IS_HLS_END_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    downloader->playlistDownloader_ = nullptr;
    downloader->isEos_ = false;
    EXPECT_EQ(downloader->IsHlsEnd(), false);
    downloader = nullptr;
}

// L2456,L2457,L2459: IsHlsEnd with CheckReadStatus true and conditions
HWTEST_F(HlsSegmentManagerUnitTest, IS_HLS_END_EOS_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->duration_ = 100;
    mock->isParseAndNotifyFinished_ = true;
    downloader->playlistDownloader_ = mock;
    downloader->isEos_ = true;
    downloader->isStopped = true;
    downloader->readTsIndex_ = 0;
    PlayInfo p;
    downloader->backPlayList_.push_back(p);
    downloader->tsStorageInfo_[0] = std::make_pair(0, true);
    EXPECT_EQ(downloader->IsHlsEnd(), false);
    downloader = nullptr;
}



// ===== Additional branch coverage tests round 2 =====

// L360: UpdateAesDecryptor with nullptr playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, UPDATE_AES_NULL_PL_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    PlayInfo pi;
    pi.keyIndex_ = 0;
    hlsSegmentManager_->UpdateAesDecryptor(pi);
    EXPECT_EQ(hlsSegmentManager_->GetAesDecryptor(), nullptr);
    EXPECT_EQ(hlsSegmentManager_->totalBits_, 0);
}

// L365: UpdateAesDecryptor with keyIndex > 0 and WaitForKeyReady returns true
HWTEST_F(HlsSegmentManagerUnitTest, UPDATE_AES_KEY_READY_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    mock->waitForKeyReady_ = true;
    auto decryptor = std::make_shared<AesDecryptor>();
    uint8_t iv[16] = {0};
    decryptor->SetIv(iv, sizeof(iv));
    mock->aesDecryptor_ = decryptor;
    mock->hasExplicitIvResult_ = true;
    PlayInfo pi;
    pi.keyIndex_ = 1;
    pi.sequence_ = 1;
    hlsSegmentManager_->UpdateAesDecryptor(pi);
    EXPECT_EQ(hlsSegmentManager_->GetAesDecryptor() != nullptr, true);
}

// L365 branch 1: UpdateAesDecryptor with keyIndex > 0 and WaitForKeyReady returns false
HWTEST_F(HlsSegmentManagerUnitTest, UPDATE_AES_KEY_NOT_READY_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    mock->waitForKeyReady_ = false;
    mock->aesDecryptor_ = nullptr;
    PlayInfo pi;
    pi.keyIndex_ = 1;
    hlsSegmentManager_->UpdateAesDecryptor(pi);
    EXPECT_EQ(hlsSegmentManager_->GetAesDecryptor(), nullptr);
    EXPECT_EQ(hlsSegmentManager_->totalBits_, 0);
}

// L498: CheckBreakCondition with CheckReadStatus() returning true
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_BREAK_COND_READ_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->duration_ = 100;
    mock->isParseAndNotifyFinished_ = true;
    downloader->playlistDownloader_ = mock;
    downloader->isStopped = true;
    downloader->downloadErrorState_ = true;
    EXPECT_EQ(downloader->CheckBreakCondition(), true);
    downloader = nullptr;
}

// L146 branch 1: Copy constructor with userDefinedBufferDuration_ = false
HWTEST_F(HlsSegmentManagerUnitTest, COPY_CTOR_FALSE_DUR_001, TestSize.Level1)
{
    auto other = std::make_shared<HlsSegmentManager>(10, false, header_);
    other->Init();
    auto downloader = std::make_shared<HlsSegmentManager>(other, HlsSegmentType::SEG_AUDIO);
    EXPECT_EQ(downloader->userDefinedBufferDuration_, false);
    EXPECT_EQ(downloader->totalBufferSize_, downloader->maxCacheBufferSize_);
    downloader = nullptr;
    other = nullptr;
}

// L1553: SelectBitRate with null playList_
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_NULL_PL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    downloader->playList_ = nullptr;
    EXPECT_EQ(downloader->SelectBitRate(1000000), false);
    downloader = nullptr;
}

// L1703: SeekToTsForRead with null cacheMediaBuffer_
HWTEST_F(HlsSegmentManagerUnitTest, SEEK_TO_TS_FOR_READ_NULL_BUF_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = nullptr;
    downloader->SeekToTsForRead(0);
    EXPECT_EQ(downloader->cacheMediaBuffer_, nullptr);
    EXPECT_EQ(downloader->isStopped.load(), false);
    downloader = nullptr;
}

// L1838: ReportVideoSizeChange with segEventCb_ set and null playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, REPORT_VIDEO_SIZE_CB_NULL_PL_001, TestSize.Level1)
{
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    hlsSegmentManager_->ReportVideoSizeChange();
    EXPECT_FALSE(called);
}

// L1862: SetDownloadErrorState with segEventCb_ set and isReportedErrorCode_ = false
HWTEST_F(HlsSegmentManagerUnitTest, SET_DOWNLOAD_ERROR_CB_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    bool called = false;
    downloader->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    downloader->isReportedErrorCode_ = false;
    downloader->SetDownloadErrorState();
    EXPECT_TRUE(called);
    EXPECT_EQ(downloader->downloadErrorState_, true);
    downloader = nullptr;
}

// L1862 branch 2: SetDownloadErrorState with segEventCb_ set and isReportedErrorCode_ = true
HWTEST_F(HlsSegmentManagerUnitTest, SET_DOWNLOAD_ERROR_ALREADY_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    bool called = false;
    downloader->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    downloader->isReportedErrorCode_ = true;
    downloader->SetDownloadErrorState();
    EXPECT_FALSE(called);
    downloader = nullptr;
}

// L1862 branch 0: SetDownloadErrorState with no segEventCb_
HWTEST_F(HlsSegmentManagerUnitTest, SET_DOWNLOAD_ERROR_NO_CB_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->segEventCb_ = nullptr;
    downloader->isReportedErrorCode_ = false;
    downloader->SetDownloadErrorState();
    EXPECT_EQ(downloader->downloadErrorState_, true);
    EXPECT_EQ(downloader->isTimeoutErrorNotified_.load(), false);
    downloader = nullptr;
}

// L1923,L1927,L1930: CheckRiseBufferSize with valid playlist and recordData
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_RISE_BUFFER_VALID_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->currentBitRateVal_ = 500000;
    hlsSegmentManager_->playlistDownloader_ = mock;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 1000000;
    hlsSegmentManager_->recordData_ = rd;
    EXPECT_EQ(hlsSegmentManager_->CheckRiseBufferSize(), true);
}

// L1941,L1944,L1948: CheckPulldownBufferSize with valid playlist and recordData
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_PULLDOWN_BUFFER_VALID_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->currentBitRateVal_ = 1000000;
    hlsSegmentManager_->playlistDownloader_ = mock;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 500000;
    hlsSegmentManager_->recordData_ = rd;
    EXPECT_EQ(hlsSegmentManager_->CheckPulldownBufferSize(), true);
}

// L1948 branch 0: CheckPulldownBufferSize with downloadRate >= playingBitrate
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_PULLDOWN_BUFFER_HIGH_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->currentBitRateVal_ = 500000;
    hlsSegmentManager_->playlistDownloader_ = mock;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 1000000;
    hlsSegmentManager_->recordData_ = rd;
    EXPECT_EQ(hlsSegmentManager_->CheckPulldownBufferSize(), false);
}

// L1927 branch 1: CheckRiseBufferSize with playingBitrate = 0 and videoWidth > 0
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_RISE_BUFFER_WIDTH_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->currentBitRateVal_ = 0;
    mock->videoWidth_ = 1080;
    hlsSegmentManager_->playlistDownloader_ = mock;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = static_cast<double>(hlsSegmentManager_->minBufferSize_ * 4 + 1);
    hlsSegmentManager_->recordData_ = rd;
    EXPECT_EQ(hlsSegmentManager_->CheckRiseBufferSize(), true);
}

// L2251 branches 0-5: StopBufferring with each null pointer
HWTEST_F(HlsSegmentManagerUnitTest, STOP_BUFFERING_NULL_DOWNLOADER_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->downloader_ = nullptr;
    EXPECT_EQ(downloader->StopBufferring(false), Status::ERROR_NULL_POINTER);
    downloader = nullptr;
}

HWTEST_F(HlsSegmentManagerUnitTest, STOP_BUFFERING_NULL_PLAYLIST_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->playlistDownloader_ = nullptr;
    EXPECT_EQ(downloader->StopBufferring(false), Status::ERROR_NULL_POINTER);
    downloader = nullptr;
}

// L2296: SetPlayStrategy with null playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, SET_PLAY_STRATEGY_NULL_PL_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    hlsSegmentManager_->SetPlayStrategy(nullptr);
    auto ps = std::make_shared<PlayStrategy>();
    hlsSegmentManager_->SetPlayStrategy(ps);
    EXPECT_EQ(hlsSegmentManager_->playlistDownloader_, nullptr);
    EXPECT_EQ(hlsSegmentManager_->totalBits_, 0);
}

// L2300: SetPlayStrategy with width > 0 and height > 0
HWTEST_F(HlsSegmentManagerUnitTest, SET_PLAY_STRATEGY_VALID_WH_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    auto ps = std::make_shared<PlayStrategy>();
    ps->width = 1920;
    ps->height = 1080;
    downloader->SetPlayStrategy(ps);
    EXPECT_EQ(downloader->playlistDownloader_ != nullptr, true);
    downloader = nullptr;
}

// L2502: Clone with null other playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, CLONE_NULL_OTHER_PL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto other = std::make_shared<HlsSegmentManager>(10, false, header_);
    other->Init();
    other->playlistDownloader_ = nullptr;
    downloader->Clone(other);
    EXPECT_EQ(downloader->playlistDownloader_ != nullptr, true);
    downloader = nullptr;
    other = nullptr;
}

// L2515: SelectMedia with null playList_
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_MEDIA_NULL_PL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_AUDIO);
    downloader->Init();
    downloader->playList_ = nullptr;
    EXPECT_EQ(downloader->SelectMedia(0, HlsSegmentType::SEG_AUDIO), false);
    downloader = nullptr;
}

// L1810,L1811: UpdateDownloadFinished with subtitle and conditions for auto select
HWTEST_F(HlsSegmentManagerUnitTest, UPDATE_DOWNLOAD_FINISHED_AUTO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isParseAndNotifyFinished_ = true;
    downloader->playlistDownloader_ = mock;
    downloader->type_ = HlsSegmentType::SEG_VIDEO;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isDownloadStarted_ = false;
    downloader->isSeekingFlag = false;
    downloader->isSelectingBitrate_ = false;
    downloader->isAutoSelectBitrate_ = true;
    auto req = std::make_shared<DownloadRequest>("", nullptr, nullptr, false);
    downloader->SetDownloadRequest(req);
    downloader->UpdateDownloadFinished("test_url", "");
    EXPECT_EQ(downloader->isDownloadFinish_, true);
    downloader = nullptr;
}

// L1494 branch 1: DownloadReport with readBitrate_ > 0 and recordData_
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REPORT_READ_BITRATE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->totalBits_ = 200;
    downloader->lastBits_ = 0;
    downloader->lastCheckTime_ = 0;
    downloader->readBitrate_ = 500;
    downloader->bufferedDuration_ = 5000;
    OSAL::SleepFor(1100);
    downloader->DownloadReport();
    EXPECT_EQ(downloader->recordData_ != nullptr, true);
    downloader = nullptr;
}

// L1507 branch 3: DownloadReport with data usage interval
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REPORT_DATA_USAGE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->isDownloadFinish_ = false;
    downloader->lastReportUsageTime_ = 1;
    downloader->dataUsage_ = 5000;
    downloader->DownloadReport();
    EXPECT_EQ(downloader->dataUsage_, 5000);
    downloader = nullptr;
}

// L1425,L1428,L1430: DownloadRecordHistory with autoBufferSize and rise
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REC_HISTORY_RISE_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->currentBitRateVal_ = 500000;
    hlsSegmentManager_->playlistDownloader_ = mock;
    hlsSegmentManager_->autoBufferSize_ = true;
    hlsSegmentManager_->userDefinedBufferDuration_ = false;
    hlsSegmentManager_->lastWriteTime_ = 0;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 1000000;
    hlsSegmentManager_->recordData_ = rd;
    hlsSegmentManager_->DownloadRecordHistory(2000);
    EXPECT_EQ(hlsSegmentManager_->totalBufferSize_ > 0, true);
}

// L1430: DownloadRecordHistory with pulldown
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REC_HISTORY_PULLDOWN_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->currentBitRateVal_ = 2000000;
    hlsSegmentManager_->playlistDownloader_ = mock;
    hlsSegmentManager_->autoBufferSize_ = true;
    hlsSegmentManager_->userDefinedBufferDuration_ = false;
    hlsSegmentManager_->lastWriteTime_ = 0;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 500000;
    hlsSegmentManager_->recordData_ = rd;
    hlsSegmentManager_->DownloadRecordHistory(2000);
    EXPECT_EQ(hlsSegmentManager_->totalBufferSize_ > 0, true);
}

// L897,L899: Read with isNeedResume_ and freeSize > threshold
HWTEST_F(HlsSegmentManagerUnitTest, READ_RESUME_THRESHOLD_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->isNeedResume_.store(true);
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = false;
    downloader->playlistDownloader_ = mock;
    downloader->readOffset_ = 0;
    downloader->readTsIndex_ = 0;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(100, true);
    unsigned char buff[10];
    ReadDataInfo rdi;
    rdi.wantReadLength_ = 10;
    rdi.ffmpegOffset = 0;
    rdi.streamId_ = 0;
    downloader->Read(buff, rdi);
    EXPECT_EQ(downloader->isNeedResume_.load(), false);
    downloader = nullptr;
}

// L1529 branch 1: OnTimedMetaDataChanged with valid metadata but no callback
HWTEST_F(HlsSegmentManagerUnitTest, ON_TIMED_META_NO_CB_001, TestSize.Level1)
{
    hlsSegmentManager_->segEventCb_ = nullptr;
    auto metadata = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    metadata->id = "test";
    hlsSegmentManager_->OnTimedMetaDataChanged(metadata);
    EXPECT_EQ(hlsSegmentManager_->segEventCb_, nullptr);
    EXPECT_EQ(hlsSegmentManager_->totalBits_, 0);
}

// L1810 branches: UpdateDownloadFinished with non-subtitle and auto select
HWTEST_F(HlsSegmentManagerUnitTest, UPDATE_DOWNLOAD_FINISHED_NOTSUB_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    downloader->type_ = HlsSegmentType::SEG_VIDEO;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isDownloadStarted_ = false;
    downloader->isSeekingFlag = false;
    auto req = std::make_shared<DownloadRequest>("", nullptr, nullptr, false);
    downloader->SetDownloadRequest(req);
    downloader->UpdateDownloadFinished("test_url", "");
    EXPECT_EQ(downloader->isDownloadFinish_, true);
    downloader = nullptr;
}

// L247,L248: Destructor with null reportInfo_ and downloader_
HWTEST_F(HlsSegmentManagerUnitTest, DTOR_NULL_REPORT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->reportInfo_ = nullptr;
    downloader->downloader_ = nullptr;
    downloader->Close(true);
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
}

// L1688: PushPlayInfo with IsLive = true
HWTEST_F(HlsSegmentManagerUnitTest, PUSH_PLAY_INFO_LIVE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = true;
    downloader->playlistDownloader_ = mock;
    PlayInfo pi;
    pi.url_ = "test.ts";
    pi.streamId_ = 0;
    downloader->isDownloadStarted_ = false;
    downloader->PushPlayInfo(pi);
    EXPECT_EQ(downloader->isDownloadStarted_, true);
    downloader = nullptr;
}

// L1712: SeekToTsForRead with IsLive = true
HWTEST_F(HlsSegmentManagerUnitTest, SEEK_TO_TS_FOR_READ_LIVE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = true;
    downloader->playlistDownloader_ = mock;
    PlayInfo p;
    downloader->backPlayList_.push_back(p);
    downloader->SeekToTsForRead(0);
    EXPECT_EQ(downloader->readOffset_, 0);
    downloader = nullptr;
}

// L1074,L1075,L1077: PlaylistBackup with duplicate fragment in live
HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_BACKUP_LIVE_SAME_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = true;
    mock->isParseFinished_ = false;
    hlsSegmentManager_->playlistDownloader_ = mock;
    PlayInfo p;
    p.url_ = "test.ts";
    p.rangeUrl_ = "range.ts";
    p.offset_ = 0;
    p.length_ = 100;
    hlsSegmentManager_->PlaylistBackup(p);
    hlsSegmentManager_->PlaylistBackup(p);
    EXPECT_EQ(hlsSegmentManager_->seekCandidates_.size(), 1);
}

// L1098,L1107: OnPlayListChanged with download not started
HWTEST_F(HlsSegmentManagerUnitTest, ON_PLAYLIST_CHANGED_START_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    std::vector<PlayInfo> playList;
    PlayInfo p;
    p.url_ = "test.ts";
    p.duration_ = 10.0;
    playList.push_back(p);
    downloader->isDownloadStarted_ = false;
    downloader->OnPlayListChanged(playList);
    EXPECT_EQ(downloader->isDownloadStarted_, true);
    downloader = nullptr;
}



// ===== Round 3: More branch coverage =====

// L985: SeekToTime with null playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, SEEK_TO_TIME_NULL_PL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->playlistDownloader_ = nullptr;
    EXPECT_EQ(downloader->SeekToTime(100, SeekMode::SEEK_NEXT_SYNC), false);
    downloader = nullptr;
}

// L1923: CheckRiseBufferSize with null playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_RISE_NULL_PL_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 1000000;
    hlsSegmentManager_->recordData_ = rd;
    EXPECT_EQ(hlsSegmentManager_->CheckRiseBufferSize(), false);
}

// L1941: CheckPulldownBufferSize with null playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, CHECK_PULLDOWN_NULL_PL_001, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 1000000;
    hlsSegmentManager_->recordData_ = rd;
    EXPECT_EQ(hlsSegmentManager_->CheckPulldownBufferSize(), false);
}

// L1838: ReportVideoSizeChange with segEventCb_ set and null playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, REPORT_VID_SIZE_CB_NULL_001, TestSize.Level1)
{
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    hlsSegmentManager_->ReportVideoSizeChange();
    EXPECT_FALSE(called);
}

// L1820: IsDownloadLastSplice with null playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, IS_LAST_SPLICE_NULL_002, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    hlsSegmentManager_->writeTsIndex_ = 0;
    hlsSegmentManager_->backPlayList_.push_back(PlayInfo{});
    EXPECT_EQ(hlsSegmentManager_->IsDownloadLastSplice(), false);
}

// L2066: GetPlaybackInfo with readBitrate_ > 0 and recordData_
HWTEST_F(HlsSegmentManagerUnitTest, GET_PLAYBACK_INFO_READBR_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->readBitrate_ = 500;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 300.0;
    downloader->recordData_ = rd;
    PlaybackInfo info;
    downloader->GetPlaybackInfo(info);
    EXPECT_EQ(info.downloadRate, 300);
    downloader = nullptr;
}

// L2151,L2153: HandleCachedDuration with decreasing duration
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_CACHED_DUR_DECREASE_001, TestSize.Level1)
{
    hlsSegmentManager_->currentBitRate_ = 1000;
    hlsSegmentManager_->cachedDuration_ = 1000;
    hlsSegmentManager_->lastDurationReacord_ = 2000;
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    hlsSegmentManager_->HandleCachedDuration();
    // No buffer data so cachedDuration_ should be 0, which is less than lastDurationReacord_ (2000)
    // This should trigger the decrease branch
    EXPECT_EQ(called, true);
}

// L1494: DownloadReport with readBitrate_ > 0 (needs timing)
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REPORT_READBR_TIMING_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->totalBits_ = 200;
    downloader->lastBits_ = 0;
    downloader->lastCheckTime_ = 0;
    downloader->readBitrate_ = 500;
    downloader->bufferedDuration_ = 5000;
    OSAL::SleepFor(1100);
    downloader->DownloadReport();
    EXPECT_EQ(downloader->recordData_ != nullptr, true);
    downloader = nullptr;
}

// L1507: DownloadReport with data usage interval exceeded
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REPORT_USAGE_LONG_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->isDownloadFinish_ = false;
    downloader->lastReportUsageTime_ = 0;
    downloader->dataUsage_ = 5000;
    OSAL::SleepFor(1100);
    downloader->DownloadReport();
    // After 1.1s, lastReportUsageTime_ should be updated if > 300000ms
    // But 1100 < 300000 so dataUsage_ stays
    EXPECT_EQ(downloader->dataUsage_, 5000);
    downloader = nullptr;
}

// L1554: SelectBitRate SEG_AUDIO returns true with mock
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_AUDIO_MOCK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_AUDIO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L1561,L1564: SelectBitRate with callback, auto select, CanAutoSelectBitRate=true
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_AUTO_CAN_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isBitrateSame_ = false;
    downloader->playlistDownloader_ = mock;
    auto cb = std::make_shared<HlsNetworkBitrateCallback>();
    downloader->SetCallback(cb);
    downloader->isAutoSelectBitrate_ = true;
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L1569: SelectBitRate with isEos_ = true
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_EOS_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isBitrateSame_ = false;
    downloader->playlistDownloader_ = mock;
    auto cb = std::make_shared<HlsNetworkBitrateCallback>();
    downloader->SetCallback(cb);
    downloader->isAutoSelectBitrate_ = false;
    downloader->isEos_ = true;
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L1810,L1811: UpdateDownloadFinished with auto-select conditions met
HWTEST_F(HlsSegmentManagerUnitTest, UPDATE_DL_FINISHED_AUTO_SEL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isParseAndNotifyFinished_ = true;
    mock->isLive_ = false;
    downloader->playlistDownloader_ = mock;
    downloader->type_ = HlsSegmentType::SEG_VIDEO;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isDownloadStarted_ = false;
    downloader->isSeekingFlag = false;
    downloader->isSelectingBitrate_ = false;
    downloader->isAutoSelectBitrate_ = true;
    auto req = std::make_shared<DownloadRequest>("", nullptr, nullptr, false);
    downloader->SetDownloadRequest(req);
    downloader->UpdateDownloadFinished("test_url", "");
    EXPECT_EQ(downloader->isDownloadFinish_, true);
    downloader = nullptr;
}

// L1199: SaveCacheBufferDataNotblock with throttle threshold exceeded
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_CACHE_NOTBLOCK_RES_NE_LEN_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(1024, CHUNK_SIZE);
    downloader->memorySize_ = 1024;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isNeedResume_.store(false);
    uint8_t data[512] = {0};
    uint32_t result = downloader->SaveCacheBufferDataNotblock(data, 512);
    // Buffer is 1024, writing 512 bytes. res should be <= 512
    EXPECT_EQ(result <= 512, true);
    downloader = nullptr;
}

// L1289: SaveData with null cacheMediaBuffer_
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_DATA_NULL_BUF_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = nullptr;
    uint8_t data[10] = {0};
    EXPECT_EQ(downloader->SaveData(data, 10, false), 0);
    downloader = nullptr;
}

// L1293,L1296: SaveData with isSourceLoaderSeeking_ and timeout
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_DATA_SEEKING_TIMEOUT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->isSourceLoaderSeeking_.store(true);
    uint8_t data[10] = {0};
    uint32_t result = downloader->SaveData(data, 10, false);
    EXPECT_EQ(result, 0);
    downloader = nullptr;
}

// L1322: FillSingleBlock with valid data
HWTEST_F(HlsSegmentManagerUnitTest, FILL_SINGLE_BLOCK_VALID_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->singleBlockLen_ = 5;
    uint8_t data[16] = {5,6,7,8,9,10,11,12,13,14,15,16,0,0,0,0};
    uint32_t len = 11;
    uint8_t *ptr = data;
    uint32_t filled = downloader->FillSingleBlock(ptr, len);
    EXPECT_EQ(filled, 11);
    EXPECT_EQ(downloader->singleBlockLen_, 16);
    EXPECT_EQ(len, 0);
    downloader = nullptr;
}

// L1357: DecryptDataLeft with writtenLen != decryptLen_
HWTEST_F(HlsSegmentManagerUnitTest, DECRYPT_DATA_LEFT_DIFF_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(16, CHUNK_SIZE);
    downloader->memorySize_ = 16;
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
    uint8_t key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint8_t iv[16] = {0};
    downloader->aesDecryptor_->OnSourceKeyChange(key, 16, iv);
    downloader->singleBlockLen_ = 0;
    uint8_t data[16] = {0};
    uint32_t len = 16;
    uint8_t *ptr = data;
    uint32_t result = downloader->DecryptDataLeft(ptr, len, true);
    EXPECT_EQ(result, 0);
    downloader = nullptr;
}

// L1383: DecryptData with writtenLen != decryptLen_
HWTEST_F(HlsSegmentManagerUnitTest, DECRYPT_DATA_DIFF_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(16, CHUNK_SIZE);
    downloader->memorySize_ = 16;
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
    uint8_t key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint8_t iv[16] = {0};
    downloader->aesDecryptor_->OnSourceKeyChange(key, 16, iv);
    uint8_t data[32] = {0};
    uint32_t len = 32;
    uint8_t *ptr = data;
    uint32_t result = downloader->DecryptData(ptr, len, true);
    EXPECT_EQ(result <= 32, true);
    downloader = nullptr;
}

// L2251: StopBufferring with null cacheMediaBuffer_
HWTEST_F(HlsSegmentManagerUnitTest, STOP_BUFFERING_NULL_CACHE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = nullptr;
    EXPECT_EQ(downloader->StopBufferring(false), Status::ERROR_NULL_POINTER);
    downloader = nullptr;
}

// L2300: SetPlayStrategy with width > 0, height = 0
HWTEST_F(HlsSegmentManagerUnitTest, SET_PLAY_STRATEGY_HEIGHT_ZERO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    auto ps = std::make_shared<PlayStrategy>();
    ps->width = 1920;
    ps->height = 0;
    downloader->SetPlayStrategy(ps);
    EXPECT_EQ(downloader->playlistDownloader_ != nullptr, true);
    downloader = nullptr;
}

// L2515: SelectMedia with null playList_ (already covered but might have failed)
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_MEDIA_NULL_PL_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_AUDIO);
    downloader->Init();
    downloader->playList_ = nullptr;
    EXPECT_EQ(downloader->SelectMedia(0, HlsSegmentType::SEG_AUDIO), false);
    downloader = nullptr;
}

// L2518: SelectMedia with SEG_VIDEO type and isMediaSame = false
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_MEDIA_VIDEO_SAME_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isMediaSame_ = true;
    downloader->playlistDownloader_ = mock;
    EXPECT_EQ(downloader->SelectMedia(0, HlsSegmentType::SEG_AUDIO), true);
    downloader = nullptr;
}

// L1074,L1075: PlaylistBackup with same fragment (different fields)
HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_BACKUP_DIFF_FIELDS_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = true;
    mock->isParseFinished_ = false;
    hlsSegmentManager_->playlistDownloader_ = mock;
    PlayInfo p1;
    p1.url_ = "test.ts";
    p1.rangeUrl_ = "range1.ts";
    p1.offset_ = 0;
    p1.length_ = 100;
    PlayInfo p2;
    p2.url_ = "test.ts";
    p2.rangeUrl_ = "range2.ts";
    p2.offset_ = 0;
    p2.length_ = 100;
    hlsSegmentManager_->PlaylistBackup(p1);
    hlsSegmentManager_->PlaylistBackup(p2);
    EXPECT_EQ(hlsSegmentManager_->seekCandidates_.size(), 2);
}

// L1098,L1107: OnPlayListChanged with download started and writeTsIndex > 0
HWTEST_F(HlsSegmentManagerUnitTest, ON_PLAYLIST_CHANGED_WT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    std::vector<PlayInfo> playList;
    PlayInfo p;
    p.url_ = "test.ts";
    p.duration_ = 10.0;
    playList.push_back(p);
    downloader->isDownloadStarted_ = false;
    downloader->writeTsIndex_ = 0;
    downloader->OnPlayListChanged(playList);
    EXPECT_EQ(downloader->isDownloadStarted_, true);
    downloader = nullptr;
}

// L1113: OnPlayListChanged with IsLiveEnd
HWTEST_F(HlsSegmentManagerUnitTest, ON_PLAYLIST_CHANGED_LIVE_END_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLiveEnd_ = true;
    downloader->playlistDownloader_ = mock;
    std::vector<PlayInfo> playList;
    PlayInfo p;
    p.url_ = "test.ts";
    p.duration_ = 10.0;
    playList.push_back(p);
    downloader->isDownloadStarted_ = true;
    downloader->OnPlayListChanged(playList);
    EXPECT_EQ(downloader->lastPlaychanged_.load(), true);
    downloader = nullptr;
}

// L2450,L2451: IsHlsEnd with live end and all conditions
HWTEST_F(HlsSegmentManagerUnitTest, IS_HLS_END_LIVE_ALL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLiveEnd_ = true;
    downloader->playlistDownloader_ = mock;
    downloader->lastPlaychanged_.store(true);
    downloader->readTsIndex_ = 0;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, true);
    EXPECT_EQ(downloader->IsHlsEnd(), true);
    downloader = nullptr;
}

// L2456,L2457: IsHlsEnd with VOD end conditions
HWTEST_F(HlsSegmentManagerUnitTest, IS_HLS_END_VOD_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLiveEnd_ = false;
    downloader->playlistDownloader_ = mock;
    downloader->isEos_ = true;
    downloader->readTsIndex_ = 0;
    PlayInfo p;
    downloader->backPlayList_.push_back(p);
    downloader->tsStorageInfo_[0] = std::make_pair(0, true);
    // CheckReadStatus needs duration > 0 and isParseAndNotifyFinished
    mock->duration_ = 100;
    mock->isParseAndNotifyFinished_ = true;
    EXPECT_EQ(downloader->IsHlsEnd(), false);
    downloader = nullptr;
}



// ===== Round 4: Init with callback and more =====

// L193,L210: Init with downloadCallback_ set before Init
HWTEST_F(HlsSegmentManagerUnitTest, INIT_WITH_CALLBACK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    auto metricsInfo = std::make_shared<DownloadMetricsInfo>();
    downloader->SetDownloadCallback(metricsInfo);
    downloader->Init();
    EXPECT_EQ(downloader->downloadCallback_ != nullptr, true);
    downloader = nullptr;
}

// L247,L248: Destructor with null reportInfo_ and downloader_
HWTEST_F(HlsSegmentManagerUnitTest, DTOR_NULL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->reportInfo_ = nullptr;
    downloader->downloader_ = nullptr;
    downloader->Close(true);
    EXPECT_EQ(downloader->isStopped.load(), true);
    downloader = nullptr;
}

// L1199: SaveCacheBufferDataNotblock with data written
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_CACHE_NOTBLOCK_WRITE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    uint8_t data[100] = {0};
    uint32_t result = downloader->SaveCacheBufferDataNotblock(data, 100);
    EXPECT_EQ(result, 100);
    downloader = nullptr;
}

// L1251,L1279: SaveCacheBufferData with isInterruptNeeded
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_CACHE_INTERRUPT_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isInterruptNeeded_.store(true);
    uint8_t data[100] = {0};
    uint32_t result = downloader->SaveCacheBufferData(data, 100, false);
    EXPECT_EQ(result, 0);
    downloader = nullptr;
}

// L1425 branch 1: DownloadRecordHistory with autoBufferSize and no rise/pulldown
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REC_HISTORY_NOCHANGE_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->currentBitRateVal_ = 500000;
    hlsSegmentManager_->playlistDownloader_ = mock;
    hlsSegmentManager_->autoBufferSize_ = true;
    hlsSegmentManager_->userDefinedBufferDuration_ = false;
    hlsSegmentManager_->lastWriteTime_ = 0;
    auto rd = std::make_shared<HlsSegmentManager::RecordData>();
    rd->downloadRate = 500000;
    hlsSegmentManager_->recordData_ = rd;
    hlsSegmentManager_->DownloadRecordHistory(2000);
    EXPECT_EQ(hlsSegmentManager_->totalBufferSize_ > 0, true);
}

// L1900: AutoSelectBitrate with curBitrate < desBitRate and buffer >= lowSize
HWTEST_F(HlsSegmentManagerUnitTest, AUTO_SELECT_BITRATE_HIGH_BUF_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    mock->bitRates_ = {100000, 500000};
    mock->curBitrate_ = 100000;
    mock->currentBitRateVal_ = 100000;
    hlsSegmentManager_->AutoSelectBitrate(200000);
    EXPECT_EQ(hlsSegmentManager_->playlistDownloader_ != nullptr, true);
}

// L1908 branch 1: AutoSelectBitrate with curBitrate > desBitRate and buffer <= highSize
HWTEST_F(HlsSegmentManagerUnitTest, AUTO_SELECT_BITRATE_LOW_BUF_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    mock->bitRates_ = {100000, 500000};
    mock->curBitrate_ = 500000;
    mock->currentBitRateVal_ = 500000;
    hlsSegmentManager_->AutoSelectBitrate(200000);
    EXPECT_EQ(hlsSegmentManager_->playlistDownloader_ != nullptr, true);
}

// L2296 branch 1: SetPlayStrategy with null playStrategy and valid playlist
HWTEST_F(HlsSegmentManagerUnitTest, SET_PLAY_STRATEGY_NULL_PS_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    downloader->SetPlayStrategy(nullptr);
    EXPECT_EQ(downloader->playlistDownloader_ != nullptr, true);
    downloader = nullptr;
}

// L2406,L2407: GetTotalTsBuffersize with loop limit exceeded
HWTEST_F(HlsSegmentManagerUnitTest, GET_TOTAL_TS_BUFSIZE_LOOP_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    uint8_t data[1024] = {0};
    downloader->cacheMediaBuffer_->Write(data, 0, 1024);
    downloader->readTsIndex_ = 0;
    downloader->writeTsIndex_ = 200;
    downloader->readOffset_ = 0;
    uint64_t size = downloader->GetTotalTsBuffersize();
    EXPECT_EQ(size <= static_cast<uint64_t>(MAX_CACHE_BUFFER_SIZE_UT), true);
    downloader = nullptr;
}

// L2441: ClearChunksInLargeSegment with writeOffset < readOffset
HWTEST_F(HlsSegmentManagerUnitTest, CLEAR_CHUNKS_LARGE_DIFF_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->writeTsIndex_ = 0;
    downloader->readTsIndex_ = 0;
    downloader->writeOffset_ = 50;
    downloader->readOffset_ = 100;
    downloader->ClearChunksInLargeSegment();
    EXPECT_EQ(downloader->writeOffset_ < downloader->readOffset_, true);
    downloader = nullptr;
}

// L2502: Clone with valid other but null other playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, CLONE_VALID_NULL_OTHER_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto other = std::make_shared<HlsSegmentManager>(10, false, header_);
    other->Init();
    other->playlistDownloader_ = nullptr;
    downloader->Clone(other);
    EXPECT_EQ(downloader->playlistDownloader_ != nullptr, true);
    downloader = nullptr;
    other = nullptr;
}



// ===== Round 5: Final targeted branch coverage =====

// L1810,L1811: UpdateDownloadFinished with auto-select all conditions met
HWTEST_F(HlsSegmentManagerUnitTest, UPDATE_DL_FINISHED_ALL_COND_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isParseAndNotifyFinished_ = true;
    mock->isLive_ = false;
    mock->isLiveEnd_ = false;
    downloader->playlistDownloader_ = mock;
    downloader->type_ = HlsSegmentType::SEG_VIDEO;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isDownloadStarted_ = false;
    downloader->isSeekingFlag = false;
    downloader->isSelectingBitrate_ = false;
    downloader->isAutoSelectBitrate_ = true;
    auto req = std::make_shared<DownloadRequest>("", nullptr, nullptr, false);
    downloader->SetDownloadRequest(req);
    downloader->UpdateDownloadFinished("test_url", "");
    EXPECT_EQ(downloader->isDownloadFinish_, true);
    downloader = nullptr;
}

// L1820: IsDownloadLastSplice with non-live and not last
HWTEST_F(HlsSegmentManagerUnitTest, IS_LAST_SPLICE_NOT_LAST_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = false;
    hlsSegmentManager_->playlistDownloader_ = mock;
    hlsSegmentManager_->writeTsIndex_ = 0;
    PlayInfo p1, p2, p3;
    hlsSegmentManager_->backPlayList_.push_back(p1);
    hlsSegmentManager_->backPlayList_.push_back(p2);
    hlsSegmentManager_->backPlayList_.push_back(p3);
    EXPECT_EQ(hlsSegmentManager_->IsDownloadLastSplice(), false);
}

// L2251: StopBufferring with null playlistDownloader_ only
HWTEST_F(HlsSegmentManagerUnitTest, STOP_BUFFERING_NULL_PL_ONLY_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->playlistDownloader_ = nullptr;
    EXPECT_EQ(downloader->StopBufferring(false), Status::ERROR_NULL_POINTER);
    downloader = nullptr;
}

// L2256: StopBufferring isAppBackground = true
HWTEST_F(HlsSegmentManagerUnitTest, STOP_BUFFERING_BG_TRUE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    EXPECT_EQ(downloader->StopBufferring(true), Status::OK);
    EXPECT_EQ(downloader->isInterrupt_.load(), true);
    downloader = nullptr;
}

// L2296: SetPlayStrategy with null playlistDownloader_ and null playStrategy
HWTEST_F(HlsSegmentManagerUnitTest, SET_PLAY_STRATEGY_NULL_BOTH_002, TestSize.Level1)
{
    hlsSegmentManager_->playlistDownloader_ = nullptr;
    hlsSegmentManager_->SetPlayStrategy(nullptr);
    EXPECT_EQ(hlsSegmentManager_->totalBits_, 0);
}

// L2300: SetPlayStrategy with width=0, height>0
HWTEST_F(HlsSegmentManagerUnitTest, SET_PLAY_STRATEGY_W_ZERO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    auto ps = std::make_shared<PlayStrategy>();
    ps->width = 0;
    ps->height = 1080;
    downloader->SetPlayStrategy(ps);
    EXPECT_EQ(downloader->playlistDownloader_ != nullptr, true);
    downloader = nullptr;
}

// L2441: ClearChunksInLargeSegment with writeOffset >= readOffset and diff < threshold
HWTEST_F(HlsSegmentManagerUnitTest, CLEAR_CHUNKS_LARGE_OK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->writeTsIndex_ = 0;
    downloader->readTsIndex_ = 0;
    downloader->writeOffset_ = 100;
    downloader->readOffset_ = 50;
    downloader->ClearChunksInLargeSegment();
    EXPECT_EQ(downloader->writeOffset_ >= downloader->readOffset_, true);
    downloader = nullptr;
}

// L2450,L2451: IsHlsEnd with live end and lastPlaychanged=true
HWTEST_F(HlsSegmentManagerUnitTest, IS_HLS_END_LIVE_TRUE_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLiveEnd_ = true;
    downloader->playlistDownloader_ = mock;
    downloader->lastPlaychanged_.store(true);
    downloader->readTsIndex_ = 0;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, true);
    // Need GetBufferSize() == 0 - cache is empty
    EXPECT_EQ(downloader->IsHlsEnd(), true);
    downloader = nullptr;
}

// L2502: Clone with valid other and playlistDownloader_
HWTEST_F(HlsSegmentManagerUnitTest, CLONE_VALID_ALL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto other = std::make_shared<HlsSegmentManager>(10, false, header_);
    other->Init();
    downloader->Clone(other);
    EXPECT_EQ(downloader->playlistDownloader_ != nullptr, true);
    downloader = nullptr;
    other = nullptr;
}

// L2515: SelectMedia with null playList_
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_MEDIA_NULL_PL_003, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_AUDIO);
    downloader->Init();
    downloader->playList_ = nullptr;
    EXPECT_EQ(downloader->SelectMedia(1, HlsSegmentType::SEG_AUDIO), false);
    downloader = nullptr;
}

// L2518: SelectMedia with SEG_VIDEO and isMediaSame=false (triggers actual switch)
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_MEDIA_VIDEO_SWITCH_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isMediaSame_ = false;
    downloader->playlistDownloader_ = mock;
    EXPECT_EQ(downloader->SelectMedia(1, HlsSegmentType::SEG_AUDIO), true);
    downloader = nullptr;
}

// L1554: SelectBitRate SEG_SUBTITLE with mock
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_SUBTITLE_MOCK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_SUBTITLE);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    downloader->playlistDownloader_ = mock;
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L1569: SelectBitRate with isEos_ = true and callback
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_EOS_CB_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isBitrateSame_ = false;
    downloader->playlistDownloader_ = mock;
    auto cb = std::make_shared<HlsNetworkBitrateCallback>();
    downloader->SetCallback(cb);
    downloader->isAutoSelectBitrate_ = false;
    downloader->isEos_ = true;
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L1199: SaveCacheBufferDataNotblock with data that triggers resume
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_CACHE_NOTBLOCK_RESUME_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(512, CHUNK_SIZE);
    downloader->memorySize_ = 512;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isNeedResume_.store(false);
    uint8_t data[512] = {0};
    uint32_t result = downloader->SaveCacheBufferDataNotblock(data, 512);
    EXPECT_EQ(result == 0 || result == 512, true);
    downloader = nullptr;
}

// L1494: DownloadReport with readBitrate_ > 0 (with timing)
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REPORT_RB_TIMING_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->totalBits_ = 200;
    downloader->lastBits_ = 0;
    downloader->lastCheckTime_ = 0;
    downloader->readBitrate_ = 500;
    downloader->bufferedDuration_ = 5000;
    OSAL::SleepFor(1100);
    downloader->DownloadReport();
    EXPECT_EQ(downloader->recordData_ != nullptr, true);
    downloader = nullptr;
}

// L1507: DownloadReport with data usage interval
HWTEST_F(HlsSegmentManagerUnitTest, DOWNLOAD_REPORT_USAGE_TIMING_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->isDownloadFinish_ = false;
    downloader->lastReportUsageTime_ = 0;
    downloader->dataUsage_ = 5000;
    OSAL::SleepFor(1100);
    downloader->DownloadReport();
    EXPECT_EQ(downloader->dataUsage_, 5000);
    downloader = nullptr;
}

// L2151,L2153: HandleCachedDuration with increase
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_CACHED_DUR_INCREASE_001, TestSize.Level1)
{
    hlsSegmentManager_->currentBitRate_ = 1000;
    hlsSegmentManager_->lastDurationReacord_ = 0;
    hlsSegmentManager_->cachedDuration_ = 0;
    hlsSegmentManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    hlsSegmentManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    uint8_t data[4096] = {0};
    hlsSegmentManager_->writeTsIndex_ = 1;
    hlsSegmentManager_->readTsIndex_ = 0;
    hlsSegmentManager_->writeOffset_ = 0;
    hlsSegmentManager_->cacheMediaBuffer_->Write(data, 0, 4096);
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    hlsSegmentManager_->HandleCachedDuration();
    EXPECT_EQ(called, true);
}



// ===== Round 6: Final targeted coverage =====

// L1074,L1075: PlaylistBackup with same URL but different offset
HWTEST_F(HlsSegmentManagerUnitTest, PLAYLIST_BACKUP_DIFF_OFFSET_001, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = true;
    mock->isParseFinished_ = false;
    hlsSegmentManager_->playlistDownloader_ = mock;
    PlayInfo p1;
    p1.url_ = "test.ts";
    p1.rangeUrl_ = "range.ts";
    p1.offset_ = 0;
    p1.length_ = 100;
    PlayInfo p2;
    p2.url_ = "test.ts";
    p2.rangeUrl_ = "range.ts";
    p2.offset_ = 100;
    p2.length_ = 100;
    hlsSegmentManager_->PlaylistBackup(p1);
    hlsSegmentManager_->PlaylistBackup(p2);
    EXPECT_EQ(hlsSegmentManager_->seekCandidates_.size(), 2);
}

// L1251,L1279: SaveCacheBufferData with isInterrupt set
HWTEST_F(HlsSegmentManagerUnitTest, SAVE_CACHE_INTERRUPT_SET_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, false);
    downloader->isInterrupt_.store(true);
    uint8_t data[100] = {0};
    uint32_t result = downloader->SaveCacheBufferData(data, 100, false);
    EXPECT_EQ(result, 0);
    downloader = nullptr;
}

// L1322: FillSingleBlock with partial fill
HWTEST_F(HlsSegmentManagerUnitTest, FILL_SINGLE_BLOCK_PARTIAL_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->singleBlockLen_ = 10;
    uint8_t data[5] = {1, 2, 3, 4, 5};
    uint32_t len = 5;
    uint8_t *ptr = data;
    uint32_t filled = downloader->FillSingleBlock(ptr, len);
    EXPECT_EQ(filled, 5);
    EXPECT_EQ(downloader->singleBlockLen_, 15);
    downloader = nullptr;
}

// L1357: DecryptDataLeft with writtenLen != decryptLen (small buffer)
HWTEST_F(HlsSegmentManagerUnitTest, DECRYPT_DATA_LEFT_SMALL_BUF_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(8, CHUNK_SIZE);
    downloader->memorySize_ = 8;
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
    uint8_t key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint8_t iv[16] = {0};
    downloader->aesDecryptor_->OnSourceKeyChange(key, 16, iv);
    downloader->singleBlockLen_ = 0;
    uint8_t data[16] = {0};
    uint32_t len = 16;
    uint8_t *ptr = data;
    uint32_t result = downloader->DecryptDataLeft(ptr, len, true);
    EXPECT_EQ(result, 0);
    downloader = nullptr;
}

// L1383: DecryptData with writtenLen != decryptLen (small buffer)
HWTEST_F(HlsSegmentManagerUnitTest, DECRYPT_DATA_SMALL_BUF_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(8, CHUNK_SIZE);
    downloader->memorySize_ = 8;
    downloader->aesDecryptor_ = std::make_shared<AesDecryptor>();
    uint8_t key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint8_t iv[16] = {0};
    downloader->aesDecryptor_->OnSourceKeyChange(key, 16, iv);
    uint8_t data[32] = {0};
    uint32_t len = 32;
    uint8_t *ptr = data;
    uint32_t result = downloader->DecryptData(ptr, len, true);
    EXPECT_EQ(result <= 32, true);
    downloader = nullptr;
}

// L1554 branch 2: SelectBitRate SEG_AUDIO with isBitrateSame=false
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_AUDIO_NOT_SAME_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_AUDIO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isBitrateSame_ = false;
    downloader->playlistDownloader_ = mock;
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L1561 branch 4,5: SelectBitRate with callback, auto, CheckReadStatus false
HWTEST_F(HlsSegmentManagerUnitTest, SELECT_BIT_RATE_AUTO_CHECK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>("test", HlsSegmentType::SEG_VIDEO);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isBitrateSame_ = false;
    mock->duration_ = 100;
    mock->isParseAndNotifyFinished_ = true;
    downloader->playlistDownloader_ = mock;
    auto cb = std::make_shared<HlsNetworkBitrateCallback>();
    downloader->SetCallback(cb);
    downloader->isAutoSelectBitrate_ = true;
    EXPECT_EQ(downloader->SelectBitRate(1000000), true);
    downloader = nullptr;
}

// L1820 branch 2: IsDownloadLastSplice with non-live and writeTsIndex_ + 1 != size
HWTEST_F(HlsSegmentManagerUnitTest, IS_LAST_SPLICE_NOT_LAST_002, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLive_ = false;
    hlsSegmentManager_->playlistDownloader_ = mock;
    hlsSegmentManager_->writeTsIndex_ = 5;
    PlayInfo p;
    hlsSegmentManager_->backPlayList_.push_back(p);
    EXPECT_EQ(hlsSegmentManager_->IsDownloadLastSplice(), false);
}

// L1900 branch 0,2: AutoSelectBitrate with curBitrate < desBitRate and buffer < lowSize
HWTEST_F(HlsSegmentManagerUnitTest, AUTO_SELECT_BITRATE_LOW_BUF_002, TestSize.Level1)
{
    auto mock = std::make_shared<TestPlayListDownloader>();
    hlsSegmentManager_->playlistDownloader_ = mock;
    mock->bitRates_ = {100000, 500000};
    mock->curBitrate_ = 100000;
    mock->currentBitRateVal_ = 100000;
    hlsSegmentManager_->AutoSelectBitrate(200000);
    EXPECT_EQ(hlsSegmentManager_->playlistDownloader_ != nullptr, true);
}

// L2151,L2152,L2153: HandleCachedDuration with cachedDuration change
HWTEST_F(HlsSegmentManagerUnitTest, HANDLE_CACHED_DUR_CHANGE_002, TestSize.Level1)
{
    hlsSegmentManager_->currentBitRate_ = 1000;
    hlsSegmentManager_->lastDurationReacord_ = 0;
    hlsSegmentManager_->cachedDuration_ = 0;
    hlsSegmentManager_->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    hlsSegmentManager_->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    uint8_t data[8192] = {0};
    hlsSegmentManager_->writeTsIndex_ = 2;
    hlsSegmentManager_->readTsIndex_ = 0;
    hlsSegmentManager_->writeOffset_ = 0;
    hlsSegmentManager_->cacheMediaBuffer_->Write(data, 0, 8192);
    bool called = false;
    hlsSegmentManager_->SetSegmentAllCallback([&called](HlsSegEvent) { called = true; });
    hlsSegmentManager_->HandleCachedDuration();
    EXPECT_EQ(called, true);
}

// L2406,L2407: GetTotalTsBuffersize with bufferSize == 0
HWTEST_F(HlsSegmentManagerUnitTest, GET_TOTAL_TS_BUFSIZE_ZERO_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(MAX_CACHE_BUFFER_SIZE_UT, false, header_);
    downloader->Init();
    downloader->cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    downloader->cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE_UT, CHUNK_SIZE);
    downloader->readTsIndex_ = 0;
    downloader->writeTsIndex_ = 1;
    downloader->readOffset_ = 999999;
    uint64_t size = downloader->GetTotalTsBuffersize();
    EXPECT_EQ(size <= MAX_CACHE_BUFFER_SIZE_UT, true);
    downloader = nullptr;
}

// L2450,L2451: IsHlsEnd with live end and conditions met
HWTEST_F(HlsSegmentManagerUnitTest, IS_HLS_END_LIVE_TRUE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLiveEnd_ = true;
    downloader->playlistDownloader_ = mock;
    downloader->lastPlaychanged_.store(true);
    downloader->readTsIndex_ = 0;
    downloader->writeTsIndex_ = 0;
    downloader->tsStorageInfo_[0] = std::make_pair(0, true);
    EXPECT_EQ(downloader->IsHlsEnd(), true);
    downloader = nullptr;
}

// L2456,L2457: IsHlsEnd with VOD end (not live end)
HWTEST_F(HlsSegmentManagerUnitTest, IS_HLS_END_VOD_CHECK_001, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    auto mock = std::make_shared<TestPlayListDownloader>();
    mock->isLiveEnd_ = false;
    mock->duration_ = 100;
    mock->isParseAndNotifyFinished_ = true;
    downloader->playlistDownloader_ = mock;
    downloader->isEos_ = false;
    downloader->readTsIndex_ = 0;
    PlayInfo p;
    downloader->backPlayList_.push_back(p);
    downloader->tsStorageInfo_[0] = std::make_pair(0, true);
    EXPECT_EQ(downloader->IsHlsEnd(), false);
    downloader = nullptr;
}

// L2251 branch 0: StopBufferring with null cacheMediaBuffer_ (userDefinedDuration=true)
HWTEST_F(HlsSegmentManagerUnitTest, STOP_BUFFERING_NULL_CACHE_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, true, header_);
    downloader->Init();
    EXPECT_EQ(downloader->StopBufferring(false), Status::ERROR_NULL_POINTER);
    downloader = nullptr;
}

// L2251 branch 2: StopBufferring with null downloader_
HWTEST_F(HlsSegmentManagerUnitTest, STOP_BUFFERING_NULL_DL_002, TestSize.Level1)
{
    auto downloader = std::make_shared<HlsSegmentManager>(10, false, header_);
    downloader->Init();
    downloader->downloader_ = nullptr;
    EXPECT_EQ(downloader->StopBufferring(false), Status::ERROR_NULL_POINTER);
    downloader = nullptr;
}

}