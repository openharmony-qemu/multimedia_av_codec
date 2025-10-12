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
#include "dash_media_downloader_unittest.h"
#include "sidx_box_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing;
using namespace testing::ext;
const static int32_t ID_TEST = 1;
const static int32_t NUM_TEST = 0;
const static int32_t NUM_10 = 10;
const static int32_t NUM_20 = 20;
const static int32_t NUM_25 = 25;
const static int32_t NUM_80 = 80;
const static int32_t NUM_SIZE = 10485760;
void DashMediaDownloaderUnittest::SetUpTestCase(void) {}
void DashMediaDownloaderUnittest::TearDownTestCase(void) {}
void DashMediaDownloaderUnittest::SetUp(void)
{
    mediaDownloader_ = std::make_shared<DashMediaDownloader>();
    mediaDownloader_->Init();
}
void DashMediaDownloaderUnittest::TearDown(void)
{
    std::cout << "Tear Down 1" << std::endl;
    mediaDownloader_ = nullptr;
    std::cout << "Tear Down 2" << std::endl;
}

/**
 * @tc.name  : Test DashMediaDownloader
 * @tc.number: DashMediaDownloader_001
 * @tc.desc  : Test sourceLoader != nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, DashMediaDownloader_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader =
        std::make_shared<MediaSourceLoaderCombinations>(nullptr);
    auto mediaDownloader = std::make_shared<DashMediaDownloader>(sourceLoader);
    mediaDownloader->Init();
    EXPECT_NE(mediaDownloader->sourceLoader_, nullptr);
    mediaDownloader = nullptr;
}

/**
 * @tc.name  : Test GetContentType
 * @tc.number: GetContentType_001
 * @tc.desc  : Test all
 */
HWTEST_F(DashMediaDownloaderUnittest, GetContentType_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto ret = mediaDownloader_->GetContentType();
    EXPECT_EQ(ret, "");
}

/**
 * @tc.name  : Test Read
 * @tc.number: Read_001
 * @tc.desc  : Test segmentDownloaders_.empty()
 */
HWTEST_F(DashMediaDownloaderUnittest, Read_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    unsigned char buff = 'a';
    ReadDataInfo readDataInfo;
    mediaDownloader_->segmentDownloaders_.clear();
    auto ret = mediaDownloader_->Read(&buff, readDataInfo);
    EXPECT_EQ(ret, Status::END_OF_STREAM);
}

/**
 * @tc.name  : Test Read
 * @tc.number: Read_002
 * @tc.desc  : Test callback_ != nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, Read_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    unsigned char buff = 'a';
    ReadDataInfo readDataInfo;
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.push_back(segmentDownloader);
    EXPECT_FALSE(mediaDownloader_->segmentDownloaders_.empty());
    mediaDownloader_->downloadErrorState_ = true;
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    auto ret = mediaDownloader_->Read(&buff, readDataInfo);
    EXPECT_EQ(ret, Status::ERROR_AGAIN);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_001
 * @tc.desc  : Test callback_ == nullptr || mpdDownloader_ == nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, PostBufferingEvent_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->callback_ = nullptr;
    mediaDownloader_->mpdDownloader_ = nullptr;
    mediaDownloader_->PostBufferingEvent(NUM_TEST, BufferingInfoType::BUFFERING_START);
    EXPECT_TRUE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);

    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    mediaDownloader_->PostBufferingEvent(NUM_TEST, BufferingInfoType::BUFFERING_START);
    EXPECT_TRUE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);

    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    mediaDownloader_->mpdDownloader_ = nullptr;
    mediaDownloader_->PostBufferingEvent(NUM_TEST, BufferingInfoType::BUFFERING_START);
    EXPECT_TRUE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_002
 * @tc.desc  : Test !(callback_ == nullptr || mpdDownloader_ == nullptr)
 *             Test streamDesc == nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, PostBufferingEvent_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);

    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(NUM_TEST);
    EXPECT_EQ(streamDesc, nullptr);
    mediaDownloader_->PostBufferingEvent(NUM_TEST, BufferingInfoType::BUFFERING_START);
    EXPECT_FALSE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_003
 * @tc.desc  : streamDesc != nullptr && streamDesc->type_ != MEDIA_TYPE_SUBTITLE
 *             Test type != BufferingInfoType::BUFFERING_PERCENT
 *             Test type == BufferingInfoType::BUFFERING_START
 *             Test bufferingFlag_ == 0
 */
HWTEST_F(DashMediaDownloaderUnittest, PostBufferingEvent_003, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    EXPECT_NE(streamDesc, nullptr);
    mediaDownloader_->bufferingFlag_ = 0;
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_START);
    EXPECT_FALSE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_004
 * @tc.desc  : Test bufferingFlag_ == 1
 */
HWTEST_F(DashMediaDownloaderUnittest, PostBufferingEvent_004, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    EXPECT_NE(streamDesc, nullptr);
    mediaDownloader_->bufferingFlag_ = 1;
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_START);
    EXPECT_FALSE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_005
 * @tc.desc  : Test type == BufferingInfoType::BUFFERING_END
 *             Test (bufferingFlag_ & flag) == 0
 *             Test lastBufferingFlag == 0
 */
HWTEST_F(DashMediaDownloaderUnittest, PostBufferingEvent_005, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    EXPECT_NE(streamDesc, nullptr);
    mediaDownloader_->bufferingFlag_ = 0;
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_END);
    EXPECT_FALSE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_006
 * @tc.desc  : Test type == BufferingInfoType::BUFFERING_END
 *             Test (bufferingFlag_ & flag) == 1
 *             Test lastBufferingFlag > 0 && bufferingFlag_ > 0
 */
HWTEST_F(DashMediaDownloaderUnittest, PostBufferingEvent_006, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    EXPECT_NE(streamDesc, nullptr);
    mediaDownloader_->bufferingFlag_ = 1;
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_END);
    EXPECT_FALSE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_007
 * @tc.desc  : Test type == BufferingInfoType::CACHED_DURATION
 */
HWTEST_F(DashMediaDownloaderUnittest, PostBufferingEvent_007, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    EXPECT_NE(streamDesc, nullptr);
    mediaDownloader_->bufferingFlag_ = 1;
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::CACHED_DURATION);
    EXPECT_FALSE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_008
 * @tc.desc  : streamDesc->type_ == MEDIA_TYPE_SUBTITLE
 */
HWTEST_F(DashMediaDownloaderUnittest, PostBufferingEvent_008, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE;
    EXPECT_NE(streamDesc, nullptr);
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_START);
    EXPECT_FALSE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_009
 * @tc.desc  : Test type == BufferingInfoType::BUFFERING_PERCENT
 *             Test downloader == nullptr
 *             Test downloader->GetStreamType() == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE
 *             Test downloader->GetStreamType() != MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE
 */
HWTEST_F(DashMediaDownloaderUnittest, PostBufferingEvent_009, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    EXPECT_NE(streamDesc, nullptr);
    std::shared_ptr<DashSegmentDownloader> segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.clear();
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_PERCENT);
    EXPECT_FALSE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
    EXPECT_EQ(mediaDownloader_->segmentDownloaders_.size(), 0);

    mediaDownloader_->segmentDownloaders_.push_back(segmentDownloader);
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_PERCENT);
    EXPECT_FALSE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
    EXPECT_EQ(mediaDownloader_->segmentDownloaders_.size(), 1);

    segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.clear();
    mediaDownloader_->segmentDownloaders_.push_back(segmentDownloader);
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_PERCENT);
    EXPECT_FALSE(mediaDownloader_->callback_ == nullptr || mediaDownloader_->mpdDownloader_ == nullptr);
    EXPECT_EQ(mediaDownloader_->segmentDownloaders_.size(), 1);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_010
 * @tc.desc  : Test percent > 0 && percent < 100 && last != percent
 */
HWTEST_F(DashMediaDownloaderUnittest, BufferConditionCase_010, TestSize.Level0) {
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    EXPECT_NE(mediaDownloader_->callback_, nullptr);
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    EXPECT_EQ(streamDesc->type_, MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    EXPECT_NE(streamDesc, nullptr);

    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    downloader->waterLineAbove_ = NUM_25;
    downloader->buffer_ = std::make_shared<RingBuffer>(NUM_10);
    downloader->buffer_->tail_ = NUM_20;
    downloader->buffer_->head_ = 0;
    mediaDownloader_->segmentDownloaders_.clear();
    mediaDownloader_->segmentDownloaders_.push_back(downloader);
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_PERCENT);
    EXPECT_EQ(callback->eventTriggered, true);
    EXPECT_EQ(mediaDownloader_->lastBufferingPercent_, NUM_80);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: PostBufferingEvent_011
 * @tc.desc  : Test percent > 0 && percent < 100 && last == percent
 */
HWTEST_F(DashMediaDownloaderUnittest, BufferConditionCase_011, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    EXPECT_NE(mediaDownloader_->callback_, nullptr);
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    EXPECT_NE(streamDesc, nullptr);
    mediaDownloader_->lastBufferingPercent_ = 50;
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    downloader->waterLineAbove_ = NUM_20;
    downloader->buffer_ = std::make_shared<RingBuffer>(NUM_10);
    downloader->buffer_->tail_ = NUM_10;
    downloader->buffer_->head_ = 0;
    
    mediaDownloader_->segmentDownloaders_.push_back(downloader);
    mediaDownloader_->PostBufferingEvent(ID_TEST, BufferingInfoType::BUFFERING_PERCENT);

    EXPECT_EQ(callback->eventTriggered, false);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: BufferConditionCase_012
 * @tc.desc  : Test percent > 0 && percent == 100
 */
HWTEST_F(DashMediaDownloaderUnittest, BufferConditionCase_012, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    EXPECT_NE(mediaDownloader_->callback_, nullptr);
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    EXPECT_NE(streamDesc, nullptr);
    mediaDownloader_->lastBufferingPercent_ = 50;
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    downloader->waterLineAbove_ = 5;
    downloader->buffer_ = std::make_shared<RingBuffer>(5);
    downloader->buffer_->tail_ = 5;
    downloader->buffer_->head_ = 0;
    
    mediaDownloader_->segmentDownloaders_.push_back(downloader);
    mediaDownloader_->PostBufferingEvent(ID_TEST, BufferingInfoType::BUFFERING_PERCENT);

    EXPECT_EQ(callback->eventTriggered, false);
}

/**
 * @tc.name  : Test PostBufferingEvent
 * @tc.number: BufferConditionCase_013
 * @tc.desc  : Test percent == 0
 */
HWTEST_F(DashMediaDownloaderUnittest, BufferConditionCase_013, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    int streamId = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback.get();
    EXPECT_NE(mediaDownloader_->callback_, nullptr);
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    EXPECT_NE(streamDesc, nullptr);
    mediaDownloader_->lastBufferingPercent_ = NUM_20;
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    downloader->waterLineAbove_ = 0;
    downloader->buffer_ = std::make_shared<RingBuffer>(NUM_10);
    
    mediaDownloader_->segmentDownloaders_.push_back(downloader);
    mediaDownloader_->PostBufferingEvent(ID_TEST, BufferingInfoType::BUFFERING_PERCENT);
    
    EXPECT_EQ(callback->eventTriggered, false);
}

/**
 * @tc.name  : Test AutoSelectBitrateInternal
 * @tc.number: AutoSelectBitrateInternal_001
 * @tc.desc  : Test mpdDownloader_ == nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, AutoSelectBitrateInternal_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    uint32_t bitrate = ID_TEST;
    mediaDownloader_->mpdDownloader_ = nullptr;
    
    auto ret = mediaDownloader_->AutoSelectBitrateInternal(bitrate);
    
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AutoSelectBitrateInternal
 * @tc.number: AutoSelectBitrateInternal_002
 * @tc.desc  : Test mpdDownloader_ != nullptr
 *             Test ret == DASH_MPD_GET_ERROR
 */
HWTEST_F(DashMediaDownloaderUnittest, AutoSelectBitrateInternal_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    uint32_t bitrate = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    mediaDownloader_->mpdDownloader_->streamDescriptions_ = {};
    auto ret = mediaDownloader_->AutoSelectBitrateInternal(bitrate);

    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AutoSelectBitrateInternal
 * @tc.number: AutoSelectBitrateInternal_003
 * @tc.desc  : Test ret != DASH_MPD_GET_ERROR
 *             Test ret != DASH_MPD_GET_UNDONE
 */
HWTEST_F(DashMediaDownloaderUnittest, AutoSelectBitrateInternal_003, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    uint32_t bitrate = ID_TEST;
    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    ptr->inUse_ = true;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    auto ret = mediaDownloader_->AutoSelectBitrateInternal(bitrate);
    EXPECT_EQ(mediaDownloader_->bitrateParam_.waitSidxFinish_, false);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test GetPlayable
 * @tc.number: GetPlayable_001
 * @tc.desc  : Test vidSegmentDownloader == nullptr && audSegmentDownloader == nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, GetPlayable_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.clear();
    auto ret = mediaDownloader_->GetPlayable();
    EXPECT_EQ(mediaDownloader_->GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_VID), nullptr);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test GetPlayable
 * @tc.number: GetPlayable_002
 * @tc.desc  : Test vidSegmentDownloader != nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, GetPlayable_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.push_back(downloader);
    auto ret = mediaDownloader_->GetPlayable();
    EXPECT_NE(mediaDownloader_->GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_VID), nullptr);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test GetPlayable
 * @tc.number: GetPlayable_003
 * @tc.desc  : Test audSegmentDownloader != nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, GetPlayable_003, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.clear();
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_AUD, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.push_back(downloader);
    auto ret = mediaDownloader_->GetPlayable();
    EXPECT_NE(mediaDownloader_->GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_AUD), nullptr);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test GetPlayable
 * @tc.number: GetPlayable_004
 * @tc.desc  : Test vidSegmentDownloader != nullptr && audSegmentDownloader != nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, GetPlayable_004, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.clear();
    auto downloader1 = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_AUD, NUM_10, nullptr);
    auto downloader2 = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.push_back(downloader1);
    mediaDownloader_->segmentDownloaders_.push_back(downloader2);
    auto ret = mediaDownloader_->GetPlayable();
    EXPECT_NE(mediaDownloader_->GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_AUD), nullptr);
    EXPECT_NE(mediaDownloader_->GetSegmentDownloaderByType(MediaAVCodec::MediaType::MEDIA_TYPE_VID), nullptr);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test GetBufferingTimeOut
 * @tc.number: GetBufferingTimeOut_001
 * @tc.desc  : Test all
 */
HWTEST_F(DashMediaDownloaderUnittest, GetBufferingTimeOut_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto ret = mediaDownloader_->GetBufferingTimeOut();
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test GetBufferSize
 * @tc.number: GetBufferSize_001
 * @tc.desc  : Test segmentDownloader == nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, GetBufferSize_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.clear();
    auto ret = mediaDownloader_->GetBufferSize();
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name  : Test GetBufferSize
 * @tc.number: GetBufferSize_002
 * @tc.desc  : Test segmentDownloader != nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, GetBufferSize_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.clear();
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    downloader->buffer_ = std::make_shared<RingBuffer>(NUM_10);
    downloader->buffer_->tail_ = NUM_10;
    downloader->buffer_->head_ = 0;
    mediaDownloader_->segmentDownloaders_.push_back(downloader);
    auto ret = mediaDownloader_->GetBufferSize();
    EXPECT_EQ(ret, NUM_10);
}

/**
 * @tc.name  : Test GetMemorySize
 * @tc.number: GetMemorySize_001
 * @tc.desc  : Test segmentDownloaders_[i] == nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, GetMemorySize_001, TestSize.Level0)
{
    mediaDownloader_->segmentDownloaders_.clear();
    mediaDownloader_->segmentDownloaders_.push_back(nullptr);
    uint64_t memorySize = mediaDownloader_->GetMemorySize();
    EXPECT_EQ(memorySize, 0);
}

/**
 * @tc.name  : Test GetMemorySize
 * @tc.number: GetMemorySize_002
 * @tc.desc  : Test segmentDownloaders_[i] != nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, GetMemorySize_002, TestSize.Level0)
{
    mediaDownloader_->segmentDownloaders_.clear();
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.push_back(downloader);
    uint64_t memorySize = mediaDownloader_->GetMemorySize();
    EXPECT_EQ(memorySize, NUM_SIZE); // 10 * 1024 * 1024
}

/**
 * @tc.name  : Test GetPlaybackInfo
 * @tc.number: GetPlaybackInfo_001
 * @tc.desc  : Test segmentDownloaders_.empty()
 */
HWTEST_F(DashMediaDownloaderUnittest, GetPlaybackInfo_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.clear();
    EXPECT_TRUE(mediaDownloader_->segmentDownloaders_.empty());
    PlaybackInfo playbackInfo;
    mediaDownloader_->GetPlaybackInfo(playbackInfo);
    
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    downloader->downloadSpeed_ = 1000;
    mediaDownloader_->segmentDownloaders_.push_back(downloader);
    mediaDownloader_->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.averageDownloadRate, 1000);
}
}
}
}
}