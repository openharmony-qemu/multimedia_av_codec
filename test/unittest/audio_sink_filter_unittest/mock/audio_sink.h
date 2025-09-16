/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_AUDIO_SINK_H
#define HISTREAMER_AUDIO_SINK_H
#include <mutex>
#include "common/status.h"
#include "meta/meta.h"
#include "sink/media_synchronous_sink.h"
#include "media_sync_manager.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_define.h"
#include "plugin/audio_sink_plugin.h"
#include "filter/filter.h"
#include "plugin/plugin_time.h"
#include "performance_utils.h"
#include <queue>
#include "audio_errors.h"
#include "gmock/gmock.h"

namespace OHOS {
namespace Media {
using namespace OHOS::Media::Plugins;

class AudioSink : public std::enable_shared_from_this<AudioSink>, public Pipeline::MediaSynchronousSink {
public:
    AudioSink();
    AudioSink(bool isRenderCallbackMode, bool isProcessInputMerged);
    ~AudioSink();
    MOCK_METHOD(Status, Init, (std::shared_ptr<Meta>& meta,
        const std::shared_ptr<Pipeline::EventReceiver>& receiver), ());
    MOCK_METHOD(sptr<AVBufferQueueProducer>, GetBufferQueueProducer, (), ());
    MOCK_METHOD(sptr<AVBufferQueueConsumer>, GetBufferQueueConsumer, (), ());
    MOCK_METHOD(Status, SetParameter, (const std::shared_ptr<Meta>& meta), ());
    MOCK_METHOD(Status, GetParameter, (std::shared_ptr<Meta>& meta), ());
    MOCK_METHOD(Status, Prepare, (), ());
    MOCK_METHOD(Status, Start, (), ());
    MOCK_METHOD(Status, Stop, (), ());
    MOCK_METHOD(Status, Pause, (), ());
    MOCK_METHOD(Status, Freeze, (), ());
    MOCK_METHOD(Status, UnFreeze, (), ());
    MOCK_METHOD(Status, Resume, (), ());
    MOCK_METHOD(Status, Flush, (), ());
    MOCK_METHOD(Status, Release, (), ());
    MOCK_METHOD(Status, SetPlayRange, (int64_t start, int64_t end), ());
    MOCK_METHOD(Status, SetVolume, (float volume), ());
    MOCK_METHOD(Status, SetVolumeMode, (int32_t mode), ());
    MOCK_METHOD(void, DrainOutputBuffer, (bool flushed), ());
    MOCK_METHOD(void, SetEventReceiver, (const std::shared_ptr<Pipeline::EventReceiver>& receiver), ());
    MOCK_METHOD(Status, GetLatency, (uint64_t& nanoSec), ());
    MOCK_METHOD(void, SetSyncCenter, (std::shared_ptr<Pipeline::MediaSyncManager> syncCenter), ());
    MOCK_METHOD(int64_t, DoSyncWrite, (const std::shared_ptr<OHOS::Media::AVBuffer>& buffer,
        int64_t& actionClock), (override));
    MOCK_METHOD(void, ResetSyncInfo, (), (override));
    MOCK_METHOD(Status, SetSpeed, (float speed), ());
    MOCK_METHOD(Status, SetAudioEffectMode, (int32_t effectMode), ());
    MOCK_METHOD(Status, GetAudioEffectMode, (int32_t &effectMode), ());
    MOCK_METHOD(int32_t, SetVolumeWithRamp, (float targetVolume, int32_t duration), ());
    MOCK_METHOD(void, SetThreadGroupId, (const std::string& groupId), ());
    MOCK_METHOD(Status, SetIsTransitent, (bool isTransitent), ());
    MOCK_METHOD(Status, ChangeTrack, (std::shared_ptr<Meta>& meta,
        const std::shared_ptr<Pipeline::EventReceiver>& receiver), ());
    MOCK_METHOD(Status, HandleFormatChange, (std::shared_ptr<Meta>& meta,
        const std::shared_ptr<Pipeline::EventReceiver>& receiver), ());
    MOCK_METHOD(Status, SetMuted, (bool isMuted), ());
    MOCK_METHOD(void, OnInterrupted, (bool isInterruptNeeded), (override));
    MOCK_METHOD(float, GetMaxAmplitude, (), ());
    MOCK_METHOD(int32_t, SetMaxAmplitudeCbStatus, (bool status), ());
    MOCK_METHOD(Status, SetPerfRecEnabled, (bool isPerfRecEnabled), ());

    static const int64_t kMinAudioClockUpdatePeriodUs = 20 * HST_USECOND;

    static const int64_t kMaxAllowedAudioSinkDelayUs = 1500 * HST_MSECOND;

    bool HasPlugin() const
    {
        return plugin_ != nullptr;
    }

    bool IsInitialized() const
    {
        return state_ == Pipeline::FilterState::INITIALIZED;
    }
    Status SetSeekTime(int64_t seekTime);
    inline bool NeedImmediateRender() const
    {
        return isApe_ || isFlac_;
    }
    MOCK_METHOD(bool, GetSyncCenterClockTime, (int64_t &clockTime), ());
    MOCK_METHOD(Status, SetIsCalledBySystemApp, (bool isCalledBySystemApp), ());
    MOCK_METHOD(Status, SetLooping, (bool loop), ());
    MOCK_METHOD(bool, IsInputBufferDataEnough, (int32_t size, bool isAudioVivid), ());
    MOCK_METHOD(bool, CopyDataToBufferDesc,
        (size_t size, bool isAudioVivid, AudioStandard::BufferDesc &bufferDesc), ());
    MOCK_METHOD(Status, GetBufferDesc, (AudioStandard::BufferDesc &bufferDesc), ());
    MOCK_METHOD(Status, EnqueueBufferDesc, (const AudioStandard::BufferDesc &bufferDesc), ());
    MOCK_METHOD(void, SyncWriteByRenderInfo, (), ());
    MOCK_METHOD(void, UpdateRenderInfo, (), ());
    MOCK_METHOD(void, UpdateAmplitude, (), ());
    MOCK_METHOD(bool, IsTimeAnchorNeedUpdate, (), ());
    MOCK_METHOD(bool, IsBufferAvailable, (std::shared_ptr<AVBuffer> &buffer, size_t &cacheBufferSize), ());
    MOCK_METHOD(bool, IsBufferDataDrained, (AudioStandard::BufferDesc &bufferDesc, std::shared_ptr<AVBuffer> &buffer,
        size_t &size, size_t &cacheBufferSize, bool isAudioVivid, int64_t &bufferPts), ());
    MOCK_METHOD(void, ReleaseCacheBuffer, (bool isSwapBuffer), ());
    MOCK_METHOD(int64_t, CalculateBufferDuration, (int64_t writeDataSize), ());
    MOCK_METHOD(void, WriteDataToRender, (std::shared_ptr<AVBuffer> &filledOutputBuffer), ());
    MOCK_METHOD(void, ResetInfo, (), ());
    MOCK_METHOD(bool, IsEosBuffer, (std::shared_ptr<AVBuffer> &filledOutputBuffer), ());
    MOCK_METHOD(void, HandleEosBuffer, (std::shared_ptr<AVBuffer> &filledOutputBuffer), ());
    MOCK_METHOD(bool, HandleAudioRenderRequest,
        (size_t size, bool isAudioVivid, AudioStandard::BufferDesc &bufferDesc), ());
    MOCK_METHOD(void, HandleAudioRenderRequestPost, (), ());
    MOCK_METHOD(Status, SetAudioHapticsSyncId, (int32_t syncId), ());

protected:
    std::atomic<OHOS::Media::Pipeline::FilterState> state_;
private:
    std::shared_ptr<Plugins::AudioSinkPlugin> plugin_ {};
    std::shared_ptr<Pipeline::EventReceiver> playerEventReceiver_;
    int32_t appUid_{0};
    int32_t appPid_{0};
    int64_t numFramesWritten_ {0};
    int64_t firstAudioAnchorTimeMediaUs_ {HST_TIME_NONE};
    int64_t nextAudioClockUpdateTimeUs_ {HST_TIME_NONE};
    int64_t lastAnchorClockTime_  {HST_TIME_NONE};
    int64_t latestBufferPts_ {HST_TIME_NONE};
    int64_t latestBufferDuration_ {0};
    int64_t bufferDurationSinceLastAnchor_ {0};
    std::atomic<bool> forceUpdateTimeAnchorNextTime_ {true};
    const std::string INPUT_BUFFER_QUEUE_NAME = "AudioSinkInputBufferQueue";
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    int64_t firstPts_ {HST_TIME_NONE};
    int32_t sampleRate_ {0};
    int32_t samplePerFrame_ {0};
    int32_t audioChannelCount_ = 0;
    int64_t fixDelay_ {0};
    bool isTransitent_ {false};
    bool isEos_ {false};
    std::mutex pluginMutex_;
    float volume_ {-1.0f};
    std::unique_ptr<Task> eosTask_ {nullptr};
    enum class EosInterruptState : int {
        NONE,
        INITIAL,
        PAUSE,
        RESUME,
        STOP,
    };
    Mutex eosMutex_ {};
    std::atomic<bool> eosDraining_ {false};
    std::atomic<EosInterruptState> eosInterruptType_ {EosInterruptState::NONE};
    float speed_ {1.0f};
    int32_t effectMode_ {-1};
    bool isApe_ {false};
    bool isFlac_ {false};
    int64_t playRangeStartTime_ = -1;
    int64_t playRangeEndTime_ = -1;
    // vars for audio progress optimization
    int64_t playingBufferDurationUs_ {0};
    int64_t lastBufferWriteTime_ {0};
    bool lastBufferWriteSuccess_ {true};
    bool isMuted_ = false;
    Mutex amplitudeMutex_ {};
    float maxAmplitude_ = 0;
    float currentMaxAmplitude_ {0};

    bool calMaxAmplitudeCbStatus_ = false;
    std::atomic<int64_t> seekTimeUs_ {HST_TIME_NONE};
    PerfRecorder perfRecorder_ {};
    bool isPerfRecEnabled_ { false };
    bool isCalledBySystemApp_ { false };
    bool isLoop_ { false };
    bool isRenderCallbackMode_ {true};
    bool isProcessInputMerged_ {true};
    std::shared_ptr<AudioSinkDataCallback> audioSinkDataCallback_ {nullptr};
    std::mutex availBufferMutex_;
    std::atomic<size_t> availDataSize_ {0};
    std::atomic<size_t> remainingDataSize_ {0};
    std::queue<std::shared_ptr<AVBuffer>> availOutputBuffers_;
    int32_t currentQueuedBufferOffset_ {0};
    bool isEosBuffer_ {false};
    std::mutex eosCbMutex_ {};
    bool hangeOnEosCb_ {false};
    std::condition_variable eosCbCond_ {};

    std::atomic<bool> formatChange_ {false};
    std::mutex formatChangeMutex_ {};
    std::condition_variable formatChangeCond_ {};
    MemoryType bufferMemoryType_ {MemoryType::UNKNOWN_MEMORY};
    int32_t maxCbDataSize_ {0};
    std::queue<std::shared_ptr<AVBuffer>> swapOutputBuffers_ {};
};
}
}

#endif // HISTREAMER_AUDIO_SINK_H
