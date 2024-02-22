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
const std::string TEST_URI = "http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8";

void HlsMediaDownloaderUnitTest::SetUpTestCase(void)
{
    hlsMediaDownloader->Open(TEST_URI);
}

void HlsMediaDownloaderUnitTest::TearDownTestCase(void)
{
    hlsMediaDownloader->Close(false);
}

void HlsMediaDownloaderUnitTest ::SetUp(void) {}

void HlsMediaDownloaderUnitTest ::TearDown(void) {}

HWTEST_F(HlsMediaDownloaderUnitTest, seek_0001, TestSize.Level1)
{
    EXPECT_EQ(hlsMediaDownloader->SeekToTime(11), true);
}

HWTEST_F(HlsMediaDownloaderUnitTest, get_bitrates_001, TestSize.Level1)
{
    std::vector<uint32_t> bitrates = hlsMediaDownloader->GetBitRates();
    EXPECT_EQ(bitrates.size(), 4);
}

HWTEST_F(HlsMediaDownloaderUnitTest, gselect_bitrates_001, TestSize.Level1)
{
    bool res = hlsMediaDownloader->SelectBitRate(200000);
    EXPECT_EQ(res, 0);
    bool res1 = hlsMediaDownloader->SelectBitRate(311111);
    EXPECT_EQ(res1, 1);
}
}