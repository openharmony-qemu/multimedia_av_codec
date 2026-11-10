/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "audio_server_sink_plugin_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
using namespace std;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::AudioStandard;

/**
 * @tc.name  : Test OnPreferredOutputDeviceUpdated
 * @tc.number: OnPreferredOutputDeviceUpdated_001
 * @tc.desc  : Test null event receiver branch and event dispatch branch
 */
HWTEST_F(AudioServerSinkPluginUnitTest, OnPreferredOutputDeviceUpdated_001, TestSize.Level0)
{
    // null event receiver: no event is dispatched, no crash
    AudioServerSinkPlugin::AudioPreferredOutputDeviceChangeCallbackImpl nullCallback(nullptr);
    std::vector<std::shared_ptr<AudioStandard::AudioDeviceDescriptor>> desc;
    nullCallback.OnPreferredOutputDeviceUpdated(desc);

    // valid event receiver: EVENT_AUDIO_PREFERRED_OUTPUT_DEVICE_CHANGE is dispatched to HiPlayer
    auto eventReceiver = std::make_shared<MockEventReceiver>();
    AudioServerSinkPlugin::AudioPreferredOutputDeviceChangeCallbackImpl callback(eventReceiver);
    Event capturedEvent;
    EXPECT_CALL(*eventReceiver, OnEvent(_))
        .WillOnce(Invoke([&capturedEvent](const Event& event) { capturedEvent = event; }));
    callback.OnPreferredOutputDeviceUpdated(desc);
    EXPECT_EQ(capturedEvent.type, EventType::EVENT_AUDIO_PREFERRED_OUTPUT_DEVICE_CHANGE);
    EXPECT_EQ(capturedEvent.srcFilter, std::string("AudioSink"));
}

/**
 * @tc.name  : Test RegisterPreferredOutputDeviceCallback
 * @tc.number: RegisterPreferredOutputDeviceCallback_001
 * @tc.desc  : Test already registered skip branch and fresh registration branch
 */
HWTEST_F(AudioServerSinkPluginUnitTest, RegisterPreferredOutputDeviceCallback_001, TestSize.Level0)
{
    ASSERT_NE(plugin_, nullptr);
    // already registered: the existing callback is kept
    auto registeredCb = std::make_shared<AudioServerSinkPlugin::AudioPreferredOutputDeviceChangeCallbackImpl>(nullptr);
    plugin_->preferredOutputDeviceCb_ = registeredCb;
    plugin_->RegisterPreferredOutputDeviceCallback();
    EXPECT_EQ(plugin_->preferredOutputDeviceCb_, registeredCb);

    // not registered: registration creates the callback
    plugin_->preferredOutputDeviceCb_ = nullptr;
    plugin_->RegisterPreferredOutputDeviceCallback();
    EXPECT_NE(plugin_->preferredOutputDeviceCb_, nullptr);
}

/**
 * @tc.name  : Test UnregisterPreferredOutputDeviceCallback
 * @tc.number: UnregisterPreferredOutputDeviceCallback_001
 * @tc.desc  : Test not registered early return branch and unregister reset branch
 */
HWTEST_F(AudioServerSinkPluginUnitTest, UnregisterPreferredOutputDeviceCallback_001, TestSize.Level0)
{
    ASSERT_NE(plugin_, nullptr);
    // not registered: early return, no crash
    plugin_->preferredOutputDeviceCb_ = nullptr;
    plugin_->UnregisterPreferredOutputDeviceCallback();
    EXPECT_EQ(plugin_->preferredOutputDeviceCb_, nullptr);
    // registered: unregister resets the callback so that re-registering creates a fresh one
    plugin_->RegisterPreferredOutputDeviceCallback();
    EXPECT_NE(plugin_->preferredOutputDeviceCb_, nullptr);
    auto firstCallback = plugin_->preferredOutputDeviceCb_;
    plugin_->UnregisterPreferredOutputDeviceCallback();
    plugin_->RegisterPreferredOutputDeviceCallback();
    EXPECT_NE(plugin_->preferredOutputDeviceCb_, firstCallback);
}

/**
 * @tc.name  : Test ReleaseRender
 * @tc.number: ReleaseRender_UnregistersPreferredOutputDeviceCallback_001
 * @tc.desc  : Test ReleaseRender unregisters the preferred output device callback
 */
HWTEST_F(AudioServerSinkPluginUnitTest, ReleaseRender_UnregistersPreferredOutputDeviceCallback_001, TestSize.Level0)
{
    ASSERT_NE(plugin_, nullptr);
    plugin_->RegisterPreferredOutputDeviceCallback();
    EXPECT_NE(plugin_->preferredOutputDeviceCb_, nullptr);
    auto registeredCallback = plugin_->preferredOutputDeviceCb_;
    plugin_->ReleaseRender();  // must unregister the preferred output device callback
    plugin_->RegisterPreferredOutputDeviceCallback();
    EXPECT_NE(plugin_->preferredOutputDeviceCb_, registeredCallback);
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS
