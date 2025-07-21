
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
#include "avcodec_info.h"
#include "avcodec_list.h"

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
OH_AVCapability *cap = nullptr;

fileInfo file_640_480_rgba{"/data/test/media/640_480.rgba", GRAPHIC_PIXEL_FMT_RGBA_8888, 640, 480 };
fileInfo file_1280_536_nv21{"/data/test/media/1280_536_nv21.yuv", GRAPHIC_PIXEL_FMT_YCRCB_420_SP, 1280, 536 };
fileInfo file_1280_720_nv12{"/data/test/media/1280_720_nv12.yuv", GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 1280, 720 };
fileInfo file_1920_816_rgba{"/data/test/media/1920_816.rgba", GRAPHIC_PIXEL_FMT_RGBA_8888, 1920, 816 };
fileInfo file_1920_1080_nv21{"/data/test/media/1920_1080_nv21.yuv", GRAPHIC_PIXEL_FMT_YCRCB_420_SP, 1920, 1080 };
fileInfo file_3840_2160_nv12{"/data/test/media/3840_2160_nv12.yuv", GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 3840, 2160 };
fileInfo file_1280_720_nv12_10bit{"/data/test/media/1280_720_nv12_10bit.yuv", GRAPHIC_PIXEL_FMT_YCBCR_P010, 1280, 720 };
fileInfo file_1080_1920_nv12{"/data/test/media/1080_1920_nv12.yuv", GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 1080, 1920 };
fileInfo file_1280_1280_nv12{"/data/test/media/1280_1280_nv12.yuv", GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 1280, 1280 };

void HwEncInnerFuncNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(g_codecMime.c_str(), true, HARDWARE);
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
bool IsSupportRgba1010102Format()
{
    const int32_t *pixelFormat = nullptr;
    uint32_t pixelFormatNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    if (capability == nullptr) {
        return false;
    }
    int32_t ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, &pixelFormat, &pixelFormatNum);
    if (pixelFormat == nullptr || pixelFormatNum == 0 || ret != AV_ERR_OK) {
        return false;
    }
    for (int i = 0; i < pixelFormatNum; i++) {
        if (pixelFormat[i] == static_cast<int32_t>(AV_PIXEL_FORMAT_RGBA1010102)) {
            return true;
        }
    }
    return false;
}
/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0100
 * @tc.name      : repeat surface h264 encode send eos,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0100, TestSize.Level0)
{
    if (cap != nullptr) {
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
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0200
 * @tc.name      : repeat surface h264 encode send eos,max count 2,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0200, TestSize.Level0)
{
    if (cap != nullptr) {
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
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0300
 * @tc.name      : repeat surface h264 encode send frame,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0300, TestSize.Level0)
{
    if (cap != nullptr) {
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
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0400
 * @tc.name      : repeat surface h264 encode send frame,max count 1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0400, TestSize.Level0)
{
    if (cap != nullptr) {
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
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0500
 * @tc.name      : repeat surface h265 encode send eos,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0500, TestSize.Level0)
{
    if (cap != nullptr) {
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
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0600
 * @tc.name      : repeat surface h265 encode send eos,max count 2,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0600, TestSize.Level0)
{
    if (cap != nullptr) {
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
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0700
 * @tc.name      : repeat surface h265 encode send frame,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0700, TestSize.Level0)
{
    if (cap != nullptr) {
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
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0800
 * @tc.name      : repeat surface h265 encode send frame,max count 1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0800, TestSize.Level0)
{
    if (cap != nullptr) {
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
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0900
 * @tc.name      : repeat surface h265 encode send frame,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_0900, TestSize.Level0)
{
    if (cap != nullptr) {
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
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_1000
 * @tc.name      : repeat surface h264 encode send frame,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_REPEAT_FUNC_1000, TestSize.Level0)
{
    if (cap != nullptr) {
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

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0010
 * @tc.name      : h264 set VIDEO_ENCODE_ENABLE_WATERMARK is 0
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0010, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 300,
        .height = 300,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0010.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = false;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0020
 * @tc.name      : h264 set VIDEO_ENCODE_ENABLE_WATERMARK is 1
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0020, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0020.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0030
 * @tc.name      : h264 before config
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0030, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0030.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0040
 * @tc.name      : h264 after start
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0040, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0040.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vEncSample->SetCustomBuffer(bufferConfig));
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0050
 * @tc.name      : h264 reset-config-SetCustomBuffer
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0050, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0050.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0060
 * @tc.name      : h264 before prepare, after config
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0060, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0060.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->Prepare());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0070
 * @tc.name      : h264 before start, after prepare
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0070, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0070.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Prepare());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0080
 * @tc.name      : h264 two times SetCustomBuffer with different watermark contents
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0080, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 256,
        .height = 144,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0080.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    vEncSample->WATER_MARK_DIR = "/data/test/media/256_144.rgba";
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0090
 * @tc.name      : h264 the watermark transparency is 0% and located in the bottom right corner with 128*72
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0090, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0090.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/128_72_0.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = vEncSample->DEFAULT_WIDTH - bufferConfig.width;
    vEncSample->videoCoordinateY = vEncSample->DEFAULT_HEIGHT - bufferConfig.height;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0100
 * @tc.name      : h264 the watermark transparency is 100% and located in the bottom right corner with 128*72
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0100, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0100.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/128_72_100.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = vEncSample->DEFAULT_WIDTH - bufferConfig.width;
    vEncSample->videoCoordinateY = vEncSample->DEFAULT_HEIGHT - bufferConfig.height;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0110
 * @tc.name      : h264 the watermark transparency is 25% and located in the bottom left corner with 256*144
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0110, TestSize.Level0)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 256,
        .height = 144,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0110.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/256_144.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = vEncSample->DEFAULT_HEIGHT - bufferConfig.height;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0120
 * @tc.name      : h264 the watermark transparency is 25% and located in the bottom right corner with 256*144
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0120, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 256,
        .height = 144,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0120.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/256_144.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = vEncSample->DEFAULT_WIDTH - bufferConfig.width;
    vEncSample->videoCoordinateY = vEncSample->DEFAULT_HEIGHT - bufferConfig.height;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}


/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0130
 * @tc.name      : h264 the watermark transparency is 50% and located in the upper left corner with 200*100
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0130, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 200,
        .height = 100,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0130.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/200_100.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = 0;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0140
 * @tc.name      : h264 the watermark transparency is 50% and located in the upper right corner with 200*100
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0140, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 200,
        .height = 100,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0140.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/200_100.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = vEncSample->DEFAULT_WIDTH - bufferConfig.width;
    vEncSample->videoCoordinateY = 0;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0150
 * @tc.name      : h264 the watermark transparency is 50% and located in the middle with 200*100
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0150, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 200,
        .height = 100,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0150.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/200_100.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = (vEncSample->DEFAULT_WIDTH - bufferConfig.width) / 2;
    vEncSample->videoCoordinateY = (vEncSample->DEFAULT_HEIGHT - bufferConfig.height) / 2;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0160
 * @tc.name      : h264 the watermark transparency is 0% with 1280*720
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0160, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 1280,
        .height = 720,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0160.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/1280_720_0.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = 0;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0170
 * @tc.name      : h264 the watermark transparency is 100% with 1280*720
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0170, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 1280,
        .height = 720,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0170.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/1280_720_100.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = 0;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0180
 * @tc.name      : h265 set VIDEO_ENCODE_ENABLE_WATERMARK is 0
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0180, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0180.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = false;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0190
 * @tc.name      : h265 set VIDEO_ENCODE_ENABLE_WATERMARK is 1
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0190, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0190.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0200
 * @tc.name      : h265 before config
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0200, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0200.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0210
 * @tc.name      : h265 after start
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0210, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0210.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vEncSample->SetCustomBuffer(bufferConfig));
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0220
 * @tc.name      : h265 reset-config-SetCustomBuffer
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0220, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0220.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0230
 * @tc.name      : h265 before prepare, after config
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0230, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0230.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->Prepare());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0240
 * @tc.name      : h265 before start, after prepare
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0240, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0240.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Prepare());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0250
 * @tc.name      : h265 two times SetCustomBuffer with different watermark contents
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0250, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 256,
        .height = 144,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0250.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    vEncSample->WATER_MARK_DIR = "/data/test/media/256_144.rgba";
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0260
 * @tc.name      : h265 the watermark transparency is 0% and located in the bottom right corner with 128*72
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0260, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0260.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/128_72_0.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = vEncSample->DEFAULT_WIDTH - bufferConfig.width;
    vEncSample->videoCoordinateY = vEncSample->DEFAULT_HEIGHT - bufferConfig.height;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0270
 * @tc.name      : h265 the watermark transparency is 100% and located in the bottom right corner with 128*72
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0270, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0270.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/128_72_100.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = vEncSample->DEFAULT_WIDTH - bufferConfig.width;
    vEncSample->videoCoordinateY = vEncSample->DEFAULT_HEIGHT - bufferConfig.height;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0280
 * @tc.name      : h265 the watermark transparency is 25% and located in the bottom left corner with 256*144
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0280, TestSize.Level0)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 256,
        .height = 144,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0280.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/256_144.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = vEncSample->DEFAULT_HEIGHT - bufferConfig.height;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0290
 * @tc.name      : h265 the watermark transparency is 25% and located in the bottom right corner with 256*144
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0290, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 256,
        .height = 144,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0290.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/256_144.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = vEncSample->DEFAULT_WIDTH - bufferConfig.width;
    vEncSample->videoCoordinateY = vEncSample->DEFAULT_HEIGHT - bufferConfig.height;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}


/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0300
 * @tc.name      : h265 the watermark transparency is 50% and located in the upper left corner with 200*100
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0300, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 200,
        .height = 100,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0300.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/200_100.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = 0;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0310
 * @tc.name      : h265 the watermark transparency is 50% and located in the upper right corner with 200*100
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0310, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 200,
        .height = 100,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0310.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/200_100.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = vEncSample->DEFAULT_WIDTH - bufferConfig.width;
    vEncSample->videoCoordinateY = 0;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0320
 * @tc.name      : h265 the watermark transparency is 50% and located in the middle with 200*100
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0320, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 200,
        .height = 100,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0320.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/200_100.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = (vEncSample->DEFAULT_WIDTH - bufferConfig.width) / 2;
    vEncSample->videoCoordinateY = (vEncSample->DEFAULT_HEIGHT - bufferConfig.height) / 2;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0330
 * @tc.name      : h265 the watermark transparency is 0% with 1280*720
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0330, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 1280,
        .height = 720,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0330.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/1280_720_0.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = 0;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0340
 * @tc.name      : h265 the watermark transparency is 100% with 1280*720
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0340, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 1280,
        .height = 720,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0340.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/1280_720_100.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = 0;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0350
 * @tc.name      : h265 the watermark transparency is 50% with 1280*720
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0350, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 1280,
        .height = 720,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0350.h265";
    vEncSample->WATER_MARK_DIR = "/data/test/media/1280_720_50.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = 0;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_WATERMARK_FUNC_0360
 * @tc.name      : h264 the watermark transparency is 50% with 1280*720
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_WATERMARK_FUNC_0360, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 1280,
        .height = 720,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_WATERMARK_FUNC_0360.h264";
    vEncSample->WATER_MARK_DIR = "/data/test/media/1280_720_50.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = 0;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0010
 * @tc.name      : h265 encode config 320_240 surface change 640_480 1280_720 1920_1080
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0010, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 320;
        vEncInnerSample->DEFAULT_HEIGHT = 240;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0010.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->fileInfos.push_back(file_640_480_rgba);
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
        vEncInnerSample->fileInfos.push_back(file_1920_1080_nv21);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0020
 * @tc.name      : h265 encode config 1280_720 surface change 640_480 1280_720 1920_1080
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0020, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 1280;
        vEncInnerSample->DEFAULT_HEIGHT = 720;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0020.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->fileInfos.push_back(file_640_480_rgba);
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
        vEncInnerSample->fileInfos.push_back(file_1920_1080_nv21);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0030
 * @tc.name      : h265 encode config 3840_2160 surface change 640_480 1280_536 1280_720 1920_816 1920_1080 3840_2160
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0030, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0030.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->fileInfos.push_back(file_640_480_rgba);
        vEncInnerSample->fileInfos.push_back(file_1280_536_nv21);
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
        vEncInnerSample->fileInfos.push_back(file_1920_816_rgba);
        vEncInnerSample->fileInfos.push_back(file_1920_1080_nv21);
        vEncInnerSample->fileInfos.push_back(file_3840_2160_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0040
 * @tc.name      : h265 encode config 1920_1080 surface change 1920_1080 1080_1920 1280_1280
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0040, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 1920;
        vEncInnerSample->DEFAULT_HEIGHT = 1080;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0040.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->fileInfos.push_back(file_1920_1080_nv21);
        vEncInnerSample->fileInfos.push_back(file_1080_1920_nv12);
        vEncInnerSample->fileInfos.push_back(file_1280_1280_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0050
 * @tc.name      : h264 encode config 320_240 surface change 640_480 1280_720 1920_1080
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0050, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 320;
        vEncInnerSample->DEFAULT_HEIGHT = 240;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0050.h264";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->fileInfos.push_back(file_640_480_rgba);
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
        vEncInnerSample->fileInfos.push_back(file_1920_1080_nv21);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0060
 * @tc.name      : h264 encode config 1280_720 surface change 640_480 1280_720 1920_1080
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0060, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 1280;
        vEncInnerSample->DEFAULT_HEIGHT = 720;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0060.h264";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->fileInfos.push_back(file_640_480_rgba);
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
        vEncInnerSample->fileInfos.push_back(file_1920_1080_nv21);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0070
 * @tc.name      : h264 encode config 3840_2160 surface change 640_480 1280_536 1280_720 1920_816 1920_1080 3840_2160
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0070, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0070.h264";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->fileInfos.push_back(file_640_480_rgba);
        vEncInnerSample->fileInfos.push_back(file_1280_536_nv21);
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
        vEncInnerSample->fileInfos.push_back(file_1920_816_rgba);
        vEncInnerSample->fileInfos.push_back(file_1920_1080_nv21);
        vEncInnerSample->fileInfos.push_back(file_3840_2160_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0080
 * @tc.name      : h264 encode config 1920_1080 surface change 1920_1080 1080_1920 1280_1280
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0080, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 1920;
        vEncInnerSample->DEFAULT_HEIGHT = 1080;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0080.h264";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->fileInfos.push_back(file_1920_1080_nv21);
        vEncInnerSample->fileInfos.push_back(file_1080_1920_nv12);
        vEncInnerSample->fileInfos.push_back(file_1280_1280_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0090
 * @tc.name      : config main10 set format 8bit surface send 8bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0090, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0090.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->configMain10 = true;
        vEncInnerSample->setFormat8Bit = true;
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0100
 * @tc.name      : config main set format 10bit surface send 10bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0100, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0100.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->configMain = true;
        vEncInnerSample->setFormat10Bit = true;
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12_10bit);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0110
 * @tc.name      : Not supported pixelFormat rgbx
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0110, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0110.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->setFormatRbgx = true;
        vEncInnerSample->fileInfos.push_back(file_640_480_rgba);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0120
 * @tc.name      : config main set format 8bit send 10bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0120, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0120.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->configMain = true;
        vEncInnerSample->setFormat8Bit = true;
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12_10bit);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0130
 * @tc.name      : config main10 set format 10bit send 8bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0130, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0130.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->configMain10 = true;
        vEncInnerSample->setFormat10Bit = true;
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0140
 * @tc.name      : config main10 set format 8bit send 10bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0140, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0140.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->configMain10 = true;
        vEncInnerSample->setFormat8Bit = true;
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12_10bit);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_MULTIFILE_0150
 * @tc.name      : config main10 set format 10bit send 8bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_INNER_MULTIFILE_0150, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
        vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_MULTIFILE_0150.h265";
        vEncInnerSample->surfaceInput = true;
        vEncInnerSample->readMultiFiles = true;
        vEncInnerSample->configMain10 = true;
        vEncInnerSample->setFormat10Bit = true;
        vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_SYNC_FUNC_0340
 * @tc.name      : 264 waterMark encode
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_SYNC_FUNC_0340, TestSize.Level2)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 128,
        .height = 72,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_SYNC_FUNC_0340.h264";
    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 100;
    vEncSample->videoCoordinateY = 100;
    vEncSample->enbleSyncMode = 1;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_SYNC_FUNC_0350
 * @tc.name      : 265 waterMark encode
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_SYNC_FUNC_0350, TestSize.Level2)
{
    auto vEncSample = make_unique<VEncNdkInnerSample>();
    if (!vEncSample->GetWaterMarkCapability(g_codecMime)) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 1280,
        .height = 720,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_SYNC_FUNC_0350.h265";
    vEncSample->WATER_MARK_DIR = "/data/test/media/1280_720_50.rgba";

    vEncSample->surfaceInput = true;
    vEncSample->enableWaterMark = true;
    vEncSample->videoCoordinateX = 0;
    vEncSample->videoCoordinateY = 0;
    vEncSample->enbleSyncMode = 1;
    vEncSample->videoCoordinateWidth = bufferConfig.width;
    vEncSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_RGBA1010102_0100
 * @tc.name      : waterMark rgba1010102 265 surface encode
 * @tc.desc      : func test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_RGBA1010102_0100, TestSize.Level2)
{
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    if (!vEncInnerSample->GetWaterMarkCapability(g_codecMime) || !IsSupportRgba1010102Format()) {
        return;
    }
    BufferRequestConfig bufferConfig = {
        .width = 1280,
        .height = 720,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_rgba1010102.yuv";
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_RGBA1010102_0100.h265";
    vEncInnerSample->WATER_MARK_DIR = "/data/test/media/1280_720_50.rgba";

    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->enableWaterMark = true;
    vEncInnerSample->videoCoordinateX = 0;
    vEncInnerSample->configMain10 = true;
    vEncInnerSample->videoCoordinateY = 0;
    vEncInnerSample->DEFAULT_PIX_FMT = static_cast<int32_t>(VideoPixelFormat::RGBA1010102);
    vEncInnerSample->videoCoordinateWidth = bufferConfig.width;
    vEncInnerSample->videoCoordinateHeight = bufferConfig.height;

    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
    ASSERT_EQ(AVCS_ERR_OK, vEncInnerSample->SetCustomBuffer(bufferConfig));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0010
 * @tc.name      : 264 video cq sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0010, TestSize.Level2)
{
    if (cap != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
        vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0010.h264";
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
        vEncInnerSample->DEFAULT_BITRATE_MODE = CQ;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0020
 * @tc.name      : 264 video cbr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0020, TestSize.Level2)
{
    if (cap != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
        vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0020.h264";
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
        vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0030
 * @tc.name      : 264 video sqr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0030, TestSize.Level2)
{
    if (cap != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
        vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0030.h264";
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
        vEncInnerSample->DEFAULT_BITRATE_MODE = SQR;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0040
 * @tc.name      : 264 video vbr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0040, TestSize.Level2)
{
    if (cap != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
        vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0040.h264";
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
        vEncInnerSample->DEFAULT_BITRATE_MODE = VBR;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0050
 * @tc.name      : 264 video surface sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0050, TestSize.Level2)
{
    if (cap != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
        vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0050.h264";
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
        vEncInnerSample->surfaceInput = true;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0060
 * @tc.name      : 264 video resolution change sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0060, TestSize.Level2)
{
    if (cap != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
		vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0070.264";
		vEncInnerSample->surfaceInput = true;
		vEncInnerSample->readMultiFiles = true;
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
		vEncInnerSample->fileInfos.push_back(file_640_480_rgba);
		vEncInnerSample->fileInfos.push_back(file_1280_536_nv21);
		vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
		vEncInnerSample->fileInfos.push_back(file_1920_816_rgba);
		vEncInnerSample->fileInfos.push_back(file_1920_1080_nv21);
		vEncInnerSample->fileInfos.push_back(file_3840_2160_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0070
 * @tc.name      : 265 video cq sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0070, TestSize.Level2)
{
    if (cap_Hevc != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_Hevc, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
        vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0070.h265";
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
        vEncInnerSample->DEFAULT_BITRATE_MODE = CQ;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0080
 * @tc.name      : 265 video cbr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0080, TestSize.Level2)
{
    if (cap_Hevc != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_Hevc, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
        vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0080.h265";
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
        vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0090
 * @tc.name      : 265 video sqr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0090, TestSize.Level2)
{
    if (cap_Hevc != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_Hevc, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
        vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0090.h265";
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
        vEncInnerSample->DEFAULT_BITRATE_MODE = SQR;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0100
 * @tc.name      : 265 video vbr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0100, TestSize.Level2)
{
    if (cap_Hevc != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_Hevc, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
        vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0100.h265";
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
        vEncInnerSample->DEFAULT_BITRATE_MODE = VBR;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0110
 * @tc.name      : 265 video surface sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0110, TestSize.Level2)
{
    if (cap_Hevc != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_Hevc, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
        vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0110.h265";
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
        vEncInnerSample->surfaceInput = true;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0120
 * @tc.name      : 265 video resolution change sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncNdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0120, TestSize.Level2)
{
    if (cap_Hevc != nullptr) {
		OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_Hevc, VIDEO_ENCODER_B_FRAME);
        if (format == nullptr) {
            return;
        } else {
			OH_AVFormat_Destroy(format);
			format = nullptr;
		}
        auto vEncInnerSample = make_unique<VEncAPI11Sample>();
		vEncInnerSample->DEFAULT_WIDTH = 3840;
        vEncInnerSample->DEFAULT_HEIGHT = 2160;
		vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_INNER_SYNC_0120.h265";
		vEncInnerSample->surfaceInput = true;
		vEncInnerSample->readMultiFiles = true;
		vEncInnerSample->enbleSyncMode = 1;
		vEncInnerSample->enbleBFrameMode = true;
		vEncInnerSample->fileInfos.push_back(file_640_480_rgba);
		vEncInnerSample->fileInfos.push_back(file_1280_536_nv21);
		vEncInnerSample->fileInfos.push_back(file_1280_720_nv12);
		vEncInnerSample->fileInfos.push_back(file_1920_816_rgba);
		vEncInnerSample->fileInfos.push_back(file_1920_1080_nv21);
		vEncInnerSample->fileInfos.push_back(file_3840_2160_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
    }
}
} // namespace