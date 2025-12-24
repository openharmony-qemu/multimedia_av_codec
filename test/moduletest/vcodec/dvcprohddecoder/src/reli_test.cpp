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
using StreamType = OHOS::Media::VDecAPI11Sample::StreamType;

namespace OHOS {
namespace Media {
class DvcprohdDecReliNdkTest : public testing::Test {
public:
    static void SetUpTestCase();    // 第一个测试用例执行前
    static void TearDownTestCase(); // 最后一个测试用例执行后
    void SetUp() override;          // 每个测试用例执行前
    void TearDown() override;       // 每个测试用例执行后
    void Release();
    int32_t Stop();

protected:
    OH_AVCodec *vdec_ = nullptr;
    const char *inpDir1080Array[16] = {
        "/data/test/media/DVCPROHD_960x720_30_422_dvhp.mov"};
};
} // namespace Media
} // namespace OHOS

static string g_codecNameDvcprohd = "";
static int32_t g_reliCount = 16;
const char *DVCNTS_FILE_PATH = "/data/test/media/DVCNTSC_720x480_25_422_dv5n.mov";
const char *DVCPAL_FILE_PATH = "/data/test/media/DVCPAL_720x576_25_411_dvpp.mov";
const char *DVCPROHD_FILE_PATH = "/data/test/media/DVCPROHD_960x720_30_422_dvhp.mov";
void DvcprohdDecReliNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_DVVIDEO, false, SOFTWARE);
    g_codecNameDvcprohd = OH_AVCapability_GetName(cap);
    cout << "g_codecNameDvcprohd: " << g_codecNameDvcprohd << endl;
}

void DvcprohdDecReliNdkTest::TearDownTestCase() {}
void DvcprohdDecReliNdkTest::SetUp() {}
void DvcprohdDecReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0010
 * @tc.name      : Dvcprohd soft decode successively 16 times
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0010, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();

        const char *file = DVCNTS_FILE_PATH;
        vDecSample->getFormat(file);
        vDecSample->streamType = StreamType::DVNTSC;
        vDecSample->sfOutput = false;
        vDecSample->sleepOnFPS = true;
        vDecSample->nocaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDvcprohd.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0020
 * @tc.name      : Dvcprohd soft decode 16 times surface mode, change videoSize to 1920x1080
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0020, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->getFormat(DVCPROHD_FILE_PATH);
        vDecSample->streamType = StreamType::DVCPRO_HD;
        vDecSample->defaultWidth = 960;
        vDecSample->defaultHeight = 720;
        vDecSample->defaultFrameRate = 30;
        vDecSample->sleepOnFPS = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sfOutput = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDvcprohd.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
    }
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0030
 * @tc.name      : 16 instances Dvcprohd soft decode with Surface mode
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0030, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->streamType = StreamType::DVCPRO_HD;
        vDecSample->getFormat(inpDir1080Array[i % 1]);
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        vDecSample->nocaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDvcprohd));
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
            {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0040
 * @tc.name      : 16 instances Dvcprohd soft decode with buffer mode
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0040, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->streamType = StreamType::DVCPRO_HD;
        vDecSample->getFormat(inpDir1080Array[i % 1]);
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        vDecSample->nocaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameDvcprohd.c_str()));
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
            {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0050
 * @tc.name      : Dvcprohd soft decode 16 times with fast Surface change
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0050, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->getFormat(DVCPROHD_FILE_PATH);
        vDecSample->sfOutput = true;
        vDecSample->streamType = StreamType::DVCPRO_HD;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDvcprohd));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
    }
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0060
 * @tc.name      : Decode 16 MSvideo1 streams simultaneously in Surface mode
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0060, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->streamType = StreamType::DVCPRO_HD;
        vDecSample->getFormat(inpDir1080Array[i % 1]);
        vDecSample->sfOutput = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDvcprohd));
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
            {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0070
 * @tc.name      : Decode 16 Dvcprohd streams simultaneously in buffer mode
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0070, TestSize.Level3)
{
    vector<shared_ptr<VDecAPI11Sample>> decVec;
    for (int i = 0; i < g_reliCount; i++)
    {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        decVec.push_back(vDecSample);
        vDecSample->streamType = StreamType::DVNTSC;
        vDecSample->sfOutput = false;
        vDecSample->getFormat(DVCNTS_FILE_PATH);
        vDecSample->sleepOnFPS = true;
        vDecSample->nocaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDvcprohd));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    }
    uint32_t errorCount = 0;
    for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample)
            {
        sample->WaitForEOS();
        errorCount += sample->errCount; });
    decVec.clear();
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0080
 * @tc.name      : 16 instances Dvcprohd soft decode with buffer mode × 5 loops
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0080, TestSize.Level3)
{
    for (int j = 0; j < 5; j++)
    {
        vector<shared_ptr<VDecAPI11Sample>> decVec;
        for (int i = 0; i < g_reliCount; i++)
        {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            decVec.push_back(vDecSample);
            vDecSample->sfOutput = false;
            vDecSample->streamType = StreamType::DVCPRO_HD;
            vDecSample->getFormat(DVCPAL_FILE_PATH);
            vDecSample->sleepOnFPS = true;
            vDecSample->nocaleHash = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDvcprohd));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
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
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0090
 * @tc.name      : Decode Dvcprohd streams simultaneously
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0090, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->getFormat(DVCPROHD_FILE_PATH);
        vDecSample->sleepOnFPS = true;
        vDecSample->streamType = StreamType::DVCPRO_HD;
        vDecSample->nocaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameDvcprohd));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        vDecSample->ReleaseInFile();
        shared_ptr<VDecAPI11Sample> vDecSample1 = make_shared<VDecAPI11Sample>();
        vDecSample1->getFormat(DVCPROHD_FILE_PATH);
        vDecSample1->sleepOnFPS = true;
        vDecSample1->nocaleHash = true;
        vDecSample1->streamType = StreamType::DVCPRO_HD;
        ASSERT_EQ(AV_ERR_OK, vDecSample1->RunVideoDec(g_codecNameDvcprohd));
        vDecSample1->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample1->errCount);
        vDecSample1->ReleaseInFile();
    }
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0100
 * @tc.name      : Decode 10-bit and 8-bit Dvcprohd streams simultaneously
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0100, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->getFormat(DVCPROHD_FILE_PATH);
        vDecSample->streamType = StreamType::DVCPRO_HD;
        vDecSample->sfOutput = false;
        vDecSample->nocaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameDvcprohd));
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
        shared_ptr<VDecAPI11Sample> vDecSample1 = make_shared<VDecAPI11Sample>();
        vDecSample1->getFormat(DVCPROHD_FILE_PATH);
        vDecSample1->sfOutput = true;
        vDecSample1->streamType = StreamType::DVCPRO_HD;
        ASSERT_EQ(AV_ERR_OK, vDecSample1->RunVideoDec_Surface(g_codecNameDvcprohd));
        vDecSample1->WaitForEOS();
        vDecSample1->ReleaseInFile();
    }
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0110
 * @tc.name      : Random width and height Dvcprohd decode
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0110, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->getFormat(DVCPROHD_FILE_PATH);
        vDecSample->streamType = StreamType::DVCPRO_HD;
        vDecSample->defaultWidth = WidthRand();
        cout << "rand width is: " << vDecSample->defaultWidth << endl;
        vDecSample->defaultHeight = HighRand();
        cout << "rand high is: " << vDecSample->defaultHeight << endl;
        vDecSample->defaultFrameRate = 30;
        vDecSample->sfOutput = false;
        vDecSample->nocaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDvcprohd));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0120
 * @tc.name      : Decode MSVideo1 streams with surface and buffer mode successively
 * @tc.desc      : reli test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0120, TestSize.Level3)
{
    for (int i = 0; i < g_reliCount; i++)
    {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->getFormat(DVCPROHD_FILE_PATH);
        vDecSample->streamType = StreamType::DVCPRO_HD;
        vDecSample->defaultWidth = 1920;
        vDecSample->defaultHeight = 1080;
        vDecSample->defaultFrameRate = 30;
        vDecSample->sfOutput = false;
        vDecSample->nocaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameDvcprohd));
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
        shared_ptr<VDecAPI11Sample> vDecSample2 = make_shared<VDecAPI11Sample>();
        vDecSample2->getFormat(DVCPROHD_FILE_PATH);
        vDecSample2->streamType = StreamType::DVCPRO_HD;
        vDecSample2->defaultWidth = 1920;
        vDecSample2->defaultHeight = 1080;
        vDecSample2->defaultFrameRate = 30;
        vDecSample2->sfOutput = false;
        vDecSample2->nocaleHash = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample2->RunVideoDec_Surface(g_codecNameDvcprohd));
        vDecSample2->WaitForEOS();
        vDecSample2->ReleaseInFile();
    }
}
/**
 * @tc.number    : VIDEO_DVCPROHDEC_STABILITY_0130
 * @tc.name      : Create and destroy Dvcprohd decoder repeatedly
 * @tc.desc      : reliable test
 */
HWTEST_F(DvcprohdDecReliNdkTest, VIDEO_DVCPROHDEC_STABILITY_0130, TestSize.Level3)
{
    shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(DVCPROHD_FILE_PATH);
    vDecSample->streamType = StreamType::DVCPRO_HD;
    for (int i = 0; i < 1000; i++) {
        OH_AVCodec *vdec_ = OH_VideoDecoder_CreateByName(g_codecNameDvcprohd.c_str());
        ASSERT_NE(nullptr, vdec_);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 960);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 720);
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