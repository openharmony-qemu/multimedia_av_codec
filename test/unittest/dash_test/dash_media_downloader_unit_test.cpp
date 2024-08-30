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
constexpr int32_t SERVERPORT = 47777;
static const std::string MPD_SEGMENT_BASE = "http://127.0.0.1:47777/test_dash/segment_base/index.mpd";
static const std::string MPD_SEGMENT_LIST = "http://127.0.0.1:47777/test_dash/segment_list/index.mpd";
static const std::string MPD_SEGMENT_TEMPLATE = "http://127.0.0.1:47777/test_dash/segment_template/index.mpd";
static const std::string MPD_MULTI_AUDIO_SUB = "http://127.0.0.1:47777/test_dash/segment_base/index_audio_subtitle.mpd";
constexpr int32_t WAIT_FOR_SIDX_TIME = 100 * 1000; // wait sidx download and parse for 100ms
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
constexpr uint32_t DEFAULT_DURATION = 20;
}

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
std::shared_ptr<DashMediaDownloader> g_mediaDownloader = nullptr;
bool g_result = false;
void DashMediaDownloaderUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer(SERVERPORT);
    std::cout << "start" << std::endl;

    g_mediaDownloader = std::make_shared<DashMediaDownloader>();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    g_mediaDownloader->SetStatusCallback(statusCallback);
    g_mediaDownloader->SetIsTriggerAutoMode(true);
    std::string testUrl = MPD_SEGMENT_BASE;
    std::map<std::string, std::string> httpHeader;
    g_result = g_mediaDownloader->Open(testUrl, httpHeader);
}

void DashMediaDownloaderUnitTest::TearDownTestCase(void)
{
    g_mediaDownloader->Pause();
    g_mediaDownloader->Resume();
    g_mediaDownloader->SetInterruptState(true);
    g_mediaDownloader->SetDemuxerState(0);
    g_mediaDownloader->Close(true);
    g_mediaDownloader = nullptr;

    g_server->StopServer();
    g_server = nullptr;
}

void DashMediaDownloaderUnitTest::SetUp(void)
{
    if (g_server == nullptr) {
        g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
        g_server->StartServer(SERVERPORT);
        std::cout << "start server" << std::endl;
    }
}

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

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SET_BITRATE, TestSize.Level1)
{
    Status status = g_mediaDownloader->SetCurrentBitRate(1, 0);
    EXPECT_EQ(status, Status::OK);
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

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SELECT_AUDIO, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    g_mediaDownloader->GetStreamInfo(streams);
    EXPECT_GE(streams.size(), 0);

    int32_t switchingStreamId = -1;
    for (auto stream : streams) {
        if (stream.type != AUDIO) {
            continue;
        }

        switchingStreamId = stream.streamId;
        break;
    }

    Status status = Status::OK;
    if (switchingStreamId != -1) {
        status = g_mediaDownloader->SelectStream(switchingStreamId);
        g_mediaDownloader->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    }

    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SELECT_VIDEO, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    g_mediaDownloader->GetStreamInfo(streams);
    EXPECT_GE(streams.size(), 0);

    int32_t switchingStreamId = -1;
    for (auto stream : streams) {
        if (stream.type != VIDEO) {
            continue;
        }

        switchingStreamId = stream.streamId;
        break;
    }

    Status status = Status::OK;
    if (switchingStreamId != -1) {
        status = g_mediaDownloader->SelectStream(switchingStreamId);
    }

    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SELECT_SUBTITLE, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>();
    std::string testUrl = MPD_MULTI_AUDIO_SUB;
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);
    std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = DEFAULT_WIDTH;
    playStrategy->height = DEFAULT_HEIGHT;
    playStrategy->duration = DEFAULT_DURATION;
    mediaDownloader->SetPlayStrategy(playStrategy);

    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();

    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GE(streams.size(), 0);

    int32_t usingStreamId = -1;
    int32_t switchingStreamId = -1;
    for (auto stream : streams) {
        if (stream.type != SUBTITLE) {
            continue;
        }

        if (usingStreamId == -1) {
            usingStreamId = stream.streamId;
            continue;
        }

        if (stream.streamId != usingStreamId) {
            switchingStreamId = stream.streamId;
            break;
        }
    }

    Status status = Status::OK;

    if (switchingStreamId != -1) {
        status = mediaDownloader->SelectStream(switchingStreamId);
        mediaDownloader->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    }

    mediaDownloader->Close(true);
    mediaDownloader = nullptr;
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SELECT_BITRATE_AFTER_SWITCH, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>();
    std::string testUrl = MPD_MULTI_AUDIO_SUB;
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);

    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();

    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GE(streams.size(), 0);

    int32_t usingAudioStreamId = -1;
    int32_t switchingAudioStreamId = -1;
    int32_t usingVideoStreamId = -1;
    unsigned int switchingBitrate = 0;
    for (auto stream : streams) {
        if (stream.type == AUDIO) {
            if (usingAudioStreamId == -1) {
                usingAudioStreamId = stream.streamId;
                continue;
            }
            
            if (stream.streamId != usingAudioStreamId) {
                switchingAudioStreamId = stream.streamId;
                continue;
            }
        } else if (stream.type == VIDEO) {
            if (usingVideoStreamId == -1) {
                usingVideoStreamId = stream.streamId;
                continue;
            }
            
            if (stream.streamId != usingVideoStreamId) {
                switchingBitrate = stream.bitRate;
                continue;
            }
        } else {
            continue;
        }
    }

    Status status = mediaDownloader->SelectStream(switchingAudioStreamId);
    bool result = mediaDownloader->SelectBitRate(switchingBitrate);

    usleep(WAIT_FOR_SIDX_TIME);
    mediaDownloader->Close(true);
    mediaDownloader = nullptr;
    EXPECT_EQ(status, Status::OK);
    EXPECT_TRUE(result);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SELECT_SUBTITLE_AFTER_SWITCH, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>();
    std::string testUrl = MPD_MULTI_AUDIO_SUB;
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);

    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();

    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GE(streams.size(), 0);

    int32_t usingSubtitleStreamId = -1;
    int32_t switchingSubtitleStreamId = -1;
    int32_t usingVideoStreamId = -1;
    unsigned int switchingBitrate = 0;
    for (auto stream : streams) {
        if (stream.type == SUBTITLE) {
            if (usingSubtitleStreamId == -1) {
                usingSubtitleStreamId = stream.streamId;
                continue;
            }
            
            if (stream.streamId != usingSubtitleStreamId) {
                switchingSubtitleStreamId = stream.streamId;
                continue;
            }
        } else if (stream.type == VIDEO) {
            if (usingVideoStreamId == -1) {
                usingVideoStreamId = stream.streamId;
                continue;
            }
            
            if (stream.streamId != usingVideoStreamId) {
                switchingBitrate = stream.bitRate;
                continue;
            }
        } else {
            continue;
        }
    }

    bool result = mediaDownloader->SelectBitRate(switchingBitrate);
    Status status = mediaDownloader->SelectStream(switchingSubtitleStreamId);

    usleep(WAIT_FOR_SIDX_TIME);
    mediaDownloader->Close(true);
    mediaDownloader = nullptr;
    EXPECT_EQ(status, Status::OK);
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
    mediaDownloader->SetDownloadErrorState();
    mediaDownloader->Read(buff, readDataInfo);
    mediaDownloader->Close(true);
    mediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(DashMediaDownloaderUnitTest, GET_PLAYBACK_INFO_001, TestSize.Level1)
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

    PlaybackInfo playbackInfo;
    mediaDownloader->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.serverIpAddress, "127.0.0.1");
    EXPECT_EQ(playbackInfo.averageDownloadRate, 0);
    EXPECT_EQ(playbackInfo.isDownloading, false);
    EXPECT_EQ(playbackInfo.downloadRate, 0);
    EXPECT_EQ(playbackInfo.bufferDuration, 0);
    mediaDownloader->Close(true);
    mediaDownloader = nullptr;
}

}
}
}
}