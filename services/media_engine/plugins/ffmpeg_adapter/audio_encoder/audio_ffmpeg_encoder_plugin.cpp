/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "Ffmpeg_Au_Encoder"

#include "audio_ffmpeg_encoder_plugin.h"
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include "osal/utils/util.h"
#include "common/log.h"
#include "plugin_caps_builder.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugin;
using namespace Ffmpeg;

std::map<std::string, std::shared_ptr<const AVCodec>> codecMap;

// const size_t BUFFER_QUEUE_SIZE = 6;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 8192;
// constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4 * 1024 * 8;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
const char* const MEDIA_MIME_AUDIO_RAW = "audio/raw";


const std::set<AVCodecID> g_supportedCodec = {
    AV_CODEC_ID_AAC,
    AV_CODEC_ID_AAC_LATM
};

std::map<AVCodecID, uint32_t> samplesPerFrameMap = {
};

void UpdateInCaps(const AVCodec* codec, CodecPluginDef& definition)
{
    Capability cap;

    cap.SetMime(MEDIA_MIME_AUDIO_RAW);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    if (codec->supported_samplerates != nullptr) {
        DiscreteCapability<uint32_t> values;
        size_t index {0};
        for (; codec->supported_samplerates[index] != 0; ++index) {
            values.push_back(codec->supported_samplerates[index]);
        }
        if (index) {
        //    cap.SetAudioSampleRateList(values);
        }
    }

    if (codec->channel_layouts != nullptr) {
        DiscreteCapability<AudioChannelLayout> values;
        size_t index {0};
        for (; codec->channel_layouts[index] != 0; ++index) {
            values.push_back(AudioChannelLayout(codec->channel_layouts[index]));
        }
        if (index) {
          //  cap.SetAudioChannelLayoutList(values);
        }
    }
    definition.AddInCaps(cap);
}

void UpdatePluginDefinition(const AVCodec* codec, CodecPluginDef& definition)
{
    UpdateInCaps(codec, definition);
}

Status RegisterAudioEncoderPlugins(const std::shared_ptr<Register>& reg)
{
    const AVCodec* codec = nullptr;
    void* ite = nullptr;
    std::cout << "registering audio encoders" << std::endl;
    while ((codec = av_codec_iterate(&ite))) {
        if (!av_codec_is_encoder(codec) || codec->type != AVMEDIA_TYPE_AUDIO) {
            continue;
        }
        if (g_supportedCodec.find(codec->id) == g_supportedCodec.end()) {
            std::cout << "codec " << codec->name << " " << codec->long_name << " is not supported right now" << std::endl;
            continue;
        }
        CodecPluginDef definition;
        definition.name = "ffmpegAuEnc_" + std::string(codec->name);
        definition.pluginType = PluginType::AUDIO_ENCODER;
        definition.rank = 100; // 100
        definition.SetCreator([](const std::string& name) -> std::shared_ptr<CodecPlugin> {
            return std::make_shared<AudioFfmpegEncoderPlugin>(name);
        });
        UpdatePluginDefinition(codec, definition);
        // do not delete the codec in the deleter
        codecMap[definition.name] = std::shared_ptr<AVCodec>(const_cast<AVCodec*>(codec), [](void* ptr) {});
        if (reg->AddPlugin(definition) != Status::OK) {
            std::cout << "register plugin " << definition.name.c_str() << " failed" << std::endl;
        }
    }
    return Status::OK;
}

/*
std::string AVStrError(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}
*/

void UnRegisterAudioEncoderPlugin()
{
    codecMap.clear();
}
} // namespace
PLUGIN_DEFINITION(FFmpegAudioEncoders, LicenseType::LGPL, RegisterAudioEncoderPlugins, UnRegisterAudioEncoderPlugin);

namespace OHOS {
namespace Media {
namespace Plugin {
namespace Ffmpeg {
AudioFfmpegEncoderPlugin::AudioFfmpegEncoderPlugin(std::string name) : CodecPlugin(std::move(name)),
    channels_(0), sampleRate_(0), bit_rate_(0), maxInputSize_(INPUT_BUFFER_SIZE_DEFAULT)
{
}

AudioFfmpegEncoderPlugin::~AudioFfmpegEncoderPlugin()
{
    DeInitLocked();
}

Status AudioFfmpegEncoderPlugin::Init()
{
    auto ite = codecMap.find(pluginName_);
    if (ite == codecMap.end()) {
        MEDIA_LOG_W("cannot find codec with name " PUBLIC_LOG_S, pluginName_.c_str());
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        avCodec_ = ite->second;
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *fp) { av_frame_free(&fp); });
    }

    return Status::OK;
}

/*
Status AudioFfmpegEncoderPlugin::Deinit()
{
    MEDIA_LOG_I("Deinit enter.");
    std::lock_guard<std::mutex> lock(avMutex_);
    std::lock_guard<std::mutex> lock1(parameterMutex_);
    return DeInitLocked();
}

*/
Status AudioFfmpegEncoderPlugin::DeInitLocked()
{
    std::cout << "DeInitLocked enter." << std::endl;
    ResetLocked();
    avCodec_.reset();
    cachedFrame_.reset();
    return Status::OK;
}

Status AudioFfmpegEncoderPlugin::SetParameter(const std::shared_ptr<Meta> parameter)
{
    std::lock_guard<std::mutex> lock(parameterMutex_);
    int32_t type;
    if (parameter->Find(Tag::AUDIO_AAC_IS_ADTS) != parameter->end()) {
        parameter->Get<Tag::AUDIO_AAC_IS_ADTS>(type);
        aacName_ = (type == 1 ? "aac" : "aac_latm");
    }

    if (parameter->Find(Tag::AUDIO_CHANNEL_COUNT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_);
        if (channels_ < MIN_CHANNELS || channels_ > MAX_CHANNELS) {
            return Status::ERROR_UNKNOWN;
        }
    } else {
        return Status::ERROR_UNKNOWN;
    }

    if (parameter->Find(Tag::AUDIO_SAMPLE_RATE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    } else {
        return Status::ERROR_UNKNOWN;
    }

    if (parameter->Find(Tag::MEDIA_BITRATE) != parameter->end()) {
        parameter->Get<Tag::MEDIA_BITRATE>(bit_rate_);
    } else {
        return Status::ERROR_UNKNOWN;
    }
    if (parameter->Find(Tag::AUDIO_CHANNEL_LAYOUT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_CHANNEL_LAYOUT>(srcLayout_);
    } else {
        return Status::ERROR_UNKNOWN;
    }
    AudioSampleFormat srcFmt;
    if (parameter->Find(Tag::AUDIO_SAMPLE_FORMAT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_FORMAT>(srcFmt);
        srcFmt_ = (AVSampleFormat)srcFmt;
    } else {
        return Status::ERROR_UNKNOWN;
    }
    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    }
    audioParameter_ = *parameter;
    return Status::OK;
}

Status AudioFfmpegEncoderPlugin::GetParameter(std::shared_ptr<Meta> parameter)
{
    std::lock_guard<std::mutex> lock(parameterMutex_);
    if (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    // add codec parameter
    audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxInputSize_);
    *parameter = audioParameter_;
    return Status::OK;
}

Status AudioFfmpegEncoderPlugin::Prepare()
{
    AVCodecContext* context = nullptr;
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        FALSE_RETURN_V(avCodec_ != nullptr, Status::ERROR_WRONG_STATE);
        context = avcodec_alloc_context3(avCodec_.get());
    }
    FALSE_RETURN_V_MSG_E(context != nullptr, Status::ERROR_NO_MEMORY, "can't allocate codec context");
    auto tmpCtx = std::shared_ptr<AVCodecContext>(context, [](AVCodecContext* ptr) {
            if (ptr != nullptr) {
                if (ptr->extradata) {
                    av_free(ptr->extradata);
                    ptr->extradata = nullptr;
                }
                avcodec_free_context(&ptr);
            }
        });
    {
        std::lock_guard<std::mutex> lock1(parameterMutex_);
        tmpCtx->channels = channels_;
        tmpCtx->sample_rate = sampleRate_;
        tmpCtx->bit_rate = bit_rate_;
        tmpCtx->channel_layout = srcLayout_;
        tmpCtx->sample_fmt = srcFmt_;

        if (!tmpCtx->time_base.den) {
            tmpCtx->time_base.den = tmpCtx->sample_rate;
            tmpCtx->time_base.num = 1;
            tmpCtx->ticks_per_frame = 1;
        }
        tmpCtx->sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16; // need recheck
        tmpCtx->request_sample_fmt = tmpCtx->sample_fmt;
        tmpCtx->channel_layout = AV_CH_LAYOUT_STEREO;

        tmpCtx->level = 1; // set to default 1
        tmpCtx->profile = FF_PROFILE_AAC_LOW;

    }
    tmpCtx->workaround_bugs = static_cast<uint32_t>(tmpCtx->workaround_bugs) | static_cast<uint32_t>(FF_BUG_AUTODETECT);
    tmpCtx->err_recognition = 1;
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        avCodecContext_ = tmpCtx;
    }
    avPacket_ = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket* ptr) {
        av_packet_free(&ptr);
    });
    return Status::OK;
}

Status AudioFfmpegEncoderPlugin::ResetLocked()
{
    std::cout << "ResetLocked enter." << std::endl;
    audioParameter_.Clear();
    StopLocked();
    avCodecContext_.reset();
    {
        std::lock_guard<std::mutex> l(bufferMetaMutex_);
        bufferMeta_.reset();
    }
    return Status::OK;
}

Status AudioFfmpegEncoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    std::lock_guard<std::mutex> lock1(parameterMutex_);
    return ResetLocked();
}

Status AudioFfmpegEncoderPlugin::OpenCtxLocked()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    if (avCodecContext_ == nullptr) {
        return Status::ERROR_WRONG_STATE;
    }
    auto res = avcodec_open2(avCodecContext_.get(), avCodec_.get(), nullptr);
    if (res != 0) {
        std::cout << "avcodec open error" << OSAL::AVStrError(res).c_str() << std::endl;
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

bool AudioFfmpegEncoderPlugin::CheckReformat()
{
    if (avCodec_ == nullptr || avCodecContext_ == nullptr) {
        return false;
    }
    for (size_t index = 0; avCodec_->sample_fmts[index] != AV_SAMPLE_FMT_NONE; ++index) {
        if (avCodec_->sample_fmts[index] == avCodecContext_->sample_fmt) {
            return false;
        }
    }
    return true;
}

Status AudioFfmpegEncoderPlugin::Start()
{
    Prepare();

    needReformat_ = CheckReformat();
    if (needReformat_) {
        srcFmt_ = avCodecContext_->sample_fmt;
        // always use the first fmt
        avCodecContext_->sample_fmt = avCodec_->sample_fmts[0];
    }
    auto res = avcodec_open2(avCodecContext_.get(), avCodec_.get(), nullptr);
    FALSE_RETURN_V_MSG_E(res == 0, Status::ERROR_UNKNOWN, "avcodec open error " PUBLIC_LOG_S " when start encoder",
                      AVStrError(res).c_str());
    FALSE_RETURN_V_MSG_E(avCodecContext_->frame_size > 0, Status::ERROR_UNKNOWN, "frame_size unknown");
    fullInputFrameSize_ = (uint32_t)av_samples_get_buffer_size(nullptr, avCodecContext_->channels,
        avCodecContext_->frame_size, srcFmt_, 1);
    srcBytesPerSample_ = av_get_bytes_per_sample(srcFmt_) * avCodecContext_->channels;
    if (needReformat_) {
        Ffmpeg::ResamplePara resamplePara = {
            static_cast<int32_t>(avCodecContext_->channels),
            static_cast<int32_t>(avCodecContext_->sample_rate),
            0,
            static_cast<int64_t>(avCodecContext_->channel_layout),
            srcFmt_,
            static_cast<int32_t>(avCodecContext_->frame_size),
            avCodecContext_->sample_fmt,
        };
        resample_ = std::make_shared<Ffmpeg::Resample>();
        FALSE_RETURN_V_MSG(resample_->Init(resamplePara) == Status::OK, Status::ERROR_UNKNOWN, "Resample init error");
    }
   // SetParameter(Tag::AUDIO_SAMPLE_PER_FRAME, static_cast<uint32_t>(avCodecContext_->frame_size));
    return OpenCtxLocked();
}

Status AudioFfmpegEncoderPlugin::CloseCtxLocked()
{
    if (avCodecContext_ != nullptr) {
        auto res = avcodec_close(avCodecContext_.get());
        if (res != 0) {
            std::cout << "avcodec close error " << OSAL::AVStrError(res).c_str() << std::endl;
            return Status::ERROR_UNKNOWN;
        }
    }
    return Status::OK;
}

Status AudioFfmpegEncoderPlugin::StopLocked()
{
    std::cout << "StopLocked enter." << std::endl;
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    if (outBuffer_) {
        outBuffer_.reset();
    }
    return ret;
}

Status AudioFfmpegEncoderPlugin::Stop()
{
    std::cout << "Stop enter." << std::endl;
    std::lock_guard<std::mutex> lock(avMutex_);
    return StopLocked();
}

Status AudioFfmpegEncoderPlugin::Release()
{
    std::cout << "Release enter." << std::endl;
    return Status::OK;
}

Status AudioFfmpegEncoderPlugin::Flush()
{
    std::cout << "Flush entered." << std::endl;
    std::lock_guard<std::mutex> lock(avMutex_);
    if (avCodecContext_ != nullptr) {
        avcodec_flush_buffers(avCodecContext_.get());
    }
    std::cout << "Flush exit." << std::endl;
    return Status::OK;
}

Status AudioFfmpegEncoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    std::cout << "GetInputBuffers entered." << std::endl;
    std::cout << "GetInputBuffers exit." << std::endl;
    return Status::OK;
}

