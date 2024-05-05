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
    hlsMediaDownloader = null;
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
    EXPECT_EQ(hlsMediaDownloader->totalRingBufferSize_, MAX_BUFFER_SIZE);
    delete downloader;
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
    EXPECT_EQ(hlsMediaDownloader->totalRingBufferSize_, RING_BUFFER_SIZE);
    delete downloader;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_OPEN_002, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    EXPECT_GE(hlsMediaDownloader->totalRingBufferSize_, RING_BUFFER_SIZE);
    delete downloader;
}

HWTEST_F(HlsMediaDownloaderUnitTest, TEST_CLOSE, TestSize.Level1)
{
    HlsMediaDownloader *downloader = new HlsMediaDownloader(10);
    std::string testUrl = TEST_URI_PATH + "test_hls/testHLSEncode.m3u8";
    downloader->Open(testUrl, httpHeader);
    downloader->Pause()
    EXPECT_FALSE(downloader->isStopped);
    delete downloader;
}
}