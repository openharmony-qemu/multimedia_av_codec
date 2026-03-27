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
#include "utils/time_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr int32_t SERVERPORT = 47777;
static const std::string MPD_SEGMENT_BASE = "http://127.0.0.1:47777/test_dash/segment_base/index.mpd";
static const std::string MPD_SEGMENT_LIST = "http://127.0.0.1:47777/test_dash/segment_list/index.mpd";
static const std::string MPD_SEGMENT_TEMPLATE = "http://127.0.0.1:47777/test_dash/segment_template/index.mpd";
static const std::string MPD_SEGMENT_LIST_TIMELINE = "http://127.0.0.1:47777/test_dash/segment_list/index_timeline.mpd";
static const std::string MPD_SEGMENT_TEMPLATE_ADPT = "http://127.0.0.1:47777/test_dash/segment_template/index_adpt.mpd";
static const std::string URL_TEMPLATE_TIMELINE = "http://127.0.0.1:47777/test_dash/segment_template/index_timeline.mpd";
static const std::string URL_SEGMENT_BASE_IN_PERIOD = "http://127.0.0.1:47777/test_dash/segment_base/index_period.mpd";
constexpr unsigned int INIT_WIDTH = 1280;
constexpr unsigned int INIT_HEIGHT = 720;
}

using namespace testing::ext;

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;

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
        downloader->GetSeekable();
    }
}

void DashMpdDownloaderUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer(SERVERPORT);
    std::cout << "start" << std::endl;
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
        g_server->StartServer(SERVERPORT);
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
    int64_t breakPoint = 30000;
    DashMpdGetRet getRet = mpdDownloader_->GetBreakPointSegment(usingStreamId, breakPoint, seg);

    std::shared_ptr<DashSegment> segError = nullptr;
    int64_t breakPointError = 300000;
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
    mpdDownloader_->SeekToTs(usingStreamId, seekTime / MS_2_NS, seg);

    EXPECT_NE(seg, nullptr);
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
    EXPECT_NE(stream, nullptr);
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
    EXPECT_GT(streamDesc->mediaSegments_.size(), 0u);
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
    
    mpdDownloader_->UpdateCurrentNumberSeqByTime(streamDesc, 150);
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
 * @tc.desc  : Test BuildDashSegment with segDurMsSum overflow
 */
HWTEST_F(DashMpdDownloaderUnitTest, BuildDashSegment_003, TestSize.Level0)
{
    EXPECT_NE(mpdDownloader_, nullptr);
    std::list<std::shared_ptr<SubSegmentIndex>> subSegIndexList;
    auto subSeg = std::make_shared<SubSegmentIndex>();
    subSeg->duration_ = UINT_MAX;
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
    
    mpdDownloader_->BuildDashSegment(subSegIndexList);
    EXPECT_EQ(mpdDownloader_->currentDownloadStream_->mediaSegments_.size(), 0u);
    
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
    
    mpdDownloader_->BuildDashSegment(subSegIndexList);
    EXPECT_GT(mpdDownloader_->currentDownloadStream_->mediaSegments_.size(), 0u);
    
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
    EXPECT_GT(streamDesc->mediaSegments_.size(), 0u);
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
    streamDesc->duration_ = 10000;
    streamDesc->startNumberSeq_ = 1;
    auto ret = mpdDownloader_->GetSegmentsWithTmpltTimelineStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    EXPECT_GT(streamDesc->mediaSegments_.size(), 0u);
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
    unsigned int periodDuration = 10000;
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
    unsigned int periodDuration = 10000;
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
    DashSegUrl* segUrl = new DashSegUrl();
    segUrl->media_ = "segment1.m4s";
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
    EXPECT_GT(streamDesc->mediaSegments_.size(), 0u);
    delete segUrl;
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
    DashSegUrl* segUrl1 = new DashSegUrl();
    segUrl1->media_ = "segment1.m4s";
    DashSegUrl* segUrl2 = new DashSegUrl();
    segUrl2->media_ = "segment2.m4s";
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
    EXPECT_EQ(streamDesc->mediaSegments_.size(), 2u);
    delete segUrl1;
    delete segUrl2;
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
    DashList<unsigned int> durationList;
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->GetSegDurationFromTimeline(periodDuration, timeScale, multSegBaseInfo, durationList);
    EXPECT_GT(durationList.size(), 0u);
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
}
}
}
}