#include <gtest/gtest.h>
#include <memory>
#include "nalu_analyzer.h"
#include "nalu_analyzer_so_loader.h"

using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class NaluAnalyzerSoLoaderTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: FactoryCreate_AVC_001
 * @tc.desc: Validate NaluAnalyzerFactory correctly loads the implementation for AVC.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, FactoryCreate_AVC_001, TestSize.Level1)
{
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType::AVC);
    EXPECT_NE(analyzer, nullptr);
}

/**
 * @tc.name: FactoryCreate_HEVC_002
 * @tc.desc: Validate NaluAnalyzerFactory correctly loads the implementation for HEVC.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, FactoryCreate_HEVC_002, TestSize.Level1)
{
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType::HEVC);
    EXPECT_NE(analyzer, nullptr);
}

/**
 * @tc.name: FactoryCreate_VVC_003
 * @tc.desc: Validate NaluAnalyzerFactory correctly loads the implementation for VVC.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, FactoryCreate_VVC_003, TestSize.Level1)
{
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType::VVC);
    EXPECT_NE(analyzer, nullptr);
}

/**
 * @tc.name: FactoryCreate_Invalid_004
 * @tc.desc: Verify that an invalid codec type creates an object but gracefully handles internal init failures.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, FactoryCreate_Invalid_004, TestSize.Level1)
{
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType::INVALID);
    EXPECT_NE(analyzer, nullptr);
}

/**
 * @tc.name: IsNonRef_WithNullBuffer_005
 * @tc.desc: Test IsNonReferenceFrame handles nullptr arguments safely.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, IsNonRef_WithNullBuffer_005, TestSize.Level1)
{
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType::AVC);
    if (analyzer) {
        EXPECT_FALSE(analyzer->IsNonReferenceFrame(nullptr));
    }
}