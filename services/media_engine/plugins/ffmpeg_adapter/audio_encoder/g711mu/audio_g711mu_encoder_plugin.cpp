/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "audio_g711mu_encoder_plugin.h"
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"


namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace G711mu;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioG711MuEncoderPlugin"};
constexpr int32_t SUPPORT_CHANNELS = 1;
constexpr int SUPPORT_SAMPLE_RATE = 8000;
constexpr int INPUT_BUFFER_SIZE_DEFAULT = 1280;  // 20ms:320
constexpr int OUTPUT_BUFFER_SIZE_DEFAULT = 640;  // 20ms:160

constexpr int AVCODEC_G711MU_LINEAR_BIAS = 0x84;
constexpr int AVCODEC_G711MU_CLIP = 8159;
constexpr uint16_t AVCODEC_G711MU_SEG_NUM = 8;
static const short AVCODEC_G711MU_SEG_END[8] = {
    0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF};


Status RegisterAudioEncoderPlugins(const std::shared_ptr<Register>& reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME);
    definition.pluginType = PluginType::AUDIO_ENCODER;
    definition.rank = 100;  // 100
    definition.SetCreator([](const std::string& name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioG711muEncoderPlugin>(name);
    });

    Capability cap;

    cap.SetMime(MimeType::AUDIO_G711MU);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    definition.AddInCaps(cap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(definition) != Status::OK) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

void UnRegisterAudioEncoderPlugin() {}

PLUGIN_DEFINITION(G711muAudioEncoder, LicenseType::APACHE_V2, RegisterAudioEncoderPlugins,
    UnRegisterAudioEncoderPlugin);
}  // namespace


namespace OHOS {
namespace Media {
namespace Plugins {
namespace G711mu {
AudioG711muEncoderPlugin::AudioG711muEncoderPlugin(const std::string& name)
    : CodecPlugin(std::move(name)),
      audioSampleFormat_(AudioSampleFormat::SAMPLE_S16LE),
      channels_(SUPPORT_CHANNELS),
      sampleRate_(SUPPORT_SAMPLE_RATE),
      maxInputSize_(INPUT_BUFFER_SIZE_DEFAULT),
      maxOutputSize_(OUTPUT_BUFFER_SIZE_DEFAULT)
{
}

AudioG711muEncoderPlugin::~AudioG711muEncoderPlugin()
{
}

bool AudioG711muEncoderPlugin::CheckFormat()
{
    if (channels_ != SUPPORT_CHANNELS) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin channels not supported");
        return false;
    }

    if (sampleRate_ != SUPPORT_SAMPLE_RATE) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin sampleRate not supported");
        return false;
    }

    if (audioSampleFormat_ != AudioSampleFormat::SAMPLE_S16LE) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin sampleFmt not supported");
        return false;
    }
    return true;
}

Status AudioG711muEncoderPlugin::Init()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    encodeResult_.reserve(OUTPUT_BUFFER_SIZE_DEFAULT);
    return Status::OK;
}

