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

#include "ffmpeg_base_encoder.h"
#include "avcodec_log.h"
#include "avcodec_common.h"
#include "ffmpeg_converter.h"
#include "osal/utils/util.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegBaseEncoder"};
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegBaseEncoder::FFmpegBaseEncoder()
    : maxInputSize_(-1),
      avCodec_(nullptr),
      avCodecContext_(nullptr),
      cachedFrame_(nullptr),
      avPacket_(nullptr),
      prevPts_(0),
      codecContextValid_(false)
{
}

FFmpegBaseEncoder::~FFmpegBaseEncoder()
{
    CloseCtxLocked();
    avCodecContext_.reset();
}

Status FFmpegBaseEncoder::ProcessSendData(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    auto memory = inputBuffer->memory_;
    if (memory == nullptr) {
        AVCODEC_LOGE("memory is nullptr");
        return Status::ERROR_INVALID_DATA;
    }
    if (memory->GetSize() <= 0 && !(inputBuffer->flag_ & BUFFER_FLAG_EOS)) {
        AVCODEC_LOGE("size is %{public}d, but flag is not 1 ", memory->GetSize());
        return Status::ERROR_INVALID_DATA;
    }
    Status ret;
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        if (avCodecContext_ == nullptr) {
            return Status::ERROR_WRONG_STATE;
        }
        ret = SendBuffer(inputBuffer);
        if (ret == Status::OK || ret == Status::END_OF_STREAM) {
            std::lock_guard<std::mutex> l(bufferMetaMutex_);
            if (inputBuffer->meta_ == nullptr) {
                AVCODEC_LOGE("encoder input buffer or meta is nullptr");
                return Status::ERROR_INVALID_DATA;
            }
            bufferMeta_ = inputBuffer->meta_;
            dataCallback_->OnInputBufferDone(inputBuffer);
            ret = Status::OK;
        }
    }
    return ret;
}

Status FFmpegBaseEncoder::PcmFillFrame(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    auto memory = inputBuffer->memory_;
    auto usedSize = memory->GetSize();
    auto frameSize = avCodecContext_->frame_size;
    cachedFrame_->nb_samples = usedSize / channelsBytesPerSample_;
    AVCODEC_LOGI("sampleRate : %{public}d, frameSize : %{public}d", avCodecContext_->sample_rate, frameSize);
    if (cachedFrame_->nb_samples > frameSize) {
        AVCODEC_LOGE("cachedFrame_->nb_samples is greater than frameSize, please enter a correct frameBytes."
                     "hint: nb_samples is %{public}d. frameSize is %{public}d.", cachedFrame_->nb_samples, frameSize);
        return Status::ERROR_UNKNOWN;
    }
    cachedFrame_->data[0] = memory->GetAddr();
    cachedFrame_->extended_data = cachedFrame_->data;
    cachedFrame_->linesize[0] = usedSize;
    return Status::OK;
}

Status FFmpegBaseEncoder::SendBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    int ret = av_frame_make_writable(cachedFrame_.get());
    if (ret != 0) {
        AVCODEC_LOGD("Frame make writable failed: %{public}s", OSAL::AVStrError(ret).c_str());
        return Status::ERROR_UNKNOWN;
    }

    auto memory = inputBuffer->memory_;
    bool isEos = inputBuffer->flag_ & BUFFER_FLAG_EOS;
    if (!isEos) {
        auto errCode = PcmFillFrame(inputBuffer);
        if (errCode != Status::OK) {
            AVCODEC_LOGE("SendBuffer PcmFillFrame error");
            return errCode;
        }
        ret = avcodec_send_frame(avCodecContext_.get(), cachedFrame_.get());
    } else {
        ret = avcodec_send_frame(avCodecContext_.get(), nullptr);
    }
    if (ret == 0) {
        return Status::OK;
    } else if (ret == AVERROR(EAGAIN)) {
        AVCODEC_LOGE("skip this frame because data not enough, msg:%{public}s", OSAL::AVStrError(ret).data());
        return Status::ERROR_NOT_ENOUGH_DATA;
    } else if (ret == AVERROR_EOF) {
        AVCODEC_LOGD("eos send frame, msg:%{public}s", OSAL::AVStrError(ret).data());
        return Status::END_OF_STREAM;
    } else {
        AVCODEC_LOGD("Send frame unknown error: %{public}s", OSAL::AVStrError(ret).c_str());
        return Status::ERROR_UNKNOWN;
    }
}

Status FFmpegBaseEncoder::ProcessReceiveData(std::shared_ptr<AVBuffer> &outputBuffer)
{
    if (!outputBuffer) {
        AVCODEC_LOGE("queue out buffer is nullptr.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(avMutext_);
    if (avCodecContext_ == nullptr) {
        return Status::ERROR_WRONG_STATE;
    }
    outBuffer_ = outputBuffer;
    Status ret = SendOutputBuffer(outputBuffer);
    return ret;
}

Status FFmpegBaseEncoder::ReceiveBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    (void)memset_s(avPacket_.get(), sizeof(AVPacket), 0, sizeof(AVPacket));
    auto ret = avcodec_receive_packet(avCodecContext_.get(), avPacket_.get());
    Status status;
    if (ret >= 0) {
        status = ReceivePacketSucc(outputBuffer);
    } else if (ret == AVERROR_EOF) {
        AVCODEC_LOGI("eos received");
        outputBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
        avcodec_flush_buffers(avCodecContext_.get());
        status = Status::END_OF_STREAM;
    } else if (ret == AVERROR(EAGAIN)) {
        status = Status::ERROR_NOT_ENOUGH_DATA;
    } else {
        AVCODEC_LOGE("audio encoder receive unknow error: %{public}s", OSAL::AVStrError(ret).c_str());
        status = Status::ERROR_UNKNOWN;
    }
    av_packet_unref(avPacket_.get());
    return status;
}

Status FFmpegBaseEncoder::ReceivePacketSucc(std::shared_ptr<AVBuffer> &outputBuffer)
{
    auto memory = outputBuffer->memory_;

    int32_t outputSize = avPacket_->size;
    if (memory->GetCapacity() < outputSize) {
        AVCODEC_LOGW("Output buffer capacity is not enough");
        return Status::ERROR_NO_MEMORY;
    }

    auto len = memory->Write(avPacket_->data, avPacket_->size, 0);
    if (len < avPacket_->size) {
        AVCODEC_LOGE("write packet data failed, len = %{public}d", len);
        return Status::ERROR_UNKNOWN;
    }

    outputBuffer->duration_ = ConvertTimeFromFFmpeg(avPacket_->duration, avCodecContext_->time_base);
    outputBuffer->pts_ = ((INT64_MAX - prevPts_) < avPacket_->duration) ?
                    (outputBuffer->duration_ - (INT64_MAX - prevPts_)) :
                    (prevPts_ + outputBuffer->duration_);
    prevPts_ = outputBuffer->pts_;
    return Status::OK;
}

Status FFmpegBaseEncoder::SendOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    Status status = ReceiveBuffer(outputBuffer);
    if (status == Status::OK || status == Status::END_OF_STREAM) {
        {
            std::lock_guard<std::mutex> l(bufferMetaMutex_);
            outBuffer_->meta_ = bufferMeta_;
        }
        dataCallback_->OnOutputBufferDone(outBuffer_);
    } else {
        AVCODEC_LOGE("SendOutputBuffer-ReceiveBuffer error");
    }
    return status;
}

Status FFmpegBaseEncoder::Stop()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    avCodecContext_ = nullptr;
    if (outBuffer_) {
        outBuffer_.reset();
        outBuffer_ = nullptr;
    }
    return ret;
}

Status FFmpegBaseEncoder::Reset()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    prevPts_ = 0;
    return ret;
}

Status FFmpegBaseEncoder::Release()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    return ret;
}

Status FFmpegBaseEncoder::Flush()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (avCodecContext_ != nullptr) {
        avcodec_flush_buffers(avCodecContext_.get());
    }
    prevPts_ = 0;
    return ReAllocateContext();
}

Status FFmpegBaseEncoder::AllocateContext(const std::string &name)
{
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        avCodec_ = std::shared_ptr<AVCodec>(const_cast<AVCodec *>(avcodec_find_encoder_by_name(name.c_str())),
                                            [](AVCodec *ptr) {});
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *fp) { av_frame_free(&fp); });
        avPacket_ = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *ptr) { av_packet_free(&ptr); });
    }
    if (avCodec_ == nullptr) {
        return Status::ERROR_INVALID_OPERATION;
    }

    AVCodecContext *context = nullptr;
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        context = avcodec_alloc_context3(avCodec_.get());
        avCodecContext_ = std::shared_ptr<AVCodecContext>(context, [](AVCodecContext *ptr) {
            avcodec_free_context(&ptr);
            avcodec_close(ptr);
        });
        av_log_set_level(AV_LOG_ERROR);
    }
    return Status::OK;
}

Status FFmpegBaseEncoder::InitContext(const std::shared_ptr<Meta> &format)
{
    format_ = format;
    format_->GetData(Tag::AUDIO_CHANNEL_COUNT, avCodecContext_->channels);
    if (avCodecContext_->channels <= 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    format_->GetData(Tag::AUDIO_SAMPLE_RATE, avCodecContext_->sample_rate);
    if (avCodecContext_->sample_rate <= 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    format_->GetData(Tag::MEDIA_BITRATE, avCodecContext_->bit_rate);
    format_->GetData(Tag::AUDIO_MAX_INPUT_SIZE, maxInputSize_);

    AudioChannelLayout channelLayout;
    format_->GetData(Tag::AUDIO_CHANNEL_LAYOUT, channelLayout);
    auto ffChannelLayout =
        FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(static_cast<AudioChannelLayout>(channelLayout));
    avCodecContext_->channel_layout = ffChannelLayout;

    AudioSampleFormat sampleFormat;
    format_->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleFormat);
    auto ffSampleFormat = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(static_cast<AudioSampleFormat>(sampleFormat));
    avCodecContext_->sample_fmt = ffSampleFormat;

    channelsBytesPerSample_ = av_get_bytes_per_sample(ffSampleFormat) * avCodecContext_->channels;
    AVCODEC_LOGI("avcodec name: %{public}s", avCodec_->name);
    return Status::OK;
}

Status FFmpegBaseEncoder::OpenContext()
{
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        auto res = avcodec_open2(avCodecContext_.get(), avCodec_.get(), nullptr);
        if (res != 0) {
            AVCODEC_LOGE("avcodec open error %{public}s", OSAL::AVStrError(res).c_str());
            return Status::ERROR_UNKNOWN;
        }
        codecContextValid_ = true;
    }
    if (avCodecContext_->frame_size <= 0) {
        AVCODEC_LOGE("frame size invalid");
    }
    return Status::OK;
}

Status FFmpegBaseEncoder::ReAllocateContext()
{
    if (!codecContextValid_) {
        AVCODEC_LOGD("Old avcodec context not valid, no need to reallocate");
        return Status::OK;
    }

    AVCodecContext *context = avcodec_alloc_context3(avCodec_.get());
    auto tmpContext = std::shared_ptr<AVCodecContext>(context, [](AVCodecContext *ptr) {
        avcodec_free_context(&ptr);
        avcodec_close(ptr);
    });

    tmpContext->channels = avCodecContext_->channels;
    tmpContext->sample_rate = avCodecContext_->sample_rate;
    tmpContext->bit_rate = avCodecContext_->bit_rate;
    tmpContext->channel_layout = avCodecContext_->channel_layout;
    tmpContext->sample_fmt = avCodecContext_->sample_fmt;

    auto res = avcodec_open2(tmpContext.get(), avCodec_.get(), nullptr);
    if (res != 0) {
        AVCODEC_LOGE("avcodec reopen error %{public}s", OSAL::AVStrError(res).c_str());
        return Status::ERROR_UNKNOWN;
    }
    avCodecContext_ = tmpContext;

    return Status::OK;
}

Status FFmpegBaseEncoder::InitFrame()
{
    cachedFrame_->nb_samples = avCodecContext_->frame_size;
    cachedFrame_->format = avCodecContext_->sample_fmt;
    cachedFrame_->channel_layout = avCodecContext_->channel_layout;
    cachedFrame_->channels = avCodecContext_->channels;
    int ret = av_frame_get_buffer(cachedFrame_.get(), 0);
    if (ret < 0) {
        AVCODEC_LOGE("Get frame buffer failed: %{public}s", OSAL::AVStrError(ret).c_str());
        return Status::ERROR_NO_MEMORY;
    }
    return Status::OK;
}

std::shared_ptr<AVCodecContext> FFmpegBaseEncoder::GetCodecContext() const
{
    return avCodecContext_;
}

int32_t FFmpegBaseEncoder::GetMaxInputSize() const noexcept
{
    return maxInputSize_;
}

void FFmpegBaseEncoder::SetCallback(DataCallback *callback)
{
    dataCallback_ = callback;
}

Status FFmpegBaseEncoder::CloseCtxLocked()
{
    if (avCodecContext_ != nullptr) {
        auto res = avcodec_close(avCodecContext_.get());
        if (res != 0) {
            AVCODEC_LOGE("avcodec close failed: %{public}s", OSAL::AVStrError(res).c_str());
            return Status::ERROR_UNKNOWN;
        }
    }
    return Status::OK;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS