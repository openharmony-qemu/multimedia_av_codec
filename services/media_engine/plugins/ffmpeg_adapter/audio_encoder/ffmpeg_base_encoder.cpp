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
#include "common/pcm_convert.h"
#include "osal/utils/util.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegBaseEncoder"};
constexpr int32_t NS_PER_US = 1000;
constexpr int32_t MAX_INPUT_BUFFER_SIZE_LIMIT = 10 * 1024 * 1024;
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
}

void FFmpegBaseEncoder::SetFormatConvert(bool enable, AudioSampleFormat srcFormat, AudioSampleFormat dstFormat)
{
    isEnableFormatConvert_ = enable;
    srcSampleFormat_ = srcFormat;
    dstSampleFormat_ = dstFormat;
    if (enable) {
        srcBytesPerSample_ = GetPcmBytesPerSample(srcFormat);
        AVCODEC_LOGI("convert enable, srcFormat:%{public}d, dstFormat:%{public}d, srcBytesPerSample:%{public}d",
            static_cast<int32_t>(srcFormat), static_cast<int32_t>(dstFormat), srcBytesPerSample_);
    }
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

    std::lock_guard<std::mutex> lock(avMutext_);
    if (avCodecContext_ == nullptr) {
        return Status::ERROR_WRONG_STATE;
    }
    if (!enableCache_) {
        ret = SendBuffer(inputBuffer);
    } else {
        bool isEos = inputBuffer->flag_ & BUFFER_FLAG_EOS;
        if (!isEos && ptsMode_ == FIRST_INPUT_START_ENCODE_PTS_MODE && isFirstInputPts_) {
            prevPts_ = inputBuffer->pts_;
            isFirstInputPts_ = false;
        }
        if (isEos) {
            ret = SendEosBuffer();
        } else {
            if (isEnableFormatConvert_ && srcSampleFormat_ != dstSampleFormat_) {
                size_t srcBufferSize = static_cast<size_t>(memory->GetSize());
                size_t inputSampleCnt = srcBufferSize / static_cast<size_t>(srcBytesPerSample_);
                size_t dstBufferSize = GetPcmConvertOutputSize(inputSampleCnt, dstSampleFormat_);
                if (dstBufferSize == 0) {
                    AVCODEC_LOGE("GetPcmConvertOutputSize failed");
                    return Status::ERROR_INVALID_PARAMETER;
                }
                convertBuffer_.resize(dstBufferSize);
                ConvertPcmSampleFormat(memory->GetAddr(), inputSampleCnt,
                    srcSampleFormat_, dstSampleFormat_, convertBuffer_.data());
                pcmCache_.insert(pcmCache_.end(), convertBuffer_.begin(), convertBuffer_.end());
            } else {
                pcmCache_.insert(pcmCache_.end(), memory->GetAddr(),
                                 memory->GetAddr() + memory->GetSize());
            }
            ret = SendCachedFrames();
        }
    }
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        std::lock_guard<std::mutex> l(bufferMetaMutex_);
        if (inputBuffer->meta_ == nullptr) {
            AVCODEC_LOGE("encoder input buffer or meta is nullptr");
            return Status::ERROR_INVALID_DATA;
        }
        bufferMeta_ = inputBuffer->meta_;
        SafeCallInputBufferDone(dataCallback_, inputBuffer);
        ret = Status::OK;
    }

    return ret;
}

