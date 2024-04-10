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
#include "hls_playlist_downloader_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace std;
using namespace testing::ext;

const std::string TEST_URI = "http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8";
const std::map<std::string, std::string> httpHeader = {
    {"ua", "userAgent"},
    {"ref", "referer"},
};

void HlsPlayListDownloaderUnitTest::SetUpTestCase(void) {}

void HlsPlayListDownloaderUnitTest::TearDownTestCase(void) {}

void HlsPlayListDownloaderUnitTest::SetUp(void)
{
    playListDownloader->Open(TEST_URI, httpHeader);
}

void HlsPlayListDownloaderUnitTest::TearDown(void)
{
    playListDownloader->Close();
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GetDurationTest, TestSize.Level1)
{
    auto duration = playListDownloader->GetDuration();
    // 验证 GetDuration 方法的返回值
    EXPECT_GE(duration, 0);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IsLiveTest, TestSize.Level1)
{
    bool isLive = playListDownloader->IsLive();
    ASSERT_EQ(isLive, false);
}
}