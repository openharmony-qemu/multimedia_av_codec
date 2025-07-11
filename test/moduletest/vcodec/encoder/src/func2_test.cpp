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
#include <limits>
#include "meta/format.h"
#include "gtest/gtest.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videoenc_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "avcodec_info.h"
#include "avcodec_list.h"
#include "avcodec_common.h"

namespace {
OH_AVCodec *venc_ = NULL;
OH_AVCapability *cap = nullptr;
OH_AVCapability *cap_hevc = nullptr;
constexpr uint32_t CODEC_NAME_SIZE = 128;
constexpr int32_t FRAME_AFTER = 73;
constexpr uint32_t COUNT_TWENTY_SEVEN = 27;
constexpr uint32_t COUNT_TWENTY_THREE = 23;
constexpr uint32_t COUNT_SEVENTEEN = 17;
constexpr uint32_t COUNT_THIRTY_SEVEN = 37;
constexpr uint32_t COUNT_THIRTY_THREE = 33;
constexpr uint32_t COUNT_TWENTY_SIX = 26;
constexpr uint32_t COUNT_THIRTY_FIVE = 35;
constexpr int32_t MAX_COUNT = 2;
char g_codecName[CODEC_NAME_SIZE] = {};
char g_codecNameHEVC[CODEC_NAME_SIZE] = {};
} // namespace
namespace OHOS {
namespace Media {
class HwEncFunc2NdkTest : public testing::Test {
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

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

fileInfo file_640_480_rgba{"/data/test/media/640_480.rgba", NATIVEBUFFER_PIXEL_FMT_RGBA_8888, 640, 480 };
fileInfo file_1280_536_nv21{"/data/test/media/1280_536_nv21.yuv", NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, 1280, 536 };
fileInfo file_1280_720_nv12{"/data/test/media/1280_720_nv12.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 1280, 720 };
fileInfo file_1280_720_nv{"/data/test/media/1280_720_nv.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 1280, 720 };
fileInfo file_1920_816_rgba{"/data/test/media/1920_816.rgba", NATIVEBUFFER_PIXEL_FMT_RGBA_8888, 1920, 816 };
fileInfo file_1920_1080_nv21{"/data/test/media/1920_1080_nv21.yuv", NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, 1920, 1080 };
fileInfo file_3840_2160_nv12{"/data/test/media/3840_2160_nv12.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 3840, 2160 };
fileInfo file_1280_720_nv12_10bit{"/data/test/media/1280_720_nv12_10bit.yuv",
    NATIVEBUFFER_PIXEL_FMT_YCBCR_P010, 1280, 720 };
fileInfo file_1080_1920_nv12{"/data/test/media/1080_1920_nv12.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 1080, 1920 };
fileInfo file_1280_1280_nv12{"/data/test/media/1280_1280_nv12.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 1280, 1280 };

void HwEncFunc2NdkTest::SetUpTestCase()
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
void HwEncFunc2NdkTest::TearDownTestCase() {}
void HwEncFunc2NdkTest::SetUp() {}
void HwEncFunc2NdkTest::TearDown()
{
    if (venc_ != NULL) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
}
namespace {
/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0010
 * @tc.name      : h265 encode config 320_240 surface change 640_480 1280_720 1920_1080
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0010, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 320;
        vEncSample->DEFAULT_HEIGHT = 240;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0010.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->fileInfos.push_back(file_640_480_rgba);
        vEncSample->fileInfos.push_back(file_1280_720_nv12);
        vEncSample->fileInfos.push_back(file_1920_1080_nv21);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0020
 * @tc.name      : h265 encode config 1280_720 surface change 640_480 1280_720 1920_1080
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0020, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0020.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->fileInfos.push_back(file_640_480_rgba);
        vEncSample->fileInfos.push_back(file_1280_720_nv12);
        vEncSample->fileInfos.push_back(file_1920_1080_nv21);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0030
 * @tc.name      : h265 encode config 3840_2160 surface change 640_480 1280_536 1280_720 1920_816 1920_1080 3840_2160
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0030, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 3840;
        vEncSample->DEFAULT_HEIGHT = 2160;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0030.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->fileInfos.push_back(file_640_480_rgba);
        vEncSample->fileInfos.push_back(file_1280_536_nv21);
        vEncSample->fileInfos.push_back(file_1280_720_nv12);
        vEncSample->fileInfos.push_back(file_1920_816_rgba);
        vEncSample->fileInfos.push_back(file_1920_1080_nv21);
        vEncSample->fileInfos.push_back(file_3840_2160_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0040
 * @tc.name      : h265 encode config 1920_1080 surface change 1920_1080 1080_1920 1280_1280
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0040, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1920;
        vEncSample->DEFAULT_HEIGHT = 1080;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0040.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->fileInfos.push_back(file_1920_1080_nv21);
        vEncSample->fileInfos.push_back(file_1080_1920_nv12);
        vEncSample->fileInfos.push_back(file_1280_1280_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0050
 * @tc.name      : h264 encode config 320_240 surface change 640_480 1280_720 1920_1080
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0050, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 320;
        vEncSample->DEFAULT_HEIGHT = 240;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0050.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->fileInfos.push_back(file_640_480_rgba);
        vEncSample->fileInfos.push_back(file_1280_720_nv12);
        vEncSample->fileInfos.push_back(file_1920_1080_nv21);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0060
 * @tc.name      : h264 encode config 1280_720 surface change 640_480 1280_720 1920_1080
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0060, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0060.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->fileInfos.push_back(file_640_480_rgba);
        vEncSample->fileInfos.push_back(file_1280_720_nv12);
        vEncSample->fileInfos.push_back(file_1920_1080_nv21);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0070
 * @tc.name      : h264 encode config 3840_2160 surface change 640_480 1280_536 1280_720 1920_816 1920_1080 3840_2160
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0070, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 3840;
        vEncSample->DEFAULT_HEIGHT = 2160;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0070.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->fileInfos.push_back(file_640_480_rgba);
        vEncSample->fileInfos.push_back(file_1280_536_nv21);
        vEncSample->fileInfos.push_back(file_1280_720_nv12);
        vEncSample->fileInfos.push_back(file_1920_816_rgba);
        vEncSample->fileInfos.push_back(file_1920_1080_nv21);
        vEncSample->fileInfos.push_back(file_3840_2160_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0080
 * @tc.name      : h264 encode config 1920_1080 surface change 1920_1080 1080_1920 1280_1280
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0080, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1920;
        vEncSample->DEFAULT_HEIGHT = 1080;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0080.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->fileInfos.push_back(file_1920_1080_nv21);
        vEncSample->fileInfos.push_back(file_1080_1920_nv12);
        vEncSample->fileInfos.push_back(file_1280_1280_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0090
 * @tc.name      : config main10 set format 8bit surface send 8bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0090, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 3840;
        vEncSample->DEFAULT_HEIGHT = 2160;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0090.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->configMain10 = true;
        vEncSample->setFormat8Bit = true;
        vEncSample->fileInfos.push_back(file_1280_720_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0100
 * @tc.name      : config main set format 10bit surface send 10bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 3840;
        vEncSample->DEFAULT_HEIGHT = 2160;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0100.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->configMain = true;
        vEncSample->setFormat10Bit = true;
        vEncSample->fileInfos.push_back(file_1280_720_nv12_10bit);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0110
 * @tc.name      : Not supported pixelFormat rgbx
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0110, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 3840;
        vEncSample->DEFAULT_HEIGHT = 2160;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0110.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->setFormatRbgx = true;
        vEncSample->fileInfos.push_back(file_640_480_rgba);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0120
 * @tc.name      : config main set format 8bit send 10bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0120, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 3840;
        vEncSample->DEFAULT_HEIGHT = 2160;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0120.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->configMain = true;
        vEncSample->setFormat8Bit = true;
        vEncSample->fileInfos.push_back(file_1280_720_nv12_10bit);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0130
 * @tc.name      : config main10 set format 10bit send 8bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0130, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 3840;
        vEncSample->DEFAULT_HEIGHT = 2160;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0130.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->configMain10 = true;
        vEncSample->setFormat10Bit = true;
        vEncSample->fileInfos.push_back(file_1280_720_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0140
 * @tc.name      : config main10 set format 8bit send 10bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0140, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 3840;
        vEncSample->DEFAULT_HEIGHT = 2160;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0140.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->configMain10 = true;
        vEncSample->setFormat8Bit = true;
        vEncSample->fileInfos.push_back(file_1280_720_nv12_10bit);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_MULTIFILE_0150
 * @tc.name      : config main10 set format 10bit send 8bit yuv
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_MULTIFILE_0150, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 3840;
        vEncSample->DEFAULT_HEIGHT = 2160;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_MULTIFILE_0150.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->configMain10 = true;
        vEncSample->setFormat10Bit = true;
        vEncSample->fileInfos.push_back(file_1280_720_nv12);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_SQR_0001
 * @tc.name      : config main set format 8bit surface with cbr mode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_SQR_0001, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_SQR_0001.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->configMain = true;
        vEncSample->setFormat8Bit = true;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->enableAutoSwitchParam = true;
        vEncSample->needResetBitrate = true;
        vEncSample->switchParamsTimeSec = 1;
        vEncSample->DEFAULT_FRAME_RATE = 10;
        vEncSample->fileInfos.push_back(file_1280_720_nv);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_SQR_0002
 * @tc.name      : config main set format 8bit surface with sqr mode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_SQR_0002, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_SQR_0002.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->configMain = true;
        vEncSample->setFormat8Bit = true;
        vEncSample->DEFAULT_BITRATE_MODE = SQR;
        vEncSample->enableAutoSwitchParam = true;
        vEncSample->needResetBitrate = true;
        vEncSample->switchParamsTimeSec = 1;
        vEncSample->DEFAULT_FRAME_RATE = 10;
        vEncSample->fileInfos.push_back(file_1280_720_nv);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_SQR_0003
 * @tc.name      : config main set format 8bit surface with sqr mode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_SQR_0003, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_SQR_0003.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->configMain = true;
        vEncSample->setFormat8Bit = true;
        vEncSample->DEFAULT_BITRATE_MODE = SQR;
        vEncSample->enableAutoSwitchParam = true;
        vEncSample->needResetSQRFactor = true;
        vEncSample->needResetBitrate = true;
        vEncSample->needResetMaxBitrate = true;
        vEncSample->needResetQuality = true;
        vEncSample->switchParamsTimeSec = 1;
        vEncSample->DEFAULT_FRAME_RATE = 10;
        vEncSample->fileInfos.push_back(file_1280_720_nv);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_SQR_0004
 * @tc.name      : config main set format 8bit surface with sqr mode
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_SQR_0004, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_SQR_0004.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->configMain = true;
        vEncSample->setFormat8Bit = true;
        vEncSample->DEFAULT_BITRATE_MODE = SQR;
        vEncSample->enableAutoSwitchParam = true;
        vEncSample->needResetSQRFactor = true;
        vEncSample->needResetBitrate = true;
        vEncSample->needResetMaxBitrate = true;
        vEncSample->needResetQuality = true;
        vEncSample->switchParamsTimeSec = 1;
        vEncSample->DEFAULT_FRAME_RATE = 10;
        vEncSample->DEFAULT_SQR_FACTOR = 35;
        vEncSample->DEFAULT_MAX_BITRATE = 600000000;
        vEncSample->fileInfos.push_back(file_1280_720_nv);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_0100
 * @tc.name      : set frame after 0
 * @tc.desc      : api test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_0100, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = 0;
        vEncSample->DEFAULT_MAX_COUNT = -1;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_INVALID_VAL, vEncSample->ConfigureVideoEncoder());
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_0200
 * @tc.name      : set frame after -1
 * @tc.desc      : api test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_0200, TestSize.Level1)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = -1;
        vEncSample->DEFAULT_MAX_COUNT = -1;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_INVALID_VAL, vEncSample->ConfigureVideoEncoder());
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_0300
 * @tc.name      : set max count 0
 * @tc.desc      : api test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_0300, TestSize.Level1)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = 1;
        vEncSample->DEFAULT_MAX_COUNT = 0;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_INVALID_VAL, vEncSample->ConfigureVideoEncoder());
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_FUNC_0100
 * @tc.name      : repeat surface h264 encode send eos,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_FUNC_0100, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0100.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->enableSeekEos = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = FRAME_AFTER;
        vEncSample->DEFAULT_MAX_COUNT = -1;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        cout << "outCount: " << vEncSample->outCount << endl;
        EXPECT_LE(vEncSample->outCount, COUNT_TWENTY_SEVEN);
        EXPECT_GE(vEncSample->outCount, COUNT_TWENTY_THREE);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_FUNC_0200
 * @tc.name      : repeat surface h264 encode send eos,max count 2,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_FUNC_0200, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0200.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->enableSeekEos = true;
        vEncSample->enableRepeat = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = FRAME_AFTER;
        vEncSample->DEFAULT_MAX_COUNT = MAX_COUNT;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        cout << "outCount: " << vEncSample->outCount << endl;
        ASSERT_EQ(COUNT_SEVENTEEN, vEncSample->outCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_FUNC_0300
 * @tc.name      : repeat surface h264 encode send frame,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_FUNC_0300, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0300.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = FRAME_AFTER;
        vEncSample->DEFAULT_MAX_COUNT = -1;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        cout << "outCount: " << vEncSample->outCount << endl;
        EXPECT_LE(vEncSample->outCount, COUNT_THIRTY_SEVEN);
        EXPECT_GE(vEncSample->outCount, COUNT_THIRTY_THREE);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_FUNC_0400
 * @tc.name      : repeat surface h264 encode send frame,max count 1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_FUNC_0400, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0400.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = FRAME_AFTER;
        vEncSample->DEFAULT_MAX_COUNT = 1;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        cout << "outCount: " << vEncSample->outCount << endl;
        ASSERT_EQ(COUNT_TWENTY_SIX, vEncSample->outCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_FUNC_0500
 * @tc.name      : repeat surface h265 encode send eos,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_FUNC_0500, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0500.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->enableSeekEos = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = FRAME_AFTER;
        vEncSample->DEFAULT_MAX_COUNT = -1;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        cout << "outCount: " << vEncSample->outCount << endl;
        EXPECT_LE(vEncSample->outCount, COUNT_TWENTY_SEVEN);
        EXPECT_GE(vEncSample->outCount, COUNT_TWENTY_THREE);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_FUNC_0600
 * @tc.name      : repeat surface h265 encode send eos,max count 2,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_FUNC_0600, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0600.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->enableSeekEos = true;
        vEncSample->enableRepeat = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = FRAME_AFTER;
        vEncSample->DEFAULT_MAX_COUNT = MAX_COUNT;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        cout << "outCount: " << vEncSample->outCount << endl;
        ASSERT_EQ(COUNT_SEVENTEEN, vEncSample->outCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_FUNC_0700
 * @tc.name      : repeat surface h265 encode send frame,max count -1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_FUNC_0700, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0700.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = FRAME_AFTER;
        vEncSample->DEFAULT_MAX_COUNT = -1;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        cout << "outCount: " << vEncSample->outCount << endl;
        EXPECT_LE(vEncSample->outCount, COUNT_THIRTY_SEVEN);
        EXPECT_GE(vEncSample->outCount, COUNT_THIRTY_THREE);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_FUNC_0800
 * @tc.name      : repeat surface h265 encode send frame,max count 1,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_FUNC_0800, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0800.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->setMaxCount = true;
        vEncSample->DEFAULT_FRAME_AFTER = FRAME_AFTER;
        vEncSample->DEFAULT_MAX_COUNT = 1;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        cout << "outCount: " << vEncSample->outCount << endl;
        ASSERT_EQ(COUNT_TWENTY_SIX, vEncSample->outCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_FUNC_0900
 * @tc.name      : repeat surface h265 encode send frame,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_FUNC_0900, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_0900.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->DEFAULT_FRAME_AFTER = FRAME_AFTER;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        cout << "outCount: " << vEncSample->outCount << endl;
        ASSERT_EQ(COUNT_THIRTY_FIVE, vEncSample->outCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_REPEAT_FUNC_1000
 * @tc.name      : repeat surface h264 encode send frame,frame after 73ms
 * @tc.desc      : function test
 */
HWTEST_F(HwEncFunc2NdkTest, VIDEO_ENCODE_REPEAT_FUNC_1000, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_BITRATE_MODE = CBR;
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_REPEAT_FUNC_1000.h264";
        vEncSample->SURF_INPUT = true;
        vEncSample->enableRepeat = true;
        vEncSample->DEFAULT_FRAME_AFTER = FRAME_AFTER;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        cout << "outCount: " << vEncSample->outCount << endl;
        ASSERT_EQ(COUNT_THIRTY_FIVE, vEncSample->outCount);
    }
}
} // namespace