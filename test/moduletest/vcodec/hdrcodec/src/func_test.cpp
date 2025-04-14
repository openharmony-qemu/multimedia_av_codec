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

#include <iostream>
#include <cstdio>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>

#include "gtest/gtest.h"
#include "hdrcodec_sample.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "native_avcodec_videodecoder.h"


using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace OHOS {
namespace Media {
class HDRFuncNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
};
namespace {
    static OH_AVCapability *cap_263 = nullptr;
    static string g_codecName263 = "";
    static OH_AVCapability *cap_264 = nullptr;
    static string g_codecName264 = "";
    static OH_AVCapability *cap_265 = nullptr;
    static string g_codecName265 = "";
    static OH_AVCapability *capSw264 = nullptr;
    static string g_codecNameSw264 = "";
    static OH_AVCapability *capSw265 = nullptr;
    static string g_codecNameSw265 = "";

    static OH_AVCapability *cap_266 = nullptr;
    static string g_codecName266 = "";
    static OH_AVCapability *cap_mpeg2 = nullptr;
    static string g_codecNameMpeg2 = "";
    static OH_AVCapability *cap_mpeg4 = nullptr;
    static string g_codecNameMpeg4 = "";

    static OH_AVCapability *capEnc_264 = nullptr;
    static string g_codecNameEnc264 = "";
    static OH_AVCapability *capEnc_265 = nullptr;
    static string g_codecNameEnc265 = "";
}
void HDRFuncNdkTest::SetUpTestCase()
{
    cap_263 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_H263, false, SOFTWARE);
    g_codecName263 = OH_AVCapability_GetName(cap_263);
    cout << "g_codecName263: " << g_codecName263 << endl;

    cap_264 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    g_codecName264 = OH_AVCapability_GetName(cap_264);
    cout << "g_codecName264: " << g_codecName264 << endl;

    capSw264 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    g_codecNameSw264 = OH_AVCapability_GetName(capSw264);
    cout << "g_codecNameSw264: " << g_codecNameSw264 << endl;

    cap_265 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    g_codecName265 = OH_AVCapability_GetName(cap_265);
    cout << "g_codecName265: " << g_codecName265 << endl;

    capSw265 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, SOFTWARE);
    g_codecNameSw265 = OH_AVCapability_GetName(capSw265);
    cout << "g_codecNameSw265: " << g_codecNameSw265 << endl;
    
    cap_266 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false, HARDWARE);
    g_codecName266 = OH_AVCapability_GetName(cap_266);
    cout << "g_codecName266: " << g_codecName266 << endl;

    cap_mpeg2 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MPEG2, false, SOFTWARE);
    g_codecNameMpeg2 = OH_AVCapability_GetName(cap_mpeg2);
    cout << "g_codecNameMpeg2: " << g_codecNameMpeg2 << endl;

    cap_mpeg4 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2, false, SOFTWARE);
    g_codecNameMpeg4 = OH_AVCapability_GetName(cap_mpeg4);
    cout << "g_codecNameMpeg4: " << g_codecNameMpeg4 << endl;

    capEnc_264 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    g_codecNameEnc264 = OH_AVCapability_GetName(capEnc_264);
    cout << "g_codecNameEnc264: " << g_codecNameEnc264 << endl;

    capEnc_265 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    g_codecNameEnc265 = OH_AVCapability_GetName(capEnc_265);
    cout << "g_codecNameEnc265: " << g_codecNameEnc265 << endl;
}
void HDRFuncNdkTest::TearDownTestCase() {}
void HDRFuncNdkTest::SetUp()
{
}
void HDRFuncNdkTest::TearDown()
{
}
} // namespace Media
} // namespace OHOS
namespace {
/**
 * @tc.number    : HDR_FUNC_0030
 * @tc.name      : decode PQ HDRVivid by display mode, and then set HEVC_PROFILE_MAIN10 to encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, HDR_FUNC_0010, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->INP_DIR = "/data/test/media/pq_vivid.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateCodec());
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
    }
}
/**
 * @tc.number    : HDR_FUNC_0020
 * @tc.name      : decode HLG HDRVivid by display mode, and then set HEVC_PROFILE_MAIN10 to encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, HDR_FUNC_0020, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->INP_DIR = "/data/test/media/hlg_vivid_4k.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateCodec());
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
    }
}

/**
 * @tc.number    : HDR_FUNC_0030
 * @tc.name      : decode PQ HDRVivid by display mode, and then set HEVC_PROFILE_MAIN to encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, HDR_FUNC_0030, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->INP_DIR = "/data/test/media/pq_vivid.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateCodec());
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        EXPECT_LE(0, sample->errorCount);
    }
}

/**
 * @tc.number    : HDR_FUNC_0040
 * @tc.name      : decode HLG HDRVivid by display mode, and then set HEVC_PROFILE_MAIN to encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, HDR_FUNC_0040, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->INP_DIR = "/data/test/media/hlg_vivid_4k.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateCodec());
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        EXPECT_LE(0, sample->errorCount);
    }
}

/**
 * @tc.number    : HDR_FUNC_0050
 * @tc.name      : decode and encode HDRVivid repeat start-stop 5 times before EOS
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, HDR_FUNC_0050, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->INP_DIR = "/data/test/media/hlg_vivid_4k.h265";
        sample->REPEAT_START_STOP_BEFORE_EOS = 5;
        ASSERT_EQ(AV_ERR_OK, sample->CreateCodec());
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
    }
}

/**
 * @tc.number    : HDR_FUNC_0060
 * @tc.name      : decode and encode HDRVivid repeat start-flush-stop 5 times before EOS
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, HDR_FUNC_0060, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->INP_DIR = "/data/test/media/hlg_vivid_4k.h265";
        sample->REPEAT_START_FLUSH_STOP_BEFORE_EOS = 5;
        ASSERT_EQ(AV_ERR_OK, sample->CreateCodec());
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
    }
}

/**
 * @tc.number    : HDR_FUNC_0070
 * @tc.name      : decode and encode HDRVivid repeat start-flush-start 5 times before EOS
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, HDR_FUNC_0070, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->INP_DIR = "/data/test/media/hlg_vivid_4k.h265";
        sample->REPEAT_START_FLUSH_BEFORE_EOS = 5;
        ASSERT_EQ(AV_ERR_OK, sample->CreateCodec());
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
    }
}

/**
 * @tc.number    : HDR_FUNC_0080
 * @tc.name      : decode and encode different resolution with same codec
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, HDR_FUNC_0080, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->INP_DIR = "/data/test/media/hlg_vivid_4k.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateCodec());
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        sample->DEFAULT_WIDTH = 1920;
        sample->DEFAULT_HEIGHT = 1080;
        sample->INP_DIR = "/data/test/media/hlg_vivid_1080p.h265";
        ASSERT_EQ(AV_ERR_OK, sample->ReConfigure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0010
 * @tc.name      : H263 swdecode and H264 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0010, TestSize.Level1)
{
    if (g_codecName263.find("H263") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 352;
        sample->DEFAULT_HEIGHT = 288;
        sample->typeDec = 1;
        sample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName263, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEC_ENC_FUNC_0020
 * @tc.name      : H264 hwdecode and H264 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0020, TestSize.Level1)
{
    if (g_codecName264.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1104;
        sample->DEFAULT_HEIGHT = 622;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/1104x622.h264";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName264, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0030
 * @tc.name      : H265 hwdecode and H264 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0030, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1920;
        sample->DEFAULT_HEIGHT = 1080;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/1920x1080.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName265, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0040
 * @tc.name      : H266 hwdecode and H264 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0040, TestSize.Level1)
{
    if (g_codecName266.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1280;
        sample->DEFAULT_HEIGHT = 720;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/1280_720_8.vvc";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName266, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0050
 * @tc.name      : Mpeg2 swdecode and H264 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0050, TestSize.Level1)
{
    if (g_codecNameMpeg2.find("MPEG2") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 352;
        sample->DEFAULT_HEIGHT = 288;
        sample->typeDec = 3;
        sample->INP_DIR = "/data/test/media/simple@low_level_352_288_30_rotation_90.m2v";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameMpeg2, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0060
 * @tc.name      : Mpeg4 swdecode and H264 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0060, TestSize.Level1)
{
    if (g_codecNameMpeg4.find("MPEG4") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1280;
        sample->DEFAULT_HEIGHT = 720;
        sample->typeDec = 4;
        sample->INP_DIR = "/data/test/media/mpeg4_1280x720_IPB.m4v";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameMpeg4, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEC_ENC_FUNC_0070
 * @tc.name      : H264 swdecode and H264 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0070, TestSize.Level1)
{
    if (g_codecNameSw264.find("AVC") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1104;
        sample->DEFAULT_HEIGHT = 622;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/1104x622.h264";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameSw264, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0080
 * @tc.name      : H265 swdecode and H264 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0080, TestSize.Level1)
{
    if (g_codecNameSw265.find("HEVC") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1920;
        sample->DEFAULT_HEIGHT = 1080;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/1920x1080.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameSw265, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0090
 * @tc.name      : H263 swdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0090, TestSize.Level1)
{
    if (g_codecName263.find("H263") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 352;
        sample->DEFAULT_HEIGHT = 288;
        sample->typeDec = 1;
        sample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName263, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEC_ENC_FUNC_0100
 * @tc.name      : H264 hwdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0100, TestSize.Level1)
{
    if (g_codecName264.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1104;
        sample->DEFAULT_HEIGHT = 622;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/1104x622.h264";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName264, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0110
 * @tc.name      : H265 hwdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0110, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1920;
        sample->DEFAULT_HEIGHT = 1080;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/1920x1080.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0111
 * @tc.name      : H265 HDRVivid hwdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0111, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->DEFAULT_WIDTH = 3840;
        sample->DEFAULT_HEIGHT = 2160;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/hlgHdrVivid_4k.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEC_ENC_FUNC_0112
 * @tc.name      : H265 HDR10 hwdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0112, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->DEFAULT_WIDTH = 3840;
        sample->DEFAULT_HEIGHT = 2160;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/hdr10.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEC_ENC_FUNC_0113
 * @tc.name      : H265 HDRHlg hwdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0113, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->DEFAULT_WIDTH = 1920;
        sample->DEFAULT_HEIGHT = 1080;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/hlgHdrVivid_1080p.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEC_ENC_FUNC_0114
 * @tc.name      : H265 dolby hwdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0114, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->DEFAULT_WIDTH = 1920;
        sample->DEFAULT_HEIGHT = 1080;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/dolby.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEC_ENC_FUNC_0120
 * @tc.name      : H266 hwdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0120, TestSize.Level1)
{
    if (g_codecName266.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1280;
        sample->DEFAULT_HEIGHT = 720;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/1280_720_8.vvc";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecName266, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0130
 * @tc.name      : Mpeg2 swdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0130, TestSize.Level1)
{
    if (g_codecNameMpeg2.find("MPEG2") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 352;
        sample->DEFAULT_HEIGHT = 288;
        sample->typeDec = 3;
        sample->INP_DIR = "/data/test/media/simple@low_level_352_288_30_rotation_90.m2v";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameMpeg2, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0140
 * @tc.name      : Mpeg4 swdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0140, TestSize.Level1)
{
    if (g_codecNameMpeg4.find("MPEG4") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1280;
        sample->DEFAULT_HEIGHT = 720;
        sample->typeDec = 4;
        sample->INP_DIR = "/data/test/media/mpeg4_1280x720_IPB.m4v";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameMpeg4, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEC_ENC_FUNC_0150
 * @tc.name      : H264 swdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0150, TestSize.Level1)
{
    if (g_codecNameSw264.find("AVC") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1104;
        sample->DEFAULT_HEIGHT = 622;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/1104x622.h264";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameSw264, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEC_ENC_FUNC_0160
 * @tc.name      : H265 swdecode and H265 encode
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEC_ENC_FUNC_0160, TestSize.Level1)
{
    if (g_codecNameSw265.find("HEVC") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1920;
        sample->DEFAULT_HEIGHT = 1080;
        sample->typeDec = 2;
        sample->INP_DIR = "/data/test/media/1920x1080.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameSw265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}


/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0010
 * @tc.name      : Demuxer and H263 swdecode and H264 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0010, TestSize.Level1)
{
    if (g_codecName263.find("H263") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        const char *file = "/data/test/media/profile0_level30_352x288.avi";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName263, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0020
 * @tc.name      : Demuxer and H264 hwdecode and H264 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0020, TestSize.Level1)
{
    if (g_codecName264.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        const char *file = "/data/test/media/AVI_H264_main@level5_1920_1080_MP2_1.avi";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName264, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0030
 * @tc.name      : Demuxer and H265 hwdecode and H264 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0030, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        const char *file = "/data/test/media/h265_aac_1mvex_fmp4.mp4";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName265, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0050
 * @tc.name      : Demuxer and Mpeg2 swdecode and H264 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0050, TestSize.Level1)
{
    if (g_codecNameMpeg2.find("MPEG2") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        const char *file = "/data/test/media/AVI_MPEG2_main@mian_640_480_MP2_1.avi";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecNameMpeg2, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0060
 * @tc.name      : Demuxer and Mpeg4 swdecode and H264 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0060, TestSize.Level1)
{
    if (g_codecNameMpeg4.find("MPEG4") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        const char *file = "/data/test/media/AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecNameMpeg4, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0070
 * @tc.name      : Demuxer and H264 swdecode and H264 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0070, TestSize.Level1)
{
    if (g_codecNameSw264.find("AVC") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        const char *file = "/data/test/media/AVI_H264_main@level5_1920_1080_MP2_1.avi";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecNameSw264, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0080
 * @tc.name      : Demuxer and H265 swdecode and H264 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0080, TestSize.Level1)
{
    if (g_codecNameSw265.find("HEVC") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        const char *file = "/data/test/media/h265_aac_1mvex_fmp4.mp4";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecNameSw265, g_codecNameEnc264));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0090
 * @tc.name      : Demuxer and H263 swdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0090, TestSize.Level1)
{
    if (g_codecName263.find("H263") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        const char *file = "/data/test/media/profile0_level30_352x288.avi";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName263, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0100
 * @tc.name      : Demuxer and H264 hwdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0100, TestSize.Level1)
{
    if (g_codecName264.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        const char *file = "/data/test/media/AVI_H264_main@level5_1920_1080_MP2_1.avi";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName264, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0110
 * @tc.name      : Demuxer and H265 hwdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0110, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        const char *file = "/data/test/media/h265_aac_1mvex_fmp4.mp4";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0111
 * @tc.name      : Demuxer and H265 HDRVivid hwdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0111, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        const char *file = "/data/test/media/audiovivid_hdrvivid_1s_fmp4.mp4";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0112
 * @tc.name      : Demuxer and H265 HDR10 hwdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0112, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        const char *file = "/data/test/media/demuxer_parser_hdr_1_hevc.mp4";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0113
 * @tc.name      : Demuxer and H265 HDRHlg hwdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0113, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        const char *file = "/data/test/media/demuxer_parser_hdr_vivid.mp4";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0114
 * @tc.name      : Demuxer and H265 dolby hwdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0114, TestSize.Level1)
{
    if (g_codecName265.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        const char *file = "/data/test/media/dolby.MOV";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0120
 * @tc.name      : Demuxer and H266 hwdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0120, TestSize.Level1)
{
    if (g_codecName266.find("hisi") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        const char *file = "/data/test/media/vvc_1280_720_8.mp4";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecName266, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0130
 * @tc.name      : Demuxer and Mpeg2 swdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0130, TestSize.Level1)
{
    if (g_codecNameMpeg2.find("MPEG2") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        const char *file = "/data/test/media/AVI_MPEG2_main@mian_640_480_MP2_1.avi";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecNameMpeg2, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0140
 * @tc.name      : Demuxer and Mpeg4 swdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0140, TestSize.Level1)
{
    if (g_codecNameMpeg4.find("MPEG4") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        const char *file = "/data/test/media/AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecNameMpeg4, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0150
 * @tc.name      : Demuxer and H264 swdecode and H265 encode and Muxer and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0150, TestSize.Level1)
{
    if (g_codecNameSw264.find("AVC") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        const char *file = "/data/test/media/AVI_H264_main@level5_1920_1080_MP2_1.avi";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecNameSw264, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : DEMUXER_DEC_ENC_MUXER_FUNC_0160
 * @tc.name      : Demuxer and H265 swdecode and H265 encode and Muxer
 * @tc.desc      : function test
 */
HWTEST_F(HDRFuncNdkTest, DEMUXER_DEC_ENC_MUXER_FUNC_0160, TestSize.Level1)
{
    if (g_codecNameSw265.find("HEVC") != string::npos) {
        shared_ptr<HDRCodecNdkSample> sample = make_shared<HDRCodecNdkSample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        const char *file = "/data/test/media/h265_aac_1mvex_fmp4.mp4";
        sample->DEMUXER_FLAG = true;
        sample->MIME_TYPE = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        ASSERT_EQ(AV_ERR_OK, sample->CreateDemuxerVideocoder(file, g_codecNameSw265, g_codecNameEnc265));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartDemuxer());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}
} // namespace