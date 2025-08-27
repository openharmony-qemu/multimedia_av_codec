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

#include "dash_segment_downloader_unittest.h"
#include "http_server_demo.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr int32_t SERVERPORT = 47777;
static const std::string AUDIO_SEGMENT_URL = "http://127.0.0.1:47777/test_dash/segment_base/media-audio-und-mp4a.mp4";

constexpr uint32_t DEFAULT_RING_BUFFER_SIZE = 5 * 1024 * 1024;
constexpr uint32_t AUD_RING_BUFFER_SIZE = 2 * 1024 * 1024;
constexpr uint32_t RECORD_TIME_INTERVAL = 1000;
constexpr uint32_t SPEED_MULTI_FACT = 1000;
constexpr uint32_t BYTE_TO_BIT = 8;
constexpr uint32_t TIMES_ONE = 1;
constexpr uint32_t ID_STREAM = 1;
constexpr uint32_t NUM_TEST = 10;
constexpr size_t MAX_BUFFERING_TIME_OUT = 30 * 1000;
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
    segmentDownloader_ = std::make_shared<DashSegmentDownloader>(nullptr, ID_STREAM,
            MediaAVCodec::MediaType::MEDIA_TYPE_AUD, NUM_TEST, nullptr);
}

void DashSegmentDownloaderUnitTest::TearDown(void)
{
    segmentDownloader_ = nullptr;
}

/**
 * @tc.name  : Test Open
 * @tc.number: Open_001
 * @tc.desc  : Test Open mediaSegment_->byteRange_.length() > 0
 *             Test Open mediaSegment_->startRangeValue_ >= 0
 *             && mediaSegment_->endRangeValue_ < mediaSegment_->startRangeValue_
 */
HWTEST_F(DashSegmentDownloaderUnitTest, Open_001, TestSize.Level1)
{
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = AUDIO_SEGMENT_URL;
    segmentSp->streamId_ = 1;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 1;
    segmentSp->numberSeq_ = 1;
    segmentSp->byteRange_ = "1024-0";
    segmentDownloader_->Open(segmentSp);
    segmentDownloader_->Close(true, true);
    EXPECT_EQ(segmentDownloader_->mediaSegment_->startRangeValue_, 1024);
    EXPECT_EQ(segmentDownloader_->mediaSegment_->endRangeValue_, 0);
}

/**
 * @tc.name  : Test Open
 * @tc.number: Open_002
 * @tc.desc  : Test Open initSegment->isDownloadFinish_ == true
 *             Test ~DashSegmentDownloader buffer_ == nullptr
 *             Test ~DashSegmentDownloader downloader_ == nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, Open_002, TestSize.Level1)
{
    // Test Open initSegment->isDownloadFinish_ == true
    std::shared_ptr<DashSegment> segmentSp = std::make_shared<DashSegment>();
    segmentSp->url_ = AUDIO_SEGMENT_URL;
    segmentSp->streamId_ = ID_STREAM;
    segmentSp->duration_ = 5;
    segmentSp->bandwidth_ = 1024;
    segmentSp->startNumberSeq_ = 1;
    segmentSp->numberSeq_ = 1;
    std::shared_ptr<DashInitSegment> initSegment = std::make_shared<DashInitSegment>();
    initSegment->streamId_ = ID_STREAM;
    segmentDownloader_->SetInitSegment(initSegment);
    initSegment->writeState_ = INIT_SEGMENT_STATE_UNUSE;
    initSegment->isDownloadFinish_ = true;
    segmentDownloader_->Open(segmentSp);
    segmentDownloader_->Close(true, true);

    // Test ~DashSegmentDownloader buffer_ == nullptr
    segmentDownloader_->buffer_->SetActive(false, true);
    segmentDownloader_->buffer_.reset();

    // Test ~DashSegmentDownloader downloader_ == nullptr
    segmentDownloader_->downloader_->Stop(false);
    segmentDownloader_->downloader_.reset();

    EXPECT_EQ(initSegment->writeState_, INIT_SEGMENT_STATE_USED);
}

/**
 * @tc.name  : Test Read
 * @tc.number: Read_001
 * @tc.desc  : Test Read currentSegment == nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, Read_001, TestSize.Level1)
{
    ReadDataInfo readDataInfo;
    readDataInfo.streamId_ = 1;
    readDataInfo.nextStreamId_ = 1;
    readDataInfo.wantReadLength_ = 1024;
    readDataInfo.realReadLength_ = 0;
    uint8_t buffer[1024];
    std::atomic<bool> isInterruptNeeded = false;

    std::shared_ptr<DashInitSegment> initSegment = std::make_shared<DashInitSegment>();
    initSegment->streamId_ = 1;
    initSegment->isDownloadFinish_ = true;
    initSegment->readState_ = INIT_SEGMENT_STATE_UNUSE;
    segmentDownloader_->initSegments_.push_back(initSegment);

    DashReadRet result = segmentDownloader_->Read(buffer, readDataInfo, isInterruptNeeded);
    segmentDownloader_->Close(true, true);
    EXPECT_EQ(result, DASH_READ_OK);
}

/**
 * @tc.name  : Test GetMaxReadLength
 * @tc.number: GetMaxReadLength_001
 * @tc.desc  : Test GetMaxReadLength currentSegment == nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetMaxReadLength_001, TestSize.Level1)
{
    uint32_t wantReadLength = 1024;
    int32_t currentStreamId = 1;
    uint32_t len = segmentDownloader_->GetMaxReadLength(wantReadLength, nullptr, currentStreamId);
    EXPECT_EQ(len, wantReadLength);
}

/**
 * @tc.name  : Test GetMaxReadLength
 * @tc.number: GetMaxReadLength_002
 * @tc.desc  : Test GetMaxReadLength availableSize <= 0
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetMaxReadLength_002, TestSize.Level1)
{
    uint32_t wantReadLength = 1024;
    int32_t currentStreamId = 1;
    std::shared_ptr<DashBufferSegment> currentSegment = std::make_shared<DashBufferSegment>();
    currentSegment->bufferPosTail_ = 0;
    uint32_t len = segmentDownloader_->GetMaxReadLength(wantReadLength, currentSegment, currentStreamId);
    EXPECT_EQ(len, wantReadLength);
}

/**
 * @tc.name  : Test IsSegmentFinished
 * @tc.number: IsSegmentFinished_001
 * @tc.desc  : Test IsSegmentFinished buffer_->GetSize() != 0
 */
HWTEST_F(DashSegmentDownloaderUnitTest, IsSegmentFinished_001, TestSize.Level1)
{
    segmentDownloader_->isAllSegmentFinished_ = true;
    segmentDownloader_->buffer_->head_ = 0;
    segmentDownloader_->buffer_->tail_ = 1024;
    uint32_t realReadLength = 0;
    DashReadRet readRet = DASH_READ_OK;
    bool ret = segmentDownloader_->IsSegmentFinished(realReadLength, readRet);
    EXPECT_TRUE(!ret);
}

/**
 * @tc.name  : Test IsSegmentFinished
 * @tc.number: IsSegmentFinished_002
 * @tc.desc  : Test IsSegmentFinished mediaSegment_ == nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, IsSegmentFinished_002, TestSize.Level1)
{
    segmentDownloader_->isAllSegmentFinished_ = true;
    segmentDownloader_->buffer_->head_ = 0;
    segmentDownloader_->buffer_->tail_ = 0;
    segmentDownloader_->mediaSegment_ = nullptr;
    uint32_t realReadLength = 0;
    DashReadRet readRet = DASH_READ_OK;
    segmentDownloader_->IsSegmentFinished(realReadLength, readRet);
    EXPECT_EQ(readRet, DASH_READ_END);
}

/**
 * @tc.name  : Test CheckReadInterrupt
 * @tc.number: CheckReadInterrupt_001
 * @tc.desc  : Test CheckReadInterrupt HandleCache() == false
 */
HWTEST_F(DashSegmentDownloaderUnitTest, CheckReadInterrupt_001, TestSize.Level1)
{
    segmentDownloader_->isAllSegmentFinished_ = false;
    segmentDownloader_->isBuffering_ = false;
    segmentDownloader_->isFirstFrameArrived_ = false;

    uint32_t realReadLength = 0;
    uint32_t wantReadLength = 0;
    DashReadRet readRet = DASH_READ_OK;
    std::atomic<bool> isInterruptNeeded = true;
    segmentDownloader_->CheckReadInterrupt(realReadLength, wantReadLength, readRet, isInterruptNeeded);
    EXPECT_EQ(readRet, DASH_READ_INTERRUPT);
}

/**
 * @tc.name  : Test HandleBuffering
 * @tc.number: HandleBuffering_001
 * @tc.desc  : Test HandleBuffering isInterruptNeeded.load() == true
 */
