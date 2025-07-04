/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

namespace OHOS {
namespace Media {
using namespace OHOS::Media::Plugins;
class AudioSink : public std::enable_shared_from_this<AudioSink>, public Pipeline::MediaSynchronousSink {
public:
    AudioSink();
    ~AudioSink();
    Status Init(std::shared_ptr<Meta>& meta, const std::shared_ptr<Pipeline::EventReceiver>& receiver);
    sptr<AVBufferQueueProducer> GetBufferQueueProducer();
    sptr<AVBufferQueueConsumer> GetBufferQueueConsumer();
    Status SetParameter(const std::shared_ptr<Meta>& meta);
    Status GetParameter(std::shared_ptr<Meta>& meta);
    Status Prepare();
    Status Start();
    Status Stop();
    Status Pause();
    Status Resume();
    Status Flush();
    Status Release();
    Status SetPlayRange(int64_t start, int64_t end);
    Status SetVolume(float volume);
    void DrainOutputBuffer(bool flushed);
    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver);
    Status GetLatency(uint64_t& nanoSec);
    void SetSyncCenter(std::shared_ptr<Pipeline::MediaSyncManager> syncCenter);
    int64_t DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer) override;
    void ResetSyncInfo() override;
    Status SetSpeed(float speed);
    Status SetAudioEffectMode(int32_t effectMode);
    Status GetAudioEffectMode(int32_t &effectMode);
    int32_t SetVolumeWithRamp(float targetVolume, int32_t duration);
    void SetThreadGroupId(const std::string& groupId);
    Status SetIsTransitent(bool isTransitent);
    Status ChangeTrack(std::shared_ptr<Meta>& meta, const std::shared_ptr<Pipeline::EventReceiver>& receiver);
    Status SetMuted(bool isMuted);
    Status SetSeekTime(int64_t seekTime);
    bool NeedImmediateRender();
    bool GetSyncCenterClockTime(int64_t &clockTime);
    Status SetIsCalledBySystemApp(bool isCalledBySystemApp);
    Status SetLooping(bool loop);

    float GetMaxAmplitude();
    int32_t SetMaxAmplitudeCbStatus(bool status);
    Status SetPerfRecEnabled(bool isPerfRecEnabled);

    static const int64_t kMinAudioClockUpdatePeriodUs = 20 * HST_USECOND;

    static const int64_t kMaxAllowedAudioSinkDelayUs = 1500 * HST_MSECOND;
protected:
    std::atomic<OHOS::Media::Pipeline::FilterState> state_;
private:
    Status PrepareInputBufferQueue();
    std::shared_ptr<Plugins::AudioSinkPlugin> CreatePlugin();
    int64_t getPendingAudioPlayoutDurationUs(int64_t nowUs);
    int64_t getDurationUsPlayedAtSampleRate(uint32_t numFrames);
    void UpdateAudioWriteTimeMayWait();
    bool UpdateTimeAnchorIfNeeded(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer);
    void DrainAndReportEosEvent();
    void HandleEosInner(bool drain);
    bool DropApeBuffer(std::shared_ptr<AVBuffer> filledOutputBuffer);
    void CalcMaxAmplitude(std::shared_ptr<AVBuffer> filledOutputBuffer);
    void CheckUpdateState(char *frame, uint64_t replyBytes, int32_t format);
    int64_t CalcBufferDuration(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer);
    void PerfRecord(int64_t audioWriteMs);

    class UnderrunDetector {
    public:
        void DetectAudioUnderrun(int64_t clkTime, int64_t latency);
        void SetEventReceiver(std::weak_ptr<Pipeline::EventReceiver> eventReceiver);
        void UpdateBufferTimeNoLock(int64_t clkTime, int64_t latency);
        void SetLastAudioBufferDuration(int64_t durationUs);
        void Reset();
    private:
        std::weak_ptr<Pipeline::EventReceiver> eventReceiver_;
        Mutex mutex_ {};
        int64_t lastClkTime_ {HST_TIME_NONE};
        int64_t lastLatency_ {HST_TIME_NONE};
        int64_t lastBufferDuration_ {HST_TIME_NONE};
    };

    class AudioLagDetector : public Pipeline::LagDetector {
    public:
        void Reset() override;

        bool CalcLag(std::shared_ptr<AVBuffer> buffer) override;

        void SetLatency(int64_t latency)
        {
            latency_ = latency;
        }

        struct AudioDrainTimeGroup {
            int64_t lastAnchorPts = 0;
            int64_t anchorDuration = 0;
            int64_t writeDuration = 0;
            int64_t nowClockTime = 0;

            AudioDrainTimeGroup() = default;
            AudioDrainTimeGroup(int64_t anchorPts, int64_t duration, int64_t writeDuration, int64_t clockTime)
                : lastAnchorPts(anchorPts),
                  anchorDuration(duration),
                  writeDuration(writeDuration),
                  nowClockTime(clockTime) {}
        };

        void UpdateDrainTimeGroup(AudioDrainTimeGroup group);
    private:
        int64_t latency_ = 0;
        int64_t lastDrainTimeMs_ = 0;
        AudioDrainTimeGroup lastDrainTimeGroup_ {};
    };

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
    bool isApe_ {false};
    bool isFlac_ {false};
    int64_t playRangeStartTime_ = -1;
    int64_t playRangeEndTime_ = -1;
    // vars for audio progress optimization
    int64_t playingBufferDurationUs_ {0};
    int64_t lastBufferWriteTime_ {0};
    bool lastBufferWriteSuccess_ {true};
    float speed_ {1.0f};
    std::mutex pluginMutex_;
    float volume_ {-1.0f};
    int32_t effectMode_ {-1};
    bool isMuted_ = false;
    Mutex amplitudeMutex_ {};
    float maxAmplitude_ = 0;

    bool calMaxAmplitudeCbStatus_ = false;
    UnderrunDetector underrunDetector_;
    AudioLagDetector lagDetector_;
    std::atomic<int64_t> seekTimeUs_ {HST_TIME_NONE};
    PerfRecorder perfRecorder_ {};
    bool isPerfRecEnabled_ { false };
    bool isCalledBySystemApp_ { false };
    bool isLoop_ { false };
};
}
}

#endif // HISTREAMER_AUDIO_SINK_H
