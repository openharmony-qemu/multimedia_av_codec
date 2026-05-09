/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <algorithm>
#include "dash_mpd_downloader_unittest.h"
#include "sidx_box_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing;
using namespace testing::ext;
const static int32_t ID_TEST = 1;
const static int32_t INVALID_TEST = -1;
const static int32_t NUM_LARGE = 900;
const static int32_t NUM_TEST = 0;
const static int32_t NUM7_TEST = 7;
const static int32_t NUM10_TEST = 10;
const static int32_t NUM2_TEST = 2;
const static uint32_t FPS_24 = 24;
const static uint32_t FPS_30 = 30;
const static int32_t NUM1000000_TEST = 1000000;
const static int32_t NUM100_TEST = 100;
const static int32_t NUM1000_TEST = 1000;
const static int32_t NUM200_TEST = 200;
const static int32_t NUM500_TEST = 500;
const static int32_t NUM300_TEST = 300;
const static int32_t NUM400_TEST = 400;
const static int32_t INVALID50_TEST = -50;
void DashMpdDownloaderUnittest::SetUpTestCase(void) {}
void DashMpdDownloaderUnittest::TearDownTestCase(void) {}
void DashMpdDownloaderUnittest::SetUp(void)
{
    mpdDownloader_ = std::make_shared<DashMpdDownloader>();
    mpdDownloader_->Init();
}
void DashMpdDownloaderUnittest::TearDown(void)
{
    mpdDownloader_ = nullptr;
}

/**
 * @tc.name  : Test DashMpdDownloader
 * @tc.number: DashMpdDownloader_001
 * @tc.desc  : Test sourceLoader != nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, DashMpdDownloader_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader =
        std::make_shared<MediaSourceLoaderCombinations>(nullptr);
    auto mpdDownloader = std::make_shared<DashMpdDownloader>(sourceLoader);
    mpdDownloader->Init();
    EXPECT_NE(mpdDownloader->downloader_, nullptr);
    EXPECT_NE(mpdDownloader->downloader_->sourceLoader_, nullptr);
    mpdDownloader = nullptr;
}

/**
 * @tc.name  : Test GetContentType
 * @tc.number: GetContentType_001
 * @tc.desc  : Test downloader_ != nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetContentType_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    auto ret = mpdDownloader_->GetContentType();
    mpdDownloader_->downloader_->isContentTypeUpdated_ = true;
    EXPECT_EQ(ret, "");
}

/**
 * @tc.name  : Test Close
 * @tc.number: Close_001
 * @tc.desc  : Test downloadRequest_ != nullptr && !downloadRequest_->IsClosed()
 */
HWTEST_F(DashMpdDownloaderUnittest, Close_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::string url = "test";
    DataSaveFunc saveData;
    StatusCallbackFunc statusCallback;
    bool requestWholeFile = false;
    mpdDownloader_->downloadRequest_ =
        std::make_shared<DownloadRequest>(url, saveData, statusCallback, requestWholeFile);
    mpdDownloader_->Close(true);
    EXPECT_NE(mpdDownloader_->downloadRequest_, nullptr);
    EXPECT_EQ(mpdDownloader_->downloadRequest_->IsClosed(), true);
}

/**
 * @tc.name  : Test GetNextSegmentByStreamId
 * @tc.number: GetNextSegmentByStreamId_001
 * @tc.desc  : Test segsState_ != DASH_SEGS_STATE_FINISH
 */
HWTEST_F(DashMpdDownloaderUnittest, GetNextSegmentByStreamId_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    int streamId = ID_TEST;
    std::shared_ptr<DashSegment> seg = nullptr;
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mpdDownloader_->streamDescriptions_.push_back(ptr);
    auto ret = mpdDownloader_->GetNextSegmentByStreamId(streamId, seg);
    EXPECT_EQ(ret, DashMpdGetRet::DASH_MPD_GET_UNDONE);
}

/**
 * @tc.name  : Test GetBreakPointSegment
 * @tc.number: GetBreakPointSegment_001
 * @tc.desc  : Test segsState_ != DASH_SEGS_STATE_FINISH
 */
HWTEST_F(DashMpdDownloaderUnittest, GetBreakPointSegment_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    int streamId = ID_TEST;
    int64_t breakpoint = ID_TEST;
    std::shared_ptr<DashSegment> seg = nullptr;
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mpdDownloader_->streamDescriptions_.push_back(ptr);
    auto ret = mpdDownloader_->GetBreakPointSegment(streamId, breakpoint, seg);
    EXPECT_EQ(ret, DashMpdGetRet::DASH_MPD_GET_UNDONE);
}

/**
 * @tc.name  : Test GetBreakPointSegment
 * @tc.number: GetBreakPointSegment_001
 * @tc.desc  : Test segmentDuration + (int64_t)(streamDescription->mediaSegments_[index]->duration_) > breakpoint
 *             Test seg != nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetBreakPointSegment_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    int streamId = ID_TEST;
    int64_t breakpoint = NUM_TEST;
    std::shared_ptr<DashSegment> seg = nullptr;
    auto ptr = std::make_shared<DashStreamDescription>();
    auto mediaSegments = std::make_shared<DashSegment>();
    mediaSegments->duration_ = ID_TEST;
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_FINISH;
    ptr->streamId_ = streamId;
    ptr->mediaSegments_.push_back(mediaSegments);
    mpdDownloader_->streamDescriptions_.push_back(ptr);
    auto ret = mpdDownloader_->GetBreakPointSegment(streamId, breakpoint, seg);
    EXPECT_NE(seg, nullptr);
    EXPECT_EQ(ret, DashMpdGetRet::DASH_MPD_GET_DONE);
    EXPECT_EQ(ptr->currentNumberSeq_, ID_TEST);
}

/**
 * @tc.name  : Test GetNextVideoStream
 * @tc.number: GetNextVideoStream_001
 * @tc.desc  : Test destStream == nullptr || currentStream == nullptr
 *             Test param.position_ != -1
 *             Test ret == DASH_MPD_GET_DONE && param.nextSegTime_ > 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetNextVideoStream_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdBitrateParam param;
    int streamId = NUM_TEST;
    auto ret = mpdDownloader_->GetNextVideoStream(param, streamId);
    EXPECT_EQ(ret, DashMpdGetRet::DASH_MPD_GET_ERROR);

    param.position_ = NUM_TEST;
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->streamId_ = streamId;
    ptr->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    ptr->inUse_ = true;
    mpdDownloader_->streamDescriptions_.push_back(ptr);
    ret = mpdDownloader_->GetNextVideoStream(param, streamId);
    EXPECT_EQ(ret, DashMpdGetRet::DASH_MPD_GET_DONE);
    EXPECT_EQ(param.nextSegTime_, 0);
}

/**
 * @tc.name  : Test GetNextTrackStream
 * @tc.number: GetNextTrackStream_001
 * @tc.desc  : Test destStream == nullptr || currentStream == nullptr
 *             Test param.position_ == -1
 *             Test ret == DASH_MPD_GET_DONE && param.nextSegTime_ > 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetNextTrackStream_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdTrackParam param;
    int streamId = NUM_TEST;
    auto ret = mpdDownloader_->GetNextTrackStream(param);
    EXPECT_EQ(ret, DashMpdGetRet::DASH_MPD_GET_ERROR);

    param.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    param.position_ = INVALID_TEST;
    param.isEnd_ = false;
    param.streamId_ = streamId;
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->currentNumberSeq_ = streamId;
    ptr->startNumberSeq_ = ID_TEST;
    ptr->streamId_ = streamId;
    ptr->inUse_ = true;
    ptr->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    mpdDownloader_->streamDescriptions_.push_back(ptr);
    ret = mpdDownloader_->GetNextTrackStream(param);
    EXPECT_EQ(ret, DashMpdGetRet::DASH_MPD_GET_DONE);
    EXPECT_EQ(param.nextSegTime_, 0);
    EXPECT_EQ(ptr->currentNumberSeq_, INVALID_TEST);
}

/**
 * @tc.name  : Test GetStreamByStreamId
 * @tc.number: GetStreamByStreamId_001
 * @tc.desc  : Test iter == streamDescriptions_.end()
 */
