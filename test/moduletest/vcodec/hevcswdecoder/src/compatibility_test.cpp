/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <iostream>
#include <cstdio>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"
#include "videodec_api11_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace OHOS {
namespace Media {
static OH_AVCapability *cap_hevc = nullptr;
static string g_codecNameHevc = "";
static string g_codecNameHevcHw = "";
constexpr int32_t SYS_MAX_INPUT_SIZE = 1024 * 1024 * 24;
class HevcSwdecCompNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
};

void HevcSwdecCompNdkTest::SetUpTestCase(void)
{
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, SOFTWARE);
    g_codecNameHevc = OH_AVCapability_GetName(cap_hevc);
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    g_codecNameHevcHw = OH_AVCapability_GetName(cap_hevc);
}
void HevcSwdecCompNdkTest::TearDownTestCase(void) {}
void HevcSwdecCompNdkTest::SetUp(void) {}
void HevcSwdecCompNdkTest::TearDown(void) {}
}
}

namespace {
/**
 * @tc.number    : VIDEO_SWDEC_H265_HDRVivid_COMP_0010
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_HDRVivid_COMP_0010, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_SURFACE_EXPIRED_COMP_0010
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_SURFACE_EXPIRED_COMP_0010, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->expireSurface = true;
        vDecSample->NEED_MD5_COMPARE = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_MAXINPUTSIZE_COMP_0010
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_MAXINPUTSIZE_COMP_0010, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->maxInputSize = SYS_MAX_INPUT_SIZE;
        vDecSample->NEED_MD5_COMPARE = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_ONERROR_COMP_0010
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_ONERROR_COMP_0010, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_CROP_COMP_0010
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_CROP_COMP_0010, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        vDecSample->needCheckOutputDesc= true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_HDRMETADATA_COMP_0010
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_HDRMETADATA_COMP_0010, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        vDecSample->hdrColorInfo = true;
        vDecSample->hdrStaticMetadata = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_HDRMETADATA_COMP_0020
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_HDRMETADATA_COMP_0020, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        vDecSample->hdrColorInfo = true;
        vDecSample->hdrStaticMetadata = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_HDRMETADATA_COMP_0030
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_HDRMETADATA_COMP_0030, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        vDecSample->hdrColorInfo = true;
        vDecSample->hdrStaticMetadata = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_HDRMETADATA_COMP_0040
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_HDRMETADATA_COMP_0040, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        vDecSample->hdrColorInfo = true;
        vDecSample->hdrStaticMetadata = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_HDRMETADATA_COMP_0050
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_HDRMETADATA_COMP_0050, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        vDecSample->hdrColorInfo = true;
        vDecSample->hdrStaticMetadata = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_HDRMETADATA_COMP_0060
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_HDRMETADATA_COMP_0060, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        vDecSample->hdrColorInfo = true;
        vDecSample->hdrStaticMetadata = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_HDRMETADATA_COMP_0070
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_HDRMETADATA_COMP_0070, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        vDecSample->hdrColorInfo = true;
        vDecSample->hdrStaticMetadata = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevcHw));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_HDRMETADATA_COMP_0080
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_H265_HDRMETADATA_COMP_0080, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->NEED_MD5_COMPARE = false;
        vDecSample->hdrColorInfo = true;
        vDecSample->hdrStaticMetadata = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevcHw));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_SET_DECODING_ORDER_0010
 * @tc.name      : software decode frame set decoding order
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_SET_DECODING_ORDER_0010, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enableDecodingOrder = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_SET_DECODING_ORDER_0020
 * @tc.name      : software decode frame set decoding order
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_SET_DECODING_ORDER_0020, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/test_continuous_b_4.hevc";
        vDecSample->DEFAULT_WIDTH = 320;
        vDecSample->DEFAULT_HEIGHT = 240;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enableDecodingOrder = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_SET_DECODING_ORDER_0030
 * @tc.name      : software decode frame set decoding order
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_SET_DECODING_ORDER_0030, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/test_continuous_b_8.hevc";
        vDecSample->DEFAULT_WIDTH = 320;
        vDecSample->DEFAULT_HEIGHT = 240;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enableDecodingOrder = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_SET_DECODING_ORDER_0050
 * @tc.name      : software decode frame set decoding order
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_SET_DECODING_ORDER_0050, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/test_ibbp.hevc";
        vDecSample->DEFAULT_WIDTH = 320;
        vDecSample->DEFAULT_HEIGHT = 240;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enableDecodingOrder = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_SET_DECODING_ORDER_0060
 * @tc.name      : software decode frame set decoding order
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_SET_DECODING_ORDER_0060, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/test_ibpbp.hevc";
        vDecSample->DEFAULT_WIDTH = 320;
        vDecSample->DEFAULT_HEIGHT = 240;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enableDecodingOrder = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_SET_DECODING_ORDER_0070
 * @tc.name      : software decode frame set decoding order
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_SET_DECODING_ORDER_0070, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enableDecodingOrder = true;
        vDecSample->enableSyncMode = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_SET_DECODING_ORDER_0080
 * @tc.name      : software decode frame set decoding order
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_SET_DECODING_ORDER_0080, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enableDecodingOrder = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_SET_DECODING_ORDER_0090
 * @tc.name      : software decode frame set decoding order
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_SET_DECODING_ORDER_0090, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/hlg_1280_720_10.h265";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enableDecodingOrder = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_SET_DECODING_ORDER_0100
 * @tc.name      : software decode frame set decoding order
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecCompNdkTest, VIDEO_SWDEC_SET_DECODING_ORDER_0100, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, SOFTWARE);
        bool ret = OH_AVCapability_IsFeatureSupported(cap_hevc, VIDEO_DECODER_OUTPUT_IN_DECODING_ORDER);
        ASSERT_TRUE(ret);
    }
}
}