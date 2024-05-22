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

#include "audio_ffmpeg_decoder_plugin.h"
#include "avcodec_trace.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "media_description.h"
#include "ffmpeg_converter.h"
#include "securec.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFfmpegDecoderPlugin"};
constexpr uint8_t LOGD_FREQUENCY = 5;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
AudioFfmpegDecoderPlugin::AudioFfmpegDecoderPlugin()
    : hasExtra_(false),
      maxInputSize_(-1),
      bufferNum_(1),
      bufferIndex_(1),
      preBufferGroupPts_(0),
      curBufferGroupPts_(0),
      bufferGroupPtsDistance(0),
      avCodec_(nullptr),
      avCodecContext_(nullptr),
      cachedFrame_(nullptr),
      avPacket_(nullptr),
      resample_(nullptr),
      needResample_(false),
      destFmt_(AV_SAMPLE_FMT_NONE)
{
}

AudioFfmpegDecoderPlugin::~AudioFfmpegDecoderPlugin()
{
    AVCODEC_LOGI("AudioFfmpegDecoderPlugin deconstructor running.");
    CloseCtxLocked();
    if (avCodecContext_ != nullptr) {
        avCodecContext_.reset();
        avCodecContext_ = nullptr;
    }
}

int32_t AudioFfmpegDecoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (avCodecContext_ == nullptr) {
        AVCODEC_LOGE("avCodecContext_ is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION;
    }
    return SendBuffer(inputBuffer);
}

static std::string AVStrError(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}

int32_t AudioFfmpegDecoderPlugin::GetMaxInputSize() const noexcept
{
    return maxInputSize_;
}

bool AudioFfmpegDecoderPlugin::HasExtraData() const noexcept
{
    return hasExtra_;
}

