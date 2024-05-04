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
#ifndef AVCODEC_AUDIO_CODEC_IMPL_H
#define AVCODEC_AUDIO_CODEC_IMPL_H

#include <queue>
#include "avcodec_audio_codec.h"
#include "nocopyable.h"
#include "task_thread.h"
#include "i_avcodec_service.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"
#include "buffer/avbuffer_queue_producer.h"
#include "foundation/multimedia/drm_framework/services/drm_service/ipc/i_keysession_service.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecAudioCodecImpl {
public:
    AVCodecAudioCodecImpl();
    ~AVCodecAudioCodecImpl();

    int32_t Configure(const Format &format);
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    int32_t QueueInputBuffer(uint32_t index);
    int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag);
    int32_t GetOutputFormat(Format &format);
    int32_t ReleaseOutputBuffer(uint32_t index);
    int32_t SetParameter(const Format &format);
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback);
    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name);
    void Notify();

private:
    void ProduceInputBuffer();
    void ConsumerOutputBuffer();
    int32_t GetInputBufferSize();
    void ClearCache();
    void StopTask();
    void PauseTask();

private:
    class AVCodecInnerCallback : public MediaCodecCallback {
    public:
        explicit AVCodecInnerCallback(AVCodecAudioCodecImpl *impl);
        ~AVCodecInnerCallback() = default;
        void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
        void OnOutputFormatChanged(const Format &format) override;
        void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
        void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;

    private:
        AVCodecAudioCodecImpl *impl_;
    };

private:
    std::atomic<bool> isRunning_;
    std::shared_ptr<ICodecService> codecService_ = nullptr;
    std::shared_ptr<Media::AVBufferQueue> implBufferQueue_;
    std::unique_ptr<TaskThread> inputTask_;
    std::unique_ptr<TaskThread> outputTask_;
    std::shared_ptr<MediaCodecCallback> callback_;
    std::condition_variable inputCondition_;
    std::condition_variable outputCondition_;
    std::mutex inputMutex_;
    std::mutex inputMutex2_;
    std::mutex outputMutex_;
    std::mutex outputMutex_2;
    std::atomic<int32_t> bufferConsumerAvailableCount_ = 0;
    std::atomic<int32_t> indexInput_ = 0;
    std::atomic<int32_t> indexOutput_ = 0;
    int32_t inputBufferSize_ = 0;
    std::queue<std::shared_ptr<AVBuffer>> inputIndexQueue;
    std::unordered_map<uint32_t, std::shared_ptr<AVBuffer>> inputBufferObjMap_;
    std::unordered_map<uint32_t, std::shared_ptr<AVBuffer>> outputBufferObjMap_;
    sptr<Media::AVBufferQueueProducer> mediaCodecProducer_;
    sptr<Media::AVBufferQueueProducer> implProducer_;
    sptr<Media::AVBufferQueueConsumer> implConsumer_;
};

class AudioCodecConsumerListener : public Media::IConsumerListener {
public:
    explicit AudioCodecConsumerListener(AVCodecAudioCodecImpl *impl);
    void OnBufferAvailable() override;
private:
    AVCodecAudioCodecImpl *impl_;
};

class AudioCodecProducerListener : public IRemoteStub<Media::IProducerListener> {
public:
    explicit AudioCodecProducerListener(AVCodecAudioCodecImpl *impl);
    void OnBufferAvailable() override;
private:
    AVCodecAudioCodecImpl *impl_;
    bool isCousumer_;
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_AUDIO_CODEC_IMPL_H