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
 
#ifndef AUDIO_SINK_UNIT_TEST_H
#define AUDIO_SINK_UNIT_TEST_H
 
#include <gmock/gmock.h>
#include "mock/avbuffer.h"

#include "gtest/gtest.h"
#include "audio_sink.h"
#include "audio_effect.h"
#include "filter/filter.h"
#include "common/log.h"
#include "sink/media_synchronous_sink.h"
#include "buffer/avsharedmemory.h"
#include "buffer/avsharedmemorybase.h"
#include "buffer/avbuffer_queue_consumer.h"

namespace OHOS {
namespace Media {
namespace Test {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_ONLY_PRERELEASE, LOG_DOMAIN_SYSTEM_PLAYER, "AudioSinkUnitTest" };
constexpr const int32_t SLEEP_TIME = 60;
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
};

class TestAudioSinkUnitMock : public AudioSinkPlugin {
public:

    explicit TestAudioSinkUnitMock(std::string name): AudioSinkPlugin(std::move(name)) {}

    Status Start() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
        return Status::ERROR_UNKNOWN;
    };

    Status Stop() override
    {
        return Status::ERROR_UNKNOWN;
    };

    Status PauseTransitent() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
        return Status::ERROR_UNKNOWN;
    };

    Status Pause() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
        return Status::ERROR_UNKNOWN;
    };

    Status Resume() override
    {
        return Status::ERROR_UNKNOWN;
    };

    Status GetLatency(uint64_t &hstTime) override
    {
        (void)hstTime;
        return Status::ERROR_UNKNOWN;
    };

    Status SetAudioEffectMode(int32_t effectMode) override
    {
        (void)effectMode;
        return Status::ERROR_UNKNOWN;
    };

    Status GetAudioEffectMode(int32_t &effectMode) override
    {
        (void)effectMode;
        return Status::ERROR_UNKNOWN;
    };

    int64_t GetPlayedOutDurationUs(int64_t nowUs) override
    {
        (void)nowUs;
        return 0;
    }
    Status GetMute(bool& mute) override
    {
        (void)mute;
        return Status::ERROR_UNKNOWN;
    }
    Status SetMute(bool mute) override
    {
        (void)mute;
        return Status::ERROR_UNKNOWN;
    }

    Status GetVolume(float& volume) override
    {
        (void)volume;
        return Status::ERROR_UNKNOWN;
    }
    Status SetVolume(float volume) override
    {
        (void)volume;
        return Status::ERROR_UNKNOWN;
    }
    Status SetVolumeMode(int32_t mode) override
    {
        (void)mode;
        return Status::ERROR_UNKNOWN;
    }
    Status GetSpeed(float& speed) override
    {
        (void)speed;
        return Status::ERROR_UNKNOWN;
    }
    Status SetSpeed(float speed) override
    {
        (void)speed;
        return Status::ERROR_UNKNOWN;
    }
    Status GetFrameSize(size_t& size) override
    {
        (void)size;
        return Status::ERROR_UNKNOWN;
    }
    Status GetFrameCount(uint32_t& count) override
    {
        (void)count;
        return Status::ERROR_UNKNOWN;
    }
    Status Write(const std::shared_ptr<AVBuffer>& input) override
    {
        (void)input;
        return Status::ERROR_UNKNOWN;
    }
    Status Flush() override
    {
        return Status::ERROR_UNKNOWN;
    }
    Status Drain() override
    {
        return Status::ERROR_UNKNOWN;
    }
    Status GetFramePosition(int32_t &framePosition) override
    {
        (void)framePosition;
        return Status::ERROR_UNKNOWN;
    }
    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver) override
    {
        (void)receiver;
    }
    int32_t SetVolumeWithRamp(float targetVolume, int32_t duration) override
    {
        (void)targetVolume;
        (void)duration;
        return 0;
    }
    Status SetMuted(bool isMuted) override
    {
        return Status::OK;
    }
    Status SetRequestDataCallback(const std::shared_ptr<AudioSinkDataCallback> &callback) override
    {
        (void)callback;
        return Status::OK;
    }
    bool GetAudioPosition(timespec &time, uint32_t &framePosition) override
    {
        (void)time;
        (void)framePosition;
        return true;
    }
    Status GetBufferDesc(AudioStandard::BufferDesc &bufDesc) override
    {
        (void)bufDesc;
        return Status::OK;
    }
    Status EnqueueBufferDesc(const AudioStandard::BufferDesc &bufDesc) override
    {
        (void)bufDesc;
        return Status::OK;
    }
    bool IsFormatSupported(const std::shared_ptr<Meta> &meta)
    {
        (void)meta;
        return true;
    }
    Status SetAudioHapticsSyncId(int32_t syncId) override
    {
        (void)syncId;
        return Status::OK;
    }
};
} // namespace Test
} // namespace Media
} // namespace OHOS

#endif // AUDIO_SINK_UNIT_TEST_H