Status AudioFfmpegEncoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    std::cout << "GetInputBuffers entered." << std::endl;
    std::cout << "GetInputBuffers exit." << std::endl;
    return Status::OK;
}

Status AudioFfmpegEncoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer>& inputAvBuffer)
{
    std::cout << "queue input buffer" << std::endl;
    auto inputBuffer = inputAvBuffer->memory_;
    if (inputBuffer->GetSize() == 0 && !(inputAvBuffer->flag_ & BUFFER_FLAG_EOS)) {
        std::cout << "Encoder does not support fd buffer." << std::endl;
        return Status::ERROR_INVALID_DATA;
    }
    Status ret = Status::OK;
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        if (avCodecContext_ == nullptr) {
            return Status::ERROR_WRONG_STATE;
        }
        ret = SendBufferLocked(inputAvBuffer);
        if (ret == Status::OK || ret == Status::END_OF_STREAM) {
            std::lock_guard<std::mutex> l(bufferMetaMutex_);
            if (inputBuffer == nullptr || inputAvBuffer->meta_ == nullptr) {
                std::cout << "Encoder input buffer is null or get buffer meta fail." << std::endl;
                return Status::ERROR_INVALID_DATA;
            }
            bufferMeta_ = inputAvBuffer->meta_;
        }
        dataCallback_->OnInputBufferDone(inputAvBuffer);
    }
    return ret;
}

Status AudioFfmpegEncoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer>& outputBuffer)
{
    std::cout << "queue output buffer"  << std::endl;
    if (!outputBuffer) {
        std::cout << "Queue out buffer is null." << std::endl;
        return Status::ERROR_INVALID_PARAMETER;
    }
    outBuffer_ = outputBuffer;
    return SendOutputBuffer();
}

