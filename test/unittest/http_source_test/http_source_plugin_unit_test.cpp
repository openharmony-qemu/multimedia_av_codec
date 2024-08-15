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

#include "http_source_plugin.h"
#include "gtest/gtest.h"
#include "http_server_demo.h"
#include "source_callback.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

const uint32_t BUFFERING_SIZE_ = 1024;
const uint32_t WATERLINE_HIGH_ = 512;
const std::string MP4_SEGMENT_BASE = "http://127.0.0.1:46666/dewu.mp4";
static const std::string MPD_SEGMENT_BASE = "http://127.0.0.1:46666/segment_base/index.mpd";
static const std::string M3U8_SEGMENT_BASE = "http://127.0.0.1:46666/test_hls/testHLSEncode.m3u8";

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server;
class HttpSourcePluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<HttpSourcePlugin> httpSourcePlugin;
    std::shared_ptr<Meta> meta;
    std::shared_ptr<Buffer> buffer;
};

void HttpSourcePluginUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
}

void HttpSourcePluginUnitTest::TearDownTestCase(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

void HttpSourcePluginUnitTest::SetUp(void)
{
    meta = std::make_shared<Meta>();
    httpSourcePlugin = std::make_shared<HttpSourcePlugin>("test");
    buffer = std::make_shared<Buffer>();
}

void HttpSourcePluginUnitTest::TearDown(void)
{
    httpSourcePlugin.reset();
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_MP4_SetPlayStrategy, TestSize.Level1)
{
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1920;
    playStrategy->height = 1080;
    playStrategy->duration = 100;
    playStrategy->preferHDR = false;
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    source->SetPlayStrategy(playStrategy);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpSourcePlugin->SetSource(source);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_M3U8_SetPlayStrategy, TestSize.Level1)
{
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1920;
    playStrategy->height = 1080;
    playStrategy->duration = 100;
    playStrategy->preferHDR = false;
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    source->SetPlayStrategy(playStrategy);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpSourcePlugin->SetSource(source);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_MPD_SetPlayStrategy, TestSize.Level1)
{
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = 1920;
    playStrategy->height = 1080;
    playStrategy->duration = 100;
    playStrategy->preferHDR = false;
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(MPD_SEGMENT_BASE);
    source->SetPlayStrategy(playStrategy);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpSourcePlugin->SetSource(source);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_MP4, TestSize.Level1)
{
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpSourcePlugin->SetSource(source);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->SeekTo(10);
    httpSourcePlugin->SeekTo(100000);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_MP4_DumuxState, TestSize.Level1)
{
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpSourcePlugin->SetSource(source);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->SetDemuxerState(0);
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->SeekTo(10);
    httpSourcePlugin->SeekTo(100000);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_FUNC, TestSize.Level1)
{
    uint64_t size;
    int64_t duration;
    std::vector<uint32_t> bitRates;
    httpSourcePlugin->GetDuration(duration);
    httpSourcePlugin->GetBitRates(bitRates);
    httpSourcePlugin->GetSize(size);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->SetCurrentBitRate(10);
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpSourcePlugin->SetSource(source);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->GetSize(size);
    httpSourcePlugin->GetDuration(duration);
    httpSourcePlugin->SelectBitRate(10);
    httpSourcePlugin->GetBitRates(bitRates);
    httpSourcePlugin->SetDemuxerState(0);
    httpSourcePlugin->SetCurrentBitRate(10);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_INFO, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    DownloadInfo downloadInfo;
    httpSourcePlugin->GetDownloadInfo(downloadInfo);
    httpSourcePlugin->GetStreamInfo(streams);
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(MP4_SEGMENT_BASE);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpSourcePlugin->SetSource(source);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->GetStreamInfo(streams);
    httpSourcePlugin->SetReadBlockingFlag(true);
    httpSourcePlugin->SetInterruptState(true);
    httpSourcePlugin->GetDownloadInfo(downloadInfo);
    httpSourcePlugin->SetDownloadErrorState();
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_MPD, TestSize.Level1)
{
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(MPD_SEGMENT_BASE);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpSourcePlugin->SetSource(source);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->SeekTo(10);
    httpSourcePlugin->SeekTo(100000);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_M3U8, TestSize.Level1)
{
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpSourcePlugin->SetSource(source);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    httpSourcePlugin->SeekToTime(100000, SeekMode::SEEK_NEXT_SYNC);
}

HWTEST_F(HttpSourcePluginUnitTest, TEST_OPEN_M3U8_SetDemuxerState, TestSize.Level1)
{
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(M3U8_SEGMENT_BASE);
    Plugins::Callback* sourceCallback = new SourceCallback();
    httpSourcePlugin->SetSource(source);
    httpSourcePlugin->SetCallback(sourceCallback);
    httpSourcePlugin->GetSeekable();
    httpSourcePlugin->SetDemuxerState(0);
    httpSourcePlugin->Read(1, buffer, 10, 100);
    OSAL::SleepFor(1 * 1000);
    httpSourcePlugin->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    httpSourcePlugin->SeekToTime(100000, SeekMode::SEEK_NEXT_SYNC);
}

HWTEST_F(HttpSourcePluginUnitTest, Prepare_IsTrue, TestSize.Level1)
{
    httpSourcePlugin->delayReady = true;
    Status status = httpSourcePlugin->Prepare();
    EXPECT_EQ(status, Status::ERROR_DELAY_READY);
}

HWTEST_F(HttpSourcePluginUnitTest, Prepare_IsFalse, TestSize.Level1)
{
    httpSourcePlugin->delayReady = false;
    Status status = httpSourcePlugin->Prepare();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_Reset, TestSize.Level1)
{
    Status status = httpSourcePlugin->Reset();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetReadBlockingFlag_01, TestSize.Level1)
{
    bool isReadBlockingAllowed = true;
    Status status = httpSourcePlugin->SetReadBlockingFlag(isReadBlockingAllowed);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetReadBlockingFlag_02, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    bool isReadBlockingAllowed = true;
    Status status = httpSourcePlugin->SetReadBlockingFlag(isReadBlockingAllowed);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_Start, TestSize.Level1)
{
    Status status = httpSourcePlugin->Start();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_Stop, TestSize.Level1)
{
    Status status = httpSourcePlugin->Stop();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_GetParameter, TestSize.Level1)
{
    Status status = httpSourcePlugin->GetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetParameter1, TestSize.Level1)
{
    meta->SetData(Tag::BUFFERING_SIZE, BUFFERING_SIZE_);
    meta->SetData(Tag::WATERLINE_HIGH, WATERLINE_HIGH_);
    Status status = httpSourcePlugin->SetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetParameter2, TestSize.Level1)
{
    meta->SetData(Tag::WATERLINE_HIGH, WATERLINE_HIGH_);
    Status status = httpSourcePlugin->SetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetParameter3, TestSize.Level1)
{
    meta->SetData(Tag::BUFFERING_SIZE, BUFFERING_SIZE_);
    Status status = httpSourcePlugin->SetParameter(meta);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, HttpSourcePlugin_SetCallback, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->SetCallback(httpSourcePlugin->callback_);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(HttpSourcePluginUnitTest, SetDownloaderBySource, TestSize.Level1)
{
    httpSourcePlugin->SetDownloaderBySource(nullptr);
    EXPECT_EQ(httpSourcePlugin->downloader_, nullptr);
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported1, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/video.m3u8";
    EXPECT_TRUE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported2, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/video.mpd";
    EXPECT_TRUE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported3, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "video/mp4";
    httpSourcePlugin->uri_ = "http://example.com/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, IsSeekToTimeSupported4, TestSize.Level1)
{
    httpSourcePlugin->mimeType_ = "application/x-mpegURL";
    httpSourcePlugin->uri_ = "http://example.com/video.mp4";
    EXPECT_FALSE(httpSourcePlugin->IsSeekToTimeSupported());
}

HWTEST_F(HttpSourcePluginUnitTest, Read_IsNull, TestSize.Level1)
{
    httpSourcePlugin->downloader_ = nullptr;
    Status status = httpSourcePlugin->Read(1, buffer, 0, 10);
    EXPECT_EQ(status, Status::ERROR_NULL_POINTER);
}

HWTEST_F(HttpSourcePluginUnitTest, SetInterruptState1, TestSize.Level1)
{
    bool isInterruptNeeded = true;
    httpSourcePlugin->SetInterruptState(isInterruptNeeded);
    EXPECT_EQ(httpSourcePlugin->isInterruptNeeded_, isInterruptNeeded);
}

HWTEST_F(HttpSourcePluginUnitTest, SetInterruptState2, TestSize.Level1)
{
    bool isInterruptNeeded = false;
    httpSourcePlugin->SetInterruptState(isInterruptNeeded);
    EXPECT_EQ(httpSourcePlugin->isInterruptNeeded_, isInterruptNeeded);
}

}
}
}
}