HWTEST_F(DashSegmentDownloaderUnitTest, HandleBuffering_001, TestSize.Level1)
{
    segmentDownloader_->isBuffering_ = true;
    std::atomic<bool> isInterruptNeeded = true;
    bool ret = segmentDownloader_->HandleBuffering(isInterruptNeeded);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name  : Test HandleBuffering
 * @tc.number: HandleBuffering_002
 * @tc.desc  : Test HandleBuffering isAllSegmentFinished_.load() == true
 *             Test SaveDataHandleBuffering isInterruptNeeded.load() == true
 */
HWTEST_F(DashSegmentDownloaderUnitTest, HandleBuffering_002, TestSize.Level1)
{
    segmentDownloader_->isBuffering_ = true;
    segmentDownloader_->isAllSegmentFinished_.store(true);

    // Test SaveDataHandleBuffering isInterruptNeeded.load() == true
    segmentDownloader_->bufferDurationForPlaying_ = 0;
    segmentDownloader_->SaveDataHandleBuffering();

    // Test HandleBuffering isAllSegmentFinished_.load() == true
    std::atomic<bool> isInterruptNeeded = false;
    bool ret = segmentDownloader_->HandleBuffering(isInterruptNeeded);
    EXPECT_TRUE(!ret);
}

/**
 * @tc.name  : Test HandleCache
 * @tc.number: HandleCache_001
 * @tc.desc  : Test HandleCache isBuffering_.exchange(true) = true
 *             Test DoBufferingEndEvent isBuffering_.exchange(false) = false
 */
HWTEST_F(DashSegmentDownloaderUnitTest, SaveDataHandleBuffering_001, TestSize.Level1)
{
    // Test DoBufferingEndEvent isBuffering_.exchange(false) = false
    segmentDownloader_->isBuffering_ = false;
    segmentDownloader_->DoBufferingEndEvent();

    // Test HandleCache isBuffering_.exchange(true) = true
    segmentDownloader_->isBuffering_ = true;
    bool ret = segmentDownloader_->HandleCache();
    EXPECT_TRUE(!ret);
}

/**
 * @tc.name  : Test GetWaterLineAbove
 * @tc.number: GetWaterLineAbove_001
 * @tc.desc  : Test GetWaterLineAbove downloadBiteRate_ != 0
 *             Test GetWaterLineAbove realTimeBitBate_ > static_cast<int64_t>(downloadBiteRate_)
 *             Test GetWaterLineAbove realTimeBitBate_ <= static_cast<int64_t>(downloadBiteRate_)
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetWaterLineAbove_001, TestSize.Level1)
{
    // Test GetWaterLineAbove realTimeBitBate_ > static_cast<int64_t>(downloadBiteRate_)
    segmentDownloader_->downloadRequest_ = std::make_shared<DownloadRequest>("test_request", nullptr, nullptr);
    segmentDownloader_->realTimeBitBate_ = 1024 * 1024;
    segmentDownloader_->downloadBiteRate_ = 1024;

    int32_t waterLineAbove = segmentDownloader_->GetWaterLineAbove();
    EXPECT_EQ(waterLineAbove, 1024 * 1024);

    // Test GetWaterLineAbove realTimeBitBate_ <= static_cast<int64_t>(downloadBiteRate_)
    segmentDownloader_->downloadBiteRate_ = 1024 * 1024 * 2;
    waterLineAbove = segmentDownloader_->GetWaterLineAbove();
    EXPECT_EQ(waterLineAbove, 39321);
}

/**
 * @tc.name  : Test GetCachedPercent
 * @tc.number: GetCachedPercent_001
 * @tc.desc  : Test GetCachedPercent waterLineAbove_ == 0
 *             Test GetCachedPercent waterLineAbove_ != 0
 *             Test CalculateBitRate fragmentSize == 0
 *             Test CalculateBitRate duration == 0
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetCachedPercent_001, TestSize.Level1)
{
    // Test CalculateBitRate fragmentSize == 0
    size_t fragmentSize = 0;
    double duration = 1.0;
    segmentDownloader_->CalculateBitRate(fragmentSize, duration);

    // Test CalculateBitRate duration == 0
    fragmentSize = 1024;
    duration = 0.0;
    segmentDownloader_->CalculateBitRate(fragmentSize, duration);

    // Test GetCachedPercent waterLineAbove_ == 0
    segmentDownloader_->waterLineAbove_ = 0;
    uint32_t cachedPercent = segmentDownloader_->GetCachedPercent();
    EXPECT_EQ(cachedPercent, 0);

    // Test GetCachedPercent waterLineAbove_ != 0
    segmentDownloader_->waterLineAbove_ = 100;
    segmentDownloader_->buffer_->head_ = 0;
    segmentDownloader_->buffer_->tail_ = 100;
    cachedPercent = segmentDownloader_->GetCachedPercent();
    EXPECT_EQ(cachedPercent, 100);
}

/**
 * @tc.name  : Test UpdateCachedPercent
 * @tc.number: UpdateCachedPercent_001
 * @tc.desc  : Test UpdateCachedPercent (waterLineAbove_ == 0 || bufferingCbFunc_ == nullptr) = false
 *             Test UpdateCachedPercent infoType == BufferingInfoType::BUFFERING_START
 *             Test UpdateCachedPercent infoType == BufferingInfoType::BUFFERING_END
 *             Test UpdateCachedPercent infoType == BufferingInfoType::BUFFERING_PERCENT
 *             Test UpdateCachedPercent infoType == BufferingInfoType::CACHED_DURATION
 */
HWTEST_F(DashSegmentDownloaderUnitTest, UpdateCachedPercent_001, TestSize.Level1)
{
    // Test (waterLineAbove_ == 0 || bufferingCbFunc_ == nullptr) = false
    segmentDownloader_->waterLineAbove_ = 1;
    segmentDownloader_->bufferingCbFunc_ = [] (uint32_t percent, BufferingInfoType infoType) {};

    // Test infoType == BufferingInfoType::BUFFERING_START
    segmentDownloader_->lastCachedSize_ = 1024;
    segmentDownloader_->UpdateCachedPercent(BufferingInfoType::BUFFERING_START);
    EXPECT_EQ(segmentDownloader_->lastCachedSize_, 0);

    // Test infoType == BufferingInfoType::BUFFERING_END
    segmentDownloader_->lastCachedSize_ = 1024;
    segmentDownloader_->UpdateCachedPercent(BufferingInfoType::BUFFERING_END);
    EXPECT_EQ(segmentDownloader_->lastCachedSize_, 0);

    // Test infoType == BufferingInfoType::BUFFERING_PERCENT
    segmentDownloader_->lastCachedSize_ = 1024;
    segmentDownloader_->UpdateCachedPercent(BufferingInfoType::BUFFERING_PERCENT);
    EXPECT_EQ(segmentDownloader_->lastCachedSize_, 1024);

    // Test infoType == BufferingInfoType::CACHED_DURATION
    segmentDownloader_->UpdateCachedPercent(BufferingInfoType::CACHED_DURATION);
    EXPECT_EQ(segmentDownloader_->lastCachedSize_, 1024);
}

/**
 * @tc.name  : Test UpdateCachedPercent
 * @tc.number: UpdateCachedPercent_002
 * @tc.desc  : Test UpdateCachedPercent (deltaSize * BUFFERING_PERCENT_FULL / waterLineAbove_) >= UPDATE_CACHE_STEP
 *             Test UpdateCachedPercent (deltaSize * BUFFERING_PERCENT_FULL / waterLineAbove_) < UPDATE_CACHE_STEP
 */
HWTEST_F(DashSegmentDownloaderUnitTest, UpdateCachedPercent_002, TestSize.Level1)
{
    segmentDownloader_->waterLineAbove_ = 1;
    segmentDownloader_->bufferingCbFunc_ = [] (uint32_t percent, BufferingInfoType infoType) {};

    // Test (deltaSize * BUFFERING_PERCENT_FULL / waterLineAbove_) >= UPDATE_CACHE_STEP
    segmentDownloader_->lastCachedSize_ = 1024;
    segmentDownloader_->buffer_->head_ = 0;
    segmentDownloader_->buffer_->tail_ = 1024 * 2;
    segmentDownloader_->UpdateCachedPercent(BufferingInfoType::BUFFERING_PERCENT);
    EXPECT_EQ(segmentDownloader_->lastCachedSize_, 1024 * 2);

    // Test (deltaSize * BUFFERING_PERCENT_FULL / waterLineAbove_) < UPDATE_CACHE_STEP
    segmentDownloader_->UpdateCachedPercent(BufferingInfoType::BUFFERING_PERCENT);
    EXPECT_EQ(segmentDownloader_->lastCachedSize_, 1024 * 2);
}

/**
 * @tc.name  : Test GetCurrentSegment
 * @tc.number: GetCurrentSegment_001
 * @tc.desc  : Test GetCurrentSegment it == segmentList_.end()
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetCurrentSegment_001, TestSize.Level1)
{
    segmentDownloader_->segmentList_.clear();
    std::shared_ptr<DashBufferSegment> item = std::make_shared<DashBufferSegment>();
    item->bufferPosHead_ = 1;
    item->bufferPosTail_ = 1024;
    segmentDownloader_->segmentList_.emplace_back(item);
    segmentDownloader_->buffer_->head_ = 0;
    EXPECT_EQ(segmentDownloader_->GetCurrentSegment(), nullptr);
}

/**
 * @tc.name  : Test UpdateInitSegmentState
 * @tc.number: UpdateInitSegmentState_001
 * @tc.desc  : Test UpdateInitSegmentState (*it)->streamId_ != currentStreamId
 */
HWTEST_F(DashSegmentDownloaderUnitTest, UpdateInitSegmentState_001, TestSize.Level1)
{
    std::shared_ptr<DashInitSegment> item = std::make_shared<DashInitSegment>();
    segmentDownloader_->initSegments_.emplace_back(item);
    int32_t currentStreamId = 1;
    segmentDownloader_->UpdateInitSegmentState(currentStreamId);
    EXPECT_EQ(item->readState_, INIT_SEGMENT_STATE_UNUSE);
}

/**
 * @tc.name  : Test ReadInitSegment
 * @tc.number: ReadInitSegment_001
 * @tc.desc  : Test ReadInitSegment initSegment->readIndex_ == contentLen && initSegment->isDownloadFinish_
 */
HWTEST_F(DashSegmentDownloaderUnitTest, ReadInitSegment_001, TestSize.Level1)
{
    std::shared_ptr<DashInitSegment> item = std::make_shared<DashInitSegment>();
    item->streamId_ = 1;
    item->readState_ = INIT_SEGMENT_STATE_UNUSE;
    item->isDownloadFinish_ = true;
    segmentDownloader_->initSegments_.emplace_back(item);

    uint8_t buff[100] = {0};
    uint32_t wantReadLength = 100;
    uint32_t realReadLength = 1;
    int32_t currentStreamId = 1;
    segmentDownloader_->ReadInitSegment(buff, wantReadLength, realReadLength, currentStreamId);
    EXPECT_EQ(item->readState_, INIT_SEGMENT_STATE_USED);
}

/**
 * @tc.name  : Test ReadInitSegment
 * @tc.number: ReadInitSegment_002
 * @tc.desc  : Test ReadInitSegment initSegment->readIndex_ == contentLen && initSegment->isDownloadFinish_
 *             when unReadSize > 0
 */
HWTEST_F(DashSegmentDownloaderUnitTest, ReadInitSegment_002, TestSize.Level1)
{
    std::shared_ptr<DashInitSegment> item = std::make_shared<DashInitSegment>();
    item->streamId_ = 1;
    item->readState_ = INIT_SEGMENT_STATE_UNUSE;
    item->isDownloadFinish_ = true;
    item->content_ = "test_content";
    segmentDownloader_->initSegments_.emplace_back(item);

    uint8_t buff[100] = {0};
    uint32_t wantReadLength = 100;
    uint32_t realReadLength = 1;
    int32_t currentStreamId = 1;
    segmentDownloader_->ReadInitSegment(buff, wantReadLength, realReadLength, currentStreamId);
    EXPECT_EQ(item->readState_, INIT_SEGMENT_STATE_USED);

    item->isDownloadFinish_ = false;
    item->readState_ = INIT_SEGMENT_STATE_UNUSE;
    segmentDownloader_->ReadInitSegment(buff, wantReadLength, realReadLength, currentStreamId);
    EXPECT_EQ(item->readState_, INIT_SEGMENT_STATE_UNUSE);

    item->readIndex_ = 12;
    segmentDownloader_->ReadInitSegment(buff, wantReadLength, realReadLength, currentStreamId);
    EXPECT_EQ(item->readState_, INIT_SEGMENT_STATE_UNUSE);
}

/**
 * @tc.name  : Test GetRingBufferInitSize
 * @tc.number: GetRingBufferInitSize_001
 * @tc.desc  : Test GetRingBufferInitSize ringBufferSizeItem == BUFFER_SIZE_MAP.end()
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetRingBufferInitSize_001, TestSize.Level1)
{
    MediaAVCodec::MediaType streamType = MediaAVCodec::MediaType::MEDIA_TYPE_TIMED_METADATA;
    size_t bufferSize = segmentDownloader_->GetRingBufferInitSize(streamType);
    EXPECT_EQ(bufferSize, DEFAULT_RING_BUFFER_SIZE);
}

/**
 * @tc.name  : Test GetRingBufferInitSize
 * @tc.number: GetRingBufferInitSize_002
 * @tc.desc  : Test GetRingBufferInitSize ringBufferSize < DEFAULT_RING_BUFFER_SIZE
 *             Test SetInitSegment initSegment == nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetRingBufferInitSize_002, TestSize.Level1)
{
    // Test SetInitSegment initSegment == nullptr
    std::shared_ptr<DashInitSegment> initSegment = nullptr;
    bool needUpdateState = true;
    segmentDownloader_->SetInitSegment(initSegment, needUpdateState);

    // Test GetRingBufferInitSize ringBufferSize < DEFAULT_RING_BUFFER_SIZE
    MediaAVCodec::MediaType streamType = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    segmentDownloader_->userDefinedBufferDuration_ = true;
    segmentDownloader_->expectDuration_ = 1;
    segmentDownloader_->currentBitrate_ = 1 * 1024 * 1024;
    size_t bufferSize = segmentDownloader_->GetRingBufferInitSize(streamType);
    EXPECT_EQ(bufferSize, DEFAULT_RING_BUFFER_SIZE);
}

/**
 * @tc.name  : Test GetContentLength
 * @tc.number: GetContentLength_001
 * @tc.desc  : Test GetContentLength (downloadRequest_ == nullptr || downloadRequest_->IsClosed()) == false
 */
HWTEST_F(DashSegmentDownloaderUnitTest, SetInitSegment_001, TestSize.Level1)
{
    segmentDownloader_->downloadRequest_ = std::make_shared<DownloadRequest>("test_request", nullptr, nullptr);
    segmentDownloader_->downloadRequest_->headerInfo_.isClosed = false;
    segmentDownloader_->downloadRequest_->headerInfo_.isChunked = true;
    segmentDownloader_->downloadRequest_->isHeaderUpdated_ = true;
    size_t fileContentLen = segmentDownloader_->GetContentLength();
    EXPECT_EQ(fileContentLen, 0);
}

/**
 * @tc.name  : Test IsSegmentFinish
 * @tc.number: IsSegmentFinish_001
 * @tc.desc  : Test IsSegmentFinish (mediaSegment_ != nullptr && mediaSegment_->isEos_) == true
 */
HWTEST_F(DashSegmentDownloaderUnitTest, IsSegmentFinish_001, TestSize.Level1)
{
    std::shared_ptr<DashBufferSegment> mediaSegment = std::make_shared<DashBufferSegment>();
    mediaSegment->isEos_ = true;
    segmentDownloader_->mediaSegment_ = mediaSegment;
    EXPECT_TRUE(segmentDownloader_->IsSegmentFinish());
}

/**
 * @tc.name  : Test CleanAllSegmentBuffer
 * @tc.number: CleanAllSegmentBuffer_001
 * @tc.desc  : Test CleanAllSegmentBuffer buffer_->GetHead() > it->bufferPosTail_
 */
HWTEST_F(DashSegmentDownloaderUnitTest, CleanAllSegmentBuffer_001, TestSize.Level1)
{
    bool isCleanAll = true;
    int64_t remainLastNumberSeq = 1;
    segmentDownloader_->buffer_->head_ = 1;
    std::shared_ptr<DashBufferSegment> segment = std::make_shared<DashBufferSegment>();
    segment->bufferPosTail_ = 0;
    segmentDownloader_->segmentList_.emplace_back(segment);
    segmentDownloader_->CleanAllSegmentBuffer(isCleanAll, remainLastNumberSeq);
    EXPECT_EQ(remainLastNumberSeq, 1);
}

/**
 * @tc.name  : Test CleanSegmentBuffer
 * @tc.number: CleanSegmentBuffer_001
 * @tc.desc  : Test CleanSegmentBuffer buffer_->GetHead() > it->bufferPosTail_ && it->bufferPosTail_ > 0
 */
HWTEST_F(DashSegmentDownloaderUnitTest, CleanSegmentBuffer_001, TestSize.Level1)
{
    bool isCleanAll = false;
    int64_t remainLastNumberSeq = 0;
    segmentDownloader_->buffer_->head_ = 10;
    std::shared_ptr<DashBufferSegment> segment = std::make_shared<DashBufferSegment>();
    segment->bufferPosTail_ = 1;
    segment->numberSeq_ = 1;
    segmentDownloader_->segmentList_.emplace_back(segment);
    segmentDownloader_->CleanSegmentBuffer(isCleanAll, remainLastNumberSeq);
    EXPECT_EQ(remainLastNumberSeq, -1);
}

/**
 * @tc.name  : Test CleanByTimeInternal
 * @tc.number: CleanByTimeInternal_001
 * @tc.desc  : Test CleanByTimeInternal buffer_->GetHead() > it->bufferPosTail_
 *             Test CleanByTimeInternal it->bufferPosHead_ >= it->bufferPosTail_
 */
HWTEST_F(DashSegmentDownloaderUnitTest, CleanByTimeInternal_001, TestSize.Level1)
{
    // Test CleanByTimeInternal buffer_->GetHead() > it->bufferPosTail_
    segmentDownloader_->buffer_->head_ = 1;
    auto segment1 = std::make_shared<DashBufferSegment>();
    segment1->numberSeq_ = 1;
    segmentDownloader_->segmentList_.emplace_back(segment1);

    // Test CleanByTimeInternal it->bufferPosHead_ >= it->bufferPosTail_
    std::shared_ptr<DashBufferSegment> segment2 = std::make_shared<DashBufferSegment>();
    segment2->bufferPosHead_ = 1;
    segment2->numberSeq_ = 1;
    segmentDownloader_->segmentList_.emplace_back(segment2);

    int64_t remainLastNumberSeq = 0;
    size_t clearTail = 0;
    bool isEnd = false;
    segmentDownloader_->CleanByTimeInternal(remainLastNumberSeq, clearTail, isEnd);
    EXPECT_EQ(remainLastNumberSeq, 0);
}

/**
 * @tc.name  : Test CleanByTimeInternal
 * @tc.number: CleanByTimeInternal_002
 * @tc.desc  : Test CleanByTimeInternal it->contentLength_ == 0
 *             Test CleanByTimeInternal it->duration_ == 0
 */
HWTEST_F(DashSegmentDownloaderUnitTest, CleanByTimeInternal_002, TestSize.Level1)
{
    int64_t remainLastNumberSeq = 0;
    size_t clearTail = 0;
    bool isEnd = false;

    // Test CleanByTimeInternal it->contentLength_ == 0
    segmentDownloader_->buffer_->head_ = 0;
    std::shared_ptr<DashBufferSegment> segment1 = std::make_shared<DashBufferSegment>();
    segment1->bufferPosTail_ = 1;
    segment1->numberSeq_ = 1;
    segment1->duration_ = 1;
    segmentDownloader_->segmentList_.emplace_back(segment1);
    segmentDownloader_->CleanByTimeInternal(remainLastNumberSeq, clearTail, isEnd);
    EXPECT_EQ(clearTail, segment1->bufferPosTail_);

    // Test CleanByTimeInternal it->duration_ == 0
    std::shared_ptr<DashBufferSegment> segment2 = std::make_shared<DashBufferSegment>();
    segment2->bufferPosTail_ = 2;
    segment2->numberSeq_ = 1;
    segment2->contentLength_ = 1;
    segmentDownloader_->segmentList_.clear();
    segmentDownloader_->segmentList_.emplace_back(segment2);
    segmentDownloader_->CleanByTimeInternal(remainLastNumberSeq, clearTail, isEnd);
    EXPECT_EQ(clearTail, segment2->bufferPosTail_);
}

/**
 * @tc.name  : Test CleanByTimeInternal
 * @tc.number: CleanByTimeInternal_003
 * @tc.desc  : Test remainDuration < segmentKeepDuration + segmentKeepDelta &&
               remainDuration + segmentKeepDelta >= segmentKeepDuration
 */
HWTEST_F(DashSegmentDownloaderUnitTest, CleanByTimeInternal_003, TestSize.Level1)
{
    int64_t remainLastNumberSeq = 0;
    size_t clearTail = 0;
    bool isEnd = false;
    segmentDownloader_->buffer_->head_ = 0;
    std::shared_ptr<DashBufferSegment> segment = std::make_shared<DashBufferSegment>();
    segment->bufferPosTail_ = 1000;
    segment->numberSeq_ = 1;
    segment->contentLength_ = 1;
    segment->duration_ = 1;
    segmentDownloader_->segmentList_.emplace_back(segment);
    segmentDownloader_->CleanByTimeInternal(remainLastNumberSeq, clearTail, isEnd);
    EXPECT_EQ(clearTail, segment->bufferPosTail_);
}

/**
 * @tc.name  : Test CleanByTimeInternal
 * @tc.number: CleanByTimeInternal_004
 * @tc.desc  : Test CleanByTimeInternal remainDuration < segmentKeepDuration
 */
HWTEST_F(DashSegmentDownloaderUnitTest, CleanByTimeInternal_004, TestSize.Level1)
{
    int64_t remainLastNumberSeq = 0;
    size_t clearTail = 0;
    bool isEnd = false;
    segmentDownloader_->buffer_->head_ = 0;
    std::shared_ptr<DashBufferSegment> segment = std::make_shared<DashBufferSegment>();
    segment->bufferPosTail_ = 800;
    segment->numberSeq_ = 1;
    segment->contentLength_ = 1;
    segment->duration_ = 1;
    segmentDownloader_->segmentList_.emplace_back(segment);
    segmentDownloader_->CleanByTimeInternal(remainLastNumberSeq, clearTail, isEnd);
    EXPECT_EQ(clearTail, segment->bufferPosTail_);
}

/**
 * @tc.name  : Test CleanByTimeInternal
 * @tc.number: CleanByTimeInternal_005
 * @tc.desc  : Test CleanByTimeInternal clearTail > 0
 */
HWTEST_F(DashSegmentDownloaderUnitTest, CleanByTimeInternal_005, TestSize.Level1)
{
    int64_t remainLastNumberSeq = 0;
    size_t clearTail = 1;
    bool isEnd = false;
    segmentDownloader_->buffer_->head_ = 0;
    std::shared_ptr<DashBufferSegment> segment = std::make_shared<DashBufferSegment>();
    segment->bufferPosTail_ = 1100;
    segment->numberSeq_ = 1;
    segment->contentLength_ = 1;
    segment->duration_ = 1;
    segmentDownloader_->segmentList_.emplace_back(segment);
    segmentDownloader_->CleanByTimeInternal(remainLastNumberSeq, clearTail, isEnd);
    EXPECT_EQ(clearTail, 1001);
}

/**
 * @tc.name  : Test SaveData
 * @tc.number: SaveData_001
 * @tc.desc  : Test SaveData freeSize == 0
 *             Test SaveData freeSize != 0
 */
HWTEST_F(DashSegmentDownloaderUnitTest, SaveData_001, TestSize.Level1)
{
    // Test SaveData freeSize == 0
    segmentDownloader_->buffer_->tail_ = AUD_RING_BUFFER_SIZE;
    segmentDownloader_->buffer_->head_ = 0;
    uint8_t data[2 * 1024 * 1024] = {0};
    uint32_t len = AUD_RING_BUFFER_SIZE;
    bool notBlock = true;
    uint32_t dataSize = segmentDownloader_->SaveData(data, len, notBlock);
    EXPECT_EQ(dataSize, 0);

    // Test SaveData freeSize != 0
    segmentDownloader_->buffer_->tail_ = 0;
    dataSize = segmentDownloader_->SaveData(data, len, notBlock);
    EXPECT_EQ(dataSize, AUD_RING_BUFFER_SIZE);
}

/**
 * @tc.name  : Test SaveData
 * @tc.number: SaveData_002
 * @tc.desc  : Test SaveData (notBlock && len < freeSize) == true
 *             Test SaveData (notBlock && len < freeSize) == false
 */
HWTEST_F(DashSegmentDownloaderUnitTest, SaveData_002, TestSize.Level1)
{
    // Test SaveData (notBlock && len < freeSize) == true
    segmentDownloader_->buffer_->tail_ = 0;
    segmentDownloader_->buffer_->head_ = 0;
    uint8_t data[2 * 1024 * 1024] = {0};
    uint32_t len = 0;
    bool notBlock = true;
    segmentDownloader_->SaveData(data, len, notBlock);
    EXPECT_TRUE(segmentDownloader_->canWrite_);

    // Test SaveData (notBlock && len < freeSize) == false
    notBlock = false;
    segmentDownloader_->canWrite_ = false;
    segmentDownloader_->SaveData(data, len, notBlock);
    EXPECT_TRUE(!segmentDownloader_->canWrite_);

    len = AUD_RING_BUFFER_SIZE;
    segmentDownloader_->SaveData(data, len, notBlock);
    EXPECT_TRUE(!segmentDownloader_->canWrite_);

    notBlock = true;
    segmentDownloader_->SaveData(data, len, notBlock);
    EXPECT_TRUE(!segmentDownloader_->canWrite_);
}

/**
 * @tc.name  : Test UpdateBufferSegment
 * @tc.number: UpdateBufferSegment_001
 * @tc.desc  : Test UpdateBufferSegment mediaSegment->contentLength_ == 0
 */
HWTEST_F(DashSegmentDownloaderUnitTest, UpdateBufferSegment_001, TestSize.Level1)
{
    std::shared_ptr<DashBufferSegment> mediaSegment = std::make_shared<DashBufferSegment>();
    mediaSegment->bufferPosTail_ = 1024;
    uint32_t len = 0;
    segmentDownloader_->UpdateBufferSegment(mediaSegment, len);
    EXPECT_EQ(mediaSegment->contentLength_, 1024);
}

/**
 * @tc.name  : Test OnWriteRingBuffer
 * @tc.number: OnWriteRingBuffer_001
 * @tc.desc  : Test OnWriteRingBuffer now > lastCheckTime_ && now - lastCheckTime_ > RECORD_TIME_INTERVAL
 *             Test OnWriteRingBuffer curDownloadBits < RECORD_DOWNLOAD_MIN_BIT
 */
HWTEST_F(DashSegmentDownloaderUnitTest, OnWriteRingBuffer_001, TestSize.Level1)
{
    segmentDownloader_->steadyClock_.Reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(RECORD_TIME_INTERVAL + 100));
    uint32_t len = 1;
    segmentDownloader_->OnWriteRingBuffer(len);
    EXPECT_EQ(segmentDownloader_->lastBits_, BYTE_TO_BIT);
}

/**
 * @tc.name  : Test OnWriteRingBuffer
 * @tc.number: OnWriteRingBuffer_002
 * @tc.desc  : Test OnWriteRingBuffer curDownloadBits >= RECORD_DOWNLOAD_MIN_BIT
 *             Test OnWriteRingBuffer recordData_ == nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, OnWriteRingBuffer_002, TestSize.Level1)
{
    segmentDownloader_->steadyClock_.Reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(RECORD_TIME_INTERVAL + 100));
    segmentDownloader_->buffer_.reset();
    uint32_t len = 1000;
    segmentDownloader_->OnWriteRingBuffer(len);
    EXPECT_EQ(segmentDownloader_->downloadBits_, len * BYTE_TO_BIT);
}

/**
 * @tc.name  : Test OnWriteRingBuffer
 * @tc.number: OnWriteRingBuffer_003
 * @tc.desc  : Test OnWriteRingBuffer recordData_ != nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, OnWriteRingBuffer_003, TestSize.Level1)
{
    segmentDownloader_->steadyClock_.Reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(RECORD_TIME_INTERVAL + 100));
    segmentDownloader_->realTimeBitBate_ = 8;
    segmentDownloader_->recordData_ = std::make_shared<DashSegmentDownloader::RecordData>();
    segmentDownloader_->buffer_->tail_ = 1;
    uint32_t len = 1000;
    segmentDownloader_->OnWriteRingBuffer(len);
    EXPECT_EQ(segmentDownloader_->recordData_->bufferDuring, 1);
}

/**
 * @tc.name  : Test GetIp
 * @tc.number: GetIp_001
 * @tc.desc  : Test GetIp downloader_ = nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetIp_001, TestSize.Level1)
{
    segmentDownloader_->downloader_.reset();
    std::string ip = "";
    segmentDownloader_->GetIp(ip);
    EXPECT_EQ(ip, "");
}

/**
 * @tc.name  : Test GetDownloadFinishState
 * @tc.number: GetDownloadFinishState_001
 * @tc.desc  : Test GetDownloadFinishState finishState == false
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetDownloadFinishState_001, TestSize.Level1)
{
    segmentDownloader_->initSegments_.clear();
    bool retResult = segmentDownloader_->GetDownloadFinishState();
    EXPECT_TRUE(retResult);
}

/**
 * @tc.name  : Test GetDownloadRecordData
 * @tc.number: GetDownloadRecordData_001
 * @tc.desc  : Test GetDownloadRecordData recordData_ == nullptr
 *             Test GetDownloadRecordData recordData_ != nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetDownloadRecordData_001, TestSize.Level1)
{
    // Test GetDownloadRecordData recordData_ == nullptr
    segmentDownloader_->recordData_ = nullptr;
    std::pair<int64_t, int64_t> pair = segmentDownloader_->GetDownloadRecordData();
    EXPECT_EQ(pair.first, 0);

    // Test GetDownloadRecordData recordData_ != nullptr
    auto recordData = std::make_shared<DashSegmentDownloader::RecordData>();
    recordData->downloadRate = 8;
    segmentDownloader_->recordData_ = recordData;
    pair = segmentDownloader_->GetDownloadRecordData();
    EXPECT_EQ(pair.first, 8);
}

/**
 * @tc.name  : Test GetBufferSize
 * @tc.number: GetBufferSize_001
 * @tc.desc  : Test GetBufferSize buffer_ == nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetBufferSize_001, TestSize.Level1)
{
    segmentDownloader_->buffer_.reset();
    uint64_t size = segmentDownloader_->GetBufferSize();
    EXPECT_EQ(size, 0);
}

/**
 * @tc.name  : Test PutRequestIntoDownloader
 * @tc.number: PutRequestIntoDownloader_001
 * @tc.desc  : Test PutRequestIntoDownloader downloader_ == nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, PutRequestIntoDownloader_001, TestSize.Level1)
{
    unsigned int duration = 10;
    int64_t startPos = 0;
    int64_t endPos = 0;
    std::string url = "";
    segmentDownloader_->downloader_.reset();
    segmentDownloader_->PutRequestIntoDownloader(duration, startPos, endPos, url);
    EXPECT_TRUE(!segmentDownloader_->isCleaningBuffer_);
}

/**
 * @tc.name  : Test UpdateDownloadFinished
 * @tc.number: UpdateDownloadFinished_001
 * @tc.desc  : Test UpdateDownloadFinished totalDownloadDuringTime_ > 0
 *             Test UpdateDownloadFinished mediaSegment_ == nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, UpdateDownloadFinished_001, TestSize.Level1)
{
    // Test totalDownloadDuringTime_ > 0
    segmentDownloader_->totalDownloadDuringTime_ = 1;
    segmentDownloader_->downloadBits_ = 1;

    // Test mediaSegment_ == nullptr
    segmentDownloader_->mediaSegment_.reset();

    segmentDownloader_->UpdateDownloadFinished("", "");
    EXPECT_EQ(segmentDownloader_->downloadSpeed_, SPEED_MULTI_FACT);
}

/**
 * @tc.name  : Test UpdateDownloadFinished
 * @tc.number: UpdateDownloadFinished_002
 * @tc.desc  : Test UpdateDownloadFinished mediaSegment_->contentLength_ == 0 && downloadRequest_ != nullptr
 */
