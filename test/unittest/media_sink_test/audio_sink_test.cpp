/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "audio_sink.h"
#include "audio_effect.h"
#include "filter/filter.h"
#include "common/log.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Test {

class TestEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TestEventReceiver()
    {
        MEDIA_LOG_I("TestEventReceiver ctor ");
    }

    void OnEvent(const Event &event)
    {
        MEDIA_LOG_I("TestEventReceiver OnEvent " PUBLIC_LOG_S, event.srcFilter.c_str());
    }

private:
};

std::shared_ptr<AudioSink> AudioSinkCreate()
{
    auto audioSink = std::make_shared<AudioSink>();
    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    auto meta = std::make_shared<Meta>();
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    if (initStatus == Status::OK) {
        return audioSink;
    } else {
        return nullptr;
    }
}

HWTEST(TestAudioSink, find_audio_sink_process, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    auto preStatus = audioSink->Prepare();
    ASSERT_TRUE(preStatus == Status::OK);
    auto startStatus = audioSink->Start();
    ASSERT_TRUE(startStatus == Status::OK);
    auto pauseStatus = audioSink->Pause();
    ASSERT_TRUE(pauseStatus == Status::OK);
    auto stopStatus = audioSink->Stop();
    ASSERT_TRUE(stopStatus == Status::OK);
    auto flushStatus = audioSink->Flush();
    ASSERT_TRUE(flushStatus == Status::OK);
    auto resumeStatus = audioSink->Resume();
    ASSERT_TRUE(resumeStatus == Status::OK);
    auto freeStatus = audioSink->Release();
    ASSERT_TRUE(freeStatus == Status::OK);
}

HWTEST(TestAudioSink, find_audio_sink_set_volume, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    float volume = 0.5f;
    auto setVolumeStatus =  audioSink->SetVolume(volume);
    ASSERT_TRUE(setVolumeStatus == Status::OK);
}

HWTEST(TestAudioSink, find_audio_sink_set_speed, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    float speed = 1.0f;
    auto setVolumeStatus =  audioSink->SetSpeed(speed);
    ASSERT_TRUE(setVolumeStatus == Status::OK);
}

HWTEST(TestAudioSink, find_audio_sink_audio_effect, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkCreate();
    ASSERT_TRUE(audioSink != nullptr);
    auto setEffectStatus =  audioSink->SetAudioEffectMode(AudioStandard::EFFECT_NONE);
    ASSERT_TRUE(setEffectStatus == Status::OK);
    int32_t audioEffectMode;
    auto getEffectStatus =  audioSink->GetAudioEffectMode(audioEffectMode);
    ASSERT_TRUE(getEffectStatus == Status::OK);
}

} // namespace Test
} // namespace Media
} // namespace OHOS