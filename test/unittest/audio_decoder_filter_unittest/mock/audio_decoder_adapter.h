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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
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
class AudioDecoderAdapter {
public:
    AudioDecoderAdapter() = default;
    virtual ~AudioDecoderAdapter() = default;
    MOCK_METHOD(Status, Configure, (const std::shared_ptr<Meta> &parameter), ());
    MOCK_METHOD(Status, Prepare, (), ());
    MOCK_METHOD(Status, Start, (), ());
    MOCK_METHOD(Status, Stop, (), ());
    MOCK_METHOD(Status, Flush, (), ());
    MOCK_METHOD(Status, Reset, (), ());
    MOCK_METHOD(Status, Release, (), ());
    MOCK_METHOD(Status, SetParameter, (const std::shared_ptr<Meta> &parameter), ());
    MOCK_METHOD(Status, SetOutputBufferQueue,
        (const sptr<OHOS::Media::AVBufferQueueProducer> &bufferQueueProducer), ());
    MOCK_METHOD(int32_t, GetOutputFormat, (std::shared_ptr<Meta> &parameter), ());
    MOCK_METHOD(sptr<OHOS::Media::AVBufferQueueProducer>, GetInputBufferQueue, (), ());
    MOCK_METHOD(sptr<OHOS::Media::AVBufferQueueConsumer>, GetInputBufferQueueConsumer, (), ());
    MOCK_METHOD(sptr<OHOS::Media::AVBufferQueueProducer>, GetOutputBufferQueueProducer, (), ());
    MOCK_METHOD(void, ProcessInputBufferInner,
        (bool isTriggeredByOutPort, bool isFlushed, uint32_t &bufferStatus), ());
    MOCK_METHOD(void, ProcessInputBuffer, (), ());
    MOCK_METHOD(int32_t, NotifyEos, (), ());
    MOCK_METHOD(Status, Init, (bool isMimeType, const std::string &name), ());
    MOCK_METHOD(Status, ChangePlugin,
        (const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta), ());
    MOCK_METHOD(void, SetDumpInfo, (bool isDump, uint64_t instanceId), ());
    MOCK_METHOD(void, OnDumpInfo, (int32_t fd), ());
    MOCK_METHOD(int32_t, SetCodecCallback,
        (const std::shared_ptr<OHOS::MediaAVCodec::MediaCodecCallback> &codecCallback), ());
    MOCK_METHOD(int32_t, SetAudioDecryptionConfig,
        (const sptr<OHOS::DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag), ());

private:

    std::shared_ptr<MediaAVCodec::AVCodecAudioCodec> audiocodec_{nullptr};
    
    sptr<Media::AVBufferQueueProducer> outputBufferQueueProducer_{nullptr};

    sptr<Media::AVBufferQueueProducer> inputBufferQueueProducer_{nullptr};

    std::atomic<bool> isRunning_{false};
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_DECODER_ADAPTER_H