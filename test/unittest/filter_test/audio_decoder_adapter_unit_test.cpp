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

#include "audio_decoder_adapter_unit_test.h"
#include "audio_decoder_adapter.h"
#include "common/log.h"
#include "media_codec.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace Media {
namespace Pipeline {
void AudioDecoderAdapterUnitTest::SetUpTestCase(void) {}

void AudioDecoderAdapterUnitTest::TearDownTestCase(void) {}

void AudioDecoderAdapterUnitTest::SetUp(void)
{
    audioDecoderAdapter_  = std::make_shared<AudioDecoderAdapter>();
}

void AudioDecoderAdapterUnitTest::TearDown(void)
{
    audioDecoderAdapter_ = nullptr;
}

/**
 * @tc.name: AudioDecoderAdapterUnitTest_Init
 * @tc.desc: INIT, isMimeType true/false
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderAdapterUnitTest, AudioDecoderAdapterUnitTest_Init, TestSize.Level1)
{
    Status ret = audioDecoderAdapter_->Init(true, "audio/mpeg");
    EXPECT_EQ(ret, Status::OK);
    ret = audioDecoderAdapter_->Init(false, "audio/mpeg");
    EXPECT_EQ(ret, Status::ERROR_INVALID_STATE);
}

/**
 * @tc.name: AudioDecoderAdapterUnitTest_Start
 * @tc.desc: Start, isRunning_ true/false
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderAdapterUnitTest, AudioDecoderAdapterUnitTest_Start, TestSize.Level1)
{
    std::shared_ptr<MockAVCodecAudioCodec> mockaudiocodec_ = std::make_shared<MockAVCodecAudioCodec>();
    audioDecoderAdapter_->audiocodec_ = mockaudiocodec_;
    audioDecoderAdapter_->OnDumpInfo(-1);
    audioDecoderAdapter_->isRunning_ = true;
    Status ret = audioDecoderAdapter_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_CALL(*mockaudiocodec_, Start()).WillRepeatedly(Return((int32_t)Status::OK));
    audioDecoderAdapter_->isRunning_ = false;
    ret = audioDecoderAdapter_->Start();
    EXPECT_EQ(ret, Status::OK);
}

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS