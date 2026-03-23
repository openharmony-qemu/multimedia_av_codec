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
#include "gtest/gtest.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "videodec_sample.h"
#include "native_avformat.h"
#include "videodec_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class HevcSwdecFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void Release();
    int32_t Stop();

protected:
    const char *ginpDir1080 = "/data/test/media/1920_1080_30.h265";
    const char *ginpDir144 = "/data/test/media/176_144_Main10.h265";
    const char *ginpDir64 = "/data/test/media/64_64_Main.h265";
    const char *ginpDir1158 = "/data/test/media/1544_1158_Rext_gray.h265";
    const char *ginpDir1440 = "/data/test/media/1920_1440_Main_HEIF.h265";
    const char *ginpDir1022 = "/data/test/media/1858_1022_Main.h265";
    const char *ginpDir71 = "/data/test/media/95_71_Rext_gray.h265";
    const char *ginpDirVar = "/data/test/media/95_71_Rext_gray.h265";
    const char *ginpDirVar1080 = "/data/test/media/1920_1080_64_64_var.h265";
};
} // namespace Media
} // namespace OHOS


int32_t g_reliCount100 = 50;
int32_t g_reliCount2 = 1;
namespace {
OH_AVCodec *vdec_ = NULL;
OH_AVFormat *format;
static OH_AVCapability *cap_hevc = nullptr;
static string g_codecNameHevc = "";
constexpr int32_t DEFAULT_WIDTH = 1920;
constexpr int32_t DEFAULT_HEIGHT = 1080;
const std::vector<OH_NativeBuffer_TransformType> transfromTypes = {
    NATIVEBUFFER_ROTATE_NONE,
    NATIVEBUFFER_ROTATE_90,
    NATIVEBUFFER_ROTATE_180,
    NATIVEBUFFER_ROTATE_270,
    NATIVEBUFFER_FLIP_H,
    NATIVEBUFFER_FLIP_V,
    NATIVEBUFFER_FLIP_H_ROT90,
    NATIVEBUFFER_FLIP_V_ROT90,
    NATIVEBUFFER_FLIP_H_ROT180,
    NATIVEBUFFER_FLIP_V_ROT180,
    NATIVEBUFFER_FLIP_H_ROT270,
    NATIVEBUFFER_FLIP_V_ROT270
};
} // namespace

void HevcSwdecFuncNdkTest::SetUpTestCase()
{
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, SOFTWARE);
    g_codecNameHevc = OH_AVCapability_GetName(cap_hevc);
    cout << "g_codecNameHevc: " << g_codecNameHevc << endl;
}

void HevcSwdecFuncNdkTest::TearDownTestCase() {}
void HevcSwdecFuncNdkTest::SetUp() {}
void HevcSwdecFuncNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0320
 * @tc.name      : test h265 decode buffer pixel foramt nv12
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0320, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0320
 * @tc.name      : test h265 decode buffer pixel foramt nv21
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0330, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0400
 * @tc.name      : test h265 decode surface, pixel foramt nv12
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0400, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->SF_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0410
 * @tc.name      : test h265 asyn decode surface, pixel foramt nv21
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0410, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->SF_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0700
 * @tc.name      : test set EOS when last frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0700, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0800
 * @tc.name      : test set EOS before last frame then stop
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0800, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->BEFORE_EOS_INPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_0900
 * @tc.name      : test set EOS before last frame then input frames
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_0900, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->BEFORE_EOS_INPUT_INPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1200
 * @tc.name      : repeat start and stop 5 times before EOS
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1200, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->REPEAT_START_STOP_BEFORE_EOS = 10;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_1300
 * @tc.name      : repeat start and flush 5 times before EOS
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_1300, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDirVar1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->REPEAT_START_FLUSH_BEFORE_EOS = 5;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : SURF_CHANGE_FUNC_001
 * @tc.name      : surf change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, SURF_CHANGE_FUNC_001, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : SURF_CHANGE_FUNC_002
 * @tc.name      : surf change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, SURF_CHANGE_FUNC_002, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : SURF_CHANGE_FUNC_003
 * @tc.name      : surf change in buffer mode
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, SURF_CHANGE_FUNC_003, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : SURF_CHANGE_FUNC_004
 * @tc.name      : repeat call setSurface fastly
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, SURF_CHANGE_FUNC_004, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
    }
}


/**
 * @tc.number    : MAX_INPUT_SIZE_CHECK_001
 * @tc.name      : MaxInputSize value incorrect
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, MAX_INPUT_SIZE_CHECK_001, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->SF_OUTPUT = false;
        vDecSample->maxInputSize = 1000;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : MAX_INPUT_SIZE_CHECK_002
 * @tc.name      : MaxInputSize value incorrect
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, MAX_INPUT_SIZE_CHECK_002, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->SF_OUTPUT = false;
        vDecSample->maxInputSize = 1000;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : SUB_MEDIA_VIDEO_SWDEC_H265_SWITCH_001
 * @tc.name      : Conversion from H.265 software decoding to hardware decoding
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, SUB_MEDIA_VIDEO_SWDEC_H265_SWITCH_001, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i <= 39; i++) {
            vdec_ = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
            ASSERT_NE(nullptr, vdec_);
            format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
            OH_AVFormat_Destroy(format);
            OH_VideoDecoder_Stop(vdec_);
            OH_VideoDecoder_Destroy(vdec_);
        }
    }
}

/**
 * @tc.number    : SUB_MEDIA_VIDEO_SWDEC_H265_SWITCH_002
 * @tc.name      : Conversion from H.265 software decoding to hardware decoding
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, SUB_MEDIA_VIDEO_SWDEC_H265_SWITCH_002, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i <= 30; i++) {
            if (i == 30) {
                shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
                vDecSample->INP_DIR = ginpDir1080;
                vDecSample->DEFAULT_WIDTH = 1920;
                vDecSample->DEFAULT_HEIGHT = 1080;
                vDecSample->DEFAULT_FRAME_RATE = 30;
                vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
                vDecSample->SF_OUTPUT = false;
                ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
                vDecSample->WaitForEOS();
                ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            }
            vdec_ = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
            ASSERT_NE(nullptr, vdec_);
            format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
            OH_AVFormat_Destroy(format);
            OH_VideoDecoder_Stop(vdec_);
            OH_VideoDecoder_Destroy(vdec_);
        }
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_0310
 * @tc.name      : test h265 asyn decode buffer, pixel foramt nv21
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_0310, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_0320
 * @tc.name      : test h265 decode buffer, pixel foramt nv12
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_0320, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_0400
 * @tc.name      : test h265 asyn decode surface, pixel foramt nv12
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_0400, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->SF_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_0410
 * @tc.name      : test h265 decode surface, pixel foramt nv21
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_0410, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->SF_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_0420
 * @tc.name      : test h265 asyn decode surface, pixel foramt nv12
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_0420, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->SF_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_0430
 * @tc.name      : test h265 asyn decode surface, pixel foramt nv21
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_0430, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->SF_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_0700
 * @tc.name      : test set EOS when last frame
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_0700, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_0800
 * @tc.name      : test set EOS before last frame then stop
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_0800, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->BEFORE_EOS_INPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_0900
 * @tc.name      : test set EOS before last frame then input frames
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_0900, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->BEFORE_EOS_INPUT_INPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_1000
 * @tc.name      : test reconfigure for new file with one decoder
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_1000, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_1100
 * @tc.name      : test reconfigure for new file with the recreated decoder
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_1100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_1200
 * @tc.name      : repeat start and stop 100 times before EOS
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_1200, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->REPEAT_START_STOP_BEFORE_EOS = 100;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_FUNCTION_1300
 * @tc.name      : repeat start and flush 5 times before EOS
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_FUNCTION_1300, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDirVar;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->REPEAT_START_FLUSH_BEFORE_EOS = 5;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_001
 * @tc.name      : surf change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_001, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_002
 * @tc.name      : surf change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_002, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_003
 * @tc.name      : surf change in buffer mode
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_003, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
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
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_004, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_005
 * @tc.name      : surf model change in flush to runing state
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_005, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_006
 * @tc.name      : surf model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_006, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_007
 * @tc.name      : surf model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_007, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
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
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_008, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_009
 * @tc.name      : buffer model change in flushed to runing state
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_009, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
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
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_012, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_013
 * @tc.name      : buffer model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_013, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_014
 * @tc.name      : buffer model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_014, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_016
 * @tc.name      : Two streams repeat call setSurface fastly
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_016, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();

        auto vDecSample_1 = make_shared<VDecAPI11Sample>();
        vDecSample_1->INP_DIR = ginpDir1080;
        vDecSample_1->DEFAULT_WIDTH = 1920;
        vDecSample_1->DEFAULT_HEIGHT = 1080;
        vDecSample_1->DEFAULT_FRAME_RATE = 30;
        vDecSample_1->SF_OUTPUT = true;
        vDecSample_1->autoSwitchSurface = true;
        vDecSample_1->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->RepeatCallSetSurface());
        vDecSample_1->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_017
 * @tc.name      : surf change in flush to runing repeat call setSurface fastly
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_017, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_018
 * @tc.name      : surf change in flush to runing repeat call setSurface fastly 100
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_018, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
        vDecSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        for (int i = 0; i < 100; i++) {
            ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
            ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        }
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}


/**
 * @tc.number    : API11_MAX_INPUT_SIZE_CHECK_001
 * @tc.name      : MaxInputSize value incorrect
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_MAX_INPUT_SIZE_CHECK_001, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->SF_OUTPUT = false;
        vDecSample->maxInputSize = 1000;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_MAX_INPUT_SIZE_CHECK_002
 * @tc.name      : MaxInputSize value incorrect
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_MAX_INPUT_SIZE_CHECK_002, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->SF_OUTPUT = false;
        vDecSample->maxInputSize = 1000;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_001
 * @tc.name      : OH_AVFormat_SetIntValue config
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_001, TestSize.Level4)
{
    if (!access("/system/lib64/media/", 0)) {
            vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHevc.c_str());
            ASSERT_NE(nullptr, vdec_);
            format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
            ASSERT_EQ(AV_ERR_UNSUPPORT, OH_VideoDecoder_Configure(vdec_, format));
            OH_AVFormat_Destroy(format);
    }
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Destroy(vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_002
 * @tc.name      : OH_AVFormat_SetIntValue config
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_002, TestSize.Level4)
{
    if (!access("/system/lib64/media/", 0)) {
            vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHevc.c_str());
            ASSERT_NE(nullptr, vdec_);
            format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV21);
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
            OH_AVFormat_Destroy(format);
    }
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Destroy(vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_003
 * @tc.name      : OH_AVFormat_SetIntValue config
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_003, TestSize.Level4)
{
    if (!access("/system/lib64/media/", 0)) {
            vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHevc.c_str());
            ASSERT_NE(nullptr, vdec_);
            format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_SURFACE_FORMAT);
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
            OH_AVFormat_Destroy(format);
    }
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Destroy(vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_004
 * @tc.name      : OH_AVFormat_SetIntValue config
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_004, TestSize.Level4)
{
    if (!access("/system/lib64/media/", 0)) {
            vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHevc.c_str());
            ASSERT_NE(nullptr, vdec_);
            format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_RGBA);
            ASSERT_EQ(AV_ERR_UNSUPPORT, OH_VideoDecoder_Configure(vdec_, format));
            OH_AVFormat_Destroy(format);
    }
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Destroy(vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_005
 * @tc.name      : OH_AVFormat_SetIntValue config
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_005, TestSize.Level4)
{
    if (!access("/system/lib64/media/", 0)) {
            vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHevc.c_str());
            ASSERT_NE(nullptr, vdec_);
            format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, 0);
            ASSERT_EQ(AV_ERR_UNSUPPORT, OH_VideoDecoder_Configure(vdec_, format));
            OH_AVFormat_Destroy(format);
    }
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Destroy(vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_006
 * @tc.name      : OH_AVFormat_SetIntValue config
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_006, TestSize.Level4)
{
    if (!access("/system/lib64/media/", 0)) {
            vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHevc.c_str());
            ASSERT_NE(nullptr, vdec_);
            format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, -1);
            ASSERT_EQ(AV_ERR_UNSUPPORT, OH_VideoDecoder_Configure(vdec_, format));
            OH_AVFormat_Destroy(format);
    }
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Destroy(vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_007
 * @tc.name      : test h265 decode buffer framerate -1
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_007, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = -1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_008
 * @tc.name      : test h265 decode buffer framerate 0
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_008, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 0;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_008_2
 * @tc.name      : test h265 decode buffer framerate 0.1
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_008_2, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 0.1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_009
 * @tc.name      : test h265 decode buffer framerate 1
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_009, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_010
 * @tc.name      : test h265 decode buffer framerate 100000
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_010, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 100000;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_011
 * @tc.name      : width set -1 height set -1
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_011, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = -1;
        vDecSample->DEFAULT_HEIGHT = -1;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_012
 * @tc.name      : width set 0 height set 0
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_012, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 0;
        vDecSample->DEFAULT_HEIGHT = 0;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_013
 * @tc.name      : width set 1 height set 1
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_013, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1;
        vDecSample->DEFAULT_HEIGHT = 1;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_014
 * @tc.name      : width set 10000 height set 10000
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_014, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 10000;
        vDecSample->DEFAULT_HEIGHT = 10000;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PARA_015
 * @tc.name      : width set 64 height set 64
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PARA_015, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 64;
        vDecSample->DEFAULT_HEIGHT = 64;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_STABILITY_0200
 * @tc.name      : confige start flush start reset destroy 50 times
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_STABILITY_FUNC_0010, TestSize.Level4)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount100; i++) {
            vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHevc.c_str());
            ASSERT_NE(nullptr, vdec_);
            format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 1920);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 30);
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
            OH_AVFormat_Destroy(format);
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(vdec_));
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec_));
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Destroy(vdec_));
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_STABILITY_0020
 * @tc.name      : confige start flush start reset 1000 times
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_STABILITY_FUNC_0020, TestSize.Level4)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount100; i++) {
            vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHevc.c_str());
            ASSERT_NE(nullptr, vdec_);
            format = OH_AVFormat_Create();
            ASSERT_NE(nullptr, format);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 1920);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV21);
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 30);
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
            OH_AVFormat_Destroy(format);
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(vdec_));
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec_));
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Destroy(vdec_));
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_STABILITY_FUNC_0030
 * @tc.name      : SetParameter 50 time
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_STABILITY_FUNC_0030, TestSize.Level4)
{
    if (!access("/system/lib64/media/", 0)) {
        vdec_ = OH_VideoDecoder_CreateByName(g_codecNameHevc.c_str());
        ASSERT_NE(nullptr, vdec_);
        format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        int64_t width = 1920;
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, width);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 30);
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
        for (int i = 0; i < g_reliCount100; i++) {
            width--;
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, width);
            ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_SetParameter(vdec_, format));
        }
        OH_AVFormat_Destroy(format);
        OH_VideoDecoder_Stop(vdec_);
        OH_VideoDecoder_Destroy(vdec_);
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_STAB_FUNC_0100
 * @tc.name      : 10bit stream and 8bit stream decode simultaneously
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_STAB_FUNC_0100, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount2; i++) {
            shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = ginpDir144;
            vDecSample->SF_OUTPUT = false;
            vDecSample->DEFAULT_WIDTH = 176;
            vDecSample->DEFAULT_HEIGHT = 144;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
            vDecSample->WaitForEOS();

            shared_ptr<VDecAPI11Sample> vDecSample1 = make_shared<VDecAPI11Sample>();
            vDecSample1->INP_DIR = ginpDir1080;
            vDecSample1->SF_OUTPUT = true;
            vDecSample1->DEFAULT_WIDTH = 1920;
            vDecSample1->DEFAULT_HEIGHT = 1080;
            vDecSample1->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vDecSample1->RunVideoDec_Surface(g_codecNameHevc));
            vDecSample1->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_STABLITY_FUNC_0110
 * @tc.name      : rand high and whith (1920 * 1080)
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_STABLITY_FUNC_0110, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount2; i++) {
            shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = ginpDir1080;
            vDecSample->DEFAULT_WIDTH = WidthRand();
            cout << "rand width is: " << vDecSample->DEFAULT_WIDTH << endl;
            vDecSample->DEFAULT_HEIGHT = HighRand();
            cout << "rand high is" << vDecSample->DEFAULT_HEIGHT << endl;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_STABLITY_FUNC_0120
 * @tc.name      : rand and whith (64 * 64)
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_STABLITY_FUNC_0120, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount2; i++) {
            shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = ginpDir64;
            vDecSample->DEFAULT_WIDTH = WidthRand();
            cout << "rand width is: " << vDecSample->DEFAULT_WIDTH << endl;
            vDecSample->DEFAULT_HEIGHT = HighRand();
            cout << "rand high is" << vDecSample->DEFAULT_HEIGHT << endl;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : API11_VIDEO_SWDEC_STABLITY_FUNC_0130
 * @tc.name      : rand high and whith (176 * 144)
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, API11_VIDEO_SWDEC_STABLITY_FUNC_0130, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount2; i++) {
            shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = ginpDir144;
            vDecSample->DEFAULT_WIDTH = WidthRand();
            cout << "rand width is: " << vDecSample->DEFAULT_WIDTH << endl;
            vDecSample->DEFAULT_HEIGHT = HighRand();
            cout << "rand high is" << vDecSample->DEFAULT_HEIGHT << endl;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_001
 * @tc.name      : test h265 decode buffer (1544 * 1158)
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_001, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->SF_OUTPUT = false;
        vDecSample->INP_DIR = ginpDir1158;
        vDecSample->DEFAULT_WIDTH = 1544;
        vDecSample->DEFAULT_HEIGHT = 1158;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_002
 * @tc.name      : test h265 decode buffer (1920 * 1440)
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_002, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1440;
        vDecSample->SF_OUTPUT = false;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1440;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_003
 * @tc.name      : test h265 decode buffer (1858 * 1022)
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_003, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->SF_OUTPUT = false;
        vDecSample->INP_DIR = ginpDir1022;
        vDecSample->DEFAULT_WIDTH = 1858;
        vDecSample->DEFAULT_HEIGHT = 1022;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_FUNCTION_004
 * @tc.name      : test h265 decode buffer (95 * 71)
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_FUNCTION_004, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->SF_OUTPUT = false;
        vDecSample->INP_DIR = ginpDir71;
        vDecSample->DEFAULT_WIDTH = 95;
        vDecSample->DEFAULT_HEIGHT = 71;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_RESOLUTION_PROFILE_0010
 * @tc.name      : Resolution and profile change
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_RESOLUTION_PROFILE_0010, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profResoChange.h265";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->NocaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(800, vDecSample->outFrameCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW265_FUNC_0010
 * @tc.name      : 265同步软解输出nv12
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW265_FUNC_0010, TestSize.Level1)
{
    if (cap_hevc != nullptr) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->enbleSyncMode = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW265_FUNC_0020
 * @tc.name      : 265同步软解输出nv21
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW265_FUNC_0020, TestSize.Level0)
{
    if (cap_hevc != nullptr) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->enbleSyncMode = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW265_FUNC_0030
 * @tc.name      : 265同步软解输出surface
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW265_FUNC_0030, TestSize.Level1)
{
    if (cap_hevc != nullptr) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->SF_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->enbleSyncMode = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_BLANK_FRAME_0010
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder h265 swd
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_H265_BLANK_FRAME_0010, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->enbleBlankFrame = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_BLANK_FRAME_0020
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder h265 swd, surface
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_H265_BLANK_FRAME_0020, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->enbleBlankFrame = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_BLANK_FRAME_0030
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder h265 swd, surface, stop-start
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_H265_BLANK_FRAME_0030, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->enbleBlankFrame = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        for (int i = 0; i < 2; i++) {
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
            vDecSample->Flush_buffer();
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_FLUSH_0010
 * @tc.name      : create-start-eos-flush-start-eos
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_H265_FLUSH_0010, TestSize.Level1)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameHevc));
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
 * @tc.number    : VIDEO_SWDEC_H265_FLUSH_0020
 * @tc.name      : create-start-eos-flush-start-eos,surface
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_H265_FLUSH_0020, TestSize.Level1)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
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
 * @tc.number    : VIDEO_SWDEC_PIXE_FORMAT_HEVC_0010
 * @tc.name      : h265 pixelformat query key
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_PIXE_FORMAT_HEVC_0010, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->isGetVideoSupportedPixelFormats = true;
        vDecSample->isGetFormatKey = true;
        vDecSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        vDecSample->isEncoder = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_LT(1, vDecSample->pixlFormatNum);
        ASSERT_EQ(24, vDecSample->firstCallBackKey);
        ASSERT_EQ(24, vDecSample->onStreamChangedKey);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0060
 * @tc.name      : 软解265, config transform
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_DECODE_TRANSFORM_0060, TestSize.Level0)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->SF_OUTPUT = true;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        vDecSample->setTransform = true;
        vDecSample->DEFAULT_TRANSFORM = -1;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->SetConfigTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        for (const auto& transformtype : transfromTypes) {
            vDecSample->DEFAULT_TRANSFORM = transformtype;
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
            ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        }
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0150
 * @tc.name      : 软解265, setparameter transform
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_DECODE_TRANSFORM_0150, TestSize.Level0)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->SF_OUTPUT = true;
        vDecSample->CreateSurface();
        vDecSample->DEFAULT_TRANSFORM = -1;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->SetParameterTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        for (const auto& transformtype : transfromTypes) {
            vDecSample->DEFAULT_TRANSFORM = transformtype;
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetParameterTransform());
            ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        }
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetParameter());
        ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0270
 * @tc.name      : 软解H265, surface chnange, transform follow
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_DECODE_TRANSFORM_0270, TestSize.Level0)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/1920_1080_20M_30.h265";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->needAutoSwitch = false;
        vDecSample->autoSwitchSurface = true;
        vDecSample->setTransform = true;
        vDecSample->DEFAULT_TRANSFORM = NATIVEBUFFER_FLIP_H_ROT90;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHevc));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(vDecSample->beforeSwitchTransform, vDecSample->afterSwitchTransform);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_H265_FLUSH_THREE_0010
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder h265 swd
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFuncNdkTest, VIDEO_SWDEC_H265_FLUSH_THREE_0010, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->NEED_MD5_COMPAIRE = false;
        vDecSample->INPUT_NAL_NUM = 3;
        vDecSample->INPUT_STREAM_TYPE = OHOS::Media::VDecAPI11Sample::Input_Stream_Type_000001;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}
} // namespace