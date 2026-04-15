#include <gtest/gtest.h>
#include "mv_data.h"
#include "mv_parser_factory.h"
#include "mv_parser_interface.h"

using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class MvAnalyzerSoLoaderTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==========================================
// MVStats Tests
// ==========================================

using namespace OHOS::MediaAVCodec::SFD;

/**
 * @tc.name: Analyze_NonUniformGridMapping_001
 * @tc.desc: Test accurate perceptual magnitude calculation with non-uniform grid mapping.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, Analyze_NonUniformGridMapping_001, TestSize.Level1)
{
    MVStats stats;
    MVData mv(0, 10, 10, 1, 0, 0, 1, -1);
    stats.addMV(mv, 0, 0, 100, 100);
    stats.addMV(mv, 50, 50, 100, 100);
    stats.addMV(mv, 99, 99, 100, 100);
    stats.finalize();
    EXPECT_NEAR(stats.getPerceptualMagnitude(), 20.0, 0.01);
}

/**
 * @tc.name: Analyze_ConsistencyPurePanning_002
 * @tc.desc: Test motion consistency under pure panning scenarios.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, Analyze_ConsistencyPurePanning_002, TestSize.Level1)
{
    MVStats stats;
    for (uint32_t i = 0; i < 10; ++i) {
        MVData mv(i, 10, 0, 1, 0, 0, 1, -1);
        stats.addMV(mv, 50, 50, 100, 100);
    }
    stats.finalize();
    EXPECT_NEAR(stats.getMotionConsistency(), 1.0, 0.01);
}

/**
 * @tc.name: Analyze_ConsistencyOpposingMotion_003
 * @tc.desc: Test motion consistency handling with opposing motion vectors in the frame.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, Analyze_ConsistencyOpposingMotion_003, TestSize.Level1)
{
    MVStats stats;
    for (uint32_t i = 0; i < 5; ++i) {
        MVData mv(i, 10, 0, 1, 0, 0, 1, -1);
        stats.addMV(mv, 50, 50, 100, 100);
    }
    for (uint32_t i = 5; i < 10; ++i) {
        MVData mv(i, -10, 0, 1, 0, 0, 1, -1);
        stats.addMV(mv, 50, 50, 100, 100);
    }
    stats.finalize();
    EXPECT_NEAR(stats.getMotionConsistency(), 0.0, 0.01);
}

/**
 * @tc.name: Analyze_ZeroMotionRatio_004
 * @tc.desc: Test calculations of zero motion ratio across different sectors.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, Analyze_ZeroMotionRatio_004, TestSize.Level1)
{
    MVStats stats;
    for (uint32_t i = 0; i < 2; ++i) {
        MVData mv(i, 10, 10, 1, 0, 0, 1, -1);
        stats.addMV(mv, 50, 50, 100, 100);
    }
    for (uint32_t i = 2; i < 10; ++i) {
        MVData mv(i, 0, 0, 1, 0, 0, 1, -1);
        stats.addMV(mv, 50, 50, 100, 100);
    }
    stats.finalize();
    EXPECT_NEAR(stats.getZeroMotionRatio(), 0.8, 0.01);
}

// ==========================================
// MVParserFactory Tests
// ==========================================

/**
 * @tc.name: FactoryCreate_AVC_005
 * @tc.desc: Factory properly initializes and returns an AVC MV parser instance.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, FactoryCreate_AVC_005, TestSize.Level1)
{
    auto parser = MVParserFactory::GetInstance().CreateParser(SFDCodecType::AVC);
    ASSERT_NE(parser, nullptr);
    EXPECT_EQ(parser->GetCodecType(), SFDCodecType::AVC);
}

/**
 * @tc.name: FactoryCreate_HEVC_006
 * @tc.desc: Factory properly initializes and returns an HEVC MV parser instance.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, FactoryCreate_HEVC_006, TestSize.Level1)
{
    auto parser = MVParserFactory::GetInstance().CreateParser(SFDCodecType::HEVC);
    ASSERT_NE(parser, nullptr);
    EXPECT_EQ(parser->GetCodecType(), SFDCodecType::HEVC);
}

/**
 * @tc.name: FactoryCreate_VVC_007
 * @tc.desc: Factory properly initializes and returns a VVC MV parser instance.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, FactoryCreate_VVC_007, TestSize.Level1)
{
    auto parser = MVParserFactory::GetInstance().CreateParser(SFDCodecType::VVC);
    ASSERT_NE(parser, nullptr);
    EXPECT_EQ(parser->GetCodecType(), SFDCodecType::VVC);
}

/**
 * @tc.name: FactoryCreate_InvalidType_008
 * @tc.desc: Verify the factory returns nullptr or handles appropriately for an invalid codec type.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, FactoryCreate_InvalidType_008, TestSize.Level1)
{
    auto parser = MVParserFactory::GetInstance().CreateParser(SFDCodecType::INVALID);
    EXPECT_EQ(parser, nullptr);
}