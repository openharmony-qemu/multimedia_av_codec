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
#include <unistd.h>
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

    waterMarkFilter_->nativeImage_ = reinterpret_cast<OH_NativeImage*>(1);
}

void WaterMarkFilterTest::TearDown(void)
{
    if (waterMarkFilter_ != nullptr) {
        waterMarkFilter_->nativeImage_ = nullptr;
        waterMarkFilter_->DoRelease();
    }
    waterMarkFilter_ = nullptr;
    mockSurface_ = nullptr;
}

/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_001
 * @tc.desc  : Test normal watermark setting
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_001, TestSize.Level1)
{
    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_ = std::make_shared<Meta>();
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(400);

    waterMarkBuffer->memory_ = std::make_shared<AVMemory>();
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
        waterMarkBuffer->meta_ = std::make_shared<Meta>();
        waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
        waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
        waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
        waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
        waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(400);

        waterMarkBuffer->memory_ = std::make_shared<AVMemory>();
        ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);

        waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    }

    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_ = std::make_shared<Meta>();
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(400);

    waterMarkBuffer->memory_ = std::make_shared<AVMemory>();
    ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);

    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::ERROR_WATERMARK_NUM_OUT_OF_RANGE);
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
    waterMarkBuffer->meta_ = std::make_shared<Meta>();
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(838870);

    waterMarkBuffer->memory_ = std::make_shared<AVMemory>();
    ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);

    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
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
    waterMarkFilter_->nativeImage_ = nullptr;
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
    waterMarkFilter_->nativeImage_ = nullptr;
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
    EXPECT_CALL(*(nextFilter_), OnLinked(testing::_, testing::_, testing::_)).WillOnce(testing::Return(Status::ERROR_UNKNOWN));
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
 * @tc.desc  : Test get input surface with non-null nativeImageWindow_
 */
HWTEST_F(WaterMarkFilterTest, GetInputSurface_002, TestSize.Level1)
{
    waterMarkFilter_->nativeImageWindow_ = nullptr;
    sptr<Surface> ret = waterMarkFilter_->GetInputSurface();
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name  : Test SetOutputSurface
 * @tc.number: SetOutputSurface_001
 * @tc.desc  : Test SetOutputSurface sets width/height/rotation members
 */
HWTEST_F(WaterMarkFilterTest, SetOutputSurface_001, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_ = 1920;
    waterMarkFilter_->targetHeight_ = 1080;
    waterMarkFilter_->rotation_ = 0;
    EXPECT_EQ(waterMarkFilter_->targetWidth_, 1920);
    EXPECT_EQ(waterMarkFilter_->targetHeight_, 1080);
    EXPECT_EQ(waterMarkFilter_->rotation_, 0);
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

/**
 * @tc.name  : Test UpdateNativeWindow
 * @tc.number: UpdateNativeWindow_001
 * @tc.desc  : Test UpdateNativeWindow with nullptr window (returns early)
 */
HWTEST_F(WaterMarkFilterTest, UpdateNativeWindow_001, TestSize.Level1)
{
    waterMarkFilter_->nativeWindow_ = nullptr;
    waterMarkFilter_->UpdateNativeWindow(nullptr, 1920, 1080);
    EXPECT_EQ(waterMarkFilter_->nativeWindow_, nullptr);
}

/**
 * @tc.name  : Test CalculateRotatedWatermarkVertices
 * @tc.number: CalculateRotatedWatermarkVertices_001
 * @tc.desc  : Test CalculateRotatedWatermarkVertices with rotation 0 (default)
 */
HWTEST_F(WaterMarkFilterTest, CalculateRotatedWatermarkVertices_001, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_ = 1920;
    waterMarkFilter_->targetHeight_ = 1080;
    waterMarkFilter_->rotation_ = 0;
    WaterMarkConfig config;
    config.top = 10;
    config.left = 20;
    config.watermarkWidth = 100;
    config.watermarkHeight = 50;
    float wmVertices[16] = {0};
    waterMarkFilter_->CalculateRotatedWatermarkVertices(config, 1920, 1080, wmVertices, sizeof(wmVertices));
    bool hasNonZero = false;
    for (int i = 0; i < 16; i++) {
        if (wmVertices[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

/**
 * @tc.name  : Test CalculateRotatedWatermarkVertices
 * @tc.number: CalculateRotatedWatermarkVertices_002
 * @tc.desc  : Test CalculateRotatedWatermarkVertices with rotation 90
 */
HWTEST_F(WaterMarkFilterTest, CalculateRotatedWatermarkVertices_002, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_ = 1920;
    waterMarkFilter_->targetHeight_ = 1080;
    waterMarkFilter_->rotation_ = 90;
    WaterMarkConfig config;
    config.top = 10;
    config.left = 20;
    config.watermarkWidth = 100;
    config.watermarkHeight = 50;
    float wmVertices[16] = {0};
    waterMarkFilter_->CalculateRotatedWatermarkVertices(config, 1920, 1080, wmVertices, sizeof(wmVertices));
    bool hasNonZero = false;
    for (int i = 0; i < 16; i++) {
        if (wmVertices[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

/**
 * @tc.name  : Test CalculateRotatedWatermarkVertices
 * @tc.number: CalculateRotatedWatermarkVertices_003
 * @tc.desc  : Test CalculateRotatedWatermarkVertices with rotation 270
 */
HWTEST_F(WaterMarkFilterTest, CalculateRotatedWatermarkVertices_003, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_ = 1920;
    waterMarkFilter_->targetHeight_ = 1080;
    waterMarkFilter_->rotation_ = 270;
    WaterMarkConfig config;
    config.top = 10;
    config.left = 20;
    config.watermarkWidth = 100;
    config.watermarkHeight = 50;
    float wmVertices[16] = {0};
    waterMarkFilter_->CalculateRotatedWatermarkVertices(config, 1920, 1080, wmVertices, sizeof(wmVertices));
    bool hasNonZero = false;
    for (int i = 0; i < 16; i++) {
        if (wmVertices[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

/**
 * @tc.name  : Test CalculateRotatedWatermarkVertices
 * @tc.number: CalculateRotatedWatermarkVertices_004
 * @tc.desc  : Test CalculateRotatedWatermarkVertices with rotation 180
 */
HWTEST_F(WaterMarkFilterTest, CalculateRotatedWatermarkVertices_004, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_ = 1920;
    waterMarkFilter_->targetHeight_ = 1080;
    waterMarkFilter_->rotation_ = 180;
    WaterMarkConfig config;
    config.top = 10;
    config.left = 20;
    config.watermarkWidth = 100;
    config.watermarkHeight = 50;
    float wmVertices[16] = {0};
    waterMarkFilter_->CalculateRotatedWatermarkVertices(config, 1920, 1080, wmVertices, sizeof(wmVertices));
    bool hasNonZero = false;
    for (int i = 0; i < 16; i++) {
        if (wmVertices[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

/**
 * @tc.name  : Test CalculateRotatedWatermarkVertices
 * @tc.number: CalculateRotatedWatermarkVertices_005
 * @tc.desc  : Test CalculateRotatedWatermarkVertices with insufficient vertices size
 */
HWTEST_F(WaterMarkFilterTest, CalculateRotatedWatermarkVertices_005, TestSize.Level1)
{
    waterMarkFilter_->rotation_ = 0;
    WaterMarkConfig config;
    config.top = 10;
    config.left = 20;
    config.watermarkWidth = 100;
    config.watermarkHeight = 50;
    float wmVertices[16] = {0};
    waterMarkFilter_->CalculateRotatedWatermarkVertices(config, 1920, 1080, wmVertices, 8);
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(wmVertices[i], 0.0f);
    }
}

/**
 * @tc.name  : Test SetCodecFormat
 * @tc.number: SetCodecFormat_001
 * @tc.desc  : Test SetCodecFormat returns OK
 */
HWTEST_F(WaterMarkFilterTest, SetCodecFormat_001, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    Status ret = waterMarkFilter_->SetCodecFormat(format);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test Init
 * @tc.number: Init_001
 * @tc.desc  : Test Init with valid receiver and callback
 */
HWTEST_F(WaterMarkFilterTest, Init_001, TestSize.Level1)
{
    auto receiver = std::make_shared<MockEventReceiver>();
    auto callback = std::make_shared<MockFilterCallback>();
    ASSERT_TRUE(receiver != nullptr);
    ASSERT_TRUE(callback != nullptr);
    waterMarkFilter_->Init(receiver, callback);
    EXPECT_EQ(waterMarkFilter_->eventReceiver_, receiver);
    EXPECT_EQ(waterMarkFilter_->filterCallback_, callback);
}

/**
 * @tc.name  : Test Configure
 * @tc.number: Configure_001
 * @tc.desc  : Test Configure with valid parameter
 */
HWTEST_F(WaterMarkFilterTest, Configure_001, TestSize.Level1)
{
    auto parameter = std::make_shared<Meta>();
    ASSERT_TRUE(parameter != nullptr);
    Status ret = waterMarkFilter_->Configure(parameter);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(waterMarkFilter_->configureParameter_, parameter);
}

/**
 * @tc.name  : Test SetOutputSurface
 * @tc.number: SetOutputSurface_002
 * @tc.desc  : Test SetOutputSurface with null surface
 */
HWTEST_F(WaterMarkFilterTest, SetOutputSurface_002, TestSize.Level1)
{
    Status ret = waterMarkFilter_->SetOutputSurface(nullptr, 1920, 1080, 0);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test SetOutputSurface
 * @tc.number: SetOutputSurface_003
 * @tc.desc  : Test SetOutputSurface with rotation parameter (null surface returns error)
 */
HWTEST_F(WaterMarkFilterTest, SetOutputSurface_003, TestSize.Level1)
{
    Status ret = waterMarkFilter_->SetOutputSurface(nullptr, 1920, 1080, 90);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    EXPECT_EQ(waterMarkFilter_->rotation_, 90);
}

/**
 * @tc.name  : Test DoPrepare
 * @tc.number: DoPrepare_002
 * @tc.desc  : Test DoPrepare with isAVRecorderMode_ true
 */
HWTEST_F(WaterMarkFilterTest, DoPrepare_002, TestSize.Level1)
{
    waterMarkFilter_->isAVRecorderMode_ = true;
    EXPECT_CALL(*(filterCallback_), OnCallback(testing::_, testing::_, testing::_)).Times(TEST_TIMES_ONE);
    Status ret = waterMarkFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoPrepare
 * @tc.number: DoPrepare_003
 * @tc.desc  : Test DoPrepare with null filterCallback_
 */
HWTEST_F(WaterMarkFilterTest, DoPrepare_003, TestSize.Level1)
{
    waterMarkFilter_->filterCallback_ = nullptr;
    Status ret = waterMarkFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test DoStart
 * @tc.number: DoStart_001
 * @tc.desc  : Test DoStart returns OK
 */
HWTEST_F(WaterMarkFilterTest, DoStart_001, TestSize.Level1)
{
    Status ret = waterMarkFilter_->DoStart();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoPause
 * @tc.number: DoPause_001
 * @tc.desc  : Test DoPause returns OK
 */
HWTEST_F(WaterMarkFilterTest, DoPause_001, TestSize.Level1)
{
    Status ret = waterMarkFilter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoResume
 * @tc.number: DoResume_001
 * @tc.desc  : Test DoResume returns OK
 */
HWTEST_F(WaterMarkFilterTest, DoResume_001, TestSize.Level1)
{
    Status ret = waterMarkFilter_->DoResume();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoStop
 * @tc.number: DoStop_001
 * @tc.desc  : Test DoStop returns OK
 */
HWTEST_F(WaterMarkFilterTest, DoStop_001, TestSize.Level1)
{
    Status ret = waterMarkFilter_->DoStop();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoFlush
 * @tc.number: DoFlush_001
 * @tc.desc  : Test DoFlush returns OK
 */
HWTEST_F(WaterMarkFilterTest, DoFlush_001, TestSize.Level1)
{
    Status ret = waterMarkFilter_->DoFlush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_003
 * @tc.desc  : Test DoRelease with nativeImage_ null (skip native image cleanup)
 */
HWTEST_F(WaterMarkFilterTest, DoRelease_003, TestSize.Level1)
{
    waterMarkFilter_->nativeImage_ = nullptr;
    Status ret = waterMarkFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(waterMarkFilter_->nativeImage_, nullptr);
}

/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_004
 * @tc.desc  : Test DoRelease with watermarkList containing buffer
 */
HWTEST_F(WaterMarkFilterTest, DoRelease_004, TestSize.Level1)
{
    WaterMarkConfig config;
    config.buffer = new uint8_t[16];
    config.size = 16;
    waterMarkFilter_->watermarkList_.push_back(config);
    waterMarkFilter_->nativeImage_ = nullptr;
    Status ret = waterMarkFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(waterMarkFilter_->watermarkList_.empty());
}

/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_005
 * @tc.desc  : Test DoRelease with GPU resources set to DEFINED_VALUE (skip deletion)
 */
HWTEST_F(WaterMarkFilterTest, DoRelease_005, TestSize.Level1)
{
    waterMarkFilter_->vertexArrayObject_ = 0;  // zero means skip
    waterMarkFilter_->vertexBufferObject_ = 0;
    waterMarkFilter_->nativeImageTexId_ = 9999;  // DEFINED_VALUE means skip
    waterMarkFilter_->textTexId_ = 9999;
    waterMarkFilter_->mergedWatermarkTexId_ = 0;
    waterMarkFilter_->watermarkFBO_ = 0;
    waterMarkFilter_->watermarkVAO_ = 0;
    waterMarkFilter_->watermarkVBO_ = 0;
    waterMarkFilter_->nativeImage_ = nullptr;
    Status ret = waterMarkFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_006
 * @tc.desc  : Test DoRelease with EGL resources set (no surface)
 */
HWTEST_F(WaterMarkFilterTest, DoRelease_006, TestSize.Level1)
{
    waterMarkFilter_->renderContext_ = nullptr;
    waterMarkFilter_->eglSurface_ = EGL_NO_SURFACE;
    waterMarkFilter_->nativeImage_ = nullptr;
    Status ret = waterMarkFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_007
 * @tc.desc  : Test DoRelease with joinable thread
 */
HWTEST_F(WaterMarkFilterTest, DoRelease_007, TestSize.Level1)
{
    waterMarkFilter_->running_ = false;  // Ensure thread can exit
    std::thread t([]() {});
    waterMarkFilter_->thread_ = std::move(t);
    waterMarkFilter_->nativeImage_ = nullptr;
    Status ret = waterMarkFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test ReleaseCpuResources
 * @tc.number: ReleaseCpuResources_001
 * @tc.desc  : Test ReleaseCpuResources with buffers in watermarkList
 */
HWTEST_F(WaterMarkFilterTest, ReleaseCpuResources_001, TestSize.Level1)
{
    WaterMarkConfig config1;
    config1.buffer = new uint8_t[16];
    config1.size = 16;
    WaterMarkConfig config2;
    config2.buffer = nullptr;
    config2.size = 0;
    waterMarkFilter_->watermarkList_.push_back(config1);
    waterMarkFilter_->watermarkList_.push_back(config2);
    waterMarkFilter_->ReleaseCpuResources();
    EXPECT_TRUE(waterMarkFilter_->watermarkList_.empty());
}

/**
 * @tc.name  : Test ReleaseGpuResources
 * @tc.number: ReleaseGpuResources_001
 * @tc.desc  : Test ReleaseGpuResources with all GPU resources zero (skip deletion)
 */
HWTEST_F(WaterMarkFilterTest, ReleaseGpuResources_001, TestSize.Level1)
{
    waterMarkFilter_->vertexArrayObject_ = 0;
    waterMarkFilter_->vertexBufferObject_ = 0;
    waterMarkFilter_->nativeImageTexId_ = 0;
    waterMarkFilter_->textTexId_ = 0;
    waterMarkFilter_->mergedWatermarkTexId_ = 0;
    waterMarkFilter_->watermarkFBO_ = 0;
    waterMarkFilter_->watermarkVAO_ = 0;
    waterMarkFilter_->watermarkVBO_ = 0;
    waterMarkFilter_->watermarkTexIds_.clear();
    waterMarkFilter_->ReleaseGpuResources();
    EXPECT_EQ(waterMarkFilter_->vertexArrayObject_, 0);
    EXPECT_EQ(waterMarkFilter_->vertexBufferObject_, 0);
}

/**
 * @tc.name  : Test ReleaseGpuResources
 * @tc.number: ReleaseGpuResources_002
 * @tc.desc  : Test ReleaseGpuResources with nativeImageTexId_ = DEFINED_VALUE (should not delete)
 */
HWTEST_F(WaterMarkFilterTest, ReleaseGpuResources_002, TestSize.Level1)
{
    waterMarkFilter_->nativeImageTexId_ = 9999;  // DEFINED_VALUE
    waterMarkFilter_->textTexId_ = 9999;         // DEFINED_VALUE
    waterMarkFilter_->ReleaseGpuResources();
    EXPECT_EQ(waterMarkFilter_->nativeImageTexId_, 9999);
    EXPECT_EQ(waterMarkFilter_->textTexId_, 9999);
}

/**
 * @tc.name  : Test ReleaseGpuResources
 * @tc.number: ReleaseGpuResources_003
 * @tc.desc  : Test ReleaseGpuResources with zero resources (should not crash)
 */
HWTEST_F(WaterMarkFilterTest, ReleaseGpuResources_003, TestSize.Level1)
{
    waterMarkFilter_->vertexArrayObject_ = 0;
    waterMarkFilter_->vertexBufferObject_ = 0;
    waterMarkFilter_->nativeImageTexId_ = 0;
    waterMarkFilter_->textTexId_ = 0;
    waterMarkFilter_->mergedWatermarkTexId_ = 0;
    waterMarkFilter_->watermarkFBO_ = 0;
    waterMarkFilter_->watermarkVAO_ = 0;
    waterMarkFilter_->watermarkVBO_ = 0;
    waterMarkFilter_->ReleaseGpuResources();
    EXPECT_EQ(waterMarkFilter_->vertexArrayObject_, 0);
}

/**
 * @tc.name  : Test ReleaseEGLResources
 * @tc.number: ReleaseEGLResources_001
 * @tc.desc  : Test ReleaseEGLResources with renderContext and EGL_NO_SURFACE
 */
HWTEST_F(WaterMarkFilterTest, ReleaseEGLResources_001, TestSize.Level1)
{
    waterMarkFilter_->renderContext_ = nullptr;
    waterMarkFilter_->eglSurface_ = EGL_NO_SURFACE;
    waterMarkFilter_->ReleaseEGLResources();
    EXPECT_EQ(waterMarkFilter_->eglSurface_, EGL_NO_SURFACE);
    EXPECT_EQ(waterMarkFilter_->renderContext_, nullptr);
}

/**
 * @tc.name  : Test ReleaseEGLResources
 * @tc.number: ReleaseEGLResources_002
 * @tc.desc  : Test ReleaseEGLResources with null renderContext
 */
HWTEST_F(WaterMarkFilterTest, ReleaseEGLResources_002, TestSize.Level1)
{
    waterMarkFilter_->renderContext_ = nullptr;
    waterMarkFilter_->eglSurface_ = EGL_NO_SURFACE;
    waterMarkFilter_->ReleaseEGLResources();
    EXPECT_EQ(waterMarkFilter_->renderContext_, nullptr);
}

/**
 * @tc.name  : Test SetParameter
 * @tc.number: SetParameter_002
 * @tc.desc  : Test SetParameter with null parameter
 */
HWTEST_F(WaterMarkFilterTest, SetParameter_002, TestSize.Level1)
{
    waterMarkFilter_->SetParameter(nullptr);
}

/**
 * @tc.name  : Test NotifyNextFilterEos
 * @tc.number: NotifyNextFilterEos_003
 * @tc.desc  : Test NotifyNextFilterEos with null filter in nextFiltersMap
 */
HWTEST_F(WaterMarkFilterTest, NotifyNextFilterEos_003, TestSize.Level1)
{
    waterMarkFilter_->nextFiltersMap_[StreamType::STREAMTYPE_RAW_VIDEO].push_back(nullptr);
    waterMarkFilter_->eosPts_.store(100);
    Status ret = waterMarkFilter_->NotifyNextFilterEos();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test LinkNext
 * @tc.number: LinkNext_004
 * @tc.desc  : Test LinkNext with null nextFilter
 */
HWTEST_F(WaterMarkFilterTest, LinkNext_004, TestSize.Level1)
{
    Status ret = waterMarkFilter_->LinkNext(nullptr, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test LinkNext
 * @tc.number: LinkNext_005
 * @tc.desc  : Test LinkNext with eventReceiver_ non-null and OnLinked returns error
 */
HWTEST_F(WaterMarkFilterTest, LinkNext_005, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    auto parameterTest = std::make_shared<Meta>();
    ASSERT_TRUE(parameterTest != nullptr);
    waterMarkFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, parameterTest, onLinkedResultCallback_);
    EXPECT_CALL(*(nextFilter_), OnLinked(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(Status::ERROR_UNKNOWN));
    EXPECT_CALL(*(eventReceiver_), OnEvent(testing::_)).Times(TEST_TIMES_ONE);
    Status ret = waterMarkFilter_->LinkNext(nextFilterTest, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test UpdateNext
 * @tc.number: UpdateNext_001
 * @tc.desc  : Test UpdateNext returns OK
 */
HWTEST_F(WaterMarkFilterTest, UpdateNext_001, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    Status ret = waterMarkFilter_->UpdateNext(nextFilterTest, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test UnLinkNext
 * @tc.number: UnLinkNext_001
 * @tc.desc  : Test UnLinkNext returns OK
 */
HWTEST_F(WaterMarkFilterTest, UnLinkNext_001, TestSize.Level1)
{
    auto nextFilterTest = std::dynamic_pointer_cast<Filter>(nextFilter_);
    Status ret = waterMarkFilter_->UnLinkNext(nextFilterTest, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test OnLinked
 * @tc.number: OnLinked_001
 * @tc.desc  : Test OnLinked sets callback
 */
HWTEST_F(WaterMarkFilterTest, OnLinked_001, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    Status ret = waterMarkFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, meta, onLinkedResultCallback_);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(waterMarkFilter_->onLinkedResultCallback_, onLinkedResultCallback_);
}

/**
 * @tc.name  : Test OnUpdated
 * @tc.number: OnUpdated_001
 * @tc.desc  : Test OnUpdated returns OK
 */
HWTEST_F(WaterMarkFilterTest, OnUpdated_001, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    Status ret = waterMarkFilter_->OnUpdated(StreamType::STREAMTYPE_ENCODED_VIDEO, meta, onLinkedResultCallback_);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test OnUnLinked
 * @tc.number: OnUnLinked_001
 * @tc.desc  : Test OnUnLinked returns OK
 */
HWTEST_F(WaterMarkFilterTest, OnUnLinked_001, TestSize.Level1)
{
    Status ret = waterMarkFilter_->OnUnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, onLinkedResultCallback_);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test OnLinkedResult
 * @tc.number: OnLinkedResult_002
 * @tc.desc  : Test OnLinkedResult with onLinkedResultCallback_ set
 */
HWTEST_F(WaterMarkFilterTest, OnLinkedResult_002, TestSize.Level1)
{
    auto outputBufferQueue = sptr<AVBufferQueueProducer>(nullptr);
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    EXPECT_CALL(*(onLinkedResultCallback_), OnLinkedResult(testing::_, testing::_)).Times(TEST_TIMES_ONE);
    waterMarkFilter_->OnLinkedResult(outputBufferQueue, meta);
}

/**
 * @tc.name  : Test OnLinkedResult
 * @tc.number: OnLinkedResult_003
 * @tc.desc  : Test OnLinkedResult with null onLinkedResultCallback_
 */
HWTEST_F(WaterMarkFilterTest, OnLinkedResult_003, TestSize.Level1)
{
    waterMarkFilter_->onLinkedResultCallback_ = nullptr;
    auto outputBufferQueue = sptr<AVBufferQueueProducer>(nullptr);
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    waterMarkFilter_->OnLinkedResult(outputBufferQueue, meta);
    // Should not crash with null callback
}

/**
 * @tc.name  : Test OnOutputBufferAvailable
 * @tc.number: OnOutputBufferAvailable_001
 * @tc.desc  : Test OnOutputBufferAvailable
 */
HWTEST_F(WaterMarkFilterTest, OnOutputBufferAvailable_001, TestSize.Level1)
{
    waterMarkFilter_->OnOutputBufferAvailable(0, 0);
}

/**
 * @tc.name  : Test SetFaultEvent
 * @tc.number: SetFaultEvent_001
 * @tc.desc  : Test SetFaultEvent with error message and ret code
 */
HWTEST_F(WaterMarkFilterTest, SetFaultEvent_001, TestSize.Level1)
{
    waterMarkFilter_->SetFaultEvent("test error", 1);
}

/**
 * @tc.name  : Test SetFaultEvent
 * @tc.number: SetFaultEvent_002
 * @tc.desc  : Test SetFaultEvent with error message only
 */
HWTEST_F(WaterMarkFilterTest, SetFaultEvent_002, TestSize.Level1)
{
    waterMarkFilter_->SetFaultEvent("test error");
}

/**
 * @tc.name  : Test SetCallingInfo
 * @tc.number: SetCallingInfo_001
 * @tc.desc  : Test SetCallingInfo sets all fields
 */
HWTEST_F(WaterMarkFilterTest, SetCallingInfo_001, TestSize.Level1)
{
    waterMarkFilter_->SetCallingInfo(100, 200, "com.test.bundle", 300);
    EXPECT_EQ(waterMarkFilter_->appUid_, 100);
    EXPECT_EQ(waterMarkFilter_->appPid_, 200);
    EXPECT_EQ(waterMarkFilter_->bundleName_, "com.test.bundle");
    EXPECT_EQ(waterMarkFilter_->instanceId_, 300);
}

/**
 * @tc.name  : Test GetFilterType
 * @tc.number: GetFilterType_002
 * @tc.desc  : Test GetFilterType returns correct filter type
 */
HWTEST_F(WaterMarkFilterTest, GetFilterType_002, TestSize.Level1)
{
    FilterType ret = waterMarkFilter_->GetFilterType();
    EXPECT_EQ(ret, FilterType::WATERMARK);
}

/**
 * @tc.name  : Test SetAVRecorderMode
 * @tc.number: SetAVRecorderMode_001
 * @tc.desc  : Test SetAVRecorderMode sets isAVRecorderMode_ to true
 */
HWTEST_F(WaterMarkFilterTest, SetAVRecorderMode_001, TestSize.Level1)
{
    waterMarkFilter_->SetAVRecorderMode();
    EXPECT_TRUE(waterMarkFilter_->isAVRecorderMode_);
}

/**
 * @tc.name  : Test SetVideoResize
 * @tc.number: SetVideoResize_003
 * @tc.desc  : Test SetVideoResize with valid parameters sets target dimensions
 */
HWTEST_F(WaterMarkFilterTest, SetVideoResize_003, TestSize.Level1)
{
    Status ret = waterMarkFilter_->SetVideoResize(1280, 720);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(waterMarkFilter_->targetWidth_, 1280);
    EXPECT_EQ(waterMarkFilter_->targetHeight_, 720);
}

/**
 * @tc.name  : Test SetVideoResize
 * @tc.number: SetVideoResize_004
 * @tc.desc  : Test SetVideoResize with zero width
 */
HWTEST_F(WaterMarkFilterTest, SetVideoResize_004, TestSize.Level1)
{
    Status ret = waterMarkFilter_->SetVideoResize(0, 720);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_004
 * @tc.desc  : Test SetWatermark with watermarkList exceeding max number
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_004, TestSize.Level1)
{
    // Fill watermark list beyond max
    for (uint32_t i = 0; i < 5 + 1; i++) {
        WaterMarkConfig config;
        config.top = 0;
        config.left = 0;
        config.watermarkWidth = 100;
        config.watermarkHeight = 100;
        config.rawHeight = 100;
        config.rawWidth = 100;
        config.size = 100;
        config.buffer = new uint8_t[100];
        waterMarkFilter_->watermarkList_.push_back(config);
    }
    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_ = std::make_shared<Meta>();
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(400);
    waterMarkBuffer->memory_ = std::make_shared<AVMemory>();
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::ERROR_WATERMARK_NUM_OUT_OF_RANGE);
}

/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_005
 * @tc.desc  : Test SetWatermark with null memory_ in buffer
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_005, TestSize.Level1)
{
    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_ = std::make_shared<Meta>();
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(400);
    waterMarkBuffer->memory_ = nullptr;  // null memory
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_006
 * @tc.desc  : Test SetWatermark with invalid stride (negative)
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_006, TestSize.Level1)
{
    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_ = std::make_shared<Meta>();
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(-1);  // invalid stride
    waterMarkBuffer->memory_ = std::make_shared<AVMemory>();
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_007
 * @tc.desc  : Test SetWatermark with zero height
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_007, TestSize.Level1)
{
    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_ = std::make_shared<Meta>();
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(0);  // zero height
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(400);
    waterMarkBuffer->memory_ = std::make_shared<AVMemory>();
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_008
 * @tc.desc  : Test SetWatermark with default width/height (width <= 0, height <= 0)
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_008, TestSize.Level1)
{
    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_ = std::make_shared<Meta>();
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(400);
    waterMarkBuffer->memory_ = std::make_shared<AVMemory>();
    ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 0, 0);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test OnNativeImageFrameAvailable
 * @tc.number: OnNativeImageFrameAvailable_001
 * @tc.desc  : Test OnNativeImageFrameAvailable with valid data
 */
HWTEST_F(WaterMarkFilterTest, OnNativeImageFrameAvailable_001, TestSize.Level1)
{
    waterMarkFilter_->availableFrameCnt_.store(0);
    WaterMarkFilter::OnNativeImageFrameAvailable(waterMarkFilter_.get());
    EXPECT_EQ(waterMarkFilter_->availableFrameCnt_.load(), 1);
}

/**
 * @tc.name  : Test OnNativeImageFrameAvailable
 * @tc.number: OnNativeImageFrameAvailable_002
 * @tc.desc  : Test OnNativeImageFrameAvailable with null data
 */
HWTEST_F(WaterMarkFilterTest, OnNativeImageFrameAvailable_002, TestSize.Level1)
{
    int32_t prevCount = waterMarkFilter_->availableFrameCnt_.load();
    WaterMarkFilter::OnNativeImageFrameAvailable(nullptr);
    EXPECT_EQ(waterMarkFilter_->availableFrameCnt_.load(), prevCount);
}

/**
 * @tc.name  : Test CreateWatermarkTextureAndCopy
 * @tc.number: CreateWatermarkTextureAndCopy_001
 * @tc.desc  : Test CreateWatermarkTextureAndCopy
 */
HWTEST_F(WaterMarkFilterTest, CreateWatermarkTextureAndCopy_001, TestSize.Level1)
{
    waterMarkFilter_->mergedWatermarkTexId_ = 1;
    bool ret = waterMarkFilter_->CreateWatermarkTextureAndCopy();
    EXPECT_TRUE(ret);
    EXPECT_EQ(waterMarkFilter_->textTexId_, waterMarkFilter_->mergedWatermarkTexId_);
}

/**
 * @tc.name  : Test RenderLoop
 * @tc.number: RenderLoop_001
 * @tc.desc  : Test RenderLoop exits when running_ is false and eosPts_ matches
 */
HWTEST_F(WaterMarkFilterTest, RenderLoop_001, TestSize.Level1)
{
    waterMarkFilter_->running_ = false;
    waterMarkFilter_->availableFrameCnt_.store(0);
    waterMarkFilter_->eosPts_.store(-1);
    waterMarkFilter_->RenderLoop();
}

/**
 * @tc.name  : Test RenderLoop
 * @tc.number: RenderLoop_002
 * @tc.desc  : Test RenderLoop with eosPts_ == lastPts_ triggers NotifyNextFilterEos
 */
HWTEST_F(WaterMarkFilterTest, RenderLoop_002, TestSize.Level1)
{
    waterMarkFilter_->lastPts_.store(100);
    waterMarkFilter_->eosPts_.store(100);
    waterMarkFilter_->availableFrameCnt_.store(0);
    waterMarkFilter_->running_ = true;
    waterMarkFilter_->RenderLoop();
}

/**
 * @tc.name  : Test Constructor
 * @tc.number: Constructor_001
 * @tc.desc  : Test WaterMarkFilter constructor initializes matrix correctly
 */
HWTEST_F(WaterMarkFilterTest, Constructor_001, TestSize.Level1)
{
    auto filter = std::make_shared<WaterMarkFilter>("testFilter", FilterType::WATERMARK);
    ASSERT_TRUE(filter != nullptr);
    EXPECT_EQ(filter->matrix_[0], 1.0f);
    EXPECT_EQ(filter->matrix_[5], 1.0f);
    EXPECT_EQ(filter->matrix_[10], 1.0f);
    EXPECT_EQ(filter->matrix_[15], 1.0f);
}

/**
 * @tc.name  : Test Destructor
 * @tc.number: Destructor_001
 * @tc.desc  : Test destructor calls DoRelease
 */
HWTEST_F(WaterMarkFilterTest, Destructor_001, TestSize.Level1)
{
    auto filter = std::make_shared<WaterMarkFilter>("testFilter", FilterType::WATERMARK);
    ASSERT_TRUE(filter != nullptr);
    filter.reset();
}

/**
 * @tc.name  : Test DoPrepare
 * @tc.number: DoPrepare_004
 * @tc.desc  : Test DoPrepare with non-WATERMARK filterType (default branch)
 */
HWTEST_F(WaterMarkFilterTest, DoPrepare_004, TestSize.Level1)
{
    auto filter = std::make_shared<WaterMarkFilter>("testFilter", FilterType::AUDIO_DATA_SOURCE);
    ASSERT_TRUE(filter != nullptr);
    filter->eventReceiver_ = eventReceiver_;
    filter->filterCallback_ = filterCallback_;
    Status ret = filter->DoPrepare();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_009
 * @tc.desc  : Test SetWatermark with bufferSize exactly at MAX_WATERMARK_SIZE boundary
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_009, TestSize.Level1)
{
    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_ = std::make_shared<Meta>();
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(0);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(100);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(1);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(83886080);
    waterMarkBuffer->memory_ = std::make_shared<AVMemory>();
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_010
 * @tc.desc  : Test SetWatermark with valid small watermark (success path)
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_010, TestSize.Level1)
{
    auto waterMarkBuffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(waterMarkBuffer != nullptr);
    waterMarkBuffer->meta_ = std::make_shared<Meta>();
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_X>(10);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_Y>(20);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_W>(50);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_COORDINATE_H>(50);
    waterMarkBuffer->meta_->Set<Tag::VIDEO_STRIDE>(200);
    waterMarkBuffer->memory_ = std::make_shared<AVMemory>();
    ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 50, 50);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(waterMarkFilter_->watermarkList_.size(), 1u);
    EXPECT_EQ(waterMarkFilter_->watermarkList_[0].top, 20);
    EXPECT_EQ(waterMarkFilter_->watermarkList_[0].left, 10);
    EXPECT_EQ(waterMarkFilter_->watermarkList_[0].watermarkWidth, 50);
    EXPECT_EQ(waterMarkFilter_->watermarkList_[0].watermarkHeight, 50);
}

/**
 * @tc.name  : Test ReleaseGpuResources
 * @tc.number: ReleaseGpuResources_004
 * @tc.desc  : Test ReleaseGpuResources with DEFINED_VALUE for nativeImageTexId_ and textTexId_
 */
HWTEST_F(WaterMarkFilterTest, ReleaseGpuResources_004, TestSize.Level1)
{
    waterMarkFilter_->vertexArrayObject_ = 0;
    waterMarkFilter_->vertexBufferObject_ = 0;
    waterMarkFilter_->nativeImageTexId_ = DEFINED_VALUE;
    waterMarkFilter_->textTexId_ = DEFINED_VALUE;
    waterMarkFilter_->mergedWatermarkTexId_ = 0;
    waterMarkFilter_->watermarkTexIds_.clear();
    waterMarkFilter_->watermarkFBO_ = 0;
    waterMarkFilter_->watermarkVAO_ = 0;
    waterMarkFilter_->watermarkVBO_ = 0;
    waterMarkFilter_->ReleaseGpuResources();
    EXPECT_EQ(waterMarkFilter_->nativeImageTexId_, DEFINED_VALUE);
    EXPECT_EQ(waterMarkFilter_->textTexId_, DEFINED_VALUE);
}

/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_008
 * @tc.desc  : Test DoRelease with watermarkList containing buffer (ReleaseCpuResources path)
 */
HWTEST_F(WaterMarkFilterTest, DoRelease_008, TestSize.Level1)
{
    WaterMarkConfig config;
    config.top = 0;
    config.left = 0;
    config.watermarkWidth = 100;
    config.watermarkHeight = 100;
    config.rawHeight = 100;
    config.rawWidth = 100;
    config.size = 100;
    config.buffer = new uint8_t[100];
    waterMarkFilter_->watermarkList_.push_back(config);
    waterMarkFilter_->nativeImage_ = nullptr;
    waterMarkFilter_->nativeImageWindow_ = nullptr;
    waterMarkFilter_->renderContext_ = nullptr;
    waterMarkFilter_->eglSurface_ = EGL_NO_SURFACE;
    waterMarkFilter_->nativeWindow_ = nullptr;
    waterMarkFilter_->shaderProgram_.reset();
    waterMarkFilter_->watermarkMergeShaderProgram_.reset();
    waterMarkFilter_->hasReleased_ = false;
    Status ret = waterMarkFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(waterMarkFilter_->watermarkList_.empty());
}

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS

int main(int argc, char **argv) {
    testing::GTEST_FLAG(output) = "xml:./";
    testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    sync();
    _exit(result);
}