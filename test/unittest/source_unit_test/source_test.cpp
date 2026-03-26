/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed undr the Apache License, Version 2.0 (the "License");
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

#include "source_test.h"
#include "plugin/plugin_base.h"
#include "plugin/source_plugin.h"
#include "common/media_source.h"
#include "plugin/plugin_event.h"
#include "meta/media_types.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

const static int32_t TEST_TIMES_ONE = 1;
const static std::string TEST_NAME = "test_source_plugin";
const std::string MEDIA_ROOT = "file:///data/test/media/";
const std::string VIDEO_FILE1 = MEDIA_ROOT + "camera_info_parser.mp4";
const static int32_t NUM_VALUE = 3;

namespace OHOS {
namespace Media {

void SourceTest::SetUp(void)
{
    source_ = std::make_shared<Source>();
    ASSERT_TRUE(source_ != nullptr);

    mockSourcePlugin_ = std::make_shared<MockSourcePlugin>(TEST_NAME);
    ASSERT_TRUE(mockSourcePlugin_ != nullptr);

    mockCallback_ = std::make_shared<MockCallback>();
    ASSERT_TRUE(mockCallback_ != nullptr);

    source_->plugin_ = mockSourcePlugin_;
    source_->mediaDemuxerCallback_ = mockCallback_;

    mockMediaSource_ = std::make_shared<MockMediaSource>(VIDEO_FILE1);
    ASSERT_TRUE(mockMediaSource_ != nullptr);
}

void SourceTest::TearDown(void)
{
    source_ = nullptr;
    mockCallback_ = nullptr;
    mockSourcePlugin_ = nullptr;
}

/**
 * @tc.name Test SetStartPts API
 * @tc.number SetStartPts_001
 * @tc.desc Test plugin_ != nullptr
 */
HWTEST_F(SourceTest, SetStartPts_001, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), SetStartPts(testing::_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    Status ret = source_->SetStartPts(0);
    EXPECT_EQ(Status::OK, ret);
}

/**
 * @tc.name Test SetStartPts API
 * @tc.number SetStartPts_002
 * @tc.desc Test plugin_ == nullptr
 */
HWTEST_F(SourceTest, SetStartPts_002, TestSize.Level1)
{
    source_->plugin_ = nullptr;
    Status ret = source_->SetStartPts(0);
    EXPECT_EQ(Status::ERROR_INVALID_OPERATION, ret);
}

/**
 * @tc.name Test SetExtraCache API
 * @tc.number SetExtraCache_001
 * @tc.desc Test plugin_ != nullptr
 */
HWTEST_F(SourceTest, SetExtraCache_001, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), SetExtraCache(testing::_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    Status ret = source_->SetExtraCache(100);
    EXPECT_EQ(Status::OK, ret);
}

/**
 * @tc.name Test SetExtraCache API
 * @tc.number SetExtraCache_002
 * @tc.desc Test plugin_ == nullptr
 */
HWTEST_F(SourceTest, SetExtraCache_002, TestSize.Level1)
{
    source_->plugin_ = nullptr;
    Status ret = source_->SetExtraCache(100);
    EXPECT_EQ(Status::ERROR_INVALID_OPERATION, ret);
}

/**
 * @tc.name Test Prepare API
 * @tc.number Prepare_001
 * @tc.desc Test plugin_ == nullptr
 */
HWTEST_F(SourceTest, Prepare_001, TestSize.Level1)
{
    source_->plugin_ = nullptr;
    Status ret = source_->Prepare();
    EXPECT_EQ(Status::OK, ret);
}

/**
 * @tc.name Test Prepare API
 * @tc.number Prepare_002
 * @tc.desc Test ret == Status::OK
 */
HWTEST_F(SourceTest, Prepare_002, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), Prepare()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    Status ret = source_->Prepare();
    EXPECT_EQ(Status::OK, ret);
}

/**
 * @tc.name Test Prepare API
 * @tc.number Prepare_003
 * @tc.desc Test ret != Status::OK && ret != Status::ERROR_DELAY_READY
 */
HWTEST_F(SourceTest, Prepare_003, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), Prepare()).WillRepeatedly(Return(Status::ERROR_WRONG_STATE));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    Status ret = source_->Prepare();
    EXPECT_EQ(Status::ERROR_WRONG_STATE, ret);
}

/**
 * @tc.name Test Prepare API
 * @tc.number Prepare_004
 * @tc.desc Test ret = Status::ERROR_DELAY_READY && isAboveWaterline_ = false
 */
HWTEST_F(SourceTest, Prepare_004, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), Prepare()).WillRepeatedly(Return(Status::ERROR_DELAY_READY));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->isAboveWaterline_ = false;
    Status ret = source_->Prepare();
    EXPECT_EQ(Status::ERROR_DELAY_READY, ret);
}

/**
 * @tc.name Test Prepare API
 * @tc.number Prepare_005
 * @tc.desc Test ret = Status::ERROR_DELAY_READY && isAboveWaterline_ = true
 */
HWTEST_F(SourceTest, Prepare_005, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), Prepare()).WillRepeatedly(Return(Status::ERROR_DELAY_READY));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->isAboveWaterline_ = true;
    Status ret = source_->Prepare();
    EXPECT_EQ(source_->isPluginReady_, false);
    EXPECT_EQ(source_->isAboveWaterline_, false);
    EXPECT_EQ(Status::ERROR_DELAY_READY, ret);
}

/**
 * @tc.name Test AutoSelectBitRate API
 * @tc.number AutoSelectBitRate_001
 * @tc.desc Test plugin_ != nullptr
 */
HWTEST_F(SourceTest, AutoSelectBitRate_001, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), AutoSelectBitRate(testing::_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    Status ret = source_->AutoSelectBitRate(100);
    EXPECT_EQ(Status::OK, ret);
}

/**
 * @tc.name Test AutoSelectBitRate API
 * @tc.number AutoSelectBitRate_002
 * @tc.desc Test plugin_ != nullptr
 */
HWTEST_F(SourceTest, AutoSelectBitRate_002, TestSize.Level1)
{
    source_->plugin_ = nullptr;
    Status ret = source_->AutoSelectBitRate(100);
    EXPECT_EQ(Status::ERROR_INVALID_OPERATION, ret);
}

/**
 * @tc.name Test SetCurrentBitRate API
 * @tc.number SetCurrentBitRate_001
 * @tc.desc Test plugin_ == nullptr
 */
HWTEST_F(SourceTest, SetCurrentBitRate_001, TestSize.Level1)
{
    source_->plugin_ = nullptr;
    Status ret = source_->SetCurrentBitRate(100, 1);
    EXPECT_EQ(Status::ERROR_INVALID_OPERATION, ret);
}

/**
 * @tc.name Test OnEvent API
 * @tc.number OnEvent_001
 * @tc.desc Test protocol_ == "http" && isInterruptNeeded_
 */
HWTEST_F(SourceTest, OnEvent_001, TestSize.Level1)
{
    source_->protocol_ = "http";
    source_->isInterruptNeeded_ = true;
    Plugins::PluginEvent event;
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->OnEvent(event);
    ASSERT_TRUE(source_->mediaDemuxerCallback_ != nullptr);
}

/**
 * @tc.name Test OnEvent API
 * @tc.number OnEvent_002
 * @tc.desc Test event.type == PluginEventType::SOURCE_DRM_INFO_UPDATE
 */
HWTEST_F(SourceTest, OnEvent_002, TestSize.Level1)
{
    source_->protocol_ = "http";
    source_->isInterruptNeeded_ = false;
    Plugins::PluginEvent event;
    event.type = PluginEventType::SOURCE_DRM_INFO_UPDATE;
    EXPECT_CALL(*(mockCallback_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->OnEvent(event);
    ASSERT_TRUE(source_->mediaDemuxerCallback_ != nullptr);
}

/**
 * @tc.name Test OnEvent API
 * @tc.number OnEvent_003
 * @tc.desc Test event.type == PluginEventType::SOURCE_BITRATE_START
 */
HWTEST_F(SourceTest, OnEvent_003, TestSize.Level1)
{
    source_->protocol_ = "http";
    source_->isInterruptNeeded_ = false;
    Plugins::PluginEvent event;
    event.type = PluginEventType::SOURCE_BITRATE_START;
    EXPECT_CALL(*(mockCallback_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->OnEvent(event);
    ASSERT_TRUE(source_->mediaDemuxerCallback_ != nullptr);
}

/**
 * @tc.name Test OnEvent API
 * @tc.number OnEvent_004
 * @tc.desc Test event.type == PluginEventType::DASH_SEEK_READY
 */
HWTEST_F(SourceTest, OnEvent_004, TestSize.Level1)
{
    source_->protocol_ = "http";
    source_->isInterruptNeeded_ = false;
    Plugins::PluginEvent event;
    event.type = PluginEventType::DASH_SEEK_READY;
    EXPECT_CALL(*(mockCallback_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->OnEvent(event);
    ASSERT_TRUE(source_->mediaDemuxerCallback_ != nullptr);
}

/**
 * @tc.name Test OnEvent API
 * @tc.number OnEvent_005
 * @tc.desc Test event.type == PluginEventType::FLV_AUTO_SELECT_BITRATE
 */
HWTEST_F(SourceTest, OnEvent_005, TestSize.Level1)
{
    source_->protocol_ = "http";
    source_->isInterruptNeeded_ = false;
    Plugins::PluginEvent event;
    event.type = PluginEventType::FLV_AUTO_SELECT_BITRATE;
    EXPECT_CALL(*(mockCallback_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->OnEvent(event);
    ASSERT_TRUE(source_->mediaDemuxerCallback_ != nullptr);
}

/**
 * @tc.name Test OnEvent API
 * @tc.number OnEvent_006
 * @tc.desc Test event.type == PluginEventType::HLS_SEEK_READY
 */
HWTEST_F(SourceTest, OnEvent_006, TestSize.Level1)
{
    source_->protocol_ = "http";
    source_->isInterruptNeeded_ = false;
    Plugins::PluginEvent event;
    event.type = PluginEventType::HLS_SEEK_READY;
    EXPECT_CALL(*(mockCallback_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->OnEvent(event);
    ASSERT_TRUE(source_->mediaDemuxerCallback_ != nullptr);
}

/**
 * @tc.name Test SetSelectBitRateFlag API
 * @tc.number SetSelectBitRateFlag_001
 * @tc.desc Test mediaDemuxerCallback_ == nullptr
 */
HWTEST_F(SourceTest, SetSelectBitRateFlag_001, TestSize.Level1)
{
    source_->mediaDemuxerCallback_ = nullptr;
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->SetSelectBitRateFlag(true, 100);
    EXPECT_EQ(source_->mediaDemuxerCallback_, nullptr);
    ASSERT_TRUE(source_->plugin_ != nullptr);
}

/**
 * @tc.name Test SetSelectBitRateFlag API
 * @tc.number SetSelectBitRateFlag_002
 * @tc.desc Test mediaDemuxerCallback_ != nullptr
 */
HWTEST_F(SourceTest, SetSelectBitRateFlag_002, TestSize.Level1)
{
    EXPECT_CALL(*(mockCallback_), SetSelectBitRateFlag(testing::_, testing::_)).Times(TEST_TIMES_ONE);
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->SetSelectBitRateFlag(true, 100);
    ASSERT_TRUE(source_->mediaDemuxerCallback_ != nullptr);
}

/**
 * @tc.name Test CanAutoSelectBitRate API
 * @tc.number CanAutoSelectBitRate_001
 * @tc.desc Test mediaDemuxerCallback_ == nullptr
 */
HWTEST_F(SourceTest, CanAutoSelectBitRate_001, TestSize.Level1)
{
    source_->mediaDemuxerCallback_ = nullptr;
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    bool ret = source_->CanAutoSelectBitRate();
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name Test ReadWithPerfRecord API
 * @tc.number ReadWithPerfRecord_001
 * @tc.desc Test seekToTimeFlag_ = true
 */
HWTEST_F(SourceTest, ReadWithPerfRecord_001, TestSize.Level1)
{
    source_->seekToTimeFlag_ = true;
    EXPECT_CALL(*(mockSourcePlugin_),
        Read(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
    Status ret = source_->ReadWithPerfRecord(1, buffer, 0, 100);
    EXPECT_EQ(Status::OK, ret);
}
/**
 * @tc.name Test SeekToTime API
 * @tc.number SeekToTime_001
 * @tc.desc Test seekable_ != Seekable::SEEKABLE && Ms2HstTime return true
 */
HWTEST_F(SourceTest, SeekToTime_001, TestSize.Level1)
{
    source_->seekable_ = Plugins::Seekable::UNSEEKABLE;
    source_->isInterruptNeeded_ = true;
    int64_t seekTime = 0;
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;
    EXPECT_CALL(*(mockSourcePlugin_), SeekToTime(testing::_, testing::_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mockSourcePlugin_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::INVALID));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    Status ret = source_->SeekToTime(seekTime, mode);
    EXPECT_EQ(Status::OK, ret);
}

/**
 * @tc.name Test SeekToTime API
 * @tc.number SeekToTime_002
 * @tc.desc Test seekable_ == Seekable::SEEKABLE && Ms2HstTime return true
 */
HWTEST_F(SourceTest, SeekToTime_002, TestSize.Level1)
{
    source_->seekable_ = Plugins::Seekable::SEEKABLE;
    source_->isInterruptNeeded_ = true;
    int64_t seekTime = 0;
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;
    EXPECT_CALL(*(mockSourcePlugin_), SeekToTime(testing::_, testing::_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mockSourcePlugin_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::INVALID));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    Status ret = source_->SeekToTime(seekTime, mode);
    EXPECT_EQ(Status::OK, ret);
}

/**
 * @tc.name Test SeekToTime API
 * @tc.number SeekToTime_003
 * @tc.desc Test seekable_ != Seekable::SEEKABLE && Ms2HstTime return false
 */
HWTEST_F(SourceTest, SeekToTime_003, TestSize.Level1)
{
    source_->seekable_ = Plugins::Seekable::UNSEEKABLE;
    source_->isInterruptNeeded_ = true;
    int64_t seekTime = INT64_MAX / HST_MSECOND + 1;
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;
    EXPECT_CALL(*(mockSourcePlugin_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::INVALID));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    Status ret = source_->SeekToTime(seekTime, mode);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, ret);
}

/**
 * @tc.name Test SeekToTime API
 * @tc.number SeekToTime_004
 * @tc.desc Test seekable_ == Seekable::SEEKABLE && Ms2HstTime return false
 */
HWTEST_F(SourceTest, SeekToTime_004, TestSize.Level1)
{
    source_->seekable_ = Plugins::Seekable::SEEKABLE;
    source_->isInterruptNeeded_ = true;
    int64_t seekTime = INT64_MAX / HST_MSECOND + 1;
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));

    Status ret = source_->SeekToTime(seekTime, mode);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, ret);
}

/**
 * @tc.name Test ParseProtocol API
 * @tc.number ParseProtocol_001
 * @tc.desc Test srcType == SourceType::SOURCE_TYPE_URI && mimeType == AVMimeTypes::APPLICATION_M3U8
 */
HWTEST_F(SourceTest, ParseProtocol_001, TestSize.Level1)
{
    std::shared_ptr<Plugins::MediaSource> source_temp =
        std::dynamic_pointer_cast<Plugins::MediaSource>(mockMediaSource_);
    ASSERT_TRUE(source_temp != nullptr);
    source_temp->type_ = SourceType::SOURCE_TYPE_URI;
    source_temp->mimeType_ = AVMimeTypes::APPLICATION_M3U8;
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));

    bool ret = source_->ParseProtocol(source_temp);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name Test ParseProtocol API
 * @tc.number ParseProtocol_002
 * @tc.desc Test srcType == SourceType::SOURCE_TYPE_FD
 */
HWTEST_F(SourceTest, ParseProtocol_002, TestSize.Level1)
{
    std::shared_ptr<Plugins::MediaSource> source_temp =
        std::dynamic_pointer_cast<Plugins::MediaSource>(mockMediaSource_);
    ASSERT_TRUE(source_temp != nullptr);
    source_temp->type_ = SourceType::SOURCE_TYPE_FD;
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));

    bool ret = source_->ParseProtocol(source_temp);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name Test ParseProtocol API
 * @tc.number ParseProtocol_003
 * @tc.desc Test srcType == SourceType::SOURCE_TYPE_STREAM
 */
HWTEST_F(SourceTest, ParseProtocol_003, TestSize.Level1)
{
    std::shared_ptr<Plugins::MediaSource> source_temp =
        std::dynamic_pointer_cast<Plugins::MediaSource>(mockMediaSource_);
    ASSERT_TRUE(source_temp != nullptr);
    source_temp->type_ = SourceType::SOURCE_TYPE_STREAM;
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));

    bool ret = source_->ParseProtocol(source_temp);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name Test FindPlugin API
 * @tc.number FindPlugin_001
 * @tc.desc Test protocol_.empty()
 */
HWTEST_F(SourceTest, FindPlugin_001, TestSize.Level1)
{
    std::shared_ptr<Plugins::MediaSource> source_temp =
        std::dynamic_pointer_cast<Plugins::MediaSource>(mockMediaSource_);
    ASSERT_TRUE(source_temp != nullptr);
    source_temp->type_ = static_cast<OHOS::Media::Plugins::SourceType>(NUM_VALUE);
    source_->protocol_= "";
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));

    Status ret = source_->FindPlugin(source_temp);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name Test GetDuration API
 * @tc.number GetDuration_001
 * @tc.desc Test ret == Status::ERROR_INVALID_PARAMETER
 */
HWTEST_F(SourceTest, GetDuration_001, TestSize.Level1)
{
    source_->seekToTimeFlag_ = true;
    EXPECT_CALL(*(mockSourcePlugin_), GetDuration(testing::_)).WillRepeatedly(Return(Status::ERROR_INVALID_PARAMETER));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    int64_t ret = source_->GetDuration();
    EXPECT_EQ(ret, Plugins::HST_TIME_NONE);
}

/**
 * @tc.name Test GetDuration API
 * @tc.number GetDuration_002
 * @tc.desc Test ret == Status::OK
 */
HWTEST_F(SourceTest, GetDuration_002, TestSize.Level1)
{
    source_->seekToTimeFlag_ = true;
    EXPECT_CALL(*(mockSourcePlugin_), GetDuration(testing::_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->GetDuration();
}

/**
 * @tc.name Test SelectStream API
 * @tc.number SelectStream_001
 * @tc.desc Test SelectStream SUCCESSED
 */
HWTEST_F(SourceTest, SelectStream_001, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), SelectStream(testing::_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    int32_t streamID = 0;
    Status ret = source_->SelectStream(streamID);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name Test GetSegmentOffset API
 * @tc.number GetSegmentOffset_001
 * @tc.desc Test GetSegmentOffset SUCCESSED
 */
HWTEST_F(SourceTest, GetSegmentOffset_001, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), GetSegmentOffset()).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    size_t ret = source_->GetSegmentOffset();
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name Test GetCachedDuration API
 * @tc.number GetCachedDuration_001
 * @tc.desc Test GetCachedDuration SUCCESSED
 */
HWTEST_F(SourceTest, GetCachedDuration_001, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), GetCachedDuration()).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    uint64_t ret = source_->GetCachedDuration();
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name Test RestartAndClearBuffer API
 * @tc.number RestartAndClearBuffer_001
 * @tc.desc Test RestartAndClearBuffer SUCCESSED
 */
HWTEST_F(SourceTest, RestartAndClearBuffer_001, TestSize.Level1)
{
    EXPECT_CALL(*(mockSourcePlugin_), RestartAndClearBuffer()).Times(TEST_TIMES_ONE);
    EXPECT_CALL(*(mockSourcePlugin_), Deinit()).WillOnce(Return(Status::OK));
    source_->RestartAndClearBuffer();
}
} // namespace Media
} // namespace OHOS
