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

HWTEST_F(HlsPlayListDownloaderUnitTest, OpenTest, TestSize.Level1) {
    playListDownloader->Open(TEST_URI);
    ASSERT_EQ(playListDownloader->url_, TEST_URI);  // 验证 URL 是否正确设置
    ASSERT_EQ(playListDownloader->master_, nullptr);  // 验证 master_ 是否被重置为 nullptr
}

HWTEST_F(HlsPlayListDownloaderUnitTest, SetPlayListCallbackTest, TestSize.Level1) {
    PlayListChangeCallback callback;
    playListDownloader->SetPlayListCallback(&callback);
    // 验证 callback 是否正确设置
    ASSERT_EQ(playListDownloader->callback_, &callback);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GetDurationTest, TestSize.Level1) {
    auto duration = playListDownloader->GetDuration();
    // 验证 GetDuration 方法的返回值
     ASSERT_EQ(duration, (downloader.master_->bLive_ ? -1.0 : HstTime2Ns(Sec2HstTime(playListDownloader->master_->duration_))));
}

HWTEST_F(HlsPlayListDownloaderUnitTest, GetSeekableTest, TestSize.Level1) {
    auto seekable = playListDownloader->GetSeekable();
    // 验证 GetSeekable 方法的返回值
     ASSERT_EQ(seekable, (playListDownloader->master_->bLive_ ? Seekable::UNSEEKABLE : Seekable::SEEKABLE));
}


HWTEST_F(HlsPlayListDownloaderUnitTest, SelectBitRateTest, TestSize.Level1) {
    uint32_t bitRate = 1024;
    playListDownloader->SelectBitRate(bitRate);
    // 验证 SelectBitRate 方法是否正确选择比特率
    ASSERT_EQ(playListDownloader->currentVariant_, playListDownloader->newVariant_);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IsBitrateSameTest, TestSize.Level1) {
    uint32_t bitRate = 1024;
    bool result = dplayListDownloader->IsBitrateSame(bitRate);
    // 验证 IsBitrateSame 方法的返回值
    ASSERT_EQ(result, (playListDownloader->newVariant_->bandWidth_ == playListDownloader->currentVariant_->bandWidth_));
}

HWTEST_F(HlsPlayListDownloaderUnitTest, IsLiveTest, TestSize.Level1) {
    bool isLive = playListDownloader->IsLive();
    ASSERT_EQ(isLive, playListDownloader->master_->bLive_);  
    
}

TEST_F(HlsPlayListDownloaderTest, UpdateManifestTest) {
    downloader.UpdateManifest();

    // 验证 UpdateManifest 方法的行为
    if (downloader.currentVariant_ && downloader.currentVariant_->m3u8_ && !downloader.currentVariant_->m3u8_->uri_.empty()) {
        EXPECT_CALL(mockDownloader, DoOpen(_)).Times(1);  // 验证 DoOpen 被调用一次
    } else {
        EXPECT_CALL(mockDownloader, LogError(_)).Times(1);  // 验证错误日志被记录一次
    }
}

}