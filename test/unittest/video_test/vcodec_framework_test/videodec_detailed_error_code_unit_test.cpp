/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <gtest/hwext/gtest-multithread.h>
#include "meta/meta_key.h"
#include "unittest_utils.h"
#include "videodec_func_test_suit.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
using namespace testing::ext;
using namespace testing::mt;

namespace VFTSUIT {
enum class Scenario : int32_t {
    UNSUPPORT_CODEC_SPECIFICATION,
    ILLEGAL_PARAMETER_SETS,
    MISSING_PARAMETER_SETS,
}
void TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), false,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
}

void TEST_SUIT::CreateByNameWithParam(std::string_view param)
{
    std::string codecName = "";
    if (param == CodecMimeType::VIDEO_AVC) {
        capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
    } else if (param == CodecMimeType::VIDEO_HEVC) {
        capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
    } else {
        capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
    }
    codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
}

void TEST_SUIT::PrepareSource(int32_t param, std::string sourcePath)
{
    if (param == VCodecTestCode::HW_HEVC) {
        videoDec_->SetSourceType(false);
    }
    videoDec_->testParam_ = param;
    std::cout << "SourcePath: " << sourcePath << std::endl;
    videoDec_->SetSource(sourcePath);
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoDec_->SetOutPath(prefix + fileName);
}

void SetDetailedErrorCodeParmater(std::shared_ptr<VDecCallbackTestExt> vdecCallbackExt,
                                  std::shared_ptr<VDecCallbackTest> vdecCallback,
                                  bool isAVBufferMode,
                                  int32_t scenario)
{
    auto& target = isAVBufferMode ? vdecCallbackExt->detailedErrorCode_ : vdecCallback->detailedErrorCode_;
    target.verification_ = false;

    switch (scenario) {
        case Scenario::UNSUPPORT_CODEC_SPECIFICATION:
            target.unsupportedSpecification_ = true;
            break;
        case Scenario::ILLEGAL_PARAMETER_SETS:
            target.illegalParam_ = true;
            break;
        case Scenario::MISSING_PARAMETER_SETS:
            target.missingParam_ = true;
            break;
        default: 
            break;
    }
}

/**
 * @tc.name: VideoDecoder_XPS_Width_001
 * @tc.desc: exceeding the maximum xps frame width
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_Width_001, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 1280;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::UNSUPPORT_CODEC_SPECIFICATION);
    CreateByNameWithParam(CodecMimeType::VIDEO_AVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_AVC, "/data/test/media/8160_720_avcc.h264");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_Width_002
 * @tc.desc: exceeding the minimum xps frame width
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_Width_002, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 1280;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::UNSUPPORT_CODEC_SPECIFICATION);
    CreateByNameWithParam(CodecMimeType::VIDEO_AVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_AVC, "/data/test/media/100_720_avcc.h264");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_Height_001
 * @tc.desc: exceeding the maximum xps frame height
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_Height_001, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 1280;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::UNSUPPORT_CODEC_SPECIFICATION);
    CreateByNameWithParam(CodecMimeType::VIDEO_AVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_AVC, "/data/test/media/720_8160_avcc.h264");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_Height_002
 * @tc.desc: exceeding the minimum xps frame height
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_Height_002, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 1280;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::UNSUPPORT_CODEC_SPECIFICATION);
    CreateByNameWithParam(CodecMimeType::VIDEO_AVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_AVC, "/data/test/media/720_100_avcc.h264");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_BitDepth_001
 * @tc.desc: invalid bit depth
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_BitDepth_001, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 1280;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::UNSUPPORT_CODEC_SPECIFICATION);
    CreateByNameWithParam(CodecMimeType::VIDEO_HEVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_HEVC, "/data/test/media/720_1280_25_12bit_avcc.h265");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_Chroma_Format_001
 * @tc.desc: xps frame chroma format is yuv400
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_Chroma_Format_001, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 1280;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::UNSUPPORT_CODEC_SPECIFICATION);
    CreateByNameWithParam(CodecMimeType::VIDEO_AVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_AVC, "/data/test/media/720_1280_yuv400p_avcc.h264");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_Chroma_Format_002
 * @tc.desc: xps frame chroma format is yuv422
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_Chroma_Format_002, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 1280;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::UNSUPPORT_CODEC_SPECIFICATION);
    CreateByNameWithParam(CodecMimeType::VIDEO_AVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_AVC, "/data/test/media/720_1280_yuv422p_avcc.h264");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_Chroma_Format_003
 * @tc.desc: xps frame chroma format is yuv444
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_Chroma_Format_003, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 1280;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::UNSUPPORT_CODEC_SPECIFICATION);
    CreateByNameWithParam(CodecMimeType::VIDEO_AVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_AVC, "/data/test/media/720_1280_yuv444p_avcc.h264");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_AVC_10Bit_001
 * @tc.desc: 264 bitstream xps frame bit depth is 10bit
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_AVC_10Bit_001, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 1280;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::UNSUPPORT_CODEC_SPECIFICATION);
    CreateByNameWithParam(CodecMimeType::VIDEO_AVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_AVC, "/data/test/media/720_1280_10bit_avcc.h264");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_MBAFF_001
 * @tc.desc: xps frame is mbaff
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_MBAFF_001, TestSize.Level1)
{
    constexpr int32_t width = 1920;
    constexpr int32_t height = 1080;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::UNSUPPORT_CODEC_SPECIFICATION);
    CreateByNameWithParam(CodecMimeType::VIDEO_AVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_AVC, "/data/test/media/1920_1080_mbaff_avcc.h264");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_Invalid_001
 * @tc.desc: xps frame data invalid
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_Invalid_001, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 576;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::ILLEGAL_PARAMETER_SETS);
    CreateByNameWithParam(CodecMimeType::VIDEO_HEVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_HEVC, "/data/test/media/720_576_xps_invalid_avcc.h265");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_Not_Exist_001
 * @tc.desc: xps frame does not exist
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_Not_Exist_001, TestSize.Level1)
{
    constexpr int32_t width = 270;
    constexpr int32_t height = 576;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::MISSING_PARAMETER_SETS);
    CreateByNameWithParam(CodecMimeType::VIDEO_AVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_AVC, "/data/test/media/270_576_no_xps_avcc.h264");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}

/**
 * @tc.name: VideoDecoder_XPS_Not_Exist_002
 * @tc.desc: xps frame does not exist
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_XPS_Not_Exist_002, TestSize.Level1)
{
    constexpr int32_t width = 720;
    constexpr int32_t height = 576;
    videoDec_->detailedError_ = true;
    SetDetailedErrorCodeParmater(vdecCallbackExt_, vdecCallback_, true, Scenario::MISSING_PARAMETER_SETS);
    CreateByNameWithParam(CodecMimeType::VIDEO_HEVC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    PrepareSource(VCodecTestCode::HW_HEVC, "/data/test/media/720_576_no_xps_avcc.h265");
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    ASSERT_EQ(true, vdecCallbackExt_->detailedErrorCode_.verification_);
}
} // namespace

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        cout << argv[i] << endl;
        if (strcmp(argv[i], "--need_dump") == 0) {
            VideoDecSample::needDump_ = true;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}