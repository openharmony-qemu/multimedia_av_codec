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

#include "videoenc_unit_test.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;

namespace {
std::atomic<int32_t> vencCount = 0;
std::string vencName = "";

void MultiThreadCreateVEnc()
{
    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    std::shared_ptr<VEncCallbackTest> vencCallback = std::make_shared<VEncCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencCallback);

    std::shared_ptr<VideoEncSample> videoEnc = std::make_shared<VideoEncSample>(vencSignal);
    ASSERT_NE(nullptr, videoEnc);

    EXPECT_LE(vencCount.load(), 16); // 16: max instances supported
    if (videoEnc->CreateVideoEncMockByName(vencName)) {
        vencCount++;
        cout << "create successed, num:" << vencCount.load() << endl;
    } else {
        cout << "create failed, num:" << vencCount.load() << endl;
        return;
    }
    sleep(1);
    videoEnc->Release();
    vencCount--;
}
} // namespace

void VideoEncUnitTest::SetUpTestCase(void) {}

void VideoEncUnitTest::TearDownTestCase(void) {}

void VideoEncUnitTest::SetUp(void)
{
    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    vencCallback_ = std::make_shared<VEncCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencCallback_);

    videoEnc_ = std::make_shared<VideoEncSample>(vencSignal);
    ASSERT_NE(nullptr, videoEnc_);

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), true,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
    vencName = capability->GetName();
}

void VideoEncUnitTest::TearDown(void)
{
    if (format_ != nullptr) {
        format_->Destroy();
    }
}

bool VideoEncUnitTest::CreateVideoCodecByMime(const std::string &encMime)
{
    if (videoEnc_->CreateVideoEncMockByMime(encMime) == false || videoEnc_->SetCallback(vencCallback_) != AV_ERR_OK) {
        return false;
    }
    return true;
}

bool VideoEncUnitTest::CreateVideoCodecByName(const std::string &encName)
{
    if (videoEnc_->CreateVideoEncMockByName(encName) == false || videoEnc_->SetCallback(vencCallback_) != AV_ERR_OK) {
        return false;
    }
    return true;
}

/**
 * @tc.name: videoEncoder_multithread_create_001
 * @tc.desc: try create 100 instances
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_multithread_create_001, TestSize.Level1)
{
    SET_THREAD_NUM(100);
    vencCount = 0;
    GTEST_RUN_TASK(MultiThreadCreateVEnc);
    cout << "remaining num: " << vencCount.load() << endl;
}

/**
 * @tc.name: videoEncoder_createWithNull_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_createWithNull_001, TestSize.Level1)
{
    ASSERT_FALSE(CreateVideoCodecByName(""));
}

/**
 * @tc.name: videoEncoder_createWithNull_002
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_createWithNull_002, TestSize.Level1)
{
    ASSERT_FALSE(CreateVideoCodecByMime(""));
}

/**
 * @tc.name: videoEncoder_create_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_create_001, TestSize.Level1)
{
    ASSERT_TRUE(CreateVideoCodecByName(vencName));
}

/**
 * @tc.name: videoEncoder_create_002
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_create_002, TestSize.Level1)
{
    ASSERT_TRUE(CreateVideoCodecByMime((CodecMimeType::VIDEO_AVC).data()));
}
