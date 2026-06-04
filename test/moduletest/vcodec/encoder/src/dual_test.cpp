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
    uint32_t bitRateMode = CBR;
    uint32_t bitRate = 5000000;
    bool surfSecondaryInput = false;
    bool surfPrimaryInput = true;
    uint32_t width = 1280;
    uint32_t height = 720;
    OH_AVPixelFormat pixFmt = AV_PIXEL_FORMAT_NV12;
    OH_HEVCProfile hevcProfile = HEVC_PROFILE_MAIN;
    uint32_t quality = 30;
    int32_t sqrFactor = 32;
    int64_t maxBiterate = 6000000;
    int32_t enbleBFrame = 0;
    bool repeat = false;
    bool maxCount = false;
    int32_t frameAfter = 73;
    int32_t maxCountFrame = 1;
    bool roi = false;
    const char *roiInfo = "660,1080-720,1280=-51";
    bool enableTemporal = false;
    bool temporalConfig = false;
    int32_t temporalGopSize = 4;
    uint32_t frameInterval = 1000;
    uint32_t rangeFlag = 0;
    bool enableColorSpaceParam = false;
    uint32_t colorPrimaries = COLOR_PRIMARY_BT709;
    uint32_t transferCharacteristics = TRANSFER_CHARACTERISTIC_BT709;
    uint32_t matrixCoefficients = MATRIX_COEFFICIENT_BT709;
    bool dualEos = false;
}ConfigParameter;

fileInfo file_640_480_rgba{"/data/test/media/640_480.rgba", NATIVEBUFFER_PIXEL_FMT_RGBA_8888, 640, 480 };
fileInfo file_1280_720_nv{"/data/test/media/1280_720_nv.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 1280, 720 };
fileInfo file_1920_1080_nv21{"/data/test/media/1920_1080_nv21.yuv", NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, 1920, 1080 };
fileInfo file_1280_536_nv21{"/data/test/media/1280_536_nv21.yuv", NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, 1280, 536 };
fileInfo file_3840_2160_nv12{"/data/test/media/3840_2160_nv12.yuv", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, 3840, 2160 };
fileInfo file_1280_720_nv12_10bit{"/data/test/media/1280_720_nv12_10bit.yuv",
    NATIVEBUFFER_PIXEL_FMT_YCBCR_P010, 1280, 720 };
fileInfo file_1920_1088_nv12_10bit{"/data/test/media/1920_1088_nv12_10bit.yuv",
    NATIVEBUFFER_PIXEL_FMT_YCBCR_P010, 1920, 1088 };
fileInfo file_176_144_rgba1010102{"/data/test/media/176_144_rgba1010102.yuv",
    NATIVEBUFFER_PIXEL_FMT_RGBA_1010102, 176, 144 };
} // namespace
namespace OHOS {
namespace Media {
class HwEncDualNdkTest : public testing::Test {
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

void HwEncDualNdkTest::SetUpTestCase()
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
void HwEncDualNdkTest::TearDownTestCase() {}
void HwEncDualNdkTest::SetUp() {}
void HwEncDualNdkTest::TearDown()
{
}
namespace {
void SetParameter(std::shared_ptr<VEncAPI11Sample>& encSample, ConfigParameter &configParameter)
{
    encSample->DEFAULT_BITRATE_MODE = configParameter.bitRateMode;
    encSample->DEFAULT_BITRATE = configParameter.bitRate;
    encSample->DEFAULT_WIDTH = configParameter.width;
    encSample->DEFAULT_HEIGHT = configParameter.height;
    encSample->DEFAULT_PIX_FMT = configParameter.pixFmt;
    encSample->hevcProfile = configParameter.hevcProfile;
    encSample->DEFAULT_QUALITY = configParameter.quality;
    encSample->DEFAULT_SQR_FACTOR = configParameter.sqrFactor;
    encSample->DEFAULT_MAX_BITERATE = configParameter.maxBiterate;
    encSample->enbleBFrameMode = configParameter.enbleBFrame;
    encSample->enableRepeat = configParameter.repeat;
    encSample->setMaxCount = configParameter.maxCount;
    encSample->DEFAULT_FRAME_AFTER = configParameter.frameAfter;
    encSample->DEFAULT_MAX_COUNT = configParameter.maxCountFrame;
    encSample->enableDualRoi = configParameter.roi;
    encSample->roiInfo = configParameter.roiInfo;
    encSample->TEMPORAL_ENABLE = configParameter.enableTemporal;
    encSample->TEMPORAL_CONFIG = configParameter.temporalConfig;
}

int32_t SetSecondaryConfig(std::shared_ptr<VEncAPI11Sample>& encSample, ConfigParameter configParameter,
    bool callBack = false)
{
    encSample->SURF_INPUT = configParameter.surfSecondaryInput;
    SetParameter(encSample, configParameter);
    int32_t ret = encSample->SwitchSecondaryEncoder();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    if (!configParameter.enableTemporal) {
        ret = encSample->ConfigureVideoEncoder();
    } else {
        ret = encSample->ConfigureVideoEncoder_Temporal(configParameter.temporalGopSize);
    }
    if (ret != AV_ERR_OK) {
        return ret;
    }
    if (!callBack) {
        ret = encSample->SetDualCallback();
    } else {
        ret = encSample->SetCallback();
    }
    if (ret != AV_ERR_OK) {
        return ret;
    }
    return ret;
}

int32_t SetPrimaryConfig(shared_ptr<VEncAPI11Sample>& encSample, ConfigParameter configParameter, bool isMain10 = false)
{
    encSample->SURF_INPUT = configParameter.surfPrimaryInput;
    encSample->configMain10 = isMain10;
    encSample->DEFAULT_KEY_FRAME_INTERVAL = configParameter.frameInterval;
    encSample->DEFAULT_RANGE_FLAG = configParameter.rangeFlag;
    encSample->enableColorSpaceParams = configParameter.enableColorSpaceParam;
    encSample->DEFAULT_COLOR_PRIMARIES = configParameter.colorPrimaries;
    encSample->DEFAULT_TRANSFER_CHARACTERISTICS = configParameter.transferCharacteristics;
    encSample->DEFAULT_MATRIX_COEFFICIENTS = configParameter.matrixCoefficients;
    encSample->dualEos = configParameter.dualEos;
    SetParameter(encSample, configParameter);
    int32_t ret = encSample->SwitchPrimaryEncoder();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    if (!configParameter.enableTemporal) {
        ret = encSample->ConfigureVideoEncoder();
    } else {
        ret = encSample->ConfigureVideoEncoder_Temporal(configParameter.temporalGopSize);
    }
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
    for (size_t i = 0; i < fileName.size(); i++) {
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
}
namespace {
/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0010
 * @tc.name      : OH_VideoEncoder_CreatePrimaryWithPreproc接口参数mime为null
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0010, TestSize.Level2)
{
    const char *mime = nullptr;
    OH_AVCodec *vencCodec = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_CreatePrimaryWithPreproc(mime, &vencCodec));
    ASSERT_EQ(nullptr, vencCodec);
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0020
 * @tc.name      : OH_VideoEncoder_CreateSecondaryFromPrimary接口参数primary为null
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0020, TestSize.Level2)
{
    OH_AVCodec *primaryVencCodec = nullptr;
    OH_AVCodec *vencCodec = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_CreateSecondaryFromPrimary(primaryVencCodec, &vencCodec));
    ASSERT_EQ(nullptr, vencCodec);
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0030
 * @tc.name      : OH_VideoEncoder_CreateSecondaryFromPrimary接口参数primary为普通编码器
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0030, TestSize.Level2)
{
    if (cap != nullptr) {
        OH_AVCodec *primaryVencCodec = OH_VideoEncoder_CreateByName(g_codecName);
        ASSERT_NE(nullptr, primaryVencCodec);
        OH_AVCodec *vencCodec = nullptr;
        ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_CreateSecondaryFromPrimary(primaryVencCodec, &vencCodec));
        ASSERT_EQ(nullptr, vencCodec);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0040
 * @tc.name      : OH_VideoEncoder_CreateSecondaryFromPrimary重复调用
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0040, TestSize.Level2)
{
    if (cap != nullptr) {
        OH_AVCodec *primaryVencCodec = nullptr;
        OH_VideoEncoder_CreatePrimaryWithPreproc(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryVencCodec);
        ASSERT_NE(nullptr, primaryVencCodec);
        OH_AVCodec *secondaryVencCodec = nullptr;
        ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_CreateSecondaryFromPrimary(primaryVencCodec, &secondaryVencCodec));
        ASSERT_NE(nullptr, secondaryVencCodec);
        ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_CreateSecondaryFromPrimary(primaryVencCodec,
            &secondaryVencCodec));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0050
 * @tc.name      : 编码H264, Primary/Secondary编码器config配置为720P
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0050, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0060
 * @tc.name      : 硬编H265, Primary/Secondary编码器config配置为1080P
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0060, TestSize.Level0)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1080_nv21}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR, .width = 1920, .height = 1080,
            .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.width = 1920, .height = 1080,
            .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0070
 * @tc.name      : 编码h264, Primary/Secondary编码器config配置为4K
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0070, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_3840_2160_nv12}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR, .width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0080
 * @tc.name      : 硬编h265, config配置width/height的值为Primary>Secondary>输入的width/height值
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0080, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR, .width = 1920, .height = 1080}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.width = 3840, .height = 2160}));
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0090
 * @tc.name      : 编码h264, config配置width/height的值为Primary<Secondary<输入的width/height值, onerror报错
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0090, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_3840_2160_nv12}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR, .width = 1920, .height = 1080}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.width = 1280, .height = 720}));
        vEncSample->DEFAULT_WIDTH = 3840;
        vEncSample->DEFAULT_HEIGHT = 2160;
        ASSERT_LT(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0100
 * @tc.name      : 编码h264, Primary/Secondary编码器均配置为CBR, OH_MD_KEY_BITRATE一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0100, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_3840_2160_nv12}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0110
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CBR, OH_MD_KEY_BITRATE一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0110, TestSize.Level0)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0120
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CBR, OH_MD_KEY_BITRATE一致, OH_MD_KEY_PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0120, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0130
 * @tc.name      : 编码h264, Primary/Secondary编码器均配置为VBR, OH_MD_KEY_BITRATE一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0130, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1920_1080_nv21}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR, .width = 1920, .height = 1080,
            .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = VBR, .width = 1920, .height = 1080,
            .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0140
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为VBR, OH_MD_KEY_BITRATE一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0140, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0150
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为VBR, OH_MD_KEY_BITRATE一致, OH_MD_KEY_PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0150, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = VBR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0160
 * @tc.name      : 编码h264, Primary/Secondary编码器均配置为CQ, OH_MD_KEY_QUALITY一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0160, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1920_1080_nv21}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CQ, .width = 1920, .height = 1080,
            .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CQ, .width = 1920, .height = 1080,
            .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0170
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CQ, OH_MD_KEY_QUALITY一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0170, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CQ, .width = 1280, .height = 720}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CQ, .width = 1280, .height = 720}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0180
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CQ, OH_MD_KEY_QUALITY一致, OH_MD_KEY_PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0180, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CQ, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CQ, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0190
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为SQR, OH_MD_KEY_SQR_FACTOR/OH_MD_KEY_MAX_BITRATE一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0190, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = SQR, .width = 1280, .height = 720}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = SQR, .width = 1280, .height = 720}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0200
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为SQR, SQR_FACTOR/MAX_BITRATE一致, OH_MD_KEY_PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0200, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = SQR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = SQR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0210
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CBR_HIGH_QUALITY, OH_MD_KEY_BITRATE一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0210, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0220
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CBR_HIGH_QUALITY, OH_MD_KEY_BITRATE一致, PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0220, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .width = 1920,
            .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .width = 1920,
            .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0230
 * @tc.name      : 编码h264, Primary/Secondary编码器均配置为CBR, OH_MD_KEY_BITRATE不一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0230, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_3840_2160_nv12}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRate = 6000000, .width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRate = 5000000, .width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0240
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CBR, OH_MD_KEY_BITRATE不一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0240, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRate = 6000000}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0250
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CBR, OH_MD_KEY_BITRATE不一致, OH_MD_KEY_PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0250, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRate = 6000000, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRate = 5000000, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0260
 * @tc.name      : 编码h264, Primary/Secondary编码器均配置为CQ, OH_MD_KEY_QUALITY不一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0260, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1920_1080_nv21}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CQ, .quality = 25, .width = 1920,
            .height = 1080, .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CQ, .quality = 30, .width = 1920,
            .height = 1080, .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0270
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CQ, OH_MD_KEY_QUALITY不一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0270, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CQ, .quality = 30}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CQ, .quality = 25}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0280
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CQ, OH_MD_KEY_QUALITY不一致, OH_MD_KEY_PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0280, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CQ, .quality = 25, .width = 1920,
            .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CQ, .quality = 30, .width = 1920,
            .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0290
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为SQR, OH_MD_KEY_SQR_FACTOR/OH_MD_KEY_MAX_BITRATE不一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0290, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = SQR, .sqrFactor = 30,
            .maxBiterate = 4000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = SQR, .sqrFactor = 32,
            .maxBiterate = 6000000}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0300
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为SQR, SQR_FACTOR/MAX_BITRATE不一致, OH_MD_KEY_PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0300, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = SQR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10, .sqrFactor = 32, .maxBiterate = 6000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = SQR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10, .sqrFactor = 30, .maxBiterate = 4000000}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0310
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CBR_HIGH_QUALITY, OH_MD_KEY_BITRATE不一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0310, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 6000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0320
 * @tc.name      : 硬编h265, Primary/Secondary编码器均配置为CBR_HIGH_QUALITY, OH_MD_KEY_BITRATE不一致, PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0320, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 5000000,
            .width = 1920, .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 6000000,
            .width = 1920, .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0330
 * @tc.name      : 编码h264, Primary编码器配置为CBR, Secondary编码器配置为CQ
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0330, TestSize.Level0)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_3840_2160_nv12}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CQ, .width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRate = 6000000, .width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0340
 * @tc.name      : 硬编h265, Primary编码器配置为CBR, Secondary编码器配置为CQ
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0340, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CQ}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .bitRate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0350
 * @tc.name      : 硬编h265, Primary编码器配置为CBR, Secondary编码器配置为CQ, PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0350, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CQ, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .bitRate = 5000000, .width = 1920,
            .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0360
 * @tc.name      : 硬编h265, Primary编码器配置为CBR, Secondary编码器配置为SQR, OH_MD_KEY_BITRATE和OH_MD_KEY_MAX_BITRATE一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0360, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = SQR, .sqrFactor = 30,
            .maxBiterate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .bitRate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0370
 * @tc.name      : 硬编h265, Primary编码器配置为CBR, Secondary编码器配置为SQR, BITRATE和MAX_BITRATE一致, PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0370, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = SQR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10, .sqrFactor = 30, .maxBiterate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .bitRate = 5000000, .width = 1920,
            .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0380
 * @tc.name      : 硬编h265, Primary编码器配置为CBR, Secondary编码器配置为CBR_HIGH_QUALITY, OH_MD_KEY_BITRATE一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0380, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .bitRate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0390
 * @tc.name      : 硬编h265, Primary编码器配置为CBR, Secondary编码器配置为CBR_HIGH_QUALITY, BITRATE一致, PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0390, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 5000000,
            .width = 1920, .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .bitRate = 5000000, .width = 1920,
            .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0400
 * @tc.name      : 硬编h265, Primary编码器配置为CQ, Secondary编码器配置为SQR, FACTOR和QUALITY一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0400, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = SQR, .sqrFactor = 30,
            .maxBiterate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CQ, .quality = 30}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0410
 * @tc.name      : 硬编h265, Primary编码器配置为CQ, Secondary编码器配置为SQR, FACTOR和QUALITY一致, PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0410, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = SQR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10, .sqrFactor = 30, .maxBiterate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CQ, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10, .quality = 30}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0420
 * @tc.name      : 硬编h265, Primary编码器配置为CQ, Secondary编码器配置为CBR_HIGH_QUALITY
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0420, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CQ, .quality = 30}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0430
 * @tc.name      : 硬编h265, Primary编码器配置为CQ, Secondary编码器配置为CBR_HIGH_QUALITY, OH_MD_KEY_PROFILE配置为10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0430, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 5000000,
            .width = 1920, .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CQ, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10, .quality = 30}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0440
 * @tc.name      : 硬编h265, Primary编码器配置为SQR, Secondary编码器配置为CBR_HIGH_QUALITY, MAX_BITRATE和BITRATE一致
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0440, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = SQR, .maxBiterate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0450
 * @tc.name      : 硬编h265, Primary编码器配置为SQR, Secondary编码器配置为CBR_HIGH_QUALITY, MAX_BITRATE和BITRATE一致, 10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0450, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 5000000,
            .width = 1920, .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = SQR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10, .maxBiterate = 5000000}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0460
 * @tc.name      : 硬编h265, Primary编码器 OH_MD_KEY_PROFILE配置为10bit, Secondary配置为8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0460, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR, .width = 1920, .height = 1088}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_LT(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0470
 * @tc.name      : 编码h264, Primary编码器setparameter中模式不变, 修改参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0470, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchPrimary = true, .enableSwitchParame = true,
            .bitrateMode = {CBR}, .cbrBitrate = 6000000};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0480
 * @tc.name      : 编码h264, Primary编码器setparameter中模式切换CBR->VBR, 修改参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0480, TestSize.Level0)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchPrimary = true, .enableSwitchMode = true, .enableSwitchParame = true,
            .bitrateMode = {VBR}, .vbrBitrate = 5000000};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0490
 * @tc.name      : 硬编h265, Primary编码器setparameter中模式切换CBR->VBR->SQR->CBR_HIGH_QUALITY, 修改参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0490, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchPrimary = true, .enableSwitchMode = true, .enableSwitchParame = true,
            .bitrateMode = {VBR, SQR, CBR_HIGH_QUALITY}, .sqrFactor = 30, .maxBitrate = 6000000,
            .vbrBitrate = 5000000, .cbrHighQuality = 5000000};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0500
 * @tc.name      : 硬编h265, Primary编码器setparameter中8bit->10bit, 报错
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0500, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchPrimary = true, .enableSwitchBitParame = true,
            .bitrateMode = {CBR}, .hevcProfile = HEVC_PROFILE_MAIN_10};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0510
 * @tc.name      : 硬编h265, Primary编码器setparameter中10bit->8bit, 报错
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0510, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchPrimary = true, .enableSwitchBitParame = true,
            .bitrateMode = {CBR}, .hevcProfile = HEVC_PROFILE_MAIN};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0520
 * @tc.name      : 编码h264, Secondary编码器setparameter中模式不变, 修改参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0520, TestSize.Level1)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchSecondary = true, .enableSwitchParame = true,
            .bitrateMode = {CBR}, .cbrBitrate = 6000000};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0530
 * @tc.name      : 编码h264, Secondary编码器setparameter中模式切换CBR->VBR, 修改参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0530, TestSize.Level0)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchSecondary = true, .enableSwitchMode = true, .enableSwitchParame = true,
            .bitrateMode = {VBR}, .vbrBitrate = 5000000};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0540
 * @tc.name      : 硬编h265, Secondary编码器setparameter中模式切换CBR->VBR->SQR->CBR_HIGH_QUALITY, 修改参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0540, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchSecondary = true, .enableSwitchMode = true, .enableSwitchParame = true,
            .bitrateMode = {VBR, SQR, CBR_HIGH_QUALITY}, .sqrFactor = 30, .maxBitrate = 6000000,
            .vbrBitrate = 5000000, .cbrHighQuality = 5000000};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0550
 * @tc.name      : 硬编h265, Secondary编码器setparameter中8bit->10bit, 报错
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0550, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchSecondary = true, .enableSwitchBitParame = true,
            .bitrateMode = {CBR}, .hevcProfile = HEVC_PROFILE_MAIN_10};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0560
 * @tc.name      : 硬编h265, Secondary编码器setparameter中10bit->8bit, 报错
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0560, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchSecondary = true, .enableSwitchBitParame = true,
            .bitrateMode = {CBR}, .hevcProfile = HEVC_PROFILE_MAIN};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0570
 * @tc.name      : 编码h264, Primary+Secondary编码器setparameter中模式不变, 修改参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0570, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchPrimary = true, .enableSwitchSecondary = true, .enableSwitchParame = true,
            .bitrateMode = {CBR}, .cbrBitrate = 6000000};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0580
 * @tc.name      : 编码h264, Primary+Secondary编码器setparameter中模式切换CBR->VBR, 修改参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0580, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchPrimary = true, .enableSwitchSecondary = true, .enableSwitchMode = true,
            .enableSwitchParame = true, .bitrateMode = {VBR}, .vbrBitrate = 5000000};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0590
 * @tc.name      : 硬编h265, Primary+Secondary编码器setparameter中模式切换CBR->VBR->SQR->CBR_HIGH_QUALITY, 修改参数
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0590, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->parameter = {.enableSwitchPrimary = true, .enableSwitchSecondary = true, .enableSwitchMode = true,
            .enableSwitchParame = true, .bitrateMode = {VBR, SQR, CBR_HIGH_QUALITY}, .sqrFactor = 30,
            .maxBitrate = 6000000, .vbrBitrate = 5000000, .cbrHighQuality = 5000000};
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
        ASSERT_EQ(false, vEncSample->setParameterFail);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0600
 * @tc.name      : 硬编h265, Primary编码器B帧编码, Secondary普通编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0600, TestSize.Level1)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .enbleBFrame = 1}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0610
 * @tc.name      : 硬编h265, Primary编码器B帧编码, SecondaryB帧编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0610, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR, .enbleBFrame = 1}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .enbleBFrame = 1}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0620
 * @tc.name      : 硬编h265, Primary编码器B帧编码, Secondary重复编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0620, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.enbleBFrame = 1}));
        vEncSample->enableRepeat = true;
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0630
 * @tc.name      : 硬编h265, Primary编码器B帧编码, Secondary ROI编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0630, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.enbleBFrame = 1}));
        vEncSample->enableDualRoi = true;
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0640
 * @tc.name      : 硬编h265, Primary编码器B帧编码, Secondary TEMPORAL
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0640, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.enbleBFrame = 1}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0650
 * @tc.name      : 编码h264, Primary重复编码, Secondary普通编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0650, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0660
 * @tc.name      : 编码h264, Primary重复编码, Secondary重复编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0660, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0670
 * @tc.name      : 编码h264, Primary重复编码, Secondary ROI编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0670, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        vEncSample->enableDualRoi = true;
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0680
 * @tc.name      : 编码h264, Primary重复编码, Secondary TEMPORAL
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0680, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0690
 * @tc.name      : 硬编h265, Primary重复编码, Secondary普通编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0690, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0700
 * @tc.name      : 硬编h265, Primary重复编码, Secondary重复编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0700, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0710
 * @tc.name      : 硬编h265, Primary重复编码, Secondary ROI编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0710, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        vEncSample->enableDualRoi = true;
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0720
 * @tc.name      : 硬编h265, Primary重复编码, Secondary TEMPORAL
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0720, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.repeat = true, .maxCount = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0730
 * @tc.name      : 编码h264, Primary ROI编码, Secondary普通编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0730, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0740
 * @tc.name      : 编码h264, Primary ROI编码, Secondary ROI编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0740, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->enableDualRoi = true;
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0750
 * @tc.name      : 编码h264, Primary ROI编码, Secondary TEMPORAL
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0750, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0760
 * @tc.name      : 硬编h265, Primary ROI编码, Secondary普通编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0760, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0770
 * @tc.name      : 硬编h265, Primary ROI编码, Secondary ROI编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0770, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0780
 * @tc.name      : 硬编h265, Primary ROI编码, Secondary TEMPORAL
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0780, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.roi = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0790
 * @tc.name      : 编码h264, Primary TEMPORAL, Secondary 普通编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0790, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0800
 * @tc.name      : 编码h264, Primary TEMPORAL, Secondary TEMPORAL
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0800, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0810
 * @tc.name      : 硬编h265, Primary TEMPORAL, Secondary 普通编码, 配置SQR
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0810, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = SQR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = SQR, .bitRate = 6000000,
            .enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0820
 * @tc.name      : 硬编h265, Primary TEMPORAL, Secondary 普通编码, 配置SQR, 10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0820, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = SQR, .width = 1920, .height = 1088,
            .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = SQR, .bitRate = 6000000, .width = 1920,
            .height = 1088, .enableTemporal = true, .temporalConfig = true}, true));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0830
 * @tc.name      : 硬编h265, Primary TEMPORAL, Secondary 普通编码, 配置CBR_HIGH_QUALITY
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0830, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 6000000}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .enableTemporal = true,
            .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0840
 * @tc.name      : 硬编h265, Primary TEMPORAL, Secondary 普通编码, 配置CBR_HIGH_QUALITY, 10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0840, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1920_1088_nv12_10bit}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .bitRate = 6000000,
            .width = 1920, .height = 1088, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR_HIGH_QUALITY, .enableTemporal = true,
            .width = 1920, .height = 1088, .temporalConfig = true}, true));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0850
 * @tc.name      : 硬编h265, Primary TEMPORAL, Secondary TEMPORAL
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0850, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.enableTemporal = true, .temporalConfig = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0860
 * @tc.name      : 编码h264, Primary变分变率/格式
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0860, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_640_480_rgba,
            file_1280_536_nv21, file_1280_720_nv, file_1920_1080_nv21, file_3840_2160_nv12}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR, .width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0870
 * @tc.name      : 硬编h265, Primary变分变率/格式，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0870, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_640_480_rgba,
            file_1280_720_nv, file_1920_1080_nv21, file_1280_536_nv21, file_3840_2160_nv12}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR, .width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .width = 3840, .height = 2160}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0880
 * @tc.name      : 硬编H265, Primary 变分变率/格式， 10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0880, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv12_10bit,
            file_176_144_rgba1010102}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .hevcProfile = HEVC_PROFILE_MAIN_10}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0890
 * @tc.name      : 编码H264, Primary I帧请求
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0890, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.frameInterval = 0}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0900
 * @tc.name      : 硬编H265, Primary I帧请求
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0900, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.frameInterval = 0}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0910
 * @tc.name      : 编码H264, Primary colorSpace Param
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0910, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.rangeFlag = 1, .enableColorSpaceParam = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0920
 * @tc.name      : 硬编H265, Primary colorSpace Param
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0920, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.rangeFlag = 1, .enableColorSpaceParam = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0930
 * @tc.name      : 编码h264, 创建Primary/Secondary编码器, 只启用Primary编码器
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0930, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .dualEos = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample, false, true));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0940
 * @tc.name      : 硬编h265, 创建Primary/Secondary编码器, 只启用Secondary编码器
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0940, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR, .surfSecondaryInput = true}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchPrimaryEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateSurface());
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetDualCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchSecondaryEncoder());
        vEncSample->isSurface = true;
        vEncSample->dualEos = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0950
 * @tc.name      : 编码H264, 销毁(destroy)Secondary, 使用Primary
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0950, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .dualEos = true}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ReleaseSecondary());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0960
 * @tc.name      : 硬编H265, Primary destroy
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0960, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->needStopInLoop = true;
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Stop());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Release());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0970
 * @tc.name      : 编码h264, Primary过程中flush、NotifyEndOfStream、reset、stop, Secondary不影响
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0970, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->needStopInLoop = true;
        vEncSample->noStop = true;
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .dualEos = true}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->NotifyEndOfStream());
        vEncSample->enbleSyncMode = 1;
        ASSERT_EQ(AV_ERR_OK, vEncSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Stop());
        ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0980
 * @tc.name      : 硬编h265, Secondary过程中flush、NotifyEndOfStream、reset、stop, Primary不影响
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0980, TestSize.Level2)
{
    if (cap_hevc != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .dualEos = true}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->NotifyEndOfStreamSecondary());
        ASSERT_EQ(AV_ERR_OK, vEncSample->FlushSecondary());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StopSecondary());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ResetSecondary());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_0990
 * @tc.name      : 编码264, 不创建Secondary, Primary不影响
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_0990, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->runDualEncoder = true;
        vEncSample->readMultiFiles = true;
        vEncSample->fileInfos.push_back(file_1280_720_nv);
        vEncSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DUAL_0990_1.h264";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreatePrimaryEncoder(OH_AVCODEC_MIMETYPE_VIDEO_AVC));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR, .dualEos = true}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample, false, true));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_1000
 * @tc.name      : 硬编H265, 创建Primary、Secondary, 销毁Secondary, 重建Secondary
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1000, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->repeatCreateSecondary = true;
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = CBR}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_1010
 * @tc.name      : 编码h264转码(解码+编码surface)
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1010, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto sample = make_unique<VideoTransCodeApi11Sample>();
        sample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1104;
        sample->DEFAULT_HEIGHT = 622;
        sample->INP_DIR = "/data/test/media/1104x622.h264";
        sample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DUAL_1010_1.h264";
        sample->OUT_DIR_SEC = "/data/test/media/VIDEO_ENCODE_DUAL_1010_2.h264";
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
 * @tc.number    : VIDEO_ENCODE_DUAL_1020
 * @tc.name      : 硬编h265转码(解码+编码surface)
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1020, TestSize.Level2)
{
    if (cap_hevc_dec != nullptr && !access("/system/lib64/media/", 0)) {
        shared_ptr<VideoTransCodeApi11Sample> sample = make_shared<VideoTransCodeApi11Sample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
        sample->DEFAULT_WIDTH = 1920;
        sample->DEFAULT_HEIGHT = 1080;
        sample->INP_DIR = "/data/test/media/1920x1080.h265";
        sample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DUAL_1020_1.h265";
        sample->OUT_DIR_SEC = "/data/test/media/VIDEO_ENCODE_DUAL_1020_2.h265";
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
 * @tc.number    : VIDEO_ENCODE_DUAL_1030
 * @tc.name      : 硬编h265 hdr转码(解码+编码surface)
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1030, TestSize.Level2)
{
    if (cap_hevc_dec != nullptr && !access("/system/lib64/media/", 0)) {
        shared_ptr<VideoTransCodeApi11Sample> sample = make_shared<VideoTransCodeApi11Sample>();
        sample->DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
        sample->DEFAULT_WIDTH = 3840;
        sample->DEFAULT_HEIGHT = 2160;
        sample->INP_DIR = "/data/test/media/hdr10.h265";
        sample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_DUAL_1030_1.h265";
        sample->OUT_DIR_SEC = "/data/test/media/VIDEO_ENCODE_DUAL_1030_2.h265";
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
 * @tc.number    : VIDEO_ENCODE_DUAL_1040
 * @tc.name      : 编码H264, Primary/Secondary配置不同的pix_format
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1040, TestSize.Level2)
{
    if (cap != nullptr && !access("/system/lib64/media/", 0)) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR, .pixFmt = AV_PIXEL_FORMAT_NV21}));
        ASSERT_EQ(AV_ERR_OK, SetPrimaryConfig(vEncSample, {.bitRateMode = VBR, .pixFmt = AV_PIXEL_FORMAT_NV12}));
        ASSERT_EQ(AV_ERR_OK, StartEncoder(vEncSample));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_1050
 * @tc.name      : 硬编H265, buffer模式相关接口
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1050, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vEncSample->PushInputBuffer(1));
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_1060
 * @tc.name      : 编码H264, 配置同步模式
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1060, TestSize.Level3)
{
    if (cap != nullptr) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        vEncSample->enbleSyncMode = 1;
        ASSERT_EQ(AV_ERR_INVALID_VAL, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_1070
 * @tc.name      : 硬编H265, 同步模式相关接口
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1070, TestSize.Level3)
{
    if (cap_hevc != nullptr) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vEncSample->QueryInputBuffer(0, 0));
        ASSERT_EQ(nullptr, vEncSample->GetInputBuffer(0));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vEncSample->QueryOutputBuffer(0, 0));
        ASSERT_EQ(nullptr, vEncSample->GetOutputBuffer(0));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_1080
 * @tc.name      : 编码264, SetCallback旧通路
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1080, TestSize.Level3)
{
    if (cap != nullptr) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        vEncSample->runDualEncoder = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreatePrimaryEncoder(OH_AVCODEC_MIMETYPE_VIDEO_AVC));
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}, true));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_1090
 * @tc.name      : 硬编H265, OH_VideoEncoder_PushInputParameter
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1090, TestSize.Level3)
{
    if (cap != nullptr) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_AVC, {file_1280_720_nv}));
        ASSERT_EQ(AV_ERR_OK, SetSecondaryConfig(vEncSample, {.bitRateMode = VBR}));
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartSecondaryEncoder());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vEncSample->PushInputParameter(0));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_DUAL_1100
 * @tc.name      : 编码H265, OH_VideoEncoder_RegisterParameterCallback
 * @tc.desc      : function test
 */
HWTEST_F(HwEncDualNdkTest, VIDEO_ENCODE_DUAL_1100, TestSize.Level3)
{
    if (cap_hevc != nullptr) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, CreateEncoder(vEncSample, OH_AVCODEC_MIMETYPE_VIDEO_HEVC, {file_1280_720_nv}));
        vEncSample->DEFAULT_BITRATE_MODE = VBR;
        vEncSample->SURF_INPUT = true;
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vEncSample->SwitchPrimaryEncoder());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vEncSample->SetVideoEncoderCallback());
    }
}
} // namespace