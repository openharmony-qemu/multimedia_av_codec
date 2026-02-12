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
#include <limits>
#include <iostream>
#include <cstdio>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>

#include "meta/format.h"
#include "gtest/gtest.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videoenc_api11_sample.h"
#include "videoenc_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "avcodec_info.h"
#include "avcodec_list.h"
#include "avcodec_common.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace {
OH_AVCodec *venc_ = NULL;
OH_AVCapability *cap = nullptr;
OH_AVCapability *cap_hevc = nullptr;
constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecName[CODEC_NAME_SIZE] = {};
char g_codecNameHEVC[CODEC_NAME_SIZE] = {};

fileInfo file_640_480_rgba{"/data/test/media/640_480.rgba", NATIVEBUFFER_PIXEL_FMT_RGBA_8888, 640, 480 };
fileInfo file_1280_536_nv21{"/data/test/media/1280_536_nv21.yuv", NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, 1280, 536 };
fileInfo file_1280_720_nv12{"/data/test/media/1280_720_nv12.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 1280, 720 };
fileInfo file_1920_816_rgba{"/data/test/media/1920_816.rgba", NATIVEBUFFER_PIXEL_FMT_RGBA_8888, 1920, 816 };
fileInfo file_1920_1080_nv21{"/data/test/media/1920_1080_nv21.yuv", NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, 1920, 1080 };
fileInfo file_3840_2160_nv12{"/data/test/media/3840_2160_nv12.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 3840, 2160 };
} // namespace
namespace OHOS {
namespace Media {
class HwEncFunc3NdkTest : public testing::Test {
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

void HwEncFunc3NdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    const char *tmpCodecNameHevc = OH_AVCapability_GetName(cap_hevc);
    if (memcpy_s(g_codecNameHEVC, sizeof(g_codecNameHEVC), tmpCodecNameHevc, strlen(tmpCodecNameHevc)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname_hevc: " << g_codecNameHEVC << endl;
}
void HwEncFunc3NdkTest::TearDownTestCase() {}
void HwEncFunc3NdkTest::SetUp() {}
void HwEncFunc3NdkTest::TearDown()
{
    if (venc_ != NULL) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
}
namespace {

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0010
 * @tc.name      : 264 video cbr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0010, TestSize.Level2)
{
    if (cap != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0010.h264";
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->DEFAULT_BITRATE_MODE = CBR;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_LT(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0020
 * @tc.name      : 264 video cq sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0020, TestSize.Level2)
{
    if (cap != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0020.h264";
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->DEFAULT_BITRATE_MODE = CQ;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0030
 * @tc.name      : 264 video sqr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0030, TestSize.Level2)
{
    if (cap != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0030.h264";
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->DEFAULT_BITRATE_MODE = SQR;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0040
 * @tc.name      : 264 video vbr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0040, TestSize.Level0)
{
    if (cap != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0040.h264";
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->DEFAULT_BITRATE_MODE = VBR;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0050
 * @tc.name      : 264 video surface sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0050, TestSize.Level0)
{
    if (cap != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0050.h264";
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->SURF_INPUT = true;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0060
 * @tc.name      : 264 video resolution change sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0060, TestSize.Level2)
{
    if (cap != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->DEFAULT_WIDTH = 3840;
            vEncSample->DEFAULT_HEIGHT = 2160;
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0070.h264";
            vEncSample->SURF_INPUT = true;
            vEncSample->readMultiFiles = true;
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->fileInfos.push_back(file_640_480_rgba);
            vEncSample->fileInfos.push_back(file_1280_536_nv21);
            vEncSample->fileInfos.push_back(file_1280_720_nv12);
            vEncSample->fileInfos.push_back(file_1920_816_rgba);
            vEncSample->fileInfos.push_back(file_1920_1080_nv21);
            vEncSample->fileInfos.push_back(file_3840_2160_nv12);
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0070
 * @tc.name      : 265 video CBR sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0070, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_hevc, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0070.h265";
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->DEFAULT_BITRATE_MODE = CBR;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0080
 * @tc.name      : 265 video CQ sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0080, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_hevc, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0080.h265";
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->DEFAULT_BITRATE_MODE = CQ;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0090
 * @tc.name      : 265 video sqr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0090, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_hevc, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0090.h265";
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->DEFAULT_BITRATE_MODE = SQR;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0100
 * @tc.name      : 265 video vbr sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0100, TestSize.Level0)
{
    if (cap_hevc != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_hevc, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0100.h265";
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->DEFAULT_BITRATE_MODE = VBR;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0110
 * @tc.name      : 265 video surface sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0110, TestSize.Level0)
{
    if (cap_hevc != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_hevc, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0110.h265";
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->SURF_INPUT = true;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_BFRAME_SYNC_0120
 * @tc.name      : 265 video resolution change sync encode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_BFRAME_SYNC_0120, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        OH_AVFormat *format =  OH_AVCapability_GetFeatureProperties(cap_hevc, VIDEO_ENCODER_B_FRAME);
        if (format != nullptr) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
            auto vEncSample = make_unique<VEncAPI11Sample>();
            vEncSample->DEFAULT_WIDTH = 3840;
            vEncSample->DEFAULT_HEIGHT = 2160;
            vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_BFRAME_SYNC_0120.h265";
            vEncSample->SURF_INPUT = true;
            vEncSample->readMultiFiles = true;
            vEncSample->enbleSyncMode = 1;
            vEncSample->enbleBFrameMode = 1;
            vEncSample->fileInfos.push_back(file_640_480_rgba);
            vEncSample->fileInfos.push_back(file_1280_536_nv21);
            vEncSample->fileInfos.push_back(file_1280_720_nv12);
            vEncSample->fileInfos.push_back(file_1920_816_rgba);
            vEncSample->fileInfos.push_back(file_1920_1080_nv21);
            vEncSample->fileInfos.push_back(file_3840_2160_nv12);
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_H264_FLUSH_0010
 * @tc.name      : create-start-eos-flush-start-eos
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_H264_FLUSH_0010, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vEncSample->Flush());
        vEncSample->FlushStatus();
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_H265_FLUSH_0010
 * @tc.name      : create-start-eos-flush-start-eos
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_H265_FLUSH_0010, TestSize.Level1)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vEncSample->Flush());
        vEncSample->FlushStatus();
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_H264_FLUSH_0020
 * @tc.name      : create-start-eos-flush-start-eos,surface
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_H264_FLUSH_0020, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        vEncSample->SURF_INPUT = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vEncSample->Flush());
        vEncSample->FlushStatus();
        vEncSample->isSurface = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_H265_FLUSH_0020
 * @tc.name      : create-start-eos-flush-start-eos,surface
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_H265_FLUSH_0020, TestSize.Level1)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        vEncSample->SURF_INPUT = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vEncSample->Flush());
        vEncSample->FlushStatus();
        vEncSample->isSurface = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}


/**
 * @tc.number    : VIDEO_ENCODE_INFO_0010
 * @tc.name      : create-start-stop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0010, TestSize.Level1)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Stop());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0020
 * @tc.name      : create-start-stop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0020, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Stop());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0030
 * @tc.name      : create-start-reset
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0030, TestSize.Level1)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0040
 * @tc.name      : create-start-reset
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0040, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0050
 * @tc.name      : create-start-destroy
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0050, TestSize.Level1)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Release());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0060
 * @tc.name      : create-start-destroy
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0060, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Release());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0070
 * @tc.name      : create-start
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0070, TestSize.Level1)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0080
 * @tc.name      : create-start
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0080, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0090
 * @tc.name      : create-reset
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0090, TestSize.Level1)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0100
 * @tc.name      : create-reset
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0100, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0110
 * @tc.name      : create-destroy
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0110, TestSize.Level1)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Release());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0120
 * @tc.name      : create-destroy
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0120, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Release());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0130
 * @tc.name      : create-Stop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0130, TestSize.Level1)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Stop());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0140
 * @tc.name      : create-Stop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0140, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Stop());
        cout << "pid--" << getprocpid() << "--uid--" << getuid() << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0150
 * @tc.name      : 多编码器start
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0150, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        std::thread Thread1([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            cout << "pid1--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        std::thread Thread2([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            cout << "pid2--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        Thread1.join();
        Thread2.join();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0160
 * @tc.name      : 多编码器 start-stop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0160, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        std::thread Thread1([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Stop());
            cout << "pid1--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        std::thread Thread2([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            cout << "pid2--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        Thread1.join();
        Thread2.join();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0170
 * @tc.name      : 多编码器 start-stop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0170, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        std::thread Thread1([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            cout << "pid1--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        std::thread Thread2([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Stop());
            cout << "pid2--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        Thread1.join();
        Thread2.join();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0180
 * @tc.name      : 多编码器 start-stop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0180, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        std::thread Thread1([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Stop());
            cout << "pid1--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        std::thread Thread2([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Stop());
            cout << "pid2--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        Thread1.join();
        Thread2.join();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0190
 * @tc.name      : 多编码器 start-reset
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0190, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        std::thread Thread1([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
            cout << "pid1--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        std::thread Thread2([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            cout << "pid2--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        Thread1.join();
        Thread2.join();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0200
 * @tc.name      : 多编码器 start-reset
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0200, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        std::thread Thread1([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            cout << "pid1--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        std::thread Thread2([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
            cout << "pid2--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        Thread1.join();
        Thread2.join();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0210
 * @tc.name      : 多编码器 start-reset
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0210, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        std::thread Thread1([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
            cout << "pid1--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        std::thread Thread2([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
            cout << "pid2--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        Thread1.join();
        Thread2.join();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0220
 * @tc.name      : 多编码器 start-Release
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0220, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        std::thread Thread1([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Release());
            cout << "pid1--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        std::thread Thread2([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            cout << "pid2--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        Thread1.join();
        Thread2.join();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0230
 * @tc.name      : 多编码器 start-Release
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0230, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        std::thread Thread1([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            cout << "pid1--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        std::thread Thread2([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Release());
            cout << "pid2--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        Thread1.join();
        Thread2.join();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_INFO_0240
 * @tc.name      : 多编码器 start-Release
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_INFO_0240, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        std::thread Thread1([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Release());
            cout << "pid1--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        std::thread Thread2([this]() {
            auto vEncSample = make_shared<VEncAPI11Sample>();
            vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Release());
            cout << "pid2--" << getprocpid() << "--uid--" << getuid() << endl;
        });
        Thread1.join();
        Thread2.join();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PIXE_FORMAT_0010
 * @tc.name      : 264 pixelformat query key
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_PIXE_FORMAT_0010, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->isGetVideoSupportedPixelFormats = true;
        vEncSample->isGetFormatKey = true;
        vEncSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
        vEncSample->isEncoder = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_LT(1, vEncSample->pixlFormatNum);
        ASSERT_EQ(0, vEncSample->firstCallBackKey);
        ASSERT_EQ(0, vEncSample->onStreamChangedKey);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PIXE_FORMAT_0020
 * @tc.name      : 265 pixelformat query key
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_PIXE_FORMAT_0020, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->isGetVideoSupportedPixelFormats = true;
        vEncSample->isGetFormatKey = true;
        vEncSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        vEncSample->isEncoder = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_LT(1, vEncSample->pixlFormatNum);
        ASSERT_EQ(0, vEncSample->firstCallBackKey);
        ASSERT_EQ(0, vEncSample->onStreamChangedKey);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_ANOTHER_0010
 * @tc.name      : 编码器1正常编码，编码器2状态机轮转
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc3NdkTest, VIDEO_ENCODE_ANOTHER_0010, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        auto vEncSample = make_unique<VEncNdkSample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        auto vEncSample2 = make_unique<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, vEncSample2->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample2->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample2->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample2->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample2->Flush());
        ASSERT_EQ(AV_ERR_OK, vEncSample2->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample2->Stop());
        ASSERT_EQ(AV_ERR_OK, vEncSample2->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample2->Reset());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}
} // namespace