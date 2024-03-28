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

#include "audio_g711mu_decoder_plugin.h"
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

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioG711MuDecoderPlugin"};
constexpr int SUPPORT_CHANNELS = 1;
constexpr int SUPPORT_SAMPLE_RATE = 8000;
constexpr int INPUT_BUFFER_SIZE_DEFAULT = 640; // 20ms:160
constexpr int OUTPUT_BUFFER_SIZE_DEFAULT = 1280; // 20ms:320
constexpr int AUDIO_G711MU_SIGN_BIT = 0x80;
constexpr int AVCODEC_G711MU_QUANT_MASK = 0xf;
constexpr int AVCODEC_G711MU_SHIFT = 4;
constexpr int AVCODEC_G711MU_SEG_MASK = 0x70;
constexpr int G711MU_LINEAR_BIAS = 0x84;


Status RegisterAudioDecoderPlugins(const std::shared_ptr<Register>& reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_G711MU_NAME);
    definition.pluginType = PluginType::AUDIO_DECODER;
    definition.rank = 100;  // 100
    definition.SetCreator([](const std::string& name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioG711muDecoderPlugin>(name);
    });

    Capability cap;
    cap.SetMime(MimeType::AUDIO_G711MU);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    definition.AddInCaps(cap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(definition) != Status::OK) {
        AVCODEC_LOGE("AudioG711muDecoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

void UnRegisterAudioDecoderPlugin() {}

PLUGIN_DEFINITION(G711muAudioDecoder, LicenseType::APACHE_V2, RegisterAudioDecoderPlugins,
    UnRegisterAudioDecoderPlugin);
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace G711mu {
AudioG711muDecoderPlugin::AudioG711muDecoderPlugin(const std::string& name)
    : CodecPlugin(std::move(name)),
      channels_(SUPPORT_CHANNELS),
      sampleRate_(SUPPORT_SAMPLE_RATE),
      maxInputSize_(INPUT_BUFFER_SIZE_DEFAULT),
      maxOutputSize_(OUTPUT_BUFFER_SIZE_DEFAULT)
{
}

AudioG711muDecoderPlugin::~AudioG711muDecoderPlugin()
{
}

bool AudioG711muDecoderPlugin::CheckFormat()
{
    if (channels_ != SUPPORT_CHANNELS) {
        AVCODEC_LOGE("AudioG711muDecoderPlugin channels not supported");
        return false;
    }

    if (sampleRate_ != SUPPORT_SAMPLE_RATE) {
        AVCODEC_LOGE("AudioG711muDecoderPlugin sampleRate not supported");
        return false;
    }
    return true;
}

Status AudioG711muDecoderPlugin::Init()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    decodeResult_.reserve(OUTPUT_BUFFER_SIZE_DEFAULT);
    return Status::OK;
}

Status AudioG711muDecoderPlugin::Start()
{
    if (!CheckFormat()) {
        AVCODEC_LOGE("Format check failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    return Status::OK;
}

int16_t AudioG711muDecoderPlugin::G711MuLawDecode(uint8_t muLawValue)
{
    uint16_t tmp;
    muLawValue = ~muLawValue;

    tmp = ((muLawValue & AVCODEC_G711MU_QUANT_MASK) << 3) + G711MU_LINEAR_BIAS;  // left shift 3 bits
    tmp <<= (static_cast<unsigned>(muLawValue) & AVCODEC_G711MU_SEG_MASK) >> AVCODEC_G711MU_SHIFT;

    return ((muLawValue & AUDIO_G711MU_SIGN_BIT) ? (G711MU_LINEAR_BIAS - tmp) : (tmp - G711MU_LINEAR_BIAS));
}

Status AudioG711muDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    auto memory = inputBuffer->memory_;
    if (memory->GetSize() < 0) {
        AVCODEC_LOGE("SendBuffer buffer size < 0. size : %{public}d", memory->GetSize());
        return Status::ERROR_UNKNOWN;
    }
    if (memory->GetSize() > memory->GetCapacity()) {
        AVCODEC_LOGE("send input buffer > allocate size. size : %{public}d, allocate size : %{public}d",
            memory->GetSize(), memory->GetCapacity());
        return Status::ERROR_UNKNOWN;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        int32_t decodeNum = memory->GetSize() / sizeof(uint8_t);
        uint8_t *muValueToDecode = reinterpret_cast<uint8_t *>(memory->GetAddr());
        decodeResult_.clear();
        for (int32_t i = 0; i < decodeNum ; ++i) {
            decodeResult_.push_back(G711MuLawDecode(muValueToDecode[i]));
        }

        dataCallback_->OnInputBufferDone(inputBuffer);
    }
    return Status::OK;
}

Status AudioG711muDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer>& outputBuffer)
{
    if (!outputBuffer) {
        AVCODEC_LOGE("AudioG711muDecoderPlugin Queue out buffer is null.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        auto memory = outputBuffer->memory_;
        auto outSize = sizeof(int16_t) * decodeResult_.size();

        memory->Write(reinterpret_cast<const uint8_t *>(decodeResult_.data()), outSize, 0);
        memory->SetSize(outSize);

        dataCallback_->OnOutputBufferDone(outputBuffer);
    }
    return Status::OK;
}

Status AudioG711muDecoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    audioParameter_.Clear();
    return Status::OK;
}

Status AudioG711muDecoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioG711muDecoderPlugin::Flush()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioG711muDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(avMutex_);
    Status ret = Status::OK;

    if (parameter->Find(Tag::AUDIO_CHANNEL_COUNT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_);
    } else {
        AVCODEC_LOGE("AudioG711muDecoderPlugin no AUDIO_CHANNEL_COUNT");
        ret = Status::ERROR_INVALID_PARAMETER;
    }

    if (parameter->Find(Tag::AUDIO_SAMPLE_RATE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    } else {
        AVCODEC_LOGE("AudioG711muDecoderPlugin no AUDIO_SAMPLE_RATE");
        ret = Status::ERROR_INVALID_PARAMETER;
    }

    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
        AVCODEC_LOGD("AudioG711muDecoderPlugin SetParameter maxInputSize_: %{public}d", maxInputSize_);
    }
    if (!CheckFormat()) {
        AVCODEC_LOGE("AudioG711muDecoderPlugin CheckFormat Failure");
        ret = Status::ERROR_INVALID_PARAMETER;
    }

    audioParameter_ = *parameter;
    return ret;
}

Status AudioG711muDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(avMutex_);
    if (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    AVCODEC_LOGD("AudioG711muDecoderPlugin GetParameter maxInputSize_: %{public}d", maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    *parameter = audioParameter_;
    return Status::OK;
}

Status AudioG711muDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status AudioG711muDecoderPlugin::Stop()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioG711muDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>>& inputBuffers)
{
    return Status::OK;
}

Status AudioG711muDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>>& outputBuffers)
{
    return Status::OK;
}

}  // namespace G711mu
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS