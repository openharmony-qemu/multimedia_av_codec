/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "audio_opus_encoder_plugin.h"
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include "osal/utils/util.h"
#include "common/log.h"
#include "meta/mime_type.h"
#include "common/pcm_convert.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Opus;

constexpr std::string_view OPUS_ENCODE_NAME = "OH.Media.Codec.Encoder.Audio.Opus";
constexpr int32_t COMPLEX_DEFAULT = 10;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 1500;
constexpr float PACKET_MS = 0.02;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 2;
constexpr int32_t MIN_BITRATE = 6000;
constexpr int32_t MAX_BITRATE = 510000;
constexpr int32_t MAX_INPUT_BUFFER_SIZE_LIMIT = 10 * 1024 * 1024; // 10MB
static const int32_t OPUS_ENCODER_SAMPLE_RATE_TABLE[] = {8000, 12000, 16000, 24000, 48000};

static std::set<AudioSampleFormat> extendedSampleFormats = {
    AudioSampleFormat::SAMPLE_U8,
    AudioSampleFormat::SAMPLE_S16LE,
    AudioSampleFormat::SAMPLE_S24LE,
    AudioSampleFormat::SAMPLE_S32LE,
    AudioSampleFormat::SAMPLE_F32LE,
};

Status RegisterAudioEncoderPlugins(const std::shared_ptr<Register> &reg)  __attribute__((no_sanitize("cfi")))
{
    CodecPluginDef definition;
    definition.name = OPUS_ENCODE_NAME;
    definition.pluginType = PluginType::AUDIO_ENCODER;
    definition.rank = 100;  // 100
    definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioOpusEncoderPlugin>(name);
    });

    Capability cap;

    cap.SetMime(MimeType::AUDIO_OPUS);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    definition.AddInCaps(cap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(definition) != Status::OK) {
        MEDIA_LOG_E("AudioOpusEncoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

void UnRegisterAudioEncoderPlugin()
{}

PLUGIN_DEFINITION(OpusAudioEncoder, LicenseType::UNKNOWN, RegisterAudioEncoderPlugins, UnRegisterAudioEncoderPlugin);
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Opus {
AudioOpusEncoderPlugin::AudioOpusEncoderPlugin(std::string name)
    : CodecPlugin(std::move(name)), fbytes_(nullptr), encodeLen_(0), pluginCodecPtr_(nullptr), channels_(0),
      sampleRate_(0), bitrate_(0), maxInputSize_(INPUT_BUFFER_SIZE_DEFAULT), maxOutputSize_(OUTPUT_BUFFER_SIZE_DEFAULT),
      pts_(0), codeData_(nullptr), srcFmt_(AudioSampleFormat::SAMPLE_S16LE)
{}

AudioOpusEncoderPlugin::~AudioOpusEncoderPlugin()
{}

Status AudioOpusEncoderPlugin::Init()
{
    MEDIA_LOG_I("Create Opus");
    OpusPluginClassEncoderCreate(&pluginCodecPtr_);
    if (!pluginCodecPtr_) {
        MEDIA_LOG_E("AudioOpusEncoderPlugin Init fail, please check");
        return Status::ERROR_NULL_POINTER;
    }
    return Status::OK;
}

bool AudioOpusEncoderPlugin::CheckChannel(const std::shared_ptr<Meta> &parameter)
{
    if (parameter->Find(Tag::AUDIO_CHANNEL_COUNT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_);
        if (channels_ < MIN_CHANNELS || channels_ > MAX_CHANNELS) {
            MEDIA_LOG_E("AudioOpusEncoderPlugin SetParameter channels wrong, please check");
            return false;
        }
    } else {
        MEDIA_LOG_E("AudioOpusEncoderPlugin SetParameter channels missing, please check");
        return false;
    }
    return true;
}

bool AudioOpusEncoderPlugin::CheckFormat(const std::shared_ptr<Meta> &parameter)
{
    if (parameter->Find(Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(isEnableFormatConvert_);
        MEDIA_LOG_I("set format convert:%{public}d", static_cast<int32_t>(isEnableFormatConvert_));
    }
    if (parameter->Find(Tag::AUDIO_SAMPLE_FORMAT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_FORMAT>(srcFmt_);
        if (!isEnableFormatConvert_ && srcFmt_ != AudioSampleFormat::SAMPLE_S16LE) {
            MEDIA_LOG_E("AudioOpusEncoderPlugin SetParameter AudioSampleFormat wrong, please check");
            return false;
        }
        if (isEnableFormatConvert_ && extendedSampleFormats.find(srcFmt_) == extendedSampleFormats.end()) {
            MEDIA_LOG_E("AudioOpusEncoderPlugin SetParameter AudioSampleFormat not support, please check");
            return false;
        }
    } else {
        MEDIA_LOG_E("AudioOpusEncoderPlugin SetParameter AudioSampleFormat missing, please check");
        return false;
    }
    return true;
}

bool AudioOpusEncoderPlugin::CheckBitrate(const std::shared_ptr<Meta> &parameter)
{
    if (parameter->Find(Tag::MEDIA_BITRATE) != parameter->end()) {
        parameter->Get<Tag::MEDIA_BITRATE>(bitrate_);
        if (bitrate_ < MIN_BITRATE || bitrate_ > MAX_BITRATE) {
            MEDIA_LOG_E("AudioOpusEncoderPlugin SetParameter bitrate wrong, please check");
            return false;
        }
    } else {
        MEDIA_LOG_E("AudioOpusEncoderPlugin SetParameter bitrate missing, please check");
        return false;
    }
    return true;
}

bool AudioOpusEncoderPlugin::CheckSamplerate(const std::shared_ptr<Meta> &parameter)
{
    int32_t paraFlag = 0;
    if (parameter->Find(Tag::AUDIO_SAMPLE_RATE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
        for (auto i : OPUS_ENCODER_SAMPLE_RATE_TABLE) {
            if (i == sampleRate_) {
                paraFlag = 1;
                break;
            }
        }
    }
    if (paraFlag == 0) {
        MEDIA_LOG_E("AudioOpusEncoderPlugin SetParameter Samplerate wrong, please check");
        return false;
    }
    return true;
}

Status AudioOpusEncoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(paramMutex_);
    if (CheckChannel(parameter) == false) {
        MEDIA_LOG_E("SetParameter channel error");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (CheckFormat(parameter) == false) {
        MEDIA_LOG_E("SetParameter format error");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (CheckBitrate(parameter) == false) {
        MEDIA_LOG_E("SetParameter bitrate error");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (CheckSamplerate(parameter) == false) {
        MEDIA_LOG_E("SetParameter samplerate error");
        return Status::ERROR_INVALID_PARAMETER;
    }
    int32_t ptsMode = DEFAULT_ENCODE_PTS_MODE;
    if (parameter->Get<Tag::AUDIO_ENCODE_PTS_MODE>(ptsMode)) {
        ptsMode_ = ptsMode;
    }
    MEDIA_LOG_I("Opus SetPrameter success");
    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    } else {
        MEDIA_LOG_W("Opus SetParameter AUDIO_MAX_INPUT_SIZE missing, please check");
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    if (parameter->Find(Tag::AUDIO_MAX_OUTPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    } else {
        MEDIA_LOG_W("Opus SetParameter AUDIO_MAX_OUTPUT_SIZE missing, please check");
        maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    }
    maxInputBufferSize_ = -1;
    enableCache_ = false;
    parameter->Get<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(maxInputBufferSize_);
    if (maxInputBufferSize_ > 0) {
        if (maxInputBufferSize_ > MAX_INPUT_BUFFER_SIZE_LIMIT) {
            MEDIA_LOG_W("maxInputBufferSize exceeds limit, clamped to %{public}d", MAX_INPUT_BUFFER_SIZE_LIMIT);
            maxInputBufferSize_ = MAX_INPUT_BUFFER_SIZE_LIMIT;
        }
        enableCache_ = true;
        MEDIA_LOG_I("AudioOpusEncoderPlugin cache enable, maxInputBufferSize: %{public}d", maxInputBufferSize_);
    }
    audioParameter_ = *parameter;
    audioParameter_.Set<Tag::MIME_TYPE>(MimeType::AUDIO_OPUS);
    return Status::OK;
}

Status AudioOpusEncoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(paramMutex_);
    if (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    if (maxOutputSize_ <= 0 || maxOutputSize_ > OUTPUT_BUFFER_SIZE_DEFAULT) {
        maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    }
    // add codec parameter
    if (maxInputBufferSize_ > 0) {
        audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputBufferSize_);
    } else {
        audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    }
    audioParameter_.Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    audioParameter_.Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    if (sampleRate_ > 0) {
        audioParameter_.Set<Tag::AUDIO_SAMPLE_PER_FRAME>(sampleRate_ * 20 / 1000);  // 20ms 1000ms
    }

    *parameter = audioParameter_;
    MEDIA_LOG_I("Opus GetParameter success");
    return Status::OK;
}

Status AudioOpusEncoderPlugin::Prepare()
{
    {
        std::lock_guard<std::mutex> lock(paramMutex_);
        if (!pluginCodecPtr_) {
            MEDIA_LOG_E("AudioOpusEncoderPlugin Prepare dlopen or dlsym error. release");
            return Status::ERROR_UNKNOWN;
        }
        int32_t retplugin = 0;
        retplugin = pluginCodecPtr_->SetPluginParameter(CHANNEL, channels_);
        retplugin += pluginCodecPtr_->SetPluginParameter(SAMPLE_RATE, sampleRate_);
        retplugin += pluginCodecPtr_->SetPluginParameter(BIT_RATE, static_cast<int32_t>(bitrate_));
        retplugin += pluginCodecPtr_->SetPluginParameter(COMPLEXITY, COMPLEX_DEFAULT);
        retplugin += pluginCodecPtr_->Init();
        if (retplugin != 0) {
            MEDIA_LOG_E("Prepare fail for plugin Init");
            return Status::ERROR_UNKNOWN;
        }
        std::vector<uint8_t> codecConfig;
        if (!pluginCodecPtr_->SetCodecConfig(codecConfig)) {
            audioParameter_.Set<Tag::MEDIA_CODEC_CONFIG>(codecConfig);
            MEDIA_LOG_I("write opus codec config len:%{public}d", static_cast<int32_t>(codecConfig.size()));
        }
    }
    MEDIA_LOG_I("Opus Prepare success");
    return Status::OK;
}

Status AudioOpusEncoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    if (!pluginCodecPtr_) {
        MEDIA_LOG_E("AudioOpusEncoderPlugin Reset dlopen or dlsym error. release");
        return Status::ERROR_UNKNOWN;
    }
    isFirstInputPts_ = true;
    prevPts_ = 0;
    ptsMode_ = DEFAULT_ENCODE_PTS_MODE;
    pcmCache_.clear();
    maxInputBufferSize_ = -1;
    enableCache_ = false;
    pluginCodecPtr_->Reset();
    MEDIA_LOG_I("Opus Reset success");
    return Status::OK;
}

Status AudioOpusEncoderPlugin::Start()
{
    MEDIA_LOG_I("Opus Start");
    return Prepare();
}

Status AudioOpusEncoderPlugin::Stop()
{
    MEDIA_LOG_I("Opus Stop");
    std::lock_guard<std::mutex> lock(avMutex_);
    if (outBuffer_) {
        outBuffer_.reset();
    }
    return Status::OK;
}

Status AudioOpusEncoderPlugin::Release()
{
    MEDIA_LOG_I("Opus Release");
    std::lock_guard<std::mutex> lock(avMutex_);
    if (!pluginCodecPtr_) {
        MEDIA_LOG_E("AudioOpusEncoderPlugin Release dlopen or dlsym error. release");
        return Status::ERROR_UNKNOWN;
    }
    pluginCodecPtr_->Release();

    delete pluginCodecPtr_;
    pluginCodecPtr_ = nullptr;
    return Status::OK;
}

Status AudioOpusEncoderPlugin::Flush()
{
    MEDIA_LOG_I("Opus Flush");
    std::lock_guard<std::mutex> lock(avMutex_);
    if (!pluginCodecPtr_) {
        MEDIA_LOG_E("AudioOpusEncoderPlugin Reset Flush or dlsym error. release");
        return Status::ERROR_UNKNOWN;
    }
    isFirstInputPts_ = true;
    prevPts_ = 0;
    ptsMode_ = DEFAULT_ENCODE_PTS_MODE;
    pcmCache_.clear();
    pluginCodecPtr_->Flush();
    return Status::OK;
}

Status AudioOpusEncoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status AudioOpusEncoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status AudioOpusEncoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputAvBuffer)
{
    MEDIA_LOG_D("Opus QueueInputBuffer");
    encodeLen_ = 0;
    if (inputAvBuffer == nullptr) {
        return Status::ERROR_INVALID_DATA;
    }
    auto inputBuffer = inputAvBuffer->memory_;
    if (inputBuffer == nullptr) {
        return Status::ERROR_INVALID_DATA;
    }
    if (inputAvBuffer->flag_ & BUFFER_FLAG_EOS) {
        // EOS
        SafeCallInputBufferDone(dataCallback_, inputAvBuffer);
        return Status::OK;
    }
    if (inputBuffer->GetSize() == 0 && !(inputAvBuffer->flag_ & BUFFER_FLAG_EOS)) {
        return Status::ERROR_INVALID_DATA;
    }
    if (!enableCache_ && inputBuffer->GetSize() !=
        static_cast<int32_t>(channels_ * sampleRate_ * PACKET_MS * sizeof(int16_t))) {
        return Status::ERROR_INVALID_DATA;
    }
    Status ret = Status::OK;

    std::lock_guard<std::mutex> lock(avMutex_);
    if (!pluginCodecPtr_) {
        return Status::ERROR_UNKNOWN;
    }
    if (ptsMode_ == FIRST_INPUT_START_ENCODE_PTS_MODE && isFirstInputPts_) {
        prevPts_ = inputAvBuffer->pts_;
        isFirstInputPts_ = false;
    }
    if (!enableCache_) {
        pts_ = inputAvBuffer->pts_;
        uint8_t *srcBuffer = inputBuffer->GetAddr();
        if (srcBuffer == nullptr) {
            return Status::ERROR_INVALID_DATA;
        }
        if (isEnableFormatConvert_ && srcFmt_ != AudioSampleFormat::SAMPLE_S16LE) {
            int32_t inputSize = inputBuffer->GetSize();
            int32_t targetSize = inputSize * GetPcmBytesPerSample(AudioSampleFormat::SAMPLE_S16LE) /
                GetPcmBytesPerSample(srcFmt_);
            convertBuf_.resize(targetSize);
            size_t sampleCount = static_cast<size_t>(inputSize) / static_cast<size_t>(GetPcmBytesPerSample(srcFmt_));
            ConvertPcmSampleFormat(
                srcBuffer, sampleCount, srcFmt_, AudioSampleFormat::SAMPLE_S16LE, convertBuf_.data());
            fbytes_ = convertBuf_.data();
        } else {
            fbytes_ = srcBuffer;
        }
        int32_t retplugin = pluginCodecPtr_->ProcessSendData(fbytes_, &encodeLen_);
        if (retplugin != 0 || encodeLen_ <= 0) {
            return Status::ERROR_INVALID_DATA;
        }
    } else {
        uint8_t *srcBuffer = inputBuffer->GetAddr();
        if (srcBuffer == nullptr) {
            return Status::ERROR_INVALID_DATA;
        }
        int32_t inputSize = inputBuffer->GetSize();
        if (isEnableFormatConvert_ && srcFmt_ != AudioSampleFormat::SAMPLE_S16LE) {
            int32_t targetSize = inputSize * GetPcmBytesPerSample(AudioSampleFormat::SAMPLE_S16LE) /
                GetPcmBytesPerSample(srcFmt_);
            convertBuf_.resize(targetSize);
            size_t sampleCount = static_cast<size_t>(inputSize) / static_cast<size_t>(GetPcmBytesPerSample(srcFmt_));
            ConvertPcmSampleFormat(
                srcBuffer, sampleCount, srcFmt_, AudioSampleFormat::SAMPLE_S16LE, convertBuf_.data());
            pcmCache_.insert(pcmCache_.end(), convertBuf_.data(), convertBuf_.data() + targetSize);
        } else {
            pcmCache_.insert(pcmCache_.end(), srcBuffer, srcBuffer + inputSize);
        }

        int32_t frameSize = static_cast<int32_t>(channels_ * sampleRate_ * PACKET_MS * sizeof(int16_t));
        if (static_cast<int32_t>(pcmCache_.size()) < frameSize) {
            MEDIA_LOG_D("pcmCache size %{public}zu less than frameSize %{public}d", pcmCache_.size(), frameSize);
            SafeCallInputBufferDone(dataCallback_, inputAvBuffer);
            return Status::OK;
        }

        pts_ = inputAvBuffer->pts_;
        fbytes_ = pcmCache_.data();
        int32_t retplugin = 0;
        retplugin = pluginCodecPtr_->ProcessSendData(fbytes_, &encodeLen_);
        pcmCache_.erase(pcmCache_.begin(), pcmCache_.begin() + frameSize);
        if (retplugin != 0 || encodeLen_ <= 0) {
            SafeCallInputBufferDone(dataCallback_, inputAvBuffer);
            return Status::ERROR_INVALID_DATA;
        }
    }
    if (ret == Status::OK || ret == Status::END_OF_STREAM) {
        std::lock_guard<std::mutex> l(bufferMetaMutex_);
        if (inputAvBuffer->meta_ == nullptr) {
            return Status::ERROR_INVALID_DATA;
        }
        bufferMeta_ = inputAvBuffer->meta_;
    }
    SafeCallInputBufferDone(dataCallback_, inputAvBuffer);

    MEDIA_LOG_D("Opus QueueInputBuffer end");
    return Status::OK;
}

Status AudioOpusEncoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    MEDIA_LOG_D("Opus QueueOutputBuffer in");
    if (!outputBuffer) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    std::lock_guard<std::mutex> lock(avMutex_);
    auto memory = outputBuffer->memory_;
    if (!pluginCodecPtr_) {
        return Status::ERROR_WRONG_STATE;
    }
    // 历史版本默认行为，输出pts等于上次输入的pts
    outputBuffer->pts_ = pts_;
    if (encodeLen_ > 0) {
        pluginCodecPtr_->ProcessRecieveData(&codeData_, encodeLen_);
        int32_t lenout = memory->Write(codeData_, encodeLen_, 0);
        if (lenout != encodeLen_) {
            return Status::ERROR_WRONG_STATE;
        }
        {
            std::lock_guard<std::mutex> l(bufferMetaMutex_);
            outputBuffer->meta_ = bufferMeta_;
        }
        int64_t frameDuration = 20000; // 20000: opus编码固定一帧时长
        if (ptsMode_ != DEFAULT_ENCODE_PTS_MODE) {
            outputBuffer->pts_ = prevPts_;
            outputBuffer->duration_ = frameDuration;
            if (prevPts_ < 0) {
                prevPts_ += frameDuration;
            } else {
                prevPts_ = ((INT64_MAX - prevPts_) < frameDuration)
                           ? (frameDuration - (INT64_MAX - prevPts_))
                           : (prevPts_ + frameDuration);
            }
        }
        int32_t frameSize = static_cast<int32_t>(channels_ * sampleRate_ * PACKET_MS * sizeof(int16_t));
        if (enableCache_ && static_cast<int32_t>(pcmCache_.size()) >= frameSize) {
            fbytes_ = pcmCache_.data();
            int32_t retplugin = pluginCodecPtr_->ProcessSendData(fbytes_, &encodeLen_);
            if (retplugin != 0 || encodeLen_ <= 0) {
                SafeCallOutputBufferDone(dataCallback_, outputBuffer);
                return Status::ERROR_INVALID_DATA;
            }
            pcmCache_.erase(pcmCache_.begin(), pcmCache_.begin() + frameSize);
            SafeCallOutputBufferDone(dataCallback_, outputBuffer);
            return Status::ERROR_AGAIN;
        }
        SafeCallOutputBufferDone(dataCallback_, outputBuffer);
    } else if (outputBuffer->flag_ & BUFFER_FLAG_EOS) {
        memory->SetSize(0);
        SafeCallOutputBufferDone(dataCallback_, outputBuffer);
        return Status::END_OF_STREAM;
    } else {
        return Status::ERROR_NOT_ENOUGH_DATA;
    }

    MEDIA_LOG_D("Opus QueueOutputBuffer end");
    return Status::OK;
}
}  // namespace Opus
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS
