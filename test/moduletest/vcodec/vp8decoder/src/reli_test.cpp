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
class Vp8DecReliNdkTest : public testing::Test {
public:
    static void SetUpTestCase();    // 第一个测试用例执行前
    static void TearDownTestCase(); // 最后一个测试用例执行后
    void SetUp() override;          // 每个测试用例执行前
    void TearDown() override;       // 每个测试用例执行后
    void Release();
    int32_t Stop();

protected:
    OH_AVCodec *vdec_;
    const char *inpDir480Array[16] = {
        "/data/test/media/vp8_720x480_30fps.ivf", "/data/test/media/vp8_720x480_30fps_1.ivf",
        "/data/test/media/vp8_720x480_30fps_2.ivf", "/data/test/media/vp8_720x480_30fps_3.ivf",
        "/data/test/media/vp8_720x480_30fps_4.ivf", "/data/test/media/vp8_720x480_30fps_5.ivf",
        "/data/test/media/vp8_720x480_30fps_6.ivf", "/data/test/media/vp8_720x480_30fps_7.ivf",
        "/data/test/media/vp8_720x480_30fps_8.ivf", "/data/test/media/vp8_720x480_30fps_9.ivf",
        "/data/test/media/vp8_720x480_30fps_10.ivf", "/data/test/media/vp8_720x480_30fps_11.ivf",
        "/data/test/media/vp8_720x480_30fps_12.ivf", "/data/test/media/vp8_720x480_30fps_13.ivf",
        "/data/test/media/vp8_720x480_30fps_14.ivf", "/data/test/media/vp8_720x480_30fps_15.ivf",
    };
};
} // namespace Media
} // namespace OHOS

static string g_codecNameVp8 = "";
int32_t g_reliCount = 16;
int32_t g_reliCount1000 = 1000;
const char *FILE_PATH = "/data/test/media/vp8_720x480_30fps.ivf";
const char *INP_DIR_1080 = "/data/test/media/vp8_1080p.ivf";
void Vp8DecReliNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_VP8, false, SOFTWARE);
    g_codecNameVp8 = OH_AVCapability_GetName(cap);
    cout << "g_codecNameVp8: " << g_codecNameVp8 << endl;
}

void Vp8DecReliNdkTest::TearDownTestCase() {}
void Vp8DecReliNdkTest::SetUp() {}
void Vp8DecReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_VP8DEC_STABILITY_0010
 * @tc.name      : Vp8 soft decode 16 times sequentially
 * @tc.desc      : reli test
 */
HWTEST_F(Vp8DecReliNdkTest, VIDEO_VP8DEC_STABILITY_0010, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH;
        vDecSample->SF_OUTPUT = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameVp8.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}
/**
 * @tc.number    : VIDEO_VP8DEC_STABILITY_0020
 * @tc.name      : Vp8 decode with rapid surface changes
 * @tc.desc      : reli test
 */
HWTEST_F(Vp8DecReliNdkTest, VIDEO_VP8DEC_STABILITY_0020, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->SF_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameVp8.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_VP8DEC_STABILITY_0030
 * @tc.name      : Decode 16 Vp8 streams with surface mode
 * @tc.desc      : reli test
 */
HWTEST_F(Vp8DecReliNdkTest, VIDEO_VP8DEC_STABILITY_0030, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->INP_DIR = inpDir480Array[i];
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameVp8));
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
             {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}
/**
 * @tc.number    : VIDEO_VP8DEC_STABILITY_0040
 * @tc.name      : Decode 16 Vp8 streams with buffer mode
 * @tc.desc      : reli test
 */
HWTEST_F(Vp8DecReliNdkTest, VIDEO_VP8DEC_STABILITY_0040, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->INP_DIR = inpDir480Array[i];
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameVp8.c_str()));
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
             {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}

/**
 * @tc.number    : VIDEO_VP8DEC_STABILITY_0080
 * @tc.name      : Decode 16 Vp8 streams in buffer mode across 5 loops
 * @tc.desc      : reli test
 */
HWTEST_F(Vp8DecReliNdkTest, VIDEO_VP8DEC_STABILITY_0080, TestSize.Level3)
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
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameVp8));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
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
 * @tc.number    : VIDEO_VP8DEC_STABILITY_0090
 * @tc.name      : Simultaneous decoding of Vp8 streams
 * @tc.desc      : reli test
 */
HWTEST_F(Vp8DecReliNdkTest, VIDEO_VP8DEC_STABILITY_0090, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameVp8));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        vDecSample->ReleaseInFile();
        shared_ptr<VDecAPI11Sample> vDecSample1 = make_shared<VDecAPI11Sample>();
        vDecSample1->INP_DIR = INP_DIR_1080;
        vDecSample1->sleepOnFPS = true;
        vDecSample1->DEFAULT_WIDTH = 1920;
        vDecSample1->DEFAULT_HEIGHT = 1080;
        ASSERT_EQ(AV_ERR_OK, vDecSample1->RunVideoDec(g_codecNameVp8));
        vDecSample1->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample1->errCount);
        vDecSample1->ReleaseInFile();
    }
}
/**
 * @tc.number    : VIDEO_VP8DEC_STABILITY_0100
 * @tc.name      : Simultaneous decoding with surface and buffer modes
 * @tc.desc      : reli test
 */
HWTEST_F(Vp8DecReliNdkTest, VIDEO_VP8DEC_STABILITY_0100, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameVp8));
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
        shared_ptr<VDecAPI11Sample> vDecSample1 = make_shared<VDecAPI11Sample>();
        vDecSample1->INP_DIR = INP_DIR_1080;
        vDecSample1->SF_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample1->RunVideoDec_Surface(g_codecNameVp8));
        vDecSample1->WaitForEOS();
        vDecSample1->ReleaseInFile();
    }
}
/**
 * @tc.number    : VIDEO_VP8DEC_STABILITY_0110
 * @tc.name      : Decode Vp8 streams with random resolutions
 * @tc.desc      : reli test
 */
HWTEST_F(Vp8DecReliNdkTest, VIDEO_VP8DEC_STABILITY_0110, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = FILE_PATH;
        vDecSample->DEFAULT_WIDTH = WidthRand();
        cout << "rand width is: " << vDecSample->DEFAULT_WIDTH << endl;
        vDecSample->DEFAULT_HEIGHT = HighRand();
        cout << "rand high is" << vDecSample->DEFAULT_HEIGHT << endl;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameVp8));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_VP8DEC_STABILITY_0120
 * @tc.name      : Decode common Vp8 streams with different modes
 * @tc.desc      : reli test
 */
HWTEST_F(Vp8DecReliNdkTest, VIDEO_VP8DEC_STABILITY_0120, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameVp8));
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
        shared_ptr<VDecAPI11Sample> vDecSample2 = make_shared<VDecAPI11Sample>();
        vDecSample2->INP_DIR = INP_DIR_1080;
        vDecSample2->DEFAULT_WIDTH = 1920;
        vDecSample2->DEFAULT_HEIGHT = 1080;
        vDecSample2->DEFAULT_FRAME_RATE = 30;
        vDecSample2->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample2->RunVideoDec_Surface(g_codecNameVp8));
        vDecSample2->WaitForEOS();
        vDecSample2->ReleaseInFile();
    }
}

/**
 * @tc.number    : VIDEO_VP8DEC_STABILITY_0130
 * @tc.name      : Repeated creation and destruction of Vp8 decoder
 * @tc.desc      : reli test
 */
HWTEST_F(Vp8DecReliNdkTest, VIDEO_VP8DEC_STABILITY_0130, TestSize.Level3)
{
    for (int i = 0; i < 1000; i++) {
        OH_AVCodec *vdec_ = OH_VideoDecoder_CreateByName(g_codecNameVp8.c_str());
        ASSERT_NE(nullptr, vdec_);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 1920);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 30);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
        OH_VideoDecoder_Stop(vdec_);
        OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }
}
} // namespace