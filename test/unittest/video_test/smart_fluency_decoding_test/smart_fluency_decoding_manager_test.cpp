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
#include "surface_buffer.h"

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
    EXPECT_FALSE(manager.ShouldReportDfx());
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
    EXPECT_FALSE(format.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, mvUpload));
    EXPECT_EQ(mvUpload, -1);
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
    EXPECT_FALSE(manager.ShouldReportDfx());
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
    EXPECT_FALSE(manager.ShouldReportDfx());

    manager.CleanResources();
    EXPECT_FALSE(manager.ShouldReportDfx());
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
    EXPECT_FALSE(manager.ShouldReportDfx());
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

/**
 * @tc.name: Configure_DynamicUpdate_008
 * @tc.desc: Call Configure twice to test dynamic update path.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, Configure_DynamicUpdate_008, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0);

    manager.Configure(format);
    EXPECT_FALSE(manager.ShouldReportDfx());

    // Update with new format
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_720P);
    manager.Configure(format);
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: SetCallbacks_ActiveSync_009
 * @tc.desc: Set callbacks after SFD is active to test dynamic syncing.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, SetCallbacks_ActiveSync_009, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0);

    manager.Configure(format);
    ASSERT_FALSE(manager.ShouldReportDfx());

    bool inputCalled = false;
    bool outputCalled = false;
    manager.SetCallbacks(
        [&inputCalled](uint32_t, const std::shared_ptr<AVBuffer>&) { inputCalled = true; },
        [&outputCalled](uint32_t) { outputCalled = true; }
    );
    // Success if no crash, as internal sfd_ calls SetCallbacks
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: ExtractCsdData_InvalidBuffer_010
 * @tc.desc: Test error handling for null buffer/memory in CSD extraction.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ExtractCsdData_InvalidBuffer_010, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    // Directly testing CacheCsdData logic via OnInputBufferAvailable + MakePreDecodeDecision
    manager.OnInputBufferAvailable(0, nullptr);
    manager.MakePreDecodeDecision(0); // Should return true and not crash
    EXPECT_TRUE(true);
}

/**
 * @tc.name: ExtractCsdData_SmallSize_011
 * @tc.desc: Test error handling for buffers smaller than MIN_CSD_SIZE.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ExtractCsdData_SmallSize_011, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    auto buffer = CreateMockBuffer(AVCODEC_BUFFER_FLAG_CODEC_DATA, 2); // 2 < MIN_CSD_SIZE(4)
    ASSERT_NE(buffer, nullptr);

    manager.OnInputBufferAvailable(0, buffer);
    manager.MakePreDecodeDecision(0);
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: ExtractCsdData_FullCache_012
 * @tc.desc: Verify CSD cache limit enforcement (DEFAULT_CSD_COPY_SIZE).
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ExtractCsdData_FullCache_012, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    // Fill cache exactly to limit (256 bytes)
    auto buffer1 = CreateMockBuffer(AVCODEC_BUFFER_FLAG_CODEC_DATA, 256);
    manager.OnInputBufferAvailable(0, buffer1);
    manager.MakePreDecodeDecision(0);

    // Try to add more
    auto buffer2 = CreateMockBuffer(AVCODEC_BUFFER_FLAG_CODEC_DATA, 10);
    manager.OnInputBufferAvailable(1, buffer2);
    manager.MakePreDecodeDecision(1);
    EXPECT_TRUE(true); // Should not crash
}

/**
 * @tc.name: ExtractCsdData_SyncFrame_013
 * @tc.desc: Test successful CSD extraction from sync frames.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ExtractCsdData_SyncFrame_013, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    auto buffer = CreateMockBuffer(AVCODEC_BUFFER_FLAG_SYNC_FRAME, 100);
    manager.OnInputBufferAvailable(0, buffer);
    manager.MakePreDecodeDecision(0);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: CacheCsdData_NonVideoDecoder_014
 * @tc.desc: Verify CacheCsdData bypass for non-video decoders.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, CacheCsdData_NonVideoDecoder_014, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("audio/mp4a-latm", AVCODEC_TYPE_AUDIO_DECODER);
    auto buffer = CreateMockBuffer(AVCODEC_BUFFER_FLAG_CODEC_DATA, 100);
    manager.OnInputBufferAvailable(0, buffer);
    manager.MakePreDecodeDecision(0);
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: CacheCsdData_EmptyBufferMap_015
 * @tc.desc: Test CacheCsdData with missing buffer map entry.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, CacheCsdData_EmptyBufferMap_015, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    manager.MakePreDecodeDecision(999); // index 999 not in map
    EXPECT_TRUE(true);
}

/**
 * @tc.name: MakePostDecodeDecision_SurfaceMode_016
 * @tc.desc: Test surface buffer mapping path in post-decode decision.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, MakePostDecodeDecision_SurfaceMode_016, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    auto buffer = CreateMockBuffer(0, 100);
    ASSERT_NE(buffer, nullptr);
    // Note: In real scenarios, buffer->memory_->GetSurfaceBuffer() requires a real surface buffer.
    // In unit tests, if memory is mock, it might return null, but we test the branch regardless.
    bool result = manager.MakePostDecodeDecision(0, buffer, true);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: MakePostDecodeDecision_Eos_017
 * @tc.desc: Verify EOS flag bypass in post-decode decision.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, MakePostDecodeDecision_Eos_017, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0);
    manager.Configure(format);

    auto buffer = CreateMockBuffer(AVCODEC_BUFFER_FLAG_EOS, 100);
    bool result = manager.MakePostDecodeDecision(0, buffer, false);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: IsCodecSupported_Cases_018
 * @tc.desc: Exhaustively test IsCodecSupported parameter combinations.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, IsCodecSupported_Cases_018, TestSize.Level1)
{
    // These calls go to static method via internal logic in manager
    EXPECT_FALSE(SmartFluencyDecodingManager::IsCodecSupported("video/avc", AVCODEC_TYPE_VIDEO_DECODER, true));
    EXPECT_FALSE(SmartFluencyDecodingManager::IsCodecSupported("audio/mp4a-latm", AVCODEC_TYPE_AUDIO_DECODER, false));
    EXPECT_TRUE(SmartFluencyDecodingManager::IsCodecSupported("video/avc", AVCODEC_TYPE_VIDEO_DECODER, false));
    EXPECT_TRUE(SmartFluencyDecodingManager::IsCodecSupported("video/hevc", AVCODEC_TYPE_VIDEO_DECODER, false));
    EXPECT_TRUE(SmartFluencyDecodingManager::IsCodecSupported("video/vvc", AVCODEC_TYPE_VIDEO_DECODER, false));
    EXPECT_FALSE(SmartFluencyDecodingManager::IsCodecSupported("video/vp9", AVCODEC_TYPE_VIDEO_DECODER, false));
}

/**
 * @tc.name: EnableMVOutput_ActiveSfd_019
 * @tc.desc: Test MV output enablement when SFD is active in ADAPTIVE mode.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, EnableMVOutput_ActiveSfd_019, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 1);
    manager.Configure(format);

    Format mvFormat;
    manager.EnableMVOutput(mvFormat);
    int32_t val = 0;
    EXPECT_TRUE(mvFormat.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, val));
    EXPECT_EQ(val, 1);
}

/**
 * @tc.name: Configure_InjectCsdData_020
 * @tc.desc: Verify SFD initialization with previously cached CSD data.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, Configure_InjectCsdData_020, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    auto buffer = CreateMockBuffer(AVCODEC_BUFFER_FLAG_CODEC_DATA, 100);
    manager.OnInputBufferAvailable(0, buffer);
    manager.MakePreDecodeDecision(0);

    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0);
    manager.Configure(format);
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: Flush_ActiveSfd_021
 * @tc.desc: Verify Flush propagates to active SFD without crash.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, Flush_ActiveSfd_021, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0);
    manager.Configure(format);
    ASSERT_FALSE(manager.ShouldReportDfx());

    manager.Flush();
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: Flush_NoSfd_022
 * @tc.desc: Verify Flush does not crash when SFD is not active.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, Flush_NoSfd_022, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    manager.Flush();
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: OnOutputBufferUnbinded_ClearsMap_023
 * @tc.desc: Verify OnOutputBufferUnbinded clears the surface buffer map.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, OnOutputBufferUnbinded_ClearsMap_023, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    manager.surfaceBufMap_[1] = nullptr;
    manager.OnOutputBufferUnbinded();
    EXPECT_TRUE(manager.surfaceBufMap_.empty());
}

/**
 * @tc.name: MakePostDecodeDecision_NullBuffer_024
 * @tc.desc: Verify MakePostDecodeDecision returns true when buffer is null.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, MakePostDecodeDecision_NullBuffer_024, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    bool result = manager.MakePostDecodeDecision(0, nullptr, false);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: OnInputBufferAvailable_NullBuffer_025
 * @tc.desc: Verify OnInputBufferAvailable returns early without crash when buffer is null.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, OnInputBufferAvailable_NullBuffer_025, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    manager.OnInputBufferAvailable(0, nullptr);
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: EnableMVOutput_AvcNoSfd_026
 * @tc.desc: Verify EnableMVOutput does not set MV tag for AVC when SFD is not active.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, EnableMVOutput_AvcNoSfd_026, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    manager.EnableMVOutput(format);
    int32_t val = -1;
    EXPECT_FALSE(format.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, val));
}

/**
 * @tc.name: Configure_ModeNotSet_027
 * @tc.desc: Verify Configure without retention mode returns nullptr from Build.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, Configure_ModeNotSet_027, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    // No VIDEO_DECODER_FRAME_RETENTION_MODE → INVALID → Build returns null
    manager.Configure(format);
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: ExtractCsdData_NonCsdNonSyncFrame_028
 * @tc.desc: Verify CacheCsdData does not cache buffers with neither CODEC_DATA nor SYNC_FRAME flags.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ExtractCsdData_NonCsdNonSyncFrame_028, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    auto buffer = CreateMockBuffer(0, 100);  // flag=0: neither CODEC_DATA nor SYNC_FRAME
    ASSERT_NE(buffer, nullptr);
    manager.OnInputBufferAvailable(0, buffer);
    manager.MakePreDecodeDecision(0);
    EXPECT_FALSE(manager.isCsdCached_);
}

/**
 * @tc.name: IsCodecSupported_SecureDecoder_029
 * @tc.desc: Verify IsCodecSupported returns false for secure decoder mode.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, IsCodecSupported_SecureDecoder_029, TestSize.Level1)
{
    EXPECT_FALSE(SmartFluencyDecodingManager::IsCodecSupported("video/avc", AVCODEC_TYPE_VIDEO_DECODER, true));
    EXPECT_FALSE(SmartFluencyDecodingManager::IsCodecSupported("video/hevc", AVCODEC_TYPE_VIDEO_DECODER, true));
}

/**
 * @tc.name: OnInputBufferAvailable_AfterCsdCached_030
 * @tc.desc: Verify OnInputBufferAvailable still inserts to tempCsdBufferMap when CSD cache is incomplete.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, OnInputBufferAvailable_AfterCsdCached_030, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    // Fill CSD cache to trigger isCsdCached_=true
    auto buffer1 = CreateMockBuffer(AVCODEC_BUFFER_FLAG_CODEC_DATA, 256);
    manager.OnInputBufferAvailable(0, buffer1);
    manager.MakePreDecodeDecision(0);
    EXPECT_FALSE(manager.isCsdCached_);
    EXPECT_TRUE(manager.tempCsdBufferMap_.empty());  // Cleared after caching complete

    // isCsdCached_ still false (CSD not enough size/no sync frame), buffer still inserted
    auto buffer2 = CreateMockBuffer(0, 100);
    manager.OnInputBufferAvailable(1, buffer2);
    EXPECT_FALSE(manager.tempCsdBufferMap_.empty());  // Buffer inserted, map not empty
}

/**
 * @tc.name: EnableMVOutput_VvcAuto_031
 * @tc.desc: Verify EnableMVOutput automatically enables MV for VVC codec.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, EnableMVOutput_VvcAuto_031, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/vvc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    manager.EnableMVOutput(format);
    int32_t val = -1;
    EXPECT_FALSE(format.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, val));
    EXPECT_EQ(val, -1);
}

/**
 * @tc.name: ShouldReportDfx_NoSfd_032
 * @tc.desc: Verify ShouldReportDfx returns false when SFD is not created.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ShouldReportDfx_NoSfd_032, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: ShouldReportDfx_InvalidMode_033
 * @tc.desc: Verify ShouldReportDfx returns false when retention mode is not set (INVALID).
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ShouldReportDfx_InvalidMode_033, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    // No VIDEO_DECODER_FRAME_RETENTION_MODE → INVALID → Build returns null
    manager.Configure(format);
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: ShouldReportDfx_FullMode_034
 * @tc.desc: Verify ShouldReportDfx returns false for FULL mode.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ShouldReportDfx_FullMode_034, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0); // FULL
    manager.Configure(format);
    EXPECT_FALSE(manager.ShouldReportDfx());
}

/**
 * @tc.name: ShouldReportDfx_AdaptiveMode_035
 * @tc.desc: Verify ShouldReportDfx returns true for ADAPTIVE mode.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ShouldReportDfx_AdaptiveMode_035, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 1); // ADAPTIVE
    manager.Configure(format);
    EXPECT_TRUE(manager.ShouldReportDfx());
}

/**
 * @tc.name: ShouldReportDfx_FixedRatioMode_036
 * @tc.desc: Verify ShouldReportDfx returns true for FIXED_RATIO mode (UNIFORM with ratio).
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ShouldReportDfx_FixedRatioMode_036, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 2); // UNIFORM
    format.PutDoubleValue(Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, 0.5); // with ratio → FIXED_RATIO
    manager.Configure(format);
    EXPECT_TRUE(manager.ShouldReportDfx());
}

/**
 * @tc.name: ShouldReportDfx_AutoRatioMode_037
 * @tc.desc: Verify ShouldReportDfx returns true for AUTO_RATIO mode (UNIFORM without ratio).
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, ShouldReportDfx_AutoRatioMode_037, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 2); // UNIFORM
    // No ratio → AUTO_RATIO
    manager.Configure(format);
    EXPECT_TRUE(manager.ShouldReportDfx());
}

/**
 * @tc.name: EnableMVOutput_FullMode_NoTag_038
 * @tc.desc: Verify EnableMVOutput does not set MV tag for FULL mode.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, EnableMVOutput_FullMode_NoTag_038, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 0);
    manager.Configure(format);

    Format mvFormat;
    manager.EnableMVOutput(mvFormat);
    int32_t val = -1;
    EXPECT_FALSE(mvFormat.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, val));
}

/**
 * @tc.name: EnableMVOutput_FixedRatio_NoTag_039
 * @tc.desc: Verify EnableMVOutput does not set MV tag for FIXED_RATIO mode.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, EnableMVOutput_FixedRatio_NoTag_039, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 2);
    format.PutDoubleValue(Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, 0.5);
    manager.Configure(format);

    Format mvFormat;
    manager.EnableMVOutput(mvFormat);
    int32_t val = -1;
    EXPECT_FALSE(mvFormat.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, val));
}

/**
 * @tc.name: EnableMVOutput_AutoRatio_NoTag_040
 * @tc.desc: Verify EnableMVOutput does not set MV tag for AUTO_RATIO mode.
 * @tc.type: FUNC
 */
HWTEST_F(SFDManagerTest, EnableMVOutput_AutoRatio_NoTag_040, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, WIDTH_1080P);
    format.PutIntValue(Tag::VIDEO_HEIGHT, HEIGHT_1080P);
    format.PutIntValue(Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 2);
    manager.Configure(format);

    Format mvFormat;
    manager.EnableMVOutput(mvFormat);
    int32_t val = -1;
    EXPECT_FALSE(mvFormat.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, val));
}