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

#include <memory>
#include "video_resize_filter_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing;
using namespace testing::ext;
using OHOS::Media::Pipeline::StreamType;
using OHOS::Media::Status;

const static int32_t TEST_TIMES_ONE = 1;

namespace OHOS {
namespace Media {

#ifdef USE_VIDEO_PROCESSING_ENGINE
using namespace VideoProcessingEngine;
#endif

namespace Pipeline {
void VideoResizeFilterTest::SetUpTestCase(void) {}

void VideoResizeFilterTest::TearDownTestCase(void) {}

void VideoResizeFilterTest::SetUp(void)
{
    videoResizeFilter_ = std::make_shared<VideoResizeFilter>("videoResizeFilter", FilterType::FILTERTYPE_VIDRESIZE);
    ASSERT_TRUE(videoResizeFilter_ != nullptr);

    eventReceiver_ = std::make_shared<OHOS::Media::Pipeline::MockEventReceiver>();
    ASSERT_TRUE(eventReceiver_ != nullptr);

    filterCallback_ = std::make_shared<MockFilterCallback>();
    ASSERT_TRUE(filterCallback_ != nullptr);

    onLinkedResultCallback_ = std::make_shared<MockFilterLinkCallback>();
    ASSERT_TRUE(onLinkedResultCallback_ != nullptr);

    nextFilter_ = std::make_shared<MockFilter>("test", FilterType::FILTERTYPE_FSINK, false);
    ASSERT_TRUE(nextFilter_ != nullptr);

    releaseBufferTask_ = std::make_shared<MockTask>("test", "", TaskType::SINGLETON, TaskPriority::NORMAL, true);
    ASSERT_TRUE(releaseBufferTask_ != nullptr);

    mockSurface_ = new MockSurface();
    ASSERT_TRUE(mockSurface_ != nullptr);

    videoResizeFilter_->eventReceiver_ = eventReceiver_;
    videoResizeFilter_->filterCallback_ = filterCallback_;
    videoResizeFilter_->onLinkedResultCallback_ = onLinkedResultCallback_;
    videoResizeFilter_->nextFilter_ = nextFilter_;
    videoResizeFilter_->releaseBufferTask_ = releaseBufferTask_;

#ifdef USE_VIDEO_PROCESSING_ENGINE
    videoEnhancer_ = std::make_shared<OHOS::Media::VideoProcessingEngine::MockDetailEnhancerVideo>();
    ASSERT_TRUE(videoEnhancer_ != nullptr);

    videoResizeFilter_->videoEnhancer_ = videoEnhancer_;
#endif
}

void VideoResizeFilterTest::TearDown(void)
{
    videoResizeFilter_ = nullptr;
}

#ifdef USE_VIDEO_PROCESSING_ENGINE
/**
 * @tc.name  : Test OnVPEError
 * @tc.number: OnVPEError_001
 * @tc.desc  : Test eventReceiver_->OnEvent()
 */
HWTEST_F(VideoResizeFilterTest, OnVPEError_001, TestSize.Level1)
{
    int32_t errorCode = 0;
    EXPECT_CALL(*(eventReceiver_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);
    videoResizeFilter_->isVPEReportError_ = false;
    videoResizeFilter_->OnVPEError(errorCode);
    EXPECT_EQ(videoResizeFilter_->isVPEReportError_, true);
}

/**
 * @tc.name  : Test OnOutputBufferAvailable
 * @tc.number: OnOutputBufferAvailable_001
 * @tc.desc  : Test flag == static_cast<uint32_t>(DETAIL_ENH_BUFFER_FLAG_EOS
 */
HWTEST_F(VideoResizeFilterTest, OnOutputBufferAvailable_001, TestSize.Level1)
{
    uint32_t index = 0;
    uint32_t flag = static_cast<uint32_t>(DETAIL_ENH_BUFFER_FLAG_EOS);
    videoResizeFilter_->indexs_.clear();
    videoResizeFilter_->OnOutputBufferAvailable(index, flag);
    EXPECT_EQ(videoResizeFilter_->indexs_.size(), 0);
}

/**
 * @tc.name  : Test OnOutputBufferAvailable
 * @tc.number: OnOutputBufferAvailable_002
 * @tc.desc  : Test flag != static_cast<uint32_t>(DETAIL_ENH_BUFFER_FLAG_EOS
 */
HWTEST_F(VideoResizeFilterTest, OnOutputBufferAvailable_002, TestSize.Level1)
{
    uint32_t index = 0;
    uint32_t flag = 0;
    videoResizeFilter_->indexs_.clear();
    videoResizeFilter_->OnOutputBufferAvailable(index, flag);
    ASSERT_TRUE(videoResizeFilter_->indexs_.size() > 0);
}

/**
 * @tc.name  : Test OnOutputBufferAvailable
 * @tc.number: OnOutputBufferAvailable_003
 * @tc.desc  : Test videoEnhancer_ = nullptr
 */
HWTEST_F(VideoResizeFilterTest, OnOutputBufferAvailable_003, TestSize.Level1)
{
    uint32_t index = 0;
    uint32_t flag = 0;
    videoResizeFilter_->indexs_.clear();
    videoResizeFilter_->videoEnhancer_ = nullptr;
    videoResizeFilter_->OnOutputBufferAvailable(index, flag);
    ASSERT_TRUE(videoResizeFilter_->indexs_.size() > 0);
}

/**
 * @tc.name  : Test OnOutputBufferAvailable
 * @tc.number: OnOutputBufferAvailable_004
 * @tc.desc  : Test videoEnhancer_ != nullptr && currentFrameNum_.load() < frameNum_
 */
HWTEST_F(VideoResizeFilterTest, OnOutputBufferAvailable_004, TestSize.Level1)
{
    uint32_t index = 0;
    uint32_t flag = 0;
    videoResizeFilter_->indexs_.clear();
    videoResizeFilter_->frameNum_ = UINT32_MAX;
    
    videoResizeFilter_->OnOutputBufferAvailable(index, flag);
    ASSERT_TRUE(videoResizeFilter_->indexs_.size() > 0);
    ASSERT_TRUE(videoResizeFilter_->frameNum_ > videoResizeFilter_->currentFrameNum_.load());
}

/**
 * @tc.name  : Test OnOutputBufferAvailable
 * @tc.number: OnOutputBufferAvailable_005
 * @tc.desc  : Test videoEnhancer_ != nullptr && currentFrameNum_.load() >= frameNum_
 */
HWTEST_F(VideoResizeFilterTest, OnOutputBufferAvailable_005, TestSize.Level1)
{
    uint32_t index = 0;
    uint32_t flag = 0;
    videoResizeFilter_->indexs_.clear();
    videoResizeFilter_->frameNum_ = 0;
    EXPECT_CALL(*(videoEnhancer_), NotifyEos()).Times(TEST_TIMES_ONE);

    videoResizeFilter_->OnOutputBufferAvailable(index, flag);
    ASSERT_TRUE(videoResizeFilter_->indexs_.size() > 0);
    ASSERT_TRUE(videoResizeFilter_->frameNum_ < videoResizeFilter_->currentFrameNum_.load());
}

/**
 * @tc.name  : Test SetParameter
 * @tc.number: SetParameter_001
 * @tc.desc  : Test parameter->Get<Tag::USER_PUSH_DATA_TIME>(frameNum_) = true && isEos = false
 */
HWTEST_F(VideoResizeFilterTest, SetParameter_001, TestSize.Level1)
{
    std::shared_ptr<Meta> parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    parameterTest_->Clear();
    parameterTest_->Set<Tag::MEDIA_END_OF_STREAM>(false);
    parameterTest_->Set<Tag::USER_FRAME_PTS>(1);
    parameterTest_->Set<Tag::USER_PUSH_DATA_TIME>(1);

    EXPECT_CALL(*(videoEnhancer_),
        SetParameter(testing::_, testing::_)).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
 
    videoResizeFilter_->SetParameter(parameterTest_);
    EXPECT_EQ(videoResizeFilter_->frameNum_, 1);
    EXPECT_EQ(videoResizeFilter_->eosPts_, 1);
}

/**
 * @tc.name  : Test SetParameter
 * @tc.number: SetParameter_002
 * @tc.desc  : Test isEos = true && videoEnhancer_ == nullptr
 */
HWTEST_F(VideoResizeFilterTest, SetParameter_002, TestSize.Level1)
{
    std::shared_ptr<Meta> parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    parameterTest_->Clear();
    parameterTest_->Set<Tag::MEDIA_END_OF_STREAM>(true);
    parameterTest_->Set<Tag::USER_FRAME_PTS>(1);
    parameterTest_->Set<Tag::USER_PUSH_DATA_TIME>(1);

    videoResizeFilter_->videoEnhancer_ = nullptr;

    videoResizeFilter_->SetParameter(parameterTest_);
    EXPECT_EQ(videoResizeFilter_->frameNum_, 1);
    EXPECT_EQ(videoResizeFilter_->eosPts_, 1);
}

/**
 * @tc.name  : Test SetParameter
 * @tc.number: SetParameter_003
 * @tc.desc  : Test currentFrameNum_.load() < frameNum_
 */
HWTEST_F(VideoResizeFilterTest, SetParameter_003, TestSize.Level1)
{
    std::shared_ptr<Meta> parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    parameterTest_->Clear();
    parameterTest_->Set<Tag::MEDIA_END_OF_STREAM>(true);
    parameterTest_->Set<Tag::USER_FRAME_PTS>(1);
    parameterTest_->Set<Tag::USER_PUSH_DATA_TIME>(1);
    videoResizeFilter_->currentFrameNum_ = 0;

    videoResizeFilter_->SetParameter(parameterTest_);
    EXPECT_EQ(videoResizeFilter_->frameNum_, 1);
    EXPECT_EQ(videoResizeFilter_->eosPts_, 1);
}

/**
 * @tc.name  : Test SetParameter
 * @tc.number: SetParameter_004
 * @tc.desc  : Test currentFrameNum_.load() >= frameNum_
 */
HWTEST_F(VideoResizeFilterTest, SetParameter_004, TestSize.Level1)
{
    std::shared_ptr<Meta> parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    parameterTest_->Clear();
    parameterTest_->Set<Tag::MEDIA_END_OF_STREAM>(true);
    parameterTest_->Set<Tag::USER_FRAME_PTS>(1);
    parameterTest_->Set<Tag::USER_PUSH_DATA_TIME>(1);
    videoResizeFilter_->currentFrameNum_ = 1;

    EXPECT_CALL(*(videoEnhancer_), NotifyEos()).Times(TEST_TIMES_ONE);

    videoResizeFilter_->SetParameter(parameterTest_);
    EXPECT_EQ(videoResizeFilter_->frameNum_, 1);
    EXPECT_EQ(videoResizeFilter_->eosPts_, 1);
}

/**
 * @tc.name  : Test SetParameter
 * @tc.number: SetParameter_005
 * @tc.desc  : Test ret == 0
 */
HWTEST_F(VideoResizeFilterTest, SetParameter_005, TestSize.Level1)
{
    std::shared_ptr<Meta> parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    parameterTest_->Clear();

    EXPECT_CALL(*(videoEnhancer_),
        SetParameter(testing::_, testing::_)).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
 
    videoResizeFilter_->SetParameter(parameterTest_);
}

/**
 * @tc.name  : Test SetParameter
 * @tc.number: SetParameter_006
 * @tc.desc  : Test ret != 0 && eventReceiver_ = nullptr
 */
HWTEST_F(VideoResizeFilterTest, SetParameter_006, TestSize.Level1)
{
    std::shared_ptr<Meta> parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    parameterTest_->Clear();

    EXPECT_CALL(*(videoEnhancer_),
        SetParameter(testing::_, testing::_)).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    videoResizeFilter_->eventReceiver_ = nullptr;

    videoResizeFilter_->SetParameter(parameterTest_);
}

/**
 * @tc.name  : Test SetParameter
 * @tc.number: SetParameter_007
 * @tc.desc  : Test ret != 0 && eventReceiver_ != nullptr
 */
HWTEST_F(VideoResizeFilterTest, SetParameter_007, TestSize.Level1)
{
    auto parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    parameterTest_->Clear();

    EXPECT_CALL(*(videoEnhancer_),
        SetParameter(testing::_, testing::_)).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_CALL(*(eventReceiver_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);

    videoResizeFilter_->SetParameter(parameterTest_);
}

/**
 * @tc.name  : Test DoStop
 * @tc.number: DoStop_001
 * @tc.desc  : Test eventReceiver_ = nullptr
 */
HWTEST_F(VideoResizeFilterTest, DoStop_001, TestSize.Level1)
{
    videoResizeFilter_->releaseBufferTask_ = nullptr;
    videoResizeFilter_->eventReceiver_ = nullptr;
    EXPECT_CALL(*(videoEnhancer_), Stop()).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));

    Status ret = videoResizeFilter_->DoStop();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test DoStop
 * @tc.number: DoStop_002
 * @tc.desc  : Test eventReceiver_ != nullptr
 */
HWTEST_F(VideoResizeFilterTest, DoStop_002, TestSize.Level1)
{
    videoResizeFilter_->releaseBufferTask_ = nullptr;
    EXPECT_CALL(*(videoEnhancer_), Stop()).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_CALL(*(eventReceiver_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);

    Status ret = videoResizeFilter_->DoStop();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test DoStart
 * @tc.number: DoStart_001
 * @tc.desc  : Test eventReceiver_ = nullptr
 */
HWTEST_F(VideoResizeFilterTest, DoStart_001, TestSize.Level1)
{
    videoResizeFilter_->releaseBufferTask_ = nullptr;
    videoResizeFilter_->eventReceiver_ = nullptr;
    EXPECT_CALL(*(videoEnhancer_),
        Start()).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));

    Status ret = videoResizeFilter_->DoStart();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test DoStart
 * @tc.number: DoStart_002
 * @tc.desc  : Test eventReceiver_ != nullptr
 */
HWTEST_F(VideoResizeFilterTest, DoStart_002, TestSize.Level1)
{
    videoResizeFilter_->releaseBufferTask_ = nullptr;
    EXPECT_CALL(*(videoEnhancer_),
        Start()).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_CALL(*(eventReceiver_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);

    Status ret = videoResizeFilter_->DoStart();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test SetOutputSurface
 * @tc.number: SetOutputSurface_001
 * @tc.desc  : Test eventReceiver_ = nullptr
 */
HWTEST_F(VideoResizeFilterTest, SetOutputSurface_001, TestSize.Level1)
{
    sptr<Surface> surface_test = sptr<Surface>(mockSurface_.GetRefPtr());
    videoResizeFilter_->eventReceiver_ = nullptr;
    EXPECT_CALL(*(mockSurface_),
        SetRequestWidthAndHeight(testing::_, testing::_)).Times(TEST_TIMES_ONE);

    EXPECT_CALL(*(videoEnhancer_),
        SetOutputSurface(testing::_)).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));

    Status ret = videoResizeFilter_->SetOutputSurface(surface_test, 0, 0);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test SetOutputSurface
 * @tc.number: SetOutputSurface_002
 * @tc.desc  : Test eventReceiver_ != nullptr
 */
HWTEST_F(VideoResizeFilterTest, SetOutputSurface_002, TestSize.Level1)
{
    sptr<Surface> surface_test = sptr<Surface>(mockSurface_.GetRefPtr());
    EXPECT_CALL(*(mockSurface_),
        SetRequestWidthAndHeight(testing::_, testing::_)).Times(TEST_TIMES_ONE);
    EXPECT_CALL(*(videoEnhancer_),
        SetOutputSurface(testing::_)).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_CALL(*(eventReceiver_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);

    Status ret = videoResizeFilter_->SetOutputSurface(surface_test, 0, 0);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test Configure
 * @tc.number: Configure_001
 * @tc.desc  : Test eventReceiver_ = nullptr
 */
HWTEST_F(VideoResizeFilterTest, Configure_001, TestSize.Level1)
{
    auto parameterTest_ = std::make_shared<Meta>();
    videoResizeFilter_->eventReceiver_ = nullptr;
    EXPECT_CALL(*(videoEnhancer_),
        SetParameter(testing::_, testing::_)).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    Status ret = videoResizeFilter_->Configure(parameterTest_);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test Configure
 * @tc.number: Configure_002
 * @tc.desc  : Test eventReceiver_ != nullptr
 */
HWTEST_F(VideoResizeFilterTest, Configure_002, TestSize.Level1)
{
    auto parameterTest_ = std::make_shared<Meta>();
    EXPECT_CALL(*(videoEnhancer_),
        SetParameter(testing::_, testing::_)).WillOnce(testing::Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_CALL(*(eventReceiver_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);

    Status ret = videoResizeFilter_->Configure(parameterTest_);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}
#endif

/**
 * @tc.name  : Test LinkNext
 * @tc.number: LinkNext_001
 * @tc.desc  : Test ret == Status::OK  && eventReceiver_ != nullptr
 */
HWTEST_F(VideoResizeFilterTest, LinkNext_001, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    auto parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    videoResizeFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, parameterTest_, onLinkedResultCallback_);
    EXPECT_CALL(*(nextFilter_), OnLinked(testing::_, testing::_, testing::_)).WillOnce(testing::Return(Status::OK));

    Status ret = videoResizeFilter_->LinkNext(nextFilterTest, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test LinkNext
 * @tc.number: LinkNext_002
 * @tc.desc  : Test ret != Status::OK && eventReceiver_ = nullptr
 */
HWTEST_F(VideoResizeFilterTest, LinkNext_002, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    auto parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    videoResizeFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, parameterTest_, onLinkedResultCallback_);
    EXPECT_CALL(*(nextFilter_), OnLinked(testing::_, testing::_, testing::_)).WillOnce(Return(Status::ERROR_UNKNOWN));
    videoResizeFilter_->eventReceiver_ = nullptr;
    Status ret = videoResizeFilter_->LinkNext(nextFilterTest, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test LinkNext
 * @tc.number: LinkNext_003
 * @tc.desc  : Test ret != Status::OK && eventReceiver_ != nullptr
 */
HWTEST_F(VideoResizeFilterTest, LinkNext_003, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    auto parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    videoResizeFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, parameterTest_, onLinkedResultCallback_);
    EXPECT_CALL(*(nextFilter_),
        OnLinked(testing::_, testing::_, testing::_)).WillOnce(testing::Return(Status::ERROR_UNKNOWN));
    EXPECT_CALL(*(eventReceiver_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);

    Status ret = videoResizeFilter_->LinkNext(nextFilterTest, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test LinkNext
 * @tc.number: LinkNext_004
 * @tc.desc  : Test ret == Status::OK && eventReceiver_ = nullptr
 */
HWTEST_F(VideoResizeFilterTest, LinkNext_004, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    auto parameterTest_ = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest_ != nullptr);
    videoResizeFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, parameterTest_, onLinkedResultCallback_);
    EXPECT_CALL(*(nextFilter_), OnLinked(testing::_, testing::_, testing::_)).WillOnce(testing::Return(Status::OK));
    videoResizeFilter_->eventReceiver_ = nullptr;
    Status ret = videoResizeFilter_->LinkNext(nextFilterTest, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test NotifyNextFilterEos
 * @tc.number: NotifyNextFilterEos_001
 * @tc.desc  : Test nextFiltersMap_ is empty
 */
HWTEST_F(VideoResizeFilterTest, NotifyNextFilterEos_001, TestSize.Level1)
{
    videoResizeFilter_->nextFiltersMap_.clear();
    Status ret = videoResizeFilter_->NotifyNextFilterEos();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test NotifyNextFilterEos
 * @tc.number: NotifyNextFilterEos_002
 * @tc.desc  : Test nextFiltersMap_ is not empty
 */
HWTEST_F(VideoResizeFilterTest, NotifyNextFilterEos_002, TestSize.Level1)
{
    videoResizeFilter_->nextFiltersMap_.clear();
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    StreamType outTypeTest = StreamType::STREAMTYPE_ENCODED_VIDEO;
    videoResizeFilter_->nextFiltersMap_[outTypeTest].push_back(nextFilterTest);
    EXPECT_CALL(*(nextFilter_), SetParameter(testing::_)).Times(TEST_TIMES_ONE);

    Status ret = videoResizeFilter_->NotifyNextFilterEos();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoSetPerfRecEnabled_001
 * @tc.desc: Test DoSetPerfRecEnabled interface enable performance recording
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterTest, DoSetPerfRecEnabled_001, TestSize.Level1)
{
    Status ret = videoResizeFilter_->DoSetPerfRecEnabled(true);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoSetPerfRecEnabled_002
 * @tc.desc: Test DoSetPerfRecEnabled interface disable performance recording
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterTest, DoSetPerfRecEnabled_002, TestSize.Level1)
{
    Status ret = videoResizeFilter_->DoSetPerfRecEnabled(false);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoSetPerfRecEnabled_003
 * @tc.desc: Test DoSetPerfRecEnabled interface repeat setting
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterTest, DoSetPerfRecEnabled_003, TestSize.Level1)
{
    Status ret1 = videoResizeFilter_->DoSetPerfRecEnabled(true);
    EXPECT_EQ(ret1, Status::OK);
    Status ret2 = videoResizeFilter_->DoSetPerfRecEnabled(true);
    EXPECT_EQ(ret2, Status::OK);
    Status ret3 = videoResizeFilter_->DoSetPerfRecEnabled(false);
    EXPECT_EQ(ret3, Status::OK);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS