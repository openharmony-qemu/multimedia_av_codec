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
#include <iostream>
#include <cstdio>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>

#include "meta/format.h"
#include "gtest/gtest.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videoenc_api11_sample.h"
#include "videoenc_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "avcodec_info.h"
#include "avcodec_list.h"
#include "avcodec_common.h"
#include "videotranscode_api11_sample.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace {
OH_AVCapability *cap = nullptr;
OH_AVCapability *cap_hevc = nullptr;
OH_AVCapability *cap_dec = nullptr;
OH_AVCapability *cap_hevc_dec = nullptr;
constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecName[CODEC_NAME_SIZE] = {};
char g_codecNameHEVC[CODEC_NAME_SIZE] = {};
char g_codecNameDec[CODEC_NAME_SIZE] = {};
char g_codecNameDecHEVC[CODEC_NAME_SIZE] = {};

typedef struct ConfigParameter {
    bool surfSecondaryInput = false;
    bool surfPrimaryInput = true;
    uint32_t width = 1280;
    uint32_t height = 720;
    OH_AVPixelFormat pixFmt = AV_PIXEL_FORMAT_NV12;
    bool dualEos = false;
    bool configEnable = false;
    bool setParameterEnable = false;
    bool enableCrop = false;
    bool enableDFrame = false;
    bool enableDownsampling = false;
    int32_t cropLeft = 0;
    int32_t cropRight = 0;
    int32_t cropTop = 0;
    int32_t cropBottom = 0;
    int32_t downsamplingWidth = 0;
    int32_t downsamplingHeight = 0;
    double dropFrameRate = 0.00;
    int32_t cropNum = 4;
    int32_t downsamplingNum = 2;
    bool dropFramePtsSame = false;
}ConfigParameter;

