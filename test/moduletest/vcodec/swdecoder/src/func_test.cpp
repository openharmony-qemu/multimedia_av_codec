/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "gtest/gtest.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "videodec_sample.h"
#include "videodec_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "native_avformat.h"

#ifdef SUPPORT_DRM
#include "native_mediakeysession.h"
#include "native_mediakeysystem.h"
#endif

#define MAX_THREAD 16

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class SwdecFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();

protected:
    const string CODEC_NAME = "video_decoder.avc";
    const char *INP_DIR_720_30 = "/data/test/media/1280_720_30_10Mb.h264";
    const char *INP_DIR_1080_30 = "/data/test/media/1920_1080_10_30Mb.h264";
};
} // namespace Media
} // namespace OHOS

namespace {
static OH_AVCapability *cap_avc = nullptr;
static string g_codecNameAvc = "";
} // namespace

void SwdecFuncNdkTest::SetUpTestCase()
{
    cap_avc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    g_codecNameAvc = OH_AVCapability_GetName(cap_avc);
    cout << "g_codecNameAvc: " << g_codecNameAvc << endl;
}

void SwdecFuncNdkTest::TearDownTestCase() {}
void SwdecFuncNdkTest::SetUp() {}
void SwdecFuncNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_001
 * @tc.name      : surf model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_001, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameAvc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_002
 * @tc.name      : surf model change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_002, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_003
 * @tc.name      : surf model change in runing state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_003, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_004
 * @tc.name      : repeat call setSurface fastly
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_004, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_005
 * @tc.name      : surf model change in flush to runnig state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_005, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_006
 * @tc.name      : surf model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_006, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameAvc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_007
 * @tc.name      : buffer model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_007, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameAvc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_008
 * @tc.name      : buffer model change in runing to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_008, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_009
 * @tc.name      : buffer model change in flushed to runing state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_009, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_012
 * @tc.name      : buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_012, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameAvc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_013
 * @tc.name      : surf model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_013, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_014
 * @tc.name      : surf model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_014, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAvc));;
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_015
 * @tc.name      : Two object repeat call setSurface fastly
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_015, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();

        auto vDecSample_1 = make_shared<VDecAPI11Sample>();
        vDecSample_1->INP_DIR = INP_DIR_1080_30;
        vDecSample_1->DEFAULT_WIDTH = 1920;
        vDecSample_1->DEFAULT_HEIGHT = 1080;
        vDecSample_1->DEFAULT_FRAME_RATE = 30;
        vDecSample_1->SURFACE_OUTPUT = true;
        vDecSample_1->autoSwitchSurface = true;
        vDecSample_1->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
        vDecSample_1->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_016
 * @tc.name      : repeat call setSurface fastly 2 time
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_016, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < 2; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_1080_30;
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SURFACE_OUTPUT = true;
            vDecSample->autoSwitchSurface = true;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameAvc));
            ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
            vDecSample->WaitForEOS();
        }
    }
}


/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0200
 * @tc.name      : create nonexist decoder
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0200, TestSize.Level1)
{
    OH_AVCodec *vdec_ = OH_VideoDecoder_CreateByName("OMX.h264.decode.111.222.333");
    ASSERT_EQ(nullptr, vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0300
 * @tc.name      : test h264  decode buffer
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0300, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0400
 * @tc.name      : test h264  decode surface
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0400, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    bool isVaild = false;
    OH_VideoDecoder_IsValid(vDecSample->vdec_, &isVaild);
    ASSERT_EQ(false, isVaild);
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0700
 * @tc.name      : test set EOS when last frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0700, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0800
 * @tc.name      : test set EOS before last frame then stop
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0800, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->BEFORE_EOS_INPUT = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}
/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_4000
 * @tc.name      : test set EOS before last frame then stop surface
 * @tc.desc      : function test
 */

HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_4000, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->BEFORE_EOS_INPUT = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1000
 * @tc.name      : test reconfigure for new file with one decoder
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1000, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1100
 * @tc.name      : test reconfigure for new file with the recreated decoder
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1100, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1200
 * @tc.name      : repeat start and stop 5 times before EOS
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1200, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->REPEAT_START_STOP_BEFORE_EOS = 5;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1300
 * @tc.name      : repeat start and flush 5 times before EOS
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1300, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->REPEAT_START_FLUSH_BEFORE_EOS = 5;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1400
 * @tc.name      : set larger width and height
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1400, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_720_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->checkOutPut = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1500
 * @tc.name      : set the width and height to a samller value
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1500, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->checkOutPut = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1600
 * @tc.name      : resolution change
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1600, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/resolutionChange.h264";
    vDecSample->DEFAULT_WIDTH = 1104;
    vDecSample->DEFAULT_HEIGHT = 622;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->checkOutPut = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1700
 * @tc.name      : decode h264 stream ,output pixel format is nv21
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1700, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1800
 * @tc.name      : decode h264 stream ,output pixel format is rgba
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1800, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_RGBA;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_ATTIME_0010
 * @tc.name      : test h264 asyn decode surface,use at time
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_ATTIME_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_720_30;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->rsAtTime = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1900
 * @tc.name      : Increase frame rate judgment
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1910, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_720_30;
    vDecSample->OUT_DIR = "/data/test/media/SW_720_30.yuv";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(101, vDecSample->outFrameCount);
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_RESOLUTION_PROFILE_0010
 * @tc.name      : Resolution and profile change
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_RESOLUTION_PROFILE_0010, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profResoChange.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SURFACE_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(350, vDecSample->outFrameCount);
    ASSERT_EQ(0, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW264_FUNC_0010
 * @tc.name      : 264同步软解输出nv12
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW264_FUNC_0010, TestSize.Level1)
{
    if (cap_avc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW264_FUNC_0020
 * @tc.name      : 264同步软解输出nv21
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW264_FUNC_0020, TestSize.Level0)
{
    if (cap_avc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW264_FUNC_0030
 * @tc.name      : 264同步软解输出surface
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW264_FUNC_0030, TestSize.Level1)
{
    if (cap_avc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->enbleSyncMode = 1;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW264_FUNC_0040
 * @tc.name      : 264同步软解输出rgba
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW264_FUNC_0040, TestSize.Level1)
{
    if (cap_avc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDECODE_BLANK_FRAME_0010
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder h264 swd
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDECODE_BLANK_FRAME_0010, TestSize.Level2)
{
    if (cap_avc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleBlankFrame = 1;
        vDecSample->SURFACE_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameAvc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDECODE_BLANK_FRAME_0020
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder h264 swd, surface
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDECODE_BLANK_FRAME_0020, TestSize.Level2)
{
    if (cap_avc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleBlankFrame = 1;
        vDecSample->SURFACE_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_FLUSH_0010
 * @tc.name      : create-start-eos-flush-start-eos
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_H264_FLUSH_0010, TestSize.Level1)
{
    if (cap_avc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameAvc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        vDecSample->FlushStatus();
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_FLUSH_0020
 * @tc.name      : create-start-eos-flush-start-eos,surface
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_H264_FLUSH_0020, TestSize.Level1)
{
    if (cap_avc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameAvc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        vDecSample->FlushStatus();
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PIXE_FORMAT_AVC_0010
 * @tc.name      : h264 pixelformat query key
 * @tc.desc      : function test
 */
HWTEST_F(SwdecFuncNdkTest, VIDEO_SWDEC_PIXE_FORMAT_AVC_0010, TestSize.Level2)
{
    if (cap_avc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->isGetVideoSupportedPixelFormats = true;
        vDecSample->isGetFormatKey = true;
        vDecSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
        vDecSample->isEncoder = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAvc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_LT(1, vDecSample->pixlFormatNum);
        ASSERT_EQ(24, vDecSample->firstCallBackKey);
        ASSERT_EQ(24, vDecSample->onStreamChangedKey);
    }
}
} // namespace