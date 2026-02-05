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
class HwdecHdr2SdrInnerTest : public testing::Test {
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
    const char *INP_DIR_720_10 = "/data/test/media/hlg_1280_720_10.h265";
    const char *INP_DIR_1080_10 = "/data/test/media/hlg_1920_1080_10.h265";
    const char *INP_DIR_1920_1440_30 = "/data/test/media/hlg_1920_1440_30.h265";
    const char *INP_DIR_2336_1080_10 = "/data/test/media/hlg_2336_1080_10.h265";
    const char *INP_DIR_2336_1088_10 = "/data/test/media/hlg_2336_1088_10.h265";
    const char *INP_DIR_4K_10 = "/data/test/media/hlg_3840_2160_10.h265";
    const char *inpDirResolution = "/data/test/media/hlg_resolution_change_10.h265";
    const char *inpDirSdr = "/data/test/media/1920_1080_20M_30.h265";
    const char *inpDirHdr10 = "/data/test/media/hdr10.h265";
    const char *inpDirHlg = "/data/test/media/hlg.h265";
    const char *inpDirPqlimit = "/data/test/media/pqHdrVivid_4k.h265";
    const char *inpDirHlgLimit = "/data/test/media/hlgHdrVivid_1080p.h265";
};
} // namespace Media
} // namespace OHOS

namespace {
static OH_AVCapability *cap = nullptr;
static OH_AVCapability *cap_hevc = nullptr;
static OH_AVCapability *cap_swhevc = nullptr;
static string g_codecName = "";
static string g_codecNameHEVC = "";
static string g_codecNameSwHEVC = "";
} // namespace

void HwdecHdr2SdrInnerTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    g_codecName = OH_AVCapability_GetName(cap);
    cout << "g_codecName: " << g_codecName << endl;
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    g_codecNameHEVC = OH_AVCapability_GetName(cap_hevc);
    cout << "g_codecNameHEVC: " << g_codecNameHEVC << endl;
    cap_swhevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, SOFTWARE);
    g_codecNameSwHEVC = OH_AVCapability_GetName(cap_swhevc);
    cout << "g_codecNameSwHEVC: " << g_codecNameSwHEVC << endl;
}
void HwdecHdr2SdrInnerTest::TearDownTestCase() {}
void HwdecHdr2SdrInnerTest::SetUp() {}
void HwdecHdr2SdrInnerTest::TearDown() {}

namespace {
/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0001
 * @tc.name      : h265 hard decode surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, 720p HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0001, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_720_10;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0002
 * @tc.name      : h265 hard decode surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 LIMIT, 720p HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0002, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_720_10;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->BT709_LIMIT_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0003
 * @tc.name      : h265 hard decode surface, nv21, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, 1080p HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0003, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_1080_10;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV21);
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0004
 * @tc.name      : 265 hw surface, nv21, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 LIMIT, 1080p HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0004, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_1080_10;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV21);
    vDecSample->SF_OUTPUT = true;
    vDecSample->BT709_LIMIT_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0005
 * @tc.name      : 265 hd surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, 1920_1440 HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0005, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_1920_1440_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1440;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0006
 * @tc.name      : 265 hw surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 LIMIT, 1920_1440 HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0006, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_1920_1440_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1440;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->BT709_LIMIT_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0007
 * @tc.name      : 265 hw surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, 2336_1080 HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0007, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_2336_1080_10;
    vDecSample->DEFAULT_WIDTH = 2336;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0008
 * @tc.name      : 265 hw surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 LIMIT, 2336_1080 HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0008, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_2336_1080_10;
    vDecSample->DEFAULT_WIDTH = 2336;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->BT709_LIMIT_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0009
 * @tc.name      : 265 hw surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, 2336_1088 HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0009, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_2336_1088_10;
    vDecSample->DEFAULT_WIDTH = 2336;
    vDecSample->DEFAULT_HEIGHT = 1088;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0010
 * @tc.name      : 265 hw surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 LIMIT, 2336_1088 HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0010, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_2336_1088_10;
    vDecSample->DEFAULT_WIDTH = 2336;
    vDecSample->DEFAULT_HEIGHT = 1088;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->BT709_LIMIT_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0011
 * @tc.name      : 265 hw switch surface, nv21, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, 4k HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0011, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_4K_10;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV21);
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0012
 * @tc.name      : h265 hw surface, nv21, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 LIMIT, 4k HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0012, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_4K_10;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV21);
    vDecSample->SF_OUTPUT = true;
    vDecSample->BT709_LIMIT_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0013
 * @tc.name      : H265 hw surface, nv21, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, res chnage HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0013, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = inpDirResolution;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV21);
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0014
 * @tc.name      : 265 hw surface, nv21, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 LIMIT, res change HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0014, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = inpDirResolution;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV21);
    vDecSample->SF_OUTPUT = true;
    vDecSample->BT709_LIMIT_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0015
 * @tc.name      : H265 hw, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0015, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_720_10;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->BT709_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, vDecSample->Configure());
    } else {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0016
 * @tc.name      : H265 hw, rgba, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0016, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_720_10;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::RGBA);
    ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
    ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0017
 * @tc.name      : H265 hard decode surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL,SDR
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0017, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = inpDirSdr;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->HDR2SDR = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0018
 * @tc.name      : H265 hard decode surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, HDR 10
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0018, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = inpDirHdr10;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->HDR2SDR = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0019
 * @tc.name      : H265 hard decode surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, HLG
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0019, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = inpDirHlg;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->HDR2SDR = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0020
 * @tc.name      : H265 hard decode surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, PQ limit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0020, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = inpDirPqlimit;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->HDR2SDR = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0021
 * @tc.name      : H265 hard decode surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, HLG limit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0021, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = inpDirHlgLimit;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->HDR2SDR = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0022
 * @tc.name      : H265 hw surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, no prepare before start
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0022, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_720_10;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->CreateSurface();
    ASSERT_NE(vDecSample->nativeWindow[0], nullptr);
    ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Configure());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, vDecSample->Start());
    } else {
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0023
 * @tc.name      : H265 hw buffer, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0023, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_720_10;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, vDecSample->RunVideoDecoder(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    } else {
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_INNER_FUNC_0024
 * @tc.name      : H265 hard decode, nv21, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, HLG FULl
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_HW_HDR2SDR_INNER_FUNC_0024, TestSize.Level0)
{
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_720_10;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->P3_FULL_FLAG = true;
    vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV21);
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecName));
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, vDecSample->Configure());
    } else {
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0001
 * @tc.name      : h265 soft decode surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, 720p HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0001, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_10;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SF_OUTPUT = true;
        vDecSample->P3_FULL_FLAG = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameSwHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0002
 * @tc.name      : h265 soft decode surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 LIMIT, 720p HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0002, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_10;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SF_OUTPUT = true;
        vDecSample->BT709_LIMIT_FLAG = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameSwHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0003
 * @tc.name      : h265 soft decode surface, nv21, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, 1080p HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0003, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_1080_10;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV21);
        vDecSample->SF_OUTPUT = true;
        vDecSample->P3_FULL_FLAG = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameSwHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0004
 * @tc.name      : 265 sw surface, nv21, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 LIMIT, 1080p HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0004, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_1080_10;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV21);
        vDecSample->SF_OUTPUT = true;
        vDecSample->BT709_LIMIT_FLAG = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameSwHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0005
 * @tc.name      : 265 sd surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, 1920_1440 HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0005, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_1920_1440_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1440;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->P3_FULL_FLAG = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameSwHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0006
 * @tc.name      : 265 hw surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 LIMIT, 1920_1440 HLG FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0006, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
    shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
    vDecSample->INP_DIR = INP_DIR_1920_1440_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1440;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->BT709_LIMIT_FLAG = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameSwHEVC));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0007
 * @tc.name      : H265 sw, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE BT709 FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0007, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_10;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SF_OUTPUT = true;
        vDecSample->BT709_FULL_FLAG = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameSwHEVC));
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0008
 * @tc.name      : H265 sw, rgba, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0008, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_10;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SF_OUTPUT = true;
        vDecSample->P3_FULL_FLAG = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::RGBA);
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameSwHEVC));
        ASSERT_EQ(AVCS_ERR_UNSUPPORT, vDecSample->Configure());
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0010
 * @tc.name      : H265 soft decode surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, HLG limit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0010, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = inpDirHlgLimit;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->P3_FULL_FLAG = true;
        vDecSample->HDR2SDR = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameSwHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0011
 * @tc.name      : H265 sw surface, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, no prepare before start
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0011, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_10;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SF_OUTPUT = true;
        vDecSample->P3_FULL_FLAG = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->CreateSurface();
        ASSERT_NE(vDecSample->nativeWindow[0], nullptr);
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameSwHEVC));
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->Configure());
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, vDecSample->Start());
    }
}
  
/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0012
 * @tc.name      : H265 sw buffer, nv12, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0012, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_10;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->P3_FULL_FLAG = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, vDecSample->RunVideoDecoder(g_codecNameSwHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_SW_HDR2SDR_INNER_FUNC_0013
 * @tc.name      : H265 soft decode, nv21, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE P3 FULL, HLG FULl
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrInnerTest, HEVC_SW_HDR2SDR_INNER_FUNC_0013, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_720_10;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SF_OUTPUT = true;
        vDecSample->P3_FULL_FLAG = true;
        vDecSample->DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV21);
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;   
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->CreateByName(g_codecNameSwHEVC));
    }
}
} //namespace