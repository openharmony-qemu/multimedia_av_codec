#include <gtest/gtest.h>
#include <memory>
#include "smart_fluency_decoding_builder.h"
#include "smart_fluency_decoding.h"
#include "meta/video_types.h"

using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class SFDBuilderTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
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
    format.PutIntValue(Tag::VIDEO_WIDTH, 1920);
    format.PutIntValue(Tag::VIDEO_HEIGHT, 1080);
    format.PutStringValue(Tag::MIME_TYPE, "video/avc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
        static_cast<int32_t>(Plugins::FrameRetentionMode::UNIFORM));
    format.PutDoubleValue(Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, 0.8);

    builder.SetFromFormat(format);
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
    format.PutIntValue(Tag::VIDEO_WIDTH, 1920);
    format.PutIntValue(Tag::VIDEO_HEIGHT, 1080);
    format.PutStringValue(Tag::MIME_TYPE, "video/hevc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, -99);

    builder.SetFromFormat(format);
    auto sfd = builder.Build();
    EXPECT_EQ(sfd, nullptr);
}

/**
 * @tc.name: IsCodecSupported_VariousCodecs_005
 * @tc.desc: Correctly identify SFD readiness for multiple defined codec names against versions.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, IsCodecSupported_VariousCodecs_005, TestSize.Level1)
{
    EXPECT_TRUE(SmartFluencyDecodingBuilder::IsCodecSupported("video/avc", 1));
    EXPECT_TRUE(SmartFluencyDecodingBuilder::IsCodecSupported("video/hevc", 1));
    EXPECT_TRUE(SmartFluencyDecodingBuilder::IsCodecSupported("video/vvc", 1));
    EXPECT_FALSE(SmartFluencyDecodingBuilder::IsCodecSupported("video/avc", 0));
    EXPECT_FALSE(SmartFluencyDecodingBuilder::IsCodecSupported("audio/mp4a-latm", 1));
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
    format.PutIntValue(Tag::VIDEO_WIDTH, 1280);
    format.PutIntValue(Tag::VIDEO_HEIGHT, 720);
    format.PutStringValue(Tag::MIME_TYPE, "video/avc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
        static_cast<int32_t>(Plugins::FrameRetentionMode::UNIFORM));
    format.PutDoubleValue(Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, 0.75);
    format.PutDoubleValue(Tag::VIDEO_DECODER_SPEED, 1.5);

    auto& ref = builder.SetFromFormat(format);
    EXPECT_EQ(&ref, &builder);

    auto sfd = builder.Build();
    EXPECT_NE(sfd, nullptr);
}

/**
 * @tc.name: EnableMVOutput_FormatTag_007
 * @tc.desc: Test whether EnableMVOutput modifies Format properties correctly.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, EnableMVOutput_FormatTag_007, TestSize.Level1)
{
    Format format;
    SmartFluencyDecodingBuilder::EnableMVOutput(format);
    int32_t mvUpload = 0;
    EXPECT_TRUE(format.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, mvUpload));
    EXPECT_EQ(mvUpload, 1);
}

/**
 * @tc.name: ParseCodecType_ValidTypes_008
 * @tc.desc: Validate strings matching successfully mapped implementations.
 * @tc.type: FUNC
 */
HWTEST_F(SFDBuilderTest, ParseCodecType_ValidTypes_008, TestSize.Level1)
{
    EXPECT_EQ(SmartFluencyDecodingBuilder::ParseCodecType("video/avc"), SFDCodecType::AVC);
    EXPECT_EQ(SmartFluencyDecodingBuilder::ParseCodecType("video/hevc"), SFDCodecType::HEVC);
    EXPECT_EQ(SmartFluencyDecodingBuilder::ParseCodecType("video/vvc"), SFDCodecType::VVC);
    EXPECT_EQ(SmartFluencyDecodingBuilder::ParseCodecType("video/unknown"), SFDCodecType::INVALID);
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
    format.PutIntValue(Tag::VIDEO_WIDTH, 1920);
    format.PutIntValue(Tag::VIDEO_HEIGHT, 1080);
    format.PutStringValue(Tag::MIME_TYPE, "video/hevc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
        static_cast<int32_t>(Plugins::FrameRetentionMode::FULL));

    builder.SetFromFormat(format);
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
    format.PutIntValue(Tag::VIDEO_WIDTH, 1920);
    format.PutIntValue(Tag::VIDEO_HEIGHT, 1080);
    format.PutStringValue(Tag::MIME_TYPE, "video/avc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
        static_cast<int32_t>(Plugins::FrameRetentionMode::ADAPTIVE));

    builder.SetFromFormat(format);
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
    format.PutIntValue(Tag::VIDEO_WIDTH, 1920);
    format.PutIntValue(Tag::VIDEO_HEIGHT, 1080);
    format.PutStringValue(Tag::MIME_TYPE, "video/avc");
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
        static_cast<int32_t>(Plugins::FrameRetentionMode::UNIFORM));
    format.PutDoubleValue(Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, -0.5);

    builder.SetFromFormat(format);
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
    auto& ref = builder.SetVideoWidth(1920).SetVideoHeight(1080).SetCodecMime("video/avc");
    EXPECT_EQ(&ref, &builder);
}
