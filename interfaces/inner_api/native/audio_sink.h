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

namespace OHOS {
namespace Media {
using namespace OHOS::Media::Plugins;

class AudioSink : public std::enable_shared_from_this<AudioSink>, public Pipeline::MediaSynchronousSink {
public:
    AudioSink();
    AudioSink(bool isRenderCallbackMode, bool isProcessInputMerged);
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
    Status Freeze();
    Status UnFreeze();
    Status Resume();
    Status Flush();
    Status Release();
    Status SetPlayRange(int64_t start, int64_t end);
    Status SetVolume(float volume);
    Status SetVolumeMode(int32_t mode);
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
    Status HandleFormatChange(std::shared_ptr<Meta>& meta, const std::shared_ptr<Pipeline::EventReceiver>& receiver);
    Status SetMuted(bool isMuted);
    virtual void OnInterrupted(bool isInterruptNeeded) override;

    float GetMaxAmplitude();
    int32_t SetMaxAmplitudeCbStatus(bool status);
    Status SetPerfRecEnabled(bool isPerfRecEnabled);

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
    void SetIsInPrePausing(bool isInPrePausing);
    inline void SetIsAudioDemuxDecodeAsync(bool isAudioDemuxDecodeAsync)
    {
        isAudioDemuxDecodeAsync_ = isAudioDemuxDecodeAsync;
    }
    bool GetSyncCenterClockTime(int64_t &clockTime);
    Status SetIsCalledBySystemApp(bool isCalledBySystemApp);
    Status SetLooping(bool loop);
    bool IsInputBufferDataEnough(int32_t size, bool isAudioVivid);
    bool CopyDataToBufferDesc(size_t size, bool isAudioVivid, AudioStandard::BufferDesc &bufferDesc);
    Status GetBufferDesc(AudioStandard::BufferDesc &bufferDesc);
    Status EnqueueBufferDesc(const AudioStandard::BufferDesc &bufferDesc);
    void SyncWriteByRenderInfo();
    void UpdateRenderInfo();
    void UpdateAmplitude();
    bool IsTimeAnchorNeedUpdate();
    bool IsBufferAvailable(std::shared_ptr<AVBuffer> &buffer, size_t &cacheBufferSize);
    bool IsBufferDataDrained(AudioStandard::BufferDesc &bufferDesc, std::shared_ptr<AVBuffer> &buffer,
        size_t &size, size_t &cacheBufferSize, bool isAudioVivid, int64_t &bufferPts);
    void ReleaseCacheBuffer(bool isSwapBuffer = false);
    int64_t CalculateBufferDuration(int64_t writeDataSize);
    void WriteDataToRender(std::shared_ptr<AVBuffer> &filledOutputBuffer);
    void ResetInfo();
    bool IsEosBuffer(std::shared_ptr<AVBuffer> &filledOutputBuffer);
    void HandleEosBuffer(std::shared_ptr<AVBuffer> &filledOutputBuffer);
    bool HandleAudioRenderRequest(size_t size, bool isAudioVivid, AudioStandard::BufferDesc &bufferDesc);
    void HandleAudioRenderRequestPost();
    Status SetAudioHapticsSyncId(int32_t syncId);

protected:
    std::atomic<OHOS::Media::Pipeline::FilterState> state_;
private:
    Status PrepareInputBufferQueue();
    std::shared_ptr<Plugins::AudioSinkPlugin> CreatePlugin();
    bool OnNewAudioMediaTime(int64_t mediaTimeUs);
    int64_t getPendingAudioPlayoutDurationUs(int64_t nowUs);
    int64_t getDurationUsPlayedAtSampleRate(uint32_t numFrames);
    void UpdateAudioWriteTimeMayWait();
    bool UpdateTimeAnchorIfNeeded(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer);
    void DrainAndReportEosEvent();
    void HandleEosInner(bool drain);
    void CalcMaxAmplitude(std::shared_ptr<AVBuffer> filledOutputBuffer);
    void CheckUpdateState(char *frame, uint64_t replyBytes, int32_t format);
    bool DropApeBuffer(std::shared_ptr<AVBuffer> filledOutputBuffer);
    int64_t CalcBufferDuration(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer);
    void PerfRecord(int64_t audioWriteMs);
    void ClearInputBuffer();
    int32_t GetSampleFormatBytes();
    bool CopyBufferData(AudioStandard::BufferDesc &bufferDesc, std::shared_ptr<AVBuffer> &buffer,
        size_t &size, size_t &cacheBufferSize, int64_t &bufferPts);
    bool CopyAudioVividBufferData(AudioStandard::BufferDesc &bufferDesc, std::shared_ptr<AVBuffer> &buffer,
        size_t &size, size_t &cacheBufferSize, int64_t &bufferPts);
    Status InitAudioSinkPlugin(std::shared_ptr<Meta>& meta, const std::shared_ptr<Pipeline::EventReceiver>& receiver);
    Status InitAudioSinkInfo(std::shared_ptr<Meta>& meta);
    Status SetAudioSinkPluginParameters();
    void GetAvailableOutputBuffers();
    void ClearAvailableOutputBuffers();
    void DriveBufferCircle();
    void WaitForAllBufferConsumed();
    std::shared_ptr<AVBuffer> CopyBuffer(const std::shared_ptr<AVBuffer> buffer);
    Status MuteAudioBuffer(size_t size, AudioStandard::BufferDesc &bufferDesc, bool isEos);

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

    class AudioSinkDataCallbackImpl : public AudioSinkDataCallback {
    public:
        explicit AudioSinkDataCallbackImpl(std::shared_ptr<AudioSink> sink);
        void OnWriteData(int32_t size, bool isAudioVivid) override;
    private:
        std::weak_ptr<AudioSink> audioSink_;
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
    UnderrunDetector underrunDetector_;
    AudioLagDetector lagDetector_;
    std::atomic<int64_t> seekTimeUs_ {HST_TIME_NONE};
    PerfRecorder perfRecorder_ {};
    bool isPerfRecEnabled_ { false };
    bool isCalledBySystemApp_ { false };
    bool isLoop_ { false };
    bool isRenderCallbackMode_ {true};
    bool isProcessInputMerged_ {true};
    bool isAudioDemuxDecodeAsync_ {true};
    std::atomic<bool> isInPrePausing_ {false};
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
    class AudioDataSynchroizer {
        public:
            void UpdateCurrentBufferInfo(int64_t bufferPts, int64_t bufferDuration);
            int64_t GetLastReportedClockTime() const;
            int64_t GetLastBufferPTS() const;
            int64_t GetBufferDuration() const;
            int64_t CalculateAudioLatency();
            void UpdateReportTime(int64_t nowClockTime);
            void UpdateLastBufferPTS(int64_t bufferOffset, float speed);
            void OnRenderPositionUpdated(int64_t currentRenderPTS, int64_t currentRenderClockTime);
            void Reset();
        private:
            int64_t lastBufferPTS_ {HST_TIME_NONE};
            int64_t startPTS_ {HST_TIME_NONE};
            int64_t curBufferPTS_ {HST_TIME_NONE};
            int64_t bufferDuration_ {0};
            int64_t currentRenderClockTime_ {0};
            int64_t currentRenderPTS_ {0};
            int64_t lastReportedClockTime_ {HST_TIME_NONE};
            int64_t lastBufferOffset_ {0};
            int64_t compensateDuration_ {0};
            int64_t sumDuration_ {0};
    };
    std::unique_ptr<AudioDataSynchroizer> innerSynchroizer_ = std::make_unique<AudioDataSynchroizer>();
    MemoryType bufferMemoryType_ {MemoryType::UNKNOWN_MEMORY};
    int32_t maxCbDataSize_ {0};
    std::queue<std::shared_ptr<AVBuffer>> swapOutputBuffers_ {};
};
}
}

#endif // HISTREAMER_AUDIO_SINK_H
