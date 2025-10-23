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

#ifdef SUPPORT_DRM
#include "native_mediakeysession.h"
#include "native_mediakeysystem.h"
#endif

#define PIXFORMAT_NUM 4

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace OHOS {
namespace Media {
class WMV3SwdecApiNdkTest : public testing::Test {
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

OH_AVCodec *vdec_ = NULL;
OH_AVCapability *cap = nullptr;
const string INVALID_CODEC_NAME = "avdec_WMV3";
const string CODEC_NAME = "OH.Media.Codec.Decoder.Video.WMV3";
VDecAPI11Signal *signal_;
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
constexpr uint32_t DEFAULT_FRAME_RATE = 24;
constexpr uint8_t WMV3_HDR_EXTRADATA[] = {0x4b, 0xf1, 0x0a, 0x01};
constexpr uint32_t WMV3_HDR_EXTRDATA_SIZE = sizeof(WMV3_HDR_EXTRADATA);
OH_AVFormat *format;
void WMV3SwdecApiNdkTest::SetUpTestCase() {}
void WMV3SwdecApiNdkTest::TearDownTestCase() {}
void WMV3SwdecApiNdkTest::SetUp()
{
    signal_ = new VDecAPI11Signal();
}
void WMV3SwdecApiNdkTest::TearDown()
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
 * @tc.number    : VIDEO_WMV3SWDEC_API_0100
 * @tc.name      : repeat create OH_VideoDecoder_CreateByName
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_0100, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(vdec_, NULL);
    OH_AVCodec *vdec_2 = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(vdec_2, NULL);
    OH_VideoDecoder_Destroy(vdec_2);
    vdec_2 = nullptr;
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_API_0200
 * @tc.name      : create configure configure
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_0200, TestSize.Level2)
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
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_API_0300
 * @tc.name      : create configure start start
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_0300, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, WMV3_HDR_EXTRADATA, WMV3_HDR_EXTRDATA_SIZE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Start(vdec_));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_API_0400
 * @tc.name      : create configure start stop stop
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_0400, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, WMV3_HDR_EXTRADATA, WMV3_HDR_EXTRDATA_SIZE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(vdec_));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_API_0500
 * @tc.name      : create configure start stop reset reset
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_0500, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, WMV3_HDR_EXTRADATA, WMV3_HDR_EXTRDATA_SIZE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec_));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_API_0600
 * @tc.name      : create configure start EOS EOS
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_0600, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, WMV3_HDR_EXTRADATA, WMV3_HDR_EXTRDATA_SIZE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));

    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputBuffer(vdec_, 0));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputBuffer(vdec_, 0));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_API_0700
 * @tc.name      : create configure start flush flush
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_0700, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, WMV3_HDR_EXTRADATA, WMV3_HDR_EXTRDATA_SIZE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(vdec_));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_API_0800
 * @tc.name      : create configure start stop release release
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_0800, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(NULL, vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, WMV3_HDR_EXTRADATA, WMV3_HDR_EXTRDATA_SIZE);

    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Stop(vdec_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Destroy(vdec_));
    vdec_ = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_Destroy(vdec_));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_API_0900
 * @tc.name      : create create
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_0900, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_WMV3);
    ASSERT_NE(vdec_, NULL);
    OH_AVCodec *vdec_2 = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_WMV3);
    ASSERT_NE(vdec_2, NULL);
    OH_VideoDecoder_Destroy(vdec_2);
    vdec_2 = nullptr;
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_API_1000
 * @tc.name      : repeat OH_VideoDecoder_RegisterSetCallback
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_1000, TestSize.Level2)
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
 * @tc.number    : VIDEO_WMV3SWDEC_API_1100
 * @tc.name      : repeat OH_VideoDecoder_GetOutputDescription
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_1100, TestSize.Level2)
{
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    OH_AVFormat *format = OH_VideoDecoder_GetOutputDescription(vdec_);
    ASSERT_NE(NULL, format);
    format = OH_VideoDecoder_GetOutputDescription(vdec_);
    ASSERT_NE(NULL, format);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_API_1200
 * @tc.name      : repeat OH_VideoDecoder_SetParameter
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_API_1200, TestSize.Level2)
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
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_0100
 * @tc.name      : OH_AVCodec_GetCapability param correct
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_0100, TestSize.Level1)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(cap, nullptr);
    ASSERT_FALSE(OH_AVCapability_IsHardware(cap));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_0200
 * @tc.name      : OH_AVCapability_GetMaxSupportedInstances param error
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_0200, TestSize.Level1)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(cap, nullptr);
    ASSERT_EQ(0, OH_AVCapability_GetMaxSupportedInstances(nullptr));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_0300
 * @tc.name      : OH_AVCapability_GetMaxSupportedInstances param correct
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_0300, TestSize.Level1)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(cap, nullptr);
    ASSERT_EQ(64, OH_AVCapability_GetMaxSupportedInstances(cap));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_0400
 * @tc.name      : OH_AVCapability_GetName param error
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_0400, TestSize.Level1)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(cap, nullptr);
    ASSERT_NE(CODEC_NAME, OH_AVCapability_GetName(nullptr));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_0500
 * @tc.name      : OH_AVCapability_GetName param correct
 * @tc.desc      : function test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_0500, TestSize.Level1)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(cap, nullptr);
    ASSERT_EQ(CODEC_NAME, OH_AVCapability_GetName(cap));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_0600
 * @tc.name      : OH_AVCapability_GetVideoWidthAlignment param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_0600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthAlignment(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_0700
 * @tc.name      : OH_AVCapability_GetVideoWidthAlignment param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_0700, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoWidthAlignment(capability, &alignment);
    cout << "WidthAlignment " << alignment << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(alignment, 2);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_0800
 * @tc.name      : OH_AVCapability_GetVideoHeightAlignment param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_0800, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightAlignment(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_0900
 * @tc.name      : OH_AVCapability_GetVideoHeightAlignment param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_0900, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoHeightAlignment(capability, &alignment);
    cout << "HeightAlignment " << alignment << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(alignment, 2);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_1000
 * @tc.name      : OH_AVCapability_GetVideoWidthRangeForHeight param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_1000, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(nullptr, DEFAULT_HEIGHT, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_1100
 * @tc.name      : OH_AVCapability_GetVideoWidthRangeForHeight param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_1100, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULT_HEIGHT, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_1200
 * @tc.name      : OH_AVCapability_GetVideoWidthRangeForHeight param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_1200, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_1300
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_1300, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULT_HEIGHT, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 4096);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_1400
 * @tc.name      : OH_AVCapability_GetVideoWidthRangeForHeight param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_1400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    const char *codecName = OH_AVCapability_GetName(capability);
    ASSERT_NE(nullptr, codecName);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULT_HEIGHT, &range);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(2, range.minVal);
    ASSERT_EQ(4096, range.maxVal);
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
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_1500
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_1500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(nullptr, DEFAULT_WIDTH, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_1600
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_1600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULT_WIDTH, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_1700
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_1700, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_1800
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_1800, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULT_WIDTH, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 4096);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_1900
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_1900, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    const char *codecName = OH_AVCapability_GetName(capability);
    ASSERT_NE(nullptr, codecName);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULT_WIDTH, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(2, range.minVal);
    ASSERT_EQ(4096, range.maxVal);
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
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_2000
 * @tc.name      : OH_AVCapability_GetVideoWidthRange param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_2000, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_2100
 * @tc.name      : OH_AVCapability_GetVideoWidthRange param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_2100, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_2200
 * @tc.name      : OH_AVCapability_GetVideoWidthRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_2200, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 4096);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_2300
 * @tc.name      : OH_AVCapability_GetVideoHeightRange param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_2300, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_2400
 * @tc.name      : OH_AVCapability_GetVideoHeightRange param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_2400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_2500
 * @tc.name      : OH_AVCapability_GetVideoHeightRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_2500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(range.minVal, 2);
    ASSERT_EQ(range.maxVal, 4096);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_2600
 * @tc.name      : OH_AVCapability_GetVideoHeightRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_2600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    memset_s(&widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(&heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    const char *codecName = OH_AVCapability_GetName(capability);
    ASSERT_NE(nullptr, codecName);
    ret = OH_AVCapability_GetVideoHeightRange(capability, &heightRange);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << heightRange.minVal << "  maxval=" << heightRange.maxVal << endl;
    ret = OH_AVCapability_GetVideoWidthRange(capability, &widthRange);
    ASSERT_EQ(AV_ERR_OK, ret);
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
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_2700
 * @tc.name      : OH_AVCapability_IsVideoSizeSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_2700, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, 0, DEFAULT_HEIGHT));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_2800
 * @tc.name      : OH_AVCapability_IsVideoSizeSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_2800, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, DEFAULT_WIDTH, 0));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_2900
 * @tc.name      : OH_AVCapability_IsVideoSizeSupported param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_2900, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVRange heightRange;
    OH_AVRange widthRange;
    ASSERT_EQ(AV_ERR_OK, OH_AVCapability_GetVideoHeightRange(capability, &heightRange));
    ASSERT_EQ(AV_ERR_OK, OH_AVCapability_GetVideoWidthRange(capability, &widthRange));
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, widthRange.maxVal + 1, heightRange.maxVal + 1));
}
/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_3000
 * @tc.name      : OH_AVCapability_IsVideoSizeSupported param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_3000, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(true, OH_AVCapability_IsVideoSizeSupported(capability, DEFAULT_WIDTH, DEFAULT_HEIGHT));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_3100
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRange param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_3100, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_3200
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_3200, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_GE(range.minVal, 0);
    ASSERT_GT(range.maxVal, 0);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_3300
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRangeForSize param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_3300, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, DEFAULT_WIDTH, DEFAULT_HEIGHT, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_3400
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRangeForSize param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_3400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 0, DEFAULT_HEIGHT, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_3500
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRangeForSize param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_3500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, DEFAULT_WIDTH, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_3600
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRangeForSize param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_3600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    int32_t width = 1280;
    int32_t height = 720;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, width, height, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_GE(range.minVal, 0);
    ASSERT_GT(range.maxVal, 0);

    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, DEFAULT_WIDTH, DEFAULT_HEIGHT, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_GE(range.minVal, 0);
    ASSERT_GT(range.maxVal, 0);
    width = 2048;
    height = 1152;
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, width, height, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_GE(range.minVal, 0);
    ASSERT_GT(range.maxVal, 0);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_3700
 * @tc.name      : OH_AVCapability_AreVideoSizeAndFrameRateSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_3700, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, 0, DEFAULT_HEIGHT, 30));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_3800
 * @tc.name      : OH_AVCapability_AreVideoSizeAndFrameRateSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_3800, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, DEFAULT_WIDTH, 0, 30));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_3900
 * @tc.name      : OH_AVCapability_AreVideoSizeAndFrameRateSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_3900, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, DEFAULT_WIDTH, DEFAULT_HEIGHT, 0));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_4000
 * @tc.name      : OH_AVCapability_AreVideoSizeAndFrameRateSupported param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_4000, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(true, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, DEFAULT_WIDTH, DEFAULT_HEIGHT, 30));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_4100
 * @tc.name      : OH_AVCapability_GetVideoSupportedPixelFormats param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_4100, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    uint32_t pixelFormatNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, nullptr, &pixelFormatNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_4200
 * @tc.name      : OH_AVCapability_GetVideoSupportedPixelFormats param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_4200, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *pixelFormat = nullptr;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, &pixelFormat, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_4300
 * @tc.name      : OH_AVCapability_GetVideoSupportedPixelFormats param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_4300, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *pixelFormat = nullptr;
    uint32_t pixelFormatNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, &pixelFormat, &pixelFormatNum);
    ASSERT_NE(nullptr, pixelFormat);
    ASSERT_EQ(pixelFormatNum, PIXFORMAT_NUM);
    ASSERT_EQ(AV_ERR_OK, ret);
    for (int i = 0; i < pixelFormatNum; i++) {
        vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
        ASSERT_NE(nullptr, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        EXPECT_EQ(pixelFormat[i], i == pixelFormatNum - 1 ? i + 2 : i + 1);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, pixelFormat[i]);
        EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
        OH_AVFormat_Destroy(format);
        OH_VideoDecoder_Destroy(vdec_);
    }
    vdec_ = OH_VideoDecoder_CreateByName(CODEC_NAME.c_str());
    ASSERT_NE(nullptr, vdec_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_RGBA + AV_PIXEL_FORMAT_RGBA);
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_4400
 * @tc.name      : OH_AVCapability_GetSupportedProfiles param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_4400, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, &profiles, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_4500
 * @tc.name      : OH_AVCapability_GetSupportedProfiles param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_4500, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    uint32_t profileNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, nullptr, &profileNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_4600
 * @tc.name      : OH_AVCapability_GetSupportedProfiles param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_4600, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    uint32_t profileNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(nullptr, &profiles, &profileNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
    ASSERT_EQ(profileNum, 0);
    ASSERT_EQ(nullptr, profiles);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_4700
 * @tc.name      : OH_AVCapability_GetSupportedProfiles param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_4700, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    uint32_t profileNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, &profiles, &profileNum);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_GT(profileNum, 0);
    ASSERT_NE(nullptr, profiles);
    for (int i = 0; i < profileNum; i++) {
        EXPECT_GE(profiles[i], 0);
    }
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_4800
 * @tc.name      : OH_AVCapability_GetSupportedLevelsForProfile param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_4800, TestSize.Level1)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *levels = nullptr;
    uint32_t levelNum = 0;
    const int32_t *profiles = nullptr;
    uint32_t profileNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, &profiles, &profileNum);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_GT(profileNum, 0);
    ASSERT_NE(nullptr, profiles);
    for (int i = 0; i < profileNum; i++) {
        EXPECT_GE(profiles[i], 0);
        ret = OH_AVCapability_GetSupportedLevelsForProfile(capability, profiles[i], &levels, &levelNum);
        ASSERT_EQ(AV_ERR_OK, ret);
        ASSERT_NE(nullptr, levels);
        EXPECT_GT(levelNum, 0);
        for (int j = 0; j < levelNum; j++) {
            EXPECT_GE(levels[j], 0);
        }
    }
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_4900
 * @tc.name      : OH_AVCapability_AreProfileAndLevelSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_4900, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t level = WMV3_LEVEL_HIGH + 1;
    ASSERT_EQ(false, OH_AVCapability_AreProfileAndLevelSupported(capability, WMV3_PROFILE_MAIN, level));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_5000
 * @tc.name      : OH_AVCapability_AreProfileAndLevelSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_5000, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t level = WMV3_LEVEL_HIGH;
    ASSERT_EQ(false, OH_AVCapability_AreProfileAndLevelSupported(capability, WMV3_PROFILE_SIMPLE, level));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_5100
 * @tc.name      : OH_AVCapability_AreProfileAndLevelSupported param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_5100, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t level = WMV3_LEVEL_MEDIUM;
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(capability, WMV3_PROFILE_SIMPLE, level));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_5200
 * @tc.name      : OH_AVCapability_AreProfileAndLevelSupported param correct
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_5200, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    int32_t level = WMV3_LEVEL_MEDIUM;
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(capability, WMV3_PROFILE_MAIN, level));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_5300
 * @tc.name      : OH_AVCapability_IsFeatureSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_5300, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsFeatureSupported(capability, VIDEO_LOW_LATENCY));
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_CAP_API_5400
 * @tc.name      : OH_AVCapability_GetFeatureProperties param error
 * @tc.desc      : api test
 */
HWTEST_F(WMV3SwdecApiNdkTest, VIDEO_WMV3SWDEC_CAP_API_5400, TestSize.Level1)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    OH_AVFormat *format = OH_AVCapability_GetFeatureProperties(capability, VIDEO_LOW_LATENCY);
    ASSERT_EQ(nullptr, format);
    OH_AVFormat_Destroy(format);
}
} // namespace