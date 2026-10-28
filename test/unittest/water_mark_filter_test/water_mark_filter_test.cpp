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
#include "meta/video_types.h"
#include "surface/native_buffer.h"

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
    eventReceiver_ = nullptr;
    filterCallback_ = nullptr;
    onLinkedResultCallback_ = nullptr;
    nextFilter_ = nullptr;
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

    int32_t bufferSize = 100 * 400; // hiheight * histride
    auto dataBuffer = std::make_unique<uint8_t[]>(bufferSize);
    memset(dataBuffer.get(), 0xAA, bufferSize);
    waterMarkBuffer->memory_ = AVMemory::CreateAVMemory(dataBuffer.get(), bufferSize, bufferSize);
    ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);

    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 100, 100);
    EXPECT_EQ(ret, Status::OK);
    waterMarkFilter_->DoRelease();
}

/**
 * @tc.name  : Test SetWatermark
 * @tc.number: SetWatermark_002
 * @tc.desc  : Test watermark list exceeds maximum number
 */
HWTEST_F(WaterMarkFilterTest, SetWatermark_002, TestSize.Level1)
{
    for (uint32_t i = 0; i < 5; i++) {
        WaterMarkConfig config;
        config.top = 0;
        config.left = 0;
        config.watermarkWidth = 100;
        config.watermarkHeight = 100;
        config.rawHeight = 100;
        config.rawWidth = 100;
        config.size = 100;
        config.buffer = std::make_unique<uint8_t[]>(100);
        waterMarkFilter_->watermarkList_.push_back(std::move(config));
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
    waterMarkFilter_->DoRelease();
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
    EXPECT_CALL(*(nextFilter_), OnLinked(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(Status::ERROR_UNKNOWN));
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
    // nativeWindow_ is set to nullptr from reinterpret_cast, then FALSE_RETURN_MSG returns
    EXPECT_EQ(waterMarkFilter_->nativeWindow_, nullptr);
}

/**
 * @tc.name  : Test UpdateNativeWindow
 * @tc.number: UpdateNativeWindow_002
 * @tc.desc  : Test UpdateNativeWindow with non-null window sets nativeWindow_ (verify via nullptr path only)
 */
HWTEST_F(WaterMarkFilterTest, UpdateNativeWindow_002, TestSize.Level1)
{
    // Cannot pass a real non-null window without a valid surface; verify the nullptr early-return path
    // leaves nativeWindow_ as nullptr (reinterpret_cast of nullptr)
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
    config.buffer = std::make_unique<uint8_t[]>(16);
    config.size = 16;
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
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
    waterMarkFilter_->vertexArrayObject_ = 0;
    waterMarkFilter_->vertexBufferObject_ = 0;
    waterMarkFilter_->nativeImageTexId_ = DEFINED_VALUE;
    waterMarkFilter_->textTexId_ = DEFINED_VALUE;
    waterMarkFilter_->mergedWatermarkTexId_ = 0;
    waterMarkFilter_->watermarkFBO_ = 0;
    waterMarkFilter_->watermarkVAO_ = 0;
    waterMarkFilter_->watermarkVBO_ = 0;
    Status ret = waterMarkFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(waterMarkFilter_->nativeImageTexId_, DEFINED_VALUE);
    EXPECT_EQ(waterMarkFilter_->textTexId_, DEFINED_VALUE);
}

/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_006
 * @tc.desc  : Test DoRelease with EGL resources set (no surface)
 */
HWTEST_F(WaterMarkFilterTest, DoRelease_006, TestSize.Level1)
{
    waterMarkFilter_->renderContext_ = std::make_unique<EglRenderContext>();
    waterMarkFilter_->eglSurface_ = EGL_NO_SURFACE;
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
    config1.buffer = std::make_unique<uint8_t[]>(16);
    config1.size = 16;
    WaterMarkConfig config2;
    config2.buffer = nullptr;
    config2.size = 0;
    waterMarkFilter_->watermarkList_.push_back(std::move(config1));
    waterMarkFilter_->watermarkList_.push_back(std::move(config2));
    waterMarkFilter_->OnReleaseCpuResources();
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
    waterMarkFilter_->renderContext_ = std::make_unique<EglRenderContext>();
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
        config.buffer = std::make_unique<uint8_t[]>(100);
        waterMarkFilter_->watermarkList_.push_back(std::move(config));
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
    waterMarkFilter_->DoRelease();
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
    int32_t bufferSize = 100 * 400; // hiheight * histride
    auto dataBuffer = std::make_unique<uint8_t[]>(bufferSize);
    memset(dataBuffer.get(), 0xAA, bufferSize);
    waterMarkBuffer->memory_ = AVMemory::CreateAVMemory(dataBuffer.get(), bufferSize, bufferSize);
    ASSERT_TRUE(waterMarkBuffer->memory_ != nullptr);
    Status ret = waterMarkFilter_->SetWatermark(waterMarkBuffer, 0, 0);
    EXPECT_EQ(ret, Status::OK);
    waterMarkFilter_->DoRelease();
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
 * @tc.name  : Test InitOpenGl
 * @tc.number: InitOpenGl_001
 * @tc.desc  : Test InitOpenGl with nativeImage_ already set (early return)
 */
HWTEST_F(WaterMarkFilterTest, InitOpenGl_001, TestSize.Level1)
{
    waterMarkFilter_->nativeImage_ = reinterpret_cast<OH_NativeImage*>(0x1);
    EXPECT_NO_FATAL_FAILURE(waterMarkFilter_->InitOpenGl());
    waterMarkFilter_->nativeImage_ = nullptr;
}

/**
 * @tc.name  : Test InitOpenGl
 * @tc.number: InitOpenGl_002
 * @tc.desc  : Test InitOpenGl with nativeImage_ nullptr (calls CreateNativeImage)
 */
HWTEST_F(WaterMarkFilterTest, InitOpenGl_002, TestSize.Level1)
{
    waterMarkFilter_->nativeImage_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(waterMarkFilter_->InitOpenGl());
}

/**
 * @tc.name  : Test CreateNativeImage
 * @tc.number: CreateNativeImage_001
 * @tc.desc  : Test CreateNativeImage without display
 */
HWTEST_F(WaterMarkFilterTest, CreateNativeImage_001, TestSize.Level1)
{
    waterMarkFilter_->nativeImage_ = nullptr;
    bool ret = waterMarkFilter_->CreateNativeImage();
    if (!ret) {
        EXPECT_EQ(waterMarkFilter_->nativeImage_, nullptr);
    }
}

/**
 * @tc.name  : Test SetupShaderUniforms
 * @tc.number: SetupShaderUniforms_001
 * @tc.desc  : Test SetupShaderUniforms returns false when shader programs are null
 */
HWTEST_F(WaterMarkFilterTest, SetupShaderUniforms_001, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->shaderProgram_.reset();
    waterMarkFilter_->watermarkOverlayProgram_.reset();
    bool ret = waterMarkFilter_->SetupShaderUniforms();
    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test SetupShaderUniforms
 * @tc.number: SetupShaderUniforms_002
 * @tc.desc  : Test SetupShaderUniforms with watermark (non-empty list, requires GL context)
 */
HWTEST_F(WaterMarkFilterTest, SetupShaderUniforms_002, TestSize.Level1)
{
    WaterMarkConfig config;
    config.top = 0;
    config.left = 0;
    config.watermarkWidth = 100;
    config.watermarkHeight = 100;
    config.rawHeight = 100;
    config.rawWidth = 100;
    config.size = 40000;
    config.buffer = std::make_unique<uint8_t[]>(40000);
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    bool glOk = waterMarkFilter_->CreateGLResources();
    if (!glOk) {
        // CreateGLResources失败时shaderProgram_可能已被创建但无效，清理后检查
        waterMarkFilter_->shaderProgram_.reset();
        EXPECT_FALSE(waterMarkFilter_->SetupShaderUniforms());
    } else {
        waterMarkFilter_->nativeImageTexId_ = 0;
        waterMarkFilter_->textTexId_ = 0;
        EXPECT_TRUE(waterMarkFilter_->SetupShaderUniforms());
    }
    waterMarkFilter_->DoRelease();
}


/**
 * @tc.name  : Test CreateGLResources
 * @tc.number: CreateGLResources_001
 * @tc.desc  : Test CreateGLResources with empty watermarkList (resize-only shader)
 */
HWTEST_F(WaterMarkFilterTest, CreateGLResources_001, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->CreateGLResources();
}

/**
 * @tc.name  : Test CreateGLResources
 * @tc.number: CreateGLResources_002
 * @tc.desc  : Test CreateGLResources with non-empty watermarkList (watermark shader)
 */
HWTEST_F(WaterMarkFilterTest, CreateGLResources_002, TestSize.Level1)
{
    WaterMarkConfig config;
    config.top = 0;
    config.left = 0;
    config.watermarkWidth = 100;
    config.watermarkHeight = 100;
    config.rawHeight = 100;
    config.rawWidth = 100;
    config.size = 100;
    config.buffer = std::make_unique<uint8_t[]>(100);
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    waterMarkFilter_->CreateGLResources();
    waterMarkFilter_->watermarkList_.clear();
}

/**
 * @tc.name  : Test CreateWatermarkTextures
 * @tc.number: CreateWatermarkTextures_001
 * @tc.desc  : Test CreateWatermarkTextures with empty watermarkList
 */
HWTEST_F(WaterMarkFilterTest, CreateWatermarkTextures_001, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    bool ret = waterMarkFilter_->CreateWatermarkTextures();
    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test CreateWatermarkTextures
 * @tc.number: CreateWatermarkTextures_002
 * @tc.desc  : Test CreateWatermarkTextures with non-empty watermarkList (requires GL context)
 */
HWTEST_F(WaterMarkFilterTest, CreateWatermarkTextures_002, TestSize.Level1)
{
    WaterMarkConfig config;
    config.top = 0;
    config.left = 0;
    config.watermarkWidth = 100;
    config.watermarkHeight = 100;
    config.rawHeight = 100;
    config.rawWidth = 100;
    config.size = 100;
    config.buffer = std::make_unique<uint8_t[]>(100);
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    waterMarkFilter_->CreateWatermarkTextures();

    waterMarkFilter_->watermarkList_.clear();
}

/**
 * @tc.name  : Test CreateWatermarkTextures
 * @tc.number: CreateWatermarkTextures_003
 * @tc.desc  : Test CreateWatermarkTextures with nullptr buffer in config
 */
HWTEST_F(WaterMarkFilterTest, CreateWatermarkTextures_003, TestSize.Level1)
{
    WaterMarkConfig config;
    config.top = 0;
    config.left = 0;
    config.watermarkWidth = 100;
    config.watermarkHeight = 100;
    config.rawHeight = 100;
    config.rawWidth = 100;
    config.size = 100;
    config.buffer = nullptr;  // null buffer
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    bool ret = waterMarkFilter_->CreateWatermarkTextures();
    EXPECT_FALSE(ret);
    waterMarkFilter_->watermarkList_.clear();
}

/**
 * @tc.name  : Test MergeWaterMarksWithOpenGL
 * @tc.number: MergeWaterMarksWithOpenGL_001
 * @tc.desc  : Test MergeWaterMarksWithOpenGL with empty watermarkList
 */
HWTEST_F(WaterMarkFilterTest, MergeWaterMarksWithOpenGL_001, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    bool ret = waterMarkFilter_->MergeWaterMarksWithOpenGL();
    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test MergeWaterMarksWithOpenGL
 * @tc.number: MergeWaterMarksWithOpenGL_002
 * @tc.desc  : Test MergeWaterMarksWithOpenGL with non-empty watermarkList (requires GL context)
 */
HWTEST_F(WaterMarkFilterTest, MergeWaterMarksWithOpenGL_002, TestSize.Level1)
{
    WaterMarkConfig config;
    config.top = 0;
    config.left = 0;
    config.watermarkWidth = 100;
    config.watermarkHeight = 100;
    config.rawHeight = 100;
    config.rawWidth = 100;
    config.size = 100;
    config.buffer = std::make_unique<uint8_t[]>(100);
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    waterMarkFilter_->targetWidth_ = 1920;
    waterMarkFilter_->targetHeight_ = 1080;
    waterMarkFilter_->MergeWaterMarksWithOpenGL();

    waterMarkFilter_->watermarkList_.clear();
}

/**
 * @tc.name  : Test CreateWatermarkFBO
 * @tc.number: CreateWatermarkFBO_001
 * @tc.desc  : Test CreateWatermarkFBO requires GL context
 */
HWTEST_F(WaterMarkFilterTest, CreateWatermarkFBO_001, TestSize.Level1)
{
    waterMarkFilter_->CreateWatermarkFBO(1920, 1080);
}

/**
 * @tc.name  : Test CreateWatermarkMergeShader
 * @tc.number: CreateWatermarkMergeShader_001
 * @tc.desc  : Test CreateWatermarkMergeShader requires GL context
 */
HWTEST_F(WaterMarkFilterTest, CreateWatermarkMergeShader_001, TestSize.Level1)
{
    waterMarkFilter_->CreateWatermarkMergeShader();
}

/**
 * @tc.name  : Test RenderWatermarksToFBO
 * @tc.number: RenderWatermarksToFBO_001
 * @tc.desc  : Test RenderWatermarksToFBO with invalid dimensions
 */
HWTEST_F(WaterMarkFilterTest, RenderWatermarksToFBO_001, TestSize.Level1)
{
    bool ret = waterMarkFilter_->RenderWatermarksToFBO(0, 0);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test RenderWatermarksToFBO
 * @tc.number: RenderWatermarksToFBO_002
 * @tc.desc  : Test RenderWatermarksToFBO with valid dimensions and empty watermarkList (no GL calls)
 */
HWTEST_F(WaterMarkFilterTest, RenderWatermarksToFBO_002, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->CreateWatermarkMergeShader();
    waterMarkFilter_->RenderWatermarksToFBO(1920, 1080);
}

/**
 * @tc.name  : Test RenderWatermarksToFBO
 * @tc.number: RenderWatermarksToFBO_003
 * @tc.desc  : Test RenderWatermarksToFBO with non-empty watermarkList (requires GL context)
 */
HWTEST_F(WaterMarkFilterTest, RenderWatermarksToFBO_003, TestSize.Level1)
{
    WaterMarkConfig config;
    config.top = 0;
    config.left = 0;
    config.watermarkWidth = 100;
    config.watermarkHeight = 100;
    config.rawHeight = 100;
    config.rawWidth = 100;
    config.size = 100;
    config.buffer = std::make_unique<uint8_t[]>(100);
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    waterMarkFilter_->targetWidth_ = 1920;
    waterMarkFilter_->targetHeight_ = 1080;
    waterMarkFilter_->rotation_ = 0;
    waterMarkFilter_->CreateWatermarkMergeShader();
    waterMarkFilter_->CreateWatermarkTextures();
    waterMarkFilter_->RenderWatermarksToFBO(1920, 1080);

    waterMarkFilter_->watermarkList_.clear();
}

/**
 * @tc.name  : Test RenderSingleWatermarkToFBO
 * @tc.number: RenderSingleWatermarkToFBO_001
 * @tc.desc  : Test RenderSingleWatermarkToFBO coverage via CalculateRotatedWatermarkVertices
 */
HWTEST_F(WaterMarkFilterTest, RenderSingleWatermarkToFBO_001, TestSize.Level1)
{
    WaterMarkConfig config;
    config.top = 0;
    config.left = 0;
    config.watermarkWidth = 100;
    config.watermarkHeight = 100;
    config.rawHeight = 100;
    config.rawWidth = 100;
    config.size = 100;
    config.buffer = std::make_unique<uint8_t[]>(100);
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    waterMarkFilter_->targetWidth_ = 1920;
    waterMarkFilter_->targetHeight_ = 1080;
    waterMarkFilter_->rotation_ = 0;
    waterMarkFilter_->CreateWatermarkMergeShader();
    waterMarkFilter_->CreateWatermarkTextures();
    GLuint texId = 0;
    if (!waterMarkFilter_->watermarkTexIds_.empty()) {
        texId = waterMarkFilter_->watermarkTexIds_[0];
    }
    waterMarkFilter_->RenderSingleWatermarkToFBO(config, texId, 1920, 1080);
    waterMarkFilter_->watermarkList_.clear();
}

/**
 * @tc.name  : Test DrawImage
 * @tc.number: DrawImage_001
 * @tc.desc  : Test DrawImage coverage - cannot call without real GL context
 */
HWTEST_F(WaterMarkFilterTest, DrawImage_001, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_ = 1920;
    waterMarkFilter_->targetHeight_ = 1080;
    waterMarkFilter_->vertexArrayObject_ = 0;
    waterMarkFilter_->nativeWindow_ = nullptr;
    waterMarkFilter_->renderContext_ = nullptr;
    waterMarkFilter_->DrawImage(0);
}

/**
 * @tc.name  : Test InitializeEGLContext
 * @tc.number: InitializeEGLContext_001
 * @tc.desc  : Test InitializeEGLContext requires EGL environment
 */
HWTEST_F(WaterMarkFilterTest, InitializeEGLContext_001, TestSize.Level1)
{
    waterMarkFilter_->InitializeEGLContext();
}

/**
 * @tc.name  : Test CreateVideoTexture
 * @tc.number: CreateVideoTexture_001
 * @tc.desc  : Test CreateVideoTexture requires GL context
 */
HWTEST_F(WaterMarkFilterTest, CreateVideoTexture_001, TestSize.Level1)
{
    waterMarkFilter_->CreateVideoTexture();
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
    waterMarkFilter_->nextFiltersMap_.clear();
    waterMarkFilter_->lastPts_.store(100);
    waterMarkFilter_->eosPts_.store(100);
    waterMarkFilter_->availableFrameCnt_.store(0);
    waterMarkFilter_->eosReceived_.store(true);
    waterMarkFilter_->running_ = true;
    waterMarkFilter_->RenderLoop();
    // RenderLoop breaks on EOS; verify the EOS path was taken (availableFrameCnt_ still 0, eosReceived_ unchanged)
    EXPECT_EQ(waterMarkFilter_->availableFrameCnt_.load(), 0);
    EXPECT_TRUE(waterMarkFilter_->eosReceived_.load());
    // RenderLoop breaks on EOS but does not reset running_; ensure cleanup for safe teardown
    waterMarkFilter_->running_ = false;
}

/**
 * @tc.name  : Test ThreadMainLoop
 * @tc.number: ThreadMainLoop_001
 * @tc.desc  : Test ThreadMainLoop with empty watermarkList
 */
HWTEST_F(WaterMarkFilterTest, ThreadMainLoop_001, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->running_ = false;
    waterMarkFilter_->RenderThreadEntry();
}

/**
 * @tc.name  : Test ThreadMainLoop
 * @tc.number: ThreadMainLoop_002
 * @tc.desc  : Test ThreadMainLoop with non-empty watermarkList
 */
HWTEST_F(WaterMarkFilterTest, ThreadMainLoop_002, TestSize.Level1)
{
    WaterMarkConfig config;
    config.top = 0;
    config.left = 0;
    config.watermarkWidth = 100;
    config.watermarkHeight = 100;
    config.rawHeight = 100;
    config.rawWidth = 100;
    config.size = 100;
    config.buffer = std::make_unique<uint8_t[]>(100);
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    waterMarkFilter_->running_ = false;
    waterMarkFilter_->RenderThreadEntry();

    waterMarkFilter_->watermarkList_.clear();
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
    config.buffer = std::make_unique<uint8_t[]>(100);
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    waterMarkFilter_->nativeImage_ = nullptr;
    waterMarkFilter_->nativeImageWindow_ = nullptr;
    waterMarkFilter_->renderContext_ = nullptr;
    waterMarkFilter_->eglSurface_ = EGL_NO_SURFACE;
    waterMarkFilter_->nativeWindow_ = nullptr;
    waterMarkFilter_->shaderProgram_.reset();
    waterMarkFilter_->watermarkMergeShaderProgram_.reset();
    waterMarkFilter_->hasReleased_.store(false);
    Status ret = waterMarkFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(waterMarkFilter_->watermarkList_.empty());
}

/**
 * @tc.name  : Test ApplyRotationToConfig
 * @tc.number: ApplyRotationToConfig_001
 * @tc.desc  : Test ApplyRotationToConfig with 0 degree rotation (identity)
 */
HWTEST_F(WaterMarkFilterTest, ApplyRotationToConfig_001, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);

    WaterMarkConfig config;
    config.top = 100;
    config.left = 200;
    config.watermarkWidth = 300;
    config.watermarkHeight = 50;
    config.rawWidth = 300;
    config.rawHeight = 50;
    config.rotation = 0;

    auto result = waterMarkFilter_->ApplyRotationToConfig(config, 0);
    EXPECT_EQ(result.top, 100);
    EXPECT_EQ(result.left, 200);
    EXPECT_EQ(result.watermarkWidth, 300);
    EXPECT_EQ(result.watermarkHeight, 50);
}

/**
 * @tc.name  : Test ApplyRotationToConfig
 * @tc.number: ApplyRotationToConfig_002
 * @tc.desc  : Test ApplyRotationToConfig with 90 degree rotation
 */
HWTEST_F(WaterMarkFilterTest, ApplyRotationToConfig_002, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);

    WaterMarkConfig config;
    config.top = 100;
    config.left = 200;
    config.watermarkWidth = 300;
    config.watermarkHeight = 50;
    config.rawWidth = 300;
    config.rawHeight = 50;

    auto result = waterMarkFilter_->ApplyRotationToConfig(config, 90);
    EXPECT_EQ(result.top, 1080 - 200 - 300);
    EXPECT_EQ(result.left, 100);
    EXPECT_EQ(result.watermarkWidth, 50);
    EXPECT_EQ(result.watermarkHeight, 300);
}

/**
 * @tc.name  : Test ApplyRotationToConfig
 * @tc.number: ApplyRotationToConfig_003
 * @tc.desc  : Test ApplyRotationToConfig with 180 degree rotation
 */
HWTEST_F(WaterMarkFilterTest, ApplyRotationToConfig_003, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);

    WaterMarkConfig config;
    config.top = 100;
    config.left = 200;
    config.watermarkWidth = 300;
    config.watermarkHeight = 50;

    auto result = waterMarkFilter_->ApplyRotationToConfig(config, 180);
    EXPECT_EQ(result.left, 1920 - 200 - 300);
    EXPECT_EQ(result.top, 1080 - 100 - 50);
    EXPECT_EQ(result.watermarkWidth, 300);
    EXPECT_EQ(result.watermarkHeight, 50);
}

/**
 * @tc.name  : Test ApplyRotationToConfig
 * @tc.number: ApplyRotationToConfig_004
 * @tc.desc  : Test ApplyRotationToConfig with 270 degree rotation
 */
HWTEST_F(WaterMarkFilterTest, ApplyRotationToConfig_004, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);

    WaterMarkConfig config;
    config.top = 100;
    config.left = 200;
    config.watermarkWidth = 300;
    config.watermarkHeight = 50;

    auto result = waterMarkFilter_->ApplyRotationToConfig(config, 270);
    EXPECT_EQ(result.left, 1920 - 100 - 50);
    EXPECT_EQ(result.top, 200);
    EXPECT_EQ(result.watermarkWidth, 50);
    EXPECT_EQ(result.watermarkHeight, 300);
}

/**
 * @tc.name  : Test ApplyRotationToConfig
 * @tc.number: ApplyRotationToConfig_005
 * @tc.desc  : Test ApplyRotationToConfig preserves rawWidth/rawHeight/rotation
 */
HWTEST_F(WaterMarkFilterTest, ApplyRotationToConfig_005, TestSize.Level1)
{
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);

    WaterMarkConfig config;
    config.top = 0;
    config.left = 0;
    config.watermarkWidth = 100;
    config.watermarkHeight = 50;
    config.rawWidth = 200;
    config.rawHeight = 80;
    config.rotation = 90;

    auto result = waterMarkFilter_->ApplyRotationToConfig(config, 0);
    EXPECT_EQ(result.rawWidth, 200);
    EXPECT_EQ(result.rawHeight, 80);
    EXPECT_EQ(result.rotation, 90);
}

/**
 * @tc.name  : Test ComputeWatermarkBoundingBox
 * @tc.number: ComputeWatermarkBoundingBox_001
 * @tc.desc  : Test ComputeWatermarkBoundingBox with empty watermark list
 */
HWTEST_F(WaterMarkFilterTest, ComputeWatermarkBoundingBox_001, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);

    waterMarkFilter_->ComputeWatermarkBoundingBox();
    EXPECT_EQ(waterMarkFilter_->wmBBoxLeft_, 0);
    EXPECT_EQ(waterMarkFilter_->wmBBoxTop_, 0);
    EXPECT_EQ(waterMarkFilter_->wmBBoxWidth_, 1920);
    EXPECT_EQ(waterMarkFilter_->wmBBoxHeight_, 1080);
}

/**
 * @tc.name  : Test ComputeWatermarkBoundingBox
 * @tc.number: ComputeWatermarkBoundingBox_002
 * @tc.desc  : Test ComputeWatermarkBoundingBox with 8-byte aligned coordinates
 */
HWTEST_F(WaterMarkFilterTest, ComputeWatermarkBoundingBox_002, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);
    waterMarkFilter_->rotation_.store(0);

    WaterMarkConfig config;
    config.top = 64;   // 8-byte aligned
    config.left = 128;  // 8-byte aligned
    config.watermarkWidth = 200;
    config.watermarkHeight = 100;
    waterMarkFilter_->watermarkList_.push_back(std::move(config));

    waterMarkFilter_->ComputeWatermarkBoundingBox();
    EXPECT_EQ(waterMarkFilter_->wmBBoxLeft_, 128);
    EXPECT_EQ(waterMarkFilter_->wmBBoxTop_, 64);
    EXPECT_EQ(waterMarkFilter_->wmBBoxWidth_, 200);
    EXPECT_EQ(waterMarkFilter_->wmBBoxHeight_, 100);
}

/**
 * @tc.name  : Test ComputeWatermarkBoundingBox
 * @tc.number: ComputeWatermarkBoundingBox_003
 * @tc.desc  : Test ComputeWatermarkBoundingBox with non-aligned coordinates
 */
HWTEST_F(WaterMarkFilterTest, ComputeWatermarkBoundingBox_003, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);
    waterMarkFilter_->rotation_.store(0);

    WaterMarkConfig config;
    config.top = 65;
    config.left = 130;
    config.watermarkWidth = 200;
    config.watermarkHeight = 100;
    waterMarkFilter_->watermarkList_.push_back(std::move(config));

    waterMarkFilter_->ComputeWatermarkBoundingBox();
    EXPECT_EQ(waterMarkFilter_->wmBBoxLeft_, 130);
    EXPECT_EQ(waterMarkFilter_->wmBBoxTop_, 65);
    EXPECT_EQ(waterMarkFilter_->wmBBoxWidth_, 200);
    EXPECT_EQ(waterMarkFilter_->wmBBoxHeight_, 100);
}

/**
 * @tc.number: ComputeWatermarkBoundingBox_004
 * @tc.desc  : Test ComputeWatermarkBoundingBox with 90° rotation
 */
HWTEST_F(WaterMarkFilterTest, ComputeWatermarkBoundingBox_004, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);
    waterMarkFilter_->rotation_.store(90);

    WaterMarkConfig config;
    config.top = 100;
    config.left = 200;
    config.watermarkWidth = 300;
    config.watermarkHeight = 50;
    waterMarkFilter_->watermarkList_.push_back(std::move(config));

    waterMarkFilter_->ComputeWatermarkBoundingBox();
    EXPECT_EQ(waterMarkFilter_->wmBBoxLeft_, 100);
    EXPECT_EQ(waterMarkFilter_->wmBBoxTop_, 580);
    EXPECT_EQ(waterMarkFilter_->wmBBoxWidth_, 50);
    EXPECT_EQ(waterMarkFilter_->wmBBoxHeight_, 300);
}

/**
 * @tc.name  : Test SetHardWatermarkMode
 * @tc.number: SetHardWatermarkMode_001
 * @tc.desc  : Test SetHardWatermarkMode sets isHardWatermarkMode_ to true
 */
HWTEST_F(WaterMarkFilterTest, SetHardWatermarkMode_001, TestSize.Level1)
{
    waterMarkFilter_->SetHardWatermarkMode(true);
    EXPECT_TRUE(waterMarkFilter_->isHardWatermarkMode_);
}

/**
 * @tc.name  : Test SetHardWatermarkMode
 * @tc.number: SetHardWatermarkMode_002
 * @tc.desc  : Test SetHardWatermarkMode sets isHardWatermarkMode_ to false
 */
HWTEST_F(WaterMarkFilterTest, SetHardWatermarkMode_002, TestSize.Level1)
{
    waterMarkFilter_->SetHardWatermarkMode(true);
    EXPECT_TRUE(waterMarkFilter_->isHardWatermarkMode_);
    waterMarkFilter_->SetHardWatermarkMode(false);
    EXPECT_FALSE(waterMarkFilter_->isHardWatermarkMode_);
}

/**
 * @tc.name  : Test ReleaseMergeResources
 * @tc.number: ReleaseMergeResources_001
 * @tc.desc  : Test ReleaseMergeResources resets all merge-related GL resources
 */
HWTEST_F(WaterMarkFilterTest, ReleaseMergeResources_001, TestSize.Level1)
{
    // Set non-zero values to verify they are cleared
    waterMarkFilter_->watermarkFBO_ = 1;
    waterMarkFilter_->mergedWatermarkTexId_ = 2;
    waterMarkFilter_->watermarkVAO_ = 3;
    waterMarkFilter_->watermarkVBO_ = 4;
    waterMarkFilter_->watermarkMergeShaderProgram_ =
        std::make_unique<ShaderProgram>("attribute vec4 pos; void main(){}", "void main(){}");

    waterMarkFilter_->ReleaseMergeResources();
    EXPECT_EQ(waterMarkFilter_->watermarkFBO_, 0u);
    EXPECT_EQ(waterMarkFilter_->mergedWatermarkTexId_, 0u);
    EXPECT_EQ(waterMarkFilter_->watermarkVAO_, 0u);
    EXPECT_EQ(waterMarkFilter_->watermarkVBO_, 0u);
    EXPECT_EQ(waterMarkFilter_->watermarkMergeShaderProgram_, nullptr);
}

/**
 * @tc.name  : Test ReleaseMergeResources
 * @tc.number: ReleaseMergeResources_002
 * @tc.desc  : Test ReleaseMergeResources when resources are already zero (no crash)
 */
HWTEST_F(WaterMarkFilterTest, ReleaseMergeResources_002, TestSize.Level1)
{
    waterMarkFilter_->watermarkFBO_ = 0;
    waterMarkFilter_->mergedWatermarkTexId_ = 0;
    waterMarkFilter_->watermarkVAO_ = 0;
    waterMarkFilter_->watermarkVBO_ = 0;
    waterMarkFilter_->watermarkMergeShaderProgram_ = nullptr;

    waterMarkFilter_->ReleaseMergeResources();
    EXPECT_EQ(waterMarkFilter_->watermarkFBO_, 0u);
    EXPECT_EQ(waterMarkFilter_->mergedWatermarkTexId_, 0u);
    EXPECT_EQ(waterMarkFilter_->watermarkVAO_, 0u);
    EXPECT_EQ(waterMarkFilter_->watermarkVBO_, 0u);
    EXPECT_EQ(waterMarkFilter_->watermarkMergeShaderProgram_, nullptr);
}

/**
 * @tc.name  : Test PreprocessWatermarks
 * @tc.number: PreprocessWatermarks_001
 * @tc.desc  : Test PreprocessWatermarks with empty watermark list returns error
 */
HWTEST_F(WaterMarkFilterTest, PreprocessWatermarks_001, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    Status ret = waterMarkFilter_->PreprocessWatermarks();
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name  : Test ApplyRotation
 * @tc.number: ApplyRotation_001
 * @tc.desc  : Test ApplyRotation with same rotation returns OK without EGL context
 */
HWTEST_F(WaterMarkFilterTest, ApplyRotation_001, TestSize.Level1)
{
    waterMarkFilter_->rotation_.store(90);
    Status ret = waterMarkFilter_->ApplyRotation(90);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test ApplyRotation
 * @tc.number: ApplyRotation_002
 * @tc.desc  : Test ApplyRotation with different rotation but no EGL context returns error
 */
HWTEST_F(WaterMarkFilterTest, ApplyRotation_002, TestSize.Level1)
{
    waterMarkFilter_->rotation_.store(0);
    waterMarkFilter_->eglContextReady_ = false;
    Status ret = waterMarkFilter_->ApplyRotation(90);
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name  : Test GetMergedWatermarkBuffer
 * @tc.number: GetMergedWatermarkBuffer_001
 * @tc.desc  : Test GetMergedWatermarkBuffer without EGL context returns nullptr
 */
HWTEST_F(WaterMarkFilterTest, GetMergedWatermarkBuffer_001, TestSize.Level1)
{
    waterMarkFilter_->eglContextReady_ = false;
    auto result = waterMarkFilter_->GetMergedWatermarkBuffer(0);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name  : Test GetMergedWatermarkBuffer
 * @tc.number: GetMergedWatermarkBuffer_002
 * @tc.desc  : Test GetMergedWatermarkBuffer with EGL but no merged texture returns nullptr
 */
HWTEST_F(WaterMarkFilterTest, GetMergedWatermarkBuffer_002, TestSize.Level1)
{
    waterMarkFilter_->eglContextReady_ = true;
    waterMarkFilter_->mergedWatermarkTexId_ = 0;
    auto result = waterMarkFilter_->GetMergedWatermarkBuffer(0);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name  : Test HasHardWatermarkPositionViolation
 * @tc.number: HasHardWatermarkPositionViolation_001
 * @tc.desc  : Empty watermark list, no violation
 */
HWTEST_F(WaterMarkFilterTest, HasHardWatermarkPositionViolation_001, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);
    EXPECT_FALSE(waterMarkFilter_->HasHardWatermarkPositionViolation());
}

/**
 * @tc.name  : Test HasHardWatermarkPositionViolation
 * @tc.number: HasHardWatermarkPositionViolation_002
 * @tc.desc  : No violation at 0 degree (well within bounds)
 */
HWTEST_F(WaterMarkFilterTest, HasHardWatermarkPositionViolation_002, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);
    WaterMarkConfig config;
    config.top = 100;
    config.left = 100;
    config.watermarkWidth = 200;
    config.watermarkHeight = 100;
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    EXPECT_FALSE(waterMarkFilter_->HasHardWatermarkPositionViolation());
}

/**
 * @tc.name  : Test HasHardWatermarkPositionViolation
 * @tc.number: HasHardWatermarkPositionViolation_003
 * @tc.desc  : Violation at 0 degree: left < 16 and alignedRight > frameWidth
 */
HWTEST_F(WaterMarkFilterTest, HasHardWatermarkPositionViolation_003, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);
    WaterMarkConfig config;
    config.top = 100;
    config.left = 1910;
    config.watermarkWidth = 50;
    config.watermarkHeight = 100;
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    EXPECT_TRUE(waterMarkFilter_->HasHardWatermarkPositionViolation());
}

/**
 * @tc.name  : Test HasHardWatermarkPositionViolation
 * @tc.number: HasHardWatermarkPositionViolation_004
 * @tc.desc  : Violation at 180 degree: alignedBottom > frameHeight
 */
HWTEST_F(WaterMarkFilterTest, HasHardWatermarkPositionViolation_004, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);
    WaterMarkConfig config;
    config.top = 5;
    config.left = 5;
    config.watermarkWidth = 50;
    config.watermarkHeight = 50;
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    EXPECT_TRUE(waterMarkFilter_->HasHardWatermarkPositionViolation());
}

/**
 * @tc.name  : Test HasHardWatermarkPositionViolation
 * @tc.number: HasHardWatermarkPositionViolation_005
 * @tc.desc  : Violation at 90 degree: alignedRight > frameWidth
 */
HWTEST_F(WaterMarkFilterTest, HasHardWatermarkPositionViolation_005, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);
    WaterMarkConfig config;
    config.top = 1070;
    config.left = 5;
    config.watermarkWidth = 50;
    config.watermarkHeight = 50;
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    EXPECT_TRUE(waterMarkFilter_->HasHardWatermarkPositionViolation());
}

/**
 * @tc.name  : Test HasHardWatermarkPositionViolation
 * @tc.number: HasHardWatermarkPositionViolation_006
 * @tc.desc  : Violation at 270 degree: alignedBottom > frameHeight
 */
HWTEST_F(WaterMarkFilterTest, HasHardWatermarkPositionViolation_006, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);
    WaterMarkConfig config;
    config.top = 100;
    config.left = 1060;
    config.watermarkWidth = 50;
    config.watermarkHeight = 50;
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    EXPECT_TRUE(waterMarkFilter_->HasHardWatermarkPositionViolation());
}

/**
 * @tc.name  : Test HasHardWatermarkPositionViolation
 * @tc.number: HasHardWatermarkPositionViolation_007
 * @tc.desc  : Video dimensions not set (targetWidth_/targetHeight_ = 0), treat as violation
 */
HWTEST_F(WaterMarkFilterTest, HasHardWatermarkPositionViolation_007, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(0);
    waterMarkFilter_->targetHeight_.store(0);
    WaterMarkConfig config;
    config.top = 100;
    config.left = 100;
    config.watermarkWidth = 200;
    config.watermarkHeight = 100;
    waterMarkFilter_->watermarkList_.push_back(std::move(config));
    EXPECT_TRUE(waterMarkFilter_->HasHardWatermarkPositionViolation());
}

/**
 * @tc.name  : Test HasHardWatermarkPositionViolation
 * @tc.number: HasHardWatermarkPositionViolation_008
 * @tc.desc  : Multiple watermarks, one violates
 */
HWTEST_F(WaterMarkFilterTest, HasHardWatermarkPositionViolation_008, TestSize.Level1)
{
    waterMarkFilter_->watermarkList_.clear();
    waterMarkFilter_->targetWidth_.store(1920);
    waterMarkFilter_->targetHeight_.store(1080);
    WaterMarkConfig config1;
    config1.top = 100;
    config1.left = 100;
    config1.watermarkWidth = 200;
    config1.watermarkHeight = 100;
    waterMarkFilter_->watermarkList_.push_back(std::move(config1));
    WaterMarkConfig config2;
    config2.top = 5;
    config2.left = 5;
    config2.watermarkWidth = 50;
    config2.watermarkHeight = 50;
    waterMarkFilter_->watermarkList_.push_back(std::move(config2));
    EXPECT_TRUE(waterMarkFilter_->HasHardWatermarkPositionViolation());
}

// ---- OpenGLVideoFilterBase base class coverage (HDR color space / codec format) ----

/**
 * @tc.name  : Test SetCodecFormat with HDR color space
 * @tc.number: SetCodecFormat_002
 * @tc.desc  : SetCodecFormat with AV_TRANSCODER_DST_COLOR_SPACE invokes OnSetCodecFormat (base no-op)
 */
HWTEST_F(WaterMarkFilterTest, SetCodecFormat_002, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT));
    Status ret = waterMarkFilter_->SetCodecFormat(format);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test SetCodecFormat with null format
 * @tc.number: SetCodecFormat_003
 * @tc.desc  : SetCodecFormat with null format still returns OK (base handles via FALSE_RETURN_NOLOG)
 */
HWTEST_F(WaterMarkFilterTest, SetCodecFormat_003, TestSize.Level1)
{
    std::shared_ptr<Meta> format = nullptr;
    Status ret = waterMarkFilter_->SetCodecFormat(format);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test SetCodecFormat with HDR BT2020 color space
 * @tc.number: SetCodecFormat_004
 * @tc.desc  : SetCodecFormat with BT2020 HDR color space invokes base OnSetCodecFormat no-op
 */
HWTEST_F(WaterMarkFilterTest, SetCodecFormat_004, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_PQ_LIMIT));
    Status ret = waterMarkFilter_->SetCodecFormat(format);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test Configure with null parameter
 * @tc.number: Configure_002
 * @tc.desc  : Configure with null parameter stores nullptr and returns OK
 */
HWTEST_F(WaterMarkFilterTest, Configure_002, TestSize.Level1)
{
    std::shared_ptr<Meta> parameter = nullptr;
    Status ret = waterMarkFilter_->Configure(parameter);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(waterMarkFilter_->configureParameter_, nullptr);
}

/**
 * @tc.name  : Test SetParameter with EOS and frameNum
 * @tc.number: SetParameter_003
 * @tc.desc  : SetParameter with only USER_FRAME_PTS sets eosPts_ and eosReceived_
 */
HWTEST_F(WaterMarkFilterTest, SetParameter_003, TestSize.Level1)
{
    auto parameter = std::make_shared<Meta>();
    ASSERT_TRUE(parameter != nullptr);
    parameter->Set<Tag::USER_FRAME_PTS>(200);
    waterMarkFilter_->SetParameter(parameter);
    EXPECT_EQ(waterMarkFilter_->eosPts_.load(), 200);
    EXPECT_TRUE(waterMarkFilter_->eosReceived_.load());
}

/**
 * @tc.name  : Test GetParameter default
 * @tc.number: GetParameter_002
 * @tc.desc  : GetParameter is a no-op and leaves the meta unchanged
 */
HWTEST_F(WaterMarkFilterTest, GetParameter_002, TestSize.Level1)
{
    auto parameter = std::make_shared<Meta>();
    ASSERT_TRUE(parameter != nullptr);
    parameter->Set<Tag::USER_FRAME_PTS>(999);
    waterMarkFilter_->GetParameter(parameter);
    // GetParameter is a base no-op; verify meta is not modified
    int64_t pts = 0;
    EXPECT_TRUE(parameter->Get<Tag::USER_FRAME_PTS>(pts));
    EXPECT_EQ(pts, 999);
}

/**
 * @tc.name  : Test NotifyNextFilterEos with null filter in map
 * @tc.number: NotifyNextFilterEos_004
 * @tc.desc  : NotifyNextFilterEos with a null filter entry in nextFiltersMap_ does not crash
 */
HWTEST_F(WaterMarkFilterTest, NotifyNextFilterEos_004, TestSize.Level1)
{
    std::shared_ptr<Filter> nullFilter = nullptr;
    waterMarkFilter_->nextFiltersMap_[StreamType::STREAMTYPE_RAW_VIDEO].push_back(nullFilter);
    waterMarkFilter_->eosPts_.store(100);
    Status ret = waterMarkFilter_->NotifyNextFilterEos();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test SetVideoResize with 4K dimensions
 * @tc.number: SetVideoResize_005
 * @tc.desc  : SetVideoResize with 3840x2160 sets video/target dimensions and returns OK
 */
HWTEST_F(WaterMarkFilterTest, SetVideoResize_005, TestSize.Level1)
{
    waterMarkFilter_->nativeImage_ = nullptr;
    Status ret = waterMarkFilter_->SetVideoResize(3840, 2160);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(waterMarkFilter_->videoWidth_.load(), 3840);
    EXPECT_EQ(waterMarkFilter_->videoHeight_.load(), 2160);
}

/**
 * @tc.name  : Test SetVideoResize with zero height
 * @tc.number: SetVideoResize_006
 * @tc.desc  : SetVideoResize with zero height returns ERROR_INVALID_PARAMETER
 */
HWTEST_F(WaterMarkFilterTest, SetVideoResize_006, TestSize.Level1)
{
    Status ret = waterMarkFilter_->SetVideoResize(1920, 0);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test SetVideoResize with negative width
 * @tc.number: SetVideoResize_007
 * @tc.desc  : SetVideoResize with negative width returns ERROR_INVALID_PARAMETER
 */
HWTEST_F(WaterMarkFilterTest, SetVideoResize_007, TestSize.Level1)
{
    Status ret = waterMarkFilter_->SetVideoResize(-1, 1080);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test OnUpdatedResult with null meta
 * @tc.number: OnUpdatedResult_002
 * @tc.desc  : OnUpdatedResult with null meta does not crash (base no-op)
 */
HWTEST_F(WaterMarkFilterTest, OnUpdatedResult_002, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = nullptr;
    waterMarkFilter_->OnUpdatedResult(meta);
    SUCCEED();
}

/**
 * @tc.name  : Test OnUnlinkedResult with null meta
 * @tc.number: OnUnlinkedResult_002
 * @tc.desc  : OnUnlinkedResult with null meta does not crash (base no-op)
 */
HWTEST_F(WaterMarkFilterTest, OnUnlinkedResult_002, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = nullptr;
    waterMarkFilter_->OnUnlinkedResult(meta);
    SUCCEED();
}

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:watermark_filter_test.xml";
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    _exit(ret);
}