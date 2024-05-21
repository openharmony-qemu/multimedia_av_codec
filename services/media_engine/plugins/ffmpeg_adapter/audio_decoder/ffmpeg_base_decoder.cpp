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
#include "ffmpeg_base_decoder.h"
#include "avcodec_log.h"
#include "avcodec_common.h"
#include "ffmpeg_converter.h"
#include "avcodec_audio_common.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FfmpegBaseDecoder"};
constexpr uint8_t LOGD_FREQUENCY = 5;
constexpr AVSampleFormat DEFAULT_FFMPEG_SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
static std::vector<OHOS::MediaAVCodec::AudioSampleFormat> supportedSampleFormats = {
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_U8,
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE,
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S32LE,
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_F32LE
};
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FfmpegBaseDecoder::FfmpegBaseDecoder()
    : isFirst(true),
      hasExtra_(false),
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
      format_(nullptr),
      needResample_(false),
      destFmt_(AV_SAMPLE_FMT_NONE)
{
}

FfmpegBaseDecoder::~FfmpegBaseDecoder()
{
    AVCODEC_LOGI("FfmpegBaseDecoder deconstructor running.");
    CloseCtxLocked();
    if (avCodecContext_ != nullptr) {
        avCodecContext_.reset();
        avCodecContext_ = nullptr;
    }
}

Status FfmpegBaseDecoder::ProcessSendData(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (avCodecContext_ == nullptr) {
        AVCODEC_LOGE("avCodecContext_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return SendBuffer(inputBuffer);
}

static std::string AVStrError(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}

int32_t FfmpegBaseDecoder::GetMaxInputSize() const noexcept
{
    return maxInputSize_;
}

bool FfmpegBaseDecoder::HasExtraData() const noexcept
{
    return hasExtra_;
}

Status FfmpegBaseDecoder::SendBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    if (!inputBuffer) {
        AVCODEC_LOGE("inputBuffer is nullptr");
        return Status::ERROR_NULL_POINTER;
    }
    if (inputBuffer->flag_ != MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS) {
        auto memory = inputBuffer->memory_;
        uint8_t *ptr = memory->GetAddr();
        int32_t size = memory->GetSize();
        if (size <= 0) {
            AVCODEC_LOGE("send input buffer is less than 0. size:%{public}d", size);
            return Status::ERROR_UNKNOWN;
        }
        avPacket_->size = memory->GetSize();
        avPacket_->data = ptr;
        avPacket_->pts = inputBuffer->pts_;
    } else {
        avPacket_->size = 0;
        avPacket_->data = nullptr;
        avPacket_->pts = inputBuffer->pts_;
    }
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "SendBuffer buffer size:%{public}u,name:%{public}s", avPacket_->size,
                       name_.data());
    auto ret = avcodec_send_packet(avCodecContext_.get(), avPacket_.get());
    av_packet_unref(avPacket_.get());
    if (ret == 0) {
        dataCallback_->OnInputBufferDone(inputBuffer);
        return Status::OK;
    } else if (ret == AVERROR(EAGAIN)) {
        AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "skip frame data no enough, msg:%{public}s", AVStrError(ret).data());
        return Status::ERROR_NOT_ENOUGH_DATA;
    } else if (ret == AVERROR_EOF) {
        dataCallback_->OnInputBufferDone(inputBuffer);
        AVCODEC_LOGW("eos send frame, msg:%{public}s", AVStrError(ret).data());
        return Status::END_OF_STREAM;
    } else if (ret == AVERROR_INVALIDDATA) {
        AVCODEC_LOGE("error msg:%{public}s", AVStrError(ret).data());
        return Status::ERROR_INVALID_DATA;
    } else {
        AVCODEC_LOGE("error msg:%{public}s", AVStrError(ret).data());
        return Status::ERROR_UNKNOWN;
    }
}

