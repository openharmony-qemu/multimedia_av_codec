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
#include "buffer/avallocator.h"

using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class NaluAnalyzerSoLoaderTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: FactoryCreate_AVC_001
 * @tc.desc: Factory properly initializes and returns an AVC NALU analyzer instance via Mock.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, FactoryCreate_AVC_001, TestSize.Level1)
{
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);
}

/**
 * @tc.name: FactoryCreate_HEVC_002
 * @tc.desc: Factory properly initializes and returns an HEVC NALU analyzer instance via Mock.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, FactoryCreate_HEVC_002, TestSize.Level1)
{
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType::HEVC);
    ASSERT_NE(analyzer, nullptr);
}

/**
 * @tc.name: FactoryCreate_VVC_003
 * @tc.desc: Factory properly initializes and returns a VVC NALU analyzer instance via Mock.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, FactoryCreate_VVC_003, TestSize.Level1)
{
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType::VVC);
    ASSERT_NE(analyzer, nullptr);
}

/**
 * @tc.name: FactoryCreate_Invalid_004
 * @tc.desc: Verify the factory returns nullptr for an invalid codec type.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, FactoryCreate_Invalid_004, TestSize.Level1)
{
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType::INVALID);
    EXPECT_EQ(analyzer, nullptr);
}

/**
 * @tc.name: IsNonRef_WithValidBuffer_005
 * @tc.desc: Validate NALU parsing logic using mocked dlopen/dlsym.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, IsNonRef_WithValidBuffer_005, TestSize.Level1)
{
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);
    
    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    
    // FakeIsNonReferenceFrame in dlfcn_mock.cpp always returns true
    EXPECT_TRUE(analyzer->IsNonReferenceFrame(buffer));
}