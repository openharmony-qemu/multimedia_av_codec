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
#include "audio_decoder_inner_mock.h"
#include "nocopyable.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "native_avbuffer_info.h"

using namespace OHOS;
using namespace OHOS::Media;
namespace OHOS {
namespace MediaAVCodec {
#define DEMO_LOG(...) printf(__VA_ARGS__);printf("\n")
static constexpr int32_t TIME_OUT_MS = 1000;
std::unique_ptr<AudioDecoderMockBase> AudioDecoderMockBase::CreateDecoder()
{
    return std::make_unique<AudioDecoderInnerMock>();
}

class AudioCodecConsumerListener : public OHOS::Media::IConsumerListener {
public:
    explicit AudioCodecConsumerListener(AudioDecoderInnerMock *impl)
    {
        impl_ = impl;
    }
    void OnBufferAvailable() override;

private:
    AudioDecoderInnerMock *impl_;
};

void AudioCodecConsumerListener::OnBufferAvailable()
{
    impl_->OnBufferAvailable();
}

void AudioDecoderInnerMock::OnBufferAvailable()
{
    std::shared_ptr<AVBuffer> outputBuffer = nullptr;
    Media::Status ret = implConsumer_->AcquireBuffer(outputBuffer);
    if (ret != Media::Status::OK) {
        DEMO_LOG("AudioDecoderInnerMock::OnBufferAvailable AcquireBuffer error!");
        return;
    }
    if (outputBuffer == nullptr) {
        DEMO_LOG("AudioDecoderInnerMock::OnBufferAvailable AcquireBuffer null!");
        return;
    }
    std::unique_lock<std::mutex> inLock(mutex_);
    outputBufferQueue_.push(outputBuffer);
    cond_.notify_all();
}

int32_t AudioDecoderInnerMock::Create(const char *mimetype)
{
    (void)mimetype;
    return 0;
}

int32_t AudioDecoderInnerMock::CreateByName(const char *name)
{
    innerBufferQueue_ = Media::AVBufferQueue::Create(10, Media::MemoryType::SHARED_MEMORY, "sikpInfoUtBuffer");  // 10
    audioCodec_ = AudioCodecFactory::CreateByName(name);
    if (audioCodec_ == nullptr) {
        return -1;
    }
    return 0;
}

int32_t AudioDecoderInnerMock::GetInputBufferSize()
{
    int32_t capacity = 0;
    std::shared_ptr<Media::Meta> bufferConfig = std::make_shared<Media::Meta>();

    int32_t ret = audioCodec_->GetOutputFormat(bufferConfig);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        return capacity;
    }
    bufferConfig->Get<Media::Tag::AUDIO_MAX_INPUT_SIZE>(capacity);
    return capacity;
}

int32_t AudioDecoderInnerMock::Start(int32_t sampleRate, int32_t channel, std::vector<uint8_t> *codecConfig)
{
    if (audioCodec_ == nullptr || innerBufferQueue_ == nullptr) {
        return -1;
    }
    int32_t enable = 1;
    auto meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::ENABLE_BUFFER_SKIP_SAMPLES>(enable);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channel);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    if (codecConfig != nullptr && codecConfig->size() > 0) {
        meta->Set<Tag::MEDIA_CODEC_CONFIG>(*codecConfig);
    }
    if (blockAlign_ != 0) {
        meta->Set<Tag::AUDIO_BLOCK_ALIGN>(blockAlign_);
    }
    audioCodec_->Configure(meta);

    audioCodec_->SetOutputBufferQueue(innerBufferQueue_->GetProducer());
    audioCodec_->Prepare();

    implConsumer_ = innerBufferQueue_->GetConsumer();
    sptr<Media::IConsumerListener> comsumerListener = new AudioCodecConsumerListener(this);
    implConsumer_->SetBufferAvailableListener(comsumerListener);
    mediaCodecProducer_ = audioCodec_->GetInputBufferQueue();
    return audioCodec_->Start();
}

int32_t AudioDecoderInnerMock::Stop()
{
    if (mediaCodecProducer_ != nullptr) {
        // EOS
        Media::AVBufferConfig avBufferConfig;
        avBufferConfig.size = GetInputBufferSize();
        std::shared_ptr<AVBuffer> inputBuffer = nullptr;
        Media::Status ret = mediaCodecProducer_->RequestBuffer(inputBuffer, avBufferConfig, 8);  // time out 8ms
        if (ret != Media::Status::OK || inputBuffer == nullptr) {
            DEMO_LOG("Stop RequestBuffer failed!");
            return -1;
        }
        inputBuffer->memory_->SetSize(1);
        inputBuffer->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
        ret = mediaCodecProducer_->PushBuffer(inputBuffer, true);
        if (ret != Media::Status::OK) {
            DEMO_LOG("Stop PushBuffer failed!");
        }
    }
    if (audioCodec_ != nullptr) {
        audioCodec_->Stop();
        audioCodec_->Release();
    }

    // 清空 outputBufferQueue_
    std::unique_lock<std::mutex> inLock(mutex_);
    std::queue<std::shared_ptr<AVBuffer>> emptyQueue;
    swap(emptyQueue, outputBufferQueue_);
    inLock.unlock();
    return 0;
}

int32_t AudioDecoderInnerMock::DecodeInput(const uint8_t *dataIn, uint32_t inSizeBytes, std::vector<uint8_t> *skipInfo)
{
    // 申请avbuffer
    outputPts_ = 0;
    outputFlag_ = 0;
    Media::AVBufferConfig avBufferConfig;
    avBufferConfig.size = GetInputBufferSize();
    std::shared_ptr<AVBuffer> inputBuffer = nullptr;
    Media::Status ret = mediaCodecProducer_->RequestBuffer(inputBuffer, avBufferConfig, 8);  // time out 8ms
    if (ret != Media::Status::OK) {
        DEMO_LOG("DecodeInput RequestBuffer failed!");
        return -1;
    }
    if (skipInfo != nullptr && skipInfo->size() > 0) {
        inputBuffer->meta_->SetData("buffer_skip_samples_info", *skipInfo);
    }
    // 拷贝数据到avbuffer里
    if (memcpy_s(inputBuffer->memory_->GetAddr(), inSizeBytes, dataIn, inSizeBytes) != EOK) {
        DEMO_LOG("DecodeInput memcpy_s failed!");
    }
    inputBuffer->memory_->SetSize(inSizeBytes);
    inputBuffer->flag_ = flag_;
    inputBuffer->pts_ = pts_;
    // buffer 送解码器
    mediaCodecProducer_->PushBuffer(inputBuffer, true);
    return 0;
}

int32_t AudioDecoderInnerMock::DecodeOutput(uint8_t *dataOut, int32_t &outSizeBytes)
{
    std::unique_lock<std::mutex> inLock(mutex_);
    // 等待解码完成
    auto &outputBufferQueue = outputBufferQueue_;
    cond_.wait_for(inLock, std::chrono::milliseconds(TIME_OUT_MS),
        [&outputBufferQueue]() {return (outputBufferQueue.size() > 0);});
    if (outputBufferQueue_.size() < 1) {
        DEMO_LOG("DecodeOutput get outputBuffer failed!");
        return -1;
    }
    std::shared_ptr<AVBuffer> outputBuffer = outputBufferQueue_.front();
    int64_t size = outputBuffer->memory_->GetSize();
    // 解码后数据拷贝到输出缓存
    if (dataOut != nullptr) {
        if (memcpy_s(dataOut, size, outputBuffer->memory_->GetAddr(), size) != EOK) {
            DEMO_LOG("DecodeOutput memcpy_s failed!");
        }
    }
    outputPts_ = outputBuffer->pts_;
    outputFlag_ = outputBuffer->flag_;
    // 释放buffer
    implConsumer_->ReleaseBuffer(outputBuffer);
    outputBufferQueue_.pop();
    outSizeBytes = static_cast<int32_t>(size);
    return static_cast<int32_t>(outputBufferQueue_.size());
}
} // namespace MediaAVCodec
} // namespace OHOS