/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file expect in compliance with the License.
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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "meta/meta_key.h"
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace testing;
using namespace testing::ext;

namespace {
constexpr int32_t DEFAULT_WIDTH = 4096;
constexpr int32_t DEFAULT_HEIGHT = 4096;
const std::string CODEC_MIME_MOCK_00 = "video/codec_mime_00";
CapabilityData HCODEC_CAP = {
    .codecName = "video.H.Decoder.Name.02",
    .codecType = AVCODEC_TYPE_VIDEO_DECODER,
    .bitrate = {1, 40000000},
    .frameRate = {1, 60},
    .channels = {1, 30},
    .alignment = {2, 2},
    .blockSize = {2, 2},
    .sampleRate = {1, 2, 3, 4, 5},
    .mimeType = CODEC_MIME_MOCK_00,
    .isVendor = false,
    .width = {96, 4096},
    .height = {96, 4096},
    .pixFormat = {1, 2, 3}
};

class AVCodecInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    std::shared_ptr<VideoCaps> videoCaps_ = nullptr;
    int32_t width_;
    int32_t height_;
};

class CodecParamCheckerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(void){};
    void TearDown(void){};
};

void AVCodecInfoTest::SetUpTestCase(void) {}

void AVCodecInfoTest::TearDownTestCase(void) {}

void AVCodecInfoTest::SetUp(void)
{
    width_ = DEFAULT_WIDTH;
    height_ = DEFAULT_HEIGHT;
    videoCaps_ = std::make_shared<VideoCaps>(&HCODEC_CAP);

    EXPECT_NE(videoCaps_, nullptr);
}

void AVCodecInfoTest::TearDown(void)
{
    videoCaps_ = nullptr;
}

/**
 * @tc.name: IsSizeSupported_Invalid_Test_001
 * @tc.desc: blockFrame is invalid
 */
HWTEST_F(AVCodecInfoTest, IsSizeSupported_Invalid_Test_001, TestSize.Level1)
{
    videoCaps_->blockPerFrameRange_ = {1000000, INT32_MAX};
    width_ = 720;
    height_ = 720;
    bool ret = videoCaps_->IsSizeSupported(width_, height_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: IsSizeSupported_Invalid_Test_002
 * @tc.desc: blockFrame is invalid
 */
HWTEST_F(AVCodecInfoTest, IsSizeSupported_Invalid_Test_002, TestSize.Level1)
{
    videoCaps_->blockPerFrameRange_ = {1000000, 2000000};
    bool ret = videoCaps_->IsSizeSupported(width_, height_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: IsSizeSupported_Invalid_Test_003
 * @tc.desc: alignment.width is zero
 */
HWTEST_F(AVCodecInfoTest, IsSizeSupported_Invalid_Test_003, TestSize.Level1)
{
    videoCaps_->data_->alignment = {0, 2};
    bool ret = videoCaps_->IsSizeSupported(width_, height_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: IsSizeSupported_Invalid_Test_004
 * @tc.desc: alignment.height is zero
 */
HWTEST_F(AVCodecInfoTest, IsSizeSupported_Invalid_Test_004, TestSize.Level1)
{
    videoCaps_->data_->alignment = {2, 0};
    bool ret = videoCaps_->IsSizeSupported(width_, height_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: IsSizeSupported_Invalid_Test_005
 * @tc.desc: width % alignment.width equals zero
 */
HWTEST_F(AVCodecInfoTest, IsSizeSupported_Invalid_Test_005, TestSize.Level1)
{
    videoCaps_->data_->alignment = {3, 2};
    bool ret = videoCaps_->IsSizeSupported(width_, height_);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: IsSizeSupported_Invalid_Test_006
 * @tc.desc: width % alignment.height equals zero
 */
HWTEST_F(AVCodecInfoTest, IsSizeSupported_Invalid_Test_006, TestSize.Level1)
{
    videoCaps_->data_->alignment = {2, 3};
    videoCaps_->IsSizeSupported(width_, height_);
}

/**
 * @tc.name: GetVideoWidthRangeForHeight_Valid_Test_001
 * @tc.desc: height < height.minVal
 */
HWTEST_F(AVCodecInfoTest, GetVideoWidthRangeForHeight_Valid_Test_001, TestSize.Level1)
{
    height_ = 30;
    videoCaps_->GetVideoWidthRangeForHeight(height_);
}

/**
 * @tc.name: GetVideoWidthRangeForHeight_Valid_Test_002
 * @tc.desc: height > height.minVal
 */
HWTEST_F(AVCodecInfoTest, GetVideoWidthRangeForHeight_Valid_Test_002, TestSize.Level1)
{
    height_ = 8192;
    videoCaps_->GetVideoWidthRangeForHeight(height_);
}

/**
 * @tc.name: GetVideoWidthRangeForHeight_Valid_Test_003
 * @tc.desc: verticalBlockNum < verticalBlockRange_.minVal
 */
HWTEST_F(AVCodecInfoTest, GetVideoWidthRangeForHeight_Valid_Test_003, TestSize.Level1)
{
    videoCaps_->horizontalBlockRange_ = {0, 0};
    videoCaps_->verticalBlockRange_.minVal = 8192;
    videoCaps_->GetVideoWidthRangeForHeight(height_);
}

/**
 * @tc.name: GetVideoWidthRangeForHeight_Valid_Test_004
 * @tc.desc: verticalBlockNum > verticalBlockRange_.maxVal
 */
HWTEST_F(AVCodecInfoTest, GetVideoWidthRangeForHeight_Valid_Test_004, TestSize.Level1)
{
    videoCaps_->horizontalBlockRange_ = {0, 0};
    videoCaps_->verticalBlockRange_.maxVal = 1024;
    videoCaps_->GetVideoWidthRangeForHeight(height_);
}

/**
 * @tc.name: GetVideoHeightRangeForWidth_Valid_Test_001
 * @tc.desc: width < width.minVal
 */
HWTEST_F(AVCodecInfoTest, GetVideoHeightRangeForWidth_Valid_Test_001, TestSize.Level1)
{
    width_ = 30;
    videoCaps_->GetVideoHeightRangeForWidth(width_);
}

/**
 * @tc.name: GetVideoHeightRangeForWidth_Valid_Test_002
 * @tc.desc: width > width.minVal
 */
HWTEST_F(AVCodecInfoTest, GetVideoHeightRangeForWidth_Valid_Test_002, TestSize.Level1)
{
    width_ = 8192;
    videoCaps_->GetVideoHeightRangeForWidth(width_);
}

/**
 * @tc.name: GetVideoHeightRangeForWidth_Valid_Test_003
 * @tc.desc: horizontalBlockRange_ < horizontalBlockRange_.minVal
 */
HWTEST_F(AVCodecInfoTest, GetVideoHeightRangeForWidth_Valid_Test_003, TestSize.Level1)
{
    videoCaps_->verticalBlockRange_ = {0, 0};
    videoCaps_->horizontalBlockRange_.minVal = 8192;
    videoCaps_->GetVideoHeightRangeForWidth(height_);
}

/**
 * @tc.name: GetVideoHeightRangeForWidth_Valid_Test_004
 * @tc.desc: verticalBlockNum > verticalBlockRange_.maxVal
 */
HWTEST_F(AVCodecInfoTest, GetVideoHeightRangeForWidth_Valid_Test_004, TestSize.Level1)
{
    videoCaps_->verticalBlockRange_ = {0, 0};
    videoCaps_->horizontalBlockRange_.maxVal = 1024;
    videoCaps_->GetVideoHeightRangeForWidth(height_);
}

/**
 * @tc.name: GetSupportedFrameRatesFor_Valid_Test_001
 * @tc.desc: size unsupported
 */
HWTEST_F(AVCodecInfoTest, GetSupportedFrameRatesFor_Valid_Test_001, TestSize.Level1)
{
    width_ = 0;
    height_ = 0;
    videoCaps_->GetSupportedFrameRatesFor(width_, height_);
}

/**
 * @tc.name: LoadMPEGLevelParams_Valid_Test_001
 * @tc.desc: mime equals VIDEO_MPEG2
 */
HWTEST_F(AVCodecInfoTest, LoadMPEGLevelParams_Valid_Test_001, TestSize.Level1)
{
    videoCaps_->data_->profileLevelsMap.insert(
        {static_cast<int32_t>(MPEG2_PROFILE_SIMPLE), {MPEG2_LEVEL_ML}});
    videoCaps_->data_->profileLevelsMap.insert({static_cast<int32_t>(MPEG2_PROFILE_MAIN), {}});
    videoCaps_->data_->profileLevelsMap.insert({static_cast<int32_t>(MPEG2_PROFILE_422), {}});
    videoCaps_->LoadMPEGLevelParams(static_cast<std::string>(CodecMimeType::VIDEO_MPEG2));
}

/**
 * @tc.name: UpdateBlockParams_Valid_Test_001
 * @tc.desc: blockWidth_ equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateBlockParams_Valid_Test_001, TestSize.Level1)
{
    constexpr int32_t blockWidth = 720;
    constexpr int32_t blockHeight = 720;
    videoCaps_->blockWidth_ = 0;
    OHOS::MediaAVCodec::Range range = OHOS::MediaAVCodec::Range(0, 0);
    videoCaps_->UpdateBlockParams(blockWidth, blockHeight, range, range);
}

/**
 * @tc.name: UpdateBlockParams_Valid_Test_002
 * @tc.desc: blockHeight_ equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateBlockParams_Valid_Test_002, TestSize.Level1)
{
    constexpr int32_t blockWidth = 720;
    constexpr int32_t blockHeight = 720;
    videoCaps_->blockHeight_ = 0;
    OHOS::MediaAVCodec::Range range = OHOS::MediaAVCodec::Range(0, 0);
    videoCaps_->UpdateBlockParams(blockWidth, blockHeight, range, range);
}

/**
 * @tc.name: UpdateBlockParams_Valid_Test_003
 * @tc.desc: blockWidth equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateBlockParams_Valid_Test_003, TestSize.Level1)
{
    constexpr int32_t blockWidth = 0;
    constexpr int32_t blockHeight = 1;
    OHOS::MediaAVCodec::Range range = OHOS::MediaAVCodec::Range(0, 0);
    videoCaps_->UpdateBlockParams(blockWidth, blockHeight, range, range);
}

/**
 * @tc.name: UpdateBlockParams_Valid_Test_004
 * @tc.desc: blockHeight equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateBlockParams_Valid_Test_004, TestSize.Level1)
{
    constexpr int32_t blockWidth = 1;
    constexpr int32_t blockHeight = 0;
    OHOS::MediaAVCodec::Range range = OHOS::MediaAVCodec::Range(0, 0);
    videoCaps_->UpdateBlockParams(blockWidth, blockHeight, range, range);
}

/**
 * @tc.name: InitParams_Valid_Test_001
 * @tc.desc: 1. blockPerSecond.minVal equals zero
 *           2. blockPerFrame.minVal equals zero
 *           3. width.minVal equals zero
 *           4. height.minVal equals zero
 *           5. frameRate.maxVal equals zero
 *           5. blockSize.width equals zero
 */
HWTEST_F(AVCodecInfoTest, InitParams_Valid_Test_001, TestSize.Level1)
{
    videoCaps_->data_->blockPerSecond.minVal = 0;
    videoCaps_->data_->blockPerFrame.minVal = 0;
    videoCaps_->data_->width.minVal = 0;
    videoCaps_->data_->height.minVal = 0;
    videoCaps_->data_->frameRate.maxVal = 0;
    videoCaps_->data_->blockSize.width = 0;
    videoCaps_->InitParams();
}

/**
 * @tc.name: InitParams_Valid_Test_002
 * @tc.desc: 1. blockPerSecond.maxVal equals zero
 *           2. blockPerFrame.maxVal equals zero
 *           3. width.maxVal equals zero
 *           4. height.maxVal equals zero
 *           5. blockSize.height equals zero
 */
HWTEST_F(AVCodecInfoTest, InitParams_Valid_Test_002, TestSize.Level1)
{
    videoCaps_->data_->blockPerSecond.maxVal = 0;
    videoCaps_->data_->blockPerFrame.maxVal = 0;
    videoCaps_->data_->width.maxVal = 0;
    videoCaps_->data_->height.maxVal = 0;
    videoCaps_->data_->blockSize.height = 0;
    videoCaps_->InitParams();
}

/**
 * @tc.name: UpdateParams_Valid_Test_001
 * @tc.desc: blockSize.width equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateParams_Valid_Test_001, TestSize.Level1)
{
    videoCaps_->data_->blockSize.width = 0;
    videoCaps_->UpdateParams();
}

/**
 * @tc.name: UpdateParams_Valid_Test_002
 * @tc.desc: blockSize.height equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateParams_Valid_Test_002, TestSize.Level1)
{
    videoCaps_->data_->blockSize.height = 0;
    videoCaps_->UpdateParams();
}

/**
 * @tc.name: UpdateParams_Valid_Test_003
 * @tc.desc: blockWidth_ equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateParams_Valid_Test_003, TestSize.Level1)
{
    videoCaps_->blockWidth_ = 0;
    videoCaps_->UpdateParams();
}

/**
 * @tc.name: UpdateParams_Valid_Test_004
 * @tc.desc: blockHeight_ equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateParams_Valid_Test_004, TestSize.Level1)
{
    videoCaps_->blockHeight_ = 0;
    videoCaps_->UpdateParams();
}

/**
 * @tc.name: UpdateParams_Valid_Test_005
 * @tc.desc: verticalBlockRange_.maxVal equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateParams_Valid_Test_005, TestSize.Level1)
{
    videoCaps_->verticalBlockRange_.maxVal = 0;
    videoCaps_->UpdateParams();
}

/**
 * @tc.name: UpdateParams_Valid_Test_006
 * @tc.desc: verticalBlockRange_.minVal equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateParams_Valid_Test_006, TestSize.Level1)
{
    videoCaps_->verticalBlockRange_.minVal = 0;
    videoCaps_->UpdateParams();
}

/**
 * @tc.name: UpdateParams_Valid_Test_007
 * @tc.desc: horizontalBlockRange_.maxVal equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateParams_Valid_Test_007, TestSize.Level1)
{
    videoCaps_->horizontalBlockRange_.maxVal = 0;
    videoCaps_->UpdateParams();
}

/**
 * @tc.name: UpdateParams_Valid_Test_008
 * @tc.desc: horizontalBlockRange_.minVal equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateParams_Valid_Test_008, TestSize.Level1)
{
    videoCaps_->horizontalBlockRange_.minVal = 0;
    videoCaps_->UpdateParams();
}

/**
 * @tc.name: UpdateParams_Valid_Test_009
 * @tc.desc: blockPerFrameRange_.minVal equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateParams_Valid_Test_009, TestSize.Level1)
{
    videoCaps_->blockPerFrameRange_.minVal = 0;
    videoCaps_->UpdateParams();
}

/**
 * @tc.name: UpdateParams_Valid_Test_010
 * @tc.desc: blockPerFrameRange_.maxVal equals zero
 */
HWTEST_F(AVCodecInfoTest, UpdateParams_Valid_Test_010, TestSize.Level1)
{
    videoCaps_->blockPerFrameRange_.maxVal = 0;
    videoCaps_->UpdateParams();
}

/**
 * @tc.name: DivRange_Valid_Test_001
 * @tc.desc: divisor equals zero
 */
HWTEST_F(AVCodecInfoTest, DivRange_Valid_Test_001, TestSize.Level1)
{
    OHOS::MediaAVCodec::Range range = OHOS::MediaAVCodec::Range(0, 0);
    constexpr int32_t divisor = 0;
    videoCaps_->DivRange(range, divisor);
}

/**
 * @tc.name: DivCeil_Valid_Test_001
 * @tc.desc: divisor equals zero
 */
HWTEST_F(AVCodecInfoTest, DivCeil_Valid_Test_001, TestSize.Level1)
{
    constexpr int32_t dividend = 0;
    constexpr int32_t divisor = 0;
    auto ret = videoCaps_->DivCeil(dividend, divisor);
    EXPECT_EQ(ret, INT32_MAX);
}

/**
 * @tc.name: GetPreferredFrameRate_Valid_Test_001
 * @tc.desc: measuredFrameRate.size() equals zero
 */
HWTEST_F(AVCodecInfoTest, GetPreferredFrameRate_Valid_Test_001, TestSize.Level1)
{
    videoCaps_->blockPerFrameRange_ = {1000000, 2000000};
    videoCaps_->blockWidth_ = 0;
    videoCaps_->data_->measuredFrameRate.clear();
    videoCaps_->GetPreferredFrameRate(width_, height_);
}

/**
 * @tc.name: GetPreferredFrameRate_Valid_Test_002
 * @tc.desc: can not match measuredFrameRate
 */
HWTEST_F(AVCodecInfoTest, GetPreferredFrameRate_Valid_Test_002, TestSize.Level1)
{
    ImgSize imageSize = {1, DEFAULT_WIDTH / 2};
    MediaAVCodec::Range range = {1, INT32_MAX};
    videoCaps_->blockPerFrameRange_ = {1000000, 2000000};
    videoCaps_->blockWidth_ = 0;
    videoCaps_->data_->measuredFrameRate.emplace(imageSize, range);
    videoCaps_->GetPreferredFrameRate(width_, height_);
}
} // namespace