HWTEST_F(DashSegmentDownloaderUnitTest, UpdateDownloadFinished_002, TestSize.Level1)
{
    segmentDownloader_->mediaSegment_ = std::make_shared<DashBufferSegment>();
    segmentDownloader_->downloadRequest_ = std::make_shared<DownloadRequest>("test_request", nullptr, nullptr);
    segmentDownloader_->downloadRequest_->headerInfo_.isChunked = true;
    segmentDownloader_->downloadRequest_->headerInfo_.fileContentLen = 1;
    segmentDownloader_->downloadRequest_->isHeaderUpdated_ = true;

    segmentDownloader_->UpdateDownloadFinished("", "");
    EXPECT_EQ(segmentDownloader_->mediaSegment_->contentLength_, 1);
}

// 0627
/**
 * @tc.name  : Test GetSegmentRemainDuration
 * @tc.number: GetSegmentRemainDuration_001
 * @tc.desc  : Test GetSegmentRemainDuration
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetSegmentRemainDuration_001, TestSize.Level1)
{
    // Test SetAppUid downloader_ != nullptr
    int32_t appUid = 1;
    segmentDownloader_->SetAppUid(appUid);

    // Test SetAppUid downloader_ == nullptr
    segmentDownloader_->downloader_.reset();
    segmentDownloader_->SetAppUid(appUid);

    // Test SetInterruptState isInterruptNeeded == false
    segmentDownloader_->SetInterruptState(false);

    // Test GetSegmentRemainDuration buffer_->GetHead() > currentSegment->bufferPosHead_
    segmentDownloader_->buffer_->head_ = 1;
    std::shared_ptr<DashBufferSegment> currentSegment = std::make_shared<DashBufferSegment>();
    currentSegment->bufferPosTail_ = 2;
    currentSegment->duration_ = 2;
    currentSegment->bufferPosHead_ = 0;
    uint32_t duration = segmentDownloader_->GetSegmentRemainDuration(currentSegment);
    EXPECT_EQ(duration, 1);
}

/**
 * @tc.name  : Test IsNeedBufferForPlaying
 * @tc.number: IsNeedBufferForPlaying_001
 * @tc.desc  : Test IsNeedBufferForPlaying GetBufferingTimeOut() && callback_
 */
HWTEST_F(DashSegmentDownloaderUnitTest, IsNeedBufferForPlaying_001, TestSize.Level1)
{
    segmentDownloader_->bufferDurationForPlaying_ = 1;
    segmentDownloader_->isDemuxerInitSuccess_.store(true);
    segmentDownloader_->isBuffering_.store(true);
    segmentDownloader_->IsNeedBufferForPlaying();

    MockCallback *mockCallback = new MockCallback();
    EXPECT_CALL(*mockCallback, OnEvent(testing::_)).Times(TIMES_ONE);
    segmentDownloader_->callback_ = mockCallback;
    segmentDownloader_->isDemuxerInitSuccess_.store(true);
    segmentDownloader_->isBuffering_.store(true);
    segmentDownloader_->IsNeedBufferForPlaying();

    segmentDownloader_->bufferingTime_ = 1;
    segmentDownloader_->steadyClock_.Reset();
    segmentDownloader_->steadyClock_.begin_ -= std::chrono::milliseconds(MAX_BUFFERING_TIME_OUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(RECORD_TIME_INTERVAL));
    segmentDownloader_->isDemuxerInitSuccess_.store(true);
    segmentDownloader_->isBuffering_.store(true);
    segmentDownloader_->IsNeedBufferForPlaying();

    delete mockCallback;
    segmentDownloader_->callback_ = nullptr;
    segmentDownloader_->isDemuxerInitSuccess_.store(true);
    segmentDownloader_->isBuffering_.store(true);
    segmentDownloader_->IsNeedBufferForPlaying();
}

/**
 * @tc.name  : Test IsNeedBufferForPlaying
 * @tc.number: IsNeedBufferForPlaying_002
 * @tc.desc  : Test IsNeedBufferForPlaying GetBufferSize() >= waterlineForPlaying_ || isAllSegmentFinished_.load()
 */
HWTEST_F(DashSegmentDownloaderUnitTest, IsNeedBufferForPlaying_002, TestSize.Level1)
{
    segmentDownloader_->bufferDurationForPlaying_ = 1;
    segmentDownloader_->isDemuxerInitSuccess_.store(true);
    segmentDownloader_->isBuffering_.store(true);
    bool retResult = segmentDownloader_->IsNeedBufferForPlaying();
    EXPECT_EQ(retResult, false);

    segmentDownloader_->isDemuxerInitSuccess_.store(true);
    segmentDownloader_->isBuffering_.store(true);
    segmentDownloader_->isAllSegmentFinished_.store(true);
    retResult = segmentDownloader_->IsNeedBufferForPlaying();
    EXPECT_EQ(retResult, false);

    segmentDownloader_->isDemuxerInitSuccess_.store(true);
    segmentDownloader_->isBuffering_.store(true);
    segmentDownloader_->waterlineForPlaying_ = 1;
    retResult = segmentDownloader_->IsNeedBufferForPlaying();
    EXPECT_EQ(retResult, false);

    segmentDownloader_->isDemuxerInitSuccess_.store(true);
    segmentDownloader_->isBuffering_.store(true);
    segmentDownloader_->waterlineForPlaying_ = 1;
    segmentDownloader_->isAllSegmentFinished_.store(false);
    retResult = segmentDownloader_->IsNeedBufferForPlaying();
    EXPECT_EQ(retResult, true);
}

/**
 * @tc.name  : Test GetBufferingTimeOut
 * @tc.number: GetBufferingTimeOut_001
 * @tc.desc  : Test GetBufferingTimeOut bufferingTime_ == 0
 *             Test NotifyInitSuccess streamType_ != MediaAVCodec::MediaType::MEDIA_TYPE_VID
 */
HWTEST_F(DashSegmentDownloaderUnitTest, GetBufferingTimeOut_001, TestSize.Level1)
{
    // Test NotifyInitSuccess streamType_ != MediaAVCodec::MediaType::MEDIA_TYPE_VID
    segmentDownloader_->streamType_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    segmentDownloader_->NotifyInitSuccess();

    // Test GetBufferingTimeOut bufferingTime_ == 0
    segmentDownloader_->bufferingTime_ = 0;
    bool retResult = segmentDownloader_->GetBufferingTimeOut();
    EXPECT_EQ(retResult, false);
}
} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS