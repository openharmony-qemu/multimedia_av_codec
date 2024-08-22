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

#include "dash_segment_downloader_unit_test.h"
#include <iostream>
#include "dash_segment_downloader.h"
#include "http_server_demo.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr int32_t SERVERPORT = 47777;
// range 0-1065
static const std::string AUDIO_SEGMENT_URL = "http://127.0.0.1:47777/test_dash/segment_base/media-audio-und-mp4a.mp4";
static const std::string VIDEO_MEDIA_SEGMENT_URL_1 = "http://127.0.0.1:47777/test_dash/segment_list/video/1/seg-1.m4s";
static const std::string VIDEO_MEDIA_SEGMENT_URL_2 = "http://127.0.0.1:47777/test_dash/segment_list/video/1/seg-2.m4s";
static const std::string VIDEO_INIT_SEGMENT_URL = "http://127.0.0.1:47777/test_dash/segment_list/video/1/init.mp4";
}
using namespace testing::ext;

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
void DashSegmentDownloaderUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer(SERVERPORT);
    std::cout << "start" << std::endl;
}

void DashSegmentDownloaderUnitTest::TearDownTestCase(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

void DashSegmentDownloaderUnitTest::SetUp(void)
{
    if (g_server == nullptr) {
        g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
        g_server->StartServer(SERVERPORT);
        std::cout << "start server" << std::endl;
    }
}

void DashSegmentDownloaderUnitTest::TearDown(void) {}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_DASH_SEGMENT_DOWNLOADER, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    
    EXPECT_NE(segmentDownloader, nullptr);
    EXPECT_EQ(segmentDownloader->GetStreamId(), 1);
    EXPECT_EQ(segmentDownloader->GetStreamType(), MediaAVCodec::MediaType::MEDIA_TYPE_VID);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_OPEN, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = AUDIO_SEGMENT_URL;
    segmentSp->streamId_ = 1;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 1;
    segmentSp->numberSeq_ = 1;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloader->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloader->SetDownloadDoneCallback(doneCallback);
    bool result = segmentDownloader->Open(segmentSp);
    segmentDownloader->Pause();
    segmentDownloader->Resume();
    segmentDownloader->Close(true, true);
    segmentDownloader = nullptr;
    EXPECT_TRUE(result);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_OPEN_WITH_RANGE, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = AUDIO_SEGMENT_URL;
    segmentSp->streamId_ = 4;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 1;
    segmentSp->numberSeq_ = 1;
    segmentSp->startRangeValue_ = 0;
    segmentSp->endRangeValue_ = 1065;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloader->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloader->SetDownloadDoneCallback(doneCallback);
    bool result = segmentDownloader->Open(segmentSp);
    segmentDownloader->GetRingBufferSize();
    segmentDownloader->GetRingBufferCapacity();
    segmentDownloader->Close(true, true);
    segmentDownloader = nullptr;
    EXPECT_TRUE(result);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_OPEN_WITH_INIT_SEGMENT, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    
    std::shared_ptr<DashInitSegment> initSeg = std::make_shared<DashInitSegment>();
    initSeg->url_ = VIDEO_INIT_SEGMENT_URL;
    initSeg->streamId_ = 1;
    segmentDownloader->SetInitSegment(initSeg);

    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = VIDEO_MEDIA_SEGMENT_URL_1;
    segmentSp->streamId_ = 1;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 1;
    segmentSp->numberSeq_ = 1;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloader->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloader->SetDownloadDoneCallback(doneCallback);
    bool result = segmentDownloader->Open(segmentSp);
    segmentDownloader->Close(true, true);
    segmentDownloader = nullptr;

    EXPECT_TRUE(result);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_READ, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = AUDIO_SEGMENT_URL;
    segmentSp->streamId_ = 1;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 1;
    segmentSp->numberSeq_ = 1;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloader->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloader->SetDownloadDoneCallback(doneCallback);
    segmentDownloader->Open(segmentSp);
    int repeat = 0;
    bool status = false;
    while (repeat++ < 1000) {
        status = segmentDownloader->GetStartedStatus();
        if (status) {
            break;
        }
        OSAL::SleepFor(2);
    }

    unsigned char buffer[1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 1;
    readDataInfo.wantReadLength_ = 1024;
    readDataInfo.realReadLength_ = 0;
    readDataInfo.nextStreamId_ = 1;
    std::atomic<bool> isInterruptNeeded = false;
    DashReadRet result = segmentDownloader->Read(buffer, readDataInfo, isInterruptNeeded);
    EXPECT_EQ(result, DASH_READ_OK);
    EXPECT_EQ(readDataInfo.nextStreamId_, 1);
    EXPECT_GE(readDataInfo.realReadLength_, 0);
    readDataInfo.streamId_ = 2;
    result = segmentDownloader->Read(buffer, readDataInfo, isInterruptNeeded);
    segmentDownloader->Close(true, true);
    segmentDownloader = nullptr;
    EXPECT_EQ(result, DASH_READ_OK);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_CLEAN_SEGMENT_BUFFER, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = AUDIO_SEGMENT_URL;
    segmentSp->streamId_ = 1;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 1;
    segmentSp->numberSeq_ = 1;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloader->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloader->SetDownloadDoneCallback(doneCallback);
    segmentDownloader->Open(segmentSp);
    int repeat = 0;
    bool status = false;
    while (repeat++ < 1000) {
        status = segmentDownloader->GetStartedStatus();
        if (status) {
            break;
        }
        OSAL::SleepFor(2);
    }

    int64_t remainLastNumberSeq = -1;
    segmentDownloader->CleanSegmentBuffer(false, remainLastNumberSeq);
    segmentDownloader->Close(true, true);
    segmentDownloader = nullptr;

    EXPECT_EQ(remainLastNumberSeq, 1);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_CLEAN_SEGMENT_BUFFER_ALL, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = AUDIO_SEGMENT_URL;
    segmentSp->streamId_ = 1;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 1;
    segmentSp->numberSeq_ = 1;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloader->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloader->SetDownloadDoneCallback(doneCallback);
    segmentDownloader->Open(segmentSp);

    int64_t remainLastNumberSeq = -1;
    bool result = segmentDownloader->CleanSegmentBuffer(true, remainLastNumberSeq);
    segmentDownloader->Close(true, true);
    segmentDownloader = nullptr;

    EXPECT_TRUE(result);
    EXPECT_EQ(remainLastNumberSeq, 1);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_UPDATE_STREAM_ID, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    segmentDownloader->UpdateStreamId(2);
    EXPECT_EQ(segmentDownloader->GetStreamId(), 2);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_GET_CONTENT_LENGTH, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    EXPECT_EQ(segmentDownloader->GetContentLength(), 0);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_GET_IS_SEGMENT_FINISH, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    EXPECT_FALSE(segmentDownloader->IsSegmentFinish());
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_SEEK_TO_TIME, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = VIDEO_MEDIA_SEGMENT_URL_1;
    segmentSp->streamId_ = 1;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 1;
    segmentSp->numberSeq_ = 1;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloader->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloader->SetDownloadDoneCallback(doneCallback);
    segmentDownloader->Open(segmentSp);

    std::shared_ptr<DashSegment> seekSegment = std::make_shared<DashSegment>();
    segmentSp->url_ = VIDEO_MEDIA_SEGMENT_URL_2;
    segmentSp->streamId_ = 1;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 1;
    segmentSp->numberSeq_ = 2;

    bool result = segmentDownloader->SeekToTime(seekSegment);
    segmentDownloader->Close(true, true);
    segmentDownloader = nullptr;
    
    EXPECT_FALSE(result);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_DASH_SEGMENT_DOWNLOADER_SUCCESS_001, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloaderSp = std::make_shared<DashSegmentDownloader>(nullptr,
            0, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = VIDEO_MEDIA_SEGMENT_URL_1;
    segmentSp->streamId_ = 0;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 0;
    segmentSp->numberSeq_ = 0;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloaderSp->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloaderSp->SetDownloadDoneCallback(doneCallback);
    segmentDownloaderSp->Open(segmentSp);
    OSAL::SleepFor(1000);
    segmentDownloaderSp->Close(true, true);
    bool status = segmentDownloaderSp->GetStartedStatus();
    EXPECT_GE(status, false);
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_DASH_SEGMENT_DOWNLOADER_WATERLINE_001, TestSize.Level1)
{
    std::shared_ptr<DashSegmentDownloader> segmentDownloaderSp = std::make_shared<DashSegmentDownloader>(nullptr,
        0, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = VIDEO_MEDIA_SEGMENT_URL_1;
    segmentSp->streamId_ = 0;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 0;
    segmentSp->numberSeq_ = 0;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloaderSp->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloaderSp->SetDownloadDoneCallback(doneCallback);
    segmentDownloaderSp->SetDemuxerState();
    segmentDownloaderSp->SetCurrentBitRate(1048576);
    unsigned char buffer[1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 1;
    readDataInfo.wantReadLength_ = 1024;
    readDataInfo.realReadLength_ = 0;
    readDataInfo.nextStreamId_ = 1;
    std::atomic<bool> isInterruptNeeded = false;
    DashReadRet result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    EXPECT_EQ(result, DASH_READ_AGAIN);
    result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    EXPECT_EQ(result, DASH_READ_AGAIN);
    segmentDownloaderSp->Open(segmentSp);
    OSAL::SleepFor(1000);
    result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    segmentDownloaderSp->Close(true, true);
    EXPECT_GE(result, DASH_READ_SEGMENT_DOWNLOAD_FINISH);
}

class SourceCallbackMock : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        (void)event;
    }

    void SetSelectBitRateFlag(bool flag) override
    {
        (void)flag;
    }

    bool CanAutoSelectBitRate() override
    {
        return true;
    }
};

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_DASH_SEGMENT_DOWNLOADER_WATERLINE_002, TestSize.Level1)
{
    Plugins::Callback* sourceCallback = new SourceCallbackMock();
    std::shared_ptr<DashSegmentDownloader> segmentDownloaderSp = std::make_shared<DashSegmentDownloader>(sourceCallback,
        0, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = VIDEO_MEDIA_SEGMENT_URL_1;
    segmentSp->streamId_ = 0;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 0;
    segmentSp->numberSeq_ = 0;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloaderSp->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloaderSp->SetDownloadDoneCallback(doneCallback);
    segmentDownloaderSp->SetDemuxerState();
    segmentDownloaderSp->SetCurrentBitRate(1048576);
    unsigned char buffer[1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 1;
    readDataInfo.wantReadLength_ = 1024;
    readDataInfo.realReadLength_ = 0;
    readDataInfo.nextStreamId_ = 1;
    std::atomic<bool> isInterruptNeeded = false;
    DashReadRet result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    EXPECT_EQ(result, DASH_READ_AGAIN);
    result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    EXPECT_EQ(result, DASH_READ_AGAIN);
    segmentDownloaderSp->Open(segmentSp);
    segmentDownloaderSp->SetAllSegmentFinished();
    result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    segmentDownloaderSp->Close(true, true);
    EXPECT_EQ(result, DASH_READ_END);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_DASH_SEGMENT_DOWNLOADER_WATERLINE_003, TestSize.Level1)
{
    Plugins::Callback* sourceCallback = new SourceCallbackMock();
    std::shared_ptr<DashSegmentDownloader> segmentDownloaderSp = std::make_shared<DashSegmentDownloader>(sourceCallback,
        0, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = VIDEO_MEDIA_SEGMENT_URL_1;
    segmentSp->streamId_ = 0;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 0;
    segmentSp->numberSeq_ = 0;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloaderSp->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloaderSp->SetDownloadDoneCallback(doneCallback);
    segmentDownloaderSp->SetDemuxerState();
    segmentDownloaderSp->SetCurrentBitRate(1048576000);
    unsigned char buffer[1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 1;
    readDataInfo.wantReadLength_ = 1024;
    readDataInfo.realReadLength_ = 0;
    readDataInfo.nextStreamId_ = 1;
    std::atomic<bool> isInterruptNeeded = false;
    DashReadRet result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    EXPECT_EQ(result, DASH_READ_AGAIN);
    result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    EXPECT_EQ(result, DASH_READ_AGAIN);
    segmentDownloaderSp->Open(segmentSp);
    result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    segmentDownloaderSp->Close(true, true);
    EXPECT_EQ(result, DASH_READ_AGAIN);
    delete sourceCallback;
    sourceCallback = nullptr;
}

HWTEST_F(DashSegmentDownloaderUnitTest, TEST_DASH_SEGMENT_DOWNLOADER_WATERLINE_004, TestSize.Level1)
{
    Plugins::Callback* sourceCallback = new SourceCallbackMock();
    std::shared_ptr<DashSegmentDownloader> segmentDownloaderSp = std::make_shared<DashSegmentDownloader>(sourceCallback,
        0, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = "http://test/index.mpd";
    segmentSp->streamId_ = 0;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 0;
    segmentSp->numberSeq_ = 0;
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    segmentDownloaderSp->SetStatusCallback(statusCallback);
    auto doneCallback = [] (int streamId) {};
    segmentDownloaderSp->SetDownloadDoneCallback(doneCallback);
    segmentDownloaderSp->SetDemuxerState();
    segmentDownloaderSp->SetCurrentBitRate(1048576);
    unsigned char buffer[1024];
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 1;
    readDataInfo.wantReadLength_ = 1024;
    readDataInfo.realReadLength_ = 0;
    readDataInfo.nextStreamId_ = 1;
    std::atomic<bool> isInterruptNeeded = false;
    segmentDownloaderSp->Open(segmentSp);
    DashReadRet result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    EXPECT_EQ(result, DASH_READ_AGAIN);
    result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    EXPECT_EQ(result, DASH_READ_AGAIN);
    isInterruptNeeded.store(true);
    result = segmentDownloaderSp->Read(buffer, readDataInfo, isInterruptNeeded);
    segmentDownloaderSp->Close(true, true);
    EXPECT_EQ(result, DASH_READ_INTERRUPT);
    delete sourceCallback;
    sourceCallback = nullptr;
}
}
}
}
}