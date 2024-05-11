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
#include "common/log.h"
#include "plugin/plugin_manager.h"
#include "plugin/plugin_time.h"

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
    Status SetVolume(float volume);
    void DrainOutputBuffer();
    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver);
    Status GetLatency(uint64_t& nanoSec);
    void SetSyncCenter(std::shared_ptr<Pipeline::MediaSyncManager> syncCenter);
    int64_t DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer) override;
    void ResetSyncInfo() override;
    Status SetSpeed(float speed);
    Status SetAudioEffectMode(int32_t effectMode);
    Status GetAudioEffectMode(int32_t &effectMode);
    int32_t SetVolumeWithRamp(float targetVolume, int32_t duration);
    Status SetIsTransitent(bool isTransitent);
    Status ChangeTrack(std::shared_ptr<Meta>& meta, const std::shared_ptr<Pipeline::EventReceiver>& receiver);

    static const int64_t kMinAudioClockUpdatePeriodUs = 20 * HST_USECOND;

    static const int64_t kMaxAllowedAudioSinkDelayUs = 1500 * HST_MSECOND;
protected:
    std::atomic<OHOS::Media::Pipeline::FilterState> state_;
private:
    Status PrepareInputBufferQueue();
    std::shared_ptr<Plugins::AudioSinkPlugin> CreatePlugin();
    bool OnNewAudioMediaTime(int64_t mediaTimeUs);
    int64_t getPendingAudioPlayoutDurationUs(int64_t nowUs);
    int64_t getDurationUsPlayedAtSampleRate(uint32_t numFrames);
    std::shared_ptr<Plugins::AudioSinkPlugin> plugin_ {};
    std::shared_ptr<Pipeline::EventReceiver> playerEventReceiver_;
    int32_t appUid_{0};
    int32_t appPid_{0};
    int64_t numFramesWritten_ {0};
    int64_t firstAudioAnchorTimeMediaUs_ {HST_TIME_NONE};
    int64_t nextAudioClockUpdateTimeUs_ {HST_TIME_NONE};
    int64_t lastReportedClockTime_ {HST_TIME_NONE};
    int64_t latestBufferPts_ {HST_TIME_NONE};
    int64_t latestBufferDuration_ {0};
    bool forceUpdateTimeAnchorNextTime_ {false};
    const std::string INPUT_BUFFER_QUEUE_NAME = "AudioSinkInputBufferQueue";
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    int64_t firstPts_ {HST_TIME_NONE};
    int32_t sampleRate_ {0};
    int32_t samplePerFrame_ {0};
    int64_t fixDelay_ {0};
    bool isTransitent_ {false};
    bool isEos_ {false};
    std::mutex pluginMutex_;
    float volume_ {-1.0f};
    float speed_ {-1.0f};
    int32_t effectMode_ {-1};
    bool isApe_ {false};
};
}
}

#endif // HISTREAMER_AUDIO_SINK_H
