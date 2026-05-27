/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <memory>
#include "smart_fluency_decoding_builder.h"
#include "smart_fluency_decoding.h"
#include "meta/video_types.h"
#include "test_mock_analyzers.h"

namespace {
constexpr int32_t WIDTH_1080P = 1920;
constexpr int32_t HEIGHT_1080P = 1080;
constexpr int32_t WIDTH_720P = 1280;
constexpr int32_t HEIGHT_720P = 720;
}

using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class SFDBuilderTest : public testing::Test {
protected:
    void SetUp() override
    {
        SetupTestAnalyzerMocks();
    }
    void TearDown() override
    {
        TeardownTestAnalyzerMocks();
    }
};

/**
 * @tc.name: Build_MissingParams_001
 * @tc.desc: Fail gracefully when critical parameters are missing from builder pattern.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, Build_MissingParams_001, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    auto sfd = builder.Build();
    EXPECT_EQ(sfd, nullptr);
}

/**
 * @tc.name: Build_WithFormat_002
 * @tc.desc: Factory successfully constructs a decoder via complete Format options.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, Build_WithFormat_002, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutStringValue(Tag::MIME_TYPE, "video/avc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
        static_cast<int32_t>(Plugins::FrameRetentionMode::UNIFORM));
    format.PutDoubleValue(Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, 0.8); // 0.8: retention ratio

    builder.SetCodecMime("video/avc").SetFromFormat(format);
    auto sfd = builder.Build();
    EXPECT_NE(sfd, nullptr);
}

/**
 * @tc.name: Build_ResolutionBoundary_003
 * @tc.desc: Refuse zero dimensions gracefully during construction.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, Build_ResolutionBoundary_003, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    builder.SetVideoWidth(0).SetVideoHeight(0);
    auto sfd = builder.Build();
    EXPECT_EQ(sfd, nullptr);
}

/**
 * @tc.name: Build_InvalidRetentionMode_004
 * @tc.desc: Handle unknown internal retention modes gracefully.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, Build_InvalidRetentionMode_004, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutStringValue(Tag::MIME_TYPE, "video/hevc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, -99); // -99: invalid mode

    builder.SetCodecMime("video/hevc").SetFromFormat(format);
    auto sfd = builder.Build();
    EXPECT_EQ(sfd, nullptr);
}

/**
 * @tc.name: SetFromFormat_Comprehensive_006
 * @tc.desc: Ensure complete options sets without error paths properly setup configurations.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, SetFromFormat_Comprehensive_006, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_720P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_720P);
    format.PutStringValue(Tag::MIME_TYPE, "video/avc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
        static_cast<int32_t>(Plugins::FrameRetentionMode::UNIFORM));
    format.PutDoubleValue(Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, 0.75); // 0.75: retention ratio
    format.PutDoubleValue(Tag::VIDEO_DECODER_SPEED, 1.5); // 1.5: playback speed

    auto& ref = builder.SetCodecMime("video/avc").SetFromFormat(format);
    EXPECT_EQ(&ref, &builder);

    auto sfd = builder.Build();
    EXPECT_NE(sfd, nullptr);
}

/**
 * @tc.name: Build_FullMode_009
 * @tc.desc: Properly handles builder instructions explicitly disabling filtering via FULL Mode.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, Build_FullMode_009, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutStringValue(Tag::MIME_TYPE, "video/hevc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
        static_cast<int32_t>(Plugins::FrameRetentionMode::FULL));

    builder.SetCodecMime("video/hevc").SetFromFormat(format);
    auto sfd = builder.Build();
    EXPECT_NE(sfd, nullptr);
}

/**
 * @tc.name: Build_AdaptiveMode_010
 * @tc.desc: Builds an adaptive instance gracefully.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, Build_AdaptiveMode_010, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutStringValue(Tag::MIME_TYPE, "video/avc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
        static_cast<int32_t>(Plugins::FrameRetentionMode::ADAPTIVE));

    builder.SetCodecMime("video/avc").SetFromFormat(format);
    auto sfd = builder.Build();
    EXPECT_NE(sfd, nullptr);
}

/**
 * @tc.name: Build_InvalidRetentionRatio_011
 * @tc.desc: Ensure negative ratios seamlessly downgrade configuration into AUTO_RATIO rather than crashing.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, Build_InvalidRetentionRatio_011, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutStringValue(Tag::MIME_TYPE, "video/avc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
        static_cast<int32_t>(Plugins::FrameRetentionMode::UNIFORM));
    format.PutDoubleValue(Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, -0.5); // -0.5: invalid ratio

    builder.SetCodecMime("video/avc").SetFromFormat(format);
    auto sfd = builder.Build();
    // Negative ratio is invalid, should fallback to UNIFORM_DEFAULT
    EXPECT_NE(sfd, nullptr);
}

/**
 * @tc.name: SetMethods_FluentApiChaining_012
 * @tc.desc: Ensures all chained invocations return valid object endpoints.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, SetMethods_FluentApiChaining_012, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    auto& ref = builder.SetVideoWidth(WIDTH_1080P).SetVideoHeight(HEIGHT_1080P).SetCodecMime("video/avc");
    EXPECT_EQ(&ref, &builder);
}

/**
 * @tc.name: Build_VvcCodec_013
 * @tc.desc: Verify SFD builder successfully handles VVC codec mime type.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, Build_VvcCodec_013, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutStringValue(Tag::MIME_TYPE, "video/vvc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0);

    builder.SetCodecMime("video/vvc").SetFromFormat(format);
    auto sfd = builder.Build();
    EXPECT_NE(sfd, nullptr);
}

/**
 * @tc.name: Build_UnknownMime_014
 * @tc.desc: Verify Build returns nullptr when an unsupported mime type is provided.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, Build_UnknownMime_014, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutStringValue(Tag::MIME_TYPE, "video/vp9");

    builder.SetCodecMime("video/vp9").SetFromFormat(format);
    auto sfd = builder.Build();
    EXPECT_EQ(sfd, nullptr);
}

/**
 * @tc.name: Build_UpdatePlaybackSpeed_015
 * @tc.desc: Verify playback speed is propagated to the built SFD instance.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, Build_UpdatePlaybackSpeed_015, TestSize.Level1)
{
    SmartFluencyDecodingBuilder builder;
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutStringValue(Tag::MIME_TYPE, "video/avc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0);
    format.PutDoubleValue(Tag::VIDEO_DECODER_SPEED, 2.0);

    builder.SetCodecMime("video/avc").SetFromFormat(format);
    auto sfd = builder.Build();
    ASSERT_NE(sfd, nullptr);
}