Status FfmpegBaseDecoder::ProcessReceiveData(std::shared_ptr<AVBuffer> &outBuffer)
{
    std::lock_guard<std::mutex> l(avMutext_);
    if (!outBuffer) {
        AVCODEC_LOGE("outBuffer is nullptr");
        return Status::ERROR_NULL_POINTER;
    }
    if (avCodecContext_ == nullptr) {
        AVCODEC_LOGE("avCodecContext_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return ReceiveBuffer(outBuffer);
}

void FfmpegBaseDecoder::DisableNeedResample()
{
    needResample_ = 0;
}

Status FfmpegBaseDecoder::ReceiveBuffer(std::shared_ptr<AVBuffer> &outBuffer)
{
    auto ret = avcodec_receive_frame(avCodecContext_.get(), cachedFrame_.get());
    Status status;
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
        dataCallback_->OnOutputBufferDone(outBuffer);
    } else if (ret == AVERROR_EOF) {
        AVCODEC_LOGI("eos received");
        outBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
        avcodec_flush_buffers(avCodecContext_.get());
        status = Status::END_OF_STREAM;
        dataCallback_->OnOutputBufferDone(outBuffer);
    } else if (ret == AVERROR(EAGAIN)) {
        AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "audio decoder not enough data");
        status = Status::ERROR_NOT_ENOUGH_DATA;
    } else {
        AVCODEC_LOGE("audio decoder receive unknow error,ffmpeg error message:%{public}s", AVStrError(ret).data());
        status = Status::ERROR_UNKNOWN;
    }
    av_frame_unref(cachedFrame_.get());
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "end received");
    return status;
}

Status FfmpegBaseDecoder::ConvertPlanarFrame(std::shared_ptr<AVBuffer> &outBuffer)
{
    convertedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *fp) { av_frame_free(&fp); });
    if (convertedFrame_ == nullptr) {
        AVCODEC_LOGE("av_frame_alloc failed");
        return Status::ERROR_NO_MEMORY;
    }
    if (resample_.ConvertFrame(convertedFrame_.get(), cachedFrame_.get()) != Status::OK) {
        AVCODEC_LOGE("convert frame failed");
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status FfmpegBaseDecoder::ReceiveFrameSucc(std::shared_ptr<AVBuffer> &outBuffer)
{
    auto outFrame = cachedFrame_;
    if (needResample_) {
        if (ConvertPlanarFrame(outBuffer) != Status::OK) {
            return Status::ERROR_UNKNOWN;
        }
        outFrame = convertedFrame_;
    }
    auto ioInfoMem = outBuffer->memory_;
    int32_t bytePerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(outFrame->format));
    int32_t outputSize = outFrame->nb_samples * bytePerSample * outFrame->channels;
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "RecvFrameSucc buffer real size:%{public}u,size:%{public}u, name:%{public}s",
                       outputSize, ioInfoMem->GetCapacity(), name_.data());
    if (ioInfoMem->GetCapacity() < outputSize) {
        AVCODEC_LOGE("output buffer size is not enough,output size:%{public}d", outputSize);
        return Status::ERROR_NO_MEMORY;
    }
    ioInfoMem->Write(outFrame->data[0], outputSize, 0);

    if (isFirst) {
        isFirst = false;
        format_->SetData(Tag::AUDIO_SAMPLE_FORMAT,
                         FFMpegConverter::ConvertFFMpegToOHAudioFormat(avCodecContext_->sample_fmt));
        auto layout = FFMpegConverter::ConvertFFToOHAudioChannelLayout(avCodecContext_->channel_layout);
        AVCODEC_LOGI("recode output description,layout:%{public}s",
                     FFMpegConverter::ConvertOHAudioChannelLayoutToString(layout).data());
        format_->SetData(Tag::AUDIO_CHANNEL_LAYOUT, layout);
    }
    outBuffer->pts_ = cachedFrame_->pts;
    ioInfoMem->SetSize(outputSize);
    return Status::ERROR_AGAIN;
}

Status FfmpegBaseDecoder::Reset()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    CloseCtxLocked();
    if (avCodecContext_ != nullptr) {
        avCodecContext_.reset();
        avCodecContext_ = nullptr;
    }
    return Status::OK;
}

Status FfmpegBaseDecoder::Release()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    auto ret = CloseCtxLocked();
    if (avCodecContext_ != nullptr) {
        avCodecContext_.reset();
        avCodecContext_ = nullptr;
    }
    return ret;
}

Status FfmpegBaseDecoder::Flush()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (avCodecContext_ != nullptr) {
        avcodec_flush_buffers(avCodecContext_.get());
    }
    return Status::OK;
}

Status FfmpegBaseDecoder::AllocateContext(const std::string &name)
{
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        avCodec_ = std::shared_ptr<AVCodec>(const_cast<AVCodec *>(avcodec_find_decoder_by_name(name.c_str())),
                                            [](AVCodec *ptr) { (void)ptr; });
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *fp) { av_frame_free(&fp); });
    }
    if (avCodec_ == nullptr) {
        AVCODEC_LOGE("AllocateContext fail,parameter avcodec is nullptr.");
        return Status::ERROR_INVALID_OPERATION;
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
    return Status::OK;
}

Status FfmpegBaseDecoder::InitContext(const std::shared_ptr<Meta> &format)
{
    if (format == nullptr) {
        AVCODEC_LOGI("format is nullptr");
        return Status::ERROR_INVALID_PARAMETER;
    }
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, avCodecContext_->channels);
    if (avCodecContext_->channels <= 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    format->GetData(Tag::AUDIO_SAMPLE_RATE, avCodecContext_->sample_rate);
    if (avCodecContext_->sample_rate <= 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    format->GetData(Tag::MEDIA_BITRATE, avCodecContext_->bit_rate);
    AudioChannelLayout channelLayout = UNKNOWN;
    format->GetData(Tag::AUDIO_CHANNEL_LAYOUT, channelLayout);
    auto ffChannelLayout = FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(channelLayout);
    if (channelLayout != UNKNOWN) {
        if (ffChannelLayout == AV_CH_LAYOUT_NATIVE) {
            AVCODEC_LOGE("the value of channelLayout is not supported");
            return Status::ERROR_INVALID_PARAMETER;
        } else {
            avCodecContext_->channel_layout = ffChannelLayout;
        }
    }
    format->GetData(Tag::AUDIO_MAX_INPUT_SIZE, maxInputSize_);

    Status ret = SetCodecExtradata(format);
    if (ret != Status::OK) {
        return ret;
    }
    if (format_ == nullptr) {
        format_ = std::make_shared<Meta>();
    }
    *format_ = *format;
    avCodecContext_->sample_fmt = AV_SAMPLE_FMT_S16;
    avCodecContext_->request_sample_fmt = avCodecContext_->sample_fmt;
    avCodecContext_->workaround_bugs =
        static_cast<uint32_t>(avCodecContext_->workaround_bugs) | static_cast<uint32_t>(FF_BUG_AUTODETECT);
    avCodecContext_->err_recognition = 1;
    return Status::OK;
}

Status FfmpegBaseDecoder::OpenContext()
{
    avPacket_ = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *ptr) { av_packet_free(&ptr); });
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        auto res = avcodec_open2(avCodecContext_.get(), avCodec_.get(), nullptr);
        if (res != 0) {
            AVCODEC_LOGE("avcodec open error %{public}s", AVStrError(res).c_str());
            return Status::ERROR_UNKNOWN;
        }

        if (InitResample() != Status::OK) {
            return Status::ERROR_UNKNOWN;
        }
    }
    return Status::OK;
}

Status FfmpegBaseDecoder::InitResample()
{
    AVCODEC_LOGI("channels :%{public}" PRId32, avCodecContext_->channels);
    AVCODEC_LOGI("sample_rate :%{public}" PRId32, avCodecContext_->sample_rate);
    AVCODEC_LOGI("bit_rate :%{public}" PRId64, avCodecContext_->bit_rate);
    AVCODEC_LOGI("channel_layout :%{public}" PRId64, avCodecContext_->channel_layout);
    AVCODEC_LOGI("ffmpeg default sample_fmt :%{public}" PRId32, avCodecContext_->sample_fmt);
    AVCODEC_LOGI("need sample_fmt :%{public}" PRId32, destFmt_);
    AVCODEC_LOGI("frameSize :%{public}" PRId32, avCodecContext_->frame_size);
    if (avCodecContext_->sample_fmt != destFmt_) {
        ResamplePara resamplePara;
        resamplePara.channels = static_cast<uint32_t>(avCodecContext_->channels);
        resamplePara.sampleRate = static_cast<uint32_t>(avCodecContext_->sample_rate);
        resamplePara.channelLayout = avCodecContext_->ch_layout;
        resamplePara.destSamplesPerFrame = 0;
        resamplePara.bitsPerSample = 0;
        resamplePara.srcFfFmt = avCodecContext_->sample_fmt;
        resamplePara.destFmt = destFmt_;
        Status ret = resample_.InitSwrContext(resamplePara);
        if (ret != Status::OK) {
            AVCODEC_LOGE("Resmaple init failed.");
            return Status::ERROR_UNKNOWN;
        }
        needResample_ = true;
    }
    return Status::OK;
}

std::shared_ptr<Meta> FfmpegBaseDecoder::GetFormat() const noexcept
{
    return format_;
}

std::shared_ptr<AVCodecContext> FfmpegBaseDecoder::GetCodecContext() const noexcept
{
    return avCodecContext_;
}

std::shared_ptr<AVPacket> FfmpegBaseDecoder::GetCodecAVPacket() const noexcept
{
    return avPacket_;
}

std::shared_ptr<AVFrame> FfmpegBaseDecoder::GetCodecCacheFrame() const noexcept
{
    return cachedFrame_;
}

Status FfmpegBaseDecoder::CloseCtxLocked()
{
    if (avCodecContext_ != nullptr) {
        auto res = avcodec_close(avCodecContext_.get());
        if (res != 0) {
            AVCODEC_LOGE("avcodec close failed, res=%{public}d", res);
            return Status::ERROR_UNKNOWN;
        }
    }
    return Status::OK;
}

void FfmpegBaseDecoder::EnableResample(AVSampleFormat destFmt)
{
    destFmt_ = destFmt;
    AVCODEC_LOGI("enable resample to destFmt:%{public}" PRId32, destFmt);
}

void FfmpegBaseDecoder::SetCallback(DataCallback *callback)
{
    dataCallback_ = callback;
}

bool FfmpegBaseDecoder::CheckSampleFormat(const std::shared_ptr<Meta> &format, int32_t channels)
{
    AudioSampleFormat sampleFormat;
    if (!format->Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat)) {
        AVCODEC_LOGW("Sample format missing, set to default s16le");
        EnableResample(DEFAULT_FFMPEG_SAMPLE_FORMAT);
        return true;
    }
    if (std::find(supportedSampleFormats.begin(), supportedSampleFormats.end(),
                  sampleFormat) == supportedSampleFormats.end()) {
        if (avCodecContext_ == nullptr) {
            AVCODEC_LOGE("avCodecContext_ is nullptr");
            return false;
        }
        auto audioFmt = FFMpegConverter::ConvertFFMpegToOHAudioFormat(avCodecContext_->sample_fmt);
        if (std::find(supportedSampleFormats.begin(), supportedSampleFormats.end(),
                      audioFmt) == supportedSampleFormats.end()) {
            AVCODEC_LOGW("Output sample format not support, change to default S16LE");
            sampleFormat = AudioSampleFormat::SAMPLE_S16LE;
        } else {
            AVCODEC_LOGW("Output sample format not support, change to algorithm default sampleFormat:%{public}" PRId32,
                sampleFormat);
            sampleFormat = audioFmt;
        }
    }
    AVCODEC_LOGI("CheckSampleFormat AudioSampleFormat:%{public}" PRId32, sampleFormat);
    if (channels == 1 && sampleFormat == AudioSampleFormat::SAMPLE_F32LE) {
        return true;
    }
    auto destFmt = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(sampleFormat);
    if (destFmt == AV_SAMPLE_FMT_NONE) {
        AVCODEC_LOGE("Convert format failed, avSampleFormat not found");
        return false;
    }
    EnableResample(destFmt);
    return true;
}

Status FfmpegBaseDecoder::SetCodecExtradata(const std::shared_ptr<Meta> &format)
{
    if (format->GetData(Tag::MEDIA_CODEC_CONFIG, config_data)) {
        AVCODEC_LOGI("Set codec config data size:%{public}zu", config_data.size());
        avCodecContext_->extradata =
            static_cast<uint8_t *>(av_mallocz(config_data.size() + AV_INPUT_BUFFER_PADDING_SIZE));
        if (avCodecContext_->extradata == nullptr) {
            AVCODEC_LOGE("extradata malloc failed!");
            return Status::ERROR_INVALID_PARAMETER;
        }
        avCodecContext_->extradata_size = static_cast<int>(config_data.size());
        errno_t rc = memcpy_s(avCodecContext_->extradata, config_data.size(), config_data.data(), config_data.size());
        if (rc != EOK) {
            AVCODEC_LOGE("extradata memcpy_s failed.");
            return Status::ERROR_INVALID_PARAMETER;
        }
        rc = memset_s(avCodecContext_->extradata + config_data.size(), AV_INPUT_BUFFER_PADDING_SIZE, 0,
                      AV_INPUT_BUFFER_PADDING_SIZE);
        if (rc != EOK) {
            AVCODEC_LOGE("extradata memset_s failed.");
            return Status::ERROR_INVALID_PARAMETER;
        }
        hasExtra_ = true;
    }
    return Status::OK;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
