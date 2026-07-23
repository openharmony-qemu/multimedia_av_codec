/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
 * @tc.desc  : Test segmentDownloaders_.Empty()
 */
HWTEST_F(DashMediaDownloaderUnittest, Read_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    unsigned char buff = 'a';
    ReadDataInfo readDataInfo;
    mediaDownloader_->segmentDownloaders_.Clear();
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
    mediaDownloader_->segmentDownloaders_.PushBack(segmentDownloader);
    EXPECT_FALSE(mediaDownloader_->segmentDownloaders_.Empty());
    mediaDownloader_->downloadErrorState_ = true;
    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback;
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
    mediaDownloader_->callback_.reset();
    mediaDownloader_->mpdDownloader_ = nullptr;
    mediaDownloader_->PostBufferingEvent(NUM_TEST, BufferingInfoType::BUFFERING_START);
    EXPECT_TRUE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);

    mediaDownloader_->mpdDownloader_ = std::make_shared<DashMpdDownloader>(nullptr);
    mediaDownloader_->PostBufferingEvent(NUM_TEST, BufferingInfoType::BUFFERING_START);
    EXPECT_TRUE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);

    auto callback = std::make_shared<TestCallback>();
    mediaDownloader_->callback_ = callback;
    mediaDownloader_->mpdDownloader_ = nullptr;
    mediaDownloader_->PostBufferingEvent(NUM_TEST, BufferingInfoType::BUFFERING_START);
    EXPECT_TRUE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
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
    mediaDownloader_->callback_ = callback;
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(NUM_TEST);
    EXPECT_EQ(streamDesc, nullptr);
    mediaDownloader_->PostBufferingEvent(NUM_TEST, BufferingInfoType::BUFFERING_START);
    EXPECT_FALSE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
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
    mediaDownloader_->callback_ = callback;
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
    EXPECT_FALSE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
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
    mediaDownloader_->callback_ = callback;
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
    EXPECT_FALSE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
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
    mediaDownloader_->callback_ = callback;
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
    EXPECT_FALSE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
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
    mediaDownloader_->callback_ = callback;
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
    EXPECT_FALSE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
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
    mediaDownloader_->callback_ = callback;
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
    EXPECT_FALSE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
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
    mediaDownloader_->callback_ = callback;
    auto ptr = std::make_shared<DashStreamDescription>();
    ptr->segsState_ = DashSegsState::DASH_SEGS_STATE_NONE;
    ptr->streamId_ = streamId;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(ptr);
    std::shared_ptr<DashStreamDescription> streamDesc =
        mediaDownloader_->mpdDownloader_->GetStreamByStreamId(streamId);
    streamDesc->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE;
    EXPECT_NE(streamDesc, nullptr);
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_START);
    EXPECT_FALSE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
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
    mediaDownloader_->callback_ = callback;
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
    mediaDownloader_->segmentDownloaders_.Clear();
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_PERCENT);
    EXPECT_FALSE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
    EXPECT_EQ(mediaDownloader_->segmentDownloaders_.Size(), 0);

    mediaDownloader_->segmentDownloaders_.PushBack(segmentDownloader);
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_PERCENT);
    EXPECT_FALSE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
    EXPECT_EQ(mediaDownloader_->segmentDownloaders_.Size(), 1);

    segmentDownloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.Clear();
    mediaDownloader_->segmentDownloaders_.PushBack(segmentDownloader);
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_PERCENT);
    EXPECT_FALSE(mediaDownloader_->callback_.expired() || mediaDownloader_->mpdDownloader_ == nullptr);
    EXPECT_EQ(mediaDownloader_->segmentDownloaders_.Size(), 1);
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
    mediaDownloader_->callback_ = callback;
    EXPECT_FALSE(mediaDownloader_->callback_.expired());
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
    mediaDownloader_->segmentDownloaders_.Clear();
    mediaDownloader_->segmentDownloaders_.PushBack(downloader);
    mediaDownloader_->PostBufferingEvent(streamId, BufferingInfoType::BUFFERING_PERCENT);
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
    mediaDownloader_->callback_ = callback;
    EXPECT_FALSE(mediaDownloader_->callback_.expired());
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
    
    mediaDownloader_->segmentDownloaders_.PushBack(downloader);
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
    mediaDownloader_->callback_ = callback;
    EXPECT_FALSE(mediaDownloader_->callback_.expired());
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
    
    mediaDownloader_->segmentDownloaders_.PushBack(downloader);
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
    mediaDownloader_->callback_ = callback;
    EXPECT_FALSE(mediaDownloader_->callback_.expired());
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
    
    mediaDownloader_->segmentDownloaders_.PushBack(downloader);
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
    mediaDownloader_->segmentDownloaders_.Clear();
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
    mediaDownloader_->segmentDownloaders_.PushBack(downloader);
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
    mediaDownloader_->segmentDownloaders_.Clear();
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_AUD, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.PushBack(downloader);
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
    mediaDownloader_->segmentDownloaders_.Clear();
    auto downloader1 = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_AUD, NUM_10, nullptr);
    auto downloader2 = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.PushBack(downloader1);
    mediaDownloader_->segmentDownloaders_.PushBack(downloader2);
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
    mediaDownloader_->segmentDownloaders_.Clear();
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
    mediaDownloader_->segmentDownloaders_.Clear();
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    downloader->buffer_ = std::make_shared<RingBuffer>(NUM_10);
    downloader->buffer_->tail_ = NUM_10;
    downloader->buffer_->head_ = 0;
    mediaDownloader_->segmentDownloaders_.PushBack(downloader);
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
    mediaDownloader_->segmentDownloaders_.Clear();
    mediaDownloader_->segmentDownloaders_.PushBack(nullptr);
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
    mediaDownloader_->segmentDownloaders_.Clear();
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    mediaDownloader_->segmentDownloaders_.PushBack(downloader);
    uint64_t memorySize = mediaDownloader_->GetMemorySize();
    EXPECT_EQ(memorySize, 4 * 1024 * 1024); // 4 * 1024 * 1024
}

/**
 * @tc.name  : Test GetPlaybackInfo
 * @tc.number: GetPlaybackInfo_001
 * @tc.desc  : Test segmentDownloaders_.Empty()
 */
HWTEST_F(DashMediaDownloaderUnittest, GetPlaybackInfo_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.Clear();
    EXPECT_TRUE(mediaDownloader_->segmentDownloaders_.Empty());
    PlaybackInfo playbackInfo;
    mediaDownloader_->GetPlaybackInfo(playbackInfo);
    
    auto downloader = std::make_shared<DashSegmentDownloader>(nullptr, 1,
        MediaAVCodec::MediaType::MEDIA_TYPE_VID, NUM_10, nullptr);
    downloader->downloadSpeed_ = 1000;
    mediaDownloader_->segmentDownloaders_.PushBack(downloader);
    mediaDownloader_->GetPlaybackInfo(playbackInfo);
    EXPECT_EQ(playbackInfo.averageDownloadRate, 1000);
}

/**
 * @tc.name  : Test StopBufferring
 * @tc.number: StopBufferring
 * @tc.desc  : Test segmentDownloaders_.Empty()
 */
HWTEST_F(DashMediaDownloaderUnittest, STOP_BUFFERING, TestSize.Level1)
{
    std::shared_ptr<DashMediaDownloader> mediaDownloader = std::make_shared<DashMediaDownloader>(nullptr);
    mediaDownloader->Init();
    EXPECT_EQ(mediaDownloader->StopBufferring(false), Status::OK);

    mediaDownloader->preparedAction_.seekPosition_ = 0;
    PlaybackInfo info;
    info.isDownloading = false;
    mediaDownloader->GetPlaybackInfo(info);
    mediaDownloader->SeekToTs(0);
    EXPECT_FALSE(info.isDownloading);
    
    std::shared_ptr<Callback> callback = nullptr;
    std::shared_ptr<DashSegmentDownloader> downloader = std::make_shared<DashSegmentDownloader>(
        callback, 0, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    downloader->Init();
    auto bufferingCallback = [] (int streamId, BufferingInfoType type) {
        (void)streamId;
        (void)type;
    };
    downloader->SetSegmentBufferingCallback(bufferingCallback);
    mediaDownloader->segmentDownloaders_.PushBack(downloader);
    mediaDownloader->segmentDownloaders_.PushBack(nullptr);
    mediaDownloader->NotifyInitSuccess();
    mediaDownloader->SetAppUid(0);
    mediaDownloader->GetPlaybackInfo(info);
    mediaDownloader->SetInterruptState(true);
    mediaDownloader->SeekInternal(0);

    DashMpdInfo mpdInfo;
    mediaDownloader->mpdDownloader_->mpdInfo_ = &mpdInfo;
    mediaDownloader->mpdDownloader_->notifyOpenOk_ = true;
    EXPECT_EQ(mediaDownloader->GetSeekable(), Seekable::INVALID);
    mediaDownloader->Pause();
    mediaDownloader->Resume();
    mediaDownloader->Close(false);
    
    EXPECT_EQ(mediaDownloader->StopBufferring(false), Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name  : Test SwitchVideoStream
 * @tc.number: SwitchVideoStream_001
 * @tc.desc  : Test SwitchVideoStream with DASH_MPD_GET_UNDONE
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchVideoStream_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    std::shared_ptr<Callback> callback = nullptr;
    auto segDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    segDownloader->Init();

    mediaDownloader_->SwitchVideoStream(newVideoStream, segDownloader);
    EXPECT_EQ(mediaDownloader_->trackParam_.streamId_, 2);
}

/**
 * @tc.name  : Test NeedRestartDecoder - both streams null
 * @tc.number: NeedRestartDecoder_001
 * @tc.desc  : Test NeedRestartDecoder when both oldStream and newStream are nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, NeedRestartDecoder_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto ret = mediaDownloader_->NeedRestartDecoder(nullptr, nullptr);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test SwitchToPeriod - period switch with needRestart
 * @tc.number: SwitchToPeriod_004
 * @tc.desc  : Test SwitchToPeriod with codec change requiring restart
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchToPeriod_004, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    oldVideoStream->width_ = 1920;
    oldVideoStream->height_ = 1080;
    oldVideoStream->codecs_ = "avc1.64001f";
    oldVideoStream->videoType_ = DASH_VIDEO_TYPE_SDR;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->inUse_ = false;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    newVideoStream->width_ = 1920;
    newVideoStream->height_ = 1080;
    newVideoStream->codecs_ = "hev1.1.6.L93.90";
    newVideoStream->videoType_ = DASH_VIDEO_TYPE_HDR_10;

    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);
    
    mediaDownloader_->SwitchToPeriod(1);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 1);
}

/**
 * @tc.name  : Test SwitchToPeriod with nullptr segment downloader
 * @tc.number: SwitchToPeriod_005
 * @tc.desc  : Test SwitchToPeriod when segmentDownloader is nullptr in Foreach
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchToPeriod_005, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->segmentDownloaders_.Clear();
    mediaDownloader_->segmentDownloaders_.PushBack(nullptr);

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->inUse_ = false;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);

    mediaDownloader_->SwitchToPeriod(1);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 1);
}

/**
 * @tc.name  : Test NeedRestartDecoder - old stream null
 * @tc.number: NeedRestartDecoder_002
 * @tc.desc  : Test NeedRestartDecoder when oldStream is nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, NeedRestartDecoder_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto newStream = std::make_shared<DashStreamDescription>();
    newStream->codecs_ = "avc1.64001f";
    newStream->videoType_ = DASH_VIDEO_TYPE_SDR;
    auto ret = mediaDownloader_->NeedRestartDecoder(nullptr, newStream);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test NeedRestartDecoder - new stream null
 * @tc.number: NeedRestartDecoder_003
 * @tc.desc  : Test NeedRestartDecoder when newStream is nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, NeedRestartDecoder_003, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto oldStream = std::make_shared<DashStreamDescription>();
    oldStream->codecs_ = "avc1.64001f";
    oldStream->videoType_ = DASH_VIDEO_TYPE_SDR;
    auto ret = mediaDownloader_->NeedRestartDecoder(oldStream, nullptr);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test NeedRestartDecoder - codecs different
 * @tc.number: NeedRestartDecoder_004
 * @tc.desc  : Test NeedRestartDecoder when codecs are different
 */
HWTEST_F(DashMediaDownloaderUnittest, NeedRestartDecoder_004, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto oldStream = std::make_shared<DashStreamDescription>();
    oldStream->codecs_ = "avc1.64001f";
    oldStream->videoType_ = DASH_VIDEO_TYPE_SDR;
    auto newStream = std::make_shared<DashStreamDescription>();
    newStream->codecs_ = "hev1.1.6.L93.90";
    newStream->videoType_ = DASH_VIDEO_TYPE_SDR;
    auto ret = mediaDownloader_->NeedRestartDecoder(oldStream, newStream);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test NeedRestartDecoder - videoType different
 * @tc.number: NeedRestartDecoder_005
 * @tc.desc  : Test NeedRestartDecoder when videoType are different
 */
HWTEST_F(DashMediaDownloaderUnittest, NeedRestartDecoder_005, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto oldStream = std::make_shared<DashStreamDescription>();
    oldStream->codecs_ = "avc1.64001f";
    oldStream->videoType_ = DASH_VIDEO_TYPE_SDR;
    auto newStream = std::make_shared<DashStreamDescription>();
    newStream->codecs_ = "avc1.64001f";
    newStream->videoType_ = DASH_VIDEO_TYPE_HDR_10;
    auto ret = mediaDownloader_->NeedRestartDecoder(oldStream, newStream);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test SwitchToPeriod - only video stream exists
 * @tc.number: SwitchToPeriod_006
 * @tc.desc  : Test SwitchToPeriod when there is only video stream (no audio)
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchToPeriod_006, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->segmentDownloaders_.Clear();

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    oldVideoStream->codecs_ = "avc1.64001f";
    oldVideoStream->videoType_ = DASH_VIDEO_TYPE_SDR;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->inUse_ = false;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    newVideoStream->codecs_ = "avc1.64001f";
    newVideoStream->videoType_ = DASH_VIDEO_TYPE_SDR;

    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    mediaDownloader_->SwitchToPeriod(1);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 1);
}

/**
 * @tc.name  : Test SwitchToPeriod - both video and audio streams
 * @tc.number: SwitchToPeriod_007
 * @tc.desc  : Test SwitchToPeriod with both video and audio stream downloaders
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchToPeriod_007, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->segmentDownloaders_.Clear();

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto oldAudioStream = std::make_shared<DashStreamDescription>();
    oldAudioStream->streamId_ = 2;
    oldAudioStream->inUse_ = true;
    oldAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 3;
    newVideoStream->inUse_ = false;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 4;
    newAudioStream->inUse_ = false;
    newAudioStream->periodIndex_ = 1;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldAudioStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newAudioStream);

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    auto audioDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    audioDownloader->Init();
    mediaDownloader_->segmentDownloaders_.PushBack(audioDownloader);

    mediaDownloader_->SwitchToPeriod(1);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 1);
}

/**
 * @tc.name  : Test GetStreamsForPeriodSwitch - old stream not found
 * @tc.number: GetStreamsForPeriodSwitch_001
 * @tc.desc  : Test GetStreamsForPeriodSwitch return false when oldStream is nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, GetStreamsForPeriodSwitch_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);

    std::shared_ptr<DashStreamDescription> oldVideoStream;
    std::shared_ptr<DashStreamDescription> newVideoStreamOut;
    std::shared_ptr<DashStreamDescription> newAudioStreamOut;

    auto ret = mediaDownloader_->GetStreamsForPeriodSwitch(1, oldVideoStream, newVideoStreamOut, newAudioStreamOut);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test GetStreamsForPeriodSwitch - new video stream not found
 * @tc.number: GetStreamsForPeriodSwitch_002
 * @tc.desc  : Test GetStreamsForPeriodSwitch return false when newVideoStream is nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, GetStreamsForPeriodSwitch_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);

    std::shared_ptr<DashStreamDescription> oldVideoStreamOut;
    std::shared_ptr<DashStreamDescription> newVideoStreamOut;
    std::shared_ptr<DashStreamDescription> newAudioStreamOut;

    auto ret = mediaDownloader_->GetStreamsForPeriodSwitch(2, oldVideoStreamOut, newVideoStreamOut, newAudioStreamOut);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test GetStreamsForPeriodSwitch - success case
 * @tc.number: GetStreamsForPeriodSwitch_003
 * @tc.desc  : Test GetStreamsForPeriodSwitch return true with valid streams
 */
HWTEST_F(DashMediaDownloaderUnittest, GetStreamsForPeriodSwitch_003, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 3;
    newAudioStream->periodIndex_ = 1;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newAudioStream);

    std::shared_ptr<DashStreamDescription> oldVideoStreamOut;
    std::shared_ptr<DashStreamDescription> newVideoStreamOut;
    std::shared_ptr<DashStreamDescription> newAudioStreamOut;

    auto ret = mediaDownloader_->GetStreamsForPeriodSwitch(1, oldVideoStreamOut, newVideoStreamOut, newAudioStreamOut);
    EXPECT_EQ(ret, true);
    EXPECT_NE(oldVideoStreamOut, nullptr);
    EXPECT_NE(newVideoStreamOut, nullptr);
    EXPECT_NE(newAudioStreamOut, nullptr);
}

/**
 * @tc.name  : Test NeedRestartDecoder - both streams not null, same codecs and videoType
 * @tc.number: NeedRestartDecoder_006
 * @tc.desc  : Test NeedRestartDecoder when both streams exists with same codecs and videoType
 */
HWTEST_F(DashMediaDownloaderUnittest, NeedRestartDecoder_006, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto oldStream = std::make_shared<DashStreamDescription>();
    oldStream->codecs_ = "avc1.64001f";
    oldStream->videoType_ = DASH_VIDEO_TYPE_SDR;
    auto newStream = std::make_shared<DashStreamDescription>();
    newStream->codecs_ = "avc1.64001f";
    newStream->videoType_ = DASH_VIDEO_TYPE_SDR;
    auto ret = mediaDownloader_->NeedRestartDecoder(oldStream, newStream);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test GetStreamsForPeriodSwitch - no video stream
 * @tc.number: GetStreamsForPeriodSwitch_004
 * @tc.desc  : Test GetStreamsForPeriodSwitch when there is video stream instreamDescription
 */
HWTEST_F(DashMediaDownloaderUnittest, GetStreamsForPeriodSwitch_004, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();

    auto audioStream = std::make_shared<DashStreamDescription>();
    audioStream->streamId_ = 1;
    audioStream->inUse_ = true;
    audioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(audioStream);

    std::shared_ptr<DashStreamDescription> oldVideoStreamOut;
    std::shared_ptr<DashStreamDescription> newVideoStreamOut;
    std::shared_ptr<DashStreamDescription> newAudioStreamOut;

    auto ret = mediaDownloader_->GetStreamsForPeriodSwitch(1, oldVideoStreamOut, newVideoStreamOut, newAudioStreamOut);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test SwitchVideoStream - basic test
 * @tc.number: SwitchVideoStream_002
 * @tc.desc  : Test SwitchVideoStream basic execution
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchVideoStream_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    std::shared_ptr<Callback> callback = nullptr;
    auto segDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    segDownloader->Init();

    mediaDownloader_->SwitchVideoStream(newVideoStream, segDownloader);
    EXPECT_EQ(mediaDownloader_->trackParam_.streamId_, 2);
}

/**
 * @tc.name  : Test SwitchAudioStream - basic test
 * @tc.number: SwitchAudioStream_002
 * @tc.desc  : Test SwitchAudioStream basic execution
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchAudioStream_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 3;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    std::shared_ptr<Callback> callback = nullptr;
    auto segDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    segDownloader->Init();

    mediaDownloader_->SwitchAudioStream(newAudioStream, segDownloader);
    EXPECT_EQ(mediaDownloader_->trackParam_.streamId_, 3);
}

/**
 * @tc.name  : Test ApplyPendingPeriodSwitch - segDownloader is nullptr
 * @tc.number: ApplyPendingPeriodSwitch_001
 * @tc.desc  : Test ApplyPendingPeriodSwitch returns false when segDownloader is nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, ApplyPendingPeriodSwitch_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    auto ret = mediaDownloader_->ApplyPendingPeriodSwitch(nullptr);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ApplyPendingPeriodSwitch - no pending switch
 * @tc.number: ApplyPendingPeriodSwitch_002
 * @tc.desc  : Test ApplyPendingPeriodSwitch returns false when no pending switch exists
 */
HWTEST_F(DashMediaDownloaderUnittest, ApplyPendingPeriodSwitch_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();

    auto ret = mediaDownloader_->ApplyPendingPeriodSwitch(videoDownloader);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ApplyPendingPeriodSwitch - pending video switch
 * @tc.number: ApplyPendingPeriodSwitch_003
 * @tc.desc  : Test ApplyPendingPeriodSwitch applies pending video switch and returns true
 */
HWTEST_F(DashMediaDownloaderUnittest, ApplyPendingPeriodSwitch_003, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->segmentDownloaders_.Clear();

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    // Set up pending video switch
    {
        std::lock_guard<std::mutex> lock(mediaDownloader_->pendingSwitchMutex_);
        mediaDownloader_->pendingVideoSwitch_.targetPeriodIndex = 1;
        mediaDownloader_->pendingVideoSwitch_.newStream = newVideoStream;
        mediaDownloader_->pendingVideoSwitch_.pending = true;
    }

    auto ret = mediaDownloader_->ApplyPendingPeriodSwitch(videoDownloader);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(mediaDownloader_->pendingVideoSwitch_.pending, false);
}

/**
 * @tc.name  : Test ApplyPendingPeriodSwitch - pending audio switch
 * @tc.number: ApplyPendingPeriodSwitch_004
 * @tc.desc  : Test ApplyPendingPeriodSwitch applies pending audio switch and returns true
 */
HWTEST_F(DashMediaDownloaderUnittest, ApplyPendingPeriodSwitch_004, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->segmentDownloaders_.Clear();

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 3;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    std::shared_ptr<Callback> callback = nullptr;
    auto audioDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    audioDownloader->Init();
    mediaDownloader_->segmentDownloaders_.PushBack(audioDownloader);

    // Set up pending audio switch
    {
        std::lock_guard<std::mutex> lock(mediaDownloader_->pendingSwitchMutex_);
        mediaDownloader_->pendingAudioSwitch_.targetPeriodIndex = 1;
        mediaDownloader_->pendingAudioSwitch_.newStream = newAudioStream;
        mediaDownloader_->pendingAudioSwitch_.pending = true;
    }

    auto ret = mediaDownloader_->ApplyPendingPeriodSwitch(audioDownloader);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.pending, false);
}

/**
 * @tc.name  : Test ApplyPendingPeriodSwitch - subtitle stream returns false
 * @tc.number: ApplyPendingPeriodSwitch_005
 * @tc.desc  : Test ApplyPendingPeriodSwitch returns false for non-video/audio stream type
 */
HWTEST_F(DashMediaDownloaderUnittest, ApplyPendingPeriodSwitch_005, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    std::shared_ptr<Callback> callback = nullptr;
    auto subtitleDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 3, MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE, 0, nullptr);
    subtitleDownloader->Init();

    auto ret = mediaDownloader_->ApplyPendingPeriodSwitch(subtitleDownloader);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test SwitchStreamToPeriod - triggerDownloader is nullptr
 * @tc.number: SwitchStreamToPeriod_001
 * @tc.desc  : Test SwitchStreamToPeriod returns early when triggerDownloader is nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchStreamToPeriod_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->SwitchStreamToPeriod(1, nullptr);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 0);
}

/**
 * @tc.name  : Test SwitchStreamToPeriod - GetStreamsForPeriodSwitch returns false
 * @tc.number: SwitchStreamToPeriod_002
 * @tc.desc  : Test SwitchStreamToPeriod returns early when GetStreamsForPeriodSwitch returns false
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchStreamToPeriod_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();
    mediaDownloader_->segmentDownloaders_.Clear();

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    mediaDownloader_->SwitchStreamToPeriod(1, videoDownloader);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 0);
}

/**
 * @tc.name  : Test SwitchStreamToPeriod - video trigger + audio buffer empty
 * @tc.number: SwitchStreamToPeriod_003
 * @tc.desc  : Test SwitchStreamToPeriod when video triggers, audio buffer is empty -> both switch immediately
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchStreamToPeriod_003, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();
    mediaDownloader_->segmentDownloaders_.Clear();

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto oldAudioStream = std::make_shared<DashStreamDescription>();
    oldAudioStream->streamId_ = 2;
    oldAudioStream->inUse_ = true;
    oldAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 3;
    newVideoStream->inUse_ = false;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 4;
    newAudioStream->inUse_ = false;
    newAudioStream->periodIndex_ = 1;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldAudioStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newAudioStream);

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    videoDownloader->SetAllSegmentFinished();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    auto audioDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    audioDownloader->Init();
    audioDownloader->SetAllSegmentFinished();
    mediaDownloader_->segmentDownloaders_.PushBack(audioDownloader);

    mediaDownloader_->SwitchStreamToPeriod(1, videoDownloader);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 1);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.pending, false);
}

/**
 * @tc.name  : Test SwitchStreamToPeriod - video trigger + audio buffer not empty
 * @tc.number: SwitchStreamToPeriod_004
 * @tc.desc  : Test SwitchStreamToPeriod when video triggers, audio has data -> audio deferred
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchStreamToPeriod_004, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();
    mediaDownloader_->segmentDownloaders_.Clear();

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto oldAudioStream = std::make_shared<DashStreamDescription>();
    oldAudioStream->streamId_ = 2;
    oldAudioStream->inUse_ = true;
    oldAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 3;
    newVideoStream->inUse_ = false;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 4;
    newAudioStream->inUse_ = false;
    newAudioStream->periodIndex_ = 1;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldAudioStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newAudioStream);

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    videoDownloader->SetAllSegmentFinished();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    auto audioDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    audioDownloader->Init();
    mediaDownloader_->segmentDownloaders_.PushBack(audioDownloader);

    mediaDownloader_->SwitchStreamToPeriod(1, videoDownloader);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 1);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.pending, true);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.targetPeriodIndex, 1u);
}

/**
 * @tc.name  : Test SwitchStreamToPeriod - audio trigger + video buffer empty
 * @tc.number: SwitchStreamToPeriod_005
 * @tc.desc  : Test SwitchStreamToPeriod when audio triggers, video buffer is empty -> both switch immediately
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchStreamToPeriod_005, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();
    mediaDownloader_->segmentDownloaders_.Clear();

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto oldAudioStream = std::make_shared<DashStreamDescription>();
    oldAudioStream->streamId_ = 2;
    oldAudioStream->inUse_ = true;
    oldAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 3;
    newVideoStream->inUse_ = false;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 4;
    newAudioStream->inUse_ = false;
    newAudioStream->periodIndex_ = 1;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldAudioStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newAudioStream);

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    videoDownloader->SetAllSegmentFinished();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    auto audioDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    audioDownloader->Init();
    audioDownloader->SetAllSegmentFinished();
    mediaDownloader_->segmentDownloaders_.PushBack(audioDownloader);

    mediaDownloader_->SwitchStreamToPeriod(1, audioDownloader);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 1);
    EXPECT_EQ(mediaDownloader_->pendingVideoSwitch_.pending, false);
}

/**
 * @tc.name  : Test SwitchStreamToPeriod - audio trigger + video buffer not empty
 * @tc.number: SwitchStreamToPeriod_006
 * @tc.desc  : Test SwitchStreamToPeriod when audio triggers, video has data -> video deferred
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchStreamToPeriod_006, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();
    mediaDownloader_->segmentDownloaders_.Clear();

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto oldAudioStream = std::make_shared<DashStreamDescription>();
    oldAudioStream->streamId_ = 2;
    oldAudioStream->inUse_ = true;
    oldAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 3;
    newVideoStream->inUse_ = false;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 4;
    newAudioStream->inUse_ = false;
    newAudioStream->periodIndex_ = 1;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldAudioStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newAudioStream);

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    // video buffer not empty: IsAllSegmentFinished is false by default
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    auto audioDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    audioDownloader->Init();
    audioDownloader->SetAllSegmentFinished();
    mediaDownloader_->segmentDownloaders_.PushBack(audioDownloader);

    mediaDownloader_->SwitchStreamToPeriod(1, audioDownloader);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 1);
    // Video should be deferred since IsAllSegmentFinished is false
    EXPECT_EQ(mediaDownloader_->pendingVideoSwitch_.pending, true);
    EXPECT_EQ(mediaDownloader_->pendingVideoSwitch_.targetPeriodIndex, 1u);
}

/**
 * @tc.name  : Test SwitchOtherStreamToPeriod - otherDownloader is nullptr
 * @tc.number: SwitchOtherStreamToPeriod_001
 * @tc.desc  : Test SwitchOtherStreamToPeriod returns early when otherDownloader is nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchOtherStreamToPeriod_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.Clear();
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 3;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    // No segment downloaders, so GetSegmentDownloaderByType returns nullptr
    mediaDownloader_->SwitchOtherStreamToPeriod(1, MediaAVCodec::MediaType::MEDIA_TYPE_VID,
        newVideoStream, newAudioStream);
    EXPECT_EQ(mediaDownloader_->pendingVideoSwitch_.pending, false);
}

/**
 * @tc.name  : Test SwitchOtherStreamToPeriod - video buffer empty, switch immediately
 * @tc.number: SwitchOtherStreamToPeriod_002
 * @tc.desc  : Test SwitchOtherStreamToPeriod switches video immediately when buffer is empty
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchOtherStreamToPeriod_002, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.Clear();

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 3;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    videoDownloader->SetAllSegmentFinished();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    mediaDownloader_->SwitchOtherStreamToPeriod(1, MediaAVCodec::MediaType::MEDIA_TYPE_VID,
        newVideoStream, newAudioStream);
    EXPECT_EQ(mediaDownloader_->pendingVideoSwitch_.pending, false);
}

/**
 * @tc.name  : Test SwitchOtherStreamToPeriod - video buffer not empty, defer switch
 * @tc.number: SwitchOtherStreamToPeriod_003
 * @tc.desc  : Test SwitchOtherStreamToPeriod defers video switch when buffer has data
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchOtherStreamToPeriod_003, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.Clear();

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 3;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    // IsAllSegmentFinished is false by default -> buffer "not empty"
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    mediaDownloader_->SwitchOtherStreamToPeriod(1, MediaAVCodec::MediaType::MEDIA_TYPE_VID,
        newVideoStream, newAudioStream);
    EXPECT_EQ(mediaDownloader_->pendingVideoSwitch_.pending, true);
    EXPECT_EQ(mediaDownloader_->pendingVideoSwitch_.targetPeriodIndex, 1u);
}

/**
 * @tc.name  : Test SwitchOtherStreamToPeriod - audio buffer empty, switch immediately
 * @tc.number: SwitchOtherStreamToPeriod_004
 * @tc.desc  : Test SwitchOtherStreamToPeriod switches audio immediately when buffer is empty
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchOtherStreamToPeriod_004, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.Clear();

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 3;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    std::shared_ptr<Callback> callback = nullptr;
    auto audioDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    audioDownloader->Init();
    audioDownloader->SetAllSegmentFinished();
    mediaDownloader_->segmentDownloaders_.PushBack(audioDownloader);

    mediaDownloader_->SwitchOtherStreamToPeriod(1, MediaAVCodec::MediaType::MEDIA_TYPE_AUD,
        newVideoStream, newAudioStream);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.pending, false);
}

/**
 * @tc.name  : Test SwitchOtherStreamToPeriod - audio buffer not empty, defer switch
 * @tc.number: SwitchOtherStreamToPeriod_005
 * @tc.desc  : Test SwitchOtherStreamToPeriod defers audio switch when buffer has data
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchOtherStreamToPeriod_005, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.Clear();

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 3;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    std::shared_ptr<Callback> callback = nullptr;
    auto audioDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    audioDownloader->Init();
    // IsAllSegmentFinished is false by default -> buffer "not empty"
    mediaDownloader_->segmentDownloaders_.PushBack(audioDownloader);

    mediaDownloader_->SwitchOtherStreamToPeriod(1, MediaAVCodec::MediaType::MEDIA_TYPE_AUD,
        newVideoStream, newAudioStream);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.pending, true);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.targetPeriodIndex, 1u);
}

/**
 * @tc.name  : Test SwitchOtherStreamToPeriod - newAudioStream is nullptr
 * @tc.number: SwitchOtherStreamToPeriod_006
 * @tc.desc  : Test SwitchOtherStreamToPeriod skips audio switch when newAudioStream is nullptr
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchOtherStreamToPeriod_006, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.Clear();

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    std::shared_ptr<Callback> callback = nullptr;
    auto audioDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    audioDownloader->Init();
    mediaDownloader_->segmentDownloaders_.PushBack(audioDownloader);

    mediaDownloader_->SwitchOtherStreamToPeriod(1, MediaAVCodec::MediaType::MEDIA_TYPE_AUD,
        newVideoStream, nullptr);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.pending, false);
}

/**
 * @tc.name  : Test SwitchStreamToPeriod - no audio stream in new period
 * @tc.number: SwitchStreamToPeriod_007
 * @tc.desc  : Test SwitchStreamToPeriod when newAudioStream is nullptr(no audio in new period)
 */
HWTEST_F(DashMediaDownloaderUnittest, SwitchStreamToPeriod_007, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();
    mediaDownloader_->segmentDownloaders_.Clear();

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 2;
    newVideoStream->inUse_ = false;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    // No audio stream in new period
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    videoDownloader->SetAllSegmentFinished();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    mediaDownloader_->SwitchStreamToPeriod(1, videoDownloader);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 1);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.pending, false);
}

/**
 * @tc.name  : Test SeekInternal clears pending switches
 * @tc.number: SeekInternal_PendingSwitch_001
 * @tc.desc  : Test SeekInternal clears pending video and audio switches before seeking
 */
HWTEST_F(DashMediaDownloaderUnittest, SeekInternal_PendingSwitch_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;

    // Set up pending switches
    {
        std::lock_guard<std::mutex> lock(mediaDownloader_->pendingSwitchMutex_);
        mediaDownloader_->pendingVideoSwitch_.pending = true;
        mediaDownloader_->pendingVideoSwitch_.targetPeriodIndex = 1;
        mediaDownloader_->pendingAudioSwitch_.pending = true;
        mediaDownloader_->pendingAudioSwitch_.targetPeriodIndex = 1;
    }

    mediaDownloader_->SeekToTime(0, SeekMode::SEEK_NEXT_SYNC);

    EXPECT_EQ(mediaDownloader_->pendingVideoSwitch_.pending, false);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.pending, false);
}

/**
 * @tc.name  : Test ApplyPendingPeriodSwitch after deferred switch
 * @tc.number: ApplyPendingPeriodSwitch_DeferredFlow_001
 * @tc.desc  : Test the full flow: SwitchStreamToPeriod defers audio, then ApplyPendingPeriodSwitch applies it
 */
HWTEST_F(DashMediaDownloaderUnittest, ApplyPendingPeriodSwitch_DeferredFlow_001, TestSize.Level0)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->currentPeriodIndex_ = 0;
    mediaDownloader_->mpdDownloader_->streamDescriptions_.clear();
    mediaDownloader_->segmentDownloaders_.Clear();

    auto oldVideoStream = std::make_shared<DashStreamDescription>();
    oldVideoStream->streamId_ = 1;
    oldVideoStream->inUse_ = true;
    oldVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto oldAudioStream = std::make_shared<DashStreamDescription>();
    oldAudioStream->streamId_ = 2;
    oldAudioStream->inUse_ = true;
    oldAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    auto newVideoStream = std::make_shared<DashStreamDescription>();
    newVideoStream->streamId_ = 3;
    newVideoStream->inUse_ = false;
    newVideoStream->periodIndex_ = 1;
    newVideoStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_VID;

    auto newAudioStream = std::make_shared<DashStreamDescription>();
    newAudioStream->streamId_ = 4;
    newAudioStream->inUse_ = false;
    newAudioStream->periodIndex_ = 1;
    newAudioStream->type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;

    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(oldAudioStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newVideoStream);
    mediaDownloader_->mpdDownloader_->streamDescriptions_.push_back(newAudioStream);

    std::shared_ptr<Callback> callback = nullptr;
    auto videoDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 0, nullptr);
    videoDownloader->Init();
    videoDownloader->SetAllSegmentFinished();
    mediaDownloader_->segmentDownloaders_.PushBack(videoDownloader);

    auto audioDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 2, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 0, nullptr);
    audioDownloader->Init();
    mediaDownloader_->segmentDownloaders_.PushBack(audioDownloader);

    mediaDownloader_->SwitchStreamToPeriod(1, videoDownloader);
    EXPECT_EQ(mediaDownloader_->currentPeriodIndex_, 1);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.pending, true);

    audioDownloader->SetAllSegmentFinished();
    auto ret = mediaDownloader_->ApplyPendingPeriodSwitch(audioDownloader);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(mediaDownloader_->pendingAudioSwitch_.pending, false);
}

/**
 * @tc.name  : Test GetPlayable no segment downloaders
 * @tc.number: GetPlayable_NoDownloaders
 * @tc.desc  : Test GetPlayable returns false when no vid or aud segment downloaders exist
 */
HWTEST_F(DashMediaDownloaderUnittest, GetPlayable_NoDownloaders, TestSize.Level1)
{
    ASSERT_NE(mediaDownloader_, nullptr);
    mediaDownloader_->segmentDownloaders_.Clear();
    bool result = mediaDownloader_->GetPlayable();
    EXPECT_FALSE(result);
}

/**
 * @tc.name  : Test GetPlayable vid only not buffering
 * @tc.number: GetPlayable_VidOnly_NotBuffering
 * @tc.desc  : Test GetPlayable with vid downloader not buffering
 */
HWTEST_F(DashMediaDownloaderUnittest, GetPlayable_VidOnly_NotBuffering, TestSize.Level1)
{
    auto callback = std::make_shared<TestCallback>();
    auto vidDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 0, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10, nullptr);
    vidDownloader->Init();
    vidDownloader->isBuffering_.store(false);
    mediaDownloader_->segmentDownloaders_.PushBack(vidDownloader);
    bool result = mediaDownloader_->GetPlayable();
    EXPECT_TRUE(result);
}

/**
 * @tc.name  : Test GetPlayable both downloaders buffering
 * @tc.number: GetPlayable_Both_Buffering
 * @tc.desc  : Test GetPlayable returns false when both downloaders are buffering
 */
HWTEST_F(DashMediaDownloaderUnittest, GetPlayable_Both_Buffering, TestSize.Level1)
{
    auto callback = std::make_shared<TestCallback>();
    auto vidDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 0, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10, nullptr);
    vidDownloader->Init();
    vidDownloader->isBuffering_.store(true);
    mediaDownloader_->segmentDownloaders_.PushBack(vidDownloader);
    auto audDownloader = std::make_shared<DashSegmentDownloader>(
        callback, 1, MediaAVCodec::MediaType::MEDIA_TYPE_AUD, 10, nullptr);
    audDownloader->Init();
    audDownloader->isBuffering_.store(true);
    mediaDownloader_->segmentDownloaders_.PushBack(audDownloader);
    bool result = mediaDownloader_->GetPlayable();
    EXPECT_FALSE(result);
}
}
}
}
}