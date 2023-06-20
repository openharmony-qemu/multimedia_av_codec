#include "gtest/gtest.h"
#include "videoenc_ndk_sample.h"
#include "native_avcodec_videoencoder.h"
#include "native_avcodec_base.h"
#include <iostream>
#include <stdio.h>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class EncReliNdkTest : public testing::Test {
public:
    static void SetUpTestCase();    // 第一个测试用例执行前
    static void TearDownTestCase(); // 最后一个测试用例执行后
    void SetUp() override;          // 每个测试用例执行前
    void TearDown() override;       // 每个测试用例执行后
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();

protected:
    const char *CODEC_NAME_AVC = "OMX.hisi.video.encoder.avc";
    const char *INP_DIR_720 = "/data/test/media/1280_720_nv.yuv";
    const char *INP_DIR_720_ARRAY[16] = {"/data/test/media/1280_720_nv.yuv",    "/data/test/media/1280_720_nv_1.yuv",
                                         "/data/test/media/1280_720_nv_2.yuv",  "/data/test/media/1280_720_nv_3.yuv",
                                         "/data/test/media/1280_720_nv_7.yuv",  "/data/test/media/1280_720_nv_10.yuv",
                                         "/data/test/media/1280_720_nv_13.yuv", "/data/test/media/1280_720_nv_4.yuv",
                                         "/data/test/media/1280_720_nv_8.yuv",  "/data/test/media/1280_720_nv_11.yuv",
                                         "/data/test/media/1280_720_nv_14.yuv", "/data/test/media/1280_720_nv_5.yuv",
                                         "/data/test/media/1280_720_nv_9.yuv",  "/data/test/media/1280_720_nv_12.yuv",
                                         "/data/test/media/1280_720_nv_15.yuv", "/data/test/media/1280_720_nv_6.yuv"};
    bool createCodecSuccess_ = false;
    OH_AVCodec *vdec_;
};
} // namespace Media
} // namespace OHOS

void EncReliNdkTest::SetUpTestCase() {}

void EncReliNdkTest::TearDownTestCase() {}

void EncReliNdkTest::SetUp() {}

void EncReliNdkTest::TearDown() {}

HWTEST_F(EncReliNdkTest, VIDEO_HWENC_PERFORMANCE_WHILE_0100, TestSize.Level3)
{
    while (true) {
        shared_ptr<VEncNdkSample> vEncSample = make_shared<VEncNdkSample>();
        vEncSample->INP_DIR = INP_DIR_720;
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

HWTEST_F(EncReliNdkTest, VIDEO_HWENC_PERFORMANCE_WHILE_0200, TestSize.Level3)
{
    for (int i = 0; i < 16; i++) {
        VEncNdkSample *vEncSample = new VEncNdkSample();
        vEncSample->INP_DIR = INP_DIR_720_ARRAY[i];
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        vEncSample->repeatRun = true;
        vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        if (i == 15) {
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

HWTEST_F(EncReliNdkTest, VIDEO_HWENC_PERFORMANCE_WHILE_0300, TestSize.Level3)
{
    shared_ptr<VEncNdkSample> vEncSample = make_shared<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->repeatRun = true;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncReliNdkTest, VIDEO_HWENC_PERFORMANCE_WHILE_0400, TestSize.Level3)
{
    while (true) {
        vector<shared_ptr<VEncNdkSample>> encVec;
        for (int i = 0; i < 16; i++) {
            auto vEncSample = make_shared<VEncNdkSample>();
            encVec.push_back(vEncSample);
            vEncSample->INP_DIR = INP_DIR_720_ARRAY[i];
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        }
        for (int i = 0; i < 16; i++) {
            encVec[i]->WaitForEOS();
        }
    }
}

// /**
//  * @tc.number    : VIDEO_HWDEC_PERFORMANCE_WHILE_0200
//  * @tc.name      :
//  * @tc.desc      : perf test
//  */
// HWTEST_F(EncReliNdkTest, VIDEO_HWDEC_PERFORMANCE_WHILE_0200, TestSize.Level3)
// {
//     for(int i=0;i<16;i++) {
//         VEncNdkSample *vDecSample = new VEncNdkSample();
//         vDecSample->SURFACE_OUTPUT = false;
//         vDecSample->INP_DIR = INP_DIR_720_30_ARRAY[i];
//         vDecSample->DEFAULT_WIDTH = 1280;
//         vDecSample->DEFAULT_HEIGHT = 720;
//         vDecSample->DEFAULT_FRAME_RATE = 30;
//         vDecSample->sleepOnFPS = true;
//         ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(CODEC_NAME));
//         ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
//         ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
//         ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderNdkTest());
//         if(i == 15) {
//             vDecSample->WaitForEOS();
//             ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
//         }
//     }
// }

// /**
//  * @tc.number    : VIDEO_HWDEC_PERFORMANCE_WHILE_0300
//  * @tc.name      :
//  * @tc.desc      : perf test
//  */
// HWTEST_F(EncReliNdkTest, VIDEO_HWDEC_PERFORMANCE_WHILE_0300, TestSize.Level3)
// {
//     shared_ptr<VEncNdkSample> vDecSample = make_shared<VEncNdkSample>();
//     vDecSample->SURFACE_OUTPUT = false;
//     vDecSample->INP_DIR = INP_DIR_720_30;
//     vDecSample->DEFAULT_WIDTH = 1280;
//     vDecSample->DEFAULT_HEIGHT = 720;
//     vDecSample->DEFAULT_FRAME_RATE = 30;
//     vDecSample->sleepOnFPS = true;
//     vDecSample->repeatRun = true;
//     ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(CODEC_NAME));
//     ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
//     ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
//     ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderNdkTest());
//     vDecSample->WaitForEOS();
//     ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
// }

// /**
//  * @tc.number    : VIDEO_HWDEC_PERFORMANCE_WHILE_0400
//  * @tc.name      :
//  * @tc.desc      : perf test
//  */
// HWTEST_F(EncReliNdkTest, VIDEO_HWDEC_PERFORMANCE_WHILE_0400, TestSize.Level3)
// {
//     while (true) {
//         vector<shared_ptr<VEncNdkSample>> decVec;
//         for(int i=0;i<16;i++) {
//             auto vDecSample = make_shared<VEncNdkSample>();
//             decVec.push_back(vDecSample);
//             vDecSample->SURFACE_OUTPUT = false;
//             vDecSample->INP_DIR = INP_DIR_720_30_ARRAY[i];
//             vDecSample->DEFAULT_WIDTH = 1280;
//             vDecSample->DEFAULT_HEIGHT = 720;
//             vDecSample->DEFAULT_FRAME_RATE = 30;
//             vDecSample->sleepOnFPS = true;
//             vDecSample->repeatRun = true;
//             ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(CODEC_NAME));
//             ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
//             ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
//             ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderNdkTest());
//             if(i == 15) {
//                 vDecSample->WaitForEOS();
//                 ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
//             }
//         }
//     }
// }