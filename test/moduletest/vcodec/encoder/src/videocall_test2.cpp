/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <memory>
#include "gtest/gtest.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videoenc_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "avcodec_videocall.h"
#include "avcodec_errors.h"

namespace {
OH_AVCapability *cap = nullptr;
constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecName[CODEC_NAME_SIZE] = {};
} // namespace

namespace OHOS {
namespace Media {
class VideoCallEncModuleTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};
} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

void VideoCallEncModuleTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName),
        tmpCodecName, strlen(tmpCodecName)) != 0) {
        cout << "memcpy failed" << endl;
    }
    cout << "codecname: " << g_codecName << endl;
}

void VideoCallEncModuleTest::TearDownTestCase()
{
    cap = nullptr;
    if (memset_s(g_codecName, sizeof(g_codecName), 0, sizeof(g_codecName)) != 0) {
        cout << "memset failed" << endl;
    }
}

void VideoCallEncModuleTest::SetUp() {}

void VideoCallEncModuleTest::TearDown() {}

namespace {
constexpr int32_t TEST_EXPECT_OK = AVCS_ERR_OK;
const std::string BUNDLE_NAME_VALID = "com.test.videocall";
const std::string BUNDLE_NAME_EMPTY = "";
} // namespace

/**
 * @tc.number    : VIDEO_CALL_PIP_STATE_0010
 * @tc.name      : VideoCallNotifyPiPState with encoder created
 * @tc.desc      : Create encoder then call framework API,
 *                 VideoCallNotifyPiPState returns OK
 *                 with valid bundleName and isFloatingWindow=true
 */
HWTEST_F(VideoCallEncModuleTest, VIDEO_CALL_PIP_STATE_0010, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->ConfigureVideoEncoder());

    std::string bundleName = BUNDLE_NAME_VALID;
    bool isFloatingWindow = true;
    // Call through framework layer (AVCodecVideoCall),
    // which validates bundleName and delegates to service
    int32_t ret = AVCodecVideoCall::VideoCallNotifyPiPState(bundleName, isFloatingWindow);
    EXPECT_EQ(ret, TEST_EXPECT_OK);
    EXPECT_NE(ret, AVCS_ERR_SERVICE_DIED);
    EXPECT_TRUE(!bundleName.empty());
    EXPECT_TRUE(isFloatingWindow);
}

/**
 * @tc.number    : VIDEO_CALL_PIP_STATE_0020
 * @tc.name      : VideoCallNotifyPiPState with encoder
 *                 and isFloatingWindow=false
 * @tc.desc      : Create encoder then call framework API,
 *                 VideoCallNotifyPiPState returns OK
 *                 with valid bundleName and isFloatingWindow=false
 */
HWTEST_F(VideoCallEncModuleTest, VIDEO_CALL_PIP_STATE_0020, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->ConfigureVideoEncoder());

    std::string bundleName = BUNDLE_NAME_VALID;
    bool isFloatingWindow = false;
    int32_t ret = AVCodecVideoCall::VideoCallNotifyPiPState(bundleName, isFloatingWindow);
    EXPECT_EQ(ret, TEST_EXPECT_OK);
    EXPECT_NE(ret, AVCS_ERR_SERVICE_DIED);
    EXPECT_TRUE(!bundleName.empty());
    EXPECT_FALSE(isFloatingWindow);
}

/**
 * @tc.number    : VIDEO_CALL_PIP_STATE_0030
 * @tc.name      : VideoCallNotifyPiPState without encoder via framework
 * @tc.desc      : Call framework API directly (without encoder instance).
 *                 In module test environment the SA service is typically running,
 *                 so the proxy is available and the call should succeed.
 *                 The result depends on whether the SA service is active,
 *                 not on whether an encoder instance has been created.
 */
HWTEST_F(VideoCallEncModuleTest, VIDEO_CALL_PIP_STATE_0030, TestSize.Level1)
{
    std::string bundleName = BUNDLE_NAME_VALID;
    bool isFloatingWindow = true;
    int32_t ret = AVCodecVideoCall::VideoCallNotifyPiPState(bundleName, isFloatingWindow);
    // In module test environment, SA service is typically already running,
    // so the proxy should be available and the call succeeds.
    // If the SA service is not running, ret would be AVCS_ERR_SERVICE_DIED.
    // We verify the call does not crash and returns a valid error code.
    EXPECT_TRUE(ret == AVCS_ERR_OK || ret == AVCS_ERR_SERVICE_DIED);
    EXPECT_NE(ret, AVCS_ERR_INPUT_DATA_ERROR);
}

/**
 * @tc.number    : VIDEO_CALL_PIP_STATE_0040
 * @tc.name      : VideoCallNotifyPiPState with empty bundleName via framework
 * @tc.desc      : Framework layer (AVCodecVideoCall) validates bundleName
 *                 and returns AVCS_ERR_INPUT_DATA_ERROR for empty strings.
 *                 This tests the framework-layer guard, not the IPC/server path.
 */
HWTEST_F(VideoCallEncModuleTest, VIDEO_CALL_PIP_STATE_0040, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->ConfigureVideoEncoder());

    std::string bundleName = BUNDLE_NAME_EMPTY;
    bool isFloatingWindow = false;
    // Framework layer CHECK_AND_RETURN_RET_LOG intercepts empty bundleName
    // before reaching IPC, so this always returns AVCS_ERR_INPUT_DATA_ERROR
    int32_t ret = AVCodecVideoCall::VideoCallNotifyPiPState(bundleName, isFloatingWindow);
    EXPECT_EQ(ret, AVCS_ERR_INPUT_DATA_ERROR);
    EXPECT_TRUE(bundleName.empty());
    EXPECT_FALSE(isFloatingWindow);
}
