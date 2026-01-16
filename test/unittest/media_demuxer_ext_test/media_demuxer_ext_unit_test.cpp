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

#include "media_demuxer_ext_unit_test.h"

#include "plugin/plugin_event.h"
#include "demuxer/stream_demuxer.h"

using namespace std;
using namespace testing;
using namespace testing::ext;

using namespace OHOS;
using namespace OHOS::Media;

namespace OHOS::Media {

#undef HiSysEventWrite
#define HiSysEventWrite(domain, eventName, type, ...) (mockRetInt32_;)

static const uint32_t UNUM_0 = 0;
static const int32_t NUM_0 = 0;
static const int32_t NUM_1 = 1;
static const int32_t NUM_2 = 2;
static const int32_t NEG_1 = -1;
static const int32_t NEG_2 = -2;
static const int32_t NUM_10 = 100;
static const int32_t NUM_100 = 100;
static const int64_t NUM_1000 = 1000;
static const int64_t NUM_2000 = 2000;
static const int64_t NUM_3000 = 3000;

void MediaDemuxerExtUnitTest::SetUpTestCase(void) {}

void MediaDemuxerExtUnitTest::TearDownTestCase(void) {}

void MediaDemuxerExtUnitTest::SetUp()
{
    mediaDemuxer_ = std::make_shared<MediaDemuxer>();
    mediaDemuxer_->source_ = std::make_shared<Source>();
    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->audioTrackId_ = 1;
    mediaDemuxer_->demuxerPluginManager_ = std::make_shared<DemuxerPluginManager>();
}

void MediaDemuxerExtUnitTest::TearDown()
{
    mediaDemuxer_ = nullptr;
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest StartReferenceParser API
 * @tc.number: StartReferenceParser
 * @tc.desc  : Test MediaDemuxerExtUnitTest StartReferenceParser interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReportSceneCodeForDemuxer_001, TestSize.Level1)
{
    mediaDemuxer_->isFirstParser_ = true;
    mockRetInt32_ = MSERR_OK;
    std::shared_ptr<DemuxerPlugin> plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(plugin), ParserRefUpdatePos(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::SEEKABLE));

    auto res = mediaDemuxer_->StartReferenceParser(0, 0);
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest StartReferenceParser API
 * @tc.number: StartReferenceParser
 * @tc.desc  : Test MediaDemuxerExtUnitTest StartReferenceParser interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReportSceneCodeForDemuxer_002, TestSize.Level1)
{
    mediaDemuxer_->isFirstParser_ = true;
    mockRetInt32_ = MSERR_UNKNOWN;
    std::shared_ptr<DemuxerPlugin> plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(plugin), ParserRefUpdatePos(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::SEEKABLE));

    auto res = mediaDemuxer_->StartReferenceParser(0, 0);
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest ParserRefInfo API
 * @tc.number: ParserRefInfo_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest ParserRefInfo interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ParserRefInfo_001, TestSize.Level1)
{
    mediaDemuxer_->demuxerPluginManager_ = nullptr;
    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest ParserRefInfo API
 * @tc.number: ParserRefInfo_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest ParserRefInfo interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ParserRefInfo_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask API
 * @tc.number: AccelerateSampleConsumerTask
 * @tc.desc  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AccelerateSampleConsumerTask, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);

    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);

    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = true;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);
    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask API
 * @tc.number: AccelerateSampleConsumerTask_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AccelerateSampleConsumerTask_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);

    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);

    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = true;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);
    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask API
 * @tc.number: AccelerateSampleConsumerTask_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AccelerateSampleConsumerTask_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;

    mediaDemuxer_->AccelerateSampleConsumerTask(1);

    mediaDemuxer_->trackMap_.insert(std::make_pair(1, nullptr));
    mediaDemuxer_->AccelerateSampleConsumerTask(1);

    std::shared_ptr<MediaDemuxer::TrackWrapper> wrapper =
        std::make_shared<MediaDemuxer::TrackWrapper>(1, nullptr, mediaDemuxer_);
    wrapper->isNotifySampleConsumerNeeded_ = false;
    mediaDemuxer_->trackMap_.emplace(1, wrapper);
    mediaDemuxer_->AccelerateSampleConsumerTask(1);

    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AccelerateTrackTask API
 * @tc.number: AccelerateTrackTask_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest AccelerateTrackTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AccelerateTrackTask_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateTrackTask(-1);

    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateTrackTask(-1);

    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = true;
    mediaDemuxer_->AccelerateTrackTask(-1);
    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AccelerateTrackTask API
 * @tc.number: AccelerateTrackTask_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest AccelerateTrackTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AccelerateTrackTask_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;

    mediaDemuxer_->AccelerateTrackTask(1);

    mediaDemuxer_->trackMap_.insert(std::make_pair(1, nullptr));
    mediaDemuxer_->AccelerateTrackTask(1);

    std::shared_ptr<MediaDemuxer::TrackWrapper> wrapper =
        std::make_shared<MediaDemuxer::TrackWrapper>(1, nullptr, mediaDemuxer_);
    wrapper->isNotifySampleConsumerNeeded_ = false;
    mediaDemuxer_->trackMap_.emplace(1, wrapper);
    mediaDemuxer_->AccelerateTrackTask(1);

    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest SetTrackNotifySampleConsumerFlag API
 * @tc.number: SetTrackNotifySampleConsumerFlag_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest SetTrackNotifySampleConsumerFlag interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SetTrackNotifySampleConsumerFlag_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));

    mediaDemuxer_->SetTrackNotifySampleConsumerFlag(1, true);

    mediaDemuxer_->trackMap_.insert(std::make_pair(1, nullptr));
    mediaDemuxer_->SetTrackNotifySampleConsumerFlag(1, true);

    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetDuration API
 * @tc.number: GetDuration_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetDuration interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetDuration_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Seekable::SEEKABLE));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetDuration()).WillRepeatedly(Return(Plugins::HST_TIME_NONE));

    int64_t duration = 0;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(duration);
    auto res = mediaDemuxer_->GetDuration(duration);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetDuration API
 * @tc.number: GetDuration_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetDuration interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetDuration_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Seekable::SEEKABLE));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetDuration()).WillRepeatedly(Return(0));

    mediaDemuxer_->duration_ = 0;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();

    int64_t duration = 0;
    auto res = mediaDemuxer_->GetDuration(duration);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetDuration API
 * @tc.number: GetDuration_003
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetDuration interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetDuration_003, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Seekable::SEEKABLE));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetDuration()).WillRepeatedly(Return(0));

    mediaDemuxer_->duration_ = 0;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();

    int64_t duration = 0;
    auto res = mediaDemuxer_->GetDuration(duration);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask API
 * @tc.number: MediaDemuxerExt_AddDemuxerCopyTask_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AddDemuxerCopyTask_001, TestSize.Level1)
{
    auto res = mediaDemuxer_->AddDemuxerCopyTask(1, TaskType::AUDIO);
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask API
 * @tc.number: MediaDemuxerExt_AddDemuxerCopyTask_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AddDemuxerCopyTask_002, TestSize.Level1)
{
    auto res = mediaDemuxer_->AddDemuxerCopyTask(1, TaskType::AUDIO);
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask API
 * @tc.number: MediaDemuxerExt_AddDemuxerCopyTask_003
 * @tc.desc  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AddDemuxerCopyTask_003, TestSize.Level1)
{
    auto res = mediaDemuxer_->AddDemuxerCopyTask(1, TaskType::SUBTITLE);
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_001, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    std::vector<std::shared_ptr<Meta>> vector;
    EXPECT_EQ(mediaDemuxer_->GetTargetVideoTrackId(vector), MediaDemuxer::TRACK_ID_INVALID);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_002, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    std::vector<std::shared_ptr<Meta>> vector;
    vector.push_back(nullptr);
    EXPECT_EQ(mediaDemuxer_->GetTargetVideoTrackId(vector), MediaDemuxer::TRACK_ID_INVALID);
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_001
 * @tc.desc  : Test taskMap_[trackId] == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_001, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_1;
    mediaDemuxer_->taskMap_[trackId] = nullptr;
    auto iter = mediaDemuxer_->taskMap_.find(trackId);
    EXPECT_TRUE(iter != mediaDemuxer_->taskMap_.end());
    EXPECT_EQ(mediaDemuxer_->StartTask(trackId), Status::OK);
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_002
 * @tc.desc  : Test taskMap_[trackId] != nullptr && !taskMap_[trackId]->IsTaskRunning()
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_002, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_1;
    std::string taskName = "Demux";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(taskPtr), IsTaskRunning()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(taskPtr), Start()).Times(NUM_1);
    mediaDemuxer_->taskMap_[trackId] = std::move(taskPtr);
    Status ret = mediaDemuxer_->StartTask(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_003
 * @tc.desc  : Test taskMap_[trackId] != nullptr && taskMap_[trackId]->IsTaskRunning()
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_003, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_1;
    std::string taskName = "Demux";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(taskPtr), IsTaskRunning()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(taskPtr), Start()).Times(0);
    mediaDemuxer_->taskMap_[trackId] = std::move(taskPtr);
    Status ret = mediaDemuxer_->StartTask(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_004
 * @tc.desc  : Test trackType == TrackType::TRACK_AUDIO
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_004, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    TrackType type = mediaDemuxer_->demuxerPluginManager_->GetTrackTypeByTrackID(static_cast<int32_t>(0));
    EXPECT_EQ(type, TrackType::TRACK_AUDIO);
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_2;
    auto iter = mediaDemuxer_->taskMap_.find(trackId);
    EXPECT_TRUE(iter == mediaDemuxer_->taskMap_.end());
    Status ret = mediaDemuxer_->StartTask(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_005
 * @tc.desc  : Test trackType == TrackType::TRACK_VIDEO
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_005, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_)).Times(NUM_1);
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_2;
    Status ret = mediaDemuxer_->StartTask(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_006
 * @tc.desc  : Test trackType == TrackType::TRACK_SUBTITLE
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_006, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_SUBTITLE));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_)).Times(NUM_1);
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = 4;
    Status ret = mediaDemuxer_->StartTask(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test HandleSelectTrackStreamSeek
 * @tc.number: HandleSelectTrackStreamSeek_001
 * @tc.desc  : Test mimeType.find("audio") == 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackStreamSeek_001, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    int32_t streamID = NUM_0;
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    
    mediaDemuxer_->lastAudioPts_ = NUM_2000;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        SingleStreamSeekTo(_, _, _, _)).WillOnce(::testing::Return(Status::OK));
    mediaDemuxer_->HandleSelectTrackStreamSeek(streamID, trackId);
}

/**
 * @tc.name  : Test HandleSelectTrackStreamSeek
 * @tc.number: HandleSelectTrackStreamSeek_002
 * @tc.desc  : Test mimeType == "application/x-subrip"
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackStreamSeek_002, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    int32_t streamID = NUM_0;
    std::string audioMimeType = "application/x-subrip";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    
    mediaDemuxer_->lastAudioPts_ = NUM_2000;
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_,
        SingleStreamSeekTo(_, _, _, _)).WillOnce(::testing::Return(Status::OK));
    mediaDemuxer_->HandleSelectTrackStreamSeek(streamID, trackId);
}

/**
 * @tc.name  : Test HandleSelectTrackStreamSeek
 * @tc.number: HandleSelectTrackStreamSeek_003
 * @tc.desc  : Test mimeType == "text/vtt"
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackStreamSeek_003, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    int32_t streamID = NUM_0;
    std::string audioMimeType = "text/vtt";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    
    mediaDemuxer_->lastAudioPts_ = NUM_2000;
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_,
        SingleStreamSeekTo(_, _, _, _)).WillOnce(::testing::Return(Status::OK));
    mediaDemuxer_->HandleSelectTrackStreamSeek(streamID, trackId);
}

/**
 * @tc.name  : Test HandleSelectTrackStreamSeek
 * @tc.number: HandleSelectTrackStreamSeek_004
 * @tc.desc  : Test mimeType NUM_1= "text/vtt"
 *             Test mimeType != "application/x-subrip"
 *             Test mimeType.find("audio") != 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackStreamSeek_004, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    int32_t streamID = NUM_0;
    std::string audioMimeType = "aaaa";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);

    mediaDemuxer_->lastAudioPts_ = NUM_3000;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), SingleStreamSeekTo(_, _, _, _)).Times(0);
    mediaDemuxer_->HandleSelectTrackStreamSeek(streamID, trackId);
}

/**
 * @tc.name  : Test HandleHlsRebootPlugin
 * @tc.number: HandleHlsRebootPlugin_001
 * @tc.desc  : Test trackId == TRACK_ID_INVALID
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleHlsRebootPlugin_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = mediaDemuxer_->TRACK_ID_INVALID;
    mediaDemuxer_->audioTrackId_ = mediaDemuxer_->TRACK_ID_INVALID;
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(0);

    auto ret = mediaDemuxer_->HandleHlsRebootPlugin(mediaDemuxer_->videoTrackId_);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test HandleHlsRebootPlugin
 * @tc.number: HandleHlsRebootPlugin_002
 * @tc.desc  : Test trackId != TRACK_ID_INVALID
 *             Test seekReadyInfo.second != SEEK_TO_EOS
 *             Test seekReadyInfo.first >= 0 && seekReadyInfo.first == streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleHlsRebootPlugin_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->OnInterrupted(true);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_100));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _))
        .WillRepeatedly(Return(Status::OK));
    mediaDemuxer_->seekReadyStreamInfo_.clear();
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {NUM_100, false};

    mediaDemuxer_->HandleHlsRebootPlugin(mediaDemuxer_->videoTrackId_);
}

/**
 * @tc.name  : Test HandleHlsRebootPlugin
 * @tc.number: HandleHlsRebootPlugin_003
 * @tc.desc  : Test trackId != TRACK_ID_INVALID
 *             Test seekReadyInfo.second == SEEK_TO_EOS
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleHlsRebootPlugin_003, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_100));
    auto ret = mediaDemuxer_->demuxerPluginManager_->GetTmpStreamIDByTrackID(NUM_1);
    EXPECT_EQ(ret, NUM_100);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(0);
    mediaDemuxer_->seekReadyStreamInfo_.clear();
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {NUM_1, true};
    EXPECT_TRUE(mediaDemuxer_->seekReadyStreamInfo_.find(static_cast<int32_t>(StreamType::VIDEO)) !=
        mediaDemuxer_->seekReadyStreamInfo_.end());
    auto seekReadyInfo = mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)];
    EXPECT_EQ(seekReadyInfo.first, NUM_1);
    EXPECT_EQ(seekReadyInfo.second, true);
    mediaDemuxer_->HandleHlsRebootPlugin(mediaDemuxer_->videoTrackId_);
}

/**
 * @tc.name  : Test HandleHlsRebootPlugin
 * @tc.number: HandleHlsRebootPlugin_004
 * @tc.desc  : Test trackId != TRACK_ID_INVALID
 *             Test seekReadyInfo.second != SEEK_TO_EOS
 *             Test seekReadyInfo.first < 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleHlsRebootPlugin_004, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_100));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _))
        .WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(NUM_1);
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {-1, false};
    
    mediaDemuxer_->HandleHlsRebootPlugin(mediaDemuxer_->videoTrackId_);
}

/**
 * @tc.name  : Test HandleHlsRebootPlugin
 * @tc.number: HandleHlsRebootPlugin_005
 * @tc.desc  : Test trackId != TRACK_ID_INVALID
 *             Test seekReadyInfo.second != SEEK_TO_EOS
 *             Test seekReadyInfo.first >= 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleHlsRebootPlugin_005, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_100));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(0);
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {200, false};
    Status ret = mediaDemuxer_->HandleHlsRebootPlugin(mediaDemuxer_->videoTrackId_);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test SelectTrackChangeStream
 * @tc.number: SelectTrackChangeStream_001
 * @tc.desc  : Test type == TRACK_SUBTITLE
 */
HWTEST_F(MediaDemuxerExtUnitTest, SelectTrackChangeStream_001, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_SUBTITLE));
    TrackType type = mediaDemuxer_->demuxerPluginManager_->GetTrackTypeByTrackID(static_cast<int32_t>(0));
    EXPECT_EQ(type, TrackType::TRACK_SUBTITLE);
    auto ret = mediaDemuxer_->SelectTrackChangeStream(NUM_0);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test SelectTrackChangeStream
 * @tc.number: SelectTrackChangeStream_002
 * @tc.desc  : Test type == TRACK_VIDEO
 */
HWTEST_F(MediaDemuxerExtUnitTest, SelectTrackChangeStream_002, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    TrackType type = mediaDemuxer_->demuxerPluginManager_->GetTrackTypeByTrackID(static_cast<int32_t>(0));
    EXPECT_EQ(type, TrackType::TRACK_VIDEO);
    auto ret = mediaDemuxer_->SelectTrackChangeStream(0);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test OnEvent
 * @tc.number: OnEvent_001
 * @tc.desc  : Test case PluginEventType::SOURCE_DRM_INFO_UPDATE
 *             Test !Any::IsSameTypeWith<std::multimap<std::string, std::vector<uint8_t>>>(event.param)
 */
HWTEST_F(MediaDemuxerExtUnitTest, OnEvent_001, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    PluginEvent event{
        Plugins::PluginEventType::SOURCE_DRM_INFO_UPDATE,
        std::string(""),
        "OnEvent source drmInfo update"
    };
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(0);
    mediaDemuxer_->OnEvent(event);
}

/**
 * @tc.name  : Test OnEvent
 * @tc.number: OnEvent_002
 * @tc.desc  : Test case PluginEventType::SOURCE_DRM_INFO_UPDATE
 *             Test Any::IsSameTypeWith<std::multimap<std::string, std::vector<uint8_t>>>(event.param)
 */
HWTEST_F(MediaDemuxerExtUnitTest, OnEvent_002, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    typename std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    drmInfo.insert(std::make_pair("key1", std::vector<uint8_t>{0x01, 0x02, 0x03}));
    
    PluginEvent event{
        Plugins::PluginEventType::SOURCE_DRM_INFO_UPDATE,
        drmInfo,
        "OnEvent source drmInfo update"
    };
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(0);
    mediaDemuxer_->OnEvent(event);
}

/**
 * @tc.name  : Test OnEvent
 * @tc.number: OnEvent_003
 * @tc.desc  : Test case PluginEventType::SOURCE_BITRATE_START
 */
HWTEST_F(MediaDemuxerExtUnitTest, OnEvent_003, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    PluginEvent event{
        Plugins::PluginEventType::FLV_AUTO_SELECT_BITRATE,
        std::string(""),
        "OnEvent flv auto select bitrate"
    };
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(1);
    mediaDemuxer_->OnEvent(event);
}

/**
 * @tc.name  : Test OnSeekReadyEvent
 * @tc.number: OnSeekReadyEvent_001
 * @tc.desc  : Test case PluginEventType::DASH_SEEK_READY
 */
HWTEST_F(MediaDemuxerExtUnitTest, OnEvent_004, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    PluginEvent event{
        Plugins::PluginEventType::DASH_SEEK_READY,
        std::string(""),
        "OnEvent flv auto select bitrate"
    };
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(0);
    mediaDemuxer_->OnEvent(event);
}

/**
 * @tc.name  : Test OnSeekReadyEvent
 * @tc.number: OnSeekReadyEvent_002
 * @tc.desc  : Test case PluginEventType::HLS_SEEK_READY
 */
HWTEST_F(MediaDemuxerExtUnitTest, OnEvent_005, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    PluginEvent event{
        Plugins::PluginEventType::HLS_SEEK_READY,
        std::string(""),
        "OnEvent flv auto select bitrate"
    };
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(0);
    mediaDemuxer_->OnEvent(event);
}

/**
 * @tc.name  : Test GetLastVideoBufferAbsPts
 * @tc.number: GetLastVideoBufferAbsPts_001
 * @tc.desc  : Test syncCenter_ == nullptr
 *             Test syncCenter_ != nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, GetLastVideoBufferAbsPts_001, TestSize.Level1)
{
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetLastVideoBufferAbsPts())
        .WillRepeatedly(Return(HST_TIME_BASE));
    mediaDemuxer_->syncCenter_ = nullptr;
    uint32_t trackId = UNUM_0;
    int64_t pts = mediaDemuxer_->GetLastVideoBufferAbsPts(trackId);
    EXPECT_EQ(pts, HST_TIME_NONE);

    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    pts = mediaDemuxer_->GetLastVideoBufferAbsPts(trackId);
    EXPECT_EQ(pts, HST_TIME_BASE);
}

/**
 * @tc.name  : Test CheckDropAudioFrame
 * @tc.number: CheckDropAudioFrame_001
 * @tc.desc  : Test isInSeekDropAudio_ && sample->pts_ < videoSeekTime_
 */
HWTEST_F(MediaDemuxerExtUnitTest, CheckDropAudioFrame_001, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_ = UNUM_0;
    mediaDemuxer_->isInSeekDropAudio_ = true;
    uint32_t trackId = UNUM_0;
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    sample->pts_ = NUM_0;
    mediaDemuxer_->videoSeekTime_ = NUM_1;
    bool ret = mediaDemuxer_->CheckDropAudioFrame(sample, trackId);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test CheckDropAudioFrame
 * @tc.number: CheckDropAudioFrame_002
 * @tc.desc  : Test sample->pts_ > videoSeekTime_
 */
HWTEST_F(MediaDemuxerExtUnitTest, CheckDropAudioFrame_002, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_ = UNUM_0;
    mediaDemuxer_->isInSeekDropAudio_ = true;
    uint32_t trackId = UNUM_0;
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    sample->pts_ = NUM_10;
    mediaDemuxer_->videoSeekTime_ = NUM_1;
    mediaDemuxer_->CheckDropAudioFrame(sample, trackId);
    EXPECT_EQ(mediaDemuxer_->isInSeekDropAudio_, false);
}

/**
 * @tc.name  : Test SelectBitRate
 * @tc.number: SelectBitRate_001
 * @tc.desc  : Test isFlvLiveStream_ && IsRightMediaTrack(videoTrackId_, DemuxerTrackType::VIDEO)
 */
HWTEST_F(MediaDemuxerExtUnitTest, SelectBitRate_001, TestSize.Level1)
{
    uint32_t bitRate = UNUM_0;
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->isManualBitRateSetting_ = false;
    mediaDemuxer_->videoTrackId_ = static_cast<uint32_t>(DemuxerTrackType::VIDEO);
    EXPECT_TRUE(mediaDemuxer_->IsRightMediaTrack(mediaDemuxer_->videoTrackId_, DemuxerTrackType::VIDEO));
    EXPECT_TRUE(!(mediaDemuxer_->isManualBitRateSetting_).load());
    auto ret = mediaDemuxer_->SelectBitRate(bitRate, true);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);

    ret = mediaDemuxer_->SelectBitRate(bitRate, false);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);

    mediaDemuxer_->isManualBitRateSetting_ = true;
    EXPECT_TRUE(mediaDemuxer_->isManualBitRateSetting_.load());
    ret = mediaDemuxer_->SelectBitRate(bitRate, false);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : Test HandleSelectTrackChangeStream
 * @tc.number: HandleSelectTrackChangeStream_001
 * @tc.desc  : Test newStreamID != -1 && currentStreamID != -1 && currentStreamID != newStreamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackChangeStream_001, TestSize.Level1)
{
    int32_t trackId = NUM_1;
    int32_t newStreamID = NUM_0;
    int32_t newTrackId = NUM_0;
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamIDByTrackType(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _))
        .WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(_))
        .WillRepeatedly(Return(StreamType::VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _))
        .WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateDefaultStreamID(_, _, _))
        .WillRepeatedly(Return(Status::OK));
    int32_t currentStreamID = mediaDemuxer_->demuxerPluginManager_->GetStreamIDByTrackType(TrackType::TRACK_VIDEO);
    EXPECT_TRUE(currentStreamID != newStreamID);
    auto ret = mediaDemuxer_->HandleSelectTrackChangeStream(trackId, newStreamID, newTrackId);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test HandleSelectTrackChangeStream
 * @tc.number: HandleSelectTrackChangeStream_002
 * @tc.desc  : Test GetEnableSampleQueueFlag() && !GetEnableSampleQueueFlag()
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackChangeStream_002, TestSize.Level1)
{
    int32_t trackId = NUM_1;
    int32_t newStreamID = NUM_2;
    int32_t newTrackId = NUM_0;
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamIDByTrackType(_))
        .WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _))
        .WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(_))
        .WillRepeatedly(Return(StreamType::VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _))
        .WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateDefaultStreamID(_, _, _))
        .WillRepeatedly(Return(Status::OK));
    int32_t currentStreamID = mediaDemuxer_->demuxerPluginManager_->GetStreamIDByTrackType(TrackType::TRACK_VIDEO);
    EXPECT_TRUE(currentStreamID != newStreamID);

    mediaDemuxer_->enableSampleQueue_ = false;
    auto ret = mediaDemuxer_->HandleSelectTrackChangeStream(trackId, newStreamID, newTrackId);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test UpdateLastVideoBufferAbsPts
 * @tc.number: UpdateLastVideoBufferAbsPts_001
 * @tc.desc  : Test isFlvLiveStream_ && IsRightMediaTrack(trackId, DemuxerTrackType::VIDEO)
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateLastVideoBufferAbsPts_001, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->syncCenter_ = nullptr;
    mediaDemuxer_->UpdateLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);
    int64_t lastVideoBufferAbsPts = mediaDemuxer_->GetLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);
    EXPECT_EQ(lastVideoBufferAbsPts, HST_TIME_NONE);
}

/**
 * @tc.name  : Test UpdateLastVideoBufferAbsPts
 * @tc.number: UpdateLastVideoBufferAbsPts_001
 * @tc.desc  : Test lastVideoBufferAbsPts != HST_TIME_NONE
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateLastVideoBufferAbsPts_002, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetLastVideoBufferAbsPts())
        .WillRepeatedly(Return(HST_TIME_BASE));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    mediaDemuxer_->UpdateLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);
    int64_t lastVideoBufferAbsPts = mediaDemuxer_->GetLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);
    EXPECT_EQ(lastVideoBufferAbsPts, HST_TIME_BASE);
}

/**
 * @tc.name  : Test UpdateLastVideoBufferAbsPts
 * @tc.number: UpdateLastVideoBufferAbsPts_003
 * @tc.desc  : Test sqIt != sampleQueueMap_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateLastVideoBufferAbsPts_003, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->videoTrackId_ = NUM_0;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetLastVideoBufferAbsPts())
        .WillRepeatedly(Return(HST_TIME_BASE));
    mediaDemuxer_->sampleQueueMap_.insert(
        std::pair<uint32_t, std::shared_ptr<SampleQueue>>(mediaDemuxer_->videoTrackId_,
        mediaDemuxer_->sampleQueueMap_[mediaDemuxer_->videoTrackId_]));
    EXPECT_TRUE(mediaDemuxer_->sampleQueueMap_.find(mediaDemuxer_->videoTrackId_) !=
        mediaDemuxer_->sampleQueueMap_.end());
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    mediaDemuxer_->UpdateLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);

    mediaDemuxer_->sampleQueueMap_.erase(mediaDemuxer_->videoTrackId_);
    EXPECT_TRUE(mediaDemuxer_->sampleQueueMap_.find(mediaDemuxer_->videoTrackId_) ==
        mediaDemuxer_->sampleQueueMap_.end());
    mediaDemuxer_->UpdateLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);
}

/**
 * @tc.name  : Test TranscoderUpdateOutputBufferPts
 * @tc.number: TranscoderUpdateOutputBufferPts_001
 * @tc.desc  : Test transcoderStartPts_ > 0 && outputBuffer != nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, TranscoderUpdateOutputBufferPts_001, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> outputBuffer = std::make_shared<AVBuffer>();
    mediaDemuxer_->isTranscoderMode_ = true;
    mediaDemuxer_->transcoderStartPts_ = NUM_1;
    mediaDemuxer_->TranscoderUpdateOutputBufferPts(NUM_1, outputBuffer);
    EXPECT_TRUE(mediaDemuxer_->transcoderStartPts_ > 0 && outputBuffer != nullptr);
}

/**
 * @tc.name  : Test TranscoderUpdateOutputBufferPts
 * @tc.number: TranscoderUpdateOutputBufferPts_001
 * @tc.desc  : Test transcoderStartPts_ == 0 && outputBuffer != nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, TranscoderUpdateOutputBufferPts_002, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> outputBuffer = std::make_shared<AVBuffer>();
    mediaDemuxer_->isTranscoderMode_ = true;
    mediaDemuxer_->transcoderStartPts_ = NUM_0;
    mediaDemuxer_->TranscoderUpdateOutputBufferPts(NUM_1, outputBuffer);
    EXPECT_TRUE(mediaDemuxer_->transcoderStartPts_ == 0 && outputBuffer != nullptr);
}

/**
 * @tc.name  : Test TranscoderUpdateOutputBufferPts
 * @tc.number: TranscoderUpdateOutputBufferPts_001
 * @tc.desc  : Test transcoderStartPts_ > 0 && outputBuffer == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, TranscoderUpdateOutputBufferPts_003, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> outputBuffer = nullptr;
    mediaDemuxer_->isTranscoderMode_ = true;
    mediaDemuxer_->transcoderStartPts_ = NUM_1;
    mediaDemuxer_->TranscoderUpdateOutputBufferPts(NUM_1, outputBuffer);
    EXPECT_TRUE(mediaDemuxer_->transcoderStartPts_ > 0 && outputBuffer == nullptr);
}

/**
 * @tc.name  : Test TranscoderUpdateOutputBufferPts
 * @tc.number: TranscoderUpdateOutputBufferPts_001
 * @tc.desc  : Test transcoderStartPts_ == 0 && outputBuffer == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, TranscoderUpdateOutputBufferPts_004, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> outputBuffer = nullptr;
    mediaDemuxer_->isTranscoderMode_ = true;
    mediaDemuxer_->transcoderStartPts_ = NUM_0;
    mediaDemuxer_->TranscoderUpdateOutputBufferPts(NUM_1, outputBuffer);
    EXPECT_TRUE(mediaDemuxer_->transcoderStartPts_ == 0 && outputBuffer == nullptr);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Pause API
 * @tc.number: MediaDemuxerExt_Pause_001
 * @tc.desc  : Test !streamDemuxer_ && !source_ && !CheckTrackEnabledById(videoTrackId_) &&
 *             !GetEnableSampleQueueFlag() && source_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Pause_001, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = nullptr;
    mediaDemuxer_->source_ = nullptr;
    mediaDemuxer_->inPreroll_ = true;
    mediaDemuxer_->videoTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    mediaDemuxer_->enableSampleQueue_ = false;
    auto ret = mediaDemuxer_->Pause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest PauseDragging API
 * @tc.number: MediaDemuxerExt_PauseDragging_001
 * @tc.desc  : Test streamDemuxer_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_PauseDragging_001, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = nullptr;
    mediaDemuxer_->source_ = nullptr;
    auto ret = mediaDemuxer_->PauseDragging();
    EXPECT_EQ(ret, Status::OK);
    // Test PauseAudioAlign API
    ret = mediaDemuxer_->PauseAudioAlign();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Resume API
 * @tc.number: MediaDemuxerExt_Resume_001
 * @tc.desc  : Test streamDemuxer_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Resume_001, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = nullptr;
    mediaDemuxer_->inPreroll_ = true;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->enableSampleQueue_ = false;
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    auto ret = mediaDemuxer_->Resume();
    EXPECT_EQ(ret, Status::OK);
    // Test ResumeDragging API
    ret = mediaDemuxer_->ResumeDragging();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Preroll API
 * @tc.number: MediaDemuxerExt_Preroll_001
 * @tc.desc  : Test inPreroll_.load()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Preroll_001, TestSize.Level1)
{
    mediaDemuxer_->inPreroll_ = true;
    EXPECT_TRUE(mediaDemuxer_->inPreroll_.load());
    auto ret = mediaDemuxer_->Preroll();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Preroll API
 * @tc.number: MediaDemuxerExt_Preroll_002
 * @tc.desc  : Test !CheckTrackEnabledById(videoTrackId_)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Preroll_002, TestSize.Level1)
{
    mediaDemuxer_->inPreroll_ = false;
    mediaDemuxer_->videoTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    auto ret = mediaDemuxer_->Preroll();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Preroll API
 * @tc.number: MediaDemuxerExt_Preroll_003
 * @tc.desc  : Test !isStopped_.load() && !isPaused_.load()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Preroll_003, TestSize.Level1)
{
    mediaDemuxer_->inPreroll_ = false;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->enableSampleQueue_ = false;
    std::string taskName = "Demux";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    mediaDemuxer_->taskMap_[mediaDemuxer_->videoTrackId_] = std::move(taskPtr);
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    EXPECT_EQ(mediaDemuxer_->CheckTrackEnabledById(mediaDemuxer_->videoTrackId_), true);
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isPaused_ = false;
    auto ret = mediaDemuxer_->Preroll();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Preroll API
 * @tc.number: MediaDemuxerExt_Preroll_004
 * @tc.desc  : Test ret != Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Preroll_004, TestSize.Level1)
{
    mediaDemuxer_->inPreroll_ = false;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->enableSampleQueue_ = false;
    std::string taskName = "Demux";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    mediaDemuxer_->taskMap_[mediaDemuxer_->videoTrackId_] = std::move(taskPtr);
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    EXPECT_EQ(mediaDemuxer_->CheckTrackEnabledById(mediaDemuxer_->videoTrackId_), true);
    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->useBufferQueue_ = false;
    EXPECT_EQ(mediaDemuxer_->Start(), Status::ERROR_WRONG_STATE);
    auto ret = mediaDemuxer_->Preroll();
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest PausePreroll API
 * @tc.number: MediaDemuxerExt_PausePreroll_001
 * @tc.desc  : Test !inPreroll_.load()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_PausePreroll_001, TestSize.Level1)
{
    mediaDemuxer_->inPreroll_ = false;
    auto ret = mediaDemuxer_->PausePreroll();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest ResumeAudioAlign API
 * @tc.number: MediaDemuxerExt_ResumeAudioAlign_001
 * @tc.desc  : Test !streamDemuxer_ && !source_ &&
 *             CheckTrackEnabledById(videoTrackId_) && streamDemuxer_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ResumeAudioAlign_001, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = nullptr;
    mediaDemuxer_->source_ = nullptr;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->enableSampleQueue_ = false;
    std::string taskName = "Demux";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    mediaDemuxer_->taskMap_[mediaDemuxer_->videoTrackId_] = std::move(taskPtr);
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    EXPECT_EQ(mediaDemuxer_->CheckTrackEnabledById(mediaDemuxer_->videoTrackId_), true);
    auto ret = mediaDemuxer_->ResumeAudioAlign();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Stop API
 * @tc.number: MediaDemuxerExt_Stop_001
 * @tc.desc  : Test !useBufferQueue_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Stop_001, TestSize.Level1)
{
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->useBufferQueue_ = false;
    auto ret = mediaDemuxer_->Stop();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest SelectTrack API
 * @tc.number: MediaDemuxerExt_SelectTrack_001
 * @tc.desc  : Test demuxerPluginManager_->IsDash() && changeStreamFlag_ == true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectTrack_001, TestSize.Level1)
{
    mediaDemuxer_->useBufferQueue_ = true;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    std::shared_ptr<Meta> trackMeta1 = std::make_shared<Meta>();
    std::shared_ptr<Meta> trackMeta2 = std::make_shared<Meta>();
    trackMeta1->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta2->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta1);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta2);
    EXPECT_EQ(mediaDemuxer_->mediaMetaData_.trackMetas.size(), 2);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillRepeatedly(Return(true));
    auto ret = mediaDemuxer_->SelectTrack(NUM_0);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest SelectTrack API
 * @tc.number: MediaDemuxerExt_SelectTrack_002
 * @tc.desc  : Test demuxerPluginManager_->IsDash() && changeStreamFlag_ == false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectTrack_002, TestSize.Level1)
{
    mediaDemuxer_->useBufferQueue_ = true;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillRepeatedly(Return(false));
    auto ret = mediaDemuxer_->SelectTrack(NUM_0);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest UnselectTrack API
 * @tc.number: MediaDemuxerExt_UnselectTrack_001
 * @tc.desc  : Test IsNeedMapToInnerTrackID()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_UnselectTrack_001, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).Times(NUM_1);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).Times(NUM_1);
    auto ret = mediaDemuxer_->UnselectTrack(NUM_0);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest OnInterrupted API
 * @tc.number: MediaDemuxerExt_OnInterrupted_001
 * @tc.desc  : Test demuxerPluginManager_ != nullptr && demuxerPluginManager_->IsDash()
 *             Test source_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnInterrupted_001, TestSize.Level1)
{
    bool isInterruptNeeded = true;
    mediaDemuxer_->source_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    mediaDemuxer_->OnInterrupted(isInterruptNeeded);
    EXPECT_EQ(mediaDemuxer_->isInterruptNeeded_, true);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest OnInterrupted API
 * @tc.number: MediaDemuxerExt_OnInterrupted_002
 * @tc.desc  : Test demuxerPluginManager_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnInterrupted_002, TestSize.Level1)
{
    bool isInterruptNeeded = true;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(_)).Times(NUM_0);
    mediaDemuxer_->demuxerPluginManager_ = nullptr;
    // Test OnBufferAvailable !GetEnableSampleQueueFlag()
    mediaDemuxer_->enableSampleQueue_ = false;
    mediaDemuxer_->OnBufferAvailable(NUM_0);
    mediaDemuxer_->OnInterrupted(isInterruptNeeded);
    EXPECT_EQ(mediaDemuxer_->isInterruptNeeded_, true);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_003
 * @tc.desc  : Test meta != nullptr && !meta->GetData(Tag::MEDIA_TYPE, mediaType)
 *             Test mediaType != Plugins::MediaType::VIDEO
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_003, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = NUM_0;
    std::vector<std::shared_ptr<Meta>> vector;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MEDIA_TYPE, Plugins::MediaType::AUDIO);
    vector.push_back(trackMeta);
    vector.push_back(std::make_shared<Meta>());
    EXPECT_EQ(mediaDemuxer_->GetTargetVideoTrackId(vector), NUM_0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_004
 * @tc.desc  : Test meta->GetData(Tag::MEDIA_TYPE, mediaType) && mediaType == Plugins::MediaType::VIDEO
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_004, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = NUM_0;
    std::vector<std::shared_ptr<Meta>> vector;
    auto videoMeta = std::make_shared<Meta>();
    videoMeta->SetData(Tag::MEDIA_TYPE, Plugins::MediaType::VIDEO);
    vector.push_back(videoMeta);
    auto audioMeta = std::make_shared<Meta>();
    audioMeta->SetData(Tag::MEDIA_TYPE, Plugins::MediaType::AUDIO);
    vector.push_back(audioMeta);
    uint32_t result = mediaDemuxer_->GetTargetVideoTrackId(vector);
    EXPECT_EQ(result, NUM_0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleDashSelectTrack API
 * @tc.number: MediaDemuxerExt_HandleDashSelectTrack_001
 * @tc.desc  : Test trackId != curTrackId &&
 *             targetStreamID == demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleDashSelectTrack_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    mediaDemuxer_->audioTrackId_ = NUM_0;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    auto result = mediaDemuxer_->HandleDashSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleDashSelectTrack API
 * @tc.number: MediaDemuxerExt_HandleDashSelectTrack_002
 * @tc.desc  : Test trackId == curTrackId &&
 *             targetStreamID == demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleDashSelectTrack_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    mediaDemuxer_->audioTrackId_ = NUM_1;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    auto result = mediaDemuxer_->HandleDashSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleDashSelectTrack API
 * @tc.number: MediaDemuxerExt_HandleDashSelectTrack_003
 * @tc.desc  : Test trackId == curTrackId &&
 *             targetStreamID != demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleDashSelectTrack_003, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_2));
    mediaDemuxer_->audioTrackId_ = NUM_1;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    auto result = mediaDemuxer_->HandleDashSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleDashSelectTrack API
 * @tc.number: MediaDemuxerExt_HandleDashSelectTrack_004
 * @tc.desc  : Test trackId != curTrackId &&
 *             targetStreamID != demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleDashSelectTrack_004, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_2));
    mediaDemuxer_->audioTrackId_ = NUM_2;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    auto result = mediaDemuxer_->HandleDashSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_001
 * @tc.desc  : Test isInterruptNeeded_.load() &&
 *             seekReadyStreamInfo_.find(static_cast<int32_t>(streamType)) == seekReadyStreamInfo_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_001, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_002
 * @tc.desc  : Test !isInterruptNeeded_.load() &&
 *             seekReadyStreamInfo_.find(static_cast<int32_t>(streamType)) == seekReadyStreamInfo_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_002, TestSize.Level1)
{
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    mediaDemuxer_->isInterruptNeeded_.store(false);
    StreamType streamType = TRACK_TO_STREAM_MAP[TrackType::TRACK_VIDEO];
    std::thread notifyThread([this, streamType]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::unique_lock<std::mutex> lock(mediaDemuxer_->rebootPluginMutex_);
        mediaDemuxer_->isInterruptNeeded_.store(true);
        mediaDemuxer_->rebootPluginCondition_.notify_one();
    });
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    notifyThread.join();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_003
 * @tc.desc  : Test !isInterruptNeeded_.load() &&
 *             seekReadyStreamInfo_.find(static_cast<int32_t>(streamType)) != seekReadyStreamInfo_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_003, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(false);
    mediaDemuxer_->seekReadyStreamInfo_ = {{StreamType::VIDEO, {NUM_1, NUM_2}}};
    
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_004
 * @tc.desc  : Test isInterruptNeeded_.load() &&
 *             seekReadyStreamInfo_.find(static_cast<int32_t>(streamType)) != seekReadyStreamInfo_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_004, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {{StreamType::VIDEO, {NUM_1, NUM_2}}};
    
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_005
 * @tc.desc  : Test static_cast<uint32_t>(trackId) == TRACK_ID_DUMMY
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_005, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    int32_t trackId = MediaDemuxer::TRACK_ID_INVALID;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .Times(NUM_0);
    
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_006
 * @tc.desc  : Test seekReadyInfo.second == SEEK_TO_EOS &&
 *             seekReadyInfo.first >= 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_006, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NUM_0, NUM_1}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_1);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_007
 * @tc.desc  : Test seekReadyInfo.second == SEEK_TO_EOS &&
 *             seekReadyInfo.first < 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_007, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NEG_1, NUM_1}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_1);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_008
 * @tc.desc  : Test seekReadyInfo.second == SEEK_TO_EOS &&
 *             seekReadyInfo.first < 0 && seekReadyInfo.first == streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_008, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NEG_2));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NEG_2, NUM_1}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_1);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_009
 * @tc.desc  : Test seekReadyInfo.second == SEEK_TO_EOS &&
 *             seekReadyInfo.first >= 0 && seekReadyInfo.first == streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_009, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = 0;
    int32_t trackId = 0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(2));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NUM_2, NUM_1}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_1);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_010
 * @tc.desc  : Test seekReadyInfo.second != SEEK_TO_EOS &&
 *             seekReadyInfo.first < 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_010, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = 0;
    int32_t trackId = 0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NEG_1, NUM_0}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_0);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_011
 * @tc.desc  : Test seekReadyInfo.second != SEEK_TO_EOS &&
 *             seekReadyInfo.first >= 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_011, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NUM_2, NUM_0}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_0);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_012
 * @tc.desc  : Test seekReadyInfo.second != SEEK_TO_EOS &&
 *             seekReadyInfo.first >= 0 && seekReadyInfo.first == streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_012, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_2));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NUM_2, NUM_0}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_0);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_013
 * @tc.desc  : Test seekReadyInfo.second != SEEK_TO_EOS &&
 *             seekReadyInfo.first < 0 && seekReadyInfo.first == streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_013, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NEG_2));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NEG_2, NUM_0}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_0);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsIgonreBuffering API
 * @tc.number: MediaDemuxerExt_IsIgonreBuffering_001
 * @tc.desc  : Test IsRightMediaTrack(videoTrackId_, DemuxerTrackType::VIDEO)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsIgonreBuffering_001, TestSize.Level1)
{
    bool ret = mediaDemuxer_->IsIgonreBuffering();
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest UpdateSampleQueueCache API
 * @tc.number: MediaDemuxerExt_UpdateSampleQueueCache_002
 * @tc.desc  : Test lastClockTimeMs_ != 0 && currentClockTimeMs - lastClockTimeMs_ > UPDATE_SOURCE_CACHE_MS
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_UpdateSampleQueueCache_002, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->lastClockTimeMs_ = NUM_1;
    mediaDemuxer_->source_ = std::make_shared<Source>();
    EXPECT_CALL(*(mediaDemuxer_->source_), SetExtraCache(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetCachedDuration()).WillRepeatedly(Return(NUM_1000));
    mediaDemuxer_->UpdateSampleQueueCache();
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest UpdateSampleQueueCache API
 * @tc.number: MediaDemuxerExt_UpdateSampleQueueCache_003
 * @tc.desc  : Test lastClockTimeMs_ == 0 && currentClockTimeMs - lastClockTimeMs_ > UPDATE_SOURCE_CACHE_MS
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_UpdateSampleQueueCache_003, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->lastClockTimeMs_ = NUM_0;
    mediaDemuxer_->source_ = nullptr;
    mediaDemuxer_->UpdateSampleQueueCache();
    EXPECT_EQ(mediaDemuxer_->lastClockTimeMs_, SteadyClock::GetCurrentTimeMs());
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_001
 * @tc.desc  : Test isStopped_ && isThreadExit_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_001, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->isThreadExit_ = true;
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_002
 * @tc.desc  : Test !isStopped_ && isThreadExit_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_002, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = true;
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_003
 * @tc.desc  : Test isStopped_ && !isThreadExit_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_003, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->isThreadExit_ = false;
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_004
 * @tc.desc  : Test track == trackMap_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_004, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_005
 * @tc.desc  : Test track->second == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_005, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->trackMap_[queueId] = nullptr;
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_006
 * @tc.desc  : Test track->second == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_006, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    uint32_t trackId = NUM_0;
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    auto sampleQueue = std::make_shared<SampleQueue>();
    std::string taskName = "Demux";
    std::unique_ptr<Task> notifyTask =
        std::make_unique<MockTask>(taskName + "N", mediaDemuxer_->playerId_);
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    sptr<IProducerListener> listener =
        OHOS::sptr<MediaDemuxer::AVBufferQueueProducerListener>::MakeSptr(trackId, demuxer, notifyTask);
    auto trackWrapper = std::make_shared<MediaDemuxer::TrackWrapper>(trackId, listener, demuxer);
    mediaDemuxer_->trackMap_[queueId] = trackWrapper;
    mediaDemuxer_->sampleConsumerTaskMap_.clear();
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSelectBitrateForFlvLive API
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_006
 * @tc.desc  : Test isManualBitRateSetting_.load()
 *             Test GetEnableSampleQueueFlag()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_006, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    mediaDemuxer_->demuxerPluginManager_ = std::make_shared<DemuxerPluginManager>();
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    int64_t startPts = 0;
    uint32_t bitrate = 0;
    mediaDemuxer_->isManualBitRateSetting_ = true;
    mediaDemuxer_->enableSampleQueue_ = true;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).WillRepeatedly(Return(Status::OK));
    auto result = mediaDemuxer_->HandleSelectBitrateForFlvLive(startPts, bitrate);
    EXPECT_EQ(result, Status::OK);
}

/*
 * @tc.name  : MediaDemuxerExt_GetReadLoopRetryUs_001
 * @tc.number: MediaDemuxerExt_GetReadLoopRetryUs_001
 * @tc.desc  : Test when GetEnableSampleQueueFlag returns false, GetReadLoopRetryUs returns 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetReadLoopRetryUs_001, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = false;

    EXPECT_EQ(mediaDemuxer_->GetReadLoopRetryUs(1), false);
}

/**
 * @tc.name  : MediaDemuxerExt_GetReadLoopRetryUs_002
 * @tc.number: MediaDemuxerExt_GetReadLoopRetryUs_002
 * @tc.desc  :Test when isFlvLiveStream_ is false, GetReadLoopRetryUs returns NEXT_DELAY_TIME_US
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetReadLoopRetryUs_002, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = false;

    EXPECT_EQ(mediaDemuxer_->GetReadLoopRetryUs(1), NEXT_DELAY_TIME_US);
}

/**
 * @tc.name  : MediaDemuxerExt_GetReadLoopRetryUs_003
 * @tc.number: MediaDemuxerExt_GetReadLoopRetryUs_003
 * @tc.desc  : Test sampleQueueMap_.count = 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetReadLoopRetryUs_003, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = true;

    EXPECT_EQ(mediaDemuxer_->GetReadLoopRetryUs(1), NEXT_DELAY_TIME_US);
}

/**
 * @tc.name  : MediaDemuxerExt_GetReadLoopRetryUs_004
 * @tc.number: MediaDemuxerExt_GetReadLoopRetryUs_004
 * @tc.desc  : Test sampleDuration <= SAMPLE_FLOW_CONTROL_MIN_SAMPLE_DURATION_US
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetReadLoopRetryUs_004, TestSize.Level1)
{
    auto sampleQueue = std::make_shared<SampleQueue>();
    uint64_t cacheDuration = static_cast<uint64_t>(SAMPLE_FLOW_CONTROL_MIN_SAMPLE_DURATION_US);
    EXPECT_CALL(*sampleQueue, GetCacheDuration()).WillRepeatedly(Return(cacheDuration));

    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->sampleQueueMap_[1] = sampleQueue;

    EXPECT_EQ(mediaDemuxer_->GetReadLoopRetryUs(1), NEXT_DELAY_TIME_US);
}

/**
 * @tc.name  : MediaDemuxerExt_GetReadLoopRetryUs_005
 * @tc.number: MediaDemuxerExt_GetReadLoopRetryUs_005
 * @tc.desc  : Test sampleDuration > SAMPLE_FLOW_CONTROL_MIN_SAMPLE_DURATION_US
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetReadLoopRetryUs_005, TestSize.Level1)
{
    auto sampleQueue = std::make_shared<SampleQueue>();
    uint64_t sampleDuration = SAMPLE_FLOW_CONTROL_MIN_SAMPLE_DURATION_US + 1;

    EXPECT_CALL(*sampleQueue, GetCacheDuration()).WillRepeatedly(Return(sampleDuration));

    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->sampleQueueMap_[1] = sampleQueue;

    EXPECT_EQ(mediaDemuxer_->GetReadLoopRetryUs(1),
        static_cast<int64_t>(sampleDuration >> SAMPLE_FLOW_CONTROL_RATE_POW));
}

/**
 * @tc.name  : MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_001
 * @tc.number: MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_001
 * @tc.desc  : Test Return 0 when IsDash is true or subStreamDemuxer is not nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash())
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(1), static_cast<int64_t>(0));

    mediaDemuxer_->subStreamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(1), static_cast<int64_t>(0));
}

/**
 * @tc.name  : MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_002
 * @tc.number: MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_002
 * @tc.desc  : Test when subtitleDemuxerPlugin is nullptr, return RETRY_DELAY_TIME_US
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));

    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(1), RETRY_DELAY_TIME_US);
}

/**
 * @tc.name  : MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_003
 * @tc.number: MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_003
 * @tc.desc  : Test returns 0 when cache size is greater than 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_003, TestSize.Level1)
{
    std::shared_ptr<DemuxerPlugin> plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_))
        .WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, GetCurrentCacheSize(_, _))
        .WillOnce(DoAll(
            SetArgReferee<1>(1024), // 1024 means cacheSize
            Return(Status::OK)
        ));

    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(0), static_cast<int64_t>(0));
}

/**
 * @tc.name  : MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_004
 * @tc.number: MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_004
 * @tc.desc  : Test when the cache size is 0 or the cache size retrieval fails, RETRY_DELAY_TIME_US is returned.
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_004, TestSize.Level1)
{
    std::shared_ptr<DemuxerPlugin> plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_))
        .WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, GetCurrentCacheSize(_, _))
        .WillOnce(DoAll(
            SetArgReferee<1>(0),
            Return(Status::OK)
        ))
        .WillOnce(DoAll(
            SetArgReferee<1>(1),
            Return(Status::ERROR_UNKNOWN)
        ));

    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(1), RETRY_DELAY_TIME_US);
    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(1), RETRY_DELAY_TIME_US);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleEvent_001
 * @tc.number: MediaDemuxerExt_HandleEvent_001
 * @tc.desc  : Test HandleEvent method when event type is CLIENT_ERROR
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleEvent_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(_)).Times(1);
    PluginEvent event{
        Plugins::PluginEventType::CLIENT_ERROR,
        std::string(""),
        "OnEvent CLIENT_ERROR"
    };

    mediaDemuxer_->HandleEvent(event);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleEvent_002
 * @tc.number: MediaDemuxerExt_HandleEvent_002
 * @tc.desc  : Test HandleEvent method when event type is SERVER_ERROR
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleEvent_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(_)).Times(1);
    PluginEvent event{
        Plugins::PluginEventType::SERVER_ERROR,
        std::string(""),
        "OnEvent SERVER_ERROR"
    };

    mediaDemuxer_->HandleEvent(event);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleEvent_003
 * @tc.number: MediaDemuxerExt_HandleEvent_003
 * @tc.desc  : Test HandleEvent method when event type is INITIAL_BUFFER_SUCCESS
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleEvent_003, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(_)).Times(1);
    PluginEvent event{
        Plugins::PluginEventType::INITIAL_BUFFER_SUCCESS,
        std::string(""),
        "OnEvent INITIAL_BUFFER_SUCCESS"
    };

    mediaDemuxer_->HandleEvent(event);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleEvent_004
 * @tc.number: MediaDemuxerExt_HandleEvent_004
 * @tc.desc  : Test HandleEvent method NotifyInitialBufferingEnd false & true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleEvent_004, TestSize.Level1)
{
    InSequence seq;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(false)).Times(1);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(true)).Times(1);

    PluginEvent errorEvent{
        Plugins::PluginEventType::CLIENT_ERROR,
        std::string(""),
        "OnEvent CLIENT_ERROR"
    };
    PluginEvent successEvent{
        Plugins::PluginEventType::INITIAL_BUFFER_SUCCESS,
        std::string(""),
        "OnEvent INITIAL_BUFFER_SUCCESS"
    };

    mediaDemuxer_->HandleEvent(errorEvent);
    mediaDemuxer_->HandleEvent(successEvent);
}

/**
 * @tc.name  : MediaDemuxerExt_UpdateSampleQueueCache_001
 * @tc.number: MediaDemuxerExt_UpdateSampleQueueCache_001
 * @tc.desc  : Test isFlvLiveStream_ false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_UpdateSampleQueueCache_001, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = false;

    mediaDemuxer_->UpdateSampleQueueCache();

    EXPECT_EQ(mediaDemuxer_->lastClockTimeMs_, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSelectBitrateForFlvLive_001
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_001
 * @tc.desc  : Test source_  nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_001, TestSize.Level1)
{
    uint32_t bitRate = 1000;
    mediaDemuxer_->source_ = nullptr;

    Status ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSelectBitrateForFlvLive_002
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_002
 * @tc.desc  : Test demuxerPluginManager_  nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_002, TestSize.Level1)
{
    uint32_t bitRate = 1000;
    mediaDemuxer_->demuxerPluginManager_  = nullptr;

    Status ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSelectBitrateForFlvLive_003
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_003
 * @tc.desc  : Test case for returning error when streamDemuxer_ is null
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_003, TestSize.Level1)
{
    uint32_t bitRate = 1000;
    mediaDemuxer_->streamDemuxer_ = nullptr;

    Status ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSelectBitrateForFlvLive_004
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_004
 * @tc.desc  : Test case
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_004, TestSize.Level1)
{
    uint32_t bitRate = 1000;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->source_), SetStartPts(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), AutoSelectBitRate(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StopPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TRACK_AUDIO)).WillOnce(Return(TRACK_VIDEO)) // first
        .WillOnce(Return(TRACK_AUDIO)).WillOnce(Return(TRACK_VIDEO)) // two
        .WillOnce(Return(TRACK_AUDIO)).WillOnce(Return(TRACK_VIDEO)); // three
    // InnerSelectTrack return error
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));

    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->videoTrackId_ = 0;
    mediaDemuxer_->audioTrackId_ = 0;
    mediaDemuxer_->isFlvLiveStream_ = false;

    // isManualBitRateSetting_ true
    mediaDemuxer_->isManualBitRateSetting_ = true;
    Status ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::OK);

    // isManualBitRateSetting_ false
    mediaDemuxer_->isManualBitRateSetting_ = false;
    ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::OK);

    // enableSampleQueue_ false
    mediaDemuxer_->isManualBitRateSetting_ = true;
    mediaDemuxer_->enableSampleQueue_ = false;
    ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSelectBitrateForFlvLive_005
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_005
 * @tc.desc  : Test case
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_005, TestSize.Level1)
{
    uint32_t bitRate = 1000;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->source_), SetStartPts(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), AutoSelectBitRate(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StopPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TRACK_VIDEO))
        .WillOnce(Return(TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateTempTrackMapInfo(_, _, _))
        .Times(2); // 2 means TRACK_VIDEO true, TRACK_AUDIO true

    // InnerSelectTrack return error
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));

    mediaDemuxer_->isManualBitRateSetting_ = true;
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->videoTrackId_ = 0;
    mediaDemuxer_->audioTrackId_ = 0;
    mediaDemuxer_->isFlvLiveStream_ = false;

    Status ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_OnDashSeekReadyEvent_001
 * @tc.number: MediaDemuxerExt_OnDashSeekReadyEvent_001
 * @tc.desc  : seekTimeMs < 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnDashSeekReadyEvent_001, TestSize.Level1)
{
    int64_t seekTimeMs = -1;
    Format format;
    format.PutLongValue("seekTime", seekTimeMs);
    PluginEvent event;
    event.param = format;

    mediaDemuxer_->OnDashSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->videoSeekTime_, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_OnDashSeekReadyEvent_002
 * @tc.number: MediaDemuxerExt_OnDashSeekReadyEvent_002
 * @tc.desc  : seekTimeMs > 0 && HasVideo true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnDashSeekReadyEvent_002, TestSize.Level1)
{
    int64_t seekTimeMs = 1000; // 1000 seekTimeMs > 0
    Format format;
    format.PutLongValue("seekTime", seekTimeMs);
    PluginEvent event;
    event.param = format;

    mediaDemuxer_->videoTrackId_ = 0;

    mediaDemuxer_->OnDashSeekReadyEvent(event);
    EXPECT_TRUE(mediaDemuxer_->isInSeekDropAudio_);
}

/**
 * @tc.name  : MediaDemuxerExt_OnDashSeekReadyEvent_003
 * @tc.number: MediaDemuxerExt_OnDashSeekReadyEvent_003
 * @tc.desc  : currentStreamType == MEDIA_TYPE_VID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnDashSeekReadyEvent_003, TestSize.Level1)
{
    Format format;
    format.PutIntValue("currentStreamType", static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_VID));
    format.PutIntValue("currentStreamId", 1);
    format.PutIntValue("isEOS", 0);
    PluginEvent event;
    event.param = format;


    mediaDemuxer_->videoTrackId_ = 0;

    mediaDemuxer_->OnDashSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].first, 1);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_OnDashSeekReadyEvent_004
 * @tc.number: MediaDemuxerExt_OnDashSeekReadyEvent_004
 * @tc.desc  : currentStreamType == MEDIA_TYPE_AUD
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnDashSeekReadyEvent_004, TestSize.Level1)
{
    Format format;
    format.PutIntValue("currentStreamType", static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_AUD));
    format.PutIntValue("currentStreamId", 1);
    format.PutIntValue("isEOS", 0);
    PluginEvent event;
    event.param = format;

    mediaDemuxer_->videoTrackId_ = 0;

    mediaDemuxer_->OnDashSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)].first, 1);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)].second, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_OnDashSeekReadyEvent_005
 * @tc.number: MediaDemuxerExt_OnDashSeekReadyEvent_005
 * @tc.desc  : currentStreamType == MEDIA_TYPE_SUBTITLE
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnDashSeekReadyEvent_005, TestSize.Level1)
{
    Format format;
    format.PutIntValue("currentStreamType", static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE));
    format.PutIntValue("currentStreamId", 1);
    format.PutIntValue("isEOS", 0);
    PluginEvent event;
    event.param = format;

    mediaDemuxer_->videoTrackId_ = 0;

    mediaDemuxer_->OnDashSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::SUBTITLE)].first, 1);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::SUBTITLE)].second, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectBitRateChangeStream_001
 * @tc.number: MediaDemuxerExt_SelectBitRateChangeStream_001
 * @tc.desc  : newStreamID < 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectBitRateChangeStream_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(-1));

    EXPECT_FALSE(mediaDemuxer_->SelectBitRateChangeStream(0));
}

/**
 * @tc.name  : MediaDemuxerExt_SelectBitRateChangeStream_002
 * @tc.number: MediaDemuxerExt_SelectBitRateChangeStream_002
 * @tc.desc  : newStreamID == currentStreamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectBitRateChangeStream_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(1));

    EXPECT_FALSE(mediaDemuxer_->SelectBitRateChangeStream(0));
}

/**
 * @tc.name  : MediaDemuxerExt_SelectBitRateChangeStream_003
 * @tc.number: MediaDemuxerExt_SelectBitRateChangeStream_003
 * @tc.desc  : StartPlugin return error
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectBitRateChangeStream_003, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StopPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _))
        .WillRepeatedly(Return(Status::ERROR_UNKNOWN));

    EXPECT_FALSE(mediaDemuxer_->SelectBitRateChangeStream(0));
}

/**
 * @tc.name  : MediaDemuxerExt_SelectBitRateChangeStream_004
 * @tc.number: MediaDemuxerExt_SelectBitRateChangeStream_004
 * @tc.desc  : isHlsFmp4_ false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectBitRateChangeStream_004, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StopPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateDefaultStreamID(_, _, _))
        .WillRepeatedly(Return(Status::OK));

    // isHlsFmp4_ false
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackInfoByStreamID(_, _, _)).Times(1);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateTempTrackMapInfo(_, _, _)).Times(1);

    // InnerSelectTrack return error
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isFlvLiveStream_ = false;

    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->isHlsFmp4_ = false;

    EXPECT_TRUE(mediaDemuxer_->SelectBitRateChangeStream(0));
}

/**
 * @tc.name  : MediaDemuxerExt_SelectBitRateChangeStream_005
 * @tc.number: MediaDemuxerExt_SelectBitRateChangeStream_005
 * @tc.desc  : isHlsFmp4_ true, audioTrackId_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectBitRateChangeStream_005, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StopPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateDefaultStreamID(_, _, _))
        .WillRepeatedly(Return(Status::OK));

    // isHlsFmp4_ false
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackInfoByStreamID(_, _, _, _)).Times(3);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateTempTrackMapInfo(_, _, _)).Times(3);

    // InnerSelectTrack return error
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isFlvLiveStream_ = false;

    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->audioTrackId_ = 1;
    mediaDemuxer_->isHlsFmp4_ = true;

    // audioTrackId_ != TRACK_ID_INVALID
    EXPECT_TRUE(mediaDemuxer_->SelectBitRateChangeStream(0));

    // audioTrackId_ == TRACK_ID_DUMMY
    mediaDemuxer_->audioTrackId_ = mediaDemuxer_->TRACK_ID_INVALID;
    EXPECT_TRUE(mediaDemuxer_->SelectBitRateChangeStream(0));
}

/**
 * @tc.name  : CopyFrameToUserQueue_ShouldReturnError_WhenPluginIsNull
 * @tc.number: MediaDemuxerExt_CopyFrameToUserQueue_001
 * @tc.desc  : Test case for returning error when plugin is null
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_CopyFrameToUserQueue_001, TestSize.Level1)
{
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_)).WillOnce(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetPluginByStreamID(_)).WillOnce(Return(nullptr));

    EXPECT_EQ(mediaDemuxer_->CopyFrameToUserQueue(1), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : CopyFrameToUserQueue_ShouldReturnError_WhenGetNextSampleSizeFails
 * @tc.number: MediaDemuxerExt_CopyFrameToUserQueue_002
 * @tc.desc  : Test case for returning error when GetNextSampleSize fails
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_CopyFrameToUserQueue_002, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(-1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetPluginByStreamID(_)).WillOnce(Return(plugin));
    // IsNeedMapToInnerTrackID
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, IsDash()).WillRepeatedly(Return(false));
    mediaDemuxer_->isFlvLiveStream_ = false;

    // GetMemoryUsage return
    mediaDemuxer_->eventReceiver_ = nullptr;
    EXPECT_CALL(*plugin, GetNextSampleSize(_, _, _)).WillOnce(Return(Status::ERROR_WAIT_TIMEOUT));


    EXPECT_EQ(mediaDemuxer_->CopyFrameToUserQueue(1), Status::ERROR_WAIT_TIMEOUT);
}

/**
 * @tc.name  : MediaDemuxerExt_CopyFrameToUserQueue_003
 * @tc.number: MediaDemuxerExt_CopyFrameToUserQueue_003
 * @tc.desc  : Test case for returning error when HandleSegmentChange fails
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_CopyFrameToUserQueue_003, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(-1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetPluginByStreamID(_)).WillOnce(Return(plugin));

    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, IsDash()).WillRepeatedly(Return(false));
    mediaDemuxer_->isFlvLiveStream_ = false;

    mediaDemuxer_->eventReceiver_ = nullptr;
    EXPECT_CALL(*plugin, GetNextSampleSize(_, _, _)).WillOnce(Return(Status::END_OF_STREAM));

    mediaDemuxer_->isHls_ = true;
    EXPECT_CALL(*mediaDemuxer_->source_, IsHlsEnd(_)).WillRepeatedly(Return(false));
    EXPECT_EQ(mediaDemuxer_->CopyFrameToUserQueue(1), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : MediaDemuxerExt_ReadSampleWithPerfRecord_001
 * @tc.number: MediaDemuxerExt_ReadSampleWithPerfRecord_001
 * @tc.desc  : isAVDemuxer == true && perfRecEnabled_ == true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReadSampleWithPerfRecord_001, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    mediaDemuxer_->perfRecEnabled_ = true;
    mediaDemuxer_->eventReceiver_ = nullptr;
    mediaDemuxer_->timeout_ = 1000;
    
    EXPECT_CALL(*plugin, ReadSample(_, _)).WillOnce(Return(Status::OK));

    Status ret = mediaDemuxer_->ReadSampleWithPerfRecord(plugin, 0, sample, true);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_ReadSampleWithPerfRecord_002
 * @tc.number: MediaDemuxerExt_ReadSampleWithPerfRecord_002
 * @tc.desc  : isAVDemuxer == true && perfRecEnabled_ == false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReadSampleWithPerfRecord_002, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    mediaDemuxer_->eventReceiver_ = nullptr;
    mediaDemuxer_->perfRecEnabled_ = false;
    mediaDemuxer_->timeout_ = 1000;
    
    EXPECT_CALL(*plugin, ReadSample(_, _)).WillOnce(Return(Status::OK));

    Status ret = mediaDemuxer_->ReadSampleWithPerfRecord(plugin, 0, sample, true);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_ReadSampleWithPerfRecord_003
 * @tc.number: MediaDemuxerExt_ReadSampleWithPerfRecord_003
 * @tc.desc  : isAVDemuxer == false && perfRecEnabled_ == true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReadSampleWithPerfRecord_003, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    mediaDemuxer_->perfRecEnabled_ = true;
    mediaDemuxer_->eventReceiver_ = nullptr;
    mediaDemuxer_->timeout_ = 1000;

    EXPECT_CALL(*plugin, ReadSample(_, _, _)).WillOnce(Return(Status::OK));

    Status ret = mediaDemuxer_->ReadSampleWithPerfRecord(plugin, 0, sample, false);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_ReadSampleWithPerfRecord_004
 * @tc.number: MediaDemuxerExt_ReadSampleWithPerfRecord_004
 * @tc.desc  : isAVDemuxer == false && perfRecEnabled_ == false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReadSampleWithPerfRecord_004, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    mediaDemuxer_->perfRecEnabled_ = false;
    mediaDemuxer_->eventReceiver_ = nullptr;
    mediaDemuxer_->timeout_ = 1000;

    EXPECT_CALL(*plugin, ReadSample(_, _, _)).WillOnce(Return(Status::OK));

    Status ret = mediaDemuxer_->ReadSampleWithPerfRecord(plugin, 0, sample, false);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoSampleQueue
 * @tc.desc: test HandleVideoSampleQueue
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoSampleQueue_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[mediaDemuxer_->videoTrackId_] = sampleQueue;
    EXPECT_CALL(*sampleQueue, AddQueueSize(_)).WillRepeatedly(Return(Status::ERROR_UNKNOWN));
    EXPECT_CALL(*sampleQueue, AcquireBuffer(_)).Times(NUM_1);
    EXPECT_CALL(*sampleQueue, ReleaseBuffer(_)).Times(NUM_1);
    mediaDemuxer_->HandleVideoSampleQueue();
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoTrack_001
 * @tc.desc: test HandleVideoTrack
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoTrack_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    std::shared_ptr<AVBuffer> videoSample = std::make_shared<AVBuffer>();
    videoSample->pts_ = NUM_0;
    mediaDemuxer_->bufferMap_[0] = videoSample;
    mediaDemuxer_->sampleQueueMap_[0] = std::make_shared<SampleQueue>();
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(NUM_1);

    mediaDemuxer_->isVideoMuted_ = true;
    videoSample->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    mediaDemuxer_->hasSetLargeSize_ = false;
    std::string taskName = "SampleConsumerV";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(taskPtr), IsTaskRunning()).WillOnce(Return(true));
    mediaDemuxer_->sampleConsumerTaskMap_[NUM_0] = move(taskPtr);
    mediaDemuxer_->HandleVideoTrack(NUM_0);
    EXPECT_EQ(mediaDemuxer_->hasSetLargeSize_, true);
    EXPECT_EQ(mediaDemuxer_->needReleaseVideoDecoder_, false);
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoTrack_002
 * @tc.desc: test HandleVideoTrack
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoTrack_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    std::shared_ptr<AVBuffer> videoSample = std::make_shared<AVBuffer>();
    videoSample->pts_ = NUM_0;
    mediaDemuxer_->bufferMap_[0] = videoSample;
    mediaDemuxer_->sampleQueueMap_[0] = std::make_shared<SampleQueue>();
    EXPECT_CALL(*(mediaDemuxer_->sampleQueueMap_[NUM_0]), Clear()).WillRepeatedly(Return(Status::OK));
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(NUM_1);

    mediaDemuxer_->isVideoMuted_ = true;
    videoSample->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    mediaDemuxer_->hasSetLargeSize_ = true;
    std::string taskName = "SampleConsumerV";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(taskPtr), IsTaskRunning()).WillOnce(Return(false));
    mediaDemuxer_->sampleConsumerTaskMap_[NUM_0] = move(taskPtr);
    mediaDemuxer_->HandleVideoTrack(NUM_0);
    EXPECT_EQ(mediaDemuxer_->needReleaseVideoDecoder_, false);
}

/**
 * @tc.name: MediaDemuxerExt_HandlePushBuffer_001
 * @tc.desc: test HandlePushBuffer
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandlePushBuffer_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[NUM_0] = sptrProducer;
    std::shared_ptr<AVBuffer> dstBuffer = std::make_shared<AVBuffer>();
    dstBuffer->pts_ = NUM_1;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();

    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    mediaDemuxer_->lastAudioPtsInMute_ = NUM_0;
    Status ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[NUM_0], Status::OK);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    mediaDemuxer_->lastAudioPtsInMute_ = NUM_1;
    dstBuffer->pts_ = NUM_0;
    ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_], Status::OK);
    EXPECT_EQ(ret, Status::OK);

    mediaDemuxer_->needReleaseVideoDecoder_ = false;
    ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[NUM_0], Status::OK);
    EXPECT_EQ(ret, Status::OK);

    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    ret = mediaDemuxer_->HandlePushBuffer(NUM_0, dstBuffer, mediaDemuxer_->bufferQueueMap_[NUM_0], Status::OK);
    EXPECT_EQ(ret, Status::OK);

    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->needReleaseVideoDecoder_ = false;
    ret = mediaDemuxer_->HandlePushBuffer(NUM_0, dstBuffer, mediaDemuxer_->bufferQueueMap_[NUM_0], Status::OK);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: MediaDemuxerExt_HandlePushBuffer_002
 * @tc.desc: test HandlePushBuffer
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandlePushBuffer_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    mediaDemuxer_->sampleQueueMap_[mediaDemuxer_->videoTrackId_] = std::make_shared<SampleQueue>();
    std::shared_ptr<AVBuffer> dstBuffer = std::make_shared<AVBuffer>();
    dstBuffer->pts_ = NUM_0;
    dstBuffer->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->needReleaseVideoDecoder_ = false;
    mediaDemuxer_->isVideoMuted_ = false;
    mediaDemuxer_->needRestore_ = true;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    std::vector<uint8_t> config = {};
    trackMeta->SetData(Tag::MEDIA_CODEC_CONFIG, config);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    Status ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_], Status::OK);
    EXPECT_EQ(mediaDemuxer_->needRestore_, false);
    EXPECT_EQ(ret, Status::OK);

    mediaDemuxer_->needRestore_ = true;
    config = {static_cast<uint8_t>(NUM_0)};
    trackMeta->SetData(Tag::MEDIA_CODEC_CONFIG, config);
    mediaDemuxer_->mediaMetaData_.trackMetas.clear();
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    dstBuffer->memory_ = std::make_shared<AVMemory>();
    EXPECT_CALL(*(dstBuffer->memory_), GetSize()).WillOnce(Return(NUM_2));
    EXPECT_CALL(*(dstBuffer->memory_), Read(_, _, _)).WillOnce(Invoke([](uint8_t *data, int32_t size, int32_t offset) {
        memset_s(data, NUM_2, static_cast<uint8_t>(NUM_0), NUM_2);
        return NUM_2;
    }));
    ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_], Status::OK);
    EXPECT_EQ(mediaDemuxer_->needRestore_, false);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: MediaDemuxerExt_HandlePushBuffer_004
 * @tc.desc: test HandlePushBuffer
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandlePushBuffer_004, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    mediaDemuxer_->sampleQueueMap_[mediaDemuxer_->videoTrackId_] = std::make_shared<SampleQueue>();
    std::shared_ptr<AVBuffer> dstBuffer = std::make_shared<AVBuffer>();
    dstBuffer->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->needReleaseVideoDecoder_ = false;
    mediaDemuxer_->isVideoMuted_ = false;
    mediaDemuxer_->needRestore_ = true;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    std::vector<uint8_t> config = {static_cast<uint8_t>(NUM_0)};
    trackMeta->SetData(Tag::MEDIA_CODEC_CONFIG, config);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    dstBuffer->memory_ = std::make_shared<AVMemory>();
    EXPECT_CALL(*(dstBuffer->memory_), GetSize()).WillOnce(Return(NUM_0));
    Status ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_], Status::OK);
    EXPECT_EQ(mediaDemuxer_->needRestore_, false);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: MediaDemuxer_GetBufferFromUserQueue_001
 * @tc.desc: test GetBufferFromUserQueue
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetBufferFromUserQueue_001, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->sampleQueueMap_[NUM_0] = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_1] = std::make_shared<SampleQueue>();
    bool ret = mediaDemuxer_->GetBufferFromUserQueue(NUM_0, NUM_100);
    EXPECT_EQ(ret, true);

    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->hasSetLargeSize_ = true;
    mediaDemuxer_->isVideoMuted_ = false;
    mediaDemuxer_->needRestore_ = false;
    EXPECT_CALL(*(mediaDemuxer_->sampleQueueMap_[NUM_0]), IsEmpty()).WillOnce(Return(true));
    ret = mediaDemuxer_->GetBufferFromUserQueue(NUM_0, NUM_100);
    EXPECT_EQ(ret, true);

    mediaDemuxer_->hasSetLargeSize_ = true;
    EXPECT_CALL(*(mediaDemuxer_->sampleQueueMap_[NUM_0]), IsEmpty()).WillOnce(Return(false));
    ret = mediaDemuxer_->GetBufferFromUserQueue(NUM_0, NUM_100);
    EXPECT_EQ(ret, false);

    int64_t duration = 0;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(duration);
    mediaDemuxer_->isVideoMuted_ = true;
    ret = mediaDemuxer_->GetBufferFromUserQueue(NUM_0, NUM_100);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: MediaDemuxer_GetBufferFromUserQueue_002
 * @tc.desc: test GetBufferFromUserQueue
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetBufferFromUserQueue_002, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->sampleQueueMap_[NUM_0] = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_1] = std::make_shared<SampleQueue>();
    mediaDemuxer_->lastAudioPtsInMute_ = MAX_VIDEO_LEAD_TIME_ON_MUTE_US;
    int64_t duration = 0;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(duration);
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->lastVideoPts_ = 0;
    mediaDemuxer_->hasSetLargeSize_ = false;
    mediaDemuxer_->needReleaseVideoDecoder_ = false;

    EXPECT_CALL(*(mediaDemuxer_->sampleQueueMap_[NUM_0]), RequestBuffer(_, _, _))
        .Times(NUM_2)
        .WillOnce(Return(Status::ERROR_UNKNOWN))
        .WillRepeatedly(Return(Status::OK));
    bool ret = mediaDemuxer_->GetBufferFromUserQueue(NUM_0, NUM_100);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : MediaDemuxer_DoSelectTrack_001
 * @tc.number: MediaDemuxer_DoSelectTrack_001
 * @tc.desc  : test DoSelectTrack
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_DoSelectTrack_001, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_  = NUM_0;
    mediaDemuxer_->sampleQueueMap_[NUM_0] = nullptr;
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(0)).WillOnce(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillOnce(Return(0)).WillOnce(Return(1));
    EXPECT_CALL(*plugin, UnselectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));

    // curTrackId == audioTrackId_   true
    // sampleQueueMap_[curTrackId] != nullptr  false
    EXPECT_EQ(mediaDemuxer_->DoSelectTrack(NUM_1, NUM_0), Status::OK);
}

/**
 * @tc.name  : MediaDemuxer_DoSelectTrack_002
 * @tc.number: MediaDemuxer_DoSelectTrack_002
 * @tc.desc  : test DoSelectTrack
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_DoSelectTrack_002, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_  = NUM_0;
    mediaDemuxer_->sampleQueueMap_[NUM_2] = nullptr;
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(2)).WillOnce(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillOnce(Return(2)).WillOnce(Return(1));
    EXPECT_CALL(*plugin, UnselectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));

    // curTrackId == audioTrackId_   false
    // sampleQueueMap_[curTrackId] != nullptr  false
    EXPECT_EQ(mediaDemuxer_->DoSelectTrack(NUM_1, NUM_2), Status::OK);
}

/**
 * @tc.name  : MediaDemuxer_DoSelectTrack_003
 * @tc.number: MediaDemuxer_DoSelectTrack_003
 * @tc.desc  : test DoSelectTrack
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_DoSelectTrack_003, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_  = NUM_0;
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_0] = sampleQueue;
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(0)).WillOnce(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillOnce(Return(0)).WillOnce(Return(1));
    EXPECT_CALL(*plugin, UnselectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));

    // curTrackId == audioTrackId_   true
    // sampleQueueMap_[curTrackId] != nullptr  true
    EXPECT_EQ(mediaDemuxer_->DoSelectTrack(NUM_1, NUM_0), Status::OK);
}

/**
 * @tc.name  : MediaDemuxer_DoSelectTrack_004
 * @tc.number: MediaDemuxer_DoSelectTrack_004
 * @tc.desc  : test DoSelectTrack
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_DoSelectTrack_004, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_  = NUM_0;
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_2] = sampleQueue;
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(2)).WillOnce(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillOnce(Return(2)).WillOnce(Return(1));
    EXPECT_CALL(*plugin, UnselectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));

    // curTrackId == audioTrackId_   false
    // sampleQueueMap_[curTrackId] != nullptr  true
    EXPECT_EQ(mediaDemuxer_->DoSelectTrack(NUM_1, NUM_2), Status::OK);
}
/**
 * @tc.name  : MediaDemuxer_HandleEosDrag_001
 * @tc.number: MediaDemuxer_HandleEosDrag_001
 * @tc.desc  : test HandleEosDrag
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleEosDrag_001, TestSize.Level1)
{
    int32_t trackId = 0;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = static_cast<uint32_t>(AVBufferFlag::EOS);
    buffer->pts_ = 0;
    mediaDemuxer_->bufferMap_[trackId] = buffer;
    mediaDemuxer_->eosMap_[trackId] = false;
    bool isDiscardable = false;
    mediaDemuxer_->HandleEosDrag(trackId, isDiscardable);
    EXPECT_EQ(mediaDemuxer_->eosMap_[trackId], true);
}

/**
 * @tc.name  : MediaDemuxer_HandleEosDrag_002
 * @tc.number: MediaDemuxer_HandleEosDrag_002
 * @tc.desc  : test HandleEosDrag
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleEosDrag_002, TestSize.Level1)
{
    int32_t trackId = 0;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = static_cast<uint32_t>(AVBufferFlag::EOS);
    buffer->pts_ = 0;
    mediaDemuxer_->bufferMap_[trackId] = buffer;
    mediaDemuxer_->eosMap_[trackId] = false;
    bool isDiscardable = true;
    mediaDemuxer_->HandleEosDrag(trackId, isDiscardable);
    EXPECT_EQ(mediaDemuxer_->eosMap_[trackId], false);
}

/**
 * @tc.name  : MediaDemuxer_HandleEosDrag_003
 * @tc.number: MediaDemuxer_HandleEosDrag_003
 * @tc.desc  : test HandleEosDrag
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleEosDrag_003, TestSize.Level1)
{
    int32_t trackId = 0;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = 0;
    buffer->pts_ = 0;
    mediaDemuxer_->bufferMap_[trackId] = buffer;
    mediaDemuxer_->eosMap_[trackId] = false;
    bool isDiscardable = false;
    mediaDemuxer_->HandleEosDrag(trackId, isDiscardable);
    EXPECT_EQ(mediaDemuxer_->eosMap_[trackId], false);
}

/**
 * @tc.name  : MediaDemuxer_HandleEosDrag_004
 * @tc.number: MediaDemuxer_HandleEosDrag_004
 * @tc.desc  : test HandleEosDrag
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleEosDrag_004, TestSize.Level1)
{
    int32_t trackId = 0;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = 0;
    buffer->pts_ = 0;
    mediaDemuxer_->bufferMap_[trackId] = buffer;
    mediaDemuxer_->eosMap_[trackId] = false;
    bool isDiscardable = true;
    mediaDemuxer_->HandleEosDrag(trackId, isDiscardable);
    EXPECT_EQ(mediaDemuxer_->eosMap_[trackId], false);
}

/**
 * @tc.name  : MediaDemuxerExt_OnHlsSeekReadyEvent_001
 * @tc.number: MediaDemuxerExt_OnHlsSeekReadyEvent_001
 * @tc.desc  : Test currentStreamType == MEDIA_TYPE_AUD
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnHlsSeekReadyEvent_001, TestSize.Level1)
{
    Format format;
    format.PutIntValue("currentStreamType", static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_AUD));
    format.PutIntValue("currentStreamId", 1);
    format.PutIntValue("isEOS", 0);
    PluginEvent event;
    event.param = format;
    mediaDemuxer_->videoTrackId_ = 0;
    mediaDemuxer_->OnHlsSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)].first, 1);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)].second, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_OnHlsSeekReadyEvent_002
 * @tc.number: MediaDemuxerExt_OnHlsSeekReadyEvent_002
 * @tc.desc  : Test currentStreamType == MEDIA_TYPE_VID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnHlsSeekReadyEvent_002, TestSize.Level1)
{
    Format format;
    format.PutIntValue("currentStreamType", static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_VID));
    format.PutIntValue("currentStreamId", 1);
    format.PutIntValue("isEOS", 0);
    PluginEvent event;
    event.param = format;
    mediaDemuxer_->videoTrackId_ = 0;
    mediaDemuxer_->OnHlsSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].first, 1);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_IsSegmentEos_001
 * @tc.number: MediaDemuxerExt_IsSegmentEos_001
 * @tc.desc  : Test IsSegmentEos() == true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsSegmentEos_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = true;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    mediaDemuxer_->segmentEosMap_[NUM_1] = true;
    EXPECT_EQ(mediaDemuxer_->IsSegmentEos(), true);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_001
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_001
 * @tc.desc  : Test isAVStreamIsSame is false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;

    // IsAVStreamIdSame return false
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(NUM_1)).WillRepeatedly(Return(NUM_0));

    // HandleSegmentChange return Status::OK
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _))
        .WillOnce(Return(Status::OK));
    
    // InnerSelectTrack return Status::OK
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillRepeatedly(Return(Status::OK));

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_002
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_002
 * @tc.desc  : Test isAVStreamIdSame is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_002, TestSize.Level1)
{
    // IsSegmentEos return true
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = true;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    mediaDemuxer_->segmentEosMap_[NUM_1] = true;

    // IsAVStreamIdSame return true
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_0));

    // HandleSegmentChange return Status::OK
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _))
        .WillOnce(Return(Status::OK));
    
    // InnerSelectTrack return Status::OK
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillRepeatedly(Return(Status::OK));

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleHlsSeek_001
 * @tc.number: MediaDemuxerExt_HandleHlsSeek_001
 * @tc.desc  : Test isAVStreamIdSame is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleHlsSeek_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;

    // IsAVStreamIdSame return true
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_100));

    // HandleHlsRebootPlugin return Status::OK
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(0);
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {200, false};

    // InnerSelectTrack return Status::OK
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillRepeatedly(Return(Status::OK));

    Status ret = mediaDemuxer_->HandleHlsSeek();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleHlsSeek_002
 * @tc.number: MediaDemuxerExt_HandleHlsSeek_002
 * @tc.desc  : Test isAVStreamIdIsSame is false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleHlsSeek_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;

    // IsAVStreamIdSame return false
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(NUM_100)).WillOnce(Return(NUM_0))
        .WillOnce(Return(NUM_100)).WillOnce(Return(NUM_100)).WillOnce(Return(NUM_0))
        .WillOnce(Return(NUM_0)).WillOnce(Return(NUM_100)).WillOnce(Return(NUM_0))
        .WillRepeatedly(Return(NUM_100));

    // HandleHlsRebootPlugin return Status::OK
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TrackType::TRACK_AUDIO)).WillOnce(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(0);
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)] = {200, false};
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {200, false};

    // InnerSelectTrack return Status::OK
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillRepeatedly(Return(Status::OK));

    Status ret = mediaDemuxer_->HandleHlsSeek();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_RecordDemuxerTimeStamp_001
 * @tc.number: MediaDemuxerExt_RecordDemuxerTimeStamp_001
 * @tc.desc  : Test stage == StallingStage::DEMUXER_START
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_RecordDemuxerTimeStamp_001, TestSize.Level1)
{
    AVBuffer buffer;
    buffer.meta_ = std::make_shared<Meta>();
    StallingStage stage = StallingStage::DEMUXER_START;

    mediaDemuxer_->RecordDemuxerTimeStamp(buffer, stage);

    std::vector<int64_t> timeStampList;
    bool ret = buffer.meta_->GetData(Tag::STALLING_TIMESTAMP, timeStampList);
    ASSERT_TRUE(ret);
    ASSERT_EQ(timeStampList.size(), NUM_2);
    EXPECT_EQ(timeStampList[0], static_cast<int64_t>(stage));
    EXPECT_GT(timeStampList[1], NUM_0);
}

/**
 * @tc.name  : MediaDemuxerExt_RecordDemuxerTimeStamp_002
 * @tc.number: MediaDemuxerExt_RecordDemuxerTimeStamp_002
 * @tc.desc  : Test stage == StallingStage::DEMUXER_START
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_RecordDemuxerTimeStamp_002, TestSize.Level1)
{
    AVBuffer buffer;
    buffer.meta_ = std::make_shared<Meta>();
    StallingStage stage = StallingStage::DEMUXER_END;

    mediaDemuxer_->RecordDemuxerTimeStamp(buffer, stage);

    std::vector<int64_t> timeStampList;
    bool ret = buffer.meta_->GetData(Tag::STALLING_TIMESTAMP, timeStampList);
    ASSERT_TRUE(ret);
    ASSERT_EQ(timeStampList.size(), NUM_2);
    EXPECT_EQ(timeStampList[0], static_cast<int64_t>(stage));
    EXPECT_GT(timeStampList[1], NUM_0);
}

/**
 * @tc.name  : MediaDemuxer_IsBufferingMap
 * @tc.number: MediaDemuxer_IsBufferingMap
 * @tc.desc  : test SetTrackIsBuffering GetTrackIsBuffering
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_IsBufferingMap, TestSize.Level1)
{
    int32_t trackId = 0;
    mediaDemuxer_->isBufferingMap_[trackId] = true;
    EXPECT_TRUE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    mediaDemuxer_->SetTrackIsBuffering(trackId, false);
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    mediaDemuxer_->SetTrackIsBuffering(trackId, true);
    EXPECT_TRUE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    
    trackId = 100;
    mediaDemuxer_->isBufferingMap_[trackId] = false;
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    mediaDemuxer_->SetTrackIsBuffering(trackId, true);
    EXPECT_TRUE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    mediaDemuxer_->SetTrackIsBuffering(trackId, false);
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
}
}  // namespace OHOS::Media
