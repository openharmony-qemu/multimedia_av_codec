
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
class HwEncInnerFunc2NdkTest : public testing::Test {
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

void HwEncInnerFunc2NdkTest::SetUpTestCase()
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

void HwEncInnerFunc2NdkTest::TearDownTestCase() {}

void HwEncInnerFunc2NdkTest::SetUp()
{
}

void HwEncInnerFunc2NdkTest::TearDown()
{
}
} // namespace

namespace {
/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0010
 * @tc.name      : 264 video cq sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0010, TestSize.Level2)
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
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0020, TestSize.Level2)
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
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0030, TestSize.Level2)
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
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0040, TestSize.Level0)
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
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0050, TestSize.Level0)
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
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0060, TestSize.Level2)
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
 * @tc.name      : 265 video CBR sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0070, TestSize.Level2)
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
        vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->Configure());
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
        vEncInnerSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_INNER_SYNC_0080
 * @tc.name      : 265 video CQ sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0080, TestSize.Level2)
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
        vEncInnerSample->DEFAULT_BITRATE_MODE = CQ;
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
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0090, TestSize.Level2)
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
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0100, TestSize.Level0)
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
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0110, TestSize.Level0)
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
HWTEST_F(HwEncInnerFunc2NdkTest, VIDEO_ENCODE_BFRAME_INNER_SYNC_0120, TestSize.Level2)
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