Status FFmpegBaseEncoder::PcmFillFrame(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    auto memory = inputBuffer->memory_;
    auto usedSize = memory->GetSize();
    auto frameSize = avCodecContext_->frame_size;

    if (isEnableFormatConvert_ && srcSampleFormat_ != dstSampleFormat_) {
        size_t srcBufferSize = static_cast<size_t>(usedSize);
        size_t inputSampleCnt = srcBufferSize / static_cast<size_t>(srcBytesPerSample_);
        size_t dstBufferSize = GetPcmConvertOutputSize(inputSampleCnt, dstSampleFormat_);
        if (dstBufferSize == 0) {
            AVCODEC_LOGE("GetPcmConvertOutputSize failed");
            return Status::ERROR_INVALID_PARAMETER;
        }
        convertBuffer_.resize(dstBufferSize);
        Status ret = ConvertPcmSampleFormat(memory->GetAddr(), inputSampleCnt,
            srcSampleFormat_, dstSampleFormat_, convertBuffer_.data());
        if (ret != Status::OK) {
            AVCODEC_LOGE("ConvertPcmSampleFormat failed");
            return ret;
        }
        cachedFrame_->nb_samples = static_cast<int>(dstBufferSize / static_cast<size_t>(channelsBytesPerSample_));
        cachedFrame_->data[0] = convertBuffer_.data();
        cachedFrame_->linesize[0] = static_cast<int>(dstBufferSize);
    } else {
        cachedFrame_->nb_samples = usedSize / channelsBytesPerSample_;
        cachedFrame_->data[0] = memory->GetAddr();
        cachedFrame_->linesize[0] = usedSize;
    }

    AVCODEC_LOGI("sampleRate : %{public}d, frameSize : %{public}d", avCodecContext_->sample_rate, frameSize);
    if (cachedFrame_->nb_samples > frameSize) {
        AVCODEC_LOGE("cachedFrame_->nb_samples is greater than frameSize, please enter a correct frameBytes."
                     "hint: nb_samples is %{public}d. frameSize is %{public}d.", cachedFrame_->nb_samples, frameSize);
        return Status::ERROR_UNKNOWN;
    }
    cachedFrame_->extended_data = cachedFrame_->data;
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
    if (!isEos && ptsMode_ == FIRST_INPUT_START_ENCODE_PTS_MODE && isFirstInputPts_) {
        prevPts_ = inputBuffer->pts_;
        isFirstInputPts_ = false;
    }
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
    Status status = Status::ERROR_UNKNOWN;
    (void)memset_s(avPacket_.get(), sizeof(AVPacket), 0, sizeof(AVPacket));
    auto ret = avcodec_receive_packet(avCodecContext_.get(), avPacket_.get());
    if (ret >= 0) {
        status = ReceivePacketSucc(outputBuffer);
        if (enableCache_ && pcmCache_.size() >= frameBytes_) {
            SendCachedFrames();
        }
    } else if (ret == AVERROR_EOF) {
        AVCODEC_LOGI("eos received");
        outputBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
        avcodec_flush_buffers(avCodecContext_.get());
        status = Status::END_OF_STREAM;
    } else if (ret == AVERROR(EAGAIN)) {
        status = Status::ERROR_NOT_ENOUGH_DATA;
    } else {
        AVCODEC_LOGE("audio encoder receive unknow error: %{public}s", OSAL::AVStrError(ret).c_str());
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
    // pts us
    outputBuffer->duration_ = ConvertTimeFromFFmpeg(avPacket_->duration, avCodecContext_->time_base) /
	                          NS_PER_US;
    int64_t durationUs = outputBuffer->duration_;
    outputBuffer->pts_ = prevPts_;
    if (prevPts_ < 0) {
        prevPts_ += durationUs;
    } else {
        prevPts_ = (INT64_MAX - prevPts_ < durationUs) ? (durationUs - (INT64_MAX - prevPts_)) :
                   (prevPts_ + durationUs);
    }
    outputBuffer->flag_ &= (~static_cast<uint32_t>(BUFFER_FLAG_EOS));
    return Status::ERROR_AGAIN;
}

Status FFmpegBaseEncoder::SendOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    Status status = ReceiveBuffer(outputBuffer);
    if (status == Status::OK || status == Status::END_OF_STREAM || status == Status::ERROR_AGAIN) {
        {
            std::lock_guard<std::mutex> l(bufferMetaMutex_);
            outBuffer_->meta_ = bufferMeta_;
        }
        SafeCallOutputBufferDone(dataCallback_, outputBuffer);
    }
    return status;
}

Status FFmpegBaseEncoder::Stop()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    auto ret = CloseCtxLocked();
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
    prevPts_ = 0;
    ptsMode_ = DEFAULT_ENCODE_PTS_MODE;
    isFirstInputPts_ = true;
    pcmCache_.clear();
    convertBuffer_.clear();
    maxInputBufferSize_ = -1;
    enableCache_ = false;
    isEnableFormatConvert_ = false;
    srcSampleFormat_ = AudioSampleFormat::INVALID_WIDTH;
    dstSampleFormat_ = AudioSampleFormat::INVALID_WIDTH;
    return ret;
}

Status FFmpegBaseEncoder::Release()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    auto ret = CloseCtxLocked();
    return ret;
}

Status FFmpegBaseEncoder::Flush()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (avCodecContext_ != nullptr) {
        avcodec_flush_buffers(avCodecContext_.get());
    }
    prevPts_ = 0;
    isFirstInputPts_ = true;
    pcmCache_.clear();
    convertBuffer_.clear();
    return ReAllocateContext();
}

