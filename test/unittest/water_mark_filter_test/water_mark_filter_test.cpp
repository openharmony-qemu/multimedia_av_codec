/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "water_mark_filter_test.h"
 
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
 
namespace Pipeline {
void WaterMarkFilterTest::SetUpTestCase(void) {}
 
void WaterMarkFilterTest::TearDownTestCase(void) {}
 
void WaterMarkFilterTest::SetUp(void)
{
    waterMarkFilter_ = std::make_shared<WaterMarkFilter>("waterMarkFilter", FilterType::WATERMARK);
    ASSERT_TRUE(waterMarkFilter_ != nullptr);
 
    eventReceiver_ = std::make_shared<OHOS::Media::Pipeline::MockEventReceiver>();
    ASSERT_TRUE(eventReceiver_ != nullptr);
 
    filterCallback_ = std::make_shared<MockFilterCallback>();
    ASSERT_TRUE(filterCallback_ != nullptr);
 
    onLinkedResultCallback_ = std::make_shared<MockFilterLinkCallback>();
    ASSERT_TRUE(onLinkedResultCallback_ != nullptr);
 
    nextFilter_ = std::make_shared<MockFilter>("test", FilterType::FILTERTYPE_FSINK, false);
    ASSERT_TRUE(nextFilter_ != nullptr);
 
    mockSurface_ = new MockSurface();
    ASSERT_TRUE(mockSurface_ != nullptr);
 
    waterMarkFilter_->eventReceiver_ = eventReceiver_;
    waterMarkFilter_->filterCallback_ = filterCallback_;
    waterMarkFilter_->onLinkedResultCallback_ = onLinkedResultCallback_;
    waterMarkFilter_->nextFilter_ = nextFilter_;
}
 
void WaterMarkFilterTest::TearDown(void)
{
    waterMarkFilter_ = nullptr;
}
 
/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_001
 * @tc.desc  : Test normal watermark setting
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_001, TestSize.Level1)
{
    auto waterMarkBuffer = AVMemory::CreateAVMemory(parcel, true);
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(400);
 
    int32_t bufferSize = 100 * 400;
    waterMarkBuffer->memory_ = AVMemory::CreateAVMemory();
    ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);
 
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::OK);
}
 
/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_002
 * @tc.desc  : Test watermark list exceeds maximum number
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_002, TestSize.Level1)
{
    // Fill watermark list to maximum
    for (uint32_t i = 0; i < 5; i++) {
        auto waterMarkBuffer = std::make_shared<AVBuffer>();
        ASSERT_TRUE(waterMarkBuffer != nullptr);
        waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
        waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
        waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
        waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
        waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(400);
 
        int32_t bufferSize = 100 * 400;
        waterMarkBuffer->memory_ = AVMemory::CreateAVMemory();
        ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);
 
        waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);

    }
 
    // Try to add one more watermark (should fail)
    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(400);
 
    int32_t bufferSize = 100 * 400;
    waterMarkBuffer->memory_ = AVMemory::CreateAVMemory();
    ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);
 
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}
 
/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_003
 * @tc.desc  : Test watermark size exceeds maximum limit
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_003, TestSize.Level1)
{
    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
    // Set stride that would cause size to exceed MAX_WATERMARK_SIZE
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(210000);
 
    int32_t bufferSize = 100 * 210000;
    waterMarkBuffer->memory_ = AVMemory::CreateAVMemory();
    ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);
 
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}
 
/**
 * @tc.name  : Test SetVideoResize
 * @tc.number: SetVideoResize_001
 * @tc.desc  : Test normal video resize setting
 */
HWTEST_F(WaterMarkFilterTest, SetVideoResize_001, TestSize.Level1)
{
    Status ret = waterMarkFilter_->SetVideoResize(1920, 1080);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(waterMarkFilter_->videoWidth_.load(), 1920);
    EXPECT_EQ(waterMarkFilter_->videoHeight_.load(), 1080);
}
 
/**
 * @tc.name  : Test SetVideoResize
 * @tc.number: SetVideoResize_002
 * @tc.desc  : Test invalid resize parameters
 */
HWTEST_F(WaterMarkFilterTest, SetVideoResize_002, TestSize.Level1)
{
    Status ret = waterMarkFilter_->SetVideoResize(-1, 1080);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
 
    ret = waterMarkFilter_->SetVideoResize(1920, 0);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}
 
/**
 * @tc.name  : Test DoPrepare
 * @tc.number: DoPrepare_001
 * @tc.desc  : Test normal prepare flow
 */
HWTEST_F(WaterMarkFilterTest, DoPrepare_001, TestSize.Level1)
{
    EXPECT_CALL(*(filterCallback_), OnCallback(testing::_, testing::_, testing::_)).Times(TEST_TIMES_ONE);
    Status ret = waterMarkFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::OK);
}
 
/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_001
 * @tc.desc  : Test normal release flow
 */
HWTEST_F(WaterMarkFilterTest, DoRelease_001, TestSize.Level1)
{
    Status ret = waterMarkFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
}
 
/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_002
 * @tc.desc  : Test duplicate release calls
 */
HWTEST_F(WaterMarkFilterTest, DoRelease_002, TestSize.Level1)
{
    waterMarkFilter_->DoRelease();
    Status ret = waterMarkFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
}
 
/**
 * @tc.name  : Test SetParameter
 * @tc.number: SetParameter_001
 * @tc.desc  : Test setting PTS and frameNum
 */
HWTEST_F(WaterMarkFilterTest, SetParameter_001, TestSize.Level1)
{
    auto parameter = std::make_shared<Meta>();
    ASSERT_TRUE(parameter != nullptr);
    parameter->Set<Tag::USER_FRAME_PTS>(100);
    parameter->Set<Tag::USER_PUSH_DATA_TIME>(50);
 
    waterMarkFilter_->SetParameter(parameter);
    EXPECT_EQ(waterMarkFilter_->eosPts_.load(), 100);
    EXPECT_EQ(waterMarkFilter_->frameNum_, 50);
}
 
/**
 * @tc.name  : Test GetParameter
 * @tc.number: GetParameter_001
 * @tc.desc  : Test getting parameter
 */
HWTEST_F(WaterMarkFilterTest, GetParameter_001, TestSize.Level1)
{
    auto parameter = std::make_shared<Meta>();
    ASSERT_TRUE(parameter != nullptr);
    waterMarkFilter_->GetParameter(parameter);
}
 
/**
 * @tc.name  : Test LinkNext
 * @tc.number: LinkNext_001
 * @tc.desc  : Test normal link next filter
 */
HWTEST_F(WaterMarkFilterTest, LinkNext_001, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);

    auto parameterTest = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest != nullptr);
    waterMarkFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, parameterTest, onLinkedResultCallback_);
    EXPECT_CALL(*(nextFilter_), OnLinked(testing::_, testing::_, testing::_)).WillOnce(testing::Return(Status::OK));
 
    Status ret = waterMarkFilter_->LinkNext(nextFilterTest, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::OK);
}
 
/**
 * @tc.name  : Test LinkNext
 * @tc.number: LinkNext_002
 * @tc.desc  : Test link next filter failed with eventReceiver null
 */
HWTEST_F(WaterMarkFilterTest, LinkNext_002, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    auto parameterTest = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest != nullptr);
    waterMarkFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, parameterTest, onLinkedResultCallback_);
    EXPECT_CALL(*(nextFilter_), OnLinked(testing::_, testing::_, testing::_)).WillOnce(Return(Status::ERROR_UNKNOWN));
    waterMarkFilter_->eventReceiver_ = nullptr;
    Status ret = waterMarkFilter_->LinkNext(nextFilterTest, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}
 
/**
 * @tc.name  : Test LinkNext
 * @tc.number: LinkNext_003
 * @tc.desc  : Test link next filter failed with eventReceiver not null
 */
HWTEST_F(WaterMarkFilterTest, LinkNext_003, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    auto parameterTest = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest != nullptr);
    waterMarkFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, parameterTest, onLinkedResultCallback_);
    EXPECT_CALL(*(nextFilter_),
        OnLinked(testing::_, testing::_, testing::_)).WillOnce(testing::Return(Status::ERROR_UNKNOWN));
    EXPECT_CALL(*(eventReceiver_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);
 
    Status ret = waterMarkFilter_->LinkNext(nextFilterTest, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}
 
/**
 * @tc.name  : Test NotifyNextFilterEos
 * @tc.number: NotifyNextFilterEos_001
 * @tc.desc  : Test notify next filter EOS with empty nextFiltersMap
 */
HWTEST_F(WaterMarkFilterTest, NotifyNextFilterEos_001, TestSize.Level1)
{
    waterMarkFilter_->nextFiltersMap_.clear();
    Status ret = waterMarkFilter_->NotifyNextFilterEos();
    EXPECT_EQ(ret, Status::OK);
}
 
/**
 * @tc.name  : Test NotifyNextFilterEos
 * @tc.number: NotifyNextFilterEos_002
 * @tc.desc  : Test notify next filter EOS with non-empty nextFiltersMap
 */
HWTEST_F(WaterMarkFilterTest, NotifyNextFilterEos_002, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    waterMarkFilter_->nextFiltersMap_[StreamType::STREAMTYPE_RAW_VIDEO].push_back(nextFilterTest);
    waterMarkFilter_->eosPts_.store(100);
 
    EXPECT_CALL(*(nextFilter_), SetParameter(testing::_)).Times(TEST_TIMES_ONE);
    Status ret = waterMarkFilter_->NotifyNextFilterEos();
    EXPECT_EQ(ret, Status::OK);
}
 
/**
 * @tc.name  : Test GetInputSurface
 * @tc.number: GetInputSurface_001
 * @tc.desc  : Test get input surface with nativeImageWindow null
 */
HWTEST_F(WaterMarkFilterTest, GetInputSurface_001, TestSize.Level1)
{
    waterMarkFilter_->nativeImageWindow_ = nullptr;
    sptr<Surface> ret = waterMarkFilter_->GetInputSurface();
    EXPECT_EQ(ret, nullptr);
}
 
/**
 * @tc.name  : Test GetInputSurface
 * @tc.number: GetInputSurface_002
 * @tc.desc  : Test get input surface normally
 */
HWTEST_F(WaterMarkFilterTest, GetInputSurface_002, TestSize.Level1)
{
    // Create native image window mock
    waterMarkFilter_->nativeImageWindow_ = std::make_shared<OHNativeWindow>();
    waterMarkFilter_->nativeImageWindow_->surface = new Surface();
    sptr<Surface> ret = waterMarkFilter_->GetInputSurface();
    EXPECT_NE(ret, nullptr);
}
 
/**
 * @tc.name  : Test SetOutputSurface
 * @tc.number: SetOutputSurface_001
 * @tc.desc  : Test set output surface normally
 */
HWTEST_F(WaterMarkFilterTest, SetOutputSurface_001, TestSize.Level1)
{
    sptr<Surface> surface_test = sptr<Surface>(mockSurface_.GetRefPtr());
    Status ret = waterMarkFilter_->SetOutputSurface(surface_test, 1920, 1080);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(waterMarkFilter_->targetWidth_, 1920);
    EXPECT_EQ(waterMarkFilter_->targetHeight_, 1080);
}
 
/**
 * @tc.name  : Test GetFilterType
 * @tc.number: GetFilterType_001
 * @tc.desc  : Test get filter type
 */
HWTEST_F(WaterMarkFilterTest, GetFilterType_001, TestSize.Level1)
{
    FilterType ret = waterMarkFilter_->GetFilterType();
    EXPECT_EQ(ret, FilterType::WATERMARK);
}
 
/**
 * @tc.name  : Test OnLinkedResult
 * @tc.number: OnLinkedResult_001
 * @tc.desc  : Test OnLinkedResult callback
 */
HWTEST_F(WaterMarkFilterTest, OnLinkedResult_001, TestSize.Level1)
{
    auto outputBufferQueue = sptr<AVBufferQueueProducer>(nullptr);
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    waterMarkFilter_->OnLinkedResult(outputBufferQueue, meta);
}
 
/**
 * @tc.name  : Test OnUpdatedResult
 * @tc.number: OnUpdatedResult_001
 * @tc.desc  : Test OnUpdatedResult callback
 */
HWTEST_F(WaterMarkFilterTest, OnUpdatedResult_001, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    waterMarkFilter_->OnUpdatedResult(meta);
}
 
/**
 * @tc.name  : Test OnUnlinkedResult
 * @tc.number: OnUnlinkedResult_001
 * @tc.desc  : Test OnUnlinkedResult callback
 */
HWTEST_F(WaterMarkFilterTest, OnUnlinkedResult_001, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    waterMarkFilter_->OnUnlinkedResult(meta);
}
 
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS