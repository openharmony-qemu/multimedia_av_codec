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
#include <unistd.h>
#include <vector>
#include "meta/meta_key.h"
#include "unittest_utils.h"
#include "vdec_sample.h"
#include "i_avcodec_service.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;

namespace {
class TEST_SUIT : public testing::TestWithParam<int32_t> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    bool CreateVideoCodecByName(const std::string &decName);
    bool CreateVideoCodecByMime(const std::string &decMime);
    void CreateByNameWithParam(int32_t param);
    void SetFormatWithParam(int32_t param);
    void PrepareSource(int32_t param);
protected:
    std::shared_ptr<CodecListMock> capability_ = nullptr;
    std::shared_ptr<VideoDecSample> videoDec_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    std::shared_ptr<VDecCallbackTest> vdecCallback_ = nullptr;
    std::shared_ptr<VDecCallbackTestExt> vdecCallbackExt_ = nullptr;
};

void TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), false,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
}

void TEST_SUIT::TearDownTestCase(void) {}

void TEST_SUIT::SetUp(void)
{
    std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
    vdecCallback_ = std::make_shared<VDecCallbackTest>(vdecSignal);
    ASSERT_NE(nullptr, vdecCallback_);

    vdecCallbackExt_ = std::make_shared<VDecCallbackTestExt>(vdecSignal);
    ASSERT_NE(nullptr, vdecCallbackExt_);

    videoDec_ = std::make_shared<VideoDecSample>(vdecSignal);
    ASSERT_NE(nullptr, videoDec_);

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);
}

void TEST_SUIT::TearDown(void)
{
    if (format_ != nullptr) {
        format_->Destroy();
    }
    videoDec_ = nullptr;
}

bool TEST_SUIT::CreateVideoCodecByMime(const std::string &decMime)
{
    if (videoDec_->CreateVideoDecMockByMime(decMime) == false || videoDec_->SetCallback(vdecCallback_) != AV_ERR_OK) {
        return false;
    }
    return true;
}

bool TEST_SUIT::CreateVideoCodecByName(const std::string &decName)
{
    if (videoDec_->isAVBufferMode_) {
        if (videoDec_->CreateVideoDecMockByName(decName) == false ||
            videoDec_->SetCallback(vdecCallbackExt_) != AV_ERR_OK) {
            return false;
        }
    } else {
        if (videoDec_->CreateVideoDecMockByName(decName) == false ||
            videoDec_->SetCallback(vdecCallback_) != AV_ERR_OK) {
            return false;
        }
    }
    return true;
}

void TEST_SUIT::CreateByNameWithParam(int32_t param)
{
    std::string codecName = "";
    switch (param) {
        case VCodecTestCode::SW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
        case VCodecTestCode::HW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        case VCodecTestCode::HW_HEVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        case VCodecTestCode::HW_HDR:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        default:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
    }
    codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
}

void TEST_SUIT::PrepareSource(int32_t param)
{
    std::string sourcePath = decSourcePathMap_.at(param);
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

void TEST_SUIT::SetFormatWithParam(int32_t param)
{
    (void)param;
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
}

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, HW_HEVC, HW_HDR));

void SuspendFreeze()
{
    pid_t pid = getpid();
    const std::vector<pid_t> pidList(pid);
    auto ret = AVCodecServivceFactory::GetInstance().SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

void SuspendActivate()
{
    pid_t pid = getpid();
    const std::vector<pid_t> pidList(pid);
    auto ret = AVCodecServivceFactory::GetInstance().SuspendActivate(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

void SuspendActivateAll()
{
    pid_t pid = getpid();
    const std::vector<pid_t> pidList(pid);
    auto ret = AVCodecServivceFactory::GetInstance().SuspendActivateAll(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_001
 * @tc.desc: decoder is Initialized and freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    SuspendFreeze();
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_002
 * @tc.desc: decoder is Configured and freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    SuspendFreeze();
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_003
 * @tc.desc: decoder is Prepared and freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_003, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
        SuspendFreeze();
    }
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_004
 * @tc.desc: decoder is Executing and freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    videoDec_->isKeepExecuting_ = true;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    SuspendFreeze();
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_005
 * @tc.desc: decoder is Flush and freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    SuspendFreeze();
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_006
 * @tc.desc: decoder is EOS and freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_006, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->WaitForEos());
    SuspendFreeze();
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_001
 * @tc.desc: activate process of freeze and decoder is Initialized
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    SuspendFreeze();
    SuspendActivate();
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_002
 * @tc.desc: activate process of freeze and decoder is Configured
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    SuspendFreeze();
    SuspendActivate();
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_003
 * @tc.desc: activate process of freeze and decoder is Prepared
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_003, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
        SuspendFreeze();
        SuspendActivate();
    }
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_004
 * @tc.desc: activate process of freeze and decoder is Executing
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    videoDec_->isKeepExecuting_ = true;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    SuspendFreeze();
    SuspendActivate();
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_005
 * @tc.desc: activate process of freeze and decoder is Flush
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    SuspendFreeze();
    SuspendActivate();
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_006
 * @tc.desc: activate process of freeze and decoder is EOS
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_006, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->WaitForEos());
    SuspendFreeze();
    SuspendActivate();
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_All_001
 * @tc.desc: activate all process of freeze and decoder is Initialized
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    SuspendFreeze();
    SuspendActivateAll();
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_All_002
 * @tc.desc: activate all process of freeze and decoder is Configured
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_All_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    SuspendFreeze();
    SuspendActivateAll();
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_All_003
 * @tc.desc: activate all process of freeze and decoder is Prepared
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_All_003, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
        SuspendFreeze();
        SuspendActivateAll();
    }
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_All_004
 * @tc.desc: activate all process of freeze and decoder is Executing
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_All_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    videoDec_->isKeepExecuting_ = true;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    SuspendFreeze();
    SuspendActivateAll();
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_All_005
 * @tc.desc: activate all process of freeze and decoder is Flush
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_All_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    SuspendFreeze();
    SuspendActivateAll();
}

/**
 * @tc.name: VideoDecoder_Hardware_Activate_All_006
 * @tc.desc: activate all process of freeze and decoder is EOS
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Activate_All_006, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->WaitForEos());
    SuspendFreeze();
    SuspendActivateAll();
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