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

#include <iostream>

#include "dash_mpd_downloader.h"
#include "dash_mpd_downloader_unit_test.h"
#include "dash_mpd_util.h"
#include "gtest/gtest.h"
#include "http_server_demo.h"
#include "test_http_port.h"
#include "utils/time_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
static const std::string MPD_SEGMENT_BASE = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/test_dash/segment_base/index.mpd";
static const std::string MPD_SEGMENT_LIST = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/test_dash/segment_list/index.mpd";
static const std::string MPD_SEGMENT_TEMPLATE = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/test_dash/segment_template/index.mpd";
static const std::string MPD_SEGMENT_LIST_TIMELINE = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/test_dash/segment_list/index_timeline.mpd";
static const std::string MPD_SEGMENT_TEMPLATE_ADPT = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/test_dash/segment_template/index_adpt.mpd";
static const std::string URL_TEMPLATE_TIMELINE = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/test_dash/segment_template/index_timeline.mpd";
static const std::string URL_SEGMENT_BASE_IN_PERIOD = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/test_dash/segment_base/index_period.mpd";
constexpr unsigned int INIT_WIDTH = 1280;
constexpr unsigned int INIT_HEIGHT = 720;
}

using namespace testing::ext;

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
std::shared_ptr<DashMpdDownloader> g_mpdDownloader = nullptr;

void PrepareDefaultStreamSelection(const std::shared_ptr<DashMpdDownloader>& downloader)
{
    if (downloader == nullptr) {
        return;
    }

    std::vector<StreamInfo> streams;
    int32_t defaultVideoStreamId = -1;
    int32_t defaultAudioStreamId = -1;
    int32_t defaultSubtitleStreamId = -1;
    if (downloader->GetStreamInfo(streams) == Status::OK) {
        for (const auto& stream : streams) {
            if (stream.type == VIDEO && defaultVideoStreamId == -1) {
                defaultVideoStreamId = stream.streamId;
                continue;
            }
            if (stream.type == AUDIO && defaultAudioStreamId == -1) {
                defaultAudioStreamId = stream.streamId;
                continue;
            }
            if (stream.type == SUBTITLE && defaultSubtitleStreamId == -1) {
                defaultSubtitleStreamId = stream.streamId;
            }
        }
    }
    if (defaultVideoStreamId != -1) {
        downloader->SetDefaultStreamId(defaultVideoStreamId,
                                       defaultAudioStreamId,
                                       defaultSubtitleStreamId);
        downloader->GetSeekable();
    }
}

class MpdCallbackMock : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        lastEventType_ = event.type;
        onEventCount_++;
    }

    void OnDfxEvent(const Plugins::PluginDfxEvent &event) override
    {
        (void)event;
    }

    void SetSelectBitRateFlag(bool flag, uint32_t desBitRate) override
    {
        (void)flag;
        (void)desBitRate;
    }

    bool CanAutoSelectBitRate() override
    {
        return true;
    }

    Plugins::PluginEventType lastEventType_ {Plugins::PluginEventType::CLIENT_ERROR};
    uint32_t onEventCount_ {0};
};

class TestDashMpdCallback : public DashMpdCallback {
public:
    void OnMpdInfoUpdate(DashMpdEvent mpdEvent) override
    {
        lastEvent_ = mpdEvent;
        eventCount_++;
    }

    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos) override
    {
        (void)drmInfos;
        drmEventCount_++;
    }

    void OnTimedMetaDataChanged(const std::shared_ptr<MediaAVCodec::AVTimedMetaData> &metadata) override
    {
        (void)metadata;
    }

    DashMpdEvent lastEvent_ {DASH_MPD_EVENT_OPEN_OK};
    uint32_t eventCount_ {0};
    uint32_t drmEventCount_ {0};
};

void DashMpdDownloaderUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
    std::cout << "start" << std::endl;

    g_mpdDownloader = std::make_shared<DashMpdDownloader>();
    g_mpdDownloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    g_mpdDownloader->SetStatusCallback(statusCallback);
    g_mpdDownloader->SetInitResolution(INIT_WIDTH, INIT_HEIGHT);
    g_mpdDownloader->Open(MPD_SEGMENT_BASE);
    g_mpdDownloader->GetSeekable();

    PrepareDefaultStreamSelection(g_mpdDownloader);
}

void DashMpdDownloaderUnitTest::TearDownTestCase(void)
{
    g_server->StopServer();
    g_server = nullptr;
}

void DashMpdDownloaderUnitTest::SetUp(void)
{
    if (g_server == nullptr) {
        g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
        g_server->StartServer();
        std::cout << "start server" << std::endl;
    }

    mpdDownloader_ = std::make_shared<DashMpdDownloader>();
    mpdDownloader_->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    mpdDownloader_->SetStatusCallback(statusCallback);
    mpdDownloader_->SetInitResolution(INIT_WIDTH, INIT_HEIGHT);
    mpdDownloader_->Open(MPD_SEGMENT_BASE);
    mpdDownloader_->GetSeekable();

    PrepareDefaultStreamSelection(mpdDownloader_);
}

void DashMpdDownloaderUnitTest::TearDown(void)
{
    mpdDownloader_->SetInterruptState(true);
    mpdDownloader_->Close(true);
    mpdDownloader_ = nullptr;
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_SET_CALLBACK, TestSize.Level1)
{
    auto downloader = std::make_shared<DashMpdDownloader>();
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {
        (void)status;
        (void)downloader;
        (void)request;
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->SetInitResolution(INIT_WIDTH, INIT_HEIGHT);
    auto cb = std::make_shared<MpdCallbackMock>();
    downloader->SetCallback(cb);
    std::string testUrl = MPD_SEGMENT_BASE;
    downloader->Open(testUrl);
    (void)downloader->GetSeekable();
    EXPECT_GE(cb->onEventCount_, 1u);
    EXPECT_EQ(cb->lastEventType_, Plugins::PluginEventType::STREAM_UPDATE);
    downloader->SetInterruptState(true);
    downloader->Close(true);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_PARSE_MANIFEST_EMPTY_CONTENT, TestSize.Level1)
{
    auto downloader = std::make_shared<DashMpdDownloader>();
    downloader->Init();
    std::string fakeUrl = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/test_dash/empty/index.mpd";
    downloader->UpdateDownloadFinished(fakeUrl);
    std::vector<StreamInfo> streams;
    Status status = downloader->GetStreamInfo(streams);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(streams.size(), 0u);
    downloader->SetInterruptState(true);
    downloader->Close(true);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_SET_DEFAULT_STREAM_ID, TestSize.Level1)
{
    auto downloader = std::make_shared<DashMpdDownloader>();
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {
        (void)status;
        (void)downloader;
        (void)request;
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->SetInitResolution(INIT_WIDTH, INIT_HEIGHT);
    auto mpdCallback = std::make_shared<TestDashMpdCallback>();
    downloader->SetMpdCallback(mpdCallback);
    std::string testUrl = MPD_SEGMENT_BASE;
    downloader->Open(testUrl);
    (void)downloader->GetSeekable();
    std::vector<StreamInfo> streams;
    Status status = downloader->GetStreamInfo(streams);
    ASSERT_EQ(status, Status::OK);
    int32_t videoStreamId = -1;
    int32_t audioStreamId = -1;
    int32_t subtitleStreamId = -1;
    for (auto &info : streams) {
        auto desc = downloader->GetStreamByStreamId(info.streamId);
        ASSERT_NE(desc, nullptr);
        if (desc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_VID &&
            !desc->inUse_ && videoStreamId == -1) {
            videoStreamId = desc->streamId_;
        } else if (desc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD &&
                   audioStreamId == -1) {
            audioStreamId = desc->streamId_;
        } else if (desc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE &&
                   subtitleStreamId == -1) {
            subtitleStreamId = desc->streamId_;
        }
    }
    downloader->SetDefaultStreamId(videoStreamId, audioStreamId, subtitleStreamId);
    if (videoStreamId != -1) {
        auto videoDesc = downloader->GetStreamByStreamId(videoStreamId);
        ASSERT_NE(videoDesc, nullptr);
        EXPECT_TRUE(videoDesc->inUse_);
    }
    downloader->SetInterruptState(true);
    downloader->Close(true);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_STREAMS_INFO_IN_ADPT_SET, TestSize.Level1)
{
    auto downloader = std::make_shared<DashMpdDownloader>();
    downloader->Init();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {
        (void)status;
        (void)downloader;
        (void)request;
    };
    downloader->SetStatusCallback(statusCallback);
    downloader->SetInitResolution(INIT_WIDTH, INIT_HEIGHT);
    std::string testUrl = MPD_SEGMENT_TEMPLATE_ADPT;
    downloader->Open(testUrl);
    (void)downloader->GetSeekable();
    std::vector<StreamInfo> streams;
    Status status = downloader->GetStreamInfo(streams);
    EXPECT_EQ(status, Status::OK);
    EXPECT_GT(streams.size(), 0u);
    downloader->SetInterruptState(true);
    downloader->Close(true);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_SET_STREAM_INFO, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    Status status = mpdDownloader_->GetStreamInfo(streams);
    ASSERT_EQ(status, Status::OK);
    for (auto &info : streams) {
        auto desc = mpdDownloader_->GetStreamByStreamId(info.streamId);
        ASSERT_NE(desc, nullptr);

        EXPECT_EQ(info.bitRate, desc->bandwidth_);
        EXPECT_EQ(info.videoWidth, static_cast<int32_t>(desc->width_));
        EXPECT_EQ(info.videoHeight, static_cast<int32_t>(desc->height_));
        EXPECT_EQ(info.lang, desc->lang_);
        EXPECT_EQ(info.minBitRate, desc->minBandwidth_);
        EXPECT_EQ(info.maxBitRate, desc->maxBandwidth_);
        EXPECT_EQ(info.minWidth, desc->minWidth_);
        EXPECT_EQ(info.maxWidth, desc->maxWidth_);
        EXPECT_EQ(info.minHeight, desc->minHeight_);
        EXPECT_EQ(info.maxHeight, desc->maxHeight_);
        EXPECT_EQ(info.mimeType, desc->mimeType_);
        if (desc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
            EXPECT_FALSE(info.mimeType.empty());
        } else if (desc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
            EXPECT_TRUE(info.mimeType.rfind("audio", 0) == 0);
        }
        if (!desc->minFrameRate_.empty()) {
            EXPECT_EQ(info.minFrameRate, std::stoi(desc->minFrameRate_));
        }
        if (!desc->maxFrameRate_.empty()) {
            EXPECT_EQ(info.maxFrameRate, std::stoi(desc->maxFrameRate_));
        }
        if (desc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
            EXPECT_EQ(info.type, SUBTITLE);
        } else if (desc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
            EXPECT_EQ(info.type, AUDIO);
        } else {
            EXPECT_EQ(info.type, VIDEO);
        }
    }
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_OPEN, TestSize.Level1)
{
    std::string testUrl = MPD_SEGMENT_BASE;
    EXPECT_EQ(testUrl, mpdDownloader_->GetUrl());

    Seekable value = mpdDownloader_->GetSeekable();
    EXPECT_EQ(Seekable::SEEKABLE, value);

    EXPECT_GE(mpdDownloader_->GetDuration(), 0);

    std::vector<uint32_t> bitrates = mpdDownloader_->GetBitRates();
    EXPECT_GE(bitrates.size(), 0);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_BITRATES_BY_HDR, TestSize.Level1)
{
    std::vector<uint32_t> sdrBitrates = mpdDownloader_->GetBitRatesByHdr(false);
    EXPECT_GE(sdrBitrates.size(), 0);

    std::vector<uint32_t> hdrBitrates = mpdDownloader_->GetBitRatesByHdr(true);
    EXPECT_GE(hdrBitrates.size(), 0);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_STREAM_INFO, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    Status status = mpdDownloader_->GetStreamInfo(streams);

    EXPECT_GE(streams.size(), 0);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_BREAK_POINT_SEGMENT, TestSize.Level1)
{
    int usingStreamId = mpdDownloader_->GetInUseVideoStreamId();
    std::shared_ptr<DashSegment> seg = nullptr;
    int64_t breakPoint = static_cast<int64_t>(30000 * MS_2_NS);
    DashMpdGetRet getRet = mpdDownloader_->GetBreakPointSegment(usingStreamId, breakPoint, seg);

    std::shared_ptr<DashSegment> segError = nullptr;
    int64_t breakPointError = static_cast<int64_t>(300000 * MS_2_NS);
    mpdDownloader_->GetBreakPointSegment(usingStreamId, breakPointError, segError);

    EXPECT_NE(getRet, DASH_MPD_GET_ERROR);
    EXPECT_EQ(segError, nullptr);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_SEEK_TO_TS, TestSize.Level1)
{
    int usingStreamId = mpdDownloader_->GetInUseVideoStreamId();
    int64_t duration = mpdDownloader_->GetDuration();
    int64_t seekTime = duration / 2;
    std::shared_ptr<DashSegment> seg = nullptr;
    mpdDownloader_->SeekToTs(usingStreamId, seekTime, seg);

    ASSERT_NE(seg, nullptr);
    EXPECT_GE(seg->numberSeq_, 1); // segment sequence number must be valid (>= startNumberSeq)
    EXPECT_GT(seg->duration_, 0);  // segment duration must be positive
    EXPECT_FALSE(seg->url_.empty()); // segment URL must be populated
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_NEXT_SEGMENT, TestSize.Level1)
{
    int usingStreamId = mpdDownloader_->GetInUseVideoStreamId();
    std::shared_ptr<DashSegment> seg = nullptr;
    mpdDownloader_->GetNextSegmentByStreamId(usingStreamId, seg);
    EXPECT_NE(seg, nullptr);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_INIT_SEGMENT, TestSize.Level1)
{
    int usingStreamId = mpdDownloader_->GetInUseVideoStreamId();
    std::shared_ptr<DashInitSegment> initSeg = mpdDownloader_->GetInitSegmentByStreamId(usingStreamId);
    EXPECT_NE(initSeg, nullptr);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_SET_CURRENT_NUMBER_SEQ, TestSize.Level1)
{
    int usingStreamId = mpdDownloader_->GetInUseVideoStreamId();
    mpdDownloader_->SetCurrentNumberSeqByStreamId(usingStreamId, 10);
    std::shared_ptr<DashStreamDescription> stream = mpdDownloader_->GetStreamByStreamId(usingStreamId);
    ASSERT_NE(stream, nullptr);
    EXPECT_EQ(stream->currentNumberSeq_, 10);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_NEXT_VIDEO_STREAM, TestSize.Level1)
{
    int usingStreamId = mpdDownloader_->GetInUseVideoStreamId();
    std::shared_ptr<DashStreamDescription> stream = mpdDownloader_->GetStreamByStreamId(usingStreamId);
    EXPECT_NE(stream, nullptr);

    uint32_t switchingBitrate = 0;
    std::vector<uint32_t> bitrates = mpdDownloader_->GetBitRates();
    EXPECT_GT(bitrates.size(), 0);
    for (auto bitrate : bitrates) {
        if (bitrate != stream->bandwidth_) {
            switchingBitrate = bitrate;
            break;
        }
    }

    DashMpdGetRet ret = DASH_MPD_GET_ERROR;
    DashMpdBitrateParam bitrateParam;
    bitrateParam.bitrate_ = switchingBitrate;
    bitrateParam.type_ = DASH_MPD_SWITCH_TYPE_SMOOTH;
    ret = mpdDownloader_->GetNextVideoStream(bitrateParam, bitrateParam.streamId_);

    EXPECT_NE(DASH_MPD_GET_ERROR, ret);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_NEXT_TRACK_STREAM, TestSize.Level1)
{
    std::shared_ptr<DashStreamDescription> audioStream = mpdDownloader_->GetUsingStreamByType(
        MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
    EXPECT_NE(audioStream, nullptr);

    DashMpdTrackParam trackParam;
    trackParam.isEnd_ = false;
    trackParam.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    trackParam.streamId_ = audioStream->streamId_;
    trackParam.position_ = 2; // update by segment sequence, -1 means no segment downloading

    DashMpdGetRet ret = mpdDownloader_->GetNextTrackStream(trackParam);
    EXPECT_NE(DASH_MPD_GET_ERROR, ret);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_SET_HDR_START, TestSize.Level1)
{
    auto downloader = std::make_shared<DashMpdDownloader>();
    downloader->Init();
    std::string testUrl = MPD_SEGMENT_LIST;
    downloader->SetHdrStart(true);
    downloader->Open(testUrl);
    downloader->GetSeekable();
    PrepareDefaultStreamSelection(downloader);
    int usingStreamId = downloader->GetInUseVideoStreamId();
    std::shared_ptr<DashStreamDescription> stream = downloader->GetStreamByStreamId(usingStreamId);
    EXPECT_NE(stream, nullptr);
    EXPECT_FALSE(stream->videoType_ == DASH_VIDEO_TYPE_HDR_VIVID);
    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_OPEN_TIMELINE_MPD, TestSize.Level1)
{
    auto downloader = std::make_shared<DashMpdDownloader>();
    downloader->Init();
    std::string testUrl = MPD_SEGMENT_LIST_TIMELINE;
    downloader->Open(testUrl);
    downloader->GetSeekable();
    int64_t duration = downloader->GetDuration();
    EXPECT_GE(duration, 0);
    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_OPEN_TEMPLATE_TIMELINE_MPD, TestSize.Level1)
{
    auto downloader = std::make_shared<DashMpdDownloader>();
    downloader->Init();
    std::string testUrl = URL_TEMPLATE_TIMELINE;
    downloader->Open(testUrl);
    downloader->GetSeekable();
    int64_t duration = downloader->GetDuration();
    EXPECT_GE(duration, 0);
    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_OPEN_ADPT_MPD, TestSize.Level1)
{
    auto downloader = std::make_shared<DashMpdDownloader>();
    downloader->Init();
    std::string testUrl = MPD_SEGMENT_TEMPLATE_ADPT;
    downloader->Open(testUrl);
    downloader->GetSeekable();
    int64_t duration = downloader->GetDuration();
    EXPECT_GE(duration, 0);
    downloader->Close(true);
    downloader = nullptr;
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_OPEN_PERIOD_MPD, TestSize.Level1)
{
    auto downloader = std::make_shared<DashMpdDownloader>();
    downloader->Init();
    std::string testUrl = URL_SEGMENT_BASE_IN_PERIOD;
    downloader->Open(testUrl);
    downloader->GetSeekable();
    int64_t duration = downloader->GetDuration();
    EXPECT_GE(duration, 0);
    downloader->Close(true);
    downloader = nullptr;
}

/**
 * @tc.name  : Test SaveData
 * @tc.number: SaveData_001
 * @tc.desc  : Test data == nullptr
 */
HWTEST_F(DashMpdDownloaderUnitTest, SaveData_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    uint8_t* data = nullptr;
    uint32_t len = 0;
    bool notBlock = false;
    auto ret = mpdDownloader_->SaveData(data, len, notBlock);
    EXPECT_EQ(ret, 0u);
}

/**
 * @tc.name  : Test SaveData
 * @tc.number: SaveData_002
 * @tc.desc  : Test len > MPD_MAX_SIZE with large buffer
 */
HWTEST_F(DashMpdDownloaderUnitTest, SaveData_002, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::vector<uint8_t> data(100 * 1024 * 1024 + 1); // 分配足够大的缓冲区
    std::fill(data.begin(), data.end(), 0x55); // 填充数据
    uint32_t len = data.size();
    bool notBlock = false;
    auto ret = mpdDownloader_->SaveData(data.data(), len, notBlock);
    EXPECT_EQ(ret, 0u);  // 应该返回0，因为 len > MPD_MAX_SIZE
}

/**
 * @tc.name  : Test SaveData
 * @tc.number: SaveData_003
 * @tc.desc  : Test normal save data
 */
HWTEST_F(DashMpdDownloaderUnitTest, SaveData_003, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->downloadContent_.clear();
    uint8_t data[] = {1, 2, 3, 4, 5};
    uint32_t len = 5;
    bool notBlock = false;
    auto ret = mpdDownloader_->SaveData(data, len, notBlock);
    EXPECT_EQ(ret, 5u);
    EXPECT_EQ(mpdDownloader_->downloadContent_.length(), 5u);
}

/**
 * @tc.name  : Test GetSegmentsWithSegTemplate
 * @tc.number: GetSegmentsWithSegTemplate_002
 * @tc.desc  : Test GetSegmentsWithSegTemplate with dynamic type
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsWithSegTemplate_002, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashSegTmpltInfo* segTmpltInfo = new DashSegTmpltInfo();
    segTmpltInfo->segTmpltMedia_ = "segment$Number$.m4s";
    segTmpltInfo->multSegBaseInfo_.duration_ = 1000;
    segTmpltInfo->multSegBaseInfo_.segBaseInfo_.timeScale_ = 1000;
    std::string id = "rep1";
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->bandwidth_ = 1000000;
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    auto ret = mpdDownloader_->GetSegmentsWithSegTemplate(segTmpltInfo, id, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);
    delete segTmpltInfo;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetSegmentsInMpd
 * @tc.number: GetSegmentsInMpd_003
 * @tc.desc  : Test GetSegmentsInMpd with null mpdInfo
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsInMpd_003, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->mpdInfo_ = nullptr;
    auto streamDesc = std::make_shared<DashStreamDescription>();
    auto ret = mpdDownloader_->GetSegmentsInMpd(streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);
}

/**
 * @tc.name  : Test SetDownloadRequest and GetDownloadRequest
 * @tc.number: SetDownloadRequest_001
 * @tc.desc  : Test SetDownloadRequest and GetDownloadRequest
 */
HWTEST_F(DashMpdDownloaderUnitTest, SetDownloadRequest_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::string url = "testUrl";
    DataSaveFunc saveData;
    StatusCallbackFunc statusCallback;
    bool requestWholeFile = false;
    auto downloadRequest = std::make_shared<DownloadRequest>(url, saveData, statusCallback, requestWholeFile);
    mpdDownloader_->SetDownloadRequest(downloadRequest);
    auto ret = mpdDownloader_->GetDownloadRequest();
    EXPECT_NE(ret, nullptr);
    EXPECT_EQ(ret->GetUrl(), url);
}

/**
 * @tc.name  : Test PutStreamToDownload
 * @tc.number: PutStreamToDownload_002
 * @tc.desc  : Test PutStreamToDownload with valid stream
 */
HWTEST_F(DashMpdDownloaderUnitTest, PutStreamToDownload_002, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto ret = mpdDownloader_->PutStreamToDownload();
    EXPECT_EQ(ret, false);
    auto stream = std::make_shared<DashStreamDescription>();
    stream->inUse_ = true;
    stream->segsState_ = DASH_SEGS_STATE_NONE;
    stream->indexSegment_ = std::make_shared<DashIndexSegment>();
    stream->indexSegment_->url_ = "http://test.com/segment.mp4";
    mpdDownloader_->streamDescriptions_.push_back(stream);
    ret = mpdDownloader_->PutStreamToDownload();
    EXPECT_EQ(ret, true);
    mpdDownloader_->streamDescriptions_.clear();
}

/**
 * @tc.name  : Test SetMpdCallback
 * @tc.number: SetMpdCallback_001
 * @tc.desc  : Test SetMpdCallback
 */
HWTEST_F(DashMpdDownloaderUnitTest, SetMpdCallback_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto callback = std::make_shared<MockDashMpdCallback>();
    mpdDownloader_->SetMpdCallback(callback);
    EXPECT_NE(mpdDownloader_->callback_.lock(), nullptr);
}

/**
 * @tc.name  : Test ProcessDrmInfos
 * @tc.number: ProcessDrmInfos_001
 * @tc.desc  : Test ProcessDrmInfos with valid DRM info
 */
HWTEST_F(DashMpdDownloaderUnitTest, ProcessDrmInfos_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* period = new DashPeriodInfo();
    period->id_ = "period1";
    DashAdptSetInfo* adptSet = new DashAdptSetInfo();
    adptSet->id_ = 1;
    DashDescriptor* contentProtection = new DashDescriptor();
    contentProtection->schemeIdUrl_ = "urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
    contentProtection->elementMap_[MPD_LABEL_PSSH] = "AAACAAE=";
    adptSet->commonAttrsAndElements_.contentProtectionList_.push_back(contentProtection);
    period->adptSetList_.push_back(adptSet);
    mpdInfo->periodInfoList_.push_back(period);
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->ProcessDrmInfos();
    EXPECT_GT(mpdDownloader_->localDrmInfos_.size(), 0u);
    delete contentProtection;
    delete adptSet;
    delete period;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetSegmentsWithTmpltDurationStatic
 * @tc.number: GetSegmentsWithTmpltDurationStatic_003
 * @tc.desc  : Test GetSegmentsWithTmpltDurationStatic with zero duration
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsWithTmpltDurationStatic_003, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashSegTmpltInfo* segTmpltInfo = new DashSegTmpltInfo();
    segTmpltInfo->multSegBaseInfo_.duration_ = 0;
    std::string mediaUrl = "segment$Number$.m4s";
    unsigned int timeScale = 1000;
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->duration_ = 10000;
    streamDesc->startNumberSeq_ = 1;
    auto ret = mpdDownloader_->GetSegmentsWithTmpltDurationStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);
    delete segTmpltInfo;
}

/**
 * @tc.name  : Test GetSegmentsInMpd
 * @tc.number: GetSegmentsInMpd_002
 * @tc.desc  : Test GetSegmentsInMpd with valid streamDesc
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsInMpd_002, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* period = new DashPeriodInfo();
    mpdInfo->periodInfoList_.push_back(period);
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->url_ = "http://test.com/test.mpd";
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->periodIndex_ = 0;
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc->adptSetIndex_ = 0;
    streamDesc->representationIndex_ = 0;
    streamDesc->bandwidth_ = 1000000;
    streamDesc->duration_ = 10000;
    streamDesc->startNumberSeq_ = 1;
    DashAdptSetInfo* adptSet = new DashAdptSetInfo();
    DashSegTmpltInfo* segTmplt = new DashSegTmpltInfo();
    segTmplt->segTmpltMedia_ = "segment$Number$.m4s";
    segTmplt->multSegBaseInfo_.duration_ = 1000;
    segTmplt->multSegBaseInfo_.segBaseInfo_.timeScale_ = 1000;
    adptSet->adptSetSegTmplt_ = segTmplt;
    period->adptSetList_.push_back(adptSet);
    auto ret = mpdDownloader_->GetSegmentsInMpd(streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    // Lazy loading: mediaSegments_ is not expanded, check lazy state instead
    EXPECT_TRUE(streamDesc->lazyMode_ != DASH_LAZY_MODE_NONE);
    EXPECT_GT(streamDesc->GetMediaSegmentCount(), 0u);
    delete segTmplt;
    delete adptSet;
    delete period;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test Init with downloadCallback
 * @tc.number: Init_001
 * @tc.desc  : Test Init with downloadCallback
 */
HWTEST_F(DashMpdDownloaderUnitTest, Init_001, TestSize.Level0)
{
    auto sourceLoader = std::make_shared<MediaSourceLoaderCombinations>(nullptr);
    auto mpdDownloader = std::make_shared<DashMpdDownloader>(sourceLoader);
    auto callback = std::make_shared<DownloadMetricsInfo>();
    mpdDownloader->SetDownloadCallback(callback);
    mpdDownloader->Init();
    EXPECT_NE(mpdDownloader->downloadCallback_, nullptr);
}

/**
 * @tc.name  : Test GetNextVideoStream with bandwidth selection
 * @tc.number: GetNextVideoStream_003
 * @tc.desc  : Test GetNextVideoStream bandwidth selection
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetNextVideoStream_003, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashMpdBitrateParam param;
    param.bitrate_ = 1500000;
    int streamId = 0;

    auto ptr1 = std::make_shared<DashStreamDescription>();
    ptr1->streamId_ = 0;
    ptr1->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    ptr1->inUse_ = true;
    ptr1->bandwidth_ = 1000000;

    auto ptr2 = std::make_shared<DashStreamDescription>();
    ptr2->streamId_ = 1;
    ptr2->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    ptr2->inUse_ = false;
    ptr2->bandwidth_ = 2000000;

    mpdDownloader_->streamDescriptions_.push_back(ptr1);
    mpdDownloader_->streamDescriptions_.push_back(ptr2);

    auto ret = mpdDownloader_->GetNextVideoStream(param, streamId);
    EXPECT_EQ(ret, DASH_MPD_GET_ERROR);
    EXPECT_EQ(streamId, 0);

    mpdDownloader_->streamDescriptions_.clear();
}

/**
 * @tc.name  : Test GetNextVideoStream with nextSegTime update
 * @tc.number: GetNextVideoStream_004
 * @tc.desc  : Test GetNextVideoStream nextSegTime update
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetNextVideoStream_004, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashMpdBitrateParam param;
    param.bitrate_ = 2000000;
    param.position_ = -1;
    int streamId = 0;

    auto ptr1 = std::make_shared<DashStreamDescription>();
    ptr1->streamId_ = 0;
    ptr1->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    ptr1->inUse_ = true;
    ptr1->bandwidth_ = 1000000;
    auto seg1 = std::make_shared<DashSegment>();
    seg1->numberSeq_ = 1;
    seg1->duration_ = 1000;
    ptr1->mediaSegments_.push_back(seg1);

    auto ptr2 = std::make_shared<DashStreamDescription>();
    ptr2->streamId_ = 1;
    ptr2->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    ptr2->inUse_ = false;
    ptr2->bandwidth_ = 2000000;
    auto seg2 = std::make_shared<DashSegment>();
    seg2->numberSeq_ = 2;
    seg2->duration_ = 1000;
    ptr2->mediaSegments_.push_back(seg2);

    mpdDownloader_->streamDescriptions_.push_back(ptr1);
    mpdDownloader_->streamDescriptions_.push_back(ptr2);

    auto ret = mpdDownloader_->GetNextVideoStream(param, streamId);
    EXPECT_EQ(ret, DASH_MPD_GET_ERROR);
    EXPECT_EQ(param.nextSegTime_, 0u);

    mpdDownloader_->streamDescriptions_.clear();
}

/**
 * @tcTest  : Test GetNextTrackStream with startNumberSeq update
 * @tc.number: GetNextTrackStream_003
 * @tc.desc  : Test GetNextTrackStream startNumberSeq update
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetNextTrackStream_003, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashMpdTrackParam param;
    param.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    param.position_ = -1;
    param.streamId_ = 1;

    auto ptr1 = std::make_shared<DashStreamDescription>();
    ptr1->streamId_ = 0;
    ptr1->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    ptr1->inUse_ = true;
    ptr1->startNumberSeq_ = 1;
    auto seg1 = std::make_shared<DashSegment>();
    seg1->numberSeq_ = 1;
    seg1->duration_ = 1000;
    ptr1->mediaSegments_.push_back(seg1);

    auto ptr2 = std::make_shared<DashStreamDescription>();
    ptr2->streamId_ = 1;
    ptr2->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    ptr2->inUse_ = false;
    ptr2->startNumberSeq_ = 2;
    auto seg2 = std::make_shared<DashSegment>();
    seg2->numberSeq_ = 2;
    seg2->duration_ = 1000;
    ptr2->mediaSegments_.push_back(seg2);

    mpdDownloader_->streamDescriptions_.push_back(ptr1);
    mpdDownloader_->streamDescriptions_.push_back(ptr2);

    auto ret = mpdDownloader_->GetNextTrackStream(param);
    EXPECT_EQ(ret, DASH_MPD_GET_ERROR);
    EXPECT_EQ(param.nextSegTime_, 0u);

    mpdDownloader_->streamDescriptions_.clear();
}

/**
 * @tc.name  : Test UpdateCurrentNumberSeqByTime with delta
 * @tc.number: UpdateCurrentNumberSeqByTime_003
 * @tc.desc  : Test UpdateCurrentNumberSeqByTime with SEGMENT_DURATION_DELTA
 */
HWTEST_F(DashMpdDownloaderUnitTest, UpdateCurrentNumberSeqByTime_003, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->startNumberSeq_ = 1;
    
    auto seg1 = std::make_shared<DashSegment>();
    seg1->numberSeq_ = 1;
    seg1->duration_ = 100;
    auto seg2 = std::make_shared<DashSegment>();
    seg2->numberSeq_ = 2;
    seg2->duration_ = 100;
    
    streamDesc->mediaSegments_.push_back(seg1);
    streamDesc->mediaSegments_.push_back(seg2);
    streamDesc->currentNumberSeq_ = 0;
    
    mpdDownloader_->UpdateCurrentNumberSeqByTime(streamDesc, 200);
    EXPECT_EQ(streamDesc->currentNumberSeq_, 2);
}

/**
 * @tc.name  : Test ProcessDrmInfos with isReported
 * @tc.number: ProcessDrmInfos_002
 * @tc.desc  : Test ProcessDrmInfos with isReported true
 */
HWTEST_F(DashMpdDownloaderUnitTest, ProcessDrmInfos_002, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* period = new DashPeriodInfo();
    period->id_ = "period1";
    DashAdptSetInfo* adptSet = new DashAdptSetInfo();
    adptSet->id_ = 1;
    DashDescriptor* contentProtection = new DashDescriptor();
    contentProtection->schemeIdUrl_ = "urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
    contentProtection->elementMap_[MPD_LABEL_PSSH] = "AAACAAE=";
    adptSet->commonAttrsAndElements_.contentProtectionList_.push_back(contentProtection);
    period->adptSetList_.push_back(adptSet);
    mpdInfo->periodInfoList_.push_back(period);
    mpdDownloader_->mpdInfo_ = mpdInfo;
    
    DashDrmInfo drmInfo;
    drmInfo.drmId_ = "test";
    drmInfo.uuid_ = "test";
    drmInfo.pssh_ = "test";
    mpdDownloader_->localDrmInfos_.push_back(drmInfo);
    
    mpdDownloader_->ProcessDrmInfos();
    EXPECT_EQ(mpdDownloader_->localDrmInfos_.size(), 2u);
    
    delete contentProtection;
    delete adptSet;
    delete period;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
    mpdDownloader_->localDrmInfos_.clear();
}

/**
 * @tc.name  : Test ProcessDrmInfos with small pssh
 * @tc.number: ProcessDrmInfos_003
 * @tc.desc  : Test ProcessDrmInfos with psshSize < DRM_UUID_OFFSET
 */
HWTEST_F(DashMpdDownloaderUnitTest, ProcessDrmInfos_003, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* period = new DashPeriodInfo();
    period->id_ = "period1";
    DashAdptSetInfo* adptSet = new DashAdptSetInfo();
    adptSet->id_ = 1;
    DashDescriptor* contentProtection = new DashDescriptor();
    contentProtection->schemeIdUrl_ = "urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
    contentProtection->elementMap_[MPD_LABEL_PSSH] = "AAE=";
    adptSet->commonAttrsAndElements_.contentProtectionList_.push_back(contentProtection);
    period->adptSetList_.push_back(adptSet);
    mpdInfo->periodInfoList_.push_back(period);
    mpdDownloader_->mpdInfo_ = mpdInfo;
    
    mpdDownloader_->ProcessDrmInfos();
    EXPECT_EQ(mpdDownloader_->localDrmInfos_.size(), 1u);
    
    delete contentProtection;
    delete adptSet;
    delete period;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test BuildDashSegment with overflow
 * @tc.number: BuildDashSegment_003
 * @tc.desc  : Test BuildSidxEntries with SafeMul(duration, S_2_NS) overflow
 */
HWTEST_F(DashMpdDownloaderUnitTest, BuildDashSegment_003, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::list<std::shared_ptr<SubSegmentIndex>> subSegIndexList;
    auto subSeg = std::make_shared<SubSegmentIndex>();
    // UINT64_MAX * S_2_NS overflows uint64_t → SafeMul returns false → entry is skipped.
    subSeg->duration_ = UINT64_MAX;
    subSeg->timeScale_ = 1;
    subSegIndexList.push_back(subSeg);
    
    mpdDownloader_->currentDownloadStream_ = std::make_shared<DashStreamDescription>();
    mpdDownloader_->currentDownloadStream_->startNumberSeq_ = 1;
    mpdDownloader_->currentDownloadStream_->streamId_ = 1;
    mpdDownloader_->currentDownloadStream_->bandwidth_ = 1000000;
    mpdDownloader_->currentDownloadStream_->indexSegment_ = std::make_shared<DashIndexSegment>();
    mpdDownloader_->currentDownloadStream_->indexSegment_->url_ = "http://test.com/segment.mp4";
    
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    
    mpdDownloader_->BuildSidxEntries(subSegIndexList);
    EXPECT_EQ(mpdDownloader_->currentDownloadStream_->sidxEntries_.size(), 0u);
    
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test BuildDashSegment with dynamic type
 * @tc.number: BuildDashSegment_004
 * @tc.desc  : Test BuildDashSegment with DASH_TYPE_DYNAMIC
 */
HWTEST_F(DashMpdDownloaderUnitTest, BuildDashSegment_004, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::list<std::shared_ptr<SubSegmentIndex>> subSegIndexList;
    auto subSeg = std::make_shared<SubSegmentIndex>();
    subSeg->duration_ = 1000;
    subSeg->timeScale_ = 1000;
    subSegIndexList.push_back(subSeg);
    
    mpdDownloader_->currentDownloadStream_ = std::make_shared<DashStreamDescription>();
    mpdDownloader_->currentDownloadStream_->startNumberSeq_ = 1;
    mpdDownloader_->currentDownloadStream_->streamId_ = 1;
    mpdDownloader_->currentDownloadStream_->bandwidth_ = 1000000;
    mpdDownloader_->currentDownloadStream_->indexSegment_ = std::make_shared<DashIndexSegment>();
    mpdDownloader_->currentDownloadStream_->indexSegment_->url_ = "http://test.com/segment.mp4";
    
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    
    mpdDownloader_->BuildSidxEntries(subSegIndexList);
    const auto& entries = mpdDownloader_->currentDownloadStream_->sidxEntries_;
    ASSERT_EQ(entries.size(), 1u); // exactly one subSeg pushed
    EXPECT_EQ(entries[0].numberSeq, 1); // startNumberSeq_ = 1
    EXPECT_GT(entries[0].durationNs, 0u); // (1000 * 1e9) / 1000 = 1e9 ns
    EXPECT_EQ(entries[0].url, "http://test.com/segment.mp4"); // from indexSegment_->url_

    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test DoOpen with statusCallback
 * @tc.number: DoOpen_004
 * @tc.desc  : Test DoOpen with statusCallback
 */
HWTEST_F(DashMpdDownloaderUnitTest, DoOpen_004, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->statusCallback_ = [](DownloadStatus status,
        std::shared_ptr<Downloader>& downloader, std::shared_ptr<DownloadRequest>& request) {};
    
    std::string url = "testUrl";
    int64_t startRange = 0;
    int64_t endRange = 0;
    mpdDownloader_->DoOpen(url, startRange, endRange);
    EXPECT_NE(mpdDownloader_->downloadRequest_, nullptr);
}

/**
 * @tc.name  : Test DoOpen with range
 * @tc.number: DoOpen_005
 * @tc.desc  : Test DoOpen with range
 */
HWTEST_F(DashMpdDownloaderUnitTest, DoOpen_005, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::string url = "testUrl";
    int64_t startRange = 100;
    int64_t endRange = 1000;
    mpdDownloader_->DoOpen(url, startRange, endRange);
    EXPECT_NE(mpdDownloader_->downloadRequest_, nullptr);
}

/**
 * @tc.name  : Test GetSegmentsWithTmpltDurationStatic with remainder
 * @tc.number: GetSegmentsWithTmpltDurationStatic_005
 * @tc.desc  : Test GetSegmentsWithTmpltDurationStatic with remainder
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsWithTmpltDurationStatic_005, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashSegTmpltInfo* segTmpltInfo = new DashSegTmpltInfo();
    segTmpltInfo->multSegBaseInfo_.duration_ = 1000;
    segTmpltInfo->multSegBaseInfo_.segBaseInfo_.timeScale_ = 1000;
    std::string mediaUrl = "segment$Number$.m4s";
    unsigned int timeScale = 1000;
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->duration_ = 9500;
    streamDesc->startNumberSeq_ = 1;
    auto ret = mpdDownloader_->GetSegmentsWithTmpltDurationStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    // Lazy loading: mediaSegments_ is not expanded, check lazy state instead
    EXPECT_TRUE(streamDesc->lazyMode_ != DASH_LAZY_MODE_NONE);
    EXPECT_GT(streamDesc->GetMediaSegmentCount(), 0u);
    delete segTmpltInfo;
}

/**
 * @tc.name  : Test GetSegmentsWithTmpltTimelineStatic with segCount < 0
 * @tc.number: GetSegmentsWithTmpltTimelineStatic_005
 * @tc.desc  : Test GetSegmentsWithTmpltTimelineStatic with segCount < 0
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsWithTmpltTimelineStatic_005, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashSegTmpltInfo* segTmpltInfo = new DashSegTmpltInfo();
    DashSegTimeline* timeline = new DashSegTimeline();
    timeline->d_ = 1000;
    timeline->r_ = -1;
    segTmpltInfo->multSegBaseInfo_.segTimeline_.push_back(timeline);
    std::string mediaUrl = "segment$Number$.m4s";
    unsigned int timeScale = 1000;
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->duration_ = 10000 * MS_2_NS;
    streamDesc->startNumberSeq_ = 1;
    auto ret = mpdDownloader_->GetSegmentsWithTmpltTimelineStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    // Lazy loading: mediaSegments_ is not expanded, check lazy state instead
    EXPECT_TRUE(streamDesc->lazyMode_ != DASH_LAZY_MODE_NONE);
    EXPECT_GT(streamDesc->timelineEntries_.size(), 0u);
    EXPECT_GT(streamDesc->GetMediaSegmentCount(), 0u);
    delete timeline;
    delete segTmpltInfo;
}

/**
 * @tc.name  : Test GetSegCountFromTimeline with nextStartTime <= startTime
 * @tc.number: GetSegCountFromTimeline_005
 * @tc.desc  : Test GetSegCountFromTimeline with nextStartTime <= startTime
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegCountFromTimeline_005, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashSegTimeline* dashSegTimeline = new DashSegTimeline();
    dashSegTimeline->r_ = -1;
    dashSegTimeline->d_ = 1000;
    dashSegTimeline->t_ = 0;
    DashList<DashSegTimeline*> dashList;
    dashList.push_back(dashSegTimeline);
    DashList<DashSegTimeline*>::iterator it = dashList.begin();
    DashList<DashSegTimeline*>::iterator end = dashList.end();
    uint64_t periodDuration = 10000 * MS_2_NS;
    unsigned int timeScale = 1000;
    uint64_t startTime = 1000;
    auto ret = mpdDownloader_->GetSegCountFromTimeline(it, end, periodDuration, timeScale, startTime);
    EXPECT_EQ(ret, 8);
    delete dashSegTimeline;
}

/**
 * @tc.name  : Test GetSegCountFromTimeline with nextStartTime > startTime
 * @tc.number: GetSegCountFromTimeline_006
 * @tc.desc  : Test GetSegCountFromTimeline with nextStartTime > startTime
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegCountFromTimeline_006, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashSegTimeline* dashSegTimeline = new DashSegTimeline();
    dashSegTimeline->r_ = -1;
    dashSegTimeline->d_ = 1000;
    dashSegTimeline->t_ = 0;
    DashList<DashSegTimeline*> dashList;
    dashList.push_back(dashSegTimeline);
    DashList<DashSegTimeline*>::iterator it = dashList.begin();
    DashList<DashSegTimeline*>::iterator end = dashList.end();
    uint64_t periodDuration = 10000 * MS_2_NS;
    unsigned int timeScale = 1000;
    uint64_t startTime = 0;
    auto ret = mpdDownloader_->GetSegCountFromTimeline(it, end, periodDuration, timeScale, startTime);
    EXPECT_EQ(ret, 9);
    delete dashSegTimeline;
}

/**
 * @tc.name  : Test GetSegmentsWithSegListWith timeline
 * @tc.number: GetSegmentsWithSegList_006
 * @tc.desc  : Test GetSegmentsWithSegList with timeline
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsWithSegList_006, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashSegListInfo* segListInfo = new DashSegListInfo();
    DashSegUrlEntry segUrl;
    segUrl.media_ = "segment1.m4s";
    segListInfo->segmentUrl_.push_back(segUrl);
    segListInfo->multSegBaseInfo_.duration_ = 1000;
    segListInfo->multSegBaseInfo_.segBaseInfo_.timeScale_ = 1000;
    DashSegTimeline* timeline = new DashSegTimeline();
    timeline->d_ = 1000;
    timeline->r_ = 2;
    segListInfo->multSegBaseInfo_.segTimeline_.push_back(timeline);
    std::string baseUrl = "http://test.com/";
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->bandwidth_ = 1000000;
    streamDesc->duration_ = 10000;
    streamDesc->startNumberSeq_ = 1;
    auto ret = mpdDownloader_->GetSegmentsWithSegList(segListInfo, baseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    // Lazy loading: mediaSegments_ is not expanded, check segListEntries_ instead
    EXPECT_GT(streamDesc->segListEntries_.size(), 0u);
    delete timeline;
    delete segListInfo;
}

/**
 * @tc.name  : Test GetSegmentsWithSegList with multiple timeline durations
 * @tc.number: GetSegmentsWithSegList_007
 * @tc.desc  : Test GetSegmentsWithSegList with multiple timeline durations
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsWithSegList_007, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashSegListInfo* segListInfo = new DashSegListInfo();
    DashSegUrlEntry segUrl1;
    segUrl1.media_ = "segment1.m4s";
    DashSegUrlEntry segUrl2;
    segUrl2.media_ = "segment2.m4s";
    segListInfo->segmentUrl_.push_back(segUrl1);
    segListInfo->segmentUrl_.push_back(segUrl2);
    segListInfo->multSegBaseInfo_.duration_ = 1000;
    segListInfo->multSegBaseInfo_.segBaseInfo_.timeScale_ = 1000;
    DashSegTimeline* timeline = new DashSegTimeline();
    timeline->d_ = 1000;
    timeline->r_ = 2;
    segListInfo->multSegBaseInfo_.segTimeline_.push_back(timeline);
    std::string baseUrl = "http://test.com/";
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->bandwidth_ = 1000000;
    streamDesc->duration_ = 10000;
    streamDesc->startNumberSeq_ = 1;
    auto ret = mpdDownloader_->GetSegmentsWithSegList(segListInfo, baseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    // Lazy loading: mediaSegments_ is not expanded, check segListEntries_ instead
    EXPECT_EQ(streamDesc->segListEntries_.size(), 2u);
    delete timeline;
    delete segListInfo;
}

/**
 * @tc.name  : Test GetSegDurationFromTimeline with timeline
 * @tc.number: GetSegDurationFromTimeline_007
 * @tc.desc  : Test GetSegDurationFromTimeline with timeline
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegDurationFromTimeline_007, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    unsigned int periodDuration = 10000;
    unsigned int timeScale = 1000;
    DashMultSegBaseInfo* multSegBaseInfo = new DashMultSegBaseInfo();
    multSegBaseInfo->duration_ = 0;
    DashSegTimeline* timeline = new DashSegTimeline();
    timeline->d_ = 1000;
    timeline->r_ = 4;
    multSegBaseInfo->segTimeline_.push_back(timeline);
    DashList<uint64_t> durationList;
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->GetSegDurationFromTimeline(periodDuration, timeScale, multSegBaseInfo, durationList);
    // d_=1000, r_=4, timeScale=1000 → segDuration = (1000 * 1e9) / 1000 = 1e9 ns, repeat 5 times
    ASSERT_EQ(durationList.size(), 5u); // r_=4 means 5 segments (repeat 0..4)
    constexpr uint64_t expectedSegDurationNs = 1000000000ULL; // (1000 * S_2_NS) / 1000
    for (const auto& dur : durationList) {
        EXPECT_EQ(dur, expectedSegDurationNs); // each segment duration = 1e9 ns = 1s
    }
    delete timeline;
    delete multSegBaseInfo;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetStreamsInfoInPeriod with ondemandSegBase
 * @tc.number: GetStreamsInfoInPeriod_005
 * @tc.desc  : Test GetStreamsInfoInPeriod with ondemandSegBase
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetStreamsInfoInPeriod_005, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashPeriodInfo* period = new DashPeriodInfo();
    period->duration_ = 10000;
    DashSegBaseInfo* periodSegBase = new DashSegBaseInfo();
    periodSegBase->indexRange_ = "0-1000";
    period->periodSegBase_ = periodSegBase;
    std::string mpdBaseUrl = "http://test.com/";
    mpdDownloader_->GetStreamsInfoInPeriod(period, 0, mpdBaseUrl);
    EXPECT_GT(mpdDownloader_->streamDescriptions_.size(), 0u);
    delete periodSegBase;
    delete period;
}

/**
 * @tc.name  : Test GetStreamsInfoInAdptSet with ondemandSegBase
 * @tc.number: GetStreamsInfoInAdptSet_005
 * @tc.desc  : Test GetStreamsInfoInAdptSet with ondemandSegBase
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetStreamsInfoInAdptSet_005, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashAdptSetInfo* adptSetInfo = new DashAdptSetInfo();
    adptSetInfo->id_ = 1;
    adptSetInfo->lang_ = "en";
    DashSegBaseInfo* adptSetSegBase = new DashSegBaseInfo();
    adptSetSegBase->indexRange_ = "0-1000";
    adptSetInfo->adptSetSegBase_ = adptSetSegBase;
    std::string periodBaseUrl = "http://test.com/";
    DashStreamDescription streamDesc;
    streamDesc.duration_ = 10000;
    streamDesc.periodIndex_ = 0;
    streamDesc.adptSetIndex_ = 0;
    streamDesc.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    mpdDownloader_->GetStreamsInfoInAdptSet(adptSetInfo, periodBaseUrl, streamDesc);
    EXPECT_GT(mpdDownloader_->streamDescriptions_.size(), 0u);
    delete adptSetSegBase;
    delete adptSetInfo;
}

/**
 * @tc.name  : Test GetStreamDescriptions with width and height
 * @tc.number: GetStreamDescriptions_005
 * @tc.desc  : Test GetStreamDescriptions with width and height
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetStreamDescriptions_005, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::string periodBaseUrl = "http://test.com/";
    DashStreamDescription streamDesc;
    streamDesc.duration_ = 10000;
    streamDesc.periodIndex_ = 0;
    streamDesc.adptSetIndex_ = 0;
    streamDesc.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc.startNumberSeq_ = 1;
    streamDesc.videoType_ = DASH_VIDEO_TYPE_SDR;
    std::string adptSetBaseUrl = "http://test.com/";
    std::list<DashRepresentationInfo*> repList;
    DashRepresentationInfo* rep = new DashRepresentationInfo();
    rep->id_ = "rep1";
    rep->bandwidth_ = 1000000;
    rep->commonAttrsAndElements_.width_ = 1920;
    rep->commonAttrsAndElements_.height_ = 1080;
    repList.push_back(rep);
    mpdDownloader_->GetStreamDescriptions(periodBaseUrl, streamDesc, adptSetBaseUrl, repList);
    EXPECT_GT(mpdDownloader_->streamDescriptions_.size(), 0u);
    EXPECT_EQ(mpdDownloader_->streamDescriptions_[0]->width_, 0u);
    EXPECT_EQ(mpdDownloader_->streamDescriptions_[0]->height_, 0u);
    delete rep;
}

/**
 * @tc.name  : Test GetStreamDescriptions with HDR VIVID
 * @tc.number: GetStreamDescriptions_006
 * @tc.desc  : Test GetStreamDescriptions with HDR VIVID
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetStreamDescriptions_006, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::string periodBaseUrl = "http://test.com/";
    DashStreamDescription streamDesc;
    streamDesc.duration_ = 10000;
    streamDesc.periodIndex_ = 0;
    streamDesc.adptSetIndex_ = 0;
    streamDesc.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc.startNumberSeq_ = 1;
    streamDesc.videoType_ = DASH_VIDEO_TYPE_HDR_10;
    std::string adptSetBaseUrl = "http://test.com/";
    std::list<DashRepresentationInfo*> repList;
    DashRepresentationInfo* rep = new DashRepresentationInfo();
    rep->id_ = "rep1";
    rep->bandwidth_ = 1000000;
    rep->commonAttrsAndElements_.cuvvVersion_ = "cuvv.test";
    repList.push_back(rep);
    mpdDownloader_->GetStreamDescriptions(periodBaseUrl, streamDesc, adptSetBaseUrl, repList);
    EXPECT_GT(mpdDownloader_->streamDescriptions_.size(), 0u);
    EXPECT_EQ(mpdDownloader_->streamDescriptions_[0]->videoType_, DASH_VIDEO_TYPE_SDR);
    delete rep;
}

/**
 * @tc.name  : Test GetStreamDescriptions with representationSegBase
 * @tc.number: GetStreamDescriptions_007
 * @tc.desc  : Test GetStreamDescriptions with representationSegBase
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetStreamDescriptions_007, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::string periodBaseUrl = "http://test.com/";
    DashStreamDescription streamDesc;
    streamDesc.duration_ = 10000;
    streamDesc.periodIndex_ = 0;
    streamDesc.adptSetIndex_ = 0;
    streamDesc.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc.startNumberSeq_ = 1;
    std::string adptSetBaseUrl = "http://test.com/";
    std::list<DashRepresentationInfo*> repList;
    DashRepresentationInfo* rep = new DashRepresentationInfo();
    rep->id_ = "rep1";
    rep->bandwidth_ = 1000000;
    DashSegBaseInfo* repSegBase = new DashSegBaseInfo();
    repSegBase->indexRange_ = "0-1000";
    rep->representationSegBase_ = repSegBase;
    repList.push_back(rep);
    mpdDownloader_->GetStreamDescriptions(periodBaseUrl, streamDesc, adptSetBaseUrl, repList);
    EXPECT_GT(mpdDownloader_->streamDescriptions_.size(), 0u);
    EXPECT_NE(mpdDownloader_->streamDescriptions_[0]->indexSegment_, nullptr);
    delete repSegBase;
    delete rep;
}

/**
 * @tc.name  : Test UpdateInitSegUrl with template
 * @tc.number: UpdateInitSegUrl_003
 * @tc.desc  : Test UpdateInitSegUrl with template
 */
HWTEST_F(DashMpdDownloaderUnitTest, UpdateInitSegUrl_003, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
    streamDesc->bandwidth_ = 1000000;
    DashUrlType* urlType = new DashUrlType();
    urlType->sourceUrl_ = "$RepresentationID$_$Bandwidth$.mp4";
    int segTmpltFlag = 1;
    std::string representationID = "rep1";
    mpdDownloader_->UpdateInitSegUrl(streamDesc, urlType, segTmpltFlag, representationID);
    EXPECT_EQ(streamDesc->initSegment_->url_, "rep1_1000000.mp4");
    delete urlType;
}

/**
 * @tc.name  : Test GetSegmentsInNewStream with GetSegmentsInMpd failed
 * @tc.number: GetSegmentsInNewStream_006
 * @tc.desc  : Test GetSegmentsInNewStream with GetSegmentsInMpd failed
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsInNewStream_006, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->ondemandSegBase_ = false;
    mpdDownloader_->mpdInfo_ = nullptr;
    auto destStream = std::make_shared<DashStreamDescription>();
    destStream->segsState_ = DASH_SEGS_STATE_NONE;
    destStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    auto ret = mpdDownloader_->GetSegmentsInNewStream(destStream);
    EXPECT_EQ(ret, DASH_MPD_GET_DONE);
}

/**
 * @tc.name  : Test GetSegmentsInNewStream with nextSegTime update
 * @tc.number: GetSegmentsInNewStream_007
 * @tc.desc  : Test GetSegmentsInNewStream with nextSegTime update
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsInNewStream_007, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->ondemandSegBase_ = false;
    auto destStream = std::make_shared<DashStreamDescription>();
    destStream->segsState_ = DASH_SEGS_STATE_FINISH;
    destStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    auto seg1 = std::make_shared<DashSegment>();
    seg1->numberSeq_ = 1;
    seg1->duration_ = 1000;
    destStream->mediaSegments_.push_back(seg1);

    auto srcStream = std::make_shared<DashStreamDescription>();
    srcStream->segsState_ = DASH_SEGS_STATE_FINISH;
    srcStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    auto seg2 = std::make_shared<DashSegment>();
    seg2->numberSeq_ = 2;
    seg2->duration_ = 1000;
    srcStream->mediaSegments_.push_back(seg2);
    srcStream->inUse_ = true;
    mpdDownloader_->streamDescriptions_.push_back(srcStream);

    destStream->inUse_ = true;
    auto ret = mpdDownloader_->GetSegmentsInNewStream(destStream);
    EXPECT_EQ(ret, DASH_MPD_GET_DONE);

    mpdDownloader_->streamDescriptions_.clear();
}

/**
 * @tc.name  : Test ChooseStreamToPlay with nextSegTime update
 * @tc.number: ChooseStreamToPlay_006
 * @tc.desc  : Test ChooseStreamToPlay with nextSegTime update
 */
HWTEST_F(DashMpdDownloaderUnitTest, ChooseStreamToPlay_006, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->ondemandSegBase_ = false;

    auto srcStream = std::make_shared<DashStreamDescription>();
    srcStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    srcStream->inUse_ = true;
    srcStream->bandwidth_ = 1000000;
    auto seg1 = std::make_shared<DashSegment>();
    seg1->numberSeq_ = 1;
    seg1->duration_ = 1000;
    srcStream->mediaSegments_.push_back(seg1);
    mpdDownloader_->streamDescriptions_.push_back(srcStream);

    auto destStream = std::make_shared<DashStreamDescription>();
    destStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    destStream->bandwidth_ = 2000000;
    mpdDownloader_->streamDescriptions_.push_back(destStream);

    auto ret = mpdDownloader_->ChooseStreamToPlay(MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(destStream->inUse_, false);
    EXPECT_EQ(destStream->currentNumberSeq_, srcStream->currentNumberSeq_);

    mpdDownloader_->streamDescriptions_.clear();
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test MakeAbsoluteWithBaseUrl with null segment
 * @tc.number: MakeAbsoluteWithBaseUrl_001
 * @tc.desc  : Test MakeAbsoluteWithBaseUrl with null segment
 */
HWTEST_F(DashMpdDownloaderUnitTest, MakeAbsoluteWithBaseUrl_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::vector<std::shared_ptr<DashSegment>> segmentsVector;
    auto seg = std::make_shared<DashSegment>();
    seg->url_ = "segment.mp4";
    segmentsVector.push_back(seg);
    std::string baseUrl = "http://test.com/";
    DashAppendBaseUrl(baseUrl, segmentsVector[0]->url_);
    EXPECT_EQ(segmentsVector[0]->url_, "segment.mp4");
}

/**
 * @tc.name  : Test MakeAbsoluteWithBaseUrl with absolute URL
 * @tc.number: MakeAbsoluteWithBaseUrl_002
 * @tc.desc  : Test MakeAbsoluteWithBaseUrl with absolute URL
 */
HWTEST_F(DashMpdDownloaderUnitTest, MakeAbsoluteWithBaseUrl_002, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::vector<std::shared_ptr<DashSegment>> segmentsVector;
    auto seg = std::make_shared<DashSegment>();
    seg->url_ = "http://test.com/segment.mp4";
    segmentsVector.push_back(seg);
    std::string baseUrl = "http://test.com/";
    DashAppendBaseUrl(baseUrl, segmentsVector[0]->url_);
    EXPECT_EQ(segmentsVector[0]->url_, "http://test.com/segment.mp4");
}

/**
 * @tc.name  : Test GetSegmentsInMpd with dynamic type
 * @tc.number: GetSegmentsInMpd_008
 * @tc.desc  : Test GetSegmentsInMpd with DASH_TYPE_DYNAMIC
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsInMpd_008, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* period = new DashPeriodInfo();
    mpdInfo->periodInfoList_.push_back(period);
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->url_ = "http://test.com/test.mpd";
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->periodIndex_ = 0;
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc->adptSetIndex_ = 0;
    streamDesc->bandwidth_ = 1000000;
    streamDesc->duration_ = 10000;
    streamDesc->startNumberSeq_ = 1;
    auto seg = std::make_shared<DashSegment>();
    seg->numberSeq_ = 1;
    seg->duration_ = 1000;
    streamDesc->mediaSegments_.push_back(seg);
    auto ret = mpdDownloader_->GetSegmentsInMpd(streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    EXPECT_GT(streamDesc->mediaSegments_.size(), 0u);
    delete period;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetSegmentsInMpd with last segment
 * @tc.number: GetSegmentsInMpd_009
 * @tc.desc  : Test GetSegmentsInMpd with last segment isLast
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsInMpd_009, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* period = new DashPeriodInfo();
    mpdInfo->periodInfoList_.push_back(period);
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->url_ = "http://test.com/test.mpd";
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->periodIndex_ = 0;
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc->adptSetIndex_ = 0;
    streamDesc->bandwidth_ = 1000000;
    streamDesc->duration_ = 10000;
    streamDesc->startNumberSeq_ = 1;
    auto seg = std::make_shared<DashSegment>();
    seg->numberSeq_ = 1;
    seg->duration_ = 1000;
    streamDesc->mediaSegments_.push_back(seg);
    auto ret = mpdDownloader_->GetSegmentsInMpd(streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    EXPECT_EQ(streamDesc->mediaSegments_[0]->isLast_, false);
    delete period;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetSegmentsInPeriod with baseUrl
 * @tc.number: GetSegmentsInPeriod_005
 * @tc.desc  : Test GetSegmentsInPeriod with baseUrl
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsInPeriod_005, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashPeriodInfo* period = new DashPeriodInfo();
    period->baseUrl_.push_back("http://test.com/segment.mp4");
    std::string mpdBaseUrl = "http://test.com/";
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc->adptSetIndex_ = 999;
    streamDesc->bandwidth_ = 1000000;
    streamDesc->duration_ = 10000;
    streamDesc->startNumberSeq_ = 1;
    auto ret = mpdDownloader_->GetSegmentsInPeriod(period, mpdBaseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    EXPECT_GT(streamDesc->mediaSegments_.size(), 0u);
    delete period;
}

/**
 * @tc.name  : Test GetSegmentsInAdptSet with baseUrl
 * @tc.number: GetSegmentsInAdptSet_005
 * @tc.desc  : Test GetSegmentsInAdptSet with baseUrl
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsInAdptSet_005, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashAdptSetInfo* adptSetInfo = new DashAdptSetInfo();
    adptSetInfo->baseUrl_.push_back("http://test.com/segment.mp4");
    std::string periodBaseUrl = "http://test.com/";
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->representationIndex_ = 999;
    streamDesc->bandwidth_ = 1000000;
    streamDesc->duration_ = 10000;
    streamDesc->startNumberSeq_ = 1;
    auto ret = mpdDownloader_->GetSegmentsInAdptSet(adptSetInfo, periodBaseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    EXPECT_GT(streamDesc->mediaSegments_.size(), 0u);
    delete adptSetInfo;
}

/**
 * @tc.name  : Test GetSegmentsInRepresentation with baseUrl
 * @tc.number: GetSegmentsInRepresentation_005
 * @tc.desc  : Test GetSegmentsInRepresentation with baseUrl
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetSegmentsInRepresentation_005, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    DashRepresentationInfo* repInfo = new DashRepresentationInfo();
    repInfo->baseUrl_.push_back("http://test.com/segment.mp4");
    std::string adptSetBaseUrl = "http://test.com/";
    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->bandwidth_ = 1000000;
    streamDesc->duration_ = 10000;
    streamDesc->startNumberSeq_ = 1;
    auto ret = mpdDownloader_->GetSegmentsInRepresentation(repInfo, adptSetBaseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    EXPECT_GT(streamDesc->mediaSegments_.size(), 0u);
    delete repInfo;
}

// ===========================================================================
// Lazy expansion branch coverage tests for vulnerability fix commit 2ca11db0a
// Covers GetMediaSegmentCount/GetMediaSegmentDuration/GetMediaSegmentNumberSeq
// per lazy mode, BuildLazySegment per lazy mode, DashParseRange rejection,
// and UpdateCurrentNumberSeqByTime with lazy modes.
// ===========================================================================

/**
 * @tc.name  : Test GetMediaSegmentCount in TIMELINE mode
 * @tc.number: GetMediaSegmentCount_Timeline_001
 * @tc.desc  : TIMELINE mode counts repeatCount across entries
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentCount_Timeline_001, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_TIMELINE;
    DashTimelineEntry entry1;
    entry1.repeatCount = 3;
    entry1.duration = 1000;
    entry1.segDurationNs = 1000000;
    entry1.startTime = 0;
    entry1.startSeq = 1;
    stream->timelineEntries_.push_back(entry1);
    DashTimelineEntry entry2;
    entry2.repeatCount = 2;
    entry2.duration = 1000;
    entry2.segDurationNs = 1000000;
    entry2.startTime = 3000;
    entry2.startSeq = 4;
    stream->timelineEntries_.push_back(entry2);
    EXPECT_EQ(stream->GetMediaSegmentCount(), 5u);
}

/**
 * @tc.name  : Test GetMediaSegmentCount in TIMELINE mode with zero repeat
 * @tc.number: GetMediaSegmentCount_Timeline_002
 * @tc.desc  : TIMELINE mode skips entries with repeatCount <= 0
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentCount_Timeline_002, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_TIMELINE;
    DashTimelineEntry entry1;
    entry1.repeatCount = 0; // should be skipped
    entry1.duration = 1000;
    entry1.segDurationNs = 1000000;
    stream->timelineEntries_.push_back(entry1);
    DashTimelineEntry entry2;
    entry2.repeatCount = 4;
    entry2.duration = 1000;
    entry2.segDurationNs = 1000000;
    stream->timelineEntries_.push_back(entry2);
    EXPECT_EQ(stream->GetMediaSegmentCount(), 4u);
}

/**
 * @tc.name  : Test GetMediaSegmentCount in DURATION mode
 * @tc.number: GetMediaSegmentCount_Duration_001
 * @tc.desc  : DURATION mode computes count from totalDuration/segDurationNs
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentCount_Duration_001, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_DURATION;
    stream->durationSegInfo_.segDurationNs = 1000000; // 1s
    stream->durationSegInfo_.totalDuration = 9500000; // 9.5s
    // ceil(9.5 / 1) = 10
    EXPECT_EQ(stream->GetMediaSegmentCount(), 10u);
}

/**
 * @tc.name  : Test GetMediaSegmentCount in DURATION mode with zero values
 * @tc.number: GetMediaSegmentCount_Duration_002
 * @tc.desc  : DURATION mode returns 0 when segDurationNs or totalDuration is 0
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentCount_Duration_002, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_DURATION;
    stream->durationSegInfo_.segDurationNs = 0;
    stream->durationSegInfo_.totalDuration = 1000000;
    EXPECT_EQ(stream->GetMediaSegmentCount(), 0u);

    stream->durationSegInfo_.segDurationNs = 1000000;
    stream->durationSegInfo_.totalDuration = 0;
    EXPECT_EQ(stream->GetMediaSegmentCount(), 0u);
}

/**
 * @tc.name  : Test GetMediaSegmentCount in SIDX and SEGLIST modes
 * @tc.number: GetMediaSegmentCount_SidxSegList_001
 * @tc.desc  : SIDX and SEGLIST modes return entry vector sizes
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentCount_SidxSegList_001, TestSize.Level0)
{
    auto streamSidx = std::make_shared<DashStreamDescription>();
    streamSidx->lazyMode_ = DASH_LAZY_MODE_SIDX;
    streamSidx->sidxEntries_.push_back(DashSidxEntry{1000000, 1, 0, 100, "url1"});
    streamSidx->sidxEntries_.push_back(DashSidxEntry{2000000, 2, 101, 200, "url2"});
    EXPECT_EQ(streamSidx->GetMediaSegmentCount(), 2u);

    auto streamList = std::make_shared<DashStreamDescription>();
    streamList->lazyMode_ = DASH_LAZY_MODE_SEGLIST;
    streamList->segListEntries_.push_back(DashSegListEntry{"seg1.m4s", "0-100", 1000000, 1});
    streamList->segListEntries_.push_back(DashSegListEntry{"seg2.m4s", "101-200", 1000000, 2});
    streamList->segListEntries_.push_back(DashSegListEntry{"seg3.m4s", "201-300", 1000000, 3});
    EXPECT_EQ(streamList->GetMediaSegmentCount(), 3u);
}

/**
 * @tc.name  : Test GetMediaSegmentCount in NONE mode
 * @tc.number: GetMediaSegmentCount_None_001
 * @tc.desc  : NONE mode returns mediaSegments_ size
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentCount_None_001, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_NONE;
    auto seg1 = std::make_shared<DashSegment>();
    auto seg2 = std::make_shared<DashSegment>();
    stream->mediaSegments_.push_back(seg1);
    stream->mediaSegments_.push_back(seg2);
    EXPECT_EQ(stream->GetMediaSegmentCount(), 2u);
}

/**
 * @tc.name  : Test GetMediaSegmentDuration in DURATION mode with remainder
 * @tc.number: GetMediaSegmentDuration_Duration_001
 * @tc.desc  : DURATION mode last segment uses remainder when totalDuration % segDurationNs != 0
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentDuration_Duration_001, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_DURATION;
    stream->durationSegInfo_.segDurationNs = 1000000;
    stream->durationSegInfo_.totalDuration = 9500000;
    // 10 segments (0..9), last (index 9) should have remainder 500000
    EXPECT_EQ(stream->GetMediaSegmentDuration(0), 1000000u);
    EXPECT_EQ(stream->GetMediaSegmentDuration(8), 1000000u);
    EXPECT_EQ(stream->GetMediaSegmentDuration(9), 500000u);
}

/**
 * @tc.name  : Test GetMediaSegmentDuration in DURATION mode exact division
 * @tc.number: GetMediaSegmentDuration_Duration_002
 * @tc.desc  : DURATION mode last segment returns segDurationNs when totalDuration is exact multiple
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentDuration_Duration_002, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_DURATION;
    stream->durationSegInfo_.segDurationNs = 1000000;
    stream->durationSegInfo_.totalDuration = 10000000; // exact multiple
    // 10 segments (0..9), last (index 9) should have full segDurationNs since remainder == 0
    EXPECT_EQ(stream->GetMediaSegmentDuration(9), 1000000u);
}

/**
 * @tc.name  : Test GetMediaSegmentDuration out-of-bounds
 * @tc.number: GetMediaSegmentDuration_OOB_001
 * @tc.desc  : Out-of-bounds index returns 0 for each lazy mode
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentDuration_OOB_001, TestSize.Level0)
{
    auto streamDur = std::make_shared<DashStreamDescription>();
    streamDur->lazyMode_ = DASH_LAZY_MODE_DURATION;
    streamDur->durationSegInfo_.segDurationNs = 1000000;
    streamDur->durationSegInfo_.totalDuration = 5000000;
    EXPECT_EQ(streamDur->GetMediaSegmentDuration(100), 0u);

    auto streamSidx = std::make_shared<DashStreamDescription>();
    streamSidx->lazyMode_ = DASH_LAZY_MODE_SIDX;
    streamSidx->sidxEntries_.push_back(DashSidxEntry{1000000, 1, 0, 100, "url1"});
    EXPECT_EQ(streamSidx->GetMediaSegmentDuration(5), 0u);

    auto streamList = std::make_shared<DashStreamDescription>();
    streamList->lazyMode_ = DASH_LAZY_MODE_SEGLIST;
    streamList->segListEntries_.push_back(DashSegListEntry{"seg1.m4s", "0-100", 1000000, 1});
    EXPECT_EQ(streamList->GetMediaSegmentDuration(5), 0u);

    auto streamNone = std::make_shared<DashStreamDescription>();
    streamNone->lazyMode_ = DASH_LAZY_MODE_NONE;
    auto seg = std::make_shared<DashSegment>();
    seg->duration_ = 1000000;
    streamNone->mediaSegments_.push_back(seg);
    EXPECT_EQ(streamNone->GetMediaSegmentDuration(5), 0u);
}

/**
 * @tc.name  : Test GetMediaSegmentDuration in TIMELINE mode
 * @tc.number: GetMediaSegmentDuration_Timeline_001
 * @tc.desc  : TIMELINE mode returns segDurationNs for segments within each entry
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentDuration_Timeline_001, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_TIMELINE;
    DashTimelineEntry entry1;
    entry1.repeatCount = 3;
    entry1.duration = 1000;
    entry1.segDurationNs = 500000;
    entry1.startTime = 0;
    entry1.startSeq = 1;
    stream->timelineEntries_.push_back(entry1);
    DashTimelineEntry entry2;
    entry2.repeatCount = 2;
    entry2.duration = 2000;
    entry2.segDurationNs = 800000;
    entry2.startTime = 3000;
    entry2.startSeq = 4;
    stream->timelineEntries_.push_back(entry2);
    // First 3 segments belong to entry1 (500000us each)
    EXPECT_EQ(stream->GetMediaSegmentDuration(0), 500000u);
    EXPECT_EQ(stream->GetMediaSegmentDuration(2), 500000u);
    // Segments 3-4 belong to entry2 (800000us each)
    EXPECT_EQ(stream->GetMediaSegmentDuration(3), 800000u);
    EXPECT_EQ(stream->GetMediaSegmentDuration(4), 800000u);
    // Out of bounds
    EXPECT_EQ(stream->GetMediaSegmentDuration(5), 0u);
}

/**
 * @tc.name  : Test GetMediaSegmentDuration in SIDX and SEGLIST modes
 * @tc.number: GetMediaSegmentDuration_SidxSegList_001
 * @tc.desc  : SIDX and SEGLIST modes return per-entry durationNs
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentDuration_SidxSegList_001, TestSize.Level0)
{
    auto streamSidx = std::make_shared<DashStreamDescription>();
    streamSidx->lazyMode_ = DASH_LAZY_MODE_SIDX;
    streamSidx->sidxEntries_.push_back(DashSidxEntry{1500000, 1, 0, 100, "url1"});
    streamSidx->sidxEntries_.push_back(DashSidxEntry{2500000, 2, 101, 200, "url2"});
    EXPECT_EQ(streamSidx->GetMediaSegmentDuration(0), 1500000u);
    EXPECT_EQ(streamSidx->GetMediaSegmentDuration(1), 2500000u);

    auto streamList = std::make_shared<DashStreamDescription>();
    streamList->lazyMode_ = DASH_LAZY_MODE_SEGLIST;
    streamList->segListEntries_.push_back(DashSegListEntry{"seg1.m4s", "0-100", 3000000, 1});
    streamList->segListEntries_.push_back(DashSegListEntry{"seg2.m4s", "101-200", 4000000, 2});
    EXPECT_EQ(streamList->GetMediaSegmentDuration(0), 3000000u);
    EXPECT_EQ(streamList->GetMediaSegmentDuration(1), 4000000u);
}

/**
 * @tc.name  : Test GetMediaSegmentNumberSeq in TIMELINE mode
 * @tc.number: GetMediaSegmentNumberSeq_Timeline_001
 * @tc.desc  : TIMELINE mode uses SafeAdd(entry.startSeq, indexWithinEntry)
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentNumberSeq_Timeline_001, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_TIMELINE;
    DashTimelineEntry entry1;
    entry1.repeatCount = 3;
    entry1.duration = 1000;
    entry1.segDurationNs = 1000000;
    entry1.startTime = 0;
    entry1.startSeq = 5;
    stream->timelineEntries_.push_back(entry1);
    // Segment 0 -> seq 5, segment 2 -> seq 7
    EXPECT_EQ(stream->GetMediaSegmentNumberSeq(0), 5);
    EXPECT_EQ(stream->GetMediaSegmentNumberSeq(2), 7);
    // Out of bounds
    EXPECT_EQ(stream->GetMediaSegmentNumberSeq(3), -1);
}

/**
 * @tc.name  : Test GetMediaSegmentNumberSeq in DURATION mode
 * @tc.number: GetMediaSegmentNumberSeq_Duration_001
 * @tc.desc  : DURATION mode uses SafeAdd(startNumberSeq, index)
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentNumberSeq_Duration_001, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_DURATION;
    stream->durationSegInfo_.segDurationNs = 1000000;
    stream->durationSegInfo_.totalDuration = 5000000;
    stream->durationSegInfo_.startNumberSeq = 10;
    EXPECT_EQ(stream->GetMediaSegmentNumberSeq(0), 10);
    EXPECT_EQ(stream->GetMediaSegmentNumberSeq(4), 14);
    // Out of bounds
    EXPECT_EQ(stream->GetMediaSegmentNumberSeq(5), -1);
}

/**
 * @tc.name  : Test GetMediaSegmentNumberSeq in DURATION mode overflow
 * @tc.number: GetMediaSegmentNumberSeq_Duration_002
 * @tc.desc  : DURATION mode SafeAdd returns false on overflow, caller returns -1
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentNumberSeq_Duration_002, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_DURATION;
    stream->durationSegInfo_.segDurationNs = 1000000;
    stream->durationSegInfo_.totalDuration = 2000000;
    stream->durationSegInfo_.startNumberSeq = INT64_MAX;
    EXPECT_EQ(stream->GetMediaSegmentNumberSeq(0), INT64_MAX);
    EXPECT_EQ(stream->GetMediaSegmentNumberSeq(1), -1);
}

/**
 * @tc.name  : Test GetMediaSegmentNumberSeq in SIDX and SEGLIST modes
 * @tc.number: GetMediaSegmentNumberSeq_SidxSegList_001
 * @tc.desc  : SIDX and SEGLIST modes return per-entry numberSeq, -1 for OOB
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentNumberSeq_SidxSegList_001, TestSize.Level0)
{
    auto streamSidx = std::make_shared<DashStreamDescription>();
    streamSidx->lazyMode_ = DASH_LAZY_MODE_SIDX;
    streamSidx->sidxEntries_.push_back(DashSidxEntry{1000000, 7, 0, 100, "url1"});
    streamSidx->sidxEntries_.push_back(DashSidxEntry{2000000, 8, 101, 200, "url2"});
    EXPECT_EQ(streamSidx->GetMediaSegmentNumberSeq(0), 7);
    EXPECT_EQ(streamSidx->GetMediaSegmentNumberSeq(1), 8);
    EXPECT_EQ(streamSidx->GetMediaSegmentNumberSeq(2), -1);

    auto streamList = std::make_shared<DashStreamDescription>();
    streamList->lazyMode_ = DASH_LAZY_MODE_SEGLIST;
    streamList->segListEntries_.push_back(DashSegListEntry{"seg1.m4s", "0-100", 1000000, 3});
    streamList->segListEntries_.push_back(DashSegListEntry{"seg2.m4s", "101-200", 1000000, 4});
    EXPECT_EQ(streamList->GetMediaSegmentNumberSeq(0), 3);
    EXPECT_EQ(streamList->GetMediaSegmentNumberSeq(1), 4);
    EXPECT_EQ(streamList->GetMediaSegmentNumberSeq(2), -1);
}

/**
 * @tc.name  : Test GetMediaSegmentNumberSeq in NONE mode
 * @tc.number: GetMediaSegmentNumberSeq_None_001
 * @tc.desc  : NONE mode returns numberSeq_ from mediaSegments_, -1 for OOB
 */
HWTEST_F(DashMpdDownloaderUnitTest, GetMediaSegmentNumberSeq_None_001, TestSize.Level0)
{
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_NONE;
    auto seg1 = std::make_shared<DashSegment>();
    seg1->numberSeq_ = 15;
    stream->mediaSegments_.push_back(seg1);
    EXPECT_EQ(stream->GetMediaSegmentNumberSeq(0), 15);
    EXPECT_EQ(stream->GetMediaSegmentNumberSeq(1), -1);
}

/**
 * @tc.name  : Test BuildLazySegment in TIMELINE mode
 * @tc.number: BuildLazySegment_Timeline_001
 * @tc.desc  : BuildLazySegment builds DashSegment with correct URL and numberSeq for TIMELINE
 */
HWTEST_F(DashMpdDownloaderUnitTest, BuildLazySegment_Timeline_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_TIMELINE;
    stream->streamId_ = 5;
    stream->bandwidth_ = 500000;
    stream->startNumberSeq_ = 1;
    DashTimelineEntry entry;
    entry.repeatCount = 2;
    entry.duration = 1000;
    entry.segDurationNs = 1000000;
    entry.startTime = 0;
    entry.startSeq = 1;
    stream->timelineEntries_.push_back(entry);
    // Need durationSegInfo_.mediaUrlTemplate for URL building
    stream->durationSegInfo_.mediaUrlTemplate = "segment$Number$_$Time$.m4s";

    auto seg = mpdDownloader_->BuildLazySegment(stream, 0);
    ASSERT_NE(seg, nullptr);
    EXPECT_EQ(seg->streamId_, 5);
    EXPECT_EQ(seg->bandwidth_, 500000u);
    EXPECT_EQ(seg->duration_, 1000000u);
    EXPECT_EQ(seg->numberSeq_, 1);
    EXPECT_EQ(seg->startNumberSeq_, 1);
    EXPECT_FALSE(seg->isLast_); // index 0 of 2, last is index 1
    EXPECT_FALSE(seg->url_.find("$Number") != std::string::npos);
    EXPECT_FALSE(seg->url_.find("$Time") != std::string::npos);

    auto seg2 = mpdDownloader_->BuildLazySegment(stream, 1);
    ASSERT_NE(seg2, nullptr);
    EXPECT_EQ(seg2->numberSeq_, 2);
    EXPECT_TRUE(seg2->isLast_);

    // Out of bounds
    auto segOob = mpdDownloader_->BuildLazySegment(stream, 2);
    EXPECT_EQ(segOob, nullptr);
}

/**
 * @tc.name  : Test BuildLazySegment in DURATION mode
 * @tc.number: BuildLazySegment_Duration_001
 * @tc.desc  : BuildLazySegment builds DashSegment with correct URL and numberSeq for DURATION
 */
HWTEST_F(DashMpdDownloaderUnitTest, BuildLazySegment_Duration_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_DURATION;
    stream->streamId_ = 3;
    stream->bandwidth_ = 300000;
    stream->startNumberSeq_ = 1;
    stream->durationSegInfo_.mediaUrlTemplate = "seg$Number$_$Time$.m4s";
    stream->durationSegInfo_.segDurationNs = 1000000;
    stream->durationSegInfo_.totalDuration = 3000000;
    stream->durationSegInfo_.baseDuration = 1000;
    stream->durationSegInfo_.startNumberSeq = 1;

    auto seg = mpdDownloader_->BuildLazySegment(stream, 0);
    ASSERT_NE(seg, nullptr);
    EXPECT_EQ(seg->streamId_, 3);
    EXPECT_EQ(seg->bandwidth_, 300000u);
    EXPECT_EQ(seg->numberSeq_, 1);
    EXPECT_EQ(seg->duration_, 1000000u);
    EXPECT_EQ(seg->startNumberSeq_, 1);
    EXPECT_FALSE(seg->isLast_);
    EXPECT_FALSE(seg->url_.find("$Number") != std::string::npos);
    EXPECT_FALSE(seg->url_.find("$Time") != std::string::npos);

    auto segLast = mpdDownloader_->BuildLazySegment(stream, 2);
    ASSERT_NE(segLast, nullptr);
    EXPECT_EQ(segLast->numberSeq_, 3);
    EXPECT_TRUE(segLast->isLast_);
    EXPECT_FALSE(segLast->url_.find("$Number") != std::string::npos);
    EXPECT_FALSE(segLast->url_.find("$Time") != std::string::npos);

    // Out of bounds
    auto segOob = mpdDownloader_->BuildLazySegment(stream, 3);
    EXPECT_EQ(segOob, nullptr);
}

/**
 * @tc.name  : Test BuildLazySegment in SIDX mode
 * @tc.number: BuildLazySegment_Sidx_001
 * @tc.desc  : BuildLazySegment builds DashSegment with correct URL, range, numberSeq for SIDX
 */
HWTEST_F(DashMpdDownloaderUnitTest, BuildLazySegment_Sidx_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_SIDX;
    stream->streamId_ = 7;
    stream->bandwidth_ = 700000;
    stream->startNumberSeq_ = 1;
    stream->sidxEntries_.push_back(DashSidxEntry{1000000, 10, 0, 500, "http://test.com/seg1.m4s"});
    stream->sidxEntries_.push_back(DashSidxEntry{2000000, 11, 501, 1000, "http://test.com/seg2.m4s"});

    auto seg = mpdDownloader_->BuildLazySegment(stream, 0);
    ASSERT_NE(seg, nullptr);
    EXPECT_EQ(seg->streamId_, 7);
    EXPECT_EQ(seg->bandwidth_, 700000u);
    EXPECT_EQ(seg->duration_, 1000000u);
    EXPECT_EQ(seg->numberSeq_, 10);
    EXPECT_EQ(seg->startNumberSeq_, 1);
    EXPECT_EQ(seg->startRangeValue_, 0);
    EXPECT_EQ(seg->endRangeValue_, 500);
    EXPECT_EQ(seg->url_, "http://test.com/seg1.m4s");
    EXPECT_FALSE(seg->isLast_);

    auto segLast = mpdDownloader_->BuildLazySegment(stream, 1);
    ASSERT_NE(segLast, nullptr);
    EXPECT_EQ(segLast->numberSeq_, 11);
    EXPECT_TRUE(segLast->isLast_);

    // Out of bounds
    auto segOob = mpdDownloader_->BuildLazySegment(stream, 2);
    EXPECT_EQ(segOob, nullptr);
}

/**
 * @tc.name  : Test BuildLazySegment in SEGLIST mode
 * @tc.number: BuildLazySegment_SegList_001
 * @tc.desc  : BuildLazySegment builds DashSegment with correct URL, range, numberSeq for SEGLIST
 */
HWTEST_F(DashMpdDownloaderUnitTest, BuildLazySegment_SegList_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_SEGLIST;
    stream->streamId_ = 9;
    stream->bandwidth_ = 900000;
    stream->startNumberSeq_ = 1;
    stream->segListEntries_.push_back(DashSegListEntry{"http://test.com/s1.m4s", "0-200", 1000000, 20});
    stream->segListEntries_.push_back(DashSegListEntry{"http://test.com/s2.m4s", "201-400", 2000000, 21});

    auto seg = mpdDownloader_->BuildLazySegment(stream, 0);
    ASSERT_NE(seg, nullptr);
    EXPECT_EQ(seg->streamId_, 9);
    EXPECT_EQ(seg->bandwidth_, 900000u);
    EXPECT_EQ(seg->duration_, 1000000u);
    EXPECT_EQ(seg->numberSeq_, 20);
    EXPECT_EQ(seg->startNumberSeq_, 1);
    EXPECT_EQ(seg->url_, "http://test.com/s1.m4s");
    EXPECT_EQ(seg->byteRange_, "0-200");
    EXPECT_FALSE(seg->isLast_);

    auto segLast = mpdDownloader_->BuildLazySegment(stream, 1);
    ASSERT_NE(segLast, nullptr);
    EXPECT_EQ(segLast->numberSeq_, 21);
    EXPECT_EQ(segLast->duration_, 2000000u);
    EXPECT_TRUE(segLast->isLast_);

    // Out of bounds
    auto segOob = mpdDownloader_->BuildLazySegment(stream, 2);
    EXPECT_EQ(segOob, nullptr);
}

/**
 * @tc.name  : Test BuildLazySegment with null stream and NONE mode
 * @tc.number: BuildLazySegment_NullAndNone_001
 * @tc.desc  : BuildLazySegment returns nullptr for null stream, falls back for NONE mode
 */
HWTEST_F(DashMpdDownloaderUnitTest, BuildLazySegment_NullAndNone_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    // Null stream
    std::shared_ptr<DashStreamDescription> nullStream = nullptr;
    auto seg = mpdDownloader_->BuildLazySegment(nullStream, 0);
    EXPECT_EQ(seg, nullptr);

    // NONE mode with valid index
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_NONE;
    auto seg1 = std::make_shared<DashSegment>();
    seg1->numberSeq_ = 42;
    stream->mediaSegments_.push_back(seg1);
    auto builtSeg = mpdDownloader_->BuildLazySegment(stream, 0);
    ASSERT_NE(builtSeg, nullptr);
    EXPECT_EQ(builtSeg->numberSeq_, 42);

    // NONE mode out of bounds
    auto segOob = mpdDownloader_->BuildLazySegment(stream, 5);
    EXPECT_EQ(segOob, nullptr);
}

/**
 * @tc.name  : Test DashParseRange with end < start rejection
 * @tc.number: DashParseRange_EndLessThanStart_001
 * @tc.desc  : DashParseRange rejects ranges where end < start, leaving outputs unchanged
 */
HWTEST_F(DashMpdDownloaderUnitTest, DashParseRange_EndLessThanStart_001, TestSize.Level0)
{
    int64_t startRange = 999;
    int64_t endRange = 888;
    // end < start should be rejected, outputs remain unchanged
    DashParseRange("500-100", startRange, endRange);
    EXPECT_EQ(startRange, 999);
    EXPECT_EQ(endRange, 888);
}

/**
 * @tc.name  : Test DashParseRange with valid range
 * @tc.number: DashParseRange_Valid_001
 * @tc.desc  : DashParseRange parses valid range string correctly
 */
HWTEST_F(DashMpdDownloaderUnitTest, DashParseRange_Valid_001, TestSize.Level0)
{
    int64_t startRange = -1;
    int64_t endRange = -1;
    DashParseRange("100-500", startRange, endRange);
    EXPECT_EQ(startRange, 100);
    EXPECT_EQ(endRange, 500);
}

/**
 * @tc.name  : Test DashParseRange with no dash
 * @tc.number: DashParseRange_NoDash_001
 * @tc.desc  : DashParseRange rejects range string without dash separator
 */
HWTEST_F(DashMpdDownloaderUnitTest, DashParseRange_NoDash_001, TestSize.Level0)
{
    int64_t startRange = 111;
    int64_t endRange = 222;
    DashParseRange("100500", startRange, endRange);
    EXPECT_EQ(startRange, 111);
    EXPECT_EQ(endRange, 222);
}

/**
 * @tc.name  : Test DashParseRange with dash at beginning
 * @tc.number: DashParseRange_DashAtBegin_001
 * @tc.desc  : DashParseRange rejects range string starting with dash
 */
HWTEST_F(DashMpdDownloaderUnitTest, DashParseRange_DashAtBegin_001, TestSize.Level0)
{
    int64_t startRange = 111;
    int64_t endRange = 222;
    DashParseRange("-500", startRange, endRange);
    EXPECT_EQ(startRange, 111);
    EXPECT_EQ(endRange, 222);
}

/**
 * @tc.name  : Test DashParseRange with negative start
 * @tc.number: DashParseRange_NegativeStart_001
 * @tc.desc  : DashParseRange rejects range starting with dash (negative start)
 */
HWTEST_F(DashMpdDownloaderUnitTest, DashParseRange_NegativeStart_001, TestSize.Level0)
{
    int64_t startRange = 111;
    int64_t endRange = 222;
    DashParseRange("-100-500", startRange, endRange);
    EXPECT_EQ(startRange, 111);
    EXPECT_EQ(endRange, 222);
}

/**
 * @tc.name  : Test DashParseRange with empty string
 * @tc.number: DashParseRange_Empty_001
 * @tc.desc  : DashParseRange does nothing on empty string
 */
HWTEST_F(DashMpdDownloaderUnitTest, DashParseRange_Empty_001, TestSize.Level0)
{
    int64_t startRange = 111;
    int64_t endRange = 222;
    DashParseRange("", startRange, endRange);
    EXPECT_EQ(startRange, 111);
    EXPECT_EQ(endRange, 222);
}

/**
 * @tc.name  : Test DashParseRange with end equal to start
 * @tc.number: DashParseRange_EqualStartEnd_001
 * @tc.desc  : DashParseRange accepts range where end == start
 */
HWTEST_F(DashMpdDownloaderUnitTest, DashParseRange_EqualStartEnd_001, TestSize.Level0)
{
    int64_t startRange = -1;
    int64_t endRange = -1;
    DashParseRange("100-100", startRange, endRange);
    EXPECT_EQ(startRange, 100);
    EXPECT_EQ(endRange, 100);
}

/**
 * @tc.name  : Test UpdateCurrentNumberSeqByTime with DURATION lazy mode
 * @tc.number: UpdateCurrentNumberSeqByTime_Duration_001
 * @tc.desc  : UpdateCurrentNumberSeqByTime correctly seeks in DURATION lazy mode
 */
HWTEST_F(DashMpdDownloaderUnitTest, UpdateCurrentNumberSeqByTime_Duration_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_DURATION;
    stream->startNumberSeq_ = 1;
    stream->currentNumberSeq_ = -1;
    stream->durationSegInfo_.segDurationNs = 1000000000; // 1s per segment in ns
    stream->durationSegInfo_.totalDuration = 5000000000; // 5 segments total in ns
    stream->durationSegInfo_.startNumberSeq = 1;

    // nextSegTime = 2.5s: after seg0(1s)+seg1(2s) prev=2s<=2.5s,
    // seg2 makes prev=3s>2.5s so break with numberSeq=2
    mpdDownloader_->UpdateCurrentNumberSeqByTime(stream, 2500000000);
    EXPECT_EQ(stream->currentNumberSeq_, 2);
}

/**
 * @tc.name  : Test UpdateCurrentNumberSeqByTime with TIMELINE lazy mode
 * @tc.number: UpdateCurrentNumberSeqByTime_Timeline_001
 * @tc.desc  : UpdateCurrentNumberSeqByTime correctly seeks in TIMELINE lazy mode
 */
HWTEST_F(DashMpdDownloaderUnitTest, UpdateCurrentNumberSeqByTime_Timeline_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_TIMELINE;
    stream->startNumberSeq_ = 1;
    stream->currentNumberSeq_ = -1;
    DashTimelineEntry entry;
    entry.repeatCount = 5;
    entry.duration = 1000;
    entry.segDurationNs = 1000000000; // 1s in ns
    entry.startTime = 0;
    entry.startSeq = 1;
    stream->timelineEntries_.push_back(entry);

    // nextSegTime = 3.5s: after seg0..2 prev=3s<=3.5s,
    // seg3 makes prev=4s>3.5s so break with numberSeq=3
    mpdDownloader_->UpdateCurrentNumberSeqByTime(stream, 3500000000);
    EXPECT_EQ(stream->currentNumberSeq_, 3);
}

/**
 * @tc.name  : Test UpdateCurrentNumberSeqByTime with SEGLIST lazy mode
 * @tc.number: UpdateCurrentNumberSeqByTime_SegList_001
 * @tc.desc  : UpdateCurrentNumberSeqByTime correctly seeks in SEGLIST lazy mode
 */
HWTEST_F(DashMpdDownloaderUnitTest, UpdateCurrentNumberSeqByTime_SegList_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_SEGLIST;
    stream->startNumberSeq_ = 1;
    stream->currentNumberSeq_ = -1;
    stream->segListEntries_.push_back(DashSegListEntry{"s1.m4s", "0-100", 1000000000, 1});
    stream->segListEntries_.push_back(DashSegListEntry{"s2.m4s", "101-200", 1000000000, 2});
    stream->segListEntries_.push_back(DashSegListEntry{"s3.m4s", "201-300", 1000000000, 3});

    // nextSegTime = 2.5s: after seg0+seg1 prev=2s<=2.5s,
    // seg2 makes prev=3s>2.5s so break with numberSeq=2
    mpdDownloader_->UpdateCurrentNumberSeqByTime(stream, 2500000000);
    EXPECT_EQ(stream->currentNumberSeq_, 2);
}

/**
 * @tc.name  : Test UpdateCurrentNumberSeqByTime with SIDX lazy mode
 * @tc.number: UpdateCurrentNumberSeqByTime_Sidx_001
 * @tc.desc  : UpdateCurrentNumberSeqByTime correctly seeks in SIDX lazy mode
 */
HWTEST_F(DashMpdDownloaderUnitTest, UpdateCurrentNumberSeqByTime_Sidx_001, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    auto stream = std::make_shared<DashStreamDescription>();
    stream->lazyMode_ = DASH_LAZY_MODE_SIDX;
    stream->startNumberSeq_ = 1;
    stream->currentNumberSeq_ = -1;
    stream->sidxEntries_.push_back(DashSidxEntry{1000000000, 1, 0, 100, "url1"});
    stream->sidxEntries_.push_back(DashSidxEntry{1000000000, 2, 101, 200, "url2"});
    stream->sidxEntries_.push_back(DashSidxEntry{1000000000, 3, 201, 300, "url3"});

    // nextSegTime = 2.5s: after seg0+seg1 prev=2s<=2.5s,
    // seg2 makes prev=3s>2.5s so break with numberSeq=2
    mpdDownloader_->UpdateCurrentNumberSeqByTime(stream, 2500000000);
    EXPECT_EQ(stream->currentNumberSeq_, 2);
}
} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS