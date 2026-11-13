/*
 * Copyright (C) 2024-2026 Huawei Device Co., Ltd.
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
    // videoCodecMimeType_ is empty (audio-only mode), so GetCurrentPtsMs returns audio PTS
    muxerFilter_->lastVideoPts_.store(1000);
    EXPECT_EQ(muxerFilter_->GetCurrentPtsMs(), muxerFilter_->lastAudioPts_.load() / 1000);
    muxerFilter_->lastVideoPts_.store(0);
    muxerFilter_->lastAudioPts_.store(1000);
    EXPECT_EQ(muxerFilter_->GetCurrentPtsMs(), muxerFilter_->lastAudioPts_.load() / 1000);
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
    EXPECT_EQ(muxerFilter_->isStarted.load(), true);
    EXPECT_EQ(muxerFilter_->DoStart(), Status::OK);
    EXPECT_EQ(muxerFilter_->isStarted.load(), true);
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
    EXPECT_EQ(muxerFilter_->isStarted.load(), true);
    
    muxerFilter_->stopCount_.store(0);
    muxerFilter_->preFilterCount_.store(2);

    EXPECT_EQ(muxerFilter_->DoStop(), Status::OK);
    EXPECT_EQ(muxerFilter_->isStarted.load(), true);
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::NO_ERROR));
    EXPECT_EQ(muxerFilter_->DoStop(), Status::OK);
    EXPECT_EQ(muxerFilter_->isStarted.load(), false);

    muxerFilter_->stopCount_.store(0);
    muxerFilter_->preFilterCount_.store(1);
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::ERROR_WRONG_STATE));
    EXPECT_EQ(muxerFilter_->DoStop(), Status::OK);

    muxerFilter_->stopCount_.store(0);
    muxerFilter_->preFilterCount_.store(1);
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::ERROR_UNKNOWN));
    EXPECT_EQ(muxerFilter_->DoStop(), Status::ERROR_MUXER_STOP_FAILED);
}

/**
 * @tc.name: MuxerFilter_DoStop_0200
 * @tc.desc: DoStop if (newCount != preFilterCount_) TRUE branch - returns OK without calling Stop
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStop_0200, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->isStarted.store(true);
    muxerFilter_->stopCount_.store(0);
    muxerFilter_->preFilterCount_.store(2);
    EXPECT_EQ(muxerFilter_->DoStop(), Status::OK);
    EXPECT_EQ(muxerFilter_->isStarted.load(), true);
}

/**
 * @tc.name: MuxerFilter_DoStop_0300
 * @tc.desc: DoStop if#1 FALSE + if (ret == Status::OK) TRUE branch - sets isStarted false
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStop_0300, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->isStarted.store(true);
    muxerFilter_->stopCount_.store(0);
    muxerFilter_->preFilterCount_.store(1);
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::OK));
    EXPECT_EQ(muxerFilter_->DoStop(), Status::OK);
    EXPECT_EQ(muxerFilter_->isStarted.load(), false);
}

/**
 * @tc.name: MuxerFilter_DoStop_0400
 * @tc.desc: DoStop if#1 FALSE + else-if (ret == ERROR_WRONG_STATE) TRUE branch
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStop_0400, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->isStarted.store(true);
    muxerFilter_->stopCount_.store(0);
    muxerFilter_->preFilterCount_.store(1);
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::ERROR_WRONG_STATE));
    EXPECT_EQ(muxerFilter_->DoStop(), Status::OK);
}

/**
 * @tc.name: MuxerFilter_DoStop_0500
 * @tc.desc: DoStop if#1 FALSE + else branch (ret is other error)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStop_0500, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->isStarted.store(true);
    muxerFilter_->stopCount_.store(0);
    muxerFilter_->preFilterCount_.store(1);
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::ERROR_UNKNOWN));
    EXPECT_EQ(muxerFilter_->DoStop(), Status::ERROR_MUXER_STOP_FAILED);
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
    muxerFilter_->isTransCoderMode.store(false);
    muxerFilter_->preFilterCount_.store(0);
    inputBuffer->pts_ = 3000000000;
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->bufferPtsMap_[trackIndex], inputBuffer->pts_);
    muxerFilter_->isTransCoderMode.store(true);
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->lastVideoPts_.load(), inputBuffer->pts_);
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->lastVideoPts_.load(), inputBuffer->pts_);
    muxerFilter_->EventCompleteStopAsync();
    EXPECT_EQ(muxerFilter_->lastVideoPts_.load(), inputBuffer->pts_);
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
    EXPECT_EQ(muxerFilter_->videoIsEos.load(), true);
    streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->audioIsEos.load(), true);
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
    muxerFilter_->preFilterCount_.store(0);
    muxerFilter_->videoIsEos.store(true);
    muxerFilter_->audioIsEos.store(false);
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
    inputBuffer->pts_ = 5000000;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->lastAudioPts_.load(), 5000000);
    inputBuffer->flag_ = 0;
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->preFilterCount_.store(1);
    muxerFilter_->videoIsEos.store(true);
    muxerFilter_->audioIsEos.store(true);
    inputBuffer->pts_ = 7000000;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->lastAudioPts_.load(), 7000000);
    muxerFilter_->lastAudioPts_.store(1);
    inputBuffer->pts_ = 0;
    muxerFilter_->videoIsEos.store(true);
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(inputBuffer->pts_, muxerFilter_->lastAudioPts_.load());
    inputBuffer->pts_ = 1000;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(inputBuffer->pts_, muxerFilter_->lastAudioPts_.load());
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
    muxerFilter_->preFilterCount_.store(0);
    inputBuffer->flag_ = 0;
    muxerFilter_->videoIsEos.store(true);
    muxerFilter_->audioIsEos.store(true);
    muxerFilter_->lastVideoPts_.store(1000);
    inputBuffer->pts_ = 500;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->lastVideoPts_.load(), 1000);
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
    muxerFilter_->preFilterCount_.store(0);
    inputBuffer->flag_ = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    streamType = StreamType::STREAMTYPE_MAX;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->videoIsEos.load(), true);
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
    EXPECT_TRUE(muxerFilter_->isTransCoderMode.load());
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

HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetOrGetParameter_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> receivedMeta = std::make_shared<Meta>();
    muxerFilter_->GetParameter(receivedMeta);
    ASSERT_NE(receivedMeta, nullptr);
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    muxerFilter_->SetParameter(userMeta);
    EXPECT_NE(muxerFilter_->mediaMuxer_->SetParameter(userMeta), Status::OK);
}

/**
 * @tc.name: MuxerFilter_ConfigureTrackMeta_0100
 * @tc.desc: ConfigureTrackMeta with audio MIME type
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_ConfigureTrackMeta_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::MIME_TYPE>(std::string("audio/mp4a-latm"));
    muxerFilter_->ConfigureTrackMeta(meta, "audio/mp4a-latm");
    EXPECT_EQ(muxerFilter_->audioCodecMimeType_, "audio/mp4a-latm");
}

/**
 * @tc.name: MuxerFilter_ConfigureTrackMeta_0200
 * @tc.desc: ConfigureTrackMeta with video MIME type
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_ConfigureTrackMeta_0200, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    muxerFilter_->ConfigureTrackMeta(meta, "video/avc");
    EXPECT_EQ(muxerFilter_->videoCodecMimeType_, "video/avc");
}

/**
 * @tc.name: MuxerFilter_ConfigureTrackMeta_0300
 * @tc.desc: ConfigureTrackMeta with metadata MIME type
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_ConfigureTrackMeta_0300, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    muxerFilter_->ConfigureTrackMeta(meta, "meta/timed");
    EXPECT_EQ(muxerFilter_->metaDataCodecMimeType_, "meta/timed");
}

/**
 * @tc.name: MuxerFilter_OnBufferFilled_MinOtherPts_0100
 * @tc.desc: OnBufferFilled with multiple tracks, should use min pts of other tracks for comparison
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnBufferFilled_MinOtherPts_0100, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    muxerFilter_->isTransCoderMode.store(false);
    muxerFilter_->preFilterCount_.store(2);
    muxerFilter_->isReachMaxDuration_.store(false);
    muxerFilter_->maxDuration_ = INT32_MAX;
    // Set up bufferPtsMap_ with two other tracks: one with small pts, one with large pts
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        muxerFilter_->bufferPtsMap_[0] = 1000000;   // small pts (1ms)
        muxerFilter_->bufferPtsMap_[2] = 5000000000; // large pts (5s)
    }
    // Current buffer has pts between them - should compare against min (1000000)
    inputBuffer->pts_ = 4000000000; // 4s, diff from min = ~4s > 3s threshold
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    int64_t capturedPts = 0;
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        capturedPts = muxerFilter_->bufferPtsMap_[trackIndex];
    }
    EXPECT_EQ(capturedPts, inputBuffer->pts_);
}

/**
 * @tc.name: MuxerFilter_OnBufferFilled_MinOtherPts_0200
 * @tc.desc: OnBufferFilled with single track (preFilterCount_ = 1), no pts comparison
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnBufferFilled_MinOtherPts_0200, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 0;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    muxerFilter_->isTransCoderMode.store(false);
    muxerFilter_->preFilterCount_.store(1);
    muxerFilter_->isReachMaxDuration_.store(false);
    muxerFilter_->maxDuration_ = INT32_MAX;
    inputBuffer->pts_ = 1000000;
    // No other tracks in bufferPtsMap_, minOtherPts remains INT64_MAX, no log triggered
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    int64_t capturedPts = 0;
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        capturedPts = muxerFilter_->bufferPtsMap_[trackIndex];
    }
    EXPECT_EQ(capturedPts, inputBuffer->pts_);
}

/**
 * @tc.name: MuxerFilter_OnBufferFilled_SameTrackInMap_0100
 * @tc.desc: OnBufferFilled with bufferPtsMap_ containing same trackIndex, covers false branch of key comparison
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnBufferFilled_SameTrackInMap_0100, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    muxerFilter_->isTransCoderMode.store(false);
    muxerFilter_->preFilterCount_.store(2);
    muxerFilter_->isReachMaxDuration_.store(false);
    muxerFilter_->maxDuration_ = INT32_MAX;
    // Set up bufferPtsMap_ with same trackIndex and another track
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        muxerFilter_->bufferPtsMap_[0] = 500000;    // other track, key != trackIndex -> true branch
        muxerFilter_->bufferPtsMap_[1] = 800000;    // same track, key == trackIndex -> false branch
    }
    inputBuffer->pts_ = 1000000;
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    int64_t capturedPts = 0;
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        capturedPts = muxerFilter_->bufferPtsMap_[trackIndex];
    }
    EXPECT_EQ(capturedPts, inputBuffer->pts_);
}

/**
 * @tc.name: MuxerFilter_SetClipStartTimeUs_0100
 * @tc.desc: SetClipStartTimeUs sets the clip start time member
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetClipStartTimeUs_0100, TestSize.Level1)
{
    int64_t expected = 5000000;
    muxerFilter_->SetClipStartTimeUs(expected);
    EXPECT_EQ(muxerFilter_->clipStartTimeUs_, expected);
}

/**
 * @tc.name: MuxerFilter_SetClipEndTimeUs_0100
 * @tc.desc: SetClipEndTimeUs sets the clip end time member
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetClipEndTimeUs_0100, TestSize.Level1)
{
    int64_t expected = 5000000;
    muxerFilter_->SetClipEndTimeUs(expected);
    EXPECT_EQ(muxerFilter_->clipEndTimeUs_, expected);
}

/**
 * @tc.name: MuxerFilter_GetCurrentPtsMs_AudioOnly_0100
 * @tc.desc: GetCurrentPtsMs in audio-only mode uses audio PTS
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_GetCurrentPtsMs_AudioOnly_0100, TestSize.Level1)
{
    muxerFilter_->SetOutputParameter(0, 0, 0, 0);
    muxerFilter_->videoCodecMimeType_.clear();
    muxerFilter_->lastAudioPts_.store(2000);
    EXPECT_EQ(muxerFilter_->GetCurrentPtsMs(), 2);
    muxerFilter_->lastAudioPts_.store(0);
    EXPECT_EQ(muxerFilter_->GetCurrentPtsMs(), 0);
}

/**
 * @tc.name: MuxerFilter_GetCurrentPtsMs_WithVideo_0100
 * @tc.desc: GetCurrentPtsMs with video track uses video PTS even if audio PTS is non-zero
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_GetCurrentPtsMs_WithVideo_0100, TestSize.Level1)
{
    muxerFilter_->videoCodecMimeType_ = "video/hevc";
    muxerFilter_->lastVideoPts_.store(3000);
    muxerFilter_->lastAudioPts_.store(5000);
    EXPECT_EQ(muxerFilter_->GetCurrentPtsMs(), 3);
}

/**
 * @tc.name: MuxerFilter_FilterPreClipBuffers_0100
 * @tc.desc: FilterPreClipBuffers discards audio frame before clip start
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_FilterPreClipBuffers_0100, TestSize.Level1)
{
    muxerFilter_->clipStartTimeUs_ = 1000000;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    // Frame before clip start: pts < clipStartTimeUs_ -> should be filtered
    inputBuffer->flag_ = 0;
    inputBuffer->pts_ = 500000;
    bool consumed = muxerFilter_->FilterPreClipBuffers(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, inputBufferQueue);
    EXPECT_TRUE(consumed);
    EXPECT_EQ(muxerFilter_->audioClipFilteredCount_.load(), 1);
}

/**
 * @tc.name: MuxerFilter_FilterPreClipBuffers_0200
 * @tc.desc: FilterPreClipBuffers keeps CODEC_CONFIG frames (not filtered)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_FilterPreClipBuffers_0200, TestSize.Level1)
{
    muxerFilter_->clipStartTimeUs_ = 1000000;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    // CODEC_CONFIG frame (flag bit 3 = 1 = 0x08) should NOT be filtered
    inputBuffer->flag_ = 0x08;
    inputBuffer->pts_ = 500000;
    bool consumed = muxerFilter_->FilterPreClipBuffers(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, inputBufferQueue);
    EXPECT_FALSE(consumed);
}

/**
 * @tc.name: MuxerFilter_FilterPreClipBuffers_0300
 * @tc.desc: FilterPreClipBuffers keeps EOS frames (not filtered)
 * @type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_FilterPreClipBuffers_0300, TestSize.Level1)
{
    muxerFilter_->clipStartTimeUs_ = 1000000;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    // EOS frame (flag bit 0 = 1 = 0x01) should NOT be filtered
    inputBuffer->flag_ = 0x01;
    inputBuffer->pts_ = 500000;
    bool consumed = muxerFilter_->FilterPreClipBuffers(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, inputBufferQueue);
    EXPECT_FALSE(consumed);
}

/**
 * @tc.name: MuxerFilter_FilterPreClipBuffers_0400
 * @tc.desc: FilterPreClipBuffers returns false when clipStartTimeUs_ is 0
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_FilterPreClipBuffers_0400, TestSize.Level1)
{
    muxerFilter_->clipStartTimeUs_ = 0;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->flag_ = 0;
    inputBuffer->pts_ = 1000000;
    bool consumed = muxerFilter_->FilterPreClipBuffers(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, inputBufferQueue);
    EXPECT_FALSE(consumed);
}

/**
 * @tc.name: MuxerFilter_HandleClipEnd_0100
 * @tc.desc: HandleClipEnd discards further buffers when track already EOS
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleClipEnd_0100, TestSize.Level1)
{
    muxerFilter_->clipEndTimeUs_ = 5000000;
    muxerFilter_->videoIsEos.store(true);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 6000000;
    bool consumed = muxerFilter_->HandleClipEnd(inputBuffer, StreamType::STREAMTYPE_ENCODED_VIDEO, inputBufferQueue);
    EXPECT_TRUE(consumed);
}

/**
 * @tc.name: MuxerFilter_HandleClipEnd_0200
 * @tc.desc: HandleClipEnd discards data and sends EOS when pts > clipEndTime
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleClipEnd_0200, TestSize.Level1)
{
    muxerFilter_->clipEndTimeUs_ = 5000000;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 5010000;
    bool consumed = muxerFilter_->HandleClipEnd(inputBuffer, StreamType::STREAMTYPE_ENCODED_VIDEO, inputBufferQueue);
    EXPECT_FALSE(consumed);
    EXPECT_TRUE((inputBuffer->flag_ & 0x01) != 0);
}

/**
 * @tc.name: MuxerFilter_HandleClipEnd_0300
 * @tc.desc: HandleClipEnd returns false when clipEndTimeUs_ is 0
 * @type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleClipEnd_0300, TestSize.Level1)
{
    muxerFilter_->clipEndTimeUs_ = 0;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    bool consumed = muxerFilter_->HandleClipEnd(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, inputBufferQueue);
    EXPECT_FALSE(consumed);
}

/**
 * @tc.name: MuxerFilter_AdjustPtsForClip_0100
 * @tc.desc: AdjustPtsForClip zeroes video CODEC_CONFIG frames
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_AdjustPtsForClip_0100, TestSize.Level1)
{
    muxerFilter_->clipStartTimeUs_ = 1000000;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    inputBuffer->flag_ = 0x08;  // CODEC_CONFIG
    inputBuffer->pts_ = 5000;
    muxerFilter_->AdjustPtsForClip(inputBuffer, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(inputBuffer->pts_, 0);
}

/**
 * @tc.name: MuxerFilter_AdjustPtsForClip_0200
 * @tc.desc: AdjustPtsForClip offsets non-CODEC_CONFIG audio frame
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_AdjustPtsForClip_0200, TestSize.Level1)
{
    muxerFilter_->clipStartTimeUs_ = 1000000;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    inputBuffer->flag_ = 0;
    inputBuffer->pts_ = 1500000;
    muxerFilter_->AdjustPtsForClip(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO);
    EXPECT_EQ(inputBuffer->pts_, 500000);
}

/**
 * @tc.name: MuxerFilter_AdjustPtsForClip_0300
 * @tc.desc: AdjustPtsForClip returns early when clipStartTimeUs_ is 0
 * @type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_AdjustPtsForClip_0300, TestSize.Level1)
{
    muxerFilter_->clipStartTimeUs_ = 0;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    inputBuffer->flag_ = 0;
    inputBuffer->pts_ = 1000000;
    muxerFilter_->AdjustPtsForClip(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO);
    EXPECT_EQ(inputBuffer->pts_, 1000000);
}

/**
 * @tc.name: MuxerFilter_IsTrackEos_0100
 * @tc.desc: IsTrackEos checks video and audio EOS state
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_IsTrackEos_0100, TestSize.Level1)
{
    muxerFilter_->videoIsEos.store(false);
    muxerFilter_->audioIsEos.store(false);
    EXPECT_FALSE(muxerFilter_->IsTrackEos(StreamType::STREAMTYPE_ENCODED_VIDEO));
    EXPECT_FALSE(muxerFilter_->IsTrackEos(StreamType::STREAMTYPE_ENCODED_AUDIO));

    muxerFilter_->videoIsEos.store(true);
    EXPECT_TRUE(muxerFilter_->IsTrackEos(StreamType::STREAMTYPE_ENCODED_VIDEO));

    muxerFilter_->audioIsEos.store(true);
    EXPECT_TRUE(muxerFilter_->IsTrackEos(StreamType::STREAMTYPE_ENCODED_AUDIO));

    // Unknown stream type never reports EOS
    EXPECT_FALSE(muxerFilter_->IsTrackEos(StreamType::STREAMTYPE_RAW_AUDIO));
}

/**
 * @tc.name: MuxerFilter_HandleTransCoderEos_0100
 * @tc.desc: HandleTransCoderEos sets isCompleted when both tracks are EOS
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleTransCoderEos_0100, TestSize.Level1)
{
    muxerFilter_->preFilterCount_.store(2);
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->videoIsEos.store(false);
    muxerFilter_->audioIsEos.store(false);

    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    inputBuffer->flag_ = 0x01;  // BUFFER_IS_EOS set so HandleTransCoderEos proceeds

    // Audio EOS: not completed (video not yet)
    bool isCompleted = false;
    muxerFilter_->HandleTransCoderEos(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, isCompleted);
    EXPECT_FALSE(isCompleted);
    EXPECT_TRUE(muxerFilter_->audioIsEos.load());

    // Video EOS: should be completed
    isCompleted = false;
    muxerFilter_->HandleTransCoderEos(inputBuffer, StreamType::STREAMTYPE_ENCODED_VIDEO, isCompleted);
    EXPECT_TRUE(isCompleted);
    EXPECT_TRUE(muxerFilter_->videoIsEos.load());
}

/**
 * @tc.name: MuxerFilter_HandleTransCoderEos_0200
 * @tc.desc: HandleTransCoderEos sets isCompleted when eosCount equals preFilterCount
 * @type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleTransCoderEos_0200, TestSize.Level1)
{
    muxerFilter_->preFilterCount_.store(1);
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->videoIsEos.store(true);
    muxerFilter_->audioIsEos.store(false);

    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    inputBuffer->flag_ = 0x01;  // BUFFER_IS_EOS set so HandleTransCoderEos proceeds

    bool isCompleted = false;
    muxerFilter_->HandleTransCoderEos(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, isCompleted);
    EXPECT_TRUE(isCompleted);
}

/**
 * @tc.name: MuxerFilter_WriteTransCoderBuffer_0100
 * @tc.desc: WriteTransCoderBuffer writes audio buffer in audio-only mode (videoCodecMimeType_ is empty)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_WriteTransCoderBuffer_0100, TestSize.Level1)
{
    muxerFilter_->videoCodecMimeType_.clear();
    muxerFilter_->lastVideoPts_.store(0);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 2000000;
    muxerFilter_->WriteTransCoderBuffer(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->lastAudioPts_.load(), 2000000);
}

/**
 * @tc.name: MuxerFilter_WriteTransCoderBuffer_0200
 * @tc.desc: WriteTransCoderBuffer returns early when streamType is neither audio nor video
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_WriteTransCoderBuffer_0200, TestSize.Level1)
{
    muxerFilter_->lastAudioPts_.store(0);
    muxerFilter_->lastVideoPts_.store(0);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 1000000;
    // STREAMTYPE_RAW_AUDIO is neither ENCODED_AUDIO nor ENCODED_VIDEO
    muxerFilter_->WriteTransCoderBuffer(inputBuffer, StreamType::STREAMTYPE_RAW_AUDIO, inputBufferQueue);
    // Neither lastAudioPts_ nor lastVideoPts_ should be updated for unknown stream type
    EXPECT_EQ(muxerFilter_->lastAudioPts_.load(), 0);
    EXPECT_EQ(muxerFilter_->lastVideoPts_.load(), 0);
}

/**
 * @tc.name: MuxerFilter_OnTransCoderBufferFilled_0500
 * @tc.desc: OnTransCoderBufferFilled returns early when inputBuffer is nullptr (null guard)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnTransCoderBufferFilled_0500, TestSize.Level1)
{
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    int32_t trackIndex = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    std::shared_ptr<AVBuffer> nullBuffer = nullptr;
    // Should return early without crash due to null guard
    muxerFilter_->OnTransCoderBufferFilled(nullBuffer, trackIndex, streamType, inputBufferQueue);
    // No crash, no side effects — eosCount unchanged
    EXPECT_EQ(muxerFilter_->eosCount_.load(), 0);
}

/**
 * @tc.name: MuxerFilter_OnTransCoderBufferFilled_0600
 * @tc.desc: OnTransCoderBufferFilled returns early when inputBufferQueue is nullptr (null guard)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnTransCoderBufferFilled_0600, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    sptr<AVBufferQueueProducer> nullQueue = nullptr;
    // Should return early without crash due to null guard
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, nullQueue);
    // No crash, no side effects — eosCount unchanged
    EXPECT_EQ(muxerFilter_->eosCount_.load(), 0);
}

/**
 * @tc.name: MuxerFilter_WriteTransCoderBuffer_0300
 * @tc.desc: WriteTransCoderBuffer writes video buffer and updates lastVideoPts_
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_WriteTransCoderBuffer_0300, TestSize.Level1)
{
    muxerFilter_->videoIsEos.store(false);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 5000000;
    muxerFilter_->WriteTransCoderBuffer(inputBuffer, StreamType::STREAMTYPE_ENCODED_VIDEO, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->lastVideoPts_.load(), 5000000);
}

/**
 * @tc.name: MuxerFilter_WriteTransCoderBuffer_0400
 * @tc.desc: WriteTransCoderBuffer video does not update lastVideoPts_ when videoIsEos is true
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_WriteTransCoderBuffer_0400, TestSize.Level1)
{
    muxerFilter_->videoIsEos.store(true);
    muxerFilter_->lastVideoPts_.store(3000000);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 9000000;
    muxerFilter_->WriteTransCoderBuffer(inputBuffer, StreamType::STREAMTYPE_ENCODED_VIDEO, inputBufferQueue);
    // lastVideoPts_ should remain unchanged because videoIsEos is true
    EXPECT_EQ(muxerFilter_->lastVideoPts_.load(), 3000000);
}

/**
 * @tc.name: MuxerFilter_WriteTransCoderBuffer_0500
 * @tc.desc: WriteTransCoderBuffer audio in video+audio mode returns buffer when pts <= lastVideoPts
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_WriteTransCoderBuffer_0500, TestSize.Level1)
{
    muxerFilter_->videoCodecMimeType_ = "video/avc";
    muxerFilter_->videoIsEos.store(false);
    muxerFilter_->lastVideoPts_.store(5000000);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 3000000; // audio pts <= lastVideoPts + SYNC_THRESHOLD_US
    muxerFilter_->WriteTransCoderBuffer(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->lastAudioPts_.load(), 3000000);
}

/**
 * @tc.name: MuxerFilter_WriteTransCoderBuffer_0600
 * @tc.desc: WriteTransCoderBuffer audio returns buffer when videoIsEos is true (no wait)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_WriteTransCoderBuffer_0600, TestSize.Level1)
{
    muxerFilter_->videoCodecMimeType_ = "video/avc";
    muxerFilter_->videoIsEos.store(true);
    muxerFilter_->lastVideoPts_.store(0);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 8000000; // audio ahead, but videoIsEos so no wait
    muxerFilter_->WriteTransCoderBuffer(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->lastAudioPts_.load(), 8000000);
}

/**
 * @tc.name: MuxerFilter_HandleTransCoderEos_0300
 * @tc.desc: HandleTransCoderEos returns early when EOS flag is not set
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleTransCoderEos_0300, TestSize.Level1)
{
    muxerFilter_->preFilterCount_.store(2);
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->videoIsEos.store(false);
    muxerFilter_->audioIsEos.store(false);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    inputBuffer->flag_ = 0; // No EOS flag — should return early
    bool isCompleted = false;
    muxerFilter_->HandleTransCoderEos(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, isCompleted);
    // isCompleted unchanged (FALSE_RETURN_NOLOG does not modify it), eosCount unchanged
    EXPECT_FALSE(isCompleted);
    EXPECT_EQ(muxerFilter_->eosCount_.load(), 0);
    EXPECT_FALSE(muxerFilter_->audioIsEos.load());
}

/**
 * @tc.name: MuxerFilter_HandleClipEnd_0400
 * @tc.desc: HandleClipEnd audio discards data and sets EOS when pts > clipEndTime
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleClipEnd_0400, TestSize.Level1)
{
    muxerFilter_->clipEndTimeUs_ = 5000000;
    muxerFilter_->videoIsEos.store(false);
    muxerFilter_->audioIsEos.store(false);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 6000000; // pts > clipEndTimeUs_
    inputBuffer->flag_ = 0;
    bool consumed = muxerFilter_->HandleClipEnd(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO, inputBufferQueue);
    EXPECT_FALSE(consumed); // Returns false (does not consume — sends EOS-tagged buffer downstream)
    EXPECT_TRUE((inputBuffer->flag_ & 0x01) != 0);
}

/**
 * @tc.name: MuxerFilter_HandleClipEnd_0500
 * @tc.desc: HandleClipEnd sets EOS on buffer with null memory_ (no crash)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleClipEnd_0500, TestSize.Level1)
{
    muxerFilter_->clipEndTimeUs_ = 5000000;
    muxerFilter_->videoIsEos.store(false);
    muxerFilter_->audioIsEos.store(false);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 6000000;
    inputBuffer->flag_ = 0;
    inputBuffer->memory_ = nullptr; // null memory_ — should not crash
    bool consumed = muxerFilter_->HandleClipEnd(inputBuffer, StreamType::STREAMTYPE_ENCODED_VIDEO, inputBufferQueue);
    EXPECT_FALSE(consumed);
    EXPECT_TRUE((inputBuffer->flag_ & 0x01) != 0);
}

/**
 * @tc.name: MuxerFilter_AdjustPtsForClip_0400
 * @tc.desc: AdjustPtsForClip audio frame with CODEC_CONFIG flag still offsets PTS (not zeroed)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_AdjustPtsForClip_0400, TestSize.Level1)
{
    muxerFilter_->clipStartTimeUs_ = 1000000;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    inputBuffer->pts_ = 3000000;
    inputBuffer->flag_ = 0x02; // BUFFER_IS_CODEC_CONFIG — audio should still offset, not zero
    muxerFilter_->AdjustPtsForClip(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO);
    // Audio CODEC_CONFIG is a data frame with valid PTS — should be offset, not zeroed
    EXPECT_EQ(inputBuffer->pts_, 2000000);
}

/**
 * @tc.name: MuxerFilter_AdjustPtsForClip_0500
 * @tc.desc: AdjustPtsForClip clamps PTS to 0 when pts < clipStartTimeUs_
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_AdjustPtsForClip_0500, TestSize.Level1)
{
    muxerFilter_->clipStartTimeUs_ = 5000000;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    inputBuffer->pts_ = 2000000; // pts < clipStartTimeUs_ → std::max(2000000-5000000, 0) = 0
    inputBuffer->flag_ = 0;
    muxerFilter_->AdjustPtsForClip(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO);
    EXPECT_EQ(inputBuffer->pts_, 0);
}

/**
 * @tc.name: MuxerFilter_FilterPreClipBuffers_0500
 * @tc.desc: FilterPreClipBuffers discards video frame before clip start (non-audio else branch)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_FilterPreClipBuffers_0500, TestSize.Level1)
{
    muxerFilter_->clipStartTimeUs_ = 1000000;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    inputBuffer->pts_ = 500000; // pts < clipStartTimeUs_
    inputBuffer->flag_ = 0;
    bool consumed = muxerFilter_->FilterPreClipBuffers(inputBuffer, StreamType::STREAMTYPE_ENCODED_VIDEO, inputBufferQueue);
    EXPECT_TRUE(consumed);
}

/**
 * @tc.name: MuxerFilter_LogFirstAudioAfterClip_0100
 * @tc.desc: LogFirstAudioAfterClip increments passedCount and returns early for non-audio
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_LogFirstAudioAfterClip_0100, TestSize.Level1)
{
    muxerFilter_->audioClipPassedCount_.store(0);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    inputBuffer->pts_ = 1000000;
    // Non-audio streamType — should return early, passedCount unchanged
    muxerFilter_->LogFirstAudioAfterClip(inputBuffer, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(muxerFilter_->audioClipPassedCount_.load(), 0);
    // Audio streamType — passedCount increments
    muxerFilter_->LogFirstAudioAfterClip(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO);
    EXPECT_EQ(muxerFilter_->audioClipPassedCount_.load(), 1);
    // Second call — passed > 1, no first-frame log but count still increments
    muxerFilter_->LogFirstAudioAfterClip(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO);
    EXPECT_EQ(muxerFilter_->audioClipPassedCount_.load(), 2);
}

/**
 * @tc.name: MuxerFilter_UpdateTrackEosState_0100
 * @tc.desc: UpdateTrackEosState sets videoIsEos and audioIsEos correctly
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_UpdateTrackEosState_0100, TestSize.Level1)
{
    muxerFilter_->videoIsEos.store(false);
    muxerFilter_->audioIsEos.store(false);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    inputBuffer->pts_ = 5000000;
    muxerFilter_->UpdateTrackEosState(inputBuffer, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_TRUE(muxerFilter_->videoIsEos.load());
    EXPECT_FALSE(muxerFilter_->audioIsEos.load());
    muxerFilter_->UpdateTrackEosState(inputBuffer, StreamType::STREAMTYPE_ENCODED_AUDIO);
    EXPECT_TRUE(muxerFilter_->audioIsEos.load());
    // Unknown stream type — no state change, no crash
    muxerFilter_->videoIsEos.store(false);
    muxerFilter_->UpdateTrackEosState(inputBuffer, StreamType::STREAMTYPE_RAW_AUDIO);
    EXPECT_FALSE(muxerFilter_->videoIsEos.load());
}

/**
 * @tc.name: MuxerFilter_OnBufferFilled_PtsInterval_0100
 * @tc.desc: OnBufferFilled if (preFilterCount_ != 1 && abs(pts diff) >= WAIT_TIME_OUT_NS) TRUE branch
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnBufferFilled_PtsInterval_0100, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    muxerFilter_->isTransCoderMode.store(false);
    muxerFilter_->preFilterCount_.store(2);
    muxerFilter_->isReachMaxDuration_.store(false);
    muxerFilter_->maxDuration_ = INT32_MAX;
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        muxerFilter_->bufferPtsMap_[0] = 0;
    }
    inputBuffer->pts_ = 4000000000; // 4s, diff from track 0 (0) = 4s >= 3s (WAIT_TIME_OUT_NS)
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    int64_t capturedPts = 0;
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        capturedPts = muxerFilter_->bufferPtsMap_[trackIndex];
    }
    EXPECT_EQ(capturedPts, inputBuffer->pts_);
}
 
/**
 * @tc.name: MuxerFilter_OnBufferFilled_PtsInterval_0200
 * @tc.desc: OnBufferFilled if (preFilterCount_ != 1 && abs(pts diff) >= WAIT_TIME_OUT_NS) FALSE branch
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnBufferFilled_PtsInterval_0200, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    muxerFilter_->isTransCoderMode.store(false);
    muxerFilter_->preFilterCount_.store(2);
    muxerFilter_->isReachMaxDuration_.store(false);
    muxerFilter_->maxDuration_ = INT32_MAX;
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        muxerFilter_->bufferPtsMap_[0] = 0;
    }
    inputBuffer->pts_ = 1000000000; // 1s, diff from track 0 (0) = 1s < 3s (WAIT_TIME_OUT_NS)
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    int64_t capturedPts = 0;
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        capturedPts = muxerFilter_->bufferPtsMap_[trackIndex];
    }
    EXPECT_EQ(capturedPts, inputBuffer->pts_);
}

// Counting AVBufferQueueProducer to verify whether ReturnBuffer is called (evidence of early-return guard)
class CountingAVBufferQueueProducer : public OHOS::Media::Pipeline::MyAVBufferQueueProducer {
public:
    int32_t returnBufferCount_ = 0;
    Status ReturnBuffer(const std::shared_ptr<AVBuffer>& inBuffer, bool available) override
    {
        returnBufferCount_++;
        return Status::OK;
    }
};

/**
 * @tc.name: MuxerFilter_OnBufferFilled_InputBufferNull_0200
 * @tc.desc: OnBufferFilled L424 inputBuffer==nullptr guard hits early return: no new key in bufferPtsMap_,
 *           counting queue ReturnBuffer not called. Safety remediation null guard blocks L432-456 main path —
 *           otherwise L449 would write bufferPtsMap_[trackIndex] and L455 would call ReturnBuffer.
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnBufferFilled_InputBufferNull_0200, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = nullptr;
    int32_t trackIndex = 1;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    sptr<CountingAVBufferQueueProducer> inputBufferQueue = new CountingAVBufferQueueProducer();
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    muxerFilter_->isTransCoderMode.store(false);
    muxerFilter_->preFilterCount_.store(1);
    muxerFilter_->isReachMaxDuration_.store(false);
    muxerFilter_->maxDuration_ = INT32_MAX;
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        muxerFilter_->bufferPtsMap_.clear();
    }
    // Pass counting queue (non-null), but inputBuffer==nullptr hits L424 guard, L455 ReturnBuffer not called
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
    EXPECT_EQ(static_cast<int32_t>(muxerFilter_->bufferPtsMap_.size()), 0);
    EXPECT_EQ(inputBufferQueue->returnBufferCount_, 0);
}

/**
 * @tc.name: MuxerFilter_OnBufferFilled_QueueNull_0200
 * @tc.desc: OnBufferFilled L428 inputBufferQueue==nullptr guard hits early return (inputBuffer non-null passes L424):
 *           In non-transcoder mode, if guard missed, L449 would write bufferPtsMap_[trackIndex].
 *           So no new trackIndex key + pre-set otherTrack key unchanged = guard indeed returns early
 *           (distinct from L424 guard: inputBuffer is non-null here, proving L428 is hit not L424).
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnBufferFilled_QueueNull_0200, TestSize.Level1)
{
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    ASSERT_NE(inputBuffer, nullptr);
    int32_t trackIndex = 1;
    int32_t otherTrack = 0;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    muxerFilter_->isTransCoderMode.store(false);
    muxerFilter_->preFilterCount_.store(1);
    muxerFilter_->isReachMaxDuration_.store(false);
    muxerFilter_->maxDuration_ = INT32_MAX;
    inputBuffer->pts_ = 5000000;
    {
        std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
        muxerFilter_->bufferPtsMap_.clear();
        muxerFilter_->bufferPtsMap_[otherTrack] = 0;  // pre-set otherTrack key, verify L449 won't add trackIndex key
    }
    // Intentionally pass nullptr queue to trigger L428 guard
    sptr<AVBufferQueueProducer> nullQueue = nullptr;
    muxerFilter_->OnBufferFilled(inputBuffer, trackIndex, streamType, nullQueue);
    std::lock_guard<std::mutex> lock(muxerFilter_->bufferPtsMutex_);
    // Guard hit: trackIndex key absent, map size stays 1
    EXPECT_EQ(static_cast<int32_t>(muxerFilter_->bufferPtsMap_.count(trackIndex)), 0);
    EXPECT_EQ(static_cast<int32_t>(muxerFilter_->bufferPtsMap_.size()), 1);
}

/**
 * @tc.name: MuxerFilter_SetParameter_Null_0100
 * @tc.desc: SetParameter nullptr guard (L219) — early return, mediaMuxer_->SetParameter not called;
 *           positive control — non-null parameter is forwarded verbatim (same object + content)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetParameter_Null_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    // Guard path: nullptr triggers SUT L219 FALSE_RETURN_MSG, mediaMuxer_->SetParameter must not be called
    EXPECT_CALL(*mockMediaMuxer, SetParameter(testing::_)).Times(0);
    muxerFilter_->SetParameter(nullptr);
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(mockMediaMuxer.get()));

    // Positive control: non-null parameter is forwarded to mediaMuxer_->SetParameter verbatim
    const std::string testMime = "video/avc";
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->SetData(Tag::MIME_TYPE, testMime);
    std::shared_ptr<Meta> capturedParam;
    EXPECT_CALL(*mockMediaMuxer, SetParameter(testing::_))
        .Times(1)
        .WillOnce(testing::DoAll(testing::SaveArg<0>(&capturedParam), testing::Return(Status::OK)));
    muxerFilter_->SetParameter(param);
    EXPECT_EQ(capturedParam.get(), param.get());
    std::string forwardedMime;
    ASSERT_TRUE(capturedParam != nullptr);
    EXPECT_TRUE(capturedParam->GetData(Tag::MIME_TYPE, forwardedMime));
    EXPECT_EQ(forwardedMime, testMime);
}

/**
 * @tc.name: MuxerFilter_SetUserMeta_Null_0100
 * @tc.desc: SetUserMeta nullptr guard (L228) — returns ERROR_INVALID_PARAMETER
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetUserMeta_Null_0100, TestSize.Level1)
{
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    Status ret = muxerFilter_->SetUserMeta(nullptr);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: MuxerFilter_HandleTransCoderComplete_0100
 * @tc.desc: OnTransCoderBufferFilled triggers HandleTransCoderComplete when eosCount_==preFilterCount_
 *           covers L512 stopCount_.store(preFilterCount_.load())
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleTransCoderComplete_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->preFilterCount_.store(1);
    muxerFilter_->videoIsEos.store(false);
    muxerFilter_->audioIsEos.store(false);
    inputBuffer->flag_ = 1; // EOS flag
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::OK));
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    // After EOS on video with preFilterCount_=1, eosCount_ becomes 1 == preFilterCount_,
    // HandleTransCoderComplete runs, stopCount_ should be set to preFilterCount_ (1)
    EXPECT_EQ(muxerFilter_->stopCount_.load(), muxerFilter_->preFilterCount_.load());
}

/**
 * @tc.name: MuxerFilter_Init_0100
 * @tc.desc: Init stores the event receiver into eventReceiver_
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_Init_0100, TestSize.Level1)
{
    std::shared_ptr<EventReceiver> receiver = std::make_shared<MyEventReceiver>();
    muxerFilter_->eventReceiver_ = nullptr;
    muxerFilter_->Init(receiver, nullptr);
    EXPECT_EQ(muxerFilter_->eventReceiver_.get(), receiver.get());
}

/**
 * @tc.name: MuxerFilter_SetMaxDuration_0100
 * @tc.desc: SetMaxDuration with a valid value (>=1) stores it directly
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetMaxDuration_0100, TestSize.Level1)
{
    muxerFilter_->maxDuration_ = 0;
    muxerFilter_->SetMaxDuration(100);
    EXPECT_EQ(muxerFilter_->maxDuration_, 100);
}

/**
 * @tc.name: MuxerFilter_SetMaxDuration_0200
 * @tc.desc: SetMaxDuration with an invalid value (<1) clamps to INT32_MAX
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetMaxDuration_0200, TestSize.Level1)
{
    muxerFilter_->maxDuration_ = 50;
    muxerFilter_->SetMaxDuration(0);
    EXPECT_EQ(muxerFilter_->maxDuration_, INT32_MAX);
}

/**
 * @tc.name: MuxerFilter_GetContainerFormat_0100
 * @tc.desc: GetContainerFormat returns the mime string for a supported format (DEFAULT)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_GetContainerFormat_0100, TestSize.Level1)
{
    const std::string& result = muxerFilter_->GetContainerFormat(Plugins::OutputFormat::DEFAULT);
    EXPECT_EQ(result, std::string("media/mp4"));
}

/**
 * @tc.name: MuxerFilter_GetContainerFormat_0200
 * @tc.desc: GetContainerFormat returns an empty string for an unsupported format (FLAC)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_GetContainerFormat_0200, TestSize.Level1)
{
    const std::string& result = muxerFilter_->GetContainerFormat(Plugins::OutputFormat::FLAC);
    EXPECT_EQ(result, std::string(""));
}

/**
 * @tc.name: MuxerFilter_DoStart_NullMuxer_0100
 * @tc.desc: DoStart returns ERROR_FRAMEWORK_ERR when mediaMuxer_ is null
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStart_NullMuxer_0100, TestSize.Level1)
{
    muxerFilter_->mediaMuxer_ = nullptr;
    muxerFilter_->isStarted.store(false);
    Status ret = muxerFilter_->DoStart();
    EXPECT_EQ(ret, Status::ERROR_FRAMEWORK_ERR);
    EXPECT_FALSE(muxerFilter_->isStarted.load());
}

/**
 * @tc.name: MuxerFilter_DoStart_StartError_0100
 * @tc.desc: DoStart returns ERROR_MUXER_START_FAILED when mediaMuxer_->Start fails
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStart_StartError_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->isStarted.store(false);
    EXPECT_CALL(*mockMediaMuxer, Start()).WillOnce(testing::Return(Status::ERROR_UNKNOWN));
    Status ret = muxerFilter_->DoStart();
    EXPECT_EQ(ret, Status::ERROR_MUXER_START_FAILED);
    EXPECT_FALSE(muxerFilter_->isStarted.load());
}

/**
 * @tc.name: MuxerFilter_OnLinked_NullMeta_0100
 * @tc.desc: OnLinked returns ERROR_NULL_POINTER when meta is null
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnLinked_NullMeta_0100, TestSize.Level1)
{
    muxerFilter_->mediaMuxer_ = nullptr;
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    std::shared_ptr<FilterLinkCallback> callback = nullptr;
    Status ret = muxerFilter_->OnLinked(streamType, nullptr, callback);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: MuxerFilter_OnLinked_NullMuxer_0100
 * @tc.desc: OnLinked returns ERROR_NULL_POINTER when mediaMuxer_ is null and meta is non-null
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnLinked_NullMuxer_0100, TestSize.Level1)
{
    muxerFilter_->mediaMuxer_ = nullptr;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    std::shared_ptr<FilterLinkCallback> callback = nullptr;
    Status ret = muxerFilter_->OnLinked(streamType, meta, callback);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: MuxerFilter_SetUserMeta_NullMuxer_0100
 * @tc.desc: SetUserMeta returns ERROR_NULL_POINTER when mediaMuxer_ is null
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetUserMeta_NullMuxer_0100, TestSize.Level1)
{
    muxerFilter_->mediaMuxer_ = nullptr;
    std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    Status ret = muxerFilter_->SetUserMeta(userMeta);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: MuxerFilter_ConfigureTrackMeta_Unknown_0100
 * @tc.desc: ConfigureTrackMeta with an unknown mime type leaves codec mime members unchanged
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_ConfigureTrackMeta_Unknown_0100, TestSize.Level1)
{
    muxerFilter_->audioCodecMimeType_ = "audio/preexisting";
    muxerFilter_->videoCodecMimeType_ = "video/preexisting";
    muxerFilter_->metaDataCodecMimeType_ = "meta/preexisting";
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    muxerFilter_->ConfigureTrackMeta(meta, "application/octet-stream");
    EXPECT_EQ(muxerFilter_->audioCodecMimeType_, "audio/preexisting");
    EXPECT_EQ(muxerFilter_->videoCodecMimeType_, "video/preexisting");
    EXPECT_EQ(muxerFilter_->metaDataCodecMimeType_, "meta/preexisting");
}

/**
 * @tc.name: MuxerFilter_HandleTransCoderComplete_StopError_0100
 * @tc.desc: HandleTransCoderComplete (via OnTransCoderBufferFilled) reports EVENT_ERROR when muxer Stop fails
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleTransCoderComplete_StopError_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    auto receiver = std::make_shared<RecordingEventReceiver>();
    muxerFilter_->eventReceiver_ = receiver;
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer();
    int32_t trackIndex = 1;
    sptr<AVBufferQueueProducer> inputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->preFilterCount_.store(1);
    muxerFilter_->videoIsEos.store(false);
    muxerFilter_->audioIsEos.store(false);
    inputBuffer->flag_ = 1; // BUFFER_IS_EOS
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::ERROR_UNKNOWN));
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(receiver->eventCount_, 1);
    EXPECT_EQ(receiver->lastEventType_, EventType::EVENT_ERROR);
}

/**
 * @tc.name: MuxerFilter_DoStop_NullMuxer_0200
 * @tc.desc: DoStop L196 FALSE_RETURN_V_MSG(mediaMuxer_ != nullptr) TRUE branch (null guard fires)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStop_NullMuxer_0200, TestSize.Level1)
{
    muxerFilter_->mediaMuxer_ = nullptr;
    muxerFilter_->isStarted.store(true);
    muxerFilter_->stopCount_.store(0);
    muxerFilter_->preFilterCount_.store(1);
    Status ret = muxerFilter_->DoStop();
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
    EXPECT_TRUE(muxerFilter_->isStarted.load());
}

/**
 * @tc.name: MuxerFilter_SetParameter_NullMuxer_0100
 * @tc.desc: SetParameter L232 FALSE_RETURN_MSG(mediaMuxer_ != nullptr) TRUE branch (null guard fires)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_SetParameter_NullMuxer_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    EXPECT_CALL(*mockMediaMuxer, SetParameter(testing::_)).WillOnce(testing::Return(Status::OK));
    muxerFilter_->SetParameter(param);
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(mockMediaMuxer.get()));

    muxerFilter_->mediaMuxer_ = nullptr;
    EXPECT_CALL(*mockMediaMuxer, SetParameter(testing::_)).Times(0);
    muxerFilter_->SetParameter(param);
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(mockMediaMuxer.get()));
}

/**
 * @tc.name: MuxerFilter_EventCompleteStopAsync_NullReceiver_0100
 * @tc.desc: EventCompleteStopAsync L463 FALSE_RETURN_MSG(eventReceiver_ != nullptr) TRUE branch (null guard fires)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_EventCompleteStopAsync_NullReceiver_0100, TestSize.Level1)
{
    auto receiver = std::make_shared<RecordingEventReceiver>();
    muxerFilter_->eventReceiver_ = receiver;
    muxerFilter_->EventCompleteStopAsync();
    EXPECT_EQ(receiver->eventCount_, 1);
    muxerFilter_->eventReceiver_ = nullptr;
    muxerFilter_->EventCompleteStopAsync();
    EXPECT_EQ(receiver->eventCount_, 1);
}

/**
 * @tc.name: MuxerFilter_HandleTransCoderComplete_NullMuxer_0100
 * @tc.desc: HandleTransCoderComplete L651 FALSE_RETURN_MSG(mediaMuxer_ != nullptr) TRUE branch (null guard fires);
 *           stopCount_ must remain unchanged (proves guard returned before L653)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleTransCoderComplete_NullMuxer_0100, TestSize.Level1)
{
    muxerFilter_->mediaMuxer_ = nullptr;
    muxerFilter_->preFilterCount_.store(5);
    muxerFilter_->stopCount_.store(0);
    muxerFilter_->HandleTransCoderComplete();
    EXPECT_EQ(muxerFilter_->stopCount_.load(), 0);
}

/**
 * @tc.name: MuxerFilter_HandleTransCoderComplete_NullReceiver_0100
 * @tc.desc: HandleTransCoderComplete L655 FALSE_RETURN_MSG(eventReceiver_ != nullptr) TRUE branch:
 *           mediaMuxer_ non-null (passes L651) so Stop is called and stopCount_ is set to preFilterCount_,
 *           then L655 null-receiver guard fires before OnEvent (no crash on null receiver)
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_HandleTransCoderComplete_NullReceiver_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->eventReceiver_ = nullptr;
    muxerFilter_->preFilterCount_.store(5);
    muxerFilter_->stopCount_.store(0);
    EXPECT_CALL(*mockMediaMuxer, Stop()).WillOnce(testing::Return(Status::OK));
    muxerFilter_->HandleTransCoderComplete();
    EXPECT_EQ(muxerFilter_->stopCount_.load(), muxerFilter_->preFilterCount_.load());
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(mockMediaMuxer.get()));
}

/**
 * @tc.name: MuxerFilter_DoStart_AddPendingMetaTracksFail_0100
 * @tc.desc: DoStart L166 FALSE_RETURN_V_MSG_E(ret == Status::OK, ret) TRUE branch:
 *           a pending meta track whose source IS registered triggers AddTrack (which fails),
 *           so AddPendingMetaTracks returns the error and DoStart propagates it
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStart_AddPendingMetaTracksFail_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->isStarted.store(false);
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    muxerFilter_->trackIndexMap_["audio/mp4a-latm"] = 0;
    MuxerFilter::PendingMetaTrack pending;
    pending.meta = std::make_shared<Meta>();
    pending.meta->Set<Tag::TIMED_METADATA_SRC_TRACK_MIME>(std::string("audio/mp4a-latm"));
    pending.callback = nullptr;
    pending.streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
    muxerFilter_->pendingMetaTracks_.push_back(pending);
    EXPECT_CALL(*mockMediaMuxer, AddTrack(testing::_, testing::_))
        .WillOnce(testing::Return(Status::ERROR_UNKNOWN));
    Status ret = muxerFilter_->DoStart();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    EXPECT_FALSE(muxerFilter_->isStarted.load());
}

/**
 * @tc.name: MuxerFilter_DoStart_AddPendingMetaTracksRepend_0100
 * @tc.desc: AddPendingMetaTracks L301 (it == trackIndexMap_.end()) TRUE branch:
 *           pending source track not yet registered → re-pended, AddPendingMetaTracks returns OK, DoStart proceeds
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStart_AddPendingMetaTracksRepend_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->isStarted.store(false);
    MuxerFilter::PendingMetaTrack pending;
    pending.meta = std::make_shared<Meta>();
    pending.callback = nullptr;
    pending.streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
    muxerFilter_->pendingMetaTracks_.push_back(pending);
    EXPECT_CALL(*mockMediaMuxer, Start()).WillOnce(testing::Return(Status::OK));
    Status ret = muxerFilter_->DoStart();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(muxerFilter_->isStarted.load());
}

/**
 * @tc.name: MuxerFilter_DoStart_AddPendingMetaTracksNullQueue_0100
 * @tc.desc: AddPendingMetaTracks L323 (inputBufferQueue == nullptr) TRUE branch:
 *           AddTrack succeeds but GetInputBufferQueue returns null → returns ERROR_NULL_POINTER,
 *           propagated by DoStart L166
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStart_AddPendingMetaTracksNullQueue_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->isStarted.store(false);
    muxerFilter_->trackIndexMap_["audio/mp4a-latm"] = 0;
    MuxerFilter::PendingMetaTrack pending;
    pending.meta = std::make_shared<Meta>();
    pending.meta->Set<Tag::TIMED_METADATA_SRC_TRACK_MIME>(std::string("audio/mp4a-latm"));
    pending.callback = nullptr;
    pending.streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
    muxerFilter_->pendingMetaTracks_.push_back(pending);
    EXPECT_CALL(*mockMediaMuxer, AddTrack(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(0), testing::Return(Status::OK)));
    EXPECT_CALL(*mockMediaMuxer, GetInputBufferQueue(testing::_))
        .WillOnce(testing::Return(nullptr));
    Status ret = muxerFilter_->DoStart();
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: MuxerFilter_OnLinked_AddPendingMetaTracksFail_0100
 * @tc.desc: OnLinked L385 FALSE_RETURN_V_MSG_E(ret == Status::OK, ret) TRUE branch:
 *           current audio track AddTrack succeeds, then AddPendingMetaTracks' AddTrack fails → propagated
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnLinked_AddPendingMetaTracksFail_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    muxerFilter_->trackIndexMap_.clear();
    MuxerFilter::PendingMetaTrack pending;
    pending.meta = std::make_shared<Meta>();
    pending.meta->Set<Tag::TIMED_METADATA_SRC_TRACK_MIME>(std::string("audio/mp4a-latm"));
    pending.callback = nullptr;
    pending.streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
    muxerFilter_->pendingMetaTracks_.push_back(pending);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::MIME_TYPE>(std::string("audio/mp4a-latm"));
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
    EXPECT_CALL(*mockMediaMuxer, AddTrack(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(0), testing::Return(Status::OK)))
        .WillOnce(testing::Return(Status::ERROR_UNKNOWN));
    Status ret = muxerFilter_->OnLinked(streamType, meta, nullptr);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: MuxerFilter_OnLinked_NullInputBufferQueue_0100
 * @tc.desc: OnLinked L389 FALSE_RETURN_V_MSG(inputBufferQueue != nullptr) TRUE branch:
 *           AddTrack succeeds, GetInputBufferQueue returns null → returns ERROR_NULL_POINTER
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnLinked_NullInputBufferQueue_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->trackIndexMap_.clear();
    muxerFilter_->pendingMetaTracks_.clear();
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::MIME_TYPE>(std::string("video/avc"));
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    std::shared_ptr<FilterLinkCallback> callback = std::make_shared<MyFilterLinkCallback>();
    EXPECT_CALL(*mockMediaMuxer, AddTrack(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(0), testing::Return(Status::OK)));
    EXPECT_CALL(*mockMediaMuxer, GetInputBufferQueue(testing::_))
        .WillOnce(testing::Return(nullptr));
    Status ret = muxerFilter_->OnLinked(streamType, meta, callback);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: MuxerFilter_OnLinked_NullCallback_0100
 * @tc.desc: OnLinked L390 FALSE_RETURN_V_MSG(callback != nullptr) TRUE branch:
 *           AddTrack succeeds, GetInputBufferQueue returns a valid queue, callback is null → ERROR_NULL_POINTER
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_OnLinked_NullCallback_0100, TestSize.Level1)
{
    std::shared_ptr<MockMediaMuxer> mockMediaMuxer = std::make_shared<MockMediaMuxer>(0, 0);
    muxerFilter_->mediaMuxer_ = std::static_pointer_cast<MediaMuxer>(mockMediaMuxer);
    muxerFilter_->trackIndexMap_.clear();
    muxerFilter_->pendingMetaTracks_.clear();
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::MIME_TYPE>(std::string("video/avc"));
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    sptr<AVBufferQueueProducer> queue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    EXPECT_CALL(*mockMediaMuxer, AddTrack(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(0), testing::Return(Status::OK)));
    EXPECT_CALL(*mockMediaMuxer, GetInputBufferQueue(testing::_))
        .WillOnce(testing::Return(queue));
    Status ret = muxerFilter_->OnLinked(streamType, meta, nullptr);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS