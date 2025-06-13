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

#ifndef AUDIO_DECODER_ADAPTER_H
#define AUDIO_DECODER_ADAPTER_H

#include <shared_mutex>
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "osal/task/condition_variable.h"
#include "meta/format.h"
#include "avcodec_audio_codec.h"
#include "avcodec_common.h"

namespace OHOS {
namespace Media {
class AudioDecoderAdapter : public std::enable_shared_from_this<AudioDecoderAdapter> {
public:
    AudioDecoderAdapter();
    virtual ~AudioDecoderAdapter();

    Status Configure(const std::shared_ptr<Meta> &parameter);

    Status Prepare();

    Status Start();

    Status Stop();

    Status Flush();

    Status Reset();

    Status Release();

    Status SetParameter(const std::shared_ptr<Meta> &parameter);

    Status SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer);

    int32_t GetOutputFormat(std::shared_ptr<Meta> &parameter);

    sptr<Media::AVBufferQueueProducer> GetInputBufferQueue();

    sptr<Media::AVBufferQueueConsumer> GetInputBufferQueueConsumer();

    sptr<Media::AVBufferQueueProducer> GetOutputBufferQueueProducer();

    void ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed, uint32_t &bufferStatus);

    void ProcessInputBuffer();

    int32_t NotifyEos();

    Status Init(bool isMimeType, const std::string &name);

    Status ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta);

    void SetDumpInfo(bool isDump, uint64_t instanceId);

    void OnDumpInfo(int32_t fd);

    int32_t SetCodecCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &codecCallback);

    int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag);

private:

    std::shared_ptr<MediaAVCodec::AVCodecAudioCodec> audiocodec_{nullptr};
    
    sptr<Media::AVBufferQueueProducer> outputBufferQueueProducer_{nullptr};

    sptr<Media::AVBufferQueueProducer> inputBufferQueueProducer_{nullptr};

    std::atomic<bool> isRunning_{false};
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_DECODER_ADAPTER_H