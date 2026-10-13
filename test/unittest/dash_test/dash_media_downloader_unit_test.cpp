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

struct MultiAudioSubSwitchInfo {
    int32_t defaultVideoStreamId = -1;
    int32_t defaultAudioStreamId = -1;
    int32_t defaultSubtitleStreamId = -1;
    int32_t usingAudioStreamId = -1;
    int32_t switchingAudioStreamId = -1;
    int32_t usingSubtitleStreamId = -1;
    int32_t switchingSubtitleStreamId = -1;
    int32_t usingVideoStreamId = -1;
    unsigned int switchingBitrate = 0;
};

std::shared_ptr<DashMediaDownloader> CreateMultiAudioSubDownloader(bool needPlayStrategy)
{
    auto mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);

    if (needPlayStrategy) {
        std::shared_ptr<PlayStrategy> playStrategy = std::make_shared<PlayStrategy>();
        playStrategy->width = DEFAULT_WIDTH;
        playStrategy->height = DEFAULT_HEIGHT;
        playStrategy->duration = DEFAULT_DURATION;
        playStrategy->audioLanguage = "eng";
        playStrategy->subtitleLanguage = "en_GB";
        mediaDownloader->SetPlayStrategy(playStrategy);
    }

    mediaDownloader->Open(MPD_MULTI_AUDIO_SUB, httpHeader);
    mediaDownloader->GetSeekable();
    return mediaDownloader;
}

MultiAudioSubSwitchInfo BuildSwitchInfoAndPrepare(const std::shared_ptr<DashMediaDownloader>& mediaDownloader,
    std::vector<StreamInfo>& streams)
{
    MultiAudioSubSwitchInfo info;
    mediaDownloader->GetStreamInfo(streams);
    for (auto stream : streams) {
        if (stream.type == AUDIO) {
            if (info.defaultAudioStreamId == -1) {
                info.defaultAudioStreamId = stream.streamId;
            }
            if (info.usingAudioStreamId == -1) {
                info.usingAudioStreamId = stream.streamId;
            } else if (stream.streamId != info.usingAudioStreamId && info.switchingAudioStreamId == -1) {
                info.switchingAudioStreamId = stream.streamId;
            }
            continue;
        }

        if (stream.type == VIDEO) {
            if (info.defaultVideoStreamId == -1) {
                info.defaultVideoStreamId = stream.streamId;
            }
            if (info.usingVideoStreamId == -1) {
                info.usingVideoStreamId = stream.streamId;
            } else if (stream.streamId != info.usingVideoStreamId && info.switchingBitrate == 0) {
                info.switchingBitrate = stream.bitRate;
            }
            continue;
        }

        if (stream.type == SUBTITLE) {
            if (info.defaultSubtitleStreamId == -1) {
                info.defaultSubtitleStreamId = stream.streamId;
            }
            if (info.usingSubtitleStreamId == -1) {
                info.usingSubtitleStreamId = stream.streamId;
            } else if (stream.streamId != info.usingSubtitleStreamId && info.switchingSubtitleStreamId == -1) {
                info.switchingSubtitleStreamId = stream.streamId;
            }
        }
    }

    if (info.defaultVideoStreamId != -1) {
        mediaDownloader->SetDefaultStreamId(info.defaultVideoStreamId,
            info.defaultAudioStreamId, info.defaultSubtitleStreamId);
        mediaDownloader->GetSeekable();
    }
    return info;
}
}

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
std::shared_ptr<DashMediaDownloader> g_mediaDownloader = nullptr;
bool g_result = false;
void DashMediaDownloaderUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer(SERVERPORT);
    std::cout << "start" << std::endl;

    g_mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    g_mediaDownloader->Init();
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
    g_mediaDownloader->Close(false);
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
class DashCallbackMock1 : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        switch (event.type) {
            case PluginEventType::STREAM_UPDATE: {
                int videoId = -1;
                int audioId = -1;
                int textId = -1;
                mediaDownloader->SetDefaultStreamId(videoId, audioId, textId);
                break;
            }
            default:break;
        }
    }
    void SetDownload(std::shared_ptr<DashMediaDownloader> downloader)
    {
        mediaDownloader = downloader;
    }
    std::shared_ptr<DashMediaDownloader> mediaDownloader;
};

void DashMediaDownloaderUnitTest::TearDown(void) {}

class DashCallbackMock : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        lastEventType_ = event.type;
        onEventCount_++;
    }

    void SetSelectBitRateFlag(bool flag, uint32_t desBitRate) override
    {
        lastFlag_ = flag;
        lastBitrate_ = desBitRate;
        setFlagCount_++;
    }

    bool CanAutoSelectBitRate() override
    {
        canAutoCount_++;
        return true;
    }

    Plugins::PluginEventType lastEventType_ {Plugins::PluginEventType::CLIENT_ERROR};
    bool lastFlag_ {false};
    uint32_t lastBitrate_ {0};
    uint32_t onEventCount_ {0};
    uint32_t setFlagCount_ {0};
    uint32_t canAutoCount_ {0};
};

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_CONTENT_LENGTH, TestSize.Level1)
{
    EXPECT_EQ(g_mediaDownloader->GetContentLength(), 0);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SET_CALLBACK, TestSize.Level1)
{
    auto mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();

    auto cb = std::make_shared<DashCallbackMock>();
    mediaDownloader->SetCallback(cb);
    mediaDownloader->SetDownloadErrorState();

    EXPECT_GE(cb->onEventCount_, 1u);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_OPEN, TestSize.Level1)
{
    EXPECT_TRUE(g_result);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_SEEKABLE, TestSize.Level1)
{
    OSAL::SleepFor(100);
    int32_t defaultVideoStreamId = 0;
    g_mediaDownloader->SetDefaultStreamId(defaultVideoStreamId, defaultVideoStreamId, defaultVideoStreamId);
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

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_STREAM_INFO_UPDATE, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    Status status = g_mediaDownloader->GetStreamInfo(streams, true);
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

    int32_t defaultVideoStreamId = -1;
    int32_t defaultAudioStreamId = -1;
    int32_t defaultSubtitleStreamId = -1;
    unsigned int switchingBitrate = 0;
    int32_t usingStreamId = -1;
    for (auto stream : streams) {
        if (stream.type != VIDEO) {
            continue;
        }

        if (stream.type == VIDEO && defaultVideoStreamId == -1) {
            defaultVideoStreamId = stream.streamId;
        }
        if (stream.type == AUDIO && defaultAudioStreamId == -1) {
            defaultAudioStreamId = stream.streamId;
        }

        if (usingStreamId == -1) {
            usingStreamId = stream.streamId;
            defaultSubtitleStreamId = stream.streamId;
            continue;
        }

        if (stream.streamId != usingStreamId) {
            switchingBitrate = stream.bitRate;
            break;
        }
    }

    if (defaultVideoStreamId != -1) {
        g_mediaDownloader->SetDefaultStreamId(defaultVideoStreamId, defaultAudioStreamId, defaultSubtitleStreamId);
    }

    bool result = false;
    if (switchingBitrate > 0) {
        result = g_mediaDownloader->SelectBitRate(switchingBitrate);
    }

    EXPECT_TRUE(result);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SET_DEFAULT_STREAM_ID, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    Status status = g_mediaDownloader->GetStreamInfo(streams);
    EXPECT_EQ(status, Status::OK);
    EXPECT_GT(streams.size(), 0u);
    int32_t videoId = -1;
    int32_t audioId = -1;
    int32_t subId = -1;
    for (const auto &s : streams) {
        if (s.type == VIDEO && videoId == -1) {
            videoId = s.streamId;
        } else if (s.type == AUDIO && audioId == -1) {
            audioId = s.streamId;
        } else if (s.type == SUBTITLE && subId == -1) {
            subId = s.streamId;
        }
    }
    g_mediaDownloader->SetDefaultStreamId(videoId, audioId, subId);
    std::vector<StreamInfo> streamsAfter;
    status = g_mediaDownloader->GetStreamInfo(streamsAfter, true);
    EXPECT_EQ(status, Status::OK);
    EXPECT_GE(streamsAfter.size(), 0u);
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

HWTEST_F(DashMediaDownloaderUnitTest, TEST_VIDEO_SEGMENT_DOWNLOAD_FINISHED_AUTO_SELECT, TestSize.Level1)
{
    auto mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    auto cb = std::make_shared<DashCallbackMock>();
    mediaDownloader->SetCallback(cb);
    mediaDownloader->SetIsTriggerAutoMode(true);
    std::string testUrl = MPD_SEGMENT_BASE;
    std::map<std::string, std::string> httpHeader;
    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();
    usleep(WAIT_FOR_SIDX_TIME);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0u);
    int32_t videoId = -1;
    for (const auto &s : streams) {
        if (s.type == VIDEO) {
            videoId = s.streamId;
            break;
        }
    }
    ASSERT_NE(videoId, -1);
    mediaDownloader->UpdateDownloadFinished(videoId);
    EXPECT_GE(cb->canAutoCount_, 1u);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SELECT_SUBTITLE, TestSize.Level1)
{
    auto mediaDownloader = CreateMultiAudioSubDownloader(true);

    std::vector<StreamInfo> streams;
    MultiAudioSubSwitchInfo switchInfo = BuildSwitchInfoAndPrepare(mediaDownloader, streams);
    EXPECT_GE(streams.size(), 0);

    Status status = Status::OK;

    if (switchInfo.switchingSubtitleStreamId != -1) {
        status = mediaDownloader->SelectStream(switchInfo.switchingSubtitleStreamId);
        mediaDownloader->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    }

    usleep(WAIT_FOR_SIDX_TIME);
    mediaDownloader->Close(false);
    mediaDownloader = nullptr;
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SELECT_BITRATE_AFTER_SWITCH, TestSize.Level1)
{
    auto mediaDownloader = CreateMultiAudioSubDownloader(false);

    std::vector<StreamInfo> streams;
    MultiAudioSubSwitchInfo switchInfo = BuildSwitchInfoAndPrepare(mediaDownloader, streams);
    EXPECT_GE(streams.size(), 0);

    Status status = mediaDownloader->SelectStream(switchInfo.switchingAudioStreamId);
    bool result = mediaDownloader->SelectBitRate(switchInfo.switchingBitrate);

    usleep(WAIT_FOR_SIDX_TIME);
    mediaDownloader->Close(false);
    mediaDownloader = nullptr;
    EXPECT_EQ(status, Status::OK);
    EXPECT_TRUE(result);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SELECT_SUBTITLE_AFTER_SWITCH, TestSize.Level1)
{
    auto mediaDownloader = CreateMultiAudioSubDownloader(false);

    std::vector<StreamInfo> streams;
    MultiAudioSubSwitchInfo switchInfo = BuildSwitchInfoAndPrepare(mediaDownloader, streams);
    EXPECT_GE(streams.size(), 0);

    bool result = mediaDownloader->SelectBitRate(switchInfo.switchingBitrate);
    Status status = mediaDownloader->SelectStream(switchInfo.switchingSubtitleStreamId);

    usleep(WAIT_FOR_SIDX_TIME);
    mediaDownloader->Close(false);
    mediaDownloader = nullptr;
    EXPECT_EQ(status, Status::OK);
    EXPECT_TRUE(result);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_SEEK_TO_TIME, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    std::string testUrl = MPD_SEGMENT_TEMPLATE;
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);

    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();
    bool result = mediaDownloader->SeekToTime(1, SeekMode::SEEK_NEXT_SYNC);
    mediaDownloader->Close(false);
    mediaDownloader = nullptr;
    EXPECT_TRUE(result);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_GET_READ, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    std::string testUrl = MPD_SEGMENT_LIST;
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);

    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();
    int32_t appUid = 1;
    mediaDownloader->SetAppUid(appUid);
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
    mediaDownloader->Close(false);
    mediaDownloader = nullptr;
    EXPECT_GE(readDataInfo.realReadLength_, 0);
}

HWTEST_F(DashMediaDownloaderUnitTest, GET_PLAYBACK_INFO_001, TestSize.Level1)
{
    int32_t defaultVideoStreamId = -1;
    int32_t defaultAudioStreamId = -1;
    int32_t defaultSubtitleStreamId = -1;
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    std::string testUrl = MPD_SEGMENT_LIST;
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);
    mediaDownloader->Open(testUrl, httpHeader);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);
    for (auto stream : streams) {
        if (stream.type == VIDEO && defaultVideoStreamId == -1) {
            defaultVideoStreamId = stream.streamId;
        }
        if (stream.type == AUDIO && defaultAudioStreamId == -1) {
            defaultAudioStreamId = stream.streamId;
        }
        if (stream.type == SUBTITLE && defaultSubtitleStreamId  == -1) {
            defaultSubtitleStreamId  = stream.streamId;
        }
    }
    mediaDownloader->SetDefaultStreamId(defaultVideoStreamId, defaultAudioStreamId, defaultSubtitleStreamId);
    mediaDownloader->GetSeekable();
    PlaybackInfo playbackInfo;
    mediaDownloader->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.serverIpAddress, "127.0.0.1");
    EXPECT_GT(playbackInfo.averageDownloadRate, 0);
    EXPECT_EQ(playbackInfo.isDownloading, false);
    EXPECT_GT(playbackInfo.downloadRate, 0);
    mediaDownloader->Close(false);
    mediaDownloader = nullptr;
}

HWTEST_F(DashMediaDownloaderUnitTest, STOP_BUFFERING_001, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
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
    EXPECT_EQ(mediaDownloader->StopBufferring(true), Status::OK);
    EXPECT_EQ(mediaDownloader->StopBufferring(false), Status::OK);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_RECEIVE_MPD_STREAM_INIT_EVENT_001, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    auto reportInfo = std::make_shared<OHOS::MediaAVCodec::SourceStatisticsReportInfo>();
    mediaDownloader->SetSourceStatisticsDfx(reportInfo);
    ASSERT_NE(mediaDownloader->reportInfo_, nullptr);

    std::string testUrl = MPD_SEGMENT_BASE;
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
    };
    mediaDownloader->SetStatusCallback(statusCallback);
    mediaDownloader->Open(testUrl, httpHeader);
    OSAL::SleepFor(WAIT_FOR_SIDX_TIME);

    EXPECT_GT(mediaDownloader->reportInfo_->videoStreamCnt_, 0u);

    mediaDownloader->Close(false);
    mediaDownloader = nullptr;
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_RECEIVE_MPD_STREAM_INIT_EVENT_002, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    EXPECT_NO_FATAL_FAILURE(mediaDownloader->OnMpdInfoUpdate(DASH_MPD_EVENT_STREAM_INIT));

    mediaDownloader->Close(false);
    mediaDownloader = nullptr;
}

std::shared_ptr<DashMediaDownloader> CreateAndInitMediaDownloader(const std::string& testUrl)
{
    auto mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    auto cb = std::make_shared<DashCallbackMock1>();
    cb->SetDownload(mediaDownloader);
    mediaDownloader->SetCallback(cb);
    std::map<std::string, std::string> httpHeader;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    mediaDownloader->SetStatusCallback(statusCallback);
    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();
    return mediaDownloader;
}

void PrepareReadDataInfo(ReadDataInfo& readDataInfo, int32_t streamId, int32_t nextStreamId, uint32_t wantReadLength)
{
    readDataInfo.streamId_ = streamId;
    readDataInfo.nextStreamId_ = nextStreamId;
    readDataInfo.wantReadLength_ = wantReadLength;
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_READ_001, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    unsigned char buff[1024];
    ReadDataInfo readDataInfo;
    PrepareReadDataInfo(readDataInfo, 10, streams[0].streamId, 1024);
    Status status = mediaDownloader->Read(buff, readDataInfo);
    EXPECT_EQ(status, Status::END_OF_STREAM);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_READ_002, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    unsigned char* buff = nullptr;
    ReadDataInfo readDataInfo;
    PrepareReadDataInfo(readDataInfo, streams[0].streamId, streams[0].streamId, 1024);
    Status status = mediaDownloader->Read(buff, readDataInfo);
    EXPECT_EQ(status, Status::END_OF_STREAM);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_READ_003, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    unsigned char buff[1024];
    ReadDataInfo readDataInfo;
    PrepareReadDataInfo(readDataInfo, -1, streams[0].streamId, 1024);
    Status status = mediaDownloader->Read(buff, readDataInfo);
    EXPECT_EQ(status, Status::END_OF_STREAM);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_READ_004, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    mediaDownloader->SetDownloadErrorState();
    unsigned char buff[1024];
    ReadDataInfo readDataInfo;
    PrepareReadDataInfo(readDataInfo, streams[0].streamId, streams[0].streamId, 1024);
    Status status = mediaDownloader->Read(buff, readDataInfo);
    EXPECT_EQ(status, Status::ERROR_AGAIN);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_READ_005, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    unsigned char buff[1024];
    ReadDataInfo readDataInfo;
    PrepareReadDataInfo(readDataInfo, streams[0].streamId, streams[0].streamId, 1024);
    Status status = Status::OK;
    int readCount = 0;
    int maxReadCount = 1000;
    while (status == Status::OK && readCount < maxReadCount) {
        status = mediaDownloader->Read(buff, readDataInfo);
        if (readDataInfo.realReadLength_ > 0) {
            readCount++;
        } else {
            usleep(100);
        }
    }
    EXPECT_EQ(status, Status::END_OF_STREAM);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_READ_006, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    mediaDownloader->SetInterruptState(true);
    unsigned char buff[1024];
    ReadDataInfo readDataInfo;
    PrepareReadDataInfo(readDataInfo, streams[0].streamId, streams[0].streamId, 1024);
    Status status = mediaDownloader->Read(buff, readDataInfo);
    EXPECT_EQ(status, Status::END_OF_STREAM);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_SET_DEMUXER_STATE_001, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    int32_t invalidStreamId = 9999;
    Status status = mediaDownloader->SetCurrentBitRate(0, invalidStreamId);
    EXPECT_EQ(status, Status::OK);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_UPDATE_DOWNLOAD_FINISHED_001, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    int32_t invalidStreamId = 9999;
    mediaDownloader->UpdateDownloadFinished(invalidStreamId);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_UPDATE_DOWNLOAD_FINISHED_002, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_MULTI_AUDIO_SUB);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    int32_t subtitleStreamId = -1;
    for (auto stream : streams) {
        if (stream.type == SUBTITLE) {
            subtitleStreamId = stream.streamId;
            break;
        }
    }
    
    if (subtitleStreamId != -1) {
        mediaDownloader->UpdateDownloadFinished(subtitleStreamId);
    }
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_GET_SEEKABLE_001, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    mediaDownloader->SetInterruptState(true);
    Seekable seekable = mediaDownloader->GetSeekable();
    EXPECT_EQ(seekable, Seekable::INVALID);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_GET_SEEKABLE_003, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    mediaDownloader->SetInterruptState(true);
    Seekable seekable = mediaDownloader->GetSeekable();
    EXPECT_EQ(seekable, Seekable::INVALID);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_POST_BUFFERING_EVENT_001, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    int32_t videoStreamId = -1;
    for (auto stream : streams) {
        if (stream.type == VIDEO) {
            videoStreamId = stream.streamId;
            break;
        }
    }
    EXPECT_NE(videoStreamId, -1);

    unsigned char buff[1024];
    ReadDataInfo readDataInfo;
    PrepareReadDataInfo(readDataInfo, videoStreamId, videoStreamId, 1024);
    
    Status status = Status::OK;
    int readCount = 0;
    int maxReadCount = 100;
    while (status == Status::OK && readCount < maxReadCount) {
        status = mediaDownloader->Read(buff, readDataInfo);
        if (readDataInfo.realReadLength_ > 0) {
            readCount++;
        } else {
            usleep(100);
        }
    }
    
    EXPECT_EQ(status, Status::OK);
    
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_POST_BUFFERING_EVENT_002, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);
    int32_t videoStreamId = -1;
    for (auto stream : streams) {
        if (stream.type == VIDEO) {
            videoStreamId = stream.streamId;
            break;
        }
    }
    EXPECT_NE(videoStreamId, -1);
    unsigned char buff[1024];
    ReadDataInfo readDataInfo;
    PrepareReadDataInfo(readDataInfo, videoStreamId, videoStreamId, 1024);
    Status status = Status::OK;
    int readCount = 0;
    int maxReadCount = 100;
    while (status == Status::OK && readCount < maxReadCount) {
        status = mediaDownloader->Read(buff, readDataInfo);
        if (readDataInfo.realReadLength_ > 0) {
            readCount++;
        } else {
            usleep(100);
        }
    }
    EXPECT_EQ(status, Status::OK);
    
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_SELECT_STREAM_001, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    int32_t invalidStreamId = 9999;
    Status status = mediaDownloader->SelectStream(invalidStreamId);
    EXPECT_EQ(status, Status::ERROR_INVALID_PARAMETER);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_SEEK_TO_TS_001, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    mediaDownloader->SeekToTs(-1);
    int64_t duration = mediaDownloader->GetDuration();
    EXPECT_GE(duration, 0);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_SEEK_TO_TS_002, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    int64_t duration = mediaDownloader->GetDuration();
    EXPECT_GE(duration, 0);
    int64_t seekTimeNs = INT64_MAX;
    mediaDownloader->SeekToTs(seekTimeNs);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_SET_PLAY_STRATEGY_001, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::shared_ptr<PlayStrategy> playStrategy = nullptr;
    mediaDownloader->SetPlayStrategy(playStrategy);
    int64_t duration = mediaDownloader->GetDuration();
    EXPECT_GE(duration, 0);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_ON_MPD_INFO_UPDATE_001, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    mediaDownloader->OnMpdInfoUpdate(DASH_MPD_EVENT_OPEN_OK);
    int64_t duration = mediaDownloader->GetDuration();
    EXPECT_GE(duration, 0);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_ON_MPD_INFO_UPDATE_003, TestSize.Level1)
{
    auto mediaDownloader = CreateAndInitMediaDownloader(MPD_SEGMENT_LIST);
    std::vector<StreamInfo> streams;
    mediaDownloader->GetStreamInfo(streams);
    EXPECT_GT(streams.size(), 0);

    int32_t videoStreamId = -1;
    for (auto stream : streams) {
        if (stream.type == VIDEO) {
            videoStreamId = stream.streamId;
            break;
        }
    }
    EXPECT_NE(videoStreamId, -1);
    unsigned char buff[1024];
    ReadDataInfo readDataInfo;
    PrepareReadDataInfo(readDataInfo, videoStreamId, videoStreamId, 1024);
    Status status = Status::OK;
    int readCount = 0;
    int maxReadCount = 1000;
    while (status == Status::OK && readCount < maxReadCount) {
        status = mediaDownloader->Read(buff, readDataInfo);
        if (readDataInfo.realReadLength_ > 0) {
            readCount++;
        } else {
            usleep(100);
        }
    }
    
    EXPECT_EQ(readDataInfo.isEos_, true);
    EXPECT_EQ(status, Status::END_OF_STREAM);
    mediaDownloader->OnMpdInfoUpdate(DASH_MPD_EVENT_STREAM_INIT);
    mediaDownloader->Close(false);
}

HWTEST_F(DashMediaDownloaderUnitTest, TEST_DASH_MEDIA_DOWNLOADER_OPEN_INIT_SEGMENT_001, TestSize.Level1)
{
    auto mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    std::string testUrl = MPD_SEGMENT_LIST;
    std::map<std::string, std::string> httpHeader;
    mediaDownloader->Open(testUrl, httpHeader);
    mediaDownloader->GetSeekable();
    mediaDownloader->OnMpdInfoUpdate(DASH_MPD_EVENT_STREAM_INIT);
    int64_t duration = mediaDownloader->GetDuration();
    EXPECT_GE(duration, 0);
    mediaDownloader->Close(false);
}
}
}
}
}