HWTEST_F(DashMpdDownloaderUnittest, GetStreamByStreamId_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    int streamId = NUM_TEST;
    auto ret = mpdDownloader_->GetStreamByStreamId(streamId);
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name  : Test GetUsingStreamByType
 * @tc.number: GetUsingStreamByType_001
 * @tc.desc  : Test iter == streamDescriptions_.end()
 */
HWTEST_F(DashMpdDownloaderUnittest, GetUsingStreamByType_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    MediaAVCodec::MediaType type = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    auto ret = mpdDownloader_->GetUsingStreamByType(type);
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name  : Test UpdateCurrentNumberSeqByTime & GetInitSegmentByStreamId
 * @tc.number: GetInitSegmentByStreamId_001
 * @tc.desc  : Test GetInitSegmentByStreamId: iter == streamDescriptions_.end()
 *             Test UpdateCurrentNumberSeqByTime : streamDesc == nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetInitSegmentByStreamId_001, TestSize.Level0)
{
    // Test UpdateCurrentNumberSeqByTime : streamDesc == nullptr
    ASSERT_NE(mpdDownloader_, nullptr);
    std::shared_ptr<DashStreamDescription> streamDesc;
    uint32_t nextSegTime = NUM_TEST;
    mpdDownloader_->UpdateCurrentNumberSeqByTime(streamDesc, nextSegTime);

    // Test GetInitSegmentByStreamId : iter == streamDescriptions_.end()
    int streamId = NUM_TEST;
    auto ret = mpdDownloader_->GetInitSegmentByStreamId(streamId);
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name  : Test ParseManifest
 * @tc.number: ParseManifest_001
 * @tc.desc  : Test downloadContent_.length() == 0
 *             Test callback_ != nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseManifest_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->downloadContent_ = "";
    mpdDownloader_->ParseManifest();

    mpdDownloader_->downloadContent_ = "Test";
    mpdDownloader_->ondemandSegBase_ = false;
    auto callback = std::make_shared<MockDashMpdCallback>();
    mpdDownloader_->callback_ = callback;
    mpdDownloader_->ParseManifest();
    OSAL::SleepFor(100);
    int32_t defaultVideoStreamId = -1;
    mpdDownloader_->SetDefaultStreamId(defaultVideoStreamId, defaultVideoStreamId, defaultVideoStreamId);
    mpdDownloader_->GetSeekable();
    EXPECT_EQ(mpdDownloader_->notifyOpenOk_, true);
    mpdDownloader_->callback_.reset();
}

/**
 * @tc.name  : Test GetDrmInfos
 * @tc.number: GetDrmInfos_001
 * @tc.desc  : Test periodInfo == nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetDrmInfos_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* fakePtr = nullptr;
    mpdInfo->periodInfoList_.push_back(fakePtr);
    mpdDownloader_->mpdInfo_ = mpdInfo;
    std::vector<DashDrmInfo> drmInfos;
    mpdDownloader_->GetDrmInfos(drmInfos);
    EXPECT_NE(mpdDownloader_->mpdInfo_, nullptr);
    EXPECT_EQ(mpdDownloader_->mpdInfo_->periodInfoList_.size(), ID_TEST);
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetAdpDrmInfos
 * @tc.number: GetAdpDrmInfos_001
 * @tc.desc  : Test adptSetInfo == nullptr
 *             Test representationInfo == nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetAdpDrmInfos_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::vector<DashDrmInfo> drmInfo;
    DashPeriodInfo *const periodInfo = new DashPeriodInfo();
    DashAdptSetInfo* fakePtr = nullptr;
    periodInfo->adptSetList_.push_back(fakePtr);
    std::string periodDrmId = "testId";
    mpdDownloader_->GetAdpDrmInfos(drmInfo, periodInfo, periodDrmId);
    EXPECT_NE(periodInfo, nullptr);
    EXPECT_EQ(periodInfo->adptSetList_.size(), ID_TEST);

    fakePtr = new DashAdptSetInfo();
    DashRepresentationInfo* fakePtr2 = nullptr;
    fakePtr->representationList_.push_back(fakePtr2);
    mpdDownloader_->GetAdpDrmInfos(drmInfo, periodInfo, periodDrmId);
    EXPECT_NE(fakePtr, nullptr);
    EXPECT_EQ(fakePtr->representationList_.size(), ID_TEST);
    delete fakePtr;
    delete periodInfo;
}

/**
 * @tc.name  : Test GetDrmInfos
 * @tc.number: GetDrmInfos_001
 * @tc.desc  : Test contentProtection == nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetDrmInfos_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::string drmId = "TestID";
    DashList<DashDescriptor *> contentProtections;
    DashDescriptor* fakePtr = nullptr;
    contentProtections.push_back(fakePtr);
    std::vector<DashDrmInfo> drmInfoList;
    mpdDownloader_->GetDrmInfos(drmId, contentProtections, drmInfoList);
    EXPECT_EQ(contentProtections.size(), ID_TEST);
    bool hasNullptr = std::any_of(
        contentProtections.begin(),
        contentProtections.end(),
        [](DashDescriptor* ptr) { return ptr == nullptr; }
    );
    EXPECT_EQ(hasNullptr, true);
}

/**
 * @tc.name  : Test ParseSidx
 * @tc.number: ParseSidx_001
 * @tc.desc  : Test downloadContent_.length() == 0 && currentDownloadStream_ == nullptr
 *             Test parseRet != 0
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseSidx_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->ParseSidx();

    mpdDownloader_->downloadContent_ = "testContent";
    mpdDownloader_->currentDownloadStream_ = std::make_shared<DashStreamDescription>();
    mpdDownloader_->currentDownloadStream_->indexSegment_ = std::make_shared<DashIndexSegment>();
    mpdDownloader_->currentDownloadStream_->indexSegment_->indexRangeBegin_ = ID_TEST;
    mpdDownloader_->currentDownloadStream_->initSegment_ = std::make_shared<DashInitSegment>();
    EXPECT_EQ(mpdDownloader_->currentDownloadStream_->initSegment_->isDownloadFinish_, false);
    mpdDownloader_->ParseSidx();
    EXPECT_EQ(mpdDownloader_->currentDownloadStream_->initSegment_->isDownloadFinish_, true);
    std::list<std::shared_ptr<SubSegmentIndex>> subSegIndexList;
    int32_t parseRet = SidxBoxParser::ParseSidxBox(const_cast<char *>(mpdDownloader_->downloadContent_.c_str()),
        mpdDownloader_->downloadContent_.length(),
        mpdDownloader_->currentDownloadStream_->indexSegment_->indexRangeEnd_, subSegIndexList);
    EXPECT_NE(parseRet, 0);
}

/**
 * @tc.name  : Test BuildDashSegment
 * @tc.number: BuildDashSegment_001
 * @tc.desc  : Test segDurMsSum >= UINT_MAX
 */
HWTEST_F(DashMpdDownloaderUnittest, BuildDashSegment_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::list<std::shared_ptr<SubSegmentIndex>> subSegIndexList;
    std::shared_ptr<SubSegmentIndex> index = std::make_shared<SubSegmentIndex>();
    index->duration_ = (UINT_MAX / NUM_LARGE);
    subSegIndexList.push_back(index);
    mpdDownloader_->currentDownloadStream_ = std::make_shared<DashStreamDescription>();
    mpdDownloader_->BuildDashSegment(subSegIndexList);
    EXPECT_EQ(((index->duration_ * S_2_MS) > UINT_MAX), true);
}

/**
 * @tc.name  : Test DoOpen
 * @tc.number: DoOpen_001
 * @tc.desc  : Test statusCallback_ != nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, DoOpen_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->statusCallback_ = [](DownloadStatus status,
        std::shared_ptr<Downloader>& downloader, std::shared_ptr<DownloadRequest>& request) {
        std::cout << "Test Function" << std::endl;
    };
    std::string url = "testUrl";
    int64_t startRange = NUM_TEST;
    int64_t endRange = NUM_TEST;
    EXPECT_EQ(mpdDownloader_->downloadRequest_, nullptr);
    mpdDownloader_->DoOpen(url, startRange, endRange);
    EXPECT_NE(mpdDownloader_->downloadRequest_, nullptr);
}

/**
 * @tc.name  : Test GetSeekable
 * @tc.number: GetSeekable_001
 * @tc.desc  : Test times >= RETRY_TIMES || isInterruptNeeded_ == true
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSeekable_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->isInterruptNeeded_ = true;
    auto ret = mpdDownloader_->GetSeekable();
    EXPECT_EQ(ret, Seekable::INVALID);
}

/**
 * @tc.name  : Test SeekToTs
 * @tc.number: SeekToTs_001
 * @tc.desc  : Test mediaSegment == nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, SeekToTs_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    int streamId = NUM_TEST;
    int64_t seekTime = NUM_TEST;
    std::shared_ptr<DashSegment> seg;
    
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->streamId_ = streamId;
    ptr->segsState_ = DASH_SEGS_STATE_FINISH;
    std::shared_ptr<DashSegment> fakePtr = nullptr;
    ptr->mediaSegments_.push_back(fakePtr);
    mpdDownloader_->streamDescriptions_.push_back(ptr);
    auto ret = mpdDownloader_->SeekToTs(streamId, seekTime, seg);
    EXPECT_EQ(ret, NUM_TEST);
    bool hasNullptr = std::any_of(
        ptr->mediaSegments_.begin(),
        ptr->mediaSegments_.end(),
        [](std::shared_ptr<DashSegment> ptr) { return ptr == nullptr; }
    );
    EXPECT_EQ(hasNullptr, true);
}

/**
 * @tc.name  : Test GetSegmentsByPeriodInfo
 * @tc.number: GetSegmentsByPeriodInfo_001
 * @tc.desc  : Test periodInfo->periodSegTmplt_ != nullptr &&
 *                  periodInfo->periodSegTmplt_->segTmpltMedia_.length() > 0
 *             Test adptSetInfo != nullptr && streamDesc->bandwidth_ > 0
 *             Test repInfo != nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsByPeriodInfo_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashPeriodInfo *periodInfo = new DashPeriodInfo();
    auto periodSegTmplt = new DashSegTmpltInfo();
    periodInfo->periodSegTmplt_ = periodSegTmplt;
    periodInfo->periodSegTmplt_->segTmpltMedia_ = "testName";
    DashAdptSetInfo *adptSetInfo = new DashAdptSetInfo();
    std::string periodBaseUrl = "testUrl";
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->bandwidth_ = ID_TEST;
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->mpdInfo_->type_ = DashType::DASH_TYPE_DYNAMIC;
    auto ret = mpdDownloader_->GetSegmentsByPeriodInfo(periodInfo, adptSetInfo, periodBaseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);
    delete mpdInfo;
    delete periodSegTmplt;
    periodInfo->periodSegTmplt_ = nullptr;
    delete adptSetInfo;
    delete periodInfo;
}

/**
 * @tc.name  : Test GetSegmentsByPeriodInfo
 * @tc.number: GetSegmentsByPeriodInfo_002
 * @tc.desc  : Test periodInfo->periodSegList_ != nullptr && periodInfo->periodSegList_->segmentUrl_.size() > 0
 *             Test periodSegTmplt_ == nullptr && periodSegList_ == nullptr && baseUrl_.size() = 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsByPeriodInfo_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashPeriodInfo *periodInfo = new DashPeriodInfo();
    auto periodSegList = new DashSegListInfo();
    periodInfo->periodSegList_ = periodSegList;
    DashSegUrl* fakePtr = nullptr;
    periodInfo->periodSegList_->segmentUrl_.push_back(fakePtr);
    DashAdptSetInfo *adptSetInfo = new DashAdptSetInfo();
    std::string periodBaseUrl = "testUrl";
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->bandwidth_ = ID_TEST;
    auto ret = mpdDownloader_->GetSegmentsByPeriodInfo(periodInfo, adptSetInfo, periodBaseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    delete periodSegList;
    periodInfo->periodSegList_ = nullptr;

    ret = mpdDownloader_->GetSegmentsByPeriodInfo(periodInfo, adptSetInfo, periodBaseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_UNDO);
    delete adptSetInfo;
    delete periodInfo;
}

/**
 * @tc.name  : Test SetOndemandSegBase
 * @tc.number: SetOndemandSegBase_001
 * @tc.desc  : Test times >= RETRY_TIMES || isInterruptNeeded_ == true
 */
HWTEST_F(DashMpdDownloaderUnittest, SetOndemandSegBase_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::list<DashAdptSetInfo*> adptSetList;
    DashAdptSetInfo* setInfo = new DashAdptSetInfo();
    DashSegBaseInfo* adpSetSegBase =  new DashSegBaseInfo();
    setInfo->adptSetSegBase_ = adpSetSegBase;
    setInfo->adptSetSegBase_->indexRange_ = "testRange";
    adptSetList.push_back(setInfo);
    auto ret = mpdDownloader_->SetOndemandSegBase(adptSetList);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(mpdDownloader_->ondemandSegBase_, true);
    delete adpSetSegBase;
    setInfo->adptSetSegBase_ = nullptr;
    delete setInfo;
}

/**
 * @tc.name  : Test CheckToDownloadSidxWithInitSeg
 * @tc.number: CheckToDownloadSidxWithInitSeg_001
 * @tc.desc  : Test streamDesc->initSegment_ == nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, CheckToDownloadSidxWithInitSeg_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->initSegment_ = nullptr;
    auto ret = mpdDownloader_->CheckToDownloadSidxWithInitSeg(streamDesc);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test GetStreamsInfoInMpd
 * @tc.number: GetStreamsInfoInMpd_001
 * @tc.desc  : Test period == nullptr
 *             Test mpdInfo_ == nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetStreamsInfoInMpd_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->mpdInfo_ = nullptr;
    auto ret = mpdDownloader_->GetStreamsInfoInMpd();
    EXPECT_EQ(ret, false);

    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* fakePtr = nullptr;
    mpdInfo->periodInfoList_.push_back(fakePtr);
    mpdDownloader_->mpdInfo_ = mpdInfo;
    ret = mpdDownloader_->GetStreamsInfoInMpd();
    EXPECT_EQ(ret, true);
    EXPECT_EQ(mpdDownloader_->streamDescriptions_.size(), 0);
    delete mpdInfo;
}

/**
 * @tc.name  : Test GetStreamDescriptions
 * @tc.number: GetStreamDescriptions_001
 * @tc.desc  : Test it == nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetStreamDescriptions_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::string periodBaseUrl = "Test Url";
    DashStreamDescription streamDesc;
    std::string adptSetBaseUrl = "Test Url";
    std::list<DashRepresentationInfo *> repList;
    mpdDownloader_->GetStreamDescriptions(periodBaseUrl, streamDesc, adptSetBaseUrl, repList);
    EXPECT_EQ(repList.size(), NUM_TEST);
}

/**
 * @tc.name  : Test GetResolutionDelta
 * @tc.number: GetResolutionDelta_001
 * @tc.desc  : Test resolution > initResolution_
 */
HWTEST_F(DashMpdDownloaderUnittest, GetResolutionDelta_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    unsigned int width = ID_TEST;
    unsigned int height = ID_TEST;
    mpdDownloader_->initResolution_ = NUM_TEST;
    auto ret = mpdDownloader_->GetResolutionDelta(width, height);
    EXPECT_EQ(ret, ID_TEST);
}

/**
 * @tc.name  : Test IsLangMatch
 * @tc.number: IsLangMatch_001
 * @tc.desc  : Test type == MediaAVCodec::MediaType::MEDIA_TYPE_AUD && defaultAudioLang_.length() > 0
 *             Test defaultAudioLang_.compare(lang) == 0
 *             Test defaultAudioLang_.compare(lang) != 0
 *             Test type == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE && defaultAudioLang_.length() > 0
 *             Test defaultAudioLang_.compare(lang) == 0
 *             Test defaultAudioLang_.compare(lang) != 0
 */
HWTEST_F(DashMpdDownloaderUnittest, IsLangMatch_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::string lang = "testLang";
    MediaAVCodec::MediaType type = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    mpdDownloader_->defaultAudioLang_ = lang;
    auto ret = mpdDownloader_->IsLangMatch(lang, type);
    EXPECT_EQ(ret, true);

    ret = mpdDownloader_->IsLangMatch("lang", type);
    EXPECT_EQ(ret, false);

    type = MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE;
    mpdDownloader_->defaultSubtitleLang_ = lang;
    ret = mpdDownloader_->IsLangMatch(lang, type);
    EXPECT_EQ(ret, true);

    ret = mpdDownloader_->IsLangMatch("lang", type);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ChooseStreamToPlay
 * @tc.number: ChooseStreamToPlay_001
 * @tc.desc  : Test mpdInfo_ == nullptr || streamDescriptions_.size() == 0
 */
HWTEST_F(DashMpdDownloaderUnittest, ChooseStreamToPlay_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    MediaAVCodec::MediaType type = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    mpdDownloader_->mpdInfo_ = nullptr;
    ASSERT_EQ(mpdDownloader_->streamDescriptions_.size(), 0);
    int32_t streamId = 0;
    auto ret = mpdDownloader_->ChooseStreamToPlay(type, streamId);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test GetSegTimeBySeq
 * @tc.number: GetSegTimeBySeq_001
 * @tc.desc  : Test segments.size() == 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegTimeBySeq_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::vector<std::shared_ptr<DashSegment>> segments;
    int64_t segSeq = NUM_TEST;
    ASSERT_EQ(segments.size(), 0);
    auto ret = mpdDownloader_->GetSegTimeBySeq(segments, segSeq);
    EXPECT_EQ(ret, NUM_TEST);
}

/**
 * @tc.name  : Test GetSegmentsInMpd
 * @tc.number: GetSegmentsInMpd_001
 * @tc.desc  : Test segments.size() == 0
 *             Test streamDesc->mediaSegments_.size() == 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsInMpd_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->mpdInfo_ = nullptr;
    std::shared_ptr<DashStreamDescription> streamDesc = nullptr;
    auto ret = mpdDownloader_->GetSegmentsInMpd(streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);

    
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdDownloader_->mpdInfo_ = mpdInfo;
    streamDesc =  std::make_shared<DashStreamDescription>();
    EXPECT_EQ(streamDesc->mediaSegments_.size(), 0);
    ret = mpdDownloader_->GetSegmentsInMpd(streamDesc);
    ASSERT_EQ(ret, DASH_SEGMENT_INIT_FAILED);
}

/**
 * @tc.name  : Test GetSegmentsInPeriod
 * @tc.number: GetSegmentsInPeriod_001
 * @tc.desc  : Test streamDesc->type_ != MediaAVCodec::MediaType::MEDIA_TYPE_VID
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsInPeriod_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->mpdInfo_ = nullptr;
    DashPeriodInfo *periodInfo = new DashPeriodInfo();
    std::string mpdBaseUrl = "testUrl";
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    streamDesc->adptSetIndex_ = NUM_LARGE;
    auto ret = mpdDownloader_->GetSegmentsInPeriod(periodInfo, mpdBaseUrl, streamDesc);
    ASSERT_EQ(ret, DASH_SEGMENT_INIT_UNDO);
    delete periodInfo;
}

/**
 * @tc.name  : Test GetSegmentsInAdptSet
 * @tc.number: GetSegmentsInAdptSet_001
 * @tc.desc  : Test adptSetInfo == nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsInAdptSet_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashAdptSetInfo *adptSetInfo = nullptr;
    std::string periodBaseUrl = "testUrl";
    std::shared_ptr<DashStreamDescription> streamDesc;
    ASSERT_EQ(adptSetInfo, nullptr);
    auto ret = mpdDownloader_->GetSegmentsInAdptSet(adptSetInfo, periodBaseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_UNDO);
}

/**
 * @tc.name  : Test GetSegmentsInRepresentation
 * @tc.number: GetSegmentsInRepresentation_001
 * @tc.desc  : Test repInfo->representationSegTmplt_ != nullptr &&
 *             repInfo->representationSegTmplt_->segTmpltMedia_.length() > 0
 *             Test repInfo->baseUrl_.size() > 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsInRepresentation_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashRepresentationInfo *repInfo = new DashRepresentationInfo();
    DashSegTmpltInfo *tmpltInfo = new DashSegTmpltInfo();
    repInfo->representationSegTmplt_ = tmpltInfo;
    repInfo->representationSegTmplt_->segTmpltMedia_ = "testMedia";
    std::string periodBaseUrl = "testUrl";
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    ASSERT_NE(repInfo, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    auto ret = mpdDownloader_->GetSegmentsInRepresentation(repInfo, periodBaseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);
    delete tmpltInfo;
    repInfo->representationSegTmplt_ = nullptr;

    repInfo->baseUrl_.push_back(periodBaseUrl);
    ret = mpdDownloader_->GetSegmentsInRepresentation(repInfo, periodBaseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    delete repInfo;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetSegmentsWithSegTemplate
 * @tc.number: GetSegmentsWithSegTemplate_001
 * @tc.desc  : Test posIden != std::string::npos
 *             Test DashSubstituteTmpltStr(media, "$RepresentationID", id) == -1
 *             Test DashSubstituteTmpltStr(media, "$Bandwidth", std::to_string(streamDesc->bandwidth_)) == -1
 *             Test mpdInfo_->type_ != DashType::DASH_TYPE_STATIC
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsWithSegTemplate_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashSegTmpltInfo *segTmpltInfo = new DashSegTmpltInfo();
    segTmpltInfo->segTmpltMedia_ = "$$RepresentationID%03.mp4";
    std::string id = "test01";
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    auto ret = mpdDownloader_->GetSegmentsWithSegTemplate(segTmpltInfo, id, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);

    segTmpltInfo->segTmpltMedia_ = "$$Bandwidth%03.mp4";
    ret = mpdDownloader_->GetSegmentsWithSegTemplate(segTmpltInfo, id, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);

    segTmpltInfo->segTmpltMedia_ = "";
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    ret = mpdDownloader_->GetSegmentsWithSegTemplate(segTmpltInfo, id, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);
    delete segTmpltInfo;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetSegmentsWithTmpltStatic
 * @tc.number: GetSegmentsWithTmpltStatic_001
 * @tc.desc  : Test segTmpltInfo->multSegBaseInfo_.duration_ == 0 &&
 *             segTmpltInfo->multSegBaseInfo_.segTimeline_.size() == 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsWithTmpltStatic_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashSegTmpltInfo *segTmpltInfo = new DashSegTmpltInfo();
    segTmpltInfo->multSegBaseInfo_.duration_ = NUM_TEST;
    std::string mediaUrl = "testUrl";
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    EXPECT_EQ(segTmpltInfo->multSegBaseInfo_.segTimeline_.size(), NUM_TEST);
    auto ret = mpdDownloader_->GetSegmentsWithTmpltStatic(segTmpltInfo, mediaUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);
    delete segTmpltInfo;
}

/**
 * @tc.name  : Test GetSegmentsWithTmpltDurationStatic
 * @tc.number: GetSegmentsWithTmpltDurationStatic_001
 * @tc.desc  : Test segmentDuration == 0
 *             Test desc->duration_ - accumulateDuration < segRealDur
 *             Test DashSubstituteTmpltStr(tempUrl, "$Time", std::to_string(startTime)) == -1
 *             Test DashSubstituteTmpltStr(tempUrl, "$Number", std::to_string(segmentSeq)) == -1
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsWithTmpltDurationStatic_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashSegTmpltInfo *segTmpltInfo = new DashSegTmpltInfo();
    segTmpltInfo->multSegBaseInfo_.duration_ = NUM_TEST;
    std::string mediaUrl = "testUrl";
    unsigned int timeScale = NUM_TEST;
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    auto ret = mpdDownloader_->GetSegmentsWithTmpltDurationStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);

    segTmpltInfo->multSegBaseInfo_.duration_ = ID_TEST;
    streamDesc->duration_ = ID_TEST;
    mediaUrl = "$$Time%03.mp4";
    ret = mpdDownloader_->GetSegmentsWithTmpltDurationStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);

    mediaUrl = "$$Number%03.mp4";
    ret = mpdDownloader_->GetSegmentsWithTmpltDurationStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);

    mediaUrl = "testUrl";
    ret = mpdDownloader_->GetSegmentsWithTmpltDurationStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    delete segTmpltInfo;
}

/**
 * @tc.name  : Test GetSegmentsWithTmpltTimelineStatic
 * @tc.number: GetSegmentsWithTmpltTimelineStatic_001
 * @tc.desc  : Test timeScale == 0
 *             Test segCount < 0
 *             Test GetSegmentsInOneTimeline(_, _, _, _, _) == DASH_SEGMENT_INIT_FAILED
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsWithTmpltTimelineStatic_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashSegTmpltInfo *segTmpltInfo = new DashSegTmpltInfo();
    segTmpltInfo->multSegBaseInfo_.duration_ = NUM_TEST;
    std::string mediaUrl = "testUrl";
    unsigned int timeScale = NUM_TEST;
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    EXPECT_EQ(segTmpltInfo->multSegBaseInfo_.segTimeline_.size(), NUM_TEST);
    auto ret = mpdDownloader_->GetSegmentsWithTmpltTimelineStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);

    DashSegTimeline* dashSegTimeline = new DashSegTimeline();
    DashSegTimeline* fakePtr = new DashSegTimeline();
    dashSegTimeline->r_ = INVALID_TEST;
    dashSegTimeline->d_ = ID_TEST;
    fakePtr->r_ = NUM_TEST;
    fakePtr->d_ = ID_TEST;
    segTmpltInfo->multSegBaseInfo_.segTimeline_.push_back(fakePtr);
    segTmpltInfo->multSegBaseInfo_.segTimeline_.push_back(dashSegTimeline);
    mediaUrl = "$$Time%03.mp4";
    ret = mpdDownloader_->GetSegmentsWithTmpltTimelineStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);
    delete segTmpltInfo;
    delete fakePtr;
    delete dashSegTimeline;
}

/**
 * @tc.name  : Test GetSegmentsInOneTimeline
 * @tc.number: GetSegmentsInOneTimeline_001
 * @tc.desc  : Test DashSubstituteTmpltStr(tempUrl, "$Time", std::to_string(startTime)) == -1
 *             Test DashSubstituteTmpltStr(tempUrl, "$Number", std::to_string(segmentSeq)) == -1
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsInOneTimeline_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    
    DashSegTimeline* dashSegTimeline = new DashSegTimeline();
    MediaSegSampleInfo sampleInfo;
    sampleInfo.segCount_ = NUM_TEST;
    sampleInfo.mediaUrl_ = "$$Time%03.mp4";
    int64_t segmentSeq = NUM_TEST;
    uint64_t startTime = NUM_TEST;
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    auto ret = mpdDownloader_->GetSegmentsInOneTimeline(dashSegTimeline,
        sampleInfo, segmentSeq, startTime, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);

    sampleInfo.mediaUrl_ = "$$Number%03.mp4";
    ret = mpdDownloader_->GetSegmentsInOneTimeline(dashSegTimeline,
        sampleInfo, segmentSeq, startTime, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_FAILED);
    delete dashSegTimeline;
}

/**
 * @tc.name  : Test GetSegCountFromTimeline
 * @tc.number: GetSegCountFromTimeline_001
 * @tc.desc  : Test segCount < 0 && (*it)->d_ == 0
 *             Test it != end && *it != nullptr
 *             Test nextStartTime <= startTime
 *             Test nextStartTime > startTime
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegCountFromTimeline_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashSegTimeline* dashSegTimeline = new DashSegTimeline();
    DashSegTimeline* fakePtr = new DashSegTimeline();
    dashSegTimeline->r_ = INVALID_TEST;
    dashSegTimeline->t_ = NUM_TEST;
    dashSegTimeline->d_ = ID_TEST;
    fakePtr->r_ = INVALID_TEST;
    fakePtr->d_ = ID_TEST;
    DashList<DashSegTimeline *> dashList;
    dashList.push_back(fakePtr);
    dashList.push_back(dashSegTimeline);
    DashList<DashSegTimeline *>::iterator it = dashList.begin();
    DashList<DashSegTimeline *>::iterator end = dashList.end();
    unsigned int periodDuration = NUM_TEST;
    unsigned int timeScale = NUM_TEST;
    uint64_t startTime = ID_TEST;
    auto ret = mpdDownloader_->GetSegCountFromTimeline(it, end, periodDuration, timeScale, startTime);
    EXPECT_EQ(ret, INVALID_TEST);

    dashSegTimeline->t_ = NUM2_TEST;
    ret = mpdDownloader_->GetSegCountFromTimeline(it, end, periodDuration, timeScale, startTime);
    EXPECT_EQ(ret, NUM_TEST);
    delete dashSegTimeline;
    delete fakePtr;
}

/**
 * @tc.name  : Test GetSegCountFromTimeline
 * @tc.number: GetSegCountFromTimeline_002
 * @tc.desc  : Test it == end
 *             Test scaleDuration <= startTime
 *             Test scaleDuration > startTime
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegCountFromTimeline_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashSegTimeline* fakePtr = new DashSegTimeline();
    fakePtr->r_ = INVALID_TEST;
    fakePtr->d_ = ID_TEST;
    DashList<DashSegTimeline *> dashList;
    dashList.push_back(fakePtr);
    DashList<DashSegTimeline *>::iterator it = dashList.begin();
    DashList<DashSegTimeline *>::iterator end = dashList.end();
    unsigned int periodDuration = NUM_TEST;
    unsigned int timeScale = NUM_TEST;
    uint64_t startTime = ID_TEST;
    auto ret = mpdDownloader_->GetSegCountFromTimeline(it, end, periodDuration, timeScale, startTime);
    EXPECT_EQ(ret, INVALID_TEST);

    fakePtr->t_ = NUM2_TEST;
    periodDuration = NUM_LARGE;
    timeScale = NUM10_TEST;
    ret = mpdDownloader_->GetSegCountFromTimeline(it, end, periodDuration, timeScale, startTime);
    EXPECT_EQ(ret, NUM7_TEST);
    delete fakePtr;
}

/**
 * @tc.name  : Test GetSegmentsWithSegList
 * @tc.number: GetSegmentsWithSegList_001
 * @tc.desc  : Test *it == nullptr
 *             Test baseUrl.length() > 0
 *             Test (*it)->media_.length() == 0 && baseUrl.length() == 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsWithSegList_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashSegListInfo *segListInfo = new DashSegListInfo();
    DashSegUrl* fakePtr = nullptr;
    DashSegUrl* dashSegUrl = new DashSegUrl();
    dashSegUrl->media_ = "";
    segListInfo->segmentUrl_.push_back(fakePtr);
    segListInfo->segmentUrl_.push_back(dashSegUrl);
    std::string baseUrl = "testUrl";
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    auto ret = mpdDownloader_->GetSegmentsWithSegList(segListInfo, baseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    EXPECT_EQ(streamDesc->mediaSegments_.size(), ID_TEST);

    baseUrl = "";
    streamDesc->mediaSegments_.clear();
    ret = mpdDownloader_->GetSegmentsWithSegList(segListInfo, baseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    EXPECT_EQ(streamDesc->mediaSegments_.size(), NUM_TEST);
    delete dashSegUrl;
    delete segListInfo;
}

/**
 * @tc.name  : Test GetSegDurationFromTimeline
 * @tc.number: GetSegDurationFromTimeline_001
 * @tc.desc  : Test timeScale == 0
 *             Test *it == nullptr
 *             Test segCount < 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegDurationFromTimeline_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    unsigned int periodDuration = NUM_TEST;
    unsigned int timeScale = NUM_TEST;
    DashMultSegBaseInfo *multSegBaseInfo = new DashMultSegBaseInfo();
    multSegBaseInfo->duration_ = NUM_TEST;
    DashSegTimeline *fakePtr = nullptr;
    DashSegTimeline *dashSegTimeline = new DashSegTimeline();
    multSegBaseInfo->segTimeline_.push_back(fakePtr);
    multSegBaseInfo->segTimeline_.push_back(dashSegTimeline);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    DashList<unsigned int> durationList;
    mpdDownloader_->GetSegDurationFromTimeline(periodDuration, timeScale, multSegBaseInfo, durationList);

    timeScale = ID_TEST;
    mpdDownloader_->GetSegDurationFromTimeline(periodDuration, timeScale, multSegBaseInfo, durationList);
    EXPECT_EQ(durationList.size(), NUM_TEST);
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
    delete dashSegTimeline;
    delete multSegBaseInfo;
}

/**
 * @tc.name  : Test GetRepresemtationFromAdptSet
 * @tc.number: GetRepresemtationFromAdptSet_001
 * @tc.desc  : Test it == adptSetInfo->representationList_.end()
 */
HWTEST_F(DashMpdDownloaderUnittest, GetRepresemtationFromAdptSet_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashAdptSetInfo *adptSetInfo = new DashAdptSetInfo();
    unsigned int repIndex = NUM_TEST;
    EXPECT_EQ(adptSetInfo->representationList_.size(), NUM_TEST);
    auto ret = mpdDownloader_->GetRepresemtationFromAdptSet(adptSetInfo, repIndex);
    EXPECT_EQ(ret, nullptr);
    delete adptSetInfo;
}

/**
 * @tc.name  : Test GetSegmentsByAdptSetInfo
 * @tc.number: GetSegmentsByAdptSetInfo_001
 * @tc.desc  : Test adptSetInfo->baseUrl_.size() > 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsByAdptSetInfo_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashAdptSetInfo* adptSetInfo = new DashAdptSetInfo();
    adptSetInfo->adptSetSegTmplt_ = nullptr;
    adptSetInfo->baseUrl_.push_back("testUrl");
    DashRepresentationInfo* repInfo = nullptr;
    std::string baseUrl = "testUrl";
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    auto ret = mpdDownloader_->GetSegmentsByAdptSetInfo(adptSetInfo, repInfo, baseUrl, streamDesc);
    EXPECT_EQ(ret, DASH_SEGMENT_INIT_SUCCESS);
    delete adptSetInfo;
}

/**
 * @tc.name  : Test GetInitSegFromPeriod
 * @tc.number: GetInitSegFromPeriod_001
 * @tc.desc  : Test initSegment != nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetInitSegFromPeriod_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashUrlType *dashUrlType = new DashUrlType();
    mpdDownloader_->periodManager_->initSegment_ = dashUrlType;
    std::string periodBaseUrl = "testUrl";
    std::string repId = "testId";
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    auto ret = mpdDownloader_->GetInitSegFromPeriod(periodBaseUrl, repId, streamDesc);
    EXPECT_EQ(ret, true);
    delete dashUrlType;
    mpdDownloader_->periodManager_->initSegment_ = nullptr;
}

/**
 * @tc.name  : Test GetInitSegFromAdptSet
 * @tc.number: GetInitSegFromAdptSet_001
 * @tc.desc  : Test initSegment == nullptr
 */
HWTEST_F(DashMpdDownloaderUnittest, GetInitSegFromAdptSet_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->adptSetManager_->initSegment_ = nullptr;
    std::string periodBaseUrl = "testUrl";
    std::string repId = "testId";
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    auto ret = mpdDownloader_->GetInitSegFromAdptSet(periodBaseUrl, repId, streamDesc);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test GetSegmentsInNewStream
 * @tc.number: GetSegmentsInNewStream_001
 * @tc.desc  : Test destStream->indexSegment_ == nullptr
 *             Test ondemandSegBase_ == false
 *             Test GetSegmentsInMpd(destStream) == DASH_SEGMENT_INIT_FAILED
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSegmentsInNewStream_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->ondemandSegBase_ = true;
    std::shared_ptr<DashStreamDescription> destStream = std::make_shared<DashStreamDescription>();
    destStream->segsState_ = DASH_SEGS_STATE_NONE;
    destStream->indexSegment_ = nullptr;
    auto ret = mpdDownloader_->GetSegmentsInNewStream(destStream);
    EXPECT_EQ(ret, DASH_MPD_GET_ERROR);

    mpdDownloader_->ondemandSegBase_ = false;
    EXPECT_EQ(mpdDownloader_->mpdInfo_, nullptr);
    ret = mpdDownloader_->GetSegmentsInNewStream(destStream);
    EXPECT_EQ(ret, DASH_MPD_GET_DONE);
}

/**
 * @tc.name  : Test UpdateInitSegUrl
 * @tc.number: UpdateInitSegUrl_001
 * @tc.desc  : Test posIden != std::string::npos
 *             Test DashSubstituteTmpltStr
 *                  (streamDesc->initSegment_->url_, "$RepresentationID", representationID) == -1
 *             Test DashSubstituteTmpltStr(streamDesc->initSegment_->url_, "$Bandwidth",
 *                  std::to_string(streamDesc->bandwidth_)) == -1
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateInitSegUrl_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
    DashUrlType *urlType = new DashUrlType();
    urlType->sourceUrl_ = "$$RepresentationID%03.mp4";
    int segTmpltFlag = ID_TEST;
    std::string representationID = "testID";
    mpdDownloader_->UpdateInitSegUrl(streamDesc, urlType, segTmpltFlag, representationID);
    EXPECT_EQ(streamDesc->initSegment_->url_, "$RepresentationID%03.mp4");

    urlType->sourceUrl_ = "$$Bandwidth%03.mp4";
    mpdDownloader_->UpdateInitSegUrl(streamDesc, urlType, segTmpltFlag, representationID);
    EXPECT_EQ(streamDesc->initSegment_->url_, "$Bandwidth%03.mp4");
    delete urlType;
}

/**
 * @tc.name  : Test GetStreamInfo && GetVideoType
 * @tc.number: GetStreamInfo_001
 * @tc.desc  : Test streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE
 *             Test streamDescriptions_[index]->adptSetIndex_ == subtitleAdptSetIndex
 *             Test streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE
 *             Test GetVideoType: videoType == DASH_VIDEO_TYPE_HDR_10
 */
HWTEST_F(DashMpdDownloaderUnittest, GetStreamInfo_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE;
    ptr->adptSetIndex_ = ID_TEST;
    mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::vector<StreamInfo> streams;
    auto ret = mpdDownloader_->GetStreamInfo(streams);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(streams.size(), 1);

    mpdDownloader_->streamDescriptions_.clear();
    auto ptr2 = std::make_shared<DashStreamDescription>();
    ptr2->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr2->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE;
    ptr2->adptSetIndex_ = NUM_TEST;
    ptr->videoType_ = DASH_VIDEO_TYPE_HDR_10;
    mpdDownloader_->streamDescriptions_.push_back(ptr2);
    std::vector<StreamInfo> streams2;
    ret = mpdDownloader_->GetStreamInfo(streams2);
    EXPECT_EQ(ret, Status::OK);
    ASSERT_NE(streams2.size(), 0);
    EXPECT_EQ(streams2[0].type, SUBTITLE);
    EXPECT_EQ(streams2[0].videoType, VIDEO_TYPE_HDR_10);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_001
 * @tc.desc  : Test str.empty()
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = FPS_24;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("", value);
    EXPECT_EQ(ret, false);
    EXPECT_EQ(value, FPS_24);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_002
 * @tc.desc  : Test numStr.empty()
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("/2", value);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_003
 * @tc.desc  : Test denStr.empty()
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_003, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("24/", value);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_004
 * @tc.desc  : Test pos < numStr.length()
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_004, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("24x/2", value);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_005
 * @tc.desc  : Test pos < denStr.length()
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_005, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("24/2x", value);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_006
 * @tc.desc  : Test den < 0.0
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_006, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("24/-2", value);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_007
 * @tc.desc  : Test std::abs(den) < TOLERANCE
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_007, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("24/0", value);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_008
 * @tc.desc  : Test fps < 0.0 in ratio branch
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_008, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("-24/2", value);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_009
 * @tc.desc  : Test ratio branch success with integer frame rate
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_009, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("24/1", value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, FPS_24);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_010
 * @tc.desc  : Test ratio branch success with ceil result
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_010, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("30000/1001", value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, FPS_30);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_011
 * @tc.desc  : Test ratio branch success when fps is zero
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_011, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = FPS_24;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("0/2", value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, static_cast<uint32_t>(NUM_TEST));
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_012
 * @tc.desc  : Test pos < frameRateStr.length()
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_012, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("29.97fps", value);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_013
 * @tc.desc  : Test fps < 0.0 in plain branch
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_013, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("-29.97", value);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_014
 * @tc.desc  : Test plain branch success with integer frame rate
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_014, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("24", value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, FPS_24);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_015
 * @tc.desc  : Test plain branch success with ceil result
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_015, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = NUM_TEST;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("29.01", value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, FPS_30);
}

/**
 * @tc.name  : Test ParseFrameRateToUint32
 * @tc.number: ParseFrameRateToUint32_016
 * @tc.desc  : Test plain branch success when fps is zero
 */
HWTEST_F(DashMpdDownloaderUnittest, ParseFrameRateToUint32_016, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    uint32_t value = FPS_24;

    auto ret = mpdDownloader_->ParseFrameRateToUint32("0", value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, static_cast<uint32_t>(NUM_TEST));
}

/**
 * @tc.name  : Test GetDuration
 * @tc.number: GetDuration_001
 * @tc.desc  : Test mpdInfo_ != nullptr && mpdInfo_->type_ == DashType::DASH_TYPE_DYNAMIC
 */
HWTEST_F(DashMpdDownloaderUnittest, GetDuration_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* fakePtr = nullptr;
    mpdInfo->periodInfoList_.push_back(fakePtr);
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->mpdInfo_->timeShiftBufferDepth_ = 0;
    mpdDownloader_->duration_ = ID_TEST;
    auto ret = mpdDownloader_->GetDuration();
    EXPECT_EQ(ret, NUM1000000_TEST);
    EXPECT_NE(mpdDownloader_->mpdInfo_, nullptr);
    EXPECT_EQ(mpdDownloader_->mpdInfo_->periodInfoList_.size(), ID_TEST);
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetDuration
 * @tc.number: GetDuration_002
 * @tc.desc  : Test mpdInfo_ != nullptr && mpdInfo_->type_ == DashType::DASH_TYPE_DYNAMIC & timeShiftDepth > 0
 */
HWTEST_F(DashMpdDownloaderUnittest, GetDuration_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* fakePtr = nullptr;
    mpdInfo->periodInfoList_.push_back(fakePtr);
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->mpdInfo_->timeShiftBufferDepth_ = ID_TEST;
    mpdDownloader_->duration_ = ID_TEST;
    auto ret = mpdDownloader_->GetDuration();
    EXPECT_EQ(ret, NUM1000000_TEST);
    EXPECT_NE(mpdDownloader_->mpdInfo_, nullptr);
    EXPECT_EQ(mpdDownloader_->mpdInfo_->periodInfoList_.size(), ID_TEST);
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetDuration
 * @tc.number: GetDuration_003
 * @tc.desc  : Test mpdInfo_->type_ == DashType::DASH_TYPE_STATIC
 */
HWTEST_F(DashMpdDownloaderUnittest, GetDuration_003, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* fakePtr = nullptr;
    mpdInfo->periodInfoList_.push_back(fakePtr);
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->mpdInfo_->timeShiftBufferDepth_ = 0;
    mpdDownloader_->duration_ = 0;
    auto ret = mpdDownloader_->GetDuration();
    EXPECT_EQ(ret, 0);
    EXPECT_NE(mpdDownloader_->mpdInfo_, nullptr);
    EXPECT_EQ(mpdDownloader_->mpdInfo_->periodInfoList_.size(), ID_TEST);
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetSeekable
 * @tc.number: GetSeekable_002
 * @tc.desc  : Test mpdInfo_->type_ == DASH_TYPE_DYNAMIC & timeShiftBufferDepth_ > 0, return Seekable::SEEKABLE
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSeekable_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->isInterruptNeeded_ = false;
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdInfo->timeShiftBufferDepth_ = NUM100_TEST;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->notifyOpenOk_ = true;
    auto ret = mpdDownloader_->GetSeekable();
    EXPECT_EQ(ret, Seekable::SEEKABLE);
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetSeekable
 * @tc.number: GetSeekable_003
 * @tc.desc  : Test times >= RETRY_TIMES || isInterruptNeeded_ == false & mpdInfo_->type_ != DashType::DASH_TYPE_DYNAMIC
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSeekable_003, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->isInterruptNeeded_ = false;
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    DashPeriodInfo* fakePtr = nullptr;
    mpdInfo->periodInfoList_.push_back(fakePtr);
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->notifyOpenOk_ = true;
    mpdDownloader_->mpdInfo_->timeShiftBufferDepth_ = 0;
    auto ret = mpdDownloader_->GetSeekable();
    EXPECT_EQ(ret, Seekable::SEEKABLE);
    EXPECT_NE(mpdDownloader_->mpdInfo_, nullptr);
    EXPECT_EQ(mpdDownloader_->mpdInfo_->periodInfoList_.size(), ID_TEST);
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetSeekable
 * @tc.number: GetSeekable_004
 * @tc.desc  : Test mpdInfo_->type_ == DASH_TYPE_DYNAMIC & timeShiftBufferDepth_ <= 0, return Seekable::UNSEEKABLE
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSeekable_004, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->isInterruptNeeded_ = false;
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdInfo->timeShiftBufferDepth_ = 0;
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->notifyOpenOk_ = true;
    auto ret = mpdDownloader_->GetSeekable();
    EXPECT_EQ(ret, Seekable::UNSEEKABLE);
    mpdInfo->timeShiftBufferDepth_ = INVALID50_TEST;
    ret = mpdDownloader_->GetSeekable();
    EXPECT_EQ(ret, Seekable::SEEKABLE);
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_001
 * @tc.desc  : Test periodRanges empty and duration_ > 0, fill default range
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->mpdInfo_ = nullptr;
    mpdDownloader_->duration_ = NUM1000_TEST;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), 1);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].start, 0);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].end, NUM1000_TEST * MS_2_NS);
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_003
 * @tc.desc  : Test dynamic type + timeShiftBufferDepth_>0 + windowStart<0
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_003, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdInfo->timeShiftBufferDepth_ = NUM500_TEST;
    DashPeriodInfo* periodInfo = new DashPeriodInfo();
    periodInfo->start_ = NUM100_TEST;
    periodInfo->duration_ = NUM200_TEST;
    mpdInfo->periodInfoList_.push_back(periodInfo);
    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), ID_TEST);
    delete periodInfo;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_004
 * @tc.desc  : Test dynamic type + timeShiftBufferDepth_>0 + windowStart≥0
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_004, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdInfo->timeShiftBufferDepth_ = NUM200_TEST;
    DashPeriodInfo* periodInfo = new DashPeriodInfo();
    periodInfo->start_ = NUM100_TEST;
    periodInfo->duration_ = NUM300_TEST;
    mpdInfo->periodInfoList_.push_back(periodInfo);

    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), ID_TEST);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].start, NUM200_TEST * MS_2_NS);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].end, NUM400_TEST * MS_2_NS);
    delete periodInfo;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_005
 * @tc.desc  : Test non-dynamic type, directly use periodRanges
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_005, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdInfo->timeShiftBufferDepth_ = NUM100_TEST;
    DashPeriodInfo* p1 = new DashPeriodInfo();
    p1->start_ = 0;
    p1->duration_ = NUM500_TEST;
    DashPeriodInfo* p2 = new DashPeriodInfo();
    p2->start_ = NUM500_TEST;
    p2->duration_ = NUM500_TEST;
    mpdInfo->periodInfoList_.push_back(p1);
    mpdInfo->periodInfoList_.push_back(p2);

    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), NUM2_TEST);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].start, 0);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].end, NUM500_TEST * MS_2_NS);
    EXPECT_EQ(mpdDownloader_->seekRanges_[1].start, NUM500_TEST * MS_2_NS);
    EXPECT_EQ(mpdDownloader_->seekRanges_[1].end, NUM1000_TEST * MS_2_NS);
    delete p1;
    delete p2;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_006
 * @tc.desc  : Test CollectPeriodRanges durationMs=0, fill by nextStartMs
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_006, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    DashPeriodInfo* p1 = new DashPeriodInfo();
    p1->start_ = NUM100_TEST;
    p1->duration_ = 0;
    DashPeriodInfo* p2 = new DashPeriodInfo();
    p2->start_ = NUM400_TEST;
    p2->duration_ = NUM200_TEST;
    mpdInfo->periodInfoList_.push_back(p1);
    mpdInfo->periodInfoList_.push_back(p2);

    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), NUM2_TEST);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].start, NUM100_TEST * MS_2_NS);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].end, NUM400_TEST * MS_2_NS);
    delete p1;
    delete p2;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_007
 * @tc.desc  : Test ClipRangesToWindow empty, fill window range
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_007, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdInfo->timeShiftBufferDepth_ = NUM100_TEST;
    DashPeriodInfo* p1 = new DashPeriodInfo();
    p1->start_ = NUM200_TEST;
    p1->duration_ = NUM100_TEST;
    mpdInfo->periodInfoList_.push_back(p1);

    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), ID_TEST);
    delete p1;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_008
 * @tc.desc  : Test periodRanges.empty() && duration_ == 0, no range added
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_008, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->mpdInfo_ = nullptr;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), 0);
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_009
 * @tc.desc  : Test CollectPeriodRanges: periodInfo == nullptr in list
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_009, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdInfo->periodInfoList_.push_back(nullptr);
    DashPeriodInfo* p1 = new DashPeriodInfo();
    p1->start_ = NUM100_TEST;
    p1->duration_ = NUM200_TEST;
    mpdInfo->periodInfoList_.push_back(p1);

    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), 1);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].start, NUM100_TEST * MS_2_NS);
    delete p1;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_010
 * @tc.desc  : Test CollectPeriodRanges: durationMs == 0, fill by mediaPresentationDuration_
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_010, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdInfo->mediaPresentationDuration_ = NUM1000_TEST;
    DashPeriodInfo* p1 = new DashPeriodInfo();
    p1->start_ = NUM100_TEST;
    p1->duration_ = 0;
    mpdInfo->periodInfoList_.push_back(p1);

    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), 1);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].start, NUM100_TEST * MS_2_NS);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].end, NUM1000_TEST * MS_2_NS);
    delete p1;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_011
 * @tc.desc  : Test CollectPeriodRanges: durationMs == 0, fill by fallbackDurationMs
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_011, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdInfo->mediaPresentationDuration_ = 0;
    DashPeriodInfo* p1 = new DashPeriodInfo();
    p1->start_ = NUM100_TEST;
    p1->duration_ = 0;
    mpdInfo->periodInfoList_.push_back(p1);

    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = NUM1000_TEST;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), 1);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].start, NUM100_TEST * MS_2_NS);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].end, NUM1000_TEST * MS_2_NS);
    delete p1;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_012
 * @tc.desc  : Test CollectPeriodRanges: durationMs == 0 after all attempts, should skip
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_012, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    mpdInfo->mediaPresentationDuration_ = 0;
    DashPeriodInfo* p1 = new DashPeriodInfo();
    p1->start_ = NUM100_TEST;
    p1->duration_ = 0;
    mpdInfo->periodInfoList_.push_back(p1);

    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), 0);
    delete p1;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_013
 * @tc.desc  : Test CollectPeriodRanges: multiple periods, take max totalEndNs
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_013, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_STATIC;
    DashPeriodInfo* p1 = new DashPeriodInfo();
    p1->start_ = 0;
    p1->duration_ = NUM500_TEST;
    DashPeriodInfo* p2 = new DashPeriodInfo();
    p2->start_ = NUM500_TEST;
    p2->duration_ = NUM500_TEST;
    mpdInfo->periodInfoList_.push_back(p1);
    mpdInfo->periodInfoList_.push_back(p2);

    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), 2);
    EXPECT_EQ(mpdDownloader_->seekRanges_[1].end, NUM1000_TEST * MS_2_NS);
    delete p1;
    delete p2;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_014
 * @tc.desc  : Test ClipRangesToWindow: normal clipping case with DYNAMIC type
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_014, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashMpdInfo* mpdInfo = new DashMpdInfo();
    mpdInfo->type_ = DashType::DASH_TYPE_DYNAMIC;
    mpdInfo->timeShiftBufferDepth_ = NUM500_TEST;
    DashPeriodInfo* p1 = new DashPeriodInfo();
    p1->start_ = 0;
    p1->duration_ = NUM1000_TEST;
    mpdInfo->periodInfoList_.push_back(p1);

    mpdDownloader_->mpdInfo_ = mpdInfo;
    mpdDownloader_->duration_ = 0;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), 1);
    EXPECT_GE(mpdDownloader_->seekRanges_[0].start, 0);
    EXPECT_LE(mpdDownloader_->seekRanges_[0].end, NUM1000_TEST * MS_2_NS);
    delete p1;
    delete mpdInfo;
    mpdDownloader_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test UpdateSeekRanges
 * @tc.number: UpdateSeekRanges_015
 * @tc.desc  : Test mpdInfo_ == nullptr, falls back to duration_
 */
