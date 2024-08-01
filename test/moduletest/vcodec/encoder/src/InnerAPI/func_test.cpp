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
#include <iostream>
#include <cstdio>
#include <string>

#include "gtest/gtest.h"
#include "avcodec_common.h"
#include "meta/format.h"
#include "avcodec_video_encoder.h"
#include "videoenc_inner_sample.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
class HwEncInnerFuncNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp() override;
    // TearDown: Called after each test cases
    void TearDown() override;
};

std::string g_codecMime = "video/avc";
std::string g_codecName = "";
std::string g_codecMimeHevc = "video/hevc";
std::string g_codecNameHevc = "";

void HwEncInnerFuncNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(g_codecMime.c_str(), true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    g_codecName = tmpCodecName;
    cout << "g_codecName: " << g_codecName << endl;

    OH_AVCapability *capHevc = OH_AVCodec_GetCapabilityByCategory(g_codecMimeHevc.c_str(), true, HARDWARE);
    const char *tmpCodecNameHevc = OH_AVCapability_GetName(capHevc);
    g_codecNameHevc = tmpCodecNameHevc;
    cout << "g_codecNameHevc: " << g_codecNameHevc << endl;
}

void HwEncInnerFuncNdkTest::TearDownTestCase() {}

void HwEncInnerFuncNdkTest::SetUp()
{
}

void HwEncInnerFuncNdkTest::TearDown()
{
}
} // namespace

namespace {

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0100
 * @tc.name      : repeat surface h264 encode send eos,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0100, TestSize.Level0)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0100.h264";
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableRepeat = true;
    vEncInnerSample->enableSeekEos = true;
    vEncInnerSample->setMaxCount = true;
    vEncInnerSample->DEFAULT_FRAME_AFTER = 73;
    vEncInnerSample->DEFAULT_MAX_COUNT = -1;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    cout << "outCount: " << vEncInnerSample->outCount << endl;
    EXPECT_LE(vEncInnerSample->outCount, 27);
    EXPECT_GE(vEncInnerSample->outCount, 23);
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0200
 * @tc.name      : repeat surface h264 encode send eos,max count 2,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0200, TestSize.Level0)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0200.h264";
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableSeekEos = true;
    vEncInnerSample->enableRepeat = true;
    vEncInnerSample->setMaxCount = true;
    vEncInnerSample->DEFAULT_FRAME_AFTER = 73;
    vEncInnerSample->DEFAULT_MAX_COUNT = 2;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    cout << "outCount: " << vEncInnerSample->outCount << endl;
    ASSERT_EQ(17, vEncInnerSample->outCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0300
 * @tc.name      : repeat surface h264 encode send frame,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0300, TestSize.Level0)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0300.h264";
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableRepeat = true;
    vEncInnerSample->setMaxCount = true;
    vEncInnerSample->DEFAULT_FRAME_AFTER = 73;
    vEncInnerSample->DEFAULT_MAX_COUNT = -1;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    cout << "outCount: " << vEncInnerSample->outCount << endl;
    EXPECT_LE(vEncInnerSample->outCount, 37);
    EXPECT_GE(vEncInnerSample->outCount, 33);
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0400
 * @tc.name      : repeat surface h264 encode send frame,max count 1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0400, TestSize.Level0)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0400.h264";
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableRepeat = true;
    vEncInnerSample->setMaxCount = true;
    vEncInnerSample->DEFAULT_FRAME_AFTER = 73;
    vEncInnerSample->DEFAULT_MAX_COUNT = 1;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    cout << "outCount: " << vEncInnerSample->outCount << endl;
    ASSERT_EQ(26, vEncInnerSample->outCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0500
 * @tc.name      : repeat surface h265 encode send eos,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0500, TestSize.Level0)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0500.h264";
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableRepeat = true;
    vEncInnerSample->enableSeekEos = true;
    vEncInnerSample->setMaxCount = true;
    vEncInnerSample->DEFAULT_FRAME_AFTER = 73;
    vEncInnerSample->DEFAULT_MAX_COUNT = -1;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    cout << "outCount: " << vEncInnerSample->outCount << endl;
    EXPECT_LE(vEncInnerSample->outCount, 27);
    EXPECT_GE(vEncInnerSample->outCount, 23);
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0600
 * @tc.name      : repeat surface h265 encode send eos,max count 2,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0600, TestSize.Level0)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0600.h264";
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableSeekEos = true;
    vEncInnerSample->enableRepeat = true;
    vEncInnerSample->setMaxCount = true;
    vEncInnerSample->DEFAULT_FRAME_AFTER = 73;
    vEncInnerSample->DEFAULT_MAX_COUNT = 2;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    cout << "outCount: " << vEncInnerSample->outCount << endl;
    ASSERT_EQ(17, vEncInnerSample->outCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0700
 * @tc.name      : repeat surface h265 encode send frame,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0700, TestSize.Level0)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0700.h264";
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableRepeat = true;
    vEncInnerSample->setMaxCount = true;
    vEncInnerSample->DEFAULT_FRAME_AFTER = 73;
    vEncInnerSample->DEFAULT_MAX_COUNT = -1;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    cout << "outCount: " << vEncInnerSample->outCount << endl;
    EXPECT_LE(vEncInnerSample->outCount, 37);
    EXPECT_GE(vEncInnerSample->outCount, 33);
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0800
 * @tc.name      : repeat surface h265 encode send frame,max count 1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0800, TestSize.Level0)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0800.h264";
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableRepeat = true;
    vEncInnerSample->setMaxCount = true;
    vEncInnerSample->DEFAULT_FRAME_AFTER = 73;
    vEncInnerSample->DEFAULT_MAX_COUNT = 1;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    cout << "outCount: " << vEncInnerSample->outCount << endl;
    ASSERT_EQ(26, vEncInnerSample->outCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0900
 * @tc.name      : repeat surface h265 encode send frame,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0900, TestSize.Level0)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0900.h264";
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableRepeat = true;
    vEncInnerSample->DEFAULT_FRAME_AFTER = 73;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    cout << "outCount: " << vEncInnerSample->outCount << endl;
    ASSERT_EQ(35, vEncInnerSample->outCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_1000
 * @tc.name      : repeat surface h264 encode send frame,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_1000, TestSize.Level0)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_1000.h264";
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableRepeat = true;
    vEncInnerSample->DEFAULT_FRAME_AFTER = 73;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    cout << "outCount: " << vEncInnerSample->outCount << endl;
    ASSERT_EQ(35, vEncInnerSample->outCount);
}
/**
 * @tc.number    : VIDEO_ENCODE_DISCARD_FUNC_0100
 * @tc.name      : discard the 1th frame with KEY_I_FRAME_INTERVAL 10
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_DISCARD_FUNC_0100, TestSize.Level1)
{
    std::shared_ptr<VEncInnerSignal> vencInnerSignal = std::make_shared<VEncInnerSignal>();
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>(vencInnerSignal);
    std::shared_ptr<VEncParamWithAttrCallbackTest> VencParamWithAttrCallback_
        = std::make_shared<VEncParamWithAttrCallbackTest>(vencInnerSignal);
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DISCARD_FUNC_0100.h264";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->isDiscardFrame = true;
    vEncInnerSample->discardMaxIndex = 1;
    vEncInnerSample->enableRepeat = false;
    vEncInnerSample->discardMinIndex = 1;
    vEncInnerSample->surfaceInput = true;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback(VencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    ASSERT_EQ(true, vEncInnerSample->CheckOutputFrameCount());
}

/**
 * @tc.number    : VIDEO_ENCODE_DISCARD_FUNC_0200
 * @tc.name      : discard the 10th frame with KEY_I_FRAME_INTERVAL 10
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_DISCARD_FUNC_0200, TestSize.Level1)
{
    std::shared_ptr<VEncInnerSignal> vencInnerSignal = std::make_shared<VEncInnerSignal>();
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>(vencInnerSignal);
    std::shared_ptr<VEncParamWithAttrCallbackTest> VencParamWithAttrCallback_
        = std::make_shared<VEncParamWithAttrCallbackTest>(vencInnerSignal);
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DISCARD_FUNC_0200.h264";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->isDiscardFrame = true;
    vEncInnerSample->discardMaxIndex = 10;
    vEncInnerSample->discardMinIndex = 10;
    vEncInnerSample->surfaceInput = true;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback(VencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    ASSERT_EQ(true, vEncInnerSample->CheckOutputFrameCount());
}

/**
 * @tc.number    : VIDEO_ENCODE_DISCARD_FUNC_0300
 * @tc.name      : discard the 11th frame with KEY_I_FRAME_INTERVAL 10
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_DISCARD_FUNC_0300, TestSize.Level1)
{
    std::shared_ptr<VEncInnerSignal> vencInnerSignal = std::make_shared<VEncInnerSignal>();
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>(vencInnerSignal);
    std::shared_ptr<VEncParamWithAttrCallbackTest> VencParamWithAttrCallback_
        = std::make_shared<VEncParamWithAttrCallbackTest>(vencInnerSignal);
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DISCARD_FUNC_0300.h264";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->isDiscardFrame = true;
    vEncInnerSample->discardMaxIndex = 11;
    vEncInnerSample->discardMinIndex = 11;
    vEncInnerSample->surfaceInput = true;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback(VencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    ASSERT_EQ(true, vEncInnerSample->CheckOutputFrameCount());
}

/**
 * @tc.number    : VIDEO_ENCODE_DISCARD_FUNC_0400
 * @tc.name      : random discard with KEY_I_FRAME_INTERVAL 10
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_DISCARD_FUNC_0400, TestSize.Level1)
{
    std::shared_ptr<VEncInnerSignal> vencInnerSignal = std::make_shared<VEncInnerSignal>();
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>(vencInnerSignal);
    std::shared_ptr<VEncParamWithAttrCallbackTest> VencParamWithAttrCallback_
        = std::make_shared<VEncParamWithAttrCallbackTest>(vencInnerSignal);
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DISCARD_FUNC_0400.h264";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->isDiscardFrame = true;
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->PushRandomDiscardIndex(3, 25, 1);
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback(VencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    ASSERT_EQ(true, vEncInnerSample->CheckOutputFrameCount());
}

/**
 * @tc.number    : VIDEO_ENCODE_DISCARD_FUNC_0500
 * @tc.name      : every 3 frames lose 1 frame with KEY_I_FRAME_INTERVAL 10
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_DISCARD_FUNC_0500, TestSize.Level1)
{
    std::shared_ptr<VEncInnerSignal> vencInnerSignal = std::make_shared<VEncInnerSignal>();
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>(vencInnerSignal);
    std::shared_ptr<VEncParamWithAttrCallbackTest> VencParamWithAttrCallback_
        = std::make_shared<VEncParamWithAttrCallbackTest>(vencInnerSignal);
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DISCARD_FUNC_0500.h264";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->isDiscardFrame = true;
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->discardInterval = 3;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback(VencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    ASSERT_EQ(true, vEncInnerSample->CheckOutputFrameCount());
}

/**
 * @tc.number    : VIDEO_ENCODE_DISCARD_FUNC_0600
 * @tc.name      : continuous loss of cache buffer frames with KEY_I_FRAME_INTERVAL 10
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_DISCARD_FUNC_0600, TestSize.Level1)
{
    std::shared_ptr<VEncInnerSignal> vencInnerSignal = std::make_shared<VEncInnerSignal>();
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>(vencInnerSignal);
    std::shared_ptr<VEncParamWithAttrCallbackTest> VencParamWithAttrCallback_
        = std::make_shared<VEncParamWithAttrCallbackTest>(vencInnerSignal);
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DISCARD_FUNC_0600.h264";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->isDiscardFrame = true;
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->discardMaxIndex = 14;
    vEncInnerSample->discardMinIndex = 5;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback(VencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    ASSERT_EQ(true, vEncInnerSample->CheckOutputFrameCount());
}

/**
 * @tc.number    : VIDEO_ENCODE_DISCARD_FUNC_0700
 * @tc.name      : retain the first frame with KEY_I_FRAME_INTERVAL 10
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_DISCARD_FUNC_0700, TestSize.Level1)
{
    std::shared_ptr<VEncInnerSignal> vencInnerSignal = std::make_shared<VEncInnerSignal>();
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>(vencInnerSignal);
    std::shared_ptr<VEncParamWithAttrCallbackTest> VencParamWithAttrCallback_
        = std::make_shared<VEncParamWithAttrCallbackTest>(vencInnerSignal);
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DISCARD_FUNC_0700.h264";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->isDiscardFrame = true;
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->discardMaxIndex = 25;
    vEncInnerSample->discardMinIndex = 2;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback(VencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    ASSERT_EQ(true, vEncInnerSample->CheckOutputFrameCount());
}

/**
 * @tc.number    : VIDEO_ENCODE_DISCARD_FUNC_0800
 * @tc.name      : keep the last frame with KEY_I_FRAME_INTERVAL 10
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_DISCARD_FUNC_0800, TestSize.Level1)
{
    std::shared_ptr<VEncInnerSignal> vencInnerSignal = std::make_shared<VEncInnerSignal>();
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>(vencInnerSignal);
    std::shared_ptr<VEncParamWithAttrCallbackTest> VencParamWithAttrCallback_
        = std::make_shared<VEncParamWithAttrCallbackTest>(vencInnerSignal);
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DISCARD_FUNC_0800.h264";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->isDiscardFrame = true;
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->discardMaxIndex = 24;
    vEncInnerSample->discardMinIndex = 1;
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback(VencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    ASSERT_EQ(true, vEncInnerSample->CheckOutputFrameCount());
}
} // namespace