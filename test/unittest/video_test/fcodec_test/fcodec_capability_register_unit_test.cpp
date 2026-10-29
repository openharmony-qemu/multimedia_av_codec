/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <string>
#include "avcodec_codec_name.h"
#include "fcodec_surport_codec.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Codec;
using namespace testing::ext;

namespace {
// WMV1 supported resolution range: [16, 4096] x [16, 4096]
constexpr int32_t WMV1_MIN_SIZE = 16;
constexpr int32_t WMV1_MAX_SIZE = 4096;
// A resolution valid for every entry in SUPPORT_VCODEC: it satisfies even the
// most restrictive codec (DVVIDEO [720,1440]x[480,1080], minPixels 720*480).
constexpr int32_t COMMON_VALID_WIDTH = 720;
constexpr int32_t COMMON_VALID_HEIGHT = 480;
} // namespace

// GetResolutionRange()/ValidateVideoResolution() live in fcodec_capability_register.cpp.
// Compiling that translation unit into this test binary makes the static helper
// reachable, so its per-codec branches (incl. WMV1) get exercised for coverage.
class FcodecCapabilityRegisterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void FcodecCapabilityRegisterUnitTest::SetUpTestCase(void) {}
void FcodecCapabilityRegisterUnitTest::TearDownTestCase(void) {}
void FcodecCapabilityRegisterUnitTest::SetUp(void) {}
void FcodecCapabilityRegisterUnitTest::TearDown(void) {}

/**
 * @tc.name: ValidateVideoResolution_WMV1_Valid_001
 * @tc.desc: Validate WMV1 resolutions within the supported range
 * @tc.type: FUNC
 */
HWTEST_F(FcodecCapabilityRegisterUnitTest, ValidateVideoResolution_WMV1_Valid_001, TestSize.Level1)
{
    auto name = std::string(AVCodecCodecName::VIDEO_DECODER_WMV1_NAME);
    // typical inner resolution
    EXPECT_TRUE(ValidateVideoResolution(name, COMMON_VALID_WIDTH, COMMON_VALID_HEIGHT));
    // lower boundary: min width/height, min pixels (16 * 16)
    EXPECT_TRUE(ValidateVideoResolution(name, WMV1_MIN_SIZE, WMV1_MIN_SIZE));
    // upper boundary: max width/height, max pixels (4096 * 4096)
    EXPECT_TRUE(ValidateVideoResolution(name, WMV1_MAX_SIZE, WMV1_MAX_SIZE));
}

/**
 * @tc.name: ValidateVideoResolution_WMV1_Invalid_001
 * @tc.desc: Validate WMV1 resolutions outside the supported range
 * @tc.type: FUNC
 */
HWTEST_F(FcodecCapabilityRegisterUnitTest, ValidateVideoResolution_WMV1_Invalid_001, TestSize.Level1)
{
    auto name = std::string(AVCodecCodecName::VIDEO_DECODER_WMV1_NAME);
    // below min width
    EXPECT_FALSE(ValidateVideoResolution(name, WMV1_MIN_SIZE - 1, COMMON_VALID_HEIGHT));
    // above max width
    EXPECT_FALSE(ValidateVideoResolution(name, WMV1_MAX_SIZE + 1, COMMON_VALID_HEIGHT));
    // below min height
    EXPECT_FALSE(ValidateVideoResolution(name, COMMON_VALID_WIDTH, WMV1_MIN_SIZE - 1));
    // above max height
    EXPECT_FALSE(ValidateVideoResolution(name, COMMON_VALID_WIDTH, WMV1_MAX_SIZE + 1));
    // non-positive dimensions are rejected before the range lookup
    EXPECT_FALSE(ValidateVideoResolution(name, 0, COMMON_VALID_HEIGHT));
    EXPECT_FALSE(ValidateVideoResolution(name, COMMON_VALID_WIDTH, 0));
    EXPECT_FALSE(ValidateVideoResolution(name, -1, COMMON_VALID_HEIGHT));
}

/**
 * @tc.name: ValidateVideoResolution_AllSupportedCodecs_001
 * @tc.desc: A common in-range resolution is valid for every supported decoder
 * @tc.type: FUNC
 */
HWTEST_F(FcodecCapabilityRegisterUnitTest, ValidateVideoResolution_AllSupportedCodecs_001, TestSize.Level1)
{
    for (uint32_t i = 0; i < SUPPORT_VCODEC_NUM; ++i) {
        auto name = std::string(SUPPORT_VCODEC[i].codecName);
        EXPECT_TRUE(ValidateVideoResolution(name, COMMON_VALID_WIDTH, COMMON_VALID_HEIGHT))
            << "codec name: " << name;
    }
}

/**
 * @tc.name: ValidateVideoResolution_Dvvideo_PixelsCheck_001
 * @tc.desc: DVVIDEO exercises the pixel-count boundaries of ValidateVideoResolution
 * @tc.type: FUNC
 */
HWTEST_F(FcodecCapabilityRegisterUnitTest, ValidateVideoResolution_Dvvideo_PixelsCheck_001, TestSize.Level1)
{
    // DVVIDEO range is [720,1440]x[480,1080]; min/max pixel counts are reached
    // independently of the per-side bounds, so they exercise the pixel checks.
    auto name = std::string(AVCodecCodecName::VIDEO_DECODER_DVVIDEO_NAME);
    // both sides within [480, 1440] but pixels (480*480) < minPixels (720*480)
    EXPECT_FALSE(ValidateVideoResolution(name, 480, 480));
    // both sides within [480, 1440] but pixels (1440*1440) > maxPixels (1440*1080)
    EXPECT_FALSE(ValidateVideoResolution(name, 1440, 1440));
    // within range and pixel bounds
    EXPECT_TRUE(ValidateVideoResolution(name, COMMON_VALID_WIDTH, COMMON_VALID_HEIGHT));
}

/**
 * @tc.name: ValidateVideoResolution_UnknownCodec_001
 * @tc.desc: Unknown codec falls back to the default resolution range
 * @tc.type: FUNC
 */
HWTEST_F(FcodecCapabilityRegisterUnitTest, ValidateVideoResolution_UnknownCodec_001, TestSize.Level1)
{
    constexpr char unknownName[] = "OH.Media.Codec.Decoder.Video.UNKNOWN";
    // default range {2, 4096, 2, 4096}; 720x480 is valid
    EXPECT_TRUE(ValidateVideoResolution(unknownName, COMMON_VALID_WIDTH, COMMON_VALID_HEIGHT));
    // below default min (2)
    EXPECT_FALSE(ValidateVideoResolution(unknownName, 1, COMMON_VALID_HEIGHT));
}
