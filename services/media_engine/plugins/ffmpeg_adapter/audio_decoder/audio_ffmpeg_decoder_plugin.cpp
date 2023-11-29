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

#define HST_LOG_TAG "Ffmpeg_Au_Decoder"

#include "audio_ffmpeg_decoder_plugin.h"
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include "osal/utils/util.h"
#include "common/log.h"
#include "meta/mime_type.h"
#include "plugin_caps_builder.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugin;
using namespace Ffmpeg;

std::map<std::string, std::shared_ptr<const AVCodec>> codecMap;

const size_t BUFFER_QUEUE_SIZE = 6;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 8192;
// constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4 * 1024 * 8;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;

const std::set<AVCodecID> g_supportedCodec = {
    AV_CODEC_ID_MP3,
    AV_CODEC_ID_FLAC,
    AV_CODEC_ID_AAC,
    AV_CODEC_ID_AAC_LATM,
    AV_CODEC_ID_VORBIS,
    AV_CODEC_ID_APE,
    AV_CODEC_ID_AMR_NB,
    AV_CODEC_ID_AMR_WB,
    AV_CODEC_ID_OPUS,
};

std::map<AVCodecID, uint32_t> samplesPerFrameMap = {
    {AV_CODEC_ID_MP3, 1152}, // 1152
    {AV_CODEC_ID_FLAC, 8192}, // 8192
    {AV_CODEC_ID_AAC, 2048},  // 2048
    {AV_CODEC_ID_AAC_LATM, 2048}, // 2048
    {AV_CODEC_ID_VORBIS, 8192}, // 8192
    {AV_CODEC_ID_APE, 4608}, // 4608
    {AV_CODEC_ID_AMR_NB, 160}, // 160
    {AV_CODEC_ID_AMR_WB, 160}, // 160
    {AV_CODEC_ID_OPUS, 960}, // 960
};

void UpdatePluginDefinition(const AVCodec* codec, CodecPluginDef& definition);
void SetCapMime(const AVCodec* codec, Capability& cap);

void SetCapMime(const AVCodec* codec, Capability& cap)
{
    switch (codec->id) {
        case AV_CODEC_ID_MP3:
            cap.SetMime(MimeType::AUDIO_MPEG);
            break;
        case AV_CODEC_ID_AAC:
            cap.SetMime(MimeType::AUDIO_AAC);
            break;
        case AV_CODEC_ID_AAC_LATM:
            cap.SetMime("audio/aac-latm");
            break;
        case AV_CODEC_ID_VORBIS:
            cap.SetMime(MimeType::AUDIO_VORBIS);
            break;
        case AV_CODEC_ID_APE:
            cap.SetMime("audio/ape");
            break;
        case AV_CODEC_ID_AMR_NB:
            cap.SetMime(MimeType::AUDIO_AMR_NB);
            break;
        case AV_CODEC_ID_AMR_WB:
            cap.SetMime(MimeType::AUDIO_AMR_WB);
            break;
        case AV_CODEC_ID_OPUS:
            cap.SetMime(MimeType::AUDIO_OPUS);
            break;
        case AV_CODEC_ID_FLAC:
            cap.SetMime(MimeType::AUDIO_FLAC);
            break;
        default:
            MEDIA_LOG_I("codec is not supported right now");
    }
}

void UpdateInCaps(const AVCodec* codec, CodecPluginDef& definition)
{
    Capability cap;
    SetCapMime(codec, cap);

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

Status RegisterAudioDecoderPlugins(const std::shared_ptr<Register>& reg)
{
    const AVCodec* codec = nullptr;
    void* ite = nullptr;
    std::cout << "registering audio decoders" << std::endl;
    while ((codec = av_codec_iterate(&ite))) {
        if (!av_codec_is_decoder(codec) || codec->type != AVMEDIA_TYPE_AUDIO) {
            continue;
        }
        if (g_supportedCodec.find(codec->id) == g_supportedCodec.end()) {
            std::cout << "codec "<< codec->name << codec->long_name << "is not supported right now" << std::endl;
            continue;
        }
        CodecPluginDef definition;
        definition.name = "ffmpegAuDec_" + std::string(codec->name);
        definition.pluginType = PluginType::AUDIO_DECODER;
        definition.rank = 100; // 100
        definition.SetCreator([](const std::string& name) -> std::shared_ptr<CodecPlugin> {
            return std::make_shared<AudioFfmpegDecoderPlugin>(name);
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

void UnRegisterAudioDecoderPlugin()
{
    codecMap.clear();
}
} // namespace
PLUGIN_DEFINITION(FFmpegAudioDecoders, LicenseType::LGPL, RegisterAudioDecoderPlugins, UnRegisterAudioDecoderPlugin);

namespace OHOS {
namespace Media {
namespace Plugin {
namespace Ffmpeg {
AudioFfmpegDecoderPlugin::AudioFfmpegDecoderPlugin(std::string name) : CodecPlugin(std::move(name)),
    channels_(0), sampleRate_(0), bit_rate_(0), maxInputSize_(INPUT_BUFFER_SIZE_DEFAULT)
{
}

AudioFfmpegDecoderPlugin::~AudioFfmpegDecoderPlugin()
{
    DeInitLocked();
}

Status AudioFfmpegDecoderPlugin::Init()
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

    {
        std::lock_guard<std::mutex> lock1(parameterMutex_);
        audioParameter_[Tag::REQUIRED_OUT_BUFFER_CNT] = (uint32_t) BUFFER_QUEUE_SIZE;
        if (samplesPerFrameMap.count(avCodec_->id)) {
            audioParameter_[Tag::AUDIO_SAMPLE_PER_FRAME] = samplesPerFrameMap[avCodec_->id];
        } else {
            return Status::ERROR_UNSUPPORTED_FORMAT;
        }
    }
#ifdef DUMP_RAW_DATA
    auto deleter = [](std::ofstream* fs){
            if (fs != nullptr) {
                if(fs->is_open()) {
                    fs->flush();
                    fs->close();
                }
                fs->close();
            }};
        dumpDataOutputAVFrameFs_ = std::shared_ptr<std::ofstream>(
            new std::ofstream("./ffmpeg_adecoder_dumpoutput_avframe", std::ios::binary | std::ios::trunc), deleter);
        dumpDataOutputAVBufferFs_ = std::shared_ptr<std::ofstream>(
            new std::ofstream("./ffmpeg_adecoder_dumpoutput_avbuffer", std::ios::binary | std::ios::trunc), deleter);
#endif
    return Status::OK;
}

/*
Status AudioFfmpegDecoderPlugin::Deinit()
{
    MEDIA_LOG_I("Deinit enter.");
    std::lock_guard<std::mutex> lock(avMutex_);
    std::lock_guard<std::mutex> lock1(parameterMutex_);
    return DeInitLocked();
}

*/
Status AudioFfmpegDecoderPlugin::DeInitLocked()
{
    std::cout << "DeInitLocked enter." << std::endl;
    ResetLocked();
    avCodec_.reset();
    cachedFrame_.reset();
    return Status::OK;
}

Status AudioFfmpegDecoderPlugin::SetParameter(const std::shared_ptr<Meta> parameter)
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

    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    }
    audioParameter_ = *parameter;
    return Status::OK;
}

Status AudioFfmpegDecoderPlugin::GetParameter(std::shared_ptr<Meta> parameter)
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

Status AudioFfmpegDecoderPlugin::Prepare()
{
    AVCodecContext* context = nullptr;
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        FALSE_RETURN_V(avCodec_ != nullptr, Status::ERROR_WRONG_STATE);
        context = avcodec_alloc_context3(avCodec_.get());
    }
    FALSE_RETURN_V_MSG_E(context != nullptr, Status::ERROR_NO_MEMORY, "can't allocate codec context");
    auto tmpCtx = std::shared_ptr<AVCodecContext>(context, [](AVCodecContext* ptr) {
        avcodec_free_context(&ptr);
    });
    {
        std::lock_guard<std::mutex> lock1(parameterMutex_);
        tmpCtx->channels = channels_;
        tmpCtx->sample_rate = sampleRate_;
        tmpCtx->bit_rate = bit_rate_;
        tmpCtx->sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16; // need recheck
        tmpCtx->request_sample_fmt = tmpCtx->sample_fmt;
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

Status AudioFfmpegDecoderPlugin::ResetLocked()
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

Status AudioFfmpegDecoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    std::lock_guard<std::mutex> lock1(parameterMutex_);
    return ResetLocked();
}

Status AudioFfmpegDecoderPlugin::OpenCtxLocked()
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

Status AudioFfmpegDecoderPlugin::Start()
{
    Prepare();
    return OpenCtxLocked();
}

Status AudioFfmpegDecoderPlugin::CloseCtxLocked()
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

Status AudioFfmpegDecoderPlugin::StopLocked()
{
    std::cout << "StopLocked enter." << std::endl;
    auto ret = CloseCtxLocked();
    avCodecContext_.reset();
    if (outBuffer_) {
        outBuffer_.reset();
    }
    return ret;
}

Status AudioFfmpegDecoderPlugin::Stop()
{
    std::cout << "Stop enter." << std::endl;
    std::lock_guard<std::mutex> lock(avMutex_);
    return StopLocked();
}

Status AudioFfmpegDecoderPlugin::Release()
{
    std::cout << "Release enter." << std::endl;
    return Status::OK;
}

Status AudioFfmpegDecoderPlugin::Flush()
{
    std::cout << "Flush entered." << std::endl;
    std::lock_guard<std::mutex> lock(avMutex_);
    if (avCodecContext_ != nullptr) {
        avcodec_flush_buffers(avCodecContext_.get());
    }
    std::cout << "Flush exit." << std::endl;
    return Status::OK;
}

Status AudioFfmpegDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    std::cout << "GetInputBuffers entered." << std::endl;
    std::cout << "GetInputBuffers exit." << std::endl;
    return Status::OK;
}

Status AudioFfmpegDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    std::cout << "GetInputBuffers entered." << std::endl;
    std::cout << "GetInputBuffers exit." << std::endl;
    return Status::OK;
}

Status AudioFfmpegDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer>& inputAvBuffer)
{
    std::cout << "queue input buffer" << std::endl;
    auto inputBuffer = inputAvBuffer->memory_;
    if (inputBuffer->GetSize() == 0 && !(inputAvBuffer->flag_ & BUFFER_FLAG_EOS)) {
        std::cout << "Decoder does not support fd buffer." << std::endl;
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
                std::cout << "Decoder input buffer is null or get buffer meta fail." << std::endl;
                return Status::ERROR_INVALID_DATA;
            }
            bufferMeta_ = inputAvBuffer->meta_;
        }
        dataCallback_->OnInputBufferDone(inputAvBuffer);
    }
    return ret;
}

Status AudioFfmpegDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer>& outputBuffer)
{
    std::cout << "queue output buffer"  << std::endl;
    if (!outputBuffer) {
        std::cout << "Queue out buffer is null." << std::endl;
        return Status::ERROR_INVALID_PARAMETER;
    }
    outBuffer_ = outputBuffer;
    return SendOutputBuffer();
}

Status AudioFfmpegDecoderPlugin::SendOutputBuffer()
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

Status AudioFfmpegDecoderPlugin::SendBufferLocked(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    if (inputBuffer && !(inputBuffer->flag_ & BUFFER_FLAG_EOS)) {
        auto inputMemory = inputBuffer->memory_;
        const uint8_t* ptr = inputMemory->GetAddr();
        auto bufferLength = inputMemory->GetSize();
        // pad to data if needed
        if (bufferLength % AV_INPUT_BUFFER_PADDING_SIZE != 0) {
            if (paddedBufferSize_ < bufferLength + AV_INPUT_BUFFER_PADDING_SIZE) {
                paddedBufferSize_ = bufferLength + AV_INPUT_BUFFER_PADDING_SIZE;
                paddedBuffer_.reserve(paddedBufferSize_);
                std::cout << "increase padded buffer size to " << paddedBufferSize_ << std::endl;
            }
            paddedBuffer_.assign(ptr, ptr + bufferLength);
            paddedBuffer_.insert(paddedBuffer_.end(), AV_INPUT_BUFFER_PADDING_SIZE, 0);
            ptr = paddedBuffer_.data();
        }
        avPacket_->data = const_cast<uint8_t*>(ptr);
        avPacket_->size = bufferLength;
        avPacket_->pts = inputBuffer->pts_;
    }
    auto ret = avcodec_send_packet(avCodecContext_.get(), avPacket_.get());
    av_packet_unref(avPacket_.get());
    if (ret == 0) {
        return Status::OK;
    } else if (ret == AVERROR(EAGAIN)) {
        return Status::ERROR_AGAIN;
    } else if (ret == AVERROR_EOF) {  // AVStrError(ret).c_str() == "End of file"
        return Status::END_OF_STREAM;
    } else {
        std::cout << "send buffer error " << OSAL::AVStrError(ret).c_str() << std::endl;
        return Status::ERROR_UNKNOWN;
    }
}

Status AudioFfmpegDecoderPlugin::ReceiveFrameSucc(const std::shared_ptr<AVBuffer>& ioInfo)
{
    if (ioInfo == nullptr) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    int32_t channels = cachedFrame_->channels;
    int32_t samples = cachedFrame_->nb_samples;
    auto sampleFormat = static_cast<AVSampleFormat>(cachedFrame_->format);
    int32_t bytePerSample = av_get_bytes_per_sample(sampleFormat);
    size_t outputSize =
            static_cast<size_t>(samples) * static_cast<size_t>(bytePerSample) * static_cast<size_t>(channels);
    auto ioInfoMem = ioInfo->memory_;
    if (ioInfoMem->GetCapacity() < outputSize) {
        std::cout << "output buffer size is not enough" << std::endl;
        return Status::ERROR_NO_MEMORY;
    }
    if (av_sample_fmt_is_planar(avCodecContext_->sample_fmt)) {
        size_t planarSize = outputSize / channels;
        for (int32_t idx = 0; idx < channels; idx++) {
            ioInfoMem->Write(cachedFrame_->extended_data[idx], planarSize, 0);
        }
    } else {
        ioInfoMem->Write(cachedFrame_->data[0], outputSize, 0);
    }
#ifdef DUMP_RAW_DATA
    if (dumpDataOutputAVFrameFs_ && ioInfoMem->GetAddr()) {
        dumpDataOutputAVFrameFs_->write(reinterpret_cast<const char*>(cachedFrame_->data[0]), outputSize);
    }
    if (dumpDataOutputAVBufferFs_ && ioInfoMem->GetAddr()) {
        dumpDataOutputAVBufferFs_->write(reinterpret_cast<const char*>(ioInfoMem->GetAddr() + ioInfoMem->GetOffset()),
                                         ioInfoMem->GetSize());
    }
#endif
    ioInfo->pts_ = static_cast<uint64_t>(cachedFrame_->pts);
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
Status AudioFfmpegDecoderPlugin::ReceiveBufferLocked(const std::shared_ptr<AVBuffer>& ioInfo)
{
    MEDIA_LOG_DD("AudioFfmpegDecoderPlugin::ReceiveBufferLocked begin");
    if (ioInfo == nullptr) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    Status status;
    auto ret = avcodec_receive_frame(avCodecContext_.get(), cachedFrame_.get());
    if (ret >= 0) {
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
                cachedFrame_->pts = curBufferGroupPts_ + abs(curBufferGroupPts_ - preBufferGroupPts_) *
                    (bufferIndex_ - 1) / bufferNum_;
            }
        }
        std::cout << "receive one frame" << std::endl;
        status = ReceiveFrameSucc(ioInfo);
    } else if (ret == AVERROR_EOF) {
        std::cout << "eos received" << std::endl;
        ioInfo->memory_->Reset();
        ioInfo->flag_ = BUFFER_FLAG_EOS;
        avcodec_flush_buffers(avCodecContext_.get());
        status = Status::END_OF_STREAM;
    } else if (ret == AVERROR(EAGAIN)) {
        status = Status::ERROR_NOT_ENOUGH_DATA;
    } else {
        std::cout << "audio decoder receive error:"<< OSAL::AVStrError(ret).c_str() << std::endl;
        status = Status::ERROR_UNKNOWN;
    }
    av_frame_unref(cachedFrame_.get());

    MEDIA_LOG_DD("AudioFfmpegDecoderPlugin::ReceiveBufferLocked end");
    return status;
}

Status AudioFfmpegDecoderPlugin::ReceiveBuffer()
{
    std::shared_ptr<AVBuffer> ioInfo {outBuffer_};
    if ((ioInfo == nullptr) || ioInfo->memory_->GetCapacity() == 0 || ioInfo->meta_ == nullptr ||
#ifndef MEDIA_OHOS
        (ioInfo->memory_->GetMemoryType() != MemoryType::VIRTUAL_MEMORY)) {
#else
        (ioInfo->memory_->GetMemoryType() != MemoryType::SHARED_MEMORY)) {
#endif
        MEDIA_LOG_E("cannot fetch valid buffer to output");
        return Status::ERROR_NO_MEMORY;
    }
    Status status;
    {
        std::lock_guard<std::mutex> l(avMutex_);
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
