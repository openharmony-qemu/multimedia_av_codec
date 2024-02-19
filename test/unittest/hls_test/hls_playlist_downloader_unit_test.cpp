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
#include "gtest/gtest.h"

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

// 测试 PlayListDownloader 的 Resume 函数
HWTEST_F(HlsPlayListDownloaderUnitTest, Resume, TestSize.Level1)
{
    // 创建 MockDownloader 对象
    std::shared_ptr<MockDownloader> mockDownloader = std::make_shared<MockDownloader>("hlsPlayList");
    // 创建 PlayListDownloader 对象
    PlayListDownloader downloader;
    // 设置 Downloader 为 MockDownloader
    downloader.downloader_ = mockDownloader;
    // 期望调用 MockDownloader 的 Resume 函数
    EXPECT_CALL(*mockDownloader, Resume());
    // 调用 Resume 函数
    downloader.Resume();
}

// 测试 PlayListDownloader 的 Pause 函数
HWTEST_F(HlsPlayListDownloaderUnitTest, Pause, TestSize.Level1)
{
    // 创建 MockDownloader 对象
    std::shared_ptr<MockDownloader> mockDownloader = std::make_shared<MockDownloader>("hlsPlayList");
    // 创建 PlayListDownloader 对象
    PlayListDownloader downloader;
    // 设置 Downloader 为 MockDownloader
    downloader.downloader_ = mockDownloader;
    // 期望调用 MockDownloader 的 Pause 函数
    EXPECT_CALL(*mockDownloader, Pause());
    // 调用 Pause 函数
    downloader.Pause();
}

// 测试 PlayListDownloader 的 SetStatusCallback 函数
HWTEST_F(HlsPlayListDownloaderUnitTest, SetStatusCallback, TestSize.Level1)
{
    // 创建 MockStatusCallbackFunc 对象
    StatusCallbackFunc mockStatusCallback;
    // 创建 PlayListDownloader 对象
    PlayListDownloader downloader;
    // 设置回调函数
    downloader.SetStatusCallback(mockStatusCallback);
    // 可以添加适当的检查，例如检查回调函数是否成功设置
}

// 测试 PlayListDownloader 的 Close 函数
HWTEST_F(HlsPlayListDownloaderUnitTest, Close, TestSize.Level1)
{
    // 创建 MockDownloader 对象
    std::shared_ptr<MockDownloader> mockDownloader = std::make_shared<MockDownloader>("hlsPlayList");
    // 创建 PlayListDownloader 对象
    PlayListDownloader downloader;
    // 设置 Downloader 为 MockDownloader
    downloader.downloader_ = mockDownloader;
    // 期望调用 MockDownloader 的 Stop 函数
    EXPECT_CALL(*mockDownloader, Stop());
    // 调用 Close 函数
    downloader.Close();
}

// 测试 PlayListDownloader 的 Stop 函数
HWTEST_F(HlsPlayListDownloaderUnitTest, Stop, TestSize.Level1)
{
    // 创建 MockDownloader 对象
    std::shared_ptr<MockDownloader> mockDownloader = std::make_shared<MockDownloader>("hlsPlayList");
    // 创建 PlayListDownloader 对象
    PlayListDownloader downloader;
    // 设置 Downloader 为 MockDownloader
    downloader.downloader_ = mockDownloader;
    // 期望调用 MockDownloader 的 Stop 函数
    EXPECT_CALL(*mockDownloader, Stop());
    // 调用 Stop 函数
    downloader.Stop();
}

// 测试 PlayListDownloader 的 Start 函数
HWTEST_F(HlsPlayListDownloaderUnitTest, Start, TestSize.Level1)
{
    // 创建 MockDownloader 对象
    std::shared_ptr<MockDownloader> mockDownloader = std::make_shared<MockDownloader>("hlsPlayList");
    // 创建 PlayListDownloader 对象
    PlayListDownloader downloader;
    // 设置 Downloader 为 MockDownloader
    downloader.downloader_ = mockDownloader;
    // 期望调用 MockDownloader 的 Start 函数
    EXPECT_CALL(*mockDownloader, Start());
    // 调用 Start 函数
    downloader.Start();
}

// 测试 PlayListDownloader 的 Cancel 函数
HWTEST_F(HlsPlayListDownloaderUnitTest, Cancel, TestSize.Level1)
{
    // 创建 MockDownloader 对象
    std::shared_ptr<MockDownloader> mockDownloader = std::make_shared<MockDownloader>("hlsPlayList");
    // 创建 PlayListDownloader 对象
    PlayListDownloader downloader;
    // 设置 Downloader 为 MockDownloader
    downloader.downloader_ = mockDownloader;
    // 期望调用 MockDownloader 的 Cancel 函数
    EXPECT_CALL(*mockDownloader, Cancel());
    // 调用 Cancel 函数
    downloader.Cancel();
}
}