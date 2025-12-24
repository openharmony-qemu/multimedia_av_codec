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
class Mpeg1DecReliNdkTest : public testing::Test {
public:
    static void SetUpTestCase();    // 第一个测试用例执行前
    static void TearDownTestCase(); // 最后一个测试用例执行后
    void SetUp() override;          // 每个测试用例执行前
    void TearDown() override;       // 每个测试用例执行后
    void Release();
    int32_t Stop();

protected:
    OH_AVCodec *vdec_;
    const char *inpDir1080Array[16] = {
        "/data/test/media/mpeg1.mkv"};
};
} // namespace Media
} // namespace OHOS

static string g_codecNameMpeg1 = "";
int32_t g_reliCount = 16;
int32_t g_reliCount1000 = 1000;
const char *FILE_PATH = "/data/test/media/mpeg1.mkv";
void Mpeg1DecReliNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MPEG1, false, SOFTWARE);
    g_codecNameMpeg1 = OH_AVCapability_GetName(cap);
    cout << "g_codecNameMpeg1: " << g_codecNameMpeg1 << endl;
}

void Mpeg1DecReliNdkTest::TearDownTestCase() {}
void Mpeg1DecReliNdkTest::SetUp() {}
void Mpeg1DecReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0010
 * @tc.name      : MPEG1 soft decode successively 16 times
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0010, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->inputDir = FILE_PATH;
        vDecSample->getFormat(FILE_PATH);
        vDecSample->sfOutput = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg1.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0020
 * @tc.name      : MPEG1 soft decode 16 times surface mode, change videoSize to 1920x1080
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0020, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->inputDir = FILE_PATH;
        vDecSample->getFormat(FILE_PATH);
        vDecSample->defaultWidth = 1920;
        vDecSample->defaultHeight = 1080;
        vDecSample->defaultFrameRate = 30;
        vDecSample->sleepOnFPS = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sfOutput = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg1.c_str()));
        vDecSample->WaitForEOS();
    }
}
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0030
 * @tc.name      : 16 instances MPEG1 soft decode with Surface mode
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0030, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->inputDir = inpDir1080Array[i % 1];
        vDecSample->getFormat(FILE_PATH);
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        vDecSample->noCaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg1));
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
            {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0040
 * @tc.name      : 16 instances MPEG1 soft decode with buffer mode
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0040, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->inputDir = inpDir1080Array[i % 1];
        vDecSample->getFormat(FILE_PATH);
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        vDecSample->noCaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg1.c_str()));
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
            {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0050
 * @tc.name      : MPEG1 soft decode 16 times with fast Surface change
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0050, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->inputDir = FILE_PATH;
        vDecSample->getFormat(FILE_PATH);
        vDecSample->sfOutput = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
    }
}
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0060
 * @tc.name      : Decode 16 MPEG1 streams simultaneously in Surface mode
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0060, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->inputDir = inpDir1080Array[i % 1];
        vDecSample->getFormat(FILE_PATH);
        vDecSample->sfOutput = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg1));
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
            {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0070
 * @tc.name      : Decode 16 Mpeg1 streams simultaneously in buffer mode
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0070, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->sfOutput = false;
        vDecSample->inputDir = FILE_PATH;
        vDecSample->sleepOnFPS = true;
        vDecSample->noCaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg1));
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
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0080
 * @tc.name      : 16 instances MPEG1 soft decode with buffer mode × 5 loops
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0080, TestSize.Level3)
{
    for (int j = 0; j < 5; j++)
    {
        vector<shared_ptr<VDecAPI11Sample>> decVec;
        for (int i = 0; i < g_reliCount; i++)
        {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            decVec.push_back(vDecSample);
            vDecSample->sfOutput = false;
            vDecSample->inputDir = FILE_PATH;
            vDecSample->sleepOnFPS = true;
            vDecSample->noCaleHash = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg1));
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
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0090
 * @tc.name      : Decode 10-bit and 8-bit MPEG1 streams simultaneously
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0090, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->inputDir = FILE_PATH;
        vDecSample->getFormat(FILE_PATH);
        vDecSample->sfOutput = false;
        vDecSample->noCaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg1));
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
        shared_ptr<VDecAPI11Sample> vDecSample1 = make_shared<VDecAPI11Sample>();
        vDecSample1->inputDir = FILE_PATH;
        vDecSample1->getFormat(FILE_PATH);
        vDecSample1->sfOutput = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample1->RunVideoDec_Surface(g_codecNameMpeg1));
        vDecSample1->WaitForEOS();
        vDecSample1->ReleaseInFile();
    }
}
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0100
 * @tc.name      : Random width and height MPEG1 decode (720x480)
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0100, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->inputDir = FILE_PATH;
        vDecSample->defaultWidth = WidthRand();
        cout << "rand width is: " << vDecSample->defaultWidth << endl;
        vDecSample->defaultHeight = HighRand();
        cout << "rand high is" << vDecSample->defaultHeight << endl;
        vDecSample->defaultFrameRate = 60;
        vDecSample->sfOutput = false;
        vDecSample->noCaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0110
 * @tc.name      : Decode MPEG1 streams with surface and buffer mode successively
 * @tc.desc      : reli test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0110, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->inputDir = FILE_PATH;
        vDecSample->getFormat(FILE_PATH);
        vDecSample->defaultWidth = 720;
        vDecSample->defaultHeight = 480;
        vDecSample->defaultFrameRate = 60;
        vDecSample->sfOutput = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg1));
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
        shared_ptr<VDecAPI11Sample> vDecSample2 = make_shared<VDecAPI11Sample>();
        vDecSample2->inputDir = FILE_PATH;
        vDecSample2->getFormat(FILE_PATH);
        vDecSample2->defaultWidth = 720;
        vDecSample2->defaultHeight = 480;
        vDecSample2->defaultFrameRate = 60;
        vDecSample2->sfOutput = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample2->RunVideoDec_Surface(g_codecNameMpeg1));
        vDecSample2->WaitForEOS();
        vDecSample2->ReleaseInFile();
    }
}
/**
 * @tc.number    : VIDEO_MPEG1DEC_STABILITY_0120
 * @tc.name      : Create and destroy MPEG1 decoder repeatedly
 * @tc.desc      : reliable test
 */
HWTEST_F(Mpeg1DecReliNdkTest, VIDEO_MPEG1DEC_STABILITY_0120, TestSize.Level3)
{
    shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = FILE_PATH;
        vDecSample->getFormat(FILE_PATH);
    for (int i = 0; i < 1000; i++) {
        OH_AVCodec *vdec_ = OH_VideoDecoder_CreateByName(g_codecNameMpeg1.c_str());
        ASSERT_NE(nullptr, vdec_);
        (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_WIDTH, 720);
        (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_HEIGHT, 480);
        (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_FRAME_RATE, 60);
        (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, vDecSample->trackFormat));
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
        OH_VideoDecoder_Stop(vdec_);
        OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }
}
} // namespace