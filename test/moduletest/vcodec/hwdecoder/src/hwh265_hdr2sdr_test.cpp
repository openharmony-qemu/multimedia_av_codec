/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "native_avcodec_videodecoder.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videodec_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "native_buffer.h"

#define MAX_THREAD 16

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class HwdecHdr2SdrNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();
};
} // namespace Media
} // namespace OHOS

namespace {
static OH_AVCapability *cap_avc = nullptr;
static OH_AVCapability *cap_hevc = nullptr;
static string g_codecNameAVC = "";
static string g_codecNameHEVC = "";
OH_AVCodec *vdec_ = NULL;
OH_AVFormat *format;
constexpr int32_t DEFAULT_WIDTH = 1920;
constexpr int32_t DEFAULT_HEIGHT = 1080;
} // namespace

void HwdecHdr2SdrNdkTest::SetUpTestCase()
{
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    g_codecNameHEVC = OH_AVCapability_GetName(cap_hevc);
    cout << "g_codecNameHEVC: " << g_codecNameHEVC << endl;
}
void HwdecHdr2SdrNdkTest::TearDownTestCase() {}

void HwdecHdr2SdrNdkTest::SetUp(void) {}

void HwdecHdr2SdrNdkTest::TearDown()
{
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    if (vdec_ != NULL) {
        OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }
}

namespace {
/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_001
 * @tc.name      : OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE设置为BT_709_FULL
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_001, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_FULL));
        ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(vdec_, format));
    }
    else {
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_FULL));
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, OH_VideoDecoder_Configure(vdec_, format));
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_002
 * @tc.name      : OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE设置为BT_709_LIMIT, pixel foramt RGBA
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_002, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_LIMIT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_RGBA));
        ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(vdec_, format));
    }
    else {
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_LIMIT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_RGBA));
        ASSERT_EQ(AV_ERR_UNSUPPORT, OH_VideoDecoder_Configure(vdec_, format));
    }
}


/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_003
 * @tc.name      : OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE设置为BT_2020_LIMIT
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_003, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT2020_HLG_LIMIT));
        ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(vdec_, format));
    }
    else {
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT2020_HLG_LIMIT));
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, OH_VideoDecoder_Configure(vdec_, format));
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_0032
 * @tc.name      : OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE设置为BT_709_LIMIT
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_0032, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_LIMIT));
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    }
    else {
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_LIMIT));
        ASSERT_EQ(AV_ERR_UNSUPPORT, OH_VideoDecoder_Configure(vdec_, format));
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_004
 * @tc.name      : test h265 hard decode surface, pixel foramt nv12, KEY设置为BT_709_LIMIT, SDR
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_004, TestSize.Level2)

{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_005
 * @tc.name      : test h265 hard decode surface, pixel foramt nv12, KEY设置为BT_709_LIMIT, HDR 10
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_005, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hdr10.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hdr10.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_006
 * @tc.name      : test h265 hard decode surface, pixel foramt nv12, KEY设置为BT_709_LIMIT, HLG
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_006, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlg.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlg.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_0061
 * @tc.name      : test h265 hard decode surface, pixel foramt nv12, KEY设置为BT_709_LIMIT, 4k_720p_1080p
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_0061, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_4k_720p_1080p.h265";
        vDecSample->DEFAULT_WIDTH = 3840;
        vDecSample->DEFAULT_HEIGHT = 2160;
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_4k_720p_1080p.h265";
        vDecSample->DEFAULT_WIDTH = 3840;
        vDecSample->DEFAULT_HEIGHT = 2160;
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_007
 * @tc.name      : test h265 hard decode surface, pixel foramt nv12, KEY设置为BT_709_LIMIT, PQ HDR vivid
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_007, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/pqHdrVivid_4k.h265";
        vDecSample->DEFAULT_WIDTH = 3840;
        vDecSample->DEFAULT_HEIGHT = 2160;
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/pqHdrVivid_4k.h265";
        vDecSample->DEFAULT_WIDTH = 3840;
        vDecSample->DEFAULT_HEIGHT = 2160;
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_008
 * @tc.name      : test h265 hard decode surface, pixel foramt nv12, KEY设置为BT_709_LIMIT, HLG HDR vivid
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_008, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_4k.h265";
        vDecSample->DEFAULT_WIDTH = 3840;
        vDecSample->DEFAULT_HEIGHT = 2160;
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_4k.h265";
        vDecSample->DEFAULT_WIDTH = 3840;
        vDecSample->DEFAULT_HEIGHT = 2160;
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_009
 * @tc.name      : test h265 hard decode surface, pixel foramt nv12, KEY设置为BT_709_LIMIT, HLG HDR vivid
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_009, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_010
 * @tc.name      : test h265 hard decode surface, pixel foramt nv21, KEY设置为BT_709_LIMIT, PQ HDR vivid
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_010, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/pqHdrVivid_4k.h265";
        vDecSample->DEFAULT_WIDTH = 3840;
        vDecSample->DEFAULT_HEIGHT = 2160;
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        vDecSample->NV21_FLAG = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/pqHdrVivid_4k.h265";
        vDecSample->DEFAULT_WIDTH = 3840;
        vDecSample->DEFAULT_HEIGHT = 2160;
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        vDecSample->NV21_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_011
 * @tc.name      : test h265 hard decode surface, pixel foramt nv21, KEY设置为BT_709_LIMIT, HLG HDR vivid
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_011, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_4k.h265";
        vDecSample->DEFAULT_WIDTH = 3840;
        vDecSample->DEFAULT_HEIGHT = 2160;
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        vDecSample->NV21_FLAG = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_4k.h265";
        vDecSample->DEFAULT_WIDTH = 3840;
        vDecSample->DEFAULT_HEIGHT = 2160;
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        vDecSample->NV21_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_012
 * @tc.name      : test h265 hard decode surface, pixel foramt nv21, KEY设置为BT_709_LIMIT, HLG HDR vivid
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_012, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        vDecSample->NV21_FLAG = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        vDecSample->NV21_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_013
 * @tc.name      : test h265 hard decode buffer, pixel foramt nv21, KEY设置为BT_709_LIMIT, HLG HDR vivid
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_013, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->SF_OUTPUT = false;
        vDecSample->TRANSFER_FLAG = true;
        vDecSample->NV21_FLAG = true;
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->RunVideoDec(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->SF_OUTPUT = false;
        vDecSample->TRANSFER_FLAG = true;
        vDecSample->NV21_FLAG = true;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_014
 * @tc.name      : test h265 encoder set OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE to BT_709_LIMIT
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_014, TestSize.Level2)
{
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    g_codecNameHEVC = OH_AVCapability_GetName(cap_hevc);
    vdec_ = OH_VideoEncoder_CreateByName(g_codecNameHEVC.c_str());
    ASSERT_NE(NULL, vdec_);
    format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
    OH_COLORSPACE_BT709_LIMIT));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(vdec_, format));
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_015
 * @tc.name      : test h265 software decoder, KEY设置为BT_709_LIMIT
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_015, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, SOFTWARE);
        g_codecNameHEVC = OH_AVCapability_GetName(cap_hevc);
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_LIMIT));
        ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(vdec_, format));
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_016
 * @tc.name      : test h264 hardware decoder, set OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE to BT_709_LIMIT
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_016, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        cap_avc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
        g_codecNameAVC = OH_AVCapability_GetName(cap_avc);
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameAVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_LIMIT));
        ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(vdec_, format));
    }
    else {
        cap_avc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
        g_codecNameAVC = OH_AVCapability_GetName(cap_avc);
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameAVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_LIMIT));
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, OH_VideoDecoder_Configure(vdec_, format));
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_017
 * @tc.name      : test h264 software decoder, set OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE to BT_709_LIMIT
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_017, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        cap_avc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
        g_codecNameAVC = OH_AVCapability_GetName(cap_avc);
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameAVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_LIMIT));
        ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(vdec_, format));
    }
    else {
        cap_avc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
        g_codecNameAVC = OH_AVCapability_GetName(cap_avc);
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameAVC.c_str());
        ASSERT_NE(NULL, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(NULL, format);
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_COLORSPACE_BT709_LIMIT));
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, OH_VideoDecoder_Configure(vdec_, format));
    }
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_FUNC_018
 * @tc.name      : test h265 HDR2SDR, no prepare before start
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrNdkTest, HEVC_HW_HDR2SDR_FUNC_018, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
        g_codecNameHEVC = OH_AVCapability_GetName(cap_hevc);
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        vDecSample->PREPARE_FLAG = false;
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
    else {
        cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
        g_codecNameHEVC = OH_AVCapability_GetName(cap_hevc);
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->SF_OUTPUT = true;
        vDecSample->TRANSFER_FLAG = true;
        vDecSample->PREPARE_FLAG = false;
        ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}
} //namespace