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

#include "audio_ffmpeg_aac_encoder_plugin.h"
#include <set>
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "media_description.h"
#include "avcodec_mime_type.h"
#include "ffmpeg_converter.h"
#include "avcodec_audio_common.h"
#include "securec.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegAacEncoderPlugin"};
constexpr std::string_view AUDIO_CODEC_NAME = "aac";
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 4 * 1024 * 8;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr uint32_t ADTS_HEADER_SIZE = 7;
constexpr uint8_t SAMPLE_FREQUENCY_INDEX_DEFAULT = 4;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
constexpr int32_t INVALID_CHANNELS = 7;
static std::map<int32_t, uint8_t> sampleFreqMap = {{96000, 0},  {88200, 1}, {64000, 2}, {48000, 3}, {44100, 4},
                                                   {32000, 5},  {24000, 6}, {22050, 7}, {16000, 8}, {12000, 9},
                                                   {11025, 10}, {8000, 11}, {7350, 12}};
static std::set<OHOS::MediaAVCodec::AudioSampleFormat> supportedSampleFormats = {
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE,
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_F32LE,
};
static std::map<int32_t, int64_t> channelLayoutMap = {{1, AV_CH_LAYOUT_MONO},
                                                      {2, AV_CH_LAYOUT_STEREO},
                                                      {3, AV_CH_LAYOUT_SURROUND},
                                                      {4, AV_CH_LAYOUT_4POINT0},
                                                      {5, AV_CH_LAYOUT_5POINT0_BACK},
                                                      {6, AV_CH_LAYOUT_5POINT1_BACK},
                                                      {7, AV_CH_LAYOUT_7POINT0},
                                                      {8, AV_CH_LAYOUT_7POINT1}};
}

namespace OHOS {
namespace MediaAVCodec {
AudioFFMpegAacEncoderPlugin::AudioFFMpegAacEncoderPlugin()
    : maxInputSize_(-1), avCodec_(nullptr), avCodecContext_(nullptr), cachedFrame_(nullptr), avPacket_(nullptr),
      prevPts_(0), resample_(nullptr), needResample_(false), srcFmt_(AVSampleFormat::AV_SAMPLE_FMT_NONE),
      srcLayout_(-1), codecContextValid_(false)
{}

AudioFFMpegAacEncoderPlugin::~AudioFFMpegAacEncoderPlugin()
{
    Release();
}

int32_t AudioFFMpegAacEncoderPlugin::GetAdtsHeader(std::string &adtsHeader, int32_t &headerSize,
                                                   std::shared_ptr<AVCodecContext> ctx, int aacLength)
{
    uint8_t freqIdx = SAMPLE_FREQUENCY_INDEX_DEFAULT; // 0: 96000 Hz  3: 48000 Hz 4: 44100 Hz
    auto iter = sampleFreqMap.find(ctx->sample_rate);
    if (iter != sampleFreqMap.end()) {
        freqIdx = iter->second;
    }
    uint8_t chanCfg = static_cast<uint8_t>(ctx->channels);
    uint32_t frameLength = static_cast<uint32_t>(aacLength) + ADTS_HEADER_SIZE;
    uint8_t profile = static_cast<uint8_t>(ctx->profile);
    adtsHeader += 0xFF;
    adtsHeader += 0xF1;
    adtsHeader += (profile << 0x6) + (freqIdx << 0x2) + (chanCfg >> 0x2);
    adtsHeader += (((chanCfg & 0x3) << 0x6) + (frameLength >> 0xB));
    adtsHeader += ((frameLength & 0x7FF) >> 0x3);
    adtsHeader += (((frameLength & 0x7) << 0x5) + 0x1F);
    adtsHeader += 0xFC;
    headerSize = ADTS_HEADER_SIZE;

    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

bool AudioFFMpegAacEncoderPlugin::CheckSampleRate(const int sampleRate)
{
    return sampleFreqMap.find(sampleRate) != sampleFreqMap.end() ? true : false;
}

bool AudioFFMpegAacEncoderPlugin::CheckSampleFormat(const Format &format)
{
    int32_t sampleFormat;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFormat);
    if (supportedSampleFormats.find(static_cast<AudioSampleFormat>(sampleFormat)) == supportedSampleFormats.end()) {
        AVCODEC_LOGE("input sample format not supported");
        return false;
    }
    srcFmt_ = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(static_cast<AudioSampleFormat>(sampleFormat));
    if (srcFmt_ == AV_SAMPLE_FMT_NONE) {
        AVCODEC_LOGE("Check format failed, avSampleFormat not support");
        return false;
    }
    needResample_ = CheckResample();
    return true;
}

bool AudioFFMpegAacEncoderPlugin::CheckChannelLayout(const Format &format, int channels)
{
    int64_t channelLayout;
    if (format.GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, channelLayout)) {
        srcLayout_ = static_cast<int64_t>(
            FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(static_cast<AudioChannelLayout>(channelLayout)));
        if (av_get_channel_layout_nb_channels(srcLayout_) != channels) {
            AVCODEC_LOGE("channel layout channels mismatch");
            return false;
        }
        return true;
    }

    // channel layout not available
    auto iter = channelLayoutMap.find(channels);
    if (iter == channelLayoutMap.end()) {
        AVCODEC_LOGE("channel layout not found, channels: %{public}d", channels);
        return false;
    }
    srcLayout_ = iter->second;
    return true;
}

bool AudioFFMpegAacEncoderPlugin::CheckBitRate(const Format &format) const
{
    if (!format.ContainKey(MediaDescriptionKey::MD_KEY_BITRATE)) {
        AVCODEC_LOGW("parameter bit_rate not available");
        return true;
    }
    int64_t bitRate;
    if (!format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate)) {
        AVCODEC_LOGE("parameter bit_rate type invalid");
        return false;
    }
    if (bitRate < 0) {
        AVCODEC_LOGE("parameter bit_rate illegal");
        return false;
    }
    return true;
}

bool AudioFFMpegAacEncoderPlugin::CheckFormat(const Format &format)
{
    if (!format.ContainKey(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT) ||
        !format.ContainKey(MediaDescriptionKey::MD_KEY_SAMPLE_RATE) ||
        !format.ContainKey(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT)) {
        AVCODEC_LOGE("Format parameter missing");
        return false;
    }

    if (!CheckSampleFormat(format)) {
        return false;
    }

    if (!CheckBitRate(format)) {
        return false;
    }

    int sampleRate;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate);
    if (!CheckSampleRate(sampleRate)) {
        AVCODEC_LOGE("Sample rate not supported");
        return false;
    }

    int channels;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels);
    if (channels < MIN_CHANNELS || channels > MAX_CHANNELS || channels == INVALID_CHANNELS) {
        return false;
    }

    if (!CheckChannelLayout(format, channels)) {
        return false;
    }

    return true;
}

void AudioFFMpegAacEncoderPlugin::SetFormat(const Format &format) noexcept
{
    format_ = format;
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLES_PER_FRAME, avCodecContext_->frame_size);
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC);
}

int32_t AudioFFMpegAacEncoderPlugin::Init(const Format &format)
{
    int32_t ret = AllocateContext("aac");
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("Allocat aac context failed, ret = %{public}d", ret);
        return ret;
    }
    if (!CheckFormat(format)) {
        AVCODEC_LOGE("Format check failed.");
        return AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT_AUD_PARAMS;
    }
    ret = InitContext(format);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("Init context failed, ret = %{public}d", ret);
        return ret;
    }
    ret = OpenContext();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("Open context failed, ret = %{public}d", ret);
        return ret;
    }

    SetFormat(format);

    ret = InitFrame();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("Init frame failed, ret = %{public}d", ret);
        return ret;
    }

    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFFMpegAacEncoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    std::unique_lock lock(avMutext_);
    if (avCodecContext_ == nullptr) {
        AVCODEC_LOGE("avCodecContext_ is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION;
    }
    return SendBuffer(inputBuffer);
}

int32_t AudioFFMpegAacEncoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    if (!outBuffer) {
        AVCODEC_LOGE("outBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    int32_t status;
    {
        std::unique_lock l(avMutext_);
        if (avCodecContext_ == nullptr) {
            AVCODEC_LOGE("avCodecContext_ is nullptr");
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION;
        }
        status = ReceiveBuffer(outBuffer);
    }
    return status;
}

int32_t AudioFFMpegAacEncoderPlugin::Reset()
{
    std::unique_lock lock(avMutext_);
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    prevPts_ = 0;
    return ret;
}

int32_t AudioFFMpegAacEncoderPlugin::Release()
{
    std::unique_lock lock(avMutext_);
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    return ret;
}

int32_t AudioFFMpegAacEncoderPlugin::Flush()
{
    std::unique_lock lock(avMutext_);
    if (avCodecContext_ != nullptr) {
        avcodec_flush_buffers(avCodecContext_.get());
    }
    prevPts_ = 0;
    return ReAllocateContext();
}

int32_t AudioFFMpegAacEncoderPlugin::GetInputBufferSize() const
{
    if (maxInputSize_ < 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        return INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxInputSize_;
}

int32_t AudioFFMpegAacEncoderPlugin::GetOutputBufferSize() const
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}

Format AudioFFMpegAacEncoderPlugin::GetFormat() const noexcept
{
    return format_;
}

std::string_view AudioFFMpegAacEncoderPlugin::GetCodecType() const noexcept
{
    return AUDIO_CODEC_NAME;
}

int32_t AudioFFMpegAacEncoderPlugin::AllocateContext(const std::string &name)
{
    {
        std::unique_lock lock(avMutext_);
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
        std::unique_lock lock(avMutext_);
        context = avcodec_alloc_context3(avCodec_.get());
        avCodecContext_ = std::shared_ptr<AVCodecContext>(context, [](AVCodecContext *ptr) {
            avcodec_free_context(&ptr);
            avcodec_close(ptr);
        });
        av_log_set_level(AV_LOG_ERROR);
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFFMpegAacEncoderPlugin::InitContext(const Format &format)
{
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, avCodecContext_->channels);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, avCodecContext_->sample_rate);
    format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, avCodecContext_->bit_rate);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, maxInputSize_);
    avCodecContext_->channel_layout = srcLayout_;
    avCodecContext_->sample_fmt = srcFmt_;

    if (needResample_) {
        avCodecContext_->sample_fmt = avCodec_->sample_fmts[0];
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFFMpegAacEncoderPlugin::OpenContext()
{
    {
        std::unique_lock lock(avMutext_);
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

    if (needResample_) {
        ResamplePara resamplePara = {
            .channels = avCodecContext_->channels,
            .sampleRate = avCodecContext_->sample_rate,
            .bitsPerSample = 0,
            .channelLayout = avCodecContext_->ch_layout,
            .srcFmt = srcFmt_,
            .destSamplesPerFrame = avCodecContext_->frame_size,
            .destFmt = avCodecContext_->sample_fmt,
        };
        resample_ = std::make_shared<AudioResample>();
        if (resample_->Init(resamplePara) != AVCodecServiceErrCode::AVCS_ERR_OK) {
            AVCODEC_LOGE("Resmaple init failed.");
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
    }

    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

bool AudioFFMpegAacEncoderPlugin::CheckResample() const
{
    if (avCodec_ == nullptr || avCodecContext_ == nullptr) {
        return false;
    }
    for (size_t index = 0; avCodec_->sample_fmts[index] != AV_SAMPLE_FMT_NONE; ++index) {
        if (avCodec_->sample_fmts[index] == srcFmt_) {
            return false;
        }
    }
    return true;
}

int32_t AudioFFMpegAacEncoderPlugin::InitFrame()
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

int32_t AudioFFMpegAacEncoderPlugin::SendBuffer(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
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
        AVCODEC_LOGW("skip this frame because data not enough, msg:%{public}s",
                     FFMpegConverter::AVStrError(ret).data());
        return AVCodecServiceErrCode::AVCS_ERR_NOT_ENOUGH_DATA;
    } else if (ret == AVERROR_EOF) {
        AVCODEC_LOGW("eos send frame, msg:%{public}s", FFMpegConverter::AVStrError(ret).data());
        return AVCodecServiceErrCode::AVCS_ERR_END_OF_STREAM;
    } else {
        AVCODEC_LOGE("Send frame unknown error: %{public}s", FFMpegConverter::AVStrError(ret).c_str());
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
}

int32_t AudioFFMpegAacEncoderPlugin::PcmFillFrame(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    auto memory = inputBuffer->GetBuffer();
    auto bytesPerSample = av_get_bytes_per_sample(avCodecContext_->sample_fmt);
    const uint8_t *srcBuffer = memory->GetBase();
    uint8_t *destBuffer = const_cast<uint8_t*>(srcBuffer);
    size_t srcBufferSize = static_cast<size_t>(inputBuffer->GetBufferAttr().size);
    size_t destBufferSize = srcBufferSize;
    if (needResample_ && resample_ != nullptr) {
        if (resample_->Convert(srcBuffer, srcBufferSize, destBuffer, destBufferSize) !=
            AVCodecServiceErrCode::AVCS_ERR_OK) {
            AVCODEC_LOGE("Convert sample format failed");
        }
    }

    cachedFrame_->nb_samples = static_cast<int>(destBufferSize / (bytesPerSample * avCodecContext_->channels));
    if (cachedFrame_->nb_samples > avCodecContext_->frame_size) {
        AVCODEC_LOGE("Input frame size out of range, input smaples: %{public}d, frame_size: %{public}d",
                     cachedFrame_->nb_samples, avCodecContext_->frame_size);
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    if (!inputBuffer->CheckIsEos() && cachedFrame_->nb_samples != avCodecContext_->frame_size) {
        AVCODEC_LOGW("Input frame size not enough, input smaples: %{public}d, frame_size: %{public}d",
                     cachedFrame_->nb_samples, avCodecContext_->frame_size);
    }

    cachedFrame_->extended_data = cachedFrame_->data;
    cachedFrame_->extended_data[0] = destBuffer;
    cachedFrame_->linesize[0] = cachedFrame_->nb_samples * bytesPerSample;
    for (int i = 1; i < avCodecContext_->channels; i++) {
        cachedFrame_->extended_data[i] = cachedFrame_->extended_data[i-1] + cachedFrame_->linesize[0];
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFFMpegAacEncoderPlugin::ReceiveBuffer(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    (void)memset_s(avPacket_.get(), sizeof(AVPacket), 0, sizeof(AVPacket));
    auto ret = avcodec_receive_packet(avCodecContext_.get(), avPacket_.get());
    int32_t status;
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

int32_t AudioFFMpegAacEncoderPlugin::ReceivePacketSucc(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    int32_t headerSize = 0;
    auto memory = outBuffer->GetBuffer();
    std::string header;
    GetAdtsHeader(header, headerSize, avCodecContext_, avPacket_->size);
    if (headerSize == 0) {
        AVCODEC_LOGE("Get header failed.");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    int32_t writeBytes = memory->Write(reinterpret_cast<uint8_t *>(const_cast<char *>(header.c_str())), headerSize);
    if (writeBytes < headerSize) {
        AVCODEC_LOGE("Write header failed");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    int32_t outputSize = avPacket_->size + headerSize;
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

int32_t AudioFFMpegAacEncoderPlugin::CloseCtxLocked()
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

int32_t AudioFFMpegAacEncoderPlugin::ReAllocateContext()
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
} // namespace MediaAVCodec
} // namespace OHOS