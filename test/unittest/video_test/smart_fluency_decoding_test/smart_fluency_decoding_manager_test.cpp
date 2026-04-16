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
#include <vector>
#include "smart_fluency_decoding_manager.h"
#include "smart_fluency_decoding.h"
#include "meta/video_types.h"
#include "avcodec_mime_type.h"
#include "avcodec_common.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer.h"

using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class SFDManagerTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: IsCodecSupported_VariousCodecs_001
 * @tc.desc: Correctly identify SFD readiness for multiple defined codec names against versions.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, IsCodecSupported_VariousCodecs_001, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    EXPECT_TRUE(manager.IsCodecSupported("video/avc", AVCODEC_TYPE_VIDEO_DECODER));
    EXPECT_TRUE(manager.IsCodecSupported("video/hevc", AVCODEC_TYPE_VIDEO_DECODER));
    EXPECT_TRUE(manager.IsCodecSupported("video/vvc", AVCODEC_TYPE_VIDEO_DECODER));
    EXPECT_FALSE(manager.IsCodecSupported("video/avc", AVCODEC_TYPE_VIDEO_ENCODER));
    EXPECT_FALSE(manager.IsCodecSupported("audio/mp4a-latm", AVCODEC_TYPE_VIDEO_DECODER));
}

/**
 * @tc.name: EnableMVOutput_FormatTag_002
 * @tc.desc: Test whether EnableMVOutput modifies Format properties correctly.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, EnableMVOutput_FormatTag_002, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    manager.EnableMVOutput(format);
    int32_t mvUpload = 0;
    // By default, sfd_ is null, so for AVC it should not enable unless sfd_ is active.
    // Wait, let's check the logic: 
    // if (sfd_ != nullptr || codecMime_ == HEVC || codecMime_ == VVC)
    EXPECT_FALSE(format.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, mvUpload));

    // For HEVC, it should enable even without sfd_
    SmartFluencyDecodingManager managerHevc("video/hevc", AVCODEC_TYPE_VIDEO_DECODER);
    managerHevc.EnableMVOutput(format);
    EXPECT_TRUE(format.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, mvUpload));
    EXPECT_EQ(mvUpload, 1);
}

/**
 * @tc.name: ExtractCsdData_Functionality_003
 * @tc.desc: Test CSD data extraction from AVBuffer.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ExtractCsdData_Functionality_003, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);

    // Null buffer
    auto csd = manager.ExtractCsdData(nullptr);
    EXPECT_TRUE(csd.empty());

    // Valid CSD buffer
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MEMORY_READ_WRITE);
    auto buffer = AVBuffer::CreateAVBuffer(allocator, 10);
    uint8_t rawData[] = {0, 0, 0, 1, 0x67, 0x42, 0x00, 0x1F, 0x96, 0x54};
    memcpy_s(buffer->memory_->GetAddr(), 10, rawData, 10);
    buffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;

    csd = manager.ExtractCsdData(buffer);
    EXPECT_EQ(csd.size(), 10);
    EXPECT_EQ(csd[4], 0x67);
}

/**
 * @tc.name: ReplayCsdData_Functionality_004
 * @tc.desc: Test CSD data replay to SFD core.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ReplayCsdData_Functionality_004, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    auto sfd = std::make_unique<SmartFluencyDecoding>();

    // Initialize sfd enough to pass isInitialized_ check
    SFDConfig config;
    config.width = 1920;
    config.height = 1080;
    config.codecType = SFDCodecType::AVC;
    config.initMode = RetentionStrategyType::FULL;
    sfd->Initialize(config);

    std::vector<uint8_t> csdData = {0, 0, 0, 1, 0x67, 0x42, 0x00, 0x1F};

    // Case 1: Null sfd - should not crash
    manager.ReplayCsdData(nullptr, csdData);

    // Case 2: Empty data - should not crash, csdBuffer_ remains null
    manager.ReplayCsdData(sfd.get(), {});
    EXPECT_EQ(sfd->csdBuffer_, nullptr);

    // Case 3: Valid data - csdBuffer_ should be set
    manager.ReplayCsdData(sfd.get(), csdData);
    ASSERT_NE(sfd->csdBuffer_, nullptr);
    EXPECT_EQ(sfd->csdBuffer_->flag_, AVCODEC_BUFFER_FLAG_CODEC_DATA);
    EXPECT_EQ(sfd->csdBuffer_->memory_->GetCapacity(), static_cast<int32_t>(csdData.size()));
}
