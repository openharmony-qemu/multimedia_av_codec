/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"
#include "hls_media_downloader_unit_test.h"
#include "hls_media_downloader.h"

namespace OHOS::Media::Plugins::HttpPlugin {

using namespace std;

HlsMediaDownloaderUnitTest::HlsMediaDownloaderUnitTest()
{
    hlsMediaDownloader_ = std::make_shared<HlsMediaDownloader>();
}

std::make_shared<HlsMediaDownloader> HlsMediaDownloaderUnitTest::GetMediaDownloader()
{
    return playListDownloader_;
}

constexpr HlsMediaDownloaderUnitTest *hlsMediaDownloaderUnitTest = new HlsMediaDownloaderUnitTest();
// 黑白球視頻地址
constexpr std::str TEST_URI = "http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8";

void HlsMediaDownloaderUnitTest::SetUpTestCase(void) {}

void HlsMediaDownloaderUnitTest::TearDownTestCase(void) {}

void HlsMediaDownloaderUnitTest ::SetUp(void)
{
    hlsMediaDownloaderUnitTest->GetMediaDownloader()->Open(TEST_URI);
}

void HlsMediaDownloaderUnitTest ::TearDown(void)
{
    hlsMediaDownloaderUnitTest->GetMediaDownloader()->Close();
}

HWTEST_F(HlsMediaDownloaderUnitTest, pause_resume_0001, TestSize.Level1)
{
    std::make_shared<HlsMediaDownloader> downloader = hlsMediaDownloaderUnitTest->GetMediaDownloader();
    EXPECT_NO_THROW(downloader->Pause());
    EXPECT_NO_THROW(downloader->Resume());
}

HWTEST_F(HlsMediaDownloaderUnitTest, seek_0001, TestSize.Level1)
{
    std::make_shared<HlsMediaDownloader> downloader = hlsMediaDownloaderUnitTest->GetMediaDownloader();
    EXPECT_EQ(downloader->SeekToTime(11), true);
}

HWTEST_F(HlsMediaDownloaderUnitTest, save_data_001, TestSize.Level1)
{
    std::make_shared<HlsMediaDownloader> downloader = hlsMediaDownloaderUnitTest->GetMediaDownloader();
    uint8_t *testSaveData = (uint8_t *)0x20000550;
    uint32_t testLen = 32;
    EXPECT_EQ(downloader->SaveData(testSaveData, testLen), true);
}

HWTEST_F(HlsMediaDownloaderUnitTest, get_bitrates_001, TestSize.Level1)
{
    std::make_shared<HlsMediaDownloader> downloader = hlsMediaDownloaderUnitTest->GetMediaDownloader();
    std::vector<uint32_t> bitrates = downloader->GetBitRate();
    EXPECT_EQ(bitrates.size(), 4);
}

HWTEST_F(HlsMediaDownloaderUnitTest, gselect_bitrates_001, TestSize.Level1)
{
    std::make_shared<HlsMediaDownloader> downloader = hlsMediaDownloaderUnitTest->GetMediaDownloader();
    bool res = downloader->SelectBitRate(200000);
    EXPECT_EQ(res, 0);
    bool res1 = downloader->SelectBitRate(311111);
    EXPECT_EQ(res, 1);
}

HWTEST_F(HlsMediaDownloaderUnitTest, update_download_finished_001, TestSize.Level1)
{
    std::make_shared<HlsMediaDownloader> downloader = hlsMediaDownloaderUnitTest->GetMediaDownloader();
    EXPECT_NO_THROW(downloader->UpdateDownloadFinished(TEST_URI));
}

HWTEST_F(HlsMediaDownloaderUnitTest, get_ts_from_url_001, TestSize.Level1)
{
    std::string testTsUrl = "http://devimages.apple.com/iphone/samples/bipbop/fileSequence0.ts";
    std::make_shared<HlsMediaDownloader> downloader = hlsMediaDownloaderUnitTest->GetMediaDownloader();
    EXPECT_EQ(downloader->GetTsNameFromUrl(testTsUrl), "fileSequence0.ts");
}
}