/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "audio_server_sink_plugin_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
using namespace std;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::AudioStandard;

static const int32_t NUM_1 = 1;
static const float FLOAT_0 = 0;

void AudioServerSinkPluginUnitTest::SetUpTestCase(void) {}

void AudioServerSinkPluginUnitTest::TearDownTestCase(void) {}

void AudioServerSinkPluginUnitTest::SetUp(void)
{
    plugin_ = std::make_shared<AudioServerSinkPlugin>("testAudioServerSinkPlugin");
}

void AudioServerSinkPluginUnitTest::TearDown(void)
{
    plugin_ = nullptr;
}

/**
 * @tc.name  : Test OnStateChange
 * @tc.number: OnStateChange_001
 * @tc.desc  : Test cmdType == AudioStandard::StateChangeCmdType::CMD_FROM_SYSTEM
 */
HWTEST_F(AudioServerSinkPluginUnitTest, OnStateChange_001, TestSize.Level0)
{
    auto eventReceiver = std::make_shared<MockEventReceiver>();
    AudioServerSinkPlugin::AudioRendererCallbackImpl impl(eventReceiver, true);
    auto state = OHOS::AudioStandard::RendererState::RENDERER_NEW;
    auto cmdType = OHOS::AudioStandard::StateChangeCmdType::CMD_FROM_SYSTEM;
    EXPECT_CALL(*eventReceiver, OnEvent(_)).WillOnce(Return());
    impl.OnStateChange(state, cmdType);
}

/**
 * @tc.name  : Test ChooseVolumeMode
 * @tc.number:ChooseVolumeMode_001
 * @tc.desc  : Test audioRenderInfo_.volumeMode ==
 *                  static_cast<int32_t>(AudioStandard::AudioVolumeMode::AUDIOSTREAM_VOLUMEMODE_APP_INDIVIDUAL)
 */
HWTEST_F(AudioServerSinkPluginUnitTest, ChooseVolumeMode_001, TestSize.Level0)
{
    ASSERT_NE(plugin_, nullptr);
    plugin_->volumeMode_ =
        static_cast<int32_t>(AudioStandard::AudioVolumeMode::AUDIOSTREAM_VOLUMEMODE_SYSTEM_GLOBAL);
    plugin_->audioRenderInfo_.volumeMode =
        static_cast<int32_t>(AudioStandard::AudioVolumeMode::AUDIOSTREAM_VOLUMEMODE_APP_INDIVIDUAL);
    auto ret = plugin_->ChooseVolumeMode();
    EXPECT_EQ(ret, NUM_1);
}

/**
 * @tc.name  : Test ReleaseFile
 * @tc.number: ReleaseFile_001
 * @tc.desc  : Test entireDumpFile_ != nullptr
 *             Test sliceDumpFile_ != nullptr
 */
HWTEST_F(AudioServerSinkPluginUnitTest, ReleaseFile_001, TestSize.Level0)
{
    ASSERT_NE(plugin_, nullptr);
    plugin_->entireDumpFile_ = fopen("test_1.txt", "w");
    ASSERT_NE(plugin_->entireDumpFile_, nullptr);
    plugin_->sliceDumpFile_ = fopen("test_2.txt", "w");
    ASSERT_NE(plugin_->sliceDumpFile_, nullptr);
    plugin_->ReleaseFile();
    EXPECT_EQ(plugin_->entireDumpFile_, nullptr);
    EXPECT_EQ(plugin_->sliceDumpFile_, nullptr);
}

/**
 * @tc.name  : Test SetVolumeWithRamp
 * @tc.number: SetVolumeWithRamp_001
 * @tc.desc  : Test duration != 0
 */
HWTEST_F(AudioServerSinkPluginUnitTest, SetVolumeWithRamp_001, TestSize.Level0)
{
    ASSERT_NE(plugin_, nullptr);
    auto mockRenderer = new MockAudioRenderer();
    plugin_->audioRenderer_.reset(mockRenderer);
    EXPECT_CALL(*mockRenderer, SetVolumeWithRamp(_, _)).WillOnce(Return(1));
    auto ret = plugin_->SetVolumeWithRamp(FLOAT_0, NUM_1);
    EXPECT_EQ(ret, NUM_1);
}

/**
 * @tc.name  : Test SetUpAudioRenderSetFlagSetter
 * @tc.number: SetUpAudioRenderSetFlagSetter_001
 * @tc.desc  : Test Any::IsSameTypeWith<bool>(para) == false
 */
HWTEST_F(AudioServerSinkPluginUnitTest, SetUpAudioRenderSetFlagSetter_001, TestSize.Level0)
{
    ASSERT_NE(plugin_, nullptr);
    plugin_->SetUpAudioRenderSetFlagSetter();
    auto it = plugin_->paramsSetterMap_.find(Tag::AUDIO_RENDER_SET_FLAG);
    EXPECT_NE(it, plugin_->paramsSetterMap_.end());
    std::function<Status(const ValueType &para)> func = plugin_->paramsSetterMap_[Tag::AUDIO_RENDER_SET_FLAG];
    auto eventReceiver = std::make_shared<MockEventReceiver>();
    plugin_->playerEventReceiver_ = eventReceiver;
    EXPECT_CALL(*eventReceiver, OnEvent(_)).WillOnce(Return());
    const ValueType para = OHOS::Media::Any(NUM_1);
    auto ret = func(para);
    EXPECT_EQ(ret, Status::ERROR_MISMATCHED_TYPE);
}

} // namespace Plugins
} // namespace Media
} // namespace OHOS