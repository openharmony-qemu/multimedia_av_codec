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
#include <thread>
#include "avcodec_suspend.h"
#include "meta/meta_key.h"
#include "unittest_utils.h"
#include "vdec_sample.h"

#define TEST_SUIT VideoHDecoderMemoryRecyleTest

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;

namespace {
const std::vector<pid_t> invalidPidList = {INT_MIN, INT_MAX};
constexpr int32_t DEFAULT_PROCESS_COUNT = 3;
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
    void CreateExecutingDecoder();
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

void TEST_SUIT::CreateExecutingDecoder()
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    videoDec_->isKeepExecuting_ = true;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, HW_HEVC));

void SuspendFreeze()
{
    pid_t pid = getpid();
    std::vector<pid_t> pidList = {pid};
    auto ret = AVCodecSuspend::SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

void SuspendActive()
{
    pid_t pid = getpid();
    std::vector<pid_t> pidList = {pid};
    auto ret = AVCodecSuspend::SuspendActive(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

void SuspendActiveAll()
{
    auto ret = AVCodecSuspend::SuspendActiveAll();
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

void CreateMultiHardwareDecoder(std::vector<int>& pidList, int32_t testCode)
{
    for (int32_t i = 0; i < DEFAULT_PROCESS_COUNT; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            const char* arg = nullptr;
            switch (testCode) {
                case HW_AVC:
                    arg = "--create_multi_avc_dec";
                    break;
                case HW_HEVC:
                    arg = "--create_multi_hevc_dec";
                    break;
                default:
                    arg = "--create_multi_avc_dec";
                    break;
            }

            execl("/data/test/videodec_hdecoder_memrecycle_unit_test",
                "videodec_hdecoder_memrecycle_unit_test",
                arg,
                nullptr);
            std::cerr << "execl failed!" << std::endl;
            exit(1);
        } else if (pid > 0) {
            pidList.emplace_back(pid);
        } if (pid < 0) {
            std::cerr << "fork process failed!" << std::endl;
            exit(1);
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // 3000ms
}

void DestroyMultiHardwareDecoder(const std::vector<int> pidList)
{
    for (int32_t i = 0; i < DEFAULT_PROCESS_COUNT; i++) {
        kill(pidList[i], SIGTERM);
        wait(nullptr);
    }
}

/**
 * @tc.name: Create_Hardware_Avc_Decoder
 * @tc.desc: create a executing hadware avc decoder
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Create_Hardware_Avc_Decoder, TestSize.Level1)
{
    CreateByNameWithParam(VCodecTestCode::HW_AVC);
    SetFormatWithParam(VCodecTestCode::HW_AVC);
    PrepareSource(VCodecTestCode::HW_AVC);
    videoDec_->isKeepExecuting_ = true;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: Create_Hardware_Hevc_Decoder
 * @tc.desc: create a executing hadware hevc decoder
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Create_Hardware_Hevc_Decoder, TestSize.Level1)
{
    CreateByNameWithParam(VCodecTestCode::HW_HEVC);
    SetFormatWithParam(VCodecTestCode::HW_HEVC);
    PrepareSource(VCodecTestCode::HW_HEVC);
    videoDec_->isKeepExecuting_ = true;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
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
 * @tc.name: VideoDecoder_Hardware_Freeze_004
 * @tc.desc: decoder is Flush and freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_004, TestSize.Level1)
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
 * @tc.name: VideoDecoder_Hardware_Freeze_005
 * @tc.desc: decoder is EOS and freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    SuspendFreeze();
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_006
 * @tc.desc: 1.decoder is EOS
 *           2.pidList contains int_min
 *           3.freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_006, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    auto ret = AVCodecSuspend::SuspendFreeze({INT_MIN});
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_007
 * @tc.desc: 1.decoder is EOS
 *           2.pidList contains int_max
 *           3.freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_007, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    auto ret = AVCodecSuspend::SuspendFreeze({INT_MAX});
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_008
 * @tc.desc: 1.decoder is EOS
 *           2.pidList contains int_min and pid of the current process
 *           3.freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_008, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    auto ret = AVCodecSuspend::SuspendFreeze({INT_MIN, getpid()});
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_009
 * @tc.desc: 1.decoder is Flush
 *           2.freeze process
 *           3.pidList is empty
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Freeze_009, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    auto ret = AVCodecSuspend::SuspendFreeze({});
    ASSERT_EQ(AVCS_ERR_INPUT_DATA_ERROR, ret);
}

/**
 * @tc.name: VideoDecoder_Hardware_MultiProcess_Freeze_001
 * @tc.desc: decoder is executing and freeze all process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_MultiProcess_Freeze_001, TestSize.Level1)
{
    std::vector<pid_t> pidList;
    CreateMultiHardwareDecoder(pidList, GetParam());
    std::vector<pid_t> freezePidList = pidList.empty() ? invalidPidList : pidList;
    auto ret = AVCodecSuspend::SuspendFreeze(freezePidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    DestroyMultiHardwareDecoder(pidList);
}

/**
 * @tc.name: VideoDecoder_Hardware_MultiProcess_Freeze_002
 * @tc.desc: decoder is executing and freeze one of process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_MultiProcess_Freeze_002, TestSize.Level1)
{
    std::vector<pid_t> pidList;
    CreateMultiHardwareDecoder(pidList, GetParam());
    std::vector<pid_t> freezePidList = pidList.empty() ? invalidPidList : std::vector<pid_t>{pidList[0]};
    auto ret = AVCodecSuspend::SuspendFreeze(freezePidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    DestroyMultiHardwareDecoder(pidList);
}

/**
 * @tc.name: VideoDecoder_Hardware_MultiProcess_Freeze_003
 * @tc.desc: decoder is executing and freeze one of process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_MultiProcess_Freeze_003, TestSize.Level1)
{
    std::vector<pid_t> pidList;
    CreateMultiHardwareDecoder(pidList, GetParam());
    std::vector<pid_t> freezePidList = pidList.empty() ? invalidPidList : std::vector<pid_t>{INT_MIN, pidList[0]};
    auto ret = AVCodecSuspend::SuspendFreeze(freezePidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    DestroyMultiHardwareDecoder(pidList);
}

/**
 * @tc.name: VideoDecoder_Hardware_MultiProcess_Freeze_004
 * @tc.desc: decoder is executing and freeze all process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_MultiProcess_Freeze_004, TestSize.Level1)
{
    std::vector<pid_t> pidList;
    CreateMultiHardwareDecoder(pidList, GetParam());
    std::vector<pid_t> freezePidList = pidList;
    freezePidList.emplace_back(INT_MIN);
    auto ret = AVCodecSuspend::SuspendFreeze(freezePidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    DestroyMultiHardwareDecoder(pidList);
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_001
 * @tc.desc: active process of freeze and decoder is Initialized
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    SuspendFreeze();
    SuspendActive();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_002
 * @tc.desc: active process of freeze and decoder is Configured
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    SuspendFreeze();
    SuspendActive();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_004
 * @tc.desc: active process of freeze and decoder is Executing
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_004, TestSize.Level1)
{
    std::thread runDecoder(&TEST_SUIT::CreateExecutingDecoder, this);
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 300ms
    SuspendFreeze();
    SuspendActive();
    runDecoder.join();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_005
 * @tc.desc: active process of freeze and decoder is Flush
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    SuspendFreeze();
    SuspendActive();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_006
 * @tc.desc: active process of freeze and decoder is EOS
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_006, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    SuspendActive();
    SuspendFreeze();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_007
 * @tc.desc: 1.decoder is EOS
 *           2.pidList contains int_min
 *           3.activate process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_007, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    auto ret = AVCodecSuspend::SuspendFreeze({INT_MIN});
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActive({INT_MIN});
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_008
 * @tc.desc: 1.decoder is EOS
 *           2.pidList contains int_max
 *           3.activate process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_008, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    auto ret = AVCodecSuspend::SuspendFreeze({INT_MAX});
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActive({INT_MAX});
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_009
 * @tc.desc: 1.decoder is EOS
 *           2.pidList contains int_min and pid of the current process
 *           3.activate process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_009, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    auto ret = AVCodecSuspend::SuspendFreeze({INT_MIN, getpid()});
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActive({INT_MIN, getpid()});
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_010
 * @tc.desc: 1.decoder is Flush
 *           2.activate process
 *           3.pidList is empty
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_010, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    auto ret = AVCodecSuspend::SuspendActive({});
    ASSERT_EQ(AVCS_ERR_INPUT_DATA_ERROR, ret);
}

/**
 * @tc.name: VideoDecoder_Hardware_MultiProcess_Active_001
 * @tc.desc: decoder is executing and active all process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_MultiProcess_Active_001, TestSize.Level1)
{
    std::vector<pid_t> pidList;
    CreateMultiHardwareDecoder(pidList, GetParam());
    std::vector<pid_t> freezePidList = pidList.empty() ? invalidPidList : pidList;
    auto ret = AVCodecSuspend::SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActive(freezePidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    DestroyMultiHardwareDecoder(pidList);
}

/**
 * @tc.name: VideoDecoder_Hardware_MultiProcess_Active_002
 * @tc.desc: decoder is executing and active one of process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_MultiProcess_Active_002, TestSize.Level1)
{
    std::vector<pid_t> pidList;
    CreateMultiHardwareDecoder(pidList, GetParam());
    std::vector<pid_t> freezePidList = pidList.empty() ? invalidPidList : std::vector<pid_t>{pidList[0]};
    auto ret = AVCodecSuspend::SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActive(freezePidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    DestroyMultiHardwareDecoder(pidList);
}

/**
 * @tc.name: VideoDecoder_Hardware_MultiProcess_Active_003
 * @tc.desc: decoder is executing and active one of process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_MultiProcess_Active_003, TestSize.Level1)
{
    std::vector<pid_t> pidList;
    CreateMultiHardwareDecoder(pidList, GetParam());
    std::vector<pid_t> freezePidList = pidList.empty() ? invalidPidList : std::vector<pid_t>{INT_MIN, pidList[0]};
    auto ret = AVCodecSuspend::SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActive(freezePidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    DestroyMultiHardwareDecoder(pidList);
}

/**
 * @tc.name: VideoDecoder_Hardware_MultiProcess_Active_004
 * @tc.desc: decoder is executing and active all process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_MultiProcess_Active_004, TestSize.Level1)
{
    std::vector<pid_t> pidList;
    CreateMultiHardwareDecoder(pidList, GetParam());
    std::vector<pid_t> freezePidList = pidList;
    freezePidList.emplace_back(INT_MIN);
    auto ret = AVCodecSuspend::SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActive(freezePidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    DestroyMultiHardwareDecoder(pidList);
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_All_001
 * @tc.desc: active all process of freeze and decoder is Initialized
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_All_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    SuspendFreeze();
    SuspendActiveAll();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_All_002
 * @tc.desc: active all process of freeze and decoder is Configured
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_All_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    SuspendFreeze();
    SuspendActiveAll();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_All_004
 * @tc.desc: active all process of freeze and decoder is Executing
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_All_004, TestSize.Level1)
{
    std::thread runDecoder(&TEST_SUIT::CreateExecutingDecoder, this);
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 300ms
    SuspendFreeze();
    SuspendActiveAll();
    runDecoder.join();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_All_005
 * @tc.desc: active all process of freeze and decoder is Flush
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_All_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    SuspendFreeze();
    SuspendActiveAll();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_All_006
 * @tc.desc: active all process of freeze and decoder is EOS
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Active_All_006, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    SuspendFreeze();
    SuspendActiveAll();
}

/**
 * @tc.name: VideoDecoder_Hardware_MultiProcess_Active_All_001
 * @tc.desc: decoder is executing and active all process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_MultiProcess_Active_All_001, TestSize.Level1)
{
    std::vector<pid_t> pidList;
    CreateMultiHardwareDecoder(pidList, GetParam());
    std::vector<pid_t> freezePidList = pidList.empty() ? invalidPidList : pidList;
    auto ret = AVCodecSuspend::SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActiveAll();
    ASSERT_EQ(AVCS_ERR_OK, ret);
    DestroyMultiHardwareDecoder(pidList);
}

/**
 * @tc.name: VideoDecoder_Hardware_MultiProcess_Active_All_002
 * @tc.desc: decoder is executing and active one of process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_MultiProcess_Active_All_002, TestSize.Level1)
{
    std::vector<pid_t> pidList;
    CreateMultiHardwareDecoder(pidList, GetParam());
    std::vector<pid_t> freezePidList = pidList.empty() ? invalidPidList : std::vector<pid_t>{pidList[0]};
    auto ret = AVCodecSuspend::SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActiveAll();
    ASSERT_EQ(AVCS_ERR_OK, ret);
    DestroyMultiHardwareDecoder(pidList);
}

/**
 * @tc.name: VideoDecoder_Hardware_Memory_Recycle_007
 * @tc.desc: unordered memory recycle function invocation
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Memory_Recycle_007, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    SuspendFreeze();
    SuspendFreeze();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    SuspendFreeze();
    SuspendActiveAll();
    SuspendActive();
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    SuspendFreeze();
    SuspendActive();
    SuspendFreeze();
    SuspendActiveAll();
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    SuspendActiveAll();
    SuspendActive();
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    SuspendFreeze();
    SuspendActive();
}

/**
 * @tc.name: VideoDecoder_Hardware_Memory_Recycle_008
 * @tc.desc: unordered memory recycle function invocation
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Hardware_Memory_Recycle_008, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    SuspendFreeze();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    SuspendFreeze();
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    SuspendActive();
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    SuspendFreeze();
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
    SuspendActiveAll();
    SuspendActive();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    SuspendActiveAll();
    EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
    SuspendFreeze();
    SuspendActive();
}

#ifdef HMOS_TEST
/**
 * @tc.name: VideoDecoder_Hardware_Freeze_003
 * @tc.desc: decoder is Prepared and freeze process
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Hardware_Freeze_003, TestSize.Level1)
{
    CreateByNameWithParam(VCodecTestCode::HW_HDR);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(VCodecTestCode::HW_HDR);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
    SuspendFreeze();
}

/**
 * @tc.name: VideoDecoder_Hardware_Freeze_010
 * @tc.desc: 1.decoder called post processing
 *           2.decoder is executing
 *           3.freeze process
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Hardware_Freeze_010, TestSize.Level1)
{
    CreateByNameWithParam(VCodecTestCode::HW_HDR);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(VCodecTestCode::HW_HDR);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    videoDec_->isKeepExecuting_ = true;

    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AVCS_ERR_OK, videoDec_->Start());
    SuspendFreeze();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_All_003
 * @tc.desc: active all process of freeze and decoder is Prepared
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Hardware_Active_All_003, TestSize.Level1)
{
    CreateByNameWithParam(VCodecTestCode::HW_HDR);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(VCodecTestCode::HW_HDR);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
    SuspendFreeze();
    SuspendActiveAll();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_All_007
 * @tc.desc: 1.decoder called post processing
 *           2.decoder is executing
 *           3.active all process
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Hardware_Active_All_007, TestSize.Level1)
{
    CreateByNameWithParam(VCodecTestCode::HW_HDR);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(VCodecTestCode::HW_HDR);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    videoDec_->isKeepExecuting_ = true;

    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AVCS_ERR_OK, videoDec_->Start());
    SuspendFreeze();
    SuspendActiveAll();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_003
 * @tc.desc: active process of freeze and decoder is Prepared
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Hardware_Active_003, TestSize.Level1)
{
    CreateByNameWithParam(VCodecTestCode::HW_HDR);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(VCodecTestCode::HW_HDR);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
    SuspendFreeze();
    SuspendActive();
}

/**
 * @tc.name: VideoDecoder_Hardware_Active_011
 * @tc.desc: 1.decoder called post processing
 *           2.decoder is executing
 *           3.active all process
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Hardware_Active_011, TestSize.Level1)
{
    CreateByNameWithParam(VCodecTestCode::HW_HDR);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(VCodecTestCode::HW_HDR);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    videoDec_->isKeepExecuting_ = true;

    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AVCS_ERR_OK, videoDec_->Start());
    SuspendFreeze();
    SuspendActive();
}
#endif // HMOS_TEST
} // namespace

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        cout << argv[i] << endl;
        if (strcmp(argv[i], "--create_multi_avc_dec") == 0) {
            DecArgv(i, argc, argv);
            testing::InitGoogleTest(&argc, argv);
            testing::GTEST_FLAG(filter) = "VideoHDecoderMemoryRecyleTest.Create_Hardware_Avc_Decoder";
            return RUN_ALL_TESTS();
        } else if (strcmp(argv[i], "--create_multi_hevc_dec") == 0) {
            DecArgv(i, argc, argv);
            testing::InitGoogleTest(&argc, argv);
            testing::GTEST_FLAG(filter) = "VideoHDecoderMemoryRecyleTest.Create_Hardware_Hevc_Decoder";
            return RUN_ALL_TESTS();
        } else if (strcmp(argv[i], "--need_dump") == 0) {
            VideoDecSample::needDump_ = true;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}