fileInfo file_640_480_rgba{"/data/test/media/640_480.rgba", NATIVEBUFFER_PIXEL_FMT_RGBA_8888, 640, 480 };
fileInfo file_1280_720_nv{"/data/test/media/1280_720_nv.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 1280, 720 };
fileInfo file_1280_720_nv12{"/data/test/media/1280_720_nv12.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 1280, 720 };
fileInfo file_1920_1080_nv21{"/data/test/media/1920_1080_nv21.yuv", NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, 1920, 1080 };
fileInfo file_1280_536_nv21{"/data/test/media/1280_536_nv21.yuv", NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, 1280, 536 };
fileInfo file_3840_2160_nv12{"/data/test/media/3840_2160_nv12.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 3840, 2160 };
} // namespace
namespace OHOS {
namespace Media {
class HwEncPreProcessNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();
};
} // namespace Media
} // namespace OHOS

void HwEncPreProcessNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    const char *tmpCodecNameHevc = OH_AVCapability_GetName(cap_hevc);
    if (memcpy_s(g_codecNameHEVC, sizeof(g_codecNameHEVC), tmpCodecNameHevc, strlen(tmpCodecNameHevc)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname_hevc: " << g_codecNameHEVC << endl;
    cap_dec = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    const char *tmpCodecNameDec = OH_AVCapability_GetName(cap_dec);
    if (memcpy_s(g_codecNameDec, sizeof(g_codecNameDec), tmpCodecNameDec, strlen(tmpCodecNameDec)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname_dec: " << g_codecNameDec << endl;
    cap_hevc_dec = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    const char *tmpCodecNameDecHevc = OH_AVCapability_GetName(cap_hevc_dec);
    if (memcpy_s(g_codecNameDecHEVC, sizeof(g_codecNameDecHEVC), tmpCodecNameDecHevc, strlen(tmpCodecNameDecHevc)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname_hevc_dec: " << g_codecNameDecHEVC << endl;
}
void HwEncPreProcessNdkTest::TearDownTestCase() {}
void HwEncPreProcessNdkTest::SetUp() {}
void HwEncPreProcessNdkTest::TearDown()
{
}
namespace {
void SetParameter(std::shared_ptr<VEncAPI11Sample>& encSample, ConfigParameter &configParameter)
{
    encSample->DEFAULT_WIDTH = configParameter.width;
    encSample->DEFAULT_HEIGHT = configParameter.height;
    encSample->preProcessParameter.setParameterEnable = configParameter.setParameterEnable;
    encSample->preProcessParameter.configEnable = configParameter.configEnable;
    encSample->preProcessParameter.enableDownsampling = configParameter.enableDownsampling;
    encSample->preProcessParameter.downsamplingWidth = configParameter.downsamplingWidth;
    encSample->preProcessParameter.downsamplingHeight = configParameter.downsamplingHeight;
    encSample->preProcessParameter.enableCrop = configParameter.enableCrop;
    encSample->preProcessParameter.cropLeft = configParameter.cropLeft;
    encSample->preProcessParameter.cropRight = configParameter.cropRight;
    encSample->preProcessParameter.cropTop = configParameter.cropTop;
    encSample->preProcessParameter.cropBottom = configParameter.cropBottom;
    encSample->preProcessParameter.enableDFrame = configParameter.enableDFrame;
    encSample->preProcessParameter.dropFrameRate = configParameter.dropFrameRate;
    encSample->preProcessParameter.cropNum = configParameter.cropNum;
    encSample->preProcessParameter.downsamplingNum = configParameter.downsamplingNum;
    encSample->preProcessParameter.dropFramePtsSame = configParameter.dropFramePtsSame;
}

int32_t SetSecondaryConfig(std::shared_ptr<VEncAPI11Sample>& encSample, ConfigParameter configParameter)
{
    encSample->SURF_INPUT = configParameter.surfSecondaryInput;
    encSample->dualEos = configParameter.dualEos;
    SetParameter(encSample, configParameter);
    int32_t ret = encSample->SwitchSecondaryEncoder();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = encSample->ConfigureVideoEncoder();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = encSample->SetDualCallback();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    return ret;
}

int32_t SetPrimaryConfig(std::shared_ptr<VEncAPI11Sample>& encSample, ConfigParameter configParameter)
{
    encSample->SURF_INPUT = configParameter.surfPrimaryInput;
    encSample->dualEos = configParameter.dualEos;
    SetParameter(encSample, configParameter);
    int32_t ret = encSample->SwitchPrimaryEncoder();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = encSample->ConfigureVideoEncoder();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = encSample->SetDualCallback();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    return ret;
}

int32_t CreateEncoder(std::shared_ptr<VEncAPI11Sample>& encSample, const char *codeMime, vector<fileInfo> fileName)
{
    encSample->runDualEncoder = true;
    encSample->readMultiFiles = true;
    for (size_t i = 0; i < fileName.size(); ++i) {
        encSample->fileInfos.push_back(fileName[i]);
    }
    if (codeMime == OH_AVCODEC_MIMETYPE_VIDEO_AVC) {
        encSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DUAL_OUTPUT_1.h264";
        encSample->OUT_DIR_SEC = "/data/test/media/VIDEO_ENCODE_DUAL_OUTPUT_2.h264";
    } else if (codeMime == OH_AVCODEC_MIMETYPE_VIDEO_HEVC) {
        encSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DUAL_OUTPUT_1.h265";
        encSample->OUT_DIR_SEC = "/data/test/media/VIDEO_ENCODE_DUAL_OUTPUT_2.h265";
    }
    int32_t ret = encSample->CreatePrimaryEncoder(codeMime);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = encSample->CreateSecondaryEncoder();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    return ret;
}

int32_t StartEncoder(std::shared_ptr<VEncAPI11Sample>& encSample, bool useSenondary = true, bool usePrimary = true)
{
    int32_t ret = AV_ERR_OK;
    if (useSenondary) {
        ret = encSample->StartSecondaryEncoder();
        if (ret != AV_ERR_OK) {
            return ret;
        }
    }
    if (usePrimary) {
        ret = encSample->StartVideoEncoder();
        if (ret != AV_ERR_OK) {
            return ret;
        }
    }
    encSample->WaitForEOS();
    if (encSample->errCount != AV_ERR_OK) {
        return encSample->errCount;
    }
    return ret;
}

int32_t ReConfigure(std::shared_ptr<VEncAPI11Sample>& encSample, ConfigParameter configParameter)
{
    int32_t ret = encSample->ResetSecondary();
    if (ret != AV_ERR_OK) {
        return AV_ERR_UNKNOWN;
    }
    encSample->SURF_INPUT = configParameter.surfSecondaryInput;
    SetParameter(encSample, configParameter);
    ret = encSample->ConfigureVideoEncoder();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    return ret;
}

bool SupportPreprocess(OH_AVCapability *capability)
{
    if (!OH_AVCapability_IsFeatureSupported(capability, VIDEO_ENCODER_PREPROC_DOWNSAMPLING)) {
        return false;
    }
    if (!OH_AVCapability_IsFeatureSupported(capability, VIDEO_ENCODER_PREPROC_CROP)) {
        return false;
    }
    return true;
}
}
namespace {
/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0010
 * @tc.name      : 编码h264,主副编码都开,config配置scaling
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0010, TestSize.Level0)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 640, .downsamplingHeight = 360}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 380, .downsamplingHeight = 200}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0020
 * @tc.name      : 编码h265,主副编码都开,config配置crop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0020, TestSize.Level1)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableCrop = true, .cropLeft = 100,
            .cropRight = 1199, .cropTop = 100, .cropBottom = 699}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .enableCrop = true, .cropLeft = 50,
            .cropRight = 999, .cropTop = 0, .cropBottom = 499}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0030
 * @tc.name      : 编码h264,主副编码都开,config配置DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0030, TestSize.Level0)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv12}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableDFrame = true,
            .dropFrameRate = 25.57}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .enableDFrame = true,
            .dropFrameRate = 15.89}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0040
 * @tc.name      : 编码h265,主副编码都开,config配置scaling+crop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0040, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv12}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchSecondaryEncoder());
        ASSERT_EQ(AV_ERR_INVALID_VAL, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 640, .downsamplingHeight = 360, .enableCrop = true, .cropLeft = 100, .cropRight = 1099,
            .cropTop = 100, .cropBottom = 599}));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0050
 * @tc.name      : 编码h264,主副编码都开,config配置scaling+DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0050, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 640, .downsamplingHeight = 360, .enableDFrame = true, .dropFrameRate = 25.57}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 380, .downsamplingHeight = 180, .enableDFrame = true, .dropFrameRate = 15.74}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0060
 * @tc.name      : 硬编h265,主副编码都开,config配置crop+DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0060, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 999, .cropTop = 50, .cropBottom = 699, .enableDFrame = true,
            .dropFrameRate = 18.00}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 999, .cropTop = 50, .cropBottom = 699, .enableDFrame = true,
            .dropFrameRate = 18.00}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0070
 * @tc.name      : 编码h264,主副编码都开,config未配置scaling,setParameter再配置scaling
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0070, TestSize.Level0)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.setParameterEnable = true,
            .enableDownsampling = true, .downsamplingWidth = 640, .downsamplingHeight = 360}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.setParameterEnable = true,
            .enableDownsampling = true, .downsamplingWidth = 640, .downsamplingHeight = 360}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0080
 * @tc.name      : 硬编h265,主副编码都开,config未配置crop,setParameter再配置crop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0080, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->runDualEncoder = true;
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.setParameterEnable = true, .enableCrop = true,
            .cropLeft = 50, .cropRight = 899, .cropTop = 20, .cropBottom = 599}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.setParameterEnable = true, .enableCrop = true,
            .cropLeft = 50, .cropRight = 899, .cropTop = 20, .cropBottom = 599}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0090
 * @tc.name      : 编码h264,主副编码都开,setParameter配置drop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0090, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.setParameterEnable = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.setParameterEnable = true, .enableDFrame = true,
            .dropFrameRate = 15.89}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0100
 * @tc.name      : 编码h264,主副编码都开,setParameter配置scaling+crop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0100, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.setParameterEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 640, .downsamplingHeight = 360, .enableCrop = true, .cropLeft = 50, .cropTop = 50,
            .cropRight = 1099, .cropBottom = 699}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.setParameterEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 640, .downsamplingHeight = 360, .enableCrop = true, .cropLeft = 50, .cropTop = 50,
            .cropRight = 1099, .cropBottom = 699}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0110
 * @tc.name      :  硬编h265,主副编码都开,config未配置，setParameter配置scaling+DROP_TOFRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0110, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.setParameterEnable = true, .enableDownsampling = true,
            .enableDFrame = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.setParameterEnable = true, .enableDownsampling = true,
            .enableDFrame = true, .downsamplingWidth = 380, .downsamplingHeight = 180, .dropFrameRate = 15.74}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0120
 * @tc.name      : 编码h264,主副编码都开,config未配置，setParameter配置crop+DROP_TOFRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0120, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.setParameterEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 1099, .cropTop = 100, .cropBottom = 699, .enableDFrame = true,
            .dropFrameRate = 18.00}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.setParameterEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 1099, .cropTop = 100, .cropBottom = 699, .enableDFrame = true,
            .dropFrameRate = 18.00}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0130
 * @tc.name      : 硬编h265,主副编码都开,config+setParameter均配置scaling+DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0130, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .setParameterEnable = true,
            .enableDownsampling = true, .downsamplingWidth = 640, .downsamplingHeight = 360, .enableDFrame = true,
            .dropFrameRate = 25.57}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .setParameterEnable = true,
            .enableDownsampling = true, .downsamplingWidth = 380, .downsamplingHeight = 180, .enableDFrame = true,
            .dropFrameRate = 15.74}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0140
 * @tc.name      : 编码h264,主副编码都开,config+setParameter均配置crop+DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0140, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .setParameterEnable = true,
            .enableCrop = true, .cropLeft = 100, .cropRight = 1099, .cropTop = 50, .cropBottom = 699,
            .enableDFrame = true, .dropFrameRate = 18.00}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .setParameterEnable = true,
            .enableCrop = true, .cropLeft = 100, .cropRight = 1099, .cropTop = 50, .cropBottom = 699,
            .enableDFrame = true, .dropFrameRate = 18.00}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0150
 * @tc.name      : 硬编h265,主副编码都开,stop/flush
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0150, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.setParameterEnable = true, .enableDFrame = true,
            .dropFrameRate = 25.57}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.setParameterEnable = true, .enableDFrame = true,
            .dropFrameRate = 25.57}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->FlushVideoEncode());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchPrimaryEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0160
 * @tc.name      : 编码h264,主副编码都开,reset/setParameter/config
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0160, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        for (int i=0; i < 2; i++) {
            ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.setParameterEnable = true, .enableDFrame = true,
                .dropFrameRate = 25.57}));
            ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.setParameterEnable = true, .enableDFrame = true,
                .dropFrameRate = 25.57}));
            ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
            ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
            ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchSecondaryEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0170
 * @tc.name      : 硬编h265，普通编码器，stop/flush
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0170, TestSize.Level2 )
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0160.h264";
        vEncSample->preProcessParameter.setParameterEnable = true;
        vEncSample->preProcessParameter.enableDFrame = true;
        vEncSample->preProcessParameter.dropFrameRate = 25.57;
        vEncSample->SURF_INPUT = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateSurface());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->FlushVideoEncode());
        vEncSample->isSurface = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0180
 * @tc.name      : 编码h264，普通编码器，reset/setParameter/config
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0180, TestSize.Level2)
{
    if (cap_hevc != nullptr && SupportPreprocess(cap_hevc)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0180.h265";
        vEncSample->preProcessParameter.setParameterEnable = true;
        vEncSample->preProcessParameter.enableDFrame = true;
        vEncSample->preProcessParameter.dropFrameRate = 25.57;
        vEncSample->SURF_INPUT = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
        for (int i=0; i < 2; i++) {
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
            ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
            vEncSample->isSurface = true;
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0190
 * @tc.name      : 硬编h265，普通编码器,config+setParameter均配置scaling+DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0190, TestSize.Level1)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0190.h264";
        vEncSample->preProcessParameter.configEnable = true;
        vEncSample->preProcessParameter.setParameterEnable = true;
        vEncSample->preProcessParameter.enableDownsampling = true;
        vEncSample->preProcessParameter.downsamplingWidth = 640;
        vEncSample->preProcessParameter.downsamplingHeight = 360;
        vEncSample->preProcessParameter.enableDFrame = true;
        vEncSample->preProcessParameter.dropFrameRate = 25.57;
        vEncSample->SURF_INPUT = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateSurface());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->FlushVideoEncode());
        vEncSample->isSurface = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0200
 * @tc.name      : 编码h264，普通编码器,config+setParameter均配置crop+DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0200, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_unique<VEncAPI11Sample>();
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0200.h264";
        vEncSample->preProcessParameter.configEnable = true;
        vEncSample->preProcessParameter.setParameterEnable = true;
        vEncSample->preProcessParameter.enableCrop = true;
        vEncSample->preProcessParameter.cropLeft = 100;
        vEncSample->preProcessParameter.cropRight = 1099;
        vEncSample->preProcessParameter.cropTop = 50;
        vEncSample->preProcessParameter.cropBottom = 699;
        vEncSample->preProcessParameter.enableDFrame = true;
        vEncSample->preProcessParameter.dropFrameRate = 18.00;
        vEncSample->SURF_INPUT = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateSurface());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->FlushVideoEncode());
        vEncSample->isSurface = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0210
 * @tc.name      : 硬编h265，只开Primary编码器,config+setParameter均配置scaling+DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0210, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->runDualEncoder = true;
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0210_1.h264";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreatePrimaryEncoder(OH_AVCODEC_MIMETYPE_VIDEO_AVC));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .setParameterEnable = true,
            .enableDownsampling = true, .downsamplingWidth = 640, .downsamplingHeight = 360, .enableDFrame = true,
            .dropFrameRate = 25.57, .dualEos = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample, false, true));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0220
 * @tc.name      : 编码h264，只开Primary编码器,config+setParameter均配置crop+DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0220, TestSize.Level0)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->runDualEncoder = true;
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0220_1.h265";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreatePrimaryEncoder(OH_AVCODEC_MIMETYPE_VIDEO_HEVC));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .setParameterEnable = true,
            .enableCrop = true, .cropLeft = 100, .cropRight = 1099, .cropTop = 50, .cropBottom = 699,
            .enableDFrame = true, .dropFrameRate = 18.00, .dualEos = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample, false, true));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0230
 * @tc.name      : 硬编h265，只开Secondary编码器,config+setParameter均配置scaling+DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0230, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .setParameterEnable = true,
            .enableDownsampling = true, .downsamplingWidth = 640, .downsamplingHeight = 360, .enableDFrame = true,
            .dropFrameRate = 25.57, .dualEos = true, .surfSecondaryInput = true}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchPrimaryEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateSurface());
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchSecondaryEncoder());
        vEncSample->isSurface = true;
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample, false, true));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0240
 * @tc.name      : 编码h264，只开Secondary编码器,config+setParameter均配置crop+DROP_TO_FRAME_RATE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0240, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .setParameterEnable = true,
            .enableCrop = true, .cropLeft = 100, .cropRight = 1099, .cropTop = 50, .cropBottom = 699,
            .enableDFrame = true, .dropFrameRate = 18.00, .dualEos = true, .surfSecondaryInput = true}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchPrimaryEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateSurface());
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchSecondaryEncoder());
        vEncSample->isSurface = true;
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample, false, true));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0250
 * @tc.name      : 硬编h265，只开Primary编码器,stop/flush
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0250, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->runDualEncoder = true;
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0250_1.h264";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreatePrimaryEncoder(OH_AVCODEC_MIMETYPE_VIDEO_AVC));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .enableDFrame = true,
            .dropFrameRate = 25.57, .dualEos = true}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateSurface());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Start());
        ASSERT_EQ(AV_ERR_OK, vEncSample->FlushVideoEncode());
        vEncSample->isSurface = true;
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample, false, true));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0260
 * @tc.name      : 编码h264，只开Primary编码器,reset/setParameter/config
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0260, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->runDualEncoder = true;
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0260_1.h265";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreatePrimaryEncoder(OH_AVCODEC_MIMETYPE_VIDEO_HEVC));
        for (int i=0; i < 2; i++)  {
            ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .setParameterEnable = true,
                .enableDFrame = true, .dropFrameRate = 25.57, .dualEos = true}));
            ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample, false, true));
            ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
        }
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0270
 * @tc.name      : 硬编h265,config crop 4个参数都配置为0
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0270, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableCrop = true, .cropLeft = 0,
                .cropRight = 0, .cropTop = 0, .cropBottom = 0}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .enableCrop = true, .cropLeft = 0,
                .cropRight = 0, .cropTop = 0, .cropBottom = 0}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0280
 * @tc.name      : 硬编h265,config crop 只配置1/2/3个参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0280, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 699, .cropTop = 50, .cropBottom = 499, .cropNum = 1}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 699, .cropTop = 50, .cropBottom = 499, .cropNum = 2}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 699, .cropTop = 50, .cropBottom = 499, .cropNum = 3}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->ResetSecondary());
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0290
 * @tc.name      : 硬编h265,config配置crop的值left<0/left>right/right>max/left>max/top<0/top>bottom/bottom>max/top>max
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0290, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = -1, .cropRight = 999, .cropTop = 50, .cropBottom = 499}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 1100, .cropRight = 999, .cropTop = 50, .cropBottom = 499}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 1499, .cropTop = 50, .cropBottom = 499}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 1500, .cropRight = 1101, .cropTop = 50, .cropBottom = 499}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 1101, .cropTop = -1, .cropBottom = 499}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 1101, .cropTop = 600, .cropBottom = 499}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 1101, .cropTop = 50, .cropBottom = 899}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableCrop = true,
            .cropLeft = 100, .cropRight = 1101, .cropTop = 900, .cropBottom = 499}));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0300
 * @tc.name      : 硬编h265,config downsampling 2个参数都配置为0
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0300, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 0, .downsamplingHeight = 0}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 0, .downsamplingHeight = 0}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0310
 * @tc.name      : 硬编h265,config downsampling 只配置1个参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0310, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 640, .downsamplingHeight = 480, .downsamplingNum = 1}));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0320
 * @tc.name      : 硬编h265,config配置downsampling的值width<0/width>max/width=0/height<0/height>max/height=0
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0320, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = -1, .downsamplingHeight = 480}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 1380, .downsamplingHeight = 480}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 0, .downsamplingHeight = 480}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 640, .downsamplingHeight = -1}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 640, .downsamplingHeight = 820}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 640, .downsamplingHeight = 0}));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0330
 * @tc.name      : 硬编h265,config DROP_TO_FRAME_RAT配置为0
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0330, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.setParameterEnable = true, .enableDFrame = true,
            .dropFrameRate = 0}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.setParameterEnable = true, .enableDFrame = true,
            .dropFrameRate = 0}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0340
 * @tc.name      : 硬编h265,config配置DROP_TO_FRAME_RAT>=frame_rate/DROP_TO_FRAME_RAT<0
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0340, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableDFrame = true,
            .dropFrameRate = -1}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, ReConfigure(vEncSample, {.configEnable = true, .enableDFrame = true,
            .dropFrameRate = 100}));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0350
 * @tc.name      : 硬编h265,config DROP_TO_FRAME_RAT pts异常
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0350, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.setParameterEnable = true, .enableDFrame = true,
            .dropFramePtsSame = true, .dropFrameRate = 15}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.setParameterEnable = true, .enableDFrame = true,
            .dropFramePtsSame = true, .dropFrameRate = 15}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}


/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0360
 * @tc.name      : 硬编h265,config downsampling增大
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0360, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_INVALID_VAL, SetSecondaryConfig(vEncSample, {.configEnable = true, .enableDownsampling = true,
            .downsamplingWidth = 1380, .downsamplingHeight = 820}));
    }
}


/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0370
 * @tc.name      : 主副编码都开-变分辨率/格式-前处理
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0370, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_640_480_rgba,
            file_1280_536_nv21, file_1280_720_nv, file_1920_1080_nv21, file_3840_2160_nv12}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.width = 3840, .height = 2160, .configEnable = true,
            .setParameterEnable = true, .enableCrop = true, .cropLeft = 100, .cropRight = 1109, .cropTop = 100,
            .cropBottom = 699, .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.width = 3840, .height = 2160, .configEnable = true,
            .setParameterEnable = true, .enableCrop = true, .cropLeft = 100, .cropRight = 1109, .cropTop = 100,
            .cropBottom = 699, .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0380
 * @tc.name      : 编码h264转码(解码+编码surface) + 前处理
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0380, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto sample = make_unique<VideoTransCodeApi11Sample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1104;
        sample->DEFAULT_HEIGHT = 622;
        sample->INP_DIR = "/data/test/media/1104x622.h264";
        sample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0380_1.h264";
        sample->OUT_DIR_SEC = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0380_2.h264";
        sample->enableDownsampling = true;
        sample->downsamplingWidth = 640;
        sample->downsamplingHeight = 360;
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameDec, OH_AVCODEC_MIMETYPE_VIDEO_AVC));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        ASSERT_EQ(300, sample->frameCountDec);
        ASSERT_EQ(300, sample->frameCountEnc);
        ASSERT_EQ(300, sample->frameCountEncSecondary);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0390
 * @tc.name      : 硬编h265转码(解码+编码surface) + 前处理
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0390, TestSize.Level2)
{
    if (cap_hevc != nullptr && SupportPreprocess(cap_hevc)) {
        shared_ptr<VideoTransCodeApi11Sample> sample = make_shared<VideoTransCodeApi11Sample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1920;
        sample->DEFAULT_HEIGHT = 1080;
        sample->INP_DIR = "/data/test/media/1920x1080.h265";
        sample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0390_1.h265";
        sample->OUT_DIR_SEC = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0390_2.h265";
        sample->enableDownsampling = true;
        sample->downsamplingWidth = 640;
        sample->downsamplingHeight = 360;
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameDecHEVC, OH_AVCODEC_MIMETYPE_VIDEO_HEVC));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        ASSERT_EQ(300, sample->frameCountDec);
        ASSERT_EQ(300, sample->frameCountEnc);
        ASSERT_EQ(300, sample->frameCountEncSecondary);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0400
 * @tc.name      : 硬编h265 hdr转码(解码+编码surface) + 前处理
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0400, TestSize.Level2)
{
    if (cap_hevc != nullptr && SupportPreprocess(cap_hevc)) {
        shared_ptr<VideoTransCodeApi11Sample> sample = make_shared<VideoTransCodeApi11Sample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->DEFAULT_WIDTH = 3840;
        sample->DEFAULT_HEIGHT = 2160;
        sample->enableDownsampling = true;
        sample->downsamplingWidth = 640;
        sample->downsamplingHeight = 360;
        sample->INP_DIR = "/data/test/media/hdr10.h265";
        sample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0400_1.h265";
        sample->OUT_DIR_SEC = "/data/test/media/VIDEO_ENCODE_PRE_PROCESS_0400_2.h265";
        ASSERT_EQ(AV_ERR_OK, sample->CreateVideocoder(g_codecNameDecHEVC, OH_AVCODEC_MIMETYPE_VIDEO_HEVC));
        ASSERT_EQ(AV_ERR_OK, sample->Configure());
        ASSERT_EQ(AV_ERR_OK, sample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OK, sample->Start());
        sample->WaitForEos();
        ASSERT_EQ(0, sample->errorCount);
        ASSERT_EQ(600, sample->frameCountDec);
        ASSERT_EQ(600, sample->frameCountEnc);
        ASSERT_EQ(600, sample->frameCountEncSecondary);
        cout << "frameCountDec--" << sample->frameCountDec << endl;
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0410
 * @tc.name      : 下采样的宽高是奇数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0410, TestSize.Level2)
{
    if (cap != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->runDualEncoder = true;
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreatePrimaryEncoder(OH_AVCODEC_MIMETYPE_VIDEO_AVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateSecondaryEncoder());
        vEncSample->SURF_INPUT = false;
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchSecondaryEncoder());
        vEncSample->preProcessParameter.configEnable = true;
        vEncSample->preProcessParameter.enableDownsampling = true;
        vEncSample->preProcessParameter.downsamplingWidth = 641;
        vEncSample->preProcessParameter.downsamplingHeight = 360;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ResetSecondary());
        vEncSample->preProcessParameter.downsamplingWidth = 640;
        vEncSample->preProcessParameter.downsamplingHeight = 361;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ResetSecondary());
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_PRE_PROCESS_0420
 * @tc.name      : crop起始坐标宽高是奇数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncPreProcessNdkTest, VIDEO_ENCODE_PRE_PROCESS_0420, TestSize.Level2)
{
    if (cap_hevc != nullptr && SupportPreprocess(cap)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->runDualEncoder = true;
        vEncSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreatePrimaryEncoder(OH_AVCODEC_MIMETYPE_VIDEO_HEVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateSecondaryEncoder());
        vEncSample->SURF_INPUT = false;
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchSecondaryEncoder());
        vEncSample->preProcessParameter.configEnable = true;
        vEncSample->preProcessParameter.enableCrop = true;
        vEncSample->preProcessParameter.cropLeft = 100;
        vEncSample->preProcessParameter.cropRight = 1200;
        vEncSample->preProcessParameter.cropTop = 100;
        vEncSample->preProcessParameter.cropBottom = 699;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ResetSecondary());
        vEncSample->preProcessParameter.cropLeft = 100;
        vEncSample->preProcessParameter.cropRight = 1199;
        vEncSample->preProcessParameter.cropTop = 100;
        vEncSample->preProcessParameter.cropBottom = 700;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ResetSecondary());
    }
}
} // namespace