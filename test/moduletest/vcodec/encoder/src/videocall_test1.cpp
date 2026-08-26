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
#include "avcodec_videocall.h"
#include "avcodec_errors.h"

namespace OHOS {
namespace MediaAVCodec {
class VideoCallNotifyPiPModuleTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void) {}
    void TearDown(void) {}
};

class TestLowPowerCallback : public MediaCodecLowPowerConsumptionCallback {
public:
    void OnLowPowerConsumptionAvailable(std::shared_ptr<StrategyInfo> strategy) override
    {
        if (strategy != nullptr) {
            lastBundleName = strategy->bundleName;
            lastStrategyId = strategy->strategyId;
            callCount++;
        }
    }
    std::string lastBundleName;
    int32_t lastStrategyId = -1;
    int32_t callCount = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
constexpr int32_t TEST_EXPECT_OK = AVCS_ERR_OK;
constexpr int32_t TEST_EXPECT_INPUT_ERROR = AVCS_ERR_INPUT_DATA_ERROR;
const std::string BUNDLE_NAME_VALID = "com.test.videocall";
const std::string BUNDLE_NAME_DIFFERENT = "com.example.app.video";
const std::string BUNDLE_NAME_EMPTY = "";
} // namespace

/**
 * @tc.name: VideoCall_NotifyPiPState_001
 * @tc.desc: VideoCallNotifyPiPState returns OK
 *           with valid bundleName and isFloatingWindow=true
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_NotifyPiPState_001, TestSize.Level1)
{
    std::string bundleName = BUNDLE_NAME_VALID;
    bool isFloatingWindow = true;
    int32_t ret = AVCodecVideoCall::VideoCallNotifyPiPState(bundleName, isFloatingWindow);
    EXPECT_EQ(ret, TEST_EXPECT_OK);
    // Valid bundleName with floating window mode should succeed
    EXPECT_NE(ret, TEST_EXPECT_INPUT_ERROR);
}

/**
 * @tc.name: VideoCall_NotifyPiPState_002
 * @tc.desc: VideoCallNotifyPiPState returns INPUT_DATA_ERROR
 *           when bundleName is empty and isFloatingWindow=true
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_NotifyPiPState_002, TestSize.Level1)
{
    std::string bundleName = BUNDLE_NAME_EMPTY;
    bool isFloatingWindow = true;
    int32_t ret = AVCodecVideoCall::VideoCallNotifyPiPState(bundleName, isFloatingWindow);
    EXPECT_EQ(ret, TEST_EXPECT_INPUT_ERROR);
    // Empty bundleName should fail with input data error
    EXPECT_NE(ret, TEST_EXPECT_OK);
}

/**
 * @tc.name: VideoCall_NotifyPiPState_003
 * @tc.desc: VideoCallNotifyPiPState returns OK
 *           with different valid bundleName formats
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_NotifyPiPState_003, TestSize.Level1)
{
    std::string bundleName = BUNDLE_NAME_DIFFERENT;
    bool isFloatingWindow = false;
    int32_t ret = AVCodecVideoCall::VideoCallNotifyPiPState(bundleName, isFloatingWindow);
    EXPECT_EQ(ret, TEST_EXPECT_OK);
    // Different bundleName format should also succeed
    EXPECT_NE(ret, TEST_EXPECT_INPUT_ERROR);
    // Verify return value is positive success code
    EXPECT_GE(ret, 0);
}

/**
 * @tc.name: VideoCall_NotifyPiPState_004
 * @tc.desc: VideoCallNotifyPiPState returns INPUT_DATA_ERROR
 *           when bundleName is empty and isFloatingWindow=false
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_NotifyPiPState_004, TestSize.Level1)
{
    std::string bundleName = BUNDLE_NAME_EMPTY;
    bool isFloatingWindow = false;
    int32_t ret = AVCodecVideoCall::VideoCallNotifyPiPState(bundleName, isFloatingWindow);
    EXPECT_EQ(ret, TEST_EXPECT_INPUT_ERROR);
    // Empty bundleName should fail regardless of isFloatingWindow
    EXPECT_NE(ret, TEST_EXPECT_OK);
}

/**
 * @tc.name: VideoCall_RegisterCallback_001
 * @tc.desc: RegisterCallback returns OK with valid callback
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_RegisterCallback_001, TestSize.Level1)
{
    auto callback = std::make_shared<TestLowPowerCallback>();
    int32_t ret = AVCodecVideoCall::RegisterCallback(callback);
    EXPECT_EQ(ret, TEST_EXPECT_OK);
    // Valid callback should register successfully
    EXPECT_NE(ret, TEST_EXPECT_INPUT_ERROR);
    // Clean up
    AVCodecVideoCall::UnRegisterCallback();
}

/**
 * @tc.name: VideoCall_RegisterCallback_002
 * @tc.desc: RegisterCallback returns INPUT_DATA_ERROR when callback is nullptr
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_RegisterCallback_002, TestSize.Level1)
{
    int32_t ret = AVCodecVideoCall::RegisterCallback(nullptr);
    EXPECT_EQ(ret, TEST_EXPECT_INPUT_ERROR);
    // nullptr callback should fail with input data error
    EXPECT_NE(ret, TEST_EXPECT_OK);
}

/**
 * @tc.name: VideoCall_RegisterCallback_003
 * @tc.desc: RegisterCallback with re-register replaces the previous callback
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_RegisterCallback_003, TestSize.Level1)
{
    auto callback1 = std::make_shared<TestLowPowerCallback>();
    int32_t ret1 = AVCodecVideoCall::RegisterCallback(callback1);
    EXPECT_EQ(ret1, TEST_EXPECT_OK);

    auto callback2 = std::make_shared<TestLowPowerCallback>();
    int32_t ret2 = AVCodecVideoCall::RegisterCallback(callback2);
    EXPECT_EQ(ret2, TEST_EXPECT_OK);
    // Re-registering with a new callback should succeed
    AVCodecVideoCall::UnRegisterCallback();
}

/**
 * @tc.name: VideoCall_UnRegisterCallback_001
 * @tc.desc: UnRegisterCallback succeeds after valid RegisterCallback
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_UnRegisterCallback_001, TestSize.Level1)
{
    auto callback = std::make_shared<TestLowPowerCallback>();
    int32_t ret = AVCodecVideoCall::RegisterCallback(callback);
    EXPECT_EQ(ret, TEST_EXPECT_OK);
    // UnRegisterCallback should not crash after valid registration
    AVCodecVideoCall::UnRegisterCallback();
    // No return value to check; just ensure no crash
    EXPECT_EQ(ret, TEST_EXPECT_OK);
}

/**
 * @tc.name: VideoCall_UnRegisterCallback_002
 * @tc.desc: UnRegisterCallback called without prior RegisterCallback does not crash
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_UnRegisterCallback_002, TestSize.Level1)
{
    // UnRegisterCallback without any prior registration should not crash
    AVCodecVideoCall::UnRegisterCallback();
    // No return value to check; just ensure no crash
    SUCCEED();
}

/**
 * @tc.name: VideoCall_UnRegisterCallback_003
 * @tc.desc: UnRegisterCallback called multiple times consecutively does not crash
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_UnRegisterCallback_003, TestSize.Level1)
{
    auto callback = std::make_shared<TestLowPowerCallback>();
    int32_t ret = AVCodecVideoCall::RegisterCallback(callback);
    EXPECT_EQ(ret, TEST_EXPECT_OK);

    AVCodecVideoCall::UnRegisterCallback();
    // Calling UnRegisterCallback again after already unregistered should not crash
    AVCodecVideoCall::UnRegisterCallback();
    AVCodecVideoCall::UnRegisterCallback();
    SUCCEED();
}

/**
 * @tc.name: VideoCall_RegisterUnRegister_001
 * @tc.desc: Register and UnRegister repeatedly does not crash or leak
 */
HWTEST_F(VideoCallNotifyPiPModuleTest, VideoCall_RegisterUnRegister_001, TestSize.Level1)
{
    constexpr int32_t repeatCount = 10;
    for (int32_t i = 0; i < repeatCount; i++) {
        auto callback = std::make_shared<TestLowPowerCallback>();
        int32_t ret = AVCodecVideoCall::RegisterCallback(callback);
        EXPECT_EQ(ret, TEST_EXPECT_OK);
        AVCodecVideoCall::UnRegisterCallback();
    }
    SUCCEED();
}
