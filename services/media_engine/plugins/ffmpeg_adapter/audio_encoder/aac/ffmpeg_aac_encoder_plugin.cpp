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

#include "ffmpeg_aac_encoder_plugin.h"
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "osal/utils/util.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_AUDIO, "FFmpegEncoderPlugin" };

namespace {
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 4 * 1024 * 8;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr int32_t ADTS_HEADER_SIZE = 7;
constexpr uint8_t SAMPLE_FREQUENCY_INDEX_DEFAULT = 4;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
constexpr int32_t INVALID_CHANNELS = 7;
constexpr int32_t AAC_MIN_BIT_RATE = 1;
constexpr int32_t AAC_DEFAULT_BIT_RATE = 128000;
constexpr int32_t AAC_MAX_BIT_RATE = 500000;
constexpr int64_t FRAMES_PER_SECOND = 1000 / 20;
constexpr int32_t BUFFER_FLAG_EOS = 0x00000001;
constexpr int32_t NS_PER_US = 1000;
static std::map<int32_t, uint8_t> sampleFreqMap = {{96000, 0},  {88200, 1}, {64000, 2}, {48000, 3}, {44100, 4},
                                                   {32000, 5},  {24000, 6}, {22050, 7}, {16000, 8}, {12000, 9},
                                                   {11025, 10}, {8000, 11}, {7350, 12}};

static std::set<AudioSampleFormat> supportedSampleFormats = {
    AudioSampleFormat::SAMPLE_S16LE,
    AudioSampleFormat::SAMPLE_S32LE,
    AudioSampleFormat::SAMPLE_F32LE,
};
static std::map<int32_t, int64_t> channelLayoutMap = {{1, AV_CH_LAYOUT_MONO},         {2, AV_CH_LAYOUT_STEREO},
                                                      {3, AV_CH_LAYOUT_SURROUND},     {4, AV_CH_LAYOUT_4POINT0},
                                                      {5, AV_CH_LAYOUT_5POINT0_BACK}, {6, AV_CH_LAYOUT_5POINT1_BACK},
                                                      {7, AV_CH_LAYOUT_7POINT0},      {8, AV_CH_LAYOUT_7POINT1}};
} // namespace
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegAACEncoderPlugin::FFmpegAACEncoderPlugin(const std::string& name)
    : CodecPlugin(std::move(name)),
      needResample_(false),
      codecContextValid_(false),
      avCodec_(nullptr),
      avCodecContext_(nullptr),
      fifo_(nullptr),
      cachedFrame_(nullptr),
      avPacket_(nullptr),
      prevPts_(0),
      resample_(nullptr),
      srcFmt_(AVSampleFormat::AV_SAMPLE_FMT_NONE),
      audioSampleFormat_(AudioSampleFormat::INVALID_WIDTH),
      srcLayout_(AudioChannelLayout::UNKNOWN),
      channels_(MIN_CHANNELS),
      sampleRate_(0),
      bitRate_(0),
      maxInputSize_(-1),
      maxOutputSize_(-1),
      outfile(nullptr)
{
}

FFmpegAACEncoderPlugin::~FFmpegAACEncoderPlugin()
{
    CloseCtxLocked();
    avCodecContext_.reset();
}

Status FFmpegAACEncoderPlugin::GetAdtsHeader(std::string &adtsHeader, int32_t &headerSize,
                                             std::shared_ptr<AVCodecContext> ctx, int aacLength)
{
    uint8_t freqIdx = SAMPLE_FREQUENCY_INDEX_DEFAULT; // 0: 96000 Hz  3: 48000 Hz 4: 44100 Hz
    auto iter = sampleFreqMap.find(ctx->sample_rate);
    if (iter != sampleFreqMap.end()) {
        freqIdx = iter->second;
    }
    uint8_t chanCfg = static_cast<uint8_t>(ctx->channels);
    uint32_t frameLength = static_cast<uint32_t>(aacLength + ADTS_HEADER_SIZE);
    uint8_t profile = static_cast<uint8_t>(ctx->profile);
    adtsHeader += 0xFF;
    adtsHeader += 0xF1;
    adtsHeader += ((profile) << 0x6) + (freqIdx << 0x2) + (chanCfg >> 0x2);
    adtsHeader += (((chanCfg & 0x3) << 0x6) + (frameLength >> 0xB));
    adtsHeader += ((frameLength & 0x7FF) >> 0x3);
    adtsHeader += (((frameLength & 0x7) << 0x5) + 0x1F);
    adtsHeader += 0xFC;
    headerSize = ADTS_HEADER_SIZE;

    return Status::OK;
}

bool FFmpegAACEncoderPlugin::CheckSampleRate(const int sampleRate)
{
    return sampleFreqMap.find(sampleRate) != sampleFreqMap.end() ? true : false;
}

bool FFmpegAACEncoderPlugin::CheckSampleFormat()
{
    if (supportedSampleFormats.find(audioSampleFormat_) == supportedSampleFormats.end()) {
        AVCODEC_LOGE("input sample format not supported,srcFmt_=%{public}d", (int32_t)srcFmt_);
        return false;
    }
    AudioSampleFormat2AVSampleFormat(audioSampleFormat_, srcFmt_);
    AVCODEC_LOGE("AUDIO_SAMPLE_FORMAT found,srcFmt:%{public}d to ffmpeg-srcFmt_:%{public}d",
                 (int32_t)audioSampleFormat_, (int32_t)srcFmt_);
    needResample_ = CheckResample();
    return true;
}

bool FFmpegAACEncoderPlugin::CheckChannelLayout()
{
    uint64_t ffmpegChlayout = FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(
        static_cast<AudioChannelLayout>(srcLayout_));
    // channel layout not available
    if (av_get_channel_layout_nb_channels(ffmpegChlayout) != channels_) {
        AVCODEC_LOGE("channel layout channels mismatch");
        return false;
    }
    return true;
}

bool FFmpegAACEncoderPlugin::CheckBitRate() const
{
    if (bitRate_ < AAC_MIN_BIT_RATE || bitRate_ > AAC_MAX_BIT_RATE) {
        AVCODEC_LOGE("parameter bit_rate illegal");
        return false;
    }
    return true;
}

bool FFmpegAACEncoderPlugin::CheckFormat()
{
    if (!CheckSampleFormat()) {
        AVCODEC_LOGE("sampleFormat not supported");
        return false;
    }

    if (!CheckBitRate()) {
        AVCODEC_LOGE("bitRate not supported");
        return false;
    }

    if (!CheckSampleRate(sampleRate_)) {
        AVCODEC_LOGE("sample rate not supported");
        return false;
    }

    if (channels_ < MIN_CHANNELS || channels_ > MAX_CHANNELS || channels_ == INVALID_CHANNELS) {
        AVCODEC_LOGE("channels not supported");
        return false;
    }

    if (!CheckChannelLayout()) {
        AVCODEC_LOGE("channelLayout not supported");
        return false;
    }

    return true;
}

bool FFmpegAACEncoderPlugin::AudioSampleFormat2AVSampleFormat(const AudioSampleFormat &audioFmt, AVSampleFormat &fmt)
{
    /* AudioSampleFormat to AVSampleFormat */
    static const std::unordered_map<AudioSampleFormat, AVSampleFormat> formatTable = {
        {AudioSampleFormat::SAMPLE_U8, AVSampleFormat::AV_SAMPLE_FMT_U8},
        {AudioSampleFormat::SAMPLE_S16LE, AVSampleFormat::AV_SAMPLE_FMT_S16},
        {AudioSampleFormat::SAMPLE_S32LE, AVSampleFormat::AV_SAMPLE_FMT_S32},
        {AudioSampleFormat::SAMPLE_F32LE, AVSampleFormat::AV_SAMPLE_FMT_FLT},
        {AudioSampleFormat::SAMPLE_U8P, AVSampleFormat::AV_SAMPLE_FMT_U8P},
        {AudioSampleFormat::SAMPLE_S16P, AVSampleFormat::AV_SAMPLE_FMT_S16P},
        {AudioSampleFormat::SAMPLE_F32P, AVSampleFormat::AV_SAMPLE_FMT_FLTP},
    };
    // 使用迭代器遍历 unordered_map
    for (auto itM = formatTable.begin(); itM != formatTable.end(); ++itM) {
        AVCODEC_LOGE("formatTable key:%{public}d   Value:%{public}d ", (int32_t)itM->first, (int32_t)itM->second);
    }

    auto it = formatTable.find(audioFmt);
    if (it != formatTable.end()) {
        fmt = it->second;
        return true;
    }
    AVCODEC_LOGE("AudioSampleFormat2AVSampleFormat fail, from fmt:%{public}d to fmt:%{public}d",
                 (int32_t)audioFmt, (int32_t)fmt);
    return false;
}

Status FFmpegAACEncoderPlugin::Init()
{
    AVCODEC_LOGI("Init enter");
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::Start()
{
    AVCODEC_LOGI("Start enter");
    Status status = AllocateContext("aac");
    if (status != Status::OK) {
        AVCODEC_LOGD("Allocat aac context failed, status = %{public}d", status);
        return status;
    }
    if (!CheckFormat()) {
        AVCODEC_LOGD("Format check failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    status = InitContext();
    if (status != Status::OK) {
        AVCODEC_LOGD("Init context failed, status = %{public}d", status);
        return status;
    }
    status = OpenContext();
    if (status != Status::OK) {
        AVCODEC_LOGD("Open context failed, status = %{public}d", status);
        return status;
    }

    status = InitFrame();
    if (status != Status::OK) {
        AVCODEC_LOGD("Init frame failed, status = %{public}d", status);
        return status;
    }
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    auto memory = inputBuffer->memory_;
    if (memory == nullptr) {
        return Status::ERROR_INVALID_DATA;
    }
    if (memory->GetSize() == 0 && !(inputBuffer->flag_ & BUFFER_FLAG_EOS)) {
        AVCODEC_LOGE("size is 0, but flag is not 1");
        return Status::ERROR_INVALID_DATA;
    }
    Status ret;
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        if (avCodecContext_ == nullptr) {
            return Status::ERROR_WRONG_STATE;
        }
        ret = PushInFifo(inputBuffer);
        if (ret == Status::OK) {
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

Status FFmpegAACEncoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    if (!outputBuffer) {
        AVCODEC_LOGE("queue out buffer is nullptr.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(avMutex_);
    if (avCodecContext_ == nullptr) {
        return Status::ERROR_WRONG_STATE;
    }
    outBuffer_ = outputBuffer;
    Status ret = SendOutputBuffer(outputBuffer);
    return ret;
}

Status FFmpegAACEncoderPlugin::ReceivePacketSucc(std::shared_ptr<AVBuffer> &outBuffer)
{
    int32_t headerSize = 0;
    auto memory = outBuffer->memory_;
    std::string header;
    GetAdtsHeader(header, headerSize, avCodecContext_, avPacket_->size);
    if (headerSize == 0) {
        AVCODEC_LOGE("Get header failed.");
        return Status::ERROR_UNKNOWN;
    }
    int32_t writeBytes = memory->Write(
        reinterpret_cast<uint8_t *>(const_cast<char *>(header.c_str())), headerSize, 0);
    if (writeBytes < headerSize) {
        AVCODEC_LOGE("Write header failed");
        return Status::ERROR_UNKNOWN;
    }

    int32_t outputSize = avPacket_->size + headerSize;
    if (memory->GetCapacity() < outputSize) {
        AVCODEC_LOGE("Output buffer capacity is not enough");
        return Status::ERROR_NO_MEMORY;
    }

    auto len = memory->Write(avPacket_->data, avPacket_->size, headerSize);
    if (len < avPacket_->size) {
        AVCODEC_LOGE("write packet data failed, len = %{public}d", len);
        return Status::ERROR_UNKNOWN;
    }

    // how get perfect pts with upstream pts(us)
    outBuffer->duration_ = ConvertTimeFromFFmpeg(avPacket_->duration, avCodecContext_->time_base) / NS_PER_US;
    // adjust ffmpeg duration with sample rate
    outBuffer->pts_ = ((INT64_MAX - prevPts_) < avPacket_->duration)
                          ? (outBuffer->duration_ - (INT64_MAX - prevPts_))
                          : (prevPts_ + outBuffer->duration_);
    prevPts_ = outBuffer->pts_;
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::ReceiveBuffer(std::shared_ptr<AVBuffer> &outBuffer)
{
    AVCODEC_LOGD("ReceiveBuffer enter");
    (void)memset_s(avPacket_.get(), sizeof(AVPacket), 0, sizeof(AVPacket));
    auto ret = avcodec_receive_packet(avCodecContext_.get(), avPacket_.get());
    Status status;
    if (ret >= 0) {
        AVCODEC_LOGD("receive one packet");
        status = ReceivePacketSucc(outBuffer);
    } else if (ret == AVERROR_EOF) {
        outBuffer->flag_ = BUFFER_FLAG_EOS;
        avcodec_flush_buffers(avCodecContext_.get());
        status = Status::END_OF_STREAM;
        AVCODEC_LOGE("ReceiveBuffer EOF");
    } else if (ret == AVERROR(EAGAIN)) {
        status = Status::ERROR_NOT_ENOUGH_DATA;
        AVCODEC_LOGE("ReceiveBuffer EAGAIN");
    } else {
        AVCODEC_LOGE("audio encoder receive unknow error: %{public}s", OSAL::AVStrError(ret).c_str());
        status = Status::ERROR_UNKNOWN;
    }
    av_packet_unref(avPacket_.get());
    return status;
}

Status FFmpegAACEncoderPlugin::SendOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    Status status = SendFrameToFfmpeg();
    if (status == Status::ERROR_NOT_ENOUGH_DATA) {
        AVCODEC_LOGD("SendFrameToFfmpeg no one frame data");
        // last frame mark eos
        if (outputBuffer->flag_ & BUFFER_FLAG_EOS) {
            dataCallback_->OnOutputBufferDone(outBuffer_);
            return Status::OK;
        }
        return status;
    }
    status = ReceiveBuffer(outputBuffer);
    if (status == Status::OK || status == Status::END_OF_STREAM) {
        {
            std::lock_guard<std::mutex> l(bufferMetaMutex_);
            if (outBuffer_ == nullptr) {
                AVCODEC_LOGE("SendOutputBuffer ERROR_NULL_POINTER");
                return Status::ERROR_NULL_POINTER;
            }
            outBuffer_->meta_ = bufferMeta_;
        }
        int32_t fifoSize = av_audio_fifo_size(fifo_);
        if (fifoSize >= avCodecContext_->frame_size) {
            outputBuffer->flag_ = 0; // not eos
            AVCODEC_LOGD("fifoSize:%{public}d need another encoder", fifoSize);
            dataCallback_->OnOutputBufferDone(outBuffer_);
            return Status::ERROR_AGAIN;
        }
        dataCallback_->OnOutputBufferDone(outBuffer_);
        return Status::OK;
    } else {
        AVCODEC_LOGE("SendOutputBuffer-ReceiveBuffer error");
    }
    return status;
}

Status FFmpegAACEncoderPlugin::Reset()
{
    AVCODEC_LOGI("Reset enter");
    std::lock_guard<std::mutex> lock(avMutex_);
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    prevPts_ = 0;
    return ret;
}

Status FFmpegAACEncoderPlugin::Release()
{
    AVCODEC_LOGI("Release enter");
    std::lock_guard<std::mutex> lock(avMutex_);
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    return ret;
}

Status FFmpegAACEncoderPlugin::Flush()
{
    AVCODEC_LOGI("Flush enter");
    std::lock_guard<std::mutex> lock(avMutex_);
    if (avCodecContext_ != nullptr) {
        avcodec_flush_buffers(avCodecContext_.get());
    }
    prevPts_ = 0;
    if (fifo_) {
        av_audio_fifo_reset(fifo_);
    }
    return ReAllocateContext();
}

Status FFmpegAACEncoderPlugin::ReAllocateContext()
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

Status FFmpegAACEncoderPlugin::AllocateContext(const std::string &name)
{
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        avCodec_ = std::shared_ptr<AVCodec>(const_cast<AVCodec *>(avcodec_find_encoder_by_name(name.c_str())),
                                            [](AVCodec *ptr) {});
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *fp) { av_frame_free(&fp); });
        avPacket_ = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *ptr) { av_packet_free(&ptr); });
    }
    if (avCodec_ == nullptr) {
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }

    AVCodecContext *context = nullptr;
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        context = avcodec_alloc_context3(avCodec_.get());
        avCodecContext_ = std::shared_ptr<AVCodecContext>(context, [](AVCodecContext *ptr) {
            avcodec_free_context(&ptr);
            avcodec_close(ptr);
        });
    }
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::InitContext()
{
    avCodecContext_->channels = channels_;
    avCodecContext_->sample_rate = sampleRate_;
    avCodecContext_->bit_rate = bitRate_;
    avCodecContext_->channel_layout = srcLayout_;
    avCodecContext_->sample_fmt = srcFmt_;

    if (needResample_) {
        avCodecContext_->sample_fmt = avCodec_->sample_fmts[0];
    }
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::OpenContext()
{
    {
        std::unique_lock lock(avMutex_);
        AVCODEC_LOGI("avCodecContext_->channels %{public}d", avCodecContext_->channels);
        AVCODEC_LOGI("avCodecContext_->sample_rate %{public}d", avCodecContext_->sample_rate);
        AVCODEC_LOGI("avCodecContext_->bit_rate %{public}lld", avCodecContext_->bit_rate);
        AVCODEC_LOGI("avCodecContext_->channel_layout  %{public}lld", avCodecContext_->channel_layout);
        AVCODEC_LOGI("avCodecContext_->sample_fmt %{public}d", static_cast<int32_t>(*(avCodec_.get()->sample_fmts)));
        AVCODEC_LOGI("avCodecContext_ old srcFmt_ %{public}d", static_cast<int32_t>(srcFmt_));
        AVCODEC_LOGI("avCodecContext_->codec_id %{public}d", static_cast<int32_t>(avCodec_.get()->id));
        auto res = avcodec_open2(avCodecContext_.get(), avCodec_.get(), nullptr);
        if (res != 0) {
            AVCODEC_LOGE("avcodec open error %{public}s", OSAL::AVStrError(res).c_str());
            return Status::ERROR_UNKNOWN;
        }
        av_log_set_level(AV_LOG_DEBUG);

        codecContextValid_ = true;
    }
    if (avCodecContext_->frame_size <= 0) {
        AVCODEC_LOGE("frame size invalid");
    }
    int32_t destSamplesPerFrame = (avCodecContext_->frame_size > (avCodecContext_->sample_rate / FRAMES_PER_SECOND)) ?
        avCodecContext_->frame_size : (avCodecContext_->sample_rate / FRAMES_PER_SECOND);
    if (needResample_) {
        ResamplePara resamplePara = {
            .channels = static_cast<uint32_t>(avCodecContext_->channels),
            .sampleRate = static_cast<uint32_t>(avCodecContext_->sample_rate),
            .bitsPerSample = 0,
            .channelLayout = avCodecContext_->ch_layout,
            .srcFfFmt = srcFmt_,
            .destSamplesPerFrame = static_cast<uint32_t>(destSamplesPerFrame),
            .destFmt = avCodecContext_->sample_fmt,
        };
        resample_ = std::make_shared<Ffmpeg::Resample>();
        if (resample_->Init(resamplePara) != Status::OK) {
            AVCODEC_LOGE("Resmaple init failed.");
            return Status::ERROR_UNKNOWN;
        }
    }
    return Status::OK;
}

bool FFmpegAACEncoderPlugin::CheckResample() const
{
    if (avCodec_ == nullptr || avCodecContext_ == nullptr) {
        return false;
    }
    for (size_t index = 0; avCodec_->sample_fmts[index] != AV_SAMPLE_FMT_NONE; ++index) {
        if (avCodec_->sample_fmts[index] == srcFmt_) {
            return false;
        }
    }
    AVCODEC_LOGI("CheckResample need resample");
    return true;
}

Status FFmpegAACEncoderPlugin::GetMetaData(const std::shared_ptr<Meta> &meta)
{
    int32_t type;
    AVCODEC_LOGI("GetMetaData enter");
    if (meta->Get<Tag::AUDIO_AAC_IS_ADTS>(type)) {
        aacName_ = (type == 1 ? "aac" : "aac_latm");
    }
    if (meta->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_)) {
        if (channels_ < MIN_CHANNELS || channels_ > MAX_CHANNELS) {
            AVCODEC_LOGE("AUDIO_CHANNEL_COUNT error");
            return Status::ERROR_INVALID_PARAMETER;
        }
    } else {
        AVCODEC_LOGE("no AUDIO_CHANNEL_COUNT");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (!meta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_)) {
        AVCODEC_LOGE("no AUDIO_SAMPLE_RATE");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (!meta->Get<Tag::MEDIA_BITRATE>(bitRate_)) {
        AVCODEC_LOGE("no MEDIA_BITRATE, set to 32k");
        bitRate_ = AAC_DEFAULT_BIT_RATE;
    }
    if (meta->Get<Tag::AUDIO_SAMPLE_FORMAT>(audioSampleFormat_)) {
        AVCODEC_LOGD("AUDIO_SAMPLE_FORMAT found, srcFmt:%{public}d", audioSampleFormat_);
    } else {
        AVCODEC_LOGE("no AUDIO_SAMPLE_FORMAT");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (meta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_)) {
        AVCODEC_LOGI("maxInputSize: %{public}d", maxInputSize_);
    }
    if (meta->Get<Tag::AUDIO_CHANNEL_LAYOUT>(srcLayout_)) {
        AVCODEC_LOGI("srcLayout_: %{public}llu", srcLayout_);
    } else {
        auto iter = channelLayoutMap.find(channels_);
        if (iter == channelLayoutMap.end()) {
            AVCODEC_LOGE("channel layout not found, channels: %{public}d", channels_);
            return Status::ERROR_UNKNOWN;
        } else {
            srcLayout_ = static_cast<AudioChannelLayout>(iter->second);
        }
    }
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::SetParameter(const std::shared_ptr<Meta> &meta)
{
    AVCODEC_LOGI("SetParameter enter");
    std::lock_guard<std::mutex> lock(parameterMutex_);
    Status ret = GetMetaData(meta);
    if (!CheckFormat()) {
        AVCODEC_LOGE("CheckFormat fail");
        return Status::ERROR_INVALID_PARAMETER;
    }
    audioParameter_ = *meta;
    return ret;
}

Status FFmpegAACEncoderPlugin::GetParameter(std::shared_ptr<Meta> &meta)
{
    std::lock_guard<std::mutex> lock(parameterMutex_);
    if (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    AVCODEC_LOGI("GetParameter maxInputSize_: %{public}d", maxInputSize_);
    // add codec meta
    audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    *meta = audioParameter_;
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::InitFrame()
{
    AVCODEC_LOGI("InitFrame enter");
    cachedFrame_->nb_samples = avCodecContext_->frame_size;
    cachedFrame_->format = avCodecContext_->sample_fmt;
    cachedFrame_->channel_layout = avCodecContext_->channel_layout;
    cachedFrame_->channels = avCodecContext_->channels;
    int ret = av_frame_get_buffer(cachedFrame_.get(), 0);
    if (ret < 0) {
        AVCODEC_LOGE("Get frame buffer failed: %{public}s", OSAL::AVStrError(ret).c_str());
        return Status::ERROR_NO_MEMORY;
    }

    if (!(fifo_ =
              av_audio_fifo_alloc(avCodecContext_->sample_fmt, avCodecContext_->channels, cachedFrame_->nb_samples))) {
        AVCODEC_LOGE("Could not allocate FIFO");
    }
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::SendEncoder(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    auto memory = inputBuffer->memory_;
    if (memory->GetSize() < 0) {
        AVCODEC_LOGE("SendEncoder buffer size is less than 0. size : %{public}d", memory->GetSize());
        return Status::ERROR_UNKNOWN;
    }
    if (memory->GetSize() > memory->GetCapacity()) {
        AVCODEC_LOGE("send input buffer is > allocate size. size : %{public}d, allocate size : %{public}d",
                     memory->GetSize(), memory->GetCapacity());
        return Status::ERROR_UNKNOWN;
    }
    auto errCode = PcmFillFrame(inputBuffer);
    if (errCode != Status::OK) {
        AVCODEC_LOGE("SendEncoder PcmFillFrame error");
        return errCode;
    }
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::PushInFifo(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    if (!inputBuffer) {
        AVCODEC_LOGD("inputBuffer is nullptr");
        return Status::ERROR_INVALID_PARAMETER;
    }
    int ret = av_frame_make_writable(cachedFrame_.get());
    if (ret != 0) {
        AVCODEC_LOGD("Frame make writable failed: %{public}s", OSAL::AVStrError(ret).c_str());
        return Status::ERROR_UNKNOWN;
    }
    bool isEos = inputBuffer->flag_ & BUFFER_FLAG_EOS;
    if (!isEos) {
        auto status = SendEncoder(inputBuffer);
        if (status != Status::OK) {
            AVCODEC_LOGE("input push in fifo fail");
            return status;
        }
    } else {
        AVCODEC_LOGI("input eos");
    }
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::SendFrameToFfmpeg()
{
    AVCODEC_LOGD("SendFrameToFfmpeg enter");
    int32_t fifoSize = av_audio_fifo_size(fifo_);
    if (fifoSize < avCodecContext_->frame_size) {
        AVCODEC_LOGD("fifoSize:%{public}d not enough", fifoSize);
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    cachedFrame_->nb_samples = avCodecContext_->frame_size;
    int32_t bytesPerSample = av_get_bytes_per_sample(avCodecContext_->sample_fmt);
    // adjest data addr
    for (int i = 1; i < avCodecContext_->channels; i++) {
        cachedFrame_->extended_data[i] =
            cachedFrame_->extended_data[i - 1] + cachedFrame_->nb_samples * bytesPerSample;
    }
    int readRet =
        av_audio_fifo_read(fifo_, reinterpret_cast<void **>(cachedFrame_->data), avCodecContext_->frame_size);
    if (readRet < 0) {
        AVCODEC_LOGE("fifo read error");
        return Status::ERROR_UNKNOWN;
    }
    cachedFrame_->linesize[0] = readRet * av_get_bytes_per_sample(avCodecContext_->sample_fmt);
    int32_t ret = avcodec_send_frame(avCodecContext_.get(), cachedFrame_.get());
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

Status FFmpegAACEncoderPlugin::PcmFillFrame(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    AVCODEC_LOGD("PcmFillFrame enter, buffer->pts %{public}lld", inputBuffer->pts_);
    auto memory = inputBuffer->memory_;
    auto bytesPerSample = av_get_bytes_per_sample(avCodecContext_->sample_fmt);
    const uint8_t *srcBuffer = memory->GetAddr();
    uint8_t *destBuffer = const_cast<uint8_t *>(srcBuffer);
    size_t srcBufferSize = static_cast<size_t>(memory->GetSize());
    size_t destBufferSize = srcBufferSize;
    if (needResample_ && resample_ != nullptr) {
        if (resample_->Convert(srcBuffer, srcBufferSize, destBuffer, destBufferSize) != Status::OK) {
            AVCODEC_LOGE("Convert sample format failed");
        }
    }

    cachedFrame_->nb_samples = static_cast<int>(destBufferSize) / (bytesPerSample * avCodecContext_->channels);
    if (!(inputBuffer->flag_ & BUFFER_FLAG_EOS) && cachedFrame_->nb_samples != avCodecContext_->frame_size) {
        AVCODEC_LOGD("Input frame size not match, input samples: %{public}d, frame_size: %{public}d",
                     cachedFrame_->nb_samples, avCodecContext_->frame_size);
    }
    int32_t destSamplesPerFrame = (avCodecContext_->frame_size > (avCodecContext_->sample_rate / FRAMES_PER_SECOND)) ?
        avCodecContext_->frame_size : (avCodecContext_->sample_rate / FRAMES_PER_SECOND);
    cachedFrame_->extended_data = cachedFrame_->data;
    cachedFrame_->extended_data[0] = destBuffer;
    cachedFrame_->linesize[0] = cachedFrame_->nb_samples * bytesPerSample;
    for (int i = 1; i < avCodecContext_->channels; i++) {
        // after convert, the length of line is destSamplesPerFrame
        cachedFrame_->extended_data[i] =
            cachedFrame_->extended_data[i - 1] + static_cast<uint32_t>(destSamplesPerFrame * bytesPerSample);
    }
    int32_t cacheSize = av_audio_fifo_size(fifo_);
    int32_t ret = av_audio_fifo_realloc(fifo_, cacheSize + cachedFrame_->nb_samples);
    if (ret < 0) {
        AVCODEC_LOGE("realloc ret: %{public}d, cacheSize: %{public}d", ret, cacheSize);
    }
    AVCODEC_LOGD("realloc nb_samples:%{public}d cacheSize:%{public}d channels:%{public}d",
        cachedFrame_->nb_samples, cacheSize, avCodecContext_->channels);
    int32_t writeSamples =
        av_audio_fifo_write(fifo_, reinterpret_cast<void **>(cachedFrame_->data), cachedFrame_->nb_samples);
    if (writeSamples < cachedFrame_->nb_samples) {
        AVCODEC_LOGE("write smaples: %{public}d, nb_samples: %{public}d", writeSamples, cachedFrame_->nb_samples);
    }
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::Stop()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    avCodecContext_ = nullptr;
    if (outBuffer_) {
        outBuffer_.reset();
        outBuffer_ = nullptr;
    }
    AVCODEC_LOGI("Stop");
    return ret;
}

Status FFmpegAACEncoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegAACEncoderPlugin::CloseCtxLocked()
{
    if (avCodecContext_ != nullptr) {
        auto res = avcodec_close(avCodecContext_.get());
        if (res != 0) {
            AVCODEC_LOGE("avcodec close failed: %{public}s", OSAL::AVStrError(res).c_str());
            return Status::ERROR_UNKNOWN;
        }
    }
    if (fifo_) {
        av_audio_fifo_free(fifo_);
        fifo_ = nullptr;
    }
    return Status::OK;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
