/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "audio_mp3_encoder_plugin.h"
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "lame.h"
#include "common/pcm_convert.h"
#include <set>
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Mp3;
using namespace std;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioMp3EncoderPlugin"};
constexpr int32_t ONE_CHANNEL = 1;
constexpr int32_t TWO_CHANNELS = 2;
constexpr int32_t SUPPORT_SAMPLE_RATE = 9;
constexpr int32_t SUPPORT_BIT_RATE = 16;
constexpr int32_t ONE_THOUSAND_BITRATE = 1000; // 1khz
constexpr int32_t SAMPLE_RATE_16000 = 16000;
constexpr int32_t SAMPLE_RATE_32000 = 32000;
constexpr int32_t BIT_RATE_32000 = 32000;
constexpr int32_t BIT_RATE_64000 = 64000;
constexpr int32_t BIT_RATE_160000 = 160000;

constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4096;
constexpr int32_t LAME_BUFFER_SIZE_DEFAULT = 8640;
constexpr int32_t MAX_INPUT_BUFFER_SIZE_LIMIT = 10 * 1024 * 1024;  // 10MB
constexpr int32_t LAME_INPUT_BUFFER_SIZE_ONE_CHAN = 2304;  // 1152*2=2304
constexpr int32_t LAME_INPUT_BUFFER_SIZE_TWO_CHAN = 4608;  // 1152*2*2=4608
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 5120;        // 1152*2*2=4608

constexpr int32_t BUFFER_FLAG_EOS = 0x00000001;

constexpr int32_t BIT_RATE_PICK[SUPPORT_BIT_RATE] = {8000, 16000, 32000, 40000, 48000, 56000,
                                                     64000, 80000, 96000, 112000, 128000, 160000,
                                                     192000, 224000, 256000, 320000};
constexpr int32_t SAMPLE_RATE_PICK[SUPPORT_SAMPLE_RATE] = {8000, 11025, 12000, 16000, 22050,
                                                           24000, 32000, 44100, 48000};
constexpr int32_t DEFAULT_COMPRESSION_LEVEL = 5;

static std::set<AudioSampleFormat> extendSampleFormats = {
    AudioSampleFormat::SAMPLE_U8,
    AudioSampleFormat::SAMPLE_S16LE,
    AudioSampleFormat::SAMPLE_S24LE,
    AudioSampleFormat::SAMPLE_S32LE,
    AudioSampleFormat::SAMPLE_F32LE,
};

Status RegisterAudioEncoderPlugins(const std::shared_ptr<Register>& reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_ENCODER_MP3_NAME);
    definition.pluginType = PluginType::AUDIO_ENCODER;
    definition.rank = 100;  // 100
    definition.SetCreator([](const std::string& name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioMp3EncoderPlugin>(name);
    });

    Capability cap;

    cap.SetMime(MimeType::AUDIO_MPEG);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    definition.AddInCaps(cap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(definition) != Status::OK) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

void UnRegisterAudioEncoderPlugin() {}

PLUGIN_DEFINITION(Mp3AudioEncoder, LicenseType::APACHE_V2, RegisterAudioEncoderPlugins, UnRegisterAudioEncoderPlugin);
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mp3 {

struct AudioMp3EncoderPlugin::LameInfo {
    lame_global_flags* gfp;
};

AudioMp3EncoderPlugin::AudioMp3EncoderPlugin(const std::string& name)
    : CodecPlugin(std::move(name)),
      audioSampleFormat_(AudioSampleFormat::SAMPLE_S16LE),
      lameInitFlag(0),
      channels_(ONE_CHANNEL),
      bitrate_(0),
      pts_(0),
      sampleRate_(SUPPORT_SAMPLE_RATE),
      maxInputSize_(INPUT_BUFFER_SIZE_DEFAULT),
      maxOutputSize_(OUTPUT_BUFFER_SIZE_DEFAULT),
      outputSize_(0)
{
    std::lock_guard<std::mutex> lock(avMutex_);
    lameMp3Buffer = std::make_unique<unsigned char []>(LAME_BUFFER_SIZE_DEFAULT);
    lameInfo = std::make_unique<LameInfo>();
}

AudioMp3EncoderPlugin::~AudioMp3EncoderPlugin() {}

bool AudioMp3EncoderPlugin::CheckFormat()
{
    if (channels_ != ONE_CHANNEL && channels_ != TWO_CHANNELS) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin channels not supported");
        return false;
    }
    for (int32_t i = 0; i < SUPPORT_BIT_RATE; i++) {
        if (bitrate_ == BIT_RATE_PICK[i]) {
            break;
        } else if (i == SUPPORT_BIT_RATE - 1) {
            AVCODEC_LOGE("AudioMp3EncoderPlugin bitRate not supported");
            return false;
        }
    }
    for (int32_t i = 0; i < SUPPORT_SAMPLE_RATE; i++) {
        if (sampleRate_ == SAMPLE_RATE_PICK[i]) {
            break;
        } else if (i == SUPPORT_SAMPLE_RATE - 1) {
            AVCODEC_LOGE("AudioMp3EncoderPlugin sampleRate not supported");
            return false;
        }
    }
    if (!isEnableFormatConvert_ && audioSampleFormat_ != AudioSampleFormat::SAMPLE_S16LE) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin sampleFmt not supported");
        return false;
    }
    if (isEnableFormatConvert_ &&
        extendSampleFormats.find(audioSampleFormat_) == extendSampleFormats.end()) {
        AVCODEC_LOGE("enable convert but input sample format not support:%{public}d",
            static_cast<int32_t>(audioSampleFormat_));
        return false;
    }
    int32_t bytesPerSample = GetPcmBytesPerSample(audioSampleFormat_);
    if (bytesPerSample == 0) {
        AVCODEC_LOGE("get bytes per sample fail from:%{public}d", static_cast<int32_t>(audioSampleFormat_));
        return false;
    }
    originSampleByte_ = bytesPerSample;
    if (sampleRate_ < SAMPLE_RATE_16000 && bitrate_ > BIT_RATE_64000) {
        AVCODEC_LOGE("sample<16k,bitrate must <=64k");
        return false;
    } else if (sampleRate_ < SAMPLE_RATE_32000 && bitrate_ > BIT_RATE_160000) {
        AVCODEC_LOGE("sample<32k,bitrate must <=160k");
        return false;
    } else if (sampleRate_ >= SAMPLE_RATE_32000 && bitrate_ < BIT_RATE_32000) {
        AVCODEC_LOGE("sample>=32k,bitrate must >=32k");
        return false;
    }
    return true;
}

Status AudioMp3EncoderPlugin::Init()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    if (lameInfo == nullptr) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin lameInfo allocation failed");
        return Status::ERROR_UNKNOWN;
    }
    lameInfo->gfp = lame_init();
    lameInitFlag = 0;
    if (lameInfo->gfp == nullptr) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin LAME initialization error");
        return Status::ERROR_UNKNOWN;
    }
    if (lameMp3Buffer == nullptr) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin lameMp3Buffer allocation failed");
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioMp3EncoderPlugin::Start()
{
    if (!CheckFormat()) {
        AVCODEC_LOGE("Format check failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    return Status::OK;
}

Status AudioMp3EncoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    auto memory = inputBuffer->memory_;
    auto inputSize = memory->GetSize();
    CHECK_AND_RETURN_RET_LOG(inputSize >= 0, Status::ERROR_UNKNOWN,
        "SendBuffer buffer is less than zero.  size: %{public}d", inputSize);
    if (inputSize == 0 && !(inputBuffer->flag_ & BUFFER_FLAG_EOS)) {
        AVCODEC_LOGE("size is 0, but eos flag is not 1");
        return Status::ERROR_INVALID_DATA;
    }
    if (inputSize > memory->GetCapacity()) {
        AVCODEC_LOGE("send input buffer is > allocate size. size : %{public}d, allocate size : %{public}d",
                     inputSize, memory->GetCapacity());
        return Status::ERROR_UNKNOWN;
    }
    std::lock_guard<std::mutex> lock(avMutex_);
    if (enableCache_) {
        std::vector<uint8_t> tmpBuffer;
        if (isEnableFormatConvert_ && audioSampleFormat_ != AudioSampleFormat::SAMPLE_S16LE) {
            size_t sampleNum = static_cast<size_t>(inputSize) / static_cast<size_t>(originSampleByte_);
            tmpBuffer.resize(sampleNum * sizeof(int16_t));
            ConvertPcmSampleFormat(memory->GetAddr(), sampleNum, audioSampleFormat_,
                AudioSampleFormat::SAMPLE_S16LE, tmpBuffer.data());
            pcmCache_.insert(pcmCache_.end(), tmpBuffer.begin(), tmpBuffer.end());
        } else {
            pcmCache_.insert(pcmCache_.end(), memory->GetAddr(), memory->GetAddr() + inputSize);
        }
    } else {
        if ((channels_ == ONE_CHANNEL && inputSize > LAME_INPUT_BUFFER_SIZE_ONE_CHAN) ||
            (channels_ == TWO_CHANNELS && inputSize > LAME_INPUT_BUFFER_SIZE_TWO_CHAN)) {
            AVCODEC_LOGE("SendBuffer size > max InputBufferSize(1 ch: 2304; 2 ch: 4608) size: %{public}d",
                         inputSize);
            return Status::ERROR_UNKNOWN;
        }
        EncodeDirect(inputBuffer);
    }
    if (ptsMode_ == FIRST_INPUT_START_ENCODE_PTS_MODE && isFirstInputPts_) {
        prevPts_ = inputBuffer->pts_;
        isFirstInputPts_ = false;
    }
    SafeCallInputBufferDone(dataCallback_, inputBuffer);
    return Status::OK;
}

Status AudioMp3EncoderPlugin::QueueOutputBufferBasic(std::shared_ptr<AVBuffer> &outputBuffer)
{
    auto memory = outputBuffer->memory_;
    if (outputBuffer->flag_ & BUFFER_FLAG_EOS) {
        SafeCallOutputBufferDone(dataCallback_, outputBuffer);
        return Status::END_OF_STREAM;
    }
    if (outputSize_ == 0) {
        AVCODEC_LOGD("AudioMp3EncoderPlugin lame outputSize of this frame is 0.");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    memory->Write(const_cast<const uint8_t*>(lameMp3Buffer.get()), outputSize_, 0);
    memory->SetSize(outputSize_);
    SetOutputPts(outputBuffer);
    outputSize_ = 0;
    SafeCallOutputBufferDone(dataCallback_, outputBuffer);
    return Status::OK;
}

int32_t AudioMp3EncoderPlugin::EncodePcmFromCache(int32_t sampleNum, size_t frameBytes)
{
    const short* pcmData = reinterpret_cast<const short*>(pcmCache_.data());
    if (channels_ == ONE_CHANNEL) {
        return lame_encode_buffer(lameInfo->gfp, pcmData, pcmData, sampleNum,
                                  lameMp3Buffer.get(), LAME_BUFFER_SIZE_DEFAULT);
    }
    return lame_encode_buffer_interleaved(lameInfo->gfp, const_cast<short*>(pcmData),
                                          sampleNum, lameMp3Buffer.get(), LAME_BUFFER_SIZE_DEFAULT);
}

void AudioMp3EncoderPlugin::WriteOutputBuffer(std::shared_ptr<AVBuffer>& outputBuffer)
{
    auto memory = outputBuffer->memory_;
    if (outputSize_ > 0) {
        memory->Write(const_cast<const uint8_t*>(lameMp3Buffer.get()), outputSize_, 0);
        memory->SetSize(outputSize_);
        SetOutputPts(outputBuffer);
    } else {
        memory->SetSize(0);
    }
}

Status AudioMp3EncoderPlugin::EncodeOneFrame(std::shared_ptr<AVBuffer>& outputBuffer,
    size_t frameBytes, bool isEos)
{
    int32_t sampleNum = static_cast<int32_t>(frameBytes / static_cast<size_t>(channels_) / sizeof(int16_t));
    outputSize_ = EncodePcmFromCache(sampleNum, frameBytes);
    pcmCache_.erase(pcmCache_.begin(), pcmCache_.begin() + frameBytes);
    if (outputSize_ < 0) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin lame encode error.");
        return isEos ? OutputEos(outputBuffer, "encode error") : Status::ERROR_UNKNOWN;
    }
    if (outputSize_ == 0 && pcmCache_.size() >= frameBytes) {
        outputSize_ = EncodePcmFromCache(sampleNum, frameBytes);
        pcmCache_.erase(pcmCache_.begin(), pcmCache_.begin() + frameBytes);
        if (outputSize_ < 0) {
            AVCODEC_LOGE("AudioMp3EncoderPlugin lame encode error.");
            return isEos ? OutputEos(outputBuffer, "encode error") : Status::ERROR_UNKNOWN;
        }
    }
    if (outputSize_ > 0) {
        WriteOutputBuffer(outputBuffer);
        SafeCallOutputBufferDone(dataCallback_, outputBuffer);
        return Status::ERROR_AGAIN;
    }
    return Status::ERROR_NOT_ENOUGH_DATA;
}

Status AudioMp3EncoderPlugin::EncodeResidual(std::shared_ptr<AVBuffer>& outputBuffer)
{
    int32_t residualSampleNum = static_cast<int32_t>(
        pcmCache_.size() / static_cast<size_t>(channels_) / sizeof(int16_t));
    outputSize_ = EncodePcmFromCache(residualSampleNum, pcmCache_.size());
    pcmCache_.clear();
    if (outputSize_ < 0) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin lame encode error.");
        return OutputEos(outputBuffer, "residual encode error");
    }
    WriteOutputBuffer(outputBuffer);
    SafeCallOutputBufferDone(dataCallback_, outputBuffer);
    return Status::ERROR_AGAIN;
}

Status AudioMp3EncoderPlugin::FlushAndOutput(std::shared_ptr<AVBuffer>& outputBuffer)
{
    outputSize_ = lame_encode_flush(lameInfo->gfp, lameMp3Buffer.get(), LAME_BUFFER_SIZE_DEFAULT);
    outputSize_ = outputSize_ > 0 ? outputSize_ : 0;
    WriteOutputBuffer(outputBuffer);
    outputBuffer->flag_ = BUFFER_FLAG_EOS;
    SafeCallOutputBufferDone(dataCallback_, outputBuffer);
    return Status::OK;
}

Status AudioMp3EncoderPlugin::QueueOutputBufferCache(std::shared_ptr<AVBuffer> &outputBuffer)
{
    bool isEos = (outputBuffer->flag_ & BUFFER_FLAG_EOS) != 0;
    auto frameBytes = static_cast<size_t>(frameSize_ * channels_ * sizeof(int16_t));
    auto minResidualBytes = static_cast<size_t>(channels_ * sizeof(int16_t));
    if (pcmCache_.size() >= frameBytes) {
        return EncodeOneFrame(outputBuffer, frameBytes, isEos);
    }
    if (isEos && pcmCache_.size() > minResidualBytes) {
        return EncodeResidual(outputBuffer);
    }
    if (isEos) {
        return FlushAndOutput(outputBuffer);
    }
    return Status::ERROR_NOT_ENOUGH_DATA;
}

Status AudioMp3EncoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer>& outputBuffer)
{
    if (!outputBuffer) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Queue out buffer is null.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(avMutex_);
    if (!enableCache_) {
        return QueueOutputBufferBasic(outputBuffer);
    }
    return QueueOutputBufferCache(outputBuffer);
}

Status AudioMp3EncoderPlugin::Reset()
{
    AVCODEC_LOGD("Reset begins");
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        isFirstInputPts_ = true;
        prevPts_ = 0;
        ptsMode_ = DEFAULT_ENCODE_PTS_MODE;
        pcmCache_.clear();
        maxInputBufferSize_ = -1;
        enableCache_ = false;
        isEnableFormatConvert_ = false;
    }
    auto ret = Release();
    if (ret != Status::OK) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Reset Release error.");
        return ret;
    }
    ret = Init();
    if (ret != Status::OK) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Reset Init error.");
        return ret;
    }

    return Status::OK;
}

Status AudioMp3EncoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    if (lameInfo != nullptr && lameInfo->gfp != nullptr) {
        if (lameInitFlag == 0) {
            if (lame_init_params(lameInfo->gfp) < 0) {
                AVCODEC_LOGE("AudioMp3EncoderPlugin Release LAME parameter initialization error");
                return Status::ERROR_UNKNOWN;
            }
            lameInitFlag = 1;
        }
        int ret = lame_encode_flush(lameInfo->gfp, lameMp3Buffer.get(), LAME_BUFFER_SIZE_DEFAULT);
        if (ret < 0) {
            AVCODEC_LOGE("AudioMp3EncoderPlugin Release lame_encode_flush error.");
            return Status::ERROR_UNKNOWN;
        }
        (void)lame_close(lameInfo->gfp);

        lameInfo->gfp = nullptr;
        lameInitFlag = 0;
    }
    return Status::OK;
}

Status AudioMp3EncoderPlugin::Flush()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    prevPts_ = 0;
    pcmCache_.clear();
    return Status::OK;
}

Status AudioMp3EncoderPlugin::SetParameter(const std::shared_ptr<Meta>& parameter)
{
    std::lock_guard<std::mutex> lock(avMutex_);
    std::lock_guard<std::mutex> lock2(paramMutex_);
    if (!parameter->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_)) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin SetParameter error. no AUDIO_CHANNEL_COUNT");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (!parameter->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_)) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin SetParameter error. no AUDIO_SAMPLE_RATE");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (!parameter->Get<Tag::AUDIO_SAMPLE_FORMAT>(audioSampleFormat_)) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin SetParameter error. no AUDIO_SAMPLE_FORMAT");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (parameter->Get<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(isEnableFormatConvert_)) {
        AVCODEC_LOGI("set format convert:%{public}d", static_cast<int32_t>(isEnableFormatConvert_));
    }
    if (!parameter->Get<Tag::MEDIA_BITRATE>(bitrate_)) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin SetParameter error. no MEDIA_BITRATE of mp3 encoder CBR");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
        AVCODEC_LOGD("AudioMp3EncoderPlugin SetParameter maxInputSize_: %{public}d", maxInputSize_);
    }
    if (!CheckFormat()) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin SetParameter error. CheckFormat fail");
        return Status::ERROR_INVALID_PARAMETER;
    }
    audioParameter_ = *parameter;
    ConfigureLame(parameter);
    return Status::OK;
}

void AudioMp3EncoderPlugin::EncodeDirect(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    auto memory = inputBuffer->memory_;
    auto inputSize = memory->GetSize();
    int32_t sampleNumTmp = channels_ == 1 ? inputSize / originSampleByte_ :
        inputSize / originSampleByte_ / TWO_CHANNELS;
    int outputSize = 0;
    short *inputPcmBuffer = reinterpret_cast<short *>(memory->GetAddr());
    std::vector<uint8_t> tmpBuffer;
    if (isEnableFormatConvert_ && audioSampleFormat_ != AudioSampleFormat::SAMPLE_S16LE) {
        size_t sampleNum = static_cast<size_t>(inputSize) / static_cast<size_t>(originSampleByte_);
        tmpBuffer.resize(sampleNum * sizeof(int16_t));
        ConvertPcmSampleFormat(reinterpret_cast<uint8_t *>(inputPcmBuffer), sampleNum, audioSampleFormat_,
            AudioSampleFormat::SAMPLE_S16LE, tmpBuffer.data());
        inputPcmBuffer = reinterpret_cast<short *>(tmpBuffer.data());
    }
    if (sampleNumTmp > 0) {
        if (channels_ == 1) {
            outputSize = lame_encode_buffer(lameInfo->gfp, inputPcmBuffer, inputPcmBuffer, sampleNumTmp,
                                            lameMp3Buffer.get(), LAME_BUFFER_SIZE_DEFAULT);
        } else {
            outputSize = lame_encode_buffer_interleaved(lameInfo->gfp, inputPcmBuffer,
                                                        sampleNumTmp, lameMp3Buffer.get(), LAME_BUFFER_SIZE_DEFAULT);
        }
    } else if (sampleNumTmp == 0) {
        outputSize = lame_encode_flush(lameInfo->gfp, lameMp3Buffer.get(), LAME_BUFFER_SIZE_DEFAULT);
    }
    if (outputSize < 0) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin lame encode error.");
        return;
    }
    outputSize_ = outputSize;
    pts_ = inputBuffer->pts_;
}

void AudioMp3EncoderPlugin::SetOutputPts(std::shared_ptr<AVBuffer>& outputBuffer)
{
    int64_t frameDuration = static_cast<int64_t>(frameSize_) * 1000000LL / sampleRate_;
    if (ptsMode_ == ZERO_START_ENCODE_PTS_MODE || ptsMode_ == FIRST_INPUT_START_ENCODE_PTS_MODE) {
        outputBuffer->pts_ = prevPts_;
        prevPts_ = (prevPts_ > INT64_MAX - frameDuration) ? prevPts_ : prevPts_ + frameDuration;
    } else {
        outputBuffer->pts_ = pts_;
    }
}

Status AudioMp3EncoderPlugin::OutputEos(std::shared_ptr<AVBuffer>& outputBuffer, const char* reason)
{
    AVCODEC_LOGW("AudioMp3EncoderPlugin OutputEos: %{public}s", reason);
    auto memory = outputBuffer->memory_;
    pcmCache_.clear();
    outputSize_ = lame_encode_flush(lameInfo->gfp, lameMp3Buffer.get(), LAME_BUFFER_SIZE_DEFAULT);
    outputSize_ = outputSize_ > 0 ? outputSize_ : 0;
    if (outputSize_ > 0) {
        memory->Write(const_cast<const uint8_t*>(lameMp3Buffer.get()), outputSize_, 0);
        memory->SetSize(outputSize_);
        SetOutputPts(outputBuffer);
    } else {
        memory->SetSize(0);
    }
    outputBuffer->flag_ = BUFFER_FLAG_EOS;
    SafeCallOutputBufferDone(dataCallback_, outputBuffer);
    return Status::OK;
}

void AudioMp3EncoderPlugin::ConfigureLame(const std::shared_ptr<Meta>& parameter)
{
    lame_set_in_samplerate(lameInfo->gfp, sampleRate_);
    lame_set_out_samplerate(lameInfo->gfp, sampleRate_);
    lame_set_num_channels(lameInfo->gfp, channels_);
    lame_set_quality(lameInfo->gfp, DEFAULT_COMPRESSION_LEVEL);
    if (channels_ == 1) {
        lame_set_mode(lameInfo->gfp, MPEG_mode_e::MONO);
    }
    lame_set_brate(lameInfo->gfp, bitrate_ / ONE_THOUSAND_BITRATE);
    if (lame_init_params(lameInfo->gfp) < 0) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin LAME parameter initialization error");
        return;
    }
    int32_t frameSize = lame_get_framesize(lameInfo->gfp);
    audioParameter_.Set<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize);
    frameSize_ = frameSize;
    lameInitFlag = 1;
    int32_t ptsMode = DEFAULT_ENCODE_PTS_MODE;
    if (parameter->Get<Tag::AUDIO_ENCODE_PTS_MODE>(ptsMode)) {
        AVCODEC_LOGI("mp3 encode set pts mode:%{public}d", static_cast<int32_t>(ptsMode));
        ptsMode_ = ptsMode;
    }
    ConfigureCache(parameter);
}

void AudioMp3EncoderPlugin::ConfigureCache(const std::shared_ptr<Meta>& parameter)
{
    maxInputBufferSize_ = -1;
    enableCache_ = false;
    if (parameter->Get<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(maxInputBufferSize_) && maxInputBufferSize_ > 0) {
        if (maxInputBufferSize_ > MAX_INPUT_BUFFER_SIZE_LIMIT) {
            maxInputBufferSize_ = MAX_INPUT_BUFFER_SIZE_LIMIT;
        }
        enableCache_ = true;
    }
}

Status AudioMp3EncoderPlugin::GetParameter(std::shared_ptr<Meta>& parameter)
{
    std::lock_guard<std::mutex> lock(paramMutex_);
    if (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    AVCODEC_LOGD("AudioMp3EncoderPlugin GetParameter maxInputSize_: %{public}d", maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    audioParameter_.Set<Tag::MIME_TYPE>(MimeType::AUDIO_MPEG);
    if (maxInputBufferSize_ > 0) {
        audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputBufferSize_);
    } else {
        audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    }
    audioParameter_.Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    audioParameter_.Set<Tag::MEDIA_BITRATE>(bitrate_);
    *parameter = audioParameter_;
    return Status::OK;
}

Status AudioMp3EncoderPlugin::Prepare()
{
    return Status::OK;
}

Status AudioMp3EncoderPlugin::Stop()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    if (!lameInfo || !lameMp3Buffer) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Stop lameInfo or lameMp3Buffer is nullptr");
        return Status::ERROR_NULL_POINTER;
    }
    int result = lame_encode_flush(lameInfo->gfp, lameMp3Buffer.get(), LAME_BUFFER_SIZE_DEFAULT);
    if (result < 0) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Stop lame_encode_flush error.");
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioMp3EncoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>>& inputBuffers)
{
    return Status::OK;
}

Status AudioMp3EncoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>>& outputBuffers)
{
    return Status::OK;
}

}  // namespace Mp3
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS