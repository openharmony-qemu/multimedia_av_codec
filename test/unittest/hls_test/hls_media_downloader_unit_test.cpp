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

HWTEST_F(HlsMediaDownloaderUnitTest, OpenTest, TestSize.Level1)
{
    bool result = hlsMediaDownloader->Open(TEST_URI);
    ASSERT_TRUE(result);
}


HWTEST_F(HlsMediaDownloaderUnitTest, ReadTest, TestSize.Level1)
{
    unsigned char buffer[1024];
    unsigned int realReadLength;
    bool isEos;
    bool result = hlsMediaDownloader->Read(buffer, 1024, realReadLength, isEos);
    ASSERT_TRUE(result);
    ASSERT_EQ(realReadLength, 1024);
    ASSERT_FALSE(isEos);
}

HWTEST_F(HlsMediaDownloaderUnitTest, SeekToTimeTest, TestSize.Level1)
{
    int64_t seekTime = 5000;
    bool result = hlsMediaDownloader->SeekToTime(seekTime);
    ASSERT_TRUE(result);
}

HWTEST_F(HlsMediaDownloaderUnitTest, GetContentLengthDurationTest, TestSize.Level1)
{
    int64_t duration = hlsMediaDownloader->GetDuration();
    EXPECT_GE(duration, 0);
}
}