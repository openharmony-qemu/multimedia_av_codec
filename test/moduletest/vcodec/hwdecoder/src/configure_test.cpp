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
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"


using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class HwdecConfigureNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();

protected:
    const char *INP_DIR_720_30 = "/data/test/media/1280_720_30_10Mb.h264";
    const char *INP_DIR_1080_30 = "/data/test/media/1920_1080_10_30Mb.h264";
};
} // namespace Media
} // namespace OHOS

namespace {
static OH_AVCapability *cap = nullptr;
static OH_AVCapability *cap_hevc = nullptr;
static string g_codecName = "";
static string g_codecNameHEVC = "";
constexpr int32_t DEFAULT_WIDTH = 1920;
constexpr int32_t DEFAULT_HEIGHT = 1080;
} // namespace

void HwdecConfigureNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    g_codecName = OH_AVCapability_GetName(cap);
    cout << "codecname: " << g_codecName << endl;
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    g_codecNameHEVC = OH_AVCapability_GetName(cap_hevc);
    cout << "g_codecNameHEVC: " << g_codecNameHEVC << endl;
}
void HwdecConfigureNdkTest::TearDownTestCase() {}
void HwdecConfigureNdkTest::SetUp() {}
void HwdecConfigureNdkTest::TearDown() {
}

namespace {
/**
 * @tc.number    : VIDEO_HWDEC_CONFIGURE_0010
 * @tc.name      : set max input size with illegal value
 * @tc.desc      : configure test
 */
HWTEST_F(HwdecConfigureNdkTest, VIDEO_HWDEC_CONFIGURE_0010, TestSize.Level1)
{
    OH_AVCodec *vdec = OH_VideoDecoder_CreateByName(g_codecName.c_str());
    ASSERT_NE(NULL, vdec);
    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, -1);
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec, format));
    OH_VideoDecoder_Destroy(vdec);
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_HWDEC_CONFIGURE_0020
 * @tc.name      : set max input size with illegal value
 * @tc.desc      : configure test
 */
HWTEST_F(HwdecConfigureNdkTest, VIDEO_HWDEC_CONFIGURE_0020, TestSize.Level1)
{
    OH_AVCodec *vdec = OH_VideoDecoder_CreateByName(g_codecName.c_str());
    ASSERT_NE(NULL, vdec);
    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, 0);
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec, format));
    OH_VideoDecoder_Destroy(vdec);
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_HWDEC_CONFIGURE_0030
 * @tc.name      : set max input size with illegal value HEVC
 * @tc.desc      : configure test
 */
HWTEST_F(HwdecConfigureNdkTest, VIDEO_HWDEC_CONFIGURE_0030, TestSize.Level1)
{
    OH_AVCodec *vdec = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
    ASSERT_NE(NULL, vdec);
    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, -1);
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec, format));
    OH_VideoDecoder_Destroy(vdec);
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_HWDEC_CONFIGURE_0040
 * @tc.name      : set max input size with illegal value HEVC
 * @tc.desc      : configure test
 */
HWTEST_F(HwdecConfigureNdkTest, VIDEO_HWDEC_CONFIGURE_0040, TestSize.Level1)
{
    OH_AVCodec *vdec = OH_VideoDecoder_CreateByName(g_codecNameHEVC.c_str());
    ASSERT_NE(NULL, vdec);
    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, 0);
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec, format));
    OH_VideoDecoder_Destroy(vdec);
    OH_AVFormat_Destroy(format);
}
}