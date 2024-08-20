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
    muxerFilter_->startCount_ = 0;
    muxerFilter_->preFilterCount_ = 1;
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    muxerFilter_->DoStart();
    muxerFilter_->startCount_ = 0;
    muxerFilter_->preFilterCount_ = 0;
    EXPECT_EQ(muxerFilter_->DoStart(), Status::OK);
}

/**
 * @tc.name: MuxerFilter_DoStop_0100
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(MuxerFilterUnitTest, MuxerFilter_DoStop_0100, TestSize.Level1)
{
    muxerFilter_->stopCount_ = 0;
    muxerFilter_->preFilterCount_ = 1;
    muxerFilter_->mediaMuxer_ = std::make_shared<MediaMuxer>(0, 0);
    EXPECT_EQ(muxerFilter_->DoStop(), Status::OK);
    muxerFilter_->stopCount_ = 0;
    muxerFilter_->preFilterCount_ = 0;
    muxerFilter_->DoStop();
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
    muxerFilter_->eosCount_ = 0;
    muxerFilter_->preFilterCount_ = 0;
    inputBuffer->flag_ = 0;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    streamType = StreamType::STREAMTYPE_ENCODED_AUDIO;
    muxerFilter_->lastAudioPts_ = 1;
    inputBuffer->pts_ = 0;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    inputBuffer->pts_ = 1000;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    streamType = StreamType::STREAMTYPE_MAX;
    muxerFilter_->OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
    EXPECT_EQ(muxerFilter_->videoIsEos, true);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS