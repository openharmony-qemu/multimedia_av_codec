/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef AUDIO_DECODER_CAPI_MOCK
#define AUDIO_DECODER_CAPI_MOCK
#include <mutex>
#include <queue>
#include <thread>
#include "audio_decoder_mock_base.h"
#include "avcodec_audio_codec.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"

namespace OHOS {
namespace MediaAVCodec {
class AudioDecoderInnerMock : public AudioDecoderMockBase {
public:
    AudioDecoderInnerMock() = default;
    ~AudioDecoderInnerMock() override = default;
    int32_t Create(const char *mimetype) override;
    int32_t CreateByName(const char *name) override;
    int32_t Start(int32_t sampleRate, int32_t channel, std::vector<uint8_t> *codecConfig) override;
    int32_t Stop() override;
    int32_t DecodeInput(const uint8_t *dataIn, uint32_t inSizeBytes, std::vector<uint8_t> *skipInfo) override;
    int32_t DecodeOutput(uint8_t *dataOut, int32_t &outSizeBytes) override;
    void OnBufferAvailable();

private:
    int32_t GetInputBufferSize();
    std::shared_ptr<AVCodecAudioCodec> audioCodec_;
    std::shared_ptr<Media::Meta> meta_;
    std::shared_ptr<Media::AVBufferQueue> innerBufferQueue_;
    sptr<Media::AVBufferQueueConsumer> implConsumer_;
    sptr<Media::AVBufferQueueProducer> mediaCodecProducer_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<std::shared_ptr<AVBuffer>> outputBufferQueue_;
};

} // namespace MediaAVCodec
} // namespace OHOS
#endif  // AUDIO_DECODER_CAPI_MOCK