Status AudioFfmpegEncoderPlugin::SendOutputBuffer()
{
    std::cout << "send output buffer" << std::endl;
    Status status = ReceiveBuffer();
    if (status == Status::OK || status == Status::END_OF_STREAM) {
        {
            std::lock_guard<std::mutex> l(bufferMetaMutex_);
            if (outBuffer_ == nullptr) {
                return Status::ERROR_NULL_POINTER;
            }
            outBuffer_->meta_ = bufferMeta_;
        }
        dataCallback_->OnOutputBufferDone(outBuffer_);
    }
    outBuffer_.reset();
    return status;
}

void AudioFfmpegEncoderPlugin::FillInFrameCache(const std::shared_ptr<AVMemory>& mem)
{
    uint8_t* sampleData = nullptr;
    int32_t nbSamples = 0;
    auto srcBuffer = mem->GetAddr();
    auto destBuffer = const_cast<uint8_t*>(srcBuffer);
    auto srcLength = mem->GetSize();
    size_t destLength = srcLength;
    if (needReformat_ && resample_) {
        FALSE_LOG(resample_->Convert(srcBuffer, srcLength, destBuffer, destLength) == Status::OK);
        if (destLength) {
            sampleData = destBuffer;
            nbSamples = destLength / av_get_bytes_per_sample(avCodecContext_->sample_fmt) / avCodecContext_->channels;
        }
    } else {
        sampleData = destBuffer;
        nbSamples = destLength / srcBytesPerSample_;
    }
    cachedFrame_->format = avCodecContext_->sample_fmt;
    cachedFrame_->sample_rate = avCodecContext_->sample_rate;
    cachedFrame_->channels = avCodecContext_->channels;
    cachedFrame_->channel_layout = avCodecContext_->channel_layout;
    cachedFrame_->nb_samples = nbSamples;
    if (av_sample_fmt_is_planar(avCodecContext_->sample_fmt) && avCodecContext_->channels > 1) {
        if (avCodecContext_->channels > AV_NUM_DATA_POINTERS) {
            av_freep(cachedFrame_->extended_data);
            cachedFrame_->extended_data = static_cast<uint8_t**>(av_malloc_array(avCodecContext_->channels,
                sizeof(uint8_t *)));
        } else {
            cachedFrame_->extended_data = cachedFrame_->data;
        }
        cachedFrame_->extended_data[0] = sampleData;
        cachedFrame_->linesize[0] = nbSamples * av_get_bytes_per_sample(avCodecContext_->sample_fmt);
        for (int i = 1; i < avCodecContext_->channels; i++) {
            cachedFrame_->extended_data[i] = cachedFrame_->extended_data[i-1] + cachedFrame_->linesize[0];
        }
    } else {
        cachedFrame_->data[0] = sampleData;
        cachedFrame_->extended_data = cachedFrame_->data;
        cachedFrame_->linesize[0] = nbSamples * av_get_bytes_per_sample(avCodecContext_->sample_fmt) *
            avCodecContext_->channels;
    }
}

Status AudioFfmpegEncoderPlugin::SendBufferLocked(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    bool eos = false;
    if (inputBuffer == nullptr || (inputBuffer->flag_ & BUFFER_FLAG_EOS) != 0) {
        // eos buffer
        eos = true;
    } else {
        auto inputMemory = inputBuffer->memory_;
        FALSE_RETURN_V_MSG_W(inputMemory->GetSize() == fullInputFrameSize_, Status::ERROR_NOT_ENOUGH_DATA,
            "Not enough data, input: " PUBLIC_LOG_ZU ", fullInputFrameSize: " PUBLIC_LOG_U32,
            inputMemory->GetSize(), fullInputFrameSize_);
        FillInFrameCache(inputMemory);
    }
    AVFrame* inputFrame = nullptr;
    if (!eos) {
        inputFrame = cachedFrame_.get();
    }
    auto ret = avcodec_send_frame(avCodecContext_.get(), inputFrame);
    if (!eos && inputFrame) {
        av_frame_unref(inputFrame);
    }
    if (ret == 0) {
        return Status::OK;
    } else if (ret == AVERROR_EOF) {
        return Status::END_OF_STREAM;
    } else if (ret == AVERROR(EAGAIN)) {
        return Status::ERROR_AGAIN;
    } else {
        MEDIA_LOG_E("send buffer error " PUBLIC_LOG_S, AVStrError(ret).c_str());
        return Status::ERROR_UNKNOWN;
    }
}

Status AudioFfmpegEncoderPlugin::ReceiveFrameSucc(const std::shared_ptr<AVBuffer>& ioInfo, const std::shared_ptr<AVPacket>& packet)
{
    auto ioInfoMem = ioInfo->memory_;
    FALSE_RETURN_V_MSG_W(ioInfoMem->GetCapacity() >= static_cast<size_t>(packet->size),
                         Status::ERROR_NO_MEMORY, "buffer size is not enough");
    ioInfoMem->Write(packet->data, packet->size, 0);
    // how get perfect pts with upstream pts ?
    ioInfo->duration_ = ConvertTimeFromFFmpeg(packet->duration, avCodecContext_->time_base);
    ioInfo->pts_ = (UINT64_MAX - prev_pts_ < static_cast<uint64_t>(packet->duration)) ?
                  (ioInfo->duration_ - (UINT64_MAX - prev_pts_)) :
                  (prev_pts_ + ioInfo->duration_);
    prev_pts_ = ioInfo->pts_;
    return Status::OK;
}
/*
 Audio/Video Track is composed of multiple BufferGroups,
 and BufferGroup is composed of multiple Buffers.
 Each BufferGroup has a pts, it's the pts of the first buffer in group.
 We should calculate the other buffer's pts.
┌────────────────────────────────────────────┐
│                                            │
│         Audio / Video Track                │
│                                            │
├─────────────────────┬──────────────────────┤
│                     │                      │
│    BufferGroup      │   BufferGroup        │
│                     │                      │
├──────┬──────┬───────┼──────┬───────┬───────┤
│      │      │       │      │       │       │
│Buffer│Buffer│Buffer │Buffer│Buffer │Buffer │
│      │      │       │      │       │       │
└──────┴──────┴───────┴──────┴───────┴───────┘
 */
Status AudioFfmpegEncoderPlugin::ReceiveBufferLocked(const std::shared_ptr<AVBuffer>& ioInfo)
{
    Status status;
    std::shared_ptr<AVPacket> packet = std::make_shared<AVPacket>();
    auto ret = avcodec_receive_packet(avCodecContext_.get(), packet.get());
    if (ret >= 0) {
        MEDIA_LOG_DD("receive one frame");
        status = ReceiveFrameSucc(ioInfo, packet);
    } else if (ret == AVERROR_EOF) {
        MEDIA_LOG_I("eos received");
        ioInfo->memory_->Reset();
        ioInfo->flag_ = BUFFER_FLAG_EOS;
        status = Status::END_OF_STREAM;
    } else if (ret == AVERROR(EAGAIN)) {
        status = Status::ERROR_NOT_ENOUGH_DATA;
    } else {
        MEDIA_LOG_E("audio encoder receive error: " PUBLIC_LOG_S, AVStrError(ret).c_str());
        status = Status::ERROR_UNKNOWN;
    }
    av_frame_unref(cachedFrame_.get());
    return status;
}

Status AudioFfmpegEncoderPlugin::ReceiveBuffer()
{
    std::shared_ptr<AVBuffer> ioInfo = outBuffer_;
    if ((ioInfo == nullptr) || ioInfo->memory_->GetCapacity() == 0 || ioInfo->meta_ == nullptr ||
#ifndef MEDIA_OHOS
        (ioInfo->memory_->GetMemoryType() != MemoryType::VIRTUAL_MEMORY)) {
#else
        (ioInfo->memory_->GetMemoryType() != MemoryType::SHARED_MEMORY)) {
#endif
        std::cout << "cannot fetch valid buffer to output"<< std::endl;
        return Status::ERROR_NO_MEMORY;
    }
    Status status = Status::OK;
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        if (avCodecContext_ == nullptr) {
            return Status::ERROR_WRONG_STATE;
        }
        status = ReceiveBufferLocked(ioInfo);
    }
    return status;
}

} // Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS
