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