int32_t AudioFfmpegDecoderPlugin::SendBuffer(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    if (!inputBuffer) {
        AVCODEC_LOGE("inputBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    auto attr = inputBuffer->GetBufferAttr();
    if (!inputBuffer->CheckIsEos()) {
        auto memory = inputBuffer->GetBuffer();
        const uint8_t *ptr = memory->GetBase();
        if (attr.size <= 0) {
            AVCODEC_LOGE("send input buffer is less than 0. size:%{public}d", attr.size);
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
        if (attr.size > memory->GetSize()) {
            AVCODEC_LOGE("send input buffer is > allocate size. size:%{public}d,allocate size:%{public}d", attr.size,
                         memory->GetSize());
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
        avPacket_->size = attr.size;
        avPacket_->data = const_cast<uint8_t *>(ptr);
        avPacket_->pts = attr.presentationTimeUs;
    } else {
        avPacket_->size = 0;
        avPacket_->data = nullptr;
        avPacket_->pts = attr.presentationTimeUs;
    }
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "SendBuffer buffer size:%{public}u,name:%{public}s", attr.size, name_.data());
    auto ret = avcodec_send_packet(avCodecContext_.get(), avPacket_.get());
    av_packet_unref(avPacket_.get());
    if (ret == 0) {
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    } else if (ret == AVERROR(EAGAIN)) {
        AVCODEC_LOGW("skip this frame because data not enough, msg:%{public}s", AVStrError(ret).data());
        return AVCodecServiceErrCode::AVCS_ERR_NOT_ENOUGH_DATA;
    } else if (ret == AVERROR_EOF) {
        AVCODEC_LOGW("eos send frame, msg:%{public}s", AVStrError(ret).data());
        return AVCodecServiceErrCode::AVCS_ERR_END_OF_STREAM;
    } else if (ret == AVERROR_INVALIDDATA) {
        AVCODEC_LOGE("ffmpeg error message, msg:%{public}s", AVStrError(ret).data());
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_DATA;
    } else {
        AVCODEC_LOGE("ffmpeg error message, msg:%{public}s", AVStrError(ret).data());
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
}

int32_t AudioFfmpegDecoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    std::lock_guard<std::mutex> l(avMutext_);
    if (!outBuffer) {
        AVCODEC_LOGE("outBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    if (avCodecContext_ == nullptr) {
        AVCODEC_LOGE("avCodecContext_ is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION;
    }
    return ReceiveBuffer(outBuffer);
}

void AudioFfmpegDecoderPlugin::DisableNeedResample()
{
    needResample_ = 0;
}

int32_t AudioFfmpegDecoderPlugin::ReceiveBuffer(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    auto ret = avcodec_receive_frame(avCodecContext_.get(), cachedFrame_.get());
    int32_t status;
    AVCodecBufferInfo initAttr = {0};
    outBuffer->SetBufferAttr(initAttr);
    if (ret >= 0) {
        AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "receive one frame");
        if (cachedFrame_->pts != AV_NOPTS_VALUE) {
            preBufferGroupPts_ = curBufferGroupPts_;
            curBufferGroupPts_ = cachedFrame_->pts;
            if (bufferGroupPtsDistance == 0) {
                bufferGroupPtsDistance = abs(curBufferGroupPts_ - preBufferGroupPts_);
            }
            if (bufferIndex_ >= bufferNum_) {
                bufferNum_ = bufferIndex_;
            }
            bufferIndex_ = 1;
        } else {
            bufferIndex_++;
            if (abs(curBufferGroupPts_ - preBufferGroupPts_) > bufferGroupPtsDistance) {
                cachedFrame_->pts = curBufferGroupPts_;
                preBufferGroupPts_ = curBufferGroupPts_;
            } else {
                cachedFrame_->pts =
                    curBufferGroupPts_ + abs(curBufferGroupPts_ - preBufferGroupPts_) * (bufferIndex_ - 1) / bufferNum_;
            }
        }
        status = ReceiveFrameSucc(outBuffer);
    } else if (ret == AVERROR_EOF) {
        AVCODEC_LOGI("eos received");
        outBuffer->SetEos(true);
        avcodec_flush_buffers(avCodecContext_.get());
        status = AVCodecServiceErrCode::AVCS_ERR_END_OF_STREAM;
    } else if (ret == AVERROR(EAGAIN)) {
        AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "audio decoder not enough data");
        status = AVCodecServiceErrCode::AVCS_ERR_NOT_ENOUGH_DATA;
    } else {
        AVCODEC_LOGE("audio decoder receive unknow error,ffmpeg error message:%{public}s", AVStrError(ret).data());
        status = AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    av_frame_unref(cachedFrame_.get());
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "end received");
    return status;
}

int32_t AudioFfmpegDecoderPlugin::ConvertPlanarFrame(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    convertedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *fp) { av_frame_free(&fp); });
    if (convertedFrame_ == nullptr) {
        AVCODEC_LOGE("av_frame_alloc failed");
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }
    if (resample_->ConvertFrame(convertedFrame_.get(), cachedFrame_.get()) != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("convert frame failed");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegDecoderPlugin::ReceiveFrameSucc(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    auto outFrame = cachedFrame_;
    if (needResample_) {
        if (ConvertPlanarFrame(outBuffer) != AVCodecServiceErrCode::AVCS_ERR_OK) {
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
        outFrame = convertedFrame_;
    }
    auto ioInfoMem = outBuffer->GetBuffer();
    int32_t bytePerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(outFrame->format));
    int32_t outputSize = outFrame->nb_samples * bytePerSample * outFrame->channels;
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "ReceiveFrameSucc buffer real size:%{public}u,size:%{public}u, name:%{public}s",
                       outputSize, ioInfoMem->GetSize(), name_.data());
    if (ioInfoMem->GetSize() < outputSize) {
        AVCODEC_LOGE("output buffer size is not enough,output size:%{public}d", outputSize);
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }

    ioInfoMem->Write(outFrame->data[0], outputSize);

    if (outBuffer->CheckIsFirstFrame()) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE,
                            FFMpegConverter::ConvertFFMpegToOHAudioFormat(avCodecContext_->sample_fmt));
        auto layout = FFMpegConverter::ConvertFFToOHAudioChannelLayout(avCodecContext_->channel_layout);
        AVCODEC_LOGI("recode output description,layout:%{public}s",
                     FFMpegConverter::ConvertOHAudioChannelLayoutToString(layout).data());
        format_.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, static_cast<uint64_t>(layout));
    }
    auto attr = outBuffer->GetBufferAttr();
    attr.presentationTimeUs = static_cast<uint64_t>(cachedFrame_->pts);
    attr.size = outputSize;
    outBuffer->SetBufferAttr(attr);
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegDecoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    CloseCtxLocked();
    if (avCodecContext_ != nullptr) {
        avCodecContext_.reset();
        avCodecContext_ = nullptr;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegDecoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    auto ret = CloseCtxLocked();
    if (avCodecContext_ != nullptr) {
        avCodecContext_.reset();
        avCodecContext_ = nullptr;
    }
    return ret;
}

int32_t AudioFfmpegDecoderPlugin::Flush()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (avCodecContext_ != nullptr) {
        avcodec_flush_buffers(avCodecContext_.get());
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegDecoderPlugin::AllocateContext(const std::string &name)
{
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        avCodec_ = std::shared_ptr<AVCodec>(const_cast<AVCodec *>(avcodec_find_decoder_by_name(name.c_str())),
                                            [](AVCodec *ptr) { (void)ptr; });
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *fp) { av_frame_free(&fp); });
    }
    if (avCodec_ == nullptr) {
        AVCODEC_LOGE("AllocateContext fail,parameter avcodec is nullptr.");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION;
    }
    name_ = name;
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

int32_t AudioFfmpegDecoderPlugin::InitContext(const Format &format)
{
    format_ = format;
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, avCodecContext_->channels);
    if (avCodecContext_->channels <= 0) {
        return AVCodecServiceErrCode::AVCS_ERR_CONFIGURE_MISMATCH_CHANNEL_COUNT;
    }
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, avCodecContext_->sample_rate);
    if (avCodecContext_->sample_rate <= 0) {
        return AVCodecServiceErrCode::AVCS_ERR_MISMATCH_SAMPLE_RATE;
    }
    format_.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, avCodecContext_->bit_rate);
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, maxInputSize_);

    int32_t status = SetCodecExtradata();
    if (status != AVCodecServiceErrCode::AVCS_ERR_OK) {
        return status;
    }

    avCodecContext_->sample_fmt = AV_SAMPLE_FMT_S16;
    avCodecContext_->request_sample_fmt = avCodecContext_->sample_fmt;
    avCodecContext_->workaround_bugs =
        static_cast<uint32_t>(avCodecContext_->workaround_bugs) | static_cast<uint32_t>(FF_BUG_AUTODETECT);
    avCodecContext_->err_recognition = 1;
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegDecoderPlugin::OpenContext()
{
    avPacket_ = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *ptr) { av_packet_free(&ptr); });
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        auto res = avcodec_open2(avCodecContext_.get(), avCodec_.get(), nullptr);
        if (res != 0) {
            AVCODEC_LOGE("avcodec open error %{public}s", AVStrError(res).c_str());
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }

        if (InitResample() != AVCodecServiceErrCode::AVCS_ERR_OK) {
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFfmpegDecoderPlugin::InitResample()
{
    if (needResample_) {
        ResamplePara resamplePara = {
            .channels = avCodecContext_->channels,
            .sampleRate = avCodecContext_->sample_rate,
            .bitsPerSample = 0,
            .channelLayout = avCodecContext_->ch_layout,
            .srcFmt = avCodecContext_->sample_fmt,
            .destSamplesPerFrame = 0,
            .destFmt = destFmt_,
        };
        convertedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *fp) { av_frame_free(&fp); });
        resample_ = std::make_shared<AudioResample>();
        if (resample_->InitSwrContext(resamplePara) != AVCodecServiceErrCode::AVCS_ERR_OK) {
            AVCODEC_LOGE("Resample init failed.");
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

Format AudioFfmpegDecoderPlugin::GetFormat() const noexcept
{
    return format_;
}

std::shared_ptr<AVCodecContext> AudioFfmpegDecoderPlugin::GetCodecContext() const noexcept
{
    return avCodecContext_;
}

std::shared_ptr<AVPacket> AudioFfmpegDecoderPlugin::GetCodecAVPacket() const noexcept
{
    return avPacket_;
}

std::shared_ptr<AVFrame> AudioFfmpegDecoderPlugin::GetCodecCacheFrame() const noexcept
{
    return cachedFrame_;
}

int32_t AudioFfmpegDecoderPlugin::CloseCtxLocked()
{
    if (avCodecContext_ != nullptr) {
        auto res = avcodec_close(avCodecContext_.get());
        if (res != 0) {
            AVCODEC_LOGE("avcodec close failed, res=%{public}d", res);
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

void AudioFfmpegDecoderPlugin::EnableResample(AVSampleFormat destFmt)
{
    needResample_ = true;
    destFmt_ = destFmt;
}

int32_t AudioFfmpegDecoderPlugin::SetCodecExtradata()
{
    size_t extraSize;
    uint8_t *extraData;
    if (format_.GetBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, &extraData, extraSize)) {
        avCodecContext_->extradata = static_cast<uint8_t *>(av_mallocz(extraSize + AV_INPUT_BUFFER_PADDING_SIZE));
        if (avCodecContext_->extradata == nullptr) {
            AVCODEC_LOGE("extradata malloc failed!");
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
        }
        avCodecContext_->extradata_size = static_cast<int>(extraSize);
        errno_t rc = memcpy_s(avCodecContext_->extradata, extraSize, extraData, extraSize);
        if (rc != EOK) {
            AVCODEC_LOGE("extradata memcpy_s failed.");
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
        }
        rc = memset_s(avCodecContext_->extradata + extraSize, AV_INPUT_BUFFER_PADDING_SIZE, 0,
                      AV_INPUT_BUFFER_PADDING_SIZE);
        if (rc != EOK) {
            AVCODEC_LOGE("extradata memset_s failed.");
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
        }
        hasExtra_ = true;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS
