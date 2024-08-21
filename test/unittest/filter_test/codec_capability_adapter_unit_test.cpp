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

#include "codec_capability_adapter_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void CodecCapabilityAdapterUnitTest::SetUpTestCase(void) {}

void CodecCapabilityAdapterUnitTest::TearDownTestCase(void) {}

void CodecCapabilityAdapterUnitTest::SetUp(void)
{
    codecCapabilityAdapter_ =
        std::make_shared<CodecCapabilityAdapter>();
}

void CodecCapabilityAdapterUnitTest::TearDown(void)
{
    codecCapabilityAdapter_ = nullptr;
}

/**
 * @tc.name: CodecCapabilityAdapter_IsWatermarkSupported_0100
 * @tc.desc: CodecCapabilityAdapter_IsWatermarkSupported_0100
 * @tc.type: FUNC
 */
HWTEST_F(CodecCapabilityAdapterUnitTest, CodecCapabilityAdapter_IsWatermarkSupported_0100, TestSize.Level1)
{
    ASSERT_NE(codecCapabilityAdapter_, nullptr);
    codecCapabilityAdapter_->Init();
    std::string codecMimeType = std::string("audio/test");
    bool isWatermarkSupported = true;
    EXPECT_EQ(codecCapabilityAdapter_->IsWatermarkSupported(codecMimeType, isWatermarkSupported),
        Status::ERROR_UNKNOWN);
    codecMimeType = std::string(MediaAVCodec::CodecMimeType::VIDEO_AVC);
    isWatermarkSupported = true;
    EXPECT_EQ(codecCapabilityAdapter_->IsWatermarkSupported(codecMimeType, isWatermarkSupported), Status::OK);
    isWatermarkSupported = false;
    EXPECT_EQ(codecCapabilityAdapter_->IsWatermarkSupported(codecMimeType, isWatermarkSupported), Status::OK);
}

/**
 * @tc.name: CodecCapabilityAdapter_GetAvailableEncoder_0100
 * @tc.desc: CodecCapabilityAdapter_GetAvailableEncoder_0100
 * @tc.type: FUNC
 */
HWTEST_F(CodecCapabilityAdapterUnitTest, CodecCapabilityAdapter_GetAvailableEncoder_0100, TestSize.Level1)
{
    ASSERT_NE(codecCapabilityAdapter_, nullptr);
    codecCapabilityAdapter_->Init();
    std::vector<MediaAVCodec::CapabilityData*> encoderCapData;
    EXPECT_EQ(codecCapabilityAdapter_->GetAvailableEncoder(encoderCapData), Status::OK);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS