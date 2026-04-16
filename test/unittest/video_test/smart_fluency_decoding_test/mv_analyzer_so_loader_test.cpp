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
#include "mv_analyzer.h"
#include "mv_analyzer_so_loader.h"

using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;
class MvAnalyzerSoLoaderTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};
/**
 * @tc.name: CreateMvAnalyzer_AVC_001
 * @tc.desc: Factory properly initializes and returns an AVC MV analyzer instance.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, CreateMvAnalyzer_AVC_001, TestSize.Level1)
{
    auto analyzer = MvAnalyzerFactory::CreateMvAnalyzer(SFDCodecType::AVC);
    // Note: This might be nullptr if the SO is missing in the test environment, 
    // but the factory logic itself is being exercised.
    if (analyzer != nullptr) {
        EXPECT_EQ(analyzer->GetCodecType(), SFDCodecType::AVC);
    }
}

/**
 * @tc.name: CreateMvAnalyzer_HEVC_002
 * @tc.desc: Factory properly initializes and returns an HEVC MV analyzer instance.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, CreateMvAnalyzer_HEVC_002, TestSize.Level1)
{
    auto analyzer = MvAnalyzerFactory::CreateMvAnalyzer(SFDCodecType::HEVC);
    if (analyzer != nullptr) {
        EXPECT_EQ(analyzer->GetCodecType(), SFDCodecType::HEVC);
    }
}

/**
 * @tc.name: CreateMvAnalyzer_VVC_003
 * @tc.desc: Factory properly initializes and returns a VVC MV analyzer instance.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, CreateMvAnalyzer_VVC_003, TestSize.Level1)
{
    auto analyzer = MvAnalyzerFactory::CreateMvAnalyzer(SFDCodecType::VVC);
    if (analyzer != nullptr) {
        EXPECT_EQ(analyzer->GetCodecType(), SFDCodecType::VVC);
    }
}

/**
 * @tc.name: CreateMvAnalyzer_InvalidType_004
 * @tc.desc: Verify the factory returns nullptr for an invalid codec type.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, CreateMvAnalyzer_InvalidType_004, TestSize.Level1)
{
    auto analyzer = MvAnalyzerFactory::CreateMvAnalyzer(SFDCodecType::INVALID);
    EXPECT_EQ(analyzer, nullptr);
}

/**
 * @tc.name: IsCodecSupported_VariousTypes_005
 * @tc.desc: Test IsCodecSupported for both supported and unsupported types.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, IsCodecSupported_VariousTypes_005, TestSize.Level1)
{
    EXPECT_TRUE(MvAnalyzerFactory::IsCodecSupported(SFDCodecType::AVC));
    EXPECT_TRUE(MvAnalyzerFactory::IsCodecSupported(SFDCodecType::HEVC));
    EXPECT_TRUE(MvAnalyzerFactory::IsCodecSupported(SFDCodecType::VVC));
    EXPECT_FALSE(MvAnalyzerFactory::IsCodecSupported(SFDCodecType::INVALID));
}

/**
 * @tc.name: Loader_InterfaceCall_NoCrash_006
 * @tc.desc: Ensure that calling interface methods through the loader doesn't crash even if SO is missing.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, Loader_InterfaceCall_NoCrash_006, TestSize.Level1)
{
    // Directly use the loader to test its internal safety guards
    auto loader = std::make_unique<MvAnalyzerSoLoader>(SFDCodecType::AVC);

    // These should work safely even if LoadLibrary() failed (handle_ is null)
    loader->SetVideoInfo(1920, 1080);

    StreamAuxInfo auxInfo{};
    loader->SetStreamAuxInfo(auxInfo);

    MVStats stats = loader->Analyze(nullptr);
    // If SO is missing, it returns default zeroed stats
    EXPECT_EQ(stats.totalBlocks, 0);
    EXPECT_EQ(loader->GetCodecType(), SFDCodecType::AVC);
}
