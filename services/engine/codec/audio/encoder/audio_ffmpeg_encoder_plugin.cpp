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

#include "audio_ffmpeg_encoder_plugin.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "securec.h"
#include "ffmpeg_converter.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegEncoderPlugin"};
}

namespace OHOS {
namespace MediaAVCodec {
AudioFfmpegEncoderPlugin::AudioFfmpegEncoderPlugin()
    : maxInputSize_(-1),
      avCodec_(nullptr),
      avCodecContext_(nullptr),
      cachedFrame_(nullptr),
      avPacket_(nullptr),
      prevPts_(0),
      codecContextValid_(false)
{
}

AudioFfmpegEncoderPlugin::~AudioFfmpegEncoderPlugin()
{
    CloseCtxLocked();
    avCodecContext_.reset();
}

int32_t AudioFfmpegEncoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (avCodecContext_ == nullptr) {
        AVCODEC_LOGE("avCodecContext_ is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION;
    }
    return SendBuffer(inputBuffer);
}

int32_t AudioFfmpegEncoderPlugin::PcmFillFrame(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    auto memory = inputBuffer->GetBuffer();
    auto usedSize = inputBuffer->GetBufferAttr().size;
    auto frameSize = avCodecContext_->frame_size;
    cachedFrame_->nb_samples = static_cast<int>(usedSize / channelsBytesPerSample_);
    AVCODEC_LOGI("sampleRate : %{public}d, frameSize : %{public}d", avCodecContext_->sample_rate, frameSize);
    if (cachedFrame_->nb_samples > frameSize) {
        AVCODEC_LOGE("cachedFrame_->nb_samples is greater than frameSize, please enter a correct frameBytes."
                     "hint: nb_samples is %{public}d. frameSize is %{public}d.", cachedFrame_->nb_samples, frameSize);
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    cachedFrame_->data[0] = memory->GetBase();
    cachedFrame_->extended_data = cachedFrame_->data;
    cachedFrame_->linesize[0] = usedSize;
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegEncoderPlugin::SendBuffer(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    if (!inputBuffer) {
        AVCODEC_LOGE("inputBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    int ret = av_frame_make_writable(cachedFrame_.get());
    if (ret != 0) {
        AVCODEC_LOGE("Frame make writable failed: %{public}s", FFMpegConverter::AVStrError(ret).c_str());
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    auto attr = inputBuffer->GetBufferAttr();
    bool isEos = inputBuffer->CheckIsEos();
    if (!isEos) {
        AVCODEC_LOGD("SendBuffer buffer size:%{public}d", attr.size);
    } else {
        AVCODEC_LOGD("SendBuffer EOS buffer size:%{public}d", attr.size);
    }
    if (!isEos) {
        auto memory = inputBuffer->GetBuffer();
        if (attr.size < 0) {
            AVCODEC_LOGE("SendBuffer buffer size is less than 0. size : %{public}d", attr.size);
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
        if (attr.size > memory->GetSize()) {
            AVCODEC_LOGE("send input buffer is > allocate size. size : %{public}d, allocate size : %{public}d",
                         attr.size, memory->GetSize());
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
        auto errCode = PcmFillFrame(inputBuffer);
        if (errCode != AVCodecServiceErrCode::AVCS_ERR_OK) {
            return errCode;
        }
        ret = avcodec_send_frame(avCodecContext_.get(), cachedFrame_.get());
    } else {
        ret = avcodec_send_frame(avCodecContext_.get(), nullptr);
    }
    if (ret == 0) {
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    } else if (ret == AVERROR(EAGAIN)) {
        AVCODEC_LOGW("skip this frame because not enough, msg:%{public}s", FFMpegConverter::AVStrError(ret).data());
        return AVCodecServiceErrCode::AVCS_ERR_NOT_ENOUGH_DATA;
    } else if (ret == AVERROR_EOF) {
        AVCODEC_LOGW("eos send frame, msg:%{public}s", FFMpegConverter::AVStrError(ret).data());
        return AVCodecServiceErrCode::AVCS_ERR_END_OF_STREAM;
    } else {
        AVCODEC_LOGE("Send frame unknown error: %{public}s", FFMpegConverter::AVStrError(ret).c_str());
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
}

int32_t AudioFfmpegEncoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    if (!outBuffer) {
        AVCODEC_LOGE("outBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    int32_t status;
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        if (avCodecContext_ == nullptr) {
            AVCODEC_LOGE("avCodecContext_ is nullptr");
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION;
        }
        status = ReceiveBuffer(outBuffer);
    }
    return status;
}

int32_t AudioFfmpegEncoderPlugin::ReceiveBuffer(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    (void)memset_s(avPacket_.get(), sizeof(AVPacket), 0, sizeof(AVPacket));
    auto ret = avcodec_receive_packet(avCodecContext_.get(), avPacket_.get());
    int32_t status;
    AVCodecBufferInfo initAttr = {0};
    outBuffer->SetBufferAttr(initAttr);
    if (ret >= 0) {
        AVCODEC_LOGD("receive one packet");
        status = ReceivePacketSucc(outBuffer);
    } else if (ret == AVERROR_EOF) {
        outBuffer->SetEos(true);
        avcodec_flush_buffers(avCodecContext_.get());
        status = AVCodecServiceErrCode::AVCS_ERR_END_OF_STREAM;
    } else if (ret == AVERROR(EAGAIN)) {
        status = AVCodecServiceErrCode::AVCS_ERR_NOT_ENOUGH_DATA;
    } else {
        AVCODEC_LOGE("audio encoder receive unknow error: %{public}s", FFMpegConverter::AVStrError(ret).c_str());
        status = AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    av_packet_unref(avPacket_.get());
    return status;
}

int32_t AudioFfmpegEncoderPlugin::ReceivePacketSucc(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    uint32_t headerSize = 0;
    auto memory = outBuffer->GetBuffer();

    int32_t outputSize = static_cast<int32_t>(avPacket_->size + headerSize);
    if (memory->GetSize() < outputSize) {
        AVCODEC_LOGW("Output buffer capacity is not enough");
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }

    auto len = memory->Write(avPacket_->data, avPacket_->size);
    if (len < avPacket_->size) {
        AVCODEC_LOGE("write packet data failed, len = %{public}d", len);
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    auto attr = outBuffer->GetBufferAttr();
    attr.size = static_cast<size_t>(avPacket_->size + headerSize);
    prevPts_ += avPacket_->duration;
    attr.presentationTimeUs = FFMpegConverter::ConvertAudioPtsToUs(prevPts_, avCodecContext_->time_base);
    outBuffer->SetBufferAttr(attr);
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegEncoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    prevPts_ = 0;
    return ret;
}

int32_t AudioFfmpegEncoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    return ret;
}

int32_t AudioFfmpegEncoderPlugin::Flush()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (avCodecContext_ != nullptr) {
        avcodec_flush_buffers(avCodecContext_.get());
    }
    prevPts_ = 0;
    return ReAllocateContext();
}

int32_t AudioFfmpegEncoderPlugin::AllocateContext(const std::string &name)
{
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        avCodec_ = std::shared_ptr<AVCodec>(const_cast<AVCodec *>(avcodec_find_encoder_by_name(name.c_str())),
                                            [](AVCodec *ptr) {});
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *fp) { av_frame_free(&fp); });
        avPacket_ = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *ptr) { av_packet_free(&ptr); });
    }
    if (avCodec_ == nullptr) {
        return AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT_PROTOCOL_TYPE;
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
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegEncoderPlugin::InitContext(const Format &format)
{
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, avCodecContext_->channels);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, avCodecContext_->sample_rate);
    format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, avCodecContext_->bit_rate);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, maxInputSize_);

    int64_t channelLayout;
    format.GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, channelLayout);
    auto ffChannelLayout =
        FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(static_cast<AudioChannelLayout>(channelLayout));
    avCodecContext_->channel_layout = ffChannelLayout;

    int32_t sampleFormat;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFormat);
    auto ffSampleFormat = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(static_cast<AudioSampleFormat>(sampleFormat));
    avCodecContext_->sample_fmt = ffSampleFormat;
    channelsBytesPerSample_ =
        static_cast<uint32_t>(av_get_bytes_per_sample(ffSampleFormat) * avCodecContext_->channels);
    AVCODEC_LOGI("avcodec name: %{public}s", avCodec_->name);
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegEncoderPlugin::OpenContext()
{
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        auto res = avcodec_open2(avCodecContext_.get(), avCodec_.get(), nullptr);
        if (res != 0) {
            AVCODEC_LOGE("avcodec open error %{public}s", FFMpegConverter::AVStrError(res).c_str());
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
        codecContextValid_ = true;
    }
    if (avCodecContext_->frame_size <= 0) {
        AVCODEC_LOGE("frame size invalid");
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegEncoderPlugin::ReAllocateContext()
{
    if (!codecContextValid_) {
        AVCODEC_LOGD("Old avcodec context not valid, no need to reallocate");
        return AVCodecServiceErrCode::AVCS_ERR_OK;
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
        AVCODEC_LOGE("avcodec reopen error %{public}s", FFMpegConverter::AVStrError(res).c_str());
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    avCodecContext_ = tmpContext;

    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegEncoderPlugin::InitFrame()
{
    cachedFrame_->nb_samples = avCodecContext_->frame_size;
    cachedFrame_->format = avCodecContext_->sample_fmt;
    cachedFrame_->channel_layout = avCodecContext_->channel_layout;
    cachedFrame_->channels = avCodecContext_->channels;
    int ret = av_frame_get_buffer(cachedFrame_.get(), 0);
    if (ret < 0) {
        AVCODEC_LOGE("Get frame buffer failed: %{public}s", FFMpegConverter::AVStrError(ret).c_str());
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

std::shared_ptr<AVCodecContext> AudioFfmpegEncoderPlugin::GetCodecContext() const
{
    return avCodecContext_;
}

int32_t AudioFfmpegEncoderPlugin::GetMaxInputSize() const noexcept
{
    return maxInputSize_;
}

int32_t AudioFfmpegEncoderPlugin::CloseCtxLocked()
{
    if (avCodecContext_ != nullptr) {
        auto res = avcodec_close(avCodecContext_.get());
        if (res != 0) {
            AVCODEC_LOGE("avcodec close failed: %{public}s", FFMpegConverter::AVStrError(res).c_str());
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS
