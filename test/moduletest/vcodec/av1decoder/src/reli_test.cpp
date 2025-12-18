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
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include "gtest/gtest.h"
#include "native_avcodec_videodecoder.h"
#include "videodec_api11_sample.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class Av1DecReliNdkTest : public testing::Test {
public:
    static void SetUpTestCase();    // 第一个测试用例执行前
    static void TearDownTestCase(); // 最后一个测试用例执行后
    void SetUp() override;          // 每个测试用例执行前
    void TearDown() override;       // 每个测试用例执行后
    void Release();
    int32_t Stop();

protected:
    OH_AVCodec *vdec_;
    const char *inpDir720Array[16] = {
        "/data/test/media/av1_avi.avi",
        "/data/test/media/av1_mp4.mp4",
        "/data/test/media/av1_mkv.mkv",
        "/data/test/media/av1_fmp4.mp4",
        "/data/test/media/av1_flv.flv",
        "/data/test/media/av1_webm.webm",
    };
};
} // namespace Media
} // namespace OHOS

static string g_codecNameAv1 = "";
int32_t g_reliCount = 16;
int32_t g_reliCount1000 = 1000;
const char *FILE_PATH = "/data/test/media/test_av1.ivf";
const char *FILE_PATH2 = "/data/test/media/av1_main_L4.0_1920x1080.ivf";
void Av1DecReliNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_AV1, false, SOFTWARE);
    g_codecNameAv1 = OH_AVCapability_GetName(cap);
    cout << "g_codecNameAv1: " << g_codecNameAv1 << endl;
}

void Av1DecReliNdkTest::TearDownTestCase() {}
void Av1DecReliNdkTest::SetUp() {}
void Av1DecReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0010
 * @tc.name      : AV1 soft decode successively 16 times
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0010, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH;
        vDecSample->SF_OUTPUT = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}
/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0020
 * @tc.name      : AV1 soft decode 16 times surface mode, change videoSize to 1920x1080
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0020, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH2;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->SF_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0030
 * @tc.name      : 16 instances AV1 soft decode with Surface mode
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0030, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->getFormat(inpDir720Array[i % 6]);
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameAv1));
        cout << "VIDEO_AV1DEC_STABILITY_0030 end: " << i << endl;
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
             {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}

/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0040
 * @tc.name      : 16 instances AV1 soft decode with buffer mode
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0040, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->getFormat(inpDir720Array[i % 6]);
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameAv1.c_str()));
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
             {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}

/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0050
 * @tc.name      : AV1 soft decode 16 times with fast Surface change
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0050, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0060
 * @tc.name      : Decode 16 AV1 streams simultaneously in Surface mode
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0060, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->getFormat(inpDir720Array[i % 6]);
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameAv1));
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
             {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}

/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0070
 * @tc.name      : Decode 16 AV1 streams simultaneously in buffer mode
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0070, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->SF_OUTPUT = false;
        vDecSample->INP_DIR = FILE_PATH;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
             {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}
/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0080
 * @tc.name      : 16 instances AV1 soft decode with buffer mode × 5 loops
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0080, TestSize.Level3)
{
    for (int j = 0; j < 5; j++)
    {
        vector<shared_ptr<VDecAPI11Sample>> decVec;
        for (int i = 0; i < g_reliCount; i++)
        {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            decVec.push_back(vDecSample);
            vDecSample->SF_OUTPUT = false;
            vDecSample->INP_DIR = FILE_PATH;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
        }
        uint32_t errorCount = 0;
        for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
                 {
            sample->WaitForEOS();
            errorCount += sample->errCount; });
        decVec.clear();
    }
}

/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0090
 * @tc.name      : Decode AV1 streams simultaneously
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0090, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDecForAv1(g_codecNameAv1));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        close(vDecSample->g_fd);
        vDecSample->g_fd = -1;
        shared_ptr<VDecAPI11Sample> vDecSample1 = make_shared<VDecAPI11Sample>();
        vDecSample1->INP_DIR = FILE_PATH;
        vDecSample1->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample1->RunVideoDecForAv1(g_codecNameAv1));
        vDecSample1->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample1->errCount);
        close(vDecSample1->g_fd);
        vDecSample1->g_fd = -1;
    }
}
/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0100
 * @tc.name      : Simultaneous decoding with surface and buffer modes
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0100, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDecForAv1(g_codecNameAv1));
        vDecSample->WaitForEOS();
        close(vDecSample->g_fd);
        vDecSample->g_fd = -1;
        shared_ptr<VDecAPI11Sample> vDecSample1 = make_shared<VDecAPI11Sample>();
        vDecSample1->INP_DIR = FILE_PATH;
        vDecSample1->SF_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample1->RunVideoDec_SurfaceForAv1(g_codecNameAv1));
        vDecSample1->WaitForEOS();
        close(vDecSample1->g_fd);
        vDecSample1->g_fd = -1;
    }
}
/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0110
 * @tc.name      : Random width and height AV1 decode (1920x1080)
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0110, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH2;
        vDecSample->DEFAULT_WIDTH = WidthRand();
        cout << "rand width is: " << vDecSample->DEFAULT_WIDTH << endl;
        vDecSample->DEFAULT_HEIGHT = HighRand();
        cout << "rand high is" << vDecSample->DEFAULT_HEIGHT << endl;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0120
 * @tc.name      : Decode AV1 streams with surface and buffer mode successively
 * @tc.desc      : reli test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0120, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH;
        vDecSample->DEFAULT_WIDTH = 720;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDecForAv1(g_codecNameAv1));
        vDecSample->WaitForEOS();
        close(vDecSample->g_fd);
        vDecSample->g_fd = -1;
        shared_ptr<VDecAPI11Sample> vDecSample2 = make_shared<VDecAPI11Sample>();
        vDecSample2->INP_DIR = FILE_PATH2;
        vDecSample2->DEFAULT_WIDTH = 1920;
        vDecSample2->DEFAULT_HEIGHT = 1080;
        vDecSample2->DEFAULT_FRAME_RATE = 30;
        vDecSample2->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample2->RunVideoDec_SurfaceForAv1(g_codecNameAv1));
        vDecSample2->WaitForEOS();
        close(vDecSample2->g_fd);
        vDecSample2->g_fd = -1;
    }
}

/**
 * @tc.number    : VIDEO_AV1DEC_STABILITY_0130
 * @tc.name      : Create and destroy AV1 decoder repeatedly
 * @tc.desc      : reliable test
 */
HWTEST_F(Av1DecReliNdkTest, VIDEO_AV1DEC_STABILITY_0130, TestSize.Level3)
{
    shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = FILE_PATH;
    for (int i = 0; i < 1000; i++) {
        OH_AVCodec *vdec_ = OH_VideoDecoder_CreateByName(g_codecNameAv1.c_str());
        ASSERT_NE(nullptr, vdec_);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 720);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 480);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 30);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
        OH_VideoDecoder_Stop(vdec_);
        OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
}
} // namespace