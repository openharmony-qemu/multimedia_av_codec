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

#include <gtest/gtest.h>
#include "avcodec_info.h"

using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

class AVCodecInfoTest : public testing::Test {
public:
    void SetUp(void);
    void TearDown(void);
    CapabilityData *data_;
};

void AVCodecInfoTest::SetUp(void)
{
    data_ = new CapabilityData();
    data_->codecName = "unit_test_codec";
    data_->mimeType = CodecMimeType::VIDEO_AVC;
    data_->codecType = AVCODEC_TYPE_VIDEO_DECODER;
    data_->isVendor = false;
    data_->maxInstance = 16;
    data_->alignment.width = 2;
    data_->alignment.height = 2;
    data_->width = Range(2, 1920);
    data_->height = Range(2, 1088);
    data_->frameRate = Range(1, 120);
    data_->bitrate = Range(1, 300000000);
    data_->blockPerFrame = Range(1, 8160);
    data_->blockPerSecond = Range(1, 979200);
    data_->blockSize.width = 16;
    data_->blockSize.height = 16;
    data_->supportSwapWidthHeight = false;
    data_->pixFormat = {static_cast<int32_t>(VideoPixelFormat::YUVI420),
                        static_cast<int32_t>(VideoPixelFormat::NV12),
                        static_cast<int32_t>(VideoPixelFormat::NV21),
                        static_cast<int32_t>(VideoPixelFormat::RGBA)};
}

void AVCodecInfoTest::TearDown(void)
{
    delete data_;
    data_ = nullptr;
}

HWTEST_F(AVCodecInfoTest, NO_SWAP_TEST_001, TestSize.Level3)
{
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetVideoWidthRangeForHeight(1408);
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_FALSE(widthRange.InRange(1152));
}

HWTEST_F(AVCodecInfoTest, NO_SWAP_TEST_002, TestSize.Level3)
{
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetVideoWidthRangeForHeight(1088);
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_TRUE(widthRange.InRange(1152));
}

HWTEST_F(AVCodecInfoTest, SWAP_TEST_001, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(1408);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_FALSE(range.InRange(1152));
    range = videoCap->GetVideoHeightRangeForWidth(1408);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_FALSE(range.InRange(1152));
    range = videoCap->GetVideoWidthRangeForHeight(1152);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_FALSE(range.InRange(1408));
    range = videoCap->GetVideoHeightRangeForWidth(1152);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_FALSE(range.InRange(1408));
    ASSERT_FALSE(videoCap->IsSizeSupported(1152, 1408));
    ASSERT_FALSE(videoCap->IsSizeSupported(1408, 1152));
}

HWTEST_F(AVCodecInfoTest, SWAP_TEST_002, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    data_->blockPerFrame = Range(1, 979200);
    data_->alignment.width = 1;
    data_->alignment.height = 1;
    data_->width = Range(40, 1920);
    data_->height = Range(20, 1088);
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(1000);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 20);
    ASSERT_EQ(range.maxVal, 1920);
    range = videoCap->GetVideoHeightRangeForWidth(1000);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 20);
    ASSERT_EQ(range.maxVal, 1920);
}

HWTEST_F(AVCodecInfoTest, SWAP_TEST_003, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    data_->blockPerFrame = Range(1, 979200);
    data_->alignment.width = 1;
    data_->alignment.height = 1;
    data_->width = Range(40, 1920);
    data_->height = Range(20, 1088);
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(24);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 40);
    ASSERT_EQ(range.maxVal, 1920);
    range = videoCap->GetVideoHeightRangeForWidth(24);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 40);
    ASSERT_EQ(range.maxVal, 1920);
}

HWTEST_F(AVCodecInfoTest, SWAP_TEST_004, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    data_->blockPerFrame = Range(1, 979200);
    data_->alignment.width = 1;
    data_->alignment.height = 1;
    data_->width = Range(40, 1920);
    data_->height = Range(20, 1088);
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(1200);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 20);
    ASSERT_EQ(range.maxVal, 1088);
    range = videoCap->GetVideoHeightRangeForWidth(1200);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 20);
    ASSERT_EQ(range.maxVal, 1088);
}

HWTEST_F(AVCodecInfoTest, SWAP_TEST_005, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    data_->blockPerFrame = Range(1, 979200);
    data_->alignment.width = 1;
    data_->alignment.height = 1;
    data_->width = Range(40, 1920);
    data_->height = Range(20, 1088);
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(2000);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 0);
    range = videoCap->GetVideoHeightRangeForWidth(2000);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 0);
}

HWTEST_F(AVCodecInfoTest, SWAP_TEST_006, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetVideoWidthRangeForHeight(1920);
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_FALSE(widthRange.InRange(1200));
}

HWTEST_F(AVCodecInfoTest, SWAP_TEST_007, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetVideoWidthRangeForHeight(1911);
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_EQ(widthRange.minVal, 0);
    ASSERT_EQ(widthRange.maxVal, 0);
}

HWTEST_F(AVCodecInfoTest, GET_WIDTH_RANGE_TEST_001, TestSize.Level3)
{
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetSupportedWidth();
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_EQ(widthRange.minVal, 2);
    ASSERT_EQ(widthRange.maxVal, 1920);
}

HWTEST_F(AVCodecInfoTest, GET_WIDTH_RANGE_TEST_002, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetSupportedWidth();
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_EQ(widthRange.minVal, 2);
    ASSERT_EQ(widthRange.maxVal, 1920);
}

HWTEST_F(AVCodecInfoTest, GET_HEIGHT_RANGE_TEST_001, TestSize.Level3)
{
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetSupportedHeight();
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 1088);
}

HWTEST_F(AVCodecInfoTest, GET_HEIGHT_RANGE_TEST_002, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetSupportedHeight();
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 1920);
}