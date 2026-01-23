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

#include <iostream>
#include <cstdio>

#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <fcntl.h>

#include "gtest/gtest.h"
#include "videodec_api11_sample.h"
#include "native_avcodec_videodecoder.h"
#include "native_avformat.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"

#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "native_avmemory.h"

#ifdef SUPPORT_DRM
#include "native_mediakeysession.h"
#include "native_mediakeysystem.h"
#endif

#define PIXFORMAT_NUM 3

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace OHOS {
namespace Media {
class RawVideo1decApiNdkTest : public testing::Test {
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
static OH_AVErrCode g_ret = AV_ERR_OK;
static uint32_t g_pixelFormatNum = 0;
static const int32_t *g_pixelFormat = nullptr;
static OH_AVCodec *g_vdec = nullptr;
static OH_AVCapability *g_cap = nullptr;
const string CODEC_NAME = "OH.Media.Codec.Decoder.Video.RAWVIDEO";
static VDecAPI11Signal *g_signal = nullptr;
static constexpr uint32_t DEFAULTWIDTH = 720;
static constexpr uint32_t DEFAULTHEIGHT = 480;
static constexpr uint32_t DEFAULTFRAMERATE = 30;
static OH_AVFormat *g_format = nullptr;

void RawVideo1decApiNdkTest::SetUpTestCase() {
}
void RawVideo1decApiNdkTest::TearDownTestCase() {
}
void RawVideo1decApiNdkTest::SetUp()
{
    g_signal = new VDecAPI11Signal();
}
void RawVideo1decApiNdkTest::TearDown()
{
    if (g_format != nullptr) {
        OH_AVFormat_Destroy(g_format);
        g_format = nullptr;
    }
    if (g_signal) {
        delete g_signal;
        g_signal = nullptr;
    }
    if (g_vdec != NULL) {
        OH_VideoDecoder_Destroy(g_vdec);
        g_vdec = nullptr;
    }
}
} // namespace Media
} // namespace OHOS

namespace {
/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_0100
 * @tc.name      : Test repeat creation of OH_VideoDecoder_CreateByName
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_0100, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(g_vdec, NULL);
    OH_AVCodec *vdec_2 = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(vdec_2, NULL);
    OH_VideoDecoder_Destroy(vdec_2);
    vdec_2 = nullptr;
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_0200
 * @tc.name      : Test configuration of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_0200, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, g_vdec);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULTFRAMERATE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, format));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(g_vdec, format));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_0300
 * @tc.name      : Test start operation of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_0300, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, g_vdec);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULTFRAMERATE);
    (void)OH_AVFormat_SetIntValue(format, "rawvideo_input_pix_fmt", 1);
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(g_vdec));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Start(g_vdec));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_0400
 * @tc.name      : Test stop operation of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_0400, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, g_vdec);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULTFRAMERATE);
    (void)OH_AVFormat_SetIntValue(format, "rawvideo_input_pix_fmt", 1);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(g_vdec));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(g_vdec));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(g_vdec));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_0500
 * @tc.name      : Test reset operation of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_0500, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, g_vdec);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULTFRAMERATE);
    (void)OH_AVFormat_SetIntValue(format, "rawvideo_input_pix_fmt", 1);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(g_vdec));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(g_vdec));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(g_vdec));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(g_vdec));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_0600
 * @tc.name      : Test EOS handling in OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_0600, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, g_vdec);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULTFRAMERATE);
    (void)OH_AVFormat_SetIntValue(format, "rawvideo_input_pix_fmt", 1);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(g_vdec));

    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;

    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputData(g_vdec, 0, attr));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputData(g_vdec, 0, attr));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_0700
 * @tc.name      : Test flush operation of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_0700, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, g_vdec);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULTFRAMERATE);
    (void)OH_AVFormat_SetIntValue(format, "rawvideo_input_pix_fmt", 1);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(g_vdec));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(g_vdec));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(g_vdec));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_0800
 * @tc.name      : Test release operation of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_0800, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, g_vdec);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULTFRAMERATE);
    (void)OH_AVFormat_SetIntValue(format, "rawvideo_input_pix_fmt", 1);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(g_vdec));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(g_vdec));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Destroy(g_vdec));
    g_vdec = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Destroy(g_vdec));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_0900
 * @tc.name      : Test creation of OH_VideoDecoder by MIME type
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_0900, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO);
    ASSERT_NE(g_vdec, NULL);
    OH_AVCodec *vdec_2 = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO);
    ASSERT_NE(vdec_2, NULL);
    OH_VideoDecoder_Destroy(vdec_2);
    vdec_2 = nullptr;
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_1000
 * @tc.name      : Test callback registration in OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_1000, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVCodecCallback cb_;
    cb_.onError = VdecAPI11Error;
    cb_.onStreamChanged = VdecAPI11FormatChanged;
    cb_.onNeedInputBuffer = VdecAPI11InputDataReady;
    cb_.onNewOutputBuffer = VdecAPI11OutputDataReady;
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(g_vdec, cb_, NULL));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(g_vdec, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_1001
 * @tc.name      : OH_VideoDecoder_RegisterSetCallback with null onError
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_1001, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVCodecCallback cb_;
    cb_.onError = nullptr;
    cb_.onStreamChanged = VdecAPI11FormatChanged;
    cb_.onNeedInputBuffer = VdecAPI11InputDataReady;
    cb_.onNewOutputBuffer = VdecAPI11OutputDataReady;
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(g_vdec, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_1002
 * @tc.name      : OH_VideoDecoder_RegisterSetCallback with null onNeedInputBuffer
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_1002, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVCodecCallback cb_;
    cb_.onError = VdecAPI11Error;
    cb_.onStreamChanged = VdecAPI11FormatChanged;
    cb_.onNeedInputBuffer = nullptr;
    cb_.onNewOutputBuffer = VdecAPI11OutputDataReady;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(g_vdec, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_1003
 * @tc.name      : OH_VideoDecoder_RegisterSetCallback with null onNewOutputBuffer
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_1003, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVCodecCallback cb_;
    cb_.onError = VdecAPI11Error;
    cb_.onStreamChanged = VdecAPI11FormatChanged;
    cb_.onNeedInputBuffer = VdecAPI11InputDataReady;
    cb_.onNewOutputBuffer = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(g_vdec, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_1004
 * @tc.name      : OH_VideoDecoder_RegisterSetCallback with null onStreamChanged
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_1004, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVCodecCallback cb_;
    cb_.onError = VdecAPI11Error;
    cb_.onStreamChanged = nullptr;
    cb_.onNeedInputBuffer = VdecAPI11InputDataReady;
    cb_.onNewOutputBuffer = VdecAPI11OutputDataReady;
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(g_vdec, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_1005
 * @tc.name      : OH_VideoDecoder_RegisterSetCallback with null capability
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_1005, TestSize.Level2)
{
    OH_AVCodecCallback cb_;
    cb_.onError = VdecAPI11Error;
    cb_.onStreamChanged = VdecAPI11FormatChanged;
    cb_.onNeedInputBuffer = VdecAPI11InputDataReady;
    cb_.onNewOutputBuffer = VdecAPI11OutputDataReady;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(nullptr, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_1100
 * @tc.name      : Test output description retrieval in OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_1100, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVFormat *format = OH_VideoDecoder_GetOutputDescription(g_vdec);
    ASSERT_NE(NULL, format);
    format = OH_VideoDecoder_GetOutputDescription(g_vdec);
    ASSERT_NE(NULL, format);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_API_1200
 * @tc.name      : Test parameter setting in OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_API_1200, TestSize.Level2)
{
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, g_vdec);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULTFRAMERATE);

    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_SetParameter(g_vdec, format));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_SetParameter(g_vdec, format));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    :  VIDEO_RAWVIDEODEC_CAP_API_0001
 * @tc.name      : OH_AVCodec_GetCapability with rawvideo mime
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest,  VIDEO_RAWVIDEODEC_CAP_API_0001, TestSize.Level2)
{
    g_cap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false);
    ASSERT_NE(g_cap, nullptr);
}

