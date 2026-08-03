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