Status FFmpegBaseEncoder::AllocateContext(const std::string &name)
{
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        avCodec_ = std::shared_ptr<AVCodec>(const_cast<AVCodec *>(avcodec_find_encoder_by_name(name.c_str())),
            [](AVCodec *p) {
                AVCODEC_LOGI("free avCodec_");
            });
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *fp) { av_frame_free(&fp); });
        avPacket_ = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *ptr) { av_packet_free(&ptr); });
    }
    CHECK_AND_RETURN_RET_LOG(avCodec_ != nullptr,
        Status::ERROR_INVALID_OPERATION, "AllocateContext failed %{public}s", name.c_str());

    AVCodecContext *context = nullptr;
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        context = avcodec_alloc_context3(avCodec_.get());
        avCodecContext_ = std::shared_ptr<AVCodecContext>(context, [](AVCodecContext *ptr) {
            if (ptr) {
                avcodec_free_context(&ptr);
                ptr = nullptr;
            }
        });
        CHECK_AND_RETURN_RET_LOG(avCodecContext_ != nullptr, Status::ERROR_NO_MEMORY,
            "Allocate avCodecContext_ failed.");
        av_log_set_level(AV_LOG_ERROR);
    }
    return Status::OK;
}

Status FFmpegBaseEncoder::InitContext(const std::shared_ptr<Meta> &format)
{
    format_ = format;
    int32_t channels = 0;
    format_->GetData(Tag::AUDIO_CHANNEL_COUNT, channels);
    format_->GetData(Tag::AUDIO_SAMPLE_RATE, avCodecContext_->sample_rate);
    format_->GetData(Tag::MEDIA_BITRATE, avCodecContext_->bit_rate);
    format_->GetData(Tag::AUDIO_MAX_INPUT_SIZE, maxInputSize_);
    maxInputBufferSize_ = -1;
    enableCache_ = false;
    format_->GetData(Tag::AUDIO_MAX_INPUT_BUFFER_SIZE, maxInputBufferSize_);
    if (maxInputBufferSize_ > 0) {
        if (maxInputBufferSize_ > MAX_INPUT_BUFFER_SIZE_LIMIT) {
            maxInputBufferSize_ = MAX_INPUT_BUFFER_SIZE_LIMIT;
        }
        enableCache_ = true;
    }

    AudioChannelLayout channelLayout = UNKNOWN;
    format_->GetData(Tag::AUDIO_CHANNEL_LAYOUT, channelLayout);
    auto ffChannelLayout =
        FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(static_cast<AudioChannelLayout>(channelLayout));
    if (channelLayout != UNKNOWN) {
        if (av_channel_layout_from_mask(&avCodecContext_->ch_layout, ffChannelLayout)) {
            AVCODEC_LOGW("the value of channelLayout is not supported.");
            av_channel_layout_default(&avCodecContext_->ch_layout, channels);
        }
    } else {
        AVCODEC_LOGW("channelLayout not set, unknow channelLayout.");
        av_channel_layout_default(&avCodecContext_->ch_layout, channels);
    }

    AudioSampleFormat sampleFormat;
    format_->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleFormat);
    // If format conversion is enabled, use the converted format for FFmpeg context
    AudioSampleFormat effectiveFormat = (isEnableFormatConvert_ && dstSampleFormat_ != AudioSampleFormat::INVALID_WIDTH)
        ? dstSampleFormat_ : sampleFormat;
    auto ffSampleFormat = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(
        static_cast<AudioSampleFormat>(effectiveFormat));
    avCodecContext_->sample_fmt = ffSampleFormat;

    channelsBytesPerSample_ = av_get_bytes_per_sample(ffSampleFormat) * channels;
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
        return Status::ERROR_UNKNOWN;
    }
    frameBytes_ = static_cast<uint32_t>(avCodecContext_->frame_size * channelsBytesPerSample_);
    AVCODEC_LOGI("flac encode frameBytes_:%{public}d", static_cast<int32_t>(frameBytes_));
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
        if (ptr) {
            avcodec_free_context(&ptr);
            ptr = nullptr;
        }
    });

    CHECK_AND_RETURN_RET_LOG(tmpContext != nullptr, Status::ERROR_NO_MEMORY,
        "Allocate tmpContext failed.");

    tmpContext->sample_rate = avCodecContext_->sample_rate;
    tmpContext->bit_rate = avCodecContext_->bit_rate;
    tmpContext->sample_fmt = avCodecContext_->sample_fmt;
    int ret = av_channel_layout_copy(&tmpContext->ch_layout, &avCodecContext_->ch_layout);
    CHECK_AND_RETURN_RET_LOG(!ret, Status::ERROR_UNKNOWN, "av_channel_layout_copy failed.");

    auto res = avcodec_open2(tmpContext.get(), avCodec_.get(), nullptr);
    if (res != 0) {
        AVCODEC_LOGE("avcodec reopen error %{public}s", OSAL::AVStrError(res).c_str());
        return Status::ERROR_UNKNOWN;
    }
    avCodecContext_ = tmpContext;
    frameBytes_ = static_cast<uint32_t>(avCodecContext_->frame_size * channelsBytesPerSample_);
    AVCODEC_LOGI("flac encode frameBytes_:%{public}d", static_cast<int32_t>(frameBytes_));
    return Status::OK;
}

Status FFmpegBaseEncoder::InitFrame()
{
    cachedFrame_->nb_samples = avCodecContext_->frame_size;
    cachedFrame_->format = avCodecContext_->sample_fmt;
    int res = av_channel_layout_copy(&cachedFrame_->ch_layout, &avCodecContext_->ch_layout);
    CHECK_AND_RETURN_RET_LOG(!res, Status::ERROR_UNKNOWN, "av_channel_layout_copy failed.");
    int ret = av_frame_get_buffer(cachedFrame_.get(), 0);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, Status::ERROR_NO_MEMORY,
        "Get frame buffer failed: %{public}s", OSAL::AVStrError(ret).c_str());
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

int32_t FFmpegBaseEncoder::GetMaxInputBufferSize() const noexcept
{
    return maxInputBufferSize_;
}

void FFmpegBaseEncoder::SetCallback(DataCallback *callback)
{
    dataCallback_ = callback;
}

Status FFmpegBaseEncoder::FillFrameFromCache(bool isEos)
{
    if (!isEos && pcmCache_.size() < frameBytes_) {
        return Status::ERROR_UNKNOWN;
    }
    if (isEos && pcmCache_.size() <= 0) {
        return Status::OK;
    }
    uint32_t readBytes = isEos ? pcmCache_.size() : frameBytes_;
    int ret = av_frame_make_writable(cachedFrame_.get());
    if (ret != 0) {
        AVCODEC_LOGE("FillFrameFromCache error:%{public}d", ret);
        return Status::ERROR_UNKNOWN;
    }
    errno_t err = memcpy_s(cachedFrame_->data[0], readBytes, pcmCache_.data(), readBytes);
    if (err != EOK) {
        AVCODEC_LOGE("FillFrameFromCache memcpy error");
        return Status::ERROR_UNKNOWN;
    }
    pcmCache_.erase(pcmCache_.begin(), pcmCache_.begin() + readBytes);
    cachedFrame_->nb_samples = static_cast<int>(readBytes) / channelsBytesPerSample_;
    cachedFrame_->extended_data = cachedFrame_->data;
    cachedFrame_->linesize[0] = static_cast<int>(readBytes);
    return Status::OK;
}

Status FFmpegBaseEncoder::SendCachedFrames()
{
    if (pcmCache_.size() >= frameBytes_) {
        if (FillFrameFromCache() != Status::OK) {
            return Status::ERROR_UNKNOWN;
        }
        auto ret = avcodec_send_frame(avCodecContext_.get(), cachedFrame_.get());
        if (ret == AVERROR(EAGAIN)) {
            AVCODEC_LOGD("avcodec_send_frame return EAGAIN");
            return Status::OK;
        }
        if (ret != 0) {
            AVCODEC_LOGE("avcodec_send_frame error:%{public}d", static_cast<int32_t>(ret));
            return Status::ERROR_UNKNOWN;
        }
    }
    return Status::OK;
}

Status FFmpegBaseEncoder::SendEosBuffer()
{
    if (pcmCache_.size() > 0) {
        if (FillFrameFromCache(true) != Status::OK) {
            return Status::ERROR_UNKNOWN;
        }
        auto ret = avcodec_send_frame(avCodecContext_.get(), cachedFrame_.get());
        if (ret != 0 && ret != AVERROR(EAGAIN)) {
            AVCODEC_LOGE("SendEosBuffer avcodec_send_frame error:%{public}d", static_cast<int32_t>(ret));
        }
    }
    pcmCache_.clear();
    auto ret = avcodec_send_frame(avCodecContext_.get(), nullptr);
    if (ret != 0 && ret != AVERROR(EAGAIN)) {
        AVCODEC_LOGW("Send EOS frame failed: %{public}s", OSAL::AVStrError(ret).c_str());
    }
    return Status::OK;
}

void FFmpegBaseEncoder::SetPtsMode(int32_t mode)
{
    ptsMode_ = mode;
}

Status FFmpegBaseEncoder::CloseCtxLocked()
{
    if (avCodecContext_ != nullptr) {
        avCodecContext_.reset();
    }
    return Status::OK;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
