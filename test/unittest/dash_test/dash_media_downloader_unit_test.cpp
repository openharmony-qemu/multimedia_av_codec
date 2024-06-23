/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "dash_media_downloader_unit_test.h"
#include <iostream>
#include "dash_media_downloader.h"
#include "http_server_demo.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing::ext;
namespace {
static const std::string MPD_SEGMENT_BASE = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
static const std::string MPD_SEGMENT_LIST = "http://127.0.0.1:46666/test_dash/segment_list/index.mpd";
static const std::string MPD_SEGMENT_TEMPLATE = "http://127.0.0.1:46666/test_dash/segment_template/index.mpd";
}

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
std::shared_ptr<DashMediaDownloader> g_mediaDownloader = nullptr;
bool g_result = false;
void DashMediaDownloaderUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
    std::cout << "start" << std::endl;

    g_mediaDownloader = std::make_shared<DashMediaDownloader>();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    g_mediaDownloader->SetStatusCallback(statusCallback);
    std::string testUrl = MPD_SEGMENT_BASE;
    std::map<std::string, std::string> httpHeader;
    g_result = g_mediaDownloader->Open(testUrl, httpHeader);
}

void DashMediaDownloaderUnitTest::TearDownTestCase(void)
{
    g_mediaDownloader->Close(true);
    g_mediaDownloader = nullptr;

    g_server->StopServer();
    g_server = nullptr;
}

void DashMediaDownloaderUnitTest::SetUp(void) {}

void DashMediaDownloaderUnitTest::TearDown(void) {}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_CONTENT_LENGTH, TestSize.Level1)
{
    EXPECT_EQ(g_mediaDownloader->GetContentLength(), 0);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_STARTED_STATUS, TestSize.Level1)
{
    EXPECT_TRUE(g_mediaDownloader->GetStartedStatus());
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_OPEN, TestSize.Level1)
{
    EXPECT_TRUE(g_result);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_SEEKABLE, TestSize.Level1)
{
    Seekable seekable = g_mediaDownloader->GetSeekable();
    EXPECT_EQ(seekable, Seekable::SEEKABLE);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_DURATION, TestSize.Level1)
{
    EXPECT_GE(g_mediaDownloader->GetDuration(), 0);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_BITRATES, TestSize.Level1)
{
    std::vector<uint32_t> bitrates = g_mediaDownloader->GetBitRates();
    EXPECT_GE(bitrates.size(), 0);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_STREAM_INFO, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    Status status = g_mediaDownloader->GetStreamInfo(streams);
    EXPECT_EQ(status, Status::OK);
    EXPECT_GE(streams.size(), 0);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SELECT_BITRATE, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    g_mediaDownloader->GetStreamInfo(streams);
    EXPECT_GE(streams.size(), 0);

    unsigned int switchingBitrate = 0;
    int32_t usingStreamId = -1;
    for (auto stream : streams) {
        if (stream.type != VIDEO) {
            continue;
        }

        if (usingStreamId == -1) {
            usingStreamId = stream.streamId;
            continue;
        }

        if (stream.streamId != usingStreamId) {
            switchingBitrate = stream.bitRate;
            break;
        }
    }

    bool result = false;
    if (switchingBitrate > 0) {
        result = g_mediaDownloader->SelectBitRate(switchingBitrate);
    }

    EXPECT_TRUE(result);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SEEK_TO_TIME, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>();
    std::string testUrl = MPD_SEGMENT_TEMPLATE;
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);

    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();

    bool result = mediaDownloader->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    mediaDownloader->Close(true);
    mediaDownloader = nullptr;
    EXPECT_TRUE(result);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_READ, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>();
    std::string testUrl = MPD_SEGMENT_LIST;
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);

    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();

    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    unsigned char buff[1024];;
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = streams[0].streamId;
    readDataInfo.nextStreamId_ = streams[0].streamId;
    readDataInfo.wantReadLength_ = 1024;
    mediaDownloader->Read(buff, readDataInfo);
    mediaDownloader->Close(true);
    mediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

}
}
}
}