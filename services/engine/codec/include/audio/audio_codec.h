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
class AudioCodec : public CodecBase {
public:
    explicit AudioCodec()
    {
        mediaCodec_ = std::make_shared<Media::MediaCodec>();
    }
    int32_t CreateCodecByName(const std::string &name) override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->Init(name)));
    }

    int32_t Configure(const std::shared_ptr<Media::Meta> &meta) override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->Configure(meta)));
    }
    int32_t SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer) override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->SetOutputBufferQueue(bufferQueueProducer)));
    }
    int32_t SetCodecCallback(const std::shared_ptr<Media::CodecCallback> &codecCallback)
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->SetCodecCallback(codecCallback)));
    }

    int32_t Prepare() override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->Prepare()));
    }

    sptr<Media::AVBufferQueueProducer> GetInputBufferQueue() override
    {
        return mediaCodec_->GetInputBufferQueue();
    }

    int32_t Start() override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->Start()));
    }

    int32_t Stop() override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->Stop()));
    }

    int32_t Flush() override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->Flush()));
    }

    int32_t Reset() override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->Reset()));
    }

    int32_t Release() override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->Release()));
    }

    int32_t NotifyEos() override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->NotifyEos()));
    }

    int32_t SetParameter(const std::shared_ptr<Media::Meta> &parameter) override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->SetParameter(parameter)));
    }

    int32_t GetOutputFormat(std::shared_ptr<Media::Meta> &parameter) override
    {
        return StatusConvert(static_cast<Media::Status> (mediaCodec_->GetOutputFormat(parameter)));
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

private:
    std::shared_ptr<Media::MediaCodec> mediaCodec_;

    int32_t StatusConvert(Media::Status status)
    {
        const static std::unordered_map<Media::Status, int32_t> table = {
            {Media::Status::END_OF_STREAM, AVCodecServiceErrCode::AVCS_ERR_OK},
            {Media::Status::OK, AVCodecServiceErrCode::AVCS_ERR_OK},
            {Media::Status::NO_ERROR, AVCodecServiceErrCode::AVCS_ERR_OK},
            {Media::Status::ERROR_UNKNOWN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
            {Media::Status::ERROR_PLUGIN_ALREADY_EXISTS, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
            {Media::Status::ERROR_INCOMPATIBLE_VERSION, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
            {Media::Status::ERROR_NO_MEMORY, AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY},
            {Media::Status::ERROR_WRONG_STATE, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
            {Media::Status::ERROR_UNIMPLEMENTED, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT},
            {Media::Status::ERROR_INVALID_PARAMETER, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
            {Media::Status::ERROR_INVALID_DATA, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
            {Media::Status::ERROR_MISMATCHED_TYPE, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
            {Media::Status::ERROR_TIMED_OUT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
            {Media::Status::ERROR_UNSUPPORTED_FORMAT, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT_FILE_TYPE},
            {Media::Status::ERROR_NOT_ENOUGH_DATA, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
            {Media::Status::ERROR_NOT_EXISTED, AVCodecServiceErrCode::AVCS_ERR_OPEN_FILE_FAILED},
            {Media::Status::ERROR_AGAIN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
            {Media::Status::ERROR_PERMISSION_DENIED, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
            {Media::Status::ERROR_NULL_POINTER, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
            {Media::Status::ERROR_INVALID_OPERATION, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
            {Media::Status::ERROR_CLIENT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
            {Media::Status::ERROR_SERVER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
            {Media::Status::ERROR_DELAY_READY, AVCodecServiceErrCode::AVCS_ERR_OK},
            {Media::Status::ERROR_INVALID_BUFFER_SIZE, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        };
        auto ite = table.find(status);
        if (ite == table.end()) {
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
        return ite->second;
    }
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif