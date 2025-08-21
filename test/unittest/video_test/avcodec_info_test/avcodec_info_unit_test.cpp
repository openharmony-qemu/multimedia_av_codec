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

class AVCodecInfoUnitTest : public testing::Test {
public:
    void SetUp(void);
    void TearDown(void);
    CapabilityData *data_;
};

namespace {
constexpr int32_t DEFAULT_INSTANCE_CNT = 16;
constexpr int32_t DEFAULT_ALIGN_SIZE = 2;
constexpr int32_t DEFAULT_BLOCK_SIZE = 16;
constexpr int32_t DEFAULT_MIN_SIDE = 2;
constexpr int32_t DEFAULT_MAX_SHORT_SIDE = 1088;
constexpr int32_t DEFAULT_MAX_LONG_SIDE = 1920;
constexpr int32_t DEFAULT_MIN_FRAME_RATE = 1;
constexpr int32_t DEFAULT_MAX_FRAME_RATE = 120;
constexpr int32_t DEFAULT_MIN_BIT_RATE = 1;
constexpr int32_t DEFAULT_MAX_BIT_RATE = 300000000;
constexpr int32_t DEFAULT_MIN_BLOCK_PER_FRAME = 1;
constexpr int32_t DEFAULT_MAX_BLOCK_PER_FRAME = 8160;
constexpr int32_t DEFAULT_MIN_BLOCK_PER_SEC = DEFAULT_MIN_FRAME_RATE * DEFAULT_MIN_BLOCK_PER_FRAME;
constexpr int32_t DEFAULT_MAX_BLOCK_PER_SEC = DEFAULT_MAX_FRAME_RATE * DEFAULT_MAX_BLOCK_PER_FRAME;
constexpr int32_t DEFAULT_SHORT_SIDE = 1152;
constexpr int32_t DEFAULT_LONG_SIDE = 1408;
};

void AVCodecInfoUnitTest::SetUp(void)
{
    data_ = new CapabilityData();
    data_->codecName = "unit_test_codec";
    data_->mimeType = CodecMimeType::VIDEO_AVC;
    data_->codecType = AVCODEC_TYPE_VIDEO_DECODER;
    data_->isVendor = false;
    data_->maxInstance = DEFAULT_INSTANCE_CNT;
    data_->alignment.width = DEFAULT_ALIGN_SIZE;
    data_->alignment.height = DEFAULT_ALIGN_SIZE;
    data_->width = Range(DEFAULT_MIN_SIDE, DEFAULT_MAX_LONG_SIDE);
    data_->height = Range(DEFAULT_MIN_SIDE, DEFAULT_MAX_SHORT_SIDE);
    data_->frameRate = Range(DEFAULT_MIN_FRAME_RATE, DEFAULT_MAX_FRAME_RATE);
    data_->bitrate = Range(DEFAULT_MIN_BIT_RATE, DEFAULT_MAX_BIT_RATE);
    data_->blockPerFrame = Range(DEFAULT_MIN_BLOCK_PER_FRAME, DEFAULT_MAX_BLOCK_PER_FRAME);
    data_->blockPerSecond = Range(DEFAULT_MIN_BLOCK_PER_SEC, DEFAULT_MAX_BLOCK_PER_SEC);
    data_->blockSize.width = DEFAULT_BLOCK_SIZE;
    data_->blockSize.height = DEFAULT_BLOCK_SIZE;
    data_->supportSwapWidthHeight = false;
    data_->pixFormat = {static_cast<int32_t>(VideoPixelFormat::YUVI420),
                        static_cast<int32_t>(VideoPixelFormat::NV12),
                        static_cast<int32_t>(VideoPixelFormat::NV21),
                        static_cast<int32_t>(VideoPixelFormat::RGBA)};
}

void AVCodecInfoUnitTest::TearDown(void)
{
    delete data_;
    data_ = nullptr;
}

HWTEST_F(AVCodecInfoUnitTest, NO_SWAP_TEST_001, TestSize.Level3)
{
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetVideoWidthRangeForHeight(DEFAULT_LONG_SIDE);
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_FALSE(widthRange.InRange(DEFAULT_SHORT_SIDE));
}

HWTEST_F(AVCodecInfoUnitTest, NO_SWAP_TEST_002, TestSize.Level3)
{
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetVideoWidthRangeForHeight(DEFAULT_MAX_SHORT_SIDE);
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_TRUE(widthRange.InRange(DEFAULT_SHORT_SIDE));
}

HWTEST_F(AVCodecInfoUnitTest, SWAP_TEST_001, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(DEFAULT_LONG_SIDE);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_FALSE(range.InRange(DEFAULT_SHORT_SIDE));
    range = videoCap->GetVideoHeightRangeForWidth(DEFAULT_LONG_SIDE);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_FALSE(range.InRange(DEFAULT_SHORT_SIDE));
    range = videoCap->GetVideoWidthRangeForHeight(DEFAULT_SHORT_SIDE);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_FALSE(range.InRange(DEFAULT_LONG_SIDE));
    range = videoCap->GetVideoHeightRangeForWidth(DEFAULT_SHORT_SIDE);
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_FALSE(range.InRange(DEFAULT_LONG_SIDE));
    ASSERT_FALSE(videoCap->IsSizeSupported(DEFAULT_SHORT_SIDE, DEFAULT_LONG_SIDE));
    ASSERT_FALSE(videoCap->IsSizeSupported(DEFAULT_LONG_SIDE, DEFAULT_SHORT_SIDE));
}

HWTEST_F(AVCodecInfoUnitTest, SWAP_TEST_002, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    data_->blockPerFrame = Range(1, DEFAULT_MAX_BLOCK_PER_SEC);
    data_->alignment.width = 1;
    data_->alignment.height = 1;
    data_->width = Range(40, DEFAULT_MAX_LONG_SIDE); // 40
    data_->height = Range(20, DEFAULT_MAX_SHORT_SIDE); // 20
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(1000);  // 1000
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 20); // 20
    ASSERT_EQ(range.maxVal, DEFAULT_MAX_LONG_SIDE);
    range = videoCap->GetVideoHeightRangeForWidth(1000); // 1000
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 20); // 20
    ASSERT_EQ(range.maxVal, DEFAULT_MAX_LONG_SIDE);
}

HWTEST_F(AVCodecInfoUnitTest, SWAP_TEST_003, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    data_->blockPerFrame = Range(1, DEFAULT_MAX_BLOCK_PER_SEC);
    data_->alignment.width = 1;
    data_->alignment.height = 1;
    data_->width = Range(40, DEFAULT_MAX_LONG_SIDE); // 40
    data_->height = Range(20, DEFAULT_MAX_SHORT_SIDE); // 20
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(24); // 24
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 40); // 40
    ASSERT_EQ(range.maxVal, DEFAULT_MAX_LONG_SIDE);
    range = videoCap->GetVideoHeightRangeForWidth(24); // 24
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 40); // 40
    ASSERT_EQ(range.maxVal, DEFAULT_MAX_LONG_SIDE);
}

HWTEST_F(AVCodecInfoUnitTest, SWAP_TEST_004, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    data_->blockPerFrame = Range(1, DEFAULT_MAX_BLOCK_PER_SEC);
    data_->alignment.width = 1;
    data_->alignment.height = 1;
    data_->width = Range(40, DEFAULT_MAX_LONG_SIDE); // 40
    data_->height = Range(20, DEFAULT_MAX_SHORT_SIDE); // 20
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(1200); // 1200
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 20); // 20
    ASSERT_EQ(range.maxVal, DEFAULT_MAX_SHORT_SIDE);
    range = videoCap->GetVideoHeightRangeForWidth(1200); // 1200
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 20); // 20
    ASSERT_EQ(range.maxVal, DEFAULT_MAX_SHORT_SIDE);
}

HWTEST_F(AVCodecInfoUnitTest, SWAP_TEST_005, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    data_->blockPerFrame = Range(1, DEFAULT_MAX_BLOCK_PER_SEC);
    data_->alignment.width = 1;
    data_->alignment.height = 1;
    data_->width = Range(40, DEFAULT_MAX_LONG_SIDE); // 40
    data_->height = Range(20, DEFAULT_MAX_SHORT_SIDE); // 20
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(2000); // 2000
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 0);
    range = videoCap->GetVideoHeightRangeForWidth(2000); // 2000
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 0);
}

HWTEST_F(AVCodecInfoUnitTest, SWAP_TEST_006, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetVideoWidthRangeForHeight(DEFAULT_MAX_LONG_SIDE);
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_FALSE(widthRange.InRange(1200)); // 1200
}

HWTEST_F(AVCodecInfoUnitTest, SWAP_TEST_007, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetVideoWidthRangeForHeight(1911); // 1911
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_EQ(widthRange.minVal, 0);
    ASSERT_EQ(widthRange.maxVal, 0);
}

HWTEST_F(AVCodecInfoUnitTest, SWAP_TEST_008, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    data_->blockPerFrame = Range(1, DEFAULT_MAX_BLOCK_PER_SEC);
    data_->alignment.width = 1;
    data_->alignment.height = 1;
    data_->width = Range(600, DEFAULT_MAX_LONG_SIDE); // 600
    data_->height = Range(20, 200); // 20 200
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetVideoWidthRangeForHeight(400); // 400
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 0);
    range = videoCap->GetVideoHeightRangeForWidth(400); // 400
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 0);
    ASSERT_FALSE(videoCap->IsSizeSupported(400, 100));
    ASSERT_FALSE(videoCap->IsSizeSupported(100, 400));
    ASSERT_FALSE(videoCap->IsSizeSupported(400, 800));
    ASSERT_FALSE(videoCap->IsSizeSupported(800, 400));
    range = videoCap->GetSupportedHeight();
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, 20);
    ASSERT_EQ(range.maxVal, DEFAULT_MAX_LONG_SIDE);
}

HWTEST_F(AVCodecInfoUnitTest, GET_WIDTH_RANGE_TEST_001, TestSize.Level3)
{
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetSupportedWidth();
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_EQ(widthRange.minVal, DEFAULT_MIN_SIDE);
    ASSERT_EQ(widthRange.maxVal, DEFAULT_MAX_LONG_SIDE);
}

HWTEST_F(AVCodecInfoUnitTest, GET_WIDTH_RANGE_TEST_002, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range widthRange = videoCap->GetSupportedWidth();
    printf("widthRange [%d, %d]\n", widthRange.minVal, widthRange.maxVal);
    ASSERT_EQ(widthRange.minVal, DEFAULT_MIN_SIDE);
    ASSERT_EQ(widthRange.maxVal, DEFAULT_MAX_LONG_SIDE);
}

HWTEST_F(AVCodecInfoUnitTest, GET_HEIGHT_RANGE_TEST_001, TestSize.Level3)
{
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetSupportedHeight();
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, DEFAULT_MIN_SIDE);
    ASSERT_EQ(range.maxVal, DEFAULT_MAX_SHORT_SIDE);
}

HWTEST_F(AVCodecInfoUnitTest, GET_HEIGHT_RANGE_TEST_002, TestSize.Level3)
{
    data_->supportSwapWidthHeight = true;
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(data_);
    Range range = videoCap->GetSupportedHeight();
    printf("range [%d, %d]\n", range.minVal, range.maxVal);
    ASSERT_EQ(range.minVal, DEFAULT_MIN_SIDE);
    ASSERT_EQ(range.maxVal, DEFAULT_MAX_LONG_SIDE);
}