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
#include "audio_opus_decoder_plugin.h"
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include "osal/utils/util.h"
#include "common/log.h"
#include "meta/mime_type.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Opus;

constexpr std::string_view OPUS_DECODE_NAME = "OH.Media.Codec.Decoder.Audio.Opus";
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 1500;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 255;
constexpr float TIME_ONE_SECOND = 1000000.f;
static const int32_t OPUS_DECODER_SAMPLE_RATE_TABLE[] = {8000, 12000, 16000, 24000, 48000};
constexpr int32_t OPUS_HEAD_SIZE = 19;
constexpr int32_t TWO_CHANNELS = 2;
constexpr int32_t OFFSET_2 = 2;
constexpr int32_t TIME_MS_MAX = 60;  // capability slove 60ms frame
constexpr int32_t TIME_S_MS = 1000;
const uint8_t REMAPPING_CHANNEL_LAYOUT[8][8] = {
    { 0 },
    { 0, 1 },
    { 0, 2, 1 },
    { 0, 1, 2, 3 },
    { 0, 2, 1, 3, 4 },
    { 0, 2, 1, 5, 3, 4 },
    { 0, 2, 1, 6, 5, 3, 4 },
    { 0, 2, 1, 7, 5, 6, 3, 4 },
};

Status RegisterAudioDecoderPlugin(const std::shared_ptr<Register> &reg) __attribute__((no_sanitize("cfi")))
{
    CodecPluginDef definition;
    definition.name = OPUS_DECODE_NAME;
    definition.pluginType = PluginType::AUDIO_DECODER;
    definition.rank = 100;  // 100
    definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioOpusDecoderPlugin>(name);
    });

    Capability cap;

    cap.SetMime(MimeType::AUDIO_OPUS);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    definition.AddInCaps(cap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(definition) != Status::OK) {
        MEDIA_LOG_E("AudioOpusDecoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

void UnRegisterAudioDecoderPlugins()
{}

PLUGIN_DEFINITION(OpusAudioDecoder, LicenseType::UNKNOWN, RegisterAudioDecoderPlugin, UnRegisterAudioDecoderPlugins);
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Opus {
AudioOpusDecoderPlugin::AudioOpusDecoderPlugin(std::string name)
    : CodecPlugin(std::move(name)), pluginCodecPtr_(nullptr), fbytes_(nullptr), decodeLen_(0), codeData_(nullptr),
      maxInputSize_(INPUT_BUFFER_SIZE_DEFAULT), maxOutputSize_(OUTPUT_BUFFER_SIZE_DEFAULT), channels_(0),
      sampleRate_(0), pts_(0), sampleFormat_(AudioSampleFormat::INVALID_WIDTH), isMultistream_(false), streams_(0),
      coupled_(0)
{}

AudioOpusDecoderPlugin::~AudioOpusDecoderPlugin()
{}

Status AudioOpusDecoderPlugin::Init()
{
    MEDIA_LOG_I("Create Opus");
    OpusPluginClassDecoderCreate(&pluginCodecPtr_);
    if (!pluginCodecPtr_) {
        MEDIA_LOG_E("AudioOpus DecoderPlugin Init fail, please check");
        return Status::ERROR_NULL_POINTER;
    }
    return Status::OK;
}

Status AudioOpusDecoderPlugin::SetDecoderConfig(const std::shared_ptr<Meta> &parameter)
{
    if (parameter->Find(Tag::AUDIO_CHANNEL_COUNT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_);
        if (channels_ < MIN_CHANNELS || channels_ > MAX_CHANNELS) {
            return Status::ERROR_INVALID_PARAMETER;
        }
    } else {
        MEDIA_LOG_E("SetParameter channel error");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (channels_ > TWO_CHANNELS) {
        isMultistream_ = true;
    }
    int32_t paraFlag = 0;
    if (parameter->Find(Tag::AUDIO_SAMPLE_RATE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
        for (auto i : OPUS_DECODER_SAMPLE_RATE_TABLE) {
            if (i == sampleRate_) {
                paraFlag = 1;
                break;
            }
        }
    }
    if (paraFlag == 0) {
        MEDIA_LOG_E("SetParameter samplerate error");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    } else {
        maxInputSize_ = isMultistream_ ? INPUT_BUFFER_SIZE_DEFAULT * channels_ :
            INPUT_BUFFER_SIZE_DEFAULT;
    }
    if (parameter->Find(Tag::AUDIO_MAX_OUTPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    } else {
        maxOutputSize_ = isMultistream_ ?
            static_cast<int32_t>((sampleRate_ * TIME_MS_MAX / TIME_S_MS) * channels_ * sizeof(int16_t)) :
            OUTPUT_BUFFER_SIZE_DEFAULT;
    }
    if (parameter->Find(Tag::AUDIO_SAMPLE_FORMAT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat_);
    } else {
        MEDIA_LOG_W("AudioOpusDecoderPlugin is not set AUDIO_SAMPLE_FORMAT");
    }
    return Status::OK;
}

Status AudioOpusDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(paramMutex_);
    Status ret = SetDecoderConfig(parameter);
    if (ret != Status::OK) {
        return ret;
    }
    if (isMultistream_) {
        maxOutputSize_ = static_cast<int32_t>((sampleRate_ * TIME_MS_MAX / TIME_S_MS) * channels_ * sizeof(short));
        if (parameter->Find(Tag::MEDIA_CODEC_CONFIG) == parameter->end()) {
            MEDIA_LOG_W("AudioOpusDecoderPlugin is not set MEDIA_CODEC_CONFIG");
            return Status::ERROR_INVALID_PARAMETER;
        }
        std::vector<uint8_t> extradata;
        parameter->Get<Tag::MEDIA_CODEC_CONFIG>(extradata);
        if (static_cast<int32_t>(extradata.size()) < OPUS_HEAD_SIZE + OFFSET_2 + channels_) {
            MEDIA_LOG_E("extradata size error");
            return Status::ERROR_INVALID_PARAMETER;
        }
        streams_ = extradata[OPUS_HEAD_SIZE + 0];
        coupled_ = extradata[OPUS_HEAD_SIZE + 1];
        mapping_.assign(extradata.begin() + OFFSET_2 + OPUS_HEAD_SIZE, extradata.end());
        MEDIA_LOG_I("Opus streams: %{public}d coupled: %{public}d mapping size: %{public}d",
            streams_, coupled_, static_cast<int32_t>(mapping_.size()));
        if (channels_ > 2 && channels_ <= 8) {  // 2 8
            const uint8_t *remapOffset = REMAPPING_CHANNEL_LAYOUT[channels_ - 1];
            int32_t ch;
            std::vector<uint8_t> mappingArr;
            // Remap channels
            for (ch = 0; ch < channels_; ch++) {
                mappingArr.emplace_back(mapping_[remapOffset[ch]]);
            }
            mapping_ = mappingArr;
        }
    }
    int32_t enableSkipSamples = 0;
    if (parameter->GetData(Tag::ENABLE_BUFFER_SKIP_SAMPLES, enableSkipSamples)) {
        isEnableSkipSamples_ = (enableSkipSamples == 1);
    }

    audioParameter_ = *parameter;
    MEDIA_LOG_I("enableSkipSamples: %{public}d, %{public}d .",
        enableSkipSamples, static_cast<int32_t>(isEnableSkipSamples_));
    return Status::OK;
}

Status AudioOpusDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(paramMutex_);
    if (!isMultistream_ && (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT)) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    if (!isMultistream_ && (maxOutputSize_ <= 0 || maxOutputSize_ > OUTPUT_BUFFER_SIZE_DEFAULT)) {
        maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    }
    // add codec parameter
    audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    audioParameter_.Set<Tag::AUDIO_CHANNEL_COUNT>(channels_);
    audioParameter_.Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);

    *parameter = audioParameter_;
    MEDIA_LOG_I("GetParameter success");
    return Status::OK;
}

Status AudioOpusDecoderPlugin::Prepare()
{
    MEDIA_LOG_I("Opus Prepare");
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        if (!pluginCodecPtr_) {
            MEDIA_LOG_E("AudioOpusEncoderPlugin Prepare dlopen or dlsym error. release");
            return Status::ERROR_UNKNOWN;
        }
        int32_t retplugin = 0;
        retplugin = pluginCodecPtr_->SetPluginParameter(CHANNEL, channels_);
        retplugin += pluginCodecPtr_->SetPluginParameter(SAMPLE_RATE, sampleRate_);
        if (isMultistream_) {
            retplugin = pluginCodecPtr_->SetPluginParameter(STREAMS, streams_);
            retplugin = pluginCodecPtr_->SetPluginParameter(COUPLED, coupled_);
            retplugin = pluginCodecPtr_->SetPluginParameter(MAPPING, mapping_);
        }
        retplugin += pluginCodecPtr_->Init();
        if (retplugin != 0) {
            MEDIA_LOG_E("Prepare fail for plugin Init");
            return Status::ERROR_UNKNOWN;
        }
    }
    MEDIA_LOG_I("Opus Prepare success channel: %{public}d samplerate: %{public}d", channels_, sampleRate_);
    return Status::OK;
}

Status AudioOpusDecoderPlugin::Reset()
{
    MEDIA_LOG_I("Opus Reset");
    std::lock_guard<std::mutex> lock(avMutex_);
    if (!pluginCodecPtr_) {
        MEDIA_LOG_E("AudioOpusDecoderPlugin Reset dlopen or dlsym error. release");
        return Status::ERROR_UNKNOWN;
    }
    pluginCodecPtr_->Reset();
    MEDIA_LOG_I("Opus Reset success");
    return Status::OK;
}

Status AudioOpusDecoderPlugin::Start()
{
    MEDIA_LOG_I("Opus start");
    return Prepare();
}

Status AudioOpusDecoderPlugin::Stop()
{
    MEDIA_LOG_I("Opus Stop");
    std::lock_guard<std::mutex> lock(avMutex_);
    if (outBuffer_) {
        outBuffer_.reset();
    }
    return Status::OK;
}

Status AudioOpusDecoderPlugin::Release()
{
    MEDIA_LOG_I("Opus Release");
    std::lock_guard<std::mutex> lock(avMutex_);
    if (!pluginCodecPtr_) {
        MEDIA_LOG_E("AudioOpusDecoderPlugin Release dlopen or dlsym error. release");
        return Status::ERROR_UNKNOWN;
    }
    pluginCodecPtr_->Release();

    delete pluginCodecPtr_;
    pluginCodecPtr_ = nullptr;
    return Status::OK;
}

Status AudioOpusDecoderPlugin::Flush()
{
    MEDIA_LOG_I("Opus Flush");
    std::lock_guard<std::mutex> lock(avMutex_);
    if (!pluginCodecPtr_) {
        return Status::ERROR_UNKNOWN;
    }
    pluginCodecPtr_->Flush();
    return Status::OK;
}

Status AudioOpusDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status AudioOpusDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status AudioOpusDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputAvBuffer)
{
    MEDIA_LOG_D("QueueInputBuffer in");
    if (inputAvBuffer == nullptr) {
        return Status::ERROR_INVALID_DATA;
    }
    auto inputBuffer = inputAvBuffer->memory_;
    if (inputBuffer == nullptr) {
        return Status::ERROR_INVALID_DATA;
    }
    if (inputBuffer->GetSize() == 0 && !(inputAvBuffer->flag_ & BUFFER_FLAG_EOS)) {
        return Status::ERROR_INVALID_DATA;
    }

    {
        std::lock_guard<std::mutex> lock(avMutex_);
        if (!pluginCodecPtr_) {
            return Status::ERROR_UNKNOWN;
        }
        pts_ = inputAvBuffer->pts_;
        uint8_t *srcBuffer = inputBuffer->GetAddr();
        if (srcBuffer == nullptr) {
            return Status::ERROR_INVALID_DATA;
        }
        fbytes_ = srcBuffer;
        int32_t retplugin = 0;
        decodeLen_ = inputBuffer->GetSize();
        retplugin = pluginCodecPtr_->ProcessSendData(fbytes_, &decodeLen_);
        if (retplugin != 0 || decodeLen_ <= 0) {
            MEDIA_LOG_E("QueueInputBuffer ERROR, retplugin: %{public}d", retplugin);
            return Status::ERROR_INVALID_DATA;
        }
        std::lock_guard<std::mutex> l(bufferMetaMutex_);
        if (inputAvBuffer->meta_ == nullptr) {
            return Status::ERROR_INVALID_DATA;
        }
        bufferMeta_ = inputAvBuffer->meta_;
        SafeCallInputBufferDone(dataCallback_, inputAvBuffer);
    }
    MEDIA_LOG_D("QueueInputBuffer success");
    return Status::OK;
}

Status AudioOpusDecoderPlugin::SkipSamplesProcess(
    std::vector<uint8_t> &skipInfo, std::shared_ptr<AVBuffer> &outputBuffer, uint32_t decSamples)
{
    uint8_t *outBuffer = codeData_;
    uint32_t outSamples = decSamples;
    uint32_t discardEndSamples = 0;
    auto &memory = outputBuffer->memory_;
    if (skipInfo.size() >= 10) {  // 10
        skipSamples_ = 0;
        for (int32_t i = 0; i < 4; i++) {  // 4 sizeof(uint32_t), 10 bytes
            skipSamples_ |= skipInfo[i] << static_cast<uint8_t>(i * 8);  // 8bit = 1byte
            discardEndSamples |= skipInfo[i + 4] << (i * 8);  // 8bit = 1byte  4: skipInfo[4]
        }
    }
    MEDIA_LOG_I("decSamples:%{public}u, skipSamples:%{public}u, discardEndSamples:%{public}u, size:%{public}zu",
        decSamples, skipSamples_, discardEndSamples, skipInfo.size());
    if (skipSamples_ >= decSamples) {
        // skip one frame
        skipSamples_ -= decSamples;
        if ((outputBuffer->flag_ & BUFFER_FLAG_EOS) != 0) {
            SafeCallOutputBufferDone(dataCallback_, outputBuffer);
            return Status::OK;
        }
        return Status::ERROR_NOT_ENOUGH_DATA;
    } else {
        // skip start samples
        outBuffer += skipSamples_ * static_cast<uint32_t>(channels_) * sizeof(short);
        outSamples -= skipSamples_;
        skipSamples_ = 0;
    }
    // skip end samples
    if (outSamples > discardEndSamples) {
        memory->Write(outBuffer, (outSamples - discardEndSamples) * channels_ * sizeof(int16_t), 0);
        {
            std::lock_guard<std::mutex> lock(bufferMetaMutex_);
            outputBuffer->meta_ = bufferMeta_;
        }
        SafeCallOutputBufferDone(dataCallback_, outputBuffer);
    } else if (outSamples < discardEndSamples) {
        memory->Write(outBuffer, outSamples * channels_ * sizeof(int16_t), 0);
        {
            std::lock_guard<std::mutex> lock(bufferMetaMutex_);
            outputBuffer->meta_ = bufferMeta_;
        }
        SafeCallOutputBufferDone(dataCallback_, outputBuffer);
    } else if ((outputBuffer->flag_ & BUFFER_FLAG_EOS) != 0) {
        SafeCallOutputBufferDone(dataCallback_, outputBuffer);
    } else {
        // outSamples == discardEndSamples, skip all frame
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    return Status::OK;
}

Status AudioOpusDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    MEDIA_LOG_D("QueueOutputBuffer in");
    if (!outputBuffer || sampleRate_ <= 0 || !bufferMeta_) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        auto memory = outputBuffer->memory_;
        if (!pluginCodecPtr_) {
            return Status::ERROR_WRONG_STATE;
        }
        outputBuffer->pts_ = pts_;
        float usPerSample = TIME_ONE_SECOND / sampleRate_;
        if (sampleFormat_ == SAMPLE_S16LE) {
            outputBuffer->duration_ = decodeLen_ * usPerSample;
        }
        bufferMeta_->SetData(Tag::AUDIO_SAMPLE_PER_FRAME, decodeLen_);
        // 获取skip info

        if (decodeLen_ > 0) {
            std::vector<uint8_t> skipInfo;
            if (bufferMeta_->GetData(Tag::BUFFER_SKIP_SAMPLES_INFO, skipInfo)) {
                bufferMeta_->Remove(Tag::BUFFER_SKIP_SAMPLES_INFO);
            }
            pluginCodecPtr_->ProcessRecieveData(&codeData_, decodeLen_);
            if (!isEnableSkipSamples_ || (skipInfo.size() < 10 && skipSamples_ == 0)) {  // 10 bytes
                memory->Write(codeData_, decodeLen_ * channels_ * sizeof(int16_t), 0);
                {
                    std::lock_guard<std::mutex> l(bufferMetaMutex_);
                    outputBuffer->meta_ = bufferMeta_;
                }
                SafeCallOutputBufferDone(dataCallback_, outputBuffer);
            } else {
                return SkipSamplesProcess(skipInfo, outputBuffer, static_cast<uint32_t>(decodeLen_));
            }
        } else {
            return Status::OK;
        }
    }
    MEDIA_LOG_D("QueueOutputBuffer success");
    return Status::OK;
}

}  // namespace Opus
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS