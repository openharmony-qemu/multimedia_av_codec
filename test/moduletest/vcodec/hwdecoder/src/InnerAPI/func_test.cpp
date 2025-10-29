/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include <string>
#include "gtest/gtest.h"
#include "native_averrors.h"
#include "videodec_sample.h"
#include "videodec_inner_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "avcodec_suspend.h"
#include <unistd.h>
#include "syspara/parameters.h"

#define MAX_THREAD 16

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class HwdecInnerFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();

protected:
    const char *INP_DIR_720_30 = "/data/test/media/1280_720_30_10Mb.h264";
};
} // namespace Media
} // namespace OHOS

namespace {
static OH_AVCapability *cap = nullptr;
static OH_AVCapability *cap_hevc = nullptr;
static OH_AVCapability *cap_vvc = nullptr;
static string g_codecName = "";
static string g_codecNameHEVC = "";
static string g_codecNameVVC = "";
} // namespace

void HwdecInnerFuncNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    g_codecName = OH_AVCapability_GetName(cap);
    cout << "codecname: " << g_codecName << endl;
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    g_codecNameHEVC = OH_AVCapability_GetName(cap_hevc);
    cout << "g_codecNameHEVC: " << g_codecNameHEVC << endl;
    cap_vvc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false, HARDWARE);
    g_codecNameVVC = OH_AVCapability_GetName(cap_vvc);
    cout << "g_codecNameVVC: " << g_codecNameVVC << endl;
}
void HwdecInnerFuncNdkTest::TearDownTestCase() {}
void HwdecInnerFuncNdkTest::SetUp() {}
void HwdecInnerFuncNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_API_0010
 * @tc.name      : SuspendFreeze, std::vector<pid_t> &pidList size is 0
 * @tc.desc      : api test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_API_0010, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        std::vector<pid_t> pidList = {};
        ASSERT_EQ(AVCS_ERR_INPUT_DATA_ERROR, AVCodecSuspend::SuspendFreeze(pidList));
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_API_0020
 * @tc.name      : SuspendFreeze, pid not exist
 * @tc.desc      : api test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_API_0020, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        pid_t pid = -1;
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_API_0030
 * @tc.name      : SuspendActive, std::vector<pid_t> &pidList size is 0
 * @tc.desc      : api test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_API_0030, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        std::vector<pid_t> pidList = {};
        ASSERT_EQ(AVCS_ERR_INPUT_DATA_ERROR, AVCodecSuspend::SuspendActive(pidList));
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_API_0040
 * @tc.name      : SuspendActive, pid not exist
 * @tc.desc      : api test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_API_0040, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        pid_t pid = -1;
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_API_0050
 * @tc.name      : SuspendActiveAll
 * @tc.desc      : api test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_API_0050, TestSize.Level0)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0010
 * @tc.name      : SuspendActive
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0010, TestSize.Level1)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0020
 * @tc.name      : SuspendFreeze
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0020, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        vDecSample->SetRunning();
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0030
 * @tc.name      : SuspendActiveAll
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0030, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0040
 * @tc.name      : SuspendFreeze-SuspendFreeze
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0040, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        vDecSample->SetRunning();
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0050
 * @tc.name      : SuspendActive-SuspendActive
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0050, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0060
 * @tc.name      : SuspendActiveAll-SuspendActiveAll
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0060, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0070
 * @tc.name      : SuspendFreeze-SuspendActive
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0070, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0080
 * @tc.name      : SuspendFreeze-SuspendActiveAll
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0080, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0090
 * @tc.name      : SuspendActive-SuspendFreeze
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0090, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        vDecSample->SetRunning();
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0100
 * @tc.name      : SuspendActiveAll-SuspendFreeze
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0100, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        vDecSample->SetRunning();
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0110
 * @tc.name      : SuspendFreeze-SuspendActive-SuspendFreeze-SuspendActiveAll
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0110, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0120
 * @tc.name      : SuspendFreeze-SuspendActive-SuspendFreeze-SuspendActive
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0120, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0130
 * @tc.name      : SuspendFreeze-SuspendActiveAll-SuspendFreeze-SuspendActiveAll
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0130, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0140
 * @tc.name      : SuspendActive, flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0140, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0150
 * @tc.name      : SuspendFreeze, flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0150, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0160
 * @tc.name      : SuspendActiveAll, flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0160, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0170
 * @tc.name      : SuspendFreeze-SuspendFreeze, flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0170, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0180
 * @tc.name      : SuspendActive-SuspendActive, flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0180, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0190
 * @tc.name      : SuspendActiveAll-SuspendActiveAll, flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0190, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0200
 * @tc.name      : SuspendFreeze-SuspendActive,flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0200, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0210
 * @tc.name      : SuspendFreeze-SuspendActiveAll,flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0210, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0220
 * @tc.name      : SuspendActive-SuspendFreeze,flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0220, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0230
 * @tc.name      : SuspendActiveAll-SuspendFreeze, flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0230, TestSize.Level0)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0240
 * @tc.name      : SuspendFreeze-SuspendActive-SuspendFreeze-SuspendActiveAll, flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0240, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0250
 * @tc.name      : SuspendFreeze-SuspendActive-SuspendFreeze-SuspendActive,flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0250, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0260
 * @tc.name      : SuspendFreeze-SuspendActiveAll-SuspendFreeze-SuspendActiveAll,flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0260, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Flush());
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}


/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0270
 * @tc.name      : SuspendActive, eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0270, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0280
 * @tc.name      : SuspendFreeze, eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0280, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0290
 * @tc.name      : SuspendActiveAll, eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0290, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0300
 * @tc.name      : SuspendFreeze-SuspendFreeze, eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0300, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0310
 * @tc.name      : SuspendActive-SuspendActive, eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0310, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0320
 * @tc.name      : SuspendActiveAll-SuspendActiveAll, eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0320, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0330
 * @tc.name      : SuspendFreeze-SuspendActive,flush
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0330, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0340
 * @tc.name      : SuspendFreeze-SuspendActiveAll, eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0340, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0350
 * @tc.name      : SuspendActive-SuspendFreeze,eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0350, TestSize.Level1)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0360
 * @tc.name      : SuspendActiveAll-SuspendFreeze, eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0360, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0370
 * @tc.name      : SuspendFreeze-SuspendActive-SuspendFreeze-SuspendActiveAll, eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0370, TestSize.Level0)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0380
 * @tc.name      : SuspendFreeze-SuspendActive-SuspendFreeze-SuspendActive,eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0380, TestSize.Level1)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActive(pidList));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0390
 * @tc.name      : SuspendFreeze-SuspendActiveAll-SuspendFreeze-SuspendActiveAll,eos
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_HWDEC_INNER_RECYCLEMEMORY_FUNC_0390, TestSize.Level2)
{
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (recycleMemory) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        pid_t pid = getpid();
        std::vector<pid_t> pidList = {pid};
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendFreeze(pidList));
        ASSERT_EQ(AVCS_ERR_OK, AVCodecSuspend::SuspendActiveAll());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0010
 * @tc.name      : inner接口, H264, 宽高于上限
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0010, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/8194_720.h264";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0020
 * @tc.name      : inner接口, H265, 宽高于上限
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0020, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0) && cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/8194_720.h265";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0030
 * @tc.name      : inner接口, H264, 宽低于下限
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0030, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/90_720.h264";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0040
 * @tc.name      : inner接口, H265, 宽低于下限
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0040, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/90_720.h265";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0050
 * @tc.name      : inner接口, H264, 高高于上限
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0050, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/720_8194.h264";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0060
 * @tc.name      : inner接口, H265, 高高于上限
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0060, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/720_8194.h265";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0070
 * @tc.name      : inner接口, H264, 高低于下限
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0070, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/720_90.h264";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0080
 * @tc.name      : inner接口, H265, 高低于下限
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0080, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/720_90.h265";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0090
 * @tc.name      : inner接口, H265, 位深为12bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0090, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0) && cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1280_720_12bit.h265";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0100
 * @tc.name      : inner接口, H264, format为400
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1280_720_gray.h264";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0110
 * @tc.name      : inner接口, H264, format为422
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0110, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1280_720_yuv422.h264";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0120
 * @tc.name      : inner接口, H264, format为444
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0120, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1280_720_yuv444.h264";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0130
 * @tc.name      : inner接口, H265, format为400
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0130, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1280_720_gray.h265";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0140
 * @tc.name      : inner接口, H265, format为422
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0140, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1280_720_yuv422.h265";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0150
 * @tc.name      : inner接口, H265, format为444
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0150, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1280_720_yuv444.h265";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0160
 * @tc.name      : inner接口, H264, mbaff
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0160, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1280_720_mbaff.h264";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0170
 * @tc.name      : inner接口, H264, 位深为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0170, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1280_720_10bit.h264";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_UNSUPPORTED_CODEC_SPECIFICATION, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0180
 * @tc.name      : inner接口, H264, xps元素错误
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0180, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1280_720_error_xps.h264";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        vDecSample->needSendOneFrame = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_ILLEGAL_PARAMETER_SETS, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0190
 * @tc.name      : inner接口, H265, xps元素错误
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0190, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_error_xps.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_ILLEGAL_PARAMETER_SETS, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0200
 * @tc.name      : inner接口, H264, xps为空
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0200, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        vDecSample->needXpsEmpty = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_MINSSING_PARAMETER_SETS, vDecSample->errCodeResult);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_INNER_ERRCODE_REPORT_0210
 * @tc.name      : inner接口, H265, xps为空
 * @tc.desc      : function test
 */
HWTEST_F(HwdecInnerFuncNdkTest, VIDEO_DECODE_INNER_ERRCODE_REPORT_0210, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0) && cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->checkErrCode = true;
        vDecSample->noNeedFirstFrame = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_MINSSING_PARAMETER_SETS, vDecSample->errCodeResult);
    }
}
} // namespace