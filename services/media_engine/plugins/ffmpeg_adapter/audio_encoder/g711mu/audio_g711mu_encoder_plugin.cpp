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
#include "avcodec_dfx.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"


namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace G711mu;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AvCodec-AudioG711MuEncoderPlugin"};
constexpr int32_t SUPPORT_CHANNELS = 1;
constexpr int SUPPORT_SAMPLE_RATE = 8000;
constexpr int INPUT_BUFFER_SIZE_DEFAULT = 1280;  // 20ms:320
constexpr int OUTPUT_BUFFER_SIZE_DEFAULT = 640;  // 20ms:160

constexpr int AVCODEC_G711MU_LINEAR_BIAS = 0x84;
constexpr int AVCODEC_G711MU_SEG_NUM = 8;
constexpr int AVCODEC_G711MU_CLIP = 8159;
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
PLUGIN_DEFINITION(G711muAudioEncoder, LicenseType::LGPL, RegisterAudioEncoderPlugins, UnRegisterAudioEncoderPlugin);
}  // namespace


namespace OHOS {
namespace Media {
namespace Plugins {
namespace G711mu {
AudioG711muEncoderPlugin::AudioG711muEncoderPlugin(std::string name): CodecPlugin(std::move(name))
{
}

AudioG711muEncoderPlugin::~AudioG711muEncoderPlugin()
{
    Release();
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
    int16_t mask;
    int16_t seg;
    uint8_t muLawValue;

    pcmValue = pcmValue >> 2; // right shift 2 bits
    if (pcmValue < 0) {
        pcmValue = -pcmValue;
        mask = 0x7F;
    } else {
        mask = 0xFF;
    }
    if (pcmValue > AVCODEC_G711MU_CLIP) {
        pcmValue = AVCODEC_G711MU_CLIP;
    }
    pcmValue += (AVCODEC_G711MU_LINEAR_BIAS >> 2); // right shift 2 bits

    for (int16_t i = 0; i < AVCODEC_G711MU_SEG_NUM; i++) {
        if (pcmValue <= AVCODEC_G711MU_SEG_END[i]) {
            seg = i;
            break;
        }
    }
    if (pcmValue > AVCODEC_G711MU_SEG_END[7]) { // last index 7
        seg = 8;                                 // last segment index 8
    }

    if (seg >= 8) {                             // last segment index 8
        return (uint8_t)(0x7F ^ mask);
    } else {
        muLawValue = (uint8_t)(seg << 4) | ((pcmValue >> (seg + 1)) & 0xF); // left shift 4 bits
        return (muLawValue ^ mask);
    }
}

Status AudioG711muEncoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer>& inputAvBuffer)
{
    auto memory = inputAvBuffer->memory_;
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
        int32_t tmp = memory->GetSize() % sizeof(int16_t);
        int16_t* pcmToEncode = (int16_t*)memory->GetAddr();
        uint8_t encodeValue;
        encodeResult_.clear();
        int32_t i;
        for (i = 0; i < sampleNum; ++i) {
            encodeValue = G711MuLawEncode(pcmToEncode[i]);
            encodeResult_.push_back(encodeValue);
        }
        if (tmp == 1 && i == sampleNum) {
            AVCODEC_LOGE("AudioG711muEncoderPlugin inputBuffer size in bytes is odd and the last byte is ignored");
            return Status::ERROR_INVALID_DATA;
        }
        dataCallback_->OnInputBufferDone(inputAvBuffer);
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
        outBuffer_ = outputBuffer;
        dataCallback_->OnOutputBufferDone(outputBuffer);
    }
    return Status::OK;
}

Status AudioG711muEncoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    if (outBuffer_) {
        outBuffer_.reset();
    }
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
    std::lock_guard<std::mutex> lock(parameterMutex_);
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
        AVCODEC_LOGE("SetParameter maxInputSize_: %{public}d", maxInputSize_);
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
    std::lock_guard<std::mutex> lock(parameterMutex_);
    if (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    AVCODEC_LOGE("AudioG711muEncoderPlugin GetParameter maxInputSize_: %{public}d", maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    audioParameter_.Set<Tag::AUDIO_CHANNEL_COUNT>(channels_);
    audioParameter_.Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
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
    if (outBuffer_) {
        outBuffer_.reset();
    }
    return Status::OK;
}

Status AudioG711muEncoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>>& inputBuffers)
{
    return Status::OK;
}

Status AudioG711muEncoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>>& inputBuffers)
{
    return Status::OK;
}

}  // namespace G711mu
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS