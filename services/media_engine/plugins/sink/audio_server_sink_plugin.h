/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_AU_SERVER_SINK_PLUGIN_H
#define HISTREAMER_AU_SERVER_SINK_PLUGIN_H

#include <atomic>
#include <memory>
#include <unordered_map>

#include "audio_info.h"
#include "audio_renderer.h"
#include "osal/task/mutex.h"
#include "meta/audio_types.h"
#include "plugin/audio_sink_plugin.h"
#include "ffmpeg_utils.h"
#include "ffmpeg_convert.h"
#include "timestamp.h"

namespace OHOS {
namespace Media {
namespace Plugins {
class AudioServerSinkPlugin : public Plugins::AudioSinkPlugin {
public:
    explicit AudioServerSinkPlugin(std::string name);

    ~AudioServerSinkPlugin() override;

    Status Init() override;

    __attribute__((no_sanitize("cfi"))) Status Deinit() override;

    Status Prepare() override;

    Status Reset() override;

    Status Start() override;

    Status Stop() override;

    Status GetParameter(std::shared_ptr<Meta> &meta) override;

    Status SetParameter(const std::shared_ptr<Meta> &meta) override;

    Status SetCallback(Callback *cb) override
    {
        callback_ = cb;
        return Status::OK;
    }

    Status GetMute(bool &mute) override
    {
        return Status::OK;
    }

    Status SetMute(bool mute) override
    {
        return Status::OK;
    }

    Status GetVolume(float &volume) override;

    Status SetVolume(float volume) override;

    Status GetSpeed(float &speed) override;

    Status SetSpeed(float speed) override;

    Status Pause() override;

    Status PauseTransitent() override;

    Status Resume() override;

    Status GetLatency(uint64_t &hstTime) override;

    Status GetFrameSize(size_t &size) override
    {
        return Status::OK;
    }

    Status GetFrameCount(uint32_t &count) override
    {
        return Status::OK;
    }

    Status Write(const std::shared_ptr<OHOS::Media::AVBuffer> &inputBuffer) override;

    int32_t WriteAudioVivid(const std::shared_ptr<OHOS::Media::AVBuffer> &inputBuffer);

    Status Flush() override;

    Status Drain() override;

    int64_t GetPlayedOutDurationUs(int64_t nowUs) override;

    Status GetFramePosition(int32_t &framePosition) override;

    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver) override;

    int32_t SetVolumeWithRamp(float targetVolume, int32_t duration) override;

    Status GetAudioEffectMode(int32_t &effectMode) override;

    Status SetAudioEffectMode(int32_t effectMode) override;

private:
    class AudioRendererCallbackImpl : public OHOS::AudioStandard::AudioRendererCallback,
        public OHOS::AudioStandard::AudioRendererOutputDeviceChangeCallback {
    public:
        AudioRendererCallbackImpl(const std::shared_ptr<Pipeline::EventReceiver> &receiver, const bool &isPaused);
        void OnInterrupt(const OHOS::AudioStandard::InterruptEvent &interruptEvent) override;
        void OnStateChange(const OHOS::AudioStandard::RendererState state,
                           const OHOS::AudioStandard::StateChangeCmdType cmdType) override;
        void OnOutputDeviceChange(const AudioStandard::DeviceInfo &deviceInfo,
            const AudioStandard::AudioStreamDeviceChangeReason reason) override;
    private:
        std::shared_ptr<Pipeline::EventReceiver> playerEventReceiver_;
        bool isPaused_{false};
    };
    class AudioServiceDiedCallbackImpl : public OHOS::AudioStandard::AudioRendererPolicyServiceDiedCallback {
    public:
        explicit AudioServiceDiedCallbackImpl(std::shared_ptr<Pipeline::EventReceiver> &receiver);
        void OnAudioPolicyServiceDied() override;
    private:
        std::shared_ptr<Pipeline::EventReceiver> playerEventReceiver_;
    };
    class AudioFirstFrameCallbackImpl : public OHOS::AudioStandard::AudioRendererFirstFrameWritingCallback {
    public:
        explicit AudioFirstFrameCallbackImpl(std::shared_ptr<Pipeline::EventReceiver> &receiver);
        void OnFirstFrameWriting(uint64_t latency) override;

    private:
        std::shared_ptr<Pipeline::EventReceiver> playerEventReceiver_;
    };
    void ReleaseRender();
    __attribute__((no_sanitize("cfi"))) void ReleaseFile();
    bool StopRender();
    bool AssignSampleRateIfSupported(uint32_t sampleRate);
    bool AssignChannelNumIfSupported(uint32_t channelNum);
    bool AssignSampleFmtIfSupported(AudioSampleFormat sampleFormat);
    void SetInterruptMode(AudioStandard::InterruptMode interruptMode);

    void SetUpParamsSetterMap();
    void SetUpMimeTypeSetter();
    void SetUpSampleRateSetter();
    void SetUpAudioOutputChannelsSetter();
    void SetUpMediaBitRateSetter();
    void SetUpAudioSampleFormatSetter();
    void SetUpAudioOutputChannelLayoutSetter();
    void SetUpAudioSamplePerFrameSetter();
    void SetUpBitsPerCodedSampleSetter();
    void SetUpMediaSeekableSetter();
    void SetUpAppPidSetter();
    void SetUpAppUidSetter();
    void SetUpAudioRenderInfoSetter();
    void SetUpAudioInterruptModeSetter();
    void SetUpAudioRenderSetFlagSetter();
    void SetAudioDumpBySysParam();
    void DumpEntireAudioBuffer(uint8_t* buffer, const size_t& bytesSingle);
    void DumpSliceAudioBuffer(uint8_t* buffer, const size_t& bytesSingle);

    OHOS::Media::Mutex renderMutex_{};
    Callback *callback_{};
    AudioRenderInfo audioRenderInfo_{};
    AudioStandard::AudioRendererOptions rendererOptions_{};
    AudioStandard::InterruptMode audioInterruptMode_{AudioStandard::InterruptMode::SHARE_MODE};
    std::unique_ptr<AudioStandard::AudioRenderer> audioRenderer_{nullptr};
    std::shared_ptr<AudioRendererCallbackImpl> audioRendererCallback_{nullptr};
    std::shared_ptr<OHOS::AudioStandard::AudioRendererFirstFrameWritingCallback> audioFirstFrameCallback_{nullptr};
    std::shared_ptr<OHOS::AudioStandard::AudioRendererPolicyServiceDiedCallback> audioServiceDiedCallback_{nullptr};
    AudioStandard::AudioRendererParams rendererParams_{};

    std::shared_ptr<Pipeline::EventReceiver> playerEventReceiver_;
    bool fmtSupported_{false};
    bool isForcePaused_{false};
    std::shared_ptr<Meta> meta_;
    AVSampleFormat reSrcFfFmt_{AV_SAMPLE_FMT_NONE};
    const AudioStandard::AudioSampleFormat reStdDestFmt_{AudioStandard::AudioSampleFormat::SAMPLE_S16LE};
    AudioChannelLayout channelLayout_{};
    std::string mime_type_;
    uint32_t channels_{};
    uint32_t samplesPerFrame_{};
    uint32_t bitsPerSample_{0};
    uint32_t sampleRate_{};
    int64_t bitRate_{0};
    int32_t appPid_{0};
    int32_t appUid_{0};
    bool needReformat_{false};
    Plugins::Seekable seekable_{Plugins::Seekable::INVALID};
    std::shared_ptr<Ffmpeg::Resample> resample_{nullptr};

    std::unordered_map<TagType, std::function<Status(const ValueType &para)>> paramsSetterMap_;
    float audioRendererVolume_ = 1.0;

    FILE* entireDumpFile_ = nullptr;
    FILE* sliceDumpFile_ = nullptr;
    int32_t sliceCount_ {0};
    int32_t curCount_ {-1};
    bool enableEntireDump_ {false};
    bool enableDumpSlice_ {false};
    bool audioRenderSetFlag_ {false};
};
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_AU_SERVER_SINK_PLUGIN_H