Status AudioG711muEncoderPlugin::Start()
{
    if (!CheckFormat()) {
        AVCODEC_LOGE("Format check failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    return Status::OK;
}


uint8_t AudioG711muEncoderPlugin::G711MuLawEncode(int16_t pcmValue)
{
    uint16_t mask;
    uint16_t seg;

    if (pcmValue < 0) {
        pcmValue = -pcmValue;
        mask = 0x7F;
    } else {
        mask = 0xFF;
    }
    uint16_t pcmShort = static_cast<uint16_t>(pcmValue);
    pcmShort = pcmShort >> 2; // right shift 2 bits
    if (pcmShort > AVCODEC_G711MU_CLIP) {
        pcmShort = AVCODEC_G711MU_CLIP;
    }
    pcmShort += (AVCODEC_G711MU_LINEAR_BIAS >> 2); // right shift 2 bits

    for (uint16_t i = 0; i < AVCODEC_G711MU_SEG_NUM; i++) {
        if (pcmShort <= AVCODEC_G711MU_SEG_END[i]) {
            seg = i;
            break;
        }
    }
    if (pcmShort > AVCODEC_G711MU_SEG_END[7]) { // last index 7
        seg = 8;                                 // last segment index 8
    }

    if (seg >= 8) {                             // last segment index 8
        return static_cast<uint8_t>(0x7F ^ mask);
    } else {
        uint8_t muLawValue = static_cast<uint8_t>(seg << 4) | ((pcmShort >> (seg + 1)) & 0xF); // left shift 4 bits
        return (muLawValue ^ mask);
    }
}

Status AudioG711muEncoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    auto memory = inputBuffer->memory_;
    if (memory->GetSize() < 0) {
        AVCODEC_LOGE("SendBuffer buffer size is less than 0. size : %{public}d", memory->GetSize());
        return Status::ERROR_UNKNOWN;
    }
    if (memory->GetSize() > memory->GetCapacity()) {
        AVCODEC_LOGE("send input buffer is > allocate size. size : %{public}d, allocate size : %{public}d",
                     memory->GetSize(), memory->GetCapacity());
        return Status::ERROR_UNKNOWN;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        int32_t sampleNum = memory->GetSize() / sizeof(int16_t);
        int16_t* pcmToEncode = reinterpret_cast<int16_t*>(memory->GetAddr());
        encodeResult_.clear();
        for (int32_t i = 0; i < sampleNum; ++i) {
            encodeResult_.push_back(G711MuLawEncode(pcmToEncode[i]));
        }
        if (memory->GetSize() % sizeof(int16_t) == 1) {
            AVCODEC_LOGE("AudioG711muEncoderPlugin inputBuffer size in bytes is odd and the last byte is ignored");
        }
        dataCallback_->OnInputBufferDone(inputBuffer);
    }
    return Status::OK;
}

Status AudioG711muEncoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer>& outputBuffer)
{
    if (!outputBuffer) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin Queue out buffer is null.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        auto memory = outputBuffer->memory_;
        auto outSize = sizeof(uint8_t) * encodeResult_.size();
        memory->Write(reinterpret_cast<const uint8_t *>(encodeResult_.data()), outSize, 0);
        memory->SetSize(outSize);
        dataCallback_->OnOutputBufferDone(outputBuffer);
    }
    return Status::OK;
}

Status AudioG711muEncoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioG711muEncoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioG711muEncoderPlugin::Flush()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioG711muEncoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(avMutex_);
    Status ret = Status::OK;

    if (parameter->Find(Tag::AUDIO_CHANNEL_COUNT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_);
    } else {
        AVCODEC_LOGE("no AUDIO_CHANNEL_COUNT");
        ret = Status::ERROR_INVALID_PARAMETER;
    }

    if (parameter->Find(Tag::AUDIO_SAMPLE_RATE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    } else {
        AVCODEC_LOGE("no AUDIO_SAMPLE_RATE");
        ret = Status::ERROR_INVALID_PARAMETER;
    }

    if (parameter->Find(Tag::AUDIO_SAMPLE_FORMAT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_FORMAT>(audioSampleFormat_);
    } else {
        AVCODEC_LOGE("no AUDIO_SAMPLE_FORMAT");
        ret = Status::ERROR_INVALID_PARAMETER;
    }

    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
        AVCODEC_LOGD("SetParameter maxInputSize_: %{public}d", maxInputSize_);
    }

    if (!CheckFormat()) {
        AVCODEC_LOGE("CheckFormat fail");
        ret = Status::ERROR_INVALID_PARAMETER;
    }

    audioParameter_ = *parameter;
    return ret;
}

Status AudioG711muEncoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(avMutex_);
    if (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    AVCODEC_LOGD("AudioG711muEncoderPlugin GetParameter maxInputSize_: %{public}d", maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    *parameter = audioParameter_;
    return Status::OK;
}

Status AudioG711muEncoderPlugin::Prepare()
{
    return Status::OK;
}

Status AudioG711muEncoderPlugin::Stop()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioG711muEncoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>>& inputBuffers)
{
    return Status::OK;
}

Status AudioG711muEncoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>>& outputBuffers)
{
    return Status::OK;
}

}  // namespace G711mu
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS