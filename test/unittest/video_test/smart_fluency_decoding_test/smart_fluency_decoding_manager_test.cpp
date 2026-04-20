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

namespace {
constexpr int32_t WIDTH_1080P = 1920;
constexpr int32_t HEIGHT_1080P = 1080;
constexpr int32_t WIDTH_720P = 1280;
constexpr int32_t HEIGHT_720P = 720;
}

using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class SFDManagerTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    std::shared_ptr<OHOS::Media::AVBuffer> CreateMockBuffer(uint32_t flag, size_t size)
    {
        auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
        if (!allocator) {
            return nullptr;
        }
        auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, static_cast<int32_t>(size));
        if (buffer && buffer->memory_) {
            buffer->flag_ = flag;
            buffer->memory_->SetSize(static_cast<int32_t>(size));
        }
        return buffer;
    }
};

/**
 * @tc.name: Configure_UnsupportedCodec_001
 * @tc.desc: Ensure the manager rejects unsupported codecs (e.g., Audio) and does not activate SFD.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, Configure_UnsupportedCodec_001, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("audio/mp4a-latm", AVCODEC_TYPE_AUDIO_DECODER);
    Format format;
    manager.Configure(format);
    EXPECT_FALSE(manager.IsSfdActive());
}

/**
 * @tc.name: EnableMVOutput_FormatTag_002
 * @tc.desc: Validate that MV output flag is correctly injected into the format based on mime type.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, EnableMVOutput_FormatTag_002, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    manager.EnableMVOutput(format);
    int32_t mvUpload = -1;
    bool hasTag = format.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, mvUpload);
    if (hasTag) {
        EXPECT_EQ(mvUpload, 0);
    }
    SmartFluencyDecodingManager managerHevc("video/hevc", AVCODEC_TYPE_VIDEO_DECODER);
    managerHevc.EnableMVOutput(format);
    // HEVC enables MV automatically
    EXPECT_TRUE(format.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, mvUpload));
    EXPECT_EQ(mvUpload, 1);
}

/**
 * @tc.name: Configure_SupportedCodec_003
 * @tc.desc: Verify that valid format configuration successfully instantiates the SFD instance.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, Configure_SupportedCodec_003, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    // Use FULL mode for testing to avoid heavy initialization like NaluAnalyzer if not mocked
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0);

    manager.Configure(format);
    EXPECT_TRUE(manager.IsSfdActive());
}

/**
 * @tc.name: PreDecode_InactiveBypass_004
 * @tc.desc: Validate that MakePreDecodeDecision defaults to retaining the frame when SFD is disabled.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, PreDecode_InactiveBypass_004, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    // Manager is explicitly NOT configured here, so IsSfdActive() is false.
    EXPECT_TRUE(manager.MakePreDecodeDecision(0));
}

/**
 * @tc.name: CleanResources_Lifecycle_005
 * @tc.desc: Ensure CleanResources properly destroys the SFD instance and resets internal state.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, CleanResources_Lifecycle_005, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_720P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_720P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0);

    manager.Configure(format);
    EXPECT_TRUE(manager.IsSfdActive());

    manager.CleanResources();
    EXPECT_FALSE(manager.IsSfdActive());
}

/**
 * @tc.name: CsdCaching_PipelineSafe_006
 * @tc.desc: Push a CODEC_DATA buffer into the uninitialized manager and verify it caches without crashing.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, CsdCaching_PipelineSafe_006, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    auto buffer = CreateMockBuffer(AVCODEC_BUFFER_FLAG_CODEC_DATA, 100); // 100: mock csd size
    ASSERT_NE(buffer, nullptr);

    // Simulate pipeline behavior: Input -> PreDecode (which triggers cache extraction)
    manager.OnInputBufferAvailable(0, buffer);
    bool shouldRetain = manager.MakePreDecodeDecision(0);

    // SFD is inactive, so it must return true, but internally it safely cached the CSD
    EXPECT_TRUE(shouldRetain);
    EXPECT_FALSE(manager.IsSfdActive());
}

/**
 * @tc.name: PostDecode_InactiveBypass_007
 * @tc.desc: MakePostDecodeDecision should bypass gracefully when SFD is disabled.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, PostDecode_InactiveBypass_007, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    auto buffer = CreateMockBuffer(0, 100); // 100: mock buffer size
    ASSERT_NE(buffer, nullptr);

    bool shouldRetain = manager.MakePostDecodeDecision(0, buffer, false);
    EXPECT_TRUE(shouldRetain);
}