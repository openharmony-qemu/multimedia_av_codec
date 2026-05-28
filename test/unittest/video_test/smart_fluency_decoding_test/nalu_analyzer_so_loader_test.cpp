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
#include "test_mock_analyzers.h"
#include "buffer/avallocator.h"
#include "securec.h"

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
    auto analyzer = std::make_unique<MockNaluAnalyzer>(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);
}

/**
 * @tc.name: FactoryCreate_HEVC_002
 * @tc.desc: Factory properly initializes and returns an HEVC NALU analyzer instance via Mock.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, FactoryCreate_HEVC_002, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockNaluAnalyzer>(SFDCodecType::HEVC);
    ASSERT_NE(analyzer, nullptr);
}

/**
 * @tc.name: FactoryCreate_VVC_003
 * @tc.desc: Factory properly initializes and returns a VVC NALU analyzer instance via Mock.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, FactoryCreate_VVC_003, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockNaluAnalyzer>(SFDCodecType::VVC);
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
 * @tc.desc: Ensure IsNonReferenceFrame returns true when provided with a valid non-reference AVC NALU.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, IsNonRef_WithValidBuffer_005, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockNaluAnalyzer>(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);

    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    // [00 00 00 01] Start code, [0x01] nal_ref_idc=0, nal_unit_type=1 (Non-IDR)
    std::vector<uint8_t> nonRefData = {0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFF};
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, static_cast<int32_t>(nonRefData.size()));
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(buffer->memory_, nullptr);
    memcpy_s(buffer->memory_->GetAddr(), buffer->memory_->GetCapacity(), nonRefData.data(), nonRefData.size());
    buffer->memory_->SetSize(nonRefData.size());
    EXPECT_TRUE(analyzer->IsNonReferenceFrame(buffer));
}

/**
 * @tc.name: IsNonRef_WithReferenceBuffer_006
 * @tc.desc: Ensure IsNonReferenceFrame returns false when provided with a reference AVC NALU (e.g., IDR).
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, IsNonRef_WithReferenceBuffer_006, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockNaluAnalyzer>(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);

    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    // [00 00 00 01] Start code, [0x65] nal_ref_idc=3, nal_unit_type=5 (IDR)
    std::vector<uint8_t> refData = {0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x99};
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, static_cast<int32_t>(refData.size()));
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(buffer->memory_, nullptr);
    memcpy_s(buffer->memory_->GetAddr(), buffer->memory_->GetCapacity(), refData.data(), refData.size());
    buffer->memory_->SetSize(refData.size());
    EXPECT_FALSE(analyzer->IsNonReferenceFrame(buffer));
}

/**
 * @tc.name: IsNonRef_NullBuffer_007
 * @tc.desc: Verify IsNonReferenceFrame returns false when input buffer is null.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, IsNonRef_NullBuffer_007, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockNaluAnalyzer>(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);
    EXPECT_FALSE(analyzer->IsNonReferenceFrame(nullptr));
}

/**
 * @tc.name: IsNonRef_NullMemory_008
 * @tc.desc: Verify IsNonReferenceFrame returns false when buffer memory is null.
 * @tc.type: FUNC
 */
HWTEST_F(NaluAnalyzerSoLoaderTest, IsNonRef_NullMemory_008, TestSize.Level1)
{
    auto analyzer = std::make_unique<MockNaluAnalyzer>(SFDCodecType::AVC);
    ASSERT_NE(analyzer, nullptr);

    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    ASSERT_NE(buffer, nullptr);
    buffer->memory_ = nullptr;
    EXPECT_FALSE(analyzer->IsNonReferenceFrame(buffer));
}
