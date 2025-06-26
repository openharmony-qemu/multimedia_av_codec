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
#include <string>
#include <limits>
#include "gtest/gtest.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videoenc_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
namespace {
OH_AVCodec *venc_ = NULL;
OH_AVFormat *format;
OH_AVCapability *cap = nullptr;
constexpr uint32_t CODEC_NAME_SIZE = 128;
constexpr uint32_t DEFAULT_BITRATE = 1000000;
char g_codecName[CODEC_NAME_SIZE] = {};
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
} // namespace
namespace OHOS {
namespace Media {
class AvcSwEncConfigureNdkTest : public testing::Test {
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

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

void AvcSwEncConfigureNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, SOFTWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}
void AvcSwEncConfigureNdkTest::TearDownTestCase() {}
void AvcSwEncConfigureNdkTest::SetUp() {}
void AvcSwEncConfigureNdkTest::TearDown()
{
    if (venc_ != NULL) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
}
namespace {


/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_1000
 * @tc.name      : OH_AVCapability_GetVideoWidthRangeForHeight param correct
 * @tc.desc      : api test
 */
HWTEST_F(AvcSwEncConfigureNdkTest, VIDEO_ENCODE_CAPABILITY_1000, TestSize.Level2)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULT_HEIGHT, &range);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_GE(range.minVal, 0);
    ASSERT_GT(range.maxVal, 0);
    venc_ = OH_VideoEncoder_CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, range.maxVal + 1);
    EXPECT_NE(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_VideoEncoder_Destroy(venc_);
    venc_ = OH_VideoEncoder_CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, range.minVal - 1);
    EXPECT_NE(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_2000
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param correct
 * @tc.desc      : api test
 */
HWTEST_F(AvcSwEncConfigureNdkTest, VIDEO_ENCODE_CAPABILITY_2000, TestSize.Level2)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULT_WIDTH, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_GE(range.minVal, 0);
    ASSERT_GT(range.maxVal, 0);
    venc_ = OH_VideoEncoder_CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, range.maxVal + 1);
    EXPECT_NE(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_VideoEncoder_Destroy(venc_);
    venc_ = OH_VideoEncoder_CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, range.minVal - 1);
    EXPECT_NE(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_3000
 * @tc.name      : OH_AVCapability_GetVideoWidthRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(AvcSwEncConfigureNdkTest, VIDEO_ENCODE_CAPABILITY_3000, TestSize.Level2)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_GE(range.minVal, 0);
    ASSERT_GT(range.maxVal, 0);
    venc_ = OH_VideoEncoder_CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, range.maxVal + 1);
    EXPECT_NE(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_VideoEncoder_Destroy(venc_);
    venc_ = OH_VideoEncoder_CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, range.minVal - 1);
    EXPECT_NE(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
}


/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4000
 * @tc.name      : OH_AVCapability_GetVideoHeightRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(AvcSwEncConfigureNdkTest, VIDEO_ENCODE_CAPABILITY_4000, TestSize.Level2)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, SOFTWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_GE(range.minVal, 0);
    ASSERT_GT(range.maxVal, 0);
    venc_ = OH_VideoEncoder_CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, range.maxVal + 1);
    EXPECT_NE(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_VideoEncoder_Destroy(venc_);
    venc_ = OH_VideoEncoder_CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, range.minVal - 1);
    EXPECT_NE(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
}
}