/**
 * @tc.number    :  VIDEO_RAWVIDEODEC_CAP_API_0002
 * @tc.name      : OH_AVCapability_GetName with rawvideo mime
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest,  VIDEO_RAWVIDEODEC_CAP_API_0002, TestSize.Level2)
{
    g_cap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false);
    string codec_name = OH_AVCapability_GetName(g_cap);
    ASSERT_EQ(CODEC_NAME, codec_name);
}

/**
 * @tc.number    :  VIDEO_RAWVIDEODEC_CAP_API_0003
 * @tc.name      : OH_AVCodec_GetCapability with null mime
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest,  VIDEO_RAWVIDEODEC_CAP_API_0003, TestSize.Level2)
{
    g_cap = OH_AVCodec_GetCapability(nullptr, false);
    ASSERT_EQ(g_cap, nullptr);
}

/**
 * @tc.number    :  VIDEO_RAWVIDEODEC_CAP_API_0004
 * @tc.name      : OH_AVCodec_GetCapabilityByCategory with null mime
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest,  VIDEO_RAWVIDEODEC_CAP_API_0004, TestSize.Level2)
{
    g_cap = OH_AVCodec_GetCapabilityByCategory(nullptr, false, SOFTWARE);
    ASSERT_EQ(g_cap, nullptr);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0100
 * @tc.name      : Test capability retrieval for RAWVIDEO codec
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0100, TestSize.Level1)
{
    g_cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(g_cap, nullptr);
    ASSERT_FALSE(OH_AVCapability_IsHardware(g_cap));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0101
 * @tc.name      : Test capability retrieval for null capability
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0101, TestSize.Level1)
{
    ASSERT_FALSE(OH_AVCapability_IsHardware(nullptr));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0200
 * @tc.name      : Test max supported instances retrieval for RAWVIDEO codec
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0200, TestSize.Level1)
{
    g_cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(g_cap, nullptr);
    ASSERT_EQ(64, OH_AVCapability_GetMaxSupportedInstances(g_cap));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0201
 * @tc.name      : Test max supported instances retrieval for null capability
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0201, TestSize.Level1)
{
    ASSERT_EQ(0, OH_AVCapability_GetMaxSupportedInstances(nullptr));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0300
 * @tc.name      : Test name retrieval for RAWVIDEO codec
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0300, TestSize.Level1)
{
    g_cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(g_cap, nullptr);
    ASSERT_EQ(CODEC_NAME, OH_AVCapability_GetName(g_cap));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0301
 * @tc.name      : Test name retrieval for null capability
 * @tc.desc      : function test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0301, TestSize.Level1)
{
    const char *name = OH_AVCapability_GetName(nullptr);
    ASSERT_NE(name, nullptr);
    ASSERT_EQ(strlen(name), 0);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0400
 * @tc.name      : Test video width alignment retrieval with invalid parameter
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthAlignment(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0401
 * @tc.name      : Test video width alignment retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0401, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoWidthAlignment(nullptr, &alignment);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0500
 * @tc.name      : Test video width alignment retrieval with valid parameter
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoWidthAlignment(capability, &alignment);
    cout << "WidthAlignment " << alignment << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(alignment, 2);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0600
 * @tc.name      : Test video height alignment retrieval with invalid parameter
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightAlignment(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0601
 * @tc.name      : Test video height alignment retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0601, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoHeightAlignment(nullptr, &alignment);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0700
 * @tc.name      : Test video height alignment retrieval with valid parameter
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0700, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoHeightAlignment(capability, &alignment);
    cout << "HeightAlignment " << alignment << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(alignment, 2);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0800
 * @tc.name      : Test video width range retrieval for height with invalid parameter
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0800, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(nullptr, DEFAULTHEIGHT, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_0900
 * @tc.name      : Test video width range retrieval for height with null range
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_0900, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULTHEIGHT, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1000
 * @tc.name      : Test video width range retrieval for height with zero height
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1000, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1001
 * @tc.name      : Test video width range retrieval for height with -1 height
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1001, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, -1, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1002
 * @tc.name      : Test video width range retrieval for height with 9999 height
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1002, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, 9999, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1003
 * @tc.name      : Test video width range retrieval for height with 4 height
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1003, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, 4, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1100
 * @tc.name      : Test video width range retrieval for height with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1100, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    const char *codecName = OH_AVCapability_GetName(capability);
    ASSERT_NE(nullptr, codecName);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULTHEIGHT, &range);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 4096);
    g_vdec = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, g_vdec);
    g_format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, g_format);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, range.minVal - 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, g_format));
    OH_VideoDecoder_Destroy(g_vdec);
    g_vdec = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, g_vdec);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, range.maxVal + 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, g_format));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1200
 * @tc.name      : Test video height range retrieval for width with invalid parameter
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1200, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(nullptr, DEFAULTWIDTH, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1300
 * @tc.name      : Test video height range retrieval for width with null range
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1300, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULTWIDTH, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1400
 * @tc.name      : Test video height range retrieval for width with zero width
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1401
 * @tc.name      : Test video height range retrieval for width with -1 width
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1401, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, -1, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1402
 * @tc.name      : Test video height range retrieval for width with 9999 width
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1402, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, 9999, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1500
 * @tc.name      : Test video height range retrieval for width with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    const char *codecName = OH_AVCapability_GetName(capability);
    ASSERT_NE(nullptr, codecName);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULTWIDTH, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 4096);
    g_vdec = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, g_vdec);
    g_format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, g_format);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, range.minVal - 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, g_format));
    OH_VideoDecoder_Destroy(g_vdec);
    g_vdec = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, g_vdec);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, range.maxVal + 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, g_format));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1600
 * @tc.name      : Test video width range retrieval with null range
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1601
 * @tc.name      : Test video width range retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1601, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange range;
    ret = OH_AVCapability_GetVideoWidthRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1700
 * @tc.name      : Test video width range retrieval with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1700, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 4096);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1701
 * @tc.name      : OH_AVCapability_GetVideoWidthRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1701, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 4096);
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(nullptr, g_vdec);
    g_format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, g_format);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, range.minVal - 1);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(g_vdec, g_format));
    OH_VideoDecoder_Destroy(g_vdec);
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(nullptr, g_vdec);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, range.maxVal + 1);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(g_vdec, g_format));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1800
 * @tc.name      : Test video height range retrieval with null range
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1800, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1801
 * @tc.name      : Test video height range retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1801, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange heightRange;
    ret = OH_AVCapability_GetVideoHeightRange(nullptr, &heightRange);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1802
 * @tc.name      : OH_AVCapability_GetVideoHeightRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1802, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    memset_s(&widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRange(capability, &heightRange);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(heightRange.minVal, 2);
    ASSERT_EQ(heightRange.maxVal, 4096);
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(nullptr, g_vdec);
    g_format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, g_format);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, heightRange.minVal - 1);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(g_vdec, g_format));
    OH_VideoDecoder_Destroy(g_vdec);
    g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(nullptr, g_vdec);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, heightRange.maxVal + 1);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(g_vdec, g_format));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1900
 * @tc.name      : Test video height range retrieval with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1900, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    memset_s(&widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(&heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    const char *codecName = OH_AVCapability_GetName(capability);
    ASSERT_NE(nullptr, codecName);
    ret = OH_AVCapability_GetVideoHeightRange(capability, &heightRange);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << heightRange.minVal << "  maxval=" << heightRange.maxVal << endl;
    ASSERT_EQ(heightRange.minVal, 2);
    ASSERT_EQ(heightRange.maxVal, 4096);
    ret = OH_AVCapability_GetVideoWidthRange(capability, &widthRange);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(widthRange.minVal, 2);
    ASSERT_EQ(widthRange.maxVal, 4096);
    cout << "minval=" << widthRange.minVal << "  maxval=" << widthRange.maxVal << endl;
    g_vdec = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, g_vdec);
    g_format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, g_format);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, widthRange.minVal - 1);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, heightRange.minVal - 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, g_format));
    OH_VideoDecoder_Destroy(g_vdec);
    g_vdec = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, g_vdec);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, widthRange.maxVal + 1);
    (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, heightRange.maxVal + 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, g_format));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_1901
 * @tc.name      : Test video width height range retrieval with Resolution combination max Border size
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_1901, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability =
        OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t maxHeight = 4096;
    int32_t maxWidth = 4096;
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, maxHeight, &range);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 4096);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, maxWidth, &range);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 4096);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, maxHeight + 2, &range);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 0);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, maxWidth + 2, &range);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 0);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2000
 * @tc.name      : Test video size support check with zero width
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2000, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, 0, DEFAULTHEIGHT));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2100
 * @tc.name      : Test video size support check with zero height
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2100, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, DEFAULTWIDTH, 0));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2101
 * @tc.name      : Test video size support check with null capability
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2101, TestSize.Level1)
{
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(
        nullptr, DEFAULTWIDTH, DEFAULTHEIGHT));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2200
 * @tc.name      : Test video size support check with dimensions exceeding maximum range
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2200, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange heightRange;
    OH_AVRange widthRange;
    ASSERT_EQ(AV_ERR_OK, OH_AVCapability_GetVideoHeightRange(capability, &heightRange));
    ASSERT_EQ(AV_ERR_OK, OH_AVCapability_GetVideoWidthRange(capability, &widthRange));
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, widthRange.maxVal + 1, heightRange.maxVal + 1));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2201
 * @tc.name      : Test video size support check with dimensions near maximum range
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2201, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange heightRange;
    OH_AVRange widthRange;
    ASSERT_EQ(AV_ERR_OK, OH_AVCapability_GetVideoHeightRange(capability, &heightRange));
    ASSERT_EQ(AV_ERR_OK, OH_AVCapability_GetVideoWidthRange(capability, &widthRange));
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, widthRange.maxVal - 1, heightRange.maxVal - 1));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2300
 * @tc.name      : Test video size support check with valid dimensions
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2300, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(true, OH_AVCapability_IsVideoSizeSupported(capability, DEFAULTWIDTH, DEFAULTHEIGHT));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2400
 * @tc.name      : Test video frame rate range retrieval with null range
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2401
 * @tc.name      : Test video frame rate range retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2401, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange range;
    ret = OH_AVCapability_GetVideoFrameRateRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2500
 * @tc.name      : Test video frame rate range retrieval with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 60);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2600
 * @tc.name      : Test video frame rate range retrieval for size with null range
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, DEFAULTWIDTH, DEFAULTHEIGHT, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2601
 * @tc.name      : Test video frame rate range retrieval for size with null capability
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2601, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange range;
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(nullptr, DEFAULTWIDTH, DEFAULTHEIGHT, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2700
 * @tc.name      : Test video frame rate range retrieval for size with zero width
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2700, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 0, DEFAULTHEIGHT, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2800
 * @tc.name      : Test video frame rate range retrieval for size with zero height
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2800, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, DEFAULTWIDTH, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2801
 * @tc.name      : Test video frame rate range retrieval for size with -1
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2801, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, -1, -1, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2802
 * @tc.name      : Test video frame rate range retrieval for size with 0
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2802, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 0, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2803
 * @tc.name      : Test video frame rate range retrieval for size with 9999
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2803, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 9999, 9999, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_2900
 * @tc.name      : Test video frame rate range retrieval for size with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_2900, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    int32_t width = 1280;
    int32_t height = 720;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, width, height, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 60);

    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, DEFAULTWIDTH, DEFAULTHEIGHT, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 60);
    width = 2048;
    height = 1152;
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, width, height, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 60);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3000
 * @tc.name      : Test video size and frame rate support check with zero width
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3000, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, 0, DEFAULTHEIGHT, 30));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3100
 * @tc.name      : Test video size and frame rate support check with zero height
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3100, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, DEFAULTWIDTH, 0, 30));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3200
 * @tc.name      : Test video size and frame rate support check with zero frame rate
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3200, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, DEFAULTWIDTH, DEFAULTHEIGHT, 0));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3300
 * @tc.name      : Test video size and frame rate support check with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3300, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(true, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, DEFAULTWIDTH, DEFAULTHEIGHT, 30));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3301
 * @tc.name      : Test video size and frame rate support check with invalid parameters
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3301, TestSize.Level1)
{
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(nullptr, DEFAULTWIDTH, DEFAULTHEIGHT, 30));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3400
 * @tc.name      : Test supported pixel formats retrieval with null formats
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    uint32_t pixelFormatNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, nullptr, &pixelFormatNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3500
 * @tc.name      : Test supported pixel formats retrieval with null count
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *pixelFormat = nullptr;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, &pixelFormat, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3501
 * @tc.name      : Test supported pixel formats retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3501, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *pixelFormat = nullptr;
    uint32_t pixelFormatNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(nullptr, &pixelFormat, &pixelFormatNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3502
 * @tc.name      : OH_AVCapability_GetVideoSupportedPixelFormats param correct
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3502, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    g_ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, &g_pixelFormat, &g_pixelFormatNum);
    ASSERT_NE(nullptr, g_pixelFormat);
    ASSERT_EQ(g_pixelFormatNum, 4);
    ASSERT_EQ(AV_ERR_OK, g_ret);
    for (int i = 0; i < g_pixelFormatNum; i++) {
        g_vdec = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
        ASSERT_NE(nullptr, g_vdec);
        g_format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, g_format);
        (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_WIDTH, DEFAULTWIDTH);
        (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_HEIGHT, DEFAULTHEIGHT);
        EXPECT_GE(g_pixelFormat[i], 0);
        (void)OH_AVFormat_SetIntValue(g_format, OH_MD_KEY_PIXEL_FORMAT, g_pixelFormat[i]);
        EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(g_vdec, g_format));
        OH_AVFormat_Destroy(g_format);
        OH_VideoDecoder_Destroy(g_vdec);
        g_format = nullptr;
        g_vdec = nullptr;
    }
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3600
 * @tc.name      : Test supported profiles retrieval with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    uint32_t profileNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, &profiles, &profileNum);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(profileNum, 0);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3601
 * @tc.name      : Test supported profiles retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3601, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    uint32_t profileNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(nullptr, &profiles, &profileNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3602
 * @tc.name      : Test supported profiles retrieval with null profiles
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3602, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    uint32_t profileNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, nullptr, &profileNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3603
 * @tc.name      : Test supported profiles retrieval with null profileNum
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3603, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, &profiles, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3700
 * @tc.name      : Test feature support check with unsupported feature
 * @tc.desc      : api test
 */

HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3700, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsFeatureSupported(capability, VIDEO_LOW_LATENCY));
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3800
 * @tc.name      : Test feature properties retrieval with unsupported feature
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3800, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVFormat *format = OH_AVCapability_GetFeatureProperties(capability, VIDEO_LOW_LATENCY);
    ASSERT_EQ(nullptr, format);
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_3900
 * @tc.name      : Test levels retrieval with unsupported profile
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_3900, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *levels = nullptr;
    uint32_t levelNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedLevelsForProfile(capability, 1, &levels, &levelNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
    ASSERT_EQ(levelNum, 0);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_CAP_API_4000
 * @tc.name      : OH_AVCapability_AreProfileAndLevelSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_RAWVIDEODEC_CAP_API_4000, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreProfileAndLevelSupported(capability, 1, 1));
}

/**
 * @tc.number    : VIDEO_CAPABILITY_CONIFG_1000
 * @tc.name      : set widthRange 、 heightRange  max and min  test
 * @tc.desc      : configure test
 */
HWTEST_F(RawVideo1decApiNdkTest, VIDEO_CAPABILITY_CONIFG_1000, TestSize.Level2)
{
    string codecName = "";
    OH_AVErrCode  ret = AV_ERR_OK;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    memset_s(&widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(&heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability  *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    if (capability == nullptr) {
        return;
    }
    ret = OH_AVCapability_GetVideoWidthRange(capability, &widthRange);
    ret = OH_AVCapability_GetVideoHeightRange(capability, &heightRange);
    cout  << "width minval=" << widthRange.minVal << "   width maxval=" << widthRange.maxVal << endl;
    cout  << "height minval=" << heightRange.minVal << "   height maxval=" << heightRange.maxVal << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_GE(widthRange.minVal, 0);
    ASSERT_GE(widthRange.maxVal, 0);
    ASSERT_GE(heightRange.minVal, 0);
    ASSERT_GE(heightRange.minVal, 0);
    codecName = OH_AVCapability_GetName(capability);
    OH_AVCodec  *vdec = OH_VideoDecoder_CreateByName(codecName.c_str());
    ASSERT_NE(nullptr, vdec);
    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, widthRange.minVal);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, heightRange.maxVal);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, heightRange.minVal);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, widthRange.maxVal);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, widthRange.maxVal+1);
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, widthRange.minVal-1);
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, widthRange.minVal);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, heightRange.minVal-1);
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, heightRange.maxVal+1);
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec, format));
    OH_VideoDecoder_Destroy(vdec);
    OH_AVFormat_Destroy(format);
}
} // namespace