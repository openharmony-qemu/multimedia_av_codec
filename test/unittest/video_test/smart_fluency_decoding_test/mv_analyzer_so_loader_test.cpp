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
#include "test_mock_analyzers.h"

bool GetStreamAuxInfoCalled();
const StreamAuxInfo& GetLastStreamAuxInfo();
bool GetVideoInfoCalled();
uint32_t GetLastVideoWidth();
uint32_t GetLastVideoHeight();

namespace {
constexpr int32_t WIDTH_1080P = 1920;
constexpr int32_t HEIGHT_1080P = 1080;
}

using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class MvAnalyzerSoLoaderTest : public testing::Test {
protected:
    void SetUp() override { ResetTestMvGlobals(); }
    void TearDown() override {}
};

/**
 * @tc.name: CreateMvAnalyzer_AVC_001
 * @tc.desc: Factory properly initializes and returns an AVC MV analyzer instance via Mock.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, CreateMvAnalyzer_AVC_001, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockMvAnalyzer>(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);
    EXPECT_EQ(analyzer->GetCodecType(), SFDCodecType::AVC);
}

/**
 * @tc.name: CreateMvAnalyzer_HEVC_002
 * @tc.desc: Factory properly initializes and returns an HEVC MV analyzer instance via Mock.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, CreateMvAnalyzer_HEVC_002, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockMvAnalyzer>(SFDCodecType::HEVC);
    ASSERT_NE(analyzer, nullptr);
    EXPECT_EQ(analyzer->GetCodecType(), SFDCodecType::HEVC);
}

/**
 * @tc.name: CreateMvAnalyzer_VVC_003
 * @tc.desc: Factory properly initializes and returns a VVC MV analyzer instance via Mock.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, CreateMvAnalyzer_VVC_003, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockMvAnalyzer>(SFDCodecType::VVC);
    ASSERT_NE(analyzer, nullptr);
    EXPECT_EQ(analyzer->GetCodecType(), SFDCodecType::VVC);
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
 * @tc.name: Loader_InterfaceCall_MockData_006
 * @tc.desc: Ensure that calling interface methods through the loader works correctly with mocks.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, Loader_InterfaceCall_MockData_006, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockMvAnalyzer>(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);

    analyzer->SetVideoInfo(WIDTH_1080P, HEIGHT_1080P);
    StreamAuxInfo auxInfo{};
    analyzer->SetStreamAuxInfo(auxInfo);

    MVStats stats = analyzer->Analyze(nullptr, 0);

    // Expect 100 as hardcoded in our Mock
    EXPECT_EQ(stats.validBlocks, 100); // 100: mock valid blocks
}

/**
 * @tc.name: SetVideoInfo_VerifyParams_007
 * @tc.desc: Verify SetVideoInfo correctly passes width and height to the underlying C API.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, SetVideoInfo_VerifyParams_007, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockMvAnalyzer>(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);

    analyzer->SetVideoInfo(WIDTH_1080P, HEIGHT_1080P);

    EXPECT_TRUE(GetVideoInfoCalled());
    EXPECT_EQ(GetLastVideoWidth(), WIDTH_1080P);
    EXPECT_EQ(GetLastVideoHeight(), HEIGHT_1080P);
}

/**
 * @tc.name: SetStreamAuxInfo_VerifyParams_008
 * @tc.desc: Verify SetStreamAuxInfo correctly passes StreamAuxInfo fields to the underlying C API.
 * @tc.type: FUNC
 */
HWTEST_F(MvAnalyzerSoLoaderTest, SetStreamAuxInfo_VerifyParams_008, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockMvAnalyzer>(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);

    StreamAuxInfo auxInfo{};
    auxInfo.ctuSize = 64;
    auxInfo.directSpatialMvPredFlag = 1;
    auxInfo.direct8x8InferenceFlag = 1;
    auxInfo.frameMbsOnlyFlag = 1;
    analyzer->SetStreamAuxInfo(auxInfo);

    EXPECT_TRUE(GetStreamAuxInfoCalled());
    EXPECT_EQ(GetLastStreamAuxInfo().ctuSize, 64u);
    EXPECT_EQ(GetLastStreamAuxInfo().directSpatialMvPredFlag, 1u);
    EXPECT_EQ(GetLastStreamAuxInfo().direct8x8InferenceFlag, 1u);
    EXPECT_EQ(GetLastStreamAuxInfo().frameMbsOnlyFlag, 1);
}
