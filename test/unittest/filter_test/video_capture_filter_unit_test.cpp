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

#include "video_capture_filter_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void VideoCaptureFilterUnitTest::SetUpTestCase(void) {}

void VideoCaptureFilterUnitTest::TearDownTestCase(void) {}

void VideoCaptureFilterUnitTest::SetUp(void)
{
    videoCaptureFilter_ = std::make_shared<VideoCaptureFilter>("testVideoCaptureFilter", FilterType::VIDEO_CAPTURE);
}

void VideoCaptureFilterUnitTest::TearDown(void)
{
    videoCaptureFilter_ = nullptr;
}

/**
 * @tc.name: VideoCaptureFilter_Init_001
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_Init_001, TestSize.Level1)
{
    std::shared_ptr<TestEventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> testFilterCallback = std::make_shared<TestFilterCallback>();
    videoCaptureFilter_->Init(testEventReceiver, testFilterCallback);

    EXPECT_EQ(videoCaptureFilter_->eventReceiver_, testEventReceiver);
}

/**
 * @tc.name: VideoCaptureFilter_Configure_001
 * @tc.desc: Configure
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_Configure_001, TestSize.Level1)
{
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    Status ret = videoCaptureFilter_->Configure();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_SetInputSurface_001
 * @tc.desc: SetInputSurface
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_SetInputSurface_001, TestSize.Level1)
{
    sptr<Surface> consumerSurface = nullptr;
    Status ret = videoCaptureFilter_->SetInputSurface(consumerSurface);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    consumerSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_TRUE(consumerSurface);
    ret = videoCaptureFilter_->SetInputSurface(consumerSurface);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN); 
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS