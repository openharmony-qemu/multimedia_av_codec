/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <sys/timeb.h>
#include <unordered_map>
#include "muxer_filter_unit_test.h"
#include "muxer_filter.h"
#include "common/log.h"
#include "filter/filter_factory.h"
#include "muxer/media_muxer.h"
#include "avcodec_trace.h"
#include "avcodec_sysevent.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void MuxerFilterUnitTest::SetUpTestCase(void) {}

void MuxerFilterUnitTest::TearDownTestCase(void) {}

void MuxerFilterUnitTest::SetUp(void)
{
    muxerFilter_ = std::make_shared<MuxerFilter>("testMuxerFilter", FilterType::FILTERTYPE_MUXER);
}

void MuxerFilterUnitTest::TearDown(void)
{
    muxerFilter_ = nullptr;
}

/**
 * @tc.name: MuxerFilter_GetCurrentPtsMs_0100
 * @tc.desc: GetCurrentPtsMs
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_GetCurrentPtsMs_0100, TestSize.Level1)
{
    muxerFilter_->SetOutputParameter(0, 0, 0, 0);
    muxerFilter_->lastVideoPts_ = 1000;
    EXPECT_EQ(muxerFilter_->GetCurrentPtsMs(), muxerFilter_->lastVideoPts_ / 1000);
    muxerFilter_->lastVideoPts_ = 0;
    muxerFilter_->lastAudioPts_ = 1000;
    EXPECT_EQ(muxerFilter_->GetCurrentPtsMs(), muxerFilter_->lastAudioPts_ / 1000);
}

/**
 * @tc.name: MuxerFilter_DoStart_0100
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStart_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    EXPECT_CALL(*mockMediaMuxer, Start()).WillOnce(testing::Return(Status::NO_ERROR));

    EXPECT_EQ(muxerFilter_->DoStart(), Status::OK);
    EXPECT_EQ(muxerFilter_->isStarted, true);
    EXPECT_EQ(muxerFilter_->DoStart(), Status::OK);
    EXPECT_EQ(muxerFilter_->isStarted, true);
}

/**
 * @tc.name: MuxerFilter_DoStop_0100
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStop_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    EXPECT_CALL(*mockMediaMuxer, Start()).WillOnce(testing::Return(Status::NO_ERROR));
    EXPECT_EQ(muxerFilter_->DoStart(), Status::OK);
    EXPECT_EQ(muxerFilter_->isStarted, true);
    
    muxerFilter_->stopCount_ = 0;
    muxerFilter_->preFilterCount_ = 2;

    EXPECT_EQ(muxerFilter_->DoStop(), Status::OK);
    EXPECT_EQ(muxerFilter_->isStarted, true);
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::NO_ERROR));
    EXPECT_EQ(muxerFilter_->DoStop(), Status::OK);
    EXPECT_EQ(muxerFilter_->isStarted, false);
    
    muxerFilter_->stopCount_ = 0;
    muxerFilter_->preFilterCount_ = 1;
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::ERROR_WRONG_STATE));
    EXPECT_EQ(muxerFilter_->DoStop(), Status::OK);
    
    muxerFilter_->stopCount_ = 0;
    muxerFilter_->preFilterCount_ = 1;
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::ERROR_UNKNOWN));
    EXPECT_EQ(muxerFilter_->DoStop(), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: MuxerFilter_SetUserMeta_0100
 * @tc.desc: SetUserMeta
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetUserMeta_0100, TestSize.Level1)
{
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    muxerFilter_->SetUserMeta(userMeta);
    EXPECT_NE(muxerFilter_->mediaMuxer_->SetUserMeta(userMeta), Status::OK);
}

/**
 * @tc.name: MuxerFilter_OnLinked_0100
 * @tc.desc: OnLinked
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnLinked_0100, TestSize.Level1)
{
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    std::shared_ptr<FilterLinkCallback> callback = std::make_shared<MyFilterLinkCallback>();
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->OnLinked(streamType, meta, callback);
    EXPECT_NE(muxerFilter_->OnLinked(streamType, meta, callback), Status::OK);
}

/**
 * @tc.name: MuxerFilter_OnBufferFilled_0100
 * @tc.desc: OnBufferFilled
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnBufferFilled_0100, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->isTransCoderMode = false;
    muxerFilter_->preFilterCount_ = 0;
    inputBuffer->pts_ = 3000000000;
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(inputBuffer->flag_, 0);
    muxerFilter_->isTransCoderMode = true;
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    muxerFilter_->isTransCoderMode = true;
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(inputBuffer->flag_, 0);
}

/**
 * @tc.name: MuxerFilter_OnBufferFilled_0200
 * @tc.desc: OnBufferFilled
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnBufferFilled_0200, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    muxerFilter_->GetParameter(meta);
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->isTransCoderMode = false;
    muxerFilter_->preFilterCount_ = 0;
    muxerFilter_->maxDuration_ = 100;
    inputBuffer->pts_ = 3000000000;
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(inputBuffer->flag_, 0);
}

/**
 * @tc.name: MuxerFilter_OnTransCoderBufferFilled_0100
 * @tc.desc: OnTransCoderBufferFilled
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnTransCoderBufferFilled_0100, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->flag_ = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->videoIsEos, true);
    streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->audioIsEos, true);
}

/**
 * @tc.name: MuxerFilter_OnTransCoderBufferFilled_0200
 * @tc.desc: OnTransCoderBufferFilled
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnTransCoderBufferFilled_0200, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->preFilterCount_ = 0;
    muxerFilter_->videoIsEos = true;
    muxerFilter_->audioIsEos = false;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    inputBuffer->flag_ = 0;
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->preFilterCount_ = 1;
    muxerFilter_->videoIsEos = true;
    muxerFilter_->audioIsEos = true;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    muxerFilter_->lastAudioPts_ = 1;
    inputBuffer->pts_ = 0;
    muxerFilter_->videoIsEos = true;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(inputBuffer->pts_, muxerFilter_->lastAudioPts_);
    inputBuffer->pts_ = 1000;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(inputBuffer->pts_, muxerFilter_->lastAudioPts_);
}

/**
 * @tc.name: MuxerFilter_OnTransCoderBufferFilled_0300
 * @tc.desc: OnTransCoderBufferFilled
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnTransCoderBufferFilled_0300, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->preFilterCount_ = 0;
    inputBuffer->flag_ = 0;
    muxerFilter_->videoIsEos = true;
    muxerFilter_->audioIsEos = true;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->lastVideoPts_, inputBuffer->pts_);
}

/**
 * @tc.name: MuxerFilter_OnTransCoderBufferFilled_0400
 * @tc.desc: OnTransCoderBufferFilled
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnTransCoderBufferFilled_0400, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->preFilterCount_ = 0;
    inputBuffer->flag_ = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    streamType = StreamType::STREAMTYPE_MAX;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->videoIsEos, true);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnUnLinked_0400, TestSize.Level1)
{
    std::shared_ptr<MyFilterLinkCallback> callback = std::make_shared<MyFilterLinkCallback>();
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    Status status = muxerFilter_->OnUnLinked(streamType, callback);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnUpdated_0400, TestSize.Level1)
{
    std::shared_ptr<MyFilterLinkCallback> callback = std::make_shared<MyFilterLinkCallback>();
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    Status status = muxerFilter_->OnUpdated(streamType, userMeta, callback);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_GetFilterType_0400, TestSize.Level1)
{
    FilterType filterType = muxerFilter_->GetFilterType();
    EXPECT_EQ(filterType, FilterType::FILTERTYPE_MUXER);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_UnLinkNext_0400, TestSize.Level1)
{
    std::shared_ptr<TestFilter> filter = std::make_shared<TestFilter>();
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    Status status = muxerFilter_->UnLinkNext(filter, streamType);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_UpdateNext_0400, TestSize.Level1)
{
    std::shared_ptr<TestFilter> filter = std::make_shared<TestFilter>();
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    Status status = muxerFilter_->UpdateNext(filter, streamType);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_LinkNext_0400, TestSize.Level1)
{
    std::shared_ptr<TestFilter> filter = std::make_shared<TestFilter>();
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    Status status = muxerFilter_->LinkNext(filter, streamType);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoRelease_0400, TestSize.Level1)
{
    Status status = muxerFilter_->DoRelease();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoFlush_0400, TestSize.Level1)
{
    Status status = muxerFilter_->DoFlush();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoResume_0400, TestSize.Level1)
{
    Status status = muxerFilter_->DoResume();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoPause_0400, TestSize.Level1)
{
    Status status = muxerFilter_->DoPause();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoPrepare_0400, TestSize.Level1)
{
    Status status = muxerFilter_->DoPrepare();
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetTransCoderMode_0400, TestSize.Level1)
{
    Status status = muxerFilter_->SetTransCoderMode();
    EXPECT_EQ(status, Status::OK);
    EXPECT_TRUE(muxerFilter_->isTransCoderMode);
}

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetCallingInfo_0400, TestSize.Level1)
{
    int32_t appUid = 1;
    int32_t appPid = 1;
    std::string bundleName = "test";
    uint64_t instanceId = 1;
    muxerFilter_->SetCallingInfo(appUid, appPid, bundleName, instanceId);
    EXPECT_EQ(appUid, muxerFilter_->appUid_);
    EXPECT_EQ(appPid, muxerFilter_->appPid_);
    EXPECT_EQ(bundleName, muxerFilter_->bundleName_);
    EXPECT_EQ(instanceId, muxerFilter_->instanceId_);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS