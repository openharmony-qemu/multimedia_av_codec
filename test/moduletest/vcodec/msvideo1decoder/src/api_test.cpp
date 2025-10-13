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
#include <fcntl.h>

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
class Msvideo1decApiNdkTest : public testing::Test {
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
OH_AVErrCode ret_1 = AV_ERR_OK;
uint32_t pixelFormatNum_1 = 0;
const int32_t *pixelFormat_1 = nullptr;
OH_AVCodec *vdec_ = NULL;
OH_AVCapability *cap = nullptr;
const string INVALID_CODEC_NAME = "avdec_msvideo1";
const string CODEC_NAME = "OH.Media.Codec.Decoder.Video.MSVIDEO1";
VDecAPI11Signal *signal_;
constexpr uint32_t DEFAULT_WIDTH = 720;
constexpr uint32_t DEFAULT_HEIGHT = 480;
constexpr uint32_t DEFAULT_FRAME_RATE = 30;
OH_AVFormat *format;

void Msvideo1decApiNdkTest::SetUpTestCase() {
}
void Msvideo1decApiNdkTest::TearDownTestCase() {
}
void Msvideo1decApiNdkTest::SetUp()
{
    signal_ = new VDecAPI11Signal();
}
void Msvideo1decApiNdkTest::TearDown()
{
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
    if (vdec_ != NULL) {
        OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }
}
} // namespace Media
} // namespace OHOS

namespace {
/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_0100
 * @tc.name      : Test repeat creation of OH_VideoDecoder_CreateByName
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_0100, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(vdec_, NULL);
    OH_AVCodec *vdec_2 = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(vdec_2, NULL);
    OH_VideoDecoder_Destroy(vdec_2);
    vdec_2 = nullptr;
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_0200
 * @tc.name      : Test configuration of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_0200, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(vdec_, format));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_0300
 * @tc.name      : Test start operation of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_0300, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Start(vdec_));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_0400
 * @tc.name      : Test stop operation of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_0400, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(vdec_));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_0500
 * @tc.name      : Test reset operation of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_0500, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec_));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_0600
 * @tc.name      : Test EOS handling in OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_0600, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));

    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;

    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputData(vdec_, 0, attr));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputData(vdec_, 0, attr));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_0700
 * @tc.name      : Test flush operation of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_0700, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(vdec_));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_0800
 * @tc.name      : Test release operation of OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_0800, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Destroy(vdec_));
    vdec_ = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Destroy(vdec_));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_0900
 * @tc.name      : Test creation of OH_VideoDecoder by MIME type
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_0900, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1);
    ASSERT_NE(vdec_, NULL);
    OH_AVCodec *vdec_2 = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1);
    ASSERT_NE(vdec_2, NULL);
    OH_VideoDecoder_Destroy(vdec_2);
    vdec_2 = nullptr;
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_1000
 * @tc.name      : Test callback registration in OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_1000, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVCodecCallback cb_;
    cb_.onError = VdecAPI11Error;
    cb_.onStreamChanged = VdecAPI11FormatChanged;
    cb_.onNeedInputBuffer = VdecAPI11InputDataReady;
    cb_.onNewOutputBuffer = VdecAPI11OutputDataReady;
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(vdec_, cb_, NULL));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(vdec_, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_1001
 * @tc.name      : Repeat OH_VideoDecoder_RegisterSetCallback
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_1001, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVCodecCallback cb_;
    cb_.onError = nullptr;
    cb_.onStreamChanged = VdecAPI11FormatChanged;
    cb_.onNeedInputBuffer = VdecAPI11InputDataReady;
    cb_.onNewOutputBuffer = VdecAPI11OutputDataReady;
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(vdec_, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_1002
 * @tc.name      : Repeat OH_VideoDecoder_RegisterSetCallback
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_1002, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVCodecCallback cb_;
    cb_.onError = VdecAPI11Error;
    cb_.onStreamChanged = VdecAPI11FormatChanged;
    cb_.onNeedInputBuffer = nullptr;
    cb_.onNewOutputBuffer = VdecAPI11OutputDataReady;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(vdec_, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_1003
 * @tc.name      : Repeat OH_VideoDecoder_RegisterSetCallback
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_1003, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVCodecCallback cb_;
    cb_.onError = VdecAPI11Error;
    cb_.onStreamChanged = VdecAPI11FormatChanged;
    cb_.onNeedInputBuffer = VdecAPI11InputDataReady;
    cb_.onNewOutputBuffer = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(vdec_, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_1004
 * @tc.name      : Repeat OH_VideoDecoder_RegisterSetCallback
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_1004, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVCodecCallback cb_;
    cb_.onError = VdecAPI11Error;
    cb_.onStreamChanged = nullptr;
    cb_.onNeedInputBuffer = VdecAPI11InputDataReady;
    cb_.onNewOutputBuffer = VdecAPI11OutputDataReady;
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(vdec_, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_1100
 * @tc.name      : Test output description retrieval in OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_1100, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVFormat *format = OH_VideoDecoder_GetOutputDescription(vdec_);
    ASSERT_NE(NULL, format);
    format = OH_VideoDecoder_GetOutputDescription(vdec_);
    ASSERT_NE(NULL, format);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_API_1200
 * @tc.name      : Test parameter setting in OH_VideoDecoder
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_API_1200, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_SetParameter(vdec_, format));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_SetParameter(vdec_, format));
    OH_AVFormat_Destroy(format);
    format = nullptr;
}

/**
 * @tc.number    :  VIDEO_MSVIDEO1DEC_CAP_API_0001
 * @tc.name      : OH_AVCodec_GetCapability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest,  VIDEO_MSVIDEO1DEC_CAP_API_0001, TestSize.Level2)
{
    cap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false);
    ASSERT_NE(cap, nullptr);
}

/**
 * @tc.number    :  VIDEO_MSVIDEO1DEC_CAP_API_0002
 * @tc.name      : OH_AVCodec_GetCapability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest,  VIDEO_MSVIDEO1DEC_CAP_API_0002, TestSize.Level2)
{
    cap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false);
    string codec_name = OH_AVCapability_GetName(cap);
    ASSERT_EQ(CODEC_NAME, codec_name);
}

/**
 * @tc.number    :  VIDEO_MSVIDEO1DEC_CAP_API_0003
 * @tc.name      : OH_AVCodec_GetCapability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest,  VIDEO_MSVIDEO1DEC_CAP_API_0003, TestSize.Level2)
{
    cap = OH_AVCodec_GetCapability(nullptr, false);
    ASSERT_EQ(cap, nullptr);
}

/**
 * @tc.number    :  VIDEO_MSVIDEO1DEC_CAP_API_0004
 * @tc.name      : OH_AVCodec_GetCapability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest,  VIDEO_MSVIDEO1DEC_CAP_API_0004, TestSize.Level2)
{
    cap = OH_AVCodec_GetCapabilityByCategory(nullptr, false, SOFTWARE);
    ASSERT_EQ(cap, nullptr);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0100
 * @tc.name      : Test capability retrieval for MSVIDEO1 codec
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0100, TestSize.Level1)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(cap, nullptr);
    ASSERT_FALSE(OH_AVCapability_IsHardware(cap));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0101
 * @tc.name      : Test capability retrieval for null capability
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0101, TestSize.Level1)
{
    ASSERT_FALSE(OH_AVCapability_IsHardware(nullptr));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0200
 * @tc.name      : Test max supported instances retrieval for MSVIDEO1 codec
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0200, TestSize.Level1)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(cap, nullptr);
    ASSERT_EQ(64, OH_AVCapability_GetMaxSupportedInstances(cap));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0201
 * @tc.name      : Test max supported instances retrieval for null capability
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0201, TestSize.Level1)
{
    ASSERT_EQ(0, OH_AVCapability_GetMaxSupportedInstances(nullptr));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0300
 * @tc.name      : Test name retrieval for MSVIDEO1 codec
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0300, TestSize.Level1)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(cap, nullptr);
    ASSERT_EQ(CODEC_NAME, OH_AVCapability_GetName(cap));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0301
 * @tc.name      : Test name retrieval for null capability
 * @tc.desc      : function test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0301, TestSize.Level1)
{
    const char *name = OH_AVCapability_GetName(nullptr);
    ASSERT_NE(name, nullptr);
    ASSERT_EQ(strlen(name), 0);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0400
 * @tc.name      : Test video width alignment retrieval with invalid parameter
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthAlignment(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0401
 * @tc.name      : Test video width alignment retrieval with invalid parameter
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0401, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoWidthAlignment(nullptr, &alignment);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0500
 * @tc.name      : Test video width alignment retrieval with valid parameter
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoWidthAlignment(capability, &alignment);
    cout << "WidthAlignment " << alignment << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(alignment, 2);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0600
 * @tc.name      : Test video height alignment retrieval with invalid parameter
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightAlignment(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0601
 * @tc.name      : Test video height alignment retrieval with invalid parameter
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0601, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoHeightAlignment(nullptr, &alignment);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0700
 * @tc.name      : Test video height alignment retrieval with valid parameter
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0700, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoHeightAlignment(capability, &alignment);
    cout << "HeightAlignment " << alignment << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(alignment, 2);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0800
 * @tc.name      : Test video width range retrieval for height with invalid parameter
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0800, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(nullptr, DEFAULT_HEIGHT, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_0900
 * @tc.name      : Test video width range retrieval for height with null range
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_0900, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULT_HEIGHT, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1000
 * @tc.name      : Test video width range retrieval for height with zero height
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1000, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1001
 * @tc.name      : Test video width range retrieval for height with -1 height
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1001, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, -1, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1002
 * @tc.name      : Test video width range retrieval for height with 9999 height
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1002, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, 9999, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1003
 * @tc.name      : Test video width range retrieval for height with 9999 height
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1003, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, 4, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
}


/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1100
 * @tc.name      : Test video width range retrieval for height with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1100, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    const char *codecName = OH_AVCapability_GetName(capability);
    ASSERT_NE(nullptr, codecName);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULT_HEIGHT, &range);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(range.minVal, 4);
    ASSERT_EQ(range.maxVal, 4096);
    vdec_ = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, vdec_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, range.minVal - 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    OH_VideoDecoder_Destroy(vdec_);
    vdec_ = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, vdec_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, range.maxVal + 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1200
 * @tc.name      : Test video height range retrieval for width with invalid parameter
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1200, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(nullptr, DEFAULT_WIDTH, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1300
 * @tc.name      : Test video height range retrieval for width with null range
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1300, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULT_WIDTH, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1400
 * @tc.name      : Test video height range retrieval for width with zero width
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1401
 * @tc.name      : Test video height range retrieval for width with -1 width
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1401, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, -1, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1402
 * @tc.name      : Test video height range retrieval for width with 9999 width
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1402, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, 9999, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1500
 * @tc.name      : Test video height range retrieval for width with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    const char *codecName = OH_AVCapability_GetName(capability);
    ASSERT_NE(nullptr, codecName);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULT_WIDTH, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 4);
    ASSERT_EQ(range.maxVal, 4096);
    vdec_ = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, vdec_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, range.minVal - 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    OH_VideoDecoder_Destroy(vdec_);
    vdec_ = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, vdec_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, range.maxVal + 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1600
 * @tc.name      : Test video width range retrieval with null range
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1601
 * @tc.name      : Test video width range retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1601, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange range;
    ret = OH_AVCapability_GetVideoWidthRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1700
 * @tc.name      : Test video width range retrieval with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1700, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 4);
    ASSERT_EQ(range.maxVal, 4096);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1701
 * @tc.name      : OH_AVCapability_GetVideoWidthRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1701, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(range.minVal, 4);
    ASSERT_EQ(range.maxVal, 4096);
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(nullptr, vdec_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, range.minVal - 1);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(vdec_, format));
    OH_VideoDecoder_Destroy(vdec_);
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(nullptr, vdec_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, range.maxVal + 1);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(vdec_, format));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1800
 * @tc.name      : Test video height range retrieval with null range
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1800, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1801
 * @tc.name      : Test video height range retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1801, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange heightRange;
    ret = OH_AVCapability_GetVideoHeightRange(nullptr, &heightRange);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1802
 * @tc.name      : OH_AVCapability_GetVideoHeightRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1802, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    memset_s(&widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRange(capability, &heightRange);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(heightRange.minVal, 4);
    ASSERT_EQ(heightRange.maxVal, 4096);
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(nullptr, vdec_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, heightRange.minVal - 1);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(vdec_, format));
    OH_VideoDecoder_Destroy(vdec_);
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(nullptr, vdec_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, heightRange.maxVal + 1);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Configure(vdec_, format));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_1900
 * @tc.name      : Test video height range retrieval with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_1900, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    memset_s(&widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(&heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    const char *codecName = OH_AVCapability_GetName(capability);
    ASSERT_NE(nullptr, codecName);
    ret = OH_AVCapability_GetVideoHeightRange(capability, &heightRange);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << heightRange.minVal << "  maxval=" << heightRange.maxVal << endl;
    ASSERT_EQ(heightRange.minVal, 4);
    ASSERT_EQ(heightRange.maxVal, 4096);
    ret = OH_AVCapability_GetVideoWidthRange(capability, &widthRange);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(widthRange.minVal, 4);
    ASSERT_EQ(widthRange.maxVal, 4096);
    cout << "minval=" << widthRange.minVal << "  maxval=" << widthRange.maxVal << endl;
    vdec_ = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, vdec_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, widthRange.minVal - 1);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, heightRange.minVal - 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    OH_VideoDecoder_Destroy(vdec_);
    vdec_ = OH_VideoDecoder_CreateByName(codecName);
    ASSERT_NE(nullptr, vdec_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, widthRange.maxVal + 1);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, heightRange.maxVal + 1);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2000
 * @tc.name      : Test video size support check with zero width
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2000, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, 0, DEFAULT_HEIGHT));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2100
 * @tc.name      : Test video size support check with zero height
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2100, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, DEFAULT_WIDTH, 0));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2101
 * @tc.name      : Test video size support check with null capability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2101, TestSize.Level1)
{
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(
        nullptr, DEFAULT_WIDTH, DEFAULT_HEIGHT));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2200
 * @tc.name      : Test video size support check with out-of-range dimensions
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2200, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange heightRange;
    OH_AVRange widthRange;
    ASSERT_EQ(AV_ERR_OK, OH_AVCapability_GetVideoHeightRange(capability, &heightRange));
    ASSERT_EQ(AV_ERR_OK, OH_AVCapability_GetVideoWidthRange(capability, &widthRange));
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, widthRange.maxVal + 1, heightRange.maxVal + 1));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2201
 * @tc.name      : Test video size support check with out-of-range dimensions
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2201, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange heightRange;
    OH_AVRange widthRange;
    ASSERT_EQ(AV_ERR_OK, OH_AVCapability_GetVideoHeightRange(capability, &heightRange));
    ASSERT_EQ(AV_ERR_OK, OH_AVCapability_GetVideoWidthRange(capability, &widthRange));
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, widthRange.maxVal - 1, heightRange.maxVal - 1));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2300
 * @tc.name      : Test video size support check with valid dimensions
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2300, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(true, OH_AVCapability_IsVideoSizeSupported(capability, DEFAULT_WIDTH, DEFAULT_HEIGHT));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2400
 * @tc.name      : Test video frame rate range retrieval with null range
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2401
 * @tc.name      : Test video frame rate range retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2401, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange range;
    ret = OH_AVCapability_GetVideoFrameRateRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2500
 * @tc.name      : Test video frame rate range retrieval with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 60);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2600
 * @tc.name      : Test video frame rate range retrieval for size with null range
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, DEFAULT_WIDTH, DEFAULT_HEIGHT, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2601
 * @tc.name      : Test video frame rate range retrieval for size with null capability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2601, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange range;
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(nullptr, DEFAULT_WIDTH, DEFAULT_HEIGHT, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2700
 * @tc.name      : Test video frame rate range retrieval for size with zero width
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2700, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 0, DEFAULT_HEIGHT, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2800
 * @tc.name      : Test video frame rate range retrieval for size with zero height
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2800, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, DEFAULT_WIDTH, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2801
 * @tc.name      : Test video frame rate range retrieval for size with -1
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2801, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, -1, -1, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2802
 * @tc.name      : Test video frame rate range retrieval for size with 0
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2802, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 0, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2803
 * @tc.name      : Test video frame rate range retrieval for size with 9999
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2803, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 9999, 9999, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_2900
 * @tc.name      : Test video frame rate range retrieval for size with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_2900, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    int32_t width = 1280;
    int32_t height = 720;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, width, height, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 0);
    ASSERT_EQ(range.maxVal, 60);

    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, DEFAULT_WIDTH, DEFAULT_HEIGHT, &range);
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
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3000
 * @tc.name      : Test video size and frame rate support check with zero width
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3000, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, 0, DEFAULT_HEIGHT, 30));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3100
 * @tc.name      : Test video size and frame rate support check with zero height
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3100, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, DEFAULT_WIDTH, 0, 30));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3200
 * @tc.name      : Test video size and frame rate support check with zero frame rate
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3200, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, DEFAULT_WIDTH, DEFAULT_HEIGHT, 0));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3300
 * @tc.name      : Test video size and frame rate support check with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3300, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(true, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, DEFAULT_WIDTH, DEFAULT_HEIGHT, 30));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3301
 * @tc.name      : Test video size and frame rate support check with invalid parameters
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3301, TestSize.Level1)
{
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(nullptr, DEFAULT_WIDTH, DEFAULT_HEIGHT, 30));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3302
 * @tc.name      : Test video size and frame rate support check with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3302, TestSize.Level1)
{
    double DEFAULT_FRAME_RATE = 30.0;
    int32_t DEFAULT_HEIGHT = 1920;
    int32_t DEFAULT_WIDTH = 1080;
    for (int i = 0; i < 100; i++) {
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
            OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
        ASSERT_NE(nullptr, capability);
        DEFAULT_HEIGHT = HighRand();
        usleep(1500);
        DEFAULT_WIDTH = WidthRand();
        usleep(1500);
        DEFAULT_FRAME_RATE = FrameRand();
        usleep(1500);
        ASSERT_EQ(true, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability,
             DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_FRAME_RATE));
    }
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3400
 * @tc.name      : Test supported pixel formats retrieval with null formats
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    uint32_t pixelFormatNum = 0;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, nullptr, &pixelFormatNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3500
 * @tc.name      : Test supported pixel formats retrieval with null count
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *pixelFormat = nullptr;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, &pixelFormat, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3501
 * @tc.name      : Test supported pixel formats retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3501, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *pixelFormat = nullptr;
    uint32_t pixelFormatNum = 0;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(nullptr, &pixelFormat, &pixelFormatNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3502
 * @tc.name      : OH_AVCapability_GetVideoSupportedPixelFormats param correct
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3502, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret_1 = OH_AVCapability_GetVideoSupportedPixelFormats(capability, &pixelFormat_1, &pixelFormatNum_1);
    ASSERT_NE(nullptr, pixelFormat_1);
    ASSERT_EQ(pixelFormatNum_1, 3);
    ASSERT_EQ(AV_ERR_OK, ret_1);
    for (int i = 0; i < pixelFormatNum_1; i++) {
        vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
        ASSERT_NE(nullptr, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        EXPECT_GE(pixelFormat_1[i], 0);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, pixelFormat_1[i]);
        EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
        OH_AVFormat_Destroy(format);
        OH_VideoDecoder_Destroy(vdec_);
        format = nullptr;
        vdec_ = nullptr;
    }
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3600
 * @tc.name      : Test supported profiles retrieval with valid parameters
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    uint32_t profileNum = 0;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, &profiles, &profileNum);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(profileNum, 0);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3601
 * @tc.name      : Test supported profiles retrieval with null capability
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3601, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    uint32_t profileNum = 0;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(nullptr, &profiles, &profileNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3602
 * @tc.name      : Test supported profiles retrieval with null profiles
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3602, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    uint32_t profileNum = 0;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, nullptr, &profileNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3603
 * @tc.name      : Test supported profiles retrieval with null profileNum
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3603, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, &profiles, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3700
 * @tc.name      : Test feature support check with unsupported feature
 * @tc.desc      : api test
 */

HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3700, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsFeatureSupported(capability, VIDEO_LOW_LATENCY));
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3800
 * @tc.name      : Test feature properties retrieval with unsupported feature
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3800, TestSize.Level1)
{
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVFormat *format = OH_AVCapability_GetFeatureProperties(capability, VIDEO_LOW_LATENCY);
    ASSERT_EQ(nullptr, format);
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_3900
 * @tc.name      : Test levels retrieval with unsupported profile
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_3900, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *levels = nullptr;
    uint32_t levelNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedLevelsForProfile(capability, 1, &levels, &levelNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
    ASSERT_EQ(levelNum, 0);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_CAP_API_4000
 * @tc.name      : OH_AVCapability_AreProfileAndLevelSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(Msvideo1decApiNdkTest, VIDEO_MSVIDEO1DEC_CAP_API_4000, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreProfileAndLevelSupported(capability, 1, 1));
}
} // namespace