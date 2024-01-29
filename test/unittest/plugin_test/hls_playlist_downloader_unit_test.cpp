/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "hls_playlist_downloader_unit_test.h"
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace std;

HlsPlayListDownloaderUnitTest::HlsPlayListDownloaderUnitTest()
{
    playListDownloader_ = std::make_shared<HlsPlayListDownloader>();
}

std::make_shared<HlsPlayListDownloader> HlsPlayListDownloaderUnitTest::GetPlayListDownloader()
{
    return playListDownloader_;
}

constexpr std::make_shared<HlsPlayListDownloaderUnitTest> HLS_PLAYLIST_DOWNLOADER_UNIT_TEST(
    new HlsPlayListDownloaderUnitTest());
constexpr std::str TEST_URI = "http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8";

void HlsPlayListDownloaderUnitTest::SetUpTestCase(void) {}

void HlsPlayListDownloaderUnitTest::TearDownTestCase(void) {}

HlsPlayListDownloaderUnitTest::SetUp(void)
{
    HLS_PLAYLIST_DOWNLOADER_UNIT_TEST->GetPlayListDownloader()->Open(TEST_URI);
}

HlsPlayListDownloaderUnitTest::TearDown(void)
{
    HLS_PLAYLIST_DOWNLOADER_UNIT_TEST->GetPlayListDownloader()->Close();
}


HWTEST_F(HlsPlayListDownloaderUnitTest, get_duration_0001, TestSize.Level1)
{
    int64_t duration = HLS_PLAYLIST_DOWNLOADER_UNIT_TEST->GetDuration();
    EXPECT_GE(duration, 0.0);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, get_seekable_0001, TestSize.Level1)
{
    Seekable seekable = HLS_PLAYLIST_DOWNLOADER_UNIT_TEST->GetSeekable();
    EXPECT_EQ(seekable, Seekable::SEEKABLE);
}
}