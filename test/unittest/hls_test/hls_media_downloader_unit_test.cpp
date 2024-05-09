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

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace std;
using namespace testing::ext;

// 黑白球視頻地址

const std::map<std::string, std::string> httpHeader = {
    {"User-Agent", "ABC"},
    {"Referer", "DEF"}
};
static const std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
static const std::string M3U8_PATH_1 = "test_hls/testHLSEncode.m3u8";

void HlsMediaDownloaderUnitTest::SetUpTestCase(void)
{
}

void HlsMediaDownloaderUnitTest::TearDownTestCase(void)
{
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

HWTEST_F(HlsMediaDownloaderUnitTest, TestDefaultConstructor, TestSize.Level1)
{
    EXPECT_EQ(hlsMediaDownloader->totalRingBufferSize_, RING_BUFFER_SIZE);
}

HWTEST_F(HlsMediaDownloaderUnitTest, TestUserDefinedConstructor, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(1000);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    EXPECT_EQ(downloader->totalRingBufferSize_, MAX_BUFFER_SIZE);
    delete downloader;
    downloader = nullptr;
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
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    EXPECT_EQ(downloader->totalRingBufferSize_, MAX_BUFFER_SIZE);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_OPEN_002, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    EXPECT_GE(downloader->totalRingBufferSize_, RING_BUFFER_SIZE);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_PAUSE, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->Pause();
    EXPECT_FALSE(downloader->isStopped);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_CLOSE, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->Close(false);
    EXPECT_TRUE(downloader->isStopped);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_001, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    unsigned char buff[10];
    unsigned int realReadLength;
    bool isEos;
    EXPECT_FALSE(downloader->Read(buff, 10, realReadLength, isEos));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_002, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    unsigned char buff[10];
    unsigned int realReadLength;
    bool isEos {true};
    EXPECT_FALSE(downloader->Read(buff, 10, realReadLength, isEos));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_003, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->readTime_ = 5 * 1000;
    downloader->SetDownloadErrorState();
    unsigned char buff[10];
    unsigned int realReadLength;
    bool isEos {true};
    EXPECT_FALSE(downloader->Read(buff, 10, realReadLength, isEos));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_004, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->readTime_ = 5 * 1000;
    downloader->SetDownloadErrorState();
    unsigned char buff[10];
    unsigned int realReadLength;
    bool isEos {true};
    EXPECT_FALSE(downloader->Read(buff, 10, realReadLength, isEos));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_005, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    unsigned char buff[100];
    unsigned int realReadLength;
    bool isEos {true};
    EXPECT_FALSE(downloader->Read(buff, 10*1024*1024, realReadLength, isEos));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_READ_006, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    unsigned char buff[100];
    unsigned int realReadLength;
    bool isEos {true};
    EXPECT_FALSE(downloader->Read(buff, 10, realReadLength, isEos));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_SEEK_TO_TIME, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    EXPECT_TRUE(downloader->SeekToTime(100, SeekMode::SEEK_CLOSEST));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_SEEK_TO_TIME_001, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->backPlayList_.push_back(PlayInfo{"123", 0.0, 0});
    downloader->backPlayList_.push_back(PlayInfo{"123", 0.0, 0});
    downloader->backPlayList_.push_back(PlayInfo{"123", 0.0, 0});
    downloader->SeekToTs(250, SeekMode::SEEK_CLOSEST);
    EXPECT_EQ(downloader->havePlayedTsNum_, 3);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_SAVE, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    uint8_t data[0];
    uint32_t len = 0;
    EXPECT_TRUE(downloader->SaveData(data, len));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_SAVE_001, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint32_t len = 10;
    EXPECT_TRUE(downloader->SaveData(data, len));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_SAVE_002, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    downloader->Open(testUrl, httpHeader);
    uint8_t data[17];
    uint32_t len = 17;
    EXPECT_TRUE(downloader->SaveData(data, len));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_SAVE_003, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    downloader->Open(testUrl, httpHeader);
    uint8_t data[15];
    uint32_t len = 15;
    EXPECT_TRUE(downloader->SaveData(data, len));
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_WRITE_RINGBUFFER_001, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    downloader->Open(testUrl, httpHeader);
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
    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->totalRingBufferSize_ = MAX_BUFFER_SIZE;
    downloader->RiseBufferSize();
    EXPECT_EQ(downloader->totalRingBufferSize_, MAX_BUFFER_SIZE);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, RISE_BUFFER_002, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->totalRingBufferSize_ = RING_BUFFER_SIZE;
    downloader->RiseBufferSize();
    EXPECT_EQ(downloader->totalRingBufferSize_, 6 * 1024 * 1024);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, DOWN_BUFFER_001, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->totalRingBufferSize_ = 10 * 1024 * 1024;
    downloader->DownBufferSize();
    EXPECT_EQ(downloader->totalRingBufferSize_, 9 * 1024 * 1024);
    delete downloader;
    downloader = nullptr;
}

HWTEST_F(HlsMediaDownloaderUnitTest, DOWN_BUFFER_002, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader();
    std::string testUrl = TEST_URI_PATH + "test_cbr/test_cbr.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->totalRingBufferSize_ = RING_BUFFER_SIZE;
    downloader->DownBufferSize();
    EXPECT_EQ(downloader->totalRingBufferSize_, RING_BUFFER_SIZE);
    delete downloader;
    downloader = nullptr;
}
}