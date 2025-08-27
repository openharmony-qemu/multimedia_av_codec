/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_AUDIO_SINK_FILTER_H
#define HISTREAMER_AUDIO_SINK_FILTER_H

#include <atomic>

#include "media_sync_manager.h"
#include "audio_sink.h"
#include "plugin/plugin_info.h"
#include "filter/filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class AudioSinkFilter : public Filter, public std::enable_shared_from_this<AudioSinkFilter> {
public:
    explicit AudioSinkFilter(const std::string& name, FilterType filterType = FilterType::FILTERTYPE_ASINK);
    ~AudioSinkFilter() override;

    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) override;

    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback,
                      const std::shared_ptr<InterruptMonitor> &monitor) override;

    Status DoInitAfterLink() override;

    Status DoPrepare() override;

    Status DoStart() override;

    Status DoPause() override;

    Status DoResume() override;

    Status FreezeAudioSink();

    Status UnFreezeAudioSink();

    Status DoFlush() override;

    Status DoStop() override;

    Status DoRelease() override;

    Status DoSetPlayRange(int64_t start, int64_t end) override;

    Status DoProcessInputBuffer(int recvArg, bool dropFrame) override;

    void SetParameter(const std::shared_ptr<Meta>& meta) override;

    void GetParameter(std::shared_ptr<Meta>& meta) override;

// meta include stream id, used in multiple audio track muxer.
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta>& meta,
        const std::shared_ptr<FilterLinkCallback>& callback) override;

    Status DoSetPerfRecEnabled(bool isPerfRecEnabled) override;

    Status SetVolume(float volume);

    Status SetVolumeMode(int32_t mode);

    void SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter);

    Status SetSpeed(float speed);

    int32_t SetVolumeWithRamp(float targetVolume, int32_t duration);

    Status SetAudioEffectMode(int32_t effectMode);

    Status GetAudioEffectMode(int32_t &effectMode);

    Status SetIsTransitent(bool isTransitent);

    Status ChangeTrack(std::shared_ptr<Meta>& meta);

    Status HandleFormatChange(std::shared_ptr<Meta>& meta) override;

    Status SetMuted(bool isMuted) override;

    Status SetSeekTime(int64_t seekTime);
    float GetMaxAmplitude();
    int32_t SetMaxAmplitudeCbStatus(bool status);

    void SetIsCancelStart(bool isCancelStart);

    Status SetIsCalledBySystemApp(bool isCalledBySystemApp);

    Status SetLooping(bool loop);

    Status SetAudioHapticsSyncId(int32_t syncId);

    void OnBufferAvailable();

    void SetIsAudioDemuxDecodeAsync(bool isAudioDemuxDecodeAsync);

    Status SetLoudnessGain(float loudnessGain);

    Status cacheBuffer();

protected:
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta>& meta,
        const std::shared_ptr<FilterLinkCallback>& callback) override;

    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

    void DoSetIsInPrePausing(bool isInPrePausing) override;

    class AVBufferAvailableListener : public IConsumerListener {
    public:
	    AVBufferAvailableListener(std::shared_ptr<AudioSinkFilter> audioSinkFilter);

        void OnBufferAvailable() override;
    private:
	    std::weak_ptr<AudioSinkFilter> audioSinkFilter_;
    };

private:
    inline bool NeedImmediateRender() const
    {
        return needImmediateRender_;
    }

private:
    std::shared_ptr<AudioSink> audioSink_;
    std::string name_;
    FilterType filterType_;
    std::shared_ptr<Meta> trackMeta_;
    std::shared_ptr<Meta> globalMeta_;

    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;

    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    int64_t frameCnt_ {0};
    Plugins::AudioRenderInfo audioRenderInfo_ {};

    float volume_ {-1.0f}; // default invalid value

    bool forceUpdateTimeAnchorNextTime_ {false};
    bool isCancelStart_ {false};
    bool isRenderCallbackMode_ {true};
    bool isProcessInputMerged_ {true};
    bool isAudioDemuxDecodeAsync_ {true};
    bool needImmediateRender_ {false};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_AUDIO_SINK_FILTER_H
