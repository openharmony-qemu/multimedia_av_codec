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

#ifndef CODEC_AUDIO_CODEC_H
#define CODEC_AUDIO_CODEC_H

#include "audio_base_codec.h"
#include "media_codec.h"
#include "avcodec_common.h"
#include "codecbase.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class AudioCodec;
class AudioCodecCallback : public Media::AudioBaseCodecCallback {
public:
    AudioCodecCallback(const std::shared_ptr<AudioCodec> &codec) : codec_(codec) {}
    virtual ~AudioCodecCallback()
    {
        codec_ = nullptr;
    }

    void OnError(Media::CodecErrorType errorType, int32_t errorCode) override;

    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) override;

private:
    std::shared_ptr<AudioCodec> codec_;
};
class AudioCodec : public std::enable_shared_from_this<AudioCodec>, public CodecBase {
public:
    explicit AudioCodec()
    {
        mediaCodec_ = std::make_shared<Media::MediaCodec>();
    }
    int32_t CreateCodecByName(const std::string &name) override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->Init(name)));
    }

    int32_t Configure(const std::shared_ptr<Media::Meta> &meta) override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->Configure(meta)));
    }
    int32_t SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer) override
    {
        return StatusToAVCodecServiceErrCode(
            static_cast<Media::Status>(mediaCodec_->SetOutputBufferQueue(bufferQueueProducer)));
    }

    int32_t SetCodecCallback(const std::shared_ptr<Media::CodecCallback> &codecCallback)
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->SetCodecCallback(codecCallback)));
    }

    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &codecCallback) override
    {
        callback_ = codecCallback;
        std::shared_ptr<Media::AudioBaseCodecCallback> mediaCallback =
            std::make_shared<AudioCodecCallback>(shared_from_this());
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->SetCodecCallback(mediaCallback)));
    }

    int32_t Prepare() override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->Prepare()));
    }

    sptr<Media::AVBufferQueueProducer> GetInputBufferQueue() override
    {
        return mediaCodec_->GetInputBufferQueue();
    }

    int32_t Start() override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->Start()));
    }

    int32_t Stop() override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->Stop()));
    }

    int32_t Flush() override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->Flush()));
    }

    int32_t Reset() override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->Reset()));
    }

    int32_t Release() override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->Release()));
    }

    int32_t NotifyEos() override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->NotifyEos()));
    }

    int32_t SetParameter(const std::shared_ptr<Media::Meta> &parameter) override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->SetParameter(parameter)));
    }

    int32_t GetOutputFormat(std::shared_ptr<Media::Meta> &parameter) override
    {
        return StatusToAVCodecServiceErrCode(static_cast<Media::Status>(mediaCodec_->GetOutputFormat(parameter)));
    }

    int32_t Configure(const MediaAVCodec::Format &format) override
    {
        (void)format;
        return 0;
    }
    int32_t SetParameter(const MediaAVCodec::Format &format) override
    {
        (void)format;
        return 0;
    }

    int32_t GetOutputFormat(MediaAVCodec::Format &format) override
    {
        (void)format;
        return 0;
    }

    int32_t ReleaseOutputBuffer(uint32_t index) override
    {
        (void)index;
        return 0;
    }

    void OnError(int32_t errorCode)
    {
        if (callback_) {
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, errorCode);
        }
    }

    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer)
    {
        if (callback_) {
            callback_->OnOutputBufferAvailable(0, outputBuffer);
        }
    }

#ifdef SUPPORT_DRM
    int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
                                     const bool svpFlag) override
    {
        return StatusToAVCodecServiceErrCode(
            static_cast<Media::Status>(mediaCodec_->SetAudioDecryptionConfig(keySession, svpFlag)));
    }
#endif

private:
    std::shared_ptr<Media::MediaCodec> mediaCodec_;
    std::shared_ptr<MediaCodecCallback> callback_;
};

void AudioCodecCallback::OnError(Media::CodecErrorType errorType, int32_t errorCode)
{
    if (codec_) {
        (void)errorType;
        codec_->OnError(errorCode);
    }
}

void AudioCodecCallback::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer)
{
    if (codec_) {
        codec_->OnOutputBufferDone(outputBuffer);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
#endif