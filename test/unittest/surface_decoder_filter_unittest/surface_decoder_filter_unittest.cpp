/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "surface_decoder_filter_unittest.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Pipeline;
namespace OHOS::Media {
const static int32_t NUM_TEST = 0;
void SurfaceDecoderFilterUnitTest::SetUpTestCase(void) {}

void SurfaceDecoderFilterUnitTest::TearDownTestCase(void) {}

void SurfaceDecoderFilterUnitTest::SetUp(void)
{
    std::string nameTest = "nameTest";
    FilterType type = FilterType::FILTERTYPE_VIDEODEC;
    surfaceDecoderFilter_ = std::make_shared<SurfaceDecoderFilter>(nameTest, type);
}

void SurfaceDecoderFilterUnitTest::TearDown(void)
{
    surfaceDecoderFilter_ = nullptr;
}

/**
 * @tc.name  : Test OnLinkedResult
 * @tc.number: OnLinkedResult_001
 * @tc.desc  : Test (auto surfaceDecoderFilter = surfaceDecoderFilter_.lock()) == false
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, OnLinkedResult_001, TestSize.Level0)
{
    ASSERT_NE(surfaceDecoderFilter_, nullptr);
    auto callBackPtr = std::make_shared<SurfaceDecoderFilterLinkCallback>(nullptr);
    std::shared_ptr<Meta> meta;
    sptr<AVBufferQueueProducer> queue;
    callBackPtr->OnLinkedResult(queue, meta);
    EXPECT_EQ(callBackPtr->surfaceDecoderFilter_.lock(), nullptr);
}

/**
 * @tc.name  : Test OnUnlinkedResult
 * @tc.number: OnUnlinkedResult_001
 * @tc.desc  : Test (auto surfaceDecoderFilter = surfaceDecoderFilter_.lock()) == false
 *             Test (auto surfaceDecoderFilter = surfaceDecoderFilter_.lock()) == true
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, OnUnlinkedResult_001, TestSize.Level0)
{
    ASSERT_NE(surfaceDecoderFilter_, nullptr);
    auto callBackPtr = std::make_shared<SurfaceDecoderFilterLinkCallback>(nullptr);
    std::shared_ptr<Meta> meta;
    callBackPtr->OnUnlinkedResult(meta);
    EXPECT_EQ(callBackPtr->surfaceDecoderFilter_.lock(), nullptr);

    callBackPtr->surfaceDecoderFilter_ = surfaceDecoderFilter_;
    callBackPtr->OnUnlinkedResult(meta);
    EXPECT_NE(callBackPtr->surfaceDecoderFilter_.lock(), nullptr);
}

/**
 * @tc.name  : Test OnUpdatedResult
 * @tc.number: OnUpdatedResult_001
 * @tc.desc  : Test (auto surfaceDecoderFilter = surfaceDecoderFilter_.lock()) == false
 *             Test (auto surfaceDecoderFilter = surfaceDecoderFilter_.lock()) == true
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, OnUpdatedResult_001, TestSize.Level0)
{
    ASSERT_NE(surfaceDecoderFilter_, nullptr);
    auto callBackPtr = std::make_shared<SurfaceDecoderFilterLinkCallback>(nullptr);
    std::shared_ptr<Meta> meta;
    callBackPtr->OnUpdatedResult(meta);
    EXPECT_EQ(callBackPtr->surfaceDecoderFilter_.lock(), nullptr);

    callBackPtr->surfaceDecoderFilter_ = surfaceDecoderFilter_;
    callBackPtr->OnUpdatedResult(meta);
    EXPECT_NE(callBackPtr->surfaceDecoderFilter_.lock(), nullptr);
}

/**
 * @tc.name  : Test OnError
 * @tc.number: CallBackOnError_001
 * @tc.desc  : Test (auto codecFilter = codecFilter_.lock()) == false
 *             Test (auto codecFilter = codecFilter_.lock()) == true
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, CallBackOnError_001, TestSize.Level0)
{
    ASSERT_NE(surfaceDecoderFilter_, nullptr);
    auto callBackPtr = std::make_shared<SurfaceDecoderAdapterCallback>(nullptr);
    MediaAVCodec::AVCodecErrorType errorType = MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_INTERNAL;
    int32_t errorCode = NUM_TEST;
    callBackPtr->OnError(errorType, errorCode);
    EXPECT_EQ(callBackPtr->surfaceDecoderFilter_.lock(), nullptr);

    callBackPtr->surfaceDecoderFilter_ = surfaceDecoderFilter_;
    callBackPtr->OnError(errorType, errorCode);
    EXPECT_NE(callBackPtr->surfaceDecoderFilter_.lock(), nullptr);
}

/**
 * @tc.name  : Test OnBufferEos
 * @tc.number: CallBackOnBufferEos_001
 * @tc.desc  : Test OnOutputFormatChanged all
 *             Test OnBufferEos: (auto codecFilter = codecFilter_.lock()) == true
 *             Test OnBufferEos: (auto codecFilter = codecFilter_.lock()) == false
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, CallBackOnBufferEos_001, TestSize.Level0)
{
    ASSERT_NE(surfaceDecoderFilter_, nullptr);
    auto callBackPtr = std::make_shared<SurfaceDecoderAdapterCallback>(nullptr);
    std::shared_ptr<Meta> format;
    callBackPtr->OnOutputFormatChanged(format);

    int64_t pts = NUM_TEST;
    int64_t frameNum = NUM_TEST;
    callBackPtr->OnBufferEos(pts, frameNum);
    EXPECT_EQ(callBackPtr->surfaceDecoderFilter_.lock(), nullptr);

    callBackPtr->surfaceDecoderFilter_ = surfaceDecoderFilter_;
    callBackPtr->OnBufferEos(pts, frameNum);
    EXPECT_NE(callBackPtr->surfaceDecoderFilter_.lock(), nullptr);
    EXPECT_EQ(surfaceDecoderFilter_->NotifyNextFilterEos(pts, frameNum), Status::OK);
}
} // namespace OHOS::Media