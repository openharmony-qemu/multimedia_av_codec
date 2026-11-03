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

#ifndef FILTERS_AUDIO_ENCODER_FILTER_H
#define FILTERS_AUDIO_ENCODER_FILTER_H

#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "filter/filter.h"
#ifdef SUPPORT_AUDIO_CONVERT
#include "audio_format_converter.h"
#include "audio_suite_base.h"
#include "audio_stream_info.h"
#endif

constexpr int32_t BUFFER_IS_EOS = 1;

namespace OHOS {
namespace Media {
class MediaCodec;

namespace Pipeline {
struct AudioConfigure {
    int32_t sampleRate = 0;
    int32_t channelCount = 0;
    int64_t bitrate = 0;
    std::string mimeType = Plugins::MimeType::AUDIO_AAC;
    Plugins::AudioChannelLayout channelLayout = Plugins::AudioChannelLayout::MONO;
    Plugins::AudioSampleFormat sampleFormat = Plugins::AudioSampleFormat::SAMPLE_S16LE;
};

constexpr int32_t AVTRANSCODER_MAX_BUFFER_WAIT = 8;
constexpr int32_t SAMPLE_FORMAT_16 = 16;
class AudioEncoderFilter : public Filter, public std::enable_shared_from_this<AudioEncoderFilter> {
public:
    explicit AudioEncoderFilter(std::string name, FilterType type);
    ~AudioEncoderFilter() override;
    Status SetCodecFormat(const std::shared_ptr<Meta> &format);
    void Init(const std::shared_ptr<EventReceiver> &receiver,
        const std::shared_ptr<FilterCallback> &callback) override;
    Status Configure(const std::shared_ptr<Meta> &parameter);
    sptr<Surface> GetInputSurface();
    Status DoPrepare() override;
    Status DoStart() override;
    Status DoPause() override;
    Status DoResume() override;
    Status DoStop() override;
    Status DoFlush() override;
    Status DoRelease() override;
    Status NotifyEos();
    Status SetTranscoderMode();
    Status SetPureAudioMode();
    void SetClipStartTimeUs(int64_t startTimeUs);
    void SetClipTimeFilterNeeded();
    void SetParameter(const std::shared_ptr<Meta> &parameter) override;
    void GetParameter(std::shared_ptr<Meta> &parameter) override;
    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    FilterType GetFilterType();
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);
    void OnUpdatedResult(std::shared_ptr<Meta> &meta);
    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);
    void SetFaultEvent(const std::string &errMsg);
    void SetFaultEvent(const std::string &errMsg, int32_t ret);
    void SetCallingInfo(int32_t appUid, int32_t appPid, const std::string &bundleName, uint64_t instanceId);
    void NotifyProcessingThread();

protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    Status UpdateParameterToConfigure(const std::shared_ptr<Meta> &meta);
    void GetAudioPCMConfigure(const std::shared_ptr<Meta> &meta, std::shared_ptr<AudioConfigure> &pcmConfig,
        const std::string &inputMetaName = "Meta");
    void AdjustAudioConfigure(std::shared_ptr<AudioConfigure> &adjustedConfig,
        std::shared_ptr<AudioConfigure> &inputPCM, std::shared_ptr<AudioConfigure> &outputPCM);
    void AddInternalBufferQueue();
    std::string name_;
    FilterType filterType_;

    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;

    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;

    std::shared_ptr<MediaCodec> mediaCodec_;

    std::string codecMimeType_;
    std::shared_ptr<Meta> configureParameter_;

    std::shared_ptr<Filter> nextFilter_;
    std::string bundleName_;
    std::shared_ptr<Meta> transcoderMeta_ {nullptr};
    bool isTranscoderMode_ {false};
    std::atomic<bool> isPureAudioMode_ {false};
    uint64_t instanceId_{0};
    int32_t appUid_ {0};
    int32_t appPid_ {0};
    std::atomic<bool> shouldJudgeChange {false};

    // 中间队列相关成员变量
    std::shared_ptr<AVBufferQueue> intermediateBufferQueue_;
    sptr<AVBufferQueueProducer> intermediateQueueProducer_;
    sptr<AVBufferQueueConsumer> intermediateQueueConsumer_;
    sptr<AVBufferQueueProducer> mediaCodecInputProducer;
    std::mutex producerMutex_; // 保护 mediaCodecInputProducer 的互斥锁

    // 独立处理线程相关成员变量
    std::thread processingThread_;
    std::mutex processingMutex_;
    std::condition_variable processingCondVar_;
    std::atomic<bool> isProcessingRunning_{false};
    std::atomic<bool> hasPendingBuffer_{false};

    // 处理方法
    void ProcessIntermediateBuffer();
    void ProcessingThreadFunc();
    void StopProcessingThread();
    Status AcquireIntermediateBuffer(std::shared_ptr<AVBuffer>& buffer);
    Status RequestCodecBuffer(std::shared_ptr<AVBuffer>& buffer, const AVBufferConfig& config);
    Status ConvertAudioData(const std::shared_ptr<AVBuffer>& srcBuffer, const std::shared_ptr<AVBuffer>& dstBuffer);
    Status CopyAudioData(const std::shared_ptr<AVBuffer>& srcBuffer, const std::shared_ptr<AVBuffer>& dstBuffer);
    void CopyBufferMetadata(const std::shared_ptr<AVBuffer>& srcBuffer, const std::shared_ptr<AVBuffer>& dstBuffer);
    Status PushBufferToCodec(const std::shared_ptr<AVBuffer>& buffer, bool available = true);

    // 声道转换相关
#ifdef SUPPORT_AUDIO_CONVERT
    class ConverterInputCallback : public AudioStandard::AudioSuite::FormatConverterDataCallback {
    public:
        explicit ConverterInputCallback(const std::shared_ptr<AVBuffer>& buffer);
        ~ConverterInputCallback() override = default;
        int32_t OnRequestData(const void** data, AudioStandard::AudioSuite::InputDataStatus* status) override;
    private:
        std::shared_ptr<AVBuffer> sourceBuffer_;
        size_t readOffset_ = 0;
    };

    std::shared_ptr<AudioStandard::AudioSuite::AudioFormatConverter> channelConverter_;
    AudioStandard::AudioSuite::AudioConverterFormat  converterInputFormat_;
    AudioStandard::AudioSuite::AudioConverterFormat  converterOutputFormat_;
#endif
    std::atomic<bool> needChannelConvert_ {false};
    std::atomic<bool> needClipTimeFilter_ {false};
    int64_t clipStartTimeUs_ = -1;
};
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
#endif // FILTERS_AUDIO_ENCODER_FILTER_H