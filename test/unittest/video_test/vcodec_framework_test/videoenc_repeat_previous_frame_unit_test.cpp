/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include "avcodec_log.h"
#include "codeclist_mock.h"
#include "meta/meta_key.h"
#include "native_avmagic.h"
#include "unittest_utils.h"
#include "videoenc_capi_mock.h"
#include "videoenc_func_test_suit.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
using namespace OHOS::Media;

namespace VFTSUIT {
void TEST_SUIT::SetUpTestCase(void) {}

void TEST_SUIT::CreateByNameWithParam(int32_t param)
{
    std::string codecName = "";
    switch (param) {
        case VCodecTestCode::HW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), true,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        case VCodecTestCode::HW_HEVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), true,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        default:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), true,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
    }
    codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
}

void TEST_SUIT::PrepareSource(int32_t param)
{
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoEnc_->SetOutPath(prefix + fileName);
}

void TEST_SUIT::SetFormatWithParam(int32_t param)
{
    (void)param;
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
}

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, HW_HEVC));

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_001
 * @tc.desc: key repeat previous frame is invalid
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_001, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 0;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_002
 * @tc.desc: key repeat previous frame is invalid
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_002, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = -1;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_003
 * @tc.desc: key repeat previous frame is invalid
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_003, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = INT32_MIN;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_004
 * @tc.desc: key repeat previous frame is invalid
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_004, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 30;
    constexpr int32_t repeatPreviousFrameMaxCount = 0;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_005
 * @tc.desc: repeat the previous frame all the time
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_005, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 30;
    constexpr int32_t repeatPreviousFrameMaxCount = -1;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
#ifdef HMOS_TEST
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 2 - 7;
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
#endif // HMOS_TEST
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_006
 * @tc.desc: repeat the previous frame all the time
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_006, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 30;
    constexpr int32_t repeatPreviousFrameMaxCount = INT32_MIN;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
#ifdef HMOS_TEST
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 2 - 7;
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
#endif // HMOS_TEST
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_007
 * @tc.desc: repeat the previous frame one times
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_007, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 30;
    constexpr int32_t repeatPreviousFrameMaxCount = 1;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 2 - 7;
    int32_t frameOutputCountMax = (videoEnc_->frameInputCount_ - 1) * 2 + 7;
    EXPECT_LE(videoEnc_->frameOutputCount_, frameOutputCountMax);
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_008
 * @tc.desc: repeat the previous frame two times
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_008, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 20;
    constexpr int32_t repeatPreviousFrameMaxCount = 2;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 3 - 7;
    int32_t frameOutputCountMax = (videoEnc_->frameInputCount_ - 1) * 3 + 7;
    EXPECT_LE(videoEnc_->frameOutputCount_, frameOutputCountMax);
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_009
 * @tc.desc: repeat the previous frame three times
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_009, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 10;
    constexpr int32_t repeatPreviousFrameMaxCount = 3;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 4 - 7;
    int32_t frameOutputCountMax = (videoEnc_->frameInputCount_ - 1) * 4 + 7;
    EXPECT_LE(videoEnc_->frameOutputCount_, frameOutputCountMax);
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_012
 * @tc.desc: buffer mode use repeat the previous frame
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_012, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 10;
    constexpr int32_t repeatPreviousFrameMaxCount = 3;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(videoEnc_->frameInputCount_, videoEnc_->frameOutputCount_);
}
#ifdef HMOS_TEST
/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_010
 * @tc.desc: repeat the previous frame two times
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_010, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 20;
    constexpr int32_t repeatPreviousFrameMaxCount = 10;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 3 - 27;
    int32_t frameOutputCountMax = (videoEnc_->frameInputCount_ - 1) * 3 + 27;
    EXPECT_LE(videoEnc_->frameOutputCount_, frameOutputCountMax);
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_011
 * @tc.desc: repeat the previous frame two times
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_011, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 20;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 3 - 27;
    int32_t frameOutputCountMax = (videoEnc_->frameInputCount_ - 1) * 3 + 27;
    EXPECT_LE(videoEnc_->frameOutputCount_, frameOutputCountMax);
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
}
#endif // HMOS_TEST
} // namespace

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        std::cout << argv[i] << std::endl;
        if (strcmp(argv[i], "--need_dump") == 0) {
            VideoEncAsyncSample::needDump_ = true;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}