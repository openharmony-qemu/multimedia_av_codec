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
    EXPECT_EQ(duration, 0);
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
    EXPECT_EQ(duration, 0);
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
    EXPECT_EQ(duration, 0);
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
    mediaDemuxer_->targetVideoTrackId_ = MediaDemuxer::TRACK_ID_DUMMY;
    std::vector<std::shared_ptr<Meta>> vector;
    EXPECT_EQ(mediaDemuxer_->GetTargetVideoTrackId(vector), MediaDemuxer::TRACK_ID_DUMMY);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_002, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = MediaDemuxer::TRACK_ID_DUMMY;
    std::vector<std::shared_ptr<Meta>> vector;
    vector.push_back(nullptr);
    EXPECT_EQ(mediaDemuxer_->GetTargetVideoTrackId(vector), MediaDemuxer::TRACK_ID_DUMMY);
}
}  // namespace OHOS::Media
