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

void HlsPlayListDownloaderUnitTest::SetUpTestCase(void) {}

void HlsPlayListDownloaderUnitTest::TearDownTestCase(void) {}

void HlsPlayListDownloaderUnitTest::SetUp(void)
{
    playListDownloader->Open(TEST_URI);
}

void HlsPlayListDownloaderUnitTest::TearDown(void)
{
    playListDownloader->Close();
}


HWTEST_F(HlsPlayListDownloaderUnitTest, get_duration_0001, TestSize.Level1)
{
    int64_t duration = playListDownloader->GetDuration();
    EXPECT_GE(duration, 0.0);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, get_seekable_0001, TestSize.Level1)
{
    Seekable seekable = playListDownloader->GetSeekable();
    EXPECT_EQ(seekable, Seekable::SEEKABLE);
}

// 测试 PlayListDownloader 的 SetStatusCallback 函数
HWTEST_F(HlsPlayListDownloaderUnitTest, SetStatusCallback, TestSize.Level1)
{
    // 创建 MockStatusCallbackFunc 对象
    StatusCallbackFunc mockStatusCallback;
    // 创建 PlayListDownloader 对象
    HlsPlayListDownloader downloader;
    // 设置回调函数
    downloader.SetStatusCallback(mockStatusCallback);
    // 可以添加适当的检查，例如检查回调函数是否成功设置
}
}