HWTEST_F(DashMpdDownloaderUnittest, UpdateSeekRanges_015, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->mpdInfo_ = nullptr;
    mpdDownloader_->duration_ = NUM500_TEST;
    mpdDownloader_->seekRanges_.clear();
    mpdDownloader_->UpdateSeekRanges();
    std::lock_guard<std::mutex> lock(mpdDownloader_->seekRangeMutex_);
    EXPECT_EQ(mpdDownloader_->seekRanges_.size(), 1);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].start, 0);
    EXPECT_EQ(mpdDownloader_->seekRanges_[0].end, NUM500_TEST * MS_2_NS);
}

/**
 * @tc.name  : Test FillStreamDescription
 * @tc.number: FillStreamDescription_001
 * @tc.desc  : Test FillStreamDescription updates stream fields from representation info
 */
HWTEST_F(DashMpdDownloaderUnittest, FillStreamDescription_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    DashStreamDescription streamDesc;
    DashRepresentationInfo repInfo;
    repInfo.bandwidth_ = NUM1000_TEST;
    repInfo.commonAttrsAndElements_.width_ = NUM1000_TEST;
    repInfo.commonAttrsAndElements_.height_ = NUM500_TEST;
    repInfo.commonAttrsAndElements_.codecs_ = "avc1.64001f";
    repInfo.commonAttrsAndElements_.frameRate_ = "24";
    repInfo.commonAttrsAndElements_.channels_ = NUM2_TEST;

    mpdDownloader_->FillStreamDescription(streamDesc, &repInfo, ID_TEST, DASH_VIDEO_TYPE_HDR_10);

    EXPECT_EQ(streamDesc.representationIndex_, static_cast<unsigned int>(ID_TEST));
    EXPECT_EQ(streamDesc.bandwidth_, static_cast<unsigned int>(NUM1000_TEST));
    EXPECT_EQ(streamDesc.width_, static_cast<unsigned int>(NUM1000_TEST));
    EXPECT_EQ(streamDesc.height_, static_cast<unsigned int>(NUM500_TEST));
    EXPECT_EQ(streamDesc.codecs_, "avc1.64001f");
    EXPECT_EQ(streamDesc.frameRate_, "24");
    EXPECT_EQ(streamDesc.channels_, NUM2_TEST);
    EXPECT_EQ(streamDesc.videoType_, DASH_VIDEO_TYPE_HDR_10);
}

/**
 * @tc.name  : Test GetMimeType
 * @tc.number: GetMimeType_001
 * @tc.desc  : Test GetMimeType returns normalized video mime type
 */
HWTEST_F(DashMpdDownloaderUnittest, GetMimeType_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);

    auto ret = mpdDownloader_->GetMimeType("video/avc1.64001f", VIDEO);

    EXPECT_EQ(ret, "video/avc1");
}

/**
 * @tc.name  : Test ContainsNonDigit
 * @tc.number: ContainsNonDigit_001
 * @tc.desc  : Test ContainsNonDigit returns true when input contains non-digit characters
 */
HWTEST_F(DashMpdDownloaderUnittest, ContainsNonDigit_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);

    auto ret = mpdDownloader_->ContainsNonDigit("24fps");

    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test GetSeekableRanges
 * @tc.number: GetSeekableRanges_001
 * @tc.desc  : Test GetSeekableRanges
 */
HWTEST_F(DashMpdDownloaderUnittest, GetSeekableRanges_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    auto ret = mpdDownloader_->GetSeekableRanges();
    EXPECT_EQ(ret.size(), 0);
}

/**
 * @tc.name  : Test SeekToTs with stream switch
 * @tc.number: SeekToTs_002
 * @tc.desc  : Test SwitchStreamIfNeeded(targetPeriodIndex != currentStream->periodIndex_)
 */
HWTEST_F(DashMpdDownloaderUnittest, SeekToTs_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    int streamId = 1;
    int64_t seekTime = 5000;
    std::shared_ptr<DashSegment> seg;

    auto currentStream = std::make_shared<DashStreamDescription>();
    currentStream->streamId_ = streamId;
    currentStream->segsState_ = DASH_SEGS_STATE_FINISH;
    currentStream->periodIndex_ = 0;
    currentStream->periodStartTime_ = 0;
    currentStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newStream = std::make_shared<DashStreamDescription>();
    currentStream->streamId_ = 2;
    currentStream->segsState_ = DASH_SEGS_STATE_FINISH;
    currentStream->periodIndex_ = 1;
    currentStream->periodStartTime_ = 4000;
    currentStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto seg1 = std::make_shared<DashSegment>();
    seg1->duration_ = 2000;
    seg1->numberSeq_ = 1;
    newStream->mediaSegments_.push_back(seg1);

    mpdDownloader_->streamDescriptions_.push_back(currentStream);
    mpdDownloader_->streamDescriptions_.push_back(newStream);
    
    auto ret = mpdDownloader_->SeekToTs(streamId, seekTime, seg);
    EXPECT_EQ(ret, 4000);
}

/**
 * @tc.name  : Test SeekToTs with negative relativeTime
 * @tc.number: SeekToTs_003
 * @tc.desc  : Test relativeTime < 0 case
 */
HWTEST_F(DashMpdDownloaderUnittest, SeekToTs_003, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();

    int streamId = 1;
    int64_t seekTime = 1000;
    std::shared_ptr<DashSegment> seg;

    auto currentStream = std::make_shared<DashStreamDescription>();
    currentStream->streamId_ = streamId;
    currentStream->segsState_ = DASH_SEGS_STATE_FINISH;
    currentStream->periodIndex_ = 1;
    currentStream->periodStartTime_ = 2000;
    currentStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto seg1 = std::make_shared<DashSegment>();
    seg1->duration_ = 1000;
    seg1->numberSeq_ = 1;
    newStream->mediaSegments_.push_back(seg1);

    mpdDownloader_->streamDescriptions_.push_back(currentStream);
    
    auto ret = mpdDownloader_->SeekToTs(streamId, seekTime, seg);
    EXPECT_EQ(ret, 1000);
}

/**
 * @tc.name  : Test SeekToTs with empty mediaSegment
 * @tc.number: SeekToTs_004
 * @tc.desc  : Test mediaSegments_.empty() case
 */
HWTEST_F(DashMpdDownloaderUnittest, SeekToTs_004, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();

    int streamId = 1;
    int64_t seekTime = 1000;
    std::shared_ptr<DashSegment> seg;

    auto currentStream = std::make_shared<DashStreamDescription>();
    currentStream->streamId_ = streamId;
    currentStream->segsState_ = DASH_SEGS_STATE_FINISH;
    currentStream->periodIndex_ = 0;
    currentStream->periodStartTime_ = 0;
    currentStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    mpdDownloader_->streamDescriptions_.push_back(currentStream);
    
    auto ret = mpdDownloader_->SeekToTs(streamId, seekTime, seg);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(seg, nullptr);
}

/**
 * @tc.name  : Test GetStreamByPeriodIndex
 * @tc.number: GetStreamByPeriodIndex_001
 * @tc.desc  : Test GetStreamByPeriodIndex with existing stream
 */
HWTEST_F(DashMpdDownloaderUnittest, GetStreamByPeriodIndex_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();

    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->streamId_ = 1;
    streamDesc->periodIndex_ = 0;
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    mpdDownloader_->streamDescriptions_.push_back(streamDesc);
    
    auto ret = mpdDownloader_->GetStreamByPeriodIndex(0, MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(ret, nullptr);
    EXPECT_EQ(ret->streamId_, 1);
}

/**
 * @tc.name  : Test GetStreamByPeriodIndex not found
 * @tc.number: GetStreamByPeriodIndex_002
 * @tc.desc  : Test GetStreamByPeriodIndex with non-existing period
 */
HWTEST_F(DashMpdDownloaderUnittest, GetStreamByPeriodIndex_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();
    
    auto ret = mpdDownloader_->GetStreamByPeriodIndex(99, MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name  : Test GetPeriodIndexBySeekTime
 * @tc.number: GetPeriodIndexBySeekTime_001
 * @tc.desc  : Test GetPeriodIndexBySeekTime with currentPeriodIndex
 */
HWTEST_F(DashMpdDownloaderUnittest, GetPeriodIndexBySeekTime_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();
    mpdDownloader_->currentPeriodIndex_ = 0;

    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->streamId_ = 1;
    streamDesc->periodIndex_ = 0;
    streamDesc->periodStartTime_ = 0;
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc->segState_ = DASH_SEGS_STATE_FINISH;
    mpdDownloader_->streamDescriptions_.push_back(streamDesc);

    auto ret = mpdDownloader_->GetPeriodIndexBySeekTime(1000);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name  : Test GetPeriodIndexBySeekTime with segsState FINISH
 * @tc.number: GetPeriodIndexBySeekTime_002
 * @tc.desc  : Test GetPeriodIndexBySeekTime with currentPeriodIndex stream not found
 */
HWTEST_F(DashMpdDownloaderUnittest, GetPeriodIndexBySeekTime_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();
    mpdDownloader_->currentPeriodIndex_ = 99;

    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->streamId_ = 1;
    streamDesc->periodIndex_ = 0;
    streamDesc->periodStartTime_ = 0;
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc->segState_ = DASH_SEGS_STATE_FINISH;
    mpdDownloader_->streamDescriptions_.push_back(streamDesc);

    auto ret = mpdDownloader_->GetPeriodIndexBySeekTime(1000);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name  : Test GetPeriodIndexBySeekTime with multiple periods
 * @tc.number: GetPeriodIndexBySeekTime_003
 * @tc.desc  : Test GetPeriodIndexBySeekTime selecting correct period based on seekTime
 */
HWTEST_F(DashMpdDownloaderUnittest, GetPeriodIndexBySeekTime_003, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();
    mpdDownloader_->currentPeriodIndex_ = 0;

    auto streamDesc1 = std::make_shared<DashStreamDescription>();
    streamDesc1->streamId_ = 1;
    streamDesc1->periodIndex_ = 0;
    streamDesc1->periodStartTime_ = 0;
    streamDesc1->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc1->segState_ = DASH_SEGS_STATE_FINISH;
    mpdDownloader_->streamDescriptions_.push_back(streamDesc1);

    auto streamDesc2 = std::make_shared<DashStreamDescription>();
    streamDesc2->streamId_ = 2;
    streamDesc2->periodIndex_ = 1;
    streamDesc2->periodStartTime_ = 5000;
    streamDesc2->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    streamDesc2->segState_ = DASH_SEGS_STATE_FINISH;
    mpdDownloader_->streamDescriptions_.push_back(streamDesc2);

    auto ret = mpdDownloader_->GetPeriodIndexBySeekTime(6000);
    EXPECT_EQ(ret, 1);
}

/**
 * @tc.name  : Test SwitchStreamIfNeeded with empty mediaSegment
 * @tc.number: SwitchStreamIfNeeded_001
 * @tc.desc  : Test SwitchStreamIfNeeded when mediaSegment is empty
 */
HWTEST_F(DashMpdDownloaderUnittest, SwitchStreamIfNeeded_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();

    auto currentStream = std::make_shared<DashStreamDescription>();
    currentStream->streamId_ = 1;
    currentStream->periodIndex_ = 0;
    currentStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    currentStream->mediaSegments_.clear();

    auto targetStream = std::make_shared<DashStreamDescription>();
    targetStream->streamId_ = 2;
    targetStream->periodIndex_ = 1;
    targetStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    targetStream->mediaSegments_.clear();

    mpdDownloader_->streamDescriptions_.push_back(currentStream);
    mpdDownloader_->streamDescriptions_.push_back(targetStream);

    mpdDownloader_->SwitchStreamIfNeeded(currentStream, 1);
    EXPECT_EQ(currentStream->streamId_, 2);
}

/**
 * @tc.name  : Test FindTargetPeriodIndex - need switch
 * @tc.number: FindTargetPeriodIndex_001
 * @tc.desc  : Test FindTargetPeriodIndex returns true when targetPeriodIndex != currentStream->periodIndex_
 */
HWTEST_F(DashMpdDownloaderUnittest, FindTargetPeriodIndex_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();

    auto currentStream = std::make_shared<DashStreamDescription>();
    currentStream->streamId_ = 1;
    currentStream->periodIndex_ = 0;
    currentStream->periodStartTime_ = 0;
    currentStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->streamId_ = 2;
    streamDesc->periodIndex_ = 1;
    streamDesc->periodStartTime_ = 5000;
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    mpdDownloader_->streamDescriptions_.push_back(currentStream);
    mpdDownloader_->streamDescriptions_.push_back(streamDesc);

    uint32_t targetPeriodIndex = 0;
    uint64_t periodStartTime = 0;
    auto ret = mpdDownloader_->FindTargetPeriodIndex(currentStream, 6000, targetPeriodIndex, periodStartTime);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(targetPeriodIndex, 1);
}

/**
 * @tc.name  : Test FindTargetPeriodIndex - no need switch
 * @tc.number: FindTargetPeriodIndex_002
 * @tc.desc  : Test FindTargetPeriodIndex returns false when targetPeriodIndex == currentStream->periodIndex_
 */
HWTEST_F(DashMpdDownloaderUnittest, FindTargetPeriodIndex_002, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();

    auto currentStream = std::make_shared<DashStreamDescription>();
    currentStream->streamId_ = 1;
    currentStream->periodIndex_ = 1;
    currentStream->periodStartTime_ = 5000;
    currentStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->streamId_ = 2;
    streamDesc->periodIndex_ = 1;
    streamDesc->periodStartTime_ = 5000;
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    mpdDownloader_->streamDescriptions_.push_back(currentStream);
    mpdDownloader_->streamDescriptions_.push_back(streamDesc);

    uint32_t targetPeriodIndex = 0;
    uint64_t periodStartTime = 0;
    auto ret = mpdDownloader_->FindTargetPeriodIndex(currentStream, 6000, targetPeriodIndex, periodStartTime);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test FindSegmentByRelativeTime with empty segment
 * @tc.number: FindSegmentByRelativeTime_001
 * @tc.desc  : Test FindSegmentByRelativeTime when mediaSegments_ is empty
 */
HWTEST_F(DashMpdDownloaderUnittest, FindSegmentByRelativeTime_001, TestSize.Level0)
{
    ASSERT_NE(mpdDownloader_, nullptr);
    mpdDownloader_->streamDescriptions_.clear();

    auto streamDesc = std::make_shared<DashStreamDescription>();
    streamDesc->mediaSegments_.clear();
    mpdDownloader_->streamDescriptions_.push_back(streamDesc);

    std::shared_ptr<DashSegment> seg;
    int64_t totalDuration = 0;
    mpdDownloader_->FindSegmentByRelativeTime(streamDesc, 1000, seg, 0, totalDuration);
    EXPECT_EQ(seg, nullptr);
}
}
}
}
}