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
#include "videodec_sample.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class MjpegSwdecReliNdkTest : public testing::Test {
public:
    static void SetUpTestCase();    // 第一个测试用例执行前
    static void TearDownTestCase(); // 最后一个测试用例执行后
    void SetUp() override;          // 每个测试用例执行前
    void TearDown() override;       // 每个测试用例执行后
    void Release();
    int32_t Stop();

protected:
    OH_AVCodec *vdec_;
    const char *INP_DIR_1080_30 = "/data/test/media/1920_1080_30.avi";
    const char *INP_DIR_144 = "/data/test/media/176_144_Main10.avi";
    const char *INP_DIR_64 = "/data/test/media/64_64_Main.avi";
};
} // namespace Media
} // namespace OHOS

static string g_codecNameMjpeg = "";
int32_t g_reliCount = 16;
int32_t g_reliCount1000 = 1000;
void MjpegSwdecReliNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(
        "video/mjpeg", false, SOFTWARE);
    g_codecNameMjpeg = OH_AVCapability_GetName(cap);
    cout << "g_codecNameMjpeg: " << g_codecNameMjpeg << endl;
}

void MjpegSwdecReliNdkTest::TearDownTestCase() {}
void MjpegSwdecReliNdkTest::SetUp() {}
void MjpegSwdecReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_MJPEGSWDEC_STABILITY_0010
 * @tc.name      : h265 soft decode successively 16 times
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, VIDEO_MJPEGSWDEC_STABILITY_0010, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->SF_OUTPUT = false;
            vDecSample->INP_DIR = INP_DIR_1080_30;
            const char *file = "/data/test/media/1920_1080_30.avi";
            vDecSample->getFormat(file);
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMjpeg.c_str()));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_MJPEGSWDEC_STABILITY_0020
 * @tc.name      : h265 soft decode 16 times successively with Surface change fastly
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, VIDEO_MJPEGSWDEC_STABILITY_0020, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_1080_30;
            const char *file = "/data/test/media/1920_1080_30.avi";
            vDecSample->getFormat(file);
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->sleepOnFPS = true;
            vDecSample->autoSwitchSurface = true;
            vDecSample->SF_OUTPUT = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMjpeg));
            ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
            vDecSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : VIDEO_MJPEGSWDEC_STABILITY_0060
 * @tc.name      : repeat start flush before eos 1000 time
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, VIDEO_MJPEGSWDEC_STABILITY_0060, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        const char *file = "/data/test/media/1920_1080_30.avi";
        vDecSample->getFormat(file);
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->REPEAT_START_FLUSH_BEFORE_EOS = 1000;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMjpeg));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_VIDEO_MJPEGSWDEC_STABILITY_0010
 * @tc.name      : h265 soft decode 16 times successively
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, API11_VIDEO_MJPEGSWDEC_STABILITY_0010, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->SF_OUTPUT = false;
            vDecSample->INP_DIR = INP_DIR_1080_30;
            const char *file = "/data/test/media/1920_1080_30.avi";
            vDecSample->getFormat(file);
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMjpeg));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : API11_VIDEO_MJPEGSWDEC_STABILITY_0020
 * @tc.name      : h265 soft decode 16 time successively with Surface change fastly
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, API11_VIDEO_MJPEGSWDEC_STABILITY_0020, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_1080_30;
            const char *file = "/data/test/media/1920_1080_30.avi";
            vDecSample->getFormat(file);
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->autoSwitchSurface = true;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMjpeg));
            ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
            vDecSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : API11_VIDEO_MJPEGSWDEC_STABILITY_0040
 * @tc.name      : 16 Multiple instances h265 soft decode with buffer mode
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, API11_VIDEO_MJPEGSWDEC_STABILITY_0040, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        vector<shared_ptr<VDecAPI11Sample>> decVec;
        for (int i = 0; i < g_reliCount; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            decVec.push_back(vDecSample);
            vDecSample->SF_OUTPUT = false;
            vDecSample->INP_DIR = INP_DIR_1080_30;
            const char *file = "/data/test/media/1920_1080_30.avi";
            vDecSample->getFormat(file);
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMjpeg));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        }
        uint32_t errorCount = 0;
        for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample) {
            sample->WaitForEOS();
            errorCount += sample->errCount;
        });
        decVec.clear();
    }
}

/**
 * @tc.number    : API11_VIDEO_MJPEGSWDEC_STABILITY_0050
 * @tc.name      : 16 Multiple instances h265 soft decode with buffer mode × 5 LOOP
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, API11_VIDEO_MJPEGSWDEC_STABILITY_0050, TestSize.Level3)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int j = 0; j < 5; j++) {
            vector<shared_ptr<VDecAPI11Sample>> decVec;
            for (int i = 0; i < g_reliCount; i++) {
                auto vDecSample = make_shared<VDecAPI11Sample>();
                decVec.push_back(vDecSample);
                vDecSample->SF_OUTPUT = false;
                vDecSample->INP_DIR = INP_DIR_1080_30;
                const char *file = "/data/test/media/1920_1080_30.avi";
                vDecSample->getFormat(file);
                vDecSample->DEFAULT_WIDTH = 1920;
                vDecSample->DEFAULT_HEIGHT = 1080;
                vDecSample->DEFAULT_FRAME_RATE = 30;
                vDecSample->sleepOnFPS = true;
                ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMjpeg));
                ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
                ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
                ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            }
            uint32_t errorCount = 0;
            for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample) {
                sample->WaitForEOS();
                errorCount += sample->errCount;
            });
            decVec.clear();
        }
    }
}

/**
 * @tc.number    : API11_VIDEO_MJPEGSWDEC_STABILITY_0060
 * @tc.name      : repeat start and flush 1000 times before EOS
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, API11_VIDEO_MJPEGSWDEC_STABILITY_0060, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        const char *file = "/data/test/media/1920_1080_30.avi";
        vDecSample->getFormat(file);
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->REPEAT_START_FLUSH_BEFORE_EOS = 1000;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMjpeg));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_VIDEO_MJPEGSWDEC_STABILITY_0100
 * @tc.name      : 10bit stream and 8bit stream decode simultaneously
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, API11_VIDEO_MJPEGSWDEC_STABILITY_0100, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount; i++) {
            for (int j = 0; j < 2; j++) {
                shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
                vDecSample->INP_DIR = INP_DIR_144;
                const char *file = "/data/test/media/176_144_Main10.avi";
                vDecSample->getFormat(file);
                vDecSample->DEFAULT_WIDTH = 176;
                vDecSample->DEFAULT_HEIGHT = 144;
                vDecSample->DEFAULT_FRAME_RATE = 30;
                vDecSample->SF_OUTPUT = false;
                ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMjpeg));
                vDecSample->WaitForEOS();
            }
        }
    }
}

/**
 * @tc.number    : API11_VIDEO_MJPEGSWDEC_STABLITY_FUNC_0130
 * @tc.name      : rand and whith (176 * 144)
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, API11_VIDEO_MJPEGSWDEC_STABLITY_FUNC_0130, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount; i++) {
            shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_144;
            const char *file = "/data/test/media/176_144_Main10.avi";
            vDecSample->getFormat(file);
            vDecSample->DEFAULT_WIDTH = WidthRand();
            cout << "rand width is: " << vDecSample->DEFAULT_WIDTH << endl;
            vDecSample->DEFAULT_HEIGHT = HighRand();
            cout << "rand high is" << vDecSample->DEFAULT_HEIGHT << endl;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMjpeg));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : API11_VIDEO_MJPEGSWDEC_STABILITY_0110
 * @tc.name      : SVC stream and common stream decode simultaneously
 * @tc.desc      : reli test
 */
HWTEST_F(MjpegSwdecReliNdkTest, API11_VIDEO_MJPEGSWDEC_STABILITY_0110, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < g_reliCount; i++) {
            for (int j = 0; j < 2; j++) {
                shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
                vDecSample->INP_DIR = INP_DIR_1080_30;
                const char *file = "/data/test/media/1920_1080_30.avi";
                vDecSample->getFormat(file);
                vDecSample->DEFAULT_WIDTH = 1920;
                vDecSample->DEFAULT_HEIGHT = 1080;
                vDecSample->DEFAULT_FRAME_RATE = 30;
                vDecSample->SF_OUTPUT = false;
                ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMjpeg));
                vDecSample->WaitForEOS();
            }
        }
    }
}
} // namespace