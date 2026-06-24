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
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace OHOS {
namespace Media {
bool g_isSupportedCBRHQ = false;
fileInfo file_640_480_rgba{"/data/test/media/640_480.rgba", NATIVEBUFFER_PIXEL_FMT_RGBA_8888, 640, 480 };
fileInfo file_1280_720_nv12{"/data/test/media/1280_720_nv12.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 1280, 720 };
fileInfo file_1920_1080_nv21{"/data/test/media/1920_1080_nv21.yuv", NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, 1920, 1080 };
constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecNameHEVC[CODEC_NAME_SIZE] = {};
class CBRHQNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void CBRHQNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    g_isSupportedCBRHQ = OH_AVCapability_IsEncoderBitrateModeSupported(cap, BITRATE_MODE_CBR_HIGH_QUALITY);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecNameHEVC, sizeof(g_codecNameHEVC), tmpCodecName, strlen(tmpCodecName)) != 0) {
        cout << "copy codec name failed" << endl;
    }
}
void CBRHQNdkTest::TearDownTestCase() {}
void CBRHQNdkTest::SetUp() {}
void CBRHQNdkTest::TearDown() {}
} // namespace Media
} // namespace OHOS

namespace  {
/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_0100
 * @tc.name      : config cbr hq mode with supported hevc encoder
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_0100, TestSize.Level2)
{
    if (g_isSupportedCBRHQ) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        OH_AVCodec *venc = OH_VideoEncoder_CreateByName(g_codecNameHEVC);
        ASSERT_NE(nullptr, venc);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        vEncSample->SetConfigureEnc(format);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, BITRATE_MODE_CBR_HIGH_QUALITY);
        int ret = OH_VideoEncoder_Configure(venc, format);
        OH_VideoEncoder_Destroy(venc);
        ASSERT_EQ(ret, AV_ERR_OK);
    }
}

/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_0200
 * @tc.name      : config cbr hq mode with software hevc encoder
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_0200, TestSize.Level2)
{
    if (g_isSupportedCBRHQ) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        OH_AVCodec *venc = OH_VideoEncoder_CreateByName(g_codecNameHEVC);
        ASSERT_NE(nullptr, venc);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        vEncSample->SetConfigureEnc(format);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, vEncSample->DEFAULT_QUALITY);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, BITRATE_MODE_CBR_HIGH_QUALITY);
        int ret = OH_VideoEncoder_Configure(venc, format);
        OH_VideoEncoder_Destroy(venc);
        ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
    }
}

/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_0300
 * @tc.name      : encode cbrhq mode 1280*720 resolution
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_0300, TestSize.Level2)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_CBRHQ_FUNC_0400.h265";
    vEncSample->DEFAULT_BITRATE_MODE = BITRATE_MODE_CBR_HIGH_QUALITY;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_0400
 * @tc.name      : encode cbrhq mode 3840*2160 resolution
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_0400, TestSize.Level2)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = "/data/test/media/3840_2160_nv12.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_CBRHQ_FUNC_0800.h265";
    vEncSample->DEFAULT_WIDTH = 3840;
    vEncSample->DEFAULT_HEIGHT = 2160;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->DEFAULT_BITRATE_MODE = BITRATE_MODE_CBR_HIGH_QUALITY;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_0500
 * @tc.name      : encode cbrhq mode 1920*1080 resolution
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_0500, TestSize.Level2)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = "/data/test/media/1080_1920_nv12.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_CBRHQ_FUNC_0900.h265";
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->DEFAULT_BITRATE = 3000000;
    vEncSample->DEFAULT_BITRATE_MODE = BITRATE_MODE_CBR_HIGH_QUALITY;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_0600
 * @tc.name      : encode cbrhq mode with low latency
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_0600, TestSize.Level2)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = "/data/test/media/1080_1920_nv12.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_CBRHQ_FUNC_1000.h265";
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->DEFAULT_BITRATE = 3000000;
    vEncSample->DEFAULT_BITRATE_MODE = BITRATE_MODE_CBR_HIGH_QUALITY;
    vEncSample->enableLowLatency = true;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_0700
 * @tc.name      : encode cbrhq mode and enable b frame
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_0700, TestSize.Level2)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = "/data/test/media/1080_1920_nv12.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_CBRHQ_FUNC_1000.h265";
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->DEFAULT_BITRATE = 3000000;
    vEncSample->DEFAULT_BITRATE_MODE = BITRATE_MODE_CBR_HIGH_QUALITY;
    vEncSample->enableBFrameMode = 1;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_0800
 * @tc.name      : encode cbrhq mode and request I frame in encoding
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_0800, TestSize.Level2)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = "/data/test/media/1080_1920_nv12.yuv";
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_CBRHQ_FUNC_1000.h265";
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->DEFAULT_BITRATE = 3000000;
    vEncSample->DEFAULT_BITRATE_MODE = BITRATE_MODE_CBR_HIGH_QUALITY;
    vEncSample->enableForceIDR = true;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_0900
 * @tc.name      : encode cbrhq mode and enable temporal encode
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_0900, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1920_1080_nv21.yuv";
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_CBRHQ_FUNC_1000.h265";
        vEncSample->DEFAULT_WIDTH = 1920;
        vEncSample->DEFAULT_HEIGHT = 1080;
        vEncSample->DEFAULT_BITRATE = 3000000;
        vEncSample->DEFAULT_BITRATE_MODE = BITRATE_MODE_CBR_HIGH_QUALITY;
        vEncSample->TEMPORAL_ENABLE = true;
        vEncSample->TEMPORAL_CONFIG = true;
        vEncSample->TEMPORAL_JUMP_MODE = true;
        int32_t temporalGopSize = 8;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporalGopSize));
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_1000
 * @tc.name      : encode cbrhq mode and enable LTR
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_1000, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->INP_DIR = "/data/test/media/1920_1080_nv21.yuv";
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_CBRHQ_FUNC_1000.h265";
        vEncSample->DEFAULT_WIDTH = 1920;
        vEncSample->DEFAULT_HEIGHT = 1080;
        vEncSample->DEFAULT_BITRATE = 3000000;
        vEncSample->DEFAULT_BITRATE_MODE = BITRATE_MODE_CBR_HIGH_QUALITY;
        vEncSample->enableLTR = true;
        vEncSample->ltrParam.ltrCount = 5;
        vEncSample->ltrParam.ltrInterval = 5;
        vEncSample->ltrParam.enableUseLtr = true;
        vEncSample->ltrParam.useLtrIndex = 1;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : CBR_HQ_ENCODE_FUNC_1100
 * @tc.name      : encode cbrhq mode and resolution change
 * @tc.desc      : api test
 */
HWTEST_F(CBRHQNdkTest, CBR_HQ_ENCODE_FUNC_1100, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->OUT_DIR = "/data/test/media/CBR_HQ_ENCODE_FUNC_1500.h265";
        vEncSample->SURF_INPUT = true;
        vEncSample->readMultiFiles = true;
        vEncSample->DEFAULT_WIDTH = 1920;
        vEncSample->DEFAULT_HEIGHT = 1080;
        vEncSample->noDestroy = true;
        vEncSample->DEFAULT_BITRATE_MODE = BITRATE_MODE_CBR_HIGH_QUALITY;
        vEncSample->fileInfos.push_back(file_640_480_rgba);
        vEncSample->fileInfos.push_back(file_1280_720_nv12);
        vEncSample->fileInfos.push_back(file_1920_1080